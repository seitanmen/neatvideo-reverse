"""Compare the OFX host mode of the C++ UI (qtui_ofx_drv = nvqt::ofx_main, qtui/src/app/ofx.cpp) with the Python UI's
(python3 -m nv_ui_qt.main --ofx, through qtui/tools/py_ofx_drv.py), byte for byte.

    NV_NATIVE=1 python3 qtui/tools/compare_ofx.py OUTDIR [--exe qtui/build/qtui_ofx_drv] [--cases a,b,..]

Every case writes the same host frames to both processes' stdin (test.mp4 n = 300..304 at 640 x 360 and a 320 x 180
crop with an odd size, as uint8 (--depth 8), uint16 with random low bits (16) and float32 with NaN / +-inf in some
frames (32)), runs the same script of UI steps (Apply, parameter edits, another frame, Auto Profile, profile Noise
Level, Cancel, close; see the drivers) and compares: stdout (the frames written to the host) byte for byte, the exit
code, the status-bar text after every step and the "OFX emit error" lines of stderr.  The argument errors (usage,
no frames) are compared the same way.  Settings go to a fresh file per run (NV_UI_QT_SETTINGS) in OUTDIR.
"""
import argparse
import json
import os
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
REPRO = os.path.join(ROOT, "repro")

SCRIPTS = {
    "close": "close",
    "apply2": "apply;apply;close",
    "edit": "apply;set amt_Y 40;apply;frame 1;apply;set t_radius 1;close",
    "auto": "auto;apply;tune all 1.5;apply;tune Y 0.8;cancel",
    "cancel": "cancel",
    "mix": "tab 1;setb mix_enable 1;set mix 30;set noise_Cr 140;apply;close",
}
# name: (input, depth, script, extra args)
CASES = {f"{inp}_d{d}_{sc}": (inp, d, sc, []) for inp in ("t640", "t321")
         for d in (8, 16, 32) for sc in SCRIPTS
         if inp == "t640" or sc in ("edit", "auto")}
CASES["t640_d8_max3_close"] = ("t640", 8, "close", ["30", "3"])          # FPS 30, MAX_FRAMES 3
CASES["t640_d16_fps_close"] = ("t640", 16, "apply;close", ["29.97"])


def inputs(out):
    """{name: (W, H, {depth: bytes})}: 5 frames of test.mp4 (n = 300..304) at 640 x 360 and a 321 x 181 crop."""
    res = {}
    for name, vf, W, H in (("t640", None, 640, 360), ("t321", "format=yuv444p,crop=321:181:100:60", 321, 181)):
        cmd = ["ffmpeg", "-v", "error", "-i", os.path.join(ROOT, "test.mp4"), "-vf",
               "select=between(n\\,300\\,304)" + ("," + vf if vf else ""), "-fps_mode", "passthrough", "-f", "rawvideo",
               "-pix_fmt", "yuv444p", "-"]
        # the OFX layout (4:2:0 with (W/2) x (H/2) chroma) from 4:4:4 planes, so that odd sizes keep W // 2
        c = np.frombuffer(subprocess.run(cmd, capture_output=True, check=True).stdout, np.uint8).reshape(5, 3, H, W)
        y = c[:, 0]
        frames = []
        for k in range(5):
            cb = c[k, 1][: H // 2 * 2:2, : W // 2 * 2:2]
            cr = c[k, 2][: H // 2 * 2:2, : W // 2 * 2:2]
            frames.append(np.concatenate([y[k].ravel(), cb.ravel(), cr.ravel()]))
        a8 = np.concatenate(frames)
        rng = np.random.default_rng(16)
        a16 = (a8.astype("<u2") * 256 + rng.integers(0, 256, a8.size).astype("<u2")).astype("<u2")
        a32 = ((a8.astype(np.float32) + rng.random(a8.size, dtype=np.float32) - np.float32(0.5)) / np.float32(255)).astype("<f4")
        n1 = a8.size // 5
        for k in range(5):            # non-finite samples in every frame (the emitted frame among them)
            a32[k * n1 + 1000 + k] = np.nan
            a32[k * n1 + 5000] = np.inf
            a32[k * n1 + W * H + 30] = -np.inf
            a32[k * n1 + 2000:k * n1 + 2008] = np.nan
        res[name] = (W, H, {8: a8.tobytes(), 16: a16.tobytes(), 32: a32.tobytes()})
    return res


def run(side, exe, args, data, script, out, tag, timeout=600):
    ini = os.path.join(out, f"{tag}_{side}.ini")
    if os.path.exists(ini):
        os.remove(ini)
    env = dict(os.environ, QT_QPA_PLATFORM="offscreen", NV_NATIVE="1", NV_UI_QT_SETTINGS=ini, NV_OFX_SCRIPT=script)
    cmd = [exe, "--ofx", *args] if side == "cpp" else [sys.executable, os.path.join(HERE, "py_ofx_drv.py"), "--ofx", *args]
    try:
        r = subprocess.run(cmd, input=data, capture_output=True, env=env, cwd=REPRO, timeout=timeout)
    except subprocess.TimeoutExpired as e:      # (the W / H overflow cases looped before the N3c L-1 fix)
        r = subprocess.CompletedProcess(cmd, "TIMEOUT", e.stdout or b"", e.stderr or b"")
    err = r.stderr.decode(errors="replace").splitlines()
    msgs = [l for l in err if l.startswith("MSG: ")]
    errs = [l for l in err if l.startswith(("OFX emit error", "usage:", "no frames"))]
    with open(os.path.join(out, f"{tag}_{side}.out"), "wb") as fh:
        fh.write(r.stdout)
    with open(os.path.join(out, f"{tag}_{side}.err"), "w") as fh:
        fh.write(r.stderr.decode(errors="replace"))
    return r.returncode, r.stdout, msgs, errs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--exe", default=os.path.join(ROOT, "qtui", "build", "qtui_ofx_drv"))
    ap.add_argument("--cases", default=",".join(CASES))
    A = ap.parse_args()
    out = os.path.abspath(A.out)
    os.makedirs(out, exist_ok=True)
    ins = inputs(out)
    ok_all = True
    rows = []
    for name in A.cases.split(","):
        inp, d, sc, extra = CASES[name]
        W, H, data = ins[inp]
        args = [str(W), str(H), *(extra or ["30", "0"]), "--depth", str(d)]
        res = {s: run(s, A.exe, args, data[d], SCRIPTS.get(sc, sc), out, name) for s in ("py", "cpp")}
        fs = (W * H + 2 * (W // 2) * (H // 2)) * {8: 1, 16: 2, 32: 4}[d]
        same = res["py"][:4] == res["cpp"][:4]
        ok_all &= same
        nfr = len(res["cpp"][1]) / fs
        rows.append((name, same, len(res["py"][1]), len(res["cpp"][1]), nfr))
        print(f"{name:24s} stdout {'SAME' if res['py'][1] == res['cpp'][1] else 'DIFF'} ({len(res['py'][1])} / "
              f"{len(res['cpp'][1])} B = {nfr:g} frames), exit {res['py'][0]} / {res['cpp'][0]}, "
              f"messages {'SAME' if res['py'][2] == res['cpp'][2] else 'DIFF'} ({len(res['cpp'][2])}), "
              f"errors {'SAME' if res['py'][3] == res['cpp'][3] else 'DIFF'} {res['cpp'][3][:1]}", flush=True)
        if res["py"][2] != res["cpp"][2]:
            for a, b in zip(res["py"][2], res["cpp"][2]):
                if a != b:
                    print("      py :", a)
                    print("      cpp:", b)
                    break
    # the window in OFX mode: menus (enabled states), status strings and a grab, before and after Auto Profile
    sys.path.insert(0, HERE)
    from compare_n3c import menus_same
    from compare_ui import leaves, load_png
    for d in (8, 32):
        W, H, data = ins["t640"]
        dirs = {s: os.path.join(out, f"dump_d{d}_{s}") for s in ("py", "cpp")}
        for s_, dd in dirs.items():
            for sub in ("a", "b"):
                os.makedirs(os.path.join(dd, sub), exist_ok=True)
        res = {}
        for s_ in ("py", "cpp"):
            script = f"dump {dirs[s_]}/a;auto;dump {dirs[s_]}/b;close"
            res[s_] = run(s_, A.exe, [str(W), str(H), "30", "0", "--depth", str(d)], data[d], script, out, f"dump_d{d}")
        same_out = res["py"][:2] == res["cpp"][:2]
        print(f"OFX window d{d}: stdout {'SAME' if same_out else 'DIFF'} ({len(res['cpp'][1])} B), exit {res['py'][0]} / {res['cpp'][0]}")
        ok_all &= same_out
        for sub in ("a", "b"):
            pa, pb = os.path.join(dirs["py"], sub), os.path.join(dirs["cpp"], sub)
            ok_all &= menus_same(json.load(open(os.path.join(pa, "menus_ofx.json"))),
                                 json.load(open(os.path.join(pb, "menus_ofx.json"))), f"OFX d{d} {sub} menus")
            sa, sb = json.load(open(os.path.join(pa, "state_ofx.json"))), json.load(open(os.path.join(pb, "state_ofx.json")))
            dl = leaves(sa, sb)
            print(f"   state_ofx {sub}: {len(sa)} fields, {'equal' if not dl else dl[:6]}")
            ok_all &= not dl
            ia, ib = load_png(os.path.join(pa, "win_ofx.png")), load_png(os.path.join(pb, "win_ofx.png"))
            nd = int((np.abs(ia.astype(int) - ib.astype(int)).max(-1) > 0).sum()) if ia.shape == ib.shape else -1
            print(f"   win_ofx {sub}.png: {nd} of {ia.shape[0] * ia.shape[1]} pixels differ")
            ok_all &= nd == 0
    # argument errors and an empty stdin (no window is shown)
    for args, data in ((["640"], b""), (["640", "360", "--depth", "12"], b""), (["0", "360"], b""), (["a", "b"], b""),
                       (["640", "360", "--depth"], b""), (["640", "360", "x"], b""), (["640", "360"], b""),
                       (["640", "360", "30", "0", "--depth", "16"], b"\0" * 100),
                       # W / H beyond int / a frame size beyond 64 bits (N3c verification L-1): an incomplete frame
                       (["4294967296", "120"], b""), (["2147483648", "1", "24", "2"], ins["t640"][2][8]),
                       (["4294967496", "120", "24", "2"], ins["t640"][2][8]),
                       (["99999999999999999999", "120"], ins["t640"][2][8]),
                       (["3037000500", "3037000500", "24", "0", "--depth", "32"], ins["t640"][2][8])):
        tag = "args_" + "_".join(a.replace("-", "") for a in args)
        res = {s: run(s, A.exe, args, data, "close", out, tag, 60) for s in ("py", "cpp")}
        same = res["py"] == res["cpp"]
        note = ""
        if not same and res["py"][:2] == res["cpp"][:2] and res["cpp"][3] == ["no frames received on stdin"] \
                and "OverflowError" in open(os.path.join(out, f"{tag}_py.err")).read():
            # a frame size beyond Py_ssize_t: Python's stdin.read raises OverflowError (traceback, exit 1); the C++ UI
            # reports the incomplete frame (exit 1); both write nothing (known difference of the message only)
            same, note = True, " (known: Python OverflowError traceback, C++ 'no frames'; exit and stdout equal)"
        ok_all &= same
        print(f"{' '.join(args):28s} exit {res['py'][0]} / {res['cpp'][0]}, stdout {len(res['py'][1])} / "
              f"{len(res['cpp'][1])} B, {'SAME' if same else 'DIFF'} {res['cpp'][3]}{note}", flush=True)
    print("RESULT:", "ALL SAME" if ok_all else "DIFFERENCES")
    return 0 if ok_all else 1


if __name__ == "__main__":
    sys.exit(main())
