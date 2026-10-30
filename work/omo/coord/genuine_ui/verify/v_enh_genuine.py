"""Verifier: Enhanced (Prepare YCrCb+) and Y Freq of test_v02 frame 0 (2x upscaled like Resolve's 3840x2160
timeline) vs the genuine panes. Own registration: scale + offset of the pane found by FFT NCC of the
plain Y render against shots/g_ycc2.png pane 1."""
import os, subprocess, sys
import numpy as np
from PIL import Image
from scipy.signal import fftconvolve
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, *[".."] * 5))
sys.path.insert(0, os.path.join(ROOT, "repro")); sys.path.insert(0, HERE)
from nv_ui_qt import display as D
import v_display_genuine as V

def frame0():
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", os.path.join(ROOT, "test_v02.mp4"), "-frames:v", "1",
                          "-vf", "scale=in_color_matrix=bt709:in_range=tv:flags=bicubic", "-pix_fmt", "rgb24",
                          "-f", "rawvideo", "-"], capture_output=True, check=True).stdout
    return np.frombuffer(raw, np.uint8).reshape(1080, 1920, 3)

def resize(a, s, resample=Image.BILINEAR):
    a = np.asarray(a)
    H, W = a.shape[:2]
    im = Image.fromarray(a.astype(np.uint8))
    return np.asarray(im.resize((max(1, round(W * s)), max(1, round(H * s))), resample)).astype(np.float32)

def ncc_locate(big, tpl):
    t = tpl - tpl.mean(); n = t.size
    c = fftconvolve(big, t[::-1, ::-1], mode="valid")
    s1 = fftconvolve(big, np.ones_like(t), mode="valid"); s2 = fftconvolve(big * big, np.ones_like(t), mode="valid")
    var = np.maximum(s2 - s1 * s1 / n, 1e-6)
    r = c / np.sqrt(var * (t * t).sum())
    i = np.unravel_index(np.argmax(r), r.shape)
    return float(r[i]), i

def main():
    f0 = frame0()
    up = resize(f0, 2.0, Image.BICUBIC)              # 3840x2160 like the Resolve timeline
    Yg = V.panes("g_ycc2.png")[0][..., 0]
    tpl = Yg[100:500, 100:850]                       # inner template (no label)
    Yr = D.render_simple("Y", up, 50, (0.0, 255.0))[..., 0]
    best = None
    for s in np.arange(0.30, 0.56, 0.01):
        sm = resize(Yr, s)
        if sm.shape[0] < tpl.shape[0] or sm.shape[1] < tpl.shape[1]:
            continue
        r, i = ncc_locate(sm, tpl)
        if best is None or r > best[0]:
            best = (r, s, i)
    r, s, (iy, ix) = best
    for s2 in np.arange(s - 0.01, s + 0.0101, 0.001):
        sm = resize(Yr, s2)
        r2, i2 = ncc_locate(sm, tpl)
        if r2 > r:
            r, s, (iy, ix) = r2, s2, i2
    oy, ox = iy - 100, ix - 100                     # pane origin in the scaled render
    print(f"registration: scale {s:.3f} of the 3840 frame (pane px per frame px), NCC {r:.4f}, pane origin ({oy},{ox})"
          f"; primary pane is 1280/3840 = 0.333")
    RS = Image.NEAREST if "--nearest" in sys.argv else Image.BILINEAR
    def pane_of(img):
        sm = resize(img, s, RS)
        return sm[oy:oy + V.PH, ox:ox + V.PW]
    ok = Yr is not None
    # Y plain sanity
    py = pane_of(np.repeat(Yr[..., None], 3, 2))
    msk = np.ones((V.PH, V.PW), bool); msk[-40:, -60:] = False
    print(f"plain Y (own decode/upscale) vs genuine Y pane MAE {np.abs(py[..., 0] - Yg)[msk].mean():.2f}")
    for kind, shot, names in (("Y+", "g_yccplus.png", ("Y+", "Cr+", "Cb+")), ("Y", "m_yfreq.png", ("Y High", "Y Mid", "Y Low"))):
        planes, levels = D.freq_images(up, kind, True)
        imgs = D.render_freq(list(names), planes, levels, 50)
        gp = V.panes(shot)
        # per-shot registration (the component panes centre on the selection, which differs per shot)
        from scipy.ndimage import uniform_filter
        ri = 0 if kind == "Y+" else 2
        big = uniform_filter(resize(imgs[ri], s, RS)[..., 0], 5)
        r3, (jy, jx) = ncc_locate(big, uniform_filter(gp[ri][..., 0], 5)[100:500, 100:850])
        oy2, ox2 = jy - 100, jx - 100
        msk = np.ones((V.PH, V.PW), bool); msk[-40:, -60:] = False; msk[:4] = False
        print(f"  per-shot registration NCC {r3:.3f} origin ({oy2},{ox2})")
        def pane_of(img, oy=oy2, ox=ox2):
            sm = np.pad(resize(img, s, RS), ((8, 8), (8, 8), (0, 0)), mode="edge")
            return sm[oy + 8:oy + 8 + V.PH, ox + 8:ox + 8 + V.PW]
        print(f"{shot}: display ranges {[(round(a, 2), round(b, 2)) for a, b in levels]}")
        for n, im, g in zip(names, imgs, gp):
            p = pane_of(im)
            row = []
            for c in range(3):
                a, b = p[..., c][msk], g[..., c][msk]
                row.append(f"ch{c}: ours {a.mean():6.1f}/{a.std():5.1f} genuine {b.mean():6.1f}/{b.std():5.1f} corr {np.corrcoef(a, b)[0, 1]:.3f}")
            print(f"  {n:8s} MAE {np.abs(p - g)[msk].mean():5.2f} | " + " | ".join(row))
            Image.fromarray(np.concatenate([p, g], 1).astype(np.uint8)).save(os.path.join(HERE, f"enh_{'near_' if RS == Image.NEAREST else ''}{n.replace(' ', '_').replace('+', 'p')}.png"))

if __name__ == "__main__":
    main()
