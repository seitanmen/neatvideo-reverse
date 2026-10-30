"""FUN_00ad319c: the pattern-noise part of FUN_00995250 (set 4, the family-C pass of every level), the
words from 0x1464 of the model (reference implementation; ported in sandbox/dirfilt, 2026-10-16).

Called by nv_model.model_full with ctx = dict(out, band, M, W, H, k): band = band_k, M = the family-C
image (2 band_k + up2(band_k+1) blended with the argmax-selected oriented response, FUN_009ed978).
Writes (byte offsets of the model; P = 0x1464):
  P + 0x20 k + 4 j            score of the j-th self-similar pattern of level k (j < 5)
  P + 0xa0 + 0x200 k + 0x40 j  its 24 (dx, dy) int8 offsets (12 selected + their negations)
  P + 0xaa0 / 0xab4 + 4 k      FUN_00ad1ae0 quantile 0x6c of config 0 / 1 (block-matching SAD ratios)
  P + 0xac8 / 0xadc + 4 k      FUN_00ad1ae0 quantile 0x70 of config 0 / 1
Notes on the algorithm and the float order: analysis/noise/model_notes/special_algo.md."""
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
import nv_model as nm  # noqa: E402
from nv_cov import hit  # noqa: E402

F32 = np.float32
U32 = np.uint32
P_OFF = 0x1464
C_2M10 = np.array([0x3a800000], U32).view(F32)[0]        # 2^-10
C_1E3 = np.array([0x3a83126f], U32).view(F32)[0]         # 0.001
C_1_24 = np.array([0x3d2aaaab], U32).view(F32)[0]        # 1/24
C_SCORE = np.array([0x407988e2], U32).view(F32)[0]       # 3.8989797
C_THR_TILED = np.array([0x3e19999a], U32).view(F32)[0]   # 0.15
C_THR = np.array([0x3dcccccd], U32).view(F32)[0]         # 0.1
C_064 = np.array([0x3f23d70a], U32).view(F32)[0]         # 0.64
C_1E4 = np.array([0x38d1b717], U32).view(F32)[0]         # 1e-4
C_NINTH = np.array([0x3de38e39], U32).view(F32)[0]       # 1/9
FLT_MAX = np.finfo(F32).max


# ------------------------------------------------------------------------------------------ candidate lists
def offsets_ring(rmin, r):
    """FUN_00acf05c list: (dx, dy) for dy = -r..r, dx = -r..r (raster) with rmin^2 <= dx^2 + dy^2 <= r (r + 1)."""
    out = [(dx, dy) for dy in range(-r, r + 1) for dx in range(-r, r + 1)
           if rmin * rmin <= dx * dx + dy * dy <= (r + 1) * r]
    return np.array(out, np.int64).reshape(-1, 2)


CAND24 = offsets_ring(4, 24)         # DAT_0156bdc0 + 0x160 (+0x170 dx, +0x178 dy, +0x190 count)


# ------------------------------------------------------------------------------------------ helpers
def minmax(v):
    """FUN_009efdd4(v, &min, &max, n): NEON 8-wide FMIN / FMAX over the first n & ~7 values (start +-FLT_MAX),
    lanes combined, FMINV / FMAXV; then scalar fcmp / csel over the rest (gt / mi: a NaN is passed over)."""
    v = np.ascontiguousarray(v, F32).ravel()
    n = v.size
    m = n & ~7
    mx = nb._vmax(v)                        # same structure for the maximum (FMAX lanes, FMAXV, scalar gt)
    if m:
        L = v[:m].reshape(-1, 8)
        lanes = []
        for j in range(8):
            col = L[:, j]
            nan = np.isnan(col)
            if nan.any():
                lanes.append(nb._quiet(col[np.nonzero(nan)[0][-1]]))
                continue
            lo = col.min()
            if lo > FLT_MAX:
                lo = FLT_MAX
            if lo == F32(0.0):
                lo = F32(-0.0) if np.any((col == 0) & np.signbit(col)) else F32(0.0)
            lanes.append(F32(lo))
        c = [_fmin(lanes[j], lanes[j + 4]) for j in range(4)]
        mn = _fmin(_fmin(c[0], c[1]), _fmin(c[2], c[3]))
    else:
        mn = FLT_MAX
    for x in v[m:]:
        if x < mn:
            mn = x
    return F32(mn), F32(mx)


def _fmin(a, b):
    a, b = F32(a), F32(b)
    if np.isnan(a) or np.isnan(b):
        ua, ub = int(np.asarray(a).view(U32)), int(np.asarray(b).view(U32))
        if np.isnan(a) and not ua & 0x400000:
            return nb._quiet(a)
        if np.isnan(b) and not ub & 0x400000:
            return nb._quiet(b)
        return a if np.isnan(a) else b
    if a == b == F32(0.0):
        return F32(-0.0) if (np.signbit(a) or np.signbit(b)) else F32(0.0)
    return a if a < b else b


def to_u8_rows(A, scale, offset):
    """FUN_009f1110 per row (n = W): first W & ~15 pixels fcvtnu(fma(x, scale, offset)) saturated to u8; the rest
    frintn((x * scale) + offset), fcvtzs, clamped to 0..255."""
    A = np.ascontiguousarray(A, F32)
    H, W = A.shape
    m = W & ~15
    out = np.empty((H, W), np.uint8)
    with np.errstate(invalid="ignore", over="ignore"):
        if m:
            y = npx.fma32(A[:, :m], scale, np.full((H, m), offset, F32))
            r = np.rint(y.astype(np.float64))
            r = np.where(np.isnan(r), 0, r)
            out[:, :m] = np.clip(r, 0, 255).astype(np.uint8)
        if m < W:
            hit("u8_tail")
            y = nm.fadd(nm.fmul(A[:, m:], scale), offset)
            r = np.rint(y.astype(np.float64))
            r = np.where(np.isnan(r), 0, r)
            out[:, m:] = np.clip(r, 0, 255).astype(np.uint8)
    return out


def image_u8(A):
    """minmax, then 0x80 everywhere when max == min, else the per-row conversion with s = 255 / (max - min),
    offset = s * -min."""
    mn, mx = minmax(A)
    if mx == mn:
        hit("u8_const")
        return np.full(A.shape, 0x80, np.uint8)
    s = F32(F32(255.0) / F32(mx - mn))
    off = F32(s * F32(-mn))
    return to_u8_rows(A, s, off)


def box3_fused(X):
    """FUN_00990c0c(kernel, src, ..., r = 1, mode 1) with the NEON 2-D path FUN_009ed014 (W >= 4, H >= 2):
    h = (x + left) + right (zero padded), out_0 = ((h_0 + 0) + h_1) / 9, out_r = ((h_r + h_r-1) + h_r+1) / 9,
    out_last = 0 + (h_last + h_last-1), times 1/9 (0x3de38e39)."""
    X = np.ascontiguousarray(X, F32)
    H, W = X.shape
    if W < 4 or H < 2:
        raise NotImplementedError("separable fallback of FUN_00990c0c not needed here")
    z = np.zeros((H, 1), F32)
    L = np.concatenate([z, X[:, :-1]], 1)
    R = np.concatenate([X[:, 1:], z], 1)
    h = nm.fadd(nm.fadd(X, L), R)
    out = np.empty_like(h)
    zero = np.zeros(W, F32)
    out[0] = nm.fadd(nm.fadd(h[0], zero), h[1])
    if H > 2:
        out[1:H - 1] = nm.fadd(nm.fadd(h[1:H - 1], h[0:H - 2]), h[2:H])
    out[H - 1] = nm.fadd(zero, nm.fadd(h[H - 1], h[H - 2]))
    return nm.fmul(out, C_NINTH)


def block_sum32(B):
    """FUN_009fada0 on blocks B (..., 32, 32): 4 accumulators of 4 lanes, per row acc_a += x[4a:4a+4] then
    acc_a += x[16+4a:16+4a+4]; v = (acc1 + acc0) ... ((v3 + v2) + (v1 + v0)), faddp, faddp."""
    B = np.asarray(B, F32)
    acc = np.zeros(B.shape[:-2] + (4, 4), F32)
    with np.errstate(invalid="ignore", over="ignore"):
        for r in range(32):
            row = B[..., r, :].reshape(B.shape[:-2] + (8, 4))
            acc = nm.fadd(acc, row[..., 0:4, :])
            acc = nm.fadd(acc, row[..., 4:8, :])
        v0 = nm.fadd(acc[..., 1, :], acc[..., 0, :])
        v1 = nm.fadd(acc[..., 3, :], acc[..., 2, :])
        v = nm.fadd(v1, v0)
        return nm.fadd(nm.fadd(v[..., 0], v[..., 1]), nm.fadd(v[..., 2], v[..., 3]))


def block_std32(B, m):
    """FUN_009ef664(m, block, stride): one 4-lane accumulator, d = x - m, FMLA acc += d * d over the 8 chunks of
    every row; (l0 + l1) + (l2 + l3), sqrt(sum * 2^-10)."""
    B = np.asarray(B, F32)
    m = np.asarray(m, F32)[..., None]
    acc = np.zeros(B.shape[:-2] + (4,), F32)
    with np.errstate(invalid="ignore", over="ignore"):
        for r in range(32):
            row = B[..., r, :]
            for c in range(8):
                d = nm.fsub(row[..., 4 * c:4 * c + 4], m)
                acc = npx.fma32(d, d, acc)
        s = nm.fadd(nm.fadd(acc[..., 0], acc[..., 1]), nm.fadd(acc[..., 2], acc[..., 3]))
        return nm.sqrt_abs(nm.fmul(s, C_2M10))      # s >= 0 or NaN: fsqrt


def sad_maps(U8, bx, by, cand):
    """FUN_00ad086c with the 32 x 32 SAD callbacks FUN_009f9290 / FUN_009f93ac (exact integers): cost of every
    candidate (dx, dy) = sum |U8[by+dy+r, bx+dx+c] - U8[by+r, bx+c]|.  All positions used here are inside the
    image (the out-of-range code 0x7fffffff never occurs)."""
    ref = U8[by:by + 32, bx:bx + 32].astype(np.int32)
    out = np.empty(len(cand), np.int64)
    for i, (dx, dy) in enumerate(cand):
        out[i] = np.abs(U8[by + dy:by + dy + 32, bx + dx:bx + dx + 32].astype(np.int32) - ref).sum()
    return out


def sad_all(U8, bx, by, r=24):
    """SAD for every (dx, dy) in [-r, r]^2 -> (2r+1, 2r+1) [dy + r, dx + r] (vectorised; the 32x32 sums
    fit int32 since 255 * 1024 < 2^31, so the exact integers are unchanged)."""
    ref = U8[by:by + 32, bx:bx + 32].astype(np.int16)
    reg = U8[by - r:by + r + 32, bx - r:bx + r + 32].astype(np.int16)
    win = np.lib.stride_tricks.sliding_window_view(reg, (32, 32))      # (2r+1, 2r+1, 32, 32)
    return np.abs(win - ref).sum(axis=(2, 3), dtype=np.int32).astype(np.int64)


def select_greedy(costs, cand, nsel=12, sup=2, r=24):
    """FUN_00ad0b88(..., nsel, nsel, r, sup = 2): nsel times the first strict minimum among the unmasked candidates
    (start 0x7fffffff, index 0 when none), then the (2 sup - 1)^2 square around it is masked."""
    side = 2 * sup + (2 * r | 1) - 2
    mask = np.zeros(side * side, bool)
    center = (side * side) >> 1
    midx = cand[:, 0] + side * cand[:, 1] + center
    sel = []
    c = np.asarray(costs, np.int64)
    for _ in range(nsel):
        avail = ~mask[midx]
        cc = np.where(avail, c, np.int64(0x7fffffff))
        j = int(np.argmin(cc))
        if not cc[j] < 0x7fffffff:
            j = 0
        dx, dy = int(cand[j, 0]), int(cand[j, 1])
        sel.append((dx, dy))
        for yy in range(dy - (sup - 1), dy + sup):
            base = center + side * yy + dx - (sup - 1)
            mask[base:base + 2 * sup - 1] = True
    return sel


# ------------------------------------------------------------------------------------------ FUN_00ad3c88
def local_rms(X, r):
    """FUN_00ad19ac(obj, src, dst, r) for r != 1: sqrt|box_cols(box_rows(src^2, r, 1), r, 1)| * 1.0."""
    return nm.fmul(F32(1.0), nm.sqrt_abs(nm.box2(nm.sq(X), r, 1)))


def pattern_remove_ref(src, offs, accum=None):
    """FUN_00ad3c88(obj, src, dst, accum, offsets, n): dst = src - ramp(rms16(src) / (rms2(avg) + 1e-4)) * avg,
    avg = mean of src shifted by the n offsets (borders: only the valid shifts)."""
    src = np.ascontiguousarray(src, F32)
    H, W = src.shape
    n = len(offs)
    dst = np.zeros((H, W), F32)
    with np.errstate(invalid="ignore", over="ignore", divide="ignore"):
        if H > 48:
            inv = F32(F32(1.0) / F32(n))
            for y in range(24, H - 24):
                acc = dst[y, 24:W - 24]
                for dx, dy in offs:
                    acc = nm.fadd(src[y + dy, 24 + dx:W - 24 + dx], acc)
                dst[y, 24:W - 24] = nm.fmul(acc, inv)
        for y in list(range(24)) + list(range(max(H, 48) - 24, H)):
            acc = dst[y, 24:W - 24]
            cnt = 0
            for dx, dy in offs:
                yy = y + dy
                if (y < 24 and 0 <= yy < H) or (y >= 24 and yy < H):
                    acc = nm.fadd(src[yy, 24 + dx:W - 24 + dx], acc)
                    cnt += 1
            if cnt:
                acc = nm.fmul(acc, F32(F32(1.0) / F32(cnt)))
            dst[y, 24:W - 24] = acc
        xs = list(range(24)) + list(range(max(W, 48) - 24, W))
        for y in range(H):
            for x in xs:
                acc = dst[y, x]
                cnt = 0
                for dx, dy in offs:
                    yy, xx = y + dy, x + dx
                    if 0 <= yy < H and xx < W and (x >= 24 or xx >= 0):
                        acc = nm.fadd(src[yy, xx], acc)
                        cnt += 1
                if cnt:
                    acc = F32(acc / F32(cnt))
                dst[y, x] = acc
    P = local_rms(src, 16)
    Q = local_rms(dst, 2)
    ratio = npx.ratio_eps(P.ravel(), Q.ravel(), C_1E4)
    t = F32(nm.sqrt_abs(F32(n)) * C_064)
    w = npx.ramp(t, F32(t * F32(1.5)), F32(1.0), F32(0.0), ratio)
    d = dst.ravel()
    m = d.size & ~3
    y = np.empty_like(d)
    y[:m] = nm.fmul(w[:m], d[:m])
    y[m:] = nm.fmul(d[m:], w[m:])
    y = y.reshape(H, W)
    if accum is not None:
        accum[...] = nm.fadd(accum, y)
    return nm.fsub(src, y)


# ------------------------------------------------------------------------------------------ FUN_00ad319c
def pattern_remove_fast(src, offs, accum=None):
    """pattern_remove_ref with the border rows / columns vectorised: every element still adds the valid shifted
    values in the offset order (acc = shifted + acc) and is scaled / divided by its own count."""
    src = np.ascontiguousarray(src, F32)
    H, W = src.shape
    n = len(offs)
    dst = np.zeros((H, W), F32)
    with np.errstate(invalid="ignore", over="ignore", divide="ignore"):
        if H > 48:
            inv = F32(F32(1.0) / F32(n))
            acc = dst[24:H - 24, 24:W - 24]
            for dx, dy in offs:
                acc = nm.fadd(src[24 + dy:H - 24 + dy, 24 + dx:W - 24 + dx], acc)
            dst[24:H - 24, 24:W - 24] = nm.fmul(acc, inv)
        rows = np.array(list(range(24)) + list(range(max(H, 48) - 24, H)), np.int64)
        acc = dst[rows, 24:W - 24]
        cnt = np.zeros(len(rows), np.int64)
        for dx, dy in offs:
            yy = rows + dy
            ok = np.where(rows < 24, (yy >= 0) & (yy < H), yy < H)
            yc = np.clip(yy, 0, H - 1)
            acc = np.where(ok[:, None], nm.fadd(src[yc, 24 + dx:W - 24 + dx], acc), acc)
            cnt += ok
        scale = np.where(cnt > 0, (F32(1.0) / np.maximum(cnt, 1).astype(F32)).astype(F32), F32(1.0)).astype(F32)
        acc = np.where((cnt > 0)[:, None], nm.fmul(acc, scale[:, None]), acc)
        dst[rows, 24:W - 24] = acc
        xs = np.array(list(range(24)) + list(range(max(W, 48) - 24, W)), np.int64)
        ys = np.arange(H)
        acc = dst[:, xs]
        cnt = np.zeros((H, len(xs)), np.int64)
        for dx, dy in offs:
            yy = ys + dy
            xx = xs + dx
            ok = ((yy >= 0) & (yy < H))[:, None] & ((xx < W) & ((xs >= 24) | (xx >= 0)))[None, :]
            v = src[np.clip(yy, 0, H - 1)][:, np.clip(xx, 0, W - 1)]
            acc = np.where(ok, nm.fadd(v, acc), acc)
            cnt += ok
        div = np.where(cnt > 0, (acc / np.maximum(cnt, 1).astype(F32)).astype(F32), acc)
        dst[:, xs] = nm._nanfix(div, acc) if np.isnan(acc).any() else div
    P = local_rms(src, 16)
    Q = local_rms(dst, 2)
    ratio = npx.ratio_eps(P.ravel(), Q.ravel(), C_1E4)
    t = F32(nm.sqrt_abs(F32(n)) * C_064)
    w = npx.ramp(t, F32(t * F32(1.5)), F32(1.0), F32(0.0), ratio)
    d = dst.ravel()
    m = d.size & ~3
    y = np.empty_like(d)
    y[:m] = nm.fmul(w[:m], d[:m])
    y[m:] = nm.fmul(d[m:], w[m:])
    y = y.reshape(H, W)
    if accum is not None:
        accum[...] = nm.fadd(accum, y)
    return nm.fsub(src, y)


FAST = True


def pattern_remove(src, offs, accum=None):
    return (pattern_remove_fast if FAST else pattern_remove_ref)(src, offs, accum)


def block_std32_fast(B, m):
    """block_std32 with the FMLA chain on the float64 path (npx._fma32_fast, exact with fallback)."""
    B = np.asarray(B, F32)
    m = np.asarray(m, F32)[..., None]
    acc = np.zeros(B.shape[:-2] + (4,), F32)
    with np.errstate(invalid="ignore", over="ignore"):
        for r in range(32):
            row = B[..., r, :]
            for c in range(8):
                d = nm.fsub(row[..., 4 * c:4 * c + 4], m)
                acc = nm.fma_fast(d, d, acc)
        s = nm.fadd(nm.fadd(acc[..., 0], acc[..., 1]), nm.fadd(acc[..., 2], acc[..., 3]))
        return nm.sqrt_abs(nm.fmul(s, C_2M10))


def _offsets_at(U8, bx, by):
    S = sad_all(U8, bx, by)
    costs = S[CAND24[:, 1] + 24, CAND24[:, 0] + 24]
    sel = select_greedy(costs, CAND24)
    return sel + [(-dx, -dy) for dx, dy in sel]


def sad_selected(U8, bx, by, cand, r=24):
    """sad_all restricted to the candidate offsets (same exact integers: the same |window - ref| sums,
    only the 24 candidates instead of the whole (2r+1)^2 map)."""
    ref = U8[by:by + 32, bx:bx + 32].astype(np.int16)
    reg = U8[by - r:by + r + 32, bx - r:bx + r + 32].astype(np.int16)
    win = np.lib.stride_tricks.sliding_window_view(reg, (32, 32))
    sel = win[cand[:, 1] + r, cand[:, 0] + r]
    return np.abs(sel - ref).sum(axis=(1, 2), dtype=np.int32).astype(np.int64)


def _offsets_at_fast(U8, bx, by):
    costs = sad_selected(U8, bx, by, CAND24)
    sel = select_greedy(costs, CAND24)
    return sel + [(-dx, -dy) for dx, dy in sel]


def _grid_ref(A, U8, grid):
    """Per grid position: (24 offsets, rms, score) as FUN_00ad319c computes them (score None if rms < 0.001)."""
    res = []
    for bx, by in grid:
        offs = _offsets_at(U8, bx, by)
        blocks = np.stack([A[by + dy:by + dy + 32, bx + dx:bx + dx + 32] for dx, dy in offs])
        sm = block_sum32(blocks)
        sd = block_std32(blocks, nm.fmul(sm, C_2M10))
        s8 = F32(0.0)
        for i in range(24):
            s8 = F32(s8 + F32(sd[i] * sd[i]))
        rms = F32(nm.sqrt_abs(s8) / F32(24.0))
        score = None
        if not rms < C_1E3:
            acc = np.zeros((32, 32), F32)
            for b in blocks:
                acc = nm.fadd(b, acc)
            acc = nm.fmul(C_1_24, acc).ravel()
            mean = nb.mean_exact(acc)
            std = nb.std_exact(acc, mean)
            score = F32(F32(F32(std / rms) + F32(-1.0)) / C_SCORE)
        res.append((offs, rms, score))
    return res


def _grid_fast(A, U8, grid):
    """_grid_ref for all positions at once (same float operations per position)."""
    offs_all = [(_offsets_at_fast if FAST else _offsets_at)(U8, bx, by) for bx, by in grid]
    blocks = np.stack([np.stack([A[by + dy:by + dy + 32, bx + dx:bx + dx + 32] for dx, dy in offs])
                       for (bx, by), offs in zip(grid, offs_all)])          # (npos, 24, 32, 32)
    sm = block_sum32(blocks)
    sd = block_std32_fast(blocks, nm.fmul(sm, C_2M10))                       # (npos, 24)
    s8 = np.zeros(len(grid), F32)
    for i in range(24):
        s8 = nm.fadd(s8, nm.fmul(sd[:, i], sd[:, i]))
    rms = (nm.sqrt_abs(s8) / F32(24.0)).astype(F32)
    acc = np.zeros((len(grid), 32, 32), F32)
    for i in range(24):
        acc = nm.fadd(blocks[:, i], acc)
    acc = nm.fmul(C_1_24, acc).reshape(len(grid), 1024)
    mean = nb.mean_bm(acc.T)
    std = nb.std_bm(acc.T, mean)
    with np.errstate(invalid="ignore", divide="ignore", over="ignore"):
        score = nm.fdiv(nm.fadd(nm.fdiv(std, rms), F32(-1.0)), C_SCORE)
    return [(o, rms[i], None if rms[i] < C_1E3 else score[i]) for i, o in enumerate(offs_all)]


def tile3(X):
    return np.ascontiguousarray(np.tile(np.asarray(X, F32), (3, 3)))


def ad319c(ctx, finetune=None, stats=None):
    out = ctx["out"]
    W, H, k = ctx["W"], ctx["H"], ctx["k"]
    if min(W, H) * 3 < 83:
        hit("sp_too_small")
        return
    band, M = np.asarray(ctx["band"], F32), np.asarray(ctx["M"], F32)
    if W >= 89 and H > 88:
        hit("sp_direct")
        thr = C_THR
    else:
        hit("sp_tiled")
        thr = C_THR_TILED
        band, M = tile3(band), tile3(M)
    H2, W2 = M.shape
    A = nm.fadd(M, box3_fused(M))
    cx = (int((W2 - 81) / 24)) | 3
    cy = (int((H2 - 81) / 24)) | 3
    stx = int((W2 - 81) / (cx - 1))
    sty = int((H2 - 81) / (cy - 1))
    words = out.view(U32)
    byte = out.view(np.uint8)
    score_w = lambda j: (P_OFF + 0x20 * k + 4 * j) // 4
    offs_b = lambda j: P_OFF + 0xa0 + 0x200 * k + 0x40 * j
    for j in range(5):
        hit(f"sp_iter{j}")
        U8 = image_u8(A)                # A changes at the end of each pass (pattern_remove)
        out[score_w(j)] = F32(0.0)
        best = F32(0.0)
        grid = [(24 + gx * stx, 24 + gy * sty) for gy in range(cy) for gx in range(cx)]
        cands = (_grid_fast if FAST else _grid_ref)(A, U8, grid)
        for (bx, by), (offs, rms, score) in zip(grid, cands):
            if not rms >= C_1E3 and not np.isnan(rms):
                hit("sp_rms_skip")
                continue
            if np.isnan(score):
                hit("sp_score_nan")
            if stats is not None:
                stats.append((k, j, bx, by, float(rms), float(score)))
            if score > best:                      # fcmp / b.le skips: le holds when unordered
                best = score
                out[score_w(j)] = best
                raw = np.array([(dx, dy) for dx, dy in offs], np.int8).ravel().view(np.uint8)
                byte[offs_b(j):offs_b(j) + 48] = raw
        if best < thr:
            hit("sp_break_thr")
            out[score_w(j)] = F32(0.0)
            break
        if j > 0 and out[score_w(j - 1)] < best:
            hit("sp_break_prev")
            out[score_w(j)] = F32(0.0)
            break
        offs = byte[offs_b(j):offs_b(j) + 48].view(np.int8).reshape(24, 2).astype(np.int64)
        A = pattern_remove(A, [tuple(o) for o in offs])
    # FUN_00ad42b8 (only when the first pattern counts) and FUN_00ad1ae0
    if out[score_w(0)] > F32(0.0):
        hit("sp_ad42b8")
        X = band
        for j in range(8):
            if not out[score_w(j)] > F32(0.0):
                break
            offs = byte[offs_b(j):offs_b(j) + 48].view(np.int8).reshape(24, 2).astype(np.int64)
            X = pattern_remove(X, [tuple(o) for o in offs])
        clean = X
    else:
        clean = band
    if finetune is not None:
        finetune(out, k, clean, A)
    else:
        try:
            import nv_model_bm as bm
        except ImportError:
            return
        bm.ad1ae0_both(out, k, clean, A)
