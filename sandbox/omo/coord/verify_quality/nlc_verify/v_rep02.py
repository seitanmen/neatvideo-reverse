import os, sys; sys.path.insert(0, os.getcwd())
import numpy as np
from denoise_video_v5 import probe, read_frames
import nvfilter as nv
from nv_ui_qt import backend as be, state as st
v = "../test_v02.mp4"; info = probe(v)
fr = [tuple(np.array(p) for p in f) for _, f in read_frames(v, info["W"], info["H"], 400, 409, info["fps"])]
prof = be.build_profile(fr, 4, False, "bt709", "auto")
pl = tuple(p.astype(np.float32) for p in fr[4])
P = st.backend_params(st.default_params())
_, rv = be.temporal([pl] * 5, 2, prof, P)
m = be.temporal_mask(rv[0], 5, 1.0)
print("repeat test_v02 t_noise 100: mean", m.mean(), "frac<0.99", (m < 0.99).mean(), "min", m.min())
Y = pl[0]; vy, vx = nv.estimate_flow_v5(Y, Y, 16, 16)
print("flow |max|", np.abs(vy).max(), np.abs(vx).max(), "nonzero blocks", ((vy != 0) | (vx != 0)).mean())
