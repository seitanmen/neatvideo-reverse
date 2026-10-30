# find planes on which the robust_tail mutant differs from the port (C++ vs C++, no Python)
import os, struct, subprocess, sys
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
F32 = np.float32
def run(exe, planes):
    inp = os.path.join(HERE, "tmp", "ft_in.bin"); outp = os.path.join(HERE, "tmp", "ft_out.bin")
    with open(inp, "wb") as f:
        f.write(struct.pack("<Q", len(planes)))
        for p in planes:
            f.write(struct.pack("<IIiI", p.shape[1], p.shape[0], -1, 5)); f.write(p.tobytes())
    subprocess.run([exe, inp, outp], check=True, capture_output=True)
    return np.fromfile(outp, np.uint32).reshape(len(planes), 4097)
rng = np.random.default_rng(3)
planes, desc = [], []
for (W, H) in [(45, 45), (97, 61), (89, 89), (57, 57)]:
    for dx in range(2, 16):
        for dy in range(0, 6):
            p = rng.normal(100, 5, (H, W)).astype(F32)
            p[H - 1 - dy, W - 1 - dx] = np.nan
            planes.append(p); desc.append((W, H, dx, dy))
a = run(os.path.join(ROOT, "native", "nv_model_test"), planes)
b = run(os.path.join(HERE, "tmp", "mut", "robust_tail", "nv_model_test"), planes)
hits = [d for d, x, y in zip(desc, a, b) if not np.array_equal(x, y)]
print(len(hits), "of", len(planes), hits[:20])
