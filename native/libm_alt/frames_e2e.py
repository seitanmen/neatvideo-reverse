# W2a: processed frames end to end through the UI backend (reference/python/nv_ui_qt/backend.process_frame, native core on)
# with the default libnvcore.dylib and with libm_alt/bin/libnvcore.dylib at NV_LIBM_ALT = 0..3.
#
#   python3 native/libm_alt/frames_e2e.py OUT_DIR BRIDGE_TMP     (verify_libm_alt.py --stages frames)
#
# Inputs are verify_bridge's caches in BRIDGE_TMP (the alt-0 run: system libm): the frames (test_v02
# n = 306..320, test.mp4 n = 298..308, as the UI decodes them) and the AFT profiles of n = 312 / 302.  Every
# frame with a full radius-2 window is processed with the UI defaults.  The outputs are compared with the
# default library as float32 words and as the codes the export writes (nv_depth / denoise_video_v5:
# clip(rint(x * k), 0, 2^b - 1), k = 1 / 4 / 256 for 8 / 10 / 16 bits).
import json
import os
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
NATIVE = os.path.dirname(HERE)
ROOT = os.path.dirname(NATIVE)
REPRO = os.path.join(ROOT, "reference", "python")
VIDEOS = {"v02": ("frames_v02_306_321.npz", 312), "t": ("frames_t_298_309.npz", 302)}

CHILD = r'''
import json, os, sys
import numpy as np
sys.path.insert(0, %(repro)r)
os.environ.pop("NV_NATIVE", None)
from nv_ui_qt import backend
from nv_ui_qt.state import backend_params, default_params
import nv_native
assert nv_native.AVAILABLE, "library not loaded"
backend.set_native(True)
P = backend_params(default_params())
out = {}
for video, (npz, pn) in %(videos)r.items():
    z = np.load(os.path.join(%(tmp)r, npz))
    ids = sorted({int(k[1:]) for k in z.files})
    fr = {n: tuple(z[c + str(n)].astype(np.float32) for c in "ybr") for n in ids}
    with open(os.path.join(%(tmp)r, f"aft_{video}_{pn}.json")) as f:
        prof = json.load(f)
    for n in ids:
        if n - 2 not in fr or n + 2 not in fr:
            continue
        win = [fr[i] for i in range(n - 2, n + 3)]
        o = backend.process_frame(win, 2, prof, P)
        for c, p in zip("ybr", o):
            out[f"{video}_{n}_{c}"] = np.asarray(p, np.float32)
np.savez(%(dst)r, **out)
print("lib", nv_native.LIB_PATH, "frames", len(out) // 3, nv_native.STATS)
'''


def run(dst, tmp, lib, alt):
    env = dict(os.environ)
    for k in [k for k in env if k.startswith("NV_")]:
        env.pop(k)
    env["NV_NATIVE_LIB"] = lib
    if alt is not None:
        env["NV_LIBM_ALT"] = str(alt)
    code = CHILD % {"reference", "python": REPRO, "videos": VIDEOS, "tmp": tmp, "dst": dst}
    r = subprocess.run([sys.executable, "-c", code], env=env, capture_output=True, text=True, cwd=REPRO)
    if r.returncode:
        raise RuntimeError(r.stderr[-3000:])
    return r.stdout.strip()


def codes(x, k, top):
    return np.clip(np.rint(x * np.float32(k)), 0, top).astype(np.int64)


def main():
    out, tmp = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
    d = os.path.join(out, "frames_e2e")
    os.makedirs(d, exist_ok=True)
    runs = {"default": (os.path.join(NATIVE, "libnvcore.dylib"), None)}
    for a in (0, 1, 2, 3):
        runs[f"alt{a}"] = (os.path.join(NATIVE, "libm_alt", "bin", "libnvcore.dylib"), a)
    logs = {}
    for name, (lib, alt) in runs.items():
        logs[name] = run(os.path.join(d, f"{name}.npz"), tmp, lib, alt)
        print(f"[frames] {name}: {logs[name]}", flush=True)
    ref = np.load(os.path.join(d, "default.npz"))
    res = {"frames": {v: sorted({int(k.split('_')[1]) for k in ref.files if k.startswith(v + '_')}) for v in VIDEOS},
           "runs": logs, "alts": {}}
    for name in runs:
        if name == "default":
            continue
        z = np.load(os.path.join(d, f"{name}.npz"))
        R = {}
        for k in ref.files:
            plane = "Y" if k.endswith("_y") else ("Cb" if k.endswith("_b") else "Cr")
            video = k.split("_")[0]
            a, b = ref[k], z[k]
            e = R.setdefault(f"{video}.{plane}", {"values": 0, "diff_words": 0, "max_abs": 0.0, "max_ulp": 0,
                                                  "code8": 0, "code10": 0, "code16": 0, "frames_code8": 0})
            e["values"] += a.size
            m = a.view(np.uint32) != b.view(np.uint32)
            e["diff_words"] += int(m.sum())
            if m.any():
                e["max_abs"] = max(e["max_abs"], float(np.abs(a[m].astype(np.float64) - b[m]).max()))
                ia, ib = a[m].view(np.int32).astype(np.int64), b[m].view(np.int32).astype(np.int64)
                e["max_ulp"] = max(e["max_ulp"], int(np.abs(ia - ib).max()))   # same sign: pixel values >= 0
            c8 = int((codes(a, 1, 255) != codes(b, 1, 255)).sum())
            e["code8"] += c8
            e["frames_code8"] += c8 > 0
            e["code10"] += int((codes(a, 4, 1023) != codes(b, 4, 1023)).sum())
            e["code16"] += int((codes(a, 256, 65535) != codes(b, 256, 65535)).sum())
        res["alts"][name] = R
        print(f"[frames] {name}: " + "; ".join(f"{k} {v['diff_words']}/{v['values']} words, max {v['max_ulp']} ulp "
                                               f"({v['max_abs']:.3g}), 8-bit {v['code8']}, 10-bit {v['code10']}, "
                                               f"16-bit {v['code16']}" for k, v in R.items()), flush=True)
    with open(os.path.join(out, "frames_e2e.json"), "w") as f:
        json.dump(res, f, indent=1)


if __name__ == "__main__":
    main()
