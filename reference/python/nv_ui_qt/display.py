"""Viewer display of the genuine Neat Video 6 (static analysis, sandbox/omo/coord/genuine_ui/STATIC.md §1);
numpy only, no Qt.

Colour conversion (0x97b5d0, colour mode 1 = nv_flat.transform mode 1): Y = 0.2989 R + 0.5866 G + 0.1145 B,
Cr = (R - Y) * 0.71316504, Cb = (B - Y) * 0.56465274 on the displayed RGB (0..255; the stream's matrix
is not used: the genuine computes on the RGB the host passes).

Pane mapping FUN_00051130: out = sat_u8(round(gain_c * v + off_c)) per output channel c, the value v of
the pane's component written to all three channels; [lo_c, hi_c] = the output range of the pane kind
(RANGES).  b = viewer brightness 0..100 (50 = identity for the A kind):
  (A) Normal / R / G / B / Y (ids < 5): mn, mx = min / max over all channels of the source frame,
      clamped to +-2048; b > 50: f = (b - 50) / 50, input [f mn, f (mn + 0.33 (mx - mn)) + (1 - f) 255],
      output lower end + 32 f; b <= 50: f = (50 - b) / 50, input [f (mx - 0.33 (mx - mn)), f mx +
      (1 - f) 255], output upper end - 32 f; [in_lo - 0.5, in_hi + 0.5] maps linearly onto the range.
  (B) Frequencies / Enhanced (ids 7..18): s = b / 75 + 1/3 (b <= 50) or 0.04 b - 1; out = m_c + s h_c
      (2 (v - L) / (H - L) - 1) with the pane's display range (L, H) (L == H: (L + 1, H - 1)) and the
      centre / half-width m_c, h_c of the output range.
  (C) YCrCb Cr / Cb (ids 5, 6): out = m_c + (b / 50) h_c v / 64.5.
The genuine adds a dither of about +-0.25 (a 16384-entry table, seed 0x93f02c74) before rounding; it is
left out here (rint without dither).

Frequency image FUN_009987ac per plane (BuildFreqImage 0x9b0b68), with the genuine pieces that nv_band
already ports bit for bit: x_{k+1} = down2(box3(x_k)) (FUN_009944c0 r = 1 replicated borders, rows then
columns = nv_lines.box_rows / box_cols; FUN_009f15b4 = nv_pixnoise.down2), band_k = x_k - up2(x_{k+1})
(FUN_00990da8 = nv_pixnoise.band_from), k = 0..4; up^k = FUN_00993df0 (nv_pixnoise.up2) back to full size.
  Frequencies: High / Mid / Low = up^k(band_k), k = 0, 1, 2.
  Enhanced: S = sum_k w_k up^k(b~_k) (the low pass x_5 is not added); with emphasis (Prepare page, outside
  Profile Check) b~ = b / ((box3(box3(b^2)))^(1/4) + 0.01) (FUN_009ed2f0, two FUN_009944c0 r = 1 passes,
  FUN_009ed3e0 twice, FUN_009ee04c) and w_k = 1.25^k, without (Adjust page) b~ = b, w_k = 1.
  The images are 16-bit (depth 1): rint(16 x) saturated to int16 (NaN -> 0), i.e. a 1/16 grid.
Display range FindFreqLevs FUN_009b2814 per plane: q30 / q70 from the histogram of the 16-bit values
(FUN_0099caa8: the first value whose cumulative count exceeds (int)(n q)) and the normal model g = mu +
sigma probit(q) (nv_band.probit = FUN_009d3e40); if 20 |q70 - q30| < |g70 - g30| the q are averaged with
the g; d = max(|q70 - q30|, m) (m = 0.25 Frequencies, 1.0 Enhanced); range (q30 + q70) / 2 +- 4 d (+- 8 d
for the Enhanced Cr / Cb); the Enhanced Y is further limited to its 1% / 99% values when they differ by
more than 0.01.
"""
import math

import numpy as np

import nv_band as nb
import nv_flat as nf
import nv_lines as nl
import nv_pixnoise as npx

F32 = np.float32

# output range (lower RGB, upper RGB) of a pane kind (table of FUN_00051130)
RANGES = {"Y": ((-0.5, -0.5, -0.5), (255.5, 255.5, 255.5)),
          "R": ((-0.5, -0.5, -0.5), (255.5, 200.5, 200.5)),
          "G": ((-0.5, -0.5, -0.5), (200.5, 255.5, 200.5)),
          "B": ((-0.5, -0.5, -0.5), (200.5, 200.5, 255.5)),
          "Cr": ((31.5, -0.5, -0.5), (255.5, 215.5, 225.5)),
          "Cb": ((-0.5, -0.5, 15.5), (235.5, 225.5, 255.5))}


def ycc(rgb):
    """(Y, Cr, Cb) float32 planes of an (H, W, 3) RGB image (colour mode 1, FUN_0097b5d0)."""
    f = np.asarray(rgb, F32)
    return nf.transform(np.ascontiguousarray(f[..., 0]), np.ascontiguousarray(f[..., 1]),
                        np.ascontiguousarray(f[..., 2]), 1)


def map_a(kind, b, mn, mx):
    """(gain[3], offset[3]) of the (A) mapping (Normal / R / G / B / Y)."""
    lo_o, hi_o = (np.asarray(v, F32) for v in RANGES[kind])
    mn, mx = F32(max(mn, -2048.0)), F32(min(mx, 2048.0))
    b = int(b)
    if b > 50:
        f = F32((b - 50) / 50.0)
        lo, hi = f * mn, f * (mn + (mx - mn) * F32(0.33))
    else:
        f = F32((50 - b) / 50.0)
        lo, hi = f * (mx + (mx - mn) * F32(-0.33)), mx * f
    hi = F32(hi + (F32(1.0) - f) * F32(255.0))
    lo = F32(lo)
    if hi == lo:
        return np.ones(3, F32), lo_o
    lo = F32(lo - F32(0.5))
    span = F32(F32(hi + F32(0.5)) - lo)
    m, h = (hi_o + lo_o) * F32(0.5), (hi_o - lo_o) * F32(0.5)
    if b > 50:
        bot, top = m + f * F32(32.0) - h, m + h
    else:
        bot, top = m - h, (F32(1.0) - f) * (m + h) + f * (m + h - F32(32.0))
    gain = (top - bot) / span
    return gain.astype(F32), (bot - lo * gain).astype(F32)


def map_b(kind, b, L, H):
    """(gain[3], offset[3]) of the (B) mapping (Frequencies / Enhanced) for the display range (L, H)."""
    lo_o, hi_o = (np.asarray(v, F32) for v in RANGES[kind])
    b = int(b)
    s = F32(b * F32(0.013333334) + F32(0.33333334)) if b <= 50 else F32(b * F32(0.04) - F32(1.0))
    L, H = F32(L), F32(H)
    if L == H:
        L, H = F32(L + 1), F32(H - 1)
    span = F32(H - L)
    m, h = (hi_o + lo_o) * F32(0.5), (hi_o - lo_o) * s * F32(0.5)
    bot = m - h
    gain = ((m + h) - bot) / span
    return gain.astype(F32), (bot - gain * L).astype(F32)


def map_c(kind, b):
    """(gain[3], offset[3]) of the (C) mapping (Cr / Cb of YCrCb)."""
    lo_o, hi_o = (np.asarray(v, F32) for v in RANGES[kind])
    s = F32(int(b) / 50.0)
    m, h = (hi_o + lo_o) * F32(0.5), (hi_o - lo_o) * s * F32(0.5)
    bot = m - h
    gain = ((m + h) - bot) / F32(129.0)
    return gain.astype(F32), (bot + gain * F32(64.5)).astype(F32)


def apply(v, gain, off):
    """uint8 (H, W, 3): v (H, W) replicated, or (H, W, 3) per channel, times gain plus offset, rounded."""
    v = np.asarray(v, F32)
    if v.ndim == 2:
        v = v[..., None]
    with np.errstate(invalid="ignore", over="ignore"):
        out = np.rint(v * gain + off)
    return np.clip(np.nan_to_num(out, nan=0.0), 0, 255).astype(np.uint8)


def minmax(rgb):
    a = np.asarray(rgb)
    return float(a.min()), float(a.max())


# ------------------------------------------------------------------ frequency images
def _box3(x):
    return nl.box_cols(nl.box_rows(x, 1, 0), 1, 0)


def _up_to(x, shape_chain):
    """up2 through the level sizes of shape_chain (from the next larger level down to level 0)."""
    for H2, W2 in shape_chain:
        Hc, Wc = x.shape
        x = npx.up2(x, W2 - 2 * Wc, H2 - 2 * Hc)
    return x


def to16(x):
    """The 16-bit image type: rint(16 x) saturated to int16, NaN -> 0 (as FUN_009f0554)."""
    with np.errstate(invalid="ignore", over="ignore"):
        v = np.rint(np.nan_to_num(np.asarray(x, F32), nan=0.0) * F32(16.0))
    return np.clip(v, -32768, 32767).astype(np.int16)


def decompose(x, enhanced, emphasis=True):
    """FUN_009987ac on one plane: Enhanced -> int16 S; Frequencies -> [High, Mid, Low] int16."""
    x = np.ascontiguousarray(x, F32)
    shapes = [x.shape]
    freq, S, w = [], None, F32(1.0)
    xk = x
    with np.errstate(invalid="ignore", over="ignore", divide="ignore"):
        for k in range(5):
            Hk, Wk = xk.shape
            if Hk < 4 or Wk < 4:                 # too small to split further (the genuine needs >= 48 px)
                break
            lo = npx.down2(_box3(xk))
            band = npx.band_from(lo, xk, Wk - 2 * lo.shape[1], Hk - 2 * lo.shape[0])
            chain = shapes[::-1][1:] if k else []
            if not enhanced:
                if k < 3:
                    freq.append(to16(_up_to(band, chain)))
            else:
                if emphasis:
                    r = _box3(_box3(band * band))
                    r = np.sqrt(np.abs(np.sqrt(np.abs(r))))
                    band = npx.ratio_eps_bm(band[..., None], r[..., None], F32(0.01))[..., 0]
                up = _up_to(band, chain)
                S = up * w if S is None else (S + w * up).astype(F32)
                if emphasis:
                    w = F32(w * F32(1.25))
            shapes.append(lo.shape)
            xk = lo
    if not enhanced:
        while len(freq) < 3:
            freq.append(np.zeros(x.shape, np.int16))
        return freq
    return to16(np.zeros(x.shape, F32) if S is None else S)


def hist_levels(v16, qa, qb):
    """FUN_0099caa8 on an int16 image: (value at qa, value at qb, normal-model qa, normal-model qb)."""
    v = np.asarray(v16, np.int16).ravel()
    n = v.size
    cnt = np.bincount(v.astype(np.int64) + 32768, minlength=65536)
    cum = np.cumsum(cnt)
    ta, tb = int(F32(n * F32(qa))), int(F32(n * F32(qb)))
    ia = int(np.searchsorted(cum, ta, side="right"))
    ib = int(np.searchsorted(cum, tb, side="right"))
    vals = np.arange(-32768, 32768, dtype=np.float64)
    s = float(np.dot(cnt, vals))
    mi = int(s / n) if n else 0                       # integer mean (C division toward zero)
    var = float(np.dot(cnt, (vals - mi) ** 2)) / max(n - 1, 1)
    mu, sd = F32(F32(s) / F32(n) * F32(0.0625)), F32(math.sqrt(var) * 0.0625)
    return (F32((min(ia, 65535) - 32768) * 0.0625), F32((min(ib, 65535) - 32768) * 0.0625),
            F32(mu + sd * nb.probit(qa)), F32(mu + sd * nb.probit(qb)))


def freq_levels(planes16, enhanced):
    """FUN_009b2814: display ranges [(L, H)] of the three planes."""
    m = F32(1.0) if enhanced else F32(0.25)
    out = []
    for i, p in enumerate(planes16):
        q30, q70, g30, g70 = hist_levels(p, 0.3, 0.7)
        d = F32(abs(q70 - q30))
        if d * F32(20.0) < abs(g70 - g30):
            q30, q70 = F32((q30 + g30) * F32(0.5)), F32((q70 + g70) * F32(0.5))
            d = F32(abs(q70 - q30))
        d = max(d, m)
        c = F32((q70 + q30) * F32(0.5))
        half = F32(d * F32(0.5) * F32(8.0))
        if enhanced and i > 0:
            half = F32(half + half)
        out.append([F32(c - half), F32(c + half)])
    if enhanced:
        p1, p99, _, _ = hist_levels(planes16[0], 0.01, 0.99)
        if abs(p99 - p1) > 0.01:
            out[0][0] = max(out[0][0], p1)
            out[0][1] = min(out[0][1], p99)
    return [tuple(float(v) for v in r) for r in out]


def freq_images(rgb, kind, emphasis=True):
    """kind 'Y+' = Enhanced (planes Y, Cr, Cb), or 'Y' / 'Cr' / 'Cb' = that channel's High / Mid / Low.
    Returns (three int16 planes, their display ranges)."""
    Y, Cr, Cb = ycc(rgb)
    if kind == "Y+":
        planes = [decompose(p, True, emphasis) for p in (Y, Cr, Cb)]
        return planes, freq_levels(planes, True)
    planes = decompose({"Y": Y, "Cr": Cr, "Cb": Cb}[kind], False)
    return planes, freq_levels(planes, False)


# ------------------------------------------------------------------ panes
def pane_kind(name):
    """Output-range kind of a pane name ('Y+', 'Cr High', 'R', ...)."""
    return name.split()[0].rstrip("+")


def render_simple(name, rgb, b, mnmx=None):
    """uint8 image of an (A) / (C) pane: 'rgb' (Normal, the composite), 'R', 'G', 'B', 'Y', 'Cr', 'Cb'."""
    mn, mx = mnmx if mnmx is not None else minmax(rgb)
    if name == "rgb":
        return apply(rgb, *map_a("Y", b, mn, mx))
    f = np.asarray(rgb, F32)
    if name in ("R", "G", "B"):
        return apply(f[..., "RGB".index(name)], *map_a(name, b, mn, mx))
    Y, Cr, Cb = ycc(f)
    if name == "Y":
        return apply(Y, *map_a("Y", b, mn, mx))
    return apply(Cr if name == "Cr" else Cb, *map_c(name, b))


def render_freq(names, planes16, levels, b):
    """uint8 images of the three (B) panes from freq_images()."""
    return [apply(p.astype(F32) * F32(0.0625), *map_b(pane_kind(n), b, *lv))
            for n, p, lv in zip(names, planes16, levels)]


def noise_level(rgb_crop):
    """Noise Level of a selection (FUN_009b01d4 via FUN_001576c0): sqrt(sum over levels k = 0..4 and
    R, G, B of mean_k^2), mean_k = word k of the FUN_00995250 model (nv_band.model_levels) of the crop
    plane (0..255, before colour conversion); summed in float32 level by level, R, G, B."""
    f = np.asarray(rgb_crop, F32)
    means = [nb.model_levels(np.ascontiguousarray(f[..., c]))[:5] for c in range(3)]
    tot = F32(0.0)
    for k in range(5):
        for c in range(3):
            tot = F32(tot + F32(means[c][k] * means[c][k]))
    return float(np.sqrt(tot))


def not_uniform(rgb_crop):
    """(names of the non-uniform components, names of the clipped R / G / B) of a selection: the crop in
    colour mode 1, nv_band.band_levels -> nv_flat.flatness >= 2.0 (luma flags Y 1, Cr 0, Cb 0), and
    IsClip (nv_curve.is_clip_genuine) > 0.33 of R / G / B."""
    import nv_curve
    f = np.asarray(rgb_crop, F32)
    R, G, B = (np.ascontiguousarray(f[..., c]) for c in range(3))
    planes = nf.transform(R, G, B, 1)
    bad = [n for n, p, luma in zip(("Y", "Cr", "Cb"), planes, (True, False, False))
           if not nf.flatness(nb.band_levels(np.ascontiguousarray(p, F32)), luma) < F32(2.0)]
    clip = [n for n, p in zip("RGB", (R, G, B)) if nv_curve.is_clip_genuine(p) > 0.33]
    return bad, clip


NLC_SEPIA = np.array([1.0, 0.9, 0.75], F32)            # Noise Level Check colours fitted to the genuine
NLC_BLUE = np.array([0.3, 0.45, 0.5], F32)             # screens (sandbox/omo/coord/verify_quality/nlc/SPEC.md
NLC_BLUE_OFF = np.array([0.0, 0.0, 126.5], F32)        # O2; mean residual 0.35..1.33 levels on 4 screens)


def nlc_image(rgb, m):
    """Noise Level Check image: I = (R + G + B) / 3 of the frame, sepia S = I (1, 0.9, 0.75) where the
    mask m (0..1, H x W) is 0, blue B = I (0.3, 0.45, 0.5) + (0, 0, 126.5) where it is 1, (1 - m) S + m B
    in between (float RGB 0..255)."""
    f = np.asarray(rgb, F32)
    i = f.mean(axis=2, dtype=F32)[..., None]
    w = np.asarray(m, F32)[..., None]
    return np.clip((1 - w) * (i * NLC_SEPIA) + w * (i * NLC_BLUE + NLC_BLUE_OFF), 0, 255).astype(F32)
