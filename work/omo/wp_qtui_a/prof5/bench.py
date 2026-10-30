import os, sys, time
os.environ["NV_NATIVE"] = "1"
sys.path.insert(0, "native"); sys.path.insert(0, "repro")
import numpy as np
import verify_bridge as vb
vb.TMP = "native/tmp_bridge"
import verify_bridge_prof5 as vbp
import nvfilter as nv, nv_native
from nv_ui_qt import backend
backend.set_native(True)
frames, _ = vb.frame_list("v02")
for mode, area in (("auto", None), ("spatial", None), ("auto", (40, 60, 216, 512))):
    tp, tc1, tc4 = [], [], []
    for r in range(3):
        nv._FFA_CACHE.clear(); nv_native._FFA_CACHE.clear()
        t0 = time.perf_counter(); backend.build_profile(frames, 6, False, "bt709", mode, area); tp.append(time.perf_counter() - t0)
        import verify_prof5 as vp
        planes = tuple(np.asarray(p, np.float32) for p in frames[6]); more = [tuple(np.asarray(p, np.float32) for p in frames[i]) for i in (7, 8)]
        for th, acc in ((1, tc1), (4, tc4)):
            t0 = time.perf_counter(); vp.c_build(planes, False, area, more[0], mode, 5, more, "bt709", threads=th); acc.append(time.perf_counter() - t0)
    print(f"1080p {mode} {area}: backend(native on) {min(tp):.2f} s, C 1 thread {min(tc1):.2f} s, C 4 threads {min(tc4):.2f} s")
