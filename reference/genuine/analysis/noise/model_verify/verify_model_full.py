#!/usr/bin/env python3
"""FUN_00995250 (BuildNoiseModel per-plane model): all 4096 words of `nvh model W H plane` vs
nv_model.model_full, bit for bit (the hex column; -0, NaN payloads and init patterns included).

usage: verify_model_full.py [--quick] [--fuzz N] [--jobs J] [--only NAME_SUBSTR] [--log FILE]
Inputs: crops of test_v02.mp4 / test.mp4 (R, G, B, BT.601 Y, Cr', Cb' of the profile transform,
48..512 px, square / rectangular / odd), synthetic planes (white / coloured noise, edges in 16
directions, checker, constant, quantised, black band, clipped, NaN / inf / huge), the measurable-level
boundaries and `--fuzz N` random planes (2..300 px, random content).  Planes over 512 px or with one
row / column are not passed to the genuine (Agents.md §7).  Branch counters (nv_cov) are summed over
the cases and printed: a branch with 0 hits is not verified by this run."""
import argparse
import collections
import os
import sys
import time
from concurrent.futures import ProcessPoolExecutor, ThreadPoolExecutor

import numpy as np

sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import common as C  # noqa: E402

F32 = np.float32
GROUPS = [("levels 0..54", 0, 55), ("dc 0x0dc", 0x37, 0x50), ("A 0x140", 0x50, 0x1e0), ("R 0x780", 0x1e0, 0x1f9),
          ("B 0x7e4", 0x1f9, 0x389), ("C 0xe24", 0x389, 0x519), ("special 0x1464", 0x519, 0x7d5),
          ("rest", 0x7d5, 4096)]


def luma_chroma(rgb):
    import nv_pixnoise as npx
    R, G, B = (np.ascontiguousarray(rgb[..., c], F32) for c in range(3))
    Y = npx.luma_601(R, G, B)
    return R, G, B, Y, ((R - Y) * F32(0.713)).astype(F32), ((B - Y) * F32(0.565)).astype(F32)


def real_cases(quick):
    out = []
    specs = [("test_v02.mp4", n) for n in ((312,) if quick else (0, 100, 312, 576, 700))] + \
            [("test.mp4", n) for n in ((0,) if quick else (0, 300, 450, 750))]
    rng = np.random.default_rng(1234)
    sizes = [(128, 128), (64, 64), (200, 150)] if quick else \
        [(48, 48), (64, 64), (96, 96), (128, 128), (200, 150), (97, 71), (320, 320), (512, 512), (511, 333), (60, 400)]
    for name, n in specs:
        fr = C.frame_cached(name, n).astype(F32)
        H, W = fr.shape[:2]
        for (w, h) in sizes:
            if w > W or h > H:
                continue
            x, y = int(rng.integers(0, W - w + 1)), int(rng.integers(0, H - h + 1))
            planes = luma_chroma(fr[y:y + h, x:x + w])
            pick = range(6) if not quick else (0, 3, 4)
            for p in pick:
                out.append((f"{name}:{n} {w}x{h}@{x},{y} p{p}", planes[p]))
    return out


def synthetic_cases(quick):
    rng = np.random.default_rng(99)
    out = []
    S = 96 if quick else 128
    for sd in ((10,) if quick else (1, 5, 10, 30)):
        out.append((f"white sd{sd} {S}", (128 + sd * rng.standard_normal((S, S))).astype(F32)))
    from scipy.ndimage import gaussian_filter
    out.append(("coloured noise", (128 + 20 * gaussian_filter(rng.standard_normal((S, S)), 1.5)).astype(F32)))
    yy, xx = np.mgrid[0:S, 0:S].astype(np.float64)
    angs = (0, 30, 45) if quick else range(0, 180, 12)
    for a in angs:
        t = np.deg2rad(a)
        e = np.where(np.cos(t) * (xx - S / 2) + np.sin(t) * (yy - S / 2) > 0, 180.0, 60.0)
        out.append((f"edge {a}deg", (e + 3 * rng.standard_normal((S, S))).astype(F32)))
    if not quick:
        out.append(("checker 4", (np.where(((yy // 4) + (xx // 4)) % 2, 200, 50) + rng.standard_normal((S, S))).astype(F32)))
        out.append(("constant", np.full((S, S), 77.0, F32)))
        out.append(("quantised", np.round(128 + 2 * rng.standard_normal((S, S))).astype(F32)))
        b = (128 + 5 * rng.standard_normal((S, S))).astype(F32)
        b[:20] = 16.0
        out.append(("black band", b))
        out.append(("clipped", np.clip(200 + 40 * rng.standard_normal((S, S)), 0, 255).astype(F32)))
        z = (128 + 5 * rng.standard_normal((S, S))).astype(F32)
        z[10, 10] = np.nan
        z[50, 60] = np.inf
        z[70:72, 20:24] = 1e30
        out.append(("nan/inf/huge", z))
        out.append(("lines", (128 + 60 * np.sin(xx * 0.9 + yy * 0.4) + rng.standard_normal((S, S))).astype(F32)))
        for (w, h) in ((2, 2), (5, 5), (8, 8), (9, 9), (9, 12), (12, 9), (13, 13), (16, 16), (17, 17), (24, 24),
                       (25, 25), (33, 33), (40, 9), (9, 40), (49, 49), (65, 33), (100, 13), (257, 31)):
            out.append((f"size {w}x{h}", (128 + 8 * rng.standard_normal((h, w))).astype(F32)))
    return out


def fuzz_cases(n):
    rng = np.random.default_rng(7)
    out = []
    for i in range(n):
        w, h = int(rng.integers(2, 301)), int(rng.integers(2, 301))
        kind = i % 5
        if kind == 0:
            p = rng.normal(rng.uniform(0, 255), rng.uniform(0.01, 60), (h, w))
        elif kind == 1:
            p = rng.uniform(-1e3, 1e3, (h, w))
        elif kind == 2:
            p = np.round(rng.normal(128, rng.uniform(0.2, 3), (h, w)))
        elif kind == 3:
            p = rng.normal(128, 5, (h, w)) * np.exp(rng.uniform(-30, 30))
        else:
            p = rng.normal(128, 10, (h, w))
            m = rng.random((h, w)) < 0.002
            p[m] = rng.choice([np.nan, np.inf, -np.inf, 1e35, -0.0], m.sum())
        out.append((f"fuzz {i} k{kind} {w}x{h}", p.astype(F32)))
    return out


def port_one(job):
    name, plane = job
    import nv_cov
    import nv_model as nm
    nv_cov.reset()
    t = time.time()
    try:
        w = nm.model_full(plane)
        err = None
    except Exception as e:  # noqa: BLE001
        w, err = None, repr(e)[:200]
    return name, w, err, time.time() - t, nv_cov.snapshot()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--fuzz", type=int, default=0)
    ap.add_argument("--jobs", type=int, default=6)
    ap.add_argument("--only", default=None)
    A = ap.parse_args()
    C.require_guard()
    cases = real_cases(A.quick) + synthetic_cases(A.quick) + fuzz_cases(A.fuzz)
    if A.only:
        cases = [c for c in cases if A.only in c[0]]
    t0 = time.time()
    print(f"{len(cases)} planes", flush=True)

    def gen(c):
        try:
            return C.genuine_model(c[1]), None
        except C.Rejected as e:
            return None, f"rejected: {e}"
    with ThreadPoolExecutor(A.jobs) as ex:
        gs = list(ex.map(gen, cases))
    with ProcessPoolExecutor(A.jobs) as ex:
        ps = list(ex.map(port_one, cases))
    global INIT
    import nv_band as nb
    INIT = nb.init_struct(4096)
    ok = bad = rej = 0
    cov = collections.Counter()
    grp_bad = collections.Counter()
    grp_live = collections.Counter()
    tport = []
    for (name, plane), (g, gerr), (_, p, perr, dt, cv) in zip(cases, gs, ps):
        cov.update(cv)
        if g is None:
            rej += 1
            print(f"[REJ] {name}: genuine {gerr}; port {'error ' + perr if perr else 'ran'}")
            continue
        tport.append(dt)
        if p is None:
            bad += 1
            print(f"[BAD] {name}: port error {perr}")
            continue
        d = np.nonzero(g != p)[0]
        init = INIT
        for gname, a, b in GROUPS:
            if init is not None and np.any(g[a:b] != init[a:b]):
                grp_live[gname] += 1
            if np.any((d >= a) & (d < b)):
                grp_bad[gname] += 1
        if d.size == 0:
            ok += 1
        else:
            bad += 1
            print(f"[BAD] {name} {plane.shape}: {d.size} words differ, first {d[:8].tolist()} "
                  f"genuine {[hex(v) for v in g[d[:4]]]} port {[hex(v) for v in p[d[:4]]]}", flush=True)
    print(f"\n{ok}/{ok + bad} planes bit-identical (4096 words each); {rej} rejected by the genuine")
    if grp_live:
        print("planes with non-init words per group: " + ", ".join(f"{k} {v}" for k, v in grp_live.items()))
    print("planes with differing words per group: " + (", ".join(f"{k} {v}" for k, v in grp_bad.items()) or "none"))
    if tport:
        print(f"port time per plane: median {np.median(tport):.2f} s, max {max(tport):.2f} s")
    print("branch counters (0 = not exercised):")
    for k in sorted(cov):
        print(f"   {k:48s} {cov[k]}")
    print(f"total {time.time() - t0:.0f} s")
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
