"""INT-3 checks of the C++ UI's interlaced export (qtui_export with the params' "interlace" entry) and field profiles
(qtui_fieldprof), against the Python reference (native/fields_ref.py on repro's nvfilter v5 functions).

    python3 qtui/tools/verify_interlace.py OUTDIR [--exe qtui/build_main/qtui_export]
                                                  [--prof-exe qtui/build_main/qtui_fieldprof] [--quick]

Clips (made in OUTDIR): a synthetic 60p clip (static texture + a moving circle + noise) interlaced by ffmpeg
tinterlace and coded losslessly (x264 -qp 0, interlaced coding) top field first and bottom field first, and the TFF
clip as 4:2:2 FFV1 (lossless; its 4:2:0 reading needs scale=interl=1).
  export : every output frame (decoded) == the reference: the frames read as qtui reads them (the same ffmpeg filter
           chain), fields_ref.process_frame_fields (P = v5_params(radius = the field radius)) on the frames around
           each frame, np.clip(np.rint(.), 0, 255) as nv_depth.from_internal; modes same / shift / plain, TFF / BFF;
           the Mix and a crop of the range (--start / --frames); bytes compared
           odd radii (R3, R5); Round 2 (M-1): the TFF clip exported at Bit depth 32 (gbrpf32le, FFV1): the reference
           reads it as qtui does (scale=in_color_matrix=..:in_range=..:interl=1 -> nv_depth.split / to_internal) and
           writes nv_depth.from_internal's float32 bytes
  flags  : ffprobe of the outputs: the first frame's interlaced_frame / top_field_first == the clip's order, for x264
           (mp4, mkv), FFV1, ProRes, and the stream's field_order (mp4 / mov from x264: tt / bb; mkv and the ffv1 /
           prores streams: tb / bt, ffmpeg 8.0.1's values for top / bottom field first frames, INT-1 L-1); x265 / EXR
           are reported (no field flags)
  profile: qtui_fieldprof (build_profile_fields) == nvfilter.build_profile_v5 on the fields as fields.h defines them
           (two: pair = the first field of frame f + 1, more = (the second field of f, the first of f + 1); one: the
           field sequence of that parity), every value of the dict compared; also with a manual area (frame rows
           (y0, x0, h, w) -> field rows (y0 // 2, x0, max(h // 2, 8), w))
  vf     : the 4:2:2 clip is read with scale=interl=1 (the frames == ffmpeg's decode with that filter, != without)
"""
import argparse
import json
import os
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "native"))
sys.dont_write_bytecode = True
import fields_ref as fr  # noqa: E402
from v5_cases import nv  # noqa: E402

F32 = np.float32
W, H = 320, 180


def clip_frames(n, sigma=5.0, seed=7):
    """2n progressive frames (yuv420p uint8 bytes): static texture, a moving disc, noise"""
    rng = np.random.default_rng(seed)
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float64)
    bg = 70 + 45 * np.sin(xx / 19.0) * np.cos(yy / 13.0) + 0.2 * xx
    out = []
    for t in range(2 * n):
        cx, cy = 60 + 5.0 * t, 90 + 25 * np.sin(2 * np.pi * t / 24.0)
        m = np.clip(30.5 - np.hypot(xx - cx, yy - cy), 0, 1)
        Y = bg * (1 - m) + (190 + 20 * np.sin((xx - cx) / 3.0)) * m + rng.normal(0, sigma, (H, W))
        U = 128 + 15 * np.sin(xx / 31.0) * (1 - m) - 30 * m + rng.normal(0, sigma / 2, (H, W))
        V = 128 + 15 * np.cos(yy / 23.0) * (1 - m) + 40 * m + rng.normal(0, sigma / 2, (H, W))
        d2 = lambda a: a.reshape(H // 2, 2, W // 2, 2).mean((1, 3))
        out.append(b"".join(np.clip(np.rint(p), 0, 255).astype(np.uint8).tobytes() for p in (Y, d2(U), d2(V))))
    return b"".join(out)


def make_clips(out, n):
    raw = clip_frames(n)
    clips = {}
    for first, name in ((0, "tff"), (1, "bff")):
        dst = os.path.join(out, f"syn_{name}.mp4")
        mode = "interleave_top" if first == 0 else "interleave_bottom"
        subprocess.run(["ffmpeg", "-y", "-v", "error", "-nostdin", "-f", "rawvideo", "-pix_fmt", "yuv420p", "-s",
                        f"{W}x{H}", "-r", "60", "-i", "-", "-vf", f"tinterlace=mode={mode},setfield={name}", "-c:v",
                        "libx264", "-qp", "0", "-preset", "ultrafast", "-flags", "+ilme+ildct", "-x264-params",
                        f"{name}=1", "-pix_fmt", "yuv420p", dst], input=raw, check=True)
        clips[name] = (dst, first)
    dst = os.path.join(out, "syn_tff_422.mkv")
    subprocess.run(["ffmpeg", "-y", "-v", "error", "-nostdin", "-i", clips["tff"][0], "-vf",
                    "scale=interl=1,format=yuv422p,setfield=tff", "-c:v", "ffv1", dst], check=True)
    clips["tff422"] = (dst, 0)
    return clips


def decode(path, vf=None, n=None, pix="yuv420p"):
    a = ["ffmpeg", "-v", "error", "-nostdin", "-i", path] + (["-vf", vf] if vf else []) + (
        ["-frames:v", str(n)] if n else []) + ["-fps_mode", "passthrough", "-f", "rawvideo", "-pix_fmt", pix, "-"]
    b = subprocess.run(a, capture_output=True, check=True).stdout
    fs = W * H * 3 // 2
    k = len(b) // fs
    arr = np.frombuffer(b, np.uint8)[:k * fs].reshape(k, fs)
    return [(arr[i, :W * H].reshape(H, W).astype(F32), arr[i, W * H:W * H * 5 // 4].reshape(H // 2, W // 2).astype(F32),
             arr[i, W * H * 5 // 4:].reshape(H // 2, W // 2).astype(F32)) for i in range(k)]


def probe_flags(path):
    s = json.loads(subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
                                   "stream=field_order,codec_name", "-of", "json", path],
                                  capture_output=True, text=True, check=True).stdout)["streams"][0]
    f = json.loads(subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-read_intervals", "%+#1",
                                   "-show_entries", "frame=interlaced_frame,top_field_first", "-of", "json", path],
                                  capture_output=True, text=True, check=True).stdout).get("frames", [{}])[0]
    return s.get("codec_name"), s.get("field_order", ""), f.get("interlaced_frame", -1), f.get("top_field_first", -1)


def field_profile_ref(frames, f, kind, parity, matrix="bt601", area=None, full_range=False):
    fl = lambda k, q: tuple(fr.split_plane(p, q) for p in frames[k])
    n = len(frames)
    pl = fl(f, parity)
    if kind == "one":
        more = [fl(k, parity) for k in range(f + 1, min(f + 3, n))]
        pair = more[0] if more else None
    else:
        more = [fl(f, 1 - parity)] + ([fl(f + 1, parity)] if f + 1 < n else [])
        pair = more[1] if len(more) > 1 else None
    fa = None if area is None else (area[0] // 2, area[1], max(area[2] // 2, 8), area[3])
    return nv.build_profile_v5(pl, full_range, fa, pair=pair, mode="auto", more=tuple(more), matrix=matrix)


def cmp_values(a, b, path=""):
    """differences between the Python dict a and the JSON value b (floats compared exactly)"""
    bad = []
    if isinstance(a, dict):
        for k in a:
            if k not in b:
                bad.append(f"{path}/{k} missing")
            else:
                bad += cmp_values(a[k], b[k], f"{path}/{k}")
    elif isinstance(a, (list, tuple)):
        if len(a) != len(b):
            return [f"{path} length {len(a)} != {len(b)}"]
        for i, (x, y) in enumerate(zip(a, b)):
            bad += cmp_values(x, y, f"{path}[{i}]")
    elif isinstance(a, float):
        if not (a == b or (a != a and b != b)):
            bad.append(f"{path}: {a!r} != {b!r}")
    elif a != b:
        bad.append(f"{path}: {a!r} != {b!r}")
    return bad


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--exe", default=os.path.join(ROOT, "qtui", "build_main", "qtui_export"))
    ap.add_argument("--prof-exe", default=os.path.join(ROOT, "qtui", "build_main", "qtui_fieldprof"))
    ap.add_argument("--quick", action="store_true")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    N = 8 if a.quick else 12
    clips = make_clips(a.out, N)
    ok = True
    # ---------------------------------------------------------------- profiles
    frames_t = decode(clips["tff"][0])
    for kind, parity, f, area in (("two", 0, 3, None), ("one", 0, 3, None), ("one", 1, 5, None), ("two", 1, N - 1, None),
                                  ("two", 0, 4, (37, 41, 70, 90)), ("one", 1, 2, (90, 150, 64, 64))):
        args = [a.prof_exe, clips["tff"][0], str(f), kind, str(parity)]
        if area:
            args += ["auto", ",".join(map(str, area))]
        r = subprocess.run(args, capture_output=True, text=True)
        if r.returncode != 0:
            print(f"profile {kind} parity {parity} f{f}: qtui_fieldprof failed: {r.stderr.strip()}")
            ok = False
            continue
        got = json.loads(r.stdout)
        exp = field_profile_ref(frames_t, f, kind, parity, area=area)
        bad = cmp_values(exp, got)
        extra = [got.get("fields"), got.get("field"), got.get("ui_frame")]
        good = not bad and extra == [kind, ("top", "bottom")[parity], f]
        ok &= good
        print(f"profile {kind:3s} {('top', 'bottom')[parity]:6s} frame {f}{' area ' + str(area) if area else ''}: {len(exp)} keys "
              f"{'== nvfilter.build_profile_v5 on the fields' if not bad else 'DIFFER ' + str(bad[:3])}, "
              f"fields/field/ui_frame {extra}")
    # ---------------------------------------------------------------- exports
    runs = [("tff", "same", 4, 0.0, None), ("bff", "same", 4, 0.0, None), ("tff", "shift", 2, 0.0, None),
            ("bff", "plain", 2, 0.25, None), ("tff", "same", 2, 0.0, (3, 4)), ("tff422", "same", 4, 0.0, None),
            ("tff", "plain", 3, 0.0, (2, 6)), ("bff", "same", 5, 0.0, None), ("bff", "shift", 3, 0.0, (5, 4))]
    if a.quick:
        runs = runs[:2] + runs[-1:]
    for clip, mode, R, mix, rng in runs:
        src, first = clips[clip]
        vf = "scale=interl=1" if clip == "tff422" else None
        frames = decode(src, vf)
        prof = field_profile_ref(frames, len(frames) // 2, "two", first)
        prof.update({"fields": "two", "field": ("top", "bottom")[first]})
        tag = f"{clip}_{mode}_r{R}_m{mix}" + (f"_s{rng[0]}" if rng else "")
        pj, qj = os.path.join(a.out, tag + ".prof.json"), os.path.join(a.out, tag + ".params.json")
        json.dump(prof, open(pj, "w"))
        params = {"mix": mix, "interlace": {"first": first, "mode": mode, "radius": R}}
        json.dump(params, open(qj, "w"))
        dst = os.path.join(a.out, tag + ".mkv")
        cmd = [a.exe, src, dst, "--ui-profile", pj, "--params", qj, "--crf", "0", "--codec", "x264", "--jobs", "2"]
        if rng:
            cmd += ["--start", str(rng[0]), "--frames", str(rng[1])]
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode != 0:
            print(f"export {tag}: exit {r.returncode}\n{r.stdout}")
            ok = False
            continue
        got = decode(dst)
        P = nv.v5_params(radius=R, mix=mix)
        m = R // 2 + 1
        lo, hi = (rng[0], rng[0] + rng[1]) if rng else (0, len(frames))
        nbad = 0
        for i, n in enumerate(range(lo, hi)):
            wl, wh = max(0, n - m), min(len(frames), n + m + 1)
            e = fr.process_frame_fields(frames[wl:wh], n - wl, first, fr.MODES[mode], prof, P)
            e = [np.clip(np.rint(p), 0, 255).astype(np.uint8) for p in e]
            nbad += sum(int(np.count_nonzero(x != y.astype(np.uint8))) for x, y in zip(e, got[i]))
        good = len(got) == hi - lo and nbad == 0
        ok &= good
        il_line = [ln for ln in r.stdout.splitlines() if ln.startswith("interlaced:")]
        print(f"export {tag:24s}: {len(got)}/{hi - lo} frames, {nbad} bytes differ from fields_ref "
              f"({'BIT-EXACT' if good else 'MISMATCH'}); {il_line[0] if il_line else 'no interlaced line'}")
        cf = probe_flags(dst)
        want = 1 if first == 0 else 0
        fl_ok = cf[2] == 1 and cf[3] == want and cf[1] == ("tb" if first == 0 else "bt")
        ok &= fl_ok
        print(f"  flags {cf[0]} mkv: stream field_order {cf[1] or '(none)'}, interlaced_frame {cf[2]} top_field_first "
              f"{cf[3]} ({'ok' if fl_ok else 'WRONG'})")
    # ---------------------------------------------------------------- the other containers / codecs
    for codec, ext, depth, ck, expect in (("x264", ".mp4", "8", "bff", True), ("x264", ".mp4", "8", "tff", True),
                                          ("ffv1", ".mkv", "8", "bff", True),
                                          ("prores", ".mov", "10", "tff422", True), ("x264", ".mov", "10", "bff", True),
                                          ("x265", ".mp4", "8", "bff", False), ("ffv1", ".mkv", "16", "bff", True),
                                          ("ffv1", ".mkv", "10", "tff422", True)):
        src, first = clips[ck]
        frames = decode(src, "scale=interl=1" if ck == "tff422" else None)
        prof = field_profile_ref(frames, len(frames) // 2, "two", first)
        prof.update({"fields": "two", "field": ("top", "bottom")[first]})
        if ck == "tff422" and depth != "8":     # processed as 4:4:4 (nv_depth widens 4:2:2): only the flags are checked
            prof["sub"] = 1
        pj, qj = os.path.join(a.out, f"codec_{ck}_{depth}.prof.json"), os.path.join(a.out, f"codec_{ck}_{depth}.params.json")
        json.dump(prof, open(pj, "w"))
        json.dump({"interlace": {"first": first, "mode": "same", "radius": 4}}, open(qj, "w"))
        dst = os.path.join(a.out, f"codec_{codec}_{depth}_{ck}{ext}")
        r = subprocess.run([a.exe, src, dst, "--ui-profile", pj, "--params", qj, "--crf", "0", "--codec", codec,
                            "--depth", depth, "--start", "2", "--frames", "3", "--jobs", "2"],
                           capture_output=True, text=True)
        if r.returncode != 0:
            print(f"codec {codec} {ext} depth {depth}: exit {r.returncode} {r.stdout.strip().splitlines()[-1:]}")
            ok &= not expect
            continue
        cf = probe_flags(dst)
        flagged = cf[2] == 1 and cf[3] == (1 if first == 0 else 0)
        if expect:                    # the stream's field_order: x264 in mp4 / mov tt / bb, else ffmpeg's tb / bt
            mp4ish = codec == "x264" and ext in (".mp4", ".mov")
            flagged &= cf[1] == ((("tt", "bb") if mp4ish else ("tb", "bt"))[first])
        warn = [ln for ln in r.stdout.splitlines() if ln.startswith("warning")]
        good = flagged == expect
        ok &= good
        print(f"codec {cf[0]:9s} {ext} depth {depth:2s}: stream field_order {cf[1] or '(none)'}, interlaced_frame {cf[2]} "
              f"top_field_first {cf[3]} -> {('top', 'bottom')[first] + ' field first flagged' if flagged else 'not flagged'}"
              f"{' (expected: not flagged; ' + warn[0] + ')' if not expect and warn else ''} "
              f"{'ok' if good else 'UNEXPECTED'}")
    # ---------------------------------------------------------------- Bit depth 32 (M-1): RGB float of a 4:2:0 clip
    import nv_depth as nd
    fmt = nd.pixfmt("gbrpf32le")
    src, first = clips["tff"]
    mat = "bt601"
    vf32 = f"scale=in_color_matrix={mat}:in_range=tv:interl=1"
    b = subprocess.run(["ffmpeg", "-v", "error", "-nostdin", "-i", src, "-vf", vf32, "-fps_mode", "passthrough", "-f",
                        "rawvideo", "-pix_fmt", "gbrpf32le", "-"], capture_output=True, check=True).stdout
    fs = W * H * 12
    f32 = [nd.to_internal(nd.split(b[i * fs:(i + 1) * fs], fmt, W, H), fmt, mat) for i in range(len(b) // fs)]
    f32 = [tuple(np.ascontiguousarray(p, F32) for p in fr_) for fr_ in f32]
    prof = field_profile_ref(f32, len(f32) // 2, "two", first, full_range=True)
    prof.update({"fields": "two", "field": "top"})
    pj, qj = os.path.join(a.out, "d32.prof.json"), os.path.join(a.out, "d32.params.json")
    json.dump(prof, open(pj, "w"))
    json.dump({"interlace": {"first": first, "mode": "same", "radius": 4}}, open(qj, "w"))
    dst = os.path.join(a.out, "d32_tff.mkv")
    r = subprocess.run([a.exe, src, dst, "--ui-profile", pj, "--params", qj, "--crf", "0", "--codec", "ffv1", "--depth",
                        "32", "--start", "2", "--frames", "5", "--jobs", "2"], capture_output=True, text=True)
    if r.returncode != 0:
        print(f"depth 32: exit {r.returncode}\n{r.stdout}")
        ok = False
    else:
        got = subprocess.run(["ffmpeg", "-v", "error", "-nostdin", "-i", dst, "-f", "rawvideo", "-pix_fmt", "gbrpf32le",
                              "-"], capture_output=True, check=True).stdout
        P = nv.v5_params(radius=4)
        nbad = 0
        for i, n in enumerate(range(2, 7)):
            wl, wh = max(0, n - 3), min(len(f32), n + 4)
            e = fr.process_frame_fields(f32[wl:wh], n - wl, first, fr.SAME, prof, P)
            eb = np.frombuffer(nd.from_internal(e, fmt, mat), np.uint32)
            gb = np.frombuffer(got[i * fs:(i + 1) * fs], np.uint32)
            nbad += int(np.count_nonzero(eb != gb)) if eb.size == gb.size else eb.size
        il_line = [ln for ln in r.stdout.splitlines() if ln.startswith("interlaced:")]
        rd = "interl=1" in (il_line[0] if il_line else "")
        good = len(got) == 5 * fs and nbad == 0 and rd
        ok &= good
        print(f"depth 32 (gbrpf32le, 4:2:0 source): 5 frames, {nbad} float words differ from fields_ref "
              f"({'BIT-EXACT' if good else 'MISMATCH'}); {il_line[0] if il_line else 'no interlaced line'}")
        # the plain RGB read (no interl=1) differs: the check would see a missing interl=1
        bp = subprocess.run(["ffmpeg", "-v", "error", "-nostdin", "-i", src, "-vf", vf32.replace(":interl=1", ""),
                             "-frames:v", "1", "-f", "rawvideo", "-pix_fmt", "gbrpf32le", "-"], capture_output=True,
                            check=True).stdout
        print(f"  without interl=1 the RGB read differs in {int(np.count_nonzero(np.frombuffer(bp, F32) != np.frombuffer(b[:fs], F32)))}"
              f" of {W * H * 3} samples of frame 0")

    # ---------------------------------------------------------------- the 4:2:2 read filter
    raw_i = decode(clips["tff422"][0], "scale=interl=1")
    raw_p = decode(clips["tff422"][0])
    diff = sum(int(np.count_nonzero(x[1] != y[1])) for x, y in zip(raw_i, raw_p))
    print(f"vf     : 4:2:2 source read with scale=interl=1 differs from the plain conversion in {diff} chroma samples "
          f"(the export above read it with interl=1: its frames are the reference's)")
    ok &= diff > 0
    print("ALL OK" if ok else "FAILURES")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
