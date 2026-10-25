"""Neat Video 6 (arm64 slice) line kernels and running box filters, ported to numpy.

FUN_00990448 (LineKernel): builds the table of digital lines of one orientation for a W x H
plane.  FUN_009944c0 (linefilt): running box filter of radius r along every line of the
kernel; the axis-aligned kernels (angle ~0 / ~pi/2) take the fast paths FUN_0099ba54
(rows) / FUN_0099bf40 (columns), which on this build dispatch to the NEON helpers
FUN_009eca14 / FUN_009ecbcc / FUN_009ece90 (rows) and FUN_009ec638 / FUN_009ec484 /
FUN_009ec86c (columns) because DAT_014a67a8 (SIMD flag) is 1.

Everything is float32 and the operation order of the genuine code is kept, so the results
are bit-identical to the plugin (verified with analysis/noise/verify_lines.py against
analysis/harness/nvh).  Only numpy is used; libm's tanf is called through ctypes because the
plugin calls the same tanf (numpy's float64 tan rounded to float32 differs for some angles).

Semantics (all verified against the harness):
  * line kernel, angle a (float32): |a| < 0.01 -> horizontal flag, |a - pi/2| < 0.01 ->
    vertical flag.  Otherwise f = a if a <= pi/2 else pi - a (float32); if (double)f <= pi/4
    ("shallow", +0x3828 = 1) the lines run over the columns j = 0..W-1 with row offset
    off[j] = int(rint(tanf(f) * j + 0.26)) (float32 ops, round-half-even), else ("steep")
    over the rows i = 0..H-1 with column offset off[i] = int(rint(tanf(pi/2 - f) * i + 0.26)).
    Lines are enumerated by base = -off[n-1] .. limit-1 (limit = H for shallow, W for steep);
    line L covers the entries k with 0 <= off[k] + base < limit (a contiguous range
    start..start+count-1 because off is monotone); the pixel of entry k is
    off_L + idx[k] with off_L = base*W (shallow) or base (steep) and
    idx[k] = k + W*off[k]            (shallow, a <= pi/2)
    idx[k] = (W-1-k) + W*off[k]      (shallow, a >  pi/2)
    idx[k] = k*W + off[k]            (steep,   a <= pi/2)
    idx[k] = (H-1-k)*W + off[k]      (steep,   a >  pi/2)
  * general running box (any oriented kernel), radius r, window [j-r, j+r] clipped to the
    line: running sum S (float32) with in-order add / subtract (S + in) - out;
    mode 1: out = recip * S with recip = 1/(2r+1) (zero padding);
    mode 0: out = S / float(window count) (truncated mean) except in the middle of lines
    longer than 2r (both ends inside) where out = recip * S.  Lines with count >= 2r+1 are
    taken two at a time when consecutive; in mode 0 the tail of the second line of such a
    pair is divided by r, r-1, .., 1 instead of 2r, .., r+1 (a register-reuse bug of the
    genuine code, see LineKernel.second_of_pair; the port keeps it).
  * axis-aligned fast paths: mode 1 zero padded, mode 0 border replicated, always recip * S;
    S starts as first*(r+1) + sum(x[0..r-1]) in mode 0; when n <= r+1 the whole lane is
    summed first and S = sum + (first*r + last*(r+1-n)); the row filter with n > 2r uses the
    NEON prefix-sum pattern of 4 (see _box_lanes) and r == 1 with W >= 8, W % 4 == 0 is the
    3-tap (x + prev) + next times 1/3.

The functions up to direction_filter are the reference.  The *_bm functions at the end (2026-10-16)
run the same arithmetic on B planes of one size stacked batch-innermost (H, W, B) for the batch
path of nv_pixnoise (see the comment block there); they are bit-identical per plane.
"""
import ctypes
import threading

import numpy as np

F32 = np.float32
_F0_01 = F32(0.01)
_PI = F32(3.1415927)
_HALF_PI = F32(1.5707964)
_QUARTER_PI_D = 0.7853981633974483      # DAT_012436d0 (double)
_C026 = F32(0.26)
_THIRD = np.array([0x3eaaaaab], np.uint32).view(F32)[0]   # 1/3 of FUN_009ece90

SIMD = True   # DAT_014a67a8 of the analysed build (nvh info); False = scalar fast paths

# the 16 orientations of FUN_00998d98 (table at 0x12440a8) and the second-pass tables
# DAT_01244168 (index of the second kernel) / DAT_012441e8 (its radius), int64 each
ANGLES = np.array([0x0, 0x3e7adbb0, 0x3eed6338, 0x3f24bc7d, 0x3f490fdb, 0x3f6d6338, 0x3f8db70d,
                   0x3fa9b465, 0x3fc90fdb, 0x3fe86b51, 0x40023454, 0x400db70d, 0x4016cbe4,
                   0x401fe0bb, 0x402b6374, 0x40396220], np.uint32).view(F32)
IDX2 = (0, 0, 0, 4, 4, 4, 8, 8, 8, 8, 8, 12, 12, 12, 0, 0)
RAD2 = (2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 2)

try:
    _libm = ctypes.CDLL("libSystem.B.dylib")
    _libm.tanf.restype = ctypes.c_float
    _libm.tanf.argtypes = [ctypes.c_float]
except OSError:                      # not macOS: fall back to numpy's float32 tan
    _libm = None


def tanf(x):
    """float32 tanf of the C library (what the plugin calls)."""
    x = F32(x)
    if _libm is not None:
        return F32(_libm.tanf(ctypes.c_float(float(x))))
    return F32(np.tan(x))


MAX_SIDE = 512   # the genuine tables: 512 offsets (stack array local_1058[512]), 512 idx entries
#                 (+8 .. +0x808) and 1024 line triples (+0x808 .. +0x3808) of the 0x4000-byte kernel


class LineKernel:
    """FUN_00990448(buf, W, H, angle): tables of the digital lines of one orientation.

    Attributes: W, H, angle (float32), horiz / vert (the axis-aligned flags at +0x3824 /
    +0x3825), flag (+0x3828: 1 = shallow, 0 = steep), n (entries of idx), idx (int32[n]),
    lines (int32[nlines, 3] = off, start, count), nlines, off (the offsets per entry).

    W and H must be 1..512 (ValueError otherwise): the genuine builds its tables in fixed arrays
    of 512 offsets / 512 indices / 1024 lines and overruns them for larger sides (FUN_00998d98
    only builds kernels for its <= 120 px tiles; `nvh kernel` / `linefilt` refuse sides > 512,
    so nothing larger can be checked).  The axis-aligned kernels return before any table is
    built, but the limit is applied to every angle because FUN_00998d98 builds the 16 kernels
    of a level for the same W, H.

    DAT_01561d58: in the shallow branch the genuine stores count << (DAT_01561d58 & 63).  That
    global lies in __DATA,__common (zero-fill: no value in the file) and is only written at run
    time by obfuscated integrity code (0x45114: popcount(...) - 16, i.e. 0 when the check
    passes; 0x9af244 / 0x9ebbbc: 1 after a failed check).  The port assumes 0: `nvh info` does
    not print it, but every `nvh kernel` dump (shallow and steep, all sizes of verify_lines.py)
    matched the unshifted counts."""

    def __init__(self, W, H, angle):
        self.W, self.H = int(W), int(H)
        if not (1 <= self.W <= MAX_SIDE and 1 <= self.H <= MAX_SIDE):
            raise ValueError(f"LineKernel {self.W}x{self.H}: the genuine tables hold sides of 1..{MAX_SIDE} px")
        a = F32(angle)
        self.angle = a
        self.horiz = bool(abs(a) < _F0_01)
        self.vert = False
        self.flag = 0
        self.n = 0
        self.off = np.zeros(0, np.int64)
        self.idx = np.zeros(0, np.int32)
        self.lines = np.zeros((0, 3), np.int32)
        self._gather = None
        if self.horiz:
            return
        self.vert = bool(abs(F32(a + F32(-1.5707964))) < _F0_01)
        if self.vert:
            return
        folded = F32(_PI - a) if a > _HALF_PI else a
        if float(folded) <= _QUARTER_PI_D:          # fcvt d1, s0; fcmp with DAT_012436d0
            self.flag = 1
            n, limit, stride = self.W, self.H, self.W
            t = tanf(folded)
        else:
            self.flag = 0
            n, limit, stride = self.H, self.W, 1
            t = tanf(F32(_HALF_PI - folded))
        k = np.arange(n, dtype=np.int64)
        off = np.rint(t * k.astype(F32) + _C026).astype(np.int64)   # fmul, fadd, frintn, fcvtzs
        if np.any(np.diff(off) < 0):    # the genuine's scans assume this (angles in [0, pi))
            raise ValueError(f"angle {float(a)} gives non-monotone line offsets")
        maxoff = int(off[n - 1])
        base = np.arange(-maxoff, limit, dtype=np.int64)
        start = np.searchsorted(off, -base, "left")                  # first k with off[k] + base >= 0
        end = np.searchsorted(off, limit - base, "left") - 1         # last k with off[k] + base < limit
        self.n = n
        self.off = off
        self.lines = np.stack([base * stride, start, end - start + 1], 1).astype(np.int32)
        if self.flag:
            col = k if a <= _HALF_PI else (self.W - 1 - k)
            idx = col + self.W * off
        else:
            row = k if a <= _HALF_PI else (self.H - 1 - k)
            idx = row * self.W + off
        self.idx = idx.astype(np.int32)

    @property
    def nlines(self):
        return int(self.lines.shape[0])

    def gather_tables(self):
        """(P, valid, count): P[L, j] = pixel index of entry j of line L (0 where invalid)."""
        if self._gather is None:
            off, start, count = (self.lines[:, c].astype(np.int64) for c in range(3))
            m = int(count.max()) if count.size else 0
            jj = np.arange(m, dtype=np.int64)
            valid = jj[None, :] < count[:, None]
            k = np.minimum(start[:, None] + jj[None, :], max(self.n - 1, 0))
            P = np.where(valid, off[:, None] + self.idx.astype(np.int64)[k], 0)
            self._gather = (P, valid, count)
        return self._gather

    def second_of_pair(self, r):
        """Lines FUN_009944c0 processes as the second of a pair: a line with count >= 2r+1
        is paired with the next one when that also has count >= 2r+1 (then both are
        skipped); in mode 0 the tail of the second line is divided by r, r-1, .., 1 instead
        of 2r, .., r+1 (register reuse in the genuine code, verified with the harness)."""
        count = self.lines[:, 2]
        sec = np.zeros(count.shape[0], bool)
        L, n, lim = 0, count.shape[0], 2 * r + 1
        while L < n:
            if count[L] >= lim and L + 1 < n and count[L + 1] >= lim:
                sec[L + 1] = True
                L += 2
            else:
                L += 1
        return sec


def _box_lanes(X, r, mode, prefix4):
    """Running box of radius r along axis 0 of X (n, lanes) as the fast paths compute it.

    prefix4 = True reproduces the NEON row filter (FUN_009eca14 / FUN_009ecbcc): the middle
    section (both window ends inside) is processed in groups of 4 after (M mod 4) scalar
    steps, each group adding the prefix sums p0 = d0, p1 = d0+d1, p2 = d2+p1,
    p3 = p1+(d2+d3) of the differences d = in - out to the running sum."""
    n = X.shape[0]
    recip = F32(1) / F32(2 * r + 1)
    out = np.empty_like(X)
    if mode == 0:
        first, last = X[0], X[n - 1]
    S = np.zeros(X.shape[1:], F32)
    if n <= r + 1:
        for k in range(n):
            S = S + X[k]
        if mode == 0:
            S = S + (first * F32(r) + last * F32(r + 1 - n))
            for j in range(n):
                out[j] = recip * S
                S = (last + S) - first
        else:
            out[:] = recip * S
        return out
    for k in range(r):
        S = S + X[k]
    if mode == 0:
        S = first * F32(r + 1) + S
    for j in range(min(r, n - r - 1) + 1):                 # growing: right end inside
        S = S + X[j + r]
        if mode == 0:
            S = S - first
        out[j] = recip * S
    if n <= 2 * r:
        for j in range(n - r, r + 1):                      # both ends outside
            if mode == 0:
                S = (last + S) - first
            out[j] = recip * S
    else:
        j = r + 1
        M = n - 2 * r - 1
        if prefix4:
            for _ in range(M % 4):
                S = (S + X[j + r]) - X[j - r - 1]
                out[j] = recip * S
                j += 1
            while j <= n - r - 1:
                d0 = X[j + r] - X[j - r - 1]
                d1 = X[j + r + 1] - X[j - r]
                d2 = X[j + r + 2] - X[j - r + 1]
                d3 = X[j + r + 3] - X[j - r + 2]
                p1 = d0 + d1
                p2 = d2 + p1
                p3 = p1 + (d2 + d3)
                out[j] = recip * (S + d0)
                out[j + 1] = recip * (S + p1)
                out[j + 2] = recip * (S + p2)
                out[j + 3] = recip * (S + p3)
                S = S + p3
                j += 4
        else:
            for j in range(r + 1, n - r):
                S = (S + X[j + r]) - X[j - r - 1]
                out[j] = recip * S
    for j in range(max(n - r, r + 1), n):                  # left end inside only
        if mode == 0:
            S = (last + S) - X[j - r - 1]
        else:
            S = S - X[j - r - 1]
        out[j] = recip * S
    return out


def _box3_rows(X, mode):
    """FUN_009ece90: r = 1 rows with W >= 8 and W % 4 == 0: ((x + prev) + next) * (1/3)."""
    if mode == 0:
        prev = np.concatenate([X[..., :1], X[..., :-1]], -1)
        nxt = np.concatenate([X[..., 1:], X[..., -1:]], -1)
    else:
        z = np.zeros_like(X[..., :1])
        prev = np.concatenate([z, X[..., :-1]], -1)
        nxt = np.concatenate([X[..., 1:], z], -1)
    return ((X + prev) + nxt) * _THIRD


def box_rows(X, r, mode, simd=None):
    """FUN_0099ba54: horizontal running box (kernel with the +0x3824 flag) on (..., H, W)."""
    X = np.ascontiguousarray(X, F32)
    simd = SIMD if simd is None else simd
    W = X.shape[-1]
    if simd and W > 2 * r:
        if r == 1 and W >= 8 and W % 4 == 0:
            return _box3_rows(X, mode)
        prefix4 = True
    else:
        prefix4 = False
    lanes = np.moveaxis(X, -1, 0)                         # (W, ..., H)
    return np.moveaxis(_box_lanes(lanes, r, mode, prefix4), 0, -1)


def box_cols(X, r, mode, simd=None):
    """FUN_0099bf40: vertical running box (kernel with the +0x3825 flag) on (..., H, W)."""
    X = np.ascontiguousarray(X, F32)
    lanes = np.moveaxis(X, -2, 0)                         # (H, ..., W)
    return np.moveaxis(_box_lanes(lanes, r, mode, False), 0, -2)


def _general(kernel, X, r, mode):
    """FUN_009944c0 general path: running box along every line of the kernel."""
    P, valid, count = kernel.gather_tables()
    L, m = P.shape
    flat = X.reshape(X.shape[:-2] + (-1,))
    # entry-major layout (m, ..., L): every step of the running sum touches one contiguous slice
    A = np.ascontiguousarray(np.moveaxis(np.where(valid, flat[..., P], F32(0)), -1, 0))
    out = np.zeros_like(A)
    recip = F32(1) / F32(2 * r + 1)
    S = np.zeros(A.shape[1:], F32)
    cnt = count.astype(np.int64)
    if not mode & 1:
        quirk = np.where(kernel.second_of_pair(r), r, 0)   # tail divisor of a pair's 2nd line is r less
    for j2 in range(m + r):                                # j2 = j + r: index of the entering entry
        if j2 < m:
            S = np.where(j2 < cnt, S + A[j2], S)
        jo = j2 - 2 * r - 1                                # index of the leaving entry
        if jo >= 0:
            S = S - A[jo]
        j = j2 - r
        if j < 0:
            continue
        if mode & 1:
            out[j] = recip * S
        else:
            nw = np.minimum(j + r, cnt - 1) - max(j - r, 0) + 1
            tail = (j >= cnt - r) & (cnt >= 2 * r + 1)
            nw = np.where(tail, nw - quirk, nw)
            mid = (j >= r + 1) & (j + r <= cnt - 1)
            div = np.maximum(nw, 1).astype(F32)
            out[j] = np.where(mid, recip * S, S / div)
    dst = np.zeros_like(flat)
    dst[..., P[valid]] = np.moveaxis(out, 0, -1)[..., valid]
    return dst.reshape(X.shape)


def linefilt(kernel, plane, r, mode):
    """FUN_009944c0(kernel, src, dst, r, mode) on plane (H, W) or a batch (..., H, W)."""
    X = np.ascontiguousarray(plane, F32)
    if X.shape[-2] != kernel.H or X.shape[-1] != kernel.W:
        raise ValueError(f"plane {X.shape[-2:]} does not match the kernel {kernel.H}x{kernel.W}")
    r = int(r)
    if r == 0:
        return X.copy()
    if kernel.horiz:
        return box_rows(X, r, mode)
    if kernel.vert:
        return box_cols(X, r, mode)
    return _general(kernel, X, r, mode)


def build_kernels(W, H, angles=ANGLES):
    """One LineKernel per angle (FUN_00998d98 builds them for every level's W, H)."""
    return [LineKernel(W, H, a) for a in angles]


def direction_filter(kernels, i, plane, idx2=IDX2, rad2=RAD2, r1=4, mode=1):
    """Oriented filter of direction i as FUN_00998d98 applies it: radius r1 along kernel i,
    then radius rad2[i] along kernel idx2[i] (skipped when idx2[i] < 0)."""
    f = linefilt(kernels[i], plane, r1, mode)
    if idx2[i] >= 0:
        f = linefilt(kernels[idx2[i]], f, rad2[i], mode)
    return f


# ----------------------------------------------------------------------------------------------
# Batch-minor fast paths (2026-10-16).  The functions above are the reference implementation;
# the ones below compute the same float32 operations in the same order on B planes of one size
# stacked batch-innermost as (H, W, B), so that every step of a running sum is one numpy call over
# all B planes (a slab of contiguous B-vectors) instead of one call per plane.  They are
# bit-identical to the reference per plane (checked in analysis/noise/verify_lines.py against the
# reference and against `nvh linefilt`).  Differences in form, not in arithmetic:
#   * the running sums of the middle section are kept per step and multiplied by recip in one call
#     afterwards (recip * S is the same product either way);
#   * the NEON row filter's groups of 4 compute the differences and prefix sums of all groups at
#     once; only the chain S += p3 stays sequential;
#   * the general (oriented) path gathers the lines with a padding value of -0.0 instead of masking:
#     S + (-0.0) == S bit for bit (also for S = -0.0 and NaN), so the entries past the end of a
#     line leave S unchanged exactly like the reference's np.where(j2 < cnt, S + A[j2], S); a padded
#     entry is only ever subtracted after the last valid output of its line (jo = j - r - 1 < cnt for
#     every output j < cnt), so those subtractions never reach the result.
# ----------------------------------------------------------------------------------------------
class LinePlan:
    """Gather / scatter tables of one LineKernel for the batch-minor general path.

    G (m, L) intp: pixel index of entry j of line l (H*W = the padding row for j >= count);
    inv (H*W,) intp: position j*L + l of every pixel in the (m, L) table (each pixel lies on
    exactly one line of the kernel, asserted); inv_pad: inv followed by m*L (the padding row of the
    running sums, so a gathered result keeps a -0.0 padding row)."""

    def __init__(self, kernel):
        self.kernel = kernel
        self.horiz, self.vert = kernel.horiz, kernel.vert
        if self.horiz or self.vert:
            self.G = self.inv = None
            return
        HW = kernel.W * kernel.H
        P, valid, count = kernel.gather_tables()
        L, m = P.shape
        self.G = np.ascontiguousarray(np.where(valid, P, HW).T).astype(np.intp)
        ls, js = np.nonzero(valid)
        inv = np.full(HW, -1, np.intp)
        inv[P[ls, js]] = js * L + ls
        if (inv < 0).any() or int(valid.sum()) != HW:
            raise AssertionError(f"kernel {kernel.W}x{kernel.H} angle {float(kernel.angle)}: lines do not "
                                 "partition the plane")
        self.inv = inv
        self.inv_pad = np.append(inv, m * L).astype(np.intp)   # + the padding row of the sums


_PLANS = {}
_PLAN_CACHE_MAX = 4096


def line_plans(W, H, angles=ANGLES):
    """LinePlan for every angle at (W, H), cached (FUN_00998d98 builds the same 16 kernels for every
    tile of a size).  Callers in several threads at most build a plan twice (plans are never modified,
    the dict assignment is atomic); the scratch buffers of _work are per thread."""
    out = []
    for a in angles:
        key = (int(W), int(H), int(np.asarray(a, F32).view(np.uint32)))
        p = _PLANS.get(key)
        if p is None:
            if len(_PLANS) >= _PLAN_CACHE_MAX:
                _PLANS.clear()
            p = _PLANS[key] = LinePlan(LineKernel(W, H, a))
        out.append(p)
    return out


_TLS = threading.local()


def _work(name, shape):
    """Per-thread float32 scratch array of the given shape, reused across calls (a fresh multi-MB
    array costs page faults on every call).  Only for temporaries that never leave the function that
    asks for them; one name = one live temporary."""
    bufs = getattr(_TLS, "bufs", None)
    if bufs is None:
        bufs = _TLS.bufs = {}
    n = 1
    for v in shape:
        n *= int(v)
    a = bufs.get(name)
    if a is None or a.size < n:
        a = bufs[name] = np.empty(max(n, 1), F32)
    return a[:n].reshape(shape)


def _lanes_run(X, r, mode, prefix4, out):
    """_box_lanes on X (n, *lanes) into out (n, *lanes) (views allowed; out must not overlap X),
    same operations and order.  Every operation keeps the operand order of the reference (array op
    array stays array op array, with the same first operand), so NaN payloads propagate identically."""
    n = X.shape[0]
    lane = X.shape[1:]
    recip = F32(1) / F32(2 * r + 1)
    S = np.zeros(lane, F32)
    if mode == 0:
        first, last = X[0], X[n - 1]
    if n <= r + 1:
        for k in range(n):
            np.add(S, X[k], out=S)
        if mode == 0:
            t = first * F32(r)
            np.add(t, last * F32(r + 1 - n), out=t)
            np.add(S, t, out=S)
            for j in range(n):
                np.multiply(recip, S, out=out[j])
                np.add(last, S, out=S)
                np.subtract(S, first, out=S)
        else:
            np.multiply(recip, S, out=out[0])
            out[1:] = out[0]
        return out
    for k in range(r):
        np.add(S, X[k], out=S)
    if mode == 0:
        np.add(first * F32(r + 1), S, out=S)
    for j in range(min(r, n - r - 1) + 1):                 # growing: right end inside
        np.add(S, X[j + r], out=S)
        if mode == 0:
            np.subtract(S, first, out=S)
        np.multiply(recip, S, out=out[j])
    if n <= 2 * r:
        for j in range(n - r, r + 1):                      # both ends outside
            if mode == 0:
                np.add(last, S, out=S)
                np.subtract(S, first, out=S)
            np.multiply(recip, S, out=out[j])
    else:
        j = r + 1
        M = n - 2 * r - 1
        if prefix4:
            for _ in range(M % 4):
                np.add(S, X[j + r], out=S)
                np.subtract(S, X[j - r - 1], out=S)
                np.multiply(recip, S, out=out[j])
                j += 1
            G = (n - r - j) // 4
            if G:
                D = np.subtract(X[j + r:j + r + 4 * G], X[j - r - 1:j - r - 1 + 4 * G],
                                out=_work("lanes_D", (4 * G,) + lane)).reshape((G, 4) + lane)
                d0, d1, d2, d3 = D[:, 0], D[:, 1], D[:, 2], D[:, 3]
                p1 = np.add(d0, d1, out=_work("lanes_p1", (G,) + lane))
                p2 = np.add(d2, p1, out=_work("lanes_p2", (G,) + lane))
                t = np.add(d2, d3, out=_work("lanes_t", (G,) + lane))
                p3 = np.add(p1, t, out=_work("lanes_p3", (G,) + lane))
                Sg = _work("lanes_S", (G,) + lane)        # S at the start of every group
                Sg[0] = S
                for g in range(G - 1):
                    np.add(Sg[g], p3[g], out=Sg[g + 1])
                S = Sg[G - 1] + p3[G - 1]
                for k, q in enumerate((d0, p1, p2, p3)):
                    np.add(Sg, q, out=t)
                    np.multiply(recip, t, out=out[j + k:j + 4 * G:4])
                j += 4 * G
        elif M:
            Sb = _work("lanes_S", (M,) + lane)
            prev = S
            for i in range(M):
                jj = r + 1 + i
                np.add(prev, X[jj + r], out=Sb[i])
                np.subtract(Sb[i], X[jj - r - 1], out=Sb[i])
                prev = Sb[i]
            np.multiply(recip, Sb, out=out[r + 1:n - r])
            S = Sb[M - 1].copy()
    for j in range(max(n - r, r + 1), n):                  # left end inside only
        if mode == 0:
            np.add(last, S, out=S)
            np.subtract(S, X[j - r - 1], out=S)
        else:
            np.subtract(S, X[j - r - 1], out=S)
        np.multiply(recip, S, out=out[j])
    return out


def _out_like(X, out):
    return np.empty(X.shape, F32) if out is None else out


def _box3_rows_bm(X, mode, out=None):
    """_box3_rows on (H, W, B): ((x + prev) + next) * (1/3) along axis 1, written with slices
    (border: the edge pixel itself in mode 0, +0.0 in mode 1, as the reference's padding)."""
    out = _out_like(X, out)
    np.add(X[:, 1:], X[:, :-1], out=out[:, 1:])                           # x + prev
    np.add(X[:, 0], X[:, 0] if mode == 0 else F32(0.0), out=out[:, 0])
    np.add(out[:, :-1], X[:, 1:], out=out[:, :-1])                        # (x + prev) + next
    np.add(out[:, -1], X[:, -1] if mode == 0 else F32(0.0), out=out[:, -1])
    np.multiply(out, _THIRD, out=out)
    return out


def box_rows_bm(X, r, mode, simd=None, out=None):
    """box_rows on batch-minor planes (H, W, B) (running box along axis 1); out (H, W, B) optional
    (must not overlap X)."""
    X = np.ascontiguousarray(X, F32)
    simd = SIMD if simd is None else simd
    W = X.shape[1]
    if simd and W > 2 * r:
        if r == 1 and W >= 8 and W % 4 == 0:
            return _box3_rows_bm(X, mode, out)
        prefix4 = True
    else:
        prefix4 = False
    out = _out_like(X, out)
    _lanes_run(X.transpose(1, 0, 2), r, mode, prefix4, out.transpose(1, 0, 2))
    return out


def box_cols_bm(X, r, mode, simd=None, out=None):
    """box_cols on batch-minor planes (H, W, B) (running box along axis 0); out optional (must not
    overlap X)."""
    X = np.ascontiguousarray(X, F32)
    out = _out_like(X, out)
    _lanes_run(X, r, mode, False, out)
    return out


_NEG0 = np.uint32(0x80000000)


def padded_empty(H, W, B):
    """(buffer (H*W + 1, B), view (H, W, B)): planes followed by one row of -0.0, the padding the
    general path gathers for the entries past the end of a line.  A plane created as such a view
    is used by _general_bm without the copy into a padded buffer."""
    buf = np.empty((H * W + 1, B), F32)
    buf[H * W] = F32(-0.0)
    return buf, buf[:H * W].reshape(H, W, B)


def _padded_base(X):
    """The padded buffer of padded_empty() that X (H, W, B) views, if its padding row is intact."""
    base = X.base
    if (not isinstance(base, np.ndarray) or base.ndim != 2 or base.dtype != F32 or not X.flags.c_contiguous
            or base.shape != (X.shape[0] * X.shape[1] + 1, X.shape[2]) or not base.flags.c_contiguous
            or base.ctypes.data != X.ctypes.data):
        return None
    return base if np.all(base[-1].view(np.uint32) == _NEG0) else None


def _general_bm(plan, X, r, out=None):
    """FUN_009944c0 general path, mode 1, on (H, W, B): gather the lines (padding -0.0), run the
    sums along the entries, multiply by recip and scatter back with plan.inv.  Without out the
    result is a view of a new padded buffer (padded_empty); with out (H, W, B) the result is written
    there (out may be X or a padded view, whose padding row then stays -0.0)."""
    H, W, B = X.shape
    HW = H * W
    Xp = _padded_base(X)
    if Xp is None:
        Xp = _work("gen_X", (HW + 1, B))
        Xp[:HW] = X.reshape(HW, B)
        Xp[HW] = F32(-0.0)
    m, L = plan.G.shape
    A = np.take(Xp, plan.G, axis=0, out=_work("gen_A", (m, L, B)), mode="clip")    # (m, L, B)
    recip = F32(1) / F32(2 * r + 1)
    Sb = _work("gen_S", (m * L + 1, B))                   # running sums per entry + a -0.0 row
    Sb[m * L] = F32(-0.0)
    S3 = Sb[:m * L].reshape(m, L, B)
    S = np.zeros((L, B), F32)
    for j2 in range(min(r, m)):
        np.add(S, A[j2], out=S)
    prev = S
    for j in range(m):
        j2 = j + r
        cur = S3[j]
        if j2 < m:
            np.add(prev, A[j2], out=cur)
        else:
            cur[...] = prev
        if j2 - 2 * r - 1 >= 0:
            np.subtract(cur, A[j2 - 2 * r - 1], out=cur)
        prev = cur
    if out is None:
        _, out = padded_empty(H, W, B)
    ob = _padded_base(out)
    if ob is not None:                                    # (HW + 1, B): the padding row comes along
        np.take(Sb, plan.inv_pad, axis=0, out=ob, mode="clip")
        np.multiply(recip, ob, out=ob)                    # recip * -0.0 = -0.0 keeps the padding
    else:
        o2 = out.reshape(HW, B)
        if not (o2.flags.c_contiguous and np.shares_memory(o2, out)):
            raise ValueError("out must be a C-contiguous (H, W, B) array")
        np.take(Sb, plan.inv, axis=0, out=o2, mode="clip")
        np.multiply(recip, o2, out=o2)
    return out


def linefilt_bm(plan, X, r, mode, out=None):
    """linefilt on batch-minor planes (H, W, B) with a LinePlan (bit-identical per plane).  out:
    optional (H, W, B) destination (must not overlap X for the axis-aligned kernels)."""
    X = np.ascontiguousarray(X, F32)
    k = plan.kernel
    if X.shape[0] != k.H or X.shape[1] != k.W:
        raise ValueError(f"planes {X.shape[:2]} do not match the kernel {k.H}x{k.W}")
    r = int(r)
    if r == 0:
        if out is None:
            return X.copy()
        out[...] = X
        return out
    if k.horiz:
        return box_rows_bm(X, r, mode, out=out)
    if k.vert:
        return box_cols_bm(X, r, mode, out=out)
    if mode & 1:
        return _general_bm(plan, X, r, out)
    # mode 0 of an oriented kernel (truncated mean; FUN_00998d98 never uses it): reference path
    res = np.ascontiguousarray(np.moveaxis(_general(k, np.moveaxis(X, -1, 0), r, mode), 0, -1))
    if out is None:
        return res
    out[...] = res
    return out


def direction_filter_bm(plans, i, X, idx2=IDX2, rad2=RAD2, r1=4, mode=1, out=None, tmp=None):
    """direction_filter on batch-minor planes (H, W, B) with the LinePlans of their size.  out /
    tmp: optional destinations of the second / first pass (tmp best a padded_empty view, which a
    general second pass reads without a copy)."""
    if idx2[i] < 0:
        return linefilt_bm(plans[i], X, r1, mode, out)
    f = linefilt_bm(plans[i], X, r1, mode, tmp)
    return linefilt_bm(plans[idx2[i]], f, rad2[i], mode, out)
