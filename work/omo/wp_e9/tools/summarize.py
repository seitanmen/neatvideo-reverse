"""Tables of the E9 evaluation results (eval/*.json written by e9_eval.py).

    python3 tools/summarize.py [eval/*.json] [--scope all|interior] [--domain yuv|yuv8]
"""
import argparse
import glob
import json
import os

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))


def g(d, k, f="psnr"):
    v = d.get(k)
    return "-" if v is None else f"{v[f]:.2f}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="*")
    ap.add_argument("--scope", default="all")
    ap.add_argument("--domain", default="yuv")
    a = ap.parse_args()
    files = a.files or sorted(f for f in glob.glob(os.path.join(ROOT, "eval", "*.json")) if not f.endswith("_floor.json"))
    rows_p, rows_i, pairs = [], [], []
    for f in files:
        r = json.load(open(f))
        if "tests" not in r:            # grain.json, floor files
            continue
        for nm, t in r["tests"].items():
            dom = a.domain if a.domain in t["domains"] else t["domains"][0]
            s = t[a.scope]
            tm = s.get("temporal_rms", {})
            base = [r["clip"], nm, g(s, f"{dom}.Y"), g(s, f"{dom}.Y.static"), g(s, f"{dom}.Y.small"),
                    g(s, f"{dom}.Y.large"), g(s, f"{dom}.Cb"), g(s, f"{dom}.Cr"), g(s, "rgb.RGB"), g(s, f"{dom}.Y.active"),
                    f"{tm.get('temporal.err_diff.all', float('nan')):.3f}",
                    f"{tm.get('temporal.frame_diff.static', float('nan')):.3f}"]
            if r["layout"] == "p":
                rows_p.append(base)
            else:
                cb = s.get("comb", {})
                rows_i.append(base + [g(s, f"{dom}.Y.field_top"), g(s, f"{dom}.Y.field_bottom"),
                                      f"{cb.get('ratio_large')}", f"{cb.get('err_rms')}"])
        for k, v in r.get("pairs", {}).items():
            pairs.append([r["clip"], k, f"{v['Y']['psnr']:.2f}", f"{v['RGB']['psnr']:.2f}"])
    hdr = ["clip", "test", "Y", "Y static", "Y small", "Y large", "Cb", "Cr", "RGB", "Y active", "tdiff(err) RMS",
           "fdiff static RMS"]
    print(f"scope {a.scope}, domain {a.domain} (PSNR dB; class fractions in each JSON)\n")
    for rows, h in ((rows_p, hdr), (rows_i, hdr + ["Y top", "Y bottom", "comb ratio (large)", "comb err RMS"])):
        if not rows:
            continue
        print("| " + " | ".join(h) + " |")
        print("|" + "---|" * len(h))
        for r in rows:
            print("| " + " | ".join(r) + " |")
        print()
    if pairs:
        print("| clip | pair | Y | RGB |\n|---|---|---|---|")
        for p in pairs:
            print("| " + " | ".join(p) + " |")


if __name__ == "__main__":
    main()
