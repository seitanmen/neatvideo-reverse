# G9: word-level comparison of two native/x86_bridge_replay.py --dump directories (e.g. the x86_64 replay's
# differing buffers against an arm64 replay of the same calls with --select ... --dump-all): for every buffer
# file in A that B also has, the number of differing words, how many of them are NaN on either side, and the
# largest difference in units in the last place (float32 / float64 words, by the buffer's word size).
#
#   python3 native/x86_bridge_diff.py A_DIR B_DIR [--esize 4]
import argparse
import os
import sys

import numpy as np


def ulp_diff(a, b):
    """The ULP distance of float words (sign-magnitude bits mapped to an ordered integer; NaN pairs excluded by
    the caller)."""
    n = a.dtype.itemsize
    mag = np.int64((1 << (8 * n - 1)) - 1)

    def ordered(v):
        i = v.view(f"<i{n}").astype(np.int64)
        return np.where(i < 0, -(i & mag), i)
    return np.abs(ordered(a) - ordered(b))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("a")
    ap.add_argument("b")
    ap.add_argument("--esize", type=int, default=4)
    A = ap.parse_args()
    ft = {4: np.float32, 8: np.float64}[A.esize]
    tot_w = tot_d = 0
    for fn in sorted(os.listdir(A.a)):
        pb = os.path.join(A.b, fn)
        if not os.path.exists(pb):
            continue
        x = np.fromfile(os.path.join(A.a, fn), np.uint8)
        y = np.fromfile(pb, np.uint8)
        if x.size != y.size or x.size % A.esize:
            print(f"{fn}: sizes {x.size} / {y.size}")
            continue
        xa, ya = x.view(ft), y.view(ft)
        d = x.view(f"<u{A.esize}") != y.view(f"<u{A.esize}")
        nd = int(d.sum())
        tot_w += xa.size
        tot_d += nd
        if not nd:
            print(f"{fn}: identical ({xa.size} words)")
            continue
        nan = np.isnan(xa[d]) | np.isnan(ya[d])
        fin = ~nan
        u = ulp_diff(xa[d][fin], ya[d][fin]) if fin.any() else np.zeros(0, np.int64)
        rel = np.abs(xa[d][fin].astype(np.float64) - ya[d][fin]) if fin.any() else np.zeros(0)
        hist = np.bincount(np.minimum(u, 9)) if u.size else []
        print(f"{fn}: {nd}/{xa.size} words differ, NaN on a side {int(nan.sum())}, max ulp "
              f"{int(u.max()) if u.size else 0}, max |diff| {float(rel.max()) if rel.size else 0:.3g}, "
              f"ulp histogram 0..9+ {list(hist)}")
    print(f"total: {tot_d}/{tot_w} words differ")
    return 0


if __name__ == "__main__":
    sys.exit(main())
