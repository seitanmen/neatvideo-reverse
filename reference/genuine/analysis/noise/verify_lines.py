#!/usr/bin/env python3
"""Verify repro/nv_lines.py (FUN_00990448 line kernels, FUN_009944c0 running box) against the
genuine plugin through the guarded harness (../harness/nvh kernel / linefilt).

  python3 verify_lines.py [--quick] [--sizes WxH,...]

Checks (all exact unless reported):
  1. LineKernel vs `nvh kernel W H angle`: n, nlines, flags, idx, every (off, start, count)
  2. linefilt vs `nvh linefilt`: 16 angles x r {1,2,4,10} x mode {0,1} x 8 sizes x 2 planes
     (gaussian 100 +- 5 and a ramp); counts exact (bitwise) matches, max abs / rel error
  3. the two-pass combinations of FUN_00998d98 (r 4 along kernel i, then rad2[i] along
     kernel idx2[i], mode 1) on 3 sizes
  4. fast paths (angle 0 / pi/2, border replicated in mode 0) against the general path of a
     tiny angle (0.01 / pi/2 + 0.01, truncated mean): documents the mode-0 difference of
     the genuine code itself; the port must match both.
  5. batched planes (..., H, W) give the same bits as one plane at a time (no harness)
  5b. the batch-minor fast path (nv_lines.linefilt_bm / direction_filter_bm on (H, W, B), used by
     nv_pixnoise.pixnoise_batch since 2026-10-16): in 2-4 against the same genuine outputs (both
     planes of a size in one batch), and bit for bit against the port everywhere, also with
     plain / padded destinations, padded inputs, out = input and NaN / inf / -0 planes
  6. LineKernel raises ValueError for sides outside 1..512 (the genuine tables hold 512 offsets /
     1024 lines; `nvh kernel` / `linefilt` refuse larger sides, so they cannot be verified)"""
import argparse
import os
import subprocess
import sys
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "..", "..", "reference", "python"))
import nvh_common as nc  # noqa: E402
import nv_lines as nl  # noqa: E402

F32 = np.float32
SIZES = [(12, 9), (24, 24), (120, 108), (108, 120), (97, 61), (48, 48), (512, 8), (8, 512)]
RADII = (1, 2, 4, 10)
TINY = F32(0.01)


def genuine_kernel(W, H, angle):
    out = subprocess.run([nc.NVH, "kernel", str(W), str(H), repr(float(angle))],
                         capture_output=True, text=True, timeout=120)
    if out.returncode:
        raise RuntimeError(f"nvh kernel exit {out.returncode}: {out.stderr[-300:]}")
    lines = out.stdout.splitlines()
    h = lines[0].split()
    head = dict(n=int(h[1]), nlines=int(h[3]), horiz=int(h[5]), vert=int(h[7]), flag=int(h[9]))
    idx = [int(x) for x in lines[1].split()[1:]]
    triples = [tuple(int(x) for x in l.split()[3::2]) for l in lines[2:] if l.startswith("line")]
    return head, idx, triples


def genuine_linefilt(plane, angle, r, mode, angle2=None, r2=None):
    H, W = plane.shape
    args = ["linefilt", W, H, repr(float(angle)), r, mode]
    if angle2 is not None:
        args += [repr(float(angle2)), r2]
    out = nc.run(args, [plane])
    v = np.array([float(l.split()[1]) for l in out.splitlines() if l and l[0].isdigit()], F32)
    if v.size != W * H:
        raise RuntimeError(f"nvh linefilt returned {v.size} values for {W}x{H}")
    return v.reshape(H, W)


def planes(W, H, rng):
    g = (100 + 5 * rng.standard_normal((H, W))).astype(F32)
    yy, xx = np.mgrid[0:H, 0:W]
    ramp = (255.0 * (xx + yy) / max(W + H - 2, 1)).astype(F32)
    return [("gauss", g), ("ramp", ramp)]


class Tally:
    def __init__(self, name):
        self.name, self.total, self.exact, self.maxabs, self.maxrel, self.bad = name, 0, 0, 0.0, 0.0, []

    def add(self, ref, got, label):
        self.total += 1
        same = ref.shape == got.shape and np.array_equal(ref.view(np.uint32), got.view(np.uint32))
        if same:
            self.exact += 1
            return
        if np.array_equal(ref, got):          # only the sign of a zero differs
            self.exact += 1
            return
        err = np.abs(ref.astype(np.float64) - got.astype(np.float64))
        rel = err / np.maximum(np.abs(ref.astype(np.float64)), 1e-6)
        self.maxabs = max(self.maxabs, float(err.max()))
        self.maxrel = max(self.maxrel, float(rel.max()))
        self.bad.append((label, int((ref != got).sum()), float(err.max()), float(rel.max())))

    def report(self):
        print(f"[{self.name}] cases {self.total} exact {self.exact} max_abs {self.maxabs:.3g} "
              f"max_rel {self.maxrel:.3g}")
        for b in self.bad[:40]:
            print(f"   mismatch {b[0]}: {b[1]} px, abs {b[2]:.3g}, rel {b[3]:.3g}")
        if len(self.bad) > 40:
            print(f"   ... {len(self.bad) - 40} more")


class BmTally:
    """Strict bitwise comparison of the batch-minor path with the port (no zero-sign allowance)."""

    def __init__(self):
        self.total, self.bad = 0, 0

    def add(self, port, bm, label):
        self.total += 1
        a, b = np.ascontiguousarray(port, F32), np.ascontiguousarray(bm, F32)
        if a.shape != b.shape or not np.array_equal(a.view(np.uint32), b.view(np.uint32)):
            self.bad += 1
            if self.bad <= 20:
                print(f"   batch-minor != port: {label}")

    def report(self):
        print(f"[batch-minor vs port] {self.total - self.bad}/{self.total} bit-identical")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true", help="radii 1,4 and 4 sizes only")
    ap.add_argument("--sizes", help="comma separated WxH list")
    a = ap.parse_args()
    sizes = SIZES
    radii = RADII
    if a.sizes:
        sizes = [tuple(int(v) for v in s.split("x")) for s in a.sizes.split(",")]
    if a.quick:
        radii = (1, 4)
        sizes = sizes[:4] if not a.sizes else sizes
    nc.require_guard()
    t0 = time.time()
    rng = np.random.default_rng(20261016)

    # 1. kernels
    kbad, ktotal = [], 0
    angles = list(nl.ANGLES) + [TINY, F32(nl._HALF_PI + TINY), F32(nl._HALF_PI + F32(0.02)), F32(1.58),
                                F32(3.0), F32(0.3), F32(1.2)]
    for W, H in sizes:
        for ang in angles:
            head, idx, triples = genuine_kernel(W, H, ang)
            k = nl.LineKernel(W, H, ang)
            ktotal += 1
            ok = (head == dict(n=k.n, nlines=k.nlines, horiz=int(k.horiz), vert=int(k.vert), flag=k.flag)
                  and idx == [int(v) for v in k.idx]
                  and triples == [tuple(int(v) for v in l) for l in k.lines])
            if not ok:
                kbad.append((W, H, float(ang), head, dict(n=k.n, nlines=k.nlines, flag=k.flag)))
    print(f"[kernel] cases {ktotal} exact {ktotal - len(kbad)}")
    for b in kbad:
        print("   mismatch", b)

    # 2. single pass, every angle / radius / mode / size / plane; the batch-minor path (both planes of
    #    a size in one (H, W, 2) batch) against the same genuine output, and bit for bit against the port
    one = Tally("linefilt")
    onebm = Tally("linefilt_bm")
    bm_port = BmTally()
    for W, H in sizes:
        kernels = nl.build_kernels(W, H)
        plans = nl.line_plans(W, H)
        pl = planes(W, H, rng)
        stack = np.ascontiguousarray(np.stack([p for _, p in pl], -1))
        for i, ang in enumerate(nl.ANGLES):
            for r in radii:
                for mode in (0, 1):
                    bm = nl.linefilt_bm(plans[i], stack, r, mode)
                    for b, (pname, plane) in enumerate(pl):
                        label = f"{W}x{H} {pname} dir{i} r{r} m{mode}"
                        ref = genuine_linefilt(plane, ang, r, mode)
                        got = nl.linefilt(kernels[i], plane, r, mode)
                        one.add(ref, got, label)
                        onebm.add(ref, np.ascontiguousarray(bm[..., b]), label)
                        bm_port.add(got, bm[..., b], label)
    one.report()
    onebm.report()

    # 3. two-pass combinations of FUN_00998d98 (mode 1, r 4 then rad2 along idx2), port and batch-minor
    two = Tally("two-pass")
    twobm = Tally("two-pass bm")
    for W, H in [s for s in sizes if s in [(120, 108), (97, 61), (48, 48)]] or sizes[:2]:
        kernels = nl.build_kernels(W, H)
        plans = nl.line_plans(W, H)
        pl = planes(W, H, rng)
        stack = np.ascontiguousarray(np.stack([p for _, p in pl], -1))
        for i in range(16):
            bm = nl.direction_filter_bm(plans, i, stack)
            for b, (pname, plane) in enumerate(pl):
                ref = genuine_linefilt(plane, nl.ANGLES[i], 4, 1, nl.ANGLES[nl.IDX2[i]], nl.RAD2[i])
                got = nl.direction_filter(kernels, i, plane)
                two.add(ref, got, f"{W}x{H} {pname} dir{i} two-pass")
                twobm.add(ref, np.ascontiguousarray(bm[..., b]), f"{W}x{H} {pname} dir{i} two-pass bm")
                bm_port.add(got, bm[..., b], f"{W}x{H} {pname} dir{i} two-pass")
    two.report()
    twobm.report()

    # 4. fast path vs tiny-angle general path (genuine vs genuine, and the port on both)
    fast = Tally("fast-vs-tiny port")
    print("[fast-vs-tiny genuine] mode 0: axis-aligned = border replicated / general = truncated mean;"
          " mode 1: both zero padded (max abs difference of the genuine outputs)")
    for W, H in [s for s in sizes if s in [(24, 24), (97, 61), (512, 8), (8, 512)]] or sizes[:2]:
        plane = planes(W, H, rng)[0][1]
        # pi/2 + 0.01 still rounds inside the vertical window (|a - pi/2| < 0.01 in float32), so
        # the vertical general-path neighbour is pi/2 + 0.02
        for name, a0, a1 in [("horizontal", F32(0), TINY), ("vertical", nl._HALF_PI, F32(nl._HALF_PI + F32(0.02)))]:
            k0, k1 = nl.LineKernel(W, H, a0), nl.LineKernel(W, H, a1)
            for r in radii:
                for mode in (0, 1):
                    g0 = genuine_linefilt(plane, a0, r, mode)
                    g1 = genuine_linefilt(plane, a1, r, mode)
                    p0, p1 = nl.linefilt(k0, plane, r, mode), nl.linefilt(k1, plane, r, mode)
                    fast.add(g0, p0, f"{W}x{H} {name} fast r{r} m{mode}")
                    fast.add(g1, p1, f"{W}x{H} {name} tiny r{r} m{mode}")
                    for kk, pp, gg, lab in ((k0, p0, g0, "fast"), (k1, p1, g1, "tiny")):
                        bm = nl.linefilt_bm(nl.LinePlan(kk), plane[..., None], r, mode)[..., 0]
                        fast.add(gg, np.ascontiguousarray(bm), f"{W}x{H} {name} {lab} r{r} m{mode} bm")
                        bm_port.add(pp, bm, f"{W}x{H} {name} {lab} r{r} m{mode}")
                    d = float(np.abs(g0.astype(np.float64) - g1).max())
                    print(f"   {W}x{H} {name} r{r} mode{mode}: |fast - tiny| max {d:.4g}"
                          f" (lines of the tiny kernel: {k1.nlines}, flag {k1.flag})")
    fast.report()

    # 5. batched planes (..., H, W) must give the same bits as one plane at a time (no harness)
    bbad = 0
    for W, H in sizes[:3]:
        kernels = nl.build_kernels(W, H)
        stack = np.stack([p for _, p in planes(W, H, rng)] + [planes(W, H, rng)[0][1]])
        for i in (0, 3, 8, 11, 14):
            for r in radii:
                for mode in (0, 1):
                    got = nl.linefilt(kernels[i], stack, r, mode)
                    for b in range(stack.shape[0]):
                        one_plane = nl.linefilt(kernels[i], stack[b], r, mode)
                        bbad += not np.array_equal(got[b].view(np.uint32), one_plane.view(np.uint32))
    print(f"[batch] mismatching planes {bbad}")
    # batch-minor path (H, W, B) with destinations: plain / padded out, padded input, out = input on the
    # oriented path, direction filter with out and tmp; NaN, inf and -0 planes included (no harness)
    for W, H in sizes[:3] + [(60, 54), (12, 9)]:
        plans = nl.line_plans(W, H)
        X = np.stack([p for _, p in planes(W, H, rng)] + [planes(W, H, rng)[0][1]], -1)
        X[0, 0, 1], X[H // 2, W // 3, 1] = np.nan, np.inf
        X[1, :, 2] = -0.0
        X = np.ascontiguousarray(X)
        for i in range(16):
            for r in radii:
                for mode in (0, 1):
                    with np.errstate(all="ignore"):
                        port = np.stack([nl.linefilt(plans[i].kernel, X[..., b], r, mode) for b in range(X.shape[2])], -1)
                        _, op = nl.padded_empty(H, W, X.shape[2])
                        _, xp = nl.padded_empty(H, W, X.shape[2])
                        xp[...] = X
                        outs = [nl.linefilt_bm(plans[i], X, r, mode), nl.linefilt_bm(plans[i], X, r, mode, out=op),
                                nl.linefilt_bm(plans[i], X, r, mode, out=np.empty_like(X)), nl.linefilt_bm(plans[i], xp, r, mode)]
                        if mode == 1 and not (plans[i].horiz or plans[i].vert):
                            xa = X.copy()
                            outs.append(nl.linefilt_bm(plans[i], xa, r, mode, out=xa))
                    for o in outs:
                        bm_port.add(port, o, f"{W}x{H} dir{i} r{r} m{mode} out-variants")
            with np.errstate(all="ignore"):
                port = np.stack([nl.direction_filter([p.kernel for p in plans], i, X[..., b]) for b in range(X.shape[2])], -1)
                _, t1 = nl.padded_empty(H, W, X.shape[2])
                bm_port.add(port, nl.direction_filter_bm(plans, i, X, out=np.empty_like(X), tmp=t1), f"{W}x{H} dir{i} DF out/tmp")
    bm_port.report()

    # 6. sides outside the genuine tables (512 offsets / 1024 lines): ValueError for every angle
    outside = [(513, 8), (8, 513), (600, 600), (0, 8), (8, 0)]
    refused = 0
    for W, H in outside:
        for ang in (nl.ANGLES[0], nl.ANGLES[3], nl.ANGLES[8], nl.ANGLES[11]):
            try:
                nl.LineKernel(W, H, ang)
            except ValueError:
                refused += 1
    nlim = 4 * len(outside)
    print(f"[limits] LineKernel refuses {refused}/{nlim} (size, angle) pairs outside 1..{nl.MAX_SIDE} px; "
          f"512 itself is covered by the (512, 8) / (8, 512) cases above")

    total = one.total + two.total + fast.total + onebm.total + twobm.total
    exact = one.exact + two.exact + fast.exact + onebm.exact + twobm.exact
    print(f"SUMMARY kernels {ktotal - len(kbad)}/{ktotal} exact; linefilt {exact}/{total} exact vs nvh "
          f"(port and batch-minor path; fraction {exact / max(total, 1):.6f}), "
          f"max_rel {max(one.maxrel, two.maxrel, fast.maxrel, onebm.maxrel, twobm.maxrel):.3g}; "
          f"batch-minor vs port {bm_port.total - bm_port.bad}/{bm_port.total} bit-identical; "
          f"limits {refused}/{nlim}; {time.time() - t0:.0f} s")
    return 0 if (exact == total and not kbad and not bbad and refused == nlim and not bm_port.bad) else 1


if __name__ == "__main__":
    sys.exit(main())
