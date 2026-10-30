#!/usr/bin/env python3
"""Verifier's own end-to-end frame comparison (W2a item 4b) on frames the worker did not use.

    python3 frames_indep.py OUTDIR

Decodes test_v02 n = 198..202 and 498..502, test.mp4 n = 98..102 and 698..702 (denoise_video_v5.read_frames,
yuv420p uint8 as the UI reads them), processes the centre frames (n = 200, 500, 100, 700) with
nv_ui_qt.backend.process_frame (UI defaults, radius 2, native core on) using the AFT profiles the bridge
stage cached (aft_v02_312.json / aft_t_302.json: Python path, system libm), once with the default
libnvcore.dylib and once with libm_alt/bin/libnvcore.dylib at NV_LIBM_ALT = 0..3, and counts the float32
words and the 8 / 10 / 16-bit export codes that differ.  Independent of the worker's frames_e2e.py.
"""
import json
import os
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))
NATIVE = os.path.join(ROOT, "native")
REPRO = os.path.join(ROOT, "repro")
BRIDGE_TMP = os.path.join(ROOT, "sandbox", "omo", "wp_native_libm", "runs", "bridge", "tmp_alt0")
VIDEOS = {"v02": (os.path.join(ROOT, "test_v02.mp4"), [200, 500], "bt709", "aft_v02_312.json"),
          "t": (os.path.join(ROOT, "test.mp4"), [100, 700], "bt601", "aft_t_302.json")}

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
for video, (npz, profj) in %(inputs)r.items():
    z = np.load(npz)
    fr = {int(k[1:]): None for k in z.files if k.startswith("y")}
    for n in list(fr):
        fr[n] = tuple(z[c + str(n)].astype(np.float32) for c in "ybr")
    with open(profj) as f:
        prof = json.load(f)
    for n in sorted(fr):
        if n - 2 not in fr or n + 2 not in fr:
            continue
        win = [fr[i] for i in range(n - 2, n + 3)]
        o = backend.process_frame(win, 2, prof, P)
        for c, p in zip("ybr", o):
            out[f"{video}_{n}_{c}"] = np.asarray(p, np.float32)
np.savez(%(dst)r, **out)
print("lib", nv_native.LIB_PATH, "frames", len(out) // 3, nv_native.STATS)
'''


def decode(outdir):
    sys.path.insert(0, REPRO)
    from denoise_video_v5 import probe, read_frames
    inputs = {}
    for video, (path, centres, matrix, profj) in VIDEOS.items():
        npz = os.path.join(outdir, f"frames_{video}.npz")
        if not os.path.exists(npz):
            info = probe(path)
            d = {}
            for n0 in centres:
                for i, f in read_frames(path, info["W"], info["H"], n0 - 2, n0 + 3, info["fps"]):
                    for c, p in zip("ybr", f):
                        d[f"{c}{i}"] = np.array(p)
            np.savez(npz, **d)
        inputs[video] = (npz, os.path.join(BRIDGE_TMP, profj))
    return inputs


def run(dst, inputs, lib, alt):
    env = {k: v for k, v in os.environ.items() if not k.startswith("NV_")}
    env["NV_NATIVE_LIB"] = lib
    if alt is not None:
        env["NV_LIBM_ALT"] = str(alt)
    code = CHILD % {"repro": REPRO, "inputs": inputs, "dst": dst}
    r = subprocess.run([sys.executable, "-c", code], env=env, capture_output=True, text=True, cwd=REPRO)
    if r.returncode:
        raise RuntimeError(r.stderr[-3000:])
    return r.stdout.strip()


def codes(x, k, top):
    return np.clip(np.rint(x * np.float32(k)), 0, top).astype(np.int64)


def main():
    outdir = os.path.abspath(sys.argv[1])
    os.makedirs(outdir, exist_ok=True)
    inputs = decode(outdir)
    runs = {"default": (os.path.join(NATIVE, "libnvcore.dylib"), None)}
    for a in (0, 1, 2, 3):
        runs[f"alt{a}"] = (os.path.join(NATIVE, "libm_alt", "bin", "libnvcore.dylib"), a)
    res = {"runs": {}, "alts": {}}
    for name, (lib, alt) in runs.items():
        res["runs"][name] = run(os.path.join(outdir, f"{name}.npz"), inputs, lib, alt)
        print(f"[frames] {name}: {res['runs'][name]}", flush=True)
    ref = np.load(os.path.join(outdir, "default.npz"))
    print("frames:", sorted(ref.files))
    for name in runs:
        if name == "default":
            continue
        z = np.load(os.path.join(outdir, f"{name}.npz"))
        R = {}
        for k in sorted(ref.files):
            a, b = ref[k], z[k]
            assert a.shape == b.shape
            m = a.view(np.uint32) != b.view(np.uint32)
            e = {"values": int(a.size), "diff_words": int(m.sum()), "max_abs": 0.0, "max_ulp": 0}
            if m.any():
                e["max_abs"] = float(np.abs(a[m].astype(np.float64) - b[m]).max())
                ia, ib = a[m].view(np.int32).astype(np.int64), b[m].view(np.int32).astype(np.int64)
                e["max_ulp"] = int(np.abs(ia - ib).max())
            for bits, kk in ((8, 1), (10, 4), (16, 256)):
                ca, cb = codes(a, kk, 2 ** bits - 1), codes(b, kk, 2 ** bits - 1)
                dd = np.abs(ca - cb)
                e[f"code{bits}"] = int((dd > 0).sum())
                e[f"code{bits}_max"] = int(dd.max())
            R[k] = e
        res["alts"][name] = R
        tot8 = sum(v["code8"] for v in R.values())
        print(f"[frames] {name}: 8-bit changed {tot8}; " + "; ".join(
            f"{k} {v['diff_words']}/{v['values']} ({100.0 * v['diff_words'] / v['values']:.2f}%) max {v['max_ulp']} ulp "
            f"8b {v['code8']}(max {v['code8_max']}) 10b {v['code10']} 16b {v['code16']}" for k, v in R.items()), flush=True)
    with open(os.path.join(outdir, "frames_indep.json"), "w") as f:
        json.dump(res, f, indent=1)


if __name__ == "__main__":
    main()
