#!/usr/bin/env python3
"""Speed and memory of nv_model.model_full (fast path) vs the genuine `nvh model` on one plane of
128 x 128 / 320 x 320 / 512 x 512 (G of test_v02 n=312).  Each port timing runs in a fresh process
(--one SIZE MODE); tracemalloc runs separately from the timing.  usage: bench.py [--reps 3]"""
import argparse
import json
import os
import subprocess
import sys
import time

sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import numpy as np  # noqa: E402

import common  # noqa: E402

SIZES = (128, 320, 512)


def plane(s):
    fr = common.frame_cached("test_v02.mp4", 312).astype(np.float32)
    return np.ascontiguousarray(fr[100:100 + s, 300:300 + s, 1])


def one(s, mode):
    import warnings
    warnings.simplefilter("ignore")
    import nv_model as nm
    p = plane(s)
    if mode == "mem":
        import tracemalloc
        tracemalloc.start()
        nm.model_full(p)
        cur, peak = tracemalloc.get_traced_memory()
        return dict(peak_mib=peak / 2 ** 20)
    t = time.perf_counter()
    nm.model_full(p)
    return dict(seconds=time.perf_counter() - t)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--reps", type=int, default=3)
    ap.add_argument("--one", nargs=2)
    a = ap.parse_args()
    if a.one:
        print(json.dumps(one(int(a.one[0]), a.one[1])))
        return
    common.require_guard()
    res = {}
    for s in SIZES:
        p = plane(s)
        f = os.path.join(common.TMP, f"bench_{s}.f32")
        p.tofile(f)
        gen = []
        for _ in range(a.reps):
            t = time.perf_counter()
            r = subprocess.run([common.NVH, "model", str(s), str(s), f], capture_output=True)
            gen.append(time.perf_counter() - t)
            assert r.returncode == 0
        os.unlink(f)
        port = []
        for _ in range(a.reps):
            r = subprocess.run([sys.executable, __file__, "--one", str(s), "time"], capture_output=True, text=True,
                               env=dict(os.environ, PYTHONDONTWRITEBYTECODE="1"))
            port.append(json.loads(r.stdout.strip().splitlines()[-1])["seconds"])
        r = subprocess.run([sys.executable, __file__, "--one", str(s), "mem"], capture_output=True, text=True,
                           env=dict(os.environ, PYTHONDONTWRITEBYTECODE="1"))
        mem = json.loads(r.stdout.strip().splitlines()[-1])["peak_mib"]
        res[s] = dict(genuine_s=[round(x, 3) for x in gen], port_s=[round(x, 2) for x in port],
                      tracemalloc_peak_mib=round(mem, 1))
        print(s, res[s], flush=True)
    res["load"] = os.getloadavg()
    print(json.dumps(res))


if __name__ == "__main__":
    main()
