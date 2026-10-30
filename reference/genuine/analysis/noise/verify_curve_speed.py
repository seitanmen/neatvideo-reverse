#!/usr/bin/env python3
"""Equivalence and timing of the fast path of repro/nv_curve.genuine_noise_curve (batch=True, the
default since 2026-10-16) against the reference path (batch=False: tile by tile through
nv_pixnoise.pixnoise / nv_lines.linefilt, the code verified against the genuine before the speed-up).
No genuine code is run (the genuine comparison is verify_curve.py / verify_curve_shell.py).

  python3 verify_curve_speed.py [--quick] [--no-clips] [--orig [DIR]] [--keep DIR] [--profile]
                                [--mem [--mem-dirs DIR,...] [--mem-runs N]]

Pass criterion: xs, ys, sums (double) and counts bit-identical between the two paths on every image
(a NaN only matches the same NaN bit pattern).  --orig [DIR] additionally runs the modules found in DIR
(a frozen copy of nv_lines.py / nv_pixnoise.py / nv_curve.py from before the change; without DIR
frozen_20261016/ next to this script, the modules of 2026-10-16 before the speed-up, see its README) in a
separate process on the same images and requires the fast path to match them bit for bit as well.
--mem: peak memory and time of the fast path at 1920x1080 (test_v02 n=100), 4096x2160 and 7680x4320 (that
frame tiled), each in fresh processes: ru_maxrss before / after the call and, in a second process, the
tracemalloc peak of the call; also for the module directories of --mem-dirs (their results must be
bit-identical to repro/).

Images (>= 10): test_v02.mp4 n = 0 / 312 / 576 and test.mp4 n = 300 (host RGB as in verify_curve.py);
synthetic gradient + gauss at 1920x1080, 640x360, 97x61, 4096x2160, 48x48 and the odd sizes 101x67,
333x211, 49x50, 1001x777; letterbox and blown highlights at 1920x1080; flat 128 at 640x360; the three
extreme inputs of verify_curve.py (x1e18, 1e20 / 1e25 blocks: NaN sums).  --quick drops 4096x2160,
1001x777 and the clip frames n = 0 / 576.
Reports per image: reference / fast / speed-up, and (--profile) the stage split of the fast path on
test_v02 n=312 (crops, clip weights, luma, pixnoise_batch, accumulation; and how many tiles of earlier
size batches wait for their turn in the accumulation)."""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "reference", "python"))
sys.path.insert(0, HERE)
import nv_curve as ncv  # noqa: E402

F32 = np.float32
RESULTS = []

FROZEN = os.path.join(HERE, "frozen_20261016")

_ORIG_RUNNER = r'''
import sys, time
import numpy as np
sys.dont_write_bytecode = True          # leave the frozen directory as it is
sys.path.insert(0, sys.argv[1])
import nv_curve
assert nv_curve.__file__.startswith(sys.argv[1]), nv_curve.__file__
for path in sys.argv[2:]:
    rgb = np.load(path)
    t = time.perf_counter()
    with np.errstate(all="ignore"):
        xs, ys, sums, counts = nv_curve.genuine_noise_curve(rgb, return_hist=True)
    np.savez(path[:-4] + ".orig.npz", xs=xs, ys=ys, sums=sums, counts=counts, t=time.perf_counter() - t)
    print(path, flush=True)
'''


def check(name, ok, detail=""):
    RESULTS.append((name, bool(ok)))
    print(f"[{'OK ' if ok else 'NG '}] {name}" + (f"  {detail}" if detail else ""), flush=True)
    return bool(ok)


def same(a, b):
    """Bit-identical arrays (dtype, shape and every bit, NaN payloads included)."""
    a, b = np.asarray(a), np.asarray(b)
    return a.dtype == b.dtype and a.shape == b.shape and np.array_equal(a.view(np.uint8), b.view(np.uint8))


def gradient(W, H, seed):
    rng = np.random.default_rng(seed)
    yy, xx = np.mgrid[0:H, 0:W]
    base = 30.0 + 190.0 * xx / max(W - 1, 1) + 10.0 * yy / max(H - 1, 1)
    noisy = np.stack([base, base * 0.95 + 5, base * 0.9 + 12], -1) + rng.normal(0, 4.0, (H, W, 3)) * [1.0, 0.8, 1.3]
    return np.clip(noisy, 0, 255).astype(F32)


def images(A):
    from verify_curve import clip_frames, extreme_images, synthetic_images
    out = []
    if not A.no_clips:
        for path, frames in ((A.v02, "312" if A.quick else "0,312,576"), (A.test, "300")):
            if os.path.exists(path):
                out += clip_frames(path, frames.split(","))
            else:
                print(f"    (warn) {path} not found, skipped")
    sizes = [(1920, 1080), (640, 360), (97, 61), (48, 48), (101, 67), (333, 211), (49, 50)]
    if not A.quick:
        sizes += [(4096, 2160), (1001, 777)]
    for k, (W, H) in enumerate(sizes):
        out.append((f"gradient+gauss {W}x{H}", gradient(W, H, 200 + k)))
    syn = dict(synthetic_images([(1920, 1080), (640, 360)]))
    for name in syn:
        if name.startswith(("letterbox 1920", "highlights 1920", "flat 128 640")):
            out.append((name, syn[name]))
    out += extreme_images()
    return out


def run_orig(orig, cases, workdir):
    """The frozen modules in `orig` on every image (separate process) -> {name: dict}."""
    paths = []
    for i, (_, rgb) in enumerate(cases):
        p = os.path.join(workdir, f"case{i:02d}.npy")
        np.save(p, rgb)
        paths.append(p)
    runner = os.path.join(workdir, "orig_runner.py")
    with open(runner, "w") as f:
        f.write(_ORIG_RUNNER)
    t0 = time.perf_counter()
    r = subprocess.run([sys.executable, runner, os.path.abspath(orig)] + paths, capture_output=True, text=True)
    if r.returncode:
        raise RuntimeError(f"orig runner exit {r.returncode}: {r.stderr[-600:]}")
    print(f"    frozen modules from {orig}: {len(paths)} images in {time.perf_counter() - t0:.0f} s")
    return {name: dict(np.load(p[:-4] + ".orig.npz")) for (name, _), p in zip(cases, paths)}


def profile_stages(rgb):
    """Stage split of tile_histogram_batch on one frame (the same calls, timed one by one), plus the
    largest number of tiles (and kept pixels) waiting for an earlier tile of another size batch."""
    import nv_pixnoise as npx
    H, W = rgb.shape[:2]
    tiles = ncv.tile_layout(W, H)
    planes = [rgb[..., c] for c in range(3)]
    st = dict(crops=0.0, clip_weight_batch=0.0, luma_601_batch=0.0, pixnoise_batch=0.0, accumulate=0.0)
    sw = np.lib.stride_tricks.sliding_window_view
    sums = np.zeros(ncv.NBINS)
    counts = np.zeros(ncv.NBINS, np.int64)
    pending, nxt, most, most_px = {}, 0, 0, 0
    bin_t = np.min_scalar_type(ncv.NBINS - 1)          # as tile_histogram_batch keeps waiting bins (uint8)
    t_all = time.perf_counter()
    for idx in sorted(ncv._tile_batches(tiles), key=min):
        w, h = tiles[idx[0]][2], tiles[idx[0]][3]
        t0 = time.perf_counter()
        ys = np.array([tiles[t][1] for t in idx])
        xs = np.array([tiles[t][0] for t in idx])
        cR, cG, cB = (np.asarray(sw(P, (h, w))[ys, xs], F32) for P in planes)
        t1 = time.perf_counter()
        cw = ncv.clip_weight_batch(cR, cB, G=cG)
        t2 = time.perf_counter()
        luma = npx.luma_601_batch(cR, cG, cB)
        t3 = time.perf_counter()
        noise, weight, bright = npx.pixnoise_batch(luma, cw)
        t4 = time.perf_counter()
        for t, ti in enumerate(idx):
            d = tiles[ti]
            core = (t, slice(d[9], d[9] + d[7]), slice(d[8], d[8] + d[6]))
            keep = ~(weight[core] < F32(0.5)).ravel()
            kb = ncv.bin_index(bright[core].ravel()[keep])
            kn = noise[core].ravel()[keep]
            if ti != nxt:
                pending[ti] = (kb.astype(bin_t), kn)
                most = max(most, len(pending))
                most_px = max(most_px, sum(v[1].size for v in pending.values()))
                continue
            ncv._add_kept(sums, counts, kb, kn, ncv.NBINS)
            nxt += 1
            while nxt in pending:
                ncv._add_kept(sums, counts, *pending.pop(nxt), ncv.NBINS)
                nxt += 1
        t5 = time.perf_counter()
        for k, v in zip(st, (t1 - t0, t2 - t1, t3 - t2, t4 - t3, t5 - t4)):
            st[k] += v
    t0 = time.perf_counter()
    ncv.curve_table(sums, counts, W, H)
    st["curve_table"] = time.perf_counter() - t0
    total = time.perf_counter() - t_all
    batches = [len(i) for i in sorted(ncv._tile_batches(tiles), key=min)]
    print(f"    stage split ({len(tiles)} tiles in batches {batches}; total {total:.3f} s): "
          + ", ".join(f"{k} {v:.3f}" for k, v in st.items())
          + f"; at most {most} tiles ({most_px} kept pixels) waiting for an earlier tile")
    return sums, counts


_MEM_RUNNER = r'''
import resource, sys, time
sys.dont_write_bytecode = True
sys.path.insert(0, sys.argv[1])
import numpy as np
import nv_curve
assert nv_curve.__file__.startswith(sys.argv[1]), nv_curve.__file__
img = np.load(sys.argv[2])
if sys.argv[3] == "trace":
    import tracemalloc
    tracemalloc.start()
unit = 1 if sys.platform == "darwin" else 1024          # ru_maxrss: bytes on macOS, KiB on Linux
base = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss * unit
t = time.perf_counter()
with np.errstate(all="ignore"):
    xs, ys, sums, counts = nv_curve.genuine_noise_curve(img, return_hist=True)
t = time.perf_counter() - t
peak = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss * unit
traced = tracemalloc.get_traced_memory()[1] if sys.argv[3] == "trace" else 0
np.savez(sys.argv[4], xs=xs, ys=ys, sums=sums, counts=counts)
print(t, base, peak, traced)
'''


def memory(A, workdir):
    """--mem: cold-call time, ru_maxrss before / after and the tracemalloc peak of genuine_noise_curve (fast
    path) in fresh processes, at 1920x1080, 4096x2160 and 7680x4320."""
    from verify_curve import clip_frames
    if not os.path.exists(A.v02):
        print(f"    (warn) {A.v02} not found: --mem skipped")
        return
    fr = clip_frames(A.v02, ["100"])[0][1]
    imgs = [("1920x1080 test_v02 n=100", fr), ("4096x2160 (n=100 tiled)", np.tile(fr, (2, 3, 1))[:2160, :4096]),
            ("7680x4320 (n=100 tiled)", np.tile(fr, (4, 4, 1)))]
    runner = os.path.join(workdir, "mem_runner.py")
    with open(runner, "w") as f:
        f.write(_MEM_RUNNER)
    dirs = [os.path.join(ROOT, "reference", "python")] + [os.path.abspath(d) for d in A.mem_dirs.split(",") if d]
    for k, (label, img) in enumerate(imgs):
        path = os.path.join(workdir, f"mem{k}.npy")
        np.save(path, np.ascontiguousarray(img, F32))
        ref = None
        for d in dirs:
            for run in range(A.mem_runs):
                res = {}
                for mode in ("rss", "trace"):
                    out = os.path.join(workdir, f"mem{k}_{mode}.npz")
                    r = subprocess.run([sys.executable, runner, d, path, mode, out], capture_output=True, text=True)
                    if r.returncode:
                        raise RuntimeError(f"mem runner exit {r.returncode}: {r.stderr[-600:]}")
                    res[mode] = [float(v) for v in r.stdout.split()]
                    got = dict(np.load(out))
                    ref = ref or got
                    if not all(same(ref[q], got[q]) for q in ("xs", "ys", "sums", "counts")):
                        raise RuntimeError(f"{d}: results differ from {dirs[0]} on {label}")
                t, base, peak, _ = res["rss"]
                mib = 2.0 ** 20
                print(f"    memory {label} [{os.path.relpath(d, ROOT)}]: {t:.2f} s (cold call), maxrss {base / mib:.0f} -> "
                      f"{peak / mib:.0f} MiB (+{(peak - base) / mib:.0f}), traced peak {res['trace'][3] / mib:.0f} MiB "
                      f"(input {img.nbytes / mib:.0f} MiB)", flush=True)
        os.unlink(path)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--no-clips", action="store_true")
    ap.add_argument("--orig", nargs="?", const=FROZEN, default=None, metavar="DIR",
                    help="directory with frozen pre-change nv_lines.py / nv_pixnoise.py / nv_curve.py "
                         f"(without DIR: {os.path.relpath(FROZEN, HERE)})")
    ap.add_argument("--keep", help="directory for the image dumps of --orig (default: a temp dir, removed)")
    ap.add_argument("--profile", action="store_true", help="stage split of the fast path on the first 1080p clip frame")
    ap.add_argument("--repeat", type=int, default=3, help="timing runs of the fast path per image (best is reported)")
    ap.add_argument("--mem", action="store_true", help="peak memory of the fast path at 1080p / 4096x2160 / 7680x4320")
    ap.add_argument("--mem-dirs", default="", help="further module directories for --mem (comma-separated)")
    ap.add_argument("--mem-runs", type=int, default=1, help="fresh-process runs per size and directory (--mem)")
    ap.add_argument("--v02", default=os.path.join(ROOT, "test_v02.mp4"))
    ap.add_argument("--test", default=os.path.join(ROOT, "test.mp4"))
    A = ap.parse_args()
    t_all = time.perf_counter()
    cases = images(A)
    print(f"{len(cases)} images")
    orig = None
    workdir = None
    if A.orig or A.mem:
        workdir = A.keep or tempfile.mkdtemp(prefix="nvcurve_speed_")
        os.makedirs(workdir, exist_ok=True)
    if A.orig:
        for m in ("nv_curve.py", "nv_pixnoise.py", "nv_lines.py"):
            if not os.path.exists(os.path.join(A.orig, m)):
                raise SystemExit(f"--orig {A.orig}: {m} missing")
        orig = run_orig(A.orig, cases, workdir)
    t1080 = []
    for name, rgb in cases:
        with np.errstate(all="ignore"):
            t0 = time.perf_counter()
            ref = ncv.genuine_noise_curve(rgb, return_hist=True, batch=False)
            tr = time.perf_counter() - t0
            tf = []
            for _ in range(max(1, A.repeat)):
                t0 = time.perf_counter()
                fast = ncv.genuine_noise_curve(rgb, return_hist=True)
                tf.append(time.perf_counter() - t0)
        ok = all(same(a, b) for a, b in zip(ref, fast))
        detail = f"counts {fast[3].tolist()}"
        if orig is not None:
            o = orig[name]
            ok_o = all(same(o[k], v) for k, v in zip(("xs", "ys", "sums", "counts"), fast))
            detail += f"; frozen modules {float(o['t']):.2f} s, identical: {ok_o}"
            ok &= ok_o
        nnan = int(np.isnan(fast[1]).sum())
        H, W = rgb.shape[:2]
        if (W, H) == (1920, 1080):
            t1080.append((tr, min(tf)))
        check(f"{name}: reference {tr:.2f} s, fast {min(tf):.3f} s (x{tr / min(tf):.1f}); xs, ys, sums, counts "
              f"bit-identical{f' ({nnan} NaN ys)' if nnan else ''}", ok, detail)
    if A.profile:
        frame = next((rgb for name, rgb in cases if rgb.shape[:2] == (1080, 1920)), None)
        if frame is not None:
            with np.errstate(all="ignore"):
                s_, c_ = profile_stages(frame)
                ref = ncv.genuine_noise_curve(frame, return_hist=True)
            check("profile replica == tile_histogram_batch (sums, counts)", same(s_, ref[2]) and same(c_, ref[3]))
    if A.mem:
        memory(A, workdir)
    n_ok = sum(1 for _, ok in RESULTS if ok)
    if t1080:
        tr, tf = np.mean([t[0] for t in t1080]), np.mean([t[1] for t in t1080])
        print(f"1080p (n={len(t1080)}): reference {tr:.2f} s, fast {tf:.3f} s per frame (x{tr / tf:.1f})")
    print(f"\n{n_ok}/{len(RESULTS)} checks passed ({len(cases)} images: reference vs fast"
          f"{', and the frozen modules' if orig else ''}{'; profile replica' if A.profile else ''}); "
          f"total {time.perf_counter() - t_all:.0f} s")
    if workdir and not A.keep:
        shutil.rmtree(workdir, ignore_errors=True)
    sys.exit(0 if RESULTS and n_ok == len(RESULTS) else 1)


if __name__ == "__main__":
    main()
