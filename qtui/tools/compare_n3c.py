"""N3c comparison of the C++ UI with the PySide6 UI: runs qtui_dump_n3c (qtui/tests/dump_n3c.cpp) and py_dump_n3c.py
on the same inputs, then compares
  - with compare_ui.py: state_*.json (main window strings + Performance table, Image information, export dialog
    defaults; Profile Viewer rows and text; Batch Profiler dialog), menus.json, panes/*.rgb (the viewer panes of
    10-bit, 4:2:2 and 32-bit float frames), *.png grabs (dialogs and windows);
  - here: the Preferences rows (prefs_rows.json; the C++ UI's fixed native-core box is reported as the known
    difference), the Batch Profiler's profiles (batch_*.nvprof.json text), the exports
    (mp4 byte for byte, mkv decoded frames + file length) with their side files, runs.json and the [prefs] of
    settings.ini (both UIs write the shared keys).

    NV_NATIVE=1 python3 qtui/tools/compare_n3c.py OUTDIR [--exe qtui/build/qtui_dump_n3c] [--skip-run]
"""
import argparse
import glob
import json
import os
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
REPRO = os.path.join(ROOT, "repro")
CPP_ONLY_ROWS = ()      # (G2: the Python UI has the Auto Fine-Tune and ffmpeg rows of the C++ UI too)


def make_inputs(d):
    os.makedirs(d, exist_ok=True)
    src10, src422 = os.path.join(d, "src10.mkv"), os.path.join(d, "src422.mkv")
    t = os.path.join(ROOT, "test.mp4")
    if not os.path.exists(src10):
        raw = subprocess.run(["ffmpeg", "-v", "error", "-ss", "10", "-i", t, "-frames:v", "12", "-f", "rawvideo",
                              "-pix_fmt", "yuv420p", "-"], capture_output=True, check=True).stdout
        a = np.frombuffer(raw, np.uint8).astype("<u2") * 4 + np.random.default_rng(10).integers(0, 4, len(raw)).astype("<u2")
        tags = ["-colorspace", "bt709", "-color_range", "tv"]
        subprocess.run(["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "yuv420p10le", "-s", "640x360", "-r", "30",
                        *tags, "-i", "-", "-c:v", "ffv1", "-pix_fmt", "yuv420p10le", *tags, src10], input=a.tobytes(), check=True)
    if not os.path.exists(src422):
        raw = subprocess.run(["ffmpeg", "-v", "error", "-ss", "20", "-i", t, "-frames:v", "6", "-vf", "crop=320:180:200:100",
                              "-f", "rawvideo", "-pix_fmt", "yuv422p", "-"], capture_output=True, check=True).stdout
        a = np.frombuffer(raw, np.uint8).astype("<u2") * 4 + np.random.default_rng(422).integers(0, 4, len(raw)).astype("<u2")
        tags = ["-colorspace", "bt709", "-color_range", "tv"]
        subprocess.run(["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "yuv422p10le", "-s", "320x180", "-r", "24",
                        *tags, "-i", "-", "-c:v", "ffv1", "-pix_fmt", "yuv422p10le", *tags, src422], input=a.tobytes(), check=True)
    return src10, src422


def menus_same(a, b, label):
    """The menu bars item by item (text, separator, enabled, checkable, shortcut) apart from the C++ UI's Help > About Qt."""
    n = sum(len(m["items"]) for m in a)
    bq = [{"title": m["title"], "items": [x for x in m["items"] if x["text"] != "About Qt"]} for m in b]
    extra = sum(len(m["items"]) for m in b) - sum(len(m["items"]) for m in bq)
    diffs = [(m1["title"], x, y) for m1, m2 in zip(a, bq) for x, y in zip(m1["items"], m2["items"]) if x != y]
    lens = [(m1["title"], len(m1["items"]), len(m2["items"])) for m1, m2 in zip(a, bq) if len(m1["items"]) != len(m2["items"])]
    same = not diffs and not lens and len(a) == len(bq) and [m["title"] for m in a] == [m["title"] for m in bq]
    en = sum(x["enabled"] for m in a for x in m["items"])
    print(f"== {label}: {n} Python items ({en} enabled), C++ {n + extra} (+{extra} About Qt): "
          f"{'identical apart from About Qt' if same else 'DIFFERENT'}")
    for d in (diffs + lens)[:10]:
        print("     ", d)
    return same


def framemd5(path):
    r = subprocess.run(["ffmpeg", "-v", "error", "-i", path, "-map", "0:v", "-f", "framemd5", "-"], capture_output=True, text=True)
    return [l for l in r.stdout.splitlines() if l and not l.startswith("#")]


def prefs_section(path):
    out, on = [], False
    for line in open(path):
        line = line.rstrip("\n")
        if line.startswith("["):
            on = line == "[prefs]"
            continue
        if on and line:
            out.append(line)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--exe", default=os.path.join(ROOT, "qtui", "build", "qtui_dump_n3c"))
    ap.add_argument("--skip-run", action="store_true")
    A = ap.parse_args()
    out = os.path.abspath(A.out)
    src10, src422 = make_inputs(os.path.join(out, "inputs"))
    py, cpp = os.path.join(out, "py"), os.path.join(out, "cpp")
    env = dict(os.environ, QT_QPA_PLATFORM="offscreen", NV_NATIVE="1")
    if not A.skip_run:
        for d, cmd in ((py, [sys.executable, os.path.join(HERE, "py_dump_n3c.py"), py, src10, src422]),
                       (cpp, [A.exe, cpp, src10, src422])):
            os.makedirs(d, exist_ok=True)
            r = subprocess.run(cmd, capture_output=True, text=True, env=env, cwd=REPRO, timeout=1800)
            open(os.path.join(d, "run.log"), "w").write(r.stdout + r.stderr)
            print(f"{os.path.basename(d)}: exit {r.returncode}; " + " | ".join(l for l in r.stdout.splitlines()), flush=True)
    ok = True
    print("== compare_ui (states, menus, panes, grabs)", flush=True)
    r = subprocess.run([sys.executable, os.path.join(HERE, "compare_ui.py"), py, cpp, os.path.join(out, "sbs")],
                       capture_output=True, text=True)
    print(r.stdout.rstrip())
    ok = ok and r.returncode == 0
    ma, mb = json.load(open(os.path.join(py, "menus.json"))), json.load(open(os.path.join(cpp, "menus.json")))
    ok = ok and menus_same(ma, mb, "menus.json (video + profile)")
    print("== Preferences rows")
    a, b = json.load(open(os.path.join(py, "prefs_rows.json"))), json.load(open(os.path.join(cpp, "prefs_rows.json")))
    extra = [x for x in b["rows"] if x.get("label") in CPP_ONLY_ROWS or x.get("text") in CPP_ONLY_ROWS]
    rows_b = [x for x in b["rows"] if x not in extra]
    known, diffs = [], []
    for x, y in zip(a["rows"], rows_b):
        if x.get("text", "").startswith("Use the native core"):
            for k in x:
                if x[k] != y.get(k):
                    known.append(f"native core box: {k} {x[k]!r} (Python) vs {y.get(k)!r} (C++)")
            continue
        if x != y:
            diffs.append((x, y))
    same_n = len(a["rows"]) == len(rows_b) and a["title"] == b["title"]
    print(f"  {len(a['rows'])} Python rows vs {len(rows_b)} C++ rows (+{len(extra)} C++-only: "
          f"{[x.get('label') or x.get('text') for x in extra]}); other rows {'identical' if not diffs and same_n else 'DIFFER'}")
    for k in known:
        print("   known:", k)
    for x, y in diffs[:6]:
        print("   DIFF:", x, "|", y)
    ok = ok and not diffs and same_n
    print("== Batch Profiler profiles")
    for f in sorted(os.path.basename(p) for p in glob.glob(os.path.join(py, "batch_*.nvprof.json"))):
        pa, pb = os.path.join(py, f), os.path.join(cpp, f)
        same = os.path.exists(pb) and open(pa, "rb").read() == open(pb, "rb").read()
        print(f"  {f}: {'identical text' if same else 'DIFFERENT'} ({os.path.getsize(pa)} B)")
        ok = ok and same
    print("== exports")
    for f in sorted(os.path.basename(p) for p in glob.glob(os.path.join(py, "export_*")) if not p.endswith(".json")):
        pa, pb = os.path.join(py, f), os.path.join(cpp, f)
        if f.endswith(".mkv"):
            fa, fb = framemd5(pa), framemd5(pb) if os.path.exists(pb) else []
            same = bool(fa) and fa == fb and os.path.getsize(pa) == os.path.getsize(pb)
            how = f"decoded frames {len(fa)} + length"
        else:
            same = os.path.exists(pb) and open(pa, "rb").read() == open(pb, "rb").read()
            how = "byte for byte"
        print(f"  {f}: {'SAME' if same else 'DIFF'} ({how}, {os.path.getsize(pa)} B)")
        ok = ok and same
        stem = os.path.splitext(f)[0]
        for side in (".ui_params.json", ".ui_profile.json"):
            sa, sb = os.path.join(py, stem + side), os.path.join(cpp, stem + side)
            s = os.path.exists(sb) and open(sa, "rb").read() == open(sb, "rb").read()
            print(f"    {stem + side}: {'identical text' if s else 'DIFFERENT'}")
            ok = ok and s
        ma, mb = json.load(open(os.path.join(py, stem + ".json"))), json.load(open(os.path.join(cpp, stem + ".json")))
        for m in (ma, mb):
            m.pop("seconds", None)
            m["src"] = os.path.basename(m.get("src", ""))
        s = json.dumps(ma, sort_keys=True) == json.dumps(mb, sort_keys=True)
        print(f"    {stem}.json (without seconds): {'equal' if s else 'DIFFERENT'}")
        ok = ok and s
    ra, rb = json.load(open(os.path.join(py, "runs.json"))), json.load(open(os.path.join(cpp, "runs.json")))
    print(f"== runs.json: {'equal' if ra == rb else 'DIFFERENT'} {rb}")
    ok = ok and ra == rb
    pa, pb = prefs_section(os.path.join(py, "settings.ini")), prefs_section(os.path.join(cpp, "settings.ini"))
    print(f"== settings.ini [prefs]: {'identical' if pa == pb else 'DIFFERENT'} ({len(pa)} keys)")
    if pa != pb:
        print("   py :", pa)
        print("   cpp:", pb)
    ok = ok and pa == pb
    print("RESULT:", "EQUAL (apart from the known differences)" if ok else "DIFFERENT")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
