"""Diagnosis 3: what the static-class frame difference of an output is made of, and how the removed chroma relates to
the removed luma.

    python3 tools/diag_temporal.py CLIP [--test PATH --name NAME]   (default test: runs/ours_frame/<src>/<clip>/out.mp4)

Temporal (Y, static class of frame t of the truth, frames t >= 1; d = frame t - frame t-1):
  rms_d_clean, rms_d_added (added noise n), rms_d_out
  keep_clean = <d_out, d_clean> / <d_clean, d_clean>   (share of the source's own frame-to-frame variation - its grain
                                                        and residual motion - that the output keeps; 1 = all)
  keep_added = <d_out, d_n> / <d_n, d_n>               (share of the added noise's frame-to-frame variation left)
  (d_out = d_clean + d_n - d_removed; the source's d_clean and the added d_n are independent.)
Chroma vs luma (all pixels, chroma grid; the luma value at a chroma sample = 2x2 mean):
  corr(n_Y, n_Cb), corr(n_Y, n_Cr) of the added noise (RGB noise makes them correlated) next to
  corr(m_Y, m_Cb), corr(m_Y, m_Cr) of the removed amounts m = noisy - output.
Writes eval/diag_temporal_<src>_<clip>_<name>.json.
"""
import argparse
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import e9_common as C  # noqa: E402
from e9_eval import frames_of, motion_classes  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("clip")
    ap.add_argument("--test", default=None)
    ap.add_argument("--name", default="ours_frame")
    a = ap.parse_args()
    src, lv = a.clip.split("/")
    layout, level = lv.split("_")
    test = a.test or os.path.join(ROOT, "runs", "ours_frame", src, lv, "out.mp4")
    clean = [tuple(np.array(p) for p in pl) for _, pl in C.read_y4m(os.path.join(ROOT, "clips", src, f"clean_{layout}.y4m"))]
    cls = motion_classes([c[0] for c in clean], layout)
    noisy = C.read_y4m(os.path.join(ROOT, "clips", src, f"noisy_{layout}_{level}.y4m"))
    S = {k: 0.0 for k in ("cc", "nn", "oo", "oc", "on", "cnt")}
    X = {}
    prev = None
    for k, ((_, npl), fr) in enumerate(zip(noisy, frames_of(test, layout))):
        if k >= len(clean):
            break
        yc = clean[k][0].astype(np.float64)
        nY = npl[0].astype(np.float64) - yc
        oY = fr["yuv"][0]
        if prev is not None:
            st = cls[k] == 0
            dc, dn, do = (yc - prev[0])[st], (nY - prev[1])[st], (oY - prev[2])[st]
            S["cc"] += (dc * dc).sum()
            S["nn"] += (dn * dn).sum()
            S["oo"] += (do * do).sum()
            S["oc"] += (do * dc).sum()
            S["on"] += (do * dn).sum()
            S["cnt"] += dc.size
        prev = (yc, nY, oY)
        mY = npl[0].astype(np.float64) - oY

        def half(x):
            return 0.25 * (x[0::2, 0::2] + x[1::2, 0::2] + x[0::2, 1::2] + x[1::2, 1::2])
        nY2, mY2 = half(nY), half(mY)
        for c, nm in ((1, "Cb"), (2, "Cr")):
            nc = npl[c].astype(np.float64) - clean[k][c]
            mc = npl[c].astype(np.float64) - fr["yuv"][c]
            for key, u, v in ((f"added_Y_{nm}", nY2, nc), (f"removed_Y_{nm}", mY2, mc)):
                s = X.setdefault(key, np.zeros(3))
                s += ((u * v).sum(), (u * u).sum(), (v * v).sum())
    n = S["cnt"]
    res = {"clip": a.clip, "test": os.path.relpath(test, ROOT), "name": a.name, "static_pairs_pixels": int(n),
           "temporal_static": {
               "rms_d_clean": round(np.sqrt(S["cc"] / n), 4), "rms_d_added": round(np.sqrt(S["nn"] / n), 4),
               "rms_d_out": round(np.sqrt(S["oo"] / n), 4), "keep_clean": round(S["oc"] / S["cc"], 4),
               "keep_added": round(S["on"] / S["nn"], 4)},
           "cross_corr": {k: round(v[0] / np.sqrt(v[1] * v[2]), 4) for k, v in X.items()}}
    C.write_json(os.path.join(ROOT, "eval", f"diag_temporal_{src}_{lv}_{a.name}.json"), res)
    print(a.clip, a.name, res["temporal_static"], res["cross_corr"])


if __name__ == "__main__":
    main()
