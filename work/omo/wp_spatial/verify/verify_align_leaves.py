"""空間フィルタの葉 (harness_request.md の elem / strip3 / sigfuse / ixform / sel2) を本家と移植 (src/nv_spatial.py) で比べる。

    python3 verify/verify_align_leaves.py [--quick] [--jobs 3]

最初に `nvh selftest` の終了コード 3 を確かめる。サブコマンドがまだなければ "harness pending" で終わる。
本家の出力は tmp/align_cache/ に置き、同じ入力では再利用する。比較はビット単位 (float32 の 32 ビット、u8 は値)。
"""
import argparse
import hashlib
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

import numpy as np

sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, ".."))
sys.path.insert(0, os.path.join(ROOT, "base"))
sys.path.insert(0, os.path.join(ROOT, "src"))
import nv_spatial as sp  # noqa: E402

NVH = os.path.abspath(os.path.join(ROOT, "..", "..", "..", "neatvideo_ofx_614", "analysis", "harness", "nvh"))
CACHE = os.path.join(ROOT, "tmp", "align_cache")
F = np.float32
U = np.uint32


def run(args):
    return subprocess.run([NVH] + [str(a) for a in args], capture_output=True, text=True)


def bits(x):
    return "%08x" % np.asarray(x, F).view(U).item()


def put(arr, name):
    p = os.path.join(CACHE, name)
    np.ascontiguousarray(arr).tofile(p)
    return p


def key(*parts):
    h = hashlib.sha1()
    for p in parts:
        h.update(np.ascontiguousarray(p).tobytes() if isinstance(p, np.ndarray) else str(p).encode())
    return h.hexdigest()[:16]


FULL_SEL2 = False
SPECIAL = np.array([0.0, -0.0, 1.0, -1.0, 0.5, 1e-45, -1e-45, 1.17549435e-38, 3.4028235e38, -3.4028235e38,
                    np.inf, -np.inf, 1e-4, -1e-4, 255.0, 255.5, 256.0, 254.5, -0.5, 0.4999999], F)
NANS = np.array([0x7FC00000, 0xFFC00001, 0x7F800001, 0x7FA00123], U).view(F)


def inputs(n, kind, rng):
    if kind == "g":
        return (rng.standard_normal(n) * 8).astype(F)
    if kind == "u":
        return (rng.random(n) * 260 - 2).astype(F)
    if kind == "pos":
        return (rng.random(n) * 30 + 1e-3).astype(F)
    if kind == "sp":
        v = rng.choice(np.concatenate([SPECIAL, NANS]), n)
        return v.astype(F)
    if kind == "bits":
        return rng.integers(0, 2 ** 32, n, dtype=np.uint64).astype(U).view(F)
    raise ValueError(kind)


# elem: 名前 → (スカラー数, 入力数, 移植の関数)
ELEM = {
    "recip": (1, 1, lambda s, a: sp.leaf_recip(s[0], a[0])),
    "scale": (1, 1, lambda s, a: sp.leaf_scale(s[0], a[0])),
    "affine": (2, 1, lambda s, a: sp.leaf_affine(s[0], s[1], a[0])),
    "fabs": (0, 1, lambda s, a: sp._fabs(a[0])),
    "sqr": (0, 1, lambda s, a: sp.leaf_sqr(a[0])),
    "sqrtabs": (0, 1, lambda s, a: sp.leaf_sqrtabs(a[0])),
    "sqrtk": (1, 1, lambda s, a: sp.leaf_sqrtk(s[0], a[0])),
    "u8": (0, 1, lambda s, a: sp.leaf_u8(a[0])),
    "mul": (0, 2, lambda s, a: sp.leaf_mul(a[0], a[1])),
    "add": (0, 2, lambda s, a: sp.leaf_add(a[0], a[1])),
    "sub": (0, 2, lambda s, a: sp.leaf_sub(a[0], a[1])),
    "mulk": (1, 2, lambda s, a: sp.leaf_mulk(s[0], a[0], a[1])),
    "blend": (2, 2, lambda s, a: sp.leaf_blend(s[0], s[1], a[0], a[1])),
    "rstep": (1, 2, lambda s, a: sp.leaf_rstep(s[0], a[0], a[1])),
    "rinv": (1, 2, lambda s, a: sp.leaf_rinv(s[0], a[0], a[1])),
    "clampmul": (4, 2, lambda s, a: sp.leaf_clampmul(s[0], s[1], s[2], s[3], a[0], a[1])),
    "acc": (1, 3, lambda s, a: sp.leaf_acc(s[0], a[0], a[1], a[2])),
}
SCAL = {"recip": [[sp.EPS_WHITEN_F]], "scale": [[-1.0], [-0.7], [F(np.nan)]], "affine": [[1.0, -3.5], [-0.25, 2.0], [0.37, 1.3]],
        "sqrtk": [[1.0], [0.37]], "mulk": [[sp.C_255INV], [3.0]], "blend": [[0.3, 0.7], [-1.0, 2.0]],
        "rstep": [[1.0], [0.75], [4.0]], "rinv": [[1.0], [0.5], [3.0]],
        "clampmul": [[0.4, -1.0, -1.0, -0.6], [2.0, -1.0, -1.0, -1.0], [0.0, 0.5, 0.0, 1.0]], "acc": [[1.0], [-0.5], [0.3]]}


def cases(quick):
    rng = np.random.default_rng(20261021)
    ns = [1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 33] + ([] if quick else [1023, 4099])
    kinds = ["g", "u", "pos", "sp"] + ([] if quick else ["bits"])
    out = []
    for nm, (ns_, ni, _) in ELEM.items():
        for sc in SCAL.get(nm, [[0.0]]):
            sc = (list(sc) + [0.0] * 4)[:4]
            for n in ns:
                for kd in kinds:
                    arrs = [inputs(n, kd, rng) for _ in range(ni)]
                    out.append(("elem", nm, n, [F(v) for v in sc], arrs))
    for (H, W) in [(2, 4), (2, 5), (3, 4), (5, 9), (8, 8), (17, 23), (64, 64)] + ([] if quick else [(130, 77), (512, 512)]):
        for mode in (0, 1):
            for kd in ("g", "u", "sp"):
                plane = inputs(H * W, kd, rng).reshape(H, W)
                out.append(("strip3", "box", (H, W), mode, plane))
                out.append(("strip3", "sq", (H, W), mode, plane))
    for n in [1, 3, 4, 5, 8, 13, 64] + ([] if quick else [1000, 4097]):
        for kd in ("u", "sp"):
            rgb = [inputs(n, kd, rng) for _ in range(3)]
            tab = (rng.random(3 * 257) * 4).astype(F)
            coef = (rng.random(9) * 0.6).astype(F)
            out.append(("sigfuse", n, rgb, tab, coef))
    for n in [4, 7, 64] + ([] if quick else [1001]):
        for kd in ("g", "sp"):
            out.append(("ixform", n, [inputs(n, kd, rng) for _ in range(3)]))
    for n in [4, 5, 7, 8, 16, 33] + ([] if quick else [257, 1000]):
        for flag in (0, 1):
            for kd in ("g", "sp"):
                stride = n + int(rng.integers(0, 3))
                a_, b_, be = (inputs(n, kd, rng) for _ in range(3))
                i3 = rng.integers(0, 256, n).astype(np.uint8)
                i4 = rng.integers(0, 256, n).astype(np.uint8)
                src = inputs(256 * stride, kd, rng)
                wt = (inputs(256 * stride, "pos" if kd == "g" else "sp", rng))
                out.append(("sel2", n, flag, stride, a_, b_, i3, i4, be, src, wt))
    if FULL_SEL2:                                          # FUN_0098eba8 そのもの (端数部 n & 3 を含む。nvh_patch2.diff)
        for n in [1, 2, 3, 5, 6, 7, 9, 13, 31] + ([] if quick else [255, 1001]):
            for flag in (0, 1):
                for kd in ("g", "sp"):
                    stride = n + int(rng.integers(0, 3))
                    a_, b_, be = (inputs(n, kd, rng) for _ in range(3))
                    i3 = rng.integers(0, 256, n).astype(np.uint8)
                    i4 = rng.integers(0, 256, n).astype(np.uint8)
                    src = inputs(256 * stride, kd, rng)
                    wt = inputs(256 * stride, "pos" if kd == "g" else "sp", rng)
                    out.append(("sel2full", n, flag, stride, a_, b_, i3, i4, be, src, wt))
    return out


def genuine(case):
    os.makedirs(CACHE, exist_ok=True)
    kind = case[0]
    if kind == "elem":
        _, nm, n, sc, arrs = case
        k = key("elem", nm, n, *[bits(v) for v in sc], *arrs)
        out = os.path.join(CACHE, k + ".out")
        if not os.path.exists(out):
            paths = [put(a, "%s.in%d" % (k, i)) for i, a in enumerate(arrs)]
            r = run(["elem", nm, n, out] + [bits(v) for v in sc] + paths)
            if r.returncode != 0:
                raise RuntimeError(r.stderr)
        dt = np.uint8 if nm == "u8" else F
        return np.fromfile(out, dt)
    if kind == "strip3":
        _, which, (H, W), mode, plane = case
        k = key("strip3", which, H, W, mode, plane)
        out = os.path.join(CACHE, k + ".out")
        if not os.path.exists(out):
            r = run(["strip3", W, H, put(plane, k + ".in"), which, mode, "3f800000", out])
            if r.returncode != 0:
                raise RuntimeError(r.stderr)
        return np.fromfile(out, F).reshape(H, W)
    if kind == "sigfuse":
        _, n, rgb, tab, coef = case
        k = key("sigfuse", n, *rgb, tab, coef)
        pre = os.path.join(CACHE, k + ".o")
        if not os.path.exists(pre + ".2.f32"):
            r = run(["sigfuse", n] + [put(a, "%s.c%d" % (k, i)) for i, a in enumerate(rgb)]
                    + [put(tab, k + ".tab"), put(coef, k + ".coef"), pre])
            if r.returncode != 0:
                raise RuntimeError(r.stderr)
        return [np.fromfile(pre + ".%d.f32" % c, F) for c in range(3)]
    if kind == "ixform":
        _, n, yab = case
        k = key("ixform", n, *yab)
        pre = os.path.join(CACHE, k + ".o")
        if not os.path.exists(pre + ".2.f32"):
            r = run(["ixform", n] + [put(a, "%s.c%d" % (k, i)) for i, a in enumerate(yab)] + [pre])
            if r.returncode != 0:
                raise RuntimeError(r.stderr)
        return [np.fromfile(pre + ".%d.f32" % c, F) for c in range(3)]
    if kind in ("sel2", "sel2full"):
        _, n, flag, stride, a_, b_, i3, i4, be, src, wt = case
        k = key(kind, n, flag, stride, a_, b_, i3, i4, be, src, wt)
        pre = os.path.join(CACHE, k + ".o")
        if not os.path.exists(pre + ".i4"):
            r = run(["sel2", n, flag, stride, put(a_, k + ".a"), put(b_, k + ".b"), put(i3, k + ".i3"), put(i4, k + ".i4in"),
                     put(be, k + ".best"), put(src, k + ".src"), put(wt, k + ".wt"), pre] + (["full"] if kind == "sel2full" else []))
            if r.returncode != 0:
                raise RuntimeError(r.stderr)
        return [np.fromfile(pre + ".out.f32", F), np.fromfile(pre + ".best.f32", F), np.fromfile(pre + ".b.f32", F),
                np.fromfile(pre + ".i4", np.uint8)]
    raise ValueError(kind)


def port(case):
    kind = case[0]
    if kind == "elem":
        _, nm, n, sc, arrs = case
        return ELEM[nm][2](sc, arrs)
    if kind == "strip3":
        _, which, (H, W), mode, plane = case
        return sp.strip_box3(plane, mode) if which == "box" else sp.strip_rms3(plane, mode, 1.0)
    if kind == "sigfuse":
        _, n, rgb, tab, coef = case
        t = tab.reshape(3, 257)
        return sp.sigfuse_port(rgb, (t[0], t[1], t[2]), coef.reshape(3, 3))
    if kind == "ixform":
        _, n, yab = case
        m = n & ~3
        return [o[:m] for o in sp.ixform_neon(*[a[:m] for a in yab])]
    if kind in ("sel2", "sel2full"):
        _, n, flag, stride, a_, b_, i3, i4, be, src, wt = case
        m = n if kind == "sel2full" else n & ~3
        o = sp.leaf_sel2(a_[:m], b_[:m], i3[:m], i4[:m], be[:m], src, wt, flag, stride)
        return [o[0], o[1], o[2], o[3]]
    raise ValueError(kind)


def same(a, b):
    if isinstance(a, list):
        return all(same(x, y) for x, y in zip(a, b))
    a, b = np.asarray(a), np.asarray(b)
    if a.dtype == np.uint8 or b.dtype == np.uint8:
        return a.shape == b.shape and np.array_equal(a, b)
    return a.shape == b.shape and np.array_equal(a.astype(F).view(U), b.astype(F).view(U))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--jobs", type=int, default=3)
    a = ap.parse_args()
    r = run(["selftest"])
    if r.returncode != 3:
        print("nvh selftest exit %d (expected 3): stop" % r.returncode)
        return 2
    print("selftest exit 3 OK")
    probe = run(["elem"])
    if "elem NAME" not in probe.stderr:
        print("harness pending (nvh has no `elem` subcommand yet)")
        return 0
    global FULL_SEL2
    FULL_SEL2 = "[full]" in probe.stderr
    if not FULL_SEL2:
        print("note: nvh sel2 has no [full] option (FUN_0098eba8 tail not checked)")
    cs = cases(a.quick)
    with ThreadPoolExecutor(max_workers=min(a.jobs, 3)) as ex:
        gs = list(ex.map(genuine, cs))
    stats = {}
    bad = []
    for c, g in zip(cs, gs):
        nm = c[0] if c[0] != "elem" else "elem " + c[1]
        if c[0] == "strip3":
            nm = "strip3 " + c[1]
        if c[0] in ("sel2", "sel2full"):
            nm = "%s flag%d" % (c[0], c[2])
        p = port(c)
        ok = same(g, p)
        s = stats.setdefault(nm, [0, 0])
        s[0] += 1
        s[1] += 0 if ok else 1
        if not ok and len(bad) < 20:
            bad.append((nm, c[2] if c[0] != "sigfuse" else c[1]))
    for nm, (n, m) in sorted(stats.items()):
        print("%-16s cases %5d mismatched %d" % (nm, n, m))
    for b in bad:
        print("  mismatch", b)
    tot = sum(m for n, m in stats.values())
    print("total cases %d mismatched %d" % (sum(n for n, m in stats.values()), tot))
    return 1 if tot else 0


if __name__ == "__main__":
    sys.exit(main())
