"""Neat Video 6 (arm64 slice) FUN_00998d98: the per-pixel noise estimate that
EstimateNoiseLevelCurve (FUN_009b6c8c) runs on every tile, ported to numpy.

    pixnoise(luma) -> noise, weight, bright          (all float32 planes of the tile)

The genuine routine (ghidra decompile analysis/rtti/enlc/fn_0x998d98.c, disassembly read for
the instruction order) works on two pyramid levels k = 0, 1 of sizes (W >> k, H >> k)
(FUN_0099c3a4) and, per level:

    B     = box3 of src, border replicated (FUN_009944c0 r=1 mode 0: rows then columns)
    bright = B                                      (level 0 only)
    lo    = 2x2 mean of B                            (FUN_009f15b4 per row pair)
    band  = src - up2(lo)                            (FUN_00990da8: (3,1)/4 interpolation)
    for the 16 orientations i (FUN_00990448 kernels of angle ANGLES[i]):
        f    = line box r=4 along kernel i, then r=RAD2[i] along kernel IDX2[i] (mode 1)
        E_i  = sqrt(|box3_zero(f*f)|) * (1 / |ref[k][i]|)    (FUN_009ed4e0)
        g    = the same oriented filter applied to band - f
        F_i  = f + g
    idx   = argmax_i E_i (FUN_009ee5fc: >= so the later index wins, start value 0)
    S     = F_idx (FUN_0098ed34),  R = band - S
    noise = sqrt(|box3_zero(R*R)|)                    (level 0 only)
    ER    = sqrt(|box5_zero(R*R)|),  ES = sqrt(|box5_zero(S*S)|)        (r = 2, mode 1)
    w1    = ramp(ES / (ER + 1e-4); 1 at <= 0.8 .. 0 at >= 1.2)   (FUN_009ee04c, FUN_009ee93c)
    E10   = sqrt(|box21_edge(R*R)|)                                      (r = 10, mode 0)
    w2    = ramp(ER / (E10 + 1e-4); 1 at <= 1.1 .. 0 at >= 1.5)
    weight *= up2(w1) * up2(w2)   (w1, w2 of level 1 are up-sampled to the tile size first)
    src   = lo

Everything is float32 and the operation order of the genuine code is kept, including the
NEON peculiarities that change the last bits (all verified bit-exact against the plugin with
analysis/noise/verify_pixnoise.py through analysis/harness/nvh):
  * FUN_009ee04c a/(b+eps): the first n & ~3 pixels use FRECPE + one FRECPS Newton step
    (a * (est * (2 - d*est))), the last n & 3 pixels a true division.  FRECPS(inf, 0) = +2
    (FPRecipStepFused), so an infinite denominator gives 0, not NaN.
  * FUN_009ee93c ramp: y = fma(x, slope, intercept) (FMLA), then FMIN / FMAX for the first
    n & ~3 pixels (a NaN propagates); intercept + slope*x (two roundings) and fcmp / fcsel gt
    clamps for the rest (a NaN comes out as the lower bound, vb = 0 in FUN_00998d98).
  * FUN_009f15b4 2x2 mean: the first W & ~7 columns of a row pair sum ((x0+x1)+(y0+y1)),
    the rest (((x0+x1)+y0)+y1).
  * FUN_00993df0 / FUN_009f1860 up-sampling (Wc >= 4, Hc >= 2): horizontal pass first,
    L = x*0.75 + prev*0.25, R = x*0.75 + next*0.25 (border replicated), then vertical
    out[2i-1] = h[i-1]*0.75 + h[i]*0.25, out[2i] = h[i-1]*0.25 + h[i]*0.75, out[0] = h[0],
    out[2Hc-1] = h[Hc-1]; an odd target width repeats the last column, an odd height the
    last row.  FUN_00990da8 computes the odd column / row of the band as
    (band[last] - src[last]) + src[odd].  Smaller inputs take the scalar code
    (a = 3x + x_neighbour row-wise, then (3a + a_neighbour) * 0.0625).
  * FUN_009f11fc luma: a*R then FMLA with G*b and B*c for the first n & ~3 pixels, the rest
    (R*a + G*b) + B*c.  luma_601 is the only implementation (nv_curve uses it).

The float helpers follow the AArch64 pseudocode (FPMulAdd, FPRecipEstimate, FPRecipStepFused,
FPMin / FPMax) with the FPCR the plugin runs under (it never writes FPCR: no msr fpcr and no
fesetenv in the arm64 slice): round to nearest, subnormals kept, NaN payloads propagated,
inf * 0 and the FLT_MAX / 2^128 rounding boundary as the hardware does them.  Checked against
the CPU itself (inline-asm NEON FMLA / FRECPE / FRECPS / FMIN / FMAX on the M2 Max, 373,018
operand sets including specials and subnormals, 0 differ; scratch run of 2026-10-16) and in
verify_pixnoise.py against value-based references and a table of CPU results.

Only numpy is used; the line kernels and running box filters come from nv_lines.py.

Batch fast path (2026-10-16): pixnoise_batch(luma (B, H, W), weight) runs B tiles of one size at
once (internally batch-innermost, pixnoise_bm on (H, W, B)) with the same float32 operations in the
same order: bit-identical to pixnoise() per tile, about 2.5 ms per 120x120 tile in batches of ~90
(pixnoise() itself, kept unchanged as the reference, takes ~50 ms per tile).  Its float helpers
frecpe32_fast / _fma32_fast / _frecps32_fast / _fminmax_fast are exact shortcuts of the reference
helpers (table lookup; float64 evaluation with element-wise fallback where a double rounding or a
special value is possible).

Verification (analysis/noise/verify_pixnoise.py, 2026-10-16): 30 tiles (gaussian 120x108,
108x120, 120x120, 48x120, 97x61, 95x59, 50x46, 64x33, 10x9, 9x10, 8x8; edge + gradient; disc;
flat; five tiles of frame 312 of test_v02 decoded from the clip; 11 tiles outside the 8-bit
domain whose squares overflow: x1e18, 1e20 / 1e30 blocks, single 5e19 pixels) -> noise, weight
and bright bit-identical to `nvh pixnoise` (43,517 NaN pixels compared bitwise, payloads
included); about 50 ms per 120x120 tile.
Not covered: the scalar up-sampling of a coarse plane narrower than 2 px (the genuine reads
past the row there; the port raises NotImplementedError, tiles narrower than 8 px), and the
scalar weight up-sampling (never taken for tiles of at least 8x4).  luma_601 follows the
disassembly of FUN_009f11fc but has no harness entry point of its own (it is covered end to
end by `nvh curve`).
"""
import os
import sys

import numpy as np

_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)
import nv_lines as nl  # noqa: E402
from nv_lines import build_kernels, direction_filter, box_rows, box_cols  # noqa: E402

F32 = np.float32
F64 = np.float64
U32 = np.uint32

SIMD = nl.SIMD   # DAT_014a67a8 of the analysed build (1): NEON paths of the helpers
LUMA_601 = (F32(0.2989), F32(0.5866), F32(0.1145))   # 0x3e99096c 0x3f162b6b 0x3dea7efa (FUN_0097bb88)

# Per-orientation normalisers 1/|ref| of FUN_00998d98: floats of the white-noise reference
# model DAT_01561db0 (FUN_009b4428 on a deterministic 256x256 noise plane) at byte offset
# 0x500 + level*0x40 + dir*4 (set 3, levels 0 and 1, 16 directions).  Dumped with
#   analysis/harness/nvh refmodel refmodel.bin      (0x1f54 bytes, two runs identical)
# on 2026-10-16; verify_pixnoise.py re-derives them from a fresh dump.
REF_NORM = np.array([
    [0x413d6846, 0x4105ad92, 0x40f8eb0a, 0x41077463, 0x413b18fe, 0x410ae91e, 0x410006ea, 0x41060937,
     0x413dd31f, 0x4104a534, 0x40fb1afd, 0x41043ba1, 0x413c2996, 0x410622ce, 0x40fe630e, 0x4104b3f3],
    [0x406e220a, 0x4028b230, 0x4024f222, 0x402406d4, 0x403b0d7d, 0x401e9e1b, 0x4026d658, 0x4033a0cf,
     0x406c5975, 0x4030bbf8, 0x4029441c, 0x4026107a, 0x4039f6b7, 0x4026c12e, 0x4023c705, 0x402dacc0]],
    U32).view(F32)
REF_OFFSET = 0x500          # byte offset of level 0 inside the reference model
REF_LEVEL_STRIDE = 0x40     # bytes per level (16 floats)


def ref_from_model(model_bytes):
    """The (2, 16) normaliser table out of a `nvh refmodel` dump (bytes or uint8 array)."""
    m = np.frombuffer(bytes(model_bytes), np.uint8)
    out = np.empty((2, 16), F32)
    for k in range(2):
        o = REF_OFFSET + k * REF_LEVEL_STRIDE
        out[k] = m[o:o + 64].view(F32)
    return out


# ----------------------------------------------------------------------------- float helpers
# AArch64 semantics under the FPCR the plugin runs with: the arm64 slice never writes FPCR (no
# msr fpcr and no fesetenv / fesetround in the disassembly), so the macOS default applies:
# round to nearest even, FZ = 0 (subnormals kept), DN = 0 (NaN payloads propagate), AH = 0.
_SIGN = U32(0x80000000)
_QUIET = U32(0x00400000)
_DEFAULT_NAN = U32(0x7fc00000)
_FLT_MAX = np.finfo(F32).max
_OVF_MID = 2.0 ** 128 - 2.0 ** 103      # half-way between FLT_MAX and 2^128 (exact in float64)


def _bits(x):
    return np.asarray(x, F32).view(U32)


def _process_nans(*ops):
    """FPProcessNaNs / FPProcessNaNs3 with DN = 0 on uint32 bit patterns: the first signalling
    NaN operand (quieted), else the first quiet NaN operand.  Returns (is_nan, bits)."""
    shape = np.broadcast(*ops).shape
    done = np.zeros(shape, bool)
    out = np.zeros(shape, U32)
    for quiet in (False, True):
        for u in ops:
            u = np.broadcast_to(u, shape)
            hit = ~done & ((u & U32(0x7fffffff)) > U32(0x7f800000)) & (((u & _QUIET) != 0) == quiet)
            out = np.where(hit, u | _QUIET, out)
            done |= hit
    return done, out


def fma32(a, b, c):
    """float32 a*b + c with one rounding: FMLA / FMADD, i.e. FPMulAdd(addend = c, a, b).

    The product of two float32 is exact in float64; s = round64(a*b + c) then differs from the
    exact sum only below float32 resolution, except when s lands exactly half-way between two
    float32 neighbours although the exact sum does not - the TwoSum error term then picks the
    side (also at the overflow boundary between FLT_MAX and 2^128).  Special values as
    FPMulAdd: inf*0 and inf-inf give the default NaN; NaN operands propagate as FPProcessNaNs3
    (addend first, signalling before quiet), except that a quiet-NaN addend with inf*0 gives
    the default NaN; exact zeros take the sign rules of IEEE addition (+0 unless both -0)."""
    a = np.asarray(a, F32)
    b = np.asarray(b, F32)
    c = np.asarray(c, F32)
    with np.errstate(invalid="ignore", over="ignore"):
        p = a.astype(F64) * b.astype(F64)                # exact: 24 x 24 bits fit in 53
        c64 = c.astype(F64)
        s = p + c64
        t = s - p                                        # TwoSum: err = exact (p + c) - s
        err = (p - (s - t)) + (c64 - t)
        r = s.astype(F32)                                # nearest float32 to s (ties to even)
        rd = r.astype(F64)
        up = s > rd
        other = np.where(up, np.nextafter(r, F32(np.inf)), np.nextafter(r, F32(-np.inf)))
        tie = np.isfinite(rd) & (rd != s) & ((s - rd) == (other.astype(F64) - s))
        out = np.where(tie & np.where(up, err > 0, err < 0), other, r)
        # s exactly on the FLT_MAX / 2^128 midpoint rounds to inf; an exact sum below it does not
        below = (np.abs(s) == _OVF_MID) & (err * s < 0)
        out = np.where(below, np.copysign(_FLT_MAX, s).astype(F32), out).astype(F32)
    nan = np.isnan(a) | np.isnan(b) | np.isnan(c)
    if np.any(nan):
        done, nb = _process_nans(_bits(c), _bits(a), _bits(b))
        quiet_addend = np.isnan(c) & ((_bits(c) & _QUIET) != 0)
        inf_zero = (np.isinf(a) & (b == 0)) | ((a == 0) & np.isinf(b))
        nb = np.where(quiet_addend & inf_zero, _DEFAULT_NAN, nb)
        out = np.where(done, nb.view(F32), out).astype(F32)
    return out


def frecpe32(x):
    """AArch64 FRECPE (FPRecipEstimate) on float32, FPCR as above (8-bit estimate, AH = 0).

    NaN -> the operand quieted (payload and sign kept); +-inf -> +-0; +-0 -> +-inf;
    0 < |x| < 2^-128 -> +-inf (overflow, round to nearest); otherwise the 9-bit table estimate
    RecipEstimate of the fraction scaled to [0.5, 1).  A subnormal operand is normalised first:
    fraction<51> = 1 -> fraction<50:0>:'0' (exponent 0), else fraction<49:0>:'00' (exponent -1);
    result exponent 253 - exp, results with exponent 0 / -1 come out subnormal."""
    x = np.ascontiguousarray(x, F32)
    u = x.view(U32)
    bits = u.astype(np.int64)
    sign = bits & 0x80000000
    exp = (bits >> 23) & 0xff
    frac = bits & 0x7fffff
    nan = (exp == 255) & (frac != 0)
    inf = (exp == 255) & (frac == 0)
    zero = (exp == 0) & (frac == 0)
    tiny = (exp == 0) & (frac != 0) & (frac < (1 << 21))        # |x| < 2^-128 = 2^21 * 2^-149
    mask52 = (1 << 52) - 1
    fraction = frac << 29                                        # operand<22:0> : Zeros(29)
    sub = exp == 0
    top = (fraction >> 51) & 1
    e = np.where(sub & (top == 0), -1, exp)
    fraction = np.where(sub & (top == 1), (fraction << 1) & mask52,
                        np.where(sub & (top == 0), (fraction << 2) & mask52, fraction))
    scaled = (1 << 8) | ((fraction >> 44) & 0xff)                # 9 bits, 256..511
    result_exp = 253 - e
    est = ((1 << 19) // (scaled * 2 + 1) + 1) // 2               # RecipEstimate: 256..511
    fr = (est & 0xff) << 44                                      # 52-bit fraction
    fr = np.where(result_exp == 0, (1 << 51) | (fr >> 1), fr)
    fr = np.where(result_exp == -1, (1 << 50) | (fr >> 2), fr)
    result_exp = np.where(result_exp == -1, 0, result_exp)
    out = sign | ((result_exp & 0xff) << 23) | (fr >> 29)
    out = np.where(nan, bits | 0x400000, out)
    out = np.where(inf, sign, out)
    out = np.where(zero | tiny, sign | 0x7f800000, out)
    return out.astype(U32).view(F32)


def frecps32(a, b):
    """AArch64 FRECPS (FPRecipStepFused): 2.0 - a*b with a single rounding.  The pseudocode
    negates op1 first (FPNeg also flips a NaN's sign) and takes NaNs as FPProcessNaNs(-a, b);
    inf * 0 in either order gives +2.0 (not the default NaN of a plain fused multiply-add);
    inf * x gives inf of sign(-a) xor sign(b)."""
    a = np.asarray(a, F32)
    b = np.asarray(b, F32)
    na = (_bits(a) ^ _SIGN).view(F32)                            # FPNeg (bit flip, NaNs too)
    out = fma32(na, b, F32(2.0))                                 # addend 2.0 is never a NaN
    two = (np.isinf(na) & (b == 0)) | ((na == 0) & np.isinf(b))
    return np.where(two, F32(2.0), out).astype(F32)


def _fminmax(x, c, is_max):
    """AArch64 FMIN / FMAX (FPMin / FPMax, DN = 0) of the array x (op1) and the scalar c (op2):
    NaNs propagate (FPProcessNaNs(x, c)), FPMax(+0, -0) = +0, FPMin(+0, -0) = -0."""
    x = np.asarray(x, F32)
    c = F32(c)
    out = np.where(x > c, x, c) if is_max else np.where(x < c, x, c)
    zz = (x == 0) & (c == 0)
    if np.any(zz):
        neg = (np.signbit(x) & np.signbit(c)) if is_max else (np.signbit(x) | np.signbit(c))
        out = np.where(zz, np.where(neg, F32(-0.0), F32(0.0)), out)
    nan = np.isnan(x) | np.isnan(c)
    if np.any(nan):
        done, nb = _process_nans(_bits(x), _bits(c))
        out = np.where(done, nb.view(F32), out)
    return np.asarray(out, F32)


def ratio_eps(a, b, eps, simd=None):
    """FUN_009ee04c(eps, a, b, dst, n): dst = a / (b + eps) on flat float32 arrays.

    NEON path for the first n & ~3 elements: d = eps + b, est = FRECPE(d),
    dst = a * (est * FRECPS(d, est)); the remainder uses the scalar division.  With an infinite
    d the NEON path gives est = 0, FRECPS(inf, 0) = 2 and hence 0 for a finite a (the division
    of the tail gives 0 as well); a NaN d propagates."""
    simd = SIMD if simd is None else simd
    a = np.ascontiguousarray(a, F32).ravel()
    b = np.ascontiguousarray(b, F32).ravel()
    eps = F32(eps)
    n = a.size
    m = (n & ~3) if simd else 0
    out = np.empty(n, F32)
    if m:
        d = eps + b[:m]
        est = frecpe32(d)
        step = frecps32(d, est)
        out[:m] = a[:m] * (est * step)
    if m < n:
        out[m:] = a[m:] / (b[m:] + eps)
    return out


_C_1E5 = np.array([0x3727c5ac], U32).view(F32)[0]      # 1e-05
_C_099999 = np.array([0x3f7fff58], U32).view(F32)[0]   # 0.99999
_C_100001 = np.array([0x3f800054], U32).view(F32)[0]   # 1.00001


def ramp(a, b, va, vb, x, simd=None):
    """FUN_009ee93c(a, b, va, vb, src, dst, n): linear ramp va at x <= a .. vb at x >= b.

    slope = (vb - va) / (b - a), intercept = va - a*slope (float32; a and b are first widened
    when b - a <= b*1e-5, fcmp / fcsel hi: unordered keeps them).  The branch is chosen by
    fcmp vb, va / b.le: descending (vb <= va or unordered) clamps to [vb, va], ascending to
    [va, vb]; with hi = the upper and lo = the lower bound:
      NEON body (first n & ~3 elements): y = fma(x, slope, intercept) (FMLA), FMIN with hi,
        FMAX with lo - a NaN propagates;
      scalar tail (0x9eeb04 / 0x9eebcc, and the 4-wide fcmgt / bsl copy of it): y = intercept
        + slope*x (fmul, fadd), y = (y > hi) ? hi : y, y = (y > lo) ? y : lo (fcmp / fcsel gt) -
        a NaN fails both compares and comes out as lo (vb in the descending branch)."""
    simd = SIMD if simd is None else simd
    a, b, va, vb = F32(a), F32(b), F32(va), F32(vb)
    if F32(b - a) <= F32(b * _C_1E5):
        a = F32(F32(a * _C_099999) + F32(-_C_1E5))
        b = F32(F32(b * _C_100001) + _C_1E5)
    slope = F32(F32(vb - va) / F32(b - a))
    icpt = F32(va - F32(a * slope))
    lo, hi = (vb, va) if not (vb > va) else (va, vb)
    x = np.ascontiguousarray(x, F32).ravel()
    n = x.size
    m = (n & ~3) if simd else 0
    y = np.empty(n, F32)
    if m:
        y[:m] = _fminmax(_fminmax(fma32(x[:m], slope, icpt), hi, False), lo, True)
    if m < n:
        t = icpt + slope * x[m:]
        t = np.where(t > hi, hi, t)
        y[m:] = np.where(t > lo, t, lo)
    return y


def argmax_maps(maps):
    """FUN_009ee5fc: per-pixel argmax over maps (count, n): start value 0, index 0, updated
    on >= so the later index wins ties.  Returns (idx uint8, max)."""
    best = np.zeros(maps.shape[1:], F32)
    idx = np.zeros(maps.shape[1:], np.uint8)
    for j in range(maps.shape[0]):
        hit = best <= maps[j]
        best = np.where(hit, maps[j], best)
        idx = np.where(hit, np.uint8(j), idx)
    return idx, best


def weighted_sum3(a, b, c, R, G, B, simd=None):
    """FUN_009f11fc(a, b, c, R, G, B, dst, n): dst = a*R + b*G + c*B (the luma of
    EstimateNoiseLevelCurve with 0.2989, 0.5866, 0.1145).  NEON path for the first n & ~3
    pixels: fma(B, c, fma(G, b, a*R)); the rest (R*a + G*b) + B*c."""
    simd = SIMD if simd is None else simd
    shape = np.shape(R)
    R = np.ascontiguousarray(R, F32).ravel()
    G = np.ascontiguousarray(G, F32).ravel()
    B = np.ascontiguousarray(B, F32).ravel()
    a, b, c = F32(a), F32(b), F32(c)
    n = R.size
    m = (n & ~3) if simd else 0
    out = np.empty(n, F32)
    if m:
        out[:m] = fma32(B[:m], c, fma32(G[:m], b, a * R[:m]))
    if m < n:
        out[m:] = (R[m:] * a + G[m:] * b) + B[m:] * c
    return out.reshape(shape)


def luma_601(R, G, B, simd=None):
    """The luma plane FUN_009b6c8c hands to FUN_00998d98: FUN_0097bb88 -> FUN_009f11fc with
    the BT.601 weights LUMA_601 (float32).  The single implementation of this luma: nv_curve
    uses it too."""
    return weighted_sum3(*LUMA_601, R, G, B, simd)


# ----------------------------------------------------------------------------- pyramid pieces
def level_sizes(W, H, levels=6):
    """FUN_0099c3a4: (W >> k, H >> k) per level with the odd flags (W_k & 1, H_k & 1)."""
    return [(W >> k, H >> k, (W >> k) & 1, (H >> k) & 1) for k in range(levels)]


def down2(x, simd=None):
    """FUN_009f15b4 applied to every row pair: 2x2 mean, (H >> 1) x (W >> 1).

    NEON path: the first W & ~7 input columns of a row pair are summed as
    ((x0 + x1) + (y0 + y1)) (FADDP, FADD); the remaining columns (((x0 + x1) + y0) + y1)."""
    simd = SIMD if simd is None else simd
    x = np.ascontiguousarray(x, F32)
    H, W = x.shape
    h, w = H >> 1, W >> 1
    x = x[:2 * h, :2 * w]
    x0, x1 = x[0::2, 0::2], x[0::2, 1::2]
    y0, y1 = x[1::2, 0::2], x[1::2, 1::2]
    out = np.empty((h, w), F32)
    c = ((W & ~7) >> 1) if simd else 0
    if c:
        out[:, :c] = ((x0[:, :c] + x1[:, :c]) + (y0[:, :c] + y1[:, :c])) * F32(0.25)
    if c < w:
        out[:, c:] = (((x0[:, c:] + x1[:, c:]) + y0[:, c:]) + y1[:, c:]) * F32(0.25)
    return out


def _up2_neon(lo):
    """The (3,1)/4 up-sampling of FUN_009f173c / FUN_009f1860 (Wc >= 4, Hc >= 2) without
    the odd column / row: (2Hc, 2Wc)."""
    Hc, Wc = lo.shape
    prev = np.concatenate([lo[:, :1], lo[:, :-1]], 1)
    nxt = np.concatenate([lo[:, 1:], lo[:, -1:]], 1)
    xs = lo * F32(0.75)
    L = xs + prev * F32(0.25)
    R = xs + nxt * F32(0.25)
    h = np.empty((Hc, 2 * Wc), F32)
    h[:, 0::2] = L
    h[:, 1::2] = R
    out = np.empty((2 * Hc, 2 * Wc), F32)
    out[0] = h[0]
    out[2 * Hc - 1] = h[Hc - 1]
    hp, hn = h[:-1], h[1:]
    out[1:2 * Hc - 2:2] = hp * F32(0.75) + hn * F32(0.25)
    out[2:2 * Hc - 1:2] = hp * F32(0.25) + hn * F32(0.75)
    return out


def _up2_scalar(lo, oddW, oddH):
    """The scalar branch of FUN_00993df0: a = 3*x + x_neighbour row (upper / lower output
    row), out = (3*a + a_neighbour) * 0.0625 column-wise, borders (4*a) * 0.0625."""
    Hc, Wc = lo.shape
    if Wc < 2:
        raise NotImplementedError("the genuine scalar up-sampling reads past the row for Wc < 2")
    out = np.empty((2 * Hc + oddH, 2 * Wc + oddW), F32)
    x3 = lo * F32(3.0)
    up = x3 + np.concatenate([lo[:1], lo[:-1]], 0)        # neighbour above (replicated)
    dn = x3 + np.concatenate([lo[1:], lo[-1:]], 0)        # neighbour below
    for src_rows, r0 in ((up, 0), (dn, 1)):
        a3 = src_rows * F32(3.0)
        row = np.empty((Hc, 2 * Wc), F32)
        row[:, 0] = (src_rows[:, 0] * F32(4.0)) * F32(0.0625)
        row[:, 2::2] = (a3[:, 1:] + src_rows[:, :-1]) * F32(0.0625)
        row[:, 1:-1:2] = (a3[:, :-1] + src_rows[:, 1:]) * F32(0.0625)
        row[:, 2 * Wc - 1] = (src_rows[:, Wc - 1] * F32(4.0)) * F32(0.0625)
        out[r0:2 * Hc:2, :2 * Wc] = row
    if oddW:
        out[:2 * Hc, 2 * Wc] = out[:2 * Hc, 2 * Wc - 1]
    if oddH:
        out[2 * Hc] = out[2 * Hc - 1]
    return out


def up2(lo, oddW, oddH, simd=None):
    """FUN_00993df0(src, dst, Wc, Hc, oddW, oddH): (Hc, Wc) -> (2Hc + oddH, 2Wc + oddW)."""
    simd = SIMD if simd is None else simd
    lo = np.ascontiguousarray(lo, F32)
    Hc, Wc = lo.shape
    oddW, oddH = int(oddW) & 1, int(oddH) & 1
    if Hc < 2 or Wc < 4 or not simd:
        return _up2_scalar(lo, oddW, oddH)
    core = _up2_neon(lo)
    if not (oddW or oddH):
        return core
    out = np.empty((2 * Hc + oddH, 2 * Wc + oddW), F32)
    out[:2 * Hc, :2 * Wc] = core
    if oddW:
        out[:2 * Hc, 2 * Wc] = core[:, 2 * Wc - 1]
    if oddH:
        out[2 * Hc] = out[2 * Hc - 1]
    return out


def band_from(lo, src, oddW, oddH, simd=None):
    """FUN_00990da8(lo, src, dst, Wc, Hc, oddW, oddH): dst = src - up2(lo) of size
    (2Hc + oddH, 2Wc + oddW).  NEON path (FUN_009f1860): the odd column is
    (dst[last col] - src[last col]) + src[odd col], then the odd row likewise."""
    simd = SIMD if simd is None else simd
    lo = np.ascontiguousarray(lo, F32)
    src = np.ascontiguousarray(src, F32)
    Hc, Wc = lo.shape
    oddW, oddH = int(oddW) & 1, int(oddH) & 1
    H2, W2 = 2 * Hc + oddH, 2 * Wc + oddW
    if src.shape != (H2, W2):
        raise ValueError(f"src {src.shape} does not match {H2}x{W2}")
    if Hc < 2 or Wc < 4 or not simd:
        return src - _up2_scalar(lo, oddW, oddH)
    out = np.empty((H2, W2), F32)
    out[:2 * Hc, :2 * Wc] = src[:2 * Hc, :2 * Wc] - _up2_neon(lo)
    if oddW:
        c = 2 * Wc
        out[:2 * Hc, c] = (out[:2 * Hc, c - 1] - src[:2 * Hc, c - 1]) + src[:2 * Hc, c]
    if oddH:
        r = 2 * Hc
        out[r] = (out[r - 1] - src[r - 1]) + src[r]
    return out


def box2(x, r, mode, simd=None):
    """FUN_009944c0 with the axis-aligned kernels of FUN_00998d98: rows (+0x3824) first,
    then columns (+0x3825)."""
    return box_cols(box_rows(x, r, mode, simd), r, mode, simd)


# ----------------------------------------------------------------------------- the routine
def pixnoise(luma, weight=None, ref=None, simd=None, return_intermediates=False):
    """FUN_00998d98(ctx, luma, weight, bright, noise, W, H, ref, pool) -> (noise, weight, bright).

    The return order is (noise, weight, bright), not the genuine's argument order (bright before
    noise).  luma: (H, W) float32 plane.  weight: the in/out plane (1.0 when None).  ref: (2, 16)
    normalisers (REF_NORM = the genuine reference model).  With return_intermediates a dict of the
    per-level planes follows as a fourth item."""
    simd = SIMD if simd is None else simd
    ref = REF_NORM if ref is None else np.asarray(ref, F32).reshape(2, 16)
    src = np.ascontiguousarray(luma, F32)
    if src.ndim != 2:
        raise ValueError("luma must be a 2-D plane")
    H, W = src.shape
    if W < 2 or H < 2:
        raise ValueError("the tile must be at least 2x2")
    sizes = level_sizes(W, H)
    weight = np.ones((H, W), F32) if weight is None else np.array(weight, F32, copy=True).reshape(H, W)
    bright = noise = None
    inter = {}
    for k in (0, 1):
        Wk, Hk, oddWk, oddHk = sizes[k]
        if Wk < 1 or Hk < 1:
            raise ValueError("the tile is too small for two levels")
        B = box2(src, 1, 0, simd)
        if k == 0:
            bright = B
        lo = down2(B, simd)
        band = band_from(lo, src, oddWk, oddHk, simd)
        kernels = build_kernels(Wk, Hk)
        E = np.empty((16, Hk, Wk), F32)
        Fm = np.empty((16, Hk, Wk), F32)
        for i in range(16):
            f = direction_filter(kernels, i, band)
            e = box2(f * f, 1, 1, simd)
            E[i] = np.sqrt(np.abs(e)) * F32(F32(1.0) / np.abs(ref[k, i]))
            g = direction_filter(kernels, i, band - f)
            Fm[i] = f + g
        idx, _ = argmax_maps(E.reshape(16, -1))
        S = Fm.reshape(16, -1)[idx, np.arange(Hk * Wk)].reshape(Hk, Wk)
        idx = idx.reshape(Hk, Wk)
        R = band - S
        if k == 0:
            noise = np.sqrt(np.abs(box2(R * R, 1, 1, simd)))
        ER = np.sqrt(np.abs(box2(R * R, 2, 1, simd)))
        ES = np.sqrt(np.abs(box2(S * S, 2, 1, simd)))
        w1 = ramp(0.8, 1.2, 1.0, 0.0, ratio_eps(ES, ER, 1e-4, simd), simd).reshape(Hk, Wk)
        E10 = np.sqrt(np.abs(box2(R * R, 10, 0, simd)))
        w2 = ramp(1.1, 1.5, 1.0, 0.0, ratio_eps(ER, E10, 1e-4, simd), simd).reshape(Hk, Wk)
        w1u, w2u = w1, w2
        for j in range(k, 0, -1):                 # FUN_00993df0 up to the tile size
            w1u = up2(w1u, sizes[j - 1][2], sizes[j - 1][3], simd)
        weight = weight * w1u
        for j in range(k, 0, -1):
            w2u = up2(w2u, sizes[j - 1][2], sizes[j - 1][3], simd)
        weight = weight * w2u
        if return_intermediates:
            inter[k] = dict(B=B, lo=lo, band=band, E=E, F=Fm, idx=idx, S=S, R=R, ER=ER, ES=ES,
                            E10=E10, w1=w1, w2=w2)
        src = lo
    if return_intermediates:
        return noise, weight, bright, inter
    return noise, weight, bright


# ============================================================================= batch fast path
# 2026-10-16.  pixnoise_batch runs FUN_00998d98 on B tiles of one size at once.  Everything above
# stays the reference (pixnoise(), the helpers, nv_lines.linefilt); the batch path reproduces the
# same float32 operations in the same order and is bit-identical per tile (verify_pixnoise.py
# compares it with `nvh pixnoise` on every tile and with pixnoise(); nv_curve's reference /
# batch comparison covers whole frames).  What differs is only the form:
#   * the tiles are stacked batch-innermost (H, W, B), so each step of a running sum is one numpy
#     call over all tiles (nv_lines.*_bm);
#   * the per-tile n & ~3 NEON / scalar splits (luma, ratio, ramp) are taken along the pixel axis of
#     the (H*W, B) view, i.e. per tile exactly as in the reference;
#   * the 16 orientations are reduced on the fly (running maximum with >=, the same updates as
#     argmax_maps) instead of storing 16 maps;
#   * fma32 / frecpe32 / frecps32 take a fast path where it is provably the same result and fall
#     back to the reference helpers element-wise otherwise (see _fma32_fast / frecpe32_fast).
_RECPE_TABLE = None


def frecpe32_fast(x):
    """frecpe32 by table: for a normal operand FPRecipEstimate depends only on the sign, the
    exponent and fraction<22:15> (the 8 bits it scales into the 9-bit table index), so the result
    of every normal x equals the reference result of (bits(x) >> 15) << 15.  The table holds
    frecpe32 of those 2^17 representatives; zero / subnormal (exponent 0) and inf / NaN
    (exponent 255) operands take the reference frecpe32."""
    global _RECPE_TABLE
    if _RECPE_TABLE is None:
        _RECPE_TABLE = frecpe32((np.arange(1 << 17, dtype=U32) << U32(15)).view(F32)).view(U32)
    x = np.ascontiguousarray(x, F32)
    u = x.view(U32)
    out = _RECPE_TABLE[u >> U32(15)]
    special = ((u & U32(0x7f800000)) - U32(0x00800000)) >= U32(0x7f000000)     # exponent 0 or 255
    if special.any():
        out[special] = frecpe32(x[special]).view(U32)
    return out.view(F32)


_U64 = np.uint64


def _round_or_flag(s):
    """float32(s) for the float64 s = a*b + c (product exact), and the elements where that single
    conversion may differ from one rounding of the exact a*b + c: s outside [2^-126, 2^127) (zero
    excepted: an exact zero has the IEEE sign in both), non-finite, or s exactly half-way between two
    float32 neighbours (bits 28..0 of the double's fraction = 1 followed by zeros) - only there can
    the double rounding differ.  Everywhere else float32(s) is the correctly rounded result."""
    r = s.astype(F32)
    u = s.view(_U64)
    e = (u >> _U64(52)) & _U64(0x7ff)
    flag = ((e - _U64(897)) >= _U64(253)) & ((u << _U64(1)) != _U64(0))
    flag |= (u & _U64(0x1fffffff)) == _U64(0x10000000)
    return r, flag


def _fma32_fast(a, b, c):
    """fma32 (FMLA / FPMulAdd) with the float64 path: s = a*b + c rounded once to float64 (the
    product of two float32 is exact), then to float32; flagged elements (_round_or_flag: possible
    double rounding, overflow, subnormal or special results) are recomputed with fma32."""
    a = np.asarray(a, F32)
    b = np.asarray(b, F32)
    c = np.asarray(c, F32)
    shape = np.broadcast_shapes(a.shape, b.shape, c.shape)
    if not shape:
        return fma32(a, b, c)
    with np.errstate(invalid="ignore", over="ignore"):
        s = np.multiply(a, b, dtype=F64)
        if s.shape == shape:
            np.add(s, c, out=s)
        else:
            s = np.add(s, c, dtype=F64)
    r, flag = _round_or_flag(s)
    if flag.any():
        r[flag] = fma32(np.broadcast_to(a, shape)[flag], np.broadcast_to(b, shape)[flag],
                        np.broadcast_to(c, shape)[flag])
    return r


def _frecps32_fast(a, b):
    """frecps32 (FRECPS = FPRecipStepFused 2 - a*b) with the float64 path; flagged elements
    (including every NaN / inf operand and inf * 0) are recomputed with frecps32."""
    a = np.asarray(a, F32)
    b = np.asarray(b, F32)
    shape = np.broadcast_shapes(a.shape, b.shape)
    if not shape:
        return frecps32(a, b)
    with np.errstate(invalid="ignore", over="ignore"):
        s = np.multiply(a, b, dtype=F64)
        np.subtract(F64(2.0), s, out=s)
    r, flag = _round_or_flag(s)
    if flag.any():
        r[flag] = frecps32(np.broadcast_to(a, shape)[flag], np.broadcast_to(b, shape)[flag])
    return r


def _fminmax_fast(x, c, is_max):
    """_fminmax (FMIN / FMAX with a scalar operand) with fewer passes: where() is FPMin / FPMax
    except for NaN x (propagated quieted, as FPProcessNaNs(x, c)) and for zeros when c is the zero of
    the losing sign (FPMax(+0, -0) = +0, FPMin(+0, -0) = -0); those constants, and a NaN c, use
    _fminmax."""
    c = F32(c)
    if np.isnan(c) or (c == 0 and bool(np.signbit(c)) == bool(is_max)):
        return _fminmax(x, c, is_max)
    x = np.asarray(x, F32)
    out = np.where(x > c, x, c) if is_max else np.where(x < c, x, c)
    nan = np.isnan(x)
    if nan.any():
        out[nan] = (x[nan].view(U32) | _QUIET).view(F32)
    return np.asarray(out, F32)


def _pixel_rows(x):
    """(H, W, ...) -> (H*W, ...) view (the per-tile flat pixel order of the reference)."""
    return x.reshape((x.shape[0] * x.shape[1],) + x.shape[2:])


def ratio_eps_bm(a, b, eps, simd=None):
    """ratio_eps on batch-minor planes (H, W, B): the NEON / scalar split at n & ~3 per tile."""
    simd = SIMD if simd is None else simd
    shape = a.shape
    a = _pixel_rows(np.ascontiguousarray(a, F32))
    b = _pixel_rows(np.ascontiguousarray(b, F32))
    eps = F32(eps)
    n = a.shape[0]
    m = (n & ~3) if simd else 0
    out = np.empty(a.shape, F32)
    if m:
        d = eps + b[:m]
        est = frecpe32_fast(d)
        step = _frecps32_fast(d, est)
        out[:m] = a[:m] * (est * step)
    if m < n:
        out[m:] = a[m:] / (b[m:] + eps)
    return out.reshape(shape)


def ramp_bm(a, b, va, vb, x, simd=None):
    """ramp on batch-minor planes (H, W, B): the NEON body / scalar tail split at n & ~3 per tile."""
    simd = SIMD if simd is None else simd
    a, b, va, vb = F32(a), F32(b), F32(va), F32(vb)
    if F32(b - a) <= F32(b * _C_1E5):
        a = F32(F32(a * _C_099999) + F32(-_C_1E5))
        b = F32(F32(b * _C_100001) + _C_1E5)
    slope = F32(F32(vb - va) / F32(b - a))
    icpt = F32(va - F32(a * slope))
    lo, hi = (vb, va) if not (vb > va) else (va, vb)
    shape = x.shape
    x = _pixel_rows(np.ascontiguousarray(x, F32))
    n = x.shape[0]
    m = (n & ~3) if simd else 0
    y = np.empty(x.shape, F32)
    if m:
        y[:m] = _fminmax_fast(_fminmax_fast(_fma32_fast(x[:m], slope, icpt), hi, False), lo, True)
    if m < n:
        t = icpt + slope * x[m:]
        t = np.where(t > hi, hi, t)
        y[m:] = np.where(t > lo, t, lo)
    return y.reshape(shape)


def weighted_sum3_batch(a, b, c, R, G, B, simd=None):
    """weighted_sum3 on planes (..., h, w): the NEON / scalar split at n & ~3 of every plane."""
    simd = SIMD if simd is None else simd
    shape = np.shape(R)
    n = shape[-1] * shape[-2]
    R = np.ascontiguousarray(R, F32).reshape(-1, n)
    G = np.ascontiguousarray(G, F32).reshape(-1, n)
    B = np.ascontiguousarray(B, F32).reshape(-1, n)
    a, b, c = F32(a), F32(b), F32(c)
    m = (n & ~3) if simd else 0
    out = np.empty(R.shape, F32)
    if m:
        out[:, :m] = _fma32_fast(B[:, :m], c, _fma32_fast(G[:, :m], b, a * R[:, :m]))
    if m < n:
        out[:, m:] = (R[:, m:] * a + G[:, m:] * b) + B[:, m:] * c
    return out.reshape(shape)


def luma_601_batch(R, G, B, simd=None):
    """luma_601 of every plane of (..., h, w) stacks (bit-identical to luma_601 per plane)."""
    return weighted_sum3_batch(*LUMA_601, R, G, B, simd)


def down2_bm(x, simd=None):
    """down2 on batch-minor planes (H, W, B)."""
    simd = SIMD if simd is None else simd
    x = np.ascontiguousarray(x, F32)
    H, W = x.shape[:2]
    h, w = H >> 1, W >> 1
    x = x[:2 * h, :2 * w]
    x0, x1 = x[0::2, 0::2], x[0::2, 1::2]
    y0, y1 = x[1::2, 0::2], x[1::2, 1::2]
    out = np.empty((h, w) + x.shape[2:], F32)
    c = ((W & ~7) >> 1) if simd else 0
    if c:
        out[:, :c] = ((x0[:, :c] + x1[:, :c]) + (y0[:, :c] + y1[:, :c])) * F32(0.25)
    if c < w:
        out[:, c:] = (((x0[:, c:] + x1[:, c:]) + y0[:, c:]) + y1[:, c:]) * F32(0.25)
    return out


def _up2_neon_bm(lo):
    """_up2_neon on batch-minor planes (Hc, Wc, B)."""
    Hc, Wc = lo.shape[:2]
    rest = lo.shape[2:]
    prev = np.concatenate([lo[:, :1], lo[:, :-1]], 1)
    nxt = np.concatenate([lo[:, 1:], lo[:, -1:]], 1)
    xs = lo * F32(0.75)
    L = xs + prev * F32(0.25)
    R = xs + nxt * F32(0.25)
    h = np.empty((Hc, 2 * Wc) + rest, F32)
    h[:, 0::2] = L
    h[:, 1::2] = R
    out = np.empty((2 * Hc, 2 * Wc) + rest, F32)
    out[0] = h[0]
    out[2 * Hc - 1] = h[Hc - 1]
    hp, hn = h[:-1], h[1:]
    out[1:2 * Hc - 2:2] = hp * F32(0.75) + hn * F32(0.25)
    out[2:2 * Hc - 1:2] = hp * F32(0.25) + hn * F32(0.75)
    return out


def _up2_scalar_bm(lo, oddW, oddH):
    """_up2_scalar on batch-minor planes (Hc, Wc, B)."""
    Hc, Wc = lo.shape[:2]
    rest = lo.shape[2:]
    if Wc < 2:
        raise NotImplementedError("the genuine scalar up-sampling reads past the row for Wc < 2")
    out = np.empty((2 * Hc + oddH, 2 * Wc + oddW) + rest, F32)
    x3 = lo * F32(3.0)
    up = x3 + np.concatenate([lo[:1], lo[:-1]], 0)
    dn = x3 + np.concatenate([lo[1:], lo[-1:]], 0)
    for src_rows, r0 in ((up, 0), (dn, 1)):
        a3 = src_rows * F32(3.0)
        row = np.empty((Hc, 2 * Wc) + rest, F32)
        row[:, 0] = (src_rows[:, 0] * F32(4.0)) * F32(0.0625)
        row[:, 2::2] = (a3[:, 1:] + src_rows[:, :-1]) * F32(0.0625)
        row[:, 1:-1:2] = (a3[:, :-1] + src_rows[:, 1:]) * F32(0.0625)
        row[:, 2 * Wc - 1] = (src_rows[:, Wc - 1] * F32(4.0)) * F32(0.0625)
        out[r0:2 * Hc:2, :2 * Wc] = row
    if oddW:
        out[:2 * Hc, 2 * Wc] = out[:2 * Hc, 2 * Wc - 1]
    if oddH:
        out[2 * Hc] = out[2 * Hc - 1]
    return out


def up2_bm(lo, oddW, oddH, simd=None):
    """up2 on batch-minor planes (Hc, Wc, B) -> (2Hc + oddH, 2Wc + oddW, B)."""
    simd = SIMD if simd is None else simd
    lo = np.ascontiguousarray(lo, F32)
    Hc, Wc = lo.shape[:2]
    oddW, oddH = int(oddW) & 1, int(oddH) & 1
    if Hc < 2 or Wc < 4 or not simd:
        return _up2_scalar_bm(lo, oddW, oddH)
    core = _up2_neon_bm(lo)
    if not (oddW or oddH):
        return core
    out = np.empty((2 * Hc + oddH, 2 * Wc + oddW) + lo.shape[2:], F32)
    out[:2 * Hc, :2 * Wc] = core
    if oddW:
        out[:2 * Hc, 2 * Wc] = core[:, 2 * Wc - 1]
    if oddH:
        out[2 * Hc] = out[2 * Hc - 1]
    return out


def band_from_bm(lo, src, oddW, oddH, simd=None):
    """band_from on batch-minor planes: src (2Hc + oddH, 2Wc + oddW, B) - up2(lo)."""
    simd = SIMD if simd is None else simd
    lo = np.ascontiguousarray(lo, F32)
    src = np.ascontiguousarray(src, F32)
    Hc, Wc = lo.shape[:2]
    oddW, oddH = int(oddW) & 1, int(oddH) & 1
    H2, W2 = 2 * Hc + oddH, 2 * Wc + oddW
    if src.shape[:2] != (H2, W2):
        raise ValueError(f"src {src.shape[:2]} does not match {H2}x{W2}")
    if Hc < 2 or Wc < 4 or not simd:
        return src - _up2_scalar_bm(lo, oddW, oddH)
    out = np.empty(src.shape, F32)
    out[:2 * Hc, :2 * Wc] = src[:2 * Hc, :2 * Wc] - _up2_neon_bm(lo)
    if oddW:
        c = 2 * Wc
        out[:2 * Hc, c] = (out[:2 * Hc, c - 1] - src[:2 * Hc, c - 1]) + src[:2 * Hc, c]
    if oddH:
        r = 2 * Hc
        out[r] = (out[r - 1] - src[r - 1]) + src[r]
    return out


def _box2_bm(x, r, mode, simd=None, out=None, mid=None):
    """box2 on batch-minor planes (rows into mid, then columns into out; both optional)."""
    return nl.box_cols_bm(nl.box_rows_bm(x, r, mode, simd, out=mid), r, mode, simd, out=out)


def _sqrt_abs(x):
    """np.sqrt(np.abs(x)) in place (x is a fresh temporary)."""
    np.abs(x, out=x)
    return np.sqrt(x, out=x)


def pixnoise_bm(src, weight=None, ref=None, simd=None):
    """FUN_00998d98 on batch-minor tiles: src (H, W, B) luma planes, weight (H, W, B) or None.
    Returns (noise, weight, bright), each (H, W, B); bit-identical to pixnoise() per tile."""
    simd = SIMD if simd is None else simd
    ref = REF_NORM if ref is None else np.asarray(ref, F32).reshape(2, 16)
    src = np.ascontiguousarray(src, F32)
    if src.ndim != 3:
        raise ValueError("src must be (H, W, B)")
    H, W, nb = src.shape
    if W < 2 or H < 2:
        raise ValueError("the tile must be at least 2x2")
    sizes = level_sizes(W, H)
    weight = np.ones((H, W, nb), F32) if weight is None else np.array(weight, F32, copy=True).reshape(H, W, nb)
    bright = noise = None
    for k in (0, 1):
        Wk, Hk, oddWk, oddHk = sizes[k]
        if Wk < 1 or Hk < 1:
            raise ValueError("the tile is too small for two levels")
        Bx = _box2_bm(src, 1, 0, simd)
        if k == 0:
            bright = Bx
        lo = down2_bm(Bx, simd)
        # planes reused by the 16 orientations (fresh multi-MB arrays would page-fault every time);
        # the padded ones are read by the oriented (general) passes without a copy
        shp = (Hk, Wk, nb)
        band, d, t1 = (nl.padded_empty(*shp)[1] for _ in range(3))
        band[...] = band_from_bm(lo, src, oddWk, oddHk, simd)
        f, g, ff, mid, e = (np.empty(shp, F32) for _ in range(5))
        plans = nl.line_plans(Wk, Hk)
        best = np.zeros(shp, F32)
        hit = np.empty(shp, bool)
        S = None
        for i in range(16):
            nl.direction_filter_bm(plans, i, band, out=f, tmp=t1)
            np.multiply(f, f, out=ff)
            _sqrt_abs(_box2_bm(ff, 1, 1, simd, out=e, mid=mid))
            np.multiply(e, F32(F32(1.0) / np.abs(ref[k, i])), out=e)
            np.subtract(band, f, out=d)
            nl.direction_filter_bm(plans, i, d, out=g, tmp=t1)
            np.add(f, g, out=f)                   # F_i = f + g
            # argmax_maps: hit = best <= E_i (>= lets the later index win), best = where(hit, E_i,
            # best); fmax gives the same best because E_i is never -0 (sqrt(|x|) * c, c > 0) and
            # best never NaN (a NaN E_i fails the compare and fmax returns best)
            np.less_equal(best, e, out=hit)
            np.fmax(best, e, out=best)
            S = f.copy() if S is None else np.where(hit, f, S)    # F_0 until a later map wins
        del d, t1, f, g, ff, mid, e, best, hit                    # lower the peak working set
        R = band - S
        RR = R * R
        if k == 0:
            noise = _sqrt_abs(_box2_bm(RR, 1, 1, simd))
        ER = _sqrt_abs(_box2_bm(RR, 2, 1, simd))
        ES = _sqrt_abs(_box2_bm(S * S, 2, 1, simd))
        w1 = ramp_bm(0.8, 1.2, 1.0, 0.0, ratio_eps_bm(ES, ER, 1e-4, simd), simd)
        E10 = _sqrt_abs(_box2_bm(RR, 10, 0, simd))
        w2 = ramp_bm(1.1, 1.5, 1.0, 0.0, ratio_eps_bm(ER, E10, 1e-4, simd), simd)
        w1u, w2u = w1, w2
        for j in range(k, 0, -1):
            w1u = up2_bm(w1u, sizes[j - 1][2], sizes[j - 1][3], simd)
        weight = weight * w1u
        for j in range(k, 0, -1):
            w2u = up2_bm(w2u, sizes[j - 1][2], sizes[j - 1][3], simd)
        weight = weight * w2u
        src = lo
    return noise, weight, bright


def pixnoise_batch(luma, weight=None, ref=None, simd=None):
    """pixnoise() of B tiles of one size at once -> (noise, weight, bright), each (B, H, W).

    The return order is (noise, weight, bright), as pixnoise().  luma (B, H, W) (or a list of
    (H, W) planes), weight the same shape or None.  Bit-identical to
    [pixnoise(luma[b], weight[b]) for b] (the reference)."""
    luma = np.asarray(luma, F32)
    if luma.ndim != 3:
        raise ValueError("luma must be (B, H, W)")
    wbm = None if weight is None else np.moveaxis(np.asarray(weight, F32).reshape(luma.shape), 0, -1)
    out = pixnoise_bm(np.moveaxis(luma, 0, -1), wbm, ref, simd)
    return tuple(np.ascontiguousarray(np.moveaxis(a, -1, 0)) for a in out)
