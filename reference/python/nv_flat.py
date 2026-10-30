"""Neat Video 6 (arm64 slice) FindFlatArea (FUN_009b1a5c) on the Auto Profile path (param_3 = 0),
ported bit-exactly to numpy (decision for decision: same candidates, scores and status).

    find_flat_area_genuine(rgb, noise_at, mode=1, cancel=None, curve=None) -> (x, y, w, h, status)

rgb: (H, W, 3) float32 planes on the genuine's scale (0..255), noise_at(b): the noise level at
brightness b as the genuine reads it from EstimateNoiseLevelCurve (FUN_00aedb18 on the double b;
production: nv_curve.noise_at(nv_curve.genuine_noise_curve(rgb)), which is bit-exact).
mode: the profile colour mode (profile +0x48): 0 RGB, 1 BT.601 Y / Cr / Cb, 2 mean / differences,
3 RGB (the transform table's fourth entry is the copy as well).

The routine (disassembly 0x9b1a5c..0x9b27a0):
  1. s = 8, 9, ... until W/s <= 4 or H/s <= 4 or (W/s)(H/s) <= 0x40000; size table by W*H
     (<= 0x280000: 0x1246d30, <= 0xa00000: 0x1246da8, else 0x1246e20); the first size that fits
     W and H starts the search (none: status 1).
  2. EstimateNoiseLevelCurve (here: noise_at, or curve() at this point). It fails only when the cancel
     query (vtable slot 0 of the filter core) reports a cancel: its worker FUN_009b76b0 asks before each
     tile and once more when the scheduler is empty, and returns false -> status 5, outputs not written
     (the port returns zeros, the harness's initial values).  Observed with `nvh flat` and
     NVH_CANCEL_AT=k (the host-side callback reports a cancel from its k-th query on): 1920x1080
     (240 tiles), 640x360 (28), 200x150 (6), 64x64 (1).
  3. FUN_009b7e58: s x s block means (sequential float32 sums row by row, times 1/(s*s)) of R, G, B;
     FUN_0097bb88: their BT.601 luma (nv_pixnoise.luma_601).
  4. For every size from the first that fits: r = max((long)((size/s - 1) * 0.5), 1),
     step = max(16 / s, 1); FUN_009999cc gives up to 64 candidates (cx, cy, mean, score).
     Each size starts with a cancel query; a cancel returns status 5 with the outputs as written so
     far (the best square of the earlier sizes, before the cut of step 5; zeros if none yet).
     For each candidate (in order, while score >= 0):
       x0 = max(cx*s - size/2, 0), moved to W - size when x0 + size > W (likewise y), the square is
       cut (FUN_009c7318: a copy); IsClip of its R, G, B (nv_curve.is_clip_genuine); >= 2 planes
       with a fraction > 0.33 skip it; the colour transform DAT_0147df50[mode] in place; the band
       measure FUN_009980e8 of the three planes (nv_band); flatness FUN_009b096c per plane;
       nz = (float)noise_at((double)mean), nz < 1e-6 skips it;
       L_k = (|m1_k| + |m2_k|) + |m3_k| over the levels until one mean is negative (n levels),
       E = ((((L_0 + 0) + (L_1 + L_1)) + L_2*4) + L_3*8) + L_4*16, then E / n,
       R = ((((L_0/L_1) + 0) + L_1/L_2) + L_2/L_3) + L_3/L_4, then R / (n - 1) (R = 0 for n = 1),
       score = (2*fmaxnm(2 - R, 0) + 1) * (factor * ((4*nclip + 1) * (((((fl1*fl2)*fl3) * (E/nz))
               * (|b - 96| * 1/512 + 1)) * (penalty + 1)))),  penalty = any IsClip > 0.1 (double)
       strictly smaller scores replace the best (flags flat = all fl < 2, clipped = nclip != 0).
  5. Sides over 512 are cut to 512 around the centre; status 0 ok, 2 small (< 128), 3 not flat,
     4 clipped, 1 nothing found, 5 cancelled (step 2 or 4).

The candidate squares of one size are measured in batches of at most BATCH_FLOATS values
(nv_band.band_levels_bm on their planes stacked batch-innermost); all batches of a call share one
nv_band.Work, so the memory stays at a few arrays of one batch whatever the frame size.  The
selection then runs in the genuine's order.  Only numpy is used (plus nv_band, nv_lines, nv_pixnoise,
nv_curve of this directory, read only).

Frames where no square fits (W or H < 48) end in the genuine with FUN_00a22868(NULL, 100) at 0x9b1c40,
which dereferences the NULL progress node (SIGSEGV in the harness); the port returns (0, 0, 0, 0, 1)
there.  The result does not depend on the genuine's thread count (the three band measures and the
IsClip planes run as independent tasks) except through EstimateNoiseLevelCurve.
An obfuscated fragment (0x9b1dfc..0x9b1e7c) updates a global counter (0x1488348) on every call and,
when the counter's two top bits are 0, adds a value derived from a token table to the reduced height
it then hands to FUN_009999cc.  The port follows the path without that addition: the first call of a
process skips it (the file's initial value is 0x5a270a93), which is why the genuine is compared with
one FindFlatArea call per nvh process; that a valid token table makes the addition 0 is an assumption.

Verification (2026-10-16, after the review fixes; analysis/noise/verify_flat_exact.py against
`nvh flat`, one FindFlatArea call per process): region and status identical in every case the genuine
completes - test_v02.mp4 every 24th frame (93/93) and test.mp4 every 30th frame (93/93) in colour
modes 0 / 1 / 2, 1440p / 4K (table 0x1246da8) and 4480x2520 / 8K (table 0x1246e20) frames built from
real frames (15/15), crops of 48..300 px and synthetic frames with letterbox / pillarbox / clipped
channels / constant / quantised / NaN / inf content (81/81, statuses 0 to 4 all occur); the reduced
planes, luma and candidates of every size of these frames (17579 candidates) are bit-identical to
`nvh reduce` / `nvh cand`; the production entry nvfilter.find_flat_area agrees on test_v02 every 48th
and test.mp4 every 60th frame (32/32).  The review's own sets (FUN_009999cc with radius / step 0,
reductions, transforms, flatness, ties, unseen frames, table boundaries, random frames) agree too.
"""
import os
import sys

import numpy as np

_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)
import nv_band as nb  # noqa: E402
import nv_curve  # noqa: E402
import nv_lines as nl  # noqa: E402
import nv_pixnoise as npx  # noqa: E402

F32 = np.float32
F64 = np.float64
U32 = np.uint32

FLT_MAX = np.finfo(F32).max
# FindFlatArea size tables (0x1246d30 / 0x1246da8 / 0x1246e20, `nvh info`): (max W*H, sizes, factors)
TABLES = ((0x280000, (320, 192, 128, 96, 64, 48), (1.0, 1.15, 1.33, 2.0, 4.0, 6.0)),
          (0xA00000, (512, 320, 192, 128, 96, 64, 48), (1.0, 1.15, 1.25, 2.0, 4.0, 6.0, 8.0)),
          (None, (512, 320, 192, 128, 96, 64, 48), (1.15, 1.33, 2.0, 4.0, 6.0, 8.0, 10.0)))
# DAT_0147dfc8: the "luma" flag of FUN_009b096c for the three planes of each colour mode
LUMA_FLAGS = {0: (1, 1, 1), 1: (1, 0, 0), 2: (1, 0, 0), 3: (0, 0, 0)}
CLIP_THR = F32(0.33)                      # 0x3ea8f5c3
CLIP_PENALTY = 0.1                        # DAT_0127a7a0 (double)
MASKED = F32(10000.0)                     # 0x461c4000 (DAT_012436f0 pattern, initial best)
_CR = np.array([0x3f3691fc], U32).view(F32)[0]     # 0.71316504 (FUN_0097b5d0)
_CB = np.array([0x3f108d15], U32).view(F32)[0]     # 0.56465274
_FL_C = (F32(0.6), np.array([0x3e99999a], U32).view(F32)[0])        # FUN_009b096c: c (chroma, luma)
_FL_LIM = (np.array([0x402ccccc], U32).view(F32)[0], np.array([0x3fe66666], U32).view(F32)[0])
_FL_EPS = np.array([0x3727c5ac], U32).view(F32)[0]                   # 1e-5
BATCH_FLOATS = 3 << 20      # values per nv_band batch (12 MB per stacked array; see find_flat_area_genuine)


# ------------------------------------------------------------------------------- pieces
def reduce_planes(rgb, s):
    """FUN_009b7e58 for a float image: (W/s) x (H/s) means of s x s blocks of R, G, B; each sum starts
    at +0 and adds the block row by row (float32), the mean is (1/(s*s)) * sum."""
    rgb = np.asarray(rgb, F32)
    H, W = rgb.shape[:2]
    h, w = H // s, W // s
    recip = F32(F32(1.0) / F32(s * s))
    out = []
    for c in range(3):
        blk = rgb[:h * s, :w * s, c].reshape(h, s, w, s)      # a strided view (no copy of the plane)
        acc = np.zeros((h, w), F32)
        with np.errstate(invalid="ignore", over="ignore"):
            for r in range(s):
                for q in range(s):
                    np.add(acc, blk[:, r, :, q], out=acc)
            np.multiply(recip, acc, out=acc)
        out.append(acc)
    return out


def _box(X, r, mode, rows):
    """FUN_009944c0 with the horizontal (rows, kernel flag +0x3824) or vertical kernel: radius 0 is
    the copy FUN_009f14e4 (0x9944dc: cbz x3, a bitwise memcpy), otherwise nv_lines.box_rows /
    box_cols (FUN_0099ba54 / FUN_0099bf40)."""
    if r == 0:
        return np.array(X, F32, copy=True)
    if r < 0:
        raise ValueError("FUN_009944c0 with a negative radius is not ported (FindFlatArea passes r >= 1)")
    return nl.box_rows(X, r, mode) if rows else nl.box_cols(X, r, mode)


def _fcvtzs(x):
    """fcvtzs (float32 -> int64): toward zero, saturating, NaN -> 0 (as nv_curve._fcvtzs1;
    verify_levels.py checks that both agree)."""
    x = float(x)
    if x != x:
        return 0
    if x >= 2.0 ** 63:
        return 2 ** 63 - 1
    if x < -(2.0 ** 63):
        return -(2 ** 63)
    return int(x)


def candidates(luma, rx, ry, step, nbins=16, per_bin=4):
    """FUN_009999cc(ctx, luma, W, H, rx, ry, step, nbins, per_bin, xs, ys, means, scores):
    pre-score sqrt(|box_rx,ry(box3_zero(luma - box_step(luma))^2)|) (rows with rx, columns with ry;
    a radius of 0 copies), then up to nbins * per_bin times the strictly smallest score below 10000 in
    the scan order of the genuine (rows from H-1-ry up to ry, columns from W-1-rx down to rx; NaN never
    wins), masking its (2rx+1) x (2ry+1) window, and all pixels whose window mean lies in the
    candidate's 1/nbins brightness bin once that bin holds per_bin candidates.
    Returns the arrays (xs, ys, means, scores) of length nbins * per_bin (-1 / -1.0 after the last).
    FindFlatArea passes rx = ry >= 1, step >= 1, nbins 16, per_bin 4."""
    L = np.ascontiguousarray(luma, F32)
    H, W = L.shape
    n = H * W
    total = nbins * per_bin
    xs = np.full(total, -1, np.int64)
    ys = np.full(total, -1, np.int64)
    means = np.full(total, F32(-1.0), F32)
    scores = np.full(total, F32(-1.0), F32)
    with np.errstate(invalid="ignore", over="ignore"):
        A = _box(_box(L, step, 0, True), step, 0, False)
        t = _box(_box(L - A, 1, 1, True), 1, 1, False)
        sc = _box(_box(t * t, rx, 1, True), ry, 1, False)
        sc = np.sqrt(np.abs(sc))
        M = _box(_box(L, rx, 0, True), ry, 0, False)
    if total < 1:
        return xs, ys, means, scores
    if H - 1 - ry < ry or W - 1 - rx < rx:
        return xs, ys, means, scores
    counts = np.zeros(max(nbins, 1), np.int64)
    width = F32(F32(256.0) / F32(nbins))            # fdiv s8 = 256 / nbins
    scale = F32(F32(nbins) * F32(0.00390625))        # fmul s9 = nbins * (1/256)
    with np.errstate(invalid="ignore", over="ignore"):
        for i in range(total):
            reg = sc[ry:H - ry, rx:W - rx][::-1, ::-1]       # scan order of the genuine
            key = np.where(np.isnan(reg), np.inf, reg)
            j = int(np.argmin(key))
            v = reg.flat[j]
            if not v < MASKED:
                break
            by = H - ry - 1 - j // reg.shape[1]
            bx = W - rx - 1 - j % reg.shape[1]
            mv = M[by, bx]
            xs[i], ys[i], scores[i], means[i] = bx, by, v, mv
            x1 = bx + rx if bx + rx < W else W - 1
            y0 = max(by - ry, 0)
            y1 = by + ry if by + ry < H else H - 1
            x0 = max(bx - rx, 0)
            if y0 <= y1 and x0 <= x1:
                sc[y0:y1 + 1, x0:x1 + 1] = MASKED
            b = _fcvtzs(F32(scale * mv))                     # bin of the window mean, clamped
            b = (b if b < nbins else nbins - 1) if b >= 0 else 0
            counts[b] += 1
            lo = F32(-FLT_MAX) if b == 0 else F32(width * F32(b))
            hi = F32(FLT_MAX) if b == nbins - 1 else F32(width * F32(b + 1))
            if counts[b] >= per_bin and n >= 1:
                sc[(M >= lo) & (M <= hi)] = MASKED
    return xs, ys, means, scores


def transform(p0, p1, p2, mode):
    """DAT_0147df50[mode] in place on the crop planes: 0 / 3 FUN_0097b5a4 (copy), 1 FUN_0097b5d0
    (Y = BT.601 luma as nv_pixnoise.luma_601: fused on the first n & ~3 pixels; Cr = (R - Y) * k1,
    Cb = (B - Y) * k2), 2 FUN_0097b718 (Y = ((R + G) + B) / 3, (R - Y) * 0.75, (B - Y) * 0.75).
    Planes (h, w) or stacks (..., h, w) of planes (the flat pixel order of each plane decides the
    fused / scalar split)."""
    p0, p1, p2 = (np.asarray(p, F32) for p in (p0, p1, p2))
    with np.errstate(invalid="ignore", over="ignore"):
        if mode == 1:
            Y = npx.luma_601_batch(p0, p1, p2)
            return Y, ((p0 - Y) * _CR).astype(F32), ((p2 - Y) * _CB).astype(F32)
        if mode == 2:
            Y = (((p0 + p1) + p2) / F32(3.0)).astype(F32)
            return Y, ((p0 - Y) * F32(0.75)).astype(F32), ((p2 - Y) * F32(0.75)).astype(F32)
    return p0, p1, p2


def flatness(struct, luma):
    """FUN_009b096c(model, luma): per level k from 1 (luma) / 2 (chroma) while q99[k] >= 1e-5:
    f = max over the octaves j = 1..7 of |cv_kj| / (c * 0.6^(j-1)) (c = 0.3 / 0.6, the chain stops at
    the first negative cv; the first term is fmaxnm(.., 1), later ones fcsel gt, so NaNs are passed
    over), then max(f, |q99/q50| / lim) (lim 1.8 / 0x402ccccc); the mean of the levels' values
    (1.0 when no level counts)."""
    s = np.asarray(struct).view(F32) if np.asarray(struct).dtype == U32 else np.asarray(struct, F32)
    k0 = 1 if luma else 2
    c = _FL_C[1] if luma else _FL_C[0]
    lim = _FL_LIM[1] if luma else _FL_LIM[0]
    thr = [c]
    for _ in range(6):
        thr.append(F32(thr[-1] * F32(0.6)))
    tot = F32(0.0)
    n = 0
    with np.errstate(invalid="ignore", divide="ignore", over="ignore"):
        for k in range(k0, 5):
            q99 = s[5 + k]
            if q99 < _FL_EPS:
                break
            cv = s[15 + 8 * k:23 + 8 * k]
            f = F32(1.0)
            if not cv[1] < F32(0.0):
                v = F32(np.abs(cv[1]) / thr[0])
                f = F32(1.0) if (np.isnan(v) or v < F32(1.0)) else v      # fmaxnm(v, 1)
                for j in range(2, 8):
                    if cv[j] < F32(0.0):
                        break
                    v = F32(np.abs(cv[j]) / thr[j - 1])
                    if v > f:
                        f = v
            r = F32(F32(np.abs(F32(q99 / s[10 + k]))) / lim)
            if r > f:
                f = r
            tot = F32(tot + f)
            n += 1
    return F32(tot / F32(n)) if n else F32(1.0)


def _level_terms(means):
    """Energy E and ratio R of the score from the three planes' level means (3, 5) (float32)."""
    E = F32(0.0)
    R = F32(0.0)
    with np.errstate(invalid="ignore", divide="ignore", over="ignore"):
        m1, m2, m3 = means
        if m1[0] < F32(0.0) or m2[0] < F32(0.0) or m3[0] < F32(0.0):
            return F32(0.0), F32(0.0)
        Lp = F32(F32(np.abs(m1[0]) + np.abs(m2[0])) + np.abs(m3[0]))
        E = F32(Lp + F32(0.0))
        n = 1
        weights = (None, None, F32(4.0), F32(8.0), F32(16.0))
        for k in range(1, 5):
            if m1[k] < F32(0.0) or m2[k] < F32(0.0) or m3[k] < F32(0.0):
                break
            Lk = F32(F32(np.abs(m1[k]) + np.abs(m2[k])) + np.abs(m3[k]))
            if k == 1:
                E = F32(E + F32(Lk + Lk))
                R = F32(F32(Lp / Lk) + F32(0.0))
            else:
                E = F32(E + F32(Lk * weights[k]))
                R = F32(R + F32(Lp / Lk))
            Lp = Lk
            n = k + 1
        if n > 1:
            R = F32(R / F32(n - 1))
        E = F32(E / F32(n))
    return E, R


def _score(E, R, nz, fl, b, nclip, penalty, factor):
    """The score expression in the genuine's operation order (0x9b230c..0x9b2428)."""
    with np.errstate(invalid="ignore", divide="ignore", over="ignore"):
        t = F32(F32(fl[0] * fl[1]) * fl[2])
        t = F32(t * F32(E / nz))
        t = F32(t * F32(F32(F32(np.abs(F32(b + F32(-96.0)))) * F32(0.001953125)) + F32(1.0)))
        t = F32(t * F32(F32(penalty) + F32(1.0)))
        t = F32(F32(F32(F32(nclip) * F32(4.0)) + F32(1.0)) * t)
        t = F32(F32(factor) * t)
        r = F32(F32(2.0) - R)
        r = F32(0.0) if (np.isnan(r) or r < F32(0.0)) else r                   # fmaxnm(r, 0)
        return F32(F32(F32(r + r) + F32(1.0)) * t)


def _place(c, s, size, side):
    """Square start on one axis: max(c*s - size/2, 0), moved to side - size when it overruns (if the
    square fits); returns (start, length) with the end clipped to the side."""
    x0 = c * s - (size >> 1)
    x0 = x0 if x0 >= 0 else 0
    end = x0 + size
    xs = ((side - size) if side >= size else x0) if end > side else x0
    xe = side if end > side else end
    return xs, xe - xs


# ------------------------------------------------------------------------------- the routine
def find_flat_area_genuine(rgb, noise_at, mode=1, trace=None, cancel=None, curve=None):
    """FindFlatArea (FUN_009b1a5c, param_3 = 0) -> (x, y, w, h, status); see the module docstring.
    trace: optional list that receives one dict per scored candidate (debugging).
    noise_at: brightness -> noise, or None for a failed EstimateNoiseLevelCurve (status 5).
    curve: instead of noise_at, a function () -> noise_at or None, called where the genuine runs
    EstimateNoiseLevelCurve (after the size table, so frames without a fitting size never run it).
    cancel: the cancel query (no arguments, true = cancelled), asked at the start of every size."""
    if int(mode) not in LUMA_FLAGS:
        raise ValueError(f"profile colour mode {mode}: DAT_0147df50 has the modes 0..3")
    rgb = np.asarray(rgb, F32)
    H, W = rgb.shape[:2]
    s = 7
    while True:
        s += 1
        w, h = W // s, H // s
        if not (h >= 5 and w >= 5 and w * h > 0x40000):
            break
    if w == 0 or h == 0:
        return 0, 0, 0, 0, 1
    sizes, factors = next((sz, f) for lim, sz, f in TABLES if lim is None or W * H <= lim)
    i0 = next((i for i, S in enumerate(sizes) if S <= W and S <= H), None)
    if i0 is None:
        return 0, 0, 0, 0, 1
    if curve is not None:
        noise_at = curve()
    if noise_at is None:
        return 0, 0, 0, 0, 5
    R, G, B = reduce_planes(rgb, s)
    luma = npx.luma_601(R, G, B)
    flags = LUMA_FLAGS[int(mode)]
    # scratch shared by all band-measure batches of this call (a batch holds <= BATCH_FLOATS values)
    work = nb.Work({"stack": BATCH_FLOATS, "a": BATCH_FLOATS, "b": BATCH_FLOATS, "lo0": BATCH_FLOATS // 4})
    best_sc = MASKED
    best = None
    flat_ok = clipped = False
    step = 16 // s
    step = step if step > 1 else 1
    for si in range(i0, len(sizes)):
        if cancel is not None and cancel():
            return (*(int(v) for v in (best if best is not None else (0, 0, 0, 0))), 5)
        S = sizes[si]
        r = int(F32(F32(F32(F32(S) / F32(s)) + F32(-1.0)) * F32(0.5)))
        r = r if r > 1 else 1
        xs, ys, means, scores = candidates(luma, r, r, step)
        cand = []
        for i in range(len(scores)):
            if scores[i] < F32(0.0):
                break
            x0, cw = _place(int(xs[i]), s, S, W)
            y0, ch = _place(int(ys[i]), s, S, H)
            crop = rgb[y0:y0 + ch, x0:x0 + cw]
            clip = [nv_curve.is_clip_genuine(np.ascontiguousarray(crop[..., c])) for c in range(3)]
            ncl = int(clip[0] > CLIP_THR) + int(clip[1] > CLIP_THR) + int(clip[2] > CLIP_THR)
            if ncl > 1:
                continue
            nz = F32(noise_at(F64(means[i])))
            if nz < F32(nb.EPS_1E6):
                continue
            pen = int(F64(clip[0]) > CLIP_PENALTY or F64(clip[1]) > CLIP_PENALTY or F64(clip[2]) > CLIP_PENALTY)
            cand.append((i, x0, y0, cw, ch, ncl, pen, nz))
        if not cand:
            continue
        structs = {}
        groups = {}
        for c in cand:
            groups.setdefault((c[4], c[3]), []).append(c)
        for (ch, cw), cs in groups.items():
            per = max(1, BATCH_FLOATS // (3 * ch * cw))          # candidates per band-measure batch
            for c0 in range(0, len(cs), per):
                part = cs[c0:c0 + per]
                stack = work.get("stack", (ch, cw, 3 * len(part)))   # candidate j: planes 3j..3j+2
                for j, c in enumerate(part):
                    crop = rgb[c[2]:c[2] + ch, c[1]:c[1] + cw]
                    stack[:, :, 3 * j:3 * j + 3] = np.stack(
                        transform(crop[..., 0], crop[..., 1], crop[..., 2], int(mode)), -1)
                words = nb.band_levels_bm(stack, work=work)
                for j, c in enumerate(part):
                    structs[c[0]] = words[3 * j:3 * j + 3]
        for (i, x0, y0, cw, ch, ncl, pen, nz) in cand:
            st = structs[i]
            fl = [flatness(st[p], flags[p]) for p in range(3)]
            mns = [st[p].view(F32)[0:5] for p in range(3)]
            E, Rr = _level_terms(mns)
            sc = _score(E, Rr, nz, fl, means[i], ncl, pen, factors[si])
            if trace is not None:
                trace.append(dict(size=S, i=i, x=x0, y=y0, w=cw, h=ch, mean=float(means[i]), nz=float(nz),
                                  ncl=ncl, pen=pen, fl=[float(v) for v in fl], E=float(E), R=float(Rr),
                                  score=float(sc)))
            if sc < best_sc:
                best_sc = sc
                best = (x0, y0, cw, ch)
                flat_ok = bool(fl[0] < F32(2.0)) and bool(fl[1] < F32(2.0)) and bool(fl[2] < F32(2.0))
                clipped = ncl != 0
    if best is None:
        return 0, 0, 0, 0, 1
    x0, y0, bw, bh = best
    if bw > 0x200:
        x0, bw = x0 + ((bw - 0x200) >> 1), 0x200
    if bh > 0x200:
        y0, bh = y0 + ((bh - 0x200) >> 1), 0x200
    if not flat_ok:
        status = 3
    elif clipped:
        status = 4
    else:
        status = 0 if (bh >= 0x80 and bw >= 0x80) else 2
    return int(x0), int(y0), int(bw), int(bh), status
