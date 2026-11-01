"""N4e: nvc_match_score (native/nvcore_c_match.cpp, libnvcore.dylib) == match_ref.py, bit for bit.

  python3 native/verify_match.py [--frames N] [--models M] [--seed S] [--real N]   (make -C native verify runs it)

For every input frame the Python tile models are computed once; each frame is scored against a set of models:
real ones (nvc_profile_build on areas of other frames: the verified C BuildNoiseModel port, colour mode 1, with the
reference floor; the same call the UI makes) and perturbed copies (words scaled, zeroed, negated, NaN / inf).  The
C call runs with threads 1 and 4.  Compared: the status, info, every score (the double's bits) and accept count.
Also: too small frames (status 1), all-clipped frames (status 4), a cancel at query k (status 2), the n = 0 call,
and nvc_match_model == match_ref.match_model on the same profile bytes.
Exit 0 when everything matches.
"""
import argparse
import ctypes as C
import os
import subprocess
import sys
import time

import numpy as np

_HERE = os.path.dirname(os.path.abspath(__file__))
_ROOT = os.path.abspath(os.path.join(_HERE, ".."))
sys.path.insert(0, _HERE)
import match_ref as mr          # noqa: E402

F32 = np.float32
LIB = C.CDLL(os.path.join(_ROOT, "native", "libnvcore.dylib"))
CANCEL = C.CFUNCTYPE(C.c_int)
LIB.nvc_match_score.restype = C.c_int
LIB.nvc_match_score.argtypes = [C.c_void_p, C.c_int, C.c_int, C.c_void_p, C.c_void_p, C.c_int, C.c_void_p, C.c_void_p,
                                C.c_int, CANCEL, C.c_void_p, C.c_void_p, C.c_void_p]
LIB.nvc_match_model.restype = None
LIB.nvc_match_model.argtypes = [C.c_void_p, C.c_void_p, C.c_void_p]
LIB.nvc_profile_build.restype = C.c_int
LIB.nvc_profile_build.argtypes = [C.c_void_p, C.c_int, C.c_int, C.c_int, C.c_int, C.c_int, C.c_int, C.c_int,
                                  C.c_void_p, C.c_int, C.c_int, C.c_void_p]
FLOOR = np.ascontiguousarray(np.load(os.path.join(_ROOT, "data", "ref_floor.npy")), F32)


def ptr(a):
    return a.ctypes.data_as(C.c_void_p)


def c_score(rgb, ref, faces_list, stats_list, threads, cancel=None):
    rgb = np.ascontiguousarray(rgb, F32)
    H, W = rgb.shape[:2]
    n = len(faces_list)
    fa = np.ascontiguousarray(np.array([np.concatenate(f) for f in faces_list], F32).reshape(-1)) if n else np.zeros(1, F32)
    st = np.ascontiguousarray(np.array([np.concatenate(s) for s in stats_list], F32).reshape(-1)) if n else np.zeros(1, F32)
    sc = np.zeros(max(n, 1), np.float64)
    ac = np.zeros(max(n, 1), np.int32)
    info = np.zeros(3, np.int32)
    rf = np.ascontiguousarray(np.concatenate(ref[0]), F32)
    rs = np.ascontiguousarray(np.concatenate(ref[1]), F32)
    cb = CANCEL(cancel) if cancel else CANCEL()
    r = LIB.nvc_match_score(ptr(rgb), W, H, ptr(rf), ptr(rs), n, ptr(fa), ptr(st), threads, cb, ptr(sc), ptr(ac),
                            ptr(info))
    return r, list(sc[:n]), [int(x) for x in ac[:n]], tuple(int(x) for x in info)


def c_profile(rgb, x, y, w, h):
    rgb = np.ascontiguousarray(rgb, F32)
    out = np.zeros(0xc300, np.uint8)
    st = LIB.nvc_profile_build(ptr(rgb), rgb.shape[1], rgb.shape[0], 1, x, y, w, h, ptr(FLOOR), 0, 1, ptr(out))
    return st, out


def c_model(prof):
    fa = np.zeros(3 * mr.NFACE, F32)
    st = np.zeros(3 * mr.NSTATS, F32)
    LIB.nvc_match_model(ptr(np.ascontiguousarray(prof, np.uint8)), ptr(fa), ptr(st))
    return [fa[i * mr.NFACE:(i + 1) * mr.NFACE] for i in range(3)], [st[i * mr.NSTATS:(i + 1) * mr.NSTATS] for i in range(3)]


def bits(x):
    return np.float64(x).view(np.uint64).item()


def synth(rng, H, W):
    """A noisy RGB frame: smooth background + noise of a random level, optional structure, clipping, specials."""
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float64)
    base = rng.uniform(20, 230) + rng.uniform(-40, 40) * (xx / W) + rng.uniform(-40, 40) * (yy / H)
    img = np.empty((H, W, 3), np.float64)
    sig = rng.choice([0.5, 1.5, 3.0, 6.0, 12.0, 25.0])
    for c in range(3):
        img[..., c] = base * rng.uniform(0.7, 1.3) + rng.normal(0, sig * rng.uniform(0.5, 1.5), (H, W))
    kind = rng.integers(0, 6)
    if kind == 1:                      # structure (edges / texture)
        img += 40 * (np.sin(xx / rng.uniform(2, 9)) > 0)[..., None]
    elif kind == 2:                    # a clipped block
        y0, x0 = rng.integers(0, H // 2), rng.integers(0, W // 2)
        img[y0:y0 + H // 2, x0:x0 + W // 2] = 255.0
    elif kind == 3:                    # constant tiles
        img[: H // 2, : W // 3] = 77.0
    img = np.clip(img, 0, 255).astype(F32)
    if kind == 4:                      # specials
        n = rng.integers(1, 6)
        idx = rng.integers(0, H * W * 3, n)
        img.reshape(-1)[idx] = rng.choice([np.nan, np.inf, -np.inf, 1e30], n).astype(F32)
    if kind == 5:                      # 8-bit quantised (as the 8-bit path)
        img = np.round(img).astype(F32)
    return img


def perturb(rng, faces, stats):
    F = [f.copy() for f in faces]
    S = [s.copy() for s in stats]
    kind = rng.integers(0, 7)
    c = rng.integers(0, 3)
    if kind == 0:
        F = [f * F32(rng.uniform(0.2, 5.0)) for f in F]          # level scale
    elif kind == 1:
        F[c][rng.integers(0, 492, 8)] = F32(0.0)
    elif kind == 2:
        F[c][rng.integers(0, 492, 8)] *= F32(-1.0)
    elif kind == 3:
        F[c][rng.choice([5, 6, 16, 240, 490])] = F32(np.nan)
    elif kind == 4:
        F[c][rng.choice([5, 6, 0, 1])] = F32(np.inf)
    elif kind == 5:
        S[c] = S[c] * F32(rng.uniform(0.0, 3.0))
    else:
        F[c] = (F[c] * rng.uniform(0.8, 1.25, 492)).astype(F32)
    return F, S


def own_model(rng, img, pool):
    """The frame's own model as the UI makes it (BuildNoiseModel of a square of the frame), else one of the pool."""
    H, W = img.shape[:2]
    if min(H, W) >= 48 and rng.random() < 0.8 and np.isfinite(img).all():
        S = int(rng.integers(32, min(H, W, 128) + 1))
        st, prof = c_profile(img, int(rng.integers(0, W - S + 1)), int(rng.integers(0, H - S + 1)), S, S)
        if st == 0:
            return mr.match_model(prof)
    f, s = pool[int(rng.integers(0, len(pool)))]
    return perturb(rng, f, s) if rng.random() < 0.5 else (f, s)


def real_frames(n):
    """n frames of test.mp4 (640x360 RGB, ffmpeg rgb24) cropped to random sizes."""
    src = os.path.join(_ROOT, "test.mp4")
    out = []
    for k in range(n):
        raw = subprocess.run(["ffmpeg", "-v", "error", "-ss", str(3 * k + 1), "-i", src, "-frames:v", "1", "-f", "rawvideo",
                              "-pix_fmt", "rgb24", "-"], capture_output=True, check=True).stdout
        out.append(np.frombuffer(raw, np.uint8).reshape(360, 640, 3).astype(F32))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--frames", type=int, default=24)
    ap.add_argument("--models", type=int, default=40)
    ap.add_argument("--seed", type=int, default=20261101)
    ap.add_argument("--real", type=int, default=3, help="crops of test.mp4 frames")
    a = ap.parse_args()
    rng = np.random.default_rng(a.seed)
    bad = 0
    t0 = time.time()

    # the model pool: nvc_profile_build on areas of synthetic and real frames
    pool = []
    srcs = [synth(rng, 160, 192) for _ in range(6)] + real_frames(2)
    for img in srcs:
        H, W = img.shape[:2]
        for _ in range(3):
            w = int(rng.integers(48, min(W, 160) + 1))
            h = int(rng.integers(48, min(H, 160) + 1))
            x, y = int(rng.integers(0, W - w + 1)), int(rng.integers(0, H - h + 1))
            st, prof = c_profile(img, x, y, w, h)
            if st != 0:
                continue
            fC, sC = c_model(prof)
            fP, sP = mr.match_model(prof)
            ok = all(np.array_equal(fC[i].view(np.uint32), fP[i].view(np.uint32)) and
                     np.array_equal(sC[i].view(np.uint32), sP[i].view(np.uint32)) for i in range(3))
            if not ok:
                bad += 1
                print("nvc_match_model differs")
            pool.append((fP, sP))
    print(f"model pool: {len(pool)} profiles", flush=True)

    frames = [synth(rng, int(rng.integers(48, 200)), int(rng.integers(48, 240))) for _ in range(a.frames)]
    for img in real_frames(a.real):
        h, w = int(rng.integers(64, 361)), int(rng.integers(64, 641))
        y, x = int(rng.integers(0, 361 - h)), int(rng.integers(0, 641 - w))
        frames.append(np.ascontiguousarray(img[y:y + h, x:x + w]))
    nscore = 0
    for fi, img in enumerate(frames):
        models = []
        for _ in range(a.models):
            f, s = pool[int(rng.integers(0, len(pool)))]
            models.append(perturb(rng, f, s) if rng.random() < 0.5 else (f, s))
        fl = [m[0] for m in models]
        sl = [m[1] for m in models]
        ref = own_model(rng, img, pool)
        fl.append(ref[0])                      # the reference itself among the models: score 1 (or 0 when den = 0)
        sl.append(ref[1])
        ps, psc, pac, pinfo = mr.match_score(img, ref, fl, sl)
        for th in (1, 4):
            cs, csc, cac, cinfo = c_score(img, ref, fl, sl, th)
            same = cs == ps and cinfo == pinfo and cac == pac and [bits(x) for x in csc] == [bits(x) for x in psc]
            if not same:
                bad += 1
                print(f"frame {fi} {img.shape} threads {th}: C {cs} {cinfo} vs Py {ps} {pinfo}")
                for k in range(len(fl)):
                    if bits(csc[k]) != bits(psc[k]) or cac[k] != pac[k]:
                        print(f"  model {k}: C {csc[k]!r} {cac[k]} Py {psc[k]!r} {pac[k]}")
        nscore += len(fl)
        print(f"frame {fi:2d} {img.shape[1]}x{img.shape[0]} status {ps} tiles {pinfo} "
              f"scores {min(psc):.4f}..{max(psc):.4f} ({time.time() - t0:.0f} s)", flush=True)

    # status paths
    small = synth(rng, 47, 200)
    fl, sl = [pool[0][0]], [pool[0][1]]
    ref = pool[1]
    for img, want in ((small, 1), (synth(rng, 200, 47), 1), (np.full((96, 96, 3), 255.0, F32), 4)):
        p = mr.match_score(img, ref, fl, sl)
        for th in (1, 3):
            c = c_score(img, ref, fl, sl, th)
            if c[0] != want or p[0] != want or c[3] != p[3] or [bits(x) for x in c[1]] != [bits(x) for x in p[1]]:
                bad += 1
                print(f"status path {img.shape}: C {c} Py {p[:1]} want {want}")
    img = synth(rng, 128, 160)              # 20 tiles: a cancel at query k
    for k in (1, 2, 7, 20):
        cnt = [0]

        def cancel():
            cnt[0] += 1
            return 1 if cnt[0] >= k else 0
        c = c_score(img, ref, fl, sl, 1, cancel)
        if c[0] != 2 or cnt[0] != k:
            bad += 1
            print(f"cancel at {k}: status {c[0]}, {cnt[0]} queries")
    c = c_score(img, ref, [], [], 2)       # n = 0: only the tile counts
    p = mr.match_score(img, ref, [], [])
    if c[0] != p[0] or c[3] != p[3]:
        bad += 1
        print(f"n = 0: C {c} Py {p}")
    print(f"{nscore} scores x 2 thread counts on {len(frames)} frames, model pool {len(pool)}, "
          f"{'ALL MATCH' if bad == 0 else f'{bad} DIFFER'} ({time.time() - t0:.0f} s)")
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
