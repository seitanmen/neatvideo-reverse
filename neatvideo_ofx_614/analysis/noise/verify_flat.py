#!/usr/bin/env python3
"""Compare the FindFlatArea port with the genuine FindFlatArea (harness/nvh, Auto Profile path).

Clip frames go through the production entry nvfilter.find_flat_area (host-style RGB with the
stream's matrix, noise curve, fallback) and are compared on (x, y, w, h, status).
Synthetic RGB cases have no YCbCr planes: they call find_flat_area_genuine directly.
--curve genuine (default) normalises the scores with the exact EstimateNoiseLevelCurve port
(nv_curve.genuine_noise_curve), --curve v5 with the earlier v5 single-frame stand-in.

usage: verify_flat.py [--clip PATH --frames 0,100,...] [--synthetic] [--curve genuine|v5]"""
import argparse
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "..", "repro"))
sys.path.insert(0, HERE)
import nvfilter as nv  # noqa: E402
from denoise_video_v5 import probe, read_frames  # noqa: E402
from nvh_common import genuine_flat, require_guard  # noqa: E402


def port_synthetic(rgb, mode=1, curve="genuine"):
    """Synthetic RGB: the genuine curve port on the RGB, or the v5 curve from a full-range
    luma plane (no chroma planes exist)."""
    if curve == "genuine":
        import nv_curve
        return nv.find_flat_area_genuine(rgb, nv_curve.noise_at(nv_curve.genuine_noise_curve(rgb)), mode)
    Y = rgb[..., 0] * 0.2989 + rgb[..., 1] * 0.5866 + rgb[..., 2] * 0.1145
    cv = nv.noise_curve_spatial((Y, Y * 0 + 128, Y * 0 + 128), {"Y": 0.6}, True, names=("Y",)).get("Y")
    centers = [32 * (i + 0.5) for i in range(8)]
    return nv.find_flat_area_genuine(rgb, lambda b: float(np.interp(b, centers, cv)) if cv else 1.0, mode)


def report(name, g, p):
    print(f"{name:28s} genuine {g}  port {p}  {'MATCH' if g == p else 'DIFF'}", flush=True)
    return g == p


def synthetic():
    rng = np.random.default_rng(7)
    out = []
    H, W = 540, 960
    yy, xx = np.mgrid[0:H, 0:W]
    base = (40 + 150 * (xx / W) + 30 * np.sin(yy / 23.0) * np.sin(xx / 31.0)).astype(np.float32)
    base[300:480, 600:900] = 110.0
    out.append(("gradient + flat patch", np.clip(np.stack([base] * 3, -1) + rng.normal(0, 3, (H, W, 3)), 0, 255)))
    tex = base + 25 * np.sin(xx / 3.0) * np.cos(yy / 4.0)
    tex[60:300, 80:330] = 70.0
    out.append(("texture + dark flat", np.clip(np.stack([tex] * 3, -1) + rng.normal(0, 2, (H, W, 3)), 0, 255)))
    hi = base.copy()
    hi[0:250, 0:500] = 300.0
    out.append(("blown highlight", np.clip(np.stack([hi, hi * 0.95, hi * 0.9], -1) + rng.normal(0, 3, (H, W, 3)), 0, 255)))
    lb = np.clip(np.stack([base] * 3, -1) + rng.normal(0, 3, (H, W, 3)), 0, 255)
    lb[:70], lb[-70:] = 0, 0
    out.append(("letterbox", lb))
    col = np.stack([base * 0.9, base, base * 0.7], -1) + rng.normal(0, 1, (H, W, 3)) * [4, 2, 6]
    out.append(("coloured noise RGB", np.clip(col, 0, 255)))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--clip")
    ap.add_argument("--frames", default="0")
    ap.add_argument("--synthetic", action="store_true")
    ap.add_argument("--curve", choices=("genuine", "v5"), default="genuine")
    A = ap.parse_args()
    require_guard()
    ok = n = 0
    if A.clip:
        info = probe(A.clip)
        want = sorted(int(v) for v in A.frames.split(","))
        mat = nv.matrix_from_tags(info["tags"], info["H"])
        for i, pl in read_frames(A.clip, info["W"], info["H"], want[0], want[-1] + 1, fps=info["fps"]):
            if i in want:
                pl = tuple(np.asarray(p, np.float32) for p in pl)
                rgb = np.clip(nv.yuv_to_rgb(pl, mat, info["full_range"]), 0, 255)
                g = genuine_flat(rgb)
                y0, x0, S, st = nv.find_flat_area(pl, info["full_range"], mat, with_status=True, curve=A.curve)
                ok += report(f"{os.path.basename(A.clip)} n={i} [{mat}]", g, (x0, y0, S, S, st))
                n += 1
    if A.synthetic:
        for name, rgb in synthetic():
            rgb = rgb.astype(np.float32)
            ok += report(name, genuine_flat(rgb), port_synthetic(rgb, curve=A.curve))
            n += 1
    print(f"{ok}/{n} identical")


if __name__ == "__main__":
    main()
