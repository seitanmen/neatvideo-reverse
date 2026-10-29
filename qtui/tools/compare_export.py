"""Compare the C++ export (qtui_export = qtui/src/core/export.cpp, the ExportJob of the C++ UI) with the Python
one (python3 -m nv_ui_qt.export, native core on) on the same input, profile and parameters.

    QT_QPA_PLATFORM=offscreen NV_NATIVE=1 python3 qtui/tools/compare_export.py OUTDIR [--exe qtui/build_exp/qtui_export]
                                                                                       [--cases a,b,...] [--jobs 2]

For every case the script writes the UI's profile (backend.build_profile + run_aft, as the UI does after Auto
Profile) and parameters (state.backend_params) as JSON, runs both exports with the same arguments and compares:
  - the output file byte for byte (mp4 / ts), or for .mkv (random SegmentUID) the decoded frames (ffmpeg -f
    framemd5) and the file length;
  - every ffmpeg / ffprobe command line both sides spawned (NV_EXPORT_TRACE on the C++ side, a subprocess patch
    on the Python side), word for word after normalising the temporary paths and the tool's directory.
Writes only into OUTDIR (profiles, parameters, outputs, traces, the audio test clips, a 10-bit source).
"""
import argparse
import filecmp
import json
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
REPRO = os.path.join(ROOT, "repro")
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
os.environ.setdefault("NV_NATIVE", "1")

CASES = {
    # name: (video key, ui edits, export args)
    "a_t_default": ("t", {}, ["--crf", "0", "--start", "300", "--frames", "4"]),
    "b_v02_default": ("v02", {}, ["--crf", "0", "--start", "100", "--frames", "4"]),
    "c1_t_perchan": ("t", {"bamt_Cr_medium": 60.0, "bamt_Cb_low": 30.0, "amt_Y": 50.0, "bnoise_Y_high": 130.0},
                     ["--crf", "0", "--start", "300", "--frames", "4"]),
    "c2_v02_mix": ("v02", {"mix_enable": True, "mix": 25.0, "adv_noise_level": 150.0, "adv_threshold": 3.0, "t_noise": 60.0},
                   ["--crf", "0", "--start", "100", "--frames", "4"]),
    "c3_t_radius1_s_off": ("t", {"t_radius": 1, "s_enable": False, "noise_Cb": 150.0},
                           ["--crf", "0", "--start", "300", "--frames", "4"]),
    "d_aac_copy_crf16": ("aac", {}, ["--crf", "16", "--audio", "copy", "--start", "20", "--frames", "24"]),
    "d2_ts_mp2_crf16": ("ts", {}, ["--crf", "16", "--audio", "copy", "--start", "20", "--frames", "24"]),
    "e_10bit_mkv": ("src10", {}, ["--crf", "0", "--depth", "10", "--start", "4", "--frames", "3"]),
    "f1_audio_aac": ("aac", {}, ["--crf", "16", "--audio", "aac", "--start", "20", "--frames", "12"]),
    "f2_audio_none": ("aac", {}, ["--crf", "16", "--audio", "none", "--start", "20", "--frames", "12"]),
    "h_full_short": ("aac", {}, ["--crf", "16", "--audio", "copy"]),           # whole clip (no --start / --frames)
    # the other depths (the C++ conversions of nv_depth; smoke tests, not part of the N3b acceptance)
    "e2_12bit_x265": ("src10", {}, ["--crf", "0", "--depth", "12", "--start", "4", "--frames", "3"]),
    "e3_16bit_mkv": ("src10", {}, ["--crf", "0", "--depth", "16", "--start", "4", "--frames", "3"]),
    "e4_32float_mkv": ("src10", {}, ["--crf", "0", "--depth", "32", "--start", "4", "--frames", "3"]),
    # N3c (the nv_depth conversions moved into libnvcore, nvcore_c_depth.h): 4:2:2 widening and back, float RGB with
    # NaN / +-inf (sanitize and restore), the EXR sequence, integer RGB
    "g1_422_prores": ("src422", {"amt_Cr": 50.0}, ["--crf", "0", "--depth", "source", "--codec", "prores", "--start", "2",
                                                    "--frames", "3"]),
    "g2_422_ffv1": ("src422", {}, ["--crf", "0", "--depth", "source", "--start", "1", "--frames", "4"]),
    "g3_rgbf32_nan_mkv": ("rgbf32", {"amt_Y": 90.0}, ["--crf", "0", "--depth", "32"]),
    "g4_rgbf32_exr": ("rgbf32", {}, ["--crf", "0", "--depth", "32", "--start", "2", "--frames", "4"]),
    "g5_gbrp10_mkv": ("gbrp10", {}, ["--crf", "0", "--depth", "source", "--start", "1", "--frames", "4"]),
}
PROFILE_DEPTH = {"e4_32float_mkv": "32", "g1_422_prores": "source", "g2_422_ffv1": "source", "g3_rgbf32_nan_mkv": "32",
                 "g4_rgbf32_exr": "32", "g5_gbrp10_mkv": "source"}   # the profile built at that depth
EXT = {"e_10bit_mkv": ".mkv", "d2_ts_mp2_crf16": ".mp4", "e3_16bit_mkv": ".mkv", "e4_32float_mkv": ".mkv",
       "g1_422_prores": ".mov", "g2_422_ffv1": ".mkv", "g3_rgbf32_nan_mkv": ".mkv", "g4_rgbf32_exr": ".exr",
       "g5_gbrp10_mkv": ".mkv"}


def videos(out):
    """The source files: test.mp4, test_v02.mp4, an AAC clip, an MPEG-TS clip (mp2), a 10-bit FFV1 source."""
    t, v02 = os.path.join(ROOT, "test.mp4"), os.path.join(ROOT, "test_v02.mp4")
    aac, ts, src10 = (os.path.join(out, n) for n in ("clip_aac.mp4", "clip_mp2.ts", "src10.mkv"))
    if not os.path.exists(aac):
        subprocess.run(["ffmpeg", "-y", "-v", "error", "-i", t, "-f", "lavfi", "-i", "sine=frequency=440:sample_rate=48000",
                        "-t", "6", "-c:v", "copy", "-c:a", "aac", "-shortest", aac], check=True)
    if not os.path.exists(ts):
        subprocess.run(["ffmpeg", "-y", "-v", "error", "-i", t, "-f", "lavfi", "-i", "sine=frequency=330:sample_rate=48000",
                        "-t", "6", "-c:v", "copy", "-c:a", "mp2", "-shortest", "-f", "mpegts", ts], check=True)
    if not os.path.exists(src10):
        import numpy as np
        from denoise_video_v5 import probe
        sinfo = probe(t)
        W, H = sinfo["W"], sinfo["H"]
        raw8 = subprocess.run(["ffmpeg", "-v", "error", "-i", t, "-frames:v", "12", "-f", "rawvideo", "-pix_fmt", "yuv420p", "-"],
                              capture_output=True, check=True).stdout
        a10 = np.frombuffer(raw8, np.uint8).astype("<u2") * 4 + \
            np.random.default_rng(10).integers(0, 4, len(raw8)).astype("<u2")
        tags = ["-colorspace", "bt709", "-color_range", "tv"]
        subprocess.run(["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "yuv420p10le", "-s", f"{W}x{H}",
                        "-r", sinfo["fps"], *tags, "-i", "-", "-c:v", "ffv1", "-pix_fmt", "yuv420p10le", *tags, src10],
                       input=a10.tobytes(), check=True)
    src422, rgbf32, gbrp10 = (os.path.join(out, n) for n in ("src422.mkv", "rgbf32.mkv", "gbrp10.mkv"))
    if not (os.path.exists(src422) and os.path.exists(rgbf32) and os.path.exists(gbrp10)):
        import numpy as np
        w, h, n = 320, 180, 6
        # 10-bit 4:2:2 (test.mp4 n = 600.., a 320 x 180 crop, random low bits)
        raw = subprocess.run(["ffmpeg", "-v", "error", "-ss", "20", "-i", t, "-frames:v", str(n), "-vf", "crop=320:180:200:100",
                              "-f", "rawvideo", "-pix_fmt", "yuv422p", "-"], capture_output=True, check=True).stdout
        a = np.frombuffer(raw, np.uint8).astype("<u2") * 4 + np.random.default_rng(422).integers(0, 4, len(raw)).astype("<u2")
        tags = ["-colorspace", "bt709", "-color_range", "tv"]
        subprocess.run(["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "yuv422p10le", "-s", f"{w}x{h}", "-r", "24",
                        *tags, "-i", "-", "-c:v", "ffv1", "-pix_fmt", "yuv422p10le", *tags, src422], input=a.tobytes(), check=True)
        # float RGB with NaN / +-inf / an out-of-range value in frames 3..5, and 10-bit integer RGB (test.mp4 n = 700..)
        raw = subprocess.run(["ffmpeg", "-v", "error", "-ss", "23.333", "-i", t, "-frames:v", str(n), "-vf", "crop=320:180:160:90",
                              "-f", "rawvideo", "-pix_fmt", "gbrp", "-"], capture_output=True, check=True).stdout
        a = np.frombuffer(raw, np.uint8).reshape(n, 3, h, w)
        rng = np.random.default_rng(32)
        f = (a.astype(np.float32) + rng.random(a.shape, dtype=np.float32) - 0.5) / 255.0
        for k in (3, 4, 5):
            f[k, 0, 10:14, 20:30] = np.nan
            f[k, 1, 50, 50:60] = np.inf
            f[k, 2, 100:103, 200] = -np.inf
            f[k, 2, 120, 120] = 1.75
        subprocess.run(["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "gbrpf32le", "-s", f"{w}x{h}", "-r", "30",
                        "-i", "-", "-c:v", "ffv1", "-level", "4", "-strict", "experimental", "-pix_fmt", "gbrpf32le", rgbf32],
                       input=f.astype("<f4").tobytes(), check=True)
        i10 = a.astype("<u2") * 4 + rng.integers(0, 4, a.shape).astype("<u2")
        subprocess.run(["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "gbrp10le", "-s", f"{w}x{h}", "-r", "30",
                        "-i", "-", "-c:v", "ffv1", "-pix_fmt", "gbrp10le", gbrp10], input=i10.tobytes(), check=True)
    return {"t": t, "v02": v02, "aac": aac, "ts": ts, "src10": src10, "src422": src422, "rgbf32": rgbf32, "gbrp10": gbrp10}


def make_profile(video, pf, path, depth="8"):
    """The UI's profile after Auto Profile (Current Frame pf) + Auto Fine-Tune, as .ui_profile.json (the frames read
    at the processing depth of Preferences > Bit depth, as the UI holds them)."""
    import numpy as np
    import nv_depth as nd
    import nvfilter as nv
    from denoise_video_v5 import probe, read_frames
    from nv_ui_qt import backend
    info = probe(video)
    W, H = info["W"], info["H"]
    matrix = nv.matrix_from_tags(info["tags"], H)
    fmt = nd.choose_format(nd.source_format(info["pix_fmt"]), depth)
    rfmt, vf = (None, None) if fmt.legacy else (fmt, nd.read_filter(info, fmt, matrix))
    fr = [tuple(np.array(p) for p in f) for _, f in read_frames(video, W, H, pf, pf + 3, info["fps"], rfmt, matrix, vf)]
    full_range = nd.full_range_internal(info, fmt)
    prof = backend.build_profile(fr, 0, full_range, matrix, "auto")
    prof["ui_frame"] = pf
    prof = backend.run_aft(prof, fr[0], full_range, matrix)
    with open(path, "w") as fh:
        json.dump(prof, fh, indent=1)
    return prof


def make_params(edits, path):
    from nv_ui_qt.state import backend_params, default_params
    P = backend_params(dict(default_params(), **edits))
    with open(path, "w") as fh:
        json.dump(P, fh, indent=1)
    return P


def py_driver(argv, trace):
    """Run nv_ui_qt.export.main(argv) in this process, recording every subprocess command into `trace`."""
    os.chdir(REPRO)
    sys.path.insert(0, REPRO)
    sys.dont_write_bytecode = True
    real_popen = subprocess.Popen

    def log(args):
        with open(trace, "a") as fh:
            fh.write(json.dumps([str(a) for a in args]) + "\n")

    class Popen(real_popen):
        def __init__(self, args, *a, **k):
            log(args)
            super().__init__(args, *a, **k)

    subprocess.Popen = Popen            # subprocess.run goes through Popen, so it is logged there
    import nv_ui_qt.export as E
    E.main(argv)


def norm_cmd(args, dst):
    """Normalise a command line for the comparison: the tool by its base name, temporary paths by '<tmp>', the
    output file (named per side) by '<dst>'."""
    out = [os.path.basename(args[0])]
    for a in args[1:]:
        if a.startswith(("/var/folders", "/private/var/folders", "/tmp/", "/private/tmp/")):
            a = "<tmp>/" + os.path.basename(a)
        elif a == dst:
            a = "<dst>"
        out.append(a)
    return out


def read_trace(path, dst):
    if not os.path.exists(path):
        return []
    return [norm_cmd(json.loads(l), dst) for l in open(path) if l.strip()]


def seq(pattern):
    """The files of an EXR sequence pattern (name_%05d.exr), sorted; [] for other paths."""
    if "%05d" not in pattern:
        return []
    import glob
    return sorted(glob.glob(pattern.replace("%05d", "[0-9]" * 5)))


def framemd5(path):
    r = subprocess.run(["ffmpeg", "-v", "error", "-i", path, "-map", "0:v", "-f", "framemd5", "-"], capture_output=True, text=True)
    return [l for l in r.stdout.splitlines() if l and not l.startswith("#")]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--exe", default=os.path.join(ROOT, "qtui", "build_exp", "qtui_export"))
    ap.add_argument("--cases", default=",".join(CASES))
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--py-driver", nargs=argparse.REMAINDER, help=argparse.SUPPRESS)
    A = ap.parse_args()
    if A.py_driver is not None:
        trace, argv = A.py_driver[0], A.py_driver[1:]
        py_driver(argv, trace)
        return 0
    out = os.path.abspath(A.out)
    os.makedirs(out, exist_ok=True)
    sys.path.insert(0, REPRO)
    sys.dont_write_bytecode = True
    vids = videos(out)
    profs = {}
    results = []
    for name in A.cases.split(","):
        vkey, edits, xargs = CASES[name]
        video = vids[vkey]
        pkey = "t" if vkey in ("t", "aac", "ts", "src10") else vkey
        pdepth = PROFILE_DEPTH.get(name, "8")
        if pdepth != "8":
            pkey = f"{vkey}_{pdepth}"
        if pkey not in profs:
            pp = os.path.join(out, f"profile_{pkey}.ui_profile.json")
            if not os.path.exists(pp):
                make_profile(vids[vkey if pdepth != "8" else pkey], 4 if pdepth != "8" else 300 if pkey == "t" else 100, pp, pdepth)
            profs[pkey] = pp
        ppath = os.path.join(out, f"{name}.ui_params.json")
        make_params(edits, ppath)
        ext = EXT.get(name, ".mp4")
        res = {"case": name, "video": os.path.basename(video)}
        outs, traces, logs, times = {}, {}, {}, {}
        for side in ("py", "cpp"):
            dst = os.path.join(out, f"{name}_{side}{'_%05d' if ext == '.exr' else ''}{ext}")
            for f in [dst, os.path.splitext(dst)[0] + ".json", os.path.join(out, f"{name}_{side}.json")] + seq(dst):
                if os.path.exists(f):
                    os.remove(f)
            trace = os.path.join(out, f"{name}_{side}.trace")
            if os.path.exists(trace):
                os.remove(trace)
            args = [video, dst, "--ui-profile", profs[pkey], "--params", ppath, "--jobs", str(A.jobs), *xargs]
            env = dict(os.environ, NV_NATIVE="1", QT_QPA_PLATFORM="offscreen", NV_EXPORT_TRACE=trace)
            if side == "py":
                cmd = [sys.executable, os.path.abspath(__file__), out, "--py-driver", trace, *args]
            else:
                cmd = [A.exe, *args]
            t0 = time.time()
            r = subprocess.run(cmd, capture_output=True, text=True, env=env, cwd=REPRO)
            times[side] = time.time() - t0
            logs[side] = (r.stdout + r.stderr).strip()
            outs[side] = (dst, r.returncode)
            traces[side] = read_trace(trace, dst)
            with open(os.path.join(out, f"{name}_{side}.log"), "w") as fh:
                fh.write(f"$ {' '.join(cmd)}\nexit {r.returncode} ({times[side]:.1f}s)\n{logs[side]}\n")
        dp, dc = outs["py"][0], outs["cpp"][0]
        res["exit"] = (outs["py"][1], outs["cpp"][1])
        res["bytes"] = (os.path.getsize(dp) if os.path.exists(dp) else -1, os.path.getsize(dc) if os.path.exists(dc) else -1)
        if ext == ".exr":
            a, b = seq(dp), seq(dc)
            res["bytes"] = (sum(os.path.getsize(x) for x in a), sum(os.path.getsize(x) for x in b))
            res["frames"] = (len(a), len(b))
            res["same"] = bool(a) and len(a) == len(b) and all(filecmp.cmp(x, y, shallow=False) for x, y in zip(a, b))
            res["how"] = "every EXR frame byte for byte"
            mp, mc = os.path.join(out, f"{name}_py.json"), os.path.join(out, f"{name}_cpp.json")
        elif ext == ".mkv":
            a, b = framemd5(dp) if os.path.exists(dp) else [], framemd5(dc) if os.path.exists(dc) else []
            res["frames"] = (len(a), len(b))
            res["same"] = bool(a) and a == b and res["bytes"][0] == res["bytes"][1]
            res["how"] = "decoded frames (framemd5) + file length"
        else:
            res["same"] = os.path.exists(dp) and os.path.exists(dc) and filecmp.cmp(dp, dc, shallow=False)
            res["how"] = "byte for byte"
            n = subprocess.run(["ffprobe", "-v", "error", "-count_frames", "-select_streams", "v:0", "-show_entries",
                                "stream=nb_read_frames", "-of", "csv=p=0", dc], capture_output=True, text=True).stdout.strip()
            res["frames"] = n
        # the side files: params / profile of the meta json (seconds differs; an EXR sequence: <stem>.json)
        if ext != ".exr":
            mp, mc = os.path.splitext(dp)[0] + ".json", os.path.splitext(dc)[0] + ".json"
        if os.path.exists(mp) and os.path.exists(mc):
            jp, jc = json.load(open(mp)), json.load(open(mc))
            jp.pop("seconds", None)
            jc.pop("seconds", None)
            jp["src"] = jc["src"] = ""
            res["meta_same"] = json.dumps(jp, sort_keys=True) == json.dumps(jc, sort_keys=True)
        else:
            res["meta_same"] = None
        tp, tc = traces["py"], traces["cpp"]
        res["cmds"] = (len(tp), len(tc))
        res["cmds_same"] = tp == tc
        if not res["cmds_same"]:
            diffs = [(i, p, c) for i, (p, c) in enumerate(zip(tp, tc)) if p != c]
            res["cmd_diff"] = diffs[:3] + ([("len", len(tp), len(tc))] if len(tp) != len(tc) else [])
        res["time"] = (round(times["py"], 1), round(times["cpp"], 1))
        results.append(res)
        print(f"{name:22s} output {'SAME' if res['same'] else 'DIFF'} ({res['how']}, {res['bytes']} B, frames {res['frames']}), "
              f"commands {'SAME' if res['cmds_same'] else 'DIFF'} ({res['cmds']}), meta {res['meta_same']}, "
              f"exit {res['exit']}, time py {res['time'][0]}s cpp {res['time'][1]}s", flush=True)
        if not res["cmds_same"]:
            for d in res.get("cmd_diff", []):
                print("     ", d)
        if not res["same"]:
            print("      py :", logs["py"][-600:])
            print("      cpp:", logs["cpp"][-600:])
    with open(os.path.join(out, "results.json"), "w") as fh:
        json.dump(results, fh, indent=1)
    ok = all(r["same"] and r["cmds_same"] for r in results)
    print("RESULT:", "ALL SAME" if ok else "DIFFERENCES")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
