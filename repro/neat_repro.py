#!/usr/bin/env python3
"""Neat Video v6 (OFX Demo) denoising pipeline reproduction.

Faithful-concept reimplementation of the observed architecture:
  DeviceNoiseProfile  -> per-subband noise estimation from a flat area
  ImageWithLevels     -> multi-level wavelet decomposition (pywt)
  spatial shrinkage   -> soft-threshold detail subbands by profile sigma
  MotionCompensator   -> SAD block-match motion compensation (MacroBlock)
  temporal average    -> motion-compensated multi-frame blending
  Flicker reduction   -> global luma gain matching

Validation: synthetic clip (clean known) -> add noise -> denoise -> PSNR.
Baseline: ffmpeg hqdn3d on the same clip.
"""
import json
import os
import subprocess
import numpy as np
import pywt

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")
os.makedirs(OUT, exist_ok=True)

rng = np.random.default_rng(7)

# ---------------- synthetic clip ----------------
W, H, N = 176, 144, 24
FLAT = (slice(0, 32), slice(0, 32))  # known flat patch for profiling

def make_clean():
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    base = 60 + 0.5 * xx + 0.3 * yy                      # gradient
    tex = 18 * np.sin(xx * 0.35) * np.sin(yy * 0.31)      # texture
    clip = np.zeros((N, H, W, 3), np.float32)
    for t in range(N):
        f = base + tex
        sq = 26                                          # moving square
        x0 = 20 + t * 4
        f[50:50 + sq, x0:x0 + sq] += 90
        f[FLAT] = 128.0                                  # flat profiling area
        clip[t] = np.stack([f, f * 0.92 + 8, f * 0.85 + 14], -1)
    return np.clip(clip, 0, 255)

def add_noise(clean, sigma=15.0):
    noisy = clean + rng.normal(0, sigma, clean.shape)
    gain = 1 + 0.02 * np.sin(np.arange(N) * 0.9)          # flicker
    noisy *= gain[:, None, None, None]
    return np.clip(noisy, 0, 255)

# ---------------- 1. noise profile ----------------
def build_noise_profile(noisy, flat=FLAT):
    """Estimate per-subband sigma from flat patch (MAD of HH1, like Donoho)."""
    prof = {}
    for c, name in enumerate("RGB"):
        patch = noisy[0][flat][:, :, c]
        coeff = pywt.wavedec2(patch, "db1", level=2)
        hh1 = coeff[-1][2]
        sigma = float(np.median(np.abs(hh1)) / 0.6745)
        prof[name] = {"sigma": sigma}
    return prof

# ---------------- 2. spatial (frequency) filter ----------------
def spatial_filter(frame, prof, strength=1.0):
    out = np.empty_like(frame)
    for c, name in enumerate("RGB"):
        ch = frame[:, :, c]
        coeff = pywt.wavedec2(ch, "db1", level=3)
        sigma = prof[name]["sigma"]
        T = strength * sigma * 1.2
        dn = [coeff[0]]
        for det in coeff[1:]:
            dn.append(tuple(pywt.threshold(d, T, mode="soft") for d in det))
        out[:, :, c] = pywt.waverec2(dn, "db1")[:H, :W]
    return np.clip(out, 0, 255)

# ---------------- 3. motion compensation (SAD macroblocks) ----------------
BS, SR = 8, 7  # block size, search range

def gray(f):
    return (0.299 * f[:, :, 0] + 0.587 * f[:, :, 1] + 0.114 * f[:, :, 2])

def estimate_motion(prev_g, cur_g):
    flow = np.zeros((H // BS, W // BS, 2), np.int32)
    padded = np.pad(prev_g, SR, mode="edge")
    for by in range(H // BS):
        for bx in range(W // BS):
            y, x = by * BS, bx * BS
            cur = cur_g[y:y + BS, x:x + BS]
            best, bdy, bdx = 1e18, 0, 0
            for dy in range(-SR, SR + 1):
                for dx in range(-SR, SR + 1):
                    cand = padded[y + dy + SR:y + dy + SR + BS,
                                  x + dx + SR:x + dx + SR + BS]
                    sad = np.abs(cur - cand).sum()
                    if sad < best:
                        best, bdy, bdx = sad, dy, dx
            flow[by, bx] = (bdy, bdx)
    return flow

def warp(frame, flow):
    out = np.empty_like(frame)
    for by in range(H // BS):
        for bx in range(W // BS):
            dy, dx = flow[by, bx]
            y, x = by * BS, bx * BS
            sy = min(max(y + dy, 0), H - BS)
            sx = min(max(x + dx, 0), W - BS)
            out[y:y + BS, x:x + BS] = frame[sy:sy + BS, sx:sx + BS]
    return out

def temporal_filter(frames, prof, radius=1, strength=1.0):
    sigma = np.mean([prof[c]["sigma"] for c in "RGB"])
    w = float(np.clip(strength * sigma / 12.0, 0.2, 1.0))
    grays = [gray(f) for f in frames]
    out = []
    for t in range(N):
        acc = frames[t].copy()
        norm = 1.0
        for dt in range(1, radius + 1):
            for tt in (t - dt, t + dt):
                if 0 <= tt < N:
                    fl = estimate_motion(grays[tt], grays[t])
                    acc += w * warp(frames[tt], fl)
                    norm += w
        out.append(acc / norm)
    return out

# ---------------- 4. flicker reduction ----------------
def deflicker(frames):
    means = np.array([f.mean() for f in frames])
    tgt = means.mean()
    return [np.clip(f * (tgt / m), 0, 255) for f, m in zip(frames, means)]

# ---------------- metrics / baseline ----------------
def psnr(a, b):
    mse = ((a.astype(np.float32) - b.astype(np.float32)) ** 2).mean()
    return float(10 * np.log10(255 * 255 / max(mse, 1e-9)))

def main():
    clean = make_clean()
    noisy = add_noise(clean)
    prof = build_noise_profile(noisy)
    print("profile:", {k: round(v["sigma"], 2) for k, v in prof.items()})

    spat = [spatial_filter(f, prof) for f in noisy]
    temp = temporal_filter(noisy, prof)
    full = temporal_filter(spat, prof)
    final = deflicker(full)

    res = {}
    for tag, clip in [("noisy", noisy), ("spatial", spat),
                      ("temporal", temp), ("full", full), ("final", final)]:
        arr = np.stack(clip) if isinstance(clip, list) else clip
        res[tag] = round(float(np.mean([psnr(a, b) for a, b in zip(arr, clean)])), 2)
    # hqdn3d baseline via ffmpeg (png sequence, no extra plugins)
    try:
        import imageio.v2 as imageio
        for i, f in enumerate(np.asarray(noisy).astype(np.uint8)):
            imageio.imwrite(f"{OUT}/n_{i:03d}.png", f)
        subprocess.run(["ffmpeg", "-y", "-v", "error", "-i", f"{OUT}/n_%03d.png",
                        "-vf", "hqdn3d", f"{OUT}/h_%03d.png"], check=True)
        hq = np.stack([imageio.imread(f"{OUT}/h_{i:03d}.png")[:, :, :3]
                       for i in range(N)]).astype(float)
        res["hqdn3d"] = round(float(np.mean([psnr(hq[i], clean[i]) for i in range(N)])), 2)
    except Exception as e:
        res["hqdn3d"] = f"skipped: {e}"

    import imageio.v2 as imageio
    imageio.imwrite(f"{OUT}/clean.png", clean[12].astype(np.uint8))
    imageio.imwrite(f"{OUT}/noisy.png", np.asarray(noisy)[12].astype(np.uint8))
    imageio.imwrite(f"{OUT}/denoised.png", np.asarray(final)[12].astype(np.uint8))
    with open(f"{OUT}/results.json", "w") as f:
        json.dump({"psnr_db": res, "profile": prof}, f, indent=1)
    print(json.dumps(res, indent=1))

if __name__ == "__main__":
    main()
