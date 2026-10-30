#!/usr/bin/env python3
"""Independent replay of a captured native call (W2a verification).

    python3 replay_one.py <capture call dir> <alt> [FUNCS] [--keep OUTDIR]

Runs native/libm_alt/bin/<bin> with the captured argv (inputs replaced by in_<i>, outputs by a fresh
file), NV_LIBM_ALT=<alt> (and NV_LIBM_ALT_FUNCS=FUNCS if given), and counts the 4-byte words that differ
from the captured default output def_<i>.  Written by the verifier; does not import the worker's code.
"""
import json
import os
import subprocess
import sys
import tempfile

import numpy as np

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", ".."))
ALTBIN = os.path.join(ROOT, "native", "libm_alt", "bin")


def main():
    cd = os.path.abspath(sys.argv[1])
    alt = sys.argv[2]
    funcs = None
    keep = None
    rest = sys.argv[3:]
    if "--keep" in rest:
        k = rest.index("--keep")
        keep = rest[k + 1]
        rest = rest[:k] + rest[k + 2:]
    if rest:
        funcs = rest[0]
    meta = json.load(open(os.path.join(cd, "meta.json")))
    argv = list(meta["argv"])
    for i in meta["inputs"]:
        argv[i - 1] = os.path.join(cd, f"in_{i}")
    tmpd = keep or tempfile.mkdtemp(prefix="w2a_indep_")
    outs = {}
    for i in meta["outputs"]:
        argv[i - 1] = os.path.join(tmpd, f"alt{alt}_{os.path.basename(cd)}_{i}")
        outs[i] = argv[i - 1]
    env = {k: v for k, v in os.environ.items() if not k.startswith("NV_")}
    env.update(meta["env"])
    env["NV_LIBM_ALT"] = alt
    if funcs:
        env["NV_LIBM_ALT_FUNCS"] = funcs
    r = subprocess.run([os.path.join(ALTBIN, meta["bin"])] + argv, env=env, capture_output=True, text=True,
                       cwd=meta["cwd"])
    tag = f"{os.path.basename(cd)} {meta['bin']} {meta['argv'][0] if meta['argv'] else ''} alt{alt}" + \
          (f" funcs={funcs}" if funcs else "")
    if r.returncode != meta["rc"]:
        print(f"{tag}: RC MISMATCH def {meta['rc']} alt {r.returncode}: {r.stderr[-500:]}")
    for i, p in outs.items():
        a = np.fromfile(os.path.join(cd, f"def_{i}"), dtype=np.uint8)
        b = np.fromfile(p, dtype=np.uint8) if os.path.exists(p) else np.zeros(0, np.uint8)
        n = max(a.size, b.size)
        n += (-n) % 4
        A = np.zeros(n, np.uint8); A[:a.size] = a
        B = np.zeros(n, np.uint8); B[:b.size] = b
        A = A.view("<u4"); B = B.view("<u4")
        d = np.nonzero(A != B)[0]
        print(f"{tag}: out{i} bytes {a.size}/{b.size} words {A.size} DIFF {d.size}"
              + (f" first {d[:8].tolist()}" if d.size else ""))
        if not keep:
            os.remove(p)
    if not keep:
        os.rmdir(tmpd)


if __name__ == "__main__":
    main()
