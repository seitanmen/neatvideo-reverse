"""Diagnosis 2: is what the processing removed the added noise or the picture?

    python3 tools/diag_removed.py CLIP [CLIP ...] [--test runs/ours_frame/<src>/<clip>/out.mp4 | genuine/<src>/<clip>]

added n = noisy - clean (exactly known), removed m = noisy - output (4:2:0 code values, float; an RGB test is taken to
4:2:0 by e9_common.rgb_to_yuv420), over all frames. Per plane and scope:
  rho  = corr(m, n)                      (1 = the removed part is exactly proportional to the added noise)
  beta = <m, n> / <n, n>                 (the share of the added noise that was removed; 1 = all of it)
  noise_share = rho^2                    (the share of the removed energy that is the added noise)
  rms_added, rms_removed, rms_other = RMS(m - beta*n)   (what was removed besides the added noise: picture detail,
                                          the source's own grain, motion errors; for Sintel (grain-free) only picture)
Scopes: the whole plane; Y also per field (interlaced: field_top / field_bottom, and per class) and per motion class of the truth (e9_eval.motion_classes) and per a-trous band of m and
n (nvfilter.atrous, bands 1..5 + residual: whether coarse structure was removed); Cb / Cr also per a-trous band (4 on
the half-resolution plane + residual), per motion class (the luma class map at the even pixels), per tercile of the
local luma texture of the frame (box 9x9 of |Y - box3(Y)| of the clean Y, tex_low / mid / high) and per quarter of
the clean luma (2x2 mean, luma0-63 .. luma192-255).
Writes eval/diag_removed_<src>_<clip>[_<test name>].json.
"""
import argparse
import os
import sys

import numpy as np
from scipy.ndimage import uniform_filter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import e9_common as C  # noqa: E402
from e9_eval import CLASSES, frames_of, motion_classes  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.normpath(os.path.join(ROOT, "..", "..", "..", "reference", "python")))
import nvfilter as nv  # noqa: E402


class Corr:
    def __init__(self):
        self.d = {}

    def add(self, k, m, n):
        s = self.d.setdefault(k, np.zeros(4))
        s += (float((m * n).sum()), float((n * n).sum()), float((m * m).sum()), m.size)

    def result(self):
        out = {}
        for k, (mn, nn, mm, cnt) in self.d.items():
            if cnt == 0 or nn == 0 or mm == 0:
                continue
            beta = mn / nn
            rho = mn / np.sqrt(nn * mm)
            other = max(mm - 2 * beta * mn + beta * beta * nn, 0.0)
            out[k] = {"rho": round(rho, 4), "beta": round(beta, 4), "noise_share": round(rho * rho, 4),
                      "rms_added": round(np.sqrt(nn / cnt), 4), "rms_removed": round(np.sqrt(mm / cnt), 4),
                      "rms_other": round(np.sqrt(other / cnt), 4)}
        return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("clips", nargs="+")
    ap.add_argument("--test", default=None, help="output to analyse (default: runs/ours_frame/<src>/<clip>/out.mp4)")
    ap.add_argument("--name", default="ours_frame")
    a = ap.parse_args()
    for clip in a.clips:
        src, lv = clip.split("/")
        layout, level = lv.split("_")
        test = a.test or os.path.join(ROOT, "runs", "ours_frame", src, lv, "out.mp4")
        clean = [tuple(np.array(p) for p in pl) for _, pl in C.read_y4m(os.path.join(ROOT, "clips", src, f"clean_{layout}.y4m"))]
        cls = motion_classes([c[0] for c in clean], layout)
        noisy = C.read_y4m(os.path.join(ROOT, "clips", src, f"noisy_{layout}_{level}.y4m"))
        acc = Corr()
        for k, ((_, npl), fr) in enumerate(zip(noisy, frames_of(test, layout))):
            if k >= len(clean):
                break
            for c, name in enumerate(("Y", "Cb", "Cr")):
                nz = npl[c].astype(np.float64)
                n = nz - clean[k][c]
                m = nz - fr["yuv"][c]
                acc.add(name, m, n)
                if c == 0:
                    for v, (cn, _, _) in enumerate(CLASSES):
                        sel = cls[k] == v
                        acc.add(f"Y.{cn}", m[sel], n[sel])
                    if layout != "p":
                        for par, fn in ((0, "top"), (1, "bottom")):
                            acc.add(f"Y.field_{fn}", m[par::2], n[par::2])
                            for v, (cn, _, _) in enumerate(CLASSES):
                                sel = cls[k][par::2] == v
                                acc.add(f"Y.field_{fn}.{cn}", m[par::2][sel], n[par::2][sel])
                    bm, bn = nv.atrous(m.astype(np.float32), 5), nv.atrous(n.astype(np.float32), 5)
                    for j in range(6):
                        acc.add(f"Y.band{j + 1}" if j < 5 else "Y.residual",
                                bm[j].astype(np.float64), bn[j].astype(np.float64))
                else:
                    if k == 0 or c == 1:            # per-frame luma context on the chroma grid (clean Y)
                        yc = clean[k][0].astype(np.float64)
                        y2 = 0.25 * (yc[0::2, 0::2] + yc[1::2, 0::2] + yc[0::2, 1::2] + yc[1::2, 1::2])
                        tex = uniform_filter(np.abs(yc - uniform_filter(yc, 3, mode="nearest")), 9)[0::2, 0::2]
                        t1, t2 = np.percentile(tex, [100 / 3, 200 / 3])
                        tcls = (tex >= t1).astype(np.uint8) + (tex >= t2)
                        mc = cls[k][0::2, 0::2]
                    bm, bn = nv.atrous(m.astype(np.float32), 4), nv.atrous(n.astype(np.float32), 4)
                    for j in range(5):
                        acc.add(f"{name}.band{j + 1}" if j < 4 else f"{name}.residual",
                                bm[j].astype(np.float64), bn[j].astype(np.float64))
                    for v, (cn, _, _) in enumerate(CLASSES):
                        sel = mc == v
                        acc.add(f"{name}.{cn}", m[sel], n[sel])
                    for v, tn in enumerate(("tex_low", "tex_mid", "tex_high")):
                        sel = tcls == v
                        acc.add(f"{name}.{tn}", m[sel], n[sel])
                    lb = np.clip((y2 * 4 / 256).astype(int), 0, 3)
                    for i in range(4):
                        sel = lb == i
                        acc.add(f"{name}.luma{64 * i}-{64 * i + 63}", m[sel], n[sel])
        res = {"clip": clip, "test": os.path.relpath(test, ROOT), "frames": k + 1, "result": acc.result()}
        name = "" if a.name == "ours_frame" else "_" + a.name
        C.write_json(os.path.join(ROOT, "eval", f"diag_removed_{src}_{lv}{name}.json"), res)
        print(f"== {clip} ({a.name})")
        for key, v in res["result"].items():
            print(f"  {key:12s} rho {v['rho']:.3f} beta {v['beta']:.3f} share {v['noise_share']:.3f}  "
                  f"rms added {v['rms_added']:.3f} removed {v['rms_removed']:.3f} other {v['rms_other']:.3f}")


if __name__ == "__main__":
    main()
