"""Progressive paths without AFT do not change: the libnvcore before this work (a copy of the build at the base commit,
argv[1]) and after (native/libnvcore.dylib) give the same build_profile_v5 dict and process_frame planes (bytes) on
frames of test.mp4 / test_v02.mp4 through the UI backend with the native core on.  Run once per library:
  NV_NATIVE_LIB=<lib> python3 no_aft_invariant.py OUT.npz
then compare the two .npz with --cmp A B."""
import hashlib
import json
import os
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
WT = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
sys.path.insert(0, os.path.join(WT, "reference", "python"))


def frames(path, n0, n, W, H):
    buf = subprocess.check_output(["ffmpeg", "-nostdin", "-v", "error", "-i", path, "-vf",
                                   "select=gte(n\\,%d)" % n0, "-frames:v", str(n), "-f", "rawvideo",
                                   "-pix_fmt", "yuv420p", "-"])
    fs, sz = [], W * H * 3 // 2
    for k in range(n):
        b = np.frombuffer(buf[k * sz:(k + 1) * sz], np.uint8)
        Y = b[:W * H].reshape(H, W)
        Cb = b[W * H:W * H + sz // 6].reshape(H // 2, W // 2)
        Cr = b[W * H + sz // 6:].reshape(H // 2, W // 2)
        fs.append((Y, Cb, Cr))
    return fs


def run(out):
    import nv_native as nn
    from nv_ui_qt import backend
    assert nn.AVAILABLE, nn.LIB_PATH
    res = {"lib": nn.LIB_PATH}
    for name, path, n0, W, H in (("t", os.path.join(WT, "test.mp4"), 300, 640, 360),
                                ("v02", os.path.join(WT, "test_v02.mp4"), 310, 1920, 1080)):
        fr = frames(path, n0, 5, W, H)
        for mode in ("auto", "spatial"):
            prof = backend.build_profile(fr, 1, False, "bt709", mode)
            P = dict(backend.nv.V5_DEFAULTS)
            win = [tuple(np.asarray(p, np.float32) for p in f) for f in fr]
            o = backend.process_frame(win, 2, prof, P)
            res["%s_%s_prof" % (name, mode)] = hashlib.sha256(json.dumps(prof, sort_keys=True, default=str).encode()).hexdigest()
            res["%s_%s_frame" % (name, mode)] = hashlib.sha256(b"".join(np.ascontiguousarray(p, np.float32).tobytes() for p in o)).hexdigest()
    print(json.dumps(res, indent=1))
    json.dump(res, open(out, "w"), indent=1)


if __name__ == "__main__":
    if sys.argv[1] == "--cmp":
        a, b = (json.load(open(p)) for p in sys.argv[2:4])
        ks = [k for k in a if k != "lib"]
        same = [k for k in ks if a[k] == b[k]]
        print("libs:", a["lib"], "|", b["lib"])
        print("identical %d/%d: %s" % (len(same), len(ks), " ".join(ks)))
        sys.exit(0 if len(same) == len(ks) else 1)
    run(sys.argv[1])
