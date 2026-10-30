"""Run every item of qtui_selftest on its own (G5, N3c verification I-5: `qtui_selftest <item>` must pass without the
items before it), one process per item, and print PASS / FAIL per item.

    python3 qtui/tools/selftest_each.py [--exe qtui/build_main/qtui_selftest] [--video test.mp4] [--only a,b] [--log DIR]

Exit code 0 when every item passed; 1 when an item failed, an --only name is not an item or nothing ran.  With --log, the output of each run goes to DIR/<item>.txt.
"""
import argparse
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default=os.path.join(ROOT, "qtui", "build_main", "qtui_selftest"))
    ap.add_argument("--video", default=os.path.join(ROOT, "test.mp4"))
    ap.add_argument("--only", default="")
    ap.add_argument("--log", default="")
    ap.add_argument("--timeout", type=float, default=900)
    A = ap.parse_args()
    env = dict(os.environ, QT_QPA_PLATFORM="offscreen", NV_TEST_VIDEO=os.path.abspath(A.video))
    items = subprocess.run([A.exe, "-functions"], capture_output=True, text=True, env=env, check=True).stdout.split()
    items = [i.rstrip("()") for i in items]
    if A.only:
        want = [w for w in A.only.split(",") if w]
        unknown = [w for w in want if w not in items]
        items = [i for i in items if i in want]
        if unknown:
            print(f"no such item: {', '.join(unknown)}")
    if not items or (A.only and unknown):           # nothing (or not everything asked for) to run: a failure
        print("0 items run" if not items else "some items do not exist")
        return 1
    if A.log:
        os.makedirs(A.log, exist_ok=True)
    bad = []
    for it in items:
        t0 = time.time()
        try:
            r = subprocess.run([A.exe, it], capture_output=True, text=True, env=env, timeout=A.timeout)
            out, rc = r.stdout + r.stderr, r.returncode
        except subprocess.TimeoutExpired as e:
            out, rc = (e.stdout or b"").decode(errors="replace") + "\nTIMEOUT", "TIMEOUT"
        ok = rc == 0 and f"PASS   : SelfTest::{it}()" in out
        if A.log:
            with open(os.path.join(A.log, it + ".txt"), "w") as fh:
                fh.write(out)
        tail = "" if ok else " | " + " / ".join(l.strip() for l in out.splitlines() if "FAIL" in l or "Loc:" in l)[:300]
        print(f"{'PASS' if ok else 'FAIL'} {it:40s} {time.time() - t0:6.1f} s  exit {rc}{tail}", flush=True)
        if not ok:
            bad.append(it)
    print(f"{len(items) - len(bad)}/{len(items)} items pass on their own" + (f"; failing: {', '.join(bad)}" if bad else ""))
    return 0 if not bad else 1


if __name__ == "__main__":
    sys.exit(main())
