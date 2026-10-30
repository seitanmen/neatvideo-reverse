#!/usr/bin/env python3
# G9: a stand-in for a native test binary (verify_*.py --exe, or NV_*_EXE) that runs the x86_64 build and the arm64
# build of it on the same arguments and keeps both outputs, so the verify script (fed the arm64 outputs, as usual)
# drives the x86_64 binary over its own inputs (the NaN / inf / special-value cases included).
#
#   NV_PAIR_ARM=<arm64 binary> NV_PAIR_X86=<x86_64 binary> NV_PAIR_DIR=<dir> python3 native/verify_spatial.py \
#       --exe native/x86_exe_pair.py
#   python3 native/x86_exe_pair.py --report <dir>      (word-level comparison of the kept pairs)
#
# Output files are found as the arguments that name a file the run created or changed; stdout is kept too.
# The x86_64 binary runs first (its outputs are copied to <dir>/<call>/x86_<argv index>), then the arm64 one,
# whose outputs and stdout the caller sees (<dir>/<call>/arm_<argv index>).  NV_PAIR_KEEP_IN=1 also keeps the input
# files (<dir>/<call>/in_<argv index>).
import hashlib
import os
import shutil
import subprocess
import sys
import uuid


def _state(paths):
    out = {}
    for i, p in paths:
        try:                                        # a rewrite with the same bytes counts as written (mtime)
            with open(p, "rb") as f:
                out[i] = (os.stat(p).st_mtime_ns, hashlib.sha256(f.read()).hexdigest())
        except OSError:
            out[i] = None
    return out


def run():
    arm, x86, d = os.environ["NV_PAIR_ARM"], os.environ["NV_PAIR_X86"], os.environ["NV_PAIR_DIR"]
    args = sys.argv[1:]
    cand = [(i, a) for i, a in enumerate(args) if os.sep in a or os.path.exists(a)]
    call = os.path.join(d, uuid.uuid4().hex[:12])
    os.makedirs(call)
    with open(os.path.join(call, "argv"), "w") as f:
        f.write("\n".join(args))
    before = _state(cand)
    rx = subprocess.run([x86] + args, capture_output=True)
    after = _state(cand)
    outs = [i for i, _ in cand if after[i] is not None and after[i] != before[i]]
    for i in outs:
        shutil.copyfile(args[i], os.path.join(call, f"x86_{i}"))
    with open(os.path.join(call, "x86_stdout"), "wb") as f:
        f.write(rx.stdout)
    ra = subprocess.run([arm] + args, capture_output=True)
    after2 = _state(cand)
    outs = sorted(set(outs) | {i for i, _ in cand if after2[i] is not None and after2[i] != before[i]})
    for i in outs:
        shutil.copyfile(args[i], os.path.join(call, f"arm_{i}"))
    if os.environ.get("NV_PAIR_KEEP_IN"):           # the input files too (for a per-case analysis)
        for i, a in cand:
            if i not in outs and os.path.isfile(a):
                shutil.copyfile(a, os.path.join(call, f"in_{i}"))
    with open(os.path.join(call, "arm_stdout"), "wb") as f:
        f.write(ra.stdout)
    with open(os.path.join(call, "rc"), "w") as f:
        f.write(f"{rx.returncode} {ra.returncode}")
    sys.stdout.buffer.write(ra.stdout)
    sys.stderr.buffer.write(ra.stderr)
    return ra.returncode


def report(d):
    import numpy as np
    tot = {"calls": 0, "same": 0, "bytes": 0, "words": 0, "diff": 0, "both_nan": 0, "one_nan": 0, "finite": 0}
    rows = {}
    for c in sorted(os.listdir(d)):
        p = os.path.join(d, c)
        if not os.path.isdir(p):
            continue
        argv = open(os.path.join(p, "argv")).read().split("\n")
        tag = " ".join(a for a in argv if os.sep not in a and not a.startswith("-") and not a.isdigit())[:40] \
            or os.path.basename(argv[0])
        r = rows.setdefault(tag, [0, 0, 0, 0, 0, 0, 0])
        tot["calls"] += 1
        r[0] += 1
        same = open(os.path.join(p, "rc")).read().split()[0] == open(os.path.join(p, "rc")).read().split()[1]
        files = sorted(f[4:] for f in os.listdir(p) if f.startswith("arm_"))
        for k in files:
            a = open(os.path.join(p, "arm_" + k), "rb").read()
            xp = os.path.join(p, "x86_" + k)
            x = open(xp, "rb").read() if os.path.exists(xp) else b""
            tot["bytes"] += len(a)
            if a == x:
                continue
            same = False
            if len(a) != len(x) or len(a) % 4:
                r[6] += 1
                continue
            av, xv = np.frombuffer(a, np.uint32), np.frombuffer(x, np.uint32)
            dm = av != xv
            fa, fx = av.view(np.float32), xv.view(np.float32)
            bn = dm & np.isnan(fa) & np.isnan(fx)
            on = dm & (np.isnan(fa) ^ np.isnan(fx))
            tot["words"] += av.size
            tot["diff"] += int(dm.sum())
            tot["both_nan"] += int(bn.sum())
            tot["one_nan"] += int(on.sum())
            tot["finite"] += int((dm & ~bn & ~on).sum())
            r[2] += int(dm.sum())
            r[3] += int(bn.sum())
            r[4] += int(on.sum())
            r[5] += int((dm & ~bn & ~on).sum())
        tot["same"] += same
        r[1] += same
    print("| kind | calls identical/total | words differ | both NaN (payload / sign) | NaN on one side | "
          "finite (float32 view) | size differs |")
    print("|---|---|---|---|---|---|---|")
    for k, r in sorted(rows.items()):
        print(f"| {k} | {r[1]}/{r[0]} | {r[2]} | {r[3]} | {r[4]} | {r[5]} | {r[6]} |")
    print(tot)


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--report":
        report(sys.argv[2])
    else:
        sys.exit(run())
