"""FORMATS texts of rows.py (Python) vs rows.cpp (wcheck fmt) on random and tie values."""
import os, subprocess, sys
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "..", "..", "repro"))
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
from nv_ui_qt.widgets.rows import FORMATS
rng = np.random.default_rng(3)
vals = list(rng.uniform(-300, 300, 3000)) + [k / 2 for k in range(-600, 601)] + [k / 200 + 1 for k in range(-400, 400)] + \
       [k * 0.005 for k in range(-400, 400)] + [0.0, -0.0, 1.0, 100.0, 99.5, 100.5, 0.4, -0.4]
inp = "\n".join(float(v).hex() for v in vals)
out = subprocess.run([os.path.join(HERE, "check", "build", "wcheck"), "x", "fmt"], input=inp, capture_output=True,
                     text=True).stdout.splitlines()
bad = 0
for v, line in zip(vals, out):
    ref = "|".join(f[2](f[0](float(v))) for f in (FORMATS[k] for k in ("amount", "rel", "inv", "tune", "frames", "float", "range"))) + "|"
    if ref != line:
        bad += 1
        if bad < 10: print(v, ref, line)
print(len(vals), "values, mismatches", bad, "lines", len(out))
