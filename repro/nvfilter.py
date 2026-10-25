#!/usr/bin/env python3
"""Neat Video v6 pipeline reproduction, v2.

Closer to the observed design:
  - YCbCr separation (luma gentle / chroma strong, as pro denoisers do)
  - per-subband noise profile + BayesShrink adaptive thresholding
  - hierarchical SAD motion search + half-pel refinement (bilinear)
  - occlusion-aware temporal weighting from match SAD vs profile sigma
  - global flicker gain matching

v5 (bottom of file, Agents.md §6): native YCbCr planes, flat-area + brightness-curve
noise profile, per-pixel temporal weights with residual-noise tracking, 5-band a-trous
spatial filter. The v2..v4 functions above it are kept unchanged for validate_v2/v4.
"""
import threading

import numpy as np
import pywt
from scipy.ndimage import uniform_filter, map_coordinates, gaussian_filter, median_filter

# ---------- color ----------
def rgb2ycbcr(f):
    r, g, b = f[..., 0], f[..., 1], f[..., 2]
    y = 0.299 * r + 0.587 * g + 0.114 * b
    cb = 128.0 + (-0.168736 * r - 0.331264 * g + 0.5 * b)
    cr = 128.0 + (0.5 * r - 0.418688 * g - 0.081312 * b)
    return np.stack([y, cb, cr], -1)

def ycbcr2rgb(f):
    y, cb, cr = f[..., 0], f[..., 1] - 128.0, f[..., 2] - 128.0
    r = y + 1.402 * cr
    g = y - 0.344136 * cb - 0.714136 * cr
    b = y + 1.772 * cb
    return np.stack([r, g, b], -1)

# ---------- 1. profile ----------
def find_flat(g):
    mean = uniform_filter(g, 32)
    var = uniform_filter(g * g, 32) - mean * mean
    var[:32, :] = var[-32:, :] = np.inf
    var[:, :32] = var[:, -32:] = np.inf
    y, x = np.unravel_index(np.argmin(var[16:-16, 16:-16]), var[16:-16, 16:-16].shape)
    return (slice(y + 16, y + 48), slice(x + 16, x + 48))

def build_profile_yc(yc, flat, level=5):
    """Per-band noise profile: MAD sigma seeded from finest HH, refined per level."""
    prof = {}
    for c, name in enumerate(["Y", "Cb", "Cr"]):
        coeff = pywt.wavedec2(yc[flat][:, :, c], "db1", level=2)
        hh1 = coeff[-1][2]
        base = float(np.median(np.abs(hh1)) / 0.6745)
        prof[name] = {"sigma": base, "level": level,
                      "band_sigma": {j: base for j in range(1, level + 1)}}
    return refine_profile_bands(yc, flat, prof)

def refine_profile_bands(frame_yc, flat, prof):
    """Estimate per-level sigma from the flat patch's diagonal subbands.
    Levels that don't fit the patch inherit the finer-level sigma (white-noise prior)."""
    ph = flat[0].stop - flat[0].start
    pw = flat[1].stop - flat[1].start
    import math
    maxfit = max(1, int(math.log2(min(ph, pw))) - 1)
    for c, name in enumerate(["Y", "Cb", "Cr"]):
        L = prof[name]["level"]
        Lf = min(L, maxfit)
        coeff = pywt.wavedec2(frame_yc[flat][:, :, c], "db1", level=Lf)
        for j in range(1, L + 1):
            if j <= Lf:
                hh = coeff[Lf - j + 1][2]
                if hh.size >= 64:
                    prof[name]["band_sigma"][j] = float(np.median(np.abs(hh)) / 0.6745)
                else:
                    prof[name]["band_sigma"][j] = base = prof[name]["sigma"]
            else:
                prof[name]["band_sigma"][j] = prof[name]["band_sigma"][Lf]
    return prof

# ---------- 2. spatial: BayesShrink per subband ----------
def _bayes_thresh(detail, sigma_n, strength):
    sig_y2 = float((detail ** 2).mean())
    sig_s2 = max(sig_y2 - sigma_n ** 2, 1e-12)
    T = strength * sigma_n ** 2 / (sig_s2 ** 0.5)
    return pywt.threshold(detail, min(T, 3 * 255), mode="soft")

BANDS = ["ultra-low", "very-low", "low", "medium", "high"]
# Neat defaults (User Guide): UltraLow amount 0, others ~100%
BAND_AMOUNTS = {"ultra-low": 0.0, "very-low": 1.0, "low": 1.0, "medium": 1.0, "high": 1.0}

def spatial_filter_yc(yc, prof, strength=1.0, luma=0.4, chroma=1.0,
                      band_gains=None, amount=1.0):
    """Per-band BayesShrink with application rate.
    amount ~ global Amount; luma/chroma ~ channel Amount (defaults 0.4/1.0 per manual);
    BAND_AMOUNTS ~ per-band defaults (UltraLow 0)."""
    if band_gains is None:
        band_gains = {b: 1.0 for b in BANDS}
    out = np.empty_like(yc)
    H, W = yc.shape[:2]
    for c, name in enumerate(["Y", "Cb", "Cr"]):
        s = strength * (luma if c == 0 else chroma)
        L = prof[name]["level"]
        coeff = pywt.wavedec2(yc[:, :, c], "db1", level=L)
        dn = [coeff[0]]
        for idx in range(1, L + 1):
            j = L - idx + 1  # j=1 finest .. L coarsest
            band = ["high", "medium", "low", "very-low", "ultra-low"][min(j, 5) - 1]
            sigma = prof[name]["band_sigma"][min(j, L)] * band_gains[band]
            alpha = amount * (luma if c == 0 else chroma) * BAND_AMOUNTS[band]
            alpha = min(max(alpha, 0.0), 1.0)
            shrunk = []
            for d in coeff[idx]:
                t = _bayes_thresh(d, sigma, s)
                shrunk.append(d * (1.0 - alpha) + t * alpha)
            dn.append(tuple(shrunk))
        rec = pywt.waverec2(dn, "db1")[:H, :W]
        out[:, :, c] = rec if c == 0 else np.clip(rec, 0, 255)
    return out

# ---------- 3. motion: hierarchical + half-pel ----------
def _sad_costs(prev, cur, nby, nbx, bs, rng):
    Hp, Wp = nby * bs, nbx * bs
    cur_c = cur[:Hp, :Wp]
    Pp = np.pad(prev, rng, mode="edge")
    best = np.full((nby, nbx), np.inf)
    fdy = np.zeros((nby, nbx), np.float32)
    fdx = np.zeros((nby, nbx), np.float32)
    for dy in range(-rng, rng + 1):
        for dx in range(-rng, rng + 1):
            sh = Pp[rng + dy:rng + dy + Hp, rng + dx:rng + dx + Wp]
            d = np.abs(cur_c - sh).reshape(nby, bs, nbx, bs).sum((1, 3))
            m = d < best
            best[m], fdy[m], fdx[m] = d[m], dy, dx
    return fdy, fdx, best

def _bilinear_shift(img, dy, dx):
    H, W = img.shape
    ys, xs = np.mgrid[0:H, 0:W].astype(np.float32)
    return map_coordinates(img, [np.clip(ys + dy, 0, H - 1).ravel(),
                                 np.clip(xs + dx, 0, W - 1).ravel()],
                           order=1).reshape(H, W)

def estimate_motion(prev_g, cur_g, bs=16):
    H, W = cur_g.shape
    # coarse on half resolution
    ph, ch = prev_g[::2, ::2], cur_g[::2, ::2]
    nby, nbx = (H // 2) // bs, (W // 2) // bs
    cdy, cdx, _ = _sad_costs(ph, ch, nby, nbx, bs, 4)
    fdy, fdx = cdy * 2, cdx * 2
    # fine refine full-res +-2 around coarse
    nby, nbx = H // bs, W // bs
    Hp, Wp = nby * bs, nbx * bs
    cur_c = cur_g[:Hp, :Wp]
    Pp = np.pad(prev_g, 12, mode="edge")
    best = np.full((nby, nbx), np.inf)
    rdy = np.zeros((nby, nbx), np.float32)
    rdx = np.zeros((nby, nbx), np.float32)
    ci = np.clip((np.mgrid[0:nby, 0:nbx][0] * bs) // 2 // bs, 0, fdy.shape[0] - 1)
    cj = np.clip((np.mgrid[0:nby, 0:nbx][1] * bs) // 2 // bs, 0, fdy.shape[1] - 1)
    base_dy = fdy[ci, cj].astype(np.int32)
    base_dx = fdx[ci, cj].astype(np.int32)
    for oy in range(-2, 3):
        for ox in range(-2, 3):
            dy = np.clip(base_dy + oy, -bs, bs)
            dx = np.clip(base_dx + ox, -bs, bs)
            acc = np.zeros((nby, nbx))
            for by in range(nby):
                for bx in range(nbx):
                    y, x = by * bs, bx * bs
                    acc[by, bx] = np.abs(
                        cur_c[y:y + bs, x:x + bs] -
                        Pp[y + dy[by, bx] + 12:y + dy[by, bx] + 12 + bs,
                           x + dx[by, bx] + 12:x + dx[by, bx] + 12 + bs]).sum()
            m = acc < best
            best[m] = acc[m]
            rdy[m], rdx[m] = dy[m], dx[m]
    fdy_f = rdy.astype(np.float32)
    fdx_f = rdx.astype(np.float32)
    # per-block half-pel test
    cand = [(0.5, 0), (-0.5, 0), (0, 0.5), (0, -0.5)]
    for by in range(nby):
        for bx in range(nbx):
            y, x = by * bs, bx * bs
            cur_b = cur_c[y:y + bs, x:x + bs]
            b0 = best[by, bx]
            for oy, ox in cand:
                ys = (np.arange(y, y + bs)[:, None] + rdy[by, bx] + oy).clip(0, H - 1)
                xs = (np.arange(x, x + bs)[None, :] + rdx[by, bx] + ox).clip(0, W - 1)
                ref = map_coordinates(prev_g, [ys.repeat(bs, 1).ravel(),
                                               xs.repeat(bs, 0).ravel()], order=1).reshape(bs, bs)
                s = np.abs(cur_b - ref).sum()
                if s < b0:
                    b0 = s
                    fdy_f[by, bx] = rdy[by, bx] + oy
                    fdx_f[by, bx] = rdx[by, bx] + ox
            best[by, bx] = b0
    sad = best / (bs * bs)
    return np.stack([fdy_f, fdx_f], -1), sad

def warp_float(frame, flow, bs=16):
    H, W = frame.shape[:2]
    f = flow.repeat(bs, 0).repeat(bs, 1)[:H, :W]
    ph, pw = H - f.shape[0], W - f.shape[1]
    if ph or pw:
        f = np.pad(f, ((0, max(ph, 0)), (0, max(pw, 0)), (0, 0)), mode="edge")
    ys, xs = np.mgrid[0:H, 0:W].astype(np.float32)
    sy = np.clip(ys + f[:, :, 0], 0, H - 1).ravel()
    sx = np.clip(xs + f[:, :, 1], 0, W - 1).ravel()
    if frame.ndim == 3:
        return np.stack([map_coordinates(frame[:, :, c], [sy, sx], order=1).reshape(H, W)
                         for c in range(frame.shape[2])], -1)
    return map_coordinates(frame, [sy, sx], order=1).reshape(H, W)

# ---------- 4. temporal with occlusion ----------
def temporal_filter(frames_yc, grays, prof, radius=1, strength=1.0, bs=16):
    sigma = float(prof["Y"]["sigma"])
    out = []
    for t in range(len(frames_yc)):
        acc = frames_yc[t].copy()
        norm = np.ones(frames_yc[t].shape[:2])
        for dt in range(1, radius + 1):
            for tt in (t - dt, t + dt):
                if 0 <= tt < len(frames_yc):
                    flow, sad = estimate_motion(grays[tt], grays[t], bs)
                    rel = np.exp(-np.maximum(sad - 2.0 * sigma, 0) / (4.0 * sigma + 1e-6))
                    rel_f = np.repeat(np.repeat(rel, bs, 0), bs, 1)
                    rel_f = rel_f[:acc.shape[0], :acc.shape[1]]
                    w = strength * float(np.clip(sigma / 20.0, 0.1, 1.0)) * rel_f[..., None]
                    acc = acc + w * warp_float(frames_yc[tt], flow, bs)
                    norm = norm + w[..., 0]
        out.append(acc / norm[..., None])
    return out

# ---------- 5. flicker (luma gain only; chroma centered at 128 must not scale) ----------
def deflicker(frames):
    means = np.array([f[..., 0].mean() for f in frames])
    tgt = means.mean()
    out = []
    for f, m in zip(frames, means):
        g = f.copy()
        g[..., 0] = np.clip(f[..., 0] * (tgt / m), 0, 255)
        out.append(g)
    return out


# =====================================================================
# v5 (2026-10-15): closer to the genuine pipeline (Agents.md §6).
# A frame is a tuple of native YCbCr planes (Y, Cb, Cr) float32; Cb/Cr may be
# 2x subsampled (4:2:0). Observed genuine behaviour this follows:
#   profile : FindFlatArea (genuine Auto Profile search, ported §6.5) -> per-band sigma,
#             EstimateNoiseLevelCurve (8 brightness bins, gap fill, [1,2,1]/4)
#   temporal: centre weight 1, neighbour weight = per-pixel similarity, residual
#             noise variance sum(f*w^2)/sum(w)^2 with neighbour factor f = 0.64
#   spatial : undecimated 5-band shrinkage against that residual noise
#   no global deflicker (genuine output keeps frame means within +-0.04)
# =====================================================================
V5_BANDS = ["high", "medium", "low", "very-low", "ultra-low"]
CURVE_BINS = 8                   # EstimateNoiseLevelCurve: bin = 8*Y/256
NEIGHBOUR_VAR = 0.64             # FilterTemporal: noise-variance factor of neighbour frames
CLIP_FRAC = 0.33                 # IsClip: >33% clipped pixels in >=2 channels -> reject
FLAT_FRAC, FLAT_MIN = 0.18, 64   # find_flat_area_gradient fallback: side = 18% of width, min 64 px
CHROMA_GAIN = {"Cb": 3.0, "Cr": 4.0}   # calibrated on the test_v02 genuine output (Agents.md
                                       # §6.3): it treats 3-4x our single-frame chroma estimate as noise

# threshold / spatial / luma: fitted to the genuine test_v02 output on frames 296-327 and
# confirmed on 560-591 (Agents.md §6.3); the manual's 40% luma amount fits worse here.
V5_DEFAULTS = {
    "radius": 2,          # Temporal Radius (frames on each side)
    "temporal": 1.0,      # temporal Amount (blend towards the motion-compensated average)
    "threshold": 4.0,     # temporal similarity tolerance (higher -> more averaging)
    "spatial": 2.0,       # spatial Wiener strength (multiple of the noise variance)
    "luma": 0.7,          # Noise Reduction Amount, luminance
    "chroma": 1.0,        # Noise Reduction Amount, chrominance
    "band_amounts": {"high": 1.0, "medium": 1.0, "low": 1.0, "very-low": 1.0, "ultra-low": 0.0},
    "band_noise": {b: 1.0 for b in V5_BANDS},   # per-band Noise Level (relative to profile)
    "noise_level": 1.0,   # overall Noise Level (relative to profile)
    "channel_noise": {"Y": 1.0, "Cb": 1.0, "Cr": 1.0},   # per-channel Noise Level
    "similarity": "v5",   # temporal weight: "v5" (5x5 energy, exp) or "pyramid" (FUN_0099a76c analogue)
    "pyramid_scale": 1.5, # threshold scale of the "pyramid" weight (calibrated, Agents.md §6.4)
    "bs": 16,             # motion block size (luma px)
    "search": 16,         # motion search radius (luma px)
    "mix": 0.0,           # Mix with original
}


def v5_params(**kw):
    """V5_DEFAULTS with overrides; dict values are merged, None values ignored."""
    P = {k: (dict(v) if isinstance(v, dict) else v) for k, v in V5_DEFAULTS.items()}
    for k, v in kw.items():
        if v is None:
            continue
        if isinstance(v, dict) and isinstance(P.get(k), dict):
            P[k].update(v)
        else:
            P[k] = v
    return P


def _fit(a, shape):
    """Crop / edge-pad a 2-D map to `shape`."""
    a = a[:shape[0], :shape[1]]
    ph, pw = shape[0] - a.shape[0], shape[1] - a.shape[1]
    if ph or pw:
        a = np.pad(a, ((0, ph), (0, pw)), mode="edge")
    return a


def _down2(a):
    """2x2 box down-sampling (luma grid -> 4:2:0 chroma grid)."""
    H, W = a.shape[0] // 2 * 2, a.shape[1] // 2 * 2
    return a[:H, :W].reshape(H // 2, 2, W // 2, 2).mean((1, 3))


def _to_grid(a, shape):
    """Bring a luma-grid map onto a plane's grid."""
    return a if a.shape == shape else _fit(_down2(a), shape)


def _sub(planes):
    return 1 if planes[1].shape == planes[0].shape else 2


# ---------- v5 colour (UI display / RGB sources) ----------
_KRB = {"bt709": (0.2126, 0.0722), "bt601": (0.299, 0.114), "bt2020": (0.2627, 0.0593)}
_TAG_MATRIX = {"bt709": "bt709", "smpte170m": "bt601", "bt470bg": "bt601", "bt2020nc": "bt2020",
               "bt2020c": "bt2020"}


def matrix_from_tags(tags, height):
    """YCbCr matrix of a stream from its colour_space tag (as a host would convert it to RGB);
    untagged: BT.709 from 720 lines up, else BT.601."""
    return _TAG_MATRIX.get((tags or {}).get("color_space"), "bt709" if height >= 720 else "bt601")


def yuv_to_rgb(planes, matrix="bt709", full_range=False):
    """Native planes -> RGB float (H, W, 3) for display; chroma up-sampled by repetition."""
    kr, kb = _KRB.get(matrix, _KRB["bt709"])
    Y, Cb, Cr = [np.asarray(p, np.float32) for p in planes]
    if Cb.shape != Y.shape:
        Cb = _fit(Cb.repeat(2, 0).repeat(2, 1), Y.shape)
        Cr = _fit(Cr.repeat(2, 0).repeat(2, 1), Y.shape)
    if full_range:
        y, cb, cr = Y, Cb - 128.0, Cr - 128.0
    else:
        y, cb, cr = (Y - 16.0) * 255 / 219, (Cb - 128.0) * 255 / 224, (Cr - 128.0) * 255 / 224
    r = y + 2 * (1 - kr) * cr
    b = y + 2 * (1 - kb) * cb
    g = (y - kr * r - kb * b) / (1 - kr - kb)
    return np.stack([r, g, b], -1)


def rgb_to_planes(rgb):
    """RGB float -> full-range BT.601 4:4:4 planes (synthetic clips)."""
    yc = rgb2ycbcr(np.asarray(rgb, np.float32))
    return tuple(np.ascontiguousarray(yc[..., c]) for c in range(3))


# ---------- v5 multi-scale decomposition ----------
_B3 = (1 / 16, 4 / 16, 6 / 16)


def _b3(a, step, axis):
    """1-D B3-spline smoothing with holes (dilation `step`), symmetric borders."""
    pad = [(0, 0), (0, 0)]
    pad[axis] = (2 * step, 2 * step)
    p = np.pad(a, pad, mode="symmetric")
    n = a.shape[axis]

    def tap(k):
        s = slice(k * step, k * step + n)
        return p[s] if axis == 0 else p[:, s]
    return _B3[0] * (tap(0) + tap(4)) + _B3[1] * (tap(1) + tap(3)) + _B3[2] * tap(2)


def atrous(img, levels):
    """Undecimated B3-spline (a trous) decomposition -> [band_1 (finest) .. band_L, residual]."""
    bands, c = [], np.asarray(img, np.float32)
    for j in range(levels):
        s = _b3(_b3(c, 2 ** j, 0), 2 ** j, 1)
        bands.append(c - s)
        c = s
    bands.append(c)
    return bands


_WN = {}


def _wn_band_std(levels):
    """Band std of unit white noise (exact, from the impulse response)."""
    if levels not in _WN:
        n = 8 * 2 ** levels + 1
        d = np.zeros((n, n), np.float32)
        d[n // 2, n // 2] = 1.0
        _WN[levels] = [float(np.sqrt((b * b).sum())) for b in atrous(d, levels)[:levels]]
    return _WN[levels]


# ---------- v5 profile ----------
def _clip_masks(planes, full_range, matrix="bt709"):
    """IsClip masks on RGB (luma grid). Black bars / blown highlights clip all three RGB
    channels, while in YCbCr only Y sits at its limit (letterbox: Y=16, Cb=Cr=128)."""
    rgb = yuv_to_rgb(planes, matrix, full_range)
    return [(rgb[..., c] <= 0.5) | (rgb[..., c] >= 254.5) for c in range(3)]


def _dead(Y):
    """Pixels whose 5x5 neighbourhood is exactly constant (bars, synthetic fills): no noise to
    measure there, so they must not enter any noise statistic."""
    Y = np.asarray(Y, np.float32)
    b1 = Y - _b3(_b3(Y, 1, 0), 1, 1)
    return uniform_filter(b1 * b1, 5) < 1e-6


def _noise_shape(sig, tot, decay=0.9):
    """Band ratios sigma_j/sigma_1 and total/sigma_1 with a physical prior: band noise shrinks
    with scale (<= `decay` per level) and the total stays within [1, 2*rms(ratios)]. With no
    measurable noise (sigma_1 < 0.02, e.g. black bars) the white-noise shape is used.
    decay 0.9 for static-pixel differences; 0.6 for a single-frame area, whose coarse bands
    are inflated by scene structure (test_v02: up to 4.6x). White noise decays <= 0.5 per
    level, the measured test_v02 luma noise <= 0.55."""
    wn = _wn_band_std(len(sig))
    if sig[0] < 0.02:
        r = [w / wn[0] for w in wn]
        return r, float(np.sqrt(np.sum(np.square(r))))
    r = [1.0]
    for t in sig[1:]:
        r.append(min(t / sig[0], decay * r[-1]))
    return r, min(max(tot / sig[0], 1.0), 2.0 * float(np.sqrt(np.sum(np.square(r)))))


def _mad_sigma(x):
    x = np.asarray(x, np.float32).ravel()
    return float(1.4826 * np.median(np.abs(x - np.median(x))))


def _trim_rms(x):
    """RMS without the top 1% |x| (motion outliers), Gaussian-corrected. Unlike the MAD it
    stays valid for sub-LSB noise, where quantisation makes most differences exactly 0."""
    a = np.abs(np.asarray(x, np.float32).ravel())
    if a.size == 0:
        return 0.0
    a = a[a <= np.percentile(a, 99)]
    return float(1.04 * np.sqrt((a * a).mean()))


FLAT_STATUS = {0: "ok", 1: "no square found (fallback)", 2: "small square (< 128 px)",
               3: "not flat (details in the square)", 4: "clipped pixels in the square",
               5: "cancelled (no profile built)"}


class ProfileCancelled(Exception):
    """build_profile_v5 was cancelled: FindFlatArea returned status 5 (the genuine Auto Profile then
    builds no noise model, FUN_0014e4a4)."""
_FFA_CACHE = {}
_FFA_LOCK = threading.Lock()   # the UI profiles in worker threads


def find_flat_area(planes, full_range=False, matrix="bt709", mode=1, with_status=False, curve="genuine",
                   cancel=None):
    """FindFlatArea as the genuine Auto Profile runs it (find_flat_area_genuine, decoded
    §6.5) on the host-style RGB of the frame -> (y0, x0, size) on the luma grid, plus the
    genuine status code (FLAT_STATUS) with `with_status`.
    Candidate scores are normalised by the noise-vs-brightness curve: curve="genuine" uses the
    exact port of the genuine EstimateNoiseLevelCurve (nv_curve.genuine_noise_curve on the same
    RGB); curve="v5" uses the single-frame luma curve of noise_curve_spatial converted to
    full-range luma (faster stand-in; differs from the genuine curve by 0.73-1.20x, which can
    flip near ties). Frames under 64 px (the genuine Auto Profile refuses them) and frames
    without a qualifying square use find_flat_area_gradient (status 1). Results are cached
    per frame content (the UI / calibration re-profile the same frame in several modes).
    cancel: the cancel query (no arguments, true = cancelled) the genuine asks during the noise curve
    (once per tile and once more; curve="genuine" only, the v5 stand-in is not cancellable) and at the
    start of every size; a cancel gives status 5 with the genuine's outputs at that point ((0, 0, 0) if
    the curve was cancelled or no square was found yet), no fallback and nothing cached. With a cancel,
    pass with_status=True (status 5 is otherwise indistinguishable from an empty area: ValueError).
    Only the search is cancellable; a cached result returns without a query."""
    import hashlib
    planes = tuple(np.asarray(p, np.float32) for p in planes)
    h = hashlib.blake2b(digest_size=16)
    for p in planes:
        h.update(str(p.shape).encode())
        h.update(np.ascontiguousarray(p).view(np.uint8))
    if cancel is not None and not with_status:
        raise ValueError("find_flat_area: pass with_status=True with cancel (status 5 = cancelled)")
    key = (h.hexdigest(), bool(full_range), matrix, int(mode), curve)
    with _FFA_LOCK:
        res = _FFA_CACHE.get(key)
    if res is None:
        H, W = planes[0].shape
        res = None
        if min(H, W) >= 64:
            rgb = np.clip(np.nan_to_num(yuv_to_rgb(planes, matrix, full_range), nan=0.0, posinf=255.0,
                                        neginf=0.0), 0, 255)
            if curve == "genuine":
                import nv_curve

                def make_noise_at():                # run where the genuine runs EstimateNoiseLevelCurve
                    table = nv_curve.genuine_noise_curve(rgb.astype(np.float32), cancel=cancel)
                    return None if table is None else nv_curve.noise_at(table)
            else:
                cv = noise_curve_spatial(planes, {"Y": 0.6}, full_range, matrix=matrix, names=("Y",)).get("Y")
                centers = [256.0 / CURVE_BINS * (i + 0.5) for i in range(CURVE_BINS)]
                k = 1.0 if full_range else 255.0 / 219.0

                def noise_at(b):
                    if not cv:
                        return 1.0
                    yc = b if full_range else b / k + 16.0
                    return float(np.interp(yc, centers, cv)) * k

                def make_noise_at():
                    return noise_at
            x0, y0, w, hh, status = find_flat_area_genuine(rgb, None, mode, cancel=cancel, curve=make_noise_at)
            if status == 5:
                res = (int(y0), int(x0), int(min(w, hh)), 5)
                return res if with_status else res[:3]
            if status != 1 and min(w, hh) >= 8:
                res = (int(y0), int(x0), int(min(w, hh)), int(status))
        if res is None:
            res = (*find_flat_area_gradient(planes, full_range, matrix=matrix), 1)
        with _FFA_LOCK:
            if key not in _FFA_CACHE:         # another thread may have stored it meanwhile
                while len(_FFA_CACHE) >= 16:
                    _FFA_CACHE.pop(next(iter(_FFA_CACHE)))
                _FFA_CACHE[key] = res
    return res if with_status else res[:3]


def find_flat_area_gradient(planes, full_range=False, frac=FLAT_FRAC, min_size=FLAT_MIN, matrix="bt709"):
    """v5 FindFlatArea stand-in (before the decode, kept as fallback) -> (y0, x0, size).
    Side = max(min_size, frac*W); least low-pass gradient energy wins; candidates with
    >33% clipped pixels in >=2 RGB channels (IsClip) or >5% noise-free pixels are rejected."""
    Y = np.asarray(planes[0], np.float32)
    H, W = Y.shape
    S = int(min(max(min_size, round(frac * W)), H, W))
    gy, gx = np.gradient(gaussian_filter(Y, 2.0))
    score = uniform_filter(gy * gy + gx * gx, S, mode="nearest")
    ys = np.arange(S // 2, H - S + S // 2 + 1, max(S // 8, 2))
    xs = np.arange(S // 2, W - S + S // 2 + 1, max(S // 8, 2))
    nbad = sum((uniform_filter(m.astype(np.float32), S, mode="nearest")[np.ix_(ys, xs)] > CLIP_FRAC).astype(int)
               for m in _clip_masks(planes, full_range, matrix))
    # any bar edge inside the square would dominate its coarse bands -> stricter than IsClip
    dead = uniform_filter(_dead(Y).astype(np.float32), S, mode="nearest")[np.ix_(ys, xs)] > 0.05
    ok = (nbad < 2) & ~dead
    sc = score[np.ix_(ys, xs)]
    if ok.any():
        sc = np.where(ok, sc, np.inf)
    iy, ix = np.unravel_index(np.argmin(sc), sc.shape)
    return int(ys[iy] - S // 2), int(xs[ix] - S // 2), S


def _robust_sigma(band, blk):
    """Structure-robust sigma of a band inside an area: 1.4826*MAD per sub-block, then the
    25th percentile over sub-blocks (a profiling square rarely is flat everywhere)."""
    n = min(band.shape) // blk
    if n < 2:
        return _mad_sigma(band)
    r = _blocks(band, blk)
    v = 1.4826 * np.median(np.abs(r - np.median(r, -1, keepdims=True)), -1).ravel()
    live = v[v > 0.02]                      # exactly flat sub-blocks carry no noise
    return float(np.percentile(live if live.size else v, 25))


def area_band_sigma(plane, area, levels, sub=1):
    """Per-band noise sigma inside the profiling square, from the a-trous bands of the area
    alone (sub-block size grows with the band scale, see _robust_sigma). Bands with < 16
    effective samples follow the white-noise ratio of the previous band.
    Returns (band sigmas, total detail sigma)."""
    y0, x0, S = area
    a = np.asarray(plane, np.float32)[y0 // sub:(y0 + S) // sub, x0 // sub:(x0 + S) // sub]
    b = atrous(a, levels)
    wn = _wn_band_std(levels)
    n = min(a.shape)
    sig = []
    for j in range(levels):
        if j == 0 or (n / 2 ** j) ** 2 >= 16:
            sig.append(_robust_sigma(b[j], max(16, 8 * 2 ** j)))
        else:
            sig.append(sig[-1] * wn[j] / wn[j - 1])
    return sig, _robust_sigma(a - b[levels], 32)


def _fill_curve(vals, valid):
    """EstimateNoiseLevelCurve post-processing: bins with too few samples are filled by a
    polynomial fit (quadratic inside, linear at the ends, kept positive), then [1,2,1]/4."""
    n = len(vals)
    x = np.arange(n, dtype=np.float64)
    v = np.array(vals, np.float64)
    idx = np.flatnonzero(valid)
    if len(idx) == 0:
        return None
    if len(idx) == 1:
        v[:] = v[idx[0]]
    else:
        poly = np.polyfit(x[idx], v[idx], 2 if len(idx) >= 3 else 1)
        for i in range(idx[0] + 1, idx[-1]):
            if not valid[i]:
                e = np.polyval(poly, x[i])
                v[i] = e if e > 0 else v[i - 1]
        lo, hi = idx[:3], idx[-3:]
        pl = np.polyfit(x[lo], v[lo], 1)
        ph = np.polyfit(x[hi], v[hi], 1)
        for i in range(idx[0] - 1, -1, -1):
            e = np.polyval(pl, x[i])
            v[i] = e if e > 0 else v[i + 1]
        for i in range(idx[-1] + 1, n):
            e = np.polyval(ph, x[i])
            v[i] = e if e > 0 else v[i - 1]
    s = v.copy()
    s[1:-1] = (v[:-2] + 2 * v[1:-1] + v[2:]) / 4
    return [float(t) for t in s]


def _blocks(a, blk):
    nby, nbx = a.shape[0] // blk, a.shape[1] // blk
    return a[:nby * blk, :nbx * blk].reshape(nby, blk, nbx, blk).transpose(0, 2, 1, 3).reshape(nby, nbx, -1)


def noise_curve_spatial(planes, ratio2, full_range=False, blk=16, matrix="bt709", names=("Y", "Cb", "Cr")):
    """EstimateNoiseLevelCurve analogue on one frame: robust finest-band sigma per 16 px
    block; blocks with >33% clipped pixels in >=2 RGB channels, >10% noise-free pixels or
    with structure (band-2 /
    band-1 ratio above 1.5x the flat-area ratio `ratio2`) are skipped; per brightness bin
    a low percentile over blocks; bins under W*H/1024 pixels are filled.
    `names` limits the planes computed ("Y" first: its structure test also gates chroma).
    Returns {plane: [finest-band sigma per bin] or None}."""
    Y = np.asarray(planes[0], np.float32)
    H, W = Y.shape
    nby, nbx = H // blk, W // blk
    if nby == 0 or nbx == 0:                     # smaller than one block: nothing to measure
        return {n: None for n in names}
    bins = np.clip((_blocks(Y, blk).mean(-1) * CURVE_BINS / 256.0).astype(int), 0, CURVE_BINS - 1)
    nclip = sum((_blocks(m.astype(np.float32), blk).mean(-1) > CLIP_FRAC).astype(int)
                for m in _clip_masks(planes, full_range, matrix))
    live = _blocks(_dead(Y).astype(np.float32), blk).mean(-1) < 0.1
    curves, flat = {}, None
    for name, p in zip(("Y", "Cb", "Cr"), planes):
        if name not in names:
            continue
        s = 1 if p.shape == Y.shape else 2
        b = atrous(p, 2)
        m1 = _fit(1.4826 * np.median(np.abs(_blocks(b[0], blk // s)), -1), (nby, nbx))
        m2 = _fit(1.4826 * np.median(np.abs(_blocks(b[1], blk // s)), -1), (nby, nbx))
        ok = (nclip < 2) & live & (m1 > 0.02) & (m2 < 1.5 * ratio2[name] * m1)
        if flat is None:
            flat = ok          # luma decides structure for the chroma planes too
        else:
            ok &= flat
        vals, valid = [], []
        for i in range(CURVE_BINS):
            sel = ok & (bins == i)
            # 25th percentile of the block estimates (x1.07: unbiased for pure-noise blocks,
            # whose sigma estimates spread ~10%); the median is pulled up by fine texture
            vals.append(1.07 * float(np.percentile(m1[sel], 25)) if sel.any() else 0.0)
            valid.append(sel.sum() * blk * blk >= H * W / 1024.0)
        curves[name] = _fill_curve(vals, valid)
    return curves


def noise_curve_temporal(p0, p1, levels=5, static=0.5):
    """Profile from a frame pair: on static pixels (|15x15-mean dt| < `static`) the noise is
    (p1-p0)/sqrt(2). Returns (curves, band ratios, total ratios, static fraction)."""
    Y0, Y1 = np.asarray(p0[0], np.float32), np.asarray(p1[0], np.float32)
    H, W = Y0.shape
    still = (np.abs(uniform_filter(Y1, 15) - uniform_filter(Y0, 15)) < static) & ~_dead(Y0)
    lum = (Y0 + Y1) / 2
    curves, ratios, tots = {}, {}, {}
    for name, a0, a1 in zip(("Y", "Cb", "Cr"), p0, p1):
        d = (np.asarray(a1, np.float32) - np.asarray(a0, np.float32)) / np.sqrt(2.0)
        m = still if d.shape == Y0.shape else _to_grid(still.astype(np.float32), d.shape) > 0.99
        lu = _to_grid(lum, d.shape)
        L = levels if d.shape == Y0.shape else levels - 1
        b = atrous(d, L)
        if m.sum() < 64:
            curves[name] = None
            continue
        sig = [_trim_rms(b[j][m]) for j in range(L)]
        ratios[name], tots[name] = _noise_shape(sig, _trim_rms((d - b[L])[m]))
        bins = np.clip((lu * CURVE_BINS / 256.0).astype(int), 0, CURVE_BINS - 1)
        vals, valid = [], []
        for i in range(CURVE_BINS):
            sel = m & (bins == i)
            vals.append(_trim_rms(b[0][sel]) if sel.any() else 0.0)
            valid.append(sel.sum() * (H * W / d.size) >= H * W / 1024.0)
        curves[name] = _fill_curve(vals, valid)
    return curves, ratios, tots, float(still.mean())


def _median_curves(curves):
    """Per-bin median of several {plane: curve or None} dicts."""
    out = {}
    for n in ("Y", "Cb", "Cr"):
        cs = [c[n] for c in curves if c.get(n) is not None]
        out[n] = [float(v) for v in np.median(np.array(cs), 0)] if cs else None
    return out


def build_profile_v5(planes, full_range=False, area=None, pair=None, mode="auto", levels=5, more=(),
                     matrix="bt709", cancel=None):
    """Device Noise Profile analogue.
    planes: (Y, Cb, Cr) of the profiling frame. area=(y0, x0, size) overrides FindFlatArea
    (the genuine Auto Profile search, run on the RGB the host would pass: `matrix`).
    more: further frames whose single-frame curves are median-combined with this one (an
    intra-coded frame keeps more noise than its P/B neighbours, e.g. test.mp4 GOP 12).
    mode 'spatial' : genuine-style single frame (curve from 16 px blocks, band shape from area)
         'temporal': curve and band shape from static-pixel frame differences (needs pair =
                     next frame's planes); sees only temporally varying noise
         'auto'    : curve from the single frame (like the genuine profile, which also counts
                     static compression noise), band shape from the static-pixel differences
                     when >= 10% of the pixels are static (a single-frame area is rarely flat
                     at coarse scales), else from the area. With those static pixels and a
                     single-frame curve for every plane the result does not depend on the area.
    sigma(plane, band j, px) = curve[plane](brightness) * band_ratio[plane][j].
    cancel: passed to find_flat_area (searched area only); its status 5 raises ProfileCancelled."""
    sub = _sub(planes)
    H, W = np.asarray(planes[0]).shape
    flat_status = None                      # None = manual area
    if area is None:
        *area, flat_status = find_flat_area(planes, full_range, matrix, with_status=True, cancel=cancel)
        if flat_status == 5:
            raise ProfileCancelled("profiling cancelled (FindFlatArea status 5)")
    else:                                   # manual area: keep it inside the frame
        S = int(min(max(int(area[2]), 8), H, W))
        area = (int(min(max(int(area[0]), 0), H - S)), int(min(max(int(area[1]), 0), W - S)), S)
    prof = {"version": 5, "sub": sub, "full_range": bool(full_range), "area": [int(v) for v in area],
            "mode": "spatial", "levels": {}, "band_ratio": {}, "tot_ratio": {}, "area_sigma": {},
            "curve": {}, "curve_centers": [256.0 / CURVE_BINS * (i + 0.5) for i in range(CURVE_BINS)],
            "flat_status": flat_status}
    for c, name in enumerate(("Y", "Cb", "Cr")):
        s = 1 if c == 0 else sub
        L = levels if s == 1 else levels - 1     # 4:2:0 chroma: half-res levels = medium..ultra-low
        sig, tot = area_band_sigma(planes[c], area, L, s)
        prof["levels"][name] = L
        prof["area_sigma"][name] = sig
        prof["band_ratio"][name], prof["tot_ratio"][name] = _noise_shape(sig, tot, 0.6)
    curves = None
    if mode in ("temporal", "auto") and pair is not None:
        cv, ratios, tots, still = noise_curve_temporal(planes, pair, levels)
        prof["static_fraction"] = still
        if all(cv.get(n) is not None for n in ("Y", "Cb", "Cr")) and (mode == "temporal" or still >= 0.1):
            prof["band_ratio"].update(ratios)
            prof["tot_ratio"].update(tots)
            prof["mode"] = mode
            if mode == "temporal":
                curves = cv
    if curves is None:
        r2 = {n: prof["band_ratio"][n][1] for n in ("Y", "Cb", "Cr")}
        curves = _median_curves([noise_curve_spatial(p, r2, full_range, matrix=matrix) for p in (planes, *more)])
    # safeguard (not from the genuine): a mostly exactly-flat picture (graphics, synthetic
    # sources) has no noise to remove - its flattest *textured* area would be mistaken for it
    prof["dead_fraction"] = float(_dead(planes[0]).mean())
    if prof["dead_fraction"] > 0.5:
        prof["mode"] += "+noise-free"
        curves = {n: [0.0] * CURVE_BINS for n in ("Y", "Cb", "Cr")}
    for name in ("Y", "Cb", "Cr"):
        cv = curves.get(name) or [prof["area_sigma"][name][0]] * CURVE_BINS
        prof["curve"][name] = cv if name == "Y" else [v * CHROMA_GAIN[name] for v in cv]
    lvl = float(np.mean(np.asarray(planes[0], np.float32)[area[0]:area[0] + area[2], area[1]:area[1] + area[2]]))
    prof["area_level"] = lvl
    prof["sigma"] = {n: float(np.interp(lvl, prof["curve_centers"], prof["curve"][n]) * prof["tot_ratio"][n])
                     for n in ("Y", "Cb", "Cr")}
    return prof


def curve_sigma(prof, name, lum):
    """Finest-band noise sigma per pixel from brightness `lum` (on the plane's grid)."""
    return np.interp(lum, prof["curve_centers"], prof["curve"][name]).astype(np.float32)


# ---------- v5 motion ----------
def _search(cur, ref, rng, nby, nbx, blk):
    """Full-search block SAD over +-rng (3x3-block aggregated, small zero-motion bias)."""
    Hp, Wp = nby * blk, nbx * blk
    c = cur[:Hp, :Wp]
    P = np.pad(ref, rng, mode="edge")
    best = np.full((nby, nbx), np.inf, np.float32)
    vy = np.zeros((nby, nbx), np.int32)
    vx = np.zeros((nby, nbx), np.int32)
    lam = 0.02 * blk * blk
    for dy in range(-rng, rng + 1):
        for dx in range(-rng, rng + 1):
            d = np.abs(c - P[rng + dy:rng + dy + Hp, rng + dx:rng + dx + Wp])
            d = uniform_filter(d.reshape(nby, blk, nbx, blk).sum((1, 3)), 3, mode="nearest")
            d += lam * (abs(dy) + abs(dx))
            m = d < best
            best[m], vy[m], vx[m] = d[m], dy, dx
    return vy, vx


def _refine(cur, ref, vy, vx, blk):
    """+-1 SSD refinement around (vy, vx) per block -> (new vy, new vx, 3x3 cost cube)."""
    nby, nbx = vy.shape
    pad = int(max(np.abs(vy).max(), np.abs(vx).max())) + 2
    P = np.pad(ref, pad, mode="edge")
    cb = cur[:nby * blk, :nbx * blk].reshape(nby, blk, nbx, blk).transpose(0, 2, 1, 3)
    yy = (np.arange(nby) * blk)[:, None, None, None] + np.arange(blk)[None, None, :, None] + pad
    xx = (np.arange(nbx) * blk)[None, :, None, None] + np.arange(blk)[None, None, None, :] + pad
    cost = np.empty((3, 3, nby, nbx), np.float32)
    for i, oy in enumerate((-1, 0, 1)):
        ys = yy + (vy + oy)[:, :, None, None]
        for j, ox in enumerate((-1, 0, 1)):
            d = cb - P[ys, xx + (vx + ox)[:, :, None, None]]
            cost[i, j] = (d * d).sum((2, 3))
    k = cost.reshape(9, nby, nbx).argmin(0)
    return vy + k // 3 - 1, vx + k % 3 - 1, cost


def estimate_flow_v5(ref, cur, bs=16, search=16):
    """Centre->neighbour block motion (MotionCompensator analogue), 3-level pyramid:
    quarter-res full search, +-1 SSD refinement at half and full resolution, parabolic
    sub-pixel fit on the SSD surface (no interpolation bias, unlike half-pel bilinear
    matching on noisy frames), 3x3 median regularisation.
    Returns block vectors (vy, vx) in luma px: neighbour(p + v) ~ centre(p)."""
    cur = np.asarray(cur, np.float32)
    ref = np.asarray(ref, np.float32)
    nby, nbx = cur.shape[0] // bs, cur.shape[1] // bs
    a0, b0 = gaussian_filter(cur, 1.0), gaussian_filter(ref, 1.0)
    a1, b1 = _down2(a0), _down2(b0)
    a2, b2 = _down2(a1), _down2(b1)
    vy, vx = _search(a2, b2, max(1, -(-search // 4)), nby, nbx, bs // 4)
    vy, vx, _ = _refine(a1, b1, 2 * vy, 2 * vx, bs // 2)
    vy, vx, _ = _refine(a0, b0, 2 * vy, 2 * vx, bs)
    vy, vx, c = _refine(a0, b0, vy, vx, bs)      # cost cube centred on the integer optimum
    centred = c.reshape(9, nby, nbx).argmin(0) == 4
    fy, fx = vy.astype(np.float32), vx.astype(np.float32)
    dny = c[0, 1] - 2 * c[1, 1] + c[2, 1]
    dnx = c[1, 0] - 2 * c[1, 1] + c[1, 2]
    fy += np.where(centred & (dny > 0), np.clip(0.5 * (c[0, 1] - c[2, 1]) / np.maximum(dny, 1e-6), -0.5, 0.5), 0)
    fx += np.where(centred & (dnx > 0), np.clip(0.5 * (c[1, 0] - c[1, 2]) / np.maximum(dnx, 1e-6), -0.5, 0.5), 0)
    return median_filter(fy, 3, mode="nearest"), median_filter(fx, 3, mode="nearest")


_GRID = {}


def _grid(shape):
    if shape not in _GRID:
        _GRID[shape] = np.mgrid[0:shape[0], 0:shape[1]].astype(np.float32)
    return _GRID[shape]


def dense_flow(vy, vx, shape, bs=16):
    """Bilinear interpolation of block-centre vectors to a per-pixel field (no block edges)."""
    u = (np.arange(shape[0], dtype=np.float32) - (bs - 1) / 2) / bs
    v = (np.arange(shape[1], dtype=np.float32) - (bs - 1) / 2) / bs
    uu, vv = np.meshgrid(u, v, indexing="ij")
    return (map_coordinates(vy, [uu, vv], order=1, mode="nearest"),
            map_coordinates(vx, [uu, vv], order=1, mode="nearest"))


def warp_plane(img, FY, FX):
    """Bilinear backward warp: out(p) = img(p + F(p)), clamped at the borders."""
    H, W = img.shape
    gy, gx = _grid((H, W))
    return map_coordinates(np.asarray(img, np.float32), [np.clip(gy + FY, 0, H - 1), np.clip(gx + FX, 0, W - 1)],
                           order=1, mode="nearest")


# ---------- v5 temporal / spatial ----------
# FUN_0099a76c / FUN_0099afb4 (Agents.md §6.4): per pyramid level k the luma difference is
# normalised by the local noise, its local RMS R_k (3x3 at that level) is compared with
# t_k = scale * C_k * 1.1 * 0.9 * sqrt(1 + 0.64); weight_k = clamp((2 t_k - R_k) / t_k, 0, 1)
# and the levels multiply (coarse to fine). The coarsest level also gets + T^2 = (0.16 *
# Threshold)^2 (64 by default), i.e. it effectively never rejects; it is left out here.
_PYR_C = (1.06, 1.0, 1.0)                  # DAT_012445a8
_PYR_K = 1.1 * 0.9 * 1.2806249             # 1.2806249 = sqrt(1 + NEIGHBOUR_VAR)


def pyramid_weight(d, s1, ratios, scale=1.0):
    """Multi-scale match weight for the difference d = centre - compensated neighbour (luma).
    s1: finest-band noise sigma map of the centre frame; ratios: profile band ratios. Undecimated
    a-trous bands stand in for the genuine decimated pyramid (windows grow with the scale)."""
    b = atrous(d, len(_PYR_C))
    w = np.ones(d.shape, np.float32)
    for j, c in enumerate(_PYR_C):
        x = b[j] / (s1 * ratios[j] + 1e-3)
        r = np.sqrt(uniform_filter(x * x, 2 ** (j + 1) + 1))
        t = max(scale, 1e-3) * c * _PYR_K
        w *= np.clip((2.0 * t - r) / t, 0.0, 1.0)
    return w


def temporal_v5(window, ci, prof, P, flows=None):
    """FilterTemporal analogue on frames `window` around index `ci`.
    Neighbour weight w = exp(-max(D/(2 sigma^2) - 1, 0) / threshold), D = 5x5 mean squared
    difference to the motion-compensated neighbour, sigma = profile total noise at the
    pixel brightness; centre weight 1. Also returns the residual noise variance factor
    ((1-a) + a/S)^2 + a^2 sum(0.64 w^2)/S^2 per plane (S = 1 + sum w, a = Amount).
    `flows` (dict keyed by frame offset) caches block vectors across calls; use one dict per
    centre frame and per (bs, search)."""
    sim = P.get("similarity", "v5")
    if sim not in ("v5", "pyramid"):
        raise ValueError(f"unknown similarity {sim!r} (expected 'v5' or 'pyramid')")
    cur = [np.asarray(p, np.float32) for p in window[ci]]
    Y = cur[0]
    a = float(P["temporal"])
    st = (curve_sigma(prof, "Y", gaussian_filter(Y, 1.0)) * prof["tot_ratio"]["Y"]
          * P["noise_level"] * P["channel_noise"]["Y"])
    den = 2.0 * st * st + 1e-6
    acc = [p.copy() for p in cur]
    sw = [np.ones(p.shape, np.float32) for p in cur]
    sf = [np.zeros(p.shape, np.float32) for p in cur]
    for k, nb in enumerate(window):
        if k == ci or a <= 0:
            continue
        if flows is not None and (k - ci) in flows:
            vy, vx = flows[k - ci]
        else:
            vy, vx = estimate_flow_v5(nb[0], Y, P["bs"], P["search"])
            if flows is not None:
                flows[k - ci] = (vy, vx)
        FY, FX = dense_flow(vy, vx, Y.shape, P["bs"])
        wy = warp_plane(nb[0], FY, FX)
        if sim == "pyramid":
            s1 = st / max(prof["tot_ratio"]["Y"], 1e-6)          # finest-band sigma map
            w = pyramid_weight(Y - wy, s1, prof["band_ratio"]["Y"], P.get("pyramid_scale", 1.5))
        else:
            w = np.exp(-np.maximum(uniform_filter((wy - Y) ** 2, 5) / den - 1.0, 0.0) / max(P["threshold"], 1e-3))
        for c, p in enumerate(nb):
            if c == 0:
                wp, wc = wy, w
            elif p.shape == Y.shape:
                wp, wc = warp_plane(p, FY, FX), w
            else:
                wp = warp_plane(p, _to_grid(FY, p.shape) / 2, _to_grid(FX, p.shape) / 2)
                wc = _to_grid(w, p.shape)
            acc[c] += wc * wp
            sw[c] += wc
            sf[c] += NEIGHBOUR_VAR * wc * wc
    out, rv = [], []
    for c, p in enumerate(cur):
        out.append(p + a * (acc[c] / sw[c] - p))
        rv.append(((1 - a) + a / sw[c]) ** 2 + a * a * sf[c] / (sw[c] * sw[c]))
    return out, rv


def spatial_v5(planes, prof, rv, P):
    """Spatial filter: 5-band a-trous shrinkage (Neat ultra-low..high) against the noise
    left by the temporal stage. noise(band, px) = curve(brightness) * band ratio * Noise
    Levels * sqrt(residual variance); gain = local Wiener on the 7x7 band energy;
    b' = b * (1 - Amount * (1 - gain)) with Amount = channel x band amount (applied once)."""
    Y = np.asarray(planes[0], np.float32)
    lum = gaussian_filter(Y, 1.0)
    out = []
    for c, (name, p) in enumerate(zip(("Y", "Cb", "Cr"), planes)):
        L = prof["levels"][name]
        names = V5_BANDS[len(V5_BANDS) - L:] if c else V5_BANDS[:L]
        A = float(P["luma"] if c == 0 else P["chroma"])
        s1 = (curve_sigma(prof, name, _to_grid(lum, p.shape)) * P["noise_level"]
              * P["channel_noise"][name] * np.sqrt(rv[c]))
        b = atrous(p, L)
        rec = b[L]
        for j in range(L):
            s = s1 * prof["band_ratio"][name][j] * P["band_noise"][names[j]]
            e = uniform_filter(b[j] * b[j], 7)
            g = np.maximum(e - P["spatial"] * s * s, 0.0) / (e + 1e-6)
            amt = min(max(A * P["band_amounts"][names[j]], 0.0), 1.0)
            rec = rec + b[j] * (1.0 - amt * (1.0 - g))
        out.append(rec)
    return out


def process_frame_v5(window, ci, prof, P, flows=None):
    """One output frame: Temporal -> Spatial -> Mix. `window` = list of planes tuples (frames
    around t, truncated at clip ends), `ci` = index of frame t in it. Returns float32
    planes (not clipped / rounded)."""
    R = int(P["radius"])
    lo, hi = max(0, ci - R), min(len(window), ci + R + 1)
    tmp, rv = temporal_v5(window[lo:hi], ci - lo, prof, P, flows)
    out = spatial_v5(tmp, prof, rv, P)
    m = float(P["mix"])
    if m > 0:
        out = [o + m * (np.asarray(s, np.float32) - o) for o, s in zip(out, window[ci])]
    return out


def flicker_gains(means, radius=12):
    """Optional whole-frame flicker reduction (off by default; the genuine test output shows
    none): gain = moving average of the luma means over +-radius frames / own mean."""
    m = np.asarray(means, np.float64)
    k = np.ones(2 * radius + 1)
    num = np.convolve(np.pad(m, radius, mode="edge"), k, "valid")
    return num / (2 * radius + 1) / np.maximum(m, 1e-6)


# =====================================================================
# Genuine noise-measurement primitives (decoded 2026-10-15, Agents.md §6.5). Each function
# names the routine it reproduces (Ghidra address, arm64 slice). Values are on the genuine's
# internal scale: RGB float 0..255 (full range). Since 2026-10-16 they delegate to the bit-exact
# ports nv_band.py (FUN_009980e8 / level part of FUN_00995250, FUN_00997bcc, FUN_00997f88) and
# nv_flat.py (FindFlatArea FUN_009b1a5c with FUN_009999cc, FUN_009b096c, FUN_009b7e58 and the
# colour transforms), checked word for word against the genuine through harness/nvh
# (analysis/noise/verify_levels.py, verify_flat_exact.py).
# =====================================================================
_F32 = np.float32
_EXTRAP = -1e6                   # "not measured" marker of the genuine model structs
# (the FindFlatArea size tables and the per-mode "luma" flags live in nv_flat: TABLES, LUMA_FLAGS)


def _cv_profile(e):
    """FUN_00997f88: coefficient of variation of a local-RMS map over 8 octaves (2x2 means),
    always relative to the mean of the full map; unmeasured octaves stay -1e6.
    (nv_band.cv_bm with the FUN_009ef3fc mean of the map; float64 array of the float32 values)"""
    import nv_band
    e = np.asarray(e, _F32)
    m = np.array([nv_band.mean_exact(e)], _F32)
    return nv_band.cv_bm(e[:, :, None], m)[:, 0].astype(np.float64)


def gquantile(v, q, div=4, small=True):
    """FUN_00997bcc: the genuine quantile. Starting at the mean, a threshold moves by +-std,
    halving and reversing its step up to 8 times (<= 100 counts of `x <= t`) until it brackets
    n*q; with <= 49 samples it is refined by linear interpolation between the neighbouring
    values. If n/div < max(1/(1-q), 10) (`small`), the result is raised to the normal-model
    quantile mean + probit(q)*std*c[min(n/div, 11)]. A constant input gives 0; q >= 1 gives the
    maximum (FUN_009efdd4); div must be >= 0 (div 0: n/div = 0).
    (nv_band.quantile: mean / std in the genuine's summation order, bit-exact)"""
    import nv_band
    return float(nv_band.quantile(v, q, flag=bool(small), div=div))


def band_model(plane, levels=5, border=2):
    """FUN_009980e8 (border 2, FindFlatArea) / the level part of FUN_00995250 (border 4,
    BuildNoiseModel): Laplacian pyramid band k = src_k - up2(down2(box3(src_k))), local RMS
    sqrt(box3(band^2)) (zero border), statistics over the map without a `border`-pixel rim
    (shrunk until >= 32 px remain). Levels that are too small are extrapolated as
    -0.5*|previous| (FUN_00995250: -f*|previous| with f = |q99_k / q99_k-1| of the last measured
    level clamped to [0.5, 1]; negative = invalid). Returns {"mean", "q99", "q50", "cv"}; the 99% /
    median use the genuine quantile search (gquantile). Bit-exact: nv_band.band_levels /
    nv_band.model_levels (values are the genuine's float32 words as float64). The genuine has only
    these two variants: border must be 2 or 4 and levels 1..5 (ValueError otherwise)."""
    import nv_band
    if border == 2:
        s = nv_band.band_levels(plane, levels)
    elif border == 4:
        s = nv_band.model_levels(plane, levels)
    else:
        raise ValueError("the genuine measures use border 2 (FUN_009980e8) or 4 (FUN_00995250)")
    m = nv_band.as_model(s)
    return {k: np.asarray(v[:levels], np.float64) for k, v in m.items()}


def _model_words(model):
    """{"mean", "q99", "q50", "cv"} -> the first 55 struct words (float32; missing levels -1e6)."""
    w = np.full(55, _F32(_EXTRAP), _F32)
    for key, off, width in (("mean", 0, 1), ("q99", 5, 1), ("q50", 10, 1), ("cv", 15, 8)):
        a = np.asarray(model[key], _F32).reshape(-1)[:5 * width]
        w[off:off + a.size] = a
    return w


def _flatness(model, luma):
    """FUN_009b096c: >= 1 when the local-RMS map is less uniform than noise: per level from 1
    (luma) / 2 (chroma), the larger of CV[j]/(c*0.6^(j-1)) (j = 1..7, c = 0.3 / 0.6) and
    (q99/q50)/1.8 (chroma 2.7); averaged over the measured levels (nv_flat.flatness, bit-exact)."""
    import nv_flat
    return float(nv_flat.flatness(_model_words(model), luma))


def is_clip(plane):
    """FUN_009c6f88 (IsClip, float images): histogram of 1/8-wide bins over the plane's own
    range (bin index rounded to nearest, frintn); the fraction in the most populated bin
    counts as clipped only when that bin holds more than half of the pixels or nearly all
    others lie on one side of it (a spike at the end of the distribution). A constant plane
    is fully clipped. Implemented once in nv_curve.is_clip_genuine (verified against the
    genuine on 16x16 blocks and whole frames)."""
    import nv_curve
    return float(nv_curve.is_clip_genuine(plane))


def profile_planes(rgb, mode=1):
    """DAT_0147df50 colour transforms of the profile: 0 RGB, 1 BT.601 Y with (R-Y)*0.713 and
    (B-Y)*0.565 (FUN_0097b5d0), 2 mean of RGB with 0.75 * differences (FUN_0097b718), 3 RGB as well
    (the table's fourth entry is the copy FUN_0097b5a4); other values are treated as 1.
    (nv_flat.transform, bit-exact: fused luma on the first n & ~3 pixels as the genuine)"""
    import nv_flat
    R, G, B = (np.asarray(rgb[..., c], _F32) for c in range(3))
    return nv_flat.transform(R, G, B, mode if mode in (0, 2, 3) else 1)


def _ffa_candidates(luma, r, step, nb=16, per_bin=4):
    """FUN_009999cc: pre-score = sqrt(box_r((box3(luma - box_step(luma)))^2)), then greedily the
    lowest-scoring centres (inside an r border), each masking its window; at most `per_bin`
    centres per 1/`nb` brightness bin of the window mean. -> [(x, y, mean brightness)]
    (nv_flat.candidates, bit-exact; FindFlatArea passes r, step >= 1, a radius of 0 is a copy
    as in the genuine, negative radii are rejected)."""
    import nv_flat
    xs, ys, means, scores = nv_flat.candidates(luma, r, r, step, nb, per_bin)
    out = []
    for x, y, m, s in zip(xs, ys, means, scores):
        if s < 0:
            break
        out.append((int(x), int(y), float(m)))
    return out


def find_flat_area_genuine(rgb, noise_at, mode=1, cancel=None, curve=None):
    """FindFlatArea (FUN_009b1a5c) as Auto Profile calls it (FUN_0014e4a4, param_3 = 0).
    rgb: (H, W, 3) float 0..255; noise_at(b): expected noise at full-range luma b (the
    genuine uses its EstimateNoiseLevelCurve here). Candidates come from an s x s box-reduced
    luma (s >= 8, reduced size <= 2^18 px); for each size of the table that fits, every
    candidate square is scored as
      (2*max(2 - mean L_k/L_k+1, 0) + 1) * size factor * (4*n_clip + 1) * prod(flatness)
      * (sum_k 2^k L_k / n_levels) / noise_at(b) * (|b - 96|/512 + 1) * (2 if any clip > 0.1)
    with L_k = sum over the 3 profile planes of the band_model mean. Lowest score wins.
    Returns (x, y, w, h, status): 0 good, 2 small (< 128), 3 not flat (a flatness >= 2),
    4 clipped, 1 nothing found, 5 cancelled (cancel / curve: see nv_flat). Bit-exact port with the genuine's float operation order:
    nv_flat.find_flat_area_genuine (frames with no fitting square, < 48 px, make the genuine
    crash; the port returns status 1 there). mode 3 transforms like 0 (the genuine's table entry
    3 is the copy); other values are treated as 1."""
    import nv_flat
    return nv_flat.find_flat_area_genuine(rgb, noise_at, mode if mode in (0, 1, 2, 3) else 1, cancel=cancel,
                                          curve=curve)
