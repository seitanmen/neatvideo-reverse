"""The E9 frame-5 rerun (eval/E9_rerun_frame5.md): runs e9_eval.py of the wp_e9 folder that holds the material (--root,
unchanged evaluator) on A / B / C and the genuine, writing to this checkout's eval/rerun_f5/<src>_<clip>.json
(the existing eval/*.json of --root are not touched).

    python3 tools/eval_rerun_f5.py --root WP_E9 [CLIP ...] [--jobs 2]

    A = runs/ours_frame (progressive) / runs/ours_field (interlaced): the existing outputs (frame 0, the former code)
    B = runs/ours_frame_f5 / runs/ours_field_f5: frame 5, the main sources
    C = runs/ours_frame_f5q16 / runs/ours_field_f5q16: frame 5, the aft2-callers sources
The genuine's export folder is found as eval_all.py finds it (its find_export).
"""
import argparse
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
CLIPS = ["old_town_cross/p_y3", "old_town_cross/p_y6", "park_joy/p_y3", "park_joy/p_y6", "sintel/p_y3", "sintel/p_y6",
         "old_town_cross/tff_y6", "park_joy/tff_y6", "sintel/tff_y3", "sintel/tff_y6"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("clips", nargs="*")
    ap.add_argument("--root", required=True)
    ap.add_argument("--jobs", type=int, default=2)
    a = ap.parse_args()
    root = os.path.abspath(a.root)
    sys.path.insert(0, os.path.join(root, "tools"))
    import eval_all  # noqa: E402  (the --root copy: its ROOT is --root)
    outdir = os.path.join(HERE, "eval", "rerun_f5")
    os.makedirs(outdir, exist_ok=True)

    def one(clip):
        src, lv = clip.split("/")
        kind = "frame" if lv.startswith("p_") else "field"
        tests = []
        for nm, tag in (("A", ""), ("B", "_f5"), ("C", "_f5q16")):
            p = os.path.join(root, "runs", f"ours_{kind}{tag}", src, lv, "out.mp4")
            if not os.path.exists(p):
                raise SystemExit(f"missing {p}")
            tests += ["--test", f"{nm}={p}"]
        notes = []
        g = eval_all.find_export("genuine", clip, notes)
        if not g:
            raise SystemExit(f"{clip}: no genuine export ({notes})")
        tests += ["--test", f"genuine={g}"]
        cmd = [sys.executable, os.path.join(root, "tools", "e9_eval.py"), clip] + tests + \
              ["--out", os.path.join(outdir, f"{src}_{lv}.json")]
        r = subprocess.run(cmd, capture_output=True, text=True, cwd=root)
        return clip, "".join(n + "\n" for n in notes) + "$ " + " ".join(cmd) + "\n" + r.stdout + r.stderr, r.returncode

    bad = 0
    with ThreadPoolExecutor(min(a.jobs, 2)) as ex:
        for clip, log, rc in ex.map(one, a.clips or CLIPS):
            print(f"== {clip} (rc {rc})\n{log}", end="", flush=True)
            bad += rc != 0
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
