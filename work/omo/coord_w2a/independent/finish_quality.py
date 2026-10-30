"""Completeness / Quality % of every blob in an nv_aft_test finish output, default vs alt (verifier's own parse)."""
import os, sys
import numpy as np
ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "repro"))
from nv_ui_qt import quality as Q
PROF = 0xc300
REC = PROF + 12
a = np.fromfile(sys.argv[1], np.uint8); b = np.fromfile(sys.argv[2], np.uint8)
assert a.size == b.size and a.size % REC == 0, (a.size, b.size)
n = a.size // REC
def pct(x): return int(np.floor(float(x) * 100.0 + 0.5))
blobs = qc = cc = qv = 0
ex = []
for i in range(n):
    da = bytes(a[i*REC:i*REC+PROF]); db = bytes(b[i*REC:i*REC+PROF])
    if da[0xbf34] == 0:
        continue
    blobs += 1
    qa, qb = Q.quality_from_blob(da), Q.quality_from_blob(db)
    ga, gb = Q.genuine_g(da), Q.genuine_g(db)
    pa = (None if qa is None else pct(qa), pct(ga)); pb = (None if qb is None else pct(qb), pct(gb))
    qc += pa[0] != pb[0]; cc += pa[1] != pb[1]
    qv += (qa != qb) or (ga != gb)
    if pa != pb: ex.append((i, pa, pb, float(ga), float(gb)))
print(f"cases {n}, blobs with 0xbf34!=0: {blobs}, Quality% changed {qc}, Completeness% changed {cc}, q/g value changed {qv}")
for e in ex: print("  changed:", e)
