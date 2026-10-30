#!/usr/bin/env python3
"""FindFlatArea status 5 (cancelled): genuine (`nvh flat`, NVH_CANCEL_AT=k) vs the port.

The filter core's vtable slot 0 is the cancel query.  With NVH_CANCEL_AT=k the harness's callback
reports a cancel from its k-th query on (a host-side callback, as when the user aborts a profile;
nothing of the bundle is changed).  The genuine asks it in EstimateNoiseLevelCurve's worker (before
each tile and once more when the scheduler is empty) and at the start of every FindFlatArea size.
For every k checked, the port (nv_flat.find_flat_area_genuine with curve = nv_curve.genuine_noise_curve
and the same counting cancel) must give the same (x, y, w, h, status) and the same number of queries.
Without a cancel (k past the last query) the result must be the uncancelled one.  One FindFlatArea
call per genuine process (the obfuscated check, Agents.md §6.5).
For k up to the query count both sides stop at the k-th query, so the count check mainly tests the total
without a cancel; the positions are tested by the partial squares of the size stage and by "status 5
exactly for k <= total".  The production entry is compared with the port on the same k (checked against
the genuine above) and its host-style RGB with the RGB used here (asserted equal).

usage: verify_cancel.py [--clip ../../../test_v02.mp4:312] [--clip ../../../test.mp4:0] [--jobs 4]
       [--all-k] (every k on the large frames too; default there: the curve's first / middle / last
       queries and every size query)"""
import argparse
import os
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

import numpy as np

sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
REPRO = os.path.join(HERE, "..", "..", "..", "reference", "python")
sys.path.insert(0, REPRO)
sys.path.insert(0, HERE)
import nvfilter as nv  # noqa: E402
import nv_curve  # noqa: E402
import nv_flat  # noqa: E402
from nvh_common import NVH, require_guard  # noqa: E402


def genuine(path, W, H, k, mode=1):
    env = dict(os.environ)
    env["NVH_CANCEL_AT"] = str(k)
    r = subprocess.run([NVH, "flat", str(W), str(H), path, str(mode), "0"], capture_output=True, text=True,
                       env=env, timeout=900)
    if r.returncode:
        raise RuntimeError(f"nvh flat exit {r.returncode}: {r.stderr[-300:]}")
    t = r.stdout.split()
    kv = dict(s.split("=") for s in t if "=" in s)
    q = [l for l in r.stderr.splitlines() if "cancel queries" in l]
    return (int(kv["x"]), int(kv["y"]), int(kv["w"]), int(kv["h"]), int(t[1])), int(q[0].split()[3])


class Counter:
    def __init__(self, k):
        self.k, self.n = k, 0

    def __call__(self):
        self.n += 1
        return self.k > 0 and self.n >= self.k


def port(rgb, k, mode=1):
    c = Counter(k)
    res = nv_flat.find_flat_area_genuine(
        rgb, None, mode, cancel=c,
        curve=lambda: (lambda t: None if t is None else nv_curve.noise_at(t))(
            nv_curve.genuine_noise_curve(rgb, cancel=c)))
    return tuple(int(v) for v in res), c.n


def clip_frame(spec):
    from denoise_video_v5 import probe, read_frames
    path, n = spec.rsplit(":", 1)
    n = int(n)
    info = probe(path)
    mat = nv.matrix_from_tags(info["tags"], info["H"])
    for _, pl in read_frames(path, info["W"], info["H"], n, n + 1, fps=info["fps"]):
        rgb = np.clip(np.nan_to_num(nv.yuv_to_rgb(pl, mat, info["full_range"]), nan=0.0, posinf=255.0,
                                    neginf=0.0), 0, 255)
        return f"{os.path.basename(path)}:{n}", rgb.astype(np.float32), pl, info, mat
    raise SystemExit(f"cannot read {spec}")


def synthetic():
    rng = np.random.default_rng(5)
    out = []
    for name, (h, w), mu, sd in (("noise 200x150", (150, 200), 128, 8), ("noise 64x64", (64, 64), 100, 5),
                                 ("noise 96x400", (96, 400), 60, 3), ("noise 2560x1440", (1440, 2560), 140, 6)):
        out.append((name, np.clip(mu + sd * rng.standard_normal((h, w, 3)), 0, 255).astype(np.float32)))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--clip", action="append", default=None)
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--all-k", action="store_true")
    A = ap.parse_args()
    require_guard()
    clips = A.clip or ["../../../test_v02.mp4:312", "../../../test.mp4:0"]
    cases = [(name, rgb) for name, rgb, *_ in map(clip_frame, clips)] + synthetic()
    tmp = tempfile.mkdtemp(prefix="nv_cancel_")
    ok = tot = 0
    prod_frames = []
    for spec in clips:
        prod_frames.append(clip_frame(spec))
    for name, rgb in cases:
        H, W = rgb.shape[:2]
        path = os.path.join(tmp, "rgb.f32")
        np.ascontiguousarray(np.moveaxis(rgb, -1, 0)).tofile(path)
        g0, q0 = genuine(path, W, H, 0)
        p0, n0 = port(rgb, 0)
        tiles = len(nv_curve.tile_layout(W, H))
        nsz = q0 - (tiles + 1)
        if q0 <= 60 or A.all_k:
            ks = list(range(1, q0 + 2))
        else:
            ks = sorted({1, 2, tiles // 2, tiles, tiles + 1, *range(tiles + 2, q0 + 2)})
        with ThreadPoolExecutor(A.jobs) as ex:
            gs = list(ex.map(lambda k: genuine(path, W, H, k), ks))
        bad = []
        for k, (g, gq) in zip(ks, gs):
            p, pq = port(rgb, k)
            tot += 1
            if g == p and gq == pq:
                ok += 1
            else:
                bad.append((k, g, gq, p, pq))
        st5 = sum(1 for g, _ in gs if g[4] == 5)
        part = sum(1 for g, _ in gs if g[4] == 5 and g[2] > 0)
        print(f"{name:22s} {W}x{H}: uncancelled genuine {g0} port {p0} (queries {q0} / {n0}; {tiles} tiles + 1, "
              f"{nsz} sizes); k = {len(ks)} values: {len(ks) - len(bad)}/{len(ks)} identical "
              f"(status 5 on {st5}, of which {part} with a partial square)", flush=True)
        tot += 1
        ok += int(g0 == p0 and q0 == n0)
        for b in bad[:5]:
            print("   MISMATCH k=%d genuine %s q=%d port %s q=%d" % b)
    os.unlink(path)
    os.rmdir(tmp)
    # production entry: status 5 comes out of nvfilter.find_flat_area and build_profile_v5 raises
    for name, rgb, pl, info, mat in prod_frames:
        H, W = rgb.shape[:2]
        prod_rgb = np.clip(np.nan_to_num(nv.yuv_to_rgb(pl, mat, info["full_range"]), nan=0.0, posinf=255.0,
                                         neginf=0.0), 0, 255).astype(np.float32)
        same_rgb = np.array_equal(prod_rgb.view(np.uint32), rgb.view(np.uint32))
        tot += 1
        ok += int(same_rgb)
        print(f"production {name}: host-style RGB identical to the compared RGB: {same_rgb}")
        tiles = len(nv_curve.tile_layout(W, H))
        for k in (1, tiles + 1, tiles + 3):
            nv._FFA_CACHE.clear()
            c = Counter(k)
            r = nv.find_flat_area(pl, info["full_range"], mat, with_status=True, cancel=c)
            g, gq = port(rgb, k)
            tot += 1
            good = r[3] == 5 and (r[0], r[1], r[2]) == (g[1], g[0], min(g[2], g[3])) and c.n == gq \
                and not nv._FFA_CACHE
            ok += int(good)
            print(f"production {name} k={k}: {r} (port {g}, queries {c.n}/{gq}, cache {len(nv._FFA_CACHE)}) "
                  f"{'ok' if good else 'MISMATCH'}")
        try:
            nv.build_profile_v5(pl, info["full_range"], matrix=mat, cancel=Counter(1))
            print("build_profile_v5 cancelled: no exception  MISMATCH")
            tot += 1
        except nv.ProfileCancelled:
            print("build_profile_v5 cancelled: ProfileCancelled ok")
            tot += 1
            ok += 1
    print(f"{ok}/{tot} checks passed")
    return 0 if ok == tot else 1


if __name__ == "__main__":
    sys.exit(main())
