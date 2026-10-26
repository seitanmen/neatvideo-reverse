"""§5.1: the display module against the genuine screenshots (shots/g_rgb.png, g_ycc2.png, g_yccplus.png,
m_yfreq.png). The bottom panes of g_rgb / g_ycc2 show the same part of the frame (no selection, same
centre); the R / G / B panes' own channel is the component itself at brightness 50 (gain 1, offset 0),
so RGB is read back per pane pixel and the YCrCb panes are computed from it.

    cd repro && python3 ../sandbox/omo/coord/genuine_ui/impl/test_display.py
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "repro"))
SHOTS = os.path.join(HERE, "..", "shots")

import numpy as np  # noqa: E402
from PIL import Image  # noqa: E402

from nv_ui_qt import display as D  # noqa: E402

X0 = (18, 1014, 2010)      # bottom panes of the 3600x2250 screenshots (measured: flat separator columns)
PW, Y0, Y1 = 990, 1506, 2108
LABEL = (60, 50)           # bottom-right corner with the pane label, excluded


def panes(name):
    a = np.asarray(Image.open(os.path.join(SHOTS, name)).convert("RGB")).astype(np.float32)
    return [a[Y0:Y1, x:x + PW] for x in X0]


def mask():
    m = np.ones((Y1 - Y0, PW), bool)
    m[-LABEL[1]:, -LABEL[0]:] = False
    m[:4], m[-4:], m[:, :4], m[:, -4:] = False, False, False, False
    return m


def main():
    out = []
    pr, pg, pb = panes("g_rgb.png")
    rgb = np.stack([pr[..., 0], pg[..., 1], pb[..., 2]], -1)
    M = mask()
    # self-consistency of the R / G / B panes: the other two channels of each pane from the formula
    for nm, p in zip("RGB", (pr, pg, pb)):
        pred = D.render_simple(nm, rgb, 50).astype(np.float32)
        err = np.abs(pred - p)[M].mean(0)
        out.append(f"{nm} pane: |pred - genuine| per channel {np.round(err, 3).tolist()}")
    # YCrCb panes
    gy, gcr, gcb = panes("g_ycc2.png")
    for nm, g in zip(("Y", "Cr", "Cb"), (gy, gcr, gcb)):
        pred = D.render_simple(nm, rgb, 50).astype(np.float32)
        d = np.abs(pred - g)[M]
        out.append(f"YCrCb {nm} pane: MAE per channel {np.round(d.mean(0), 3).tolist()} "
                   f"(mean {d.mean():.3f}), 5x5-blurred MAE {blurred_mae(pred, g, M):.3f}")
        # linear fit of the genuine pane's channel 0 against the component value
        Y, Cr, Cb = D.ycc(rgb)
        v = {"Y": Y, "Cr": Cr, "Cb": Cb}[nm][M]
        for c in range(3):
            A = np.vstack([v, np.ones_like(v)]).T
            k, o = np.linalg.lstsq(A, g[..., c][M], rcond=None)[0]
            kp, op = np.linalg.lstsq(A, pred[..., c][M], rcond=None)[0]
            out.append(f"    ch{c}: genuine fit {k:.3f} v + {o:.2f}; formula {kp:.3f} v + {op:.2f}")
    # Enhanced / Frequencies: same RGB, statistics of the panes (genuine shown at 1/3 of the 3840 timeline
    # = 1.5 source pixels per screen pixel; here computed on the screen-pixel RGB, see the report)
    for shot, kind, names in (("g_yccplus.png", "Y+", ("Y+", "Cr+", "Cb+")),
                              ("m_yfreq.png", "Y", ("Y High", "Y Mid", "Y Low"))):
        gp = panes(shot)
        planes, lv = D.freq_images(rgb, kind, emphasis=True)
        imgs = D.render_freq(names, planes, lv, 50)
        for nm, g, p, l in zip(names, gp, imgs, lv):
            g, p = g[M], p.astype(np.float32)[M]
            out.append(f"{shot} {nm}: genuine mean {np.round(g.mean(0), 1).tolist()} sd "
                       f"{np.round(g.std(0), 1).tolist()} | repro mean {np.round(p.mean(0), 1).tolist()} sd "
                       f"{np.round(p.std(0), 1).tolist()} range ({l[0]:.3f}, {l[1]:.3f})")
        Image.fromarray(np.concatenate([np.concatenate([g.astype(np.uint8) for g in gp], 1),
                                        np.concatenate(imgs, 1)], 0)).save(
            os.path.join(HERE, f"cmp_display_{kind.replace('+', 'plus')}.png"))
    Image.fromarray(np.concatenate([np.concatenate([g.astype(np.uint8) for g in (gy, gcr, gcb)], 1),
                                    np.concatenate([D.render_simple(n, rgb, 50) for n in ("Y", "Cr", "Cb")], 1)],
                                   0)).save(os.path.join(HERE, "cmp_display_ycc.png"))
    print("\n".join(out))


def blurred_mae(a, b, M):
    from scipy.ndimage import uniform_filter
    fa = uniform_filter(a, (5, 5, 1))
    fb = uniform_filter(b, (5, 5, 1))
    return float(np.abs(fa - fb)[M].mean())


if __name__ == "__main__":
    main()


def source_compare():
    """Enhanced / Frequencies computed as the genuine does: on the 3840x2160 timeline frame (test_v02
    frame 0 converted to RGB with BT.709 limited range and up-scaled 2x bicubic, the Resolve up-scaling
    method is not known), then reduced 3x (box mean; 33.3% zoom on a 2x screen = 1/3 device pixel) and
    aligned with the genuine pane by the best integer offset of the Y pane."""
    import time
    import nvfilter as nv
    from denoise_video_v5 import probe, read_frames
    path = os.path.join(ROOT, "test_v02.mp4")
    info = probe(path)
    _, f0 = next(iter(read_frames(path, info["W"], info["H"], 0, 1, info["fps"])))
    rgb = np.clip(nv.yuv_to_rgb(tuple(p.astype(np.float32) for p in f0), "bt709", False), 0, 255)
    up = np.stack([np.asarray(Image.fromarray(np.ascontiguousarray(rgb[..., c]), "F").resize(
        (2 * rgb.shape[1], 2 * rgb.shape[0]), Image.BICUBIC)) for c in range(3)], -1)
    lines = []
    for shot, kind, names in (("g_yccplus.png", "Y+", ("Y+", "Cr+", "Cb+")),
                              ("m_yfreq.png", "Y", ("Y High", "Y Mid", "Y Low"))):
        t0 = time.perf_counter()
        planes, lv = D.freq_images(up, kind, emphasis=True)
        imgs = D.render_freq(names, planes, lv, 50)
        dt = time.perf_counter() - t0
        if os.environ.get("REDUCE") == "nearest":     # every 3rd pixel instead of the 3x3 mean
            small = [im[1::3, 1::3].astype(np.float32) for im in imgs]
        else:
            small = [im.reshape(720, 3, 1280, 3, 3).mean((1, 3)) for im in imgs]
        gp = panes(shot)
        g0 = gp[0][..., 0]
        best = None
        for oy in range(0, 720 - g0.shape[0] + 1, 2):
            for ox in range(0, 1280 - g0.shape[1] + 1, 2):
                e = np.abs(small[0][oy:oy + g0.shape[0], ox:ox + g0.shape[1], 0] - g0)[100:-100, 100:-100].mean()
                if best is None or e < best[0]:
                    best = (e, oy, ox)
        _, oy, ox = best
        M = mask()
        crops = []
        for nm, g, s, l in zip(names, gp, small, lv):
            c = s[oy:oy + g.shape[0], ox:ox + g.shape[1]]
            crops.append(c.astype(np.uint8))
            lines.append(f"{shot} {nm} (3840x2160 source, {dt:.1f} s, offset y{oy} x{ox}): genuine mean "
                         f"{np.round(g[M].mean(0), 1).tolist()} sd {np.round(g[M].std(0), 1).tolist()} | repro mean "
                         f"{np.round(c[M].mean(0), 1).tolist()} sd {np.round(c[M].std(0), 1).tolist()} | "
                         f"MAE {np.abs(c - g)[M].mean():.2f}, range ({l[0]:.3f}, {l[1]:.3f})")
        Image.fromarray(np.concatenate([np.concatenate([g.astype(np.uint8) for g in gp], 1),
                                        np.concatenate(crops, 1)], 0)).save(
            os.path.join(HERE, f"cmp_source_{kind.replace('+', 'plus')}_{os.environ.get('REDUCE', 'mean')}.png"))
    print("\n".join(lines))


if __name__ == "__main__" and "--source" in sys.argv:
    source_compare()
