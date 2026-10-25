#!/usr/bin/env python3
"""Run the genuine FindFlatArea (FUN_009b1a5c, via harness/nvh) on frames of a clip, with the
Auto Profile size table (--sel 0) or the 18%-width square (--sel 1), and print next to it the old
v5 gradient-based choice (nvfilter.find_flat_area_gradient). The production port is compared by
verify_flat.py.

usage: genuine_flat.py CLIP [--frames 0,100,...] [--mode 1] [--matrix bt709] [--noclip]
RGB = host-style conversion of the native YCbCr frame (limited range -> 0..255 floats)."""
import argparse
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPRO = os.path.join(HERE, "..", "..", "..", "repro")
sys.path.insert(0, REPRO)
sys.path.insert(0, HERE)
import nvfilter as nv  # noqa: E402
from denoise_video_v5 import probe, read_frames  # noqa: E402
from nvh_common import genuine_flat, require_guard  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("clip")
    ap.add_argument("--frames", default="0,1,2,100,200,312,400,560,700")
    ap.add_argument("--mode", type=int, default=1)
    ap.add_argument("--matrix", default=None, help="default: from the stream's colour tags")
    ap.add_argument("--noclip", action="store_true")
    ap.add_argument("--sel", type=int, default=0, help="FindFlatArea param_3: 0 Auto Profile size table, 1 18%% square")
    A = ap.parse_args()
    require_guard()
    info = probe(A.clip)
    W, H = info["W"], info["H"]
    mat = A.matrix or nv.matrix_from_tags(info["tags"], H)
    want = sorted(int(v) for v in A.frames.split(","))
    for n, pl in read_frames(A.clip, W, H, want[0], want[-1] + 1, fps=info["fps"]):
        if n not in want:
            continue
        rgb = nv.yuv_to_rgb(pl, mat, info["full_range"])
        if not A.noclip:
            rgb = np.clip(rgb, 0, 255)
        g = genuine_flat(rgb, A.mode, A.sel)
        y0, x0, S = nv.find_flat_area_gradient(pl, info["full_range"], matrix=mat)
        Y = pl[0].astype(np.float32)
        print(f"n={n:4d} genuine x={g[0]:4d} y={g[1]:4d} {g[2]}x{g[3]} st={g[4]} meanY={Y[g[1]:g[1]+g[3], g[0]:g[0]+g[2]].mean():6.1f}"
              f" | v5 gradient x={x0:4d} y={y0:4d} {S}x{S} meanY={Y[y0:y0+S, x0:x0+S].mean():6.1f}", flush=True)


if __name__ == "__main__":
    main()
