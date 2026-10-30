"""Compare shots/py_*.png with shots/cpp_*.png: pixel agreement, and side-by-side images shots/cmp_*.png."""
import os, sys
import numpy as np
from PIL import Image
S = os.path.join(os.path.dirname(os.path.abspath(__file__)), "shots")
for n in sorted(f[3:-4] for f in os.listdir(S) if f.startswith("py_")):
    a = np.asarray(Image.open(f"{S}/py_{n}.png").convert("RGB")).astype(int)
    b = np.asarray(Image.open(f"{S}/cpp_{n}.png").convert("RGB")).astype(int)
    if a.shape != b.shape:
        print(n, "shape", a.shape, b.shape); continue
    d = np.abs(a - b).max(2)
    print(f"{n:12s} {a.shape[1]}x{a.shape[0]}  identical {100*(d==0).mean():.2f}%  max {d.max()}  bbox of diffs",
          (np.argwhere(d).min(0).tolist(), np.argwhere(d).max(0).tolist()) if d.any() else "-")
    gap = np.full((a.shape[0], 8, 3), 255)
    diff = np.where(d[..., None] > 0, [255, 0, 255], a // 3)
    Image.fromarray(np.concatenate([a, gap, b, gap, diff], 1).astype(np.uint8)).save(f"{S}/cmp_{n}.png")
