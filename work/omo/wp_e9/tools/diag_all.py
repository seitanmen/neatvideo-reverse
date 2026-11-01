"""Runs diag_removed.py and diag_temporal.py for the reproduction (runs/ours_frame, runs/ours_field, runs/ours_field_*)
and the genuine export (found as eval_all.py finds it) on the given clips, 2 processes at a time.

    python3 tools/diag_all.py CLIP [CLIP ...]
"""
import glob
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from eval_all import ROOT, find_export  # noqa: E402


def jobs(clip):
    src, lv = clip.split("/")
    tests = []
    for d in ["ours_frame", "ours_field"] + sorted(os.path.basename(x) for x in glob.glob(os.path.join(ROOT, "runs", "ours_field_*"))):
        p = os.path.join(ROOT, "runs", d, src, lv, "out.mp4")
        if os.path.exists(p):
            tests.append((d, p))
    g = find_export("genuine", clip, [])
    if g:
        tests.append(("genuine", g))
    return [[sys.executable, os.path.join(ROOT, "tools", tool), clip, "--test", p, "--name", nm]
            for tool in ("diag_removed.py", "diag_temporal.py") for nm, p in tests]


def run(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    return " ".join(os.path.basename(c) for c in cmd[1:3]) + (f" --name {cmd[-1]}" if "--name" in cmd else "") + \
        f" rc={r.returncode}\n" + (r.stderr[-2000:] if r.returncode else "")


if __name__ == "__main__":
    allj = [j for c in sys.argv[1:] for j in jobs(c)]
    with ThreadPoolExecutor(2) as ex:
        for line in ex.map(run, allj):
            print(line, end="", flush=True)
