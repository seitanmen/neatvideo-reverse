"""G1 (R/G/B equalizer) check of the C++ UI against a C++ build of the code before G1 (C++-first policy: no PySide6).

    python3 qtui/tools/g1_compare.py OUTDIR --before BUILD_DIR [--exe qtui/build_main] [--cases a,b,..]
                                     [--clips test.mp4:300,test_v02.mp4:100]

BUILD_DIR holds qtui_g1eq and qtui_export built from the code before G1 (git archive of the base commit, with
qtui/tests/g1_eq.cpp added and its after_aft / reset_fine_tune / auto_complete written as the old MainWindow did).
For every clip:
  - the default flow (Auto Profile + Auto Fine-Tune, no edit): the processed frame of qtui_g1eq, the effective
    profile's curve and sigma, Quality / Completeness and the 4-frame CRF 0 export equal the build before G1 byte for
    byte (the default output does not change);
  - every case (equalizer edits, Reset, Auto Complete): two runs of qtui_g1eq and of the export give the same bytes
    (C++ determinism).
Writes only into OUTDIR.
"""
import argparse
import filecmp
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
CASES = "default,r4,gall,all2,b6tune,reset,r4ac"
FILES = ("nvprof.json", "ui_profile.json", "ui_params.json", "quality.json", "planes.f32")


def run(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print("FAILED:", " ".join(cmd), "\n", r.stdout[-2000:], r.stderr[-2000:])
    return r.returncode == 0


def same(a, b):
    return os.path.exists(a) and os.path.exists(b) and filecmp.cmp(a, b, shallow=False)


def export(exe_dir, video, d, case, F, tag):
    dst = os.path.join(d, f"{case}_{tag}.mp4")
    run([os.path.join(exe_dir, "qtui_export"), video, dst, "--ui-profile", os.path.join(d, f"{case}.ui_profile.json"),
         "--params", os.path.join(d, f"{case}.ui_params.json"), "--crf", "0", "--start", str(F - 2), "--frames", "4",
         "--jobs", "2"])
    return dst


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--before", required=True)
    ap.add_argument("--exe", default=os.path.join(ROOT, "qtui", "build_main"))
    ap.add_argument("--cases", default=CASES)
    ap.add_argument("--clips", default="test.mp4:300,test_v02.mp4:100")
    A = ap.parse_args()
    bad = 0
    for clip in A.clips.split(","):
        name, f = clip.split(":")
        video, F = os.path.join(ROOT, name), int(f)
        tag = os.path.splitext(name)[0]
        dirs = {k: os.path.join(A.out, k, tag) for k in ("before", "run1", "run2")}
        for d in dirs.values():
            os.makedirs(d, exist_ok=True)
        run([os.path.join(A.before, "qtui_g1eq"), video, dirs["before"], str(F), "default"])
        for c in A.cases.split(","):
            for k in ("run1", "run2"):
                run([os.path.join(A.exe, "qtui_g1eq"), video, dirs[k], str(F), c])
            det = [e for e in FILES if not same(os.path.join(dirs["run1"], f"{c}.{e}"), os.path.join(dirs["run2"], f"{c}.{e}"))]
            e1, e2 = (export(A.exe, video, dirs[k], c, F, "cpp") for k in ("run1", "run2"))
            det += [] if same(e1, e2) else ["export"]
            q = open(os.path.join(dirs["run1"], f"{c}.quality.json")).read()
            print(f"{tag:9s} {c:8s} two runs: {'SAME' if not det else 'DIFF ' + ','.join(det)}  (Quality, Completeness) {q}",
                  flush=True)
            bad += bool(det)
            if c != "default":
                continue
            b, a = dirs["before"], dirs["run1"]
            pb, pa = (json.load(open(os.path.join(d, "default.ui_profile.json"))) for d in (b, a))
            eb = export(A.before, video, b, "default", F, "before")
            ok = {"frame": same(os.path.join(b, "default.planes.f32"), os.path.join(a, "default.planes.f32")),
                  "curve/sigma": pb["curve"] == pa["curve"] and pb["sigma"] == pa["sigma"],
                  "quality": same(os.path.join(b, "default.quality.json"), os.path.join(a, "default.quality.json")),
                  "export": same(eb, e1),
                  "params": same(os.path.join(b, "default.ui_params.json"), os.path.join(a, "default.ui_params.json"))}
            print(f"{tag:9s} default  vs the build before G1: " + ", ".join(f"{k} {'SAME' if v else 'DIFF'}"
                                                                     for k, v in ok.items()), flush=True)
            bad += not all(ok.values())
    print("RESULT:", "ALL SAME" if bad == 0 else f"{bad} DIFFERENCES")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
