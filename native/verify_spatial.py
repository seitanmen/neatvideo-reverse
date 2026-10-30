# Bit-exact verification of the native v5 spatial kernel (N1h) against reference/python/nvfilter.py.
#
#   make -C native -f Makefile.v5 && python3 native/verify_spatial.py [--quick] [--no-real] [--jobs N]
#
# Cases (every output word compared as uint32):
#   comp  : the scipy / numpy building blocks (nv_ndimage): gaussian_filter, uniform_filter (3/5/7/9,
#           reflect / nearest), median_filter, map_coordinates, atrous, _down2, _to_grid, curve_sigma,
#           np.exp, on random planes with NaN payloads / signalling NaN / +-inf / -0 / subnormals, and
#           constructed inputs for the double-precision details (adversarial_cases)
#   rand  : spatial_v5 (and spatial_v5 + Mix) on random planes, 4:2:0 / 4:4:4 / 4:2:2 chroma grids,
#           random profiles and parameters, special values in the planes and in rv
#   real  : test_v02 1080p frames n=100/312/576 (yuv420p and yuv444p), the frame's own v5 profile
#           (nvfilter.build_profile_v5, FindFlatArea + next frame) and its own residual variance
#           (nvfilter.temporal_v5 on frames n-2..n+2) x default and varied parameters (luma / chroma /
#           band_amounts / band_noise / noise_level / channel_noise / spatial / mix)
# Python runs in up to 3 processes (--jobs); frames, profiles and rv are cached in
# work/omo/wp_native_spatial/tmp/.
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
from v5_cases import F32, U32, fbits, nv, np  # noqa: E402
from scipy.ndimage import gaussian_filter, map_coordinates, median_filter, uniform_filter  # noqa: E402

TMP = os.path.join(vc.ROOT, "work", "omo", "wp_native_spatial", "tmp")
SPECIAL = np.concatenate([fbits(*vc.NANS), np.array([np.inf, -np.inf, 0.0, -0.0, 1e38, -1e38, 1e-40, -1e-42], F32)])


def make_plane(rng, H, W, style):
    a = (rng.standard_normal((H, W)) * rng.uniform(0.1, 40) + rng.uniform(0, 255)).astype(F32)
    if style == "int":
        a = rng.integers(0, 256, (H, W)).astype(F32)
    elif style == "const":
        a = np.full((H, W), F32(rng.uniform(0, 255)), F32)
    elif style == "special":
        k = max(1, H * W // 25)
        a.ravel()[rng.integers(0, H * W, k)] = SPECIAL[rng.integers(0, len(SPECIAL), k)]
    elif style == "dense":                       # many NaNs, both operands NaN often
        m = rng.random((H, W)) < 0.25
        a[m] = SPECIAL[rng.integers(0, len(SPECIAL), int(m.sum()))]
    elif style == "negzero":
        a[rng.random((H, W)) < 0.5] = F32(-0.0)
    elif style == "huge":
        a = (rng.standard_normal((H, W)) * 1e30).astype(F32)
    return np.ascontiguousarray(a)


# ------------------------------------------------------------------------- component cases
COMP_SHAPES = [(1, 1), (1, 6), (6, 1), (2, 2), (2, 3), (3, 2), (3, 7), (9, 4), (17, 23), (64, 48),
               (33, 65), (100, 3), (5, 40), (40, 5), (128, 96)]


def comp_cases(rng, quick):
    out = []
    shapes = COMP_SHAPES[::2] if quick else COMP_SHAPES
    for style in ("normal", "special", "dense", "negzero"):
        for (H, W) in shapes:
            a = make_plane(rng, H, W, style)
            lab = f"{style} {H}x{W}"
            out.append(dict(group="comp", label="gauss " + lab, kind="gauss", planes=[a]))
            for size in (3, 5, 7, 9):
                for mode in (0, 1):
                    out.append(dict(group="comp", label=f"uniform{size}{'rn'[mode]} " + lab, kind="uniform",
                                    ints=[size, mode], planes=[a]))
            out.append(dict(group="comp", label="median " + lab, kind="median", planes=[a]))
            for L in (1, 3, 5):
                out.append(dict(group="comp", label=f"atrous{L} " + lab, kind="atrous", ints=[L], planes=[a]))
            if H >= 2 and W >= 2:
                out.append(dict(group="comp", label="down2 " + lab, kind="down2", planes=[a]))
                for (h, w) in (((H + 1) // 2, (W + 1) // 2), (H, (W + 1) // 2), (H // 2 + 2, W // 2 + 1)):
                    out.append(dict(group="comp", label=f"to_grid{h}x{w} " + lab, kind="to_grid", ints=[h, w],
                                    planes=[a]))
            out.append(dict(group="comp", label="exp " + lab, kind="exp", planes=[a]))
            cy = rng.uniform(-3, H + 2, (H, W)).astype(F32)
            cx = rng.uniform(-3, W + 2, (H, W)).astype(F32)
            if H * W >= 4:
                cy.ravel()[:4] = [0, H - 1, -0.0, 0.5]
                cx.ravel()[:4] = [W - 1, 0, 0.25, -0.0]
            out.append(dict(group="comp", label="map_coordinates " + lab, kind="mapc", planes=[a, cy, cx]))
            prof = dict(vc.DUMMY_PROF)
            cv = list(rng.uniform(0.05, 6, 8))
            if style != "normal":
                cv[rng.integers(0, 8)] = [0.0, 1e30, float("inf")][rng.integers(0, 3)]
            prof["curve"] = {"Y": cv, "Cb": [1.0] * 8, "Cr": [1.0] * 8}
            out.append(dict(group="comp", label="curve_sigma " + lab, kind="curve", prof=prof, ints=[0], planes=[a]))
    return out


def adversarial_cases(rng):
    """Inputs that expose the double-precision details of the library code, which random inputs
    almost never reach (the float32 output differs only next to a float32 rounding boundary):
    gaussian_filter's fused multiply-add (windows built to cancel to ~0, so the product rounding
    shows), np.interp's fused multiply-add (tables with fp[j] = -slope * (x - xp[j])),
    map_coordinates' w1 = 1 - (1 - x) (tiny coordinates on huge values), uniform_filter's running
    sum order (sparse 1e20 spikes)."""
    import math
    out = []
    from scipy.ndimage._filters import _gaussian_kernel1d
    w = [float(v) for v in _gaussian_kernel1d(1.0, 0, 4)[::-1]]
    for t in range(3):
        rows = []
        for _ in range(40):
            x = [float(F32(v)) for v in rng.uniform(-200, 200, 9)]
            acc = x[4] * w[4]
            for k in (4, 3, 2):
                acc = math.fma(x[4 - k] + x[4 + k], w[4 - k], acc)
            target = -acc / w[3]
            x[3] = float(F32(target))
            x[5] = float(F32(target - x[3]))
            rows += x
        a = np.tile(np.array(rows, F32), (7, 1))
        out.append(dict(group="comp", label=f"gauss adversarial fma {t}", kind="gauss", planes=[a]))
        out.append(dict(group="comp", label=f"gauss adversarial fma {t} transposed", kind="gauss",
                        planes=[np.ascontiguousarray(a.T)]))
    xp = [16.0 + 32 * i for i in range(8)]
    for t in range(6):
        fp = [float(v) for v in rng.uniform(0.5, 6, 8)]
        xs = []
        for j in range(7):
            for _ in range(30):
                x = float(F32(xp[j] + rng.uniform(0.01, 31.9)))
                f = list(fp)
                for _ in range(30):
                    f[j] = -((f[j + 1] - f[j]) / (xp[j + 1] - xp[j]) * (x - xp[j]))
                slope = (f[j + 1] - f[j]) / (xp[j + 1] - xp[j])
                if F32(math.fma(slope, x - xp[j], f[j])) != F32(slope * (x - xp[j]) + f[j]):
                    fp = f
                    xs.append(x)
                    break
        xs += [float(v) for v in rng.uniform(0, 255, 30)]
        prof = dict(vc.DUMMY_PROF)
        prof["curve"] = {"Y": fp, "Cb": [1.0] * 8, "Cr": [1.0] * 8}
        out.append(dict(group="comp", label=f"curve_sigma adversarial fma {t}", kind="curve", prof=prof, ints=[0],
                        planes=[np.array(xs, F32).reshape(1, -1)]))
    img = rng.uniform(0, 255, (6, 6)).astype(F32)
    img[0, 1] = img[1, 0] = img[2, 3] = F32(1e30)
    img[3, 3] = F32(np.inf)
    cy = np.array([[0.0, 1e-30, 1e-38, 2.0, 2.0 + 2 ** -22, 3.0, 2 ** -60]], F32)
    cx = np.array([[1e-30, 0.0, 0.0, 3.0 - 2 ** -22, 2.0, 3.0, 0.0]], F32)
    out.append(dict(group="comp", label="map_coordinates adversarial tiny", kind="mapc", planes=[img, cy, cx]))
    for t in range(4):
        a = rng.uniform(0, 10, (24, 61)).astype(F32)
        m = rng.random(a.shape) < 0.03
        a[m] = F32(1e20) * rng.choice([-1, 1], int(m.sum())).astype(F32)
        for size in (3, 5, 7, 9):
            for mode in (0, 1):
                out.append(dict(group="comp", label=f"uniform{size}{'rn'[mode]} adversarial spikes {t}",
                                kind="uniform", ints=[size, mode], planes=[a]))
    return out


def comp_expected(c):
    k, pl = c["kind"], c["planes"]
    a = pl[0]
    if k == "gauss":
        return [gaussian_filter(a, 1.0)]
    if k == "uniform":
        return [uniform_filter(a, c["ints"][0], mode=("reflect", "nearest")[c["ints"][1]])]
    if k == "median":
        return [median_filter(a, 3, mode="nearest")]
    if k == "atrous":
        return nv.atrous(a, c["ints"][0])
    if k == "down2":
        return [nv._down2(a)]
    if k == "to_grid":
        return [nv._to_grid(a, tuple(c["ints"]))]
    if k == "exp":
        return [np.exp(a)]
    if k == "mapc":
        return [map_coordinates(a, [pl[1], pl[2]], order=1, mode="nearest")]
    if k == "curve":
        return [nv.curve_sigma(c["prof"], "Y", a)]
    raise ValueError(k)


# --------------------------------------------------------------------------- random spatial
def rand_profile(rng, Lc):
    prof = {"curve_centers": [256.0 / 8 * (i + 0.5) for i in range(8)], "levels": {"Y": int(rng.integers(1, 6)),
            "Cb": Lc, "Cr": Lc}, "curve": {}, "band_ratio": {}, "tot_ratio": {}}
    if rng.random() < 0.2:
        prof["levels"]["Y"] = 5
    for n in ("Y", "Cb", "Cr"):
        cv = [float(v) for v in rng.uniform(0.05, 8, 8)]
        if rng.random() < 0.15:
            cv[int(rng.integers(0, 8))] = 0.0
        prof["curve"][n] = cv
        r = [1.0]
        for _ in range(4):
            r.append(float(r[-1] * rng.uniform(0.3, 0.95)))
        prof["band_ratio"][n] = r[:prof["levels"][n]]
        prof["tot_ratio"][n] = float(rng.uniform(1.0, 2.5))
    return prof


def rand_params(rng, mix=False):
    kw = dict(luma=float(rng.uniform(-0.3, 1.4)), chroma=float(rng.uniform(-0.3, 1.4)),
              spatial=float(rng.uniform(0, 5)), noise_level=float(rng.uniform(0, 3)),
              channel_noise={n: float(rng.uniform(0, 3)) for n in ("Y", "Cb", "Cr")},
              band_amounts={b: float(rng.uniform(-0.2, 1.3)) for b in nv.V5_BANDS},
              band_noise={b: float(rng.uniform(0, 3)) for b in nv.V5_BANDS})
    if mix:
        kw["mix"] = float(rng.choice([0.0, rng.uniform(0, 1), 1.0, -0.2]))
    if rng.random() < 0.3:
        return nv.v5_params(**({"mix": kw["mix"]} if mix else {}))
    return nv.v5_params(**kw)


def rand_cases(rng, n):
    out = []
    shapes = [(2, 2), (3, 5), (8, 8), (16, 9), (33, 17), (48, 64), (65, 97), (96, 128), (120, 80), (130, 131)]
    styles = ["normal", "normal", "int", "const", "special", "dense", "negzero", "huge"]
    for i in range(n):
        H, W = shapes[i % len(shapes)]
        grid = ("420", "444", "422")[i % 3]
        style = styles[(i // 3) % len(styles)]
        ch = {"420": ((H + 1) // 2, (W + 1) // 2), "444": (H, W), "422": (H, (W + 1) // 2)}[grid]
        Lc = 4 if grid != "444" else 5
        if rng.random() < 0.3:
            Lc = int(rng.integers(1, 6))
        prof = rand_profile(rng, Lc)
        planes = [make_plane(rng, H, W, style)] + [make_plane(rng, *ch, style) for _ in range(2)]
        rv = []
        for p in planes:
            r = rng.uniform(0.05, 1.0, p.shape).astype(F32)
            if style in ("special", "dense") and rng.random() < 0.5:
                k = max(1, r.size // 30)
                r.ravel()[rng.integers(0, r.size, k)] = np.array([np.nan, np.inf, 0.0, -1.0, -0.0], F32)[
                    rng.integers(0, 5, k)]
            rv.append(r)
        mix = i % 2 == 1
        P = rand_params(rng, mix)
        lab = f"{style} {H}x{W} {grid} L{prof['levels']['Y']}/{Lc}"
        if mix:
            src = [make_plane(rng, *p.shape, "normal") for p in planes]
            out.append(dict(group="rand", label="mix " + lab, kind="spatial_mix", prof=prof, P=P,
                            planes=planes + rv + src))
        else:
            out.append(dict(group="rand", label=lab, kind="spatial", prof=prof, P=P, planes=planes + rv))
    return out


# ------------------------------------------------------------------------------- real frames
REAL_N = (100, 312, 576)


def real_params():
    d = nv.v5_params
    return [("default", d()),
            ("weak", d(luma=0.4, chroma=0.6, spatial=1.0, noise_level=1.3,
                       channel_noise={"Y": 0.8, "Cb": 1.5, "Cr": 1.2})),
            ("bands", d(spatial=3.5, band_amounts={"high": 0.3, "medium": 1.2, "low": 0.75, "very-low": 0.5,
                                                    "ultra-low": 0.5},
                        band_noise={"high": 2.0, "medium": 1.0, "low": 0.5, "very-low": 1.7, "ultra-low": 0.0})),
            ("mix", d(mix=0.3, luma=1.0, chroma=0.9, noise_level=0.8)),
            ("clamp", d(luma=-0.5, chroma=2.0, noise_level=0.0, channel_noise={"Y": 1.0, "Cb": 2.5, "Cr": 0.0},
                        spatial=0.0, mix=1.0))]


def real_inputs(n, fmt):
    """(planes, prof, rv) of test_v02 frame n: rv from nvfilter.temporal_v5 on n-2..n+2 (cached)."""
    planes = vc.frame_planes(TMP, "test_v02", n, fmt)
    prof = vc.profile_for(TMP, "test_v02", n, fmt)
    path = os.path.join(TMP, f"rv_test_v02_{fmt}_{n}.npz")
    if os.path.exists(path):
        z = np.load(path)
        tmp = [z[f"t{c}"] for c in range(3)]
        rv = [z[f"r{c}"] for c in range(3)]
    else:
        win = [vc.frame_planes(TMP, "test_v02", k, fmt) for k in range(n - 2, n + 3)]
        tmp, rv = nv.temporal_v5(win, 2, prof, nv.v5_params(), {})
        np.savez(path, **{f"t{c}": tmp[c] for c in range(3)}, **{f"r{c}": rv[c] for c in range(3)})
    return planes, tmp, prof, rv


def _prep_real(args):
    n, fmt = args
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        real_inputs(n, fmt)
    return args


def real_cases(quick, jobs):
    todo = [(n, fmt) for n in (REAL_N[:1] if quick else REAL_N) for fmt in ("yuv420p", "yuv444p")]
    with Pool(min(jobs, len(todo))) as pool:
        pool.map(_prep_real, todo)
    out = []
    for (n, fmt) in todo:
        planes, tmp, prof, rv = real_inputs(n, fmt)
        for name, P in (real_params()[:2] if quick else real_params()):
            lab = f"test_v02 n={n} {fmt} {name}"
            if P["mix"] > 0:
                # the temporal output and its rv go through spatial + Mix with the source frame
                out.append(dict(group="real", label=lab, kind="spatial_mix", prof=prof, P=P,
                                planes=list(tmp) + list(rv) + list(planes)))
            else:
                out.append(dict(group="real", label=lab, kind="spatial", prof=prof, P=P, planes=list(tmp) + list(rv)))
    return out


# ------------------------------------------------------------------------------------ driver
def expected(c):
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        if c["kind"] == "spatial":
            pl = c["planes"]
            return nv.spatial_v5(pl[:3], c["prof"], pl[3:6], c["P"])
        if c["kind"] == "spatial_mix":
            pl = c["planes"]
            out = nv.spatial_v5(pl[:3], c["prof"], pl[3:6], c["P"])
            m = float(c["P"]["mix"])          # process_frame_v5's Mix, verbatim
            if m > 0:
                out = [o + m * (np.asarray(s, np.float32) - o) for o, s in zip(out, pl[6:9])]
            return out
        return comp_expected(c)


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
    ap.add_argument("--seed", type=int, default=0x5A71)
    ap.add_argument("--nrand", type=int, default=60)
    ap.add_argument("--exe", default=vc.EXE)
    ap.add_argument("--keep", action="store_true")
    a = ap.parse_args()
    jobs = max(1, min(a.jobs, 3))
    os.makedirs(TMP, exist_ok=True)
    rng = np.random.default_rng(a.seed)
    cases = comp_cases(rng, a.quick) + adversarial_cases(rng) + rand_cases(rng, a.nrand // (3 if a.quick else 1))
    if not a.no_real:
        cases += real_cases(a.quick, jobs)
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
    inp, outp = os.path.join(TMP, "spatial_cases.bin"), os.path.join(TMP, "spatial_results.bin")
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
        st = stats.setdefault((c["group"], c["kind"] if c["group"] == "comp" else "spatial"), [0, 0, 0])
        st[0] += 1
        nd = sum(vc.diff_count(x, y) for x, y in zip(e, g)) + (0 if len(e) == len(g) else 1)
        st[2] += sum(np.asarray(x).size for x in e)
        if nd:
            st[1] += 1
            bad += 1
            if bad <= 20:
                print(f"  MISMATCH {c['group']} {c['label']}: {nd} words")
    for (g, k), (n, b, w) in sorted(stats.items()):
        print(f"  {g:5s} {k:14s}: {n - b}/{n} bit-exact ({w} words)")
    print(f"{len(cases) - bad}/{len(cases)} bit-exact ({nan_cases} cases with NaN in the expected output, "
          f"{nan_words} NaN words, payloads compared)")
    print(f"time: Python {t_py:.1f} s wall ({jobs} procs, {t_py_cpu:.1f} s CPU), C++ {t_cpp:.1f} s ({jobs} threads)")
    if not a.keep:
        for p in (inp, outp):
            os.remove(p)
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
