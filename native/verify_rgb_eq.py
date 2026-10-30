# G1: the R/G/B equalizer in C++ (native/nv_aft.cpp color_weights / combine_levels / rgb_effective through
# nvcore_c_rgb.h) against references written here from the processing core's nv_aft_v5.apply_aft (read only), BIT FOR
# BIT.  No PySide6 / repro/nv_ui_qt (C++-first policy, native/MIGRATION.md): the references copy apply_aft's
# operation order.
#
#   make -C native lib && python3 native/verify_rgb_eq.py [--quick] [--before-lib PATH]
#
# Groups:
#   combine    ref_combine (apply_aft steps 4-6 with the 9 R/G/B levels given) vs nvc_rgb_combine: random levels (the
#              clamp ends 0.2 / 3.0, values rounded to 3 decimals as the editor drags them, unit levels), colour weights
#              of the fixture profiles and of random model q99 (some 0: the 1/3 branch), fixture and random curves
#   effective  ref_effective (ref_combine on the curve before AFT, then x float32(tune), sigma as apply_aft) vs
#              nvc_rgb_effective_curve with random tune (all and per channel), before / after AFT bases.  (The cases
#              the state layer answers without computing - unchanged levels, unit levels after AFT - are qtui's,
#              qtui_selftest g1EditsResetComplete)
#   weights    ref_weights (nv_aft_v5._color_weights of nv_band.model_levels word 5 of the R/G/B crop) vs
#              nvc_rgb_weights on frames of test.mp4 / test_v02: FindFlatArea areas, random squares / rectangles,
#              frame edges, tiny areas
#   apply_aft  nv_aft_v5.apply_aft itself (Python) on a profile of nvfilter.build_profile_v5: its curve = nvc_rgb_combine
#              of the curve before AFT with the levels of the C AFT bytes (nvc_profile_build + nvc_aft), its colour
#              weights = nvc_rgb_weights of the same frame and area
#   legacy     the Y / Cb / Cr combination before G1 (nvc_aft_combine) on AFT bytes holding edited float32 R/G/B levels
#              = nvc_rgb_combine of the same levels; with --before-lib (a libnvcore.dylib built from the code before
#              G1) its nvc_aft_combine too
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
F32 = np.float32
LIB = os.environ.get("NV_NATIVE_LIB", os.path.join(HERE, "libnvcore.dylib"))
NAMES = ("Y", "Cb", "Cr")


def lib(path=LIB):
    L = ctypes.CDLL(path)
    P, I, D = ctypes.c_void_p, ctypes.c_int, ctypes.c_double
    L.nvc_aft_combine.argtypes = [P, P, P, I, P, D, P, I, P, P, P, P, P]
    L.nvc_aft_combine.restype = None
    if hasattr(L, "nvc_rgb_weights"):
        L.nvc_rgb_weights.argtypes = [P, I, I, P, P]
        L.nvc_rgb_effective_curve.argtypes = [P, I, P, P, P, P, D, P, P, P]
        L.nvc_rgb_combine.argtypes = [P, I, P, P, P, P, P]
        for f in (L.nvc_rgb_weights, L.nvc_rgb_effective_curve, L.nvc_rgb_combine):
            f.restype = I
        L.nvc_profile_build.argtypes = [P, I, I, I, I, I, I, I, P, I, I, P]
        L.nvc_profile_build.restype = I
        L.nvc_aft.argtypes = [P, I, I, P, I, I, P, P]
        L.nvc_aft.restype = I
    return L


# ------------------------------------------------------------------ references (nv_aft_v5.apply_aft's operation order)
def ref_combine(curve, centers, cw, levels):
    """apply_aft steps 4-6 with the 9 levels of each face given: (face curves, Y / Cb / Cr float32 curves)."""
    v5c = np.array(centers)
    raw_c = np.linspace(0, 256, 9)
    faces = [np.interp(v5c, raw_c, np.array([float(v) for v in levels[j]])) for j in range(3)]
    out = {}
    for ci, c in enumerate(NAMES):
        ws = sum(cw[ci][j] * faces[j] for j in range(3))
        out[c] = (np.array(curve[c], F32) * np.sqrt(np.maximum(ws, F32(0)))).astype(F32)
    return faces, out


def ref_effective(base, levels, tune):
    """The R/G/B equalizer's computed path (qtui state rgb_effective_profile): ref_combine on the curve before AFT
    (base curve before AFT), each curve x float32(tune_all x tune_c), sigma = interp(area_level) x tot_ratio."""
    aft = bool(base.get("aft_applied"))
    src = base["ui_pre_aft"]["curve"] if aft else base["curve"]
    cw = base["aft_color_weights"] if aft else base["ui_rgb_weights"]
    _, cur = ref_combine(src, base["curve_centers"], cw, levels)
    tk = {c: float(tune.get("all", 1.0)) * float(tune.get(c, 1.0)) for c in NAMES}
    curve = {c: (cur[c] * F32(tk[c])).tolist() for c in NAMES}
    lvl = base.get("area_level", 128.0)
    sigma = {c: float(np.interp(lvl, base["curve_centers"], curve[c]) * base["tot_ratio"][c]) for c in NAMES}
    return curve, sigma


def ref_weights(rgb, area):
    """apply_aft's colour weights: _color_weights of the model q99 (word 5) of the R/G/B crop (nv_profile faces
    0..2, whose words 0..54 are nv_band.model_levels)."""
    import nv_aft_v5
    import nv_band as nb
    y0, x0, h = (int(v) for v in area[:3])
    w = int(area[3]) if len(area) == 4 else h
    crop = rgb[y0:y0 + h, x0:x0 + w]
    q99 = [float(np.asarray(nb.model_levels(np.ascontiguousarray(crop[..., j])), F32)[5]) for j in range(3)]
    return nv_aft_v5._color_weights(q99)


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
    """The fixture profiles (qtui/tests/fixtures: Python-made, before G1), as bases."""
    out, seen = [], set()
    for f in sorted(glob.glob(os.path.join(ROOT, "qtui", "tests", "fixtures", "*prof*.json"))):
        d = json.load(open(f))
        b = d.get("base") if isinstance(d, dict) and "base" in d else d
        if isinstance(b, dict) and "curve" in b and "curve_centers" in b and "tot_ratio" in b:
            k = json.dumps([b["curve"], sorted(b)], sort_keys=True)
            if k not in seen:
                seen.add(k)
                out.append(b)
    return out


def c_blobs(L, rgb, area):
    """The raw profile and the AFT bytes of the C path (nvcore run_aft: nvc_profile_build + nvc_aft)."""
    H, W = rgb.shape[:2]
    y0, x0, h = (int(v) for v in area[:3])
    w = int(area[3]) if len(area) == 4 else h
    floor = np.ascontiguousarray(np.load(os.path.join(ROOT, "data", "ref_floor.npy")).astype(F32))
    raw = np.zeros(0xc300, np.uint8)
    assert L.nvc_profile_build(rgb.ctypes.data, W, H, 1, x0, y0, w, h, floor.ctypes.data, 1, 1, raw.ctypes.data) == 0
    aft = raw.copy()
    out3 = np.zeros(3, F32)
    assert L.nvc_aft(rgb.ctypes.data, W, H, aft.ctypes.data, 0, 1, None, out3.ctypes.data) in (0, 1)
    return raw, aft


def aft_levels(aft):
    f = aft.view(F32)
    return [[float(f[(0xbf44 + 0x120 * j + 0x20 * k) // 4]) for k in range(9)] for j in range(3)]


def c_aft_combine(L, raw, aft, curve, centers, area_level, tot):
    n = len(centers)
    cx = np.ascontiguousarray(centers, np.float64)
    cv = np.ascontiguousarray([float(v) for c in NAMES for v in curve[c]], F32)
    t3 = np.ascontiguousarray([float(tot[c]) for c in NAMES], np.float64)
    cw, face, out, sig, flags = np.zeros(9), np.zeros(3 * n), np.zeros(3 * n, F32), np.zeros(3), np.zeros(81, np.uint8)
    L.nvc_aft_combine(raw.ctypes.data, aft.ctypes.data, cx.ctypes.data, n, cv.ctypes.data, float(area_level),
                      t3.ctypes.data, 0, cw.ctypes.data, face.ctypes.data, out.ctypes.data, sig.ctypes.data,
                      flags.ctypes.data)
    return out.reshape(3, n), cw.reshape(3, 3)


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
    ap.add_argument("--before-lib", help="libnvcore.dylib of the code before G1 (legacy group)")
    A = ap.parse_args()
    import copy
    import nv_aft_v5
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
    tot_ok = tot_bad = 0

    def tally(name, ok, bad, extra=""):
        nonlocal tot_ok, tot_bad
        print(f"{name}: {ok}/{ok + bad}{extra}", flush=True)
        tot_ok, tot_bad = tot_ok + ok, tot_bad + bad

    # combine
    ok = bad = 0
    for t in range(500 if A.quick else 5000):
        cw = cws[int(rng.integers(len(cws)))]
        cv, cx = curves[int(rng.integers(len(curves)))]
        lv = rand_levels(rng)
        faces, py = ref_combine(cv, cx, cw, lv)
        cf, cc = c_combine(L, cv, cx, cw, lv)
        same = all(dbits(a) == dbits(b) for j in range(3) for a, b in zip(faces[j].tolist(), cf[j])) and \
            all(dbits(float(a)) == dbits(b) for i, c in enumerate(NAMES) for a, b in zip(py[c].tolist(), cc[i]))
        ok, bad = ok + same, bad + (not same)
    tally("combine", ok, bad)

    # effective: synthetic bases from the fixtures (before AFT: ui_rgb_weights; after AFT: pre curve + AFT weights)
    ok = bad = 0
    for t in range(400 if A.quick else 4000):
        b = json.loads(json.dumps(bases[int(rng.integers(len(bases)))]))
        cw = cws[int(rng.integers(len(cws)))]
        if b.get("aft_applied") and "ui_pre_aft" in b and rng.random() < 0.6:
            b["aft_color_weights"] = cw
        else:
            for k in ("aft_applied", "ui_pre_aft", "aft_color_weights", "aft_levels"):
                b.pop(k, None)
            b["ui_rgb_weights"] = cw
        lv = rand_levels(rng)
        tune = {k: float(np.round(rng.uniform(0.2, 3.0), 2)) for k in ("all", "Y", "Cb", "Cr") if rng.random() < 0.3}
        pc, ps = ref_effective(b, lv, tune)
        cc, cs = c_effective(L, b, lv, tune)
        same = all(dbits(a) == dbits(c) for i, n in enumerate(NAMES) for a, c in zip(pc[n], cc[i])) and \
            all(dbits(ps[n]) == dbits(cs[i]) for i, n in enumerate(NAMES))
        ok, bad = ok + same, bad + (not same)
    tally("effective", ok, bad)

    # weights on real frames (the RGB the UI models: np.clip(yuv_to_rgb(..), 0, 255))
    import denoise_video_v5 as dv
    import nvfilter as nv
    ok = bad = 0
    frames = {}
    clips = [("test.mp4", (300, 450) if A.quick else (0, 300, 450, 750)),
             ("test_v02.mp4", (312,) if A.quick else (100, 312, 576))]
    for name, ns in clips:
        path = os.path.join(ROOT, name)
        info = dv.probe(path)
        W, H = info["W"], info["H"]
        mat = nv.matrix_from_tags(info["tags"], H)
        for n in ns:
            fr = [tuple(np.array(p) for p in f) for _, f in dv.read_frames(path, W, H, n, n + 3, info["fps"])]
            rgb = np.ascontiguousarray(np.clip(nv.yuv_to_rgb(tuple(np.asarray(p, F32) for p in fr[0]), mat, False),
                                               0, 255).astype(F32))
            frames[(name, n)] = (fr, rgb, mat)
            areas = [list(nv.find_flat_area(fr[0], False, mat))[:3]]
            for _ in range(3 if A.quick else 12):
                h = int(rng.integers(8, min(H, 512)))
                w = h if rng.random() < 0.5 else int(rng.integers(8, min(W, 512)))
                y0, x0 = int(rng.integers(0, H - h + 1)), int(rng.integers(0, W - w + 1))
                areas.append([y0, x0, h] if h == w else [y0, x0, h, w])
            areas += [[0, 0, 8], [H - 32, W - 48, 32, 48], [0, 0, H, W] if H <= 512 and W <= 512 else [0, 0, 64]]
            for a in areas:
                py = ref_weights(rgb, a)
                a4 = np.ascontiguousarray([a[0], a[1], a[2], a[3] if len(a) == 4 else a[2]], np.int32)
                c9 = np.zeros(9)
                assert L.nvc_rgb_weights(rgb.ctypes.data, H, W, a4.ctypes.data, c9.ctypes.data) == 0
                same = all(dbits(py[i][j]) == dbits(c9[3 * i + j]) for i in range(3) for j in range(3))
                ok, bad = ok + same, bad + (not same)
                if not same:
                    print("  weights mismatch", name, n, a)
    tally("weights", ok, bad)

    # apply_aft itself (Python AFT, test.mp4 only: several seconds per frame) and the legacy combination
    ok = bad = lok = lbad = 0
    LB = lib(A.before_lib) if A.before_lib else None
    for key in [k for k in frames if k[0] == "test.mp4"][:1 if A.quick else 2]:
        fr, rgb, mat = frames[key]
        prof = nv.build_profile_v5(fr[0], False, pair=fr[1], mode="auto", more=fr[1:], matrix=mat)
        ref = nv_aft_v5.apply_aft(copy.deepcopy(prof), rgb)
        a = prof["area"]
        raw, aft = c_blobs(L, rgb, a)
        lv = aft_levels(aft)
        _, cc = c_combine(L, prof["curve"], prof["curve_centers"], ref["aft_color_weights"], lv)
        a4 = np.ascontiguousarray([a[0], a[1], a[2], a[3] if len(a) == 4 else a[2]], np.int32)
        c9 = np.zeros(9)
        assert L.nvc_rgb_weights(rgb.ctypes.data, rgb.shape[0], rgb.shape[1], a4.ctypes.data, c9.ctypes.data) == 0
        same = all(dbits(a_) == dbits(b_) for i, c in enumerate(NAMES) for a_, b_ in zip(ref["curve"][c], cc[i])) and \
            all(dbits(ref["aft_color_weights"][i][j]) == dbits(c9[3 * i + j]) for i in range(3) for j in range(3)) and \
            ref["aft_face_curves"] == [np.interp(np.array(prof["curve_centers"]), np.linspace(0, 256, 9),
                                                 np.array(lv[j])).tolist() for j in range(3)]
        ok, bad = ok + same, bad + (not same)
        # legacy: edited float32 levels written into the AFT bytes -> the combination before G1
        for t in range(10 if A.quick else 60):
            ed = [list(r) for r in lv]
            for _ in range(int(rng.integers(1, 10))):
                ed[int(rng.integers(3))][int(rng.integers(9))] = float(F32(rng.choice([1.2, 0.7, 0.2, 3.0, rng.uniform(0.2, 3.0)])))
            mod = aft.copy()
            f = mod.view(F32)
            for j in range(3):
                for k in range(9):
                    f[(0xbf44 + 0x120 * j + 0x20 * k) // 4] = ed[j][k]
            _, new = c_combine(L, prof["curve"], prof["curve_centers"], ref["aft_color_weights"], ed)
            libs = [L] + ([LB] if LB is not None else [])
            same = True
            for lb in libs:
                old, cw = c_aft_combine(lb, raw, mod, prof["curve"], prof["curve_centers"], prof.get("area_level", 128.0),
                                        prof["tot_ratio"])
                same &= all(dbits(float(x)) == dbits(y) for i in range(3) for x, y in zip(old[i], new[i]))
            lok, lbad = lok + same, lbad + (not same)
    tally("apply_aft", ok, bad)
    tally("legacy", lok, lbad, f" (nvc_aft_combine of {'this and the before-G1 library' if LB else 'this library'})")
    print(f"RESULT: {'ALL SAME' if tot_bad == 0 else 'MISMATCH'} ({tot_ok}/{tot_ok + tot_bad})")
    return 0 if tot_bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
