#!/usr/bin/env python3
"""Compare current repro (v4c settings) vs proposed changes against genuine Neat output.
Works on native YUV420 planes (frames 296..327 extracted). Output frames k=8..23."""
import sys
import time
import numpy as np
from scipy.ndimage import convolve1d, uniform_filter, gaussian_filter, map_coordinates, zoom

SP = sys.argv[1]
MODE = sys.argv[2] if len(sys.argv) > 2 else "all"
sys.path.insert(0, "/Users/seitan/Desktop/work/reverse/repro")
import nvfilter as nv

W, H, N = 1920, 1080, 32
FS = W * H * 3 // 2
K0, K1, R = 8, 24, 2


def load(name):
    raw = np.fromfile(f"{SP}/{name}", np.uint8).reshape(N, FS)
    Y = raw[:, :W * H].reshape(N, H, W).astype(np.float32)
    U = raw[:, W * H:W * H * 5 // 4].reshape(N, H // 2, W // 2).astype(np.float32)
    V = raw[:, W * H * 5 // 4:].reshape(N, H // 2, W // 2).astype(np.float32)
    return Y, U, V


iY, iU, iV = load("test_v02_296.yuv")
gY, gU, gV = load("test_v02_reduce_296.yuv")


def psnr(a, b):
    return 10 * np.log10(255 ** 2 / ((a.astype(np.float64) - b) ** 2).mean())


def up(c):
    return np.repeat(np.repeat(c, 2, 0), 2, 1)


def down(c):
    return c.reshape(H // 2, 2, W // 2, 2).mean((1, 3))


B3 = np.array([1, 4, 6, 4, 1], np.float32) / 16


def atrous(img, L):
    bands, c = [], img
    for j in range(L):
        k = np.zeros(4 * 2 ** j + 1, np.float32); k[::2 ** j] = B3
        s = convolve1d(convolve1d(c, k, 0, mode="reflect"), k, 1, mode="reflect")
        bands.append(c - s); c = s
    bands.append(c)
    return bands


def report(tag, oY, oU, oV):
    py = np.mean([psnr(oY[i], gY[K0 + i]) for i in range(len(oY))])
    pu = np.mean([psnr(oU[i], gU[K0 + i]) for i in range(len(oY))])
    pv = np.mean([psnr(oV[i], gV[K0 + i]) for i in range(len(oY))])
    # band rms of (out - genuine) and temporal residual ratio
    eb = np.zeros(4)
    for i in range(0, len(oY), 4):
        bo, bg = atrous(oY[i], 3), atrous(gY[K0 + i], 3)
        for j in range(4):
            x = bo[j] - bg[j]
            eb[j] += (x[60:-60, 60:-60] ** 2).mean()
    eb = np.sqrt(eb / len(range(0, len(oY), 4)))
    td = np.sqrt(np.mean([((oY[i + 1] - oY[i]) ** 2).mean() for i in range(len(oY) - 1)]))
    tg = np.sqrt(np.mean([((gY[K0 + i + 1] - gY[K0 + i]) ** 2).mean() for i in range(len(oY) - 1)]))
    print(f"[{tag}] PSNR vs genuine Y={py:.2f} U={pu:.2f} V={pv:.2f} | "
          f"band rmsErr L1={eb[0]:.3f} L2={eb[1]:.3f} L3={eb[2]:.3f} low={eb[3]:.3f} | "
          f"frame-diff rms out={td:.3f} genuine={tg:.3f}", flush=True)


# ---------- baseline: input ----------
report("input", iY[K0:K1], iU[K0:K1], iV[K0:K1])

# ---------- current repro (v4c settings) ----------
def run_current():
    t0 = time.time()
    yc = [np.stack([iY[k], up(iU[k]), up(iV[k])], -1) for k in range(N)]
    flat = nv.find_flat(yc[16][..., 0])
    prof = nv.build_profile_yc(yc[16], flat)
    for ch in ("Cb", "Cr"):
        prof[ch]["sigma"] = max(prof[ch]["sigma"], 0.25 * prof["Y"]["sigma"])
    for ch in ("Y", "Cb", "Cr"):
        prof[ch]["sigma"] *= 4
        for j in prof[ch]["band_sigma"]:
            prof[ch]["band_sigma"][j] *= 4
    sigma = prof["Y"]["sigma"]
    wbase = float(np.clip(0.8 * sigma / 20.0, 0.1, 1.0))
    print(f"  current: profile sigma Y={sigma:.2f} wbase={wbase:.3f}", flush=True)
    bs = 32
    oY, oU, oV = [], [], []
    for k in range(K0, K1):
        acc = yc[k].copy(); norm = np.ones((H, W), np.float32)
        for dt in range(1, R + 1):
            for tt in (k - dt, k + dt):
                flow, sad = nv.estimate_motion(yc[tt][..., 0], yc[k][..., 0], bs)
                rel = np.exp(-np.maximum(sad - 2.0 * sigma, 0) / (4.0 * sigma + 1e-6))
                rel_f = np.pad(np.repeat(np.repeat(rel, bs, 0), bs, 1),
                               ((0, max(H - rel.shape[0] * bs, 0)), (0, max(W - rel.shape[1] * bs, 0))),
                               mode="edge")[:H, :W]
                w = wbase * rel_f[..., None]
                acc = acc + w * nv.warp_float(yc[tt], flow, bs)
                norm = norm + w[..., 0]
        tmp = acc / norm[..., None]
        sp = nv.spatial_filter_yc(tmp, prof, strength=1.0, luma=0.8, chroma=1.4)
        oY.append(sp[..., 0]); oU.append(down(sp[..., 1])); oV.append(down(sp[..., 2]))
    print(f"  current took {time.time() - t0:.0f}s", flush=True)
    return np.array(oY), np.array(oU), np.array(oV)


# ---------- proposed ----------
def block_flow(ref, cur, bs=16, rng=8):
    """Vectorised integer full-search on blurred frames, then smooth dense flow."""
    a, b = gaussian_filter(cur, 1.0), gaussian_filter(ref, 1.0)
    nby, nbx = H // bs, W // bs
    Hp, Wp = nby * bs, nbx * bs
    Pp = np.pad(b, rng, mode="edge")
    best = np.full((nby, nbx), np.inf, np.float32)
    fy = np.zeros((nby, nbx), np.float32); fx = np.zeros((nby, nbx), np.float32)
    cc = a[:Hp, :Wp]
    for dy in range(-rng, rng + 1):
        for dx in range(-rng, rng + 1):
            d = np.abs(cc - Pp[rng + dy:rng + dy + Hp, rng + dx:rng + dx + Wp]).reshape(nby, bs, nbx, bs).sum((1, 3))
            d += 0.02 * bs * bs * (abs(dy) + abs(dx))  # small zero-motion bias
            m = d < best
            best[m], fy[m], fx[m] = d[m], dy, dx
    # median-ish regularisation then smooth upsampling (avoids block-constant warps)
    from scipy.ndimage import median_filter
    fy, fx = median_filter(fy, 3), median_filter(fx, 3)
    FY = zoom(fy, (Hp / nby, Wp / nbx), order=1)
    FX = zoom(fx, (Hp / nby, Wp / nbx), order=1)
    FY = np.pad(FY, ((0, H - Hp), (0, W - Wp)), mode="edge")
    FX = np.pad(FX, ((0, H - Hp), (0, W - Wp)), mode="edge")
    return FY, FX


def warp(img, FY, FX):
    h, w = img.shape
    ys, xs = np.mgrid[0:h, 0:w].astype(np.float32)
    return map_coordinates(img, [np.clip(ys + FY, 0, h - 1), np.clip(xs + FX, 0, w - 1)], order=1, mode="nearest")


def noise_curve(Y0, Y1, nb=8):
    """Brightness-dependent temporal noise sigma (8 bins over 0..256, like EstimateNoiseLevelCurve),
    measured on static pixels from a frame pair; [1,2,1]/4 smoothed."""
    d = Y1 - Y0
    mot = np.abs(uniform_filter(Y1, 15) - uniform_filter(Y0, 15))
    m = mot < 0.5
    yb = (Y0 + Y1) / 2
    s = np.zeros(nb); valid = np.zeros(nb, bool)
    for i in range(nb):
        sel = m & (yb >= 256 / nb * i) & (yb < 256 / nb * (i + 1))
        if sel.sum() >= Y0.size / 1024:
            s[i] = np.sqrt((d[sel] ** 2).mean() / 2); valid[i] = True
    x = np.arange(nb)
    s[~valid] = np.interp(x[~valid], x[valid], s[valid])
    sm = s.copy(); sm[1:-1] = (s[:-2] + 2 * s[1:-1] + s[2:]) / 4
    return sm


def sigma_map(Y, curve):
    nb = len(curve)
    centers = 256 / nb * (np.arange(nb) + 0.5)
    return np.interp(Y, centers, curve).astype(np.float32)


def run_proposed(spatial=True, nbr_w=0.64, h=2.0, sp_k=1.0):
    t0 = time.time()
    curveY = noise_curve(iY[15], iY[16])
    cu = noise_curve(up(iU[15]), up(iU[16])); cv = noise_curve(up(iV[15]), up(iV[16]))
    print(f"  proposed: Y noise curve {np.round(curveY, 2)} U~{cu.mean():.2f} V~{cv.mean():.2f}", flush=True)
    oY, oU, oV = [], [], []
    flows = {}
    for k in range(K0, K1):
        sY = sigma_map(iY[k], curveY)
        accY = iY[k].copy(); accU = iU[k].copy(); accV = iV[k].copy()
        nY = np.ones((H, W), np.float32); nC = np.ones((H // 2, W // 2), np.float32)
        for dt in range(1, R + 1):
            for tt in (k - dt, k + dt):
                key = (tt, k)
                if key not in flows:
                    flows[key] = block_flow(iY[tt], iY[k])
                FY, FX = flows[key]
                wy = warp(iY[tt], FY, FX)
                # per-pixel similarity on 5x5 mean squared diff vs expected 2 sigma^2
                d2 = uniform_filter((wy - iY[k]) ** 2, 5)
                excess = np.maximum(d2 - 2 * sY ** 2, 0) / (h * 2 * sY ** 2 + 1e-6)
                w = nbr_w * np.exp(-excess).astype(np.float32)
                accY += w * wy; nY += w
                wc = down(w)
                FYc, FXc = down(FY) / 2, down(FX) / 2
                accU += wc * warp(iU[tt], FYc, FXc); accV += wc * warp(iV[tt], FYc, FXc)
                nC += wc
        tY, tU, tV = accY / nY, accU / nC, accV / nC
        if spatial:
            # residual noise after temporal averaging ~ sigma / sqrt(norm)
            out = []
            for img, sig, nrm in [(tY, sY, nY), (tU, None, nC), (tV, None, nC)]:
                if sig is None:
                    sig = np.full(img.shape, (cu.mean() if img is tU else cv.mean()), np.float32)
                sres = sig / np.sqrt(nrm)
                bands = atrous(img, 4)
                ref = atrous(np.ones((64, 64), np.float32) * 0 + np.random.default_rng(0).standard_normal((64, 64)).astype(np.float32), 4)
                wn = [0.892, 0.202, 0.085, 0.040]  # white-noise std per a-trous level (unit sigma)
                rec = bands[-1].copy()
                for j in range(4):
                    b = bands[j]
                    sn2 = (sp_k * sres * wn[j] * (2.5 if j > 0 else 1.0)) ** 2  # coloured-noise boost for L2+
                    ev = uniform_filter(b * b, 7)
                    g = np.maximum(ev - sn2, 0) / (ev + 1e-6)  # local Wiener
                    rec += g * b
                out.append(rec)
            tY, tU, tV = out
        oY.append(tY); oU.append(tU); oV.append(tV)
    print(f"  proposed took {time.time() - t0:.0f}s", flush=True)
    return np.array(oY), np.array(oU), np.array(oV)


if MODE in ("all", "prop"):
    report("proposed temporal-only", *run_proposed(spatial=False))
    report("proposed temporal+spatial", *run_proposed(spatial=True))
if MODE in ("all", "cur"):
    report("current v4c", *run_current())
