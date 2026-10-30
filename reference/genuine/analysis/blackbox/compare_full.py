#!/usr/bin/env python3
"""Compare whole denoised videos with the genuine Neat output on sampled frames.

usage: compare_full.py GENUINE.mov CAND1.mp4 [CAND2.mp4 ...] [--step 24] [--offset 12]
Frames n = offset, offset+step, ... are decoded natively (yuv420p, no colour conversion);
reports Y/U/V PSNR vs genuine and the mean level shift per plane."""
import argparse
import json
import subprocess

import numpy as np


def frames(path, step, offset):
    info = json.loads(subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
                                      "stream=width,height", "-of", "json", path],
                                     capture_output=True, text=True, check=True).stdout)["streams"][0]
    W, H = info["width"], info["height"]
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", path, "-vf",
                          f"select=not(mod(n-{offset}\\,{step}))*gte(n\\,{offset})", "-vsync", "0",
                          "-f", "rawvideo", "-pix_fmt", "yuv420p", "-"], capture_output=True, check=True).stdout
    a = np.frombuffer(raw, np.uint8).reshape(-1, W * H * 3 // 2).astype(np.float64)
    q = W * H // 4
    return [(f[:W * H], f[W * H:W * H + q], f[W * H + q:]) for f in a]


def psnr(a, b):
    return 10 * np.log10(255.0 ** 2 / max(float(((a - b) ** 2).mean()), 1e-12))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("genuine")
    ap.add_argument("cands", nargs="+")
    ap.add_argument("--step", type=int, default=24)
    ap.add_argument("--offset", type=int, default=12)
    A = ap.parse_args()
    gen = frames(A.genuine, A.step, A.offset)
    print(f"{len(gen)} frames (n = {A.offset} + k*{A.step}) vs {A.genuine}")
    for c in A.cands:
        fr = frames(c, A.step, A.offset)
        n = min(len(fr), len(gen))
        r = [float(np.mean([psnr(fr[k][p], gen[k][p]) for k in range(n)])) for p in range(3)]
        s = [float(np.mean([fr[k][p].mean() - gen[k][p].mean() for k in range(n)])) for p in range(3)]
        print(f"{c}: PSNR vs genuine Y={r[0]:.2f} U={r[1]:.2f} V={r[2]:.2f} | mean shift "
              f"Y={s[0]:+.3f} U={s[1]:+.3f} V={s[2]:+.3f} (frames {n})", flush=True)


if __name__ == "__main__":
    main()
