"""Tables of the E9 frame-5 rerun (eval/rerun_f5/*.json from tools/eval_rerun_f5.py) -> eval/summary_rerun_f5.txt.

    python3 tools/summarize_rerun_f5.py --root WP_E9

Also checks that the evaluator gives the existing numbers: every metric of "input", "genuine" and A (the existing
outputs) in eval/rerun_f5/<clip>.json is compared with WP_E9/eval/<clip>.json ("input", "genuine" and
"ours_frame" / "ours_field"), all scopes, all keys present in both.
"""
import argparse
import glob
import json
import os

HERE = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
RUNS = ("input", "A", "B", "C", "genuine")


def flat(d, pre=""):
    out = {}
    for k, v in d.items():
        if isinstance(v, dict):
            out.update(flat(v, pre + k + "/"))
        elif not isinstance(v, list):
            out[pre + k] = v
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", required=True)
    a = ap.parse_args()
    files = sorted(glob.glob(os.path.join(HERE, "eval", "rerun_f5", "*.json")))
    order = ["old_town_cross_p_y3", "old_town_cross_p_y6", "park_joy_p_y3", "park_joy_p_y6", "sintel_p_y3", "sintel_p_y6",
             "old_town_cross_tff_y6", "park_joy_tff_y6", "sintel_tff_y3", "sintel_tff_y6"]
    files.sort(key=lambda f: order.index(os.path.basename(f)[:-5]) if os.path.basename(f)[:-5] in order else 99)
    L = []
    p = L.append
    # ---- evaluator check
    p("## evaluator check (rerun vs the existing eval/<clip>.json of --root; all scopes, every common key)\n")
    for f in files:
        r = json.load(open(f))
        old = json.load(open(os.path.join(a.root, "eval", os.path.basename(f))))
        aname = "ours_frame" if r["layout"] == "p" else "ours_field"
        for new_nm, old_nm in (("input", "input"), ("genuine", "genuine"), ("A", aname)):
            n = flat({s: r["tests"][new_nm][s] for s in ("all", "interior")})
            o = flat({s: old["tests"][old_nm][s] for s in ("all", "interior")})
            common = sorted(set(n) & set(o))
            diff = [k for k in common if n[k] != o[k]]
            p(f"- {r['clip']} {new_nm} vs existing {old_nm}: {len(common)} values, {len(diff)} differ"
              + (f" ({diff[:5]})" if diff else ""))
        pk_old = f"{aname}~genuine"
        if pk_old in old.get("pairs", {}):
            same = old["pairs"][pk_old] == r["pairs"]["A~genuine"]
            p(f"  pair A~genuine vs existing {pk_old}: {'same' if same else 'DIFFERENT'}")
    p("")
    # ---- main table
    def v(r, nm, key, scope="all", f="psnr"):
        t = r["tests"][nm][scope]
        return t.get(key, {}).get(f) if key in t else None

    def s(x):
        return "-" if x is None else f"{x:.2f}"

    p("## PSNR against the clean truth [dB] (all frames; yuv = YCbCr float, rgb = RGB)\n")
    keys = [("Y", "yuv.Y"), ("Cb", "yuv.Cb"), ("Cr", "yuv.Cr"), ("RGB", "rgb.RGB"), ("Y static", "yuv.Y.static"),
            ("Y small", "yuv.Y.small"), ("Y large", "yuv.Y.large"), ("Y active", "yuv.Y.active")]
    for lay in ("p", "i"):
        sel = [f for f in files if (json.load(open(f))["layout"] == "p") == (lay == "p")]
        extra = [("Y top", "yuv.Y.field_top"), ("Y bottom", "yuv.Y.field_bottom")] if lay == "i" else []
        hdr = ["clip", "run"] + [k for k, _ in keys + extra] + (["comb ratio (large)", "comb err RMS"] if lay == "i" else []) + \
              ["tdiff(err) RMS", "fdiff static RMS", "~genuine Y", "~genuine RGB"]
        p("| " + " | ".join(hdr) + " |")
        p("|" + "---|" * len(hdr))
        for f in sel:
            r = json.load(open(f))
            for nm in RUNS:
                row = [r["clip"], nm] + [s(v(r, nm, k)) for _, k in keys + extra]
                if lay == "i":
                    cb = r["tests"][nm]["all"].get("comb", {})
                    row += [f"{cb.get('ratio_large')}", f"{cb.get('err_rms')}"]
                tm = r["tests"][nm]["all"].get("temporal_rms", {})
                row += [f"{tm.get('temporal.err_diff.all', float('nan')):.3f}",
                        f"{tm.get('temporal.frame_diff.static', float('nan')):.3f}"]
                pr = r.get("pairs", {}).get(f"{nm}~genuine")
                row += [s(pr["Y"]["psnr"]) if pr else "-", s(pr["RGB"]["psnr"]) if pr else "-"]
                p("| " + " | ".join(row) + " |")
        p("")
    # ---- decomposition
    p("## decomposition (Y / Cb / Cr / RGB, dB): B-A = the profile frame, C-B = 16-bit input + stacked fields; "
      "gap = genuine - run\n")
    hdr = ["clip", "comp", "B-A", "C-B", "C-A", "gap A", "gap B", "gap C", "gap closed (A->C)", "~genuine A", "~genuine B",
           "~genuine C"]
    p("| " + " | ".join(hdr) + " |")
    p("|" + "---|" * len(hdr))
    for f in files:
        r = json.load(open(f))
        for comp, k in (("Y", "yuv.Y"), ("Cb", "yuv.Cb"), ("Cr", "yuv.Cr"), ("RGB", "rgb.RGB")):
            A, B, C, G = (v(r, nm, k) for nm in ("A", "B", "C", "genuine"))
            if k == "yuv.Y":
                pg = [s(r["pairs"][f"{nm}~genuine"]["Y"]["psnr"]) for nm in ("A", "B", "C")]
            elif k == "rgb.RGB":
                pg = [s(r["pairs"][f"{nm}~genuine"]["RGB"]["psnr"]) for nm in ("A", "B", "C")]
            else:
                pg = ["", "", ""]
            p(f"| {r['clip']} | {comp} | {B - A:+.2f} | {C - B:+.2f} | {C - A:+.2f} | {G - A:.2f} | {G - B:.2f} | "
              f"{G - C:.2f} | {(G - A) - (G - C):+.2f} | " + " | ".join(pg) + " |")
    out = os.path.join(HERE, "eval", "summary_rerun_f5.txt")
    open(out, "w").write("\n".join(L) + "\n")
    print("\n".join(L))
    print("wrote", out)


if __name__ == "__main__":
    main()
