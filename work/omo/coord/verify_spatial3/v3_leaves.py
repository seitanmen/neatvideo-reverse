"""独立再照合: src/nv_spatial.py (写し) の葉を本家 nvh と比べる。シード・入力・スカラーは実装者と別。
usage: python3 -I -B v3_leaves.py [--mutate NAME] [--seed N]"""
import argparse, hashlib, os, subprocess, sys
from concurrent.futures import ThreadPoolExecutor
import numpy as np
sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "src")); sys.path.insert(0, os.path.join(HERE, "base"))
import nv_spatial as sp
NVH = os.path.abspath(os.path.join(HERE, "..", "..", "..", "..", "neatvideo_ofx_614", "analysis", "harness", "nvh"))
CACHE = os.path.join(HERE, "cache"); F = np.float32; U = np.uint32
def run(a): return subprocess.run([NVH] + [str(x) for x in a], capture_output=True, text=True)
def bits(x): return "%08x" % np.asarray(x, F).view(U).item()
def fb(h): return np.array([int(h, 16)], U).view(F)[0]
def put(arr, name):
    p = os.path.join(CACHE, name); np.ascontiguousarray(arr).tofile(p); return p
def key(*parts):
    h = hashlib.sha1()
    for p in parts: h.update(np.ascontiguousarray(p).tobytes() if isinstance(p, np.ndarray) else str(p).encode())
    return h.hexdigest()[:16]
SPECIAL = np.array([0, -0.0, 1, -1, 2, 0.25, 1e-40, -1e-40, 1.1754944e-38, 3.0e38, -3.0e38, np.inf, -np.inf, 9.99999975e-05, 1/9., 127.5, 128.5, 255.49998, 255.50002, -0.50001, 0.5, 1.5, 2.5, 4294967296.0, 1e10, -1e10], F)
NANS = np.array([0x7FC00000, 0xFFC00000, 0x7F800001, 0xFFA00007, 0x7FC12345, 0xFF800001], U).view(F)
def inputs(n, kind, rng):
    if kind == "g": return (rng.standard_normal(n) * 40).astype(F)
    if kind == "u": return (rng.random(n) * 300 - 20).astype(F)
    if kind == "pos": return (rng.random(n) * 3 + 1e-5).astype(F)
    if kind == "sp": return rng.choice(np.concatenate([SPECIAL, NANS]), n).astype(F)
    if kind == "bits": return rng.integers(0, 2**32, n, dtype=np.uint64).astype(U).view(F)
    if kind == "sub": return (rng.integers(-2**23, 2**23, n)).astype(np.int32).view(F)  # subnormals
    if kind == "near": # 閾値付近: 整数 ±1ulp
        v = rng.integers(0, 260, n).astype(F); return (v.view(np.int32) + rng.integers(-2, 3, n)).astype(np.int32).view(F)
    raise ValueError(kind)
ELEM = {
 "recip": (1, 1, lambda s, a: sp.leaf_recip(s[0], a[0])), "scale": (1, 1, lambda s, a: sp.leaf_scale(s[0], a[0])),
 "affine": (2, 1, lambda s, a: sp.leaf_affine(s[0], s[1], a[0])), "fabs": (0, 1, lambda s, a: sp._fabs(a[0])),
 "sqr": (0, 1, lambda s, a: sp.leaf_sqr(a[0])), "sqrtabs": (0, 1, lambda s, a: sp.leaf_sqrtabs(a[0])),
 "sqrtk": (1, 1, lambda s, a: sp.leaf_sqrtk(s[0], a[0])), "u8": (0, 1, lambda s, a: sp.leaf_u8(a[0])),
 "mul": (0, 2, lambda s, a: sp.leaf_mul(a[0], a[1])), "add": (0, 2, lambda s, a: sp.leaf_add(a[0], a[1])),
 "sub": (0, 2, lambda s, a: sp.leaf_sub(a[0], a[1])), "mulk": (1, 2, lambda s, a: sp.leaf_mulk(s[0], a[0], a[1])),
 "blend": (2, 2, lambda s, a: sp.leaf_blend(s[0], s[1], a[0], a[1])), "rstep": (1, 2, lambda s, a: sp.leaf_rstep(s[0], a[0], a[1])),
 "rinv": (1, 2, lambda s, a: sp.leaf_rinv(s[0], a[0], a[1])),
 "clampmul": (4, 2, lambda s, a: sp.leaf_clampmul(s[0], s[1], s[2], s[3], a[0], a[1])),
 "acc": (1, 3, lambda s, a: sp.leaf_acc(s[0], a[0], a[1], a[2])),
}
SCAL = {"recip": [[9.99999975e-05], [0.0], [-0.25]], "scale": [[2.0], [0.1], [-3.3]], "affine": [[0.9, 7.0], [-1.7, 0.0], [1e-3, -255.0]],
 "sqrtk": [[2.0], [0.0039], [1.0]], "mulk": [[0.5], [-1.25], [1/255.]], "blend": [[0.6, 0.4], [1.0, 0.0], [-0.3, 1.3]],
 "rstep": [[2.0], [0.1], [-1.0]], "rinv": [[2.0], [0.1], [-1.0], [0.0]],
 "clampmul": [[1.0, -2.0, 0.2, 0.9], [0.3, 0.3, -1.0, 1.0], [-2.0, 3.0, 0.5, 0.5]], "acc": [[2.0], [0.1], [-1.0]]}
def cases(seed):
    rng = np.random.default_rng(seed); out = []
    ns = [1, 4, 6, 10, 11, 19, 32, 35, 100, 257, 1001]
    kinds = ["g", "u", "pos", "sp", "bits", "sub", "near"]
    for nm, (_, ni, _f) in ELEM.items():
        for sc in SCAL.get(nm, [[0.0]]):
            sc = (list(sc) + [0.0] * 4)[:4]
            for n in ns:
                for kd in kinds:
                    # 2 入力以上は種類を混ぜる
                    arrs = [inputs(n, kinds[(kinds.index(kd) + j * 3) % len(kinds)], rng) for j in range(ni)]
                    out.append(("elem", nm, n, [F(v) for v in sc], arrs))
    for (H, W) in [(2, 4), (3, 6), (4, 5), (6, 11), (9, 9), (31, 19), (50, 130), (257, 100), (400, 400)]:
        for mode in (0, 1):
            for kd in ("g", "u", "sp", "sub", "near", "pos"):
                plane = inputs(H * W, kd, rng).reshape(H, W)
                out.append(("strip3", "box", (H, W), mode, plane, F(1.0)))
                out.append(("strip3", "sq", (H, W), mode, plane, F(1.0)))
    for (H, W) in [(5, 7), (20, 20), (64, 33)]:
        for mode in (0, 1):
            for s in (0.5, 2.0, -1.5, 0.0):
                plane = inputs(H * W, "g", rng).reshape(H, W); out.append(("strip3", "sq", (H, W), mode, plane, F(s)))
    for n in [1, 2, 4, 5, 6, 9, 12, 17, 200, 3001]:
        for kd in ("u", "sp", "g", "bits", "near"):
            rgb = [inputs(n, kd, rng) for _ in range(3)]
            tab = (rng.random(3 * 257) * 9 - 1).astype(F)
            if kd == "sp": tab[::37] = rng.choice(NANS, len(tab[::37]))
            coef = (rng.random(9) * 1.3 - 0.2).astype(F)
            out.append(("sigfuse", n, rgb, tab, coef))
    for n in [4, 8, 9, 15, 100, 2049]:
        for kd in ("g", "sp", "u", "bits", "sub"):
            out.append(("ixform", n, [inputs(n, kd, rng) for _ in range(3)]))
    for n in [4, 5, 8, 11, 24, 99, 513]:
        for flag in (0, 1):
            for kd in ("g", "sp", "u", "near", "bits"):
                stride = n + int(rng.integers(0, 5))
                a_, b_, be = (inputs(n, kd, rng) for _ in range(3))
                i3 = rng.integers(0, 256, n).astype(np.uint8); i4 = rng.integers(0, 256, n).astype(np.uint8)
                if flag == 1 and n > 8: i3[::3] = i4[::3]
                src = inputs(256 * stride, kd, rng); wt = inputs(256 * stride, kd if kd in ("sp", "bits") else "pos", rng)
                out.append(("sel2", n, flag, stride, a_, b_, i3, i4, be, src, wt))
    return out
def genuine(case):
    kind = case[0]
    if kind == "elem":
        _, nm, n, sc, arrs = case
        k = key("elem", nm, n, *[bits(v) for v in sc], *arrs); out = os.path.join(CACHE, k + ".out")
        if not os.path.exists(out):
            paths = [put(a, "%s.in%d" % (k, i)) for i, a in enumerate(arrs)]
            r = run(["elem", nm, n, out] + [bits(v) for v in sc] + paths)
            if r.returncode != 0: raise RuntimeError(r.stderr)
        return np.fromfile(out, np.uint8 if nm == "u8" else F)
    if kind == "strip3":
        _, which, (H, W), mode, plane, s = case
        k = key("strip3", which, H, W, mode, plane, bits(s)); out = os.path.join(CACHE, k + ".out")
        if not os.path.exists(out):
            r = run(["strip3", W, H, put(plane, k + ".in"), which, mode, bits(s), out])
            if r.returncode != 0: raise RuntimeError(r.stderr)
        return np.fromfile(out, F).reshape(H, W)
    if kind == "sigfuse":
        _, n, rgb, tab, coef = case
        k = key("sigfuse", n, *rgb, tab, coef); pre = os.path.join(CACHE, k + ".o")
        if not os.path.exists(pre + ".2.f32"):
            r = run(["sigfuse", n] + [put(a, "%s.c%d" % (k, i)) for i, a in enumerate(rgb)] + [put(tab, k + ".tab"), put(coef, k + ".coef"), pre])
            if r.returncode != 0: raise RuntimeError(r.stderr)
        return [np.fromfile(pre + ".%d.f32" % c, F)[:n] for c in range(3)]
    if kind == "ixform":
        _, n, yab = case
        k = key("ixform", n, *yab); pre = os.path.join(CACHE, k + ".o")
        if not os.path.exists(pre + ".2.f32"):
            r = run(["ixform", n] + [put(a, "%s.c%d" % (k, i)) for i, a in enumerate(yab)] + [pre])
            if r.returncode != 0: raise RuntimeError(r.stderr)
        return [np.fromfile(pre + ".%d.f32" % c, F) for c in range(3)]
    if kind == "sel2":
        _, n, flag, stride, a_, b_, i3, i4, be, src, wt = case
        k = key("sel2", n, flag, stride, a_, b_, i3, i4, be, src, wt); pre = os.path.join(CACHE, k + ".o")
        if not os.path.exists(pre + ".i4"):
            r = run(["sel2", n, flag, stride, put(a_, k + ".a"), put(b_, k + ".b"), put(i3, k + ".i3"), put(i4, k + ".i4in"),
                     put(be, k + ".best"), put(src, k + ".src"), put(wt, k + ".wt"), pre])
            if r.returncode != 0: raise RuntimeError(r.stderr)
        return [np.fromfile(pre + ".out.f32", F), np.fromfile(pre + ".best.f32", F), np.fromfile(pre + ".b.f32", F), np.fromfile(pre + ".i4", np.uint8)]
def port(case):
    kind = case[0]
    if kind == "elem": return ELEM[case[1]][2](case[3], case[4])
    if kind == "strip3":
        _, which, _hw, mode, plane, s = case
        return sp.strip_box3(plane, mode) if which == "box" else sp.strip_rms3(plane, mode, s)
    if kind == "sigfuse":
        _, n, rgb, tab, coef = case; t = tab.reshape(3, 257)
        return [o[:n] for o in sp.sigfuse_port(rgb, (t[0], t[1], t[2]), coef.reshape(3, 3))]
    if kind == "ixform":
        _, n, yab = case; m = n & ~3
        return [o[:m] for o in sp.ixform_neon(*[a[:m] for a in yab])]
    if kind == "sel2":
        _, n, flag, stride, a_, b_, i3, i4, be, src, wt = case; m = n & ~3
        return list(sp.leaf_sel2(a_[:m], b_[:m], i3[:m], i4[:m], be[:m], src, wt, flag, stride))
def same(g, p, case=None):
    if isinstance(g, list):
        return all(same(x, y) for x, y in zip(g, p)) and len(g) == len(p)
    g, p = np.asarray(g), np.asarray(p)
    if g.dtype == np.uint8 or p.dtype == np.uint8: return g.shape == p.shape and np.array_equal(g, p)
    return g.shape == p.shape and np.array_equal(g.astype(F).view(U), p.astype(F).view(U))
def cmp_prefix(case, g, p):
    # ixform / sel2: 本家は n 全体 (端数も) を書くが、移植は n&~3 だけ。先頭 m 個を比べる
    if case[0] not in ("ixform", "sel2"): return same(g, p)
    m = case[1] & ~3
    if case[0] == "ixform": return same([x[:m] for x in g], p)
    if case[0] == "sel2": return same([x[:m] for x in g], p)
    return same(g, p)
def main():
    ap = argparse.ArgumentParser(); ap.add_argument("--seed", type=int, default=7770021); ap.add_argument("--jobs", type=int, default=3)
    ap.add_argument("--mutate"); a = ap.parse_args()
    r = run(["selftest"])
    if r.returncode != 3: print("selftest rc", r.returncode); return 2
    print("selftest exit 3 OK")
    cs = cases(a.seed)
    if a.mutate: cs = [c for c in cs if (c[0] == "elem" and c[1] == a.mutate) or (c[0] == a.mutate.split(":")[0] and c[0] != "elem" and (len(a.mutate.split(":")) < 2 or c[1] == a.mutate.split(":")[1]))]
    with ThreadPoolExecutor(max_workers=min(a.jobs, 3)) as ex: gs = list(ex.map(genuine, cs))
    stats = {}; bad = []
    for c, g in zip(cs, gs):
        nm = {"elem": "elem " + str(c[1]), "strip3": "strip3 " + str(c[1]), "sel2": "sel2 flag%s" % (c[2] if c[0] == "sel2" else "")}.get(c[0], c[0])
        ok = cmp_prefix(c, g, port(c))
        s = stats.setdefault(nm, [0, 0]); s[0] += 1; s[1] += 0 if ok else 1
        if not ok and len(bad) < 20: bad.append((nm, c[2] if c[0] in ("elem", "strip3") else c[1]))
    for nm, (n, m) in sorted(stats.items()): print("%-16s cases %5d mismatched %d" % (nm, n, m))
    for b in bad: print("  mismatch", b)
    print("total cases %d mismatched %d" % (sum(n for n, m in stats.values()), sum(m for n, m in stats.values())))
if __name__ == "__main__": sys.exit(main())
