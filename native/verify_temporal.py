# Bit-exact verification of the native v5 temporal kernel (N1g) against repro/nvfilter.py.
#
#   make -C native -f Makefile.v5 && python3 native/verify_temporal.py [--quick] [--no-real] [--jobs N]
#
# Cases (every output word compared as uint32):
#   comp  : estimate_flow_v5 (gaussian / _down2 / _search / _refine / parabola / median_filter),
#           dense_flow, warp_plane, pyramid_weight on random planes (bs / search varied), and the
#           numpy reductions of _search / _refine directly (block sums, cost cube, argmin; NaN payloads)
#   rand  : temporal_v5 and process_frame_v5 on random windows (moving textures + noise, 4:2:0 /
#           4:4:4 chroma, 2..7 frames), similarity "v5" / "pyramid", radius, temporal, threshold,
#           bs / search, pyramid_scale; some with a pre-filled flows cache; some with NaN / inf in Y or chroma
#   real  : test_v02 frames n=310..316 (1080p yuv420p) and test.mp4 n=298..304 (640x360, yuv420p and
#           yuv444p), the
#           centre frame's v5 profile, temporal_v5 / process_frame_v5 on 5-frame windows
# Compared: out (3 planes), rv (3 planes, temporal_v5) and the block vectors (vy, vx) of every
# neighbour as intermediate values (the flows cache after the call).
import argparse
import os
import sys
import time
import warnings
from multiprocessing import Pool

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.dont_write_bytecode = True
import v5_cases as vc  # noqa: E402
from v5_cases import F32, fbits, nv, np  # noqa: E402

TMP = os.path.join(vc.ROOT, "sandbox", "omo", "wp_native_temporal", "tmp")
SPECIAL = np.concatenate([fbits(*vc.NANS), np.array([np.inf, -np.inf, -0.0], F32)])


def texture(rng, H, W):
    """Smooth random texture with edges (values ~0..255), H+16 x W+16 so that it can be shifted."""
    h, w = H + 16, W + 16
    a = rng.standard_normal((h // 4 + 2, w // 4 + 2))
    a = np.kron(a, np.ones((4, 4)))[:h, :w]
    a = nv.gaussian_filter(a, 1.5) * 50 + 128
    yy, xx = np.mgrid[0:h, 0:w]
    a += 40 * ((xx // max(4, w // 5) + yy // max(4, h // 4)) % 2)
    return a


def shifted(base, H, W, dy, dx):
    """base sampled at (y + 8 + dy, x + 8 + dx) with bilinear interpolation (sub-pixel motion)."""
    y0, x0 = int(np.floor(dy)), int(np.floor(dx))
    fy, fx = dy - y0, dx - x0
    s = lambda oy, ox: base[8 + y0 + oy:8 + y0 + oy + H, 8 + x0 + ox:8 + x0 + ox + W]
    return (1 - fy) * ((1 - fx) * s(0, 0) + fx * s(0, 1)) + fy * ((1 - fx) * s(1, 0) + fx * s(1, 1))


def make_window(rng, H, W, nf, grid, style):
    base = [texture(rng, H, W) for _ in range(3)]
    v = rng.uniform(-3, 3, 2)
    frames = []
    for k in range(nf):
        dy, dx = v * (k - nf // 2) + rng.uniform(-0.5, 0.5, 2)
        dy, dx = float(np.clip(dy, -7, 7)), float(np.clip(dx, -7, 7))
        Y = shifted(base[0], H, W, dy, dx) + rng.normal(0, rng.uniform(0.5, 6), (H, W))
        if grid == "420":
            ch = ((H + 1) // 2, (W + 1) // 2)
            C = [shifted(b, H, W, dy, dx)[::2, ::2][:ch[0], :ch[1]] * 0.3 + 90 for b in base[1:]]
        else:
            C = [shifted(b, H, W, dy, dx) * 0.3 + 90 for b in base[1:]]
        pl = [np.ascontiguousarray(np.clip(p, 0, 255) if style != "huge" else p * 1e6, F32) for p in [Y] + C]
        if style == "int":
            pl = [np.round(p).astype(F32) for p in pl]
        elif style == "const" and k == 0:
            pl = [np.full(p.shape, F32(77.0), F32) for p in pl]
        elif style == "nanchroma":
            for p in pl[1:]:
                p.ravel()[rng.integers(0, p.size, max(1, p.size // 40))] = SPECIAL[rng.integers(0, len(SPECIAL))]
        elif style == "infY":
            p = pl[0]
            p.ravel()[rng.integers(0, p.size, max(1, p.size // 300))] = np.array([np.inf, -np.inf], F32)[rng.integers(0, 2)]
        elif style == "nanmixY":                 # different payloads meet in the block sums
            p = pl[0]
            k = max(2, p.size // 60)
            p.ravel()[rng.integers(0, p.size, k)] = fbits(*vc.NANS)[rng.integers(0, len(vc.NANS), k)]
        elif style == "nanY":
            p = pl[0]
            p.ravel()[rng.integers(0, p.size, max(1, p.size // 200))] = fbits(*vc.NANS[:3])[rng.integers(0, 3)]
        frames.append(pl)
    return frames


def rand_profile(rng, Lc):
    prof = {"curve_centers": [256.0 / 8 * (i + 0.5) for i in range(8)], "levels": {"Y": 5, "Cb": Lc, "Cr": Lc},
            "curve": {}, "band_ratio": {}, "tot_ratio": {}}
    for n in ("Y", "Cb", "Cr"):
        prof["curve"][n] = [float(v) for v in rng.uniform(0.3, 6, 8)]
        r = [1.0]
        for _ in range(4):
            r.append(float(r[-1] * rng.uniform(0.3, 0.95)))
        prof["band_ratio"][n] = r[:prof["levels"][n]]
        prof["tot_ratio"][n] = float(rng.uniform(1.0, 2.5))
    return prof


def rand_params(rng):
    kw = dict(similarity=str(rng.choice(["v5", "pyramid"])), radius=int(rng.integers(0, 4)),
              temporal=float(rng.choice([1.0, 1.0, 0.6, 1.3, 0.0, -0.2])),
              threshold=float(rng.choice([4.0, 2.0, 8.0, 0.5, 0.0])),
              pyramid_scale=float(rng.choice([1.5, 1.0, 2.5, 0.0])),
              noise_level=float(rng.uniform(0.3, 2.0)),
              channel_noise={"Y": float(rng.uniform(0.5, 1.5)), "Cb": 1.0, "Cr": 1.0},
              bs=int(rng.choice([16, 16, 8, 4, 12, 32])), search=int(rng.choice([16, 8, 4, 1, 3, 0])),
              luma=float(rng.uniform(0.3, 1.0)), chroma=float(rng.uniform(0.5, 1.0)),
              spatial=float(rng.uniform(1, 3)), mix=float(rng.choice([0.0, 0.0, 0.25])))
    if rng.random() < 0.25:
        return nv.v5_params(similarity=kw["similarity"])
    return nv.v5_params(**kw)


def window_case(group, label, frames, ci, prof, P, kind, flows=None):
    flows = flows or {}
    offs = sorted(flows)
    planes = [p for f in frames for p in f] + [p for o in offs for p in flows[o]]
    return dict(group=group, label=label, kind=kind, prof=prof, P=P, ints=[ci, len(frames), len(offs)] + offs,
                planes=planes, nframes=len(frames))


def rand_cases(rng, n):
    out = []
    shapes = [(32, 32), (48, 64), (64, 48), (72, 100), (96, 128), (100, 75), (128, 160), (45, 67), (40, 20),
              (20, 44), (16, 16), (50, 31)]
    styles = ["normal"] * 5 + ["int", "const", "huge", "nanchroma", "nanY", "infY", "nanmixY"]
    for i in range(n):
        H, W = shapes[i % len(shapes)]
        grid = "420" if i % 3 else "444"
        style = styles[i % len(styles)]
        P = rand_params(rng)
        if min(H, W) < P["bs"]:
            P["bs"] = 16 if min(H, W) >= 16 else 8
        nf = int(rng.integers(2, 8))
        ci = int(rng.integers(0, nf))
        frames = make_window(rng, H, W, nf, grid, style)
        prof = rand_profile(rng, 4 if grid == "420" else 5)
        kind = "frame" if i % 4 == 3 else "temporal"
        flows = None
        if i % 5 == 2 and nf > 1:            # pre-filled cache for one neighbour (arbitrary vectors)
            k = (ci + 1) % nf
            if k != ci:
                nby, nbx = H // P["bs"], W // P["bs"]
                flows = {k - ci: (rng.uniform(-4, 4, (nby, nbx)).astype(F32), rng.uniform(-4, 4, (nby, nbx)).astype(F32))}
        lab = (f"{style} {H}x{W} {grid} nf={nf} ci={ci} {P['similarity']} r={P['radius']} a={P['temporal']} "
               f"thr={P['threshold']} bs={P['bs']} s={P['search']}{' cache' if flows else ''}")
        out.append(window_case("rand", lab, frames, ci, prof, P, kind, flows))
    return out


def comp_cases(rng, quick):
    out = []
    shapes = [(16, 16), (17, 33), (40, 40), (64, 96), (99, 70), (130, 128), (60, 31), (31, 60)]
    for (H, W) in (shapes[::2] if quick else shapes):
        for (bs, search) in ((16, 16), (8, 8), (4, 2), (12, 5), (16, 0), (32, 16)):
            if min(H, W) < bs:
                continue
            f = make_window(rng, H, W, 2, "444", "normal")
            out.append(dict(group="comp", label=f"flow {H}x{W} bs={bs} s={search}", kind="flow",
                            ints=[bs, search], planes=[f[0][0], f[1][0]]))
            nby, nbx = H // bs, W // bs
            vy, vx = rng.uniform(-5, 5, (nby, nbx)).astype(F32), rng.uniform(-5, 5, (nby, nbx)).astype(F32)
            out.append(dict(group="comp", label=f"dense {H}x{W} bs={bs}", kind="dense", ints=[H, W, bs], planes=[vy, vx]))
        FY = rng.uniform(-6, 6, (H, W)).astype(F32)
        FX = rng.uniform(-6, 6, (H, W)).astype(F32)
        img = make_window(rng, H, W, 1, "444", "normal")[0][0]
        out.append(dict(group="comp", label=f"warp {H}x{W}", kind="warp", planes=[img, FY, FX]))
        img2 = img.copy()
        img2.ravel()[rng.integers(0, img.size, 20)] = SPECIAL[rng.integers(0, len(SPECIAL), 20)]
        out.append(dict(group="comp", label=f"warp special {H}x{W}", kind="warp", planes=[img2, FY, FX]))
        for scale in (1.5, 0.0, 3.0):
            d = rng.normal(0, 4, (H, W)).astype(F32)
            s1 = rng.uniform(0.5, 4, (H, W)).astype(F32)
            ratios = [1.0, float(rng.uniform(0.3, 0.9)), float(rng.uniform(0.1, 0.5))]
            out.append(dict(group="comp", label=f"pyramid {H}x{W} scale={scale}", kind="pyramid",
                            dbls=ratios + [scale], planes=[d, s1]))
    return out


def reduce_cases(rng, quick):
    """The numpy reductions of _search (d.reshape(nby, blk, nbx, blk).sum((1, 3))) and _refine
    ((d * d).sum((2, 3)) and argmin), compared directly: the motion search outputs only integer
    vectors, so their summation order is invisible downstream except at near-ties."""
    out = []
    grids = [(1, 1), (1, 3), (3, 1), (2, 2), (4, 5), (7, 3)]
    for blk in (1, 2, 3, 4, 6, 8, 12, 16, 17, 32):
        for (nby, nbx) in (grids[::2] if quick else grids):
            for style in ("normal", "nanmix"):
                d = np.abs(rng.normal(0, 30, (nby * blk, nbx * blk))).astype(F32)
                if style == "nanmix":
                    k = max(2, d.size // 6)
                    d.ravel()[rng.integers(0, d.size, k)] = np.abs(fbits(*vc.NANS))[rng.integers(0, len(vc.NANS), k)]
                out.append(dict(group="comp", label=f"blocksum {style} blk={blk} {nby}x{nbx}", kind="blocksum",
                                ints=[nby, nbx, blk], planes=[d]))
                H, W = nby * blk + int(rng.integers(0, 5)), nbx * blk + int(rng.integers(0, 5))
                cur = rng.normal(100, 30, (H, W)).astype(F32)
                ref = rng.normal(100, 30, (H, W)).astype(F32)
                if style == "nanmix":
                    for a in (cur, ref):
                        k = max(2, a.size // 10)
                        a.ravel()[rng.integers(0, a.size, k)] = fbits(*vc.NANS)[rng.integers(0, len(vc.NANS), k)]
                vy = rng.integers(-3, 4, (nby, nbx)).astype(F32)
                vx = rng.integers(-3, 4, (nby, nbx)).astype(F32)
                out.append(dict(group="comp", label=f"refine {style} blk={blk} {nby}x{nbx}", kind="refine",
                                ints=[blk], planes=[cur, ref, vy, vx]))
    return out


REAL = [("test_v02", 312, (310, 317), "yuv420p"), ("test", 301, (298, 305), "yuv420p"),
        ("test", 301, (298, 305), "yuv444p")]


def real_cases(quick):
    out = []
    d = nv.v5_params
    variants = [("default", d()), ("pyramid", d(similarity="pyramid")),
                ("thr2-bs8", d(threshold=2.0, bs=8, search=8, temporal=0.6)),
                ("pyr-s2.5", d(similarity="pyramid", pyramid_scale=2.5, noise_level=1.3, bs=32, search=12))]
    for video, pn, (lo, hi), fmt in REAL:
        frames = [list(vc.frame_planes(TMP, video, k, fmt)) for k in range(lo, hi)]
        prof = vc.profile_for(TMP, video, pn, fmt)
        centres = [2, 3, 4] if not quick else [2]
        for c in centres:
            win = frames[c - 2:c + 3]
            for name, P in (variants if not quick else variants[:2]):
                out.append(window_case("real", f"{video} {fmt} n={lo + c} {name}", win, 2, prof, P, "temporal"))
        # process_frame_v5 (temporal -> spatial -> mix) on the whole 7-frame list
        for name, P in ((("default", d()), ("pyr-mix", d(similarity="pyramid", mix=0.2, radius=3))) if not quick else
                        (("default", d()),)):
            out.append(window_case("real", f"{video} {fmt} frame n={lo + 3} {name}", frames, 3, prof, P, "frame"))
    return out


def expected(c):
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        k, pl = c["kind"], c["planes"]
        if k in ("temporal", "frame"):
            ci, nf, nfl = c["ints"][:3]
            offs = c["ints"][3:]
            win = [tuple(pl[3 * f:3 * f + 3]) for f in range(nf)]
            flows = {o: (pl[3 * nf + 2 * i], pl[3 * nf + 2 * i + 1]) for i, o in enumerate(offs)}
            if k == "temporal":
                o, rv = nv.temporal_v5(win, ci, c["prof"], c["P"], flows)
                res = list(o) + list(rv)
            else:
                res = list(nv.process_frame_v5(win, ci, c["prof"], c["P"], flows))
            for off in sorted(flows):
                res += list(flows[off])
            return res
        if k == "blocksum":
            nby, nbx, blk = c["ints"]
            return [pl[0].reshape(nby, blk, nbx, blk).sum((1, 3))]
        if k == "refine":
            vy, vx, cost = nv._refine(pl[0], pl[1], pl[2].astype(np.int32), pl[3].astype(np.int32), c["ints"][0])
            return [vy.astype(F32), vx.astype(F32)] + [cost[i, j] for i in range(3) for j in range(3)]
        if k == "flow":
            return list(nv.estimate_flow_v5(pl[0], pl[1], *c["ints"]))
        if k == "dense":
            H, W, bs = c["ints"]
            return list(nv.dense_flow(pl[0], pl[1], (H, W), bs))
        if k == "warp":
            return [nv.warp_plane(pl[0], pl[1], pl[2])]
        if k == "pyramid":
            return [nv.pyramid_weight(pl[0], pl[1], c["dbls"][:3], c["dbls"][3])]
        raise ValueError(k)


def _chunk(cs):
    t = time.time()
    return [expected(c) for c in cs], time.time() - t


def encode(c):
    return vc.encode_case(c["kind"], c.get("prof"), c.get("P"), c.get("ints", ()), c.get("dbls", ()), c["planes"])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--no-real", action="store_true")
    ap.add_argument("--jobs", type=int, default=3)
    ap.add_argument("--seed", type=int, default=0x7E3F)
    ap.add_argument("--nrand", type=int, default=64)
    ap.add_argument("--exe", default=vc.EXE)
    ap.add_argument("--keep", action="store_true")
    a = ap.parse_args()
    jobs = max(1, min(a.jobs, 3))
    os.makedirs(TMP, exist_ok=True)
    rng = np.random.default_rng(a.seed)
    cases = comp_cases(rng, a.quick) + reduce_cases(rng, a.quick) + rand_cases(rng, a.nrand // (4 if a.quick else 1))
    if not a.no_real:
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            for video, pn, _, fmt in REAL:
                vc.profile_for(TMP, video, pn, fmt)
        cases += real_cases(a.quick)
    t0 = time.time()
    chunks = [cases[i::jobs] for i in range(jobs)]
    with Pool(jobs) as pool:
        parts = pool.map(_chunk, chunks)
    exp = [None] * len(cases)
    t_py_cpu = 0.0
    for i, (part, dt) in enumerate(parts):
        t_py_cpu += dt
        for j, e in enumerate(part):
            exp[i + j * jobs] = e
    t_py = time.time() - t0
    inp, outp = os.path.join(TMP, "temporal_cases.bin"), os.path.join(TMP, "temporal_results.bin")
    vc.write_cases(inp, [encode(c) for c in cases])
    t1 = time.time()
    vc.run_exe(inp, outp, jobs, a.exe)
    t_cpp = time.time() - t1
    got = vc.read_results(outp, len(cases))
    stats, bad = {}, 0
    nan_cases = nan_words = 0
    for c, e, g in zip(cases, exp, got):
        nn = sum(int(np.isnan(np.asarray(x)).sum()) for x in e)
        nan_cases += nn > 0
        nan_words += nn
        st = stats.setdefault((c["group"], c["kind"]), [0, 0, 0, 0])
        st[0] += 1
        st[2] += sum(np.asarray(x).size for x in e)
        nd = sum(vc.diff_count(x, y) for x, y in zip(e, g)) + (0 if len(e) == len(g) else 1)
        if c["kind"] in ("temporal", "frame"):
            nimg = 6 if c["kind"] == "temporal" else 3
            st[3] += (len(e) - nimg) // 2
        if nd:
            st[1] += 1
            bad += 1
            if bad <= 20:
                parts_ = [vc.diff_count(x, y) for x, y in zip(e, g)]
                print(f"  MISMATCH {c['group']} {c['label']}: {nd} words {parts_} (lens {len(e)}/{len(g)})")
    for (g, k), (n, b, w, nfl) in sorted(stats.items()):
        extra = f", {nfl} flows compared" if nfl else ""
        print(f"  {g:5s} {k:9s}: {n - b}/{n} bit-exact ({w} words{extra})")
    print(f"{len(cases) - bad}/{len(cases)} bit-exact ({nan_cases} cases with NaN in the expected output, "
          f"{nan_words} NaN words, payloads compared)")
    print(f"time: Python {t_py:.1f} s wall ({jobs} procs, {t_py_cpu:.1f} s CPU), C++ {t_cpp:.1f} s ({jobs} threads)")
    if not a.keep:
        for p in (inp, outp):
            os.remove(p)
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
