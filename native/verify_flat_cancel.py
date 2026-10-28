# Bit-exact verification of the native FindFlatArea cancel query (status 5, native/nv_flat.{h,cpp} and
# nvc::genuine_noise_curve's cancel) against the Python port (repro/nv_flat.py find_flat_area_genuine
# with cancel= and curve=, repro/nv_curve.py genuine_noise_curve(cancel=)).
#
#   make -C native && python3 native/verify_flat_cancel.py [--jobs J] [--all-k]
#
# The same idea as neatvideo_ofx_614/analysis/noise/verify_cancel.py (genuine vs Python), here Python vs
# C++: the counting cancel query reports a cancel from its k-th query on (k = 0: never, only counted).
# The C++ driver reads k from the environment variable NV_FLAT_CANCEL_AT (nv_flat_test cancel).
# For every case and k, (x, y, w, h, status) and the number of queries must be identical, with the C++
# on -j 1 and -j J (the threaded curve takes its tiles under the lock of the query).
# k: 0 (uncancelled; also compared with the non-cancel entry `nv_flat_test flat`), 1, 2, the middle of the
# curve's queries, tiles - 1, tiles, tiles + 1 (the curve's last query), every size query, total - 1,
# total, total + 1 (past the last query: the uncancelled result).  --all-k: every k from 1 to total + 1.
# Cases: 8 real frames (test_v02.mp4 n = 0 / 312 / 576 / 700, test.mp4 n = 0 / 300 / 450 / 750; the frame
# cache of verify_flat.py) in colour mode 1, two of them also in modes 0 / 2, two with an explicit noise
# table (no curve queries: sizes only), and small synthetic frames (verify_cancel.py's 200x150 / 64x64 /
# 96x400 noise) with every k.
import argparse
import os
import struct
import subprocess
import sys
import tempfile
import time
from multiprocessing import Pool

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, os.path.join(ROOT, "repro"))
sys.path.insert(0, HERE)
sys.dont_write_bytecode = True

import numpy as np  # noqa: E402

import verify_flat as vf  # noqa: E402  (frame cache, case blobs)

F32 = np.float32
F64 = np.float64
EXE = os.environ.get("NV_FLAT_EXE", os.path.join(HERE, "nv_flat_test"))
OUT = os.path.join(ROOT, "sandbox", "omo", "wp_native_flat_cancel", "tmp")

FRAMES = [("test_v02.mp4", n) for n in (0, 312, 576, 700)] + [("test.mp4", n) for n in (0, 300, 450, 750)]


class Counter:
    """verify_cancel.py's Counter: true from the k-th query on (k = 0: never)."""
    def __init__(self, k):
        self.k, self.n = k, 0

    def __call__(self):
        self.n += 1
        return self.k > 0 and self.n >= self.k


def image(spec):
    if spec[0] == "frame":
        return vf.make_image(spec)
    _, W, H, mu, sd, seed = spec
    rng = np.random.default_rng(seed)
    return np.clip(mu + sd * rng.standard_normal((H, W, 3)), 0, 255).astype(F32)


def py_run(args):
    import nv_curve as ncv
    import nv_flat as nf
    spec, mode, table, k = args
    img = image(spec)
    c = Counter(k)
    with np.errstate(all="ignore"):
        if table is None:
            res = nf.find_flat_area_genuine(
                img, None, mode, cancel=c,
                curve=lambda: (lambda t: None if t is None else ncv.noise_at(t))(ncv.genuine_noise_curve(img, cancel=c)))
        else:
            res = nf.find_flat_area_genuine(img, ncv.noise_at(table), mode, cancel=c)
    return tuple(int(v) for v in res), c.n


def py_table(spec):
    import nv_curve as ncv
    return ncv.genuine_noise_curve(image(spec))


def cpp_run(blob, k, jobs, cmd="cancel"):
    fd, outp = tempfile.mkstemp(prefix="cancel_", suffix=".out", dir=OUT)
    os.close(fd)
    env = dict(os.environ, NV_FLAT_CANCEL_AT=str(k))
    subprocess.run([EXE, cmd, blob, outp, "-j", str(jobs)], check=True, env=env)
    data = open(outp, "rb").read()
    os.remove(outp)
    if cmd == "cancel":
        v = struct.unpack("<6i", data)
        return tuple(v[:5]), v[5]
    return tuple(struct.unpack_from("<5i", data)), None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", type=int, default=3, help="Python processes / C++ threads (max 3)")
    ap.add_argument("--all-k", action="store_true", help="every k on the real frames too")
    A = ap.parse_args()
    jobs = max(1, min(A.jobs, 3))
    if not os.path.exists(EXE):
        print(f"{EXE} not found: make -C native")
        return 2
    os.makedirs(OUT, exist_ok=True)
    t_all = time.perf_counter()
    vf.extract_frames()
    cases = []                                   # (name, spec, mode, table)
    for video, n in FRAMES:
        cases.append((f"{video} n={n} mode 1", ("frame", video, n), 1, None))
    for video, n in (FRAMES[1], FRAMES[5]):
        for mode in (0, 2):
            cases.append((f"{video} n={n} mode {mode}", ("frame", video, n), mode, None))
    with Pool(jobs) as pool:
        tabs = pool.map(py_table, [("frame", *FRAMES[1]), ("frame", *FRAMES[5])])
    for (video, n), t in zip((FRAMES[1], FRAMES[5]), tabs):
        cases.append((f"{video} n={n} mode 1, explicit noise table", ("frame", video, n), 1,
                      (np.asarray(t[0], F64), np.asarray(t[1], F64))))
    for name, W, H, mu, sd, seed in (("noise 200x150", 200, 150, 128, 8, 1), ("noise 64x64", 64, 64, 100, 5, 2),
                                     ("noise 96x400", 400, 96, 60, 3, 3)):
        cases.append((f"{name} mode 1", ("syn", W, H, mu, sd, seed), 1, None))
    import nv_curve as ncv
    checks = fails = 0
    t_py = t_cpp = 0.0
    with Pool(jobs) as pool:
        for name, spec, mode, table in cases:
            img = image(spec)
            H, W = img.shape[:2]
            fd, blob = tempfile.mkstemp(prefix="cancel_", suffix=".in", dir=OUT)
            with os.fdopen(fd, "wb") as f:
                f.write(struct.pack("<Q", 1))
                vf.write_case(f, img, mode, table, False)
            del img
            t0 = time.perf_counter()
            p0, q0 = py_run((spec, mode, table, 0))
            t_py += time.perf_counter() - t0
            tiles = 0 if table is not None else len(ncv.tile_layout(W, H)) + 1     # curve queries
            nsz = q0 - tiles
            if A.all_k or spec[0] != "frame":
                ks = list(range(0, q0 + 2))
            else:
                t = tiles - 1                                                    # tiles of the curve
                ks = sorted({k for k in (0, 1, 2, t // 2, t - 1, t, t + 1, *range(tiles + 1, q0 + 2), q0 - 1, q0,
                                         q0 + 1) if k >= 0})
            t0 = time.perf_counter()
            pys = pool.map(py_run, [(spec, mode, table, k) for k in ks], chunksize=1)
            t_py += time.perf_counter() - t0
            bad = []
            st5 = part = 0
            t0 = time.perf_counter()
            for k, (p, pq) in zip(ks, pys):
                for j in sorted({1, jobs}):
                    g, gq = cpp_run(blob, k, j)
                    checks += 1
                    if g != p or gq != pq:
                        bad.append((k, j, p, pq, g, gq))
                st5 += p[4] == 5
                part += p[4] == 5 and p[2] > 0
                # status 5 exactly for 1 <= k <= total queries; the uncancelled result past them
                checks += 1
                exp5 = 1 <= k <= q0
                if (p[4] == 5) != exp5 or (not exp5 and p != p0) or pq != (q0 if not exp5 else k):
                    bad.append((k, "py", p, pq, p0, q0))
            for j in sorted({1, jobs}):                                          # regression: no cancel
                g, _ = cpp_run(blob, 0, j, cmd="flat")
                checks += 1
                if g != p0:
                    bad.append((0, f"flat -j {j}", p0, q0, g, None))
            t_cpp += time.perf_counter() - t0
            os.remove(blob)
            fails += len(bad)
            print(f"{name:48s} {W}x{H}: uncancelled {p0}, {q0} queries ({tiles} curve + {nsz} sizes); "
                  f"k = {len(ks)} values x -j {sorted({1, jobs})}: {'ok' if not bad else f'{len(bad)} MISMATCHES'} "
                  f"(status 5 on {st5}, {part} with a partial square)", flush=True)
            for b in bad[:6]:
                print("   MISMATCH k=%s run %s: Python %s q=%s  C++/expected %s q=%s" % b)
    print(f"cancel: {checks - fails}/{checks} checks identical; Python {t_py:.1f} s, C++ {t_cpp:.1f} s (wall)")
    print(f"total {time.perf_counter() - t_all:.1f} s: " + ("ALL IDENTICAL" if fails == 0 else f"{fails} MISMATCHES"))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
