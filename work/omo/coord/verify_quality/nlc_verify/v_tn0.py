import os, sys; sys.path.insert(0, os.getcwd())
import numpy as np
from denoise_video_v5 import probe, read_frames
import nvfilter as nv
from nv_ui_qt import backend as be, state as st
v = "../test.mp4"; info = probe(v)
fr = [tuple(np.array(p) for p in f) for _, f in read_frames(v, info["W"], info["H"], 600, 609, info["fps"])]
prof = be.build_profile(fr, 4, False, "smpte170m", "auto")
pl = tuple(p.astype(np.float32) for p in fr[4])
for tn in (0.0, 1.0, 100.0):
    P = st.backend_params(dict(st.default_params(), t_noise=tn))
    _, rv = be.temporal([pl] * 5, 2, prof, P)
    m = be.temporal_mask(rv[0], 5, P["temporal"])
    print(tn, "rv min/max", rv[0].min(), rv[0].max(), "m min/mean/max", m.min(), m.mean(), m.max())
# weight internals at noise_level 0
Y = pl[0]
vy, vx = nv.estimate_flow_v5(Y, Y, 16, 16); FY, FX = nv.dense_flow(vy, vx, Y.shape, 16); wy = nv.warp_plane(Y, FY, FX)
print("flow max", np.abs(vy).max(), np.abs(vx).max(), "warp diff max", np.abs(wy - Y).max())
