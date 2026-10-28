# Speed of nv_model.model_full (Python) vs native nv_model_test (C++), one process each, per plane.
#   python3 sandbox/omo/wp_native_model/bench.py
import os
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "repro"))
sys.path.insert(0, os.path.join(ROOT, "native"))
sys.dont_write_bytecode = True
import numpy as np  # noqa: E402

import verify_model as vm  # noqa: E402

F32 = np.float32


def main():
    rng = np.random.default_rng(7)
    rows = []
    planes = []
    for (W, H) in ((128, 128), (192, 192), (320, 320)):
        for st in ("normal", "periodic"):
            planes.append((W, H, st, vm.make_plane(rng, W, H, st)))
    rgb = vm.frame("test_v02.mp4", 312)
    import nv_flat as nf
    for s in (128, 192, 320):
        c = rgb[0:s, 832:832 + s]
        Y = nf.transform(*(np.ascontiguousarray(c[..., i], F32) for i in range(3)), 1)[0]
        planes.append((s, s, "test_v02 n=312 Y", np.ascontiguousarray(Y, F32)))
    inp = os.path.join(HERE, "tmp", "bench_in.bin")
    cases = [(W, H, s, p) for (W, H, _, p) in planes for s in (-1, 4)]
    with open(inp, "wb") as f:
        f.write(struct.pack("<Q", len(cases)))
        for W, H, s, p in cases:
            f.write(struct.pack("<IIiI", W, H, s, 5))
            f.write(p.tobytes())
    r = subprocess.run([vm.EXE, "time", inp, "3"], check=True, capture_output=True, text=True)
    ct = [float(l.split()[3]) for l in r.stdout.splitlines()]
    import nv_model as nm
    i = 0
    for (W, H, st, p) in planes:
        for s in (-1, 4):
            t0 = time.time()
            with np.errstate(all="ignore"):
                nm.model_full(p, s)
            tp = time.time() - t0
            rows.append((f"{W}x{H}", st, s, tp, ct[i]))
            i += 1
    print("plane      style               set   Python s   C++ s   ratio")
    for (sz, st, s, tp, tc) in rows:
        print(f"{sz:9s}  {st:18s}  {s:3d}  {tp:9.3f}  {tc:7.3f}  {tp / tc:6.1f}")
    os.remove(inp)


if __name__ == "__main__":
    main()
