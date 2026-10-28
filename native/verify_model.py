# Bit-exact verification of the native nv_model port (N1e) against the Python implementation.
#
#   make -C native -f Makefile.model && python3 native/verify_model.py [--quick] [--no-real] [--jobs N]
#                                                                      [--genuine] [--exe PATH]
#
# Generates random / boundary planes and real-frame crops, computes nv_model.model_full (all 4096 words
# of the FUN_00995250 struct, repro/nv_model.py + nv_model_special.py + nv_model_bm.py) for set_ = -1
# and every set 0..4 with the Python port, runs the C++ binary native/nv_model_test over the same inputs
# and compares the status and all 4096 words BIT FOR BIT.
# The Python results are cached by plane hash in sandbox/omo/wp_native_model/tmp/pycache (the port
# takes ~2 s per 128^2 plane); real frames are read from the N1a frame cache
# (sandbox/omo/wp_native_band/tmp/*.npy) or cut with ffmpeg into sandbox/omo/wp_native_model/tmp/.
# --genuine additionally compares the C++ results with the cached genuine dumps of
# neatvideo_ofx_614/analysis/noise/model_verify/tmp/gcache (read only; the genuine is never run).
import argparse
import hashlib
import os
import struct
import subprocess
import sys
import time
from multiprocessing import Pool

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
REPRO = os.path.join(ROOT, "repro")
TMP = os.path.join(ROOT, "sandbox", "omo", "wp_native_model", "tmp")
PYCACHE = os.path.join(TMP, "pycache")
BAND_FRAMES = os.path.join(ROOT, "sandbox", "omo", "wp_native_band", "tmp")
MODEL_VERIFY = os.path.join(ROOT, "neatvideo_ofx_614", "analysis", "noise", "model_verify")
sys.path.insert(0, REPRO)
sys.dont_write_bytecode = True

import numpy as np  # noqa: E402

F32 = np.float32
U32 = np.uint32
EXE = os.path.join(HERE, "nv_model_test")
WORDS = 4096
STATUS = {0: "ok", 1: "NotImplementedError", 2: "ValueError"}

NANS = (0x7fc00000, 0xffc00000, 0x7fc12345, 0x7f800001, 0xff812345, 0x7fa00000)


# ------------------------------------------------------------------------------- case generation
def make_plane(rng, W, H, style):
    """Random / special planes (the styles of native/verify_band.py plus pattern-noise planes)."""
    if style == "normal":
        p = rng.normal(rng.uniform(0, 255), rng.uniform(0.1, 30), (H, W))
    elif style == "uniform":
        p = rng.uniform(0, 255, (H, W))
    elif style == "coloured":
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
    elif style == "edges":                      # oriented structure for the direction filters
        y, x = np.mgrid[0:H, 0:W]
        a = rng.uniform(0, np.pi)
        p = np.where(np.sin((x * np.cos(a) + y * np.sin(a)) / rng.uniform(2, 9)) > 0, 160.0, 90.0)
        p = p + rng.normal(0, 2, (H, W))
    elif style == "periodic":                   # fixed-pattern noise (FUN_00ad319c finds the period)
        ty, tx = int(rng.integers(5, 14)), int(rng.integers(5, 14))
        tile = rng.normal(0, rng.uniform(2, 8), (ty, tx))
        p = np.tile(tile, (H // ty + 1, W // tx + 1))[:H, :W] + rng.normal(100, rng.uniform(0.5, 3), (H, W))
    elif style == "periodic_nan":               # periodic with special values sprinkled in
        ty, tx = 7, 9
        tile = rng.normal(0, 5, (ty, tx))
        p = (np.tile(tile, (H // ty + 1, W // tx + 1))[:H, :W] + rng.normal(100, 1, (H, W))).astype(F32)
        u = p.view(U32)
        idx = rng.integers(0, H * W, max(1, H * W // 500))
        u.reshape(-1)[idx] = rng.choice(np.array(NANS + (0x7f800000, 0xff800000), U32), idx.size)
        return np.ascontiguousarray(p, F32)
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
        elif style == "densepayload":
            m = rng.random((H, W)) < 0.2
            u[m] = rng.choice(np.array(NANS, U32), int(m.sum()))
            u[rng.random((H, W)) < 0.01] = 0x7f800000
        elif style == "nanblock":
            y0, x0 = int(rng.integers(0, H)), int(rng.integers(0, W))
            u[y0:y0 + max(1, H // 5), x0:x0 + max(1, W // 5)] = 0x7fc00000
        elif style == "tailnan":                # NaN near the end: NaN neighbourhoods in the scalar tails
            u[H - 2, W - 2] = NANS[int(rng.integers(0, len(NANS)))]
            u[H - 1, max(0, W - 5)] = 0x7f800000
            u[H // 2, W - 1] = NANS[int(rng.integers(0, len(NANS)))]
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
        else:
            raise ValueError(style)
        return np.ascontiguousarray(p, F32)
    return np.ascontiguousarray(p, F32)


# (W, H): around the measurability limits of the levels, families A / dc / B / C, FUN_00ad319c
# (min side * 3 >= 83, direct at W >= 89 and H > 88, tiled 3 x 3 below) and odd sizes, up to 320^2
SIZES = [(2, 2), (3, 5), (8, 8), (9, 9), (16, 16), (17, 17), (20, 13), (24, 24), (25, 25), (28, 28),
         (29, 30), (31, 33), (37, 37), (44, 44), (45, 45), (48, 48), (50, 26), (57, 57), (64, 64), (88, 88),
         (89, 89), (89, 88), (88, 89), (97, 61), (101, 77), (120, 90), (128, 128), (133, 97), (160, 120),
         (192, 192), (210, 150), (256, 256), (257, 255), (77, 301), (320, 200), (320, 320)]
STYLES = ["normal", "periodic", "uniform", "coloured", "edges", "spike", "dc", "const", "nearconst", "bigdc",
          "tiny", "subnormal", "huge", "overflow", "int", "binary", "ramp", "steps", "neg", "nan", "inf",
          "allnan", "negnan", "snan", "payload", "densepayload", "nanblock", "negzero", "mixed",
          "periodic_nan"]
SETS = (-1, 0, 1, 2, 3, 4)


def random_cases(seed, quick):
    rng = np.random.default_rng(seed)
    cases = []
    si = 0
    for (W, H) in SIZES:
        area = W * H
        nst = 5 if area <= 64 * 64 else (3 if area <= 160 * 160 else 2)
        if quick:
            nst = 1
        for _ in range(nst):
            st = STYLES[si % len(STYLES)]
            si += 1
            p = make_plane(rng, W, H, st)
            for s in SETS:
                cases.append(("rand", W, H, s, 5, p, f"{st} {W}x{H}"))
    # periodic planes in the sizes where FUN_00ad319c works (direct and tiled)
    for (W, H) in ([(96, 96), (128, 128), (45, 60), (180, 140)] if not quick else [(96, 96)]):
        for st in ("periodic", "periodic_nan"):
            p = make_plane(rng, W, H, st)
            for s in (-1, 4):
                cases.append(("rand", W, H, s, 5, p, f"{st} {W}x{H}"))
    # NaN / inf next to the last pixels, in sizes with W * H % 4 != 0 (scalar tails of the NEON helpers)
    for (W, H) in ([(45, 45), (97, 61), (89, 89), (101, 77), (57, 57), (133, 97)] if not quick else [(45, 45)]):
        p = make_plane(rng, W, H, "tailnan")
        for s in SETS:
            cases.append(("tail", W, H, s, 5, p, f"tailnan {W}x{H}"))
    # fewer levels
    if not quick:
        for (W, H), L in (((64, 64), 1), ((128, 96), 2), ((100, 100), 3), ((90, 90), 4), ((45, 45), 4),
                          ((160, 160), 3), ((33, 70), 2), ((128, 128), 4)):
            p = make_plane(rng, W, H, "normal" if L % 2 else "periodic")
            for s in (-1, 4):
                cases.append(("levels", W, H, s, L, p, f"L{L} {W}x{H}"))
    return cases


# ------------------------------------------------------------------------------- real frames
FRAMES = [("test_v02.mp4", n) for n in (0, 100, 312, 576)] + [("test.mp4", n) for n in (0, 300, 450, 750)]


def frame(video, n):
    """RGB float32 0..255 of frame n (rgb48le / 257, as quality_draft._frame)."""
    name = f"{os.path.splitext(video)[0]}_{n}.npy"
    for d in (BAND_FRAMES, TMP):
        p = os.path.join(d, name)
        if os.path.exists(p):
            return np.load(p)
    os.makedirs(TMP, exist_ok=True)
    path = os.path.join(ROOT, video)
    wh = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
                         "stream=width,height", "-of", "csv=p=0", path],
                        check=True, capture_output=True, text=True).stdout.strip().split(",")
    W, H = int(wh[0]), int(wh[1])
    raw = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-i", path, "-vf",
                          f"select=eq(n\\,{n})", "-frames:v", "1", "-f", "rawvideo", "-pix_fmt",
                          "rgb48le", "-"], check=True, capture_output=True).stdout
    rgb = (np.frombuffer(raw, np.uint16).reshape(H, W, 3).astype(F32) / F32(257.0)).astype(F32)
    np.save(os.path.join(TMP, name), rgb)
    return rgb


def real_cases(quick):
    import nv_flat as nf
    cases = []
    frames = FRAMES if not quick else FRAMES[::4]
    for video, n in frames:
        rgb = frame(video, n)
        H, W = rgb.shape[:2]
        # (x0, y0, w, h, planes): 128^2 Y/Cr/Cb, 192^2 R/G/B, an odd-sized Y crop
        areas = [(W // 2 - 64, H // 4, 128, 128, "YCC"), ((W - 192) // 3, (H - 192) // 2, 192, 192, "RGB"),
                 (W // 5, H // 2, 101, 77, "Y")]
        for (x0, y0, w, h, kind) in areas:
            crop = rgb[y0:y0 + h, x0:x0 + w]
            R, G, B = (np.ascontiguousarray(crop[..., c], F32) for c in range(3))
            if kind == "RGB":
                planes = [("R", R), ("G", G), ("B", B)]
            else:
                T = nf.transform(R, G, B, 1)          # Y, (R-Y)*0.713, (B-Y)*0.565 (colour mode 1)
                planes = list(zip(("Y", "Cr", "Cb"), T))
                if kind == "Y":
                    planes = planes[:1]
            for name, p in planes:
                p = np.ascontiguousarray(p, F32)
                for s in SETS:
                    cases.append(("real", w, h, s, 5, p, f"{video} n={n} {name} {w}x{h}@({x0},{y0})"))
    return cases


# ------------------------------------------------------------------------------- expected values
def _key(p, s, L):
    h = hashlib.sha1(np.ascontiguousarray(p, F32).tobytes()).hexdigest()
    return os.path.join(PYCACHE, f"{h}_{p.shape[1]}x{p.shape[0]}_s{s}_L{L}.npy")


def expected(job):
    """(status, words) of nv_model.model_full; cached as uint32[4097] (status first)."""
    p, s, L = job
    path = _key(p, s, L)
    if os.path.exists(path):
        r = np.load(path)
        return int(r[0]), r[1:]
    import warnings
    warnings.simplefilter("ignore")
    import nv_model as nm
    t0 = time.time()
    try:
        with np.errstate(all="ignore"):
            w = np.asarray(nm.model_full(p, s, levels=L), U32)
        st = 0
    except NotImplementedError:
        w, st = np.zeros(WORDS, U32), 1
    except ValueError:
        w, st = np.zeros(WORDS, U32), 2
    dt = time.time() - t0
    r = np.concatenate([np.array([st], U32), w])
    tmp = path + f".{os.getpid()}.tmp.npy"
    np.save(tmp, r)
    os.replace(tmp, path)
    return st, w, dt


def run_cpp(exe, cases, tag):
    os.makedirs(TMP, exist_ok=True)
    inp = os.path.join(TMP, f"model_cases_{tag}_{os.getpid()}.bin")
    outp = os.path.join(TMP, f"model_results_{tag}_{os.getpid()}.bin")
    with open(inp, "wb") as f:
        f.write(struct.pack("<Q", len(cases)))
        for (W, H, s, L, data) in cases:
            f.write(struct.pack("<IIiI", W, H, s, L))
            f.write(np.ascontiguousarray(data, F32).tobytes())
    t1 = time.time()
    r = subprocess.run([exe, inp, outp], capture_output=True, text=True)
    t = time.time() - t1
    if r.returncode:
        raise SystemExit(f"{exe} failed ({r.returncode}): {r.stderr[-500:]}")
    res = np.fromfile(outp, U32).reshape(len(cases), WORDS + 1)
    os.remove(inp)
    os.remove(outp)
    return res, t, r.stderr.strip()


def region(w):
    if w < 55:
        return "levels"
    for name, lo, hi in (("dc", 0xdc, 0x140), ("A", 0x140, 0x780), ("R", 0x780, 0x7e4), ("B", 0x7e4, 0xe24),
                         ("C", 0xe24, 0x1464), ("score", 0x1464, 0x1504), ("offs", 0x1504, 0x1f04),
                         ("bm", 0x1f04, 0x1f54)):
        if lo <= 4 * w < hi:
            return name
    return "rest"


# ------------------------------------------------------------------------------- genuine (cached) cases
def genuine_cases():
    """(name, plane, set, genuine words) of model_verify/cases.py whose genuine dump is in the gcache."""
    sys.path.insert(0, MODEL_VERIFY)
    import cases as mc

    def frame_readonly(name, n):                  # never writes into model_verify/data
        p = os.path.join(MODEL_VERIFY, "data", f"{os.path.splitext(name)[0]}_{n}.npy")
        if not os.path.exists(p):
            raise FileNotFoundError(p)
        return np.load(p)
    mc.frame_cached = frame_readonly
    gc = os.path.join(MODEL_VERIFY, "tmp", "gcache")
    out = []
    missing = 0
    for name, group, build in mc.all_cases():
        try:
            p = np.ascontiguousarray(build(), F32)
        except FileNotFoundError:
            missing += 1
            continue
        h = hashlib.sha1(p.tobytes()).hexdigest()
        for s in SETS:
            f = os.path.join(gc, f"{h}_{p.shape[1]}x{p.shape[0]}_s{s}.npy")
            if os.path.exists(f):
                out.append((name, p, s, np.load(f)))
    return out, missing


# ------------------------------------------------------------------------------- explicit-error paths (F2)
def _bm_valid_rows(c, sizes):
    """Validity masks over tab.list1 of every (x interval, y interval) of block placements that occur in the
    sides `sizes` (the rule of sad_grid: X + dx >= 0, X + dx + bs <= Wp, X = pad + x0 = positions(n, step)
    and Wp = n + 2 pad).  Rows of costs 0 (valid) / INF: best_direction_all's qualification depends on the
    INF count per bin only, not on the cost values."""
    import nv_model_bm as bm
    g = bm.CFG[c]
    bs, step, pad, r = g["bs"], g["step"], g["pad"], g["r28"]
    L = bm.table(c).list1                              # offsets within [-r, r]^2: the ranges are clipped to it
    ivs = set()
    for n in sizes:
        for X in bm.positions(n, step):                # dx range with X + dx >= 0 and X + dx + bs <= n + 2 pad
            ivs.add((max(-r, -X), min(r, n + 2 * pad - bs - X)))
    ivs = sorted(ivs)
    rows = []
    for lx, hx in ivs:
        okx = (L[:, 0] >= lx) & (L[:, 0] <= hx)
        for ly, hy in ivs:
            ok = okx & (L[:, 1] >= ly) & (L[:, 1] <= hy)
            rows.append(np.where(ok, 0, bm.INF))
    return np.array(rows, np.int64), len(ivs)


def ub_checks(exe):
    """The two not-ported paths of nv_model_bm (sandbox/omo/wp_native_model_ub/REPORT.md):
    (1) model_full cannot reach them: FUN_00ad319c calls ad1ae0 only with sides >= 84 (min side * 3 >= 83,
        3 x 3 tiled below 89 x 89), above every padding (4 / 8), and with sides >= 84 every block has a
        qualifying direction bin.  Checked over all block placements of the sides 84..1536 (the tiled
        sides reach 3 x 512) with the Python port's own best_direction_all; the same reconstruction is
        checked against sad_grid's INF mask and must predict the failures of small sides (the check is not
        vacuous);
    (2) where reached (ad1ae0 on its own, small planes), the C++ (nv_model_test bm) gives the same explicit
        error as the Python port (NotImplementedError -> status 1) and the same quantile bits otherwise."""
    import warnings
    warnings.simplefilter("ignore")
    import nv_model_bm as bm
    bad = 0
    for c in (0, 1):
        g = bm.CFG[c]
        tab = bm.table(c)
        rows, niv = _bm_valid_rows(c, range(84, 1537))
        try:
            bm.best_direction_all(rows, tab, g["n8"] - 1)
            geo = "no block without a qualifying bin"
        except NotImplementedError as e:
            geo = f"FAILED: {e}"
            bad += 1
        # the reconstruction against sad_grid's INF mask (zero image), and its failures on small sides
        for (W, H) in ((84, 84), (89, 97), (101, 333), (17, 20), (9, 30)):
            pad = g["pad"]
            xs, ys = bm.positions(W, g["step"]), bm.positions(H, g["step"])
            grid = [(x - pad, y - pad) for y in ys for x in xs]
            C = bm.sad_grid(np.zeros((H + 2 * pad, W + 2 * pad), np.uint8), grid, g["bs"], g["r28"], pad, tab.list1)
            L = tab.list1
            R = []
            for y in ys:
                for x in xs:
                    ok = ((L[:, 0] >= -x) & (L[:, 0] <= W + 2 * pad - g["bs"] - x)
                          & (L[:, 1] >= -y) & (L[:, 1] <= H + 2 * pad - g["bs"] - y))
                    R.append(np.where(ok, 0, bm.INF))
            same = np.array_equal(np.array(R) == bm.INF, C == bm.INF)
            try:
                bm.best_direction_all(np.array(R), tab, g["n8"] - 1)
                fails = False
            except NotImplementedError:
                fails = True
            small = min(W, H) < 84
            if not same or fails != (small and min(W, H) <= (12 if c == 0 else 28)):
                bad += 1
                print(f"  ub geometry cfg {c} {W}x{H}: INF mask {'same' if same else 'DIFFERENT'}, "
                      f"failure {fails}  MISMATCH")
        print(f"  ub geometry cfg {c}: {len(rows)} placements ({niv} x {niv} dx / dy intervals of the sides 84..1536): "
              f"{geo}")
    # (2) direct calls: Python bm.ad1ae0 vs nvm::bm_quantiles
    rng = np.random.default_rng(0xB10C)
    cases = []
    for c, W, H, kind in ((1, 6, 40, "pad"), (1, 40, 8, "pad"), (0, 4, 4, "pad"), (0, 30, 3, "pad"),
                          (1, 20, 20, "bins"), (1, 17, 28, "bins"), (0, 10, 12, "bins"), (0, 9, 9, "bins"),
                          (0, 14, 14, "ok"), (1, 29, 29, "ok"), (0, 84, 84, "ok"), (1, 84, 84, "ok"),
                          (1, 89, 120, "ok"), (0, 97, 61, "ok")):
        for style in ("normal", "periodic_nan"):
            cases.append((c, W, H, kind, style, make_plane(rng, W, H, "normal" if style == "normal" else "coloured"),
                          make_plane(rng, W, H, style)))
    exp = []
    for c, W, H, kind, style, clean, A in cases:
        try:
            with np.errstate(all="ignore"):
                q = bm.ad1ae0(clean, A, c)
            exp.append((0, np.array(q, F32)))
        except NotImplementedError:
            exp.append((1, None))
    inp = os.path.join(TMP, f"model_bm_{os.getpid()}.bin")
    outp = os.path.join(TMP, f"model_bm_{os.getpid()}.out")
    with open(inp, "wb") as f:
        f.write(struct.pack("<Q", len(cases)))
        for c, W, H, kind, style, clean, A in cases:
            f.write(struct.pack("<IIi", W, H, c) + clean.tobytes() + A.tobytes())
    r = subprocess.run([exe, "bm", inp, outp], capture_output=True, text=True)
    if r.returncode:
        raise SystemExit(f"{exe} bm failed ({r.returncode}): {r.stderr[-500:]}")
    res = np.fromfile(outp, U32).reshape(len(cases), 3)
    os.remove(inp)
    os.remove(outp)
    nok = 0
    kinds = {}
    for (c, W, H, kind, style, _, _), (est, eq), row in zip(cases, exp, res):
        want = 0 if kind == "ok" else 1
        good = int(row[0]) == est == want and (est or np.array_equal(row[1:], eq.view(U32)))
        nok += good
        kinds.setdefault(kind, [0, 0])
        kinds[kind][0] += 1
        kinds[kind][1] += good
        if not good:
            print(f"  ub direct cfg {c} {W}x{H} {style} ({kind}): Python status {est} {eq}, C++ {int(row[0])} "
                  f"{row[1:].view(F32)}  MISMATCH")
    bad += len(cases) - nok
    print(f"  ub direct ad1ae0: {nok}/{len(cases)} identical (" + ", ".join(
        f"{k}: {v[1]}/{v[0]}" for k, v in kinds.items()) + "; pad / bins = NotImplementedError on both sides)")
    return bad


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--no-real", action="store_true")
    ap.add_argument("--jobs", type=int, default=3, help="Python processes (at most 3)")
    ap.add_argument("--seed", type=int, default=0x5EED)
    ap.add_argument("--exe", default=EXE, help="binary to test (default native/nv_model_test)")
    ap.add_argument("--genuine", action="store_true",
                    help="also compare with the cached genuine dumps of model_verify (no genuine run)")
    ap.add_argument("--only-genuine", action="store_true")
    a = ap.parse_args()
    os.makedirs(PYCACHE, exist_ok=True)
    bad = 0
    if not a.only_genuine:
        cases = random_cases(a.seed, a.quick)
        if not a.no_real:
            cases += real_cases(a.quick)
        t0 = time.time()
        jobs = [(c[5], c[3], c[4]) for c in cases]
        todo = [j for j in jobs if not os.path.exists(_key(*j))]
        if todo:
            nj = max(1, min(a.jobs, 3))
            print(f"Python: {len(todo)} of {len(jobs)} results not cached, computing with {nj} processes ...",
                  flush=True)
            with Pool(nj) as pool:
                py_times = [r[2] for r in pool.imap_unordered(expected, todo, chunksize=1)]
            print(f"Python: {sum(py_times):.1f} s of model_full in total", flush=True)
        exp = [expected(j)[:2] for j in jobs]
        t_py = time.time() - t0
        res, t_cpp, msg = run_cpp(a.exe, [(c[1], c[2], c[3], c[4], c[5]) for c in cases], "main")
        stats = {}
        nanc = 0
        for case, (est, ew), row in zip(cases, exp, res):
            group, W, H, s, L, data, label = case
            key = (group, "set -1" if s < 0 else "set 0-4")
            st = stats.setdefault(key, [0, 0, 0])
            st[0] += 1
            if est:
                st[2] += 1
            nanc += bool(np.isnan(data).any())
            ok = int(row[0]) == est and (est != 0 or np.array_equal(row[1:], ew))
            if not ok:
                st[1] += 1
                bad += 1
                if bad <= 12:
                    if int(row[0]) != est:
                        print(f"MISMATCH [{group}] {label} set {s} L{L}: status C++ {int(row[0])} vs Py {est}")
                    else:
                        d = np.nonzero(row[1:] != ew)[0]
                        regs = {}
                        for w in d:
                            regs[region(int(w))] = regs.get(region(int(w)), 0) + 1
                        print(f"MISMATCH [{group}] {label} set {s} L{L}: {d.size} words differ {regs}, first "
                              f"{[int(x) for x in d[:5]]}: C++ {[hex(x) for x in row[1:][d[:5]]]} "
                              f"vs Py {[hex(x) for x in ew[d[:5]]]}")
        for (group, kind), (n, nbad, nni) in sorted(stats.items()):
            extra = f" (status != 0 in Python: {nni})" if nni else ""
            print(f"  {group:6s} {kind:8s}: {n - nbad}/{n} bit-exact{extra}")
        nplanes = len({hashlib.sha1(c[5].tobytes()).hexdigest() for c in cases})
        print(f"{len(cases) - bad}/{len(cases)} bit-exact ({nplanes} planes x sets; {nanc} runs with NaN in the "
              f"input; {sum(1 for e in exp if e[0] == 0) * WORDS} words compared)"
              + ("" if bad == 0 else f"  ({bad} MISMATCHES)"))
        print(f"time: Python {t_py:.1f} s (cache + {len(todo)} computed), C++ {t_cpp:.2f} s wall ({msg})")
    if not a.only_genuine:
        bad += ub_checks(a.exe)
    if a.genuine or a.only_genuine:
        g, missing = genuine_cases()
        res, t_cpp, msg = run_cpp(a.exe, [(p.shape[1], p.shape[0], s, 5, p) for (_, p, s, _) in g], "genuine")
        gbad = 0
        nimpl = 0
        for (name, p, s, gw), row in zip(g, res):
            if int(row[0]) != 0:
                nimpl += 1
                print(f"genuine: {name} set {s}: C++ status {int(row[0])} (genuine has a dump)")
                gbad += 1
                continue
            if not np.array_equal(row[1:], gw):
                gbad += 1
                if gbad <= 10:
                    d = np.nonzero(row[1:] != gw)[0]
                    print(f"GENUINE MISMATCH {name} {p.shape[1]}x{p.shape[0]} set {s}: {d.size} words, first "
                          f"{[int(x) for x in d[:5]]}")
        print(f"genuine (cached nvh dumps): {len(g) - gbad}/{len(g)} bit-exact ({missing} cases skipped: frame "
              f"not cached); C++ {t_cpp:.1f} s ({msg})")
        bad += gbad
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
