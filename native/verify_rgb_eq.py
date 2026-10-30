# G1: the R/G/B equalizer in C++ (native/nv_aft.cpp color_weights / combine_levels / rgb_effective through
# nvcore_c_rgb.h) against the Python UI, BIT FOR BIT.
#
#   make -C native lib && python3 native/verify_rgb_eq.py [--quick]
#
# Groups:
#   combine    backend.rgb_combine vs nvc_rgb_combine: random levels (the clamp ends 0.2 / 3.0, values rounded to 3
#              decimals as the editor drags them, unit levels), colour weights of saved AFT profiles and of random
#              model q99 (some 0: the 1/3 branch), curves of saved profiles and random float32
#   effective  state.rgb_effective_profile vs nvc_rgb_effective_curve (curve, sigma) with random tune (all and per
#              channel), before / after AFT bases; the cases that return base unchanged are counted separately
#   weights    backend.rgb_weights vs nvc_rgb_weights on frames of test.mp4 / test_v02 (as the UI loads them):
#              FindFlatArea areas, random squares / rectangles, frame edges, tiny areas
#   aft        saved AFT profiles: rgb_combine of the pre-AFT curve with the profile's face curves' levels
#              reproduces base["curve"] (the AFT path of backend._run_aft), and the unit-level identity
import argparse
import ctypes
import glob
import json
import os
import struct
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
REPRO = os.path.join(ROOT, "repro")
for p in (REPRO, HERE):
    if p not in sys.path:
        sys.path.insert(0, p)
sys.dont_write_bytecode = True
os.environ.setdefault("NV_NATIVE", "0")
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
F32 = np.float32
LIB = os.environ.get("NV_NATIVE_LIB", os.path.join(HERE, "libnvcore.dylib"))
NAMES = ("Y", "Cb", "Cr")


def lib():
    L = ctypes.CDLL(LIB)
    P, I, D = ctypes.c_void_p, ctypes.c_int, ctypes.c_double
    L.nvc_rgb_weights.argtypes = [P, I, I, P, P]
    L.nvc_rgb_effective_curve.argtypes = [P, I, P, P, P, P, D, P, P, P]
    L.nvc_rgb_combine.argtypes = [P, I, P, P, P, P, P]
    for f in (L.nvc_rgb_weights, L.nvc_rgb_effective_curve, L.nvc_rgb_combine):
        f.restype = I
    return L


def dbits(x):
    return struct.pack("<d", float(x))


def c_combine(L, curve, centers, cw, levels):
    n = len(centers)
    cx = np.ascontiguousarray(centers, np.float64)
    cv = np.ascontiguousarray([float(v) for c in NAMES for v in curve[c]], np.float64)
    cw9 = np.ascontiguousarray([float(v) for r in cw for v in r], np.float64)
    lv = np.ascontiguousarray([float(v) for r in levels for v in r], np.float64)
    face, out = np.zeros(3 * n), np.zeros(3 * n)
    assert L.nvc_rgb_combine(cx.ctypes.data, n, cv.ctypes.data, cw9.ctypes.data, lv.ctypes.data, face.ctypes.data,
                             out.ctypes.data) == 0
    return face.reshape(3, n), out.reshape(3, n)


def c_effective(L, base, levels, tune):
    aft = bool(base.get("aft_applied"))
    src = base["ui_pre_aft"]["curve"] if aft else base["curve"]
    cw = base["aft_color_weights"] if aft else base["ui_rgb_weights"]
    n = len(base["curve_centers"])
    cx = np.ascontiguousarray(base["curve_centers"], np.float64)
    cv = np.ascontiguousarray([float(v) for c in NAMES for v in src[c]], np.float64)
    cw9 = np.ascontiguousarray([float(v) for r in cw for v in r], np.float64)
    lv = np.ascontiguousarray([float(v) for r in levels for v in r], np.float64)
    t4 = np.ascontiguousarray([float(tune.get(k, 1.0)) for k in ("all", "Y", "Cb", "Cr")], np.float64)
    tot = np.ascontiguousarray([float(base["tot_ratio"][c]) for c in NAMES], np.float64)
    out, sig = np.zeros(3 * n), np.zeros(3)
    assert L.nvc_rgb_effective_curve(cx.ctypes.data, n, cv.ctypes.data, cw9.ctypes.data, lv.ctypes.data, t4.ctypes.data,
                                     float(base.get("area_level", 128.0)), tot.ctypes.data, out.ctypes.data,
                                     sig.ctypes.data) == 0
    return out.reshape(3, n), sig


def saved_bases():
    out, seen = [], set()
    for f in sorted(glob.glob(os.path.join(ROOT, "**", "*.json"), recursive=True)):
        if os.path.getsize(f) > 2_000_000 or not ("prof" in os.path.basename(f)):
            continue
        try:
            d = json.load(open(f))
        except Exception:      # noqa: BLE001
            continue
        b = d.get("base") if isinstance(d, dict) and "base" in d else d
        if isinstance(b, dict) and "curve" in b and "curve_centers" in b and "tot_ratio" in b:
            k = json.dumps([b["curve"], sorted(b)], sort_keys=True)
            if k not in seen:
                seen.add(k)
                out.append(b)
    return out


def rand_levels(rng):
    kind = rng.integers(5)
    if kind == 0:
        return [[1.0] * 9 for _ in range(3)]
    lv = rng.uniform(0.2, 3.0, (3, 9))
    if kind == 1:
        lv = np.round(lv, 3)
    if kind == 2:
        lv = np.where(rng.random((3, 9)) < 0.3, rng.choice([0.2, 3.0], (3, 9)), lv)
    if kind == 3:
        lv = np.where(rng.random((3, 9)) < 0.8, 1.0, np.round(lv, 3))
    return [[float(v) for v in r] for r in lv]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true")
    A = ap.parse_args()
    import nv_aft_v5
    from nv_ui_qt import backend, state
    L = lib()
    rng = np.random.default_rng(20261030)
    bases = saved_bases()
    cws = [b["aft_color_weights"] for b in bases if "aft_color_weights" in b]
    for _ in range(60 if A.quick else 600):
        q = rng.uniform(0.01, 40.0, 3)
        if rng.random() < 0.1:
            q[rng.integers(3)] = 0.0
        if rng.random() < 0.03:
            q[:] = 0.0
        cws.append(nv_aft_v5._color_weights([float(x) for x in q]))
    curves = [(b["curve"], b["curve_centers"]) for b in bases]
    for _ in range(40 if A.quick else 200):
        c = rng.uniform(0.0, 40.0, (3, 8)).astype(F32)
        curves.append(({n: c[i].tolist() for i, n in enumerate(NAMES)}, [16.0 + 32 * i for i in range(8)]))
    ok = bad = 0

    # combine
    for t in range(500 if A.quick else 5000):
        cw = cws[int(rng.integers(len(cws)))]
        cv, cx = curves[int(rng.integers(len(curves)))]
        lv = rand_levels(rng)
        faces, py = backend.rgb_combine(cv, cx, cw, lv)
        cf, cc = c_combine(L, cv, cx, cw, lv)
        same = all(dbits(a) == dbits(b) for j in range(3) for a, b in zip(faces[j].tolist(), cf[j])) and \
            all(dbits(float(a)) == dbits(b) for i, c in enumerate(NAMES) for a, b in zip(py[c].tolist(), cc[i]))
        ok, bad = ok + same, bad + (not same)
    print(f"combine: {ok}/{ok + bad}")
    tot_ok, tot_bad = ok, bad

    # effective: synthetic bases from saved ones (before AFT: ui_rgb_weights; after AFT: pre curve + AFT weights)
    ok = bad = unchanged = pre_cases = 0
    for t in range(400 if A.quick else 4000):
        b0 = bases[int(rng.integers(len(bases)))]
        b = json.loads(json.dumps(b0))
        cw = cws[int(rng.integers(len(cws)))]
        if b.get("aft_applied") and "ui_pre_aft" in b and rng.random() < 0.6:
            b["aft_color_weights"] = cw
            b.setdefault("aft_levels", [[1.0] * 9 for _ in range(3)])
        else:
            for k in ("aft_applied", "ui_pre_aft", "aft_color_weights", "aft_levels"):
                b.pop(k, None)
            b["ui_rgb_weights"] = cw
        lv = rand_levels(rng)
        tune = {}
        for k in ("all", "Y", "Cb", "Cr"):
            if rng.random() < 0.3:
                tune[k] = float(np.round(rng.uniform(0.2, 3.0), 2))
        py = state.rgb_effective_profile(b, lv, tune)
        if py is b:
            unchanged += 1
            continue
        tk1 = all(float(tune.get("all", 1.0)) * float(tune.get(c, 1.0)) == 1.0 for c in NAMES)
        if b.get("aft_applied") and lv == [[1.0] * 9] * 3 and tk1:
            # levels 1.0 after AFT: the state layer (both UIs) returns the curve and sigma before AFT as they were
            same = py["curve"] == b["ui_pre_aft"]["curve"] and py["sigma"] == b["ui_pre_aft"]["sigma"]
            pre_cases += 1
            ok, bad = ok + same, bad + (not same)
            continue
        cc, cs = c_effective(L, b, lv, tune)
        same = all(dbits(a) == dbits(c) for i, n in enumerate(NAMES) for a, c in zip(py["curve"][n], cc[i])) and \
            all(dbits(py["sigma"][n]) == dbits(cs[i]) for i, n in enumerate(NAMES))
        ok, bad = ok + same, bad + (not same)
    print(f"effective: {ok}/{ok + bad} (of which {pre_cases} unit levels after AFT = the curve before AFT; "
          f"+ {unchanged} returned base unchanged)")
    tot_ok, tot_bad = tot_ok + ok, tot_bad + bad

    # weights on real frames
    import denoise_video_v5 as dv
    import nvfilter as nv
    ok = bad = 0
    clips = [("test.mp4", (300, 450) if A.quick else (0, 300, 450, 750)),
             ("test_v02.mp4", (312,) if A.quick else (100, 312, 576))]
    for name, ns in clips:
        path = os.path.join(ROOT, name)
        info = dv.probe(path)
        W, H = info["W"], info["H"]
        mat = nv.matrix_from_tags(info["tags"], H)
        for n in ns:
            fr = [tuple(np.array(p) for p in f) for _, f in dv.read_frames(path, W, H, n, n + 1, info["fps"])][0]
            rgb = np.ascontiguousarray(np.clip(nv.yuv_to_rgb(tuple(np.asarray(p, F32) for p in fr), mat, False),
                                               0, 255).astype(F32))
            areas = [list(nv.find_flat_area(fr, False, mat))[:3]]
            for _ in range(3 if A.quick else 12):
                h = int(rng.integers(8, min(H, 512)))
                w = h if rng.random() < 0.5 else int(rng.integers(8, min(W, 512)))
                y0, x0 = int(rng.integers(0, H - h + 1)), int(rng.integers(0, W - w + 1))
                areas.append([y0, x0, h] if h == w else [y0, x0, h, w])
            areas += [[0, 0, 8], [H - 32, W - 48, 32, 48], [0, 0, H, W] if H <= 512 and W <= 512 else [0, 0, 64]]
            for a in areas:
                py = backend.rgb_weights(fr, a, False, mat)
                a4 = np.ascontiguousarray([a[0], a[1], a[2], a[3] if len(a) == 4 else a[2]], np.int32)
                c9 = np.zeros(9)
                assert L.nvc_rgb_weights(rgb.ctypes.data, H, W, a4.ctypes.data, c9.ctypes.data) == 0
                same = all(dbits(py[i][j]) == dbits(c9[3 * i + j]) for i in range(3) for j in range(3))
                ok, bad = ok + same, bad + (not same)
                if not same:
                    print("  weights mismatch", name, n, a)
    print(f"weights: {ok}/{ok + bad}")
    tot_ok, tot_bad = tot_ok + ok, tot_bad + bad

    # aft: saved AFT profiles (levels are not stored before G1: the identity through their face curves)
    ok = bad = 0
    for b in bases:
        if not (b.get("aft_applied") and "ui_pre_aft" in b and "aft_levels" in b) or "ui_factors" in b:
            continue                                   # (effective profiles of edits carry ui_factors)
        _, cur = backend.rgb_combine(b["ui_pre_aft"]["curve"], b["curve_centers"], b["aft_color_weights"],
                                     b["aft_levels"])
        same = all(np.array_equal(cur[c].view(np.uint32), np.asarray(b["curve"][c], F32).view(np.uint32)) for c in NAMES)
        ok, bad = ok + same, bad + (not same)
    print(f"aft (saved profiles with aft_levels): {ok}/{ok + bad}")
    tot_ok, tot_bad = tot_ok + ok, tot_bad + bad
    print(f"RESULT: {'ALL SAME' if tot_bad == 0 else 'MISMATCH'} ({tot_ok}/{tot_ok + tot_bad})")
    return 0 if tot_bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
