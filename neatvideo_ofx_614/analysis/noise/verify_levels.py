#!/usr/bin/env python3
"""Bit-exact check of repro/nv_band.py against the genuine routines run through harness/nvh:
  levels  : FUN_009980e8 (FindFlatArea band measure, border 2) - all 4096 words of the dumped struct
  model   : FUN_00995250 (BuildNoiseModel per-plane model, border 4) - the level part (words 0..54)
            must be bit-identical; the directional words (not in nv_band.model_levels; all 4096 words: repro/nv_model.py, model_verify/) are counted and reported
  quantile: FUN_00997bcc itself (`nvh quantile`): q >= 1 (FUN_009efdd4 maximum), small samples, other
            q / flag / div values, NaN / inf / -0 / tied values - the returned float32 bit pattern
  helpers : copies of helpers of other modules agree with their originals (nv_band._round_or_flag vs
            nv_pixnoise._round_or_flag, nv_flat._fcvtzs vs nv_curve._fcvtzs1; no genuine code)
The struct is read from the bit-pattern column of `nvh levels|model`, so -0.0 and NaN payloads are
compared too.  Extra information per case: the max relative errors of mean / q99 / median / CV (the
tolerance numbers of the previous verification, 0 for an exact port) and, for comparison, those of
the legacy float64 port that nvfilter.band_model used before 2026-10-16 (legacy_band_model below).

usage: verify_levels.py [--quick] [--no-model] [--no-quantile] [--jobs N] [--list]
A case passes when every compared word is identical; inputs the genuine rejects (assertion, exit
code 3 of the guard) are listed as such and not counted.  `model` skips planes wider or higher than
512 (FUN_00995250's line-kernel tables) and planes with a single row or column (the genuine ends with
SIGBUS on some of them when several nvh run in parallel - observed by the 2026-10-16 review; BuildNoiseModel
only gets crops of >= 48 px)."""
import argparse
import os
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ThreadPoolExecutor

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPRO = os.path.join(HERE, "..", "..", "..", "repro")
sys.path.insert(0, REPRO)
sys.path.insert(0, HERE)
import nv_band as nb  # noqa: E402
import nv_flat as nf  # noqa: E402
from nvh_common import NVH, require_guard  # noqa: E402

F32 = np.float32
LEVEL_WORDS = 55            # mean[5], q99[5], q50[5], cv[5][8]


class Rejected(Exception):
    pass


def genuine_words(mode, plane):
    """`nvh levels|model W H plane` -> uint32[4096] (bit-pattern column); Rejected if the genuine asserts."""
    with tempfile.NamedTemporaryFile(suffix=".f32", delete=False) as f:
        np.ascontiguousarray(plane, F32).tofile(f)
        name = f.name
    try:
        r = subprocess.run([NVH, mode, str(plane.shape[1]), str(plane.shape[0]), name], capture_output=True,
                           text=True, timeout=900)
    finally:
        os.unlink(name)
    if r.returncode == 3:
        raise Rejected(r.stderr.strip().splitlines()[-1][:160] if r.stderr.strip() else "exit 3")
    if r.returncode:
        raise RuntimeError(f"nvh {mode} exit {r.returncode}: {r.stderr[-300:]}")
    w = np.array([int(l.split()[2], 16) for l in r.stdout.splitlines() if l.strip()], np.uint32)
    if w.size != nb.STRUCT_WORDS:
        raise RuntimeError(f"nvh {mode}: {w.size} words")
    return w


def genuine_quantile(v, q, flag, div):
    """`nvh quantile n v qbits flag div` -> (result bits, out bits); Rejected if the genuine asserts."""
    v = np.ascontiguousarray(v, F32).ravel()
    with tempfile.NamedTemporaryFile(suffix=".f32", delete=False) as f:
        v.tofile(f)
        name = f.name
    try:
        r = subprocess.run([NVH, "quantile", str(v.size), name, f"{int(np.float32(q).view(np.uint32)):08x}",
                            str(flag), str(div)], capture_output=True, text=True, timeout=300)
    finally:
        os.unlink(name)
    if r.returncode == 3:
        raise Rejected(r.stderr.strip().splitlines()[-1][:160] if r.stderr.strip() else "exit 3")
    if r.returncode:
        raise RuntimeError(f"nvh quantile exit {r.returncode}: {r.stderr[-300:]}")
    t = r.stdout.split()
    return int(t[1], 16), int(t[2], 16)


# ------------------------------------------------------------------------------------------------
# legacy port (nvfilter.band_model before the exact port; float64 sums, cumulative-sum boxes), kept
# only to show how far it was from the genuine
def _box1d(x, r, axis, zero=False):
    if r <= 0:
        return np.asarray(x, F32)
    pad = [(0, 0), (0, 0)]
    pad[axis] = (r, r)
    xp = np.pad(np.asarray(x, F32), pad, mode="constant" if zero else "edge")
    n = x.shape[axis]
    if r == 1:
        s = (np.take(xp, range(0, n), axis) + np.take(xp, range(1, n + 1), axis) + np.take(xp, range(2, n + 2), axis))
        return (s / F32(3)).astype(F32)
    c = np.cumsum(xp, axis=axis, dtype=np.float64)
    c = np.concatenate([np.zeros_like(np.take(c, [0], axis)), c], axis=axis)
    return ((np.take(c, range(2 * r + 1, 2 * r + 1 + n), axis) - np.take(c, range(0, n), axis)) / (2 * r + 1)).astype(F32)


def _box2d(x, r, zero=False):
    return _box1d(_box1d(x, r, 1, zero), r, 0, zero)


def _gdown2(x):
    h, w = x.shape[0] // 2, x.shape[1] // 2
    x = x[:2 * h, :2 * w]
    return ((x[0::2, 0::2] + x[0::2, 1::2] + x[1::2, 0::2] + x[1::2, 1::2]) * F32(0.25)).astype(F32)


def _gup2(lo, shape):
    def axis_up(a, ax, n):
        m = a.shape[ax]
        prev = np.take(a, [max(i - 1, 0) for i in range(m)], ax)
        nxt = np.take(a, [min(i + 1, m - 1) for i in range(m)], ax)
        out = np.stack([(F32(3) * a + prev) * F32(0.25), (F32(3) * a + nxt) * F32(0.25)], axis=ax + 1)
        shp = list(a.shape)
        shp[ax] = 2 * m
        out = out.reshape(shp)
        if n > 2 * m:
            out = np.concatenate([out, np.take(out, [2 * m - 1], ax)], axis=ax)
        return out.astype(F32)
    return axis_up(axis_up(lo, 0, shape[0]), 1, shape[1])


_GQ_SMALL = (0.0, 0.0, 1.691, 1.242, 1.132, 1.082, 1.054, 1.036, 1.024, 1.015, 1.008, 1.0)


def _legacy_probit(q):
    q = min(max(float(np.float32(q)), 0.0), 1.0)
    x = np.float32(np.float32(q) * np.float32(100.0))
    i = min(int(x), 100)
    f = np.float32(x - np.float32(i))
    hi = nb.PROBIT[min(i + 1, 100)]
    return float(np.float32((np.float32(1.0) - f) * nb.PROBIT[i] + f * hi))


def legacy_gquantile(v, q, div=4, small=True):
    """The pre-2026-10-16 nvfilter.gquantile (float64 mean / std, sorted counts)."""
    v = np.asarray(v, np.float32).ravel()
    n = v.size
    q32 = np.float32(q)
    if q32 >= 1.0:
        return float(v.max())
    m = np.float32(np.sum(v, dtype=np.float64) / n)
    sd = np.float32(np.sqrt(np.sum((v.astype(np.float64) - np.float64(m)) ** 2) / n))
    if sd == 0.0:
        return 0.0
    vs = np.sort(v)
    count = lambda t: int(np.searchsorted(vs, np.float32(t), side="right"))
    target = np.float32(np.float32(n) * q32)
    t, step, evals = m, sd, 0
    for _ in range(8):
        if step == 0.0:
            break
        while evals <= 99:
            evals += 1
            t = np.float32(t + step)
            c = count(t)
            if (step > 0 and not c < target) or (step < 0 and not target < c):
                break
        else:
            break
        step = np.float32(step * np.float32(-0.5))
    res = t
    if n <= 49:
        above, below = vs[vs >= t], vs[vs < t]
        if above.size == 0:
            res = below[-1]
        elif below.size == 0:
            res = above[0]
        else:
            hi, lo = above[0], below[-1]
            k = int(np.count_nonzero((vs > lo) & (vs <= hi)))
            slope = np.float32((hi - lo) / np.float32(k))
            res = np.float32(slope * np.float32(n) * q32 + (hi - slope * np.float32(count(hi))))
    d = n // div if div else 0
    lim = max(int(np.float32(1.0) / (np.float32(1.0) - q32)), 10)
    g = np.float32(np.float32(np.float32(_legacy_probit(q32)) * sd) * np.float32(_GQ_SMALL[min(d, 11)]) + m)
    if small and d < lim and res < g:
        res = g
    return float(res)


def legacy_band_model(plane, border=2):
    """The pre-2026-10-16 nvfilter.band_model (tolerance-level agreement only)."""
    src = np.asarray(plane, F32)
    mean, q99, q50, cv = [], [], [], []
    for k in range(5):
        h, w = src.shape
        lo = _gdown2(_box2d(src, 1)) if min(h, w) >= 2 else src
        band = (src - _gup2(lo, src.shape)).astype(F32) if min(h, w) >= 2 else src * 0
        e = np.sqrt(np.abs(_box2d((band * band).astype(F32), 1, zero=True))).astype(F32)
        bx = border if w > 2 * border else max((w - 1) // 2, 0)
        by = border if h > 2 * border else max((h - 1) // 2, 0)
        small = w < 2 * border + 1 or h < 2 * border + 1
        while (w - 2 * bx) * (h - 2 * by) < 32 and (bx or by):
            bx, by = max(bx - 1, 0), max(by - 1, 0)
            small = True
        if h < 3 or w < 3 or small:
            ex = (lambda a: -1e6 if a[-1] == -1e6 else -0.5 * abs(a[-1])) if k else (lambda a: -1e6)
            mean.append(ex(mean)), q99.append(ex(q99)), q50.append(ex(q50))
            cv.append(cv[-1] if cv else np.full(8, -1e6))
        else:
            v = e[by:h - by, bx:w - bx]
            mean.append(max(float(np.sum(v, dtype=np.float64) / v.size), 1e-6))
            q99.append(max(legacy_gquantile(v, 0.99), 1e-6))
            q50.append(max(legacy_gquantile(v, 0.5), 1e-6))
            out = np.full(8, -1e6)
            m = float(np.mean(v, dtype=np.float64))
            ee = v
            for j in range(8):
                if ee.size < 4 or m == 0.0:
                    break
                out[j] = np.sqrt(float(np.mean((ee.astype(np.float64) - m) ** 2))) / m
                ee = _gdown2(ee)
            cv.append(out)
        src = lo
    return {"mean": np.array(mean), "q99": np.array(q99), "q50": np.array(q50), "cv": np.array(cv)}


def rel_errors(g, m):
    """Max relative errors (mean, q99, q50, cv) of the dict m against the genuine words g."""
    G = nb.as_model(g.view(F32))
    out = []
    for key in ("mean", "q99", "q50"):
        a, b = np.asarray(G[key], np.float64), np.asarray(m[key], np.float64)
        ok = np.isfinite(a) & np.isfinite(b) & (a != 0)
        out.append(float(np.max(np.abs(a[ok] - b[ok]) / np.abs(a[ok]))) if ok.any() else 0.0)
    a, b = np.asarray(G["cv"], np.float64), np.asarray(m["cv"], np.float64)
    ok = (a > -1e5) & (b > -1e5) & np.isfinite(a) & np.isfinite(b)
    out.append(float(np.max(np.abs(a[ok] - b[ok]) / np.maximum(np.abs(a[ok]), 1e-9))) if ok.any() else 0.0)
    return out


# ------------------------------------------------------------------------------------------------
def frame_planes(quick):
    """Crops of real frames (test_v02 n=312, test.mp4 n=300) as profile planes of modes 0 / 1 / 2."""
    import nvfilter as nv
    from denoise_video_v5 import probe, read_frames
    clips = [("test_v02.mp4", 312), ("test.mp4", 300)]
    out = {}
    for clip, n in clips:
        path = os.path.join(REPRO, "..", clip)
        if not os.path.exists(path):
            continue
        info = probe(path)
        mat = nv.matrix_from_tags(info["tags"], info["H"])
        for i, pl in read_frames(path, info["W"], info["H"], n, n + 1, fps=info["fps"]):
            pl = tuple(np.asarray(p, F32) for p in pl)
            rgb = np.clip(nv.yuv_to_rgb(pl, mat, info["full_range"]), 0, 255).astype(F32)
            H, W = rgb.shape[:2]
            spots = [(0, 0, 128), (H // 2 - 96, W // 2 - 96, 192), (H - 48, W - 48, 48), (17, 33, 97),
                     (5, 9, min(320, H - 5, W - 9))]
            if quick:
                spots = spots[:2]
            for y, x, S in spots:
                crop = np.ascontiguousarray(rgb[y:y + S, x:x + S])
                for mode in (0, 1, 2):
                    planes = nf.transform(crop[..., 0], crop[..., 1], crop[..., 2], mode)
                    names = ("RGB" if mode == 0 else "YCrCb" if mode == 1 else "mean")
                    for p, pl_ in enumerate(planes):
                        if quick and p:
                            continue
                        out[f"{clip[:-4]} n{n} {S}@{x},{y} {names}[{p}]"] = np.asarray(pl_, F32)
    return out


def cases(quick=False):
    rng = np.random.default_rng(20261016)
    c = {}
    # sizes: smallest planes, odd / non-square / thin, up to 512 and beyond
    sizes = [(1, 1), (2, 3), (4, 4), (5, 5), (9, 5), (5, 9), (6, 6), (7, 9), (8, 8), (9, 9), (12, 12), (13, 11),
             (16, 16), (33, 17), (24, 24), (7, 31), (32, 32), (65, 33), (48, 48), (64, 64), (65, 63), (96, 96),
             (37, 100), (129, 127), (128, 128), (193, 191), (192, 192), (256, 256), (17, 300), (300, 17),
             (320, 320), (200, 345), (512, 512), (511, 513), (360, 640), (60, 700), (768, 1024)]
    if quick:
        sizes = [(5, 5), (9, 9), (65, 33), (128, 128), (200, 345), (512, 512)]
    for h, w in sizes:
        c[f"white {w}x{h}"] = rng.normal(100, 5, (h, w))
    c["white 256 (sd 10)"] = rng.normal(128, 10, (256, 256))
    c["ramp+noise 131x97"] = np.add.outer(np.arange(97), np.arange(131)) * 0.7 + rng.normal(0, 3, (97, 131))
    c["coloured 128"] = np.cumsum(rng.normal(0, 1, (128, 128)), 1) * 0.3 + rng.normal(0, 2, (128, 128))
    c["coloured 48"] = np.cumsum(rng.normal(0, 1, (48, 48)), 0) * 0.5 + rng.normal(0, 3, (48, 48))
    # constant / near-constant / DC / scale extremes
    c["const 7 (64)"] = np.full((64, 64), 7.0)
    c["const 0 (96)"] = np.zeros((96, 96))
    c["const 255 (50x70)"] = np.full((50, 70), 255.0)
    one = np.full((64, 64), 40.0)
    one[31, 17] = 41.0
    c["const + one pixel"] = one
    c["const + 1e-3 noise"] = 80.0 + rng.normal(0, 1e-3, (128, 128))
    c["DC 150+-2 (192)"] = rng.normal(150, 2, (192, 192))
    c["DC 1e4+-1 (128)"] = rng.normal(1e4, 1, (128, 128))
    c["DC 1e6+-10 (96)"] = rng.normal(1e6, 10, (96, 96))
    c["tiny 1e-30 (64)"] = rng.normal(0, 1e-30, (64, 64))
    c["subnormal 1e-41 (48)"] = rng.normal(0, 1e-41, (48, 48))
    c["huge 1e30 (64)"] = rng.normal(0, 1e30, (64, 64))
    c["integer 0..255 (100)"] = rng.integers(0, 256, (100, 100)).astype(float)
    c["integer 0..3 (128)"] = rng.integers(0, 4, (128, 128)).astype(float)
    c["binary 0/255 (64)"] = (rng.random((64, 64)) < 0.5) * 255.0
    c["levels invalid 8x200"] = rng.normal(100, 5, (8, 200))
    c["levels invalid 200x8"] = rng.normal(100, 5, (200, 8))
    c["12x400"] = rng.normal(100, 5, (12, 400))
    # level-0 crops of exactly 2047 / 2048 / 2049 / 4096 / 4097 values (2048-value blocks of the mean / std)
    for (h, w) in ((27, 93), (36, 68), (7, 687), (68, 68), (21, 245)):
        c[f"block boundary {w}x{h} (n={(h - 4) * (w - 4)})"] = rng.normal(60, 4, (h, w))
    c["one-column crop 5x36"] = rng.normal(100, 5, (36, 5))
    c["one-row crop 36x5"] = rng.normal(100, 5, (5, 36))
    small = rng.normal(100, 5, (20, 20))
    c["n<=49 at level 2 (20x20)"] = small
    c["n<=49 (11x11)"] = rng.normal(10, 3, (11, 11))
    # non-finite values
    nan1 = rng.normal(100, 5, (64, 64))
    nan1[10, 10] = np.nan
    c["one NaN (64)"] = nan1
    inf1 = rng.normal(100, 5, (64, 64))
    inf1[20, 40] = np.inf
    c["one +inf (64)"] = inf1
    ninf = rng.normal(100, 5, (64, 64))
    ninf[5, 5] = -np.inf
    c["one -inf (64)"] = ninf
    c["all NaN (32)"] = np.full((32, 32), np.nan)
    if quick:
        for k in [k for k in c if k.startswith(("subnormal", "huge", "one ", "all NaN", "integer 0..3", "block"))]:
            del c[k]
    c = {k: np.asarray(v, F32) for k, v in c.items()}
    c.update(frame_planes(quick))
    return c


def check(name, plane, mode):
    """-> (status, text): status True / False / None (genuine rejects)."""
    try:
        g = genuine_words(mode, plane)
    except Rejected as e:
        return None, f"{mode:6s} {name:44s} genuine rejects: {e}", None
    t0 = time.time()
    mine = (nb.band_levels(plane) if mode == "levels" else nb.model_levels(plane)).view(np.uint32)
    dt = time.time() - t0
    n = nb.STRUCT_WORDS if mode == "levels" else LEVEL_WORDS
    bad = np.nonzero(g[:n] != mine[:n])[0]
    rel = rel_errors(g, nb.as_model(mine))
    border = 2 if mode == "levels" else 4
    try:
        with np.errstate(all="ignore"):
            leg = (rel_errors(g, legacy_band_model(plane, border))
                   if min(plane.shape) >= 2 and np.isfinite(plane).all() else None)
    except Exception:                                          # noqa: BLE001 (legacy code, info only)
        leg = None
    txt = (f"{mode:6s} {name:44s} {'EXACT' if bad.size == 0 else 'DIFF ' + str(bad[:8].tolist())}"
           f"  rel.err mean/q99/q50/cv {rel[0]:.0e}/{rel[1]:.0e}/{rel[2]:.0e}/{rel[3]:.0e}")
    if leg is not None:
        txt += f" | legacy {leg[0]:.1e}/{leg[1]:.1e}/{leg[2]:.1e}/{leg[3]:.1e}"
    rest = None
    if mode == "model":
        rest = np.nonzero(g[LEVEL_WORDS:] != mine[LEVEL_WORDS:])[0] + LEVEL_WORDS
        txt += f" | directional words differing (not in nv_band; see nv_model): {rest.size}"
    txt += f" ({dt * 1000:.0f} ms)"
    if bad.size:
        for i in bad[:4]:
            txt += (f"\n      word {i}: genuine {g[i]:08x} ({np.uint32(g[i]).view(F32)!r}) port {mine[i]:08x} "
                    f"({np.uint32(mine[i]).view(F32)!r})")
    return bad.size == 0, txt, (rel, leg, rest)


# ------------------------------------------------------------------------------------------------ quantile
Q_BITS = {"0.99": 0x3f7d70a4, "0.5": 0x3f000000, "1.0": 0x3f800000, "0.01": 0x3c23d70a, "0.25": 0x3e800000,
          "0.75": 0x3f400000, "0.9": 0x3f666666, "0.999": 0x3f7fbe77, "0": 0x00000000, "-0.5": 0xbf000000,
          "1.5": 0x3fc00000, "inf": 0x7f800000, "nan": 0x7fc00000, "1-2^-24": 0x3f7fffff}


def quantile_cases(quick=False):
    """(name, values, q, flag, div): sizes 0 .. 9000 around the n <= 49 and SIMD (n & ~7) boundaries,
    values with ties, constants, outliers, special values; q / flag / div variants."""
    rng = np.random.default_rng(997)
    out = []
    sizes = [0, 1, 2, 3, 7, 8, 9, 15, 16, 17, 31, 48, 49, 50, 63, 64, 100, 997, 2048, 2049, 9000]
    if quick:
        sizes = [0, 1, 7, 9, 49, 50, 100, 2049]

    def base(kind, n):
        if kind == "normal":
            return rng.normal(100, 5, n)
        if kind == "ints 0..3":
            return rng.integers(0, 4, n).astype(float)
        if kind == "binary":
            return (rng.random(n) < 0.3) * 255.0
        if kind == "constant":
            return np.full(n, 42.0)
        if kind == "outlier":
            v = np.full(n, 3.0)
            if n:
                v[n // 2] = 1e4
            return v
        if kind == "huge":
            return rng.normal(0, 1e30, n)
        if kind == "tiny":
            return rng.normal(0, 1e-30, n)
        if kind == "subnormal":
            return rng.normal(0, 1e-41, n)
        if kind == "signed zeros":
            return np.where(rng.random(n) < 0.5, -0.0, 0.0)
        if kind == "all -inf":
            return np.full(n, -np.inf)
        return rng.normal(10, 3, n)
    kinds = ["normal", "ints 0..3", "binary", "constant", "outlier", "huge", "tiny", "subnormal", "signed zeros",
             "all -inf", "nan / inf"]
    extra = [("0.01", 1, 4), ("0.25", 1, 0), ("0.75", 3, 7), ("0.9", 2, 4), ("0.999", 1, 100), ("0", 1, 1),
             ("-0.5", 1, 4), ("1.5", 1, 4), ("inf", 0, 4), ("nan", 1, 4), ("1-2^-24", 1, 4), ("0.5", 0, 4)]
    j = 0
    for kind in kinds:
        for n in sizes:
            v = np.asarray(base(kind, n), F32)
            if kind == "nan / inf" and n:
                v.view(np.uint32)[rng.integers(0, n)] = 0x7fc00123          # quiet NaN with payload
                if n > 3:
                    v[rng.integers(0, n)] = np.inf
                    v.view(np.uint32)[rng.integers(0, n)] = 0x7f800001      # signalling NaN
                    v[rng.integers(0, n)] = -np.inf
            combos = [("0.99", 1, 4), ("0.5", 1, 4), ("1.0", 1, 4)]
            if not quick:
                combos.append(extra[j % len(extra)])
                j += 1
            for qn, flag, div in combos:
                out.append((f"{kind} n={n} q={qn} flag {flag} div {div}", v, np.uint32(Q_BITS[qn]).view(F32), flag, div))
    # q >= 1: NaN / zeros / -inf in the SIMD part (first n & ~7 values) and in the scalar tail
    for n, pos, val in ((16, 3, "qnan"), (16, 3, "snan"), (21, 18, "qnan"), (21, 2, "snan"), (21, 20, "-0"),
                        (8, 7, "-inf"), (13, 12, "+0"), (40, 9, "two nans"), (5, 1, "qnan")):
        v = rng.normal(-5, 1, n).astype(F32)
        if val == "qnan":
            v.view(np.uint32)[pos] = 0x7fc0abcd
        elif val == "snan":
            v.view(np.uint32)[pos] = 0xff800abc
        elif val == "two nans":
            v.view(np.uint32)[pos] = 0x7fc00001
            v.view(np.uint32)[pos + 8] = 0xffc00002
        elif val == "-0":
            v[:] = -np.abs(v)
            v[pos] = -0.0
        elif val == "+0":
            v[:] = -np.abs(v)
            v[pos] = 0.0
            v[0] = -0.0
        else:
            v[:] = -np.inf
        out.append((f"max n={n} {val} at {pos}", v, F32(1.0), 1, 4))
    return out


def check_quantile(case):
    name, v, q, flag, div = case
    try:
        g, _ = genuine_quantile(v, q, flag, div)
    except Rejected as e:
        return None, f"quantile {name:44s} genuine rejects: {e}"
    with np.errstate(all="ignore"):
        mine = int(np.asarray(nb.quantile(v, q, flag, div), F32).view(np.uint32))
    return g == mine, (f"quantile {name:44s} {'EXACT' if g == mine else 'DIFF'} genuine {g:08x} port {mine:08x}")


def check_helpers():
    """Copies of helpers of other modules against their originals (no genuine code)."""
    import nv_curve
    import nv_pixnoise as npx
    rng = np.random.default_rng(31)
    s = np.concatenate([rng.normal(0, 1, 20000) * 10.0 ** rng.uniform(-60, 60, 20000),
                        [0.0, -0.0, np.inf, -np.inf, np.nan, 2.0 ** -126, 2.0 ** -127, 2.0 ** 127, 2.0 ** 128,
                         1.0 + 2.0 ** -24, 1.0 + 3 * 2.0 ** -24, 3.4028235677973366e38]]).astype(np.float64)
    r1, f1 = nb._round_or_flag(s)
    r2, f2 = npx._round_or_flag(s)
    ok1 = np.array_equal(r1.view(np.uint32), r2.view(np.uint32)) and np.array_equal(f1, f2)
    xs = np.concatenate([rng.normal(0, 1, 2000) * 10.0 ** rng.uniform(-5, 25, 2000),
                         [np.nan, np.inf, -np.inf, 2.0 ** 63, -(2.0 ** 63), 2.0 ** 62, -0.0, 0.99999994, -1.5]]).astype(F32)
    ok2 = all(nf._fcvtzs(x) == nv_curve._fcvtzs1(x) for x in xs)
    print(f"helpers: nv_band._round_or_flag == nv_pixnoise._round_or_flag on {s.size} values: {ok1}; "
          f"nv_flat._fcvtzs == nv_curve._fcvtzs1 on {xs.size} values: {ok2}", flush=True)
    return ok1 and ok2


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true", help="a small subset")
    ap.add_argument("--no-model", action="store_true", help="skip FUN_00995250 (model)")
    ap.add_argument("--no-quantile", action="store_true", help="skip FUN_00997bcc (nvh quantile)")
    ap.add_argument("--jobs", type=int, default=8, help="parallel nvh processes")
    ap.add_argument("--list", action="store_true", help="list the cases and exit")
    A = ap.parse_args()
    require_guard()
    cs = cases(A.quick)
    qs = [] if A.no_quantile else quantile_cases(A.quick)
    if A.list:
        for k, v in cs.items():
            print(k, v.shape)
        for c in qs:
            print(c[0])
        return
    t0 = time.time()
    helpers_ok = check_helpers()
    jobs = []
    skipped = []
    for mode in ("levels",) + (() if A.no_model else ("model",)):
        for name, p in cs.items():
            if mode == "model" and max(p.shape) > 512:
                skipped.append((name, "side > 512"))      # FUN_00995250's line-kernel tables hold sides <= 512
                continue
            if mode == "model" and min(p.shape) < 2:
                skipped.append((name, "single row / column"))
                continue
            jobs.append((name, p, mode))
    ok = {"levels": 0, "model": 0}
    tot = {"levels": 0, "model": 0}
    rej = {"levels": 0, "model": 0}
    worst_leg = {"levels": [0.0] * 4, "model": [0.0] * 4}
    dir_counts, dir_union, dir_min = [], set(), None
    with ThreadPoolExecutor(A.jobs) as ex:
        for (name, p, mode), (st, txt, info) in zip(jobs, ex.map(lambda j: check(*j), jobs)):
            print(txt, flush=True)
            if st is None:
                rej[mode] += 1
                continue
            tot[mode] += 1
            ok[mode] += bool(st)
            if info and info[1] is not None:
                worst_leg[mode] = [max(a, b) for a, b in zip(worst_leg[mode], info[1])]
            if info and info[2] is not None:
                dir_counts.append(info[2].size)
                dir_union.update(info[2].tolist())
                if info[2].size:
                    dir_min = int(info[2][0]) if dir_min is None else min(dir_min, int(info[2][0]))
        qok = qtot = qrej = 0
        for st, txt in ex.map(check_quantile, qs):
            print(txt, flush=True)
            if st is None:
                qrej += 1
                continue
            qtot += 1
            qok += bool(st)
    for name, why in skipped:
        print(f"model  {name:44s} skipped ({why})")
    for mode in ok:
        if tot[mode] or rej[mode]:
            what = "all 4096 words" if mode == "levels" else "level words 0..54"
            print(f"{mode}: {ok[mode]}/{tot[mode]} bit-identical ({what}); genuine rejected {rej[mode]};"
                  f" legacy port worst rel.err mean/q99/q50/cv "
                  + "/".join(f"{v:.1e}" for v in worst_leg[mode]))
    if dir_counts:
        print(f"model directional words (not in nv_band.model_levels; nv_model ports them, model_verify/): {min(dir_counts)}..{max(dir_counts)} differing words per plane"
              f" over {len(dir_counts)} planes, {len(dir_union)} distinct positions, first at word "
              f"{dir_min} (byte {4 * dir_min:#x})" if dir_min is not None else "model directional words: none differ")
    if qs:
        print(f"quantile: {qok}/{qtot} bit-identical; genuine rejected {qrej}")
    print(f"helpers: {'ok' if helpers_ok else 'MISMATCH'}; {time.time() - t0:.1f} s")


if __name__ == "__main__":
    main()
