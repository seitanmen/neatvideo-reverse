"""FUN_00ad1ae0: the block-matching measure at the end of FUN_00ad319c (reference implementation,
sandbox/dirfilt).  Two configurations (DAT_01400f88, DAT_01401058, 0xd0 bytes each, read statically):

  cfg   block bs  step  pad  list-1 r  list-2 r  nsel1 (+self)  nsel2  nlarge  bins  q (0x6c, 0x70)
   0      16       8     4     1..8      1..8       15 (16)       15     64      8    0.5, 0.85
   1      32      16     8     2..16     2..16      31 (32)       31    128     16    0.5, 0.85

  1. u8 image of A (the last pattern-removed image), mirrored padding (FUN_0097ef28);
  2. per block (grid step bs/2, clamped to W - step): SAD of every list-1 offset, the best direction bin of
     the FUN_00acf420 table (FUN_00ad11b0: smallest sum of the nsel1 smallest costs), its nsel1 offsets + (0, 0);
  3. aggregation of the clean band over these offsets with the tent window (FUN_00ad15f0 / FUN_009f97c8),
     normalised by 1 / sum of the windows (FUN_00acfc68);
  4. u8 of the aggregate, per block on a grid of step 8: SAD of every list-2 offset, FUN_00ad0b88 mode 1:
     ratio = (sum of the nlarge largest * nsel2) / (sum of the nsel2 smallest + 1e-5) / nlarge;
  5. FUN_00997bcc(q, ratios, flag 1, div 1) for q = 0.5 and 0.85."""
import ctypes
import os
import sys

import numpy as np

sys.dont_write_bytecode = True
_HERE = os.path.dirname(os.path.abspath(__file__))
for _p in (_HERE,):
    if _p not in sys.path:
        sys.path.insert(0, _p)
import nv_band as nb  # noqa: E402
import nv_pixnoise as npx  # noqa: E402
import nv_model as nm  # noqa: E402
import nv_model_special as sp  # noqa: E402
from nv_cov import hit  # noqa: E402

F32 = np.float32
U32 = np.uint32
INF = 0x7fffffff
C_1E5 = np.array([0x3727c5ac], U32).view(F32)[0]
Q50 = F32(0.5)
Q85 = np.array([0x3f59999a], U32).view(F32)[0]

CFG = (dict(bs=16, step=8, pad=4, r1=8, rmin=1, n8=16, n10=16, n18=64, r28=8, r30=8, nbins=8, fill=F32(16.0),
            q=(Q50, Q85)),
       dict(bs=32, step=16, pad=8, r1=16, rmin=2, n8=32, n10=32, n18=128, r28=16, r30=16, nbins=16, fill=F32(32.0),
            q=(Q50, Q85)))

# ------------------------------------------------------------------------------------------ libm
_libm = ctypes.CDLL("libSystem.B.dylib")
_libm.atan2.restype = ctypes.c_double
_libm.atan2.argtypes = [ctypes.c_double, ctypes.c_double]


class _SinCos(ctypes.Structure):
    _fields_ = [("s", ctypes.c_double), ("c", ctypes.c_double)]


_libm.__sincos_stret.restype = _SinCos
_libm.__sincos_stret.argtypes = [ctypes.c_double]
PI = 3.141592653589793           # DAT_010bb1d8
NPI = -3.141592653589793         # DAT_011da6e8


def _sincos(x):
    r = _libm.__sincos_stret(x)
    return r.s, r.c


# ------------------------------------------------------------------------------------------ FUN_00acf420
def window(n):
    """acf05c tent window (+0x68): w[y][x] = (float)((h - |x - h|) * (h - |y - h|)) / (float)((h * n) >> 1)."""
    h = n >> 1
    d = F32((h * n) >> 1)
    a = h - np.abs(np.arange(n) - h)
    return (np.outer(a, a).astype(F32) / d).astype(F32)


class Table:
    """FUN_00acf420(obj, rmin, r1, r2, nbins, neighbours = 1, fill, window): candidate lists (FUN_00acf05c) and
    the direction bins (+0x78, 0x88 ints per bin; +0x80 counts)."""

    def __init__(self, rmin, r, nbins, win, flag=1):
        self.list1 = sp.offsets_ring(rmin, r)
        self.list2 = sp.offsets_ring(rmin, r)
        self.win = window(win)
        n = len(self.list1)
        L = self.list1
        bins = [[] for _ in range(nbins)]

        def find(dx, dy):
            for j in range(n):
                if L[j, 0] == dx and L[j, 1] == dy:
                    return j
            return -1

        def add(b, idx):
            if idx not in bins[b]:
                bins[b].append(idx)

        binw = PI / float(nbins)
        half = binw * 0.5
        R = float(r // (1 if flag else 2))
        for i in range(n):
            dx, dy = int(L[i, 0]), int(L[i, 1])
            a = _libm.atan2(float(dy), float(dx))
            a = half + (a + PI)
            a = a if a < PI else a + NPI
            a = a if a < PI else a + NPI
            b = int(a / binw)
            add(b, find(dx, dy))
            add(b, find(-dx, -dy))
            if flag:
                center = binw * (float(b) + 0.5)
                if center <= a:
                    b2 = (b + 1) % nbins
                    add(b2, find(dx, dy))
                    add(b2, find(-dx, -dy))
                if not a > center:
                    b2 = (b + nbins - 1) % nbins
                    add(b2, find(dx, dy))
                    add(b2, find(-dx, -dy))
            fy, fx = float(dy), float(dx)
            for l in range(nbins):
                s0, c0 = _sincos(binw * float(l) - half)
                s1, c1 = _sincos(binw * float(l + 1) - half)
                d15, d14 = c0 * R, s0 * R
                d2, d3 = c1 * R, s1 * R
                d4, d5 = -(c0 * R), -(s0 * R)
                e1, e0 = -(c1 * R), -(s1 * R)
                A = (e1 - d15) * (fy - d14) - (fx - d15) * (e0 - d14)
                B = (d4 - d2) * (fy - d3) - (fx - d2) * (d5 - d3)
                if not A * B > 0.0:
                    add(l, find(dx, dy))
                    add(l, find(-dx, -dy))
        self.bins = [np.array(b, np.int64) for b in bins]


_TABLES = {}


def table(c):
    if c not in _TABLES:
        g = CFG[c]
        _TABLES[c] = Table(g["rmin"], g["r1"], g["nbins"], g["bs"])
    return _TABLES[c]


# ------------------------------------------------------------------------------------------ pieces
def pad_mirror(X, p):
    """FUN_0097ef28: FUN_0097ef54 (rows: -i <- i, H-1+i <- H-1-i; edge row when p >= H) then FUN_0097f020
    (columns likewise, over the padded rows)."""
    H, W = X.shape
    if p >= H or p >= W:
        raise NotImplementedError("edge-replicating branch of FUN_0097ef28 (p >= side)")
    return np.pad(X, p, mode="reflect")


def positions(n, step):
    """Block origins min(g * step, n - step), g < ceil(n / step)."""
    cnt = (n + step - 1) // step
    return [min(g * step, n - step) for g in range(cnt)]


def sad_block(Up, x0, y0, bs, r, pad):
    """FUN_00ad086c on the padded u8 image Up (origin at (pad, pad)): cost of every (dx, dy) in [-r, r]^2 for the
    block at (x0, y0) (may be negative, >= -pad): sum |Up[block + (dx, dy)] - Up[block]|, INF where the moved block
    leaves the padded image (0 <= pad + x0 + dx <= W - bs + 2 pad)."""
    Hp, Wp = Up.shape
    ref = Up[pad + y0:pad + y0 + bs, pad + x0:pad + x0 + bs].astype(np.int16)
    out = np.full((2 * r + 1, 2 * r + 1), INF, np.int64)
    ys0, xs0 = pad + y0 - r, pad + x0 - r
    lo_y, lo_x = max(0, -ys0), max(0, -xs0)
    hi_y = min(2 * r, Hp - bs - ys0)
    hi_x = min(2 * r, Wp - bs - xs0)
    if hi_y < lo_y or hi_x < lo_x:
        return out
    reg = Up[ys0 + lo_y:ys0 + hi_y + bs, xs0 + lo_x:xs0 + hi_x + bs].astype(np.int16)
    win = np.lib.stride_tricks.sliding_window_view(reg, (bs, bs))
    out[lo_y:hi_y + 1, lo_x:hi_x + 1] = np.abs(win - ref).sum(axis=(2, 3), dtype=np.int64)
    return out


def sad_grid(Up, xy, bs, r, pad, lst):
    """sad_block for many blocks at once: costs (npos, len(lst)) of the offsets lst, from integral images of
    |Up[p + d] - Up[p]| (exact integers, same values as sad_block)."""
    Hp, Wp = Up.shape
    U = Up.astype(np.int16)
    xy = np.asarray(xy, np.int64)
    X = xy[:, 0] + pad
    Y = xy[:, 1] + pad
    out = np.full((len(xy), len(lst)), INF, np.int64)
    for j, (dx, dy) in enumerate(lst):
        ok = (X + dx >= 0) & (X + dx + bs <= Wp) & (Y + dy >= 0) & (Y + dy + bs <= Hp)
        if not ok.any():
            continue
        y0, y1 = max(0, -dy), Hp - max(0, dy)
        x0, x1 = max(0, -dx), Wp - max(0, dx)
        D = np.abs(U[y0 + dy:y1 + dy, x0 + dx:x1 + dx] - U[y0:y1, x0:x1])
        # int32 integral image: max value 255 * 512 * 512 < 2^31, so the sums are exact integers
        I = np.zeros((y1 - y0 + 1, x1 - x0 + 1), np.int32)
        np.cumsum(np.cumsum(D, 0, dtype=np.int32), 1, dtype=np.int32, out=I[1:, 1:])
        yy, xx = Y[ok] - y0, X[ok] - x0
        out[ok, j] = I[yy + bs, xx + bs] - I[yy, xx + bs] - I[yy + bs, xx] + I[yy, xx]
    return out


def costs_of(S, lst, r):
    return S[lst[:, 1] + r, lst[:, 0] + r]


def sel_value(v, k, lo, hi, mode):
    """FUN_009d4a2c(v, n, k, lo, hi, mode): binary search on the value with FUN_009ef9f4 (count <= t, signed)
    and FUN_009f0050 (largest element <= t; t + 1 when none)."""
    v = np.asarray(v, np.int64)
    n = v.size
    vs = np.sort(v)
    target = (k + 1) if mode != 1 else (n - k - 1) + 1
    cnt = lambda t: int(np.searchsorted(vs, t, "right"))
    mid = (hi + lo) >> 1
    c = cnt(mid)
    if hi - lo < 2:
        hit("sel_short0")
        return lo if target <= c else hi
    while c != target:
        if target <= c:
            hi = mid
        else:
            lo = mid
        mid = (hi + lo) >> 1
        c = cnt(mid)
        if hi - lo < 2:
            hit("sel_short1")
            return lo if target <= c else hi
    hit("sel_exact")
    mid = (hi + lo) >> 1
    le = v[v <= mid]
    return int(le.max()) if le.size else mid + 1


def best_direction(costs, tab, nsel):
    """FUN_00ad11b0: over the bins, the smallest sum of the nsel smallest costs (bins with nsel <= valid / 2
    only, strict <), then its entries with cost < v in order, followed by the entries == v up to nsel.
    Returns (offset list (nsel, 2) or None, bin)."""
    best, info = INF, None
    L = tab.list1
    for b, idx in enumerate(tab.bins):
        vals = costs[idx]
        m = len(idx)
        ninf = int(np.count_nonzero(vals == INF))
        fin = vals[vals != INF]
        maxv = int(fin.max()) if fin.size and fin.max() > 0 else 0
        nval = m - ninf
        if not nsel <= int(nval / 2):
            continue
        v = sel_value(vals, nsel - 1, 0, maxv, 0)
        le = vals <= v
        score = (int(vals[le].sum()) - v * (int(np.count_nonzero(le)) - nsel)) & 0xffffffff
        if score < best:
            best, info = score, (b, idx, vals, v)
    if info is None:
        hit("bm_bins_fail")
        return None, 0xff
    b, idx, vals, v = info
    out = [tuple(L[i]) for i, c in zip(idx, vals) if c < v]
    for i, c in zip(idx, vals):
        if len(out) >= nsel:
            break
        if c == v:
            out.append(tuple(L[i]))
    return np.array(out, np.int64).reshape(-1, 2), b


def ratio_sums(costs, nsel, nlarge):
    """FUN_00ad0b88 with param_7 = 1: (sum of the nsel smallest, sum of the nlarge largest finite) as uint32."""
    c = np.asarray(costs, np.int64)
    n = c.size
    ninf = int(np.count_nonzero(c == INF))
    fin = c[c != INF]
    maxv = 0
    for x in fin:
        if maxv <= x:
            maxv = int(x)
    v1 = sel_value(c, nsel - 1, 0, maxv, 0)
    v2 = sel_value(c, nlarge + ninf - 1, 0, maxv, 1)
    lt = c < v1
    s = int(c[lt].sum()) + v1 * (nsel - int(np.count_nonzero(lt)))
    gt = (c > v2) & (c != INF)
    l_ = int(c[gt].sum()) + v2 * (nlarge - int(np.count_nonzero(gt)))
    return s & 0xffffffff, l_ & 0xffffffff


def _order_stat(srt, ninf, target):
    """sel_value for a whole matrix: ascending-sorted rows srt (npos, n), target per row (int64).
    target <= 0 or no finite element -> 0; target > nval -> the largest finite; else the target-th
    smallest (exactly what the FUN_009d4a2c bisection over [0, max finite] returns)."""
    npos, n = srt.shape
    nval = n - ninf
    tgt = np.clip(target, 0, nval)
    out = np.zeros(npos, np.int64)
    live = tgt > 0
    if live.any():
        out[live] = srt[np.nonzero(live)[0], tgt[live] - 1]
    return out


def best_direction_all(C, tab, nsel):
    """best_direction for every row of C at once; returns a list of (nsel, 2) offset arrays and raises
    NotImplementedError when a row has no qualifying bin (same as the per-row loop)."""
    C = np.asarray(C, np.int64)
    npos = C.shape[0]
    BIG = np.iinfo(np.int64).max
    score = np.full((npos, len(tab.bins)), BIG, np.int64)
    vsel = np.zeros((npos, len(tab.bins)), np.int64)
    for b, idx in enumerate(tab.bins):
        if len(idx) < nsel:
            continue
        vals = C[:, idx]
        v = np.sort(vals, 1)[:, nsel - 1]
        ninf = (vals == INF).sum(1)
        if (nsel > (vals.shape[1] - ninf) // 2).all():
            continue
        le = vals <= v[:, None]
        s = np.where(le, vals, 0).sum(1)
        sc = s - v * (le.sum(1) - nsel)
        ok = nsel <= (vals.shape[1] - ninf) // 2
        score[ok, b] = sc[ok]
        vsel[ok, b] = v[ok]
    bsel = np.argmin(score, 1)
    if (score[np.arange(npos), bsel] == BIG).any():
        nfail = int((score[np.arange(npos), bsel] == BIG).sum())
        raise NotImplementedError(f"{nfail} blocks without a qualifying direction bin (list left uninitialised)")
    L = tab.list1
    out = []
    for i in range(npos):
        idx = tab.bins[bsel[i]]
        vals = C[i, idx]
        v = vsel[i, bsel[i]]
        lst = [tuple(L[j]) for j, cst in zip(idx, vals) if cst < v]
        if len(lst) < nsel:
            for j, cst in zip(idx, vals):
                if len(lst) >= nsel:
                    break
                if cst == v:
                    lst.append(tuple(L[j]))
        out.append(np.array(lst, np.int64).reshape(-1, 2))
    return out


def ratio_sums_all(C, nsel, nlarge):
    """ratio_sums for every row of C at once (same uint32 results)."""
    c = np.asarray(C, np.int64)
    npos, n = c.shape
    ninf = (c == INF).sum(1)
    srt = np.sort(c, 1)
    v1 = _order_stat(srt, ninf, np.full(npos, nsel, np.int64))
    v2 = _order_stat(srt, ninf, n - nlarge - ninf)
    lt = c < v1[:, None]
    s = np.where(lt, c, 0).sum(1) + v1 * (nsel - lt.sum(1))
    gt = (c > v2[:, None]) & (c != INF)
    l_ = np.where(gt, c, 0).sum(1) + v2 * (nlarge - gt.sum(1))
    return s & 0xffffffff, l_ & 0xffffffff


def norm_image(W, H, c):
    """FUN_00acfc68 for configuration c: 1 / (sum over the blocks of window * fill) (exact dyadic sums)."""
    g = CFG[c]
    bs, step, pad = g["bs"], g["step"], g["pad"]
    tab = table(c)
    acc = np.zeros((H + 2 * pad, W + 2 * pad), F32)
    wf = (tab.win * g["fill"]).astype(F32)
    for y in positions(H, step):
        for x in positions(W, step):
            acc[y:y + bs, x:x + bs] += wf
    with np.errstate(divide="ignore"):
        return (F32(1.0) / acc[pad:pad + H, pad:pad + W]).astype(F32)


FAST = True


def _fma(a, b, c):
    return nm.fma_fast(a, b, c) if FAST else npx.fma32(a, b, c)


def _aggregate_waves(src, dst, win, org, lists, bs):
    """The raster-order FMLA accumulation of FUN_00ad15f0 regrouped into waves: wave(b) = 1 + the largest wave of
    the earlier blocks that overlap b, so overlapping blocks keep their raster order and the blocks of one wave
    are disjoint (each pixel receives the same FMAs in the same order as block by block)."""
    org = np.asarray(org, np.int64)
    nbk = len(org)
    wave = np.zeros(nbk, np.int64)
    for b in range(nbk):
        ov = (np.abs(org[:b, 0] - org[b, 0]) < bs) & (np.abs(org[:b, 1] - org[b, 1]) < bs)
        wave[b] = 1 + (wave[:b][ov].max() if ov.any() else 0)
    rr = np.arange(bs)
    for w in range(1, wave.max() + 1):
        idx = np.nonzero(wave == w)[0]
        nmax = max(len(lists[b]) for b in idx)
        Y = org[idx, 1][:, None, None] + rr[None, :, None]
        X = org[idx, 0][:, None, None] + rr[None, None, :]
        acc = dst[Y, X]
        for t in range(nmax):
            live = np.array([t < len(lists[b]) for b in idx])
            if not live.any():
                break
            d = np.array([lists[b][t] if t < len(lists[b]) else (0, 0) for b in idx], np.int64)
            blk = src[Y + d[:, 1][:, None, None], X + d[:, 0][:, None, None]]
            new = nm.fma_fast(np.broadcast_to(win, blk.shape), blk, acc)
            acc = np.where(live[:, None, None], new, acc)
        dst[Y, X] = acc


def ad1ae0(clean, A, c, info=None):
    g = CFG[c]
    bs, step, pad = g["bs"], g["step"], g["pad"]
    tab = table(c)
    H, W = A.shape
    # 1 - 2: direction selection on the u8 image of A
    U = pad_mirror(sp.image_u8(A), pad)
    xs, ys = positions(W, step), positions(H, step)
    grid = [(x - pad, y - pad) for y in ys for x in xs]
    C1 = sad_grid(U, grid, bs, g["r28"], pad, tab.list1) if FAST else None
    if C1 is not None and (C1 == INF).any():
        hit("bm_oob1")
    if FAST:
        lists = [np.concatenate([s, np.zeros((1, 2), np.int64)], 0) for s in best_direction_all(C1, tab, g["n8"] - 1)]
    else:
        lists = []
        nfail = 0
        for i, (x0, y0) in enumerate(grid):
            cst = costs_of(sad_block(U, x0, y0, bs, g["r28"], pad), tab.list1, g["r28"])
            sel, b = best_direction(cst, tab, g["n8"] - 1)
            if sel is None:
                nfail += 1
                sel = np.zeros((0, 2), np.int64)
            lst = np.concatenate([sel, np.zeros((1, 2), np.int64)], 0)
            lists.append(lst)
        if info is not None:
            info.setdefault("nfail", []).append(nfail)
        if nfail:
            raise NotImplementedError(f"{nfail} blocks without a qualifying direction bin (list left uninitialised)")
    # 3: aggregation
    src = pad_mirror(np.asarray(clean, F32), pad)
    dst = np.zeros_like(src)
    win = nm.fmul(F32(1.0), tab.win)
    if FAST:
        _aggregate_waves(src, dst, win, [(x, y) for y in ys for x in xs], lists, bs)
    else:
        i = 0
        with np.errstate(invalid="ignore", over="ignore"):
            for y in ys:
                for x in xs:
                    y0, x0 = y, x                     # padded coordinates of (x - pad, y - pad)
                    for dx, dy in lists[i]:
                        blk = src[y0 + dy:y0 + dy + bs, x0 + dx:x0 + dx + bs]
                        dst[y0:y0 + bs, x0:x0 + bs] = _fma(win, blk, dst[y0:y0 + bs, x0:x0 + bs])
                    i += 1
    Fimg = nm.fmul(dst[pad:pad + H, pad:pad + W], norm_image(W, H, c))
    # 4: ratios on the u8 image of the aggregate
    U2 = pad_mirror(sp.image_u8(Fimg), pad)
    nsel, nl = g["n10"] - 1, g["n18"]
    grid2 = [(min(8 * gx, W - step) - pad, min(8 * gy, H - step) - pad)
             for gy in range((H + 7) >> 3) for gx in range((W + 7) >> 3)]
    C2 = sad_grid(U2, grid2, bs, g["r30"], pad, tab.list2) if FAST else None
    if FAST:
        s, l_ = ratio_sums_all(C2, nsel, nl)
        v = ((l_.astype(F32) * F32(nsel)) / (s.astype(F32) + C_1E5) / F32(nl)).astype(F32)
    else:
        ratios = []
        for i, (x0, y0) in enumerate(grid2):
            cst = costs_of(sad_block(U2, x0, y0, bs, g["r30"], pad), tab.list2, g["r30"])
            s, l_ = ratio_sums(cst, nsel, nl)
            r = F32(F32(F32(l_) * F32(nsel)) / F32(F32(s) + C_1E5))
            ratios.append(F32(r / F32(nl)))
        v = np.array(ratios, F32)
    return nb.quantile(v, g["q"][0], 1, 1), nb.quantile(v, g["q"][1], 1, 1)


def ad1ae0_both(out, k, clean, A, info=None):
    for c, (o4, o5) in enumerate(((0xaa0, 0xac8), (0xab4, 0xadc))):
        a, b = ad1ae0(clean, A, c, info)
        out[(sp.P_OFF + o4 + 4 * k) // 4] = a
        out[(sp.P_OFF + o5 + 4 * k) // 4] = b
