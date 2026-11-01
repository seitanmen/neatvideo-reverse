"""The source's own grain / noise in the clean clips (the "truth" is not noise-free).

    python3 tools/grain.py [SRC ...] [--every 10]

Measure: h = Y - box3(Y) (clean Y of clips/<src>/clean_p.y4m, box 3x3 with replicated borders); the variance of h in
16x16 blocks; the 10% point (the flat side) of the blocks of every 10th frame, as a white-noise sigma
sqrt(v * 9/8) (white noise of sigma s gives var(h) = s^2 * 8/9; the added noise y3 has s = 3).  Blocks of zero
variance (letterbox bars, constant areas) are left out and counted.  Texture raises the measure, so it is an upper
bound of the grain in the flattest parts of the picture.
"""
import argparse
import os
import sys

import numpy as np
from scipy.ndimage import uniform_filter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import e9_common as C  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src", nargs="*")
    ap.add_argument("--every", type=int, default=10)
    a = ap.parse_args()
    out = {}
    for s in a.src or sorted(C.SOURCES):
        path = os.path.join(ROOT, "clips", s, "clean_p.y4m")
        if not os.path.exists(path):
            continue
        vs, zero, tot = [], 0, 0
        for k, pl in C.read_y4m(path):
            if k % a.every:
                continue
            y = pl[0].astype(np.float64)
            h = y - uniform_filter(y, 3, mode="nearest")
            v = uniform_filter(h * h, 16)[8::16, 8::16]
            tot += v.size
            zero += int((v < 1e-9).sum())
            vs.append(v[v >= 1e-9])
        v = np.concatenate(vs)
        p = np.percentile(v, [10, 25, 50])
        out[s] = {"white_sigma_p10": round(float(np.sqrt(p[0] * 9 / 8)), 2),
                  "white_sigma_p25": round(float(np.sqrt(p[1] * 9 / 8)), 2),
                  "white_sigma_p50": round(float(np.sqrt(p[2] * 9 / 8)), 2),
                  "blocks": tot, "zero_blocks": zero}
        print(s, out[s], flush=True)
    C.write_json(os.path.join(ROOT, "eval", "grain.json"), out)


if __name__ == "__main__":
    main()
