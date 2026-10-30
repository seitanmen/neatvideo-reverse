# Timing of native/nv_profile_test (best of 3, -j 1 and -j 3) on the real cases of native/verify_profile.py,
# next to the Python build_profile times recorded in its cache (one run each, 3 processes in parallel).
#   python3 sandbox/omo/wp_native_profile/bench.py
import json
import os
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "native"))
import numpy as np  # noqa: E402
import verify_profile as vp  # noqa: E402

vp._SRC = vp.source_hash()
cases = [c for c in vp.real_cases(False) if c["area"][2] in (128, 192) and c["area"][3] in (128, 192)
         and c["mode"] == 1][:4]
inp = os.path.join(vp.TMP, "bench_cases.bin")
with open(inp, "wb") as f:
    f.write(struct.pack("<Q", len(cases)))
    for c in cases:
        H, W = c["rgb"].shape[:2]
        f.write(struct.pack("<8i", W, H, c["mode"], *c["area"], c["flags"]))
        f.write(np.ascontiguousarray(c["rgb"], np.float32).tobytes())
res = {}
for j in (1, 3):
    out = subprocess.run([vp.EXE, "-j", str(j), "--floor", vp.FLOOR, "time", inp, "3"], check=True,
                         capture_output=True, text=True).stdout.split("\n")
    res[j] = [float(l.split()[-1]) for l in out if l.strip()]
os.remove(inp)
for k, c in enumerate(cases):
    meta = vp.key(c)[:-4] + ".json"
    py = json.load(open(meta))["seconds"] if os.path.exists(meta) else float("nan")
    print(f"{c['label']}: Python {py:.1f} s, C++ -j1 {res[1][k]:.3f} s, -j3 {res[3][k]:.3f} s "
          f"(x{py / res[1][k]:.0f} / x{py / res[3][k]:.0f})")
print(subprocess.run(["uptime"], capture_output=True, text=True).stdout.strip())
