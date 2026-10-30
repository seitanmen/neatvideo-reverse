#!/usr/bin/env python3
"""Verify repro/nv_pixnoise.py (FUN_00998d98, the per-pixel noise estimate of
EstimateNoiseLevelCurve) against the genuine plugin through the guarded harness
(../harness/nvh pixnoise / refmodel).

  python3 verify_pixnoise.py [--quick] [--clip test_v02.mp4] [--frame 312] [--rgb planes.f32]
                             [--sensitivity] [--keep DIR]

Checks:
  1. `nvh refmodel` twice: identical bytes; the 2 x 16 normalisers at byte 0x500 + k*0x40 +
     i*4 are bit-identical to nv_pixnoise.REF_NORM
  2. the float helpers against references that share no code with the port: fma32 against
     exact rational arithmetic with the FPMulAdd special cases (random triples, constructed
     double-rounding ties, the FLT_MAX / 2^128 midpoint, NaN order), frecpe32 against a
     value-based FPRecipEstimate (normal, subnormal and special operands), frecps32 against
     FPRecipStepFused (inf * 0 -> +2, NaN order, rational fused step), the FMIN / FMAX of the
     ramp against FPMin / FPMax, and a table of CPU results (NEON FMLA / FRECPE / FRECPS run with
     inline asm on the M2 Max under the default FPCR, 2026-10-16)
  3. `nvh pixnoise` vs pixnoise() on gaussian tiles (120x108, 108x120, 120x120, 48x120,
     97x61 and odd / small extras), an edge + gradient tile, a disc, a flat tile, tiles cut
     from frame 312 of test_v02 (decoded from --clip: denoise_video_v5.read_frames +
     nvfilter.yuv_to_rgb(planes, 'bt709', False), clipped to 0..255, float32; --rgb reads a
     1920x1080 planar R, G, B float32 dump instead; no frame -> exit 2, never a pass without
     real tiles) with luma = 0.2989 R + 0.5866 G + 0.1145 B as FUN_009f11fc, and extreme-value
     tiles whose squares overflow (a gradient + gauss tile scaled by 1e18; 1e20 / 1e30 blocks in
     51x49, 49x51 (n % 4 = 3) and 50x46 tiles; a 1e20 block inside a 120x120 tile; one 5e19
     pixel in 51x49 / 49x51 / 97x61 tiles, which overflows level 0 only, so the NaN ratios reach
     the ramp's scalar tail): bit-exact match of the noise, weight and bright maps (NaN bit
     patterns included); max abs / rel error otherwise; coverage of the ramps' linear regions;
     timing
  4. the batch path (nv_pixnoise.pixnoise_batch, used by nv_curve.genuine_noise_curve since the
     2026-10-16 speed-up) on the same tiles: all tiles of a size in one batch (with 0 / 1 input
     weights) and every tile alone, against the genuine maps of step 3, bit for bit; its float
     shortcuts (frecpe32_fast, _fma32_fast, _frecps32_fast, _fminmax_fast) are part of step 2
     (same references, the CPU table, and every 17-bit FRECPE prefix)
  5. --sensitivity: the same tiles with the NEON peculiarities disabled one at a time
     (true division, unfused ramp, scalar 2x2 mean order), to document that they matter, and
     the extreme tiles with the two behaviours fixed on 2026-10-16 undone (FRECPS(inf, 0) as a
     plain fused 2 - a*b; a NaN-propagating ramp tail).  This patches the reference pixnoise().
Exit code 0 when every map of every tile is bit-exact (reference and batch path)."""
import argparse
import os
import subprocess
import sys
import tempfile
import time
from fractions import Fraction

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "reference", "python"))
import nvh_common as nc  # noqa: E402
import nv_pixnoise as npx  # noqa: E402

F32 = np.float32
U32 = np.uint32
SIZES = [(120, 108), (108, 120), (120, 120), (48, 120), (97, 61)]
EXTRA_SIZES = [(95, 59), (50, 46), (64, 33), (10, 9), (9, 10), (8, 8)]   # odd counts, scalar up2 of the level-1 band
REAL_TILES = [(900, 400, 120, 120), (832, 0, 120, 108), (0, 0, 120, 120), (1800, 960, 120, 120),
              (640, 500, 97, 61)]                                  # x, y, w, h in frame 312

# CPU results (a, b, c, FMLA c + a*b, FRECPE a, FRECPS a, b) as float32 bit patterns, measured with
# inline-asm NEON instructions on the M2 Max (default FPCR: RN, FZ = 0, DN = 0, AH = 0), 2026-10-16
CPU_GOLDEN = [
    (0x7f800000, 0x00000000, 0x40000000, 0x7fc00000, 0x00000000, 0x40000000),   # inf*0
    (0xff800000, 0x00000000, 0x7fc12345, 0x7fc00000, 0x80000000, 0x40000000),   # quiet-NaN addend, inf*0
    (0x00000000, 0xff800000, 0x7f812345, 0x7fc12345, 0x7f800000, 0x40000000),   # signalling addend, 0*inf
    (0x7fc11111, 0xffc22222, 0x7fc33333, 0x7fc33333, 0x7fc11111, 0xffc11111),   # NaN order
    (0x7f811111, 0x7fc22222, 0x3f800000, 0x7fc11111, 0x7fc11111, 0xffc11111),
    (0x7fc11111, 0x7f822222, 0x3f800000, 0x7fc22222, 0x7fc11111, 0x7fc22222),
    (0xffc11111, 0x3f800000, 0x3f800000, 0xffc11111, 0xffc11111, 0x7fc11111),
    (0x7f800000, 0x40000000, 0x3f800000, 0x7f800000, 0x00000000, 0xff800000),
    (0xff800000, 0x40000000, 0x7f800000, 0x7fc00000, 0x80000000, 0x7f800000),
    (0x80000000, 0x3f800000, 0x80000000, 0x80000000, 0xff800000, 0x40000000),   # signed zeros
    (0x80000000, 0x3f800000, 0x00000000, 0x00000000, 0xff800000, 0x40000000),
    (0x3f800000, 0x3f800000, 0xbf800000, 0x00000000, 0x3f7f8000, 0x3f800000),
    (0x60918e00, 0x5e612000, 0xb0800000, 0x7f7fffff, 0x1e610000, 0xff7fffff),   # a*b = 2^128 - 2^103
    (0x60918e00, 0x5e612000, 0x30800000, 0x7f800000, 0x1e610000, 0xff7fffff),
    (0x60918e00, 0x5e612000, 0x00000000, 0x7f800000, 0x1e610000, 0xff7fffff),
    (0xe0918e00, 0x5e612000, 0x30800000, 0xff7fffff, 0x9e610000, 0x7f800000),
    (0x45800800, 0x45800800, 0x2a800000, 0x4b801001, 0x397f8000, 0xcb801000),   # 4097^2 +- 2^-42 (tie)
    (0x45800800, 0x45800800, 0xaa800000, 0x4b801000, 0x397f8000, 0xcb801000),
    (0x45800800, 0x45800800, 0x00000000, 0x4b801000, 0x397f8000, 0xcb801000),
    (0x00000001, 0x3f800000, 0x00000000, 0x00000001, 0x7f800000, 0x40000000),   # subnormal operands
    (0x001fffff, 0x3f800000, 0x00000000, 0x001fffff, 0x7f800000, 0x40000000),
    (0x00200000, 0x3f800000, 0x00000000, 0x00200000, 0x7f7f8000, 0x40000000),
    (0x00400000, 0x3f800000, 0x00000000, 0x00400000, 0x7eff8000, 0x40000000),
    (0x807fffff, 0x3f800000, 0x00000000, 0x807fffff, 0xfe800000, 0x40000000),
    (0x7e800000, 0x3f800000, 0x00000000, 0x7e800000, 0x007fc000, 0xfe800000),   # subnormal estimates
    (0x7f7fffff, 0x3f800000, 0x00000000, 0x7f7fffff, 0x00200000, 0xff7fffff),
    (0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, 0x3f7f8000, 0x3f800000),
    (0x40400000, 0x3f800000, 0x00000000, 0x40400000, 0x3eaa8000, 0xbf800000),
    (0xc0a00000, 0x3e4ccccd, 0x00000000, 0xbf800000, 0xbe4c8000, 0x40400000),
    (0x00000000, 0x3f800000, 0x00000000, 0x00000000, 0x7f800000, 0x40000000),
    (0x80000000, 0x3f800000, 0x00000000, 0x00000000, 0xff800000, 0x40000000),
    (0xff800000, 0x3f800000, 0x00000000, 0xff800000, 0x80000000, 0x7f800000),
]


def genuine_pixnoise(luma, workdir, tag):
    H, W = luma.shape
    prefix = os.path.join(workdir, tag)
    nc.run(["pixnoise", W, H, prefix], [luma])
    out = {}
    for s in ("noise", "weight", "bright"):
        out[s] = np.fromfile(prefix + "." + s + ".f32", F32).reshape(H, W)
    return out


def compare(a, b):
    """(exact fraction, max abs, max rel, bitwise differences, NaN-aware differences, NaN count)."""
    a = np.ascontiguousarray(a, F32).ravel()
    b = np.ascontiguousarray(b, F32).ravel()
    bitdiff = a.view(U32) != b.view(U32)
    nan_a, nan_b = np.isnan(a), np.isnan(b)
    valdiff = bitdiff & ~(nan_a & nan_b)
    fin = ~(nan_a | nan_b)
    with np.errstate(invalid="ignore", over="ignore"):
        d = np.abs(a[fin].astype(np.float64) - b[fin].astype(np.float64))
        rel = d / np.maximum(np.abs(b[fin].astype(np.float64)), 1e-30)
    d = d[np.isfinite(d)]
    rel = rel[np.isfinite(rel)]
    return (float(np.mean(~bitdiff)), float(d.max()) if d.size else 0.0, float(rel.max()) if rel.size else 0.0,
            int(bitdiff.sum()), int(valdiff.sum()), int(nan_b.sum()))


# ------------------------------------------------------------------ 1. reference model
def check_refmodel(workdir):
    dumps = []
    for i in range(2):
        p = os.path.join(workdir, f"refmodel_{i}.bin")
        r = subprocess.run([nc.NVH, "refmodel", p], capture_output=True, text=True, timeout=300)
        if r.returncode:
            raise RuntimeError(f"nvh refmodel exit {r.returncode}: {r.stderr[-300:]}")
        with open(p, "rb") as f:
            dumps.append(f.read())
    same = dumps[0] == dumps[1]
    ref = npx.ref_from_model(dumps[0])
    bits_ok = bool(np.array_equal(ref.view(U32), npx.REF_NORM.view(U32)))
    print(f"[refmodel] {len(dumps[0])} bytes, two runs identical: {same}; "
          f"32 normalisers at 0x500 + k*0x40 + i*4 bit-identical to REF_NORM: {bits_ok}")
    for k in range(2):
        print(f"   level {k}: " + " ".join(f"{v:.7g}" for v in ref[k]))
    return same and bits_ok, ref


# ------------------------------------------------------------------ 2. float helpers
_MID = Fraction(2 ** 128 - 2 ** 103)          # half-way between FLT_MAX and 2^128
_QNAN = 0x00400000


def _u(x):
    return int(np.asarray(x, F32).view(U32))


def _f(u):
    return np.array([u], U32).view(F32)[0]


def _isnan_u(u):
    return (u & 0x7fffffff) > 0x7f800000


def round_f32_exact(v):
    """Correctly rounded float32 (ties to even, overflow to +-inf) of the non-zero Fraction v."""
    if abs(v) >= _MID:                          # FLT_MAX has an odd significand: the tie goes up
        return F32(np.inf) if v > 0 else F32(-np.inf)
    with np.errstate(over="ignore"):
        r = F32(float(v))                       # at most one float32 step away from the answer
    if np.isinf(r):
        r = F32(np.copysign(np.finfo(F32).max, float(v)))
    best = None
    for c in (r, np.nextafter(r, F32(np.inf)), np.nextafter(r, F32(-np.inf))):
        if not np.isfinite(c):
            continue
        d = abs(Fraction(float(c)) - v)
        if best is None or d < best[0] or (d == best[0] and (c.view(U32) & 1) == 0):
            best = (d, c)
    return best[1]


def _nans(*ops):
    """FPProcessNaNs(3): first signalling NaN quieted, else first quiet NaN; None if no NaN."""
    for u in ops:
        if _isnan_u(u) and not u & _QNAN:
            return u | _QNAN
    for u in ops:
        if _isnan_u(u):
            return u
    return None


def ref_fma(ua, ub, uc):
    """FPMulAdd(addend c, a, b) on bit patterns (value-based, Fractions)."""
    a, b, c = _f(ua), _f(ub), _f(uc)
    inf0 = (np.isinf(a) and b == 0) or (a == 0 and np.isinf(b))
    n = _nans(uc, ua, ub)
    if n is not None:
        return 0x7fc00000 if (_isnan_u(uc) and uc & _QNAN and inf0) else n
    sp = (ua ^ ub) >> 31
    if inf0 or (np.isinf(c) and (np.isinf(a) or np.isinf(b)) and (uc >> 31) != sp):
        return 0x7fc00000
    if np.isinf(c):
        return uc
    if np.isinf(a) or np.isinf(b):
        return 0xff800000 if sp else 0x7f800000
    v = Fraction(float(a)) * Fraction(float(b)) + Fraction(float(c))
    if v == 0:
        return 0x80000000 if (sp and uc >> 31 and (a == 0 or b == 0) and c == 0) else 0
    return _u(round_f32_exact(v))


def ref_frecpe(ux):
    """FPRecipEstimate written from the values: |x| = m * 2^e with m in [0.5, 1), the 9-bit
    table estimate of floor(m * 512), result est/256 * 2^-e (subnormal results truncated)."""
    x = _f(ux)
    s = ux & 0x80000000
    if _isnan_u(ux):
        return ux | _QNAN
    if np.isinf(x):
        return s
    if x == 0:
        return s | 0x7f800000
    v = abs(Fraction(float(x)))
    if v < Fraction(1, 2 ** 128):
        return s | 0x7f800000
    m, e = v, 0
    while m >= 1:
        m, e = m / 2, e + 1
    while m < Fraction(1, 2):
        m, e = m * 2, e - 1
    scaled = int(m * 512)
    est = ((1 << 19) // (2 * scaled + 1) + 1) // 2
    r = Fraction(est, 256) * Fraction(2) ** (-e)
    if r < Fraction(1, 2 ** 126):
        r = Fraction(int(r * 2 ** 149), 2 ** 149)
    return s | _u(F32(float(r)))


def ref_frecps(ua, ub):
    """FPRecipStepFused(a, b) = 2 - a*b (op1 negated first)."""
    u1 = ua ^ 0x80000000
    n = _nans(u1, ub)
    if n is not None:
        return n
    a, b = _f(u1), _f(ub)
    if (np.isinf(a) and b == 0) or (a == 0 and np.isinf(b)):
        return 0x40000000
    if np.isinf(a) or np.isinf(b):
        return 0xff800000 if (u1 ^ ub) >> 31 else 0x7f800000
    v = 2 + Fraction(float(a)) * Fraction(float(b))
    return 0 if v == 0 else _u(round_f32_exact(v))


def ref_fminmax(ua, ub, is_max):
    n = _nans(ua, ub)
    if n is not None:
        return n
    a, b = _f(ua), _f(ub)
    if a == 0 and b == 0:
        neg = (ua >> 31) & (ub >> 31) if is_max else (ua >> 31) | (ub >> 31)
        return 0x80000000 if neg else 0
    return ua if ((a > b) if is_max else (a < b)) else ub


def helper_operands(rng):
    sp = [0x00000000, 0x80000000, 0x00000001, 0x80000001, 0x001fffff, 0x00200000, 0x00200001, 0x003fffff,
          0x00400000, 0x007fffff, 0x00800000, 0x3f800000, 0xbf800000, 0x40000000, 0x7e800000, 0x7effffff,
          0x7f000000, 0x7f7fffff, 0xff7fffff, 0x7f800000, 0xff800000, 0x7fc00000, 0xffc00000, 0x7fc12345,
          0xffc54321, 0x7f800001, 0xff812345, 0x3f7fffff, 0x3f000000, 0x40400000, 0x5f800000, 0x1f800000]
    trip = [(a, b, c) for a in sp for b in sp for c in sp[::3]]
    rnd = rng.integers(0, 2 ** 32, (3000, 3), dtype=np.uint64).tolist()
    mag = (rng.standard_normal((3000, 3)) * 10.0 ** rng.uniform(-38, 38, (3000, 3))).astype(F32).view(U32).tolist()
    ties = [(_u(F32(4097.0)), _u(F32(2 * int(k) + 1)), _u(F32(s * 2.0 ** int(e))))
            for k, s, e in zip(rng.integers(2048, 4096, 400), rng.choice([1, -1], 400), rng.integers(-40, -25, 400))]
    mid = [(_u(F32(sa * 18631.0 * 2.0 ** 52)), _u(F32(1801.0 * 2.0 ** 51)), _u(F32(cv)))
           for sa in (1.0, -1.0) for cv in (2.0 ** -30, -2.0 ** -30, 0.0, 2.0 ** 70, -2.0 ** 70, 2.0 ** 100)]
    return [tuple(int(v) for v in t) for t in trip + rnd + mag + ties + mid]


def check_helpers():
    rng = np.random.default_rng(7)
    ok = True
    ops = helper_operands(rng)
    a = np.array([t[0] for t in ops], U32).view(F32)
    b = np.array([t[1] for t in ops], U32).view(F32)
    c = np.array([t[2] for t in ops], U32).view(F32)
    with np.errstate(all="ignore"):
        got = {"fma32": npx.fma32(a, b, c).view(U32), "frecpe32": npx.frecpe32(a).view(U32),
               "frecps32": npx.frecps32(a, b).view(U32),
               # the batch path's shortcuts (table lookup / float64 path with element-wise fallback)
               "_fma32_fast": npx._fma32_fast(a, b, c).view(U32), "frecpe32_fast": npx.frecpe32_fast(a).view(U32),
               "_frecps32_fast": npx._frecps32_fast(a, b).view(U32)}
        refs = {"fma32": [ref_fma(*t) for t in ops], "frecpe32": [ref_frecpe(t[0]) for t in ops],
                "frecps32": [ref_frecps(t[0], t[1]) for t in ops]}
        refs.update(_fma32_fast=refs["fma32"], frecpe32_fast=refs["frecpe32"], _frecps32_fast=refs["frecps32"])
        for name in got:
            bad = [i for i in range(len(ops)) if int(got[name][i]) != refs[name][i]]
            ok &= not bad
            print(f"[{name}] {len(ops)} operand sets vs the pseudocode reference: {len(bad)} differ"
                  + "".join(f"\n   {ops[i][0]:08x} {ops[i][1]:08x} {ops[i][2]:08x}: port {int(got[name][i]):08x} "
                            f"ref {refs[name][i]:08x}" for i in bad[:5]))
        # FMIN / FMAX with a scalar second operand, as the ramp's NEON body uses them
        nbad = ncase = 0
        for bv in [0x00000000, 0x80000000, 0x3f800000, 0x7f800000, 0xff800000, 0x7fc00000, 0x7f812345]:
            for is_max in (False, True):
                for fn in (npx._fminmax, npx._fminmax_fast):
                    g = fn(a, _f(bv), is_max).view(U32)
                    nbad += sum(int(g[i]) != ref_fminmax(ops[i][0], bv, is_max) for i in range(len(ops)))
                    ncase += len(ops)
        ok &= nbad == 0
        print(f"[fmin/fmax] {ncase} cases vs FPMin / FPMax (_fminmax and _fminmax_fast): {nbad} differ")
        # FRECPE shortcut: every 17-bit prefix (sign, exponent, fraction<22:15>) x 24 low parts
        pre = np.arange(1 << 17, dtype=np.uint64) << np.uint64(15)
        suf = np.concatenate([[0, 1, 0x3fff, 0x4000, 0x7fff], rng.integers(0, 1 << 15, 19)]).astype(np.uint64)
        sweep = (pre[:, None] | suf[None, :]).ravel().astype(U32).view(F32)
        nbad = int((npx.frecpe32_fast(sweep).view(U32) != npx.frecpe32(sweep).view(U32)).sum())
        ok &= nbad == 0
        print(f"[frecpe32_fast] all {sweep.size} prefix x low-part patterns vs frecpe32: {nbad} differ")
        # CPU table
        ga = np.array([t[0] for t in CPU_GOLDEN], U32).view(F32)
        gb = np.array([t[1] for t in CPU_GOLDEN], U32).view(F32)
        gc = np.array([t[2] for t in CPU_GOLDEN], U32).view(F32)
        want = np.array([t[3:] for t in CPU_GOLDEN], U32)
        for label, fma, rpe, rps in (("reference", npx.fma32, npx.frecpe32, npx.frecps32),
                                     ("batch shortcuts", npx._fma32_fast, npx.frecpe32_fast, npx._frecps32_fast)):
            res = np.stack([fma(ga, gb, gc).view(U32), rpe(ga).view(U32), rps(ga, gb).view(U32)], 1)
            nbad = int((res != want).sum())
            ok &= nbad == 0
            print(f"[cpu table] {label}: {len(CPU_GOLDEN)} x (FMLA, FRECPE, FRECPS) measured on the M2 Max: "
                  f"{nbad} differ")
        # estimate accuracy over the normal range (sanity)
        xs = (10 ** rng.uniform(-4, 6, 10000)).astype(F32)
        est = npx.frecpe32(xs)
        err = np.abs(est.astype(np.float64) * xs.astype(np.float64) - 1.0)
        err2 = np.abs((est * npx.frecps32(xs, est)).astype(np.float64) * xs.astype(np.float64) - 1.0)
    print(f"[frecpe] |x*est - 1| max {err.max():.3g} (< 2^-8 = {2 ** -8:.3g}), after one FRECPS step max {err2.max():.3g}")
    ok &= err.max() < 2 ** -8
    return ok


# ------------------------------------------------------------------ 3. tiles
def frame_rgb(clip, frame, rgb_path):
    """Planar (3, 1080, 1920) float32 R, G, B of the frame: from --rgb, else decoded from the clip."""
    if rgb_path:
        rgb = np.fromfile(rgb_path, F32)
        if rgb.size != 3 * 1920 * 1080:
            raise RuntimeError(f"{rgb_path}: {rgb.size} floats, expected 3*1920*1080")
        print(f"[real] frame planes read from {rgb_path}")
        return rgb.reshape(3, 1080, 1920)
    try:
        import nvfilter as nv
        from denoise_video_v5 import probe, read_frames
        info = probe(clip)
        for i, pl in read_frames(clip, info["W"], info["H"], frame, frame + 1, fps=info["fps"]):
            if i == frame:
                rgb = nv.yuv_to_rgb(tuple(np.asarray(p, F32) for p in pl), "bt709", False)
                rgb = np.clip(rgb, 0, 255).astype(F32)
                if rgb.shape != (1080, 1920, 3):
                    raise RuntimeError(f"{clip} n={frame}: shape {rgb.shape}, expected 1080x1920x3")
                print(f"[real] {os.path.basename(clip)} n={frame} decoded (read_frames + yuv_to_rgb bt709 limited, "
                      f"clipped 0..255)")
                return np.ascontiguousarray(np.moveaxis(rgb, -1, 0))
    except RuntimeError:
        raise
    except Exception as e:  # noqa: BLE001
        raise RuntimeError(f"cannot decode {clip} n={frame}: {type(e).__name__}: {e}")
    raise RuntimeError(f"{clip}: frame {frame} not decoded")


def extreme_tiles(rng):
    """Inputs outside the 8-bit domain whose squares overflow float32 (inf -> FRECPE 0 ->
    FRECPS(inf, 0) = 2; NaN ratios in the ramp's scalar tail)."""
    out = []
    W = H = 120
    _, xx = np.mgrid[0:H, 0:W]
    out.append(("x1e18_120x120", ((30 + 190 * xx / (W - 1) + rng.normal(0, 4, (H, W))) * 1e18).astype(F32)))
    for W, H in ((51, 49), (49, 51), (50, 46)):
        for mag in (1e20, 1e30):
            luma = (100 + 5 * rng.standard_normal((H, W))).astype(F32)
            luma[H - 12:, W - 12:] = (mag * rng.standard_normal((12, 12))).astype(F32)
            out.append((f"hot{mag:g}_{W}x{H}", luma))
    hot = 100 + 5 * rng.standard_normal((120, 120))
    hot[50:62, 60:72] = 1e20 * (1 + rng.random((12, 12)))
    out.append(("hot1e20_mid_120x120", hot.astype(F32)))
    # one 5e19 pixel: only level 0 overflows (level 1 stays finite), so the NaN ratios reach the
    # last n & 3 pixels, where the ramp's scalar tail clamps them to 0 instead of propagating
    for W, H in ((51, 49), (49, 51), (97, 61)):
        spike = (100 + 5 * rng.standard_normal((H, W))).astype(F32)
        spike[H // 2, W // 3] = F32(5e19)
        out.append((f"spike5e19_{W}x{H}", spike))
    return out


def make_tiles(rng, quick, planes):
    tiles = []
    sizes = SIZES if quick else SIZES + EXTRA_SIZES
    for W, H in sizes:
        tiles.append((f"gauss_{W}x{H}", rng.normal(100.0, 5.0, (H, W)).astype(F32)))
    W = H = 120
    yy, xx = np.mgrid[0:H, 0:W]
    t = 40.0 + 1.2 * xx + 0.5 * yy + 60.0 * (xx > 57) + 30.0 * (yy > 70)
    tiles.append(("edge_120x120", (t + rng.normal(0, 3.0, (H, W))).astype(F32)))
    t = 20.0 + 200.0 * (((xx - 60) ** 2 + (yy - 60) ** 2) < 40 ** 2)
    tiles.append(("disc_120x120", (t + rng.normal(0, 2.0, (H, W))).astype(F32)))
    tiles.append(("flat_120x120", np.full((H, W), F32(128.0))))
    R, G, B = planes
    luma = npx.luma_601(R, G, B)
    for x, y, w, h in (REAL_TILES[:2] if quick else REAL_TILES):
        tiles.append((f"real_{w}x{h}_x{x}y{y}", np.ascontiguousarray(luma[y:y + h, x:x + w])))
    tiles += extreme_tiles(np.random.default_rng(3))
    return tiles


def coverage(inter):
    """Fraction of pixels inside the linear regions of the two ramps, per level."""
    parts = []
    for k in (0, 1):
        w1, w2 = inter[k]["w1"], inter[k]["w2"]
        parts.append(f"L{k}: w1 lin {np.mean((w1 > 0) & (w1 < 1)):.2f} zero {np.mean(w1 == 0):.2f}, "
                     f"w2 lin {np.mean((w2 > 0) & (w2 < 1)):.2f} zero {np.mean(w2 == 0):.2f}")
    return "; ".join(parts)


def run_tiles(tiles, workdir, label="port", gens=None):
    all_ok = True
    worst_rel = 0.0
    worst_exact = 1.0
    times = []
    nan_total = 0
    for tag, luma in tiles:
        H, W = luma.shape
        try:
            gen = genuine_pixnoise(luma, workdir, tag)
        except RuntimeError as e:
            print(f"== {tag}: harness failed: {e}")
            all_ok = False
            continue
        if gens is not None:
            gens[tag] = gen
        t0 = time.perf_counter()
        try:
            with np.errstate(all="ignore"):
                noise, weight, bright, inter = npx.pixnoise(luma, return_intermediates=True)
        except NotImplementedError as e:
            print(f"== {tag}: port refused: {e}")
            all_ok = False
            continue
        dt = time.perf_counter() - t0
        if (W, H) == (120, 120) and not tag.startswith(("x1e", "hot")):
            times.append(dt)
        line = []
        for s, mine in (("bright", bright), ("noise", noise), ("weight", weight)):
            exact, mabs, mrel, ndiff, nval, nnan = compare(mine, gen[s])
            worst_rel = max(worst_rel, mrel)
            worst_exact = min(worst_exact, exact)
            all_ok &= ndiff == 0
            nan_total += nnan
            extra = f", {nnan} NaN, {nval} differ NaN-aware" if (nnan or nval != ndiff) else ""
            line.append(f"{s} exact {exact:.6f} ({ndiff} diff{extra}) max_abs {mabs:.3g} max_rel {mrel:.3g}")
        print(f"== {tag} [{label}] {dt * 1e3:.0f} ms: " + " | ".join(line))
        with np.errstate(all="ignore"):
            print(f"   {coverage(inter)}")
    if times:
        print(f"[timing] {label}: {np.mean(times) * 1e3:.0f} ms per 120x120 tile "
              f"(min {min(times) * 1e3:.0f}, n = {len(times)})")
    print(f"[tiles] NaN pixels in the genuine maps: {nan_total} (all compared bitwise, payloads included)")
    return all_ok, worst_exact, worst_rel


def run_batch(tiles, gens):
    """nv_pixnoise.pixnoise_batch (the batch path of nv_curve.genuine_noise_curve) against the genuine
    maps of run_tiles: the tiles of each size in one batch, and every tile alone (B = 1); noise, weight
    and bright must be bit-identical (NaN payloads included).  The tiles of a size are fed with
    different weights (1.0 and a random 0 / 1 mask): FUN_00998d98 only multiplies the incoming weight,
    so the genuine weight map times the mask is the expected weight."""
    groups = {}
    for tag, luma in tiles:
        if tag in gens:
            groups.setdefault(luma.shape, []).append((tag, luma))
    rng = np.random.default_rng(42)
    n_maps = bad = 0
    t_batch = 0.0
    n_tiles = 0
    for shape, lst in groups.items():
        L = np.stack([l for _, l in lst])
        Wt = np.where(rng.random(L.shape) < 0.15, F32(0.0), F32(1.0)).astype(F32)
        Wt[0] = 1.0
        with np.errstate(all="ignore"):
            t0 = time.perf_counter()
            nb, wb, bb = npx.pixnoise_batch(L, Wt)
            t_batch += time.perf_counter() - t0
            n_tiles += len(lst)
            for i, (tag, luma) in enumerate(lst):
                g = gens[tag]
                n1, w1, b1 = npx.pixnoise_batch(luma[None], Wt[i][None])
                want_w = (g["weight"] * Wt[i]).astype(F32)
                for s, want, got_b, got_1 in (("noise", g["noise"], nb[i], n1[0]), ("weight", want_w, wb[i], w1[0]),
                                              ("bright", g["bright"], bb[i], b1[0])):
                    for label, got in (("batch", got_b), ("B=1", got_1)):
                        n_maps += 1
                        nd = int((np.ascontiguousarray(got).view(U32) != want.view(U32)).sum())
                        if nd:
                            bad += 1
                            print(f"   {tag} {s} [{label}]: {nd} px differ from the genuine")
    print(f"[batch] pixnoise_batch vs `nvh pixnoise`: {n_maps - bad}/{n_maps} maps bit-identical "
          f"({n_tiles} tiles in {len(groups)} size batches, and each tile alone); "
          f"{t_batch / max(n_tiles, 1) * 1e3:.1f} ms per tile in these (small) batches")
    return bad == 0 and n_maps > 0


def _frecps_plain(a, b):
    """FRECPS as a plain fused 2 - a*b (inf * 0 -> NaN instead of 2): the port before 2026-10-16."""
    return npx.fma32((np.asarray(a, F32).view(U32) ^ U32(0x80000000)).view(F32), b, F32(2.0))


def _ramp_propagating(a, b, va, vb, x, simd=None):
    """The ramp with the NEON body's NaN-propagating FMIN / FMAX also on the scalar tail (the
    port before 2026-10-16)."""
    y = npx.__dict__["_orig_ramp"](a, b, va, vb, x, simd)
    n = np.asarray(x).size
    m = n & ~3 if (npx.SIMD if simd is None else simd) else 0
    xt = np.ascontiguousarray(x, F32).ravel()[m:]
    y[m:] = np.where(np.isnan(xt), xt, y[m:])            # NaN in -> NaN out on the tail too
    return y


def sensitivity(tiles, workdir):
    """Disable one NEON peculiarity at a time and count the pixels that then differ."""
    sub = [t for t in tiles if t[0] in ("gauss_120x108", "gauss_97x61") or t[0].startswith("real_120x120")][:3]
    ext = [t for t in tiles if t[0].startswith(("hot", "spike"))]
    for name, attr, fn, tl in (("FRECPS without the inf*0 -> 2 case", "frecps32", _frecps_plain, ext),
                               ("ramp tail propagating NaN", "ramp", _ramp_propagating, ext)):
        orig = getattr(npx, attr)
        npx.__dict__["_orig_" + attr] = orig
        setattr(npx, attr, fn)
        try:
            parts = []
            for tag, luma in tl:
                gen = genuine_pixnoise(luma, workdir, tag)
                with np.errstate(all="ignore"):
                    noise, weight, bright = npx.pixnoise(luma)
                parts.append(f"{tag} {compare(noise, gen['noise'])[3] + compare(weight, gen['weight'])[3]}")
            print(f"[sensitivity] {name}: px differing (noise + weight, bitwise): " + ", ".join(parts))
        finally:
            setattr(npx, attr, orig)
    variants = {
        "ratio by true division (no FRECPE/FRECPS)":
            ("ratio_eps", lambda a, b, eps, simd=None: npx.__dict__["_orig_ratio_eps"](a, b, eps, False)),
        "ramp without FMA":
            ("ramp", lambda a, b, va, vb, x, simd=None: npx.__dict__["_orig_ramp"](a, b, va, vb, x, False)),
        "2x2 mean in scalar order":
            ("down2", lambda x, simd=None: npx.__dict__["_orig_down2"](x, False)),
        "former nvfilter._gup2 style up-sampling ((3x+prev)*0.25 twice)":
            ("_up2_neon", lambda lo: _gup2_like(lo)),
    }
    for name, (attr, fn) in variants.items():
        orig = getattr(npx, attr)
        npx.__dict__["_orig_" + attr] = orig
        setattr(npx, attr, fn)
        try:
            for tag, luma in sub:
                gen = genuine_pixnoise(luma, workdir, tag)
                noise, weight, bright = npx.pixnoise(luma)
                parts = []
                for s, mine in (("noise", noise), ("weight", weight)):
                    exact, mabs, mrel, ndiff, _, _ = compare(mine, gen[s])
                    parts.append(f"{s} {ndiff} px differ (max_rel {mrel:.2g})")
                print(f"[sensitivity] {name}: {tag}: " + ", ".join(parts))
        finally:
            setattr(npx, attr, orig)


def _gup2_like(lo):
    """Up-sampling as the former repro/nvfilter._gup2 (before 2026-10-16) computed it (vertical then horizontal,
    (3*a + neighbour) * 0.25 per pass) on the NEON-sized core, for the sensitivity check."""
    def axis_up(a, ax):
        m = a.shape[ax]
        prev = np.take(a, [max(i - 1, 0) for i in range(m)], ax)
        nxt = np.take(a, [min(i + 1, m - 1) for i in range(m)], ax)
        out = np.stack([(F32(3) * a + prev) * F32(0.25), (F32(3) * a + nxt) * F32(0.25)], axis=ax + 1)
        shp = list(a.shape)
        shp[ax] = 2 * m
        return out.reshape(shp).astype(F32)
    return axis_up(axis_up(lo, 0), 1)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--clip", default=os.path.join(ROOT, "test_v02.mp4"), help="clip the real tiles come from")
    ap.add_argument("--frame", type=int, default=312)
    ap.add_argument("--rgb", default=None, help="1920x1080 planar R,G,B float32 dump to use instead of decoding")
    ap.add_argument("--sensitivity", action="store_true")
    ap.add_argument("--keep", default=None, help="directory for the harness outputs (default: temp)")
    args = ap.parse_args()
    nc.require_guard()
    print(f"[guard] nvh selftest exit 3 confirmed; SIMD flag of the port: {npx.SIMD}")
    try:
        planes = frame_rgb(args.clip, args.frame, args.rgb)
    except RuntimeError as e:                                 # no real tiles -> no pass
        print(f"[real] FAIL: {e}")
        sys.exit(2)
    workdir = args.keep or tempfile.mkdtemp(prefix="verify_pixnoise_")
    os.makedirs(workdir, exist_ok=True)
    t0 = time.time()
    ok_ref, _ = check_refmodel(workdir)
    ok_help = check_helpers()
    rng = np.random.default_rng(20261016)
    tiles = make_tiles(rng, args.quick, planes)
    n_real = sum(1 for tag, _ in tiles if tag.startswith("real_"))
    gens = {}
    ok_tiles, worst_exact, worst_rel = run_tiles(tiles, workdir, gens=gens)
    ok_batch = run_batch(tiles, gens)
    if args.sensitivity:
        sensitivity(tiles, workdir)
    ok = ok_ref and ok_help and ok_tiles and ok_batch and n_real > 0
    print(f"[summary] refmodel {ok_ref}, helpers {ok_help}, tiles {ok_tiles} "
          f"(worst exact fraction {worst_exact:.6f}, worst max_rel {worst_rel:.3g}), batch path {ok_batch}; "
          f"{len(tiles)} tiles ({n_real} real), {time.time() - t0:.0f} s -> {'OK' if ok else 'FAIL'}")
    if not args.keep:
        for f in os.listdir(workdir):
            os.unlink(os.path.join(workdir, f))
        os.rmdir(workdir)
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
