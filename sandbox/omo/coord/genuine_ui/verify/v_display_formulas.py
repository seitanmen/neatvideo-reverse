"""Verifier: independent float64 re-implementation of STATIC.md §1c (A)(B)(C) and §1d (pyramid, emphasis,
FindFreqLevs) compared with nv_ui_qt.display."""
import os, sys
import numpy as np
from scipy.ndimage import uniform_filter
from scipy.stats import norm
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.abspath(os.path.join(HERE, *[".."] * 5, "repro")))
from nv_ui_qt import display as D
RG = {"Y": ((-.5,) * 3, (255.5,) * 3), "R": ((-.5,) * 3, (255.5, 200.5, 200.5)), "G": ((-.5,) * 3, (200.5, 255.5, 200.5)),
      "B": ((-.5,) * 3, (200.5, 200.5, 255.5)), "Cr": ((31.5, -.5, -.5), (255.5, 215.5, 225.5)), "Cb": ((-.5, -.5, 15.5), (235.5, 225.5, 255.5))}

def A(kind, v, b, mn, mx):
    lo_o, hi_o = map(np.array, RG[kind]); mn, mx = max(mn, -2048), min(mx, 2048)
    if b > 50:
        f = (b - 50) / 50; lo, hi = f * mn, f * (mn + 0.33 * (mx - mn)) + (1 - f) * 255; lo_o = lo_o + 32 * f
    else:
        f = (50 - b) / 50; lo, hi = f * (mx - 0.33 * (mx - mn)), f * mx + (1 - f) * 255; hi_o = hi_o - 32 * f
    t = (v[..., None] - (lo - 0.5)) / ((hi + 0.5) - (lo - 0.5))
    return np.clip(np.rint(lo_o + t * (hi_o - lo_o)), 0, 255)

def B(kind, v, b, L, H):
    lo_o, hi_o = map(np.array, RG[kind]); m, h = (lo_o + hi_o) / 2, (hi_o - lo_o) / 2
    s = b / 75 + 1 / 3 if b <= 50 else b * 0.04 - 1
    if L == H: L, H = L + 1, H - 1
    return np.clip(np.rint(m + s * h * (2 * (v[..., None] - L) / (H - L) - 1)), 0, 255)

def C(kind, v, b):
    lo_o, hi_o = map(np.array, RG[kind]); m, h = (lo_o + hi_o) / 2, (hi_o - lo_o) / 2
    return np.clip(np.rint(m + (b / 50) * h * v[..., None] / 64.5), 0, 255)

def box3(x): return uniform_filter(x, 3, mode="nearest")
def down(x): H, W = x.shape; x = x[:H // 2 * 2, :W // 2 * 2]; return 0.25 * (x[0::2, 0::2] + x[0::2, 1::2] + x[1::2, 0::2] + x[1::2, 1::2])
def up1(lo, n, ax):
    lo = np.moveaxis(lo, ax, 0); p = np.concatenate([lo[:1], lo, lo[-1:]], 0)
    out = np.empty((2 * lo.shape[0],) + lo.shape[1:])
    out[0::2] = 0.75 * p[1:-1] + 0.25 * p[:-2]; out[1::2] = 0.75 * p[1:-1] + 0.25 * p[2:]
    if n > out.shape[0]: out = np.concatenate([out, out[-1:]], 0)
    return np.moveaxis(out, 0, ax)
def up(lo, shape): return up1(up1(lo, shape[0], 0), shape[1], 1)

def decompose(x, enhanced, emph):
    xs = [x.astype(np.float64)]; S = 0; freq = []; w = 1.0
    for k in range(5):
        lo = down(box3(xs[-1])); band = xs[-1] - up(lo, xs[-1].shape)
        if enhanced and emph:
            band = band / (np.abs(box3(box3(band * band))) ** 0.25 + 0.01)
        u = band
        for j in range(k, 0, -1): u = up(u, xs[j - 1].shape)
        if enhanced: S = S + w * u; w *= 1.25 if emph else 1.0
        elif k < 3: freq.append(u)
        xs.append(lo)
    return S if enhanced else freq

def main():
    rng = np.random.default_rng(7)
    img = np.clip(uniform_filter(rng.normal(128, 60, (180, 260, 3)), (9, 9, 1)) + rng.normal(0, 6, (180, 260, 3)), 0, 255).astype(np.float32)
    mn, mx = float(img.min()), float(img.max())
    worst = {}
    R, G, Bc = (img[..., i].astype(np.float64) for i in range(3))
    Y = 0.2989 * R + 0.5866 * G + 0.1145 * Bc; Cr = (R - Y) * 0.71316504; Cb = (Bc - Y) * 0.56465274
    for b in (0, 13, 37, 50, 51, 77, 100):
        for name, v in (("R", R), ("G", G), ("B", Bc), ("Y", Y)):
            got = D.render_simple(name, img, b, (mn, mx)).astype(float)
            worst[f"A {name}"] = max(worst.get(f"A {name}", 0), np.abs(got - A(name, v, b, mn, mx)).max())
        got = D.render_simple("rgb", img, b, (mn, mx)).astype(float)
        exp = np.stack([A("Y", img[..., c].astype(float), b, mn, mx)[..., c] for c in range(3)], -1)
        worst["A Normal"] = max(worst.get("A Normal", 0), np.abs(got - exp).max())
        for name, v in (("Cr", Cr), ("Cb", Cb)):
            got = D.render_simple(name, img, b, (mn, mx)).astype(float)
            worst[f"C {name}"] = max(worst.get(f"C {name}", 0), np.abs(got - C(name, v, b)).max())
        for L, H in ((-3.0, 4.5), (2.0, 2.0)):
            v = rng.normal(0, 3, (20, 30))
            got = D.apply(v.astype(np.float32), *D.map_b("Cb", b, L, H)).astype(float)
            worst["B Cb"] = max(worst.get("B Cb", 0), np.abs(got - B("Cb", v, b, L, H)).max())
    print("max |display - independent| per mapping over b in {0,13,37,50,51,77,100}:", {k: float(v) for k, v in worst.items()})
    # decomposition
    for enh, emph in ((True, True), (True, False), (False, False)):
        got = D.decompose(Y.astype(np.float32), enh, emph)
        exp = decompose(Y, enh, emph)
        if enh:
            g = got.astype(float) / 16; e = np.rint(exp * 16) / 16; c = (slice(24, -24), slice(24, -24))
            print(f"Enhanced emph={emph}: interior max |diff| {np.abs(g - e)[c].max():.4f}, full-frame max {np.abs(g - e).max():.4f}, "
                  f"corr {np.corrcoef(g.ravel(), e.ravel())[0, 1]:.6f}")
        else:
            for k, (g, e) in enumerate(zip(got, exp)):
                g = g.astype(float) / 16; c = (slice(24, -24), slice(24, -24))
                print(f"Freq level {k}: interior max |diff| {np.abs(g - np.rint(e * 16) / 16)[c].max():.4f}")
    # wrong-variant sensitivity: weights 1.25^(k+1) or 4 levels would differ?
    # FindFreqLevs independent
    p16 = D.decompose(Y.astype(np.float32), True, True)
    v = p16.astype(np.float64).ravel() / 16
    def q(vals, qq):
        s = np.sort(vals); return s[int(np.float32(len(s) * np.float32(qq)))]
    q30, q70 = q(v, .3), q(v, .7); mu, sd = v.mean(), v.std(ddof=1)
    g30, g70 = mu + sd * norm.ppf(.3), mu + sd * norm.ppf(.7)
    if 20 * abs(q70 - q30) < abs(g70 - g30): q30, q70 = (q30 + g30) / 2, (q70 + g70) / 2
    d = max(abs(q70 - q30), 1.0); c = (q30 + q70) / 2
    lo, hi = c - 4 * d, c + 4 * d
    p1, p99 = q(v, .01), q(v, .99)
    if abs(p99 - p1) > .01: lo, hi = max(lo, p1), min(hi, p99)
    got = D.freq_levels([p16, p16, p16], True)
    print(f"FindFreqLevs Enhanced Y: display {got[0]} independent ({lo:.4f}, {hi:.4f}); Cr slot (+-8d) {got[1]} indep ({c - 8 * d:.4f}, {c + 8 * d:.4f})")
    gotf = D.freq_levels([p16, p16, p16], False)
    d2 = max(abs(q70 - q30), 0.25)
    print(f"FindFreqLevs Freq: display {gotf[0]} independent ({c - 4 * d2:.4f}, {c + 4 * d2:.4f})")

if __name__ == "__main__":
    main()
