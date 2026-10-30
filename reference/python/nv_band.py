"""Neat Video 6 (arm64 slice) FUN_009980e8: the 5-level band noise measure of FindFlatArea, ported
bit-exactly to numpy, with the level part of FUN_00995250 (BuildNoiseModel per-plane model).

    band_levels(plane) -> float32[4096]   the 0x4000-byte struct as `nvh levels` dumps it
    model_levels(plane) -> float32[4096]  FUN_00995250 level part (fields 0..54 exact, see below)
    as_model(struct)   -> {"mean", "q99", "q50", "cv"}

Struct (float index): mean[0:5], q99[5:10], q50[10:15], cv[15:55] (5 levels x 8 octaves); the
remaining words of the 0x1f54-byte model hold the init pattern of FUN_009979fc + FUN_00aced94
(-1e6, -1.0, 0xffffffff runs, INIT_RUNS: decoded by replaying their store instructions); words
from 0x1f54 on are not written (zero in the harness, which callocs 0x4000 bytes).

Per level k (sizes W >> k, H >> k from FUN_0099c3a4), FUN_009980e8 (disassembly 0x9980e8):
    border bx = 2 if W_k > 4 else (W_k - 1) >> 1 (same for y); shrink both by one while the cropped
    map has < 32 pixels; a level is measured only if W_k, H_k >= 5 and nothing had to shrink
    (FUN_00995250: border 4, W_k, H_k >= 9).
    B    = box3 rows then columns, border replicated (FUN_009944c0 r=1 mode 0, nv_lines)
    lo   = 2x2 mean of B (FUN_009f15b4), band = src - up2(lo) (FUN_00990da8, nv_pixnoise)
    e    = sqrt(|box3_zero(band * band)|) (FUN_009ed2f0, FUN_009944c0 mode 1, FUN_009ed3e0)
    crop = e without the border, copied row by row (FUN_009f14e4)
    mean = fmaxnm(FUN_009ef3fc(crop), 1e-6)
    q99 / q50 = fmaxnm(FUN_00997bcc(q, crop, NULL, flag 1, div 4), 1e-6)
    cv   = FUN_00997f88(crop, w', h')
    src  = lo
  not measured (k > 0): mean/q99/q50 = -1e6 if the previous level's value is -1e6 else
    |prev| * -0.5 (FUN_00995250: f * -|prev| with f = clamp(|q99[k'] / q99[k'-1]|) of the last
    measured level k', 0.5 at first), cv = copy of the previous level's 8 words.

Float semantics (all float32 unless noted; the plugin runs with the default FPCR, see nv_pixnoise):
  * FUN_009ef3fc mean: blocks of 2048 values; inside a block two 4-lane accumulators (lanes 0-3,
    4-7) sum x[8i + j] sequentially from +0, reduced as ((l4+l0)+(l5+l1))+((l6+l2)+(l7+l3)), then the
    n & 7 tail added one by one; each block sum is added to a double; mean = (float)(sum / n).
  * FUN_009ef520 std: the same with d = x - m, lanes updated by FMLA (fused d*d + acc), the tail as
    separate fsub / fmul / fadd; std = (float)sqrt(sum / n) in double.
  * FUN_00997bcc quantile: threshold search from the mean in steps of +-std, halved and reversed up
    to 8 times (<= 100 counts of x <= t, NaN never counted), n <= 49: linear interpolation between
    the neighbouring values, then the normal-model bound mean + probit(q) * std * c[min(n/4, 11)]
    when n/4 < max((long)(1/(1-q)), 10) (double), probit FUN_009d3e40 on the 101-point table.
  * FUN_00997f88 CV: m = FUN_009ef3fc(map); per octave (< 8, while w*h >= 4 and m != 0):
    d = fma(x, 1, -m) (first n & ~3 values; FMLA) / x*1 + (-m), cv = fsqrt(mean(d*d)) / m, then the
    2x2 mean of FUN_009f15b4 (rows of the current width).
  * clamps are FMAXNM (a NaN becomes 1e-6).

Batch form: every routine takes B planes of one size stacked batch-innermost (H, W, B) (the layout of
the nv_lines / nv_pixnoise *_bm helpers); the per-plane results are bit-identical to B = 1.  Memory:
besides the input stack a level uses two scratch arrays of its size and a few of a quarter of it (a
Work, reusable across calls; the nv_lines column filter keeps one more array as per-thread scratch),
so the caller bounds the peak with the batch size.

Verification (2026-10-16, after the review fixes; analysis/noise/verify_levels.py reads the bit-pattern
column of `nvh levels|model` and `nvh quantile`): FUN_009980e8 161/161 planes identical in all 4096
words, the FUN_00995250 level part (words 0..54) 155/155, FUN_00997bcc 933/933 (q >= 1 = the
FUN_009efdd4 maximum, 0..9000 values, ties, constants, signed zeros, NaN payloads, signalling NaNs,
inf, q / flag / div variants) - planes 1x1 .. 1024x768 (odd, thin, non-square, level-0 crops of 2047 ..
4097 values), white / coloured noise, ramps, constant and near-constant planes, large DC offsets, tiny /
subnormal / huge values, integer and binary planes, NaN / +-inf pixels, an all-NaN plane, and 90 crops
of test_v02 n=312 / test.mp4 n=300 as RGB, YCrCb and mean / difference planes; the review's adversarial
and random planes agree as well.  Not ported: the words FUN_00995250 writes with its directional
filters (DAT_01243988 sets) and the FUN_00aced94 part, from word 55 (byte 0xdc) on; model_levels
leaves them at the init pattern (up to 702 words per plane differ, 512..702 on the real-frame crops;
the first differing word seen is 65, byte 0x104).  All 4096 words, those included: nv_model.model_full
(bit-exact port of 2026-10-16, which takes words 0..54 from model_levels unchanged).
Only numpy is used (plus nv_lines / nv_pixnoise of this directory, read only).
"""
import os
import sys

import numpy as np

_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)
import nv_lines as nl  # noqa: E402
import nv_pixnoise as npx  # noqa: E402

F32 = np.float32
F64 = np.float64
U32 = np.uint32
U64 = np.uint64

LEVELS = 5
STRUCT_WORDS = 0x4000 // 4          # what `nvh levels|model` dumps (calloc'ed 0x4000 bytes)
MODEL_WORDS = 0x1f54 // 4           # the part FUN_009979fc initialises
NEG_1E6 = np.array([0xc9742400], U32).view(F32)[0]          # -1e6, the "not measured" marker
EPS_1E6 = np.array([0x358637bd], U32).view(F32)[0]          # 1e-6 (fmaxnm clamp)
Q99 = np.array([0x3f7d70a4], U32).view(F32)[0]              # 0.99
Q50 = F32(0.5)
FLT_MAX = np.finfo(F32).max
BLOCK = 0x800                       # FUN_009ef3fc / FUN_009ef520 block length
# FUN_009979fc + FUN_00aced94 init image (word index, count, bit pattern); static replay of the
# store instructions of both functions (memset_pattern16 with DAT_012436e0 = -1e6 x 4)
INIT_RUNS = ((0x000, 1305, 0xc9742400), (0x519, 40, 0xbf800000), (0x541, 640, 0xffffffff),
             (0x7c1, 20, 0xbf800000))
# FUN_00997bcc tables: small-sample factors UNK_01244678 (12 floats) and the inverse normal CDF
# DAT_01248e48 at q = 0, 0.01, .., 1 (101 floats, clamped to +-3), read from the binary.  The probit
# reads one word past the table at q = 1 (DAT_01248fdc, 3458402877440.0, weighted by 0).
GQ_SMALL = np.array([0x0, 0x0, 0x3fd872b0, 0x3f9ef9db, 0x3f90e560, 0x3f8a7efa, 0x3f86e979,
                     0x3f849ba6, 0x3f83126f, 0x3f81eb85, 0x3f810625, 0x3f800000], U32).view(F32)
_PROBIT = [
    -3.0, -2.326, -2.054, -1.881, -1.751, -1.645, -1.555, -1.476, -1.405, -1.341,
    -1.282, -1.227, -1.175, -1.126, -1.08, -1.036, -0.994, -0.954, -0.915, -0.878,
    -0.842, -0.806, -0.772, -0.739, -0.706, -0.674, -0.643, -0.613, -0.583, -0.553,
    -0.524, -0.496, -0.468, -0.44, -0.412, -0.385, -0.358, -0.332, -0.305, -0.279,
    -0.253, -0.228, -0.202, -0.176, -0.151, -0.126, -0.1, -0.075, -0.05, -0.025,
    0.0, 0.025, 0.05, 0.075, 0.1, 0.126, 0.151, 0.176, 0.202, 0.228,
    0.253, 0.279, 0.305, 0.332, 0.358, 0.385, 0.412, 0.44, 0.468, 0.496,
    0.524, 0.553, 0.583, 0.613, 0.643, 0.674, 0.706, 0.739, 0.772, 0.806,
    0.842, 0.878, 0.915, 0.954, 0.994, 1.036, 1.08, 1.126, 1.175, 1.227,
    1.282, 1.341, 1.405, 1.476, 1.555, 1.645, 1.751, 1.881, 2.054, 2.326,
    3.0, 3458402877440.0]
PROBIT = np.array(_PROBIT, F32)     # float32 of the decimal literals = the stored words (checked)


def init_struct(n_words=STRUCT_WORDS):
    """The struct after FUN_009979fc (uint32 words; zero beyond MODEL_WORDS)."""
    w = np.zeros(n_words, U32)
    for i, n, v in INIT_RUNS:
        w[i:i + n] = v
    return w


# ------------------------------------------------------------------------------- float helpers
def fmaxnm(x, c):
    """AArch64 FMAXNM of float32 x (array or scalar) and the constant c > 0: a quiet NaN gives c."""
    x = np.asarray(x, F32)
    return np.where(np.isnan(x) | (x < c), F32(c), x).astype(F32)


def _round_or_flag(s):
    """float32(s) of the float64 s = a*b + c (product exact) and the elements where that single
    conversion can differ from one rounding of the exact sum (a copy of nv_pixnoise._round_or_flag;
    verify_levels.py checks that both agree): s outside the normal float32 range (zero excepted),
    non-finite, or exactly half-way between two floats."""
    r = s.astype(F32)
    u = s.view(U64)
    e = (u >> U64(52)) & U64(0x7ff)
    flag = ((e - U64(897)) >= U64(253)) & ((u << U64(1)) != U64(0))
    flag |= (u & U64(0x1fffffff)) == U64(0x10000000)
    return r, flag


def _fmla_sq(acc, d):
    """FMLA acc += d * d (FPMulAdd(addend = acc, d, d), one rounding) element-wise; flagged elements
    (double rounding possible, overflow, subnormal or special values) use nv_pixnoise.fma32."""
    with np.errstate(invalid="ignore", over="ignore"):
        dd = d.astype(F64)
        s = dd * dd
        s += acc
    r, flag = _round_or_flag(s)
    if flag.any():
        r[flag] = npx.fma32(d[flag], d[flag], acc[flag])
    return r


CHAIN_STEPS = 16                    # FMLA steps per chunk of _fmla_chain


def _range_flag(s):
    """float64 s that is non-zero and outside the normal float32 range [2^-126, 2^127), or not finite."""
    u = s.view(U64)
    e = (u >> U64(52)) & U64(0x7ff)
    return ((e - U64(897)) >= U64(253)) & ((u << U64(1)) != U64(0))


def _fmla_chain(X, m):
    """The FMLA accumulation of FUN_009ef520 on X (nb, steps, 8, B): per block and lane,
    acc = fma(d_i, d_i, acc) over the steps i with d_i = x_i - m (float32), from +0; returns acc
    (nb, 8, B).  Same result as repeating _fmla_sq, with fewer operations per step: a chunk of steps runs
    with the float64 sum d*d + acc rounded once to float32 (exact unless the float64 sum was rounded
    onto a float32 midpoint); the chunk's sums are then checked together with _round_or_flag, and the
    lanes with a flagged sum that is not an exact (TwoSum error 0), in-range midpoint are redone step
    by step with _fmla_sq from the chunk start."""
    nb_, steps = X.shape[:2]
    lane = (nb_,) + X.shape[2:]
    acc32 = np.zeros(lane, F32)
    acc64 = np.zeros(lane, F64)
    with np.errstate(invalid="ignore", over="ignore"):
        for c0 in range(0, steps, CHAIN_STEPS):
            k = min(CHAIN_STEPS, steps - c0)
            D = X[:, c0:c0 + k] - m                         # fsub (float32)
            D2 = D.astype(F64)
            np.multiply(D2, D2, out=D2)                     # exact squares
            S = np.empty(D2.shape, F64)
            start = acc32.copy()
            for i in range(k):
                np.add(D2[:, i], acc64, out=S[:, i])
                np.copyto(acc32, S[:, i], casting="unsafe")
                np.copyto(acc64, acc32)
            flag = _round_or_flag(S)[1]
            if not flag.any():
                continue
            ib, ii, il, ic = np.nonzero(flag)
            a = D2[ib, ii, il, ic]
            prev = np.where(ii == 0, start[ib, il, ic],
                            S[ib, np.maximum(ii - 1, 0), il, ic].astype(F32)).astype(F64)
            sf = S[ib, ii, il, ic]
            t = sf - a                                      # TwoSum error of a + prev
            err = (a - (sf - t)) + (prev - t)
            bad = (err != 0) | _range_flag(sf) | np.isnan(err)
            if not bad.any():
                continue                                    # exact midpoints: float32(s) is the FMLA result
            key = np.unique(np.ravel_multi_index((ib[bad], il[bad], ic[bad]), lane))
            jb, jl, jc = np.unravel_index(key, lane)
            acc = start[jb, jl, jc]
            Dl = D[jb, :, jl, jc]                           # (lanes, k)
            for i in range(k):
                acc = _fmla_sq(acc, Dl[:, i])
            acc32[jb, jl, jc] = acc
            acc64[jb, jl, jc] = acc
    return acc32


def _reduce8(acc):
    """fadd.4s v, v_hi, v_lo; faddp.4s; faddp.2s: ((l4+l0)+(l5+l1)) + ((l6+l2)+(l7+l3)) over axis -2
    of acc (..., 8, B)."""
    a = acc[..., 4:8, :] + acc[..., 0:4, :]
    return (a[..., 0, :] + a[..., 1, :]) + (a[..., 2, :] + a[..., 3, :])


def _block_sums(V, m=None):
    """Block sums of FUN_009ef3fc (m is None) or FUN_009ef520 (deviations from m (B,)) of V (n, B):
    returns the float32 sums of the consecutive 2048-value blocks, (nblocks, B), in order."""
    n, B = V.shape
    nfull = n // BLOCK
    out = []
    with np.errstate(invalid="ignore", over="ignore"):
        if nfull:
            X = V[:nfull * BLOCK].reshape(nfull, BLOCK // 8, 8, B)
            if m is None:
                acc = np.zeros((nfull, 8, B), F32)
                for i in range(BLOCK // 8):
                    np.add(acc, X[:, i], out=acc)
            else:
                acc = _fmla_chain(X, m)
            out.append(_reduce8(acc))
        rem = n - nfull * BLOCK
        if rem:
            T = V[nfull * BLOCK:]
            m8 = rem & ~7
            if m is None or not m8:
                acc = np.zeros((8, B), F32)
                for i in range(m8 // 8):
                    np.add(acc, T[8 * i:8 * i + 8], out=acc)
            else:
                acc = _fmla_chain(T[:m8].reshape(1, m8 // 8, 8, B), m)[0]
            s = _reduce8(acc)
            for j in range(m8, rem):
                if m is None:
                    s = s + T[j]
                else:
                    t = T[j] - m
                    s = s + t * t
            out.append(s[None])
    return np.concatenate(out, 0) if out else np.zeros((0, B), F32)


def mean_bm(V):
    """FUN_009ef3fc of every column of V (n, B) (column = one plane in its flat pixel order)."""
    V = np.ascontiguousarray(V, F32)
    n, B = V.shape
    d = np.zeros(B, F64)
    for s in _block_sums(V):
        d = d + s.astype(F64)
    with np.errstate(invalid="ignore", divide="ignore"):
        return (d / F64(n)).astype(F32)


def std_bm(V, m):
    """FUN_009ef520(m, V, n) of every column of V (n, B) with its mean m (B,)."""
    V = np.ascontiguousarray(V, F32)
    n, B = V.shape
    m = np.asarray(m, F32).reshape(B)
    d = np.zeros(B, F64)
    for s in _block_sums(V, m):
        d = d + s.astype(F64)
    with np.errstate(invalid="ignore", divide="ignore"):
        return np.sqrt(d / F64(n)).astype(F32)


def mean_exact(v):
    return mean_bm(np.asarray(v, F32).reshape(-1, 1))[0]


def std_exact(v, m):
    return std_bm(np.asarray(v, F32).reshape(-1, 1), np.array([m], F32))[0]


_PROBIT_CACHE = {}


def probit(q):
    """FUN_009d3e40: x = min(max(q, 0), 1) * 100, i = (long)x, f = x - i,
    (1 - f) * tab[i] + f * tab[i + 1] (fmax / fmin: a NaN q propagates and reads tab[0])."""
    q = F32(q)
    key = int(np.asarray(q).view(U32))
    if key in _PROBIT_CACHE:
        return _PROBIT_CACHE[key]
    _PROBIT_CACHE[key] = r = _probit(q)
    return r


def _probit(q):
    with np.errstate(invalid="ignore"):
        x = F32(min(max(float(q), 0.0), 1.0)) if not np.isnan(q) else q
        x = F32(x * F32(100.0))
    i = 0 if np.isnan(x) else int(x)
    f = F32(x - F32(i))
    return F32(F32((F32(1.0) - f) * PROBIT[i]) + F32(f * PROBIT[i + 1]))


_QUIET = U32(0x00400000)


def _quiet(x):
    return np.array([np.asarray(x, F32).view(U32) | _QUIET], U32).view(F32)[0]


def _fmax(a, b):
    """FPMax(a, b) of two float32 scalars as FMAX Vn = a, Vm = b (default FPCR): a NaN operand gives
    the quieted NaN of FPProcessNaNs (signalling a, signalling b, quiet a, quiet b); +0 > -0."""
    a, b = F32(a), F32(b)
    if np.isnan(a) or np.isnan(b):
        ua, ub = int(np.asarray(a).view(U32)), int(np.asarray(b).view(U32))
        if np.isnan(a) and not ua & 0x400000:
            return _quiet(a)
        if np.isnan(b) and not ub & 0x400000:
            return _quiet(b)
        return a if np.isnan(a) else b
    if a == b == F32(0.0):
        return F32(-0.0) if (np.signbit(a) and np.signbit(b)) else F32(0.0)
    return a if a > b else b


def _vmax(v):
    """The maximum of FUN_009efdd4(v, &min, &max, n) (SIMD path, DAT_014a67a8 = 1): eight lanes start
    at -FLT_MAX and take FMAX(x, lane) over the first n & ~7 values (a NaN makes the lane the quieted
    last NaN of that lane; +0 > -0), lanes 0-3 are combined with 4-7 (FMAX), FMAXV reduces
    FMAX(FMAX(l0, l1), FMAX(l2, l3)); the remaining values update it scalar-wise with fcmp / csel gt
    (a NaN value is passed over, a NaN maximum stays).  Only FUN_00997bcc with q >= 1 needs it."""
    v = np.ascontiguousarray(v, F32).ravel()
    n = v.size
    m = (n & ~7) if nl.SIMD else 0
    mx = F32(-FLT_MAX)
    if m:
        L = v[:m].reshape(-1, 8)
        lanes = []
        for j in range(8):
            col = L[:, j]
            nan = np.isnan(col)
            if nan.any():
                lanes.append(_quiet(col[np.nonzero(nan)[0][-1]]))
                continue
            top = col.max()
            if top < F32(-FLT_MAX):
                top = F32(-FLT_MAX)                        # the lane's start value wins over -inf
            if top == F32(0.0):
                top = F32(0.0) if np.any((col == F32(0.0)) & ~np.signbit(col)) else F32(-0.0)
            lanes.append(F32(top))
        c = [_fmax(lanes[j], lanes[j + 4]) for j in range(4)]
        mx = _fmax(_fmax(c[0], c[1]), _fmax(c[2], c[3]))
    for x in v[m:]:
        if x > mx:
            mx = x
    return F32(mx)


def _fcvtzs_d(x):
    """fcvtzs x, d: double -> int64 toward zero, saturating, NaN -> 0."""
    if x != x:
        return 0
    if x >= 2.0 ** 63:
        return 2 ** 63 - 1
    if x < -(2.0 ** 63):
        return -(2 ** 63)
    return int(x)


def quantile(v, q, flag=1, div=4, m=None, sd=None, le=None):
    """FUN_00997bcc(q, v, out, flag, div, n) -> float32 (the normal-model value it also stores
    through out is not returned).  q >= 1: the maximum of FUN_009efdd4 (_vmax).  Only bit 0 of
    flag counts (and w8, w8, w19); div >= 0 (sdiv gives 0 for div 0; a negative quotient would index
    the factor table before its start, so it is rejected here).
    m / sd: FUN_009ef3fc / FUN_009ef520 of v when already known; le: a bool scratch array of v's
    size for the counts (FUN_009ef8dc)."""
    v = np.ascontiguousarray(v, F32).ravel()
    n = v.size
    q = F32(q)
    if q >= F32(1.0):
        return _vmax(v)
    div = int(div)
    if div < 0:
        raise ValueError("FUN_00997bcc reads UNK_01244678[n / div]: div must be >= 0")
    if m is None:
        m = mean_exact(v)
    if sd is None:
        sd = std_exact(v, m)
    m, sd = F32(m), F32(sd)
    if sd == F32(0.0):
        return F32(0.0)
    if n <= 4096:
        vs = np.sort(v)

        def count(t):                       # searchsorted == less_equal counts: NaN never counted
            return int(np.searchsorted(vs, t, "right"))
    else:
        if le is None or le.size != n:
            le = np.empty(n, np.bool_)
        count = lambda t: int(np.count_nonzero(np.less_equal(v, t, out=le)))   # FUN_009ef8dc: NaN never counted
    with np.errstate(invalid="ignore", over="ignore"):
        nf = F32(n)
        target = F32(nf * q)
        # 0x997c64..0x997d00: up to 8 phases; a phase moves t by `step` until the count crosses the
        # target, then the step is multiplied by -0.5; more than 100 counts end the search
        t, step, evals, done = m, sd, 0, False
        for _ in range(8):
            if step > F32(0.0):
                while True:
                    if evals > 99:
                        done = True
                        break
                    evals += 1
                    t = F32(step + t)
                    if target <= F32(count(t)):              # b.ls
                        break
            elif step < F32(0.0):
                while True:
                    if evals > 99:
                        done = True
                        break
                    evals += 1
                    t = F32(step + t)
                    c = F32(count(t))
                    if not (target < c or np.isnan(target)):  # b.lt loops (also when unordered)
                        break
            else:                       # zero or NaN step (b.pl): t += step repeatedly up to 99 counts
                if evals <= 99:
                    t = F32(step + t)
                done = True
            if done:
                break
            step = F32(step * F32(-0.5))
        if n <= 49:
            if n < 1:
                t = F32(-FLT_MAX)
            else:
                upper, lower = F32(FLT_MAX), F32(-FLT_MAX)
                for x in v:
                    if x >= t and upper > x:
                        upper = x
                    if x < t and lower < x:
                        lower = x
                if upper == F32(FLT_MAX):
                    t = lower
                elif lower == F32(-FLT_MAX):
                    t = upper
                else:
                    c_up = F32(count(upper))
                    k = F32(int(np.count_nonzero((v > lower) & (v <= upper))))
                    slope = F32(F32(upper - lower) / k)
                    t = F32(F32(F32(slope * nf) * q) + F32(upper - F32(slope * c_up)))
        d = n // div if div else 0                             # sdiv (n >= 0, div >= 0)
        lim = _fcvtzs_d(1.0 / (1.0 - float(q)))               # fcvt, fsub, fdiv, fcvtzs (double)
        lim = lim if lim > 10 else 10
        g = F32(F32(F32(probit(q) * sd) * GQ_SMALL[d if d < 11 else 11]) + m)
        res = g if (d < lim and (int(flag) & 1) and g > t) else t
    return F32(res)


def _sub_m(X, m, out=None):
    """FUN_009ee3fc(1.0, -m, X, dst, n) on flat planes X (n, B): fma(x, 1, -m) for the first n & ~3
    values (FMLA), x*1 + (-m) for the rest; identical to x - m unless a NaN is involved."""
    nm = (-np.asarray(m, F32)).astype(F32)
    n = X.shape[0]
    out = np.empty(X.shape, F32) if out is None else out
    with np.errstate(invalid="ignore", over="ignore"):
        if np.isfinite(nm).all() and np.isfinite(X).all():
            return np.add(X, nm, out=out)
        k = n & ~3
        if k:
            out[:k] = npx.fma32(X[:k], F32(1.0), np.broadcast_to(nm, X[:k].shape))
        if k < n:
            out[k:] = (X[k:] * F32(1.0)) + nm
    return out


class Work:
    """Scratch memory of the level routines: named flat buffers handed out as views of the needed
    shape.  One Work shared by the batches of a caller (e.g. one FindFlatArea call) makes every batch,
    level and plane size reuse the same memory instead of allocating fresh arrays (which the
    allocator keeps resident); it is released with the Work object.  One name = one live array.
    reserve: {name: minimum size} for buffers whose largest use is known in advance (allocated once)."""

    def __init__(self, reserve=None):
        self._bufs = {}
        self._reserve = dict(reserve or {})

    def get(self, name, shape, dtype=F32):
        n = 1
        for v in shape:
            n *= int(v)
        a = self._bufs.get(name)
        if a is None or a.size < n or a.dtype != dtype:
            a = self._bufs[name] = np.empty(max(n, self._reserve.get(name, 0), 1), dtype)
        return a[:n].reshape(shape)


def _down2_into(x, out, tmp):
    """nv_pixnoise.down2_bm (FUN_009f15b4 on every row pair) of x (H, W, B) into out (H/2, W/2, B), with
    the same operations: ((x0 + x1) + (y0 + y1)) * 0.25 on the first (W & ~7) / 2 output columns (NEON),
    (((x0 + x1) + y0) + y1) * 0.25 on the rest; tmp is scratch of out's shape."""
    H, W = x.shape[:2]
    h, w = H >> 1, W >> 1
    x = x[:2 * h, :2 * w]
    x0, x1, y0, y1 = x[0::2, 0::2], x[0::2, 1::2], x[1::2, 0::2], x[1::2, 1::2]
    c = ((W & ~7) >> 1) if npx.SIMD else 0
    with np.errstate(invalid="ignore", over="ignore"):
        if c:
            o, t = out[:, :c], tmp[:, :c]
            np.add(x0[:, :c], x1[:, :c], out=o)
            np.add(y0[:, :c], y1[:, :c], out=t)
            np.add(o, t, out=o)
            np.multiply(o, F32(0.25), out=o)
        if c < w:
            o = out[:, c:]
            np.add(x0[:, c:], x1[:, c:], out=o)
            np.add(o, y0[:, c:], out=o)
            np.add(o, y1[:, c:], out=o)
            np.multiply(o, F32(0.25), out=o)
    return out


def cv_bm(E, m, work=None, scratch="cv_d"):
    """FUN_00997f88 on B maps E (h, w, B) with their FUN_009ef3fc means m (B,): (8, B) values, -1e6
    where the genuine writes nothing (octaves with w*h < 4, or m == 0).  work / scratch: the Work and
    the name of a free buffer of E's size for the deviations (the octave maps use "cv_o0" / "cv_o1")."""
    out = np.full((8, E.shape[2]), NEG_1E6, F32)
    m = np.asarray(m, F32)
    live = ~(m == F32(0.0))                                    # fcmp m, #0; b.eq: a NaN m goes on
    if not live.any():
        return out
    work = Work() if work is None else work
    X = np.ascontiguousarray(E, F32)
    B = X.shape[2]
    with np.errstate(invalid="ignore", over="ignore", divide="ignore"):
        for j in range(8):
            h, w = X.shape[:2]
            if h * w < 4:
                break
            d = _sub_m(X.reshape(h * w, B), m, work.get(scratch, (h * w, B)))
            np.multiply(d, d, out=d)
            out[j] = np.where(live, np.sqrt(mean_bm(d)) / m, out[j])
            if j < 7:                                          # the next octave's 2x2 mean
                nxt = work.get(f"cv_o{j & 1}", (h >> 1, w >> 1, B))
                X = _down2_into(X, nxt, work.get(scratch, nxt.shape))      # (d is dead)
    return out


# ------------------------------------------------------------------------------- the routines
def _borders(Wk, Hk, b):
    """Border (bx, by), cropped size and the `small` flag of FUN_009980e8 (b = 2) / FUN_00995250
    (b = 4): b if the side is > 2b, else (side - 1) >> 1; shrink while the crop has < 32 pixels."""
    bx = b if Wk > 2 * b else (((Wk - 1) if Wk - 1 >= 0 else Wk) >> 1)
    by = b if Hk > 2 * b else (((Hk - 1) if Hk - 1 >= 0 else Hk) >> 1)
    small = Wk < 2 * b + 1 or Hk < 2 * b + 1
    while not ((Wk - 2 * bx) * (Hk - 2 * by) > 31 or (bx == 0 and by == 0)):
        if bx > 0 or by > 0:
            small = True
        bx, by = bx - (bx > 0), by - (by > 0)
    return bx, by, small


def _band_into(lo, X, out, work):
    """out = X - up2(lo) for B planes, bit-identical to nv_pixnoise.band_from_bm (FUN_00990da8 with
    FUN_009f1860 / FUN_009f173c and the odd column / row of FUN_009f1860): the up-sampled rows are
    written straight into out and subtracted in place; the scratch lives in work's "b" buffer
    (free at this point of a level)."""
    Hc, Wc, B = lo.shape
    H2, W2 = X.shape[:2]
    oddW, oddH = W2 - 2 * Wc, H2 - 2 * Hc
    if Hc < 2 or Wc < 4 or not npx.SIMD:
        np.copyto(out, npx.band_from_bm(lo, X, oddW, oddH))   # small levels: the scalar up-sampling
        return out
    nh = Hc * 2 * Wc * B
    nl_ = Hc * Wc * B
    scratch = work.get("b", (2 * nh,))
    h = scratch[:nh].reshape(Hc, 2 * Wc, B)
    with np.errstate(invalid="ignore", over="ignore"):
        # columns: h[:, 2i] = lo*0.75 + prev*0.25, h[:, 2i+1] = lo*0.75 + next*0.25 (edges replicated)
        xs = scratch[nh:nh + nl_].reshape(lo.shape)
        q = scratch[nh + nl_:nh + 2 * nl_].reshape(lo.shape)
        np.multiply(lo, F32(0.75), out=xs)
        np.multiply(lo[:, :1], F32(0.25), out=q[:, :1])
        np.multiply(lo[:, :-1], F32(0.25), out=q[:, 1:])
        np.add(xs, q, out=h[:, 0::2])
        np.multiply(lo[:, 1:], F32(0.25), out=q[:, :-1])
        np.multiply(lo[:, -1:], F32(0.25), out=q[:, -1:])
        np.add(xs, q, out=h[:, 1::2])
        # rows: first / last copied, odd rows hp*0.75 + hn*0.25, even rows hp*0.25 + hn*0.75
        U = out[:2 * Hc, :2 * Wc]
        U[0] = h[0]
        U[2 * Hc - 1] = h[Hc - 1]
        hp, hn = h[:-1], h[1:]
        t = scratch[nh:nh + hn.size].reshape(hn.shape)      # xs and q are dead now
        np.multiply(hn, F32(0.25), out=t)
        odd = U[1:2 * Hc - 2:2]
        np.multiply(hp, F32(0.75), out=odd)
        np.add(odd, t, out=odd)
        np.multiply(hn, F32(0.75), out=t)
        even = U[2:2 * Hc - 1:2]
        np.multiply(hp, F32(0.25), out=even)
        np.add(even, t, out=even)
        np.subtract(X[:2 * Hc, :2 * Wc], U, out=U)
        if oddW:
            c = 2 * Wc
            out[:2 * Hc, c] = (out[:2 * Hc, c - 1] - X[:2 * Hc, c - 1]) + X[:2 * Hc, c]
        if oddH:
            r = 2 * Hc
            out[r] = (out[r - 1] - X[r - 1]) + X[r]
    return out


def _local_rms_crop(src, k, bx, by, work):
    """One measured level on B planes src (H, W, B): returns (lo, crop) with lo = down2(box3(src))
    (the next level's input, work buffer "lo<k>") and crop = the local RMS map
    sqrt(|box3_zero(band^2)|) of band = src - up2(lo) without the (bx, by) border (work buffer "b").
    Besides src and the small lo the level uses the work buffers "a" and "b" of src's size (and the
    nv_lines column filter its own per-thread scratch)."""
    Hk, Wk, B = src.shape
    a = work.get("a", src.shape)
    b = work.get("b", src.shape)
    with np.errstate(invalid="ignore", over="ignore"):
        nl.box_rows_bm(src, 1, 0, out=a)
        nl.box_cols_bm(a, 1, 0, out=b)                        # box3 in b
        lo = work.get(f"lo{k}", (Hk >> 1, Wk >> 1, B))
        _down2_into(b, lo, work.get("a", lo.shape))
        _band_into(lo, src, a, work)                          # band in a
        np.multiply(a, a, out=a)
        nl.box_rows_bm(a, 1, 1, out=b)
        nl.box_cols_bm(b, 1, 1, out=a)                        # local mean square in a
        np.abs(a, out=a)
        np.sqrt(a, out=a)
        crop = work.get("b", (Hk - 2 * by, Wk - 2 * bx, B))
        np.copyto(crop, a[by:Hk - by, bx:Wk - bx])
    return lo, crop


def _measure_bm(crop, out, k, work):
    """Statistics of a measured level k into out (B, words) for the cropped maps crop (h, w, B)."""
    h, w, B = crop.shape
    n = h * w
    V = crop.reshape(n, B)
    m = mean_bm(V)
    outf = out.view(F32)
    outf[:, k] = fmaxnm(m, EPS_1E6)
    sd = std_bm(V, m)
    q99 = np.empty(B, F32)
    q50 = np.empty(B, F32)
    v = work.get("plane", (n,))
    le = work.get("le", (n,), np.bool_)
    for b in range(B):
        np.copyto(v, V[:, b])                                 # one plane at a time
        q99[b] = quantile(v, Q99, 1, 4, m[b], sd[b], le)
        q50[b] = quantile(v, Q50, 1, 4, m[b], sd[b], le)
    outf[:, 5 + k] = fmaxnm(q99, EPS_1E6)
    outf[:, 10 + k] = fmaxnm(q50, EPS_1E6)
    outf[:, 15 + 8 * k:23 + 8 * k] = cv_bm(crop, m, work, "a").T     # "a" is free once crop is cut


def _extrapolate(out, k, fac=None):
    """Fields of a level that is too small: -1e6 stays, else |prev| * -0.5 (FUN_009980e8, fac None)
    or fac * -|prev| (FUN_00995250); the CV words are copied from level k - 1."""
    outf = out.view(F32)
    with np.errstate(invalid="ignore", over="ignore"):
        for base in (0, 5, 10):
            prev = outf[:, base + k - 1]
            if fac is None:
                val = np.abs(prev) * F32(-0.5)
            else:
                val = fac * (-np.abs(prev))
            outf[:, base + k] = np.where(prev == NEG_1E6, NEG_1E6, val)
    out[:, 15 + 8 * k:23 + 8 * k] = out[:, 15 + 8 * (k - 1):23 + 8 * k - 8]


def band_levels_bm(X, levels=LEVELS, work=None):
    """FUN_009980e8 on B planes X (H, W, B) of one size: (B, 4096) uint32 struct words.
    work: a Work to reuse across calls (default: a fresh one)."""
    if not 1 <= levels <= LEVELS:
        raise ValueError(f"FUN_009980e8 measures {LEVELS} levels (got {levels})")
    X = np.ascontiguousarray(X, F32)
    H, W, B = X.shape
    work = Work() if work is None else work
    out = np.tile(init_struct(), (B, 1))
    src = X
    for k in range(levels):
        Wk, Hk = W >> k, H >> k
        bx, by, small = _borders(Wk, Hk, 2)
        if Hk < 3 or Wk < 3 or small:
            if k:
                _extrapolate(out, k)
            continue
        lo, crop = _local_rms_crop(src, k, bx, by, work)
        _measure_bm(crop, out, k, work)
        src = lo
    return out


def model_levels_bm(X, levels=LEVELS, work=None):
    """Level part of FUN_00995250 (BuildNoiseModel per-plane model, `nvh model`) on B planes X
    (H, W, B): fields 0..54 as FUN_00995250 writes them (border 4, measured if W_k, H_k >= 9; the
    extrapolation factor of a too-small level is clamp(|q99[k'] / q99[k'-1]|) of the last measured
    level: 0.5 below 0.5, fmin with 1.0 above (a NaN ratio propagates), 0.5 before any ratio).
    The words its directional filters write (DAT_01243988 sets; all at word 55 = byte 0xdc or later,
    including the FUN_00aced94 part) are not computed: they keep the init pattern here
    (nv_model.model_full computes them)."""
    if not 1 <= levels <= LEVELS:
        raise ValueError(f"the model struct holds {LEVELS} levels (got {levels})")
    X = np.ascontiguousarray(X, F32)
    H, W, B = X.shape
    work = Work() if work is None else work
    out = np.tile(init_struct(), (B, 1))
    fac = np.full(B, F32(0.5), F32)
    src = X
    for k in range(levels):
        Wk, Hk = W >> k, H >> k
        bx, by, small = _borders(Wk, Hk, 4)
        if small:
            if k:
                _extrapolate(out, k, fac)
            continue
        lo, crop = _local_rms_crop(src, k, bx, by, work)
        _measure_bm(crop, out, k, work)
        if k:                      # 0x995db0: fdiv, fabs, fmin with 1.0, fcmp with 0.5 / fcsel mi
            outf = out.view(F32)
            with np.errstate(invalid="ignore", divide="ignore"):
                r = np.abs(outf[:, 5 + k] / outf[:, 4 + k])
                fac = np.where(r < F32(0.5), F32(0.5), np.minimum(r, F32(1.0))).astype(F32)
        src = lo
    return out


def _single(fn, plane, **kw):
    p = np.asarray(plane, F32)
    if p.ndim != 2:
        raise ValueError("plane must be 2-D")
    return fn(p[:, :, None], **kw)[0].view(F32)


def band_levels(plane, levels=LEVELS):
    """FUN_009980e8 on one plane (H, W): the 4096-word struct as float32 (bit patterns exact)."""
    return _single(band_levels_bm, plane, levels=levels)


def model_levels(plane, levels=LEVELS):
    """Level part of FUN_00995250 on one plane (see model_levels_bm)."""
    return _single(model_levels_bm, plane, levels=levels)


def as_model(struct):
    """Struct words -> {"mean": (5,), "q99": (5,), "q50": (5,), "cv": (5, 8)} float32 views."""
    s = np.asarray(struct).view(F32) if np.asarray(struct).dtype == U32 else np.asarray(struct, F32)
    return {"mean": s[0:5], "q99": s[5:10], "q50": s[10:15], "cv": s[15:55].reshape(5, 8)}
