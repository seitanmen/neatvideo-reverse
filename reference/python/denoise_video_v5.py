#!/usr/bin/env python3
"""Denoise a video with the v5 reproduction (Agents.md §6).

Native YCbCr 4:2:0 in and out (no RGB round trip; the input colour tags are copied, which
removes the v4c colour shift), streaming decode, frames processed in parallel processes.
Per frame: Temporal (motion-compensated, per-pixel weights) -> Spatial (5-band a-trous
against the residual noise) -> Mix. No global deflicker unless --flicker is given.

  python3 denoise_video_v5.py IN OUT [--profile auto|spatial|temporal] [--area Y,X,SIZE|Y,X,H,W]
      [--radius 2] [--temporal A] [--threshold T] [--luma A] [--chroma A] [--spatial K]
      [--noise-level N] [--chroma-noise N] [--start S --frames M] [--jobs J] [--crf 16]
      [--flicker R] [--params P.json] [--audio copy|aac|none] [--depth 8|10|12|16|32|source]
      [--codec auto|x264|x265|ffv1|prores|exr]

Bit depth (nv_depth.py): --depth 8 (default) is the original 8-bit yuv420p path. 10/12/16 read
the source at that depth (yuv sources keep their chroma format, 4:2:2 is processed as 4:4:4; RGB
sources as gbrp), 32 as RGB float (gbrpf32le); "source" picks the source's own depth. Frames are
normalised to the core's 0..255 float scale and written back at the same depth and pix_fmt
(ffmpeg converts nothing on output). --codec auto: 8 bits libx264 (as before); yuv 10 bits
libx264 and 12 bits libx265 into .mp4/.mov/.m4v; .mkv FFV1 (lossless; needed for 16 bits and for
RGB; float needs FFV1 version 4, which ffmpeg 8 marks experimental); a float EXR sequence for a
pattern such as out_%05d.exr (no audio; the .json side file drops the %05d). --crf applies to
x264 / x265 only.

Audio: each audio stream is stream-copied when the output container holds it for common
players (MP4_AUDIO / MOV_AUDIO; e.g. MP2 into .mp4 is re-encoded to AAC), else re-encoded to
AAC; undecodable streams are dropped with a warning. --start/--frames cut the same time range;
A/V offsets are kept (MPEG-TS/PS rebasing compensated, anchored on the first decoded frame).
"""
import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
import time
from collections import deque
from concurrent.futures import ProcessPoolExecutor

import numpy as np

import nv_depth as nd
import nvfilter as nv


# demuxers flagged AVFMT_TS_DISCONT: ffmpeg rebases such an input on its earliest *used*
# stream ("Correcting start time of Input #1"), so an audio-only input starts at its audio
TS_DISCONT = {"mpegts", "mpegtsraw", "mpeg", "hls"}
# audio codecs that common players (AVFoundation included) play from .mp4 / .mov; others are
# re-encoded even when ffmpeg could mux them (MP2 in .mp4 is silently dropped by AVFoundation)
MP4_AUDIO = {"aac", "alac", "mp3", "ac3", "eac3", "pcm_s16le", "pcm_s24le", "pcm_s32le", "pcm_f32le"}
MOV_AUDIO = MP4_AUDIO | {"mp2", "pcm_s16be", "pcm_s24be", "pcm_s32be", "pcm_f32be", "pcm_u8"}


def probe(path):
    """Coded size, frame rate, approximate frame count (0 = unknown; only used for defaults and
    progress), pix_fmt (and bits_per_raw_sample when known), colour tags and display rotation
    (degrees counter-clockwise)."""
    out = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
                          "stream=width,height,r_frame_rate,nb_frames,pix_fmt,bits_per_raw_sample,color_space,color_primaries,"
                          "color_transfer,color_range,start_time:stream_side_data=rotation:format=duration,start_time",
                          "-of", "json", path], capture_output=True, text=True, check=True,
                         stdin=subprocess.DEVNULL).stdout
    j = json.loads(out)
    s = j["streams"][0]
    num, den = s["r_frame_rate"].split("/")
    n = int(s.get("nb_frames") or 0)
    if n <= 0:
        try:
            n = int(round(float(j.get("format", {}).get("duration") or 0) * float(num) / float(den)))
        except (TypeError, ValueError, ZeroDivisionError):
            n = 0
    rot = 0
    for sd in s.get("side_data_list") or []:
        if "rotation" in sd:
            rot = int(round(float(sd["rotation"]))) % 360
    tags = {k: s.get(k) for k in ("color_space", "color_primaries", "color_transfer", "color_range")}
    allj = json.loads(subprocess.run(["ffprobe", "-v", "error", "-show_entries", "format=start_time,format_name:"
                                      "stream=codec_type,codec_name,start_time", "-of", "json", path],
                                     capture_output=True, text=True, stdin=subprocess.DEVNULL).stdout or "{}")
    fmt = allj.get("format", {})
    # time of the first *decoded* frame (a stream cut mid-GOP starts later than its first packet);
    # read_frames' constant-rate timeline starts there. One frame is decoded (-copyts keeps the
    # source timestamps); same values as ffprobe's first best_effort_timestamp_time.
    first = subprocess.run(["ffmpeg", "-nostdin", "-hide_banner", "-copyts", "-noautorotate", "-i", path,
                            "-map", "0:v:0", "-frames:v", "1", "-vf", "showinfo", "-f", "null", "-"],
                           capture_output=True, text=True, stdin=subprocess.DEVNULL).stderr
    m = re.search(r"pts_time:([-0-9.e+]+)", first)
    fs = _num(fmt.get("start_time"), 0.0)
    v0 = _num(m.group(1) if m else None, None)
    if v0 is None:
        v0 = _num(s.get("start_time"), fs)
    audio = [{"codec": a.get("codec_name") or "unknown", "start": _num(a.get("start_time"), None)}
             for a in allj.get("streams", []) if a.get("codec_type") == "audio"]
    return {"W": int(s["width"]), "H": int(s["height"]), "fps": s["r_frame_rate"], "n": max(n, 0), "tags": tags,
            "pix_fmt": s.get("pix_fmt"), "bits_raw": int(_num(s.get("bits_per_raw_sample"), 0)),
            "full_range": tags.get("color_range") == "pc", "rotation": rot, "v0": v0, "fs": fs,
            "discont": bool(set(fmt.get("format_name", "").split(",")) & TS_DISCONT), "audio": audio}


def _num(v, default):
    try:
        return float(v)
    except (TypeError, ValueError):
        return default


def _fps(rate):
    num, den = (rate.split("/") + ["1"])[:2]
    return float(num) / float(den)


def read_frames(path, W, H, lo=0, hi=None, fps=None, fmt=None, matrix="bt709", vf=None, nonfinite=None):
    """Yield (index, (Y, Cb, Cr) uint8) for frames lo..hi-1 (hi=None: to the end).
    fmt (nv_depth.PixFmt; None = yuv420p uint8 as before): read as fmt.name and return
    nv_depth.to_internal(planes, fmt, matrix) (float32 on the 0..255 scale for high bit depths).
    vf: extra filter (nv_depth.read_filter) appended to the chain. Float frames are returned with
    finite values (nv_depth.sanitize); a dict `nonfinite` receives {index: restore info} for the
    frames that had NaN / +-inf (pass it to nv_depth.from_internal when writing).
    With `fps` (the stream's r_frame_rate) frames are numbered on a constant-rate timeline
    built before `select` (as an NLE conforms VFR material: gaps become repeated frames), so
    whole-clip and sub-range reads agree and the CFR output keeps the source duration.
    Frames stay in coded orientation (-noautorotate; the rotation tag is copied on encode)."""
    if hi is not None and hi <= lo:
        return
    # grid anchored on the first frame (= v0, the encoder's time 0); default eof_action keeps the
    # frame count of CFR sources (eof_action=pass duplicated the last frame of some mkv)
    chain = ["setpts=PTS-STARTPTS", f"fps={fps}"] if fps else []
    if lo > 0 or hi is not None:
        chain.append("select=" + (f"between(n\\,{lo}\\,{hi - 1})" if hi is not None else f"gte(n\\,{lo})"))
    if vf:
        chain.append(vf)
    sel = (["-vf", ",".join(chain)] if chain else []) + ([] if hi is None else ["-frames:v", str(hi - lo)])
    cw, ch = (W + 1) // 2, (H + 1) // 2
    fs = W * H + 2 * cw * ch if fmt is None else fmt.frame_bytes(W, H)
    p = subprocess.Popen(["ffmpeg", "-v", "error", "-nostdin", "-noautorotate", "-i", path, *sel,
                          "-fps_mode", "passthrough", "-f", "rawvideo", "-pix_fmt",
                          "yuv420p" if fmt is None else fmt.name, "-"],
                         stdout=subprocess.PIPE, stdin=subprocess.DEVNULL)
    i = lo
    try:
        while True:
            buf = p.stdout.read(fs)
            if len(buf) < fs:
                break
            if fmt is not None:
                raw = nd.split(buf, fmt, W, H)
                if fmt.is_float:
                    raw, info = nd.sanitize(raw)
                    if info is not None and nonfinite is not None:
                        nonfinite[i] = info
                yield i, nd.to_internal(raw, fmt, matrix)
                i += 1
                continue
            a = np.frombuffer(buf, np.uint8)
            yield i, (a[:W * H].reshape(H, W), a[W * H:W * H + cw * ch].reshape(ch, cw),
                      a[W * H + cw * ch:].reshape(ch, cw))
            i += 1
    finally:
        p.stdout.close()
        if p.poll() is None:
            p.kill()
        p.wait()


def audio_plan(src, dst, info, mode):
    """[(audio stream k of src, 'copy' | 'aac')]. Per stream: copy when dst's container holds it
    for common players (MP4_AUDIO / MOV_AUDIO; anything for other containers) and a 0.5 s trial
    mux works, else AAC when a trial re-encode works, else the stream is dropped with a warning."""
    if mode == "none" or not info["audio"]:
        return []
    ext = os.path.splitext(dst)[1].lower() or ".mp4"
    safe = MP4_AUDIO if ext in (".mp4", ".m4v") else MOV_AUDIO if ext == ".mov" else None
    plan = []
    with tempfile.TemporaryDirectory() as td:
        for k, a in enumerate(info["audio"]):
            tries = ["copy", "aac"] if mode == "copy" else ["aac"]
            if safe is not None and a["codec"] not in safe:
                tries = ["aac"]
            for c in tries:
                r = subprocess.run(["ffmpeg", "-v", "error", "-nostdin", "-i", src, "-map", f"0:a:{k}", "-c:a", c,
                                    *(["-b:a", "256k"] if c == "aac" else []), "-t", "0.5", "-y",
                                    os.path.join(td, f"trial{k}{ext}")], capture_output=True, stdin=subprocess.DEVNULL)
                if r.returncode == 0:
                    plan.append((k, c))
                    break
            else:
                print(f"warning: audio stream {k} ({a['codec']}) can be neither copied nor re-encoded - dropped",
                      flush=True)
    return plan


CODECS = ("auto", "x264", "x265", "ffv1", "prores", "exr")
_MP4 = (".mp4", ".mov", ".m4v")


def is_sequence(dst):
    """An image-sequence pattern such as out_%05d.exr."""
    return bool(re.search(r"%0?\d*d", os.path.basename(dst)))


def sidecar_stem(dst):
    """Path without extension for the .json side file; an image-sequence pattern loses its %05d
    (out_%05d.exr -> out)."""
    stem = os.path.splitext(dst)[0]
    if is_sequence(dst):
        d, b = os.path.split(stem)
        stem = os.path.join(d, re.sub(r"%0?\d*d", "", b).rstrip("._-") or "frames")
    return stem


def pick_codec(fmt, dst, codec="auto"):
    """Encoder ('x264' | 'x265' | 'ffv1' | 'prores' | 'exr') for processing format `fmt` and output
    path `dst` (module docstring); ValueError with a hint for impossible combinations."""
    ext = os.path.splitext(dst)[1].lower()
    exr = ext == ".exr" and is_sequence(dst)
    if codec == "auto":
        if fmt.legacy:
            return "x264"                        # as before, whatever the container
        if ext == ".mkv":
            return "ffv1"
        if fmt.family == "rgb":
            if fmt.is_float and exr:
                return "exr"
            raise ValueError(f"{fmt.name} output needs .mkv (FFV1)" +
                             (" or an EXR sequence pattern such as out_%05d.exr" if fmt.is_float else ""))
        if fmt.bits == 16:
            raise ValueError("16-bit output needs .mkv (FFV1, lossless)")
        if ext in _MP4 and fmt.bits == 10:
            return "x264"
        if ext in (".mp4", ".mov"):
            return "x265"
        if ext == ".m4v":
            raise ValueError("12-bit output: .m4v (iPod muxer) cannot hold HEVC; use .mp4 / .mov (x265) or .mkv (FFV1)")
        raise ValueError(f"{fmt.bits}-bit output: use .mp4 / .mov / .m4v (x264 10-bit, x265 12-bit) or .mkv (FFV1)")
    ok = {"x264": fmt.family == "yuv" and fmt.bits in (8, 10),
          "x265": fmt.family == "yuv" and fmt.bits in (8, 10, 12),
          "ffv1": True,
          "prores": fmt.name in ("yuv422p10le", "yuv444p10le"),
          "exr": fmt.name == "gbrpf32le"}[codec]
    if not ok:
        need = {"x264": "8 or 10-bit yuv (--depth 8 or 10 with a yuv source)",
                "x265": "8, 10 or 12-bit yuv",
                "prores": "10-bit yuv 4:2:2 or 4:4:4 (--depth 10 with a 4:2:2 / 4:4:4 source)",
                "exr": "32-bit float (--depth 32)"}[codec]
        raise ValueError(f"--codec {codec} cannot write {fmt.name}: it needs {need}")
    if codec == "exr" and not exr:
        raise ValueError("--codec exr needs an image-sequence pattern such as out_%05d.exr")
    return codec


def _vcodec(fmt, codec, crf):
    """-c:v arguments of the non-legacy encoders."""
    return {"x264": ["libx264", "-crf", str(crf)],
            "x265": ["libx265", "-crf", str(crf), "-x265-params", "log-level=error"],
            # FFV1 float needs version 4, which ffmpeg 8 marks experimental
            "ffv1": ["ffv1", "-level", "4", "-strict", "experimental"] if fmt.is_float else ["ffv1", "-level", "3"],
            "prores": ["prores_ks", "-profile:v", "4" if fmt.chroma == "444" else "3"],
            "exr": ["exr"]}[codec]


def trial_encode(dst, fmt, codec, crf):
    """Encode one 64x64 frame with the output's encoder, pix_fmt and container into a temporary
    directory, before any processing (catches containers that cannot hold the codec, e.g. HEVC in
    .m4v, ProRes in .mp4, FFV1 in .webm). Returns None or ffmpeg's last error line."""
    ext = os.path.splitext(dst)[1].lower()
    vc = ["libx264", "-crf", str(crf)] if fmt.legacy and codec == "x264" else _vcodec(fmt, codec, crf)
    with tempfile.TemporaryDirectory() as td:
        out = os.path.join(td, "trial_%05d.exr" if codec == "exr" else "trial" + ext)
        r = subprocess.run(["ffmpeg", "-v", "error", "-nostdin", "-f", "lavfi", "-i", "color=c=gray:s=64x64:r=24",
                            "-frames:v", "1", "-c:v", *vc, "-pix_fmt", fmt.name,
                            *(["-f", "image2"] if codec == "exr" else []), out],
                           capture_output=True, text=True, stdin=subprocess.DEVNULL)
    if r.returncode == 0:
        return None
    lines = [ln for ln in r.stderr.strip().splitlines() if ln.strip()]
    return lines[0] if lines else f"ffmpeg exit {r.returncode}"


def remove_output(dst):
    """Remove an incomplete output: the file, or the frames an image-sequence pattern numbers."""
    paths = [dst]
    if is_sequence(dst):
        d, b = os.path.split(os.path.abspath(dst))
        rx = re.compile(re.sub(r"%0?(\d*)d", lambda m: r"\d{%s,}" % (m.group(1) or 1), re.escape(b)) + "$")
        paths = [os.path.join(d, n) for n in (os.listdir(d) if os.path.isdir(d) else []) if rx.match(n)]
    for q in paths:
        try:
            os.remove(q)
        except OSError:
            pass


def encoder(dst, info, crf, plan=(), src=None, start=0.0, dur=None, fmt=None, codec="x264"):
    """libx264 encoder fed with raw yuv420p on stdin, plus the audio streams of `src` listed in
    `plan`. Output time 0 is the source time cut = v0 + start (v0 = first decoded frame);
    ffmpeg puts the audio input's time `base` at 0 (format start + seek, or its earliest audio
    start for TS/PS), so the audio input gets -itsoffset (base - cut). The raw video pipe
    itself ignores -itsoffset (measured: ffmpeg 8 left a 0.5 s offset at 0).
    fmt / codec (pick_codec): other formats are fed and encoded as fmt.name, so ffmpeg converts
    no pixels; fmt None or yuv420p with x264 is the original command."""
    legacy = fmt is None or (fmt.legacy and codec == "x264")
    # The tags go on the raw input too: tagging only the output makes ffmpeg convert the
    # (assumed BT.601) raw frames to BT.709 - the same kind of shift v4c had (Y -1.9).
    # RGB formats get no matrix / range tags (they could make ffmpeg convert the frames).
    keys = (("color_space", "-colorspace"), ("color_primaries", "-color_primaries"),
            ("color_transfer", "-color_trc"), ("color_range", "-color_range"))
    if not legacy and fmt.family == "rgb":
        keys = keys[1:3]
    tags = []
    for key, flag in keys:
        v = info["tags"].get(key)
        if v and v != "unknown":
            tags += [flag, v]
    # keep frames in coded orientation and pass the rotation on as metadata (without
    # -noautorotate ffmpeg would rotate the raw input itself and drop the tag)
    rot = ["-display_rotation", str(info["rotation"]), "-noautorotate"] if info.get("rotation") else []
    crop = []
    if legacy or (codec in ("x264", "x265", "prores") and fmt.chroma == "420"):
        if info["W"] % 2 or info["H"] % 2:           # 4:2:0 needs even sizes
            crop = ["-vf", "crop=trunc(iw/2)*2:trunc(ih/2)*2"]
            print("note: odd frame size - the last row/column is dropped on encode", flush=True)
    elif codec in ("x264", "x265", "prores") and fmt.chroma == "422" and info["W"] % 2:
        crop = ["-vf", "crop=trunc(iw/2)*2:ih"]      # 4:2:2 needs an even width only
        print("note: odd frame width - the last column is dropped on encode", flush=True)
    a_in, a_out = [], []
    if plan:
        cut = info["v0"] + start
        ranged = start > 0 or dur is not None
        ss = max(cut - info["fs"], 0.0) if ranged else 0.0
        base = info["fs"] + ss
        starts = [info["audio"][k]["start"] for k, _ in plan if info["audio"][k]["start"] is not None]
        if info["discont"] and starts:
            base = max(base, min(starts))
        off = base - cut
        if dur is not None and dur - max(off, 0.0) <= 0:
            plan = ()                                       # the range holds no audio
        else:
            a_in = (["-ss", f"{ss:.6f}"] if ranged else []) + \
                   (["-itsoffset", f"{off:.6f}"] if abs(off) > 1e-4 else []) + \
                   (["-t", f"{dur:.6f}"] if dur is not None else []) + ["-i", src]   # -t counts after -itsoffset
            for j, (k, c) in enumerate(plan):
                a_out += ["-map", f"1:a:{k}", f"-c:a:{j}", c] + ([f"-b:a:{j}", "256k"] if c == "aac" else [])
            if ranged and any(c == "copy" for _, c in plan):
                a_out += ["-copypriorss:a", "0"]            # no packets from before the cut
    if legacy:
        cmd = ["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "yuv420p",
               "-s", f"{info['W']}x{info['H']}", "-r", info["fps"], *tags, *rot, "-i", "-", *a_in, *crop,
               "-map", "0:v:0", "-c:v", "libx264", "-crf", str(crf), "-pix_fmt", "yuv420p", *tags, *a_out]
        return subprocess.Popen(cmd + ["-movflags", "+faststart", dst], stdin=subprocess.PIPE)
    vc = _vcodec(fmt, codec, crf)
    cmd = ["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", fmt.name,
           "-s", f"{info['W']}x{info['H']}", "-r", info["fps"], *tags, *rot, "-i", "-", *a_in, *crop,
           "-map", "0:v:0", "-c:v", *vc, "-pix_fmt", fmt.name, *tags, *a_out]
    if codec == "exr":
        cmd += ["-f", "image2"]
    elif os.path.splitext(dst)[1].lower() in _MP4:
        cmd += ["-movflags", "+faststart"]
    return subprocess.Popen(cmd + [dst], stdin=subprocess.PIPE)


_W = {}


def _init(prof, P):
    _W["prof"], _W["P"] = prof, P


def _work(job):
    idx, window, ci = job
    win = [tuple(p.astype(np.float32) for p in f) for f in window]
    out = nv.process_frame_v5(win, ci, _W["prof"], _W["P"])
    return idx, out


def build_params(A):
    kw = json.load(open(A.params)) if A.params else {}
    for key in ("radius", "temporal", "threshold", "spatial", "luma", "chroma", "noise_level", "mix", "bs", "search"):
        v = getattr(A, key)
        if v is not None:
            kw[key] = v
    if A.chroma_noise is not None:
        kw["channel_noise"] = {**kw.get("channel_noise", {}), "Cb": A.chroma_noise, "Cr": A.chroma_noise}
    return nv.v5_params(**kw)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("src")
    ap.add_argument("dst")
    ap.add_argument("--params", help="JSON file with v5 parameters (CLI flags override it)")
    ap.add_argument("--radius", type=int)
    ap.add_argument("--temporal", type=float, help="temporal Amount")
    ap.add_argument("--threshold", type=float, help="temporal similarity tolerance")
    ap.add_argument("--spatial", type=float, help="spatial Wiener strength")
    ap.add_argument("--luma", type=float, help="Noise Reduction Amount (luminance)")
    ap.add_argument("--chroma", type=float, help="Noise Reduction Amount (chrominance)")
    ap.add_argument("--noise-level", dest="noise_level", type=float, help="overall Noise Level vs profile")
    ap.add_argument("--chroma-noise", dest="chroma_noise", type=float, help="Cb/Cr Noise Level vs profile")
    ap.add_argument("--mix", type=float, help="Mix with original")
    ap.add_argument("--bs", type=int)
    ap.add_argument("--search", type=int)
    ap.add_argument("--profile", default="auto", choices=["auto", "spatial", "temporal"])
    ap.add_argument("--profile-frame", type=int, help="frame used for the noise profile (default N//3)")
    ap.add_argument("--area", help="profiling square Y,X,SIZE or rectangle Y,X,H,W on the luma grid "
                    "(default: FindFlatArea)")
    ap.add_argument("--start", type=int, default=0)
    ap.add_argument("--frames", type=int, help="number of output frames (default: to the end)")
    ap.add_argument("--flicker", type=int, default=0, help="whole-frame flicker reduction radius (0 = off)")
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) - 2))
    ap.add_argument("--crf", type=int, default=16)
    ap.add_argument("--audio", default="copy", choices=["copy", "aac", "none"],
                    help="audio streams: copy (AAC if the container cannot hold them), aac, none")
    ap.add_argument("--depth", default="8", choices=["8", "10", "12", "16", "32", "source"],
                    help="processing / output bit depth (8 = yuv420p as before; 32 = RGB float)")
    ap.add_argument("--codec", default="auto", choices=CODECS, help="video encoder (auto: see above)")
    A = ap.parse_args()
    try:
        area = tuple(int(v) for v in A.area.split(",")) if A.area else None
    except ValueError:
        area = ()
    if area is not None and len(area) not in (3, 4):
        ap.error("--area takes Y,X,SIZE or Y,X,H,W (integers)")
    t0 = time.time()
    os.makedirs(os.path.dirname(os.path.abspath(A.dst)), exist_ok=True)
    info = probe(A.src)
    W, H, N = info["W"], info["H"], info["n"]
    fmt = nd.choose_format(nd.source_format(info["pix_fmt"]), A.depth)
    try:
        codec = pick_codec(fmt, A.dst, A.codec)
    except ValueError as e:
        sys.exit(f"error: {e}")
    err = trial_encode(A.dst, fmt, codec, A.crf)     # before any processing
    if err is not None:
        sys.exit(f"error: {codec} / {fmt.name} cannot be written to {os.path.splitext(A.dst)[1] or A.dst} "
                 f"(trial encode: {err}); choose another --codec or container (.mkv takes them all)")
    matrix = nv.matrix_from_tags(info["tags"], H)
    full_range = nd.full_range_internal(info, fmt)
    # the 8-bit path reads exactly as before (fmt None: yuv420p uint8)
    rfmt, vf = (None, None) if fmt.legacy else (fmt, nd.read_filter(info, fmt, matrix))

    def frames(lo_, hi_=None):
        return read_frames(A.src, W, H, lo_, hi_, info["fps"], rfmt, matrix, vf)
    P = build_params(A)
    R = int(P["radius"])
    lo = max(0, A.start)
    hi = lo + A.frames if A.frames else None      # None: to the end of the stream (N may be wrong)
    total = A.frames or max(N - lo, 0)           # progress display only
    print(f"{W}x{H} {info['fps']} frames~{N or '?'} rotation={info['rotation']} "
          f"out=[{lo},{hi if hi is not None else 'end'}) tags={info['tags']}", flush=True)
    print(f"source {nd.describe_source(info['pix_fmt'])} -> processing {nd.describe(fmt)}, codec {codec}", flush=True)

    pf = A.profile_frame if A.profile_frame is not None else (lo + hi) // 2 if hi is not None else N // 3
    pf = max(min(pf, N - 3), 0) if N > 0 else max(pf, 0)
    pr = [tuple(p.astype(np.float32) for p in f) for _, f in frames(pf, pf + 3)]
    if not pr and pf > 0:                       # frame count over-reported (e.g. edit lists)
        print(f"note: profile frame {pf} not decodable, using the first frames", flush=True)
        pf = 0
        pr = [tuple(p.astype(np.float32) for p in f) for _, f in frames(0, 3)]
    if not pr:
        sys.exit("no decodable video frames")
    prof = nv.build_profile_v5(pr[0], full_range, area=area, pair=pr[1] if len(pr) > 1 else None,
                               mode=A.profile, more=pr[1:], matrix=matrix)
    st = prof.get("flat_status")
    print(f"profile frame {pf}: mode={prof['mode']} area={prof['area']} "
          f"({'manual' if st is None else nv.FLAT_STATUS.get(st, st)}) "
          f"sigma={ {k: round(v, 3) for k, v in prof['sigma'].items()} }", flush=True)
    if st in (1, 3, 4):     # the genuine Auto Profile warns in these cases (FUN_0099da28)
        print(f"warning: profiling square: {nv.FLAT_STATUS[st]} - consider --area Y,X,SIZE", flush=True)

    gains = None
    if A.flicker > 0:
        means = [float(f[0].mean()) for _, f in frames(0)]
        gains = nv.flicker_gains(means, A.flicker)

    if codec == "exr" and info["audio"] and A.audio != "none":
        print("note: an EXR sequence holds no audio - audio dropped", flush=True)
    plan = audio_plan(A.src, A.dst, info, "none" if codec == "exr" else A.audio)
    fr_s = _fps(info["fps"])
    enc = encoder(A.dst, info, A.crf, plan, A.src, lo / fr_s, (hi - lo) / fr_s if hi is not None else None,
                  fmt, codec)
    if plan:
        print("audio: " + ", ".join(f"{info['audio'][k]['codec']} -> {c}" for k, c in plan), flush=True)
    dq, pending, done = deque(maxlen=2 * R + 1), deque(), [0]

    black = 0.0 if full_range else 16.0

    def write(block):
        while pending and (block or pending[0].done() or len(pending) > 2 * A.jobs):
            idx, out = pending.popleft().result()
            if gains is not None and idx < len(gains):   # keep the black level fixed
                out[0] = (out[0] - black) * gains[idx] + black
            try:
                if fmt.legacy:
                    enc.stdin.write(b"".join(np.clip(np.rint(o), 0, 255).astype(np.uint8).tobytes() for o in out))
                else:
                    enc.stdin.write(nd.from_internal(out, fmt, matrix, nonfin.pop(idx, None)))
            except BrokenPipeError:                      # the encoder stopped (its error is above)
                for fu in pending:
                    fu.cancel()
                pending.clear()
                raise
            for k in [k for k in nonfin if k < idx]:     # context frames before the range
                del nonfin[k]
            done[0] += 1
            if done[0] % 24 == 0:
                print(f"frame {done[0]}/{max(total, done[0])} {time.time() - t0:.0f}s", flush=True)

    def submit(c):
        idx = [i for i, _ in dq if abs(i - c) <= R]
        pending.append(ex.submit(_work, (c, [f for i, f in dq if abs(i - c) <= R], idx.index(c))))

    def wanted(c):
        return c >= lo and (hi is None or c < hi)

    last = lo - 1
    nonfin = {}                                     # frame -> NaN / inf to put back (float input)
    broken = False
    try:
        with ProcessPoolExecutor(A.jobs, initializer=_init, initargs=(prof, P)) as ex:
            for i, f in read_frames(A.src, W, H, max(0, lo - R), hi + R if hi is not None else None, info["fps"],
                                    rfmt, matrix, vf, nonfin):
                dq.append((i, f))
                last = i
                if wanted(i - R):
                    submit(i - R)
                write(False)
            for c in range(max(lo, last - R + 1), last + 1):
                if wanted(c):
                    submit(c)
            write(True)
        enc.stdin.close()
    except BrokenPipeError:
        broken = True
        try:
            enc.stdin.close()
        except OSError:
            pass
    enc.wait()
    print(f"frame {done[0]}/{done[0]} {time.time() - t0:.0f}s", flush=True)
    if done[0] == 0 and not broken:
        sys.exit(f"no frames in the requested range (start={lo})")
    if broken or enc.returncode != 0:
        remove_output(A.dst)
        sys.exit(f"error: the encoder failed (ffmpeg exit {enc.returncode}, message above); "
                 f"the incomplete {A.dst} was removed")
    n_out = 0 if codec == "exr" else len(subprocess.run(
        ["ffprobe", "-v", "error", "-select_streams", "a", "-show_entries", "stream=index", "-of", "csv=p=0", A.dst],
        capture_output=True, text=True, stdin=subprocess.DEVNULL).stdout.split())
    if plan and n_out == 0:
        print("warning: the requested range contains no audio - video only", flush=True)
        plan = []
    meta = {"src": A.src, "frames": done[0], "range": [lo, lo + done[0]], "seconds": round(time.time() - t0, 1),
            "audio": [{"stream": k, "codec": info["audio"][k]["codec"], "out": c} for k, c in plan] or None,
            "source_pix_fmt": info["pix_fmt"], "pix_fmt": fmt.name, "depth": fmt.bits, "codec": codec,
            "params": P, "profile": prof}
    with open(sidecar_stem(A.dst) + ".json", "w") as fh:
        json.dump(meta, fh, indent=1)
    print(f"wrote {A.dst} ({done[0]} frames, {meta['seconds']}s)", flush=True)


if __name__ == "__main__":
    main()
