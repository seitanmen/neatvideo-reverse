# Timing of FindFlatArea (curve included): C++ nv_flat_test time (-j 1 / -j 3) vs the Python port
# (find_flat_area_genuine with nv_curve.genuine_noise_curve run where the genuine runs it).
#   python3 sandbox/omo/wp_native_flat/bench.py [REPS]
import os
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "repro"))
sys.path.insert(0, os.path.join(ROOT, "native"))
import numpy as np  # noqa: E402
import verify_flat as vf  # noqa: E402

REPS = int(sys.argv[1]) if len(sys.argv) > 1 else 3
EXE = os.path.join(ROOT, "native", "nv_flat_test")


def main():
    import nv_curve as ncv
    import nv_flat as nf
    vf.extract_frames()
    imgs = [("1080p test_v02 n=312", vf.make_image(("frame", "test_v02.mp4", 312))),
            ("4K 3840x2160 (n=312 x2)", vf.make_image(("upscale", "test_v02.mp4", 312, 2, 28)))]
    for name, img in imgs:
        H, W = img.shape[:2]
        path = os.path.join(vf.TMP, "bench.in")
        with open(path, "wb") as f:
            f.write(struct.pack("<Q", 1))
            vf.write_case(f, img, 1, None, False)
        for j in (1, 3):
            out = subprocess.run([EXE, "time", path, "-j", str(j), str(REPS)], check=True, capture_output=True,
                                 text=True).stdout.strip()
            print(f"{name}: C++ {out}", flush=True)
        os.remove(path)
        tc, tf = [], []
        for _ in range(REPS):
            t = {}

            def curve():
                t0 = time.perf_counter()
                c = ncv.noise_at(ncv.genuine_noise_curve(img))
                t["curve"] = time.perf_counter() - t0
                return c
            t0 = time.perf_counter()
            with np.errstate(all="ignore"):
                res = nf.find_flat_area_genuine(img, None, 1, curve=curve)
            tot = time.perf_counter() - t0
            tc.append(t["curve"])
            tf.append(tot)
        print(f"{name}: Python total best {min(tf):.3f} s mean {sum(tf) / REPS:.3f} s (curve best {min(tc):.3f} s, "
              f"flat part best {min(a - b for a, b in zip(tf, tc)):.3f} s) region {res}", flush=True)


if __name__ == "__main__":
    main()
