#!/usr/bin/env python3
"""End-to-end check of the EstimateNoiseLevelCurve port (repro/nv_curve.genuine_noise_curve with
repro/nv_pixnoise + repro/nv_lines; its default fast path batch=True since 2026-10-16, and with
--reference also the reference path batch=False, which must then give the same bits) against the
genuine plugin: `nvh curve W H rgb.f32`
(FUN_009b6c8c, one thread) prints the 8-point (brightness, noise) table of the BaseInterpolator
with %.17g, i.e. every double exactly.  Pass criterion: the xs identical and every one of the 8
ys bit-identical to the genuine (a NaN only matches a NaN; %.17g prints every NaN as "nan").
There is no tolerance.

  python3 verify_curve.py [--quick] [--no-clips] [--no-synthetic] [--no-extreme] [--extra] [--fuzz N]
                          [--diagnose] [--reference] [--sensitivity] [--no-order] [--keep DIR] [--v02 PATH]
                          [--v02-frames 0,312,576] [--test PATH] [--test-frames 300] [--reduce PATH]
                          [--clip-sample 200] [--jobs 8]

Cases (default, short):
  * test_v02.mp4 frames 0, 312, 576 (1920x1080; read_frames -> nvfilter.yuv_to_rgb with the
    matrix of nvfilter.matrix_from_tags, clipped to 0..255, float32) and test.mp4 frame 300 (640x360)
  * synthetic 1920x1080, 640x360, 100x70, 97x61, each as: gaussian noise on a gradient; letterbox
    bars (rows of exact 0, edges not on a 16 px grid); blown highlights (a 16-aligned block of 255
    and a non-aligned one); pure flat 128 (no noise: every block clipped -> no counts -> 1.0)
  * extreme (outside the 8-bit domain, squares overflow float32): a 160x120 gradient + gauss image
    scaled by 1e18 (FRECPS(inf, 0) = 2 in the weight ratio) and 160x120 images with a 1e20 / 1e25
    block (NaN noise sums -> NaN ys)
--extra (inputs that until 2026-10-16 only the independent reviews had checked):
  * grids of real frames: 3840x2160 (2x2 of test_v02 n=100/200/450/650), 4096x2160 (that grid plus
    256 columns of n=700), the same with 2.39:1 bars (223 black rows each side), 7680x4320 (4x4 of
    test_v02 n=100/200/450/650/700 and test_v02_reduce.mov n=312 (the genuine's own output), mirrored)
  * crops of real frames: 48x48 (two), 49x97, 1001x777, 1366x768, 48x1080, 1920x48
  * test_v02 n=100 without the 0..255 clip (yuv_to_rgb as is, about -24..281) and rounded to integers
  * the count threshold: 640x360 flat 128 with a 36x32 noisy patch whose bin-3 count is 225 = (W*H)>>10
    (valid) or 224 (invalid -> curve 1.0)
  * fill_curve branches with a non-positive fitted value (8-bit bands, each with its own noise): the line
    below the first valid bin <= 0 (neighbour copied), the line above the last <= 0 (twice), a quadratic
    interior fill below zero (kept: the genuine does not check it); the notes name the branches taken
  * extreme but finite: 1920x1080 stripes (bins 1-5, noise seed 30) + a brightness-220 patch + one 1e20
    pixel (finite ys 1.6 .. 3.2e14, see huge_stripes); stripes with the same levels (noise seed 40, no
    patch) and one 1e20 pixel in a bin-2 stripe (4 NaN ys)
--fuzz N: N seeded random images of 48..260 px (gradient / stripes / flat with patches / uniform;
  optionally clipped, rounded, shifted negative or scaled by 1e-40 .. 1e30; for each of NaN, +inf, -inf,
  huge, huge negative drawn, 1-3 such pixels; every odd case made finite: non-finite -> 128,
  |v| > 1e15 -> +-U(1e9, 1e15)), each through the fast and the reference path (xs, ys, sums, counts
  bit-identical between them as well).
Per case: genuine / port timing, the number of bit-identical ys (of 8), max abs and rel difference
of the finite ys, the bin counts.
Order census (default; --no-order skips it): per case the fast path is run once more with its per-tile
additions recorded (replayed in the same order they must give the same sums, part of the pass), then
replayed with the tiles in reverse order and with each tile's pixels in reverse order.  If that changes
the port's ys (NaN = NaN, as in the genuine comparison), the case is order-sensitive and its bit-identical
ys are consistent with the port's addition order (tile by tile in scheduler order, row-major in a tile).
They do not prove it: some wrong orders give the same ys.  Against `nvh curve` (hot block 1e20 and
huge_stripes, re-verification 2026-10-16) tiles reversed, pixels reversed, column-major pixels, per-tile
partial sums, per-bin pairwise sums, the first two tiles swapped and no waiting were excluded; the order
in which waiting tiles are flushed (before the current tile, reversed) was not, and is checked shell
against shell in verify_curve_shell.py.
So far (2026-10-16) no real frame and no ordinary synthetic image was order-sensitive; the order-sensitive
inputs all mix a few huge pixels (1e13 .. 1e20) with ordinary values, so that the double bin sums round
(an image scaled as a whole, e.g. by 1e18, was not order-sensitive).
NaN ys: a NaN matches any NaN, so a curve with NaN says little.  Every case whose genuine ys contain a
NaN is localised automatically (and counts as exact only if this matches too): the port's luma of every
tile through `nvh pixnoise` vs nv_pixnoise (noise / weight / bright bitwise, NaN payloads included), the
per-tile (sums, counts) from both sets of maps (bitwise; a NaN sum matches a NaN sum), and `nvh clip`
on the 16x16 blocks whose R or B holds a non-finite or huge (> 1e15) value or whose IsClip is within
0.33 +- 0.05.  --sensitivity checks that this is not vacuous: on the hot block 1e20 case, one NaN payload
bit flipped or one finite kept noise value moved by 1 ulp per tile, or IsClip 0.5 on the blocks with huge
values (all leave the ys unchanged), must each be reported as a difference.  It also injects two wrong
accumulation orders into nv_curve.tile_histogram_batch (tiles added in reverse order; each tile's pixels
added in reverse order) and requires `nvh curve` to disagree with the port on the order-sensitive cases
hot block 1e20 and huge_stripes (whose unfaulted port must agree).
A mismatch (or --diagnose) localises the difference per tile in full: the port's luma through
`nvh pixnoise` vs nv_pixnoise (bright / noise / weight bit-exact?), the IsClip values of the
16x16 blocks through `nvh clip` (uncapped: every block whose port IsClip of R or B is non-zero,
every block within 0.33 +- 0.05, plus a seeded random sample of --clip-sample blocks with zero
IsClip; nvh clip prints %.6f, so the port's value is compared as the same %.6f string, together
with the block's zeroing decision), and the per-tile (sums, counts) from both sets of maps.
Exit code 0 when every case is exact."""
import argparse
import concurrent.futures as cf
import itertools
import os
import shutil
import sys
import tempfile
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "repro"))
sys.path.insert(0, HERE)
import nv_curve as ncv  # noqa: E402
import nv_pixnoise as npx  # noqa: E402
import nvh_common as nvh  # noqa: E402

F32 = np.float32
U32 = np.uint32
RESULTS = []
NAN_LOC = []      # automatic localisations of the cases with NaN ys (dicts, see localise)
ORDER = []        # order census: (name, replay ok, ys change with tiles reversed, with pixels reversed)


def check(name, ok, detail=""):
    RESULTS.append((name, bool(ok)))
    print(f"[{'OK ' if ok else 'NG '}] {name}" + (f"  {detail}" if detail else ""), flush=True)
    return bool(ok)


# ----------------------------------------------------------------------------------------
# genuine wrappers
# ----------------------------------------------------------------------------------------
def genuine_curve(rgb):
    """`nvh curve W H rgb.f32` -> (ok, xs, ys) as doubles (parsed from %.17g, exact)."""
    H, W = rgb.shape[:2]
    out = nvh.run(["curve", W, H], [np.moveaxis(np.asarray(rgb, F32), -1, 0)], timeout=1800)
    lines = out.strip().splitlines()
    ok, n = int(lines[0].split()[1]), int(lines[0].split()[3])
    pts = [tuple(float(v) for v in l.split()) for l in lines[1:1 + n]]
    return ok, np.array([p[0] for p in pts]), np.array([p[1] for p in pts])


def genuine_pixnoise(luma, workdir, tag):
    """`nvh pixnoise` (weight starts at 1.0) -> dict of bright / noise / weight planes."""
    H, W = luma.shape
    prefix = os.path.join(workdir, tag)
    nvh.run(["pixnoise", W, H, prefix], [np.ascontiguousarray(luma, F32)])
    out = {}
    for s in ("bright", "noise", "weight"):
        out[s] = np.fromfile(f"{prefix}.{s}.f32", F32).reshape(H, W)
        os.unlink(f"{prefix}.{s}.f32")
    return out


def genuine_clip_strings(planes):
    """`nvh clip` on up to three (h, w) planes packed as R, G, B -> their IsClip as printed (%.6f)."""
    h, w = planes[0].shape
    stack = list(planes) + [np.zeros((h, w), F32)] * (3 - len(planes))
    out = nvh.run(["clip", w, h], [np.stack(stack)])
    return out.split()[1:1 + len(planes)]


def same_bits(a, b):
    """Elementwise: identical doubles (sign of zero included) or both NaN."""
    a = np.asarray(a, np.float64)
    b = np.asarray(b, np.float64)
    return (a.view(np.uint64) == b.view(np.uint64)) | (np.isnan(a) & np.isnan(b))


def same_arrays(a, b):
    """Bit-identical arrays (dtype, shape and every bit, NaN payloads included)."""
    a, b = np.asarray(a), np.asarray(b)
    return a.dtype == b.dtype and a.shape == b.shape and np.array_equal(a.view(np.uint8), b.view(np.uint8))


# ----------------------------------------------------------------------------------------
# accumulation order: recording, replay and injected faults (fast path; also used by verify_curve_shell.py)
# ----------------------------------------------------------------------------------------
def recorded_adds(rgb, **kw):
    """nv_curve.tile_histogram_batch with every per-tile addition (nv_curve._add_kept) recorded in the order
    made -> (sums, counts, [(bins, noise) per tile]).  The bins are kept as uint8 (nbins 8) to save memory."""
    rec = []
    add0 = ncv._add_kept

    def rec_add(sums, counts, bins, noise, nbins):
        rec.append((bins.astype(np.min_scalar_type(max(nbins - 1, 0))), noise))
        add0(sums, counts, bins, noise, nbins)

    ncv._add_kept = rec_add
    try:
        sums, counts = ncv.tile_histogram_batch(rgb, **kw)
    finally:
        ncv._add_kept = add0
    return sums, counts, rec


def replay(rec, nbins=ncv.NBINS, reverse_tiles=False, reverse_pixels=False):
    """The recorded additions of recorded_adds, in their order or with the tiles / each tile's pixels reversed."""
    sums, counts = np.zeros(nbins), np.zeros(nbins, np.int64)
    for bins, noise in (reversed(rec) if reverse_tiles else rec):
        if reverse_pixels:
            bins, noise = bins[::-1], noise[::-1]
        ncv._add_kept(sums, counts, bins, noise, nbins)
    return sums, counts


def order_census(rgb):
    """Is the port's curve sensitive to the addition order on this input?  -> (the recorded additions
    replayed in order give the fast path's sums and counts, the ys change (NaN = NaN, as in the genuine
    comparison) with the tiles added in reverse order, the ys change with each tile's pixels reversed)."""
    H, W = rgb.shape[:2]
    with np.errstate(all="ignore"):
        sums, counts, rec = recorded_adds(rgb)
        ys = ncv.curve_table(sums, counts, W, H)[1]
        s, c = replay(rec)
        out = [same_arrays(s, sums) and same_arrays(c, counts)]
        for kw in (dict(reverse_tiles=True), dict(reverse_pixels=True)):
            s, c = replay(rec, **kw)
            out.append(not bool(np.all(same_bits(ys, ncv.curve_table(s, c, W, H)[1]))))
    return tuple(out)


class OrderFault:
    """Context manager: a wrong accumulation order inside nv_curve.tile_histogram_batch (tests only).
    kind "pixels": each tile's kept pixels are added in reverse order; kind "tiles": the tiles are added in
    reverse order (the per-tile additions are recorded and replayed backwards when the batch shell returns)."""

    def __init__(self, kind):
        if kind not in ("pixels", "tiles"):
            raise ValueError(kind)
        self.kind = kind

    def __enter__(self):
        add0, thb0 = self.saved = ncv._add_kept, ncv.tile_histogram_batch
        if self.kind == "pixels":
            ncv._add_kept = lambda s, c, b, n, nb: add0(s, c, b[::-1], n[::-1], nb)
        else:
            def thb(rgb, pixnoise=None, nbins=ncv.NBINS, is_clip=None, tiles=None, max_batch=ncv.MAX_BATCH):
                rec = []
                ncv._add_kept = lambda s, c, b, n, nb: rec.append((b, n))
                try:
                    sums, counts = thb0(rgb, pixnoise, nbins, is_clip, tiles, max_batch)
                finally:
                    ncv._add_kept = add0
                for b, n in reversed(rec):
                    add0(sums, counts, b, n, nbins)
                return sums, counts
            ncv.tile_histogram_batch = thb
        return self

    def __exit__(self, *exc):
        ncv._add_kept, ncv.tile_histogram_batch = self.saved
        return False


# ----------------------------------------------------------------------------------------
# inputs
# ----------------------------------------------------------------------------------------
def synthetic_images(sizes):
    """[(name, rgb float32 (H, W, 3))] - four kinds per size, seeded."""
    out = []
    for k, (W, H) in enumerate(sizes):
        rng = np.random.default_rng(100 + k)
        yy, xx = np.mgrid[0:H, 0:W]
        base = (30.0 + 190.0 * xx / max(W - 1, 1) + 10.0 * yy / max(H - 1, 1)).astype(np.float64)
        noisy = np.stack([base, base * 0.95 + 5, base * 0.9 + 12], -1) + rng.normal(0, 4.0, (H, W, 3)) * [1.0, 0.8, 1.3]
        out.append((f"gradient+gauss {W}x{H}", np.clip(noisy, 0, 255).astype(F32)))
        # letterbox: rows of exact 0 at both ends; the bar edge is not on the 16 px grid
        bar = max(3, (H * 13) // 100)
        lb = np.clip(noisy, 0, 255)
        lb[:bar] = 0.0
        lb[H - bar:] = 0.0
        out.append((f"letterbox {W}x{H} (bars {bar} rows)", lb.astype(F32)))
        # blown highlights: a 16-aligned block of 255 and a non-aligned one
        hl = np.clip(noisy, 0, 255)
        s = min(64, (min(W, H) // 2) // 16 * 16)
        ax, ay = 16 * ((W // 4) // 16), 16 * ((H // 4) // 16)
        hl[ay:ay + s, ax:ax + s] = 255.0
        bx, by = (W * 5) // 9 + 3, (H * 3) // 7 + 5
        bw, bh = max(6, s * 3 // 4 + 2), max(5, s * 2 // 3 + 1)
        hl[by:by + bh, bx:bx + bw] = 255.0
        out.append((f"highlights {W}x{H} (aligned {s}px at {ax},{ay}; {bw}x{bh} at {bx},{by})", hl.astype(F32)))
        out.append((f"flat 128 {W}x{H}", np.full((H, W, 3), 128.0, F32)))
    return out


def extreme_images():
    """Inputs outside the 8-bit domain (review cases): squares of the band values overflow."""
    out = []
    r = np.random.default_rng(10)
    for _ in range(3):
        r.uniform(0, 1, 156)
    _, xx = np.mgrid[0:120, 0:160]
    img = np.clip(30 + 190 * xx[..., None] / 159 + r.normal(0, 4, (120, 160, 3)), 0, 255) * 1e18
    out.append(("gradient+gauss 160x120 x1e18", img.astype(F32)))
    rng = np.random.default_rng(11)
    base = (40 + 180 * xx / 160)[..., None] + rng.normal(0, 4, (120, 160, 3))
    for mag in (1e20, 1e25):
        hot = base.copy()
        hot[50:58, 60:68] = mag * (1 + rng.random((8, 8, 3)))
        out.append((f"hot block {mag:g} 160x120 (8x8 at 60,50)", hot.astype(F32)))
    return out


def decode_frames(path, frames, clip=True):
    """({n: rgb}, matrix tag) for the frame numbers of a clip, converted as the host would (matrix from
    the stream tags, chroma repeated, float32; clipped to 0..255 unless clip=False)."""
    import nvfilter as nv
    from denoise_video_v5 import probe, read_frames
    info = probe(path)
    want = sorted(set(int(v) for v in frames))
    mat = nv.matrix_from_tags(info["tags"], info["H"])
    out = {}
    for i, pl in read_frames(path, info["W"], info["H"], want[0], want[-1] + 1, fps=info["fps"]):
        if i in want:
            pl = tuple(np.asarray(p, F32) for p in pl)
            rgb = nv.yuv_to_rgb(pl, mat, info["full_range"])
            out[i] = (np.clip(rgb, 0, 255) if clip else rgb).astype(F32)
    missing = [i for i in want if i not in out]
    if missing:
        print(f"    (warn) {path}: frames {missing} not decoded")
    return out, f"{mat}{', full' if info['full_range'] else ''}"


def clip_frames(path, frames, clip=True):
    """[(name, rgb)] for the frame numbers of a clip (decode_frames)."""
    out, tag = decode_frames(path, frames, clip)
    return [(f"{os.path.basename(path)} n={i} [{tag}{'' if clip else ', unclipped'}]", rgb)
            for i, rgb in sorted(out.items())]


def stripes(W, H, seed, levels, sigma=3.0):
    """Vertical stripes of constant level (equal widths) + gaussian noise, float64 (not clipped)."""
    rng = np.random.default_rng(seed)
    _, xx = np.mgrid[0:H, 0:W]
    k = np.minimum(xx * len(levels) // W, len(levels) - 1)
    return np.stack([np.asarray(levels, float)[k]] * 3, -1) + rng.normal(0, sigma, (H, W, 3))


def bands(W, H, levels, sigmas, seed):
    """Vertical bands of constant level, each with its own noise sigma, integer RGB 0..255."""
    rng = np.random.default_rng(seed)
    img = np.empty((H, W))
    edges = np.linspace(0, W, len(levels) + 1).astype(int)
    for k, (lv, s) in enumerate(zip(levels, sigmas)):
        a, b = edges[k], edges[k + 1]
        img[:, a:b] = lv + rng.normal(0, s, (H, b - a))
    rgb = np.stack([img + rng.normal(0, 0.5, (H, W)) for _ in range(3)], -1)
    return np.ascontiguousarray(np.clip(np.round(rgb), 0, 255), F32)


HUGE_STRIPES = "stripes 1920x1080 bins 1-5 + 16x100 patch at 220 + one 1e20 pixel at (1000,300) (finite huge ys)"


def huge_stripes():
    """The extreme-but-finite review case: 1920x1080 stripes (bins 1-5, noise seed 30) + a 16x100 patch of
    220 + N(0, 3) at (990, 296) + one 1e20 pixel at (1000, 300): finite ys 1.6 .. 3.2e14 whose double bin sums
    round, so they depend on the addition order."""
    img = stripes(1920, 1080, 30, (48, 80, 112, 144, 176))
    img[296:396, 990:1006] = 220.0 + np.random.default_rng(31).normal(0, 3.0, (100, 16, 3))
    img = np.clip(img, 0, 255)
    img[300, 1000] = 1e20
    return img.astype(F32)


def threshold_image(seed):
    """640x360 flat 128 + a 32x36 patch of 112 + N(0, 4) at (32, 32) (seed 1009: bin-3 count 224, seed 1019:
    225 = (W*H)>>10; found by a scan of the seeds 1000..1059)."""
    rng = np.random.default_rng(seed)
    img = np.full((360, 640, 3), 128.0)
    img[32:64, 32:68] = 112.0 + rng.normal(0, 4.0, (32, 36, 3))
    return np.clip(img, 0, 255).astype(F32)


def fill_branches(sums, counts, W, H, nbins=ncv.NBINS):
    """The fill_curve branches that met a non-positive fitted value (port side; for the case notes)."""
    m = ncv.curve_means(sums, counts, W, H, nbins)
    pts = [(float(i), float(m[i])) for i in range(nbins) if not (m[i] <= 0.0)]
    if len(pts) < 2 or len(pts) == nbins:
        return "none (fewer than 2 or all bins valid)"
    xs, ys = [p[0] for p in pts], [p[1] for p in pts]
    n, first, last = len(xs), int(xs[0]), int(xs[-1])
    ev = []
    nc = 3 if n >= 3 else 2
    c = ncv.polyfit_genuine(xs, ys, n, nc)
    for i in range(first + 1, last):
        if not (m[i] > 0.0):
            v = ncv.polyval_genuine(float(i), c, nc)
            if v <= 0.0:
                ev.append(f"interior bin {i} = {v:.4g} kept")
    for lo, rng_, side in ((True, range(first - 1, -1, -1), "below"), (False, range(last + 1, nbins), "above")):
        if not len(rng_):
            continue
        line = (first < nbins // 3) if lo else (last >= (2 * nbins) // 3)
        nfit, ncoef = ((3 if n > 2 else 2), 2) if line else (1, 1)
        sel = slice(0, nfit) if lo else slice(n - nfit, n)
        c = ncv.polyfit_genuine(xs[sel], ys[sel], nfit, ncoef)
        for i in rng_:
            v = ncv.polyval_genuine(float(i), c, ncoef)
            if v <= 0.0:
                ev.append(f"{side} bin {i} = {v:.4g} -> bin {i + 1 if lo else i - 1} copied")
    return "; ".join(ev) if ev else "none with a value <= 0"


def extra_cases(A):
    """--extra: (name, rgb, note) built on demand (the 7680x4320 grid alone is 380 MiB)."""
    fr = {}
    for path, frames, tag in ((A.v02, [100, 200, 450, 650, 700], "v02"), (A.reduce, [312], "reduce")):
        if os.path.exists(path):
            t0 = time.perf_counter()
            got, mat = decode_frames(path, frames)
            fr.update({(tag, n): rgb for n, rgb in got.items()})
            print(f"    decoded {os.path.basename(path)} n={sorted(got)} [{mat}] in {time.perf_counter() - t0:.1f} s")
        else:
            print(f"    (warn) {path} not found: the cases built from it are skipped")

    def v(n):
        return fr.get(("v02", n))

    if all(v(n) is not None for n in (100, 200, 450, 650)):
        uhd = np.ascontiguousarray(np.concatenate([np.concatenate([v(100), v(200)], 1),
                                                   np.concatenate([v(450), v(650)], 1)], 0))
        yield "grid 3840x2160 (2x2 of test_v02 n=100/200/450/650)", uhd, None
        if v(700) is not None:
            ext = v(700)
            dci = np.ascontiguousarray(np.concatenate([uhd, np.concatenate([ext[:, 800:1056], ext[:, 1100:1356]], 0)], 1))
            yield "grid 4096x2160 (the 3840x2160 grid + 256 columns of n=700)", dci, None
            dci[:223] = 0.0
            dci[2160 - 223:] = 0.0
            yield "grid 4096x2160 with 2.39:1 bars (223 black rows each side)", dci, None
            del dci
        del uhd
    names = [("v02", 100), ("v02", 200), ("v02", 450), ("v02", 650), ("v02", 700), ("reduce", 312)]
    if all(k in fr for k in names):
        flips = [lambda a: a, lambda a: a[:, ::-1], lambda a: a[::-1], lambda a: a[::-1, ::-1]]
        tiles = [flips[k // 6](fr[names[k % 6]]) for k in range(16)]
        big = np.ascontiguousarray(np.concatenate([np.concatenate(tiles[4 * r:4 * r + 4], 1) for r in range(4)], 0))
        del tiles
        yield ("grid 7680x4320 (4x4 of test_v02 n=100/200/450/650/700 and test_v02_reduce n=312, mirrored)",
               big, None)
        del big
    crops = [(700, 8, 900, 48, 48), (450, 555, 1001, 48, 48), (450, 301, 77, 97, 49), (450, 151, 313, 777, 1001),
             (100, 7, 13, 768, 1366), (650, 0, 1871, 1080, 48), (650, 1031, 0, 48, 1920)]
    for n, y, x, h, w in crops:
        if v(n) is not None:
            yield f"crop {w}x{h} of test_v02 n={n} at ({x},{y})", np.ascontiguousarray(v(n)[y:y + h, x:x + w]), None
    if os.path.exists(A.v02):
        raw, mat = decode_frames(A.v02, [100], clip=False)
        if 100 in raw:
            u = raw[100]
            yield (f"test_v02 n=100 unclipped RGB [{mat}] (range {float(u.min()):.1f} .. {float(u.max()):.1f})", u, None)
    if v(100) is not None:
        yield "test_v02 n=100 rounded to integer RGB", np.round(v(100)).astype(F32), None
    for seed, cnt in ((1019, 225), (1009, 224)):
        yield (f"count threshold 640x360: bin-3 count {cnt} vs (W*H)>>10 = 225 (seed {seed})", threshold_image(seed),
               lambda s, c, W, H: f"threshold (W*H)>>10 = {(W * H) >> 10}")
    note = (lambda s, c, W, H: f"branches: {fill_branches(s, c, W, H)}")
    yield "bands 480x270 bins 1,2,3 sigma 2/8/14 (line below <= 0)", bands(480, 270, [48, 80, 112], [2, 8, 14], 1), note
    yield "bands 480x270 bins 3,4,5 sigma 14/8/2 (line above <= 0)", bands(480, 270, [112, 144, 176], [14, 8, 2], 2), note
    yield ("bands 640x270 bins 0,1,6,7 sigma 8/2/2/6 (quadratic interior < 0)",
           bands(640, 270, [22, 48, 208, 236], [8, 2, 2, 6], 3), note)
    def ys_range(s, c, W, H):
        ys = ncv.curve_table(s, c, W, H)[1]
        return f"port ys {ys.min():.4g} .. {ys.max():.4g}"

    yield HUGE_STRIPES, huge_stripes(), ys_range
    img = np.clip(stripes(1920, 1080, 40, (48, 80, 112, 144, 176)), 0, 255)
    img[300, 600] = 1e20
    yield "stripes 1920x1080 bins 1-5 + one 1e20 pixel at (600,300) in a bin-2 stripe (NaN ys)", img.astype(F32), None


def fuzz_image(rng, k):
    """Case k of --fuzz: a random 48..260 px image (seeded generator of the 2026-10-16 review; odd k are
    made finite afterwards: NaN / inf -> 128, |v| > 1e15 -> +-U(1e9, 1e15), so that squares stay finite)."""
    W, H = int(rng.integers(48, 261)), int(rng.integers(48, 261))
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float64)
    kind = rng.integers(0, 4)
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
    else:
        img = rng.uniform(0, 255, (H, W, 3))
        tags.append("uniform")
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
    for what, p in (("nan", 0.25), ("inf", 0.15), ("-inf", 0.1), ("huge", 0.25), ("hugeneg", 0.1)):
        if rng.random() < p:
            n = int(rng.integers(1, 4))
            for _ in range(n):
                y, x, c = int(rng.integers(0, H)), int(rng.integers(0, W)), int(rng.integers(0, 3))
                v = {"nan": np.nan, "inf": np.inf, "-inf": -np.inf,
                     "huge": float(10.0 ** rng.uniform(18, 38.5)), "hugeneg": -float(10.0 ** rng.uniform(18, 38.5))}[what]
                img[y, x, c] = np.float32(min(v, 3.4e38)) if np.isfinite(v) else v
            tags.append(f"{what}{n}")
    if k % 2:
        img = np.where(np.isfinite(img), img, F32(128.0)).astype(F32)
        big = np.abs(img) > 1e15
        img[big] = (np.sign(img[big]) * rng.uniform(1e9, 1e15, int(big.sum()))).astype(F32)
        tags.append("finite")
    return f"fuzz {k:03d} {W}x{H} " + ",".join(tags), img


def fuzz_cases(n, seed=20261016):
    rng = np.random.default_rng(seed)
    for k in range(n):
        name, img = fuzz_image(rng, k)
        yield name, img, None, True


# ----------------------------------------------------------------------------------------
# comparison
# ----------------------------------------------------------------------------------------
def compare_case(name, rgb, workdir, A, note=None, reference=False):
    t0 = time.perf_counter()
    ok, gx, gy = genuine_curve(rgb)
    t1 = time.perf_counter()
    with np.errstate(all="ignore"):
        px, py, sums, counts = ncv.genuine_noise_curve(rgb, return_hist=True)
    t2 = time.perf_counter()
    ref_note, ref_same = "", True
    if A.reference or reference:
        with np.errstate(all="ignore"):
            r = ncv.genuine_noise_curve(rgb, return_hist=True, batch=False)
        t3 = time.perf_counter()
        ref_same = all(same_arrays(a, b) for a, b in zip(r, (px, py, sums, counts)))
        ref_note = f"; reference path {t3 - t2:.2f} s, {'bit-identical' if ref_same else 'DIFFERENT'}"
    n_eq, abs_, rel = 0, float("inf"), float("inf")
    if len(gy) == len(py) and ok == 1 and np.array_equal(gx, px):
        eq = same_bits(gy, py)
        n_eq = int(eq.sum())
        fin = np.isfinite(gy) & np.isfinite(py)
        d = np.abs(gy[fin] - py[fin])
        abs_ = float(d.max()) if d.size else 0.0
        rel = float(np.max(d / np.maximum(np.abs(gy[fin]), 1e-300))) if d.size else 0.0
        if not np.array_equal(np.isfinite(gy), np.isfinite(py)):
            abs_ = rel = float("inf")
    same = ok == 1 and n_eq == len(gy) == len(py) and np.array_equal(gx, px) and ref_same
    nnan = int(np.isnan(gy).sum())
    detail = f"counts {counts.tolist()}"
    if note is not None:
        H, W = rgb.shape[:2]
        with np.errstate(all="ignore"):
            detail += f"; {note(sums, counts, W, H)}"
    if A.order:
        rep_ok, sens_t, sens_p = order_census(rgb)
        ORDER.append((name, rep_ok, sens_t, sens_p))
        same &= rep_ok
        detail += ("; order: " + ("ORDER-SENSITIVE (ys change with " + " / ".join(
            k for k, f in (("tiles reversed", sens_t), ("pixels reversed", sens_p)) if f) + ")"
            if sens_t or sens_p else "not order-sensitive") + ("" if rep_ok else ", RECORDED REPLAY DIFFERS"))
    lines, nan_note = [], ""
    if not same or A.diagnose:
        lines += ["    genuine ys " + str([repr(float(v)) for v in gy]), "    port    ys " + str([repr(float(v)) for v in py])]
        same &= localise(name, rgb, gy, workdir, A, lines, mode="full")
    elif nnan:                # NaN = NaN in the ys: compare the per-tile maps and sums as well
        same &= localise(name, rgb, gy, workdir, A, lines, mode="nan")
        nan_note = f" ({nnan} NaN; per tile {NAN_LOC[-1]['text']})"
    check(f"{name}: genuine {t1 - t0:.2f} s, port {t2 - t1:.2f} s, {n_eq}/{len(gy)} ys bit-identical{nan_note}, "
          f"max_abs {abs_:.3g}, max_rel {rel:.3g}{ref_note}", same, detail)
    for line in lines:
        print(line)
    return same, n_eq, len(gy), (t1 - t0, t2 - t1)


def block_origin(i, block, size):
    """Origin of block i along a side as FUN_009b76b0 places it (the last one is pushed inside)."""
    return i * block if block <= size - i * block else size - block


def localise(name, rgb, gy, workdir, A, out, mode="full"):
    """Per tile: the port's luma through `nvh pixnoise` vs nv_pixnoise (maps bit-exact?), the IsClip
    values of 16x16 blocks (nvh clip) and the (sums, counts) of each tile from the genuine maps vs the
    port maps; report lines go to the list `out`.  mode "full": every block with a non-zero port IsClip
    of R or B, every block within 0.33 +- 0.05 and a random sample of zero blocks, detailed report;
    mode "nan" (automatic for NaN ys): the blocks whose R or B holds a non-finite or huge (> 1e15) value
    or that lie within 0.33 +- 0.05, a summary in NAN_LOC (details only on a difference).
    Returns True when everything compared is identical."""
    full = mode == "full"
    H, W = rgb.shape[:2]
    R, G, B = (np.ascontiguousarray(rgb[..., c], F32) for c in range(3))
    tiles = ncv.tile_layout(W, H)
    if full:
        out.append(f"    --- localise {name}: {len(tiles)} tiles")
    rng = np.random.default_rng(1234)
    # IsClip of the blocks, selection by the port's values
    blocks = []          # (tile, bx, by, R crop, B crop, pr, pb, extreme values)
    lumas, weights = [], []
    for ti, d in enumerate(tiles):
        x, y, w, h = d[:4]
        tR, tG, tB = R[y:y + h, x:x + w], G[y:y + h, x:x + w], B[y:y + h, x:x + w]
        with np.errstate(all="ignore"):
            lumas.append(npx.luma_601(tR, tG, tB))
            cw = ncv.clip_weight(tR, tG, tB)
        weights.append(cw)
        for r in range((h + 15) // 16):
            by = block_origin(r, 16, h)
            for c in range((w + 15) // 16):
                bx = block_origin(c, 16, w)
                cr, cb = tR[by:by + 16, bx:bx + 16], tB[by:by + 16, bx:bx + 16]
                pr, pb = float(ncv.is_clip_genuine(cr)), float(ncv.is_clip_genuine(cb))
                xtr = not (np.isfinite(cr).all() and np.isfinite(cb).all()) or \
                    max(float(np.abs(cr).max()), float(np.abs(cb).max())) > 1e15
                blocks.append((ti, bx, by, cr, cb, pr, pb, xtr))
    near = [b for b in blocks if abs(b[5] - 0.33) <= 0.05 or abs(b[6] - 0.33) <= 0.05]
    if full:
        nonzero = [b for b in blocks if b[5] > 0 or b[6] > 0]
        zero = [b for b in blocks if not (b[5] > 0 or b[6] > 0)]
        pick = rng.choice(len(zero), min(A.clip_sample, len(zero)), replace=False) if zero else []
        chosen = nonzero + near + [zero[i] for i in sorted(pick)]
    else:
        chosen = [b for b in blocks if b[7]] + near
    sel = list({(b[0], b[1], b[2]): b for b in chosen}.values())
    planes = [p for b in sel for p in (b[3], b[4])]
    groups = [planes[i:i + 3] for i in range(0, len(planes), 3)]
    with cf.ThreadPoolExecutor(A.jobs) as ex:
        strs = [s for g in ex.map(genuine_clip_strings, groups) for s in g]
    bad_val = bad_dec = 0
    for k, b in enumerate(sel):
        gr, gb = strs[2 * k], strs[2 * k + 1]
        pr, pb = f"{b[5]:.6f}", f"{b[6]:.6f}"
        g_zero = float(gr) > float(ncv.CLIP_THR) or float(gb) > float(ncv.CLIP_THR)
        p_zero = b[5] > float(ncv.CLIP_THR) or b[6] > float(ncv.CLIP_THR)
        cw = weights[b[0]]
        if gr != pr or gb != pb:
            bad_val += 1
            if bad_val <= 5:
                out.append(f"    tile {b[0]} block ({b[1]},{b[2]}): nvh clip R {gr} B {gb}, port R {pr} B {pb}")
        if g_zero != p_zero or (p_zero and cw[b[2], b[1]] != 0):
            bad_dec += 1
    if full:
        out.append(f"    IsClip blocks: {len(blocks)} in the tiles, port non-zero {len(nonzero)}, within 0.33+-0.05 "
                   f"{len(near)}, random zero sample {len(pick)} -> {len(sel)} checked with nvh clip: value mismatches "
                   f"{bad_val}, decision mismatches {bad_dec}")
    elif bad_val or bad_dec:
        out.append(f"    IsClip of {len(sel)} blocks (non-finite / huge values, near 0.33) vs nvh clip: value mismatches "
                   f"{bad_val}, decision mismatches {bad_dec}")
    # pixel maps per tile (genuine pixnoise on the port's luma, weight 1 times the clip mask)
    with cf.ThreadPoolExecutor(A.jobs) as ex:
        gens = list(ex.map(lambda a: genuine_pixnoise(a[1], workdir, f"t{a[0]}"), enumerate(lumas)))
    bad_tiles = []
    sums_g = np.zeros(ncv.NBINS)
    counts_g = np.zeros(ncv.NBINS, np.int64)
    sums_p = np.zeros(ncv.NBINS)
    counts_p = np.zeros(ncv.NBINS, np.int64)
    first_diff = None
    n_nanpx = n_fin = 0
    for ti, d in enumerate(tiles):
        g = gens[ti]
        with np.errstate(all="ignore"):
            noise_p, weight_p, bright_p = npx.pixnoise(lumas[ti], weights[ti])
            weight_g = (g["weight"] * weights[ti]).astype(F32)
        diffs = {}
        for key, a, b in (("bright", g["bright"], bright_p), ("noise", g["noise"], noise_p), ("weight", weight_g, weight_p)):
            n_nanpx += int(np.isnan(a).sum())
            nd = int(np.sum(a.view(U32) != b.view(U32)))
            if nd:
                with np.errstate(all="ignore"):
                    diffs[key] = (nd, float(np.nanmax(np.abs(a.astype(np.float64) - b))))
        if diffs:
            bad_tiles.append((ti, d[:4], diffs))
        sg, cg = np.zeros(ncv.NBINS), np.zeros(ncv.NBINS, np.int64)
        sp, cp = np.zeros(ncv.NBINS), np.zeros(ncv.NBINS, np.int64)
        with np.errstate(all="ignore"):
            ncv.accumulate(g["bright"], g["noise"], weight_g, d, sg, cg)          # this tile alone
            ncv.accumulate(bright_p, noise_p, weight_p, d, sp, cp)
            # the running sums pixel by pixel in tile order, as FUN_009b76b0 adds them (adding the
            # per-tile partial sums instead would round differently)
            ncv.accumulate(g["bright"], g["noise"], weight_g, d, sums_g, counts_g)
            ncv.accumulate(bright_p, noise_p, weight_p, d, sums_p, counts_p)
        n_fin += int(((cg > 0) & np.isfinite(sg)).sum())
        if first_diff is None and (not np.array_equal(cg, cp) or not np.all(same_bits(sg, sp))):
            first_diff = (ti, d[:4], sg, cg, sp, cp)
    with np.errstate(all="ignore"):
        ys_g = ncv.curve_table(sums_g, counts_g, W, H)[1]
        ys_p = ncv.curve_table(sums_p, counts_p, W, H)[1]
    ok = not (bad_val or bad_dec or bad_tiles) and first_diff is None
    if full or not ok:
        out.append(f"    tiles with map differences (genuine pixnoise on the port's luma vs nv_pixnoise, bitwise): "
                   f"{len(bad_tiles)}/{len(tiles)}")
        for ti, box, diffs in bad_tiles[:8]:
            out.append(f"      tile {ti} {box}: " + ", ".join(f"{k} {v[0]} px (max abs {v[1]:.3g})" for k, v in diffs.items()))
        if first_diff is not None:
            ti, box, sg, cg, sp, cp = first_diff
            out.append(f"    first tile whose (sums, counts) differ: {ti} {box}")
            out.append(f"      genuine maps: counts {cg.tolist()} sums {sg.tolist()}")
            out.append(f"      port maps:    counts {cp.tolist()} sums {sp.tolist()}")
        else:
            out.append("    per-tile (sums, counts) identical for every tile")
        out.append(f"    shell with genuine maps vs nvh curve: {int(same_bits(ys_g, gy).sum())}/8 ys bit-identical; "
                   f"shell with port maps: {int(same_bits(ys_p, gy).sum())}/8")
    if not full:
        NAN_LOC.append(dict(ok=ok, tiles=len(tiles), nanpx=n_nanpx, fin=n_fin, blocks=len(sel),
                            text=f"{'identical' if ok else 'DIFFERENT'}: maps of {len(tiles)} tiles ({n_nanpx} NaN px), "
                                 f"{n_fin} finite tile-bin sums, {len(sel)} blocks vs nvh clip"))
    return ok


def sensitivity(workdir, A):
    """--sensitivity: faults that leave the ys of a NaN case unchanged must still fail the automatic
    localisation (the hot block 1e20 case: 7 NaN ys), and wrong accumulation orders in the fast path must
    make the port disagree with `nvh curve` on the order-sensitive cases (hot block 1e20, huge_stripes).
    The port's helpers are swapped for the run only."""
    name, rgb = extreme_images()[1]
    gy = genuine_curve(rgb)[2]
    for oname, orgb, ogy in ((name, rgb, gy), (HUGE_STRIPES, huge_stripes(), None)):
        if ogy is None:
            ogy = genuine_curve(orgb)[2]
        with np.errstate(all="ignore"):
            py = ncv.genuine_noise_curve(orgb)[1]
        n_eq = int(same_bits(ogy, py).sum())
        check(f"sensitivity control, {oname}: unfaulted port {n_eq}/8 ys bit-identical to nvh curve "
              f"({int(np.isnan(ogy).sum())} NaN)", n_eq == 8)
        for kind, label in (("tiles", "tiles added in reverse order"), ("pixels", "each tile's pixels added in reverse order")):
            with OrderFault(kind), np.errstate(all="ignore"):
                fy = ncv.genuine_noise_curve(orgb)[1]
            n_diff = 8 - int(same_bits(ogy, fy).sum())
            check(f"sensitivity, {oname}: {label} in tile_histogram_batch -> {n_diff}/8 ys differ from nvh curve "
                  f"(NaN = NaN)", n_diff > 0)
    pix0, clip0 = npx.pixnoise, ncv.is_clip_genuine

    def nan_payload(luma, weight):              # another NaN payload in the first NaN of each noise map
        noise, w, bright = pix0(luma, weight)
        bad = np.flatnonzero(np.isnan(noise.ravel()))
        if bad.size:
            noise.ravel().view(U32)[bad[0]] ^= 1
        return noise, w, bright

    def one_ulp(luma, weight):                  # one kept finite noise value one ulp up in each tile
        noise, w, bright = pix0(luma, weight)
        ok = np.flatnonzero(np.isfinite(noise.ravel()) & (w.ravel() >= 0.5))
        if ok.size:
            i = ok[ok.size // 2]
            noise.ravel()[i] = np.nextafter(noise.ravel()[i], F32(np.inf))
        return noise, w, bright

    def clip_half(plane, simd=True):            # IsClip 0.5 for the blocks with non-finite or huge values
        p = np.asarray(plane, F32)
        return F32(0.5) if not np.isfinite(p).all() or np.abs(p).max() > 1e15 else clip0(plane, simd)

    for label, pix, clip in (("one NaN payload bit flipped per tile (noise map)", nan_payload, clip0),
                             ("one finite kept noise value + 1 ulp per tile", one_ulp, clip0),
                             ("IsClip 0.5 on the blocks with huge values", pix0, clip_half)):
        npx.pixnoise, ncv.is_clip_genuine = pix, clip
        try:
            lines = []
            found = not localise(name, rgb, gy, workdir, A, lines, mode="nan")
        finally:
            npx.pixnoise, ncv.is_clip_genuine = pix0, clip0
            NAN_LOC.pop()
        check(f"sensitivity, {name}: {label} -> the NaN localisation reports "
              f"{'a difference' if found else 'NO difference'}", found, " / ".join(s.strip() for s in lines[:2]))


# ----------------------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--quick", action="store_true", help="synthetic 640x360 and 97x61 only, clip frames 312 / 300")
    ap.add_argument("--no-clips", action="store_true")
    ap.add_argument("--no-synthetic", action="store_true")
    ap.add_argument("--no-extreme", action="store_true")
    ap.add_argument("--extra", action="store_true", help="add the reviewer cases (frame grids up to 7680x4320, crops, "
                    "unclipped / integer RGB, count threshold, fill branches, extreme but finite)")
    ap.add_argument("--fuzz", type=int, default=0, metavar="N", help="add N seeded random images (fast and reference path)")
    ap.add_argument("--diagnose", action="store_true", help="run the per-tile localisation on every case")
    ap.add_argument("--sensitivity", action="store_true", help="injected faults must fail the automatic NaN check, "
                    "and wrong accumulation orders the comparison with nvh curve")
    ap.add_argument("--no-order", dest="order", action="store_false",
                    help="skip the order census (one more fast-path run per case)")
    ap.add_argument("--reference", action="store_true",
                    help="also run the reference path (batch=False, ~12 s per 1080p frame) and require the same bits")
    ap.add_argument("--keep", help="directory for the nvh pixnoise dumps (default: a temp dir, removed)")
    ap.add_argument("--v02", default=os.path.join(ROOT, "test_v02.mp4"))
    ap.add_argument("--v02-frames", default="0,312,576")
    ap.add_argument("--test", default=os.path.join(ROOT, "test.mp4"))
    ap.add_argument("--test-frames", default="300")
    ap.add_argument("--reduce", default=os.path.join(ROOT, "test_v02_reduce.mov"), help="genuine output clip (--extra)")
    ap.add_argument("--clip-sample", type=int, default=200, help="zero-IsClip blocks sampled per case in localise")
    ap.add_argument("--jobs", type=int, default=8, help="parallel nvh processes in localise")
    A = ap.parse_args()

    nvh.require_guard()           # nvh selftest must exit 3 before any genuine code runs
    workdir = A.keep or tempfile.mkdtemp(prefix="nvcurve_")
    os.makedirs(workdir, exist_ok=True)
    t_all = time.perf_counter()
    cases = []
    if not A.no_clips:
        v02_frames = ["312"] if A.quick else A.v02_frames.split(",")
        for path, frames in ((A.v02, v02_frames), (A.test, A.test_frames.split(","))):
            if os.path.exists(path):
                cases += clip_frames(path, frames)
            else:
                print(f"    (warn) {path} not found, skipped")
    if not A.no_synthetic:
        sizes = [(640, 360), (97, 61)] if A.quick else [(1920, 1080), (640, 360), (100, 70), (97, 61)]
        cases += synthetic_images(sizes)
    if not A.no_extreme:
        cases += extreme_images()
    stream = itertools.chain(((n, a, None, False) for n, a in cases),
                             ((n, a, note, False) for n, a, note in (extra_cases(A) if A.extra else ())),
                             fuzz_cases(A.fuzz) if A.fuzz > 0 else ())
    t1080 = []
    n_ys = n_eq = n_cases = n_same = 0
    for name, rgb, note, ref in stream:
        try:
            same, k, n, (tg, tp) = compare_case(name, rgb, workdir, A, note, ref)
        except Exception as e:  # noqa: BLE001 (a harness failure is reported as a failed case)
            check(f"{name}: {type(e).__name__}: {e}", False)
            same, k, n, tp = False, 0, 8, None
        n_cases += 1
        n_same += same
        n_ys += n
        n_eq += k
        if rgb.shape[1] == 1920 and rgb.shape[0] == 1080 and tp is not None:
            t1080.append(tp)
    if A.sensitivity:
        sensitivity(workdir, A)
    n_ok = sum(1 for _, ok in RESULTS if ok)
    print(f"\n{n_same}/{n_cases} cases bit-identical; {n_eq}/{n_ys} ys bit-identical to `nvh curve` (%.17g)"
          + (f"; port 1080p {np.mean(t1080):.2f} s/frame (n={len(t1080)})" if t1080 else "")
          + (f"; {n_ok}/{len(RESULTS)} checks passed" if len(RESULTS) != n_cases else ""))
    if NAN_LOC:
        print(f"NaN ys localised automatically: {sum(r['ok'] for r in NAN_LOC)}/{len(NAN_LOC)} cases identical "
              f"(maps of {sum(r['tiles'] for r in NAN_LOC)} tiles bitwise with {sum(r['nanpx'] for r in NAN_LOC)} NaN "
              f"pixels, {sum(r['fin'] for r in NAN_LOC)} finite tile-bin sums, {sum(r['blocks'] for r in NAN_LOC)} "
              f"blocks vs nvh clip)")
    if ORDER:
        sens = [r for r in ORDER if r[2] or r[3]]
        named = [r[0] for r in sens if not r[0].startswith("fuzz ")]
        nfuzz = sum(1 for r in ORDER if r[0].startswith("fuzz "))
        print(f"order census: {len(sens)}/{len(ORDER)} cases order-sensitive (port ys change with the tiles reversed in "
              f"{sum(r[2] for r in sens)}, with each tile's pixels reversed in {sum(r[3] for r in sens)}; recorded replay "
              f"== fast path in {sum(r[1] for r in ORDER)}/{len(ORDER)})"
              + (f"; fuzz {sum(1 for r in sens if r[0].startswith('fuzz '))}/{nfuzz}" if nfuzz else ""))
        for n in named:
            print(f"    order-sensitive: {n}")
    print(f"total {time.perf_counter() - t_all:.0f} s")
    if not A.keep:
        shutil.rmtree(workdir, ignore_errors=True)
    sys.exit(0 if n_ok == len(RESULTS) and RESULTS else 1)


if __name__ == "__main__":
    main()
