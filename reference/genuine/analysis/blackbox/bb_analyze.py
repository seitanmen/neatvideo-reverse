#!/usr/bin/env python3
"""Black-box comparison: test_v02 (input) vs test_v02_reduce (genuine Neat output).
Native YUV420 planes, 32 frames from n=296."""
import sys
import numpy as np
from scipy.ndimage import convolve1d, uniform_filter

SP = sys.argv[1]
W, H, N = 1920, 1080, 32
FS = W * H * 3 // 2

def load(name):
    raw = np.fromfile(f"{SP}/{name}", np.uint8).reshape(N, FS)
    Y = raw[:, :W * H].reshape(N, H, W).astype(np.float64)
    U = raw[:, W * H:W * H * 5 // 4].reshape(N, H // 2, W // 2).astype(np.float64)
    V = raw[:, W * H * 5 // 4:].reshape(N, H // 2, W // 2).astype(np.float64)
    return Y, U, V

iY, iU, iV = load("test_v02_296.yuv")
oY, oU, oV = load("test_v02_reduce_296.yuv")

print("== A. frame alignment (MSE of out[k] vs in[k+d])")
for d in range(-2, 3):
    m = [((oY[k] - iY[k + d]) ** 2).mean() for k in range(4, 28)]
    print(f"  d={d:+d} mse={np.mean(m):.3f}")

print("== B. global stats (mean in/out)")
for nm, a, b in [("Y", iY, oY), ("U", iU, oU), ("V", iV, oV)]:
    print(f"  {nm}: in={a.mean():.3f} out={b.mean():.3f} diff={b.mean()-a.mean():+.4f} "
          f"per-frame diff std={np.std([(b[k]-a[k]).mean() for k in range(N)]):.4f}")

print("== C. spatial coverage of processing (mean |out-in| over frames, Y) by region")
D = np.abs(oY - iY).mean(0)
print(f"  overall {D.mean():.3f}")
for (y0, y1, x0, x1, lab) in [(0, 40, 0, W, "top40"), (H - 40, H, 0, W, "bot40"),
                              (0, H, 0, 60, "left60"), (0, H, W - 60, W, "right60"),
                              (40, 1040, 60, 1860, "center"),
                              (0, 1000, 0, 1800, "1800x1000 TL"), (1000, H, 0, W, "y>=1000"),
                              (0, H, 1800, W, "x>=1800")]:
    print(f"  {lab:14s} {D[y0:y1, x0:x1].mean():.3f}")
# coarse map 9x16
cm = D[:1080, :1920].reshape(9, 120, 16, 120).mean((1, 3))
print("  coarse |diff| map (9x16 blocks):")
for r in cm:
    print("   " + " ".join(f"{v:4.1f}" for v in r))

# a trous (B3 spline) undecimated decomposition
B3 = np.array([1, 4, 6, 4, 1], np.float64) / 16
def atrous(img, L):
    bands, c = [], img
    for j in range(L):
        k = np.zeros(4 * 2 ** j + 1); k[::2 ** j] = B3
        s = convolve1d(convolve1d(c, k, 0, mode="reflect"), k, 1, mode="reflect")
        bands.append(c - s)
        c = s
    bands.append(c)
    return bands

L = 6
print("== D. per-band transfer (a-trous B3, level 1=finest). frames 8..23, central crop")
cy, cx = slice(60, 1020), slice(60, 1860)
cyc, cxc = slice(30, 510), slice(30, 930)
for nm, a, b, sy, sx in [("Y", iY, oY, cy, cx), ("U", iU, oU, cyc, cxc), ("V", iV, oV, cyc, cxc)]:
    acc = np.zeros((L + 1, 4))
    for k in range(8, 24):
        ba, bb = atrous(a[k], L), atrous(b[k], L)
        for j in range(L + 1):
            x, y = ba[j][sy, sx], bb[j][sy, sx]
            if j == L:
                x, y = x - x.mean(), y - y.mean()
            acc[j] += [(x * x).mean(), (y * y).mean(), ((x - y) ** 2).mean(), (x * y).mean()]
    acc /= 16
    print(f"  [{nm}] lvl  E_in    E_out   E_removed  out/in(rms)  gain=<io>/<ii>")
    for j in range(L + 1):
        ei, eo, er, eio = acc[j]
        print(f"   {('L%d' % (j + 1)) if j < L else 'res'}  {ei:8.3f} {eo:8.3f} {er:8.3f}   {np.sqrt(eo / ei):6.3f}      {eio / ei:6.3f}")

print("== E. noise estimation in input")
# temporal difference in static areas: static = low |in[k]-in[k+1]| after heavy blur
def blur(x, s=9):
    return uniform_filter(x, s)
d_stack, mask_stack = [], []
for k in range(8, 24):
    d = iY[k + 1] - iY[k]
    mot = np.abs(blur(iY[k + 1], 15) - blur(iY[k], 15))
    d_stack.append(d); mask_stack.append(mot < 0.5)
frac = np.mean([m.mean() for m in mask_stack])
print(f"  static fraction (blurred |dt|<0.5): {frac:.3f}")
# per band noise of temporal difference -> sigma_band = sqrt(E[d_band^2]/2)
for nm, src, sy, sx in [("Y", iY, cy, cx), ("U", iU, cyc, cxc), ("V", iV, cyc, cxc)]:
    sig = np.zeros(L); cnt = 0
    for k in range(8, 24):
        if nm == "Y":
            msk = mask_stack[k - 8]
        else:
            msk = mask_stack[k - 8][::2, ::2]
        bd = atrous(src[k + 1] - src[k], L)
        for j in range(L):
            sig[j] += (bd[j][msk] ** 2).mean()
        cnt += 1
    sig = np.sqrt(sig / cnt / 2)
    print(f"  [{nm}] temporal-diff per-band noise sigma (static px): " + " ".join(f"L{j+1}={s:.3f}" for j, s in enumerate(sig)))
    if nm == "Y":
        # white-noise reference ratio of a-trous band energies (unit-variance white noise)
        rng = np.random.default_rng(0)
        wn = atrous(rng.standard_normal((512, 512)), L)
        ref = np.array([wn[j].std() for j in range(L)])
        print("     white-noise band std per unit sigma: " + " ".join(f"{r:.3f}" for r in ref))
        print("     -> equivalent white sigma per band:  " + " ".join(f"{s/r:.3f}" for s, r in zip(sig, ref)))

# compare with nvfilter's estimator (min-var 32x32 + HH1 MAD, Haar)
sys.path.insert(0, "/Users/seitan/Desktop/work/reverse/repro")
import nvfilter as nv
yk = iY[16]
flat = nv.find_flat(yk)
import pywt
hh = pywt.wavedec2(yk[flat], "db1", level=1)[-1][2]
print(f"  nvfilter-style flat={flat} HH1-MAD sigma = {np.median(np.abs(hh))/0.6745:.3f}, patch mean={yk[flat].mean():.1f}")
# global HH1 MAD over the frame
hh_full = pywt.wavedec2(yk, "db1", level=1)[-1][2]
print(f"  whole-frame HH1 MAD sigma = {np.median(np.abs(hh_full))/0.6745:.3f}")

print("== F. noise vs brightness (temporal diff, static px, Y full-band) & genuine residual")
yb = (iY[8:24] + iY[9:25]) / 2
dd = iY[9:25] - iY[8:24]
ms = np.stack(mask_stack)
odd = oY[9:25] - oY[8:24]
edges = [16, 40, 64, 96, 128, 160, 192, 235]
for lo, hi in zip(edges[:-1], edges[1:]):
    sel = ms & (yb >= lo) & (yb < hi)
    if sel.sum() < 2000:
        continue
    si = np.sqrt((dd[sel] ** 2).mean() / 2)
    so = np.sqrt((odd[sel] ** 2).mean() / 2)
    print(f"  Y[{lo:3d},{hi:3d}) n={sel.sum():8d} in_sigma_t={si:.3f} out_sigma_t={so:.3f} ratio={so/si:.3f}")

print("== G. temporal residual over whole frame (frame-diff energy per band, Y)")
for tag, src in [("in", iY), ("out", oY)]:
    e = np.zeros(L + 1)
    for k in range(8, 24):
        bd = atrous(src[k + 1] - src[k], L)
        for j in range(L + 1):
            e[j] += (bd[j][cy, cx] ** 2).mean()
    print(f"  {tag:3s} " + " ".join(f"{np.sqrt(v/16):.3f}" for v in e))

print("== H. local gain vs local activity (Y L1+L2) : does genuine adapt to local structure?")
for k in [16]:
    ba, bb = atrous(iY[k], L), atrous(oY[k], L)
    hf_in = ba[0] + ba[1]; hf_out = bb[0] + bb[1]
    act = np.sqrt(uniform_filter(hf_in ** 2, 9))
    qs = np.quantile(act[cy, cx], [0, .2, .4, .6, .8, .95, 1])
    for lo, hi in zip(qs[:-1], qs[1:]):
        sel = (act >= lo) & (act < hi)
        sel[:60] = sel[-60:] = False; sel[:, :60] = sel[:, -60:] = False
        g = (hf_in[sel] * hf_out[sel]).sum() / (hf_in[sel] ** 2).sum()
        print(f"  act[{lo:6.2f},{hi:6.2f}) gain={g:.3f} rms_out/in={np.sqrt((hf_out[sel]**2).mean()/(hf_in[sel]**2).mean()):.3f}")
