"""Where did the genuine take each .dnp's noise sample (the BuildNoiseModel area)?

The 6 E9 progressive .dnp carry no sample, so the area is found here: FindFlatArea (the bit-exact port in
libnvcore, colour mode 1) on frame 5 of the clip, then BuildNoiseModel (libnvcore) on that area, and the 6 model
faces (6 x 2005 words, +0x4c ..) compared with the .dnp. Files with a sample (pa1, e9i) are checked the same way
against the position located by Phase C (locate_sample.py).  Input conversions tried: q16 (rint(x*16)/16) and d257.
Nothing here runs or loads the genuine binaries.

  python3 locate_area.py            writes ../out/areas.json
"""
import json
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
WT = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
sys.path.insert(0, os.path.join(WT, "reference", "python"))
PC = "/Users/seitan/Desktop/work/reverse/work/omo/wp_aft_runtime"
sys.path.insert(0, os.path.join(PC, "tools"))
sys.path.insert(0, os.path.join(PC, "corrected"))
import aftlib as A  # noqa: E402
import aft_input  # noqa: E402
import nv_native as nn  # noqa: E402
import phase_c as P  # noqa: E402

F32 = np.float32
MODEL0, MODELN = 0x4C, 6 * 0x1F54


def model_diff(raw, gen):
    a = np.frombuffer(raw[MODEL0:MODEL0 + MODELN], "<u4")
    b = np.frombuffer(gen[MODEL0:MODEL0 + MODELN], "<u4")
    return int((a != b).sum())


def flat(rgb):
    rgb = np.ascontiguousarray(rgb, F32)
    out = (np.ctypeslib.ctypes.c_int * 5)()
    nn._lib.nvc_flat_area(nn._ptr(rgb), rgb.shape[1], rgb.shape[0], 1, nn.THREADS, nn.CANCEL_FN(), out)
    return list(out)


def main():
    assert nn.AVAILABLE, "libnvcore not loaded"
    print("lib", nn.LIB_PATH)
    res = {}
    for name, (src, lay) in P.PROG.items():
        gen = A.image(P.dnp_path(name))
        codes = A.read_codes(src, lay, 5)
        d257 = A.to_rgb(codes, "d257")
        for conv, rgb in (("q16", aft_input.to_internal16(d257)), ("d257", d257)):
            x, y, w, h, st = flat(rgb)
            raw = nn.profile_blob(aft_input.to_internal16(d257), (x, y, w, h))
            nd = model_diff(raw, gen)
            print("%-28s flat on %-4s -> x %4d y %4d w %3d h %3d status %d   model words differing (q16 build) %d/12030"
                  % (name, conv, x, y, w, h, st, nd), flush=True)
            res.setdefault(name, []).append(dict(conv=conv, x=x, y=y, w=w, h=h, status=st, model_diff=nd))
    with open(os.path.join(HERE, "..", "out", "areas.json"), "w") as f:
        json.dump(res, f, indent=1)


if __name__ == "__main__":
    main()
