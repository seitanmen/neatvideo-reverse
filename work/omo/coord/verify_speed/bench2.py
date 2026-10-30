import sys, time
sys.dont_write_bytecode = True
src, frp, which = sys.argv[1], sys.argv[2], sys.argv[3]
sys.path.insert(0, src)
import numpy as np
fr = np.load(frp)
t0 = time.perf_counter()
import nv_model as nm, nv_profile as npf
res = {}
if which == "floor":
    npf._FLOOR = None
    t = time.perf_counter(); f = npf.reference_floor(cache=None); res["floor_compute"] = time.perf_counter() - t
    np.save(sys.argv[4], f)
else:
    for s in (128, 320):
        p = np.ascontiguousarray(fr[100:100+s, 300:300+s, 1])
        ts = []
        for _ in range(2):
            t = time.perf_counter(); nm.model_full(p); ts.append(time.perf_counter() - t)
        res[f"model_full {s}"] = round(min(ts), 2)
    ts = []
    for _ in range(2):
        t = time.perf_counter(); npf.build_profile(fr, 1, (300, 100, 128, 128)); ts.append(time.perf_counter() - t)
    res["build_profile 128 (floor cached)"] = round(min(ts), 2)
print(src.split("/")[-2:], which, res, flush=True)
