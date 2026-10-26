# Bit-exact verification of the native EstimateNoiseLevelCurve port (native/nv_curve.{h,cpp})
# against the Python port (repro/nv_curve.py + nv_pixnoise.py + nv_lines.py).
#
#   make -C native -f Makefile.curve && python3 native/verify_curve.py [--fuzz N] [--clips N] [--jobs J]
#
# Cases (all compared BIT FOR BIT, no tolerance):
#   curve   genuine_noise_curve (default batch path) -> xs, ys (8 doubles each; the pass criterion) and the
#           per-bin sums / counts (diagnostic, also required to match): real frames (test_v02 n=0/100/312/576,
#           test.mp4 n=0/300/450/750, full frame), synthetic frames and N seeded random images (48..260 px,
#           NaN / +-inf / huge 1e13..1e38 / negative / tiny / quantised / clipped values).
#           The C++ side runs with -j 1 and with -j J (tile-parallel; must give the same bits).
#   clip    is_clip_genuine on random planes (16x16 blocks and odd sizes 0..700, specials).
#   interp  noise_at / CurveInterp on the curves above and on random tables (n = 2..8), queries at the
#           nodes, between, outside, +-inf and NaN.
#   tile    nv_pixnoise.pixnoise (reference) on tiles: noise, weight, bright planes (diagnostic).
import argparse
import os
import struct
import subprocess
import sys
import time
from multiprocessing import Pool

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, os.path.join(ROOT, "repro"))
sys.dont_write_bytecode = True

import numpy as np  # noqa: E402

F32 = np.float32
F64 = np.float64
TMP = os.path.join(ROOT, "sandbox", "omo", "wp_native_curve", "tmp")
EXE = os.environ.get("NV_CURVE_EXE", os.path.join(HERE, "nv_curve_test"))   # override: mutation runs

FRAMES = [("test_v02.mp4", 0), ("test_v02.mp4", 100), ("test_v02.mp4", 312), ("test_v02.mp4", 576),
          ("test.mp4", 0), ("test.mp4", 300), ("test.mp4", 450), ("test.mp4", 750)]


# ----------------------------------------------------------------------------- inputs
def frame(path, n):
    """RGB float32 0..255 of frame n (ffmpeg rgb48le / 257, as quality_draft._frame), cached in TMP."""
    os.makedirs(TMP, exist_ok=True)
    cache = os.path.join(TMP, f"{os.path.splitext(path)[0]}_n{n}.npy")
    if os.path.exists(cache):
        return np.load(cache)
    src = os.path.join(ROOT, path)
    wh = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries", "stream=width,height",
                         "-of", "csv=p=0", src], check=True, capture_output=True, text=True).stdout.strip().split(",")
    W, H = int(wh[0]), int(wh[1])
    raw = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-i", src, "-vf", f"select=eq(n\\,{n})",
                          "-frames:v", "1", "-f", "rawvideo", "-pix_fmt", "rgb48le", "-"],
                         check=True, capture_output=True).stdout
    rgb = (np.frombuffer(raw, np.uint16).reshape(H, W, 3).astype(F32) / F32(257.0)).astype(F32)
    np.save(cache, rgb)
    return rgb


def synthetic_cases():
    rng = np.random.default_rng(20261026)
    out = []
    for W, H in ((1920, 1080), (640, 360), (97, 61), (48, 48), (101, 67), (333, 211), (49, 50)):
        yy, xx = np.mgrid[0:H, 0:W].astype(F64)
        g = 20 + 215 * xx / max(W - 1, 1)
        img = np.stack([g, g * 0.9, g * 1.05], -1) + rng.normal(0, 3, (H, W, 3))
        out.append((f"grad+gauss {W}x{H}", np.clip(img, 0, 255).astype(F32)))
    W, H = 640, 360
    img = rng.normal(128, 4, (H, W, 3))
    img[:60] = 0.0
    img[-60:] = 0.0
    out.append(("black bars 640x360", np.clip(img, 0, 255).astype(F32)))
    img = rng.normal(200, 30, (H, W, 3))
    out.append(("white clipping 640x360", np.clip(img, 0, 255).astype(F32)))
    out.append(("flat 128 640x360", np.full((H, W, 3), 128.0, F32)))
    img = (np.clip(rng.normal(128, 5, (H, W, 3)), 0, 255) * 1e18).astype(F32)
    out.append(("x1e18 640x360", img))
    img = np.clip(rng.normal(100, 5, (H, W, 3)), 0, 255).astype(F32)
    img[100:140, 200:260] = F32(1e20)
    out.append(("1e20 block 640x360", img))
    img = np.clip(rng.normal(100, 5, (H, W, 3)), 0, 255).astype(F32)
    img[50:80, 50:90, 1] = np.nan
    img[200:230, 300:350, 0] = np.inf
    img[300, 10:40, 2] = -np.inf
    out.append(("NaN/inf blocks 640x360", img))
    img = np.clip(rng.normal(100, 5, (H, W, 3)), 0, 255).astype(F32)
    u = img.view(np.uint32)
    for k in range(40):                                 # NaN payloads of both signs, quiet and signalling
        y, x, c = int(rng.integers(0, H)), int(rng.integers(0, W)), k % 3
        u[y, x, c] = (0x7f800000 | int(rng.integers(1, 1 << 22)) | (0x400000 if k % 4 else 0)
                      | (0x80000000 if k % 3 == 1 else 0))
    out.append(("NaN payloads 640x360", img))
    for W2, H2, frac in ((200, 160, 1.0), (200, 160, 0.5), (130, 97, 0.25)):   # dense NaN payload regions
        img = np.clip(rng.normal(100, 5, (H2, W2, 3)), 0, 255).astype(F32)
        u = img.view(np.uint32)
        reg = (slice(20, 90), slice(30, 120))
        m = rng.random(u[reg].shape) < frac
        pay = (0x7f800000 | rng.integers(1, 1 << 22, u[reg].shape) | np.where(rng.random(u[reg].shape) < 0.8, 0x400000, 0)
               | np.where(rng.random(u[reg].shape) < 0.5, 0x80000000, 0)).astype(np.uint32)
        u[reg] = np.where(m, pay, u[reg])
        out.append((f"dense NaN payloads {frac:g} {W2}x{H2}", img))
    img = np.clip(rng.normal(100, 5, (H, W, 3)), 0, 255).astype(F32)
    for k, v in enumerate((1e13, 1e15, 1e17, 1e19, 1e20)):
        img[20 + 60 * k, 30 + 100 * k, k % 3] = F32(v)
    out.append(("isolated 1e13..1e20 pixels 640x360", img))
    W, H = 4096, 2160
    yy, xx = np.mgrid[0:H, 0:W].astype(F32)
    img = np.stack([16 + 0.05 * xx, 30 + 0.08 * yy, 200 - 0.04 * xx], -1) + rng.normal(0, 2.5, (H, W, 3)).astype(F32)
    out.append(("4096x2160 gradient", np.clip(img, 0, 255).astype(F32)))
    return out


def fuzz_image(rng, k):
    """Random 48..260 px image: the generator of analysis/noise/verify_curve.py --fuzz (odd k made finite)
    plus huge-value blocks (1e13..1e20), NaN blocks and subnormal / negative-zero planes."""
    W, H = int(rng.integers(48, 261)), int(rng.integers(48, 261))
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float64)
    kind = rng.integers(0, 5)
    tags = []
    if kind == 0:
        sig = float(rng.choice([0.05, 0.1, 0.2, 1.0, 4.0, 20.0]))
        lo, hi = sorted(rng.uniform(0, 255, 2))
        b = lo + (hi - lo) * xx / (W - 1)
        img = np.stack([b, b * rng.uniform(0.8, 1.1), b * rng.uniform(0.7, 1.2)], -1) + rng.normal(0, sig, (H, W, 3))
        tags.append(f"grad sig{sig}")
    elif kind == 1:
        n = int(rng.integers(2, 7))
        lev = rng.uniform(0, 255, n)
        img = np.stack([lev[np.minimum((xx * n // W).astype(int), n - 1)]] * 3, -1) + \
            rng.normal(0, rng.uniform(0.1, 6), (H, W, 3))
        tags.append(f"stripes{n}")
    elif kind == 2:
        img = np.full((H, W, 3), rng.uniform(0, 255))
        for _ in range(int(rng.integers(1, 4))):
            ph, pw = int(rng.integers(8, max(9, H // 2))), int(rng.integers(8, max(9, W // 2)))
            y0, x0 = int(rng.integers(0, H - ph + 1)), int(rng.integers(0, W - pw + 1))
            img[y0:y0 + ph, x0:x0 + pw] = rng.uniform(0, 255) + rng.normal(0, rng.uniform(0.5, 8), (ph, pw, 3))
        tags.append("flat+patches")
    elif kind == 3:
        img = rng.uniform(0, 255, (H, W, 3))
        tags.append("uniform")
    else:
        img = rng.normal(rng.uniform(20, 230), rng.uniform(0.5, 10), (H, W, 3))
        tags.append("gauss")
    if rng.random() < 0.5:
        img = np.clip(img, 0, 255)
        tags.append("clip")
    if rng.random() < 0.2:
        img = np.round(img)
        tags.append("int")
    r = rng.random()
    if r < 0.1:
        img = img - rng.uniform(100, 400)
        tags.append("neg")
    elif r < 0.2:
        s = float(10.0 ** rng.choice([-40, -20, -5, 5, 18, 30]))
        img = img * s
        tags.append(f"x{s:g}")
    img = img.astype(F32)
    for what, p in (("nan", 0.25), ("inf", 0.15), ("-inf", 0.1), ("huge", 0.25), ("hugeneg", 0.1),
                    ("big13_20", 0.2)):
        if rng.random() < p:
            n = int(rng.integers(1, 4))
            for _ in range(n):
                y, x, c = int(rng.integers(0, H)), int(rng.integers(0, W)), int(rng.integers(0, 3))
                v = {"nan": np.nan, "inf": np.inf, "-inf": -np.inf,
                     "huge": float(10.0 ** rng.uniform(18, 38.5)), "hugeneg": -float(10.0 ** rng.uniform(18, 38.5)),
                     "big13_20": float(10.0 ** rng.uniform(13, 20))}[what]
                img[y, x, c] = np.float32(min(v, 3.4e38)) if np.isfinite(v) else v
            tags.append(f"{what}{n}")
    if rng.random() < 0.1:                             # NaNs with payloads: +-, quiet and signalling
        n = int(rng.integers(1, 6))
        for _ in range(n):
            y, x, c = int(rng.integers(0, H)), int(rng.integers(0, W)), int(rng.integers(0, 3))
            pay = int(rng.integers(1, 1 << 22))
            u = (0x7f800000 | pay | (0x400000 if rng.random() < 0.7 else 0)) | (0x80000000 if rng.random() < 0.5 else 0)
            img[y, x, c] = np.array([u], np.uint32).view(F32)[0]
        tags.append(f"nanpay{n}")
    if rng.random() < 0.08:                            # a block of NaN / huge values
        bh, bw = int(rng.integers(4, 40)), int(rng.integers(4, 40))
        y0, x0 = int(rng.integers(0, H - min(bh, H) + 1)), int(rng.integers(0, W - min(bw, W) + 1))
        img[y0:y0 + bh, x0:x0 + bw, int(rng.integers(0, 3))] = rng.choice([np.nan, 1e16, 1e20, -1e19])
        tags.append("block")
    if rng.random() < 0.03:
        img = (img * F32(1e-41)).astype(F32)           # subnormals
        tags.append("subnormal")
    if k % 2:
        img = np.where(np.isfinite(img), img, F32(128.0)).astype(F32)
        big = np.abs(img) > 1e15
        img[big] = (np.sign(img[big]) * rng.uniform(1e9, 1e15, int(big.sum()))).astype(F32)
        tags.append("finite")
    return f"fuzz {k:03d} {W}x{H} " + ",".join(tags), np.ascontiguousarray(img)


def fuzz_cases(n, seed=20261026):
    rng = np.random.default_rng(seed)
    return [fuzz_image(rng, k) for k in range(n)]


# ----------------------------------------------------------------------------- Python side
def py_curve(img):
    import nv_curve as ncv
    with np.errstate(all="ignore"):
        xs, ys, sums, counts = ncv.genuine_noise_curve(img, return_hist=True)
    return (np.asarray(xs, F64), np.asarray(ys, F64), np.asarray(sums, F64), np.asarray(counts, np.int64))


def run_exe(mode, payload, extra=()):
    inp = os.path.join(TMP, f"tmp_curve_{mode}_{os.getpid()}.in")
    outp = os.path.join(TMP, f"tmp_curve_{mode}_{os.getpid()}.out")
    with open(inp, "wb") as f:
        f.write(payload)
    t0 = time.perf_counter()
    subprocess.run([EXE, mode, inp, outp, *extra], check=True)
    dt = time.perf_counter() - t0
    res = open(outp, "rb").read()
    os.remove(inp)
    os.remove(outp)
    return res, dt


def u64(a):
    return np.asarray(a, F64).view(np.uint64)


# ----------------------------------------------------------------------------- checks
def check_curves(cases, jobs, pool):
    payload = [struct.pack("<Q", len(cases))]
    for _, img in cases:
        H, W = img.shape[:2]
        payload.append(struct.pack("<ii", W, H))
        payload.append(np.ascontiguousarray(img[..., :3], F32).tobytes())
    payload = b"".join(payload)
    t0 = time.perf_counter()
    expected = pool.map(py_curve, [img for _, img in cases], chunksize=1)
    t_py = time.perf_counter() - t0
    rec = 1 + 8 * 8 * 4
    outs = {}
    for j in sorted({1, jobs}):
        res, dt = run_exe("curve", payload, ("-j", str(j)))
        outs[j] = res
        print(f"  C++ -j {j}: {dt:.1f} s for {len(cases)} images (Python, {pool._processes} procs: {t_py:.1f} s)")
    bad_curve = bad_hist = 0
    nan_cases = 0
    for i, ((name, _), (xs, ys, sums, counts)) in enumerate(zip(cases, expected)):
        if np.isnan(ys).any():
            nan_cases += 1
        for j, res in outs.items():
            o = rec * i
            ok = res[o]
            c_xs = np.frombuffer(res, F64, 8, o + 1)
            c_ys = np.frombuffer(res, F64, 8, o + 65)
            c_sums = np.frombuffer(res, F64, 8, o + 129)
            c_counts = np.frombuffer(res, np.int64, 8, o + 193)
            curve_ok = ok == 1 and np.array_equal(u64(c_xs), u64(xs)) and np.array_equal(u64(c_ys), u64(ys))
            hist_ok = np.array_equal(u64(c_sums), u64(sums)) and np.array_equal(c_counts, counts)
            if not curve_ok:
                bad_curve += 1
                if bad_curve <= 8:
                    print(f"  MISMATCH curve [{name}] -j {j}: ys C++ {[float(v) for v in c_ys]}\n"
                          f"                                py    {[float(v) for v in ys]}")
            if not hist_ok:
                bad_hist += 1
                if bad_hist <= 8:
                    print(f"  MISMATCH hist [{name}] -j {j}: counts {c_counts.tolist()} vs {counts.tolist()}, "
                          f"sums equal {np.array_equal(u64(c_sums), u64(sums))}")
    n = len(cases) * len(outs)
    return n, bad_curve, bad_hist, nan_cases


def clip_planes(n, seed=7):
    rng = np.random.default_rng(seed)
    out = []
    specials = np.array([np.nan, np.inf, -np.inf, 0.0, -0.0, 1e-45, 3.4e38, -3.4e38, 255.0, 1e20], F32)
    for k in range(n):
        kind = k % 10
        if kind < 5:
            size = 256
        elif kind < 8:
            size = int(rng.integers(0, 700))
        else:
            size = int(rng.choice([0, 1, 2, 7, 8, 9, 15, 16, 17, 255, 257]))
        style = rng.integers(0, 9)
        if style == 0:                                  # clipped at 255 / 0 with noise
            v = np.clip(rng.normal(rng.uniform(200, 300), rng.uniform(1, 40), size), 0, 255)
        elif style == 1:                                # narrow range (frintn matters)
            v = rng.uniform(100, 100 + rng.uniform(0.05, 0.6), size)
        elif style == 2:                                # quantised
            v = np.round(rng.normal(128, rng.uniform(0.2, 3), size))
        elif style == 3:                                # two-level
            v = np.where(rng.random(size) < rng.uniform(0, 1), 255.0, rng.uniform(0, 255, size))
        elif style == 4:
            v = rng.uniform(0, 255, size)
        elif style == 5:                                # huge range
            v = rng.normal(0, 10.0 ** rng.uniform(5, 38), size)
        elif style == 6:                                # constant
            v = np.full(size, rng.uniform(-10, 300))
        elif style == 7:                                # mostly one value + tail
            v = np.full(size, 255.0)
            m = rng.random(size) < rng.uniform(0, 0.8)
            v[m] = rng.uniform(0, 255, int(m.sum()))
        else:
            v = rng.normal(128, 2, size)
        v = np.asarray(v, F32)
        if size and rng.random() < 0.25:                # specials sprinkled (also in the scalar tail)
            m = int(rng.integers(1, 4))
            idx = rng.integers(0, size, m)
            v[idx] = rng.choice(specials, m)
        out.append(np.ascontiguousarray(v, F32))
    return out


def py_clip(v):
    import nv_curve as ncv
    with np.errstate(all="ignore"):
        return F32(ncv.is_clip_genuine(v))


def check_clip(planes, pool):
    payload = [struct.pack("<Q", len(planes))]
    for v in planes:
        payload.append(struct.pack("<Q", v.size))
        payload.append(v.tobytes())
    res, dt = run_exe("clip", b"".join(payload))
    got = np.frombuffer(res, F32)
    exp = np.array(pool.map(py_clip, planes, chunksize=64), F32)
    bad = np.nonzero(got.view(np.uint32) != exp.view(np.uint32))[0]
    for i in bad[:8]:
        print(f"  MISMATCH clip plane {i} (n={planes[i].size}): C++ {got[i]!r} py {exp[i]!r}")
    return len(planes), int(bad.size)


def interp_cases(curves, n, seed=11):
    rng = np.random.default_rng(seed)
    out = []
    for xs, ys in curves:
        q = np.concatenate([xs, (xs[:-1] + xs[1:]) / 2, [-50.0, -1e-300, 0.0, 300.0, 1e308, np.inf, -np.inf, np.nan],
                            rng.uniform(-40, 300, 24)])
        out.append((xs, ys, q))
    for k in range(n):
        m = int(rng.integers(2, 9))
        xs = np.sort(rng.uniform(-100, 400, m))
        if k % 7 == 3:
            xs = np.array([16.0 + 32 * i for i in range(m)])
        ys = rng.uniform(-5, 10, m) * (10.0 ** rng.integers(-3, 4))
        if k % 11 == 5:
            ys[rng.integers(0, m)] = np.nan
        if k % 13 == 6:
            xs[rng.integers(0, m)] = np.nan
        if k % 5 == 2:                                  # NaNs with distinct payloads / signs in ys and q
            for _ in range(int(rng.integers(1, 3))):
                u = 0x7ff8000000000000 | int(rng.integers(1, 1 << 50)) | (int(rng.integers(0, 2)) << 63)
                ys[rng.integers(0, m)] = np.array([u], np.uint64).view(F64)[0]
        if k % 17 == 8 and m > 2:
            xs[1] = xs[0]                               # duplicate node
        qn = np.array([0x7ff8000000000000 | int(rng.integers(1, 1 << 50)) | (int(rng.integers(0, 2)) << 63)],
                      np.uint64).view(F64)
        q = np.concatenate([xs, rng.uniform(-200, 500, 20), [np.nan, np.inf, -np.inf, 1e300, -1e300], qn])
        out.append((xs, ys, q))
    return out


def py_interp(case):
    import nv_curve as ncv
    xs, ys, q = case
    with np.errstate(all="ignore"):
        f = ncv.noise_at((xs, ys))
        return np.array([f(v) for v in q], F64)


def check_interp(cases, pool):
    payload = [struct.pack("<Q", len(cases))]
    for xs, ys, q in cases:
        payload.append(struct.pack("<i", len(xs)) + np.asarray(xs, F64).tobytes() + np.asarray(ys, F64).tobytes()
                       + struct.pack("<i", len(q)) + np.asarray(q, F64).tobytes())
    res, _ = run_exe("interp", b"".join(payload))
    got = np.frombuffer(res, F64)
    exp = pool.map(py_interp, cases, chunksize=32)
    off = 0
    bad = nq = 0
    for i, e in enumerate(exp):
        g = got[off:off + e.size]
        off += e.size
        nq += e.size
        d = np.nonzero(u64(g) != u64(e))[0]
        if d.size:
            bad += int(d.size)
            if bad <= 8:
                print(f"  MISMATCH interp case {i} queries {d.tolist()[:4]}: C++ {g[d[:2]]} py {e[d[:2]]}")
    return len(cases), nq, bad


def py_tile(args):
    import nv_pixnoise as npx
    luma, weight = args
    with np.errstate(all="ignore"):
        return npx.pixnoise(luma, weight)


def check_tiles(tiles, pool):
    payload = [struct.pack("<Q", len(tiles))]
    for luma, weight in tiles:
        H, W = luma.shape
        payload.append(struct.pack("<ii", W, H) + luma.tobytes() + weight.tobytes())
    res, _ = run_exe("tile", b"".join(payload))
    exp = pool.map(py_tile, tiles, chunksize=1)
    off = 0
    bad = 0
    for i, ((luma, _), e) in enumerate(zip(tiles, exp)):
        n = luma.size
        planes = [np.frombuffer(res, F32, n, off + 4 * n * k) for k in range(3)]
        off += 12 * n
        for name, g, p in zip(("noise", "weight", "bright"), planes, e):
            d = int((g.view(np.uint32) != np.asarray(p, F32).ravel().view(np.uint32)).sum())
            if d:
                bad += 1
                if bad <= 8:
                    print(f"  MISMATCH tile {i} {luma.shape} {name}: {d} pixels differ")
    return len(tiles), bad


def tile_inputs(frames, rng):
    import nv_curve as ncv
    out = []
    for img in frames[:3]:
        H, W = img.shape[:2]
        R, G, B = (np.ascontiguousarray(img[..., c], F32) for c in range(3))
        for d in ncv.tile_layout(W, H)[::max(1, len(ncv.tile_layout(W, H)) // 6)][:6]:
            x, y, w, h = d[:4]
            tR, tG, tB = R[y:y + h, x:x + w], G[y:y + h, x:x + w], B[y:y + h, x:x + w]
            out.append((np.ascontiguousarray(ncv.luma_bt601(tR, tG, tB), F32),
                        np.ascontiguousarray(ncv.clip_weight(tR, tG, tB), F32)))
    for W, H in ((120, 108), (97, 61), (50, 46), (48, 120), (64, 33), (10, 9), (8, 8)):
        luma = rng.normal(128, 5, (H, W)).astype(F32)
        if W == 97:
            luma[5, 5] = np.nan
            luma[20, 30] = 1e20
        out.append((luma, np.ones((H, W), F32)))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--fuzz", type=int, default=520, help="random images (default 520)")
    ap.add_argument("--clips", type=int, default=4000, help="random IsClip planes (default 4000)")
    ap.add_argument("--interp", type=int, default=600, help="random interpolator tables (default 600)")
    ap.add_argument("--jobs", type=int, default=4, help="processes / C++ threads (max 4)")
    ap.add_argument("--no-frames", action="store_true", help="skip the real frames")
    A = ap.parse_args()
    jobs = max(1, min(A.jobs, 4))
    if not os.path.exists(EXE):
        print(f"{EXE} not found: make -C native -f Makefile.curve")
        return 2
    os.makedirs(TMP, exist_ok=True)
    rng = np.random.default_rng(5)
    t_all = time.perf_counter()
    fails = 0
    with Pool(jobs) as pool:
        frames = [] if A.no_frames else [(f"{p} n={n}", frame(p, n)) for p, n in FRAMES]
        cases = frames + synthetic_cases() + fuzz_cases(A.fuzz)
        print(f"curve: {len(frames)} real frames + {len(cases) - len(frames) - A.fuzz} synthetic + {A.fuzz} fuzz images")
        n, bc, bh, nanc = check_curves(cases, jobs, pool)
        print(f"curve: {n - bc}/{n} curves bit-exact (xs + ys), histograms {n - bh}/{n} "
              f"({nanc} cases with NaN in ys)")
        fails += bc + bh
        import nv_curve as ncv
        curves = []
        for _, img in cases[:40]:
            with np.errstate(all="ignore"):
                xs, ys = ncv.genuine_noise_curve(img)
            curves.append((np.asarray(xs, F64), np.asarray(ys, F64)))
        planes = clip_planes(A.clips)
        n, b = check_clip(planes, pool)
        print(f"clip: {n - b}/{n} IsClip values bit-exact")
        fails += b
        nc, nq, b = check_interp(interp_cases(curves, A.interp), pool)
        print(f"interp: {nq - b}/{nq} queries bit-exact ({nc} tables)")
        fails += b
        tiles = tile_inputs([img for _, img in frames] or [img for _, img in cases[:3]], rng)
        n, b = check_tiles(tiles, pool)
        print(f"tile: {n - b // 1}/{n} pixnoise tiles bit-exact (noise, weight, bright)" if b == 0
              else f"tile: {b} plane mismatches over {n} tiles")
        fails += b
    print(f"total {time.perf_counter() - t_all:.1f} s: " + ("ALL BIT-EXACT" if fails == 0 else f"{fails} MISMATCHES"))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
