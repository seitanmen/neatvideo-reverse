# N3a: the UI's profile building in C++ (native/nv_prof5.cpp, nv_lstsq.cpp through nvcore_c_prof5.h) against
# the Python code, BIT FOR BIT (every leaf of the profile dict: keys in order, Python types, float bits).
#
#   make -C native lib && python3 native/verify_prof5.py [--jobs N] [--quick] [--only GROUP,..] [--tmp DIR]
#
# Groups:
#   lstsq    numpy.polyfit on every _fill_curve system (all index subsets of 0..7, deg 1 / 2, and the end fits)
#            x 7 value laws, against nv_lstsq through a test entry (the curve values' path)
#   small    _wn_band_std (levels 1..8), backend.noisiest_frame's candidates (n = 0..400), hh1_sigma (random
#            planes, odd sizes, tiny planes), state.effective_profile (random factors / tune, unit cases)
#   synth    nvfilter.build_profile_v5 on synthetic frames: noise + gradients / blobs, black bars, blown
#            highlights, constant frames (noise-free), 4:2:0 / 4:4:4, odd sizes, frames under 64 px (gradient
#            fallback), full range, BT.709 / 601 / 2020, modes auto / spatial / temporal with and without pair /
#            more, areas None / square / rectangle (clamped)
#   real     test_v02 n = 306..320 (BT.709) and test.mp4 n = 298..308 (BT.601) as the UI loads them
#            (native/verify_bridge.frames): backend.profile_window's planes / pair / more, modes x areas
# The reference is the Python code with the native core off (nvfilter.build_profile_v5, whose FindFlatArea is
# the Python port; repro/nv_native.find_flat_area gives the same search, native/verify_bridge.py).
import argparse
import ctypes
import itertools
import os
import subprocess
import sys
import time
import warnings
from concurrent.futures import ProcessPoolExecutor

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
REPRO = os.path.join(ROOT, "reference", "python")
for p in (REPRO, HERE):
    if p not in sys.path:
        sys.path.insert(0, p)
sys.dont_write_bytecode = True
os.environ.setdefault("NV_NATIVE", "0")
F32 = np.float32
LIB = os.environ.get("NV_NATIVE_LIB", os.path.join(HERE, "libnvcore.dylib"))
MATRIX = {"bt709": 0, "bt601": 1, "bt2020": 2}
MODES = {"auto": 0, "spatial": 1, "temporal": 2}
MODE_NAMES = ("auto", "spatial", "temporal")
NAMES = ("Y", "Cb", "Cr")

_lib = None


def lib():
    global _lib
    if _lib is None:
        L = ctypes.CDLL(LIB)
        P, I, D = ctypes.c_void_p, ctypes.c_int, ctypes.c_double
        L.nvc_build_profile_v5.argtypes = [P, I, I, I, I, P, I, P, I, I, I, I, I, P, I, P, P, P]
        L.nvc_build_profile_v5.restype = I
        L.nvc_wn_band_std.argtypes = [I, P]
        L.nvc_hh1_sigma.argtypes = [P, I, I, P]
        L.nvc_noisiest_candidates.argtypes = [I, P]
        L.nvc_effective_curve.argtypes = [P, I, P, P, P, D, P, P, P]
        _lib = L
    return _lib


CANCEL_FN = ctypes.CFUNCTYPE(ctypes.c_int)


def _ptrs(arrs):
    return (ctypes.c_void_p * len(arrs))(*[a.ctypes.data for a in arrs])


# ----------------------------------------------------------------------------------- C ABI wrappers
def c_build(planes, full_range=False, area=None, pair=None, mode="auto", levels=5, more=(), matrix="bt709",
            cancel=None, threads=1):
    """nvc_build_profile_v5 -> the Python dict of nvfilter.build_profile_v5 (or 'cancelled' / an error code)."""
    pl = [np.ascontiguousarray(p, F32) for p in planes]
    H, W = pl[0].shape
    Hc, Wc = pl[1].shape
    pp = [np.ascontiguousarray(p, F32) for p in pair] if pair is not None else None
    mm = [np.ascontiguousarray(p, F32) for f in more for p in f]
    ar = np.array(list(area) if area is not None else [0], np.int32)
    oi = np.zeros(16, np.int32)
    od = np.zeros(72, np.float64)
    cb = CANCEL_FN() if cancel is None else CANCEL_FN(lambda: 1 if cancel() else 0)
    keep = (pl, pp, mm, ar)  # noqa: F841
    r = lib().nvc_build_profile_v5(_ptrs(pl), H, W, Hc, Wc, _ptrs(pp) if pp else None, len(more),
                                   _ptrs(mm) if mm else None, int(bool(full_range)), MATRIX[matrix], MODES[mode],
                                   levels, 0 if area is None else len(area), ar.ctypes.data, threads, cb,
                                   oi.ctypes.data, od.ctypes.data)
    if r == 5:
        return "cancelled"
    if r != 0:
        return f"error {r}"
    return to_dict(oi, od)


def to_dict(oi, od):
    """The prof dict from the out_i / out_d layout of nvcore_c_prof5.h (Python key order and types)."""
    an = int(oi[0])
    area = [int(v) for v in oi[1:1 + an]]
    lev = [int(v) for v in oi[7:10]]
    mode = MODE_NAMES[int(oi[5])] + ("+noise-free" if oi[6] else "")
    n = int(oi[14])
    prof = {"version": 5, "sub": int(oi[12]), "full_range": bool(oi[13]), "area": area, "mode": mode,
            "levels": {c: lev[i] for i, c in enumerate(NAMES)},
            "band_ratio": {c: [float(v) for v in od[5 * i:5 * i + lev[i]]] for i, c in enumerate(NAMES)},
            "tot_ratio": {c: float(od[15 + i]) for i, c in enumerate(NAMES)},
            "area_sigma": {c: [float(v) for v in od[18 + 5 * i:18 + 5 * i + lev[i]]] for i, c in enumerate(NAMES)},
            "curve": {c: [float(v) for v in od[33 + 8 * i:33 + 8 * i + n]] for i, c in enumerate(NAMES)},
            "curve_centers": [float(v) for v in od[57:57 + n]],
            "flat_status": None if oi[10] == -1 else int(oi[10])}
    if oi[11]:
        prof["static_fraction"] = float(od[65])
    prof["dead_fraction"] = float(od[66])
    prof["area_level"] = float(od[67])
    prof["sigma"] = {c: float(od[68 + i]) for i, c in enumerate(NAMES)}
    return prof


def c_wn(levels):
    out = np.zeros(levels)
    lib().nvc_wn_band_std(levels, out.ctypes.data)
    return [float(v) for v in out]


def c_hh1(Y):
    Y = np.ascontiguousarray(Y, F32)
    out = np.zeros(1)
    r = lib().nvc_hh1_sigma(Y.ctypes.data, Y.shape[0], Y.shape[1], out.ctypes.data)
    return float(out[0]) if r == 1 else None


def c_candidates(n):
    out = np.zeros(64, np.int32)
    k = lib().nvc_noisiest_candidates(n, out.ctypes.data)
    return [int(v) for v in out[:k]]


def c_noisiest(frames):
    best, best_s = None, -1.0
    cand = c_candidates(len(frames))
    if not cand:
        return 0
    best = cand[0]
    for i in cand:
        s = c_hh1(frames[i][0])
        if s > best_s:
            best, best_s = i, s
    return best


def c_effective(base, factors, tune):
    """state.effective_profile(base, factors, tune) through nvc_effective_curve (the dict like Python's)."""
    import copy
    n = len(base["curve_centers"])
    cen = np.array(base["curve_centers"], np.float64)
    cur = np.array([base["curve"][c] for c in NAMES], np.float64).ravel()
    fac = np.array(factors, np.float64).ravel()
    tu = np.array([float(tune.get(k, 1.0)) for k in ("all", "Y", "Cb", "Cr")], np.float64)
    tot = np.array([base["tot_ratio"][c] for c in NAMES], np.float64)
    co, so = np.zeros(3 * n), np.zeros(3)
    r = lib().nvc_effective_curve(cen.ctypes.data, n, cur.ctypes.data, fac.ctypes.data, tu.ctypes.data,
                                  float(base.get("area_level", 128.0)), tot.ctypes.data, co.ctypes.data, so.ctypes.data)
    if r == 0:
        return base
    prof = copy.deepcopy(base)
    for i, c in enumerate(NAMES):
        prof["curve"][c] = [float(v) for v in co[i * n:(i + 1) * n]]
    prof["sigma"] = {c: float(so[i]) for i, c in enumerate(NAMES)}
    prof["ui_factors"] = [list(map(float, fs)) for fs in factors]
    prof["ui_tune"] = dict(tune)
    return prof


# ----------------------------------------------------------------------------------- compare
def compare(a, b):
    from verify_bridge import compare as cmp
    return cmp(a, b)


def py_build(planes, full_range=False, area=None, pair=None, mode="auto", levels=5, more=(), matrix="bt709"):
    import nvfilter as nv
    try:
        return nv.build_profile_v5(tuple(np.asarray(p, F32) for p in planes), full_range, area, pair, mode, levels,
                                   more, matrix)
    except nv.ProfileCancelled:
        return "cancelled"


# ----------------------------------------------------------------------------------- synthetic frames
def synth(kind, H, W, sub, seed, full_range=False):
    rng = np.random.default_rng(seed)
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float64)
    if kind == "noise":
        Y = 120 + 30 * np.sin(xx / 37.0) + 20 * np.cos(yy / 23.0) + rng.normal(0, 3.0, (H, W))
    elif kind == "flatnoise":
        Y = 100 + rng.normal(0, 2.0, (H, W))
    elif kind == "bars":
        Y = 110 + rng.normal(0, 2.5, (H, W))
        Y[: H // 6] = 16
        Y[-H // 6:] = 16
    elif kind == "blown":
        Y = 200 + 60 * (xx / W) + rng.normal(0, 2.0, (H, W))
    elif kind == "const":
        Y = np.full((H, W), 90.0)
    elif kind == "dark":
        Y = 20 + 10 * (yy / H) + rng.normal(0, 1.5, (H, W))
    else:                          # "blobs": large structures + coloured noise
        Y = 128 + 50 * np.sin(xx / 9.0) * np.cos(yy / 13.0) + rng.normal(0, 4.0, (H, W))
    Y = np.clip(np.rint(Y), 0, 255).astype(np.uint8)
    if sub == 2:
        Hc, Wc = (H + 1) // 2, (W + 1) // 2
    else:
        Hc, Wc = H, W
    cb = np.clip(np.rint(128 + rng.normal(0, 1.5, (Hc, Wc)) + (5 if kind == "blobs" else 0)), 0, 255).astype(np.uint8)
    cr = np.clip(np.rint(128 + rng.normal(0, 1.2, (Hc, Wc))), 0, 255).astype(np.uint8)
    if kind == "const":
        cb[:] = 128
        cr[:] = 128
    return Y, cb, cr


def next_frame(f, seed):
    """A following frame: same scene, new noise (static pixels for the temporal profile)."""
    rng = np.random.default_rng(seed)
    out = []
    for p in f:
        q = p.astype(np.float64) + rng.normal(0, 1.5, p.shape)
        out.append(np.clip(np.rint(q), 0, 255).astype(np.uint8))
    return tuple(out)


def synth_cases(quick):
    cases = []
    sizes = [(36, 50, 2), (48, 48, 2), (64, 64, 2), (65, 97, 2), (100, 130, 2), (128, 128, 1), (131, 77, 1),
             (160, 240, 2), (200, 300, 2), (270, 480, 2)]
    kinds = ["noise", "flatnoise", "bars", "blown", "const", "dark", "blobs"]
    k = 0
    for (H, W, sub), kind in itertools.product(sizes, kinds):
        k += 1
        if quick and k % 4:
            continue
        fr = bool(k % 5 == 0)
        mat = ("bt709", "bt601", "bt2020")[k % 3]
        f0 = synth(kind, H, W, sub, k)
        f1, f2 = next_frame(f0, 1000 + k), next_frame(f0, 2000 + k)
        sq = max(8, min(H, W) // 2)
        for mode, area, use_more in (("auto", None, 2), ("spatial", None, 0), ("temporal", None, 1),
                                     ("auto", (H // 4, W // 5, sq), 2), ("spatial", (3, 5, sq + 3, sq // 2 + 9), 1),
                                     ("temporal", (-4, W, 1000), 2), ("auto", (2, 3, 20, 20), 0)):
            more = [f1, f2][:use_more]
            cases.append(("synth", f"{kind} {H}x{W} s{sub} {mat}{' fr' if fr else ''} {mode} {area} m{use_more}",
                          f0, more, fr, mat, mode, area))
    return cases


def real_cases(quick, tmp):
    os.environ["NV_BRIDGE_TMP"] = tmp
    import verify_bridge as vb
    vb.TMP = tmp
    os.makedirs(tmp, exist_ok=True)
    out = []
    for video, ns, mat in (("v02", (306, 310, 312, 316, 318), "bt709"), ("t", (298, 300, 304, 306), "bt601")):
        fr = vb.frames(video)
        lo = min(fr)
        frames = [fr[n] for n in sorted(fr)]
        for n in (ns[:2] if quick else ns):
            f = n - lo
            more = [frames[i] for i in range(f + 1, min(f + 3, len(frames)))]
            H, W = frames[f][0].shape
            for mode, area in (("auto", None), ("spatial", None), ("temporal", None),
                               ("auto", (H // 3, W // 4, 160)), ("auto", (40, 60, 216, 512)),
                               ("spatial", (0, 0, 96, 96)), ("temporal", (H - 50, W - 70, 128, 128))):
                if quick and area is not None and mode != "auto":
                    continue
                out.append(("real", f"{video} n={n} {mode} {area} m{len(more)}", frames[f], more, False, mat, mode,
                            area))
    return out


def run_case(case):
    group, label, f0, more, fr, mat, mode, area = case
    planes = tuple(np.asarray(p, F32) for p in f0)
    mo = [tuple(np.asarray(p, F32) for p in m) for m in more]
    pair = mo[0] if mo else None
    t0 = time.perf_counter()
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        ref = py_build(planes, fr, area, pair, mode, 5, mo, mat)
    t1 = time.perf_counter()
    threads = 1 + 3 * (len(label) % 2)          # -j 1 / 4 alternately (the results must not depend on it)
    got = c_build(planes, fr, area, pair, mode, 5, mo, mat, threads=threads)
    t2 = time.perf_counter()
    w, d, first = compare(ref, got)
    return group, f"{label} -j{threads}", w, d, first, t1 - t0, t2 - t1


# ----------------------------------------------------------------------------------- lstsq group
LSQ_SRC = r'''
#include <cstdio>
#include "nv_lstsq.h"
int main() {
    int n, deg;
    while (scanf("%d %d", &n, &deg) == 2) {
        double x[16], y[16], c[8];
        for (int i = 0; i < n; i++) scanf("%la", &x[i]);
        for (int i = 0; i < n; i++) scanf("%la", &y[i]);
        if (!nvl5::polyfit(x, y, n, deg, c)) { printf("FAIL\n"); continue; }
        for (int j = 0; j <= deg; j++) printf("%a ", c[j]);
        printf("\n");
    }
}
'''


def group_lstsq(tmp, per):
    exe = os.path.join(tmp, "lsq_test")
    src = os.path.join(tmp, "lsq_test.cpp")
    with open(src, "w") as fh:
        fh.write(LSQ_SRC)
    subprocess.run(["clang++", "-std=c++17", "-O2", "-ffp-contract=off", "-fno-fast-math", "-I", HERE, "-o", exe,
                    src, os.path.join(HERE, "nv_lstsq.cpp")], check=True)
    rng = np.random.default_rng(2)

    def vals(r, k):
        law = k % 7
        if law == 0:
            return rng.uniform(0.05, 20, r)
        if law == 1:
            return 10.0 ** rng.uniform(-6, 6, r)
        if law == 2:
            return rng.integers(0, 5, r).astype(float)
        if law == 3:
            return np.full(r, rng.uniform(0.1, 9))
        if law == 4:
            return rng.normal(0, 3, r)
        if law == 5:
            return rng.uniform(0.5, 4, r) * 1.07
        return np.round(rng.uniform(0, 30, r), 3)
    cases = []
    for r in range(2, 9):
        for idx in itertools.combinations(range(8), r):
            for deg in ((1,) if r == 2 else (2, 1)):
                for t in range(per):
                    cases.append((np.array(idx, float), vals(r, t), deg))
    inp = "".join(f"{len(x)} {d} " + " ".join(v.hex() for v in x) + " " + " ".join(float(v).hex() for v in y) + "\n"
                  for x, y, d in cases)
    out = subprocess.run([exe], input=inp, capture_output=True, text=True).stdout.split("\n")
    bad = 0
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        for (x, y, d), line in zip(cases, out):
            c = np.polyfit(x, y, d)
            got = [float.fromhex(t) for t in line.split()] if line != "FAIL" else []
            if len(got) != len(c) or any(np.float64(a).tobytes() != np.float64(b).tobytes() for a, b in zip(c, got)):
                bad += 1
    return len(cases), bad


# ----------------------------------------------------------------------------------- small group
def group_small(quick):
    import nvfilter as nv
    sys.path.insert(0, REPRO)
    from nv_ui_qt import state as st
    import nv_ui_qt.backend as be
    res = []
    for L in range(1, 9):
        res.append(("wn_band_std", f"levels {L}", compare(nv._wn_band_std(L), c_wn(L))))
    bad = 0
    for n in range(0, 401):
        class Fr(list):
            pass
        last = max(n - 3, 0)
        ref = sorted(set(np.linspace(0, last, min(32, last + 1)).round().astype(int).tolist())) if n else []
        if ref != c_candidates(n):
            bad += 1
    res.append(("noisiest", "candidates n = 0..400", (401, bad, [])))
    rng = np.random.default_rng(5)
    shapes = [(1, 1), (1, 5), (2, 2), (3, 3), (5, 8), (17, 33), (64, 64), (101, 77), (360, 640)]
    for i, (H, W) in enumerate(shapes * (1 if quick else 3)):
        Y = rng.uniform(0, 255, (H, W)).astype(F32) if i % 2 else np.rint(rng.normal(120, 9, (H, W))).astype(F32)
        res.append(("hh1_sigma", f"{H}x{W} #{i}", compare(be.hh1_sigma(Y), c_hh1(Y))))
    # noisiest_frame end to end on synthetic frame lists
    for n in (1, 2, 3, 4, 7, 40):
        frames = [synth("noise", 40, 52, 2, 100 + k) for k in range(n)]
        res.append(("noisiest", f"frames n={n}", compare(be.noisiest_frame(frames), c_noisiest(frames))))
    # cancel: FindFlatArea's query (status 5 -> ProfileCancelled); a manual area is not cancellable
    f0 = synth("noise", 200, 300, 2, 77)
    planes = tuple(np.asarray(p, F32) for p in f0)
    for label, mk in (("at once", lambda: (lambda: True)), ("after 20 queries", None), ("never", lambda: (lambda: False))):
        def counter(k=20):
            c = [0]

            def f():
                c[0] += 1
                return c[0] > k
            return f
        mk = mk or counter
        try:
            ref = nv.build_profile_v5(planes, False, None, None, "spatial", 5, (), "bt709", cancel=mk())
        except nv.ProfileCancelled:
            ref = "cancelled"
        nv._FFA_CACHE.clear()
        got = c_build(planes, False, None, None, "spatial", 5, (), "bt709", cancel=mk())
        res.append(("cancel", label, compare(ref, got)))
    got = c_build(planes, False, (5, 5, 64), None, "spatial", 5, (), "bt709", cancel=lambda: True)
    res.append(("cancel", "manual area ignores the query", compare(py_build(planes, False, (5, 5, 64), None, "spatial"), got)))
    # effective_profile
    base = py_build(tuple(np.asarray(p, F32) for p in synth("noise", 100, 130, 2, 9)), False, (10, 10, 64), None,
                    "spatial")
    for t in range(6 if quick else 30):
        if t == 0:
            factors, tune = st.unit_factors(), {}
        elif t == 1:
            factors, tune = st.unit_factors(), {"all": 1.0, "Y": 1.0}
        elif t == 2:
            factors, tune = [[1.0 + 1e-13] * 9 for _ in range(3)], {}
        else:
            factors = [list(rng.uniform(0.2, 3.0, 9)) for _ in range(3)]
            if t % 3 == 0:
                factors = st.unit_factors()
            tune = {k: float(rng.uniform(0.2, 3.0)) for k in ("all", "Y", "Cb", "Cr") if rng.random() < 0.6}
        ref = st.effective_profile(base, factors, tune)
        got = c_effective(base, factors, tune)
        r = compare(ref, got)
        if (ref is base) != (got is base):
            r = (r[0] + 1, r[1] + 1, r[2] + ["identity of base differs"])
        res.append(("effective_profile", f"#{t}", r))
    return res


# ----------------------------------------------------------------------------------- main
def run(tmp, jobs, quick, only=("lstsq", "small", "synth", "real")):
    """Runs the groups; returns (ok, total, rows)."""
    rows = []
    if "lstsq" in only:
        n, bad = group_lstsq(tmp, 20 if quick else 100)
        rows.append(("lstsq", f"polyfit systems {n}", n, bad, []))
    if "small" in only:
        for g, label, (w, d, first) in group_small(quick):
            rows.append((g, label, w, d, first))
    cases = []
    if "synth" in only:
        cases += synth_cases(quick)
    if "real" in only:
        cases += real_cases(quick, tmp)
    if cases:
        with ProcessPoolExecutor(max(1, jobs)) as ex:
            for group, label, w, d, first, tp, tc in ex.map(run_case, cases):
                rows.append((group, f"{label}  (py {tp:.2f} s, C {tc:.2f} s)", w, d, first))
    ok = sum(1 for r in rows if r[3] == 0)
    return ok, len(rows), rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--only", default="lstsq,small,synth,real")
    ap.add_argument("--tmp", default=os.path.join(HERE, "tmp_bridge"))
    a = ap.parse_args()
    os.makedirs(a.tmp, exist_ok=True)
    t0 = time.time()
    ok, total, rows = run(a.tmp, a.jobs, a.quick, a.only.split(","))
    groups = {}
    for g, label, w, d, first in rows:
        s = groups.setdefault(g, [0, 0, 0])
        s[0] += 1
        s[1] += d == 0
        s[2] += w
        if d:
            print(f"MISMATCH {g}: {label}: {d} of {w} words; {first}")
    for g, (n, k, w) in groups.items():
        print(f"{g:18s} {k}/{n} cases bit-exact ({w} words)")
    print(f"total {ok}/{total} ({time.time() - t0:.0f} s)")
    return 0 if ok == total else 1


if __name__ == "__main__":
    sys.exit(main())
