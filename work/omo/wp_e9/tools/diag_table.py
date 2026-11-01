"""Markdown tables of diag_removed / diag_temporal results, the outputs side by side.

    python3 tools/diag_table.py CLIP [CLIP ...] [--names ours_frame,genuine] [--keys Y,Y.static,...]
                                [--cols removed,rho,beta,share,other]
"""
import argparse
import json
import os

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
DEFAULT = ("Y,Y.static,Y.small,Y.large,Y.band1,Y.band2,Y.band3,Y.band4,Y.band5,Y.residual,"
           "Cb,Cb.band1,Cb.band2,Cb.band3,Cb.band4,Cb.residual,Cr,Cr.band1,Cr.band2,Cr.band3,Cr.band4,Cr.residual,"
           "Cb.tex_low,Cb.tex_mid,Cb.tex_high,Cr.tex_low,Cr.tex_mid,Cr.tex_high,"
           "Cb.luma0-63,Cb.luma64-127,Cb.luma128-191,Cb.luma192-255,Cr.luma0-63,Cr.luma64-127,Cr.luma128-191,"
           "Cr.luma192-255")
COLS = {"removed": ("rms_removed", "{:.3f}"), "rho": ("rho", "{:.3f}"), "beta": ("beta", "{:.3f}"),
        "share": ("noise_share", "{:.3f}"), "other": ("rms_other", "{:.3f}")}


def load(kind, clip, name):
    src, lv = clip.split("/")
    if kind == "removed":
        suffix = "" if name == "ours_frame" else "_" + name
    else:
        suffix = "_" + name
    p = os.path.join(ROOT, "eval", f"diag_{kind}_{src}_{lv}{suffix}.json")
    return json.load(open(p)) if os.path.exists(p) else None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("clips", nargs="+")
    ap.add_argument("--names", default="ours_frame,genuine")
    ap.add_argument("--keys", default=DEFAULT)
    ap.add_argument("--cols", default="removed,rho,beta,share,other")
    a = ap.parse_args()
    keys, names, cols = a.keys.split(","), a.names.split(","), a.cols.split(",")
    for clip in a.clips:
        R = {n: load("removed", clip, n) for n in names}
        print(f"\n#### {clip}\n")
        print("| scope | added RMS | " + " | ".join(f"{n} {c}" for n in names for c in cols) + " |")
        print("|---|---|" + "---|" * (len(names) * len(cols)))
        for k in keys:
            ds = [R[n]["result"].get(k) if R[n] else None for n in names]
            base = next((d for d in ds if d), None)
            if base is None:
                continue
            cells = []
            for d in ds:
                for c in cols:
                    f, fmt = COLS[c]
                    cells.append("-" if d is None else fmt.format(d[f]))
            print(f"| {k} | {base['rms_added']:.3f} | " + " | ".join(cells) + " |")
        T = {n: load("temporal", clip, n) for n in names}
        if any(T.values()):
            print("\n| test | rms d clean | rms d added | rms d out | keep_clean | keep_added | "
                  "corr(nY,nCb) | corr(mY,mCb) | corr(nY,nCr) | corr(mY,mCr) |")
            print("|---|---|---|---|---|---|---|---|---|---|")
            for n in names:
                t = T[n]
                if not t:
                    continue
                s, x = t["temporal_static"], t["cross_corr"]
                print(f"| {n} | {s['rms_d_clean']} | {s['rms_d_added']} | {s['rms_d_out']} | {s['keep_clean']} | "
                      f"{s['keep_added']} | {x['added_Y_Cb']} | {x['removed_Y_Cb']} | {x['added_Y_Cr']} | {x['removed_Y_Cr']} |")


if __name__ == "__main__":
    main()
