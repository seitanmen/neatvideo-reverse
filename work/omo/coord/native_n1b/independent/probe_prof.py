import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import *
import nvfilter as nv
p0 = yuv_frame("test.mp4", 600, "yuv420p"); p1 = yuv_frame("test.mp4", 601, "yuv420p")
prof = nv.build_profile_v5(p0, False, None, pair=p1, mode="auto", matrix="bt601")
def walk(k, v, d=0):
    if isinstance(v, dict):
        for kk, vv in v.items(): walk(f"{k}.{kk}", vv, d+1)
    elif isinstance(v, (list, tuple)):
        ts = {type(x).__name__ for x in v}
        print(k, type(v).__name__, len(v), ts)
    else:
        print(k, type(v).__name__, getattr(v, 'dtype', ''))
for k in ("curve_centers", "curve", "levels", "band_ratio", "tot_ratio"):
    walk(k, prof[k])
print(sorted(prof.keys()))
