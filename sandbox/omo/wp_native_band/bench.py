# Timing of native nv_band_test vs repro/nv_band.py (single process each), per plane.
import os, struct, subprocess, sys, time
import numpy as np
ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "repro"))
import nv_band as nb
F32 = np.float32
EXE = os.path.join(ROOT, "native", "nv_band_test")
TMP = os.path.join(os.path.dirname(os.path.abspath(__file__)), "tmp")
rng = np.random.default_rng(7)
for (W, H) in ((128, 128), (320, 320), (512, 512), (1920, 1080)):
    p = rng.normal(100, 5, (H, W)).astype(F32)
    reps = max(3, int(2e6 // (W * H)))
    for kind, fn in ((0, nb.model_levels), (1, nb.band_levels)):
        fn(p)
        t = time.perf_counter()
        for _ in range(reps):
            fn(p)
        tpy = (time.perf_counter() - t) / reps
        inp, outp = os.path.join(TMP, "bench_in.bin"), os.path.join(TMP, "bench_out.bin")
        with open(inp, "wb") as f:
            f.write(struct.pack("<Q", reps))
            for _ in range(reps):
                f.write(struct.pack("<6I", kind, W, H, 5, 0, 0)); f.write(p.tobytes())
        r = subprocess.run([EXE, inp, outp], capture_output=True, text=True, check=True)
        s = float(r.stderr.split("model " if kind == 0 else "band ")[1].split(" s")[0]) / reps
        os.remove(inp); os.remove(outp)
        print(f"{['model_levels', 'band_levels'][kind]:12s} {W}x{H}: Python {tpy*1e3:7.2f} ms  C++ {s*1e3:7.2f} ms  ({tpy/s:.1f}x)")
