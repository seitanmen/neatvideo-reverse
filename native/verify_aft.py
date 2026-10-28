# Bit-exact verification of the native AutoFineTune port (native/nv_aft.{h,cpp}, N1i) against the
# Python implementation (neatvideo_ofx_614/analysis/noise/aft_port/src/nv_aft.py and the colour
# combination of repro/nv_aft_v5.apply_aft).
#
#   make -C native -f Makefile.aft && python3 native/verify_aft.py [--quick] [--jobs J] [--no-real]
#                                                                   [--mutants] [--exe PATH]
#
# Parts (every value is compared BIT FOR BIT, no tolerance):
#   shuffle   nv_aft.shuffle_order(n) for n = 0..4096 (all entries)
#   sim       nv_aft.similarity on random / boundary model words (zeros, negatives, NaN, inf, CV chain ends)
#   finish    nv_aft._finish (curve_post + curve_fill for modes 0 / 1, mode2_post for mode 2) on random
#             curve states and bins: the whole 0xc300-byte profile and the three mode-2 values
#   aft       nv_aft.auto_fine_tune end to end: status, the whole 0xc300-byte profile it returns, the mode-2
#             values, the diag counters and the bins of _run_tiles (captured by wrapping nv_aft._run_tiles).
#             Real frames (test_v02.mp4 n=100/312/576 1080p, test.mp4 n=0/300/750) with the profile built by
#             nv_profile.build_profile (colour mode 1, as apply_aft) on the FindFlatArea region or a 128^2
#             region; synthetic frames (gradients + white / coloured noise, clipping, constant and NaN / inf
#             blocks, sizes that are not multiples of 32, 48^2, < 48, image2, cancel).  The C++ side runs with
#             -j 1 and again with -j J (must give the same bytes).
#   combine   repro/nv_aft_v5.apply_aft run with nv_profile.build_profile / nv_aft.auto_fine_tune replaced by
#             the cached bytes: aft_color_weights, aft_face_curves, curve (Y, Cb, Cr), sigma, aft_bin_flags.
#   --mutants negative controls: rebuilds nv_aft with -DNVA_MUT_* (wrong 66% quantile, the upper extrapolation
#             threshold last > 4, CV tolerance x s_lvl, >= 3.0 flag) and requires mismatches.
# The Python results are cached in sandbox/omo/wp_native_aft/tmp/ under a key of the input hash and the hash of
# the Python sources (the AFT port takes ~30 s per 1080p frame, the profile ~15 s).
import argparse
import glob
import hashlib
import os
import pickle
import struct
import subprocess
import sys
import time
from multiprocessing import Pool

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
REPRO = os.path.join(ROOT, "repro")
AFT_SRC = os.path.join(ROOT, "neatvideo_ofx_614", "analysis", "noise", "aft_port", "src")
sys.path.insert(0, REPRO)
sys.path.insert(0, AFT_SRC)
sys.dont_write_bytecode = True

import numpy as np  # noqa: E402
import warnings  # noqa: E402

warnings.simplefilter("ignore", RuntimeWarning)     # the Python ports compute with NaN / inf inputs

F32 = np.float32
F64 = np.float64
U32 = np.uint32
TMP = os.environ.get("NV_AFT_TMP", os.path.join(ROOT, "sandbox", "omo", "wp_native_aft", "tmp"))
EXE = os.path.join(HERE, "nv_aft_test")
BAND_FRAMES = os.path.join(ROOT, "sandbox", "omo", "wp_native_band", "tmp")
PROF = 0xc300
CURVE = (0xbf44, 0xc064, 0xc184)
DIAG_KEYS = ("ntiles", "clip_skip", "simG_skip", "simR_skip", "simB_skip", "diff_skip", "accepted")
NANS = (0x7fc00000, 0xffc00000, 0x7fc12345, 0x7fa00000, 0xff812345)
MUTANTS = ("NVA_MUT_Q66", "NVA_MUT_UPPER", "NVA_MUT_CVTOL", "NVA_MUT_GT3")


def src_hash():
    files = sorted(glob.glob(os.path.join(REPRO, "nv_*.py"))) + [os.path.join(AFT_SRC, "nv_aft.py")]
    h = hashlib.sha256()
    for f in files:
        if os.path.basename(f) in ("nv_ui.py", "nv_depth.py", "nv_cov.py"):
            continue
        h.update(os.path.basename(f).encode())
        with open(f, "rb") as fp:
            h.update(fp.read())
    return h.hexdigest()[:16]


SRC_HASH = src_hash()


def fbits(a):
    return np.ascontiguousarray(a, F32).view(U32)


def f32_from_bits(u):
    return np.array([u], U32).view(F32)[0]


# ------------------------------------------------------------------------------------ frames
def frame(video, n):
    stem = os.path.splitext(video)[0]
    p = os.path.join(BAND_FRAMES, f"{stem}_{n}.npy")
    if os.path.exists(p):
        return np.load(p)
    p = os.path.join(TMP, f"{stem}_{n}.npy")
    if not os.path.exists(p):
        src = os.path.join(ROOT, video)
        wh = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
                             "stream=width,height", "-of", "csv=p=0", src],
                            check=True, capture_output=True, text=True).stdout.strip().split(",")
        W, H = int(wh[0]), int(wh[1])
        raw = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-i", src, "-vf", f"select=eq(n\\,{n})",
                              "-fps_mode", "passthrough", "-frames:v", "1", "-f", "rawvideo", "-pix_fmt", "rgb48le",
                              "-"], check=True, capture_output=True).stdout
        a = np.frombuffer(raw, np.uint16, W * H * 3).reshape(H, W, 3)
        np.save(p, (a.astype(F32) / F32(257.0)).astype(F32))
    return np.load(p)


def synth(spec):
    """Deterministic synthetic image (and optional image2) of a spec tuple (kind, seed, W, H[, 0, "image2"])."""
    kind, seed, W, H = spec[:4]
    rng = np.random.default_rng(seed)
    y, x = np.mgrid[0:H, 0:W].astype(F64)
    base = 20 + 210 * (x / max(W - 1, 1)) * 0.7 + 210 * (y / max(H - 1, 1)) * 0.3
    sig = rng.uniform(1.5, 6)
    if kind == "white":
        img = base[..., None] + rng.normal(0, sig, (H, W, 3))
    elif kind == "coloured":
        n = rng.normal(0, sig, (H + 2, W + 2, 3))
        n = (n[:-2, :-2] + n[1:-1, 1:-1] * 2 + n[2:, 2:]) / 2
        img = base[..., None] + n
    elif kind == "rgbnoise":
        img = base[..., None] * np.array([1.0, 0.8, 0.6]) + rng.normal(0, 1, (H, W, 3)) * np.array([sig, sig * 0.7, sig * 1.6])
    elif kind == "clip":
        img = base[..., None] * 1.4 + rng.normal(0, sig, (H, W, 3))
        img[: H // 3, : W // 2] = 255.0
    elif kind == "const":
        img = base[..., None] + rng.normal(0, sig, (H, W, 3))
        img[H // 2:, W // 2:] = 128.0
    elif kind == "special":
        img = (base[..., None] + rng.normal(0, sig, (H, W, 3))).astype(F32)
        u = img.view(U32)
        m = rng.random((H, W, 3)) < 0.0005
        u[m] = rng.choice(np.array(NANS + (0x7f800000, 0xff800000), U32), m.sum())
        img[10:42, 70:102, 1] = np.inf
        return img, None
    elif kind == "steps":
        img = (np.floor(x / 37) * 23 + 30)[..., None] + rng.normal(0, sig, (H, W, 3))
    else:
        raise ValueError(kind)
    img = np.clip(img, 0, 255).astype(F32) if kind != "special" else img
    img2 = None
    if len(spec) > 5 and spec[5] == "image2":
        img2 = np.clip(img - rng.normal(0, sig, (H, W, 3)) * rng.uniform(0.6, 1.2), 0, 255).astype(F32)
    return img, img2


# ------------------------------------------------------------------------------------ Python side
def _pyimports():
    import nv_aft
    import nv_curve as ncv
    import nv_flat
    import nv_profile as npr
    return nv_aft, ncv, nv_flat, npr


def profile_for(rgb, area_kind):
    """(area (x, y, w, h), profile bytes) with a cache keyed by the image hash and the source hash."""
    nv_aft, ncv, nv_flat, npr = _pyimports()
    h = hashlib.sha256(np.ascontiguousarray(rgb).tobytes()).hexdigest()[:16]
    key = hashlib.sha256(f"{h}|{area_kind}|{SRC_HASH}".encode()).hexdigest()[:16]
    p = os.path.join(TMP, f"profile_{key}.pkl")
    if os.path.exists(p):
        with open(p, "rb") as f:
            return pickle.load(f)
    H, W = rgb.shape[:2]
    if area_kind == "ffa":
        cur = ncv.genuine_noise_curve(rgb)
        x, y, w, hh, st = nv_flat.find_flat_area_genuine(rgb, ncv.noise_at(cur))
        if w < 32 or hh < 32:
            x, y, w, hh = 0, 0, min(W, 128), min(H, 128)
        area = (x, y, w, hh)
    else:                                                    # (x, y, w, h) given in the kind string
        area = tuple(int(v) for v in area_kind.split(":")[1].split(","))
    raw = npr.build_profile(rgb, 1, area)
    raw = raw[0] if isinstance(raw, tuple) else raw
    out = (area, bytes(raw))
    with open(p + ".part", "wb") as f:
        pickle.dump(out, f)
    os.replace(p + ".part", p)
    return out


def case_inputs(spec):
    """(rgb, image2, area_kind) of an aft case spec."""
    if spec[0] == "frame":
        _, video, n, area_kind = spec[:4]
        return frame(video, n), None, area_kind
    img, img2 = synth(spec[1])
    return img, img2, spec[2]


def py_aft(rgb, prof, mode, image2, cancel_at):
    nv_aft = _pyimports()[0]
    cap = {}
    orig = nv_aft._run_tiles

    def wrapped(*a, **k):
        r = orig(*a, **k)
        cap["bins"] = r
        return r

    calls = [0]

    def cancel():
        calls[0] += 1
        return calls[0] >= cancel_at

    nv_aft._run_tiles = wrapped
    try:
        diag = {}
        r = nv_aft.auto_fine_tune(rgb, prof, mode=mode, image2=image2,
                                  cancel=cancel if cancel_at >= 1 else None, diag=diag)
    finally:
        nv_aft._run_tiles = orig
    out3 = None
    if isinstance(r, tuple):
        r, out3 = r
        out3 = [int(fbits(np.array([v], F32))[0]) for v in out3]
    b = cap["bins"]
    status = 1 if b is False else 2 if b is None else 0
    bins = None
    if status == 0:
        bins = [[fbits(np.asarray(b[c][k], F32)).tobytes() for k in range(9)] for c in range(3)]
    return dict(status=status, prof=bytes(r), out3=out3, diag=[int(diag.get(k, 0)) for k in DIAG_KEYS], bins=bins)


def aft_expected(task):
    """Worker: (spec, mode, cancel_at) -> expected dict (cached)."""
    spec, mode, cancel_at = task
    rgb, img2, area_kind = case_inputs(spec)
    h = hashlib.sha256(np.ascontiguousarray(rgb).tobytes())
    if img2 is not None:
        h.update(np.ascontiguousarray(img2).tobytes())
    key = hashlib.sha256(f"{h.hexdigest()}|{area_kind}|{mode}|{cancel_at}|{SRC_HASH}".encode()).hexdigest()[:16]
    p = os.path.join(TMP, f"aft_{key}.pkl")
    if os.path.exists(p):
        with open(p, "rb") as f:
            return pickle.load(f)
    t0 = time.time()
    area, prof = profile_for(rgb, area_kind)
    t1 = time.time()
    exp = py_aft(rgb, prof, mode, img2, cancel_at)
    exp.update(area=area, prof_in=prof, t_profile=t1 - t0, t_aft=time.time() - t1)
    with open(p + ".part", "wb") as f:
        pickle.dump(exp, f)
    os.replace(p + ".part", p)
    return exp


def aft_cases(quick, real):
    cases = []
    if real:
        for n in (312, 100, 576):
            cases.append((("frame", "test_v02.mp4", n, "ffa"), 0, 0))
        if not quick:
            cases.append((("frame", "test_v02.mp4", 312, "ffa"), 1, 0))
            cases.append((("frame", "test_v02.mp4", 312, "ffa"), 2, 0))
            cases.append((("frame", "test_v02.mp4", 312, "area:832,0,128,128"), 0, 0))
        for n in (0, 300, 750):
            for ak in ("ffa", "area:256,96,128,128"):
                for mode in ((0,) if quick else (0, 1, 2)):
                    cases.append((("frame", "test.mp4", n, ak), mode, 0))
    syn = [(("white", 11, 256, 192), "area:64,32,96,96"),
           (("white", 12, 250, 190), "area:0,0,128,128"),
           (("coloured", 13, 320, 224), "area:40,40,128,128"),
           (("rgbnoise", 14, 288, 160), "area:20,10,128,128"),
           (("clip", 15, 256, 256), "area:128,128,96,96"),
           (("const", 16, 256, 224), "area:0,0,96,96"),
           (("special", 17, 224, 160), "area:100,90,64,64"),
           (("steps", 18, 300, 200), "area:10,10,64,64"),
           (("white", 19, 48, 48), "area:0,0,48,48"),
           (("white", 20, 47, 120), "area:0,0,40,64"),
           (("white", 21, 512, 96), "area:0,0,96,96"),
           (("white", 22, 256, 192, 0, "image2"), "area:64,32,96,96"),
           (("coloured", 23, 256, 192, 0, "image2"), "area:16,16,96,96")]
    for s, ak in syn:
        for mode in (0, 1, 2):
            cases.append((("syn", s, ak), mode, 0))
    for c in (1, 7, 48, 49):
        cases.append((("syn", ("white", 11, 256, 192), "area:64,32,96,96"), 0, c))
    return cases


def write_aft_input(path, items):
    """items: (rgb, image2, prof bytes, mode, cancel_at)."""
    with open(path, "wb") as f:
        f.write(struct.pack("<Q", len(items)))
        for rgb, img2, prof, mode, cancel_at in items:
            H, W = rgb.shape[:2]
            f.write(struct.pack("<IIIIq", W, H, mode, 1 if img2 is not None else 0, cancel_at))
            f.write(prof)
            f.write(np.ascontiguousarray(rgb, F32).tobytes())
            if img2 is not None:
                f.write(np.ascontiguousarray(img2, F32).tobytes())


def read_aft_output(path, n):
    out = []
    with open(path, "rb") as f:
        data = f.read()
    o = 0
    for _ in range(n):
        st = struct.unpack_from("<I", data, o)[0]
        o += 4
        prof = data[o:o + PROF]
        o += PROF
        out3 = list(struct.unpack_from("<3I", data, o))
        o += 12
        diag = list(struct.unpack_from("<7i", data, o))
        o += 28
        bins = [[None] * 9 for _ in range(3)]
        for c in range(3):
            for k in range(9):
                m = struct.unpack_from("<I", data, o)[0]
                o += 4
                bins[c][k] = data[o:o + 4 * m]
                o += 4 * m
        out.append(dict(status=st, prof=prof, out3=out3, diag=diag, bins=bins))
    return out


def run_exe(exe, *args):
    r = subprocess.run([exe] + list(args), capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError(f"{exe} {args[0]} failed: {r.stderr.strip()}")
    return r.stderr.strip()


def compare_aft(exp, got):
    errs = []
    if exp["status"] != got["status"]:
        errs.append(f"status {exp['status']} != {got['status']}")
        return errs
    if exp["prof"] != got["prof"]:
        a = np.frombuffer(exp["prof"], np.uint8)
        b = np.frombuffer(got["prof"], np.uint8)
        d = np.nonzero(a != b)[0]
        errs.append(f"profile: {d.size} bytes differ (first 0x{d[0]:x})")
    if exp["out3"] is not None and exp["out3"] != got["out3"]:
        errs.append(f"out3 {exp['out3']} != {got['out3']}")
    if exp["diag"] != got["diag"]:
        errs.append(f"diag {exp['diag']} != {got['diag']}")
    if exp["bins"] is not None:
        for c in range(3):
            for k in range(9):
                if exp["bins"][c][k] != got["bins"][c][k]:
                    errs.append(f"bins[{c}][{k}] differ (n {len(exp['bins'][c][k]) // 4} / {len(got['bins'][c][k]) // 4})")
    return errs


# ------------------------------------------------------------------------------------ sim / finish
def gen_sim(rng, n):
    cases = []
    for i in range(n):
        p = np.exp(rng.normal(0, 1.5, 2005)).astype(F32)
        spread = rng.choice([0.01, 0.05, 0.15, 0.4, 1.0])
        t = np.zeros(4096, F32)
        t[:2005] = (p * np.exp(rng.normal(0, spread, 2005))).astype(F32)
        t[2005:] = rng.normal(0, 1, 4096 - 2005).astype(F32)
        stats = np.exp(rng.normal(-1.5, 0.8, 51)).astype(F32)
        mode = int(rng.integers(0, 3))
        s_lvl = F32((3.0, 2.0, 3.0)[mode])
        s_mult = F32((4.0, 3.0, 4.0)[mode])
        kind = i % 10
        if kind == 1:                                   # CV chain ends (negative words)
            for l in (0, 1):
                m = int(rng.integers(0, 7))
                (t if rng.random() < 0.5 else p)[16 + 8 * l + m] = -0.5
        elif kind == 2:                                 # zero tolerances / zero words
            stats[rng.integers(0, 51, 6)] = 0.0
            t[rng.choice([5, 6, 240, 250, 490, 491, 16, 17])] = 0.0
            p[rng.choice([5, 6, 241, 256, 490, 0, 1])] = 0.0
        elif kind == 3:                                 # NaN / inf words (several payloads)
            for arr, lim in ((t, 500), (p, 500), (stats, 51)):
                u = arr.view(U32)
                idx = rng.integers(0, lim, int(rng.integers(1, 4)))
                u[idx] = rng.choice(np.array(NANS + (0x7f800000, 0xff800000), U32), idx.size)
        elif kind == 4:                                 # zero / NaN / inf s_lvl and s_mult
            s_lvl = F32(rng.choice([0.0, -0.0, np.inf, np.nan, 1e-30, 3.0]))
            s_mult = F32(rng.choice([0.0, np.inf, np.nan, 4.0, -1.0]))
        elif kind == 5:                                 # negative / signed values
            t[rng.integers(0, 500, 20)] *= -1
            p[rng.integers(0, 500, 20)] *= -1
        elif kind == 6:                                 # identical models (all penalties 1)
            t[:2005] = p
        elif kind == 7:                                 # subnormals / huge
            t[rng.integers(0, 500, 8)] = rng.choice([1e-42, 3e38, -1e-44, 1e30])
            p[rng.integers(0, 500, 8)] = rng.choice([1e-42, 3e38, 1e-40])
        cases.append((t, p, stats, s_lvl, s_mult))
    return cases


def py_sim(cases):
    nv_aft = _pyimports()[0]
    return [int(fbits(np.array([nv_aft.similarity(t, p, s, a, b)], F32))[0]) for t, p, s, a, b in cases]


def write_sim(path, cases):
    with open(path, "wb") as f:
        f.write(struct.pack("<Q", len(cases)))
        for t, p, s, a, b in cases:
            f.write(np.asarray(t, F32).tobytes())
            f.write(np.asarray(p, F32).tobytes())
            f.write(np.asarray(s, F32).tobytes())
            f.write(np.array([a, b], F32).tobytes())


def gen_finish(rng, n, real_profiles):
    cases = []
    for i in range(n):
        mode = int(rng.integers(0, 3))
        if real_profiles and rng.random() < 0.3:
            buf = bytearray(real_profiles[int(rng.integers(0, len(real_profiles)))])
        else:
            buf = bytearray(PROF)
            a = np.frombuffer(buf, np.uint8)
            f = a.view(F32)
            ii = a.view(np.int32)
            state = i % 4
            for c in range(3):
                for k in range(9):
                    wb = (CURVE[c] >> 2) + 8 * k
                    o = CURVE[c] + 0x20 * k
                    r = rng.random()
                    if state == 0:              # fresh profile: one filled bin
                        f[wb] = 1.0 if k == 4 else 0.0
                        ii[wb + 6] = 1 if k == 4 else 0
                        continue
                    f[wb] = F32(rng.choice([0.0, np.exp(rng.normal(0, 0.5)), 3.5, 0.1, 5.0])) if r < 0.97 else F32(np.nan)
                    f[wb + 1:wb + 6] = np.exp(rng.normal(0, 0.2, 5)).astype(F32)
                    ii[wb + 6] = int(rng.choice([-1, 0, 0, 1, 1, 1, 2]))
                    for b_ in (0x1c, 0x1d, 0x1e, 0x1f):
                        a[o + b_] = int(rng.choice([0, 0, 0, 1, 2])) if state >= 2 else 0
        bins = []
        for c in range(3):
            row = []
            for k in range(9):
                ln = int(rng.choice([0, 0, 1, 3, 4, 5, 7, 8, 9, 12, 20, 40]))
                v = np.exp(rng.normal(rng.normal(0, 0.4), rng.uniform(0.02, 0.5), ln)).astype(F32)
                if ln and rng.random() < 0.05:
                    v[int(rng.integers(0, ln))] = F32(rng.choice([np.nan, np.inf, 0.0, 1e30]))
                if ln and rng.random() < 0.05:
                    v[:] = F32(rng.choice([1.0, 3.0, 2.9999998]))
                row.append(v)
            bins.append(row)
        cases.append((mode, bytes(buf), bins))
    return cases


def py_finish(cases):
    nv_aft = _pyimports()[0]
    out = []
    for mode, buf, bins in cases:
        b = bytearray(buf)
        P = nv_aft.Prof(b)
        lists = [[[F32(x) for x in bins[c][k]] for k in range(9)] for c in range(3)]
        with np.errstate(all="ignore"):
            r = nv_aft._finish(P, lists, mode)
        out3 = [int(fbits(np.array([v], F32))[0]) for v in r] if r is not None else None
        out.append((bytes(b), out3))
    return out


def write_finish(path, cases):
    with open(path, "wb") as f:
        f.write(struct.pack("<Q", len(cases)))
        for mode, buf, bins in cases:
            f.write(struct.pack("<I", mode))
            f.write(buf)
            for c in range(3):
                for k in range(9):
                    f.write(struct.pack("<I", len(bins[c][k])))
                    f.write(np.asarray(bins[c][k], F32).tobytes())


def read_finish(path, n):
    with open(path, "rb") as f:
        data = f.read()
    out = []
    o = 0
    for _ in range(n):
        out.append((data[o:o + PROF], list(struct.unpack_from("<3I", data, o + PROF))))
        o += PROF + 12
    return out


# ------------------------------------------------------------------------------------ combine
def py_combine(raw, aft, centers, curves, lvl, tot, band):
    import nv_aft
    import nv_aft_v5
    import nv_profile as npr
    prof = {"area": [0, 0, 128], "curve_centers": list(centers),
            "curve": {n: [float(v) for v in curves[i]] for i, n in enumerate(("Y", "Cb", "Cr"))},
            "area_level": lvl, "tot_ratio": {n: tot[i] for i, n in enumerate(("Y", "Cb", "Cr"))}}
    o_bp, o_aft = npr.build_profile, nv_aft.auto_fine_tune
    npr.build_profile = lambda *a, **k: raw
    nv_aft.auto_fine_tune = lambda *a, **k: aft
    try:
        with np.errstate(all="ignore"):
            r = nv_aft_v5.apply_aft(prof, np.zeros((1, 1, 3), F32), mode=0, band=band)
    finally:
        npr.build_profile, nv_aft.auto_fine_tune = o_bp, o_aft
    f = r["aft_bin_flags"]
    return dict(cw=np.array(r["aft_color_weights"], F64).ravel().view(np.uint64).tolist(),
                face=[np.array(r["aft_face_curves"][j], F64).view(np.uint64).tolist() for j in range(3)],
                curve=[fbits(np.array(r["curve"][n], F32)).tolist() for n in ("Y", "Cb", "Cr")],
                sigma=np.array([r["sigma"][n] for n in ("Y", "Cb", "Cr")], F64).view(np.uint64).tolist(),
                flags=[[int(v) for row in f[key] for v in row] for key in ("marker", "clip", "gt3")])


def gen_combine(rng, pairs):
    """pairs: (raw, aft) profile bytes; v5 profile values are random (the combination is pure arithmetic)."""
    cases = []
    centers = [256.0 / 8 * (i + 0.5) for i in range(8)]
    for i, (raw, aft) in enumerate(pairs):
        for rep in range(3):
            curves = [np.exp(rng.normal(1.0, 0.3, 8)).astype(F32) for _ in range(3)]
            lvl = float(rng.choice([128.0, float(rng.uniform(0, 260)), 16.0, 240.0, 300.0, -5.0]))
            tot = [float(rng.uniform(0.5, 3)) for _ in range(3)]
            band = int(rng.integers(0, 5)) if rep == 2 else 0
            raw_ = raw
            if rep == 1 and i % 4 == 3:                   # zero q99 (denominator 0 -> 1/3 weights)
                b = bytearray(raw)
                for off in (0x4c, 0x1fa0, 0x3ef4):
                    struct.pack_into("<f", b, off + 4 * (5 + band), 0.0)
                raw_ = bytes(b)
            cases.append((raw_, aft, centers, curves, lvl, tot, band))
    return cases


def write_combine(path, cases):
    with open(path, "wb") as f:
        f.write(struct.pack("<Q", len(cases)))
        for raw, aft, centers, curves, lvl, tot, band in cases:
            f.write(raw)
            f.write(aft)
            f.write(struct.pack("<i", len(centers)))
            f.write(np.asarray(centers, F64).tobytes())
            for c in curves:
                f.write(np.asarray(c, F32).tobytes())
            f.write(struct.pack("<d", lvl))
            f.write(np.asarray(tot, F64).tobytes())
            f.write(struct.pack("<i", band))


def read_combine(path, cases):
    with open(path, "rb") as f:
        data = f.read()
    out = []
    o = 0
    for c in cases:
        n = len(c[2])
        cw = list(struct.unpack_from("<9Q", data, o)); o += 72
        face = []
        for _ in range(3):
            face.append(list(struct.unpack_from(f"<{n}Q", data, o))); o += 8 * n
        curve = []
        for _ in range(3):
            curve.append(list(struct.unpack_from(f"<{n}I", data, o))); o += 4 * n
        sigma = list(struct.unpack_from("<3Q", data, o)); o += 24
        flags = []
        for _ in range(3):
            flags.append(list(data[o:o + 27])); o += 27
        out.append(dict(cw=cw, face=face, curve=curve, sigma=sigma, flags=flags))
    return out


# ------------------------------------------------------------------------------------ main
def check_shuffle(exe):
    import nv_aft
    nmax = 4096
    path = os.path.join(TMP, "shuffle.out")
    run_exe(exe, "shuffle", str(nmax), path)
    got = np.fromfile(path, np.int32)
    o = 0
    bad = 0
    for n in range(nmax + 1):
        exp = nv_aft.shuffle_order(n)
        if got[o:o + n].tolist() != exp:
            bad += 1
        o += n
    if o != got.size:
        bad += 1
    return nmax + 1 - bad, nmax + 1


def check_sim(exe, cases, exp):
    path_in, path_out = os.path.join(TMP, "sim.in"), os.path.join(TMP, "sim.out")
    write_sim(path_in, cases)
    run_exe(exe, "sim", path_in, path_out)
    got = np.fromfile(path_out, U32).tolist()
    return sum(int(a == b) for a, b in zip(exp, got)), len(cases)


def check_finish(exe, cases, exp):
    path_in, path_out = os.path.join(TMP, "finish.in"), os.path.join(TMP, "finish.out")
    write_finish(path_in, cases)
    run_exe(exe, "finish", path_in, path_out)
    got = read_finish(path_out, len(cases))
    ok = 0
    for (mode, _, _), (eb, e3), (gb, g3) in zip(cases, exp, got):
        ok += int(eb == gb and (e3 is None or e3 == g3))
    return ok, len(cases)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--no-real", action="store_true")
    ap.add_argument("--jobs", type=int, default=3)
    ap.add_argument("--mutants", action="store_true")
    ap.add_argument("--exe", default=EXE)
    ap.add_argument("--sim", type=int, default=3000)
    ap.add_argument("--finish", type=int, default=3000)
    args = ap.parse_args()
    os.makedirs(TMP, exist_ok=True)
    if not os.path.exists(args.exe):
        print(f"missing {args.exe}: make -C native -f Makefile.aft")
        return 2
    nsim, nfin = (500, 500) if args.quick else (args.sim, args.finish)
    t0 = time.time()
    all_ok = True

    # aft cases: expected values (cached; computed in parallel)
    tasks = aft_cases(args.quick, not args.no_real)
    todo = [t for t in tasks]
    with Pool(max(1, min(args.jobs, 3))) as pool:
        exps = pool.map(aft_expected, todo, chunksize=1)
    print(f"aft expected values: {len(exps)} cases ({time.time() - t0:.1f} s)")
    items = []
    for (spec, mode, cancel_at), e in zip(tasks, exps):
        rgb, img2, _ = case_inputs(spec)
        items.append((rgb, img2, e["prof_in"], mode, cancel_at))
    path_in = os.path.join(TMP, "aft.in")
    write_aft_input(path_in, items)

    rng = np.random.default_rng(20261027)
    sim_cases = gen_sim(rng, nsim)
    sim_exp = py_sim(sim_cases)
    real_profiles = [e["prof_in"] for e in exps]
    fin_cases = gen_finish(rng, nfin, real_profiles)
    fin_exp = py_finish(fin_cases)
    pairs = [(e["prof_in"], e["prof"]) for (spec, mode, c), e in zip(tasks, exps) if mode == 0 and c == 0]
    comb_cases = gen_combine(rng, pairs)
    comb_exp = [py_combine(*c) for c in comb_cases]

    def run_all(exe, label, verbose):
        res = {}
        res["shuffle"] = check_shuffle(exe)
        res["sim"] = check_sim(exe, sim_cases, sim_exp)
        res["finish"] = check_finish(exe, fin_cases, fin_exp)
        for j in ((1, args.jobs) if args.jobs > 1 else (1,)):
            path_out = os.path.join(TMP, f"aft_j{j}.out")
            msg = run_exe(exe, "aft", path_in, path_out, "-j", str(j))
            got = read_aft_output(path_out, len(items))
            ok = 0
            for (spec, mode, c), e, g in zip(tasks, exps, got):
                errs = compare_aft(e, g)
                ok += int(not errs)
                if errs and verbose:
                    print(f"  MISMATCH aft -j {j} {spec} mode {mode} cancel {c}: {'; '.join(errs[:4])}")
            res[f"aft -j {j}"] = (ok, len(items))
            if verbose:
                print(f"  {msg}")
        cin, cout = os.path.join(TMP, "combine.in"), os.path.join(TMP, "combine.out")
        write_combine(cin, comb_cases)
        run_exe(exe, "combine", cin, cout)
        got = read_combine(cout, comb_cases)
        ok = 0
        for e, g in zip(comb_exp, got):
            same = e["cw"] == g["cw"] and e["face"] == g["face"] and e["curve"] == g["curve"] \
                and e["sigma"] == g["sigma"] and e["flags"] == g["flags"]
            ok += int(same)
            if not same and verbose:
                diff = [k for k in ("cw", "face", "curve", "sigma", "flags") if e[k] != g[k]]
                print(f"  MISMATCH combine: {diff}")
        res["combine"] = (ok, len(comb_cases))
        return res

    res = run_all(args.exe, "port", True)
    for k, (ok, n) in res.items():
        flag = "OK" if ok == n else "FAIL"
        all_ok &= ok == n
        print(f"{k:10s} {ok}/{n} {flag}")
    acc = [e["diag"][6] for e in exps]
    print(f"aft cases: {len(exps)} (accepted tiles per case: min {min(acc)}, max {max(acc)}, "
          f"cases with accepted > 0: {sum(a > 0 for a in acc)}; statuses "
          f"{ {s: sum(e['status'] == s for e in exps) for s in (0, 1, 2)} })")
    tp = [e["t_aft"] for e in exps if e.get("t_aft")]
    if tp:
        print(f"Python auto_fine_tune time (when computed): total {sum(tp):.1f} s")

    if args.mutants:
        print("negative controls (mutants must mismatch):")
        for m in MUTANTS:
            exe = os.path.join(TMP, f"nv_aft_mut_{m}")
            subprocess.run(["make", "-s", "-C", HERE, "-f", "Makefile.aft", "-B", f"MUTFLAGS=-D{m}",
                            "nv_aft_test"], check=True, capture_output=True)
            os.replace(EXE, exe)
            r = run_all(exe, m, False)
            bad = {k: n - ok for k, (ok, n) in r.items() if ok != n}
            detected = bool(bad)
            all_ok &= detected
            print(f"  {m:16s} {'DETECTED' if detected else 'NOT DETECTED'} {bad}")
        subprocess.run(["make", "-s", "-C", HERE, "-f", "Makefile.aft", "-B", "nv_aft_test"], check=True,
                       capture_output=True)
    print(f"total {time.time() - t0:.1f} s -> {'ALL BIT-EXACT' if all_ok else 'FAILURES'}")
    return 0 if all_ok else 1


if __name__ == "__main__":
    sys.exit(main())
