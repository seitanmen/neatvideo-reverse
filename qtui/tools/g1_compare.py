"""G1 (R/G/B equalizer) comparison of the C++ UI with the PySide6 UI: runs qtui_g1eq (qtui/tests/g1_eq.cpp) and
g1_py_eq.py on the same clips and cases (the default flow Auto Profile + Auto Fine-Tune, then the equalizer edits of
the case) and compares byte for byte:
  - the ProfileState JSON, the effective profile JSON, the backend parameters, Quality / Completeness and the
    processed frame (float32 Y, Cb, Cr) of both UIs;
  - the exports (4 frames, CRF 0) of the case: Python UI export native on, native off, C++ UI export.
With --before DIR (a run of the code before G1 with the same layout) the default case's processed frame and exports
of both UIs are also compared with the earlier ones (the default output must not change).

    QT_QPA_PLATFORM=offscreen python3 qtui/tools/g1_compare.py OUTDIR [--exe qtui/build_main] [--before DIR]
                                                               [--cases a,b,..] [--clips test.mp4:300,...]
Writes only into OUTDIR.
"""
import argparse
import filecmp
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
REPRO = os.path.join(ROOT, "repro")
CASES = "default,r4,gall,all2,b6tune,reset,r4ac"
EXPORT_CASES = ("default", "r4", "gall", "all2", "b6tune", "reset")
FILES = ("nvprof.json", "ui_profile.json", "ui_params.json", "quality.json", "planes.f32")


def run(cmd, env=None, cwd=None):
    r = subprocess.run(cmd, env=dict(os.environ, **(env or {})), cwd=cwd, capture_output=True, text=True)
    if r.returncode != 0:
        print("FAILED:", " ".join(cmd), "\n", r.stdout[-2000:], r.stderr[-2000:])
    return r.returncode == 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--exe", default=os.path.join(ROOT, "qtui", "build_main"))
    ap.add_argument("--before")
    ap.add_argument("--cases", default=CASES)
    ap.add_argument("--clips", default="test.mp4:300,test_v02.mp4:100")
    A = ap.parse_args()
    cases = A.cases.split(",")
    bad = 0
    for clip in A.clips.split(","):
        name, f = clip.split(":")
        video, F = os.path.join(ROOT, name), int(f)
        tag = os.path.splitext(name)[0]
        py_dir, cpp_dir = (os.path.join(A.out, s, tag) for s in ("eq_py", "eq_cpp"))
        os.makedirs(py_dir, exist_ok=True)
        os.makedirs(cpp_dir, exist_ok=True)
        run([sys.executable, os.path.join(HERE, "g1_py_eq.py"), video, py_dir, str(F), ",".join(cases)],
            env={"NV_NATIVE": "1"})
        for c in cases:
            run([os.path.join(A.exe, "qtui_g1eq"), video, cpp_dir, str(F), c])
            diffs = [e for e in FILES if not (os.path.exists(os.path.join(py_dir, f"{c}.{e}")) and
                                              filecmp.cmp(os.path.join(py_dir, f"{c}.{e}"),
                                                          os.path.join(cpp_dir, f"{c}.{e}"), shallow=False))]
            q = open(os.path.join(py_dir, f"{c}.quality.json")).read()
            print(f"{tag:9s} {c:8s} state/frame py vs cpp: {'SAME' if not diffs else 'DIFF ' + ','.join(diffs)}  "
                  f"(Quality, Completeness) {q}")
            bad += bool(diffs)
            if c not in EXPORT_CASES:
                continue
            outs = {}
            common = ["--crf", "0", "--start", str(F - 2), "--frames", "4", "--jobs", "2"]
            for side, d, env in (("py_on", py_dir, {"NV_NATIVE": "1"}), ("py_off", py_dir, {"NV_NATIVE": "0"})):
                dst = os.path.join(d, f"{c}_{side}.mp4")
                run([sys.executable, "-m", "nv_ui_qt.export", video, dst, "--ui-profile",
                     os.path.join(d, f"{c}.ui_profile.json"), "--params", os.path.join(d, f"{c}.ui_params.json"),
                     *common], env=env, cwd=REPRO)
                outs[side] = dst
            dst = os.path.join(cpp_dir, f"{c}_cpp.mp4")
            run([os.path.join(A.exe, "qtui_export"), video, dst, "--ui-profile", os.path.join(cpp_dir, f"{c}.ui_profile.json"),
                 "--params", os.path.join(cpp_dir, f"{c}.ui_params.json"), *common])
            outs["cpp"] = dst
            same = all(os.path.exists(p) for p in outs.values()) and \
                filecmp.cmp(outs["py_on"], outs["py_off"], shallow=False) and \
                filecmp.cmp(outs["py_on"], outs["cpp"], shallow=False)
            print(f"{tag:9s} {c:8s} export py on / py off / cpp: {'SAME' if same else 'DIFF'} "
                  f"({os.path.getsize(outs['py_on']) if os.path.exists(outs['py_on']) else 0} B)")
            bad += not same
            if A.before and c == "default":
                bp, bc = (os.path.join(A.before, s, tag) for s in ("eq_py", "eq_cpp"))
                checks = [(os.path.join(bp, "default.planes.f32"), os.path.join(py_dir, "default.planes.f32")),
                          (os.path.join(bc, "default.planes.f32"), os.path.join(cpp_dir, "default.planes.f32")),
                          (os.path.join(bp, "default_on.mp4"), outs["py_on"]),
                          (os.path.join(bc, "default_on.mp4"), outs["cpp"])]
                same = all(os.path.exists(a) and filecmp.cmp(a, b, shallow=False) for a, b in checks)
                print(f"{tag:9s} default  frame + export vs the code before G1 (py, cpp): {'SAME' if same else 'DIFF'}")
                bad += not same
    print("RESULT:", "ALL SAME" if bad == 0 else f"{bad} DIFFERENCES")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
