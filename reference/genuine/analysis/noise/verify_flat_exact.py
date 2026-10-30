#!/usr/bin/env python3
"""Decision-for-decision check of the exact FindFlatArea port (repro/nv_flat.py) against the genuine
FindFlatArea (FUN_009b1a5c, Auto Profile path param_3 = 0) run through harness/nvh (`nvh flat`).

Every case compares (x, y, w, h, status). The port gets the frame's host-style RGB (as
nvfilter.find_flat_area builds it) and the exact EstimateNoiseLevelCurve port
(nv_curve.genuine_noise_curve), i.e. the same inputs as the genuine.  Each genuine call is a fresh
nvh process with one FindFlatArea call: an obfuscated fragment of FindFlatArea (0x9b1dfc..0x9b1e7c)
updates a global counter (0x1488348, 0x5a270a93 in the file) on every call and, when its top two
bits are 0, adds a value derived from the token table (empty in the harness) to the reduced height
before the candidate search; with the file's initial value the first call of a process skips it.

Case sets (combine freely):
  --clip PATH[:N]            frames 0, N, 2N, ... of a clip (N defaults to --every; or --frames a,b,c
                             for every clip)
  --modes 0,1,2              profile colour modes (profile +0x48)
  --scales                   1440p / 4K (table 0x1246da8) and 4480x2520 / 8K (table 0x1246e20) frames
                             built from real frames (2x2 / 4x4 mosaics of different frames, mirrored,
                             and a 2x bilinear upscale)
  --small                    crops of real frames near the 48..64 px minimum (and below it)
  --synthetic                letterbox, clipped highlights, constant, coloured noise, NaN / inf pixels
  --inter                    also compare the intermediates of every frame with `nvh reduce` (FUN_009b7e58
                             + FUN_0097bb88) and `nvh cand` (FUN_009999cc, every size of the table)
  --production               (clip frames) also run the production entry nvfilter.find_flat_area (mode 1)
  --probe-crash              run the genuine once (one frame, one mode) where no square fits
                             (min(W, H) < 48); by default such frames are not given to the genuine, which
                             ends there with SIGSEGV (FUN_00a22868(NULL, 100) at 0x9b1c40, a crash report
                             each time)
Mismatches print the port's best candidates (score, flatness, energy) for diagnosis.

usage: verify_flat_exact.py --clip ../../../test_v02.mp4:24 --clip ../../../test.mp4:30 --modes 0,1,2 [--jobs 6]"""
import argparse
import os
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ProcessPoolExecutor

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPRO = os.path.join(HERE, "..", "..", "..", "reference", "python")
sys.path.insert(0, REPRO)
sys.path.insert(0, HERE)
import nv_curve  # noqa: E402
import nv_flat as nf  # noqa: E402
import nv_pixnoise as npx  # noqa: E402
from nvh_common import NVH, require_guard  # noqa: E402

F32 = np.float32


def nvh(args, arrays, timeout=1800):
    """Run `nvh args[0] W H file args[3:]` on float32 arrays; returns (returncode, stdout, stderr)."""
    with tempfile.NamedTemporaryFile(suffix=".f32", delete=False) as f:
        for a in arrays:
            np.ascontiguousarray(a, F32).tofile(f)
        name = f.name
    try:
        r = subprocess.run([NVH, args[0], *map(str, args[1:3]), name, *map(str, args[3:])],
                           capture_output=True, text=True, timeout=timeout)
    finally:
        os.unlink(name)
    return r.returncode, r.stdout, r.stderr


MIN_SIDE = 48            # the smallest square of the size tables: below it the genuine crashes (see --probe-crash)


def genuine_flat(rgb, mode):
    H, W, _ = rgb.shape
    rc, out, err = nvh(["flat", W, H, mode, 0], [np.moveaxis(rgb, -1, 0)])
    if rc == 3:
        return ("rejected", err.strip().splitlines()[-1][:120] if err.strip() else "")
    if rc == -11 and min(W, H) < MIN_SIDE:
        # no square fits: the early exit at 0x9b1c38 calls FUN_00a22868(NULL, 100), which reads [x0 + 4]
        # -> SIGSEGV; the genuine has no result there
        return ("rejected", "SIGSEGV (NULL progress node at 0x9b1c40)")
    if rc:
        raise RuntimeError(f"nvh flat {W}x{H} mode {mode} exit {rc}: {err[-300:]}")
    t = out.split()
    kv = dict(s.split("=") for s in t if "=" in s)
    return int(kv["x"]), int(kv["y"]), int(kv["w"]), int(kv["h"]), int(t[1])


def check_inter(rgb):
    """Intermediates against the genuine: reduced planes + luma, candidates for every size."""
    H, W = rgb.shape[:2]
    s = 7
    while True:
        s += 1
        w, h = W // s, H // s
        if not (h >= 5 and w >= 5 and w * h > 0x40000):
            break
    if w == 0 or h == 0:
        return "inter: no reduction"
    with tempfile.TemporaryDirectory() as d:
        pref = os.path.join(d, "r")
        rc, _, err = nvh(["reduce", W, H, s, pref], [np.moveaxis(rgb, -1, 0)])
        if rc:
            return f"inter: reduce exit {rc}"
        g = [np.fromfile(pref + suf, F32).reshape(h, w) for suf in (".r.f32", ".g.f32", ".b.f32", ".luma.f32")]
    R, G, B = nf.reduce_planes(rgb, s)
    L = npx.luma_601(R, G, B)
    msg = []
    if not all(np.array_equal(a.view(np.uint32), b.view(np.uint32)) for a, b in zip((R, G, B, L), g)):
        msg.append("reduce DIFF")
    sizes = next(sz for lim, sz, f in nf.TABLES if lim is None or W * H <= lim)
    step = max(16 // s, 1)
    ncand = 0
    for S in sizes:
        if S > W or S > H:
            continue
        r = max(int(F32(F32(F32(F32(S) / F32(s)) + F32(-1.0)) * F32(0.5))), 1)
        rc, out, err = nvh(["cand", w, h, r, r, step], [g[3]])
        if rc:
            msg.append(f"cand exit {rc}")
            continue
        rows = [l.split() for l in out.splitlines() if l.strip()]
        gx = np.array([int(t[1]) for t in rows])
        gy = np.array([int(t[2]) for t in rows])
        gm = np.array([int(t[3], 16) for t in rows], np.uint32)
        gs = np.array([int(t[4], 16) for t in rows], np.uint32)
        xs, ys, ms, ss = nf.candidates(L, r, r, step)
        if not (np.array_equal(gx, xs) and np.array_equal(gy, ys) and np.array_equal(gm, ms.view(np.uint32))
                and np.array_equal(gs, ss.view(np.uint32))):
            msg.append(f"cand size {S} DIFF")
        ncand += int((ss >= 0).sum())
    return "inter: " + ("; ".join(msg) if msg else f"reduce + luma and {ncand} candidates bit-identical")


def work(job):
    """One frame: the curve once, then every mode. Returns the report lines and (ok, n, rejected, skipped)."""
    label, rgb, modes, inter, prod, probe = job
    lines = []
    ok = n = rej = skip = 0
    t0 = time.time()
    H, W = rgb.shape[:2]
    na = None
    tcurve = 0.0
    results = {}
    for mode in modes:
        if min(H, W) < MIN_SIDE and not (probe and mode == modes[0]):      # the probe runs the genuine once
            p = nf.find_flat_area_genuine(rgb, lambda b: 1.0, mode)
            lines.append(f"{label:40s} mode {mode} not given to the genuine (no square fits: it crashes); port {p}")
            skip += 1
            continue
        tg = time.time()
        g = genuine_flat(rgb, mode)
        tg = time.time() - tg
        results[mode] = g
        if g[0] == "rejected":
            p = nf.find_flat_area_genuine(rgb, lambda b: 1.0, mode) if min(H, W) < MIN_SIDE else None
            lines.append(f"{label:40s} mode {mode} genuine rejects: {g[1]}" + (f"; port {p}" if p else ""))
            rej += 1
            continue
        if na is None:
            tc = time.time()
            na = nv_curve.noise_at(nv_curve.genuine_noise_curve(rgb))
            tcurve = time.time() - tc
        tr = []
        tp = time.time()
        p = nf.find_flat_area_genuine(rgb, na, mode, trace=tr)
        tp = time.time() - tp
        same = tuple(g) == tuple(p)
        ok += same
        n += 1
        lines.append(f"{label:40s} mode {mode} genuine {g}  port {p}  {'MATCH' if same else 'DIFF'}"
                     f"  ({tg:.2f} s genuine, {tp:.2f} s port + {tcurve:.2f} s curve)")
        if not same:
            tr.sort(key=lambda d: d["score"])
            for d in tr[:4]:
                lines.append(f"      cand size {d['size']} #{d['i']} ({d['x']},{d['y']}) score {d['score']:.9g}"
                             f" fl {['%.6g' % v for v in d['fl']]} E {d['E']:.6g} R {d['R']:.6g} nz {d['nz']:.6g}"
                             f" ncl {d['ncl']} pen {d['pen']}")
    if inter and min(H, W) >= MIN_SIDE:
        lines.append(f"{label:40s} " + check_inter(rgb))
    if prod is not None:
        import nvfilter as nv
        planes, full_range, mat = prod
        g1 = results[1] if 1 in results else genuine_flat(rgb, 1)
        y0, x0, S, st = nv.find_flat_area(planes, full_range, mat, with_status=True)
        if min(H, W) < 64 or g1[0] == "rejected":
            verdict, same = "(the production entry uses the fallback below 64 px; not compared)", None
        elif g1[4] == 1:
            # nothing found: the production entry returns its fallback area with status 1
            same = st == 1
            verdict = "MATCH (status 1, fallback area)" if same else "DIFF"
        else:
            same = (x0, y0, S, S, st) == tuple(g1)
            verdict = "MATCH" if same else "DIFF"
        lines.append(f"{label:40s} production nvfilter.find_flat_area -> (x {x0}, y {y0}, {S}, status {st}) "
                     f"genuine {g1} {verdict}")
        if same is not None:
            ok += same
            n += 1
    lines.append(f"{label:40s} total {time.time() - t0:.1f} s")
    return lines, (ok, n, rej, skip)


# ------------------------------------------------------------------------------------------------ inputs
def clip_frames(path, want=None, every=None):
    import nvfilter as nv
    from denoise_video_v5 import probe, read_frames
    info = probe(path)
    mat = nv.matrix_from_tags(info["tags"], info["H"])
    hi = (max(want) + 1) if want else None
    for i, pl in read_frames(path, info["W"], info["H"], min(want) if want else 0, hi, fps=info["fps"]):
        if (want and i in want) or (every and i % every == 0):
            pl = tuple(np.asarray(p, F32) for p in pl)
            rgb = np.clip(np.nan_to_num(nv.yuv_to_rgb(pl, mat, info["full_range"]), nan=0.0, posinf=255.0,
                                        neginf=0.0), 0, 255).astype(F32)
            yield i, rgb, (pl, info["full_range"], mat)


def scaled_frames(paths):
    """1440p / 4K / 4480x2520 / 8K frames from real frames (mosaics of different frames, mirrored)."""
    from scipy.ndimage import zoom
    frames = []
    for path, idx in paths:
        frames += [rgb for _, rgb, _ in clip_frames(path, want=set(idx))]
    f = frames
    out = []
    a, b, c, d = f[0], f[1][:, ::-1], f[2][::-1], f[3][::-1, ::-1]
    mosaic = np.concatenate([np.concatenate([a, b], 1), np.concatenate([c, d], 1)], 0)       # 3840x2160
    out.append(("mosaic 3840x2160", np.ascontiguousarray(mosaic)))
    out.append(("crop 2560x1440", np.ascontiguousarray(mosaic[360:1800, 640:3200])))
    out.append(("crop 4480x2520 (mosaic of mosaics)", np.ascontiguousarray(
        np.concatenate([np.concatenate([mosaic, mosaic[:, ::-1]], 1)] * 2, 0)[:2520, :4480])))
    big = np.concatenate([np.concatenate([mosaic, mosaic[:, ::-1]], 1),
                          np.concatenate([mosaic[::-1], mosaic[::-1, ::-1]], 1)], 0)         # 7680x4320
    out.append(("mosaic 7680x4320", np.ascontiguousarray(big)))
    up = np.clip(zoom(f[0], (2, 2, 1), order=1), 0, 255).astype(F32)                          # 3840x2160
    out.append(("bilinear x2 3840x2160", up))
    return out


def small_frames(paths):
    frames = []
    for path, idx in paths:
        frames += [(os.path.basename(path), i, rgb) for i, rgb, _ in clip_frames(path, want=set(idx))]
    out = []
    shapes = [(48, 48), (49, 63), (64, 64), (63, 64), (64, 47), (47, 64), (50, 80), (48, 100), (64, 120),
              (300, 47), (48, 300), (200, 64), (96, 96), (72, 128), (40, 40), (8, 8), (7, 100), (130, 130)]
    for k, (h, w) in enumerate(shapes):
        name, i, rgb = frames[k % len(frames)]
        H, W = rgb.shape[:2]
        y = (k * 37) % max(H - h, 1)
        x = (k * 91) % max(W - w, 1)
        out.append((f"{name[:-4]} n{i} crop {w}x{h}@{x},{y}", np.ascontiguousarray(rgb[y:y + h, x:x + w])))
    return out


def synthetic_frames():
    rng = np.random.default_rng(7)
    out = []
    H, W = 540, 960
    yy, xx = np.mgrid[0:H, 0:W]
    base = (40 + 150 * (xx / W) + 30 * np.sin(yy / 23.0) * np.sin(xx / 31.0)).astype(np.float32)
    base[300:480, 600:900] = 110.0
    out.append(("gradient + flat patch", np.clip(np.stack([base] * 3, -1) + rng.normal(0, 3, (H, W, 3)), 0, 255)))
    tex = base + 25 * np.sin(xx / 3.0) * np.cos(yy / 4.0)
    tex[60:300, 80:330] = 70.0
    out.append(("texture + dark flat", np.clip(np.stack([tex] * 3, -1) + rng.normal(0, 2, (H, W, 3)), 0, 255)))
    hi = base.copy()
    hi[0:250, 0:500] = 300.0
    out.append(("blown highlight", np.clip(np.stack([hi, hi * 0.95, hi * 0.9], -1) + rng.normal(0, 3, (H, W, 3)), 0, 255)))
    lb = np.clip(np.stack([base] * 3, -1) + rng.normal(0, 3, (H, W, 3)), 0, 255)
    lb[:70], lb[-70:] = 0, 0
    out.append(("letterbox", lb))
    pb = np.clip(np.stack([base] * 3, -1) + rng.normal(0, 3, (H, W, 3)), 0, 255)
    pb[:, :120], pb[:, -120:] = 16, 16
    out.append(("pillarbox 16", pb))
    col = np.stack([base * 0.9, base, base * 0.7], -1) + rng.normal(0, 1, (H, W, 3)) * [4, 2, 6]
    out.append(("coloured noise RGB", np.clip(col, 0, 255)))
    out.append(("constant 128", np.full((H, W, 3), 128.0)))
    out.append(("constant + noise 0.2", np.clip(128 + rng.normal(0, 0.2, (H, W, 3)), 0, 255)))
    q = np.round(np.clip(np.stack([base] * 3, -1) + rng.normal(0, 1.5, (H, W, 3)), 0, 255))
    out.append(("8-bit quantised", q))
    sat = np.clip(np.stack([base * 2.2] * 3, -1) + rng.normal(0, 4, (H, W, 3)), 0, 255)
    out.append(("mostly saturated", sat))
    # one clipped channel everywhere: every square has n_clip = 1 (status 4 when flat)
    rc = np.stack([np.full((H, W), 255.0), base, base * 0.8], -1) + rng.normal(0, 3, (H, W, 3))
    out.append(("red channel clipped", np.clip(rc, 0, 255)))
    bc = np.stack([base, base * 0.9, np.zeros((H, W))], -1) + rng.normal(0, 2, (H, W, 3))
    out.append(("blue channel at 0", np.clip(bc, 0, 255)))
    # 10..33% clipped pixels in one channel: the 0.1 penalty without n_clip
    pc = np.stack([base + 60, base, base], -1) + rng.normal(0, 3, (H, W, 3))
    pc[..., 0] += 40 * (rng.random((H, W)) < 0.2)
    out.append(("20% red spikes clipped", np.clip(pc, 0, 255)))
    nanf = np.clip(np.stack([base] * 3, -1) + rng.normal(0, 3, (H, W, 3)), 0, 255)
    nanf[200, 300, 1] = np.nan
    out.append(("one NaN pixel", nanf))
    inff = np.clip(np.stack([base] * 3, -1) + rng.normal(0, 3, (H, W, 3)), 0, 255)
    inff[100:104, 700:704, 0] = np.inf
    out.append(("inf block", inff))
    return [(n, np.ascontiguousarray(a, F32)) for n, a in out]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--clip", action="append", default=[], help="PATH or PATH:N (every N-th frame)")
    ap.add_argument("--every", type=int, help="default step for --clip without :N")
    ap.add_argument("--frames", help="frame numbers for every --clip")
    ap.add_argument("--modes", default="1")
    ap.add_argument("--scales", action="store_true")
    ap.add_argument("--small", action="store_true")
    ap.add_argument("--synthetic", action="store_true")
    ap.add_argument("--inter", action="store_true")
    ap.add_argument("--production", action="store_true")
    ap.add_argument("--probe-crash", action="store_true")
    ap.add_argument("--jobs", type=int, default=6)
    A = ap.parse_args()
    require_guard()
    modes = [int(m) for m in A.modes.split(",")]
    root = os.path.join(REPRO, "..")
    clips = []
    for c in A.clip:
        path, sep, step = c.rpartition(":")
        if sep and step.isdigit() and path:
            clips.append((path, int(step)))
        else:
            clips.append((c, A.every))
    probe = [A.probe_crash]

    def probe_once():
        p, probe[0] = probe[0], False
        return p

    def gen():
        for clip, every in clips:
            want = set(int(v) for v in A.frames.split(",")) if A.frames else None
            if not want and not every:
                raise SystemExit(f"--clip {clip}: give :N, --every or --frames")
            for i, rgb, (pl, fr, mat) in clip_frames(clip, want, every):
                yield (f"{os.path.basename(clip)} n={i}", rgb, modes, A.inter, (pl, fr, mat) if A.production else None,
                       False)
        v02, t1 = os.path.join(root, "test_v02.mp4"), os.path.join(root, "test.mp4")
        if A.scales:
            for name, rgb in scaled_frames([(v02, (100, 312, 500, 700))]):
                yield (name, rgb, modes, A.inter, None, False)
        if A.small:
            for name, rgb in small_frames([(v02, (312,)), (t1, (300,))]):
                yield (name, rgb, modes, A.inter, None, min(rgb.shape[:2]) < MIN_SIDE and probe_once())
        if A.synthetic:
            for name, rgb in synthetic_frames():
                yield (name, rgb, modes, A.inter, None, False)

    ok = n = rej = skip = 0
    t0 = time.time()
    with ProcessPoolExecutor(A.jobs) as ex:
        futs = [ex.submit(work, j) for j in gen()]
        for f in futs:
            lines, (o, k, r, sk) = f.result()
            for l in lines:
                print(l, flush=True)
            ok += o
            n += k
            rej += r
            skip += sk
    print(f"{ok}/{n} identical (region and status); genuine rejected {rej}; not given to the genuine (no square fits)"
          f" {skip}; {time.time() - t0:.0f} s")


if __name__ == "__main__":
    main()
