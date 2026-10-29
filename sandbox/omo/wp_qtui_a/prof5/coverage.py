import sys, os, collections, warnings
sys.path.insert(0, "native"); sys.path.insert(0, "repro")
import numpy as np
import verify_prof5 as vp
warnings.simplefilter("ignore")
cnt = collections.Counter()
cases = vp.synth_cases(False) + vp.real_cases(False, "native/tmp_bridge")
for g, label, f0, more, fr, mat, mode, area in cases:
    planes = tuple(np.asarray(p, np.float32) for p in f0)
    mo = [tuple(np.asarray(p, np.float32) for p in m) for m in more]
    p = vp.c_build(planes, fr, area, mo[0] if mo else None, mode, 5, mo, mat)
    if isinstance(p, str):
        cnt[(g, "str", p)] += 1; continue
    cnt[(g, "status", p["flat_status"])] += 1
    cnt[(g, "mode", p["mode"])] += 1
    cnt[(g, "static", "static_fraction" in p)] += 1
    cnt[(g, "area_n", len(p["area"]))] += 1
    cnt[(g, "sub", p["sub"])] += 1
for k in sorted(cnt, key=str): print(k, cnt[k])
