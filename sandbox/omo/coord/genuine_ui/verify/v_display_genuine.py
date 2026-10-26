"""Verifier: recompute YCrCb panes from the genuine RGB panes (shots/g_rgb.png) with nv_ui_qt.display,
compare to shots/g_ycc2.png. Own alignment: panes found as non-(89,89,89) runs; per-pane integer
shift search (+-4 px) on gradients."""
import os, sys
import numpy as np
from PIL import Image
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, *[".."] * 5))
sys.path.insert(0, os.path.join(ROOT, "repro"))
from nv_ui_qt import display as D
S = os.path.join(HERE, "..", "shots")
X0 = (18, 1014, 2010); PW, PY0, PH = 990, 1506, 604
M = 6   # margin cut after shifts

def panes(f):
    a = np.asarray(Image.open(os.path.join(S, f)).convert("RGB")).astype(np.float32)
    return [a[PY0:PY0 + PH, x:x + PW] for x in X0]

def grad(p):
    g = p.mean(2) if p.ndim == 3 else p
    return np.abs(np.diff(g, axis=1))[:-1] + np.abs(np.diff(g, axis=0))[:, :-1]

def best_shift(ref, mov, r=4):
    """shift (dy, dx) such that mov[y+dy, x+dx] ~ ref[y, x] (gradient correlation)."""
    gr, gm = grad(ref), grad(mov)
    best = None
    for dy in range(-r, r + 1):
        for dx in range(-r, r + 1):
            a = gr[M:-M, M:-M]; b = gm[M + dy:gm.shape[0] - M + dy, M + dx:gm.shape[1] - M + dx]
            c = np.corrcoef(a.ravel(), b.ravel())[0, 1]
            if best is None or c > best[0]:
                best = (c, dy, dx)
    return best

def crop(p, dy, dx):
    return p[M + dy:p.shape[0] - M + dy, M + dx:p.shape[1] - M + dx]

def main():
    R, G, B = panes("g_rgb.png")
    Yp, Crp, Cbp = panes("g_ycc2.png")
    # mask out labels (bottom-right 60x40)
    mask = np.ones((PH - 2 * M, PW - 2 * M), bool); mask[-40:, -60:] = False
    sG = best_shift(R, G); sB = best_shift(R, B)
    print("RGB pane shifts vs R pane: G", sG, "B", sB)
    rgb = np.stack([crop(R, 0, 0)[..., 0], crop(G, sG[1], sG[2])[..., 1], crop(B, sB[1], sB[2])[..., 2]], -1)
    # sanity: R pane non-R channels follow the (A) mapping of the R kind: G = B = v*201/256 + ... -> check
    gR, oR = D.map_a("R", 50, 0, 255)
    pred_R = D.apply(crop(R, 0, 0)[..., 0], gR, oR)
    print("R pane G/B channels vs (A) R-kind prediction from its R channel: MAE",
          float(np.abs(pred_R[..., 1:].astype(float) - crop(R, 0, 0)[..., 1:])[mask].mean()))
    Y, Cr, Cb = D.ycc(rgb)
    out = {}
    for name, pane, v in (("Y", Yp, Y), ("Cr", Crp, Cr), ("Cb", Cbp, Cb)):
        pred = D.render_simple(name, rgb, 50, (0.0, 255.0)).astype(np.float32)
        # align observed pane to R pane using correlation with the prediction channel 0
        best = None
        for dy in range(-4, 5):
            for dx in range(-4, 5):
                o = crop(pane, dy, dx)
                e = np.abs(o - pred)[mask].mean()
                if best is None or e < best[0]:
                    best = (e, dy, dx)
        o = crop(pane, best[1], best[2])
        per_ch = [float(np.abs(o[..., c] - pred[..., c])[mask].mean()) for c in range(3)]
        # fit: observed channel vs component value
        fits = []
        for c in range(3):
            A = np.stack([v[mask], np.ones(mask.sum())], 1)
            k, *_ = np.linalg.lstsq(A, o[..., c][mask], rcond=None)
            fits.append(tuple(round(float(t), 4) for t in k))
        g, off = (D.map_a("Y", 50, 0, 255) if name == "Y" else D.map_c(name, 50))
        print(f"{name}: shift {best[1:]} MAE all {best[0]:.3f} per channel {[round(x, 3) for x in per_ch]}; "
              f"fit (slope, icpt) {fits}; formula {[(round(float(a), 4), round(float(b), 3)) for a, b in zip(g, off)]}")
        # blurred MAE (5x5) to remove resampling noise
        from scipy.ndimage import uniform_filter
        bo = uniform_filter(o, (5, 5, 1)); bp = uniform_filter(pred, (5, 5, 1))
        print(f"   5x5-blurred MAE {float(np.abs(bo - bp)[mask].mean()):.3f}")
        out[name] = best[0]
    return out

if __name__ == "__main__":
    main()
