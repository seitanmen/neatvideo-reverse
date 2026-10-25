#!/usr/bin/env python3
"""Denoise a real video with the reproduced Neat Video pipeline.

Fast path for real footage:
  - auto flat-patch finder (min-variance 32x32 block)
  - vectorized SAD block-match motion estimation
  - wavelet spatial + motion-compensated temporal + deflicker
  - encode via ffmpeg pipe (libx264)
"""
import json
import os
import subprocess
import sys
import numpy as np
import pywt
from scipy.ndimage import uniform_filter

import argparse
ap = argparse.ArgumentParser()
ap.add_argument("src", nargs="?", default="/Users/seitan/Desktop/work/reverse/test.mp4")
ap.add_argument("dst", nargs="?", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "out", "test_denoised.mp4"))
ap.add_argument("--spatial", type=float, default=0.5, help="spatial threshold scale (1.0=strong)")
ap.add_argument("--temporal", type=float, default=0.5, help="temporal blend scale (1.0=strong)")
ap.add_argument("--bs", type=int, default=16, help="macroblock size")
ap.add_argument("--sr", type=int, default=5, help="motion search range")
A = ap.parse_args()
SRC, DST = A.src, A.dst
os.makedirs(os.path.dirname(DST), exist_ok=True)

BS, SR = A.bs, A.sr

def gray(f):
    return (0.299 * f[:, :, 0] + 0.587 * f[:, :, 1] + 0.114 * f[:, :, 2]).astype(np.float32)

def find_flat(g):
    mean = uniform_filter(g, 32)
    sq = uniform_filter(g * g, 32)
    var = sq - mean * mean
    var[:32, :] = var[-32:, :] = np.inf
    var[:, :32] = var[:, -32:] = np.inf
    y, x = np.unravel_index(np.argmin(var[16:-16, 16:-16]), var[16:-16, 16:-16].shape)
    return (slice(y + 16, y + 48), slice(x + 16, x + 48))

def build_profile(frame, flat):
    prof = {}
    for c, name in enumerate("RGB"):
        hh1 = pywt.wavedec2(frame[flat][:, :, c], "db1", level=2)[-1][2]
        prof[name] = {"sigma": float(np.median(np.abs(hh1)) / 0.6745)}
    return prof

def spatial_filter(frame, prof, strength=1.0):
    out = np.empty_like(frame)
    H, W = frame.shape[:2]
    for c, name in enumerate("RGB"):
        coeff = pywt.wavedec2(frame[:, :, c], "db1", level=3)
        T = strength * prof[name]["sigma"] * 1.2
        dn = [coeff[0]] + [tuple(pywt.threshold(d, T, mode="soft") for d in det) for det in coeff[1:]]
        out[:, :, c] = pywt.waverec2(dn, "db1")[:H, :W]
    return np.clip(out, 0, 255)

def estimate_motion_fast(prev_g, cur_g):
    H, W = cur_g.shape
    nby, nbx = H // BS, W // BS
    Hp, Wp = nby * BS, nbx * BS
    cur = cur_g[:Hp, :Wp]
    best = np.full((nby, nbx), np.inf)
    fdy = np.zeros((nby, nbx), np.int32)
    fdx = np.zeros((nby, nbx), np.int32)
    Pp = np.pad(prev_g, SR, mode="edge")
    for dy in range(-SR, SR + 1):
        for dx in range(-SR, SR + 1):
            sh = Pp[SR + dy:SR + dy + Hp, SR + dx:SR + dx + Wp]
            d = np.abs(cur - sh).reshape(nby, BS, nbx, BS).sum((1, 3))
            m = d < best
            best[m], fdy[m], fdx[m] = d[m], dy, dx
    return np.stack([fdy, fdx], -1)

def warp(frame, flow):
    H, W = frame.shape[:2]
    ys, xs = np.mgrid[0:H, 0:W]
    f = flow.repeat(BS, 0).repeat(BS, 1)[:H, :W]
    ph, pw = H - f.shape[0], W - f.shape[1]
    if ph or pw:
        f = np.pad(f, ((0, max(ph, 0)), (0, max(pw, 0)), (0, 0)), mode="edge")
    sy = np.clip(ys + f[:, :, 0], 0, H - 1)
    sx = np.clip(xs + f[:, :, 1], 0, W - 1)
    return frame[sy, sx]

def read_frames(path):
    info = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0",
                           "-show_entries", "stream=width,height,avg_frame_rate,nb_frames",
                           "-of", "csv=p=0", path], capture_output=True, text=True, check=True).stdout.strip().split(",")
    W, H = int(info[0]), int(info[1])
    num, den = info[2].split("/")
    fps = float(num) / float(den)
    cmd = ["ffmpeg", "-v", "error", "-i", path, "-f", "rawvideo", "-pix_fmt", "rgb24", "-"]
    raw = subprocess.run(cmd, capture_output=True, check=True).stdout
    n = len(raw) // (W * H * 3)
    return [np.frombuffer(raw, np.uint8)[i * W * H * 3:(i + 1) * W * H * 3].reshape(H, W, 3).astype(np.float32)
            for i in range(n)], fps

def main():
    print("decoding...", flush=True)
    frames, fps = read_frames(SRC)
    N = len(frames)
    H, W = frames[0].shape[:2]
    print(f"frames={N} {W}x{H} fps={fps}", flush=True)

    flat = find_flat(gray(frames[N // 3]))
    prof = build_profile(frames[N // 3], flat)
    print("profile:", {k: round(v["sigma"], 2) for k, v in prof.items()}, flush=True)
    sigma = float(np.mean([v["sigma"] for v in prof.values()]))
    w = float(np.clip(A.temporal * sigma / 20.0, 0.1, 1.0))

    print("spatial pass...", flush=True)
    spat = [spatial_filter(f, prof, strength=A.spatial) for f in frames]
    grays = [gray(f) for f in spat]

    cmd = ["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "rgb24",
           "-s", f"{W}x{H}", "-r", str(fps), "-i", "-",
           "-c:v", "libx264", "-pix_fmt", "yuv420p", "-crf", "16",
           "-movflags", "+faststart", DST]
    enc = subprocess.Popen(cmd, stdin=subprocess.PIPE)
    # deflicker target from spatial pass means
    means = np.array([f.mean() for f in spat])
    tgt = means.mean()
    print("temporal pass + encode...", flush=True)
    for t in range(N):
        acc, norm = spat[t], 1.0
        for tt in (t - 1, t + 1):
            if 0 <= tt < N:
                fl = estimate_motion_fast(grays[tt], grays[t])
                acc = acc + w * warp(spat[tt], fl)
                norm += w
        out = np.clip(acc / norm * (tgt / means[t]), 0, 255).astype(np.uint8)
        enc.stdin.write(out.tobytes())
        if t % 100 == 0:
            print(f"  {t}/{N}", flush=True)
    enc.stdin.close()
    enc.wait()
    json.dump({"profile": prof, "frames": N, "weight": w},
              open(os.path.join(os.path.dirname(DST), "test_denoised.json"), "w"), indent=1)
    print("wrote", DST, os.path.getsize(DST), flush=True)

if __name__ == "__main__":
    main()
