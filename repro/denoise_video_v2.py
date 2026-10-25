#!/usr/bin/env python3
"""Denoise a real video with the v2 Neat Video reproduction (YCbCr)."""
import argparse
import json
import os
import subprocess
import numpy as np
import nvfilter as nv

ap = argparse.ArgumentParser()
ap.add_argument("src")
ap.add_argument("dst")
ap.add_argument("--spatial", type=float, default=0.6)
ap.add_argument("--temporal", type=float, default=0.6)
ap.add_argument("--luma", type=float, default=0.8)
ap.add_argument("--chroma", type=float, default=1.4)
ap.add_argument("--bs", type=int, default=16)
ap.add_argument("--radius", type=int, default=2, help="temporal radius (Neat default 2)")
ap.add_argument("--sigma-scale", type=float, default=1.0, help="manual noise-level multiplier (Neat Noise Level analog)")
A = ap.parse_args()
os.makedirs(os.path.dirname(os.path.abspath(A.dst)), exist_ok=True)

def read_frames(path):
    info = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0",
                           "-show_entries", "stream=width,height,avg_frame_rate",
                           "-of", "csv=p=0", path], capture_output=True, text=True, check=True).stdout.strip().split(",")
    W, H = int(info[0]), int(info[1])
    num, den = info[2].split("/")
    fps = float(num) / float(den)
    cmd = ["ffmpeg", "-v", "error", "-i", path, "-f", "rawvideo", "-pix_fmt", "rgb24", "-"]
    raw = subprocess.run(cmd, capture_output=True, check=True).stdout
    n = len(raw) // (W * H * 3)
    arr = np.frombuffer(raw, np.uint8).reshape(n, H, W, 3)
    return [arr[i] for i in range(n)], fps, W, H

def main():
    print("decoding...", flush=True)
    frames, fps, W, H = read_frames(A.src)
    N = len(frames)
    print(f"frames={N} {W}x{H} fps={fps}", flush=True)

    yc0 = nv.rgb2ycbcr(frames[N // 3].astype(np.float32))
    flat = nv.find_flat(yc0[..., 0])
    prof = nv.build_profile_yc(yc0, flat)
    # chroma MAD on a flat patch underestimates low-freq chroma noise; floor at 25% of luma
    for ch in ("Cb", "Cr"):
        prof[ch]["sigma"] = max(prof[ch]["sigma"], 0.25 * prof["Y"]["sigma"])
    print("profile:", {k: round(v["sigma"], 2) for k, v in prof.items()}, flush=True)
    for ch in ("Y", "Cb", "Cr"):
        prof[ch]["sigma"] *= A.sigma_scale
        for j in prof[ch]["band_sigma"]:
            prof[ch]["band_sigma"][j] *= A.sigma_scale
    print("profile(scaled):", {k: round(v["sigma"], 2) for k, v in prof.items()}, flush=True)
    sigma = float(prof["Y"]["sigma"])
    wbase = float(np.clip(A.temporal * sigma / 20.0, 0.1, 1.0))

    cmd = ["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "rgb24",
           "-s", f"{W}x{H}", "-r", str(fps), "-i", "-",
           "-c:v", "libx264", "-pix_fmt", "yuv420p", "-crf", "16",
           "-movflags", "+faststart", A.dst]
    enc = subprocess.Popen(cmd, stdin=subprocess.PIPE)

    SEG, HALO = 120, 2
    done = 0
    for s0 in range(0, N, SEG):
        s1 = min(s0 + SEG, N)
        g0, g1 = max(s0 - HALO, 0), min(s1 + HALO, N)
        seg = [nv.rgb2ycbcr(frames[i].astype(np.float32)) for i in range(g0, g1)]
        grays = [f[..., 0].astype(np.float32) for f in seg]
        print(f"temporal {s0}-{s1}...", flush=True)
        # Neat order: temporal first on raw frames
        tmp = []
        for li in range(len(seg)):
            acc = seg[li].copy()
            norm = np.ones(seg[li].shape[:2])
            for dt in range(1, A.radius + 1):
                for ltt in (li - dt, li + dt):
                    if 0 <= ltt < len(seg):
                        flow, sad = nv.estimate_motion(grays[ltt], grays[li], A.bs)
                        rel = np.exp(-np.maximum(sad - 2.0 * sigma, 0) / (4.0 * sigma + 1e-6))
                        rel_f = np.pad(np.repeat(np.repeat(rel, A.bs, 0), A.bs, 1),
                                       ((0, max(H - (len(rel) * A.bs), 0)), (0, max(W - (len(rel[0]) * A.bs), 0))),
                                       mode="edge")[:H, :W]
                        w = wbase * rel_f[..., None]
                        acc = acc + w * nv.warp_float(seg[ltt], flow, A.bs)
                        norm = norm + w[..., 0]
            tmp.append(acc / norm[..., None])
        print(f"spatial {s0}-{s1}...", flush=True)
        spat = [nv.spatial_filter_yc(f, prof, strength=A.spatial, luma=A.luma, chroma=A.chroma) for f in tmp]
        means = np.array([f[..., 0].mean() for f in spat])
        tgt = means.mean()
        for t in range(s0, s1):
            lt = t - g0
            out_yc = spat[lt].copy()
            out_yc[..., 0] = np.clip(out_yc[..., 0] * (tgt / means[lt]), 0, 255)
            rgb = np.clip(nv.ycbcr2rgb(out_yc), 0, 255).astype(np.uint8)
            enc.stdin.write(rgb.tobytes())
            done += 1
            if done % 100 == 0:
                print(f"  {done}/{N}", flush=True)
    enc.stdin.close()
    enc.wait()
    json.dump({"profile": prof, "frames": N}, open(os.path.splitext(A.dst)[0] + ".json", "w"), indent=1)
    print("wrote", A.dst, os.path.getsize(A.dst), flush=True)

if __name__ == "__main__":
    main()
