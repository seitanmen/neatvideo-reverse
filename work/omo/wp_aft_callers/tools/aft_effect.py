"""How much the 16-bit step moves the AFT levels on the repo's own clips (UI backend, native core): run_aft with the
step and with it replaced by a copy (= the behaviour before this work).  python3 aft_effect.py"""
import ctypes
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
WT = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
sys.path.insert(0, os.path.join(WT, "reference", "python"))
sys.path.insert(0, HERE)
import nv_native as nn  # noqa: E402
from nv_ui_qt import backend  # noqa: E402
from no_aft_invariant import frames  # noqa: E402


def levels(p):
    return np.array(p["aft_face_curves"])


for name, path, n0, W, H in (("test.mp4 n=300", os.path.join(WT, "test.mp4"), 300, 640, 360),
                            ("test_v02 n=310", os.path.join(WT, "test_v02.mp4"), 310, 1920, 1080)):
    fr = frames(path, n0, 3, W, H)
    base = backend.build_profile(fr, 0, False, "bt709", "auto")
    a = backend.run_aft(base, fr[0], False, "bt709")
    o = nn._lib.nvc_aft_input16
    nn._lib.nvc_aft_input16 = lambda i, n, out: ctypes.memmove(out, i, 4 * n)
    try:
        b = backend.run_aft(base, fr[0], False, "bt709")
    finally:
        nn._lib.nvc_aft_input16 = o
    fa, fb = levels(a), levels(b)
    ma, mb = np.array(a["aft_bin_flags"]["marker"]), np.array(b["aft_bin_flags"]["marker"])
    print("%s area %s: AFT face curves max |rel| %.3e (median %.3e), marker bins differ %d/27, curve Y max |rel| %.3e" % (
        name, base["area"], np.max(np.abs(fa / fb - 1)), np.median(np.abs(fa / fb - 1)), int((ma != mb).sum()),
        np.max(np.abs(np.array(a["curve"]["Y"]) / np.array(b["curve"]["Y"]) - 1))))
