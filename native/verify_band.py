# Bit-exact verification of the native nv_band port (N1a) against the Python implementation.
#
#   make -C native -f Makefile.band && python3 native/verify_band.py [--quick] [--no-real] [--jobs N]
#
# Generates random / boundary planes and real-frame crops, computes nv_band.model_levels (all 4096
# words; the pass criterion is words 0..54) and nv_band.band_levels (all 4096 words) with the Python
# port (repro/nv_band.py), plus direct cases of quantile / mean / std, runs the C++ binary
# native/nv_band_test over the same inputs and compares the results BIT FOR BIT.
# Real frames are cut with ffmpeg from test_v02.mp4 / test.mp4 (workspace root) and cached in
# sandbox/omo/wp_native_band/tmp/.
import argparse
import os
import struct
import subprocess
import sys
import time
from multiprocessing import Pool

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
REPRO = os.path.join(ROOT, "repro")
TMP = os.path.join(ROOT, "sandbox", "omo", "wp_native_band", "tmp")
sys.path.insert(0, REPRO)
sys.dont_write_bytecode = True

import numpy as np  # noqa: E402

F32 = np.float32
U32 = np.uint32
EXE = os.path.join(HERE, "nv_band_test")
STRUCT_WORDS = 4096

# NaN payloads used by the special planes (bit patterns): default, negative default, quiet with a
# payload, signalling with payloads (both signs)
NANS = (0x7fc00000, 0xffc00000, 0x7fc12345, 0x7f800001, 0xff812345, 0x7fa00000)


def fbits(*u):
    return np.array(u, U32).view(F32)


# ------------------------------------------------------------------------------- case generation
SIZES = [(1, 1), (2, 3), (5, 5), (8, 8), (9, 9), (9, 10), (10, 9), (16, 9), (15, 15), (17, 17),
         (12, 13), (13, 12), (14, 30), (24, 24), (30, 30), (31, 33), (32, 32), (33, 31), (40, 64),
         (46, 46), (64, 64), (63, 65), (72, 40), (72, 41), (72, 72), (73, 73), (74, 72), (100, 37),
         (128, 128), (127, 129), (129, 127), (200, 150), (255, 257), (256, 256), (320, 320),
         (321, 319), (400, 300), (512, 512), (511, 512), (512, 384), (1, 200), (200, 1), (3, 300),
         (300, 9), (9, 300), (12, 500), (500, 12), (36, 1024), (1024, 36), (68, 68), (19, 37),
         (41, 23), (8, 9), (9, 8), (20, 76), (152, 18), (35, 35), (88, 88), (136, 72),
         (35, 5), (5, 35), (39, 9), (9, 39), (36, 5), (5, 36), (40, 9), (70, 18), (18, 70)]
STYLES = ["normal", "uniform", "coloured", "spike", "dc", "const", "nearconst", "bigdc", "tiny",
          "subnormal", "huge", "overflow", "int", "binary", "ramp", "nan", "inf", "allnan", "negnan",
          "snan", "payload", "negzero", "mixed", "neg", "steps", "densepayload"]


def make_plane(rng, W, H, style):
    if style == "normal":
        p = rng.normal(rng.uniform(0, 255), rng.uniform(0.1, 30), (H, W))
    elif style == "uniform":
        p = rng.uniform(0, 255, (H, W))
    elif style == "coloured":                    # low-pass noise (box-blurred white)
        p = rng.normal(0, 10, (H + 4, W + 4))
        p = (p[:-4, :-4] + p[2:-2, 2:-2] + p[4:, 4:] + p[:-4, 4:] + p[4:, :-4]) / 5 + 100
    elif style == "spike":
        p = rng.normal(50, 1, (H, W))
        m = rng.random((H, W)) < 0.01
        p[m] = rng.uniform(200, 1e4, m.sum())
    elif style == "dc":
        p = np.full((H, W), rng.uniform(-100, 300))
    elif style == "const":
        p = np.zeros((H, W))
    elif style == "nearconst":
        p = np.full((H, W), 128.0)
        m = rng.random((H, W)) < 0.002
        p[m] += rng.choice([-1, 1], m.sum()) * 2.0 ** -10
    elif style == "bigdc":
        p = rng.normal(1e6, 3, (H, W))
    elif style == "tiny":
        p = rng.normal(0, 1e-30, (H, W))
    elif style == "subnormal":
        p = (rng.integers(0, 2 ** 32, (H, W), dtype=np.uint64).astype(U32) & U32(0x807fffff)).view(F32)
        return np.ascontiguousarray(p, F32)
    elif style == "huge":
        p = rng.normal(0, 1e30, (H, W))
    elif style == "overflow":
        p = rng.uniform(1e37, 3.4e38, (H, W))
    elif style == "int":
        p = rng.integers(0, 256, (H, W)).astype(np.float64)
    elif style == "binary":
        p = rng.integers(0, 2, (H, W)) * 255.0
    elif style == "ramp":
        y, x = np.mgrid[0:H, 0:W]
        p = x * rng.uniform(-2, 2) + y * rng.uniform(-2, 2) + rng.normal(0, 0.5, (H, W))
    elif style == "steps":
        p = (np.mgrid[0:H, 0:W][1] // max(1, W // 7)) * 30.0 + rng.normal(0, 2, (H, W))
    elif style == "neg":
        p = rng.normal(-50, 20, (H, W))
    else:                                        # special values on a noise background
        p = rng.normal(100, 5, (H, W)).astype(F32)
        u = p.view(U32)
        n = H * W
        if style == "nan":
            u[rng.random((H, W)) < 0.003] = 0x7fc00000
        elif style == "inf":
            u[rng.random((H, W)) < 0.002] = 0x7f800000
            u[rng.random((H, W)) < 0.002] = 0xff800000
        elif style == "allnan":
            u[:] = 0x7fc00000
        elif style == "negnan":
            u[rng.random((H, W)) < 0.004] = 0xffc00000
            u[rng.random((H, W)) < 0.004] = 0x7fc00000
        elif style == "snan":
            u[rng.random((H, W)) < 0.004] = 0x7f800001
            u[rng.random((H, W)) < 0.004] = 0xff812345
        elif style == "payload":
            idx = rng.integers(0, n, max(1, n // 300))
            u.reshape(-1)[idx] = rng.choice(np.array(NANS, U32), idx.size)
        elif style == "densepayload":           # many NaNs: both operands NaN often
            m = rng.random((H, W)) < 0.2
            u[m] = rng.choice(np.array(NANS, U32), int(m.sum()))
            u[rng.random((H, W)) < 0.01] = 0x7f800000
        elif style == "negzero":
            u[:] = 0x80000000
            u[rng.random((H, W)) < 0.3] = 0x00000000
        elif style == "mixed":
            r = rng.random((H, W))
            u[r < 0.002] = 0x7f800000
            u[(r >= 0.002) & (r < 0.004)] = 0x7fc12345
            u[(r >= 0.004) & (r < 0.006)] = 0x00000001
            u[(r >= 0.006) & (r < 0.008)] = 0x80000000
            u[(r >= 0.008) & (r < 0.009)] = 0x7f7fffff
        return np.ascontiguousarray(p, F32)
    return np.ascontiguousarray(p, F32)


def random_cases(seed, quick):
    rng = np.random.default_rng(seed)
    cases = []
    sizes = SIZES if not quick else SIZES[::3]
    for kind in (0, 1):
        for (W, H) in sizes:
            styles = STYLES if (W * H <= 128 * 128 and not quick) else STYLES[::2]
            for st in styles:
                if quick and rng.random() < 0.5:
                    continue
                levels = 5 if rng.random() < 0.9 else int(rng.integers(1, 5))
                cases.append(("rand", kind, W, H, (levels, 0, 0), make_plane(rng, W, H, st),
                              f"{st} {W}x{H} L{levels}"))
        # random sizes
        for _ in range(40 if not quick else 8):
            W, H = int(rng.integers(1, 520)), int(rng.integers(1, 520))
            st = STYLES[int(rng.integers(0, len(STYLES)))]
            cases.append(("rand", kind, W, H, (5, 0, 0), make_plane(rng, W, H, st), f"{st} {W}x{H}"))
    return cases


def quantile_cases(seed, quick):
    rng = np.random.default_rng(seed)
    cases = []
    qs = [0.99, 0.5, 0.01, 0.9, 0.3, 0.0, -0.1, 0.999, 0.75]
    ns = [0, 1, 2, 3, 4, 7, 8, 9, 15, 16, 48, 49, 50, 51, 63, 64, 100, 999, 2047, 2048, 2049,
          2055, 4095, 4096, 4097, 5000, 9000]
    reps = 1 if quick else 40
    for n in ns:
        for _ in range(reps):
            st = STYLES[int(rng.integers(0, len(STYLES)))] if rng.random() < 0.7 else "densepayload"
            v = make_plane(rng, max(n, 1), 1, st).reshape(-1)[:n]
            if rng.random() < 0.2 and n:                  # ties
                with np.errstate(all="ignore"):
                    v = np.round(v / 4) * 4
            v = np.ascontiguousarray(v, F32)
            q = F32(qs[int(rng.integers(0, len(qs)))])
            if rng.random() < 0.1:
                q = fbits(int(rng.choice([0x7fc00000, 0xffc00000, 0x7fc12345])))[0]
            flag = int(rng.choice([1, 1, 1, 0, 2, 3]))
            div = int(rng.choice([4, 4, 4, 0, 1, 7, 100]))
            cases.append(("quant", 2, n, 1, (int(np.array([q], F32).view(U32)[0]), flag, div), v,
                          f"quantile {st} n={n} q={float(q)} flag={flag} div={div}"))
            cases.append(("ms", 3, n, 1, (0, 0, 0), v, f"mean/std {st} n={n}"))
    return cases


# ------------------------------------------------------------------------------- real frames
FRAMES = [("test_v02.mp4", n) for n in (0, 100, 200, 312, 450, 576, 700)] + \
         [("test.mp4", n) for n in (0, 150, 300, 450, 750)]


def frame(video, n):
    """RGB float32 0..255 of frame n (rgb48le / 257, as quality_draft._frame), cached in TMP."""
    os.makedirs(TMP, exist_ok=True)
    cache = os.path.join(TMP, f"{os.path.splitext(video)[0]}_{n}.npy")
    if os.path.exists(cache):
        return np.load(cache)
    path = os.path.join(ROOT, video)
    wh = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
                         "stream=width,height", "-of", "csv=p=0", path],
                        check=True, capture_output=True, text=True).stdout.strip().split(",")
    W, H = int(wh[0]), int(wh[1])
    raw = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-i", path, "-vf",
                          f"select=eq(n\\,{n})", "-frames:v", "1", "-f", "rawvideo", "-pix_fmt",
                          "rgb48le", "-"], check=True, capture_output=True).stdout
    rgb = (np.frombuffer(raw, np.uint16).reshape(H, W, 3).astype(F32) / F32(257.0)).astype(F32)
    np.save(cache, rgb)
    return rgb


def real_cases():
    import nv_flat as nf
    cases = []
    for video, n in FRAMES:
        rgb = frame(video, n)
        H, W = rgb.shape[:2]
        areas = [(0, 0, 128), (W // 2 - 64, H // 3, 128), ((W - 320) // 2, (H - 320) // 2, 320)]
        for (x0, y0, s) in areas:
            crop = rgb[y0:y0 + s, x0:x0 + s]
            R, G, B = (np.ascontiguousarray(crop[..., c]) for c in range(3))
            T = nf.transform(R, G, B, 1)          # Y, (R-Y)*0.713, (B-Y)*0.565 (colour mode 1)
            for name, p in zip(("Y", "Cr", "Cb"), T):
                p = np.ascontiguousarray(p, F32)
                for kind in (0, 1):
                    cases.append(("real", kind, s, s, (5, 0, 0), p,
                                  f"{video} n={n} {name} {s}x{s}@({x0},{y0})"))
    return cases


# ------------------------------------------------------------------------------- expected values
def expected(case):
    import nv_band as nb
    _, kind, W, H, (p0, p1, p2), data, _ = case
    with np.errstate(all="ignore"):
        if kind == 0:
            return np.asarray(nb.model_levels(data.reshape(H, W), levels=p0)).view(U32).tobytes()
        if kind == 1:
            return np.asarray(nb.band_levels(data.reshape(H, W), levels=p0)).view(U32).tobytes()
        if kind == 2:
            q = np.array([p0], U32).view(F32)[0]
            r = nb.quantile(data, q, p1, p2)
            return struct.pack("<I", 1) + np.array([r], F32).tobytes()
        m = nb.mean_exact(data)
        s = nb.std_exact(data, m)
        return np.array([m, s], F32).tobytes()


def _chunk(cs):
    return [expected(c) for c in cs]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--no-real", action="store_true")
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--seed", type=int, default=0x5EED)
    ap.add_argument("--keep", action="store_true", help="keep the case / result files")
    ap.add_argument("--exe", default=EXE, help="binary to test (default native/nv_band_test)")
    ap.add_argument("--cache", action="store_true",
                    help="reuse / store the Python results in the tmp directory (mutation runs)")
    a = ap.parse_args()

    cases = random_cases(a.seed, a.quick) + quantile_cases(a.seed + 1, a.quick)
    if not a.no_real:
        cases += real_cases()
    t0 = time.time()
    jobs = max(1, min(a.jobs, 4))
    cache = os.path.join(TMP, f"band_expected_{a.seed}_{int(a.quick)}_{int(a.no_real)}.npz")
    exp = None
    if a.cache and os.path.exists(cache):
        z = np.load(cache)
        exp = [z[f"e{i}"].tobytes() for i in range(len(z.files))]
        if len(exp) != len(cases):
            exp = None
    if exp is not None:
        pass
    elif jobs > 1:
        chunks = [cases[i::jobs] for i in range(jobs)]
        with Pool(jobs) as pool:
            parts = pool.map(_chunk, chunks)
        exp = [None] * len(cases)
        for i, part in enumerate(parts):
            for j, e in enumerate(part):
                exp[i + j * jobs] = e
    else:
        exp = _chunk(cases)
    t_py = time.time() - t0
    if a.cache and not os.path.exists(cache):
        os.makedirs(TMP, exist_ok=True)
        np.savez(cache, **{f"e{i}": np.frombuffer(e, np.uint8) for i, e in enumerate(exp)})

    os.makedirs(TMP, exist_ok=True)
    inp = os.path.join(TMP, "band_cases.bin")
    outp = os.path.join(TMP, "band_results.bin")
    with open(inp, "wb") as f:
        f.write(struct.pack("<Q", len(cases)))
        for (_, kind, W, H, (p0, p1, p2), data, _) in cases:
            f.write(struct.pack("<6I", kind, W, H, p0 & 0xffffffff, p1 & 0xffffffff, p2 & 0xffffffff))
            f.write(np.ascontiguousarray(data, F32).tobytes())
    t1 = time.time()
    r = subprocess.run([a.exe, inp, outp], check=True, capture_output=True, text=True)
    t_cpp = time.time() - t1
    res = open(outp, "rb").read()

    off = 0
    stats = {}
    bad = 0
    for case, e in zip(cases, exp):
        group, kind, W, H, _, _, label = case
        size = STRUCT_WORDS * 4 if kind in (0, 1) else 8
        got = res[off:off + size]
        off += size
        key = (group, kind)
        st = stats.setdefault(key, [0, 0])
        st[0] += 1
        if kind == 0:                              # pass criterion: words 0..54; report the rest too
            g55, e55 = got[:55 * 4], e[:55 * 4]
            ok = g55 == e55
            if ok and got != e:
                print(f"note: words >= 55 differ (init pattern) in {label}")
                ok = False
        else:
            ok = got == e
        if not ok:
            st[1] += 1
            bad += 1
            if bad <= 10:
                gw = np.frombuffer(got, U32)
                ew = np.frombuffer(e, U32)
                d = np.nonzero(gw != ew)[0]
                print(f"MISMATCH [{group} kind {kind}] {label}: {d.size} words differ, first "
                      f"{list(d[:6])}: C++ {[hex(x) for x in gw[d[:6]]]} vs Py {[hex(x) for x in ew[d[:6]]]}")
    assert off == len(res), "result size mismatch"
    names = {0: "model_levels", 1: "band_levels", 2: "quantile", 3: "mean/std"}
    for (group, kind), (n, nbad) in sorted(stats.items()):
        print(f"  {group:5s} {names[kind]:13s}: {n - nbad}/{n} bit-exact")
    print(f"{len(cases) - bad}/{len(cases)} bit-exact" + ("" if bad == 0 else f"  ({bad} MISMATCHES)"))
    print(f"time: Python {t_py:.1f} s ({jobs} procs), C++ {t_cpp:.2f} s (one process); "
          + r.stderr.strip())
    if not a.keep:
        for p in (inp, outp):
            os.remove(p)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
