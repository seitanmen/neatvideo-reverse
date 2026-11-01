"""Runs e9_eval.py on every clip with the outputs found at the conventional places (2 at a time):

    runs/ours_frame/<src>/<layout>_<level>/out.mp4     the reproduction, frame processing (tools/run_ours.py)
    runs/ours_field/<src>/<layout>_<level>/out.mp4     the reproduction, field processing (INT-3, default variant)
    runs/ours_field_<variant>/<src>/<layout>_<level>/out.mp4   other field-processing variants (tools/run_ours_fields.py)
    genuine/<src>/<layout>_<level>/                    the genuine's export (16-bit RGB TIFF sequence, one folder)
    genuine_noeffect/<src>/<layout>_<level>/           Resolve's export of the same clip without any effect (floor)
A genuine / genuine_noeffect folder is found, in this order, by
  1. genuine_map.json (wp_e9 root): {"genuine": {"<src>/<clip>": "folder", ...}, "genuine_noeffect": {...}};
  2. the conventional path above;
  3. a search below genuine/ (genuine_noeffect/) for the one folder whose files are named <src>_<clip>_<digits>.tif
     (Resolve names the rendered frames after the clip, e.g. progressive/Event__0001_0003/park_joy_p_y3_00000.tif).
The folder used is printed; an ambiguous search (several folders) is reported and skipped.

    python3 tools/eval_all.py [CLIP ...] [--jobs 2]
"""
import argparse
import glob
import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))


def clips():
    out = []
    for f in sorted(glob.glob(os.path.join(ROOT, "clips", "*", "noisy_*.y4m"))):
        src = os.path.basename(os.path.dirname(f))
        out.append(src + "/" + os.path.basename(f)[len("noisy_"):-len(".y4m")])
    return out


def find_export(base, clip, notes):
    """The export folder of `clip` below `base` (genuine / genuine_noeffect), or None."""
    src, lv = clip.split("/")
    mp = os.path.join(ROOT, "genuine_map.json")
    if os.path.exists(mp):
        p = json.load(open(mp)).get(base, {}).get(clip)
        if p:
            p = os.path.join(ROOT, p)
            if os.path.isdir(p) and glob.glob(os.path.join(p, "*.tif*")):
                notes.append(f"{base}: {os.path.relpath(p, ROOT)} (genuine_map.json)")
                return p
    p = os.path.join(ROOT, base, src, lv)
    if os.path.isdir(p):
        notes.append(f"{base}: {os.path.relpath(p, ROOT)}")
        return p
    pat = re.compile(rf"^{re.escape(src)}_{re.escape(lv)}_\d+\.tiff?$")
    hits = sorted({os.path.dirname(f) for f in glob.glob(os.path.join(ROOT, base, "**", "*.tif*"), recursive=True)
                   if pat.match(os.path.basename(f))})
    if len(hits) == 1:
        notes.append(f"{base}: {os.path.relpath(hits[0], ROOT)} (found by file name)")
        return hits[0]
    if len(hits) > 1:
        notes.append(f"{base}: ambiguous, skipped: " + ", ".join(os.path.relpath(h, ROOT) for h in hits))
    return None


def one(clip):
    src, lv = clip.split("/")
    tests = []
    notes = []
    variants = sorted(os.path.basename(d) for d in glob.glob(os.path.join(ROOT, "runs", "ours_field_*")))
    for nm, p in [("ours_frame", f"runs/ours_frame/{src}/{lv}/out.mp4"), ("ours_field", f"runs/ours_field/{src}/{lv}/out.mp4")] + \
            [(v, f"runs/{v}/{src}/{lv}/out.mp4") for v in variants]:
        if os.path.exists(os.path.join(ROOT, p)):
            tests += ["--test", f"{nm}={os.path.join(ROOT, p)}"]
    g = find_export("genuine", clip, notes)
    if g:
        tests += ["--test", f"genuine={g}"]
    logs = ["".join(n + "\n" for n in notes)]
    if tests:
        r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "e9_eval.py"), clip] + tests,
                           capture_output=True, text=True)
        logs.append(r.stdout + r.stderr)
    fl = find_export("genuine_noeffect", clip, notes)
    logs[0] = "".join(n + "\n" for n in notes)
    if fl:
        r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "e9_eval.py"), clip, "--floor", "--test",
                            f"noeffect={fl}"], capture_output=True, text=True)
        logs.append(r.stdout + r.stderr)
    return clip, "".join(logs) or "(no outputs)\n"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("clips", nargs="*")
    ap.add_argument("--jobs", type=int, default=2)
    a = ap.parse_args()
    with ThreadPoolExecutor(min(a.jobs, 2)) as ex:
        for clip, log in ex.map(one, a.clips or clips()):
            print(f"== {clip}\n{log}", end="", flush=True)


if __name__ == "__main__":
    main()
