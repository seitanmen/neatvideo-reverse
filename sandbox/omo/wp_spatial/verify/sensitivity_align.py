"""verify_align_leaves.py の照合が空振りしないことの確認: 誤った式 (旧移植・ありがちな取り違え) が本家とずれる件数を数える。
本家の出力は tmp/align_cache/ のキャッシュを使う (verify_align_leaves.py を先に実行しておくこと)。"""
import os
import sys
import numpy as np
sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import verify_align_leaves as V  # noqa: E402
sp = V.sp
F = np.float32
MUT = {
    ("elem", "recip"): lambda c: (1.0 / (c[4][0].astype(F) + sp.EPS_WHITEN_F)).astype(F),
    ("elem", "acc"): lambda c: ((c[4][0] * c[4][1]) * F(c[3][0]) + c[4][2]).astype(F),
    ("elem", "blend"): lambda c: (c[4][0] * F(c[3][0]) + c[4][1] * F(c[3][1])).astype(F),
    ("elem", "affine"): lambda c: (c[4][0] * F(c[3][0]) + F(c[3][1])).astype(F),
    ("elem", "rinv"): lambda c: np.where(np.abs(c[4][1] / F(c[3][0])) > 1, c[4][0] / np.abs(c[4][1] / F(c[3][0])), c[4][0]).astype(F),
    ("strip3", "sq"): lambda c: sp.strip_rms3(c[4], 0, 1.0),      # 端を複製した (旧移植の) 局所 RMS
    ("strip3", "box"): lambda c: sp.strip_box3(c[4], 1 - c[3]),   # 端の扱いの取り違え
}
for (kind, nm), f in MUT.items():
    cs = [c for c in V.cases(False) if c[0] == kind and c[1] == nm]
    with np.errstate(all="ignore"):
        d = sum(0 if V.same(V.genuine(c), f(c)) else 1 for c in cs)
    print("%-14s %-6s mutant differs in %d / %d cases" % (kind, nm, d, len(cs)))
