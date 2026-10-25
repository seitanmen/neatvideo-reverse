"""Neat Video 6 (arm64 slice) FUN_00995250: the per-plane model of BuildNoiseModel, all 4096 words of
the struct `nvh model` dumps (reference implementation, ported 2026-10-16; speed comes second).

    model_full(plane, set_=-1) -> uint32[4096]

Words 0..54 (band levels) come unchanged from nv_band.model_levels (bit-exact port of 2026-10-16).
This module adds the words of the direction-filter sets (DAT_01243988, 5 sets of 0x260 bytes, read
statically from __TEXT,__const; see analysis/noise/model_notes/layout.md) that FUN_00995250 writes per level k:

  0x0dc + 0x14 s + 4 k            "dc"  q99 of the residual weighted by the band's local RMS (sets 2..4)
  0x140 + 0x140 s + 0x40 k + 4 d  "A"   q99 of the oriented energy E_d of the robustly weighted band
  0x780 + 0x14 s + 4 k            "R"   q99 of the local RMS of the residual band - F_argmax
  0x7e4 + 0x140 s + 0x40 k + 4 d  "B"   q99 of the oriented energy of the 2-band image (radius 6)
  0xe24 + 0x140 s + 0x40 k + 4 d  "C"   q99 of the oriented energy of the 2-band image (radius 12)
  0x1464 ..                        FUN_00ad319c (set 4 only) - see nv_model.ad319c

Float semantics follow the disassembly (analysis/noise/model_notes/model_algo.md); every element-wise helper keeps the NEON
(first n & ~3 pixels) / scalar split of the genuine where the two differ."""
import os
import sys

import numpy as np

sys.dont_write_bytecode = True
_HERE = os.path.dirname(os.path.abspath(__file__))
for _p in (_HERE,):
    if _p not in sys.path:
        sys.path.insert(0, _p)
import nv_band as nb  # noqa: E402
import nv_lines as nl  # noqa: E402
import nv_pixnoise as npx  # noqa: E402
from nv_cov import hit  # noqa: E402

F32 = np.float32
U32 = np.uint32
LEVELS = 5
NEG_1E6 = nb.NEG_1E6
NEG_1E6_BITS = U32(0xc9742400)
EPS_1E6 = nb.EPS_1E6
Q99 = nb.Q99
C11 = np.array([0x3f8ccccd], U32).view(F32)[0]       # 1.1
C125 = F32(1.25)
LEVEL_C = np.array([0x3f87ae14, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000], U32).view(F32)  # DAT_012445a8

# ------------------------------------------------------------------------------------------ the set table
_ANG16 = [0x0, 0x3e7adbb0, 0x3eed6338, 0x3f24bc7d, 0x3f490fdb, 0x3f6d6338, 0x3f8db70d, 0x3fa9b465,
          0x3fc90fdb, 0x3fe86b51, 0x40023454, 0x400db70d, 0x4016cbe4, 0x401fe0bb, 0x402b6374, 0x40396220]
_ANG8 = [0x0, 0x3fc90fdb, 0x3f490fdb, 0x4016cbe4, 0x3eed6338, 0x40023454, 0x3f8db70d, 0x402b6374]
_IDX16 = (0, 0, 0, 4, 4, 4, 8, 8, 8, 8, 8, 12, 12, 12, 0, 0)
_RAD16 = (2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 2)


def _set(flag, angles, idx2, rad2, A, B, C, dc, dc_twice, special):
    """A/B/C = (radius, smoothing radius, quantile div, border) of the families; dc = (radius, div,
    border) of the "dc" measure (DAT_012438f8 / DAT_01243928 / DAT_01243950)."""
    return dict(flag=flag, angles=np.array(angles, U32).view(F32), idx2=tuple(idx2), rad2=tuple(rad2),
                A=A, B=B, C=C, dc=dc, dc_twice=dc_twice, special=special)


SETS = (
    _set(0, [], [], [], (0, 0, 0, 0), (0, 0, 0, 0), (0, 0, 0, 0), (0, 0, 0), 0, 0),
    _set(0, _ANG8[:4], [-1] * 4, [1] * 4, (4, 1, 6, 4), (0, 0, 0, 0), (0, 0, 0, 0), (0, 0, 0), 0, 0),
    _set(0, _ANG8, [-1, -1, -1, -1, 0, 1, 1, 0], [1] * 8, (4, 1, 8, 4), (0, 0, 0, 0), (0, 0, 0, 0),
         (4, 16, 4), 0, 0),
    _set(1, _ANG16, _IDX16, _RAD16, (4, 1, 16, 4), (6, 2, 24, 6), (0, 0, 0, 0), (4, 36, 6), 1, 0),
    _set(1, _ANG16, _IDX16, _RAD16, (4, 1, 16, 4), (6, 2, 24, 6), (12, 2, 48, 12), (4, 36, 6), 1, 1),
)
OFF_DC, OFF_A, OFF_R, OFF_B, OFF_C, OFF_SPECIAL = 0xdc, 0x140, 0x780, 0x7e4, 0xe24, 0x1464

_KCACHE = {}
_KCACHE_MAX = 512


def kernels(Wk, Hk, angles):
    """LineKernel per angle at (Wk, Hk), cached by the float32 bits of the angles (kernels are
    immutable after construction; the lazy _gather is shared safely)."""
    key = (int(Wk), int(Hk), tuple(int(np.asarray(a, F32).view(np.uint32)) for a in angles))
    K = _KCACHE.get(key)
    if K is None:
        if len(_KCACHE) >= _KCACHE_MAX:
            _KCACHE.clear()
        K = _KCACHE[key] = [nl.LineKernel(Wk, Hk, a) for a in angles]
    return K


# ------------------------------------------------------------------------------------------ float helpers
def _bits(x):
    return np.asarray(x, F32).view(U32)


def _nanfix(r, *ops):
    """Give r the NaN of FPProcessNaNs(ops) wherever an operand is a NaN (DN = 0)."""
    ops = [np.asarray(o, F32) for o in ops]
    nan = np.zeros(np.broadcast(*ops).shape, bool)
    for o in ops:
        nan |= np.isnan(o)
    if nan.any():
        done, b = npx._process_nans(*[_bits(o) for o in ops])
        r = np.where(done, b.view(F32), r).astype(F32)
    return r


def fsub(a, b):
    with np.errstate(invalid="ignore", over="ignore"):
        return _nanfix((np.asarray(a, F32) - np.asarray(b, F32)).astype(F32), a, b)


def fadd(a, b):
    with np.errstate(invalid="ignore", over="ignore"):
        return _nanfix((np.asarray(a, F32) + np.asarray(b, F32)).astype(F32), a, b)


def fmul(a, b):
    with np.errstate(invalid="ignore", over="ignore"):
        return _nanfix((np.asarray(a, F32) * np.asarray(b, F32)).astype(F32), a, b)


def fdiv(a, b):
    with np.errstate(invalid="ignore", over="ignore", divide="ignore"):
        return _nanfix((np.asarray(a, F32) / np.asarray(b, F32)).astype(F32), a, b)


def fma_fast(a, b, c):
    """fma32(a, b, c) (FMLA, one rounding): float64 s = a*b + c (the product is exact) rounded to float32; when
    s lies on a float32 midpoint (or out of the normal range / special) the float64 sum must itself be exact
    (TwoSum error 0) for float32(s) to be the fused result, otherwise nv_pixnoise.fma32 decides."""
    a = np.asarray(a, F32)
    b = np.asarray(b, F32)
    c = np.asarray(c, F32)
    shape = np.broadcast_shapes(a.shape, b.shape, c.shape)
    if not shape:
        return npx.fma32(a, b, c)
    with np.errstate(invalid="ignore", over="ignore"):
        p = np.multiply(a, b, dtype=np.float64)
        c64 = np.broadcast_to(c, shape).astype(np.float64)
        s = p + c64
        r, flag = nb._round_or_flag(s)
        if flag.any():
            t = s - p
            err = (p - (s - t)) + (c64 - t)
            bad = flag & ((err != 0) | nb._range_flag(s) | np.isnan(err))
            if bad.any():
                r[bad] = npx.fma32(np.broadcast_to(a, shape)[bad], np.broadcast_to(b, shape)[bad],
                                   np.broadcast_to(c, shape)[bad])
    return r


def sq(x):
    """FUN_009ed2f0: x * x."""
    return fmul(x, x)


def sqrt_abs(x):
    """FUN_009ed3e0: fsqrt(fabs(x)) (a NaN keeps its payload, sign cleared by fabs)."""
    with np.errstate(invalid="ignore"):
        a = (_bits(x) & U32(0x7fffffff)).view(F32)
        r = np.sqrt(a).astype(F32)
    return _nanfix(r, a)


def robust_weight(t, a, e):
    """FUN_009ef0d0(t, a, e, dst, n): inv = 1 / t (fdiv); z = inv * e; NEON (first n & ~3):
    dst = a * (|z| > 1 ? FRECPE(z*z) * FRECPS(z*z, est) : 1.0); scalar tail: |z| <= 1 (b.le, also taken when
    unordered) ? a : a / (z * z) - a NaN z gives a on both paths."""
    a = np.ascontiguousarray(a, F32).ravel()
    e = np.ascontiguousarray(e, F32).ravel()
    inv = F32(F32(1.0) / F32(t))
    n = a.size
    m = n & ~3
    out = np.empty(n, F32)
    with np.errstate(invalid="ignore", over="ignore", divide="ignore"):
        if m:
            z = fmul(inv, e[:m])
            big = np.abs(z) > F32(1.0)
            z2 = fmul(z, z)
            est = npx.frecpe32(z2)
            w = fmul(est, npx.frecps32(z2, est))
            sel = np.where(big, w, F32(1.0)).astype(F32)
            out[:m] = fmul(a[:m], sel)
        if m < n:
            hit("robust_tail")
            z = fmul(inv, e[m:])
            le = (np.abs(z) <= F32(1.0)) | np.isnan(z)     # fcmp / b.le: an unordered compare counts as le
            z2 = fmul(z, z)
            d = _nanfix((a[m:] / z2).astype(F32), a[m:], z2)
            out[m:] = np.where(le, a[m:], d)
    return out


def blend_add(c, d, A, B):
    """FUN_009ed978(c, d, A, B, dst, n): NEON A + fma(B, c, d) (FMLA, addend d); tail (A + B*c) + d."""
    A = np.ascontiguousarray(A, F32).ravel()
    B = np.ascontiguousarray(B, F32).ravel()
    c, d = F32(c), F32(d)
    n = A.size
    m = n & ~3
    out = np.empty(n, F32)
    if m:
        out[:m] = fadd(A[:m], npx.fma32(B[:m], c, np.full(m, d, F32)))
    if m < n:
        hit("blend_tail")
        out[m:] = fadd(fadd(A[m:], fmul(B[m:], c)), d)
    return out


def argmax_dirs(E):
    """FUN_009ee5fc(maps, idx, max, n, stride, count): NEON (first n & ~3): best = 0, idx = 0, update on
    fcmge (x >= best; a NaN never wins); scalar tail: best = (x < best) ? best : x (fcmp / fcsel lt, true
    for an unordered compare as well, so a NaN never wins there either), idx likewise.  count = 0 gives idx 0 / max 0."""
    cnt = E.shape[0]
    n = E.shape[1]
    m = n & ~3
    best = np.zeros(n, F32)
    idx = np.zeros(n, np.uint8)
    for j in range(cnt):
        x = E[j]
        upd = x >= best          # NEON fcmge; scalar fcmp / fcsel lt keeps the old value when unordered too
        best = np.where(upd, x, best).astype(F32)
        idx = np.where(upd, np.uint8(j), idx)
    return idx, best


def extrap(prev, fac):
    """-1e6 stays, else fac * -|prev| (fabs, fneg, fmul s_fac, x)."""
    prev = np.asarray(prev, F32)
    neg = (_bits(prev) & U32(0x7fffffff) | U32(0x80000000)).view(F32)
    v = fmul(np.broadcast_to(F32(fac), neg.shape), neg)
    return np.where(prev == NEG_1E6, NEG_1E6, v).astype(F32)


# ------------------------------------------------------------------------------------------ pieces
def borders(Wk, Hk, b, min_area):
    """Border / crop size of a measure: bx = min((W-1) >> 1, b) (W itself when W - 1 < 0), small when a
    side is below 2b + 1; shrink both by one while the crop has < min_area pixels.  -> (bx, by, small)."""
    hx = (Wk if Wk - 1 < 0 else Wk - 1) >> 1
    hy = (Hk if Hk - 1 < 0 else Hk - 1) >> 1
    small = hx < b or hy < b
    bx = hx if hx < b else b
    by = hy if hy < b else b
    while (Wk - 2 * bx) * (Hk - 2 * by) < min_area and (bx != 0 or by != 0):
        if bx > 0 or by > 0:
            small = True
        bx, by = bx - (bx > 0), by - (by > 0)
    return bx, by, small


def crop(E, bx, by):
    H, W = E.shape
    return np.ascontiguousarray(E[by:H - by, bx:W - bx])


def q99(v, div):
    """fmaxnm(FUN_00997bcc(0.99, v, NULL, 1, div, n), 1e-6)."""
    return nb.fmaxnm(nb.quantile(np.ascontiguousarray(v, F32).ravel(), Q99, 1, div), EPS_1E6)


def box2(X, r, mode=1):
    """rows (kernel +0x3824) then columns (+0x3825), FUN_009944c0."""
    return nl.box_cols(nl.box_rows(X, r, mode), r, mode)


def dfilt(K, i, X, r, set_):
    """Radius r along kernel i, then rad2[i] along kernel idx2[i] (if idx2[i] != -1), mode 1."""
    f = nl.linefilt(K[i], X, r, 1)
    j = set_["idx2"][i]
    if j != -1:
        f = nl.linefilt(K[j], f, set_["rad2"][i], 1)
    return f


class Pyramid:
    """Levels of FUN_00995250: src_0 = plane, B = box3 (mode 0), lo = 2x2 mean, band_k = src_k - up2(lo);
    E_k = sqrt|box3_zero(band_k^2)| (0x75c0)."""

    def __init__(self, plane, levels=LEVELS):
        src = np.ascontiguousarray(plane, F32)
        H, W = src.shape
        self.sizes = npx.level_sizes(W, H, levels + 1)
        self.band = [None] * levels
        self.E = [None] * levels
        with np.errstate(invalid="ignore", over="ignore"):
            for k in range(levels):
                Wk, Hk = self.sizes[k][:2]
                Wn, Hn = self.sizes[k + 1][:2]
                if Wn < 1 or Hn < 1:
                    break
                B = nl.box_cols(nl.box_rows(src, 1, 0), 1, 0)
                lo = npx.down2(B)
                try:
                    band = npx.band_from(lo, src, self.sizes[k][2], self.sizes[k][3])
                except NotImplementedError:
                    break
                self.band[k] = band
                self.E[k] = sqrt_abs(box2(sq(band), 1, 1))
                src = lo


def level_info(W, H, words, levels=LEVELS):
    """Per level: (small, bx, by) of the level part (border 4, area 32) and fac after the level."""
    out = []
    fac = F32(0.5)
    wf = words.view(F32)
    for k in range(levels):
        Wk, Hk = W >> k, H >> k
        bx, by, small = borders(Wk, Hk, 4, 32)
        if not small and k:
            with np.errstate(invalid="ignore", divide="ignore"):
                r = np.abs(F32(wf[5 + k] / wf[4 + k]))
            if r < F32(0.5):
                fac = F32(0.5)
            else:
                fac = npx._fminmax(np.array([r], F32), F32(1.0), False)[0]
        out.append((small, bx, by, F32(fac)))
    return out


# ------------------------------------------------------------------------------------------ the model
def model_full(plane, set_=-1, special=None, levels=LEVELS):
    """All 4096 words of `nvh model W H plane [set]`.  set_ < 0: sets 0..4, else that set only.
    special: callable(ctx) for the FUN_00ad319c part of set 4 (None: nv_model_special.ad319c if importable)."""
    p = np.ascontiguousarray(plane, F32)
    if p.ndim != 2:
        raise ValueError("plane must be 2-D")
    H, W = p.shape
    if W > nl.MAX_SIDE or H > nl.MAX_SIDE:
        raise ValueError(f"{W}x{H}: FUN_00990448's tables hold sides of <= {nl.MAX_SIDE} px")
    if W < 2 or H < 2:
        raise ValueError("planes of a single row or column are not passed to the genuine (SIGBUS)")
    if not 1 <= levels <= LEVELS:
        raise ValueError("levels must be 1..5")
    words = nb.model_levels(p, levels).view(U32).copy()
    if not np.isfinite(p).all():
        hit("input_nonfinite")
    out = words.view(F32)
    info = level_info(W, H, words, levels)
    pyr = Pyramid(p, levels)
    sets = range(5) if set_ < 0 else [int(set_)]
    if special is None:
        try:
            import nv_model_special as _sp
            special = _sp.ad319c
        except ImportError:
            special = None
    for k in range(levels):
        small, lbx, lby, fac = info[k]
        hit("level_small" if small else "level_measured")
        if not np.isfinite(fac):
            hit("fac_nonfinite")
        Wk, Hk = W >> k, H >> k
        n = Wk * Hk
        for s in sets:
            S = SETS[s]
            nd = len(S["angles"])
            K = None
            q99k = out[5 + k]
            # ---------------------------------------------------------------- family A
            r1, rs, div, b = S["A"]
            if r1 >= 1:
                bx, by, smallA = borders(Wk, Hk, b, 8 * div)
                if Wk <= 2 * r1 or Hk <= 2 * r1 or smallA:
                    hit(f"A_skip_k{min(k, 1)}")
                    if k:
                        for d in range(nd):
                            o = (OFF_A + 0x140 * s + 0x40 * k + 4 * d) // 4
                            out[o] = extrap(out[o - 16], fac)
                        for off in (OFF_R, OFF_DC):
                            o = (off + 0x14 * s + 4 * k) // 4
                            out[o] = extrap(out[o - 1], fac)
                else:
                    hit(f"A_proc_s{s}")
                    K = kernels(Wk, Hk, S["angles"])
                    band = pyr.band[k]
                    F = np.zeros((max(nd, 1), Hk, Wk), F32)
                    E = np.zeros((max(nd, 1), Hk, Wk), F32)
                    for d in range(nd):
                        T = fsub(band, nl.linefilt(K[d], band, r1, 1))
                        c = F32(1.0)
                        if S["flag"]:
                            T = sqrt_abs(box2(sq(T), 1, 1))
                            c = F32(0.75)
                        t = F32(c * F32(LEVEL_C[k] * F32(np.abs(q99k) * C11)))
                        G = robust_weight(t, band, T).reshape(Hk, Wk)
                        Fd = dfilt(K, d, G, r1, S)
                        T = sq(Fd)
                        if S["flag"]:
                            T = box2(box2(T, rs), rs)
                        else:
                            T = box2(T, rs)
                        Ed = sqrt_abs(T)
                        if S["flag"]:
                            Fd = fadd(Fd, dfilt(K, d, fsub(G, Fd), r1, S))
                        q = q99(crop(Ed, bx, by), div)
                        out[(OFF_A + 0x140 * s + 0x40 * k + 4 * d) // 4] = q
                        F[d] = Fd
                        E[d] = fmul(Ed, F32(F32(1.0) / q))
                    idx, _ = argmax_dirs(E[:nd].reshape(nd, -1))
                    Sel = F.reshape(F.shape[0], -1)[idx, np.arange(n)].reshape(Hk, Wk)
                    R = fsub(band, Sel)
                    T = sqrt_abs(box2(box2(sq(R), 1), 1))
                    out[(OFF_R + 0x14 * s + 4 * k) // 4] = q99(crop(T, lbx, lby), 4)
                    # ------------------------------------------------------------ "dc"
                    rdc, ddc, bdc = S["dc"]
                    bx2, by2, smalld = borders(Wk, Hk, bdc, 8 * ddc)
                    o = (OFF_DC + 0x14 * s + 4 * k) // 4
                    if not smalld and rdc >= 1 and Wk > 2 * rdc and Hk > 2 * rdc:
                        hit("dc_proc")
                        t = F32(F32(np.abs(q99k) * LEVEL_C[k]) * C125)
                        T = sq(robust_weight(t, R, pyr.E[k]).reshape(Hk, Wk))
                        T = box2(T, rdc)
                        if S["dc_twice"]:
                            T = box2(T, rdc)
                        T = sqrt_abs(T)
                        out[o] = q99(crop(T, bx2, by2), ddc)
                    elif k:
                        hit("dc_extrap")
                        out[o] = extrap(out[o - 1], fac)
            # ---------------------------------------------------------------- families B, C
            for fam, off in (("B", OFF_B), ("C", OFF_C)):
                r, rs, div, b = S[fam]
                if r < 1:
                    continue
                bx, by, smallF = borders(Wk, Hk, b, 8 * div)
                if Wk <= 2 * r or Hk <= 2 * r or smallF:
                    hit(f"{fam}_skip_k{min(k, 1)}")
                    if k:
                        for d in range(nd):
                            o = (off + 0x140 * s + 0x40 * k + 4 * d) // 4
                            out[o] = extrap(out[o - 16], fac)
                    continue
                hit(f"{fam}_proc_s{s}_k{k}")
                if K is None:
                    K = kernels(Wk, Hk, S["angles"])
                band = pyr.band[k]
                hit("U_up2" if k < levels - 1 else "U_zero")
                if k < levels - 1:
                    U = npx.up2(pyr.band[k + 1], pyr.sizes[k][2], pyr.sizes[k][3])
                else:
                    U = np.zeros((Hk, Wk), F32)
                U = fadd(band, U)
                if k == 0:
                    V = fmul(fadd(U, box2(U, 1, 1)), F32(0.5))
                else:
                    V = U.copy()
                F = np.zeros((max(nd, 1), Hk, Wk), F32)
                E = np.zeros((max(nd, 1), Hk, Wk), F32)
                for d in range(nd):
                    Fd = dfilt(K, d, V, r, S)
                    Ed = sqrt_abs(box2(box2(sq(Fd), rs), rs))
                    out[(off + 0x140 * s + 0x40 * k + 4 * d) // 4] = q99(crop(Ed, bx, by), div)
                    E[d] = Ed
                    if fam == "C":
                        F[d] = dfilt(K, d, U, (r if r >= 0 else r + 1) >> 1, S)
                if fam == "C" and S["special"]:
                    idx, _ = argmax_dirs(E[:nd].reshape(nd, -1))
                    Sel = F.reshape(F.shape[0], -1)[idx, np.arange(n)].reshape(Hk, Wk)
                    U2 = fadd(band, U)
                    M = blend_add(F32(1.0), F32(0.0), U2, Sel).reshape(Hk, Wk)
                    if special is not None:
                        special(dict(out=out, band=band, M=M, W=Wk, H=Hk, k=k, set=s))
    return words
