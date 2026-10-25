"""W < 4 か H < 2 の分離型の箱 (strip_box3 / strip_rms3 の代替経路) を本家の部品の組み合わせと比べる:
box = `nvh linefilt W H p 0 1 mode 1.5707964 1` (FUN_009944c0 の横 (角度 0) → 縦 (π/2)、半径 1。軸方向の核では FUN_0099ba54 /
FUN_0099bf40 に分岐する)、rms = elem sqr → linefilt → elem sqrtk (FUN_009950f4 の端数経路 0x9951c4〜0x995234 と同じ並び)。
linefilt の出力は %.9g の文字列なので有限の入力だけを使う。"""
import os, subprocess, sys
import numpy as np
sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
sys.path.insert(0, os.path.join(ROOT, "base")); sys.path.insert(0, os.path.join(ROOT, "src"))
import nv_spatial as sp  # noqa: E402
NVH = os.path.abspath(os.path.join(ROOT, "..", "..", "..", "neatvideo_ofx_614", "analysis", "harness", "nvh"))
TMP = os.path.join(ROOT, "tmp", "align_cache"); os.makedirs(TMP, exist_ok=True)
F = np.float32
assert subprocess.run([NVH, "selftest"], capture_output=True).returncode == 3
print("selftest exit 3 OK")


def lf(x, mode):
    H, W = x.shape
    p = os.path.join(TMP, "small_in.f32"); x.astype(F).tofile(p)
    o = subprocess.run([NVH, "linefilt", str(W), str(H), p, "0", "1", str(mode), "1.5707964", "1"], capture_output=True, text=True)
    if o.returncode:
        raise RuntimeError(o.stderr)
    v = np.array([float(l.split()[1]) for l in o.stdout.split("\n") if l.strip() and l[0].isdigit()], np.float64).astype(F)
    return v.reshape(H, W)


def elem(nm, s, x):
    p = os.path.join(TMP, "small_e.f32"); o = os.path.join(TMP, "small_e.out")
    x.astype(F).tofile(p)
    r = subprocess.run([NVH, "elem", nm, str(x.size), o, "%08x" % np.asarray(s, F).view(np.uint32), "0", "0", "0", p], capture_output=True)
    assert r.returncode == 0, r.stderr
    return np.fromfile(o, F)


rng = np.random.default_rng(7)
tot = bad = 0
for (H, W) in [(1, 1), (1, 2), (1, 3), (1, 4), (1, 9), (2, 1), (2, 2), (2, 3), (3, 3), (5, 2), (7, 3), (6, 1), (16, 3)]:
    for kind in ("g", "u"):
        x = ((rng.standard_normal((H, W)) * 8) if kind == "g" else (rng.random((H, W)) * 255)).astype(F)
        for mode in (0, 1):
            g = lf(x, mode); p = sp.strip_box3(x, mode)
            tot += 1; ok = np.array_equal(g.view(np.uint32), p.view(np.uint32)); bad += not ok
            if not ok: print("box mismatch", H, W, mode)
        for s in (1.0, 0.37):
            q = lf(elem("sqr", 0.0, x.reshape(-1)).reshape(H, W), 1)
            g = elem("sqrtk", s, q.reshape(-1)).reshape(H, W)
            p = sp.strip_rms3(x, 1, s)
            tot += 1; ok = np.array_equal(g.view(np.uint32), p.view(np.uint32)); bad += not ok
            if not ok: print("rms mismatch", H, W, s)
print("small planes cases %d mismatched %d" % (tot, bad))
