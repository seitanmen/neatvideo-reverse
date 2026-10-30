# Bit-exact verification of the native FindFlatArea port (native/nv_flat.{h,cpp}) against the Python
# port (repro/nv_flat.py find_flat_area_genuine / transform, with nv_curve / nv_band / nv_pixnoise).
#
#   make -C native -f Makefile.flat && python3 native/verify_flat.py [--quick] [--jobs J] [--fuzz N]
#
# Every case runs both ports; all of the following are compared BIT FOR BIT (no tolerance):
#   result      (x, y, w, h, status) - the pass criterion
#   steps       reduction factor s, reduced size, first fitting size index i0, whether the curve ran
#   curve       the 8-point EstimateNoiseLevelCurve table (xs, ys f64) the C++ computed itself
#   reduce      reduced R, G, B planes and their BT.601 luma (FUN_009b7e58 / FUN_009f11fc)
#   cand        FUN_009999cc's 64 (x, y, mean, score) of every size, with r and step
#   placed      the placed square (FUN_009c7318 crop) and IsClip of R, G, B of every candidate
#   trace       every scored candidate: size, index, square, nclip, penalty, mean, noise, 3 flatness
#               values, E, R, score (nv_flat's trace dicts, floats re-cast to f32)
#   structs     (cases marked deep) the 3 x 4096 band-measure words of every scored candidate,
#               against nv_flat.transform + nv_band.band_levels per plane (the single-plane path,
#               while find_flat_area_genuine uses the batched one)
# Cases: 20 real frames (test_v02.mp4 x 12, test.mp4 x 8; ffmpeg rgb48le / 257 as quality_draft._frame)
# x colour modes 0 / 1 / 2, synthetic frames (letterbox, pillarbox, clipping, saturated channels,
# constant, quantised ties, periodic ties, NaN / inf, NaN payloads, size-table boundaries 0x280000 /
# 0xA00000, 4K, s = 9, sizes around 48 / 64 px, explicit noise tables, mode 3) and seeded random images.
# The C++ side runs with -j 1 and, for every case, again with -j J (threaded candidates and curve:
# must give the same bits).
import argparse
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
sys.path.insert(0, os.path.join(ROOT, "reference", "python"))
sys.path.insert(0, HERE)
sys.dont_write_bytecode = True

import numpy as np  # noqa: E402

F32 = np.float32
F64 = np.float64
U32 = np.uint32
TMP = os.environ.get("NV_FLAT_TMP", os.path.join(ROOT, "work", "omo", "wp_native_flat", "tmp"))
EXE = os.environ.get("NV_FLAT_EXE", os.path.join(HERE, "nv_flat_test"))   # override: mutation runs

FRAMES = {"test_v02.mp4": (0, 48, 100, 150, 200, 260, 312, 380, 450, 520, 576, 700),
          "test.mp4": (0, 120, 240, 300, 450, 600, 750, 870)}
DEEP_FRAMES = {("test_v02.mp4", 312), ("test.mp4", 300)}


# ----------------------------------------------------------------------------- inputs
def _cache(video, n):
    return os.path.join(TMP, f"{os.path.splitext(video)[0]}_n{n}.npy")


def extract_frames():
    """One ffmpeg pass per video (select of all wanted frames, rgb48le / 257 as quality_draft._frame)."""
    os.makedirs(TMP, exist_ok=True)
    for video, ns in FRAMES.items():
        missing = [n for n in ns if not os.path.exists(_cache(video, n))]
        if not missing:
            continue
        src = os.path.join(ROOT, video)
        wh = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
                             "stream=width,height", "-of", "csv=p=0", src],
                            check=True, capture_output=True, text=True).stdout.strip().split(",")
        W, H = int(wh[0]), int(wh[1])
        sel = "+".join(f"eq(n\\,{n})" for n in missing)
        raw = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-i", src, "-vf", f"select={sel}",
                              "-fps_mode", "passthrough", "-f", "rawvideo", "-pix_fmt", "rgb48le", "-"],
                             check=True, capture_output=True).stdout
        fr = W * H * 3 * 2
        if len(raw) != fr * len(missing):
            raise RuntimeError(f"{video}: got {len(raw) // fr} of {len(missing)} frames")
        for k, n in enumerate(missing):
            a = np.frombuffer(raw, np.uint16, W * H * 3, k * fr).reshape(H, W, 3)
            np.save(_cache(video, n), (a.astype(F32) / F32(257.0)).astype(F32))


def _noise(rng, shape, sig):
    return rng.normal(0.0, sig, shape)


def make_image(spec):
    """Deterministic image of a case spec (regenerated in the worker processes)."""
    kind = spec[0]
    if kind == "frame":
        return np.load(_cache(spec[1], spec[2]))
    if kind == "upscale":                                  # real frame, nearest x k + noise
        img = np.load(_cache(spec[1], spec[2]))
        k = spec[3]
        img = np.repeat(np.repeat(img, k, 0), k, 1)
        rng = np.random.default_rng(spec[4])
        return np.clip(img + rng.normal(0, 1.5, img.shape).astype(F32), 0, 255).astype(F32)
    if kind == "fuzz":
        import verify_curve as vc
        rng = np.random.default_rng(spec[1])
        for k in range(spec[2] + 1):
            name, img = vc.fuzz_image(rng, k)
        return img
    rng = np.random.default_rng(spec[-1])
    W, H = spec[1], spec[2]
    yy, xx = np.mgrid[0:H, 0:W].astype(F64)
    if kind == "grad":                                     # gradient + gaussian noise
        g = 20 + 215 * xx / max(W - 1, 1)
        img = np.stack([g, g * 0.9, g * 1.05], -1) + _noise(rng, (H, W, 3), 3)
        img = np.clip(img, 0, 255)
    elif kind == "letterbox":
        img = 128 + 30 * np.sin(xx / 40.0)[..., None] + _noise(rng, (H, W, 3), 4)
        img[:H // 6] = 0.0
        img[H - H // 6:] = 0.0
        img = np.clip(img, 0, 255)
    elif kind == "pillarbox":
        img = 60 + 150 * (yy / max(H - 1, 1))[..., None] + _noise(rng, (H, W, 3), 2.5)
        img[:, :W // 8] = 16.0
        img[:, W - W // 8:] = 16.0
        img = np.clip(img, 0, 255)
    elif kind == "whiteclip":
        img = np.clip(rng.normal(200, 30, (H, W, 3)), 0, 255)
    elif kind == "sat1":                                   # R saturated: nclip 1 -> status 4
        img = np.clip(rng.normal(120, 4, (H, W, 3)), 0, 255)
        img[..., 0] = 255.0
    elif kind == "sat2":                                   # R and G saturated: every candidate skipped
        img = np.clip(rng.normal(120, 4, (H, W, 3)), 0, 255)
        img[..., :2] = 255.0
    elif kind == "const":
        img = np.full((H, W, 3), 128.0)
    elif kind == "quant":                                  # quantised flat noise: equal scores
        img = np.round(rng.normal(128, 0.45, (H, W, 3)))
    elif kind == "periodic":                               # a 32x32 noise tile repeated: tied candidates
        tile = rng.normal(110, 3, (32, 32, 3))
        img = np.tile(tile, (H // 32 + 1, W // 32 + 1, 1))[:H, :W]
    elif kind == "naninf":
        img = np.clip(rng.normal(100, 5, (H, W, 3)), 0, 255)
        img[50:80, 50:90, 1] = np.nan
        img[200:230, 300:350, 0] = np.inf
        img[300, 10:40, 2] = -np.inf
        img[:, -40:] = np.nan                              # a NaN column band over the candidates
    elif kind == "nanpay":
        img = np.clip(rng.normal(100, 5, (H, W, 3)), 0, 255).astype(F32)
        u = img.view(U32)
        reg = (slice(H // 4, 3 * H // 4), slice(W // 4, 3 * W // 4))
        m = rng.random(u[reg].shape) < 0.3
        pay = (0x7f800000 | rng.integers(1, 1 << 22, u[reg].shape) | np.where(rng.random(u[reg].shape) < 0.8, 0x400000, 0)
               | np.where(rng.random(u[reg].shape) < 0.5, 0x80000000, 0)).astype(U32)
        u[reg] = np.where(m, pay, u[reg])
        return img
    elif kind == "clip01":                                 # IsClip exactly 0.1f on every 320 square
        tiles = []                                         # (a 10x10-periodic pattern, 10 % at 255):
        for c in range(3):                                 # the penalty test 0.1f > 0.1 (double) is true
            t = rng.uniform(40, 200, (10, 10))
            t.flat[rng.choice(100, 10, replace=False)] = 255.0
            tiles.append(t)
        img = np.tile(np.stack(tiles, -1), (H // 10 + 1, W // 10 + 1, 1))[:H, :W]
    elif kind == "dark":                                   # dark noisy frame (brightness bins 0..2)
        img = np.clip(rng.normal(12, 3, (H, W, 3)) + 10 * np.sin(yy / 50.0)[..., None], 0, 255)
    elif kind == "huge":                                   # finite huge values in places
        img = np.clip(rng.normal(100, 5, (H, W, 3)), 0, 255)
        img[10:30, 10:30] = 3e38
        img[100:110, 200:260, 2] = -1e30
    else:
        raise ValueError(kind)
    return np.ascontiguousarray(img, F32)


def case_list(quick, nfuzz, jobs_fuzz_seed=20261027):
    """(name, spec, mode, table or None, deep)."""
    cases = []
    for video, ns in FRAMES.items():
        if quick:
            ns = ns[:3]
        for n in ns:
            for mode in (0, 1, 2):
                deep = (video, n) in DEEP_FRAMES and mode == 1 and not quick
                cases.append((f"{video} n={n} mode {mode}", ("frame", video, n), mode, None, deep))
    syn = [("letterbox 640x360", ("letterbox", 640, 360, 1)), ("pillarbox 960x540", ("pillarbox", 960, 540, 2)),
           ("white clipping 640x360", ("whiteclip", 640, 360, 3)), ("R saturated 640x360", ("sat1", 640, 360, 4)),
           ("R+G saturated 640x360", ("sat2", 640, 360, 5)), ("constant 128 640x360", ("const", 640, 360, 6)),
           ("quantised ties 640x360", ("quant", 640, 360, 7)), ("periodic ties 640x384", ("periodic", 640, 384, 8)),
           ("NaN/inf blocks 640x360", ("naninf", 640, 360, 9)), ("NaN payloads 320x240", ("nanpay", 320, 240, 10)),
           ("dark 640x360", ("dark", 640, 360, 11)), ("IsClip = 0.1f 640x360", ("clip01", 640, 360, 31)), ("huge values 640x360", ("huge", 640, 360, 12)),
           ("table 1 edge 2048x1280 (=0x280000)", ("grad", 2048, 1280, 13)),
           ("table 2 2048x1281", ("grad", 2048, 1281, 14)),
           ("64x64", ("grad", 64, 64, 15)), ("63x64", ("grad", 63, 64, 16)), ("65x97", ("grad", 65, 97, 17)),
           ("48x48", ("grad", 48, 48, 18)), ("47x47 (no size fits)", ("grad", 47, 47, 19)),
           ("49x50", ("grad", 49, 50, 20)), ("96x96", ("quant", 96, 96, 21)), ("100x60", ("grad", 100, 60, 22)),
           ("40x40 (no size fits)", ("grad", 40, 40, 23)), ("7x7 (w = 0)", ("grad", 7, 7, 24)),
           ("30x200", ("grad", 30, 200, 25)), ("127x129", ("grad", 127, 129, 26)), ("129x128", ("grad", 129, 128, 27))]
    if not quick:
        syn += [("4K 3840x2160 (test_v02 n=312 x2)", ("upscale", "test_v02.mp4", 312, 2, 28)),
                ("table 3 4224x2496 (>0xA00000)", ("grad", 4224, 2496, 29)),
                ("s = 9 4608x3712", ("grad", 4608, 3712, 30))]
    for name, spec in syn:
        small = spec[0] not in ("frame", "upscale") and spec[1] * spec[2] <= 400 * 400
        modes = (0, 1, 2, 3) if small else (1,)
        for mode in modes:
            cases.append((f"{name} mode {mode}", spec, mode, None, small and mode == 1))
    tables = [("zero noise table", (np.arange(8) * 32 + 16.0, np.zeros(8))),
              ("NaN noise table", (np.arange(8) * 32 + 16.0, np.full(8, np.nan))),
              ("negative noise table", (np.arange(8) * 32 + 16.0, -np.linspace(1, 3, 8))),
              ("huge noise table", (np.arange(8) * 32 + 16.0, np.full(8, 1e39))),
              ("tiny noise table (1e-6 edge)", (np.arange(8) * 32 + 16.0, np.full(8, 1.0000001e-6))),
              ("noise exactly 1e-6f table", (np.arange(8) * 32 + 16.0,
                                             np.full(8, float(np.array([0x358637bd], U32).view(F32)[0])))),
              ("2-point table", (np.array([0.0, 255.0]), np.array([1.0, 4.0]))),
              ("NaN payload node table", (np.arange(8) * 32 + 16.0, np.array([1, 2, 3, 0, 3, 2, 1, 1.5], F64)))]
    for k, (name, (txs, tys)) in enumerate(tables):
        tys = np.asarray(tys, F64)
        if "payload" in name:
            tys[3] = np.array([0x7ff8000000000123], np.uint64).view(F64)[0]
        cases.append((f"{name} on grad 320x240 mode 1", ("grad", 320, 240, 40 + k), 1,
                      (np.asarray(txs, F64), tys), True))
    rng = np.random.default_rng(jobs_fuzz_seed)
    for k in range(nfuzz):
        cases.append((f"fuzz {k}", ("fuzz", jobs_fuzz_seed, k), int(rng.integers(0, 4)), None, k % 5 == 0))
    return cases


# ----------------------------------------------------------------------------- Python side
def py_case(args):
    import nv_band as nb
    import nv_curve as ncv
    import nv_flat as nf
    import nv_pixnoise as npx
    spec, mode, table, deep = args
    img = make_image(spec)
    H, W = img.shape[:2]
    out = {}
    trace = []
    holder = {}
    with np.errstate(all="ignore"):
        if table is None:
            def curve():
                t = ncv.genuine_noise_curve(img)
                holder["t"] = t
                return ncv.noise_at(t)
            res = nf.find_flat_area_genuine(img, None, mode, trace=trace, curve=curve)
        else:
            res = nf.find_flat_area_genuine(img, ncv.noise_at(table), mode, trace=trace)
        out["result"] = tuple(int(v) for v in res)
        s = 7
        while True:
            s += 1
            w, h = W // s, H // s
            if not (h >= 5 and w >= 5 and w * h > 0x40000):
                break
        i0 = -1
        sizes = factors = None
        if w and h:
            sizes, factors = next((sz, f) for lim, sz, f in nf.TABLES if lim is None or W * H <= lim)
            i0 = next((i for i, S in enumerate(sizes) if S <= W and S <= H), -1)
        out["steps"] = (s, w, h, i0, int("t" in holder), int("t" in holder))
        out["curve"] = holder.get("t")
        out["reduce"] = None
        out["sizes"] = []
        if i0 >= 0:
            R, G, B = nf.reduce_planes(img, s)
            out["reduce"] = (R, G, B, npx.luma_601(R, G, B))
            step = max(16 // s, 1)
            luma = out["reduce"][3]
            for S in sizes[i0:]:
                r = int(F32(F32(F32(F32(S) / F32(s)) + F32(-1.0)) * F32(0.5)))
                r = r if r > 1 else 1
                xs, ys, means, scores = nf.candidates(luma, r, r, step)
                placed = []
                for i in range(len(scores)):
                    if scores[i] < F32(0.0):
                        break
                    x0, cw = nf._place(int(xs[i]), s, S, W)
                    y0, ch = nf._place(int(ys[i]), s, S, H)
                    crop = img[y0:y0 + ch, x0:x0 + cw]
                    clip = [F32(ncv.is_clip_genuine(np.ascontiguousarray(crop[..., c]))) for c in range(3)]
                    placed.append((x0, y0, cw, ch, clip))
                out["sizes"].append((S, r, step, xs, ys, means, scores, placed))
        out["trace"] = trace
        if deep:
            st = []
            for t in trace:
                crop = img[t["y"]:t["y"] + t["h"], t["x"]:t["x"] + t["w"]]
                planes = nf.transform(crop[..., 0], crop[..., 1], crop[..., 2], mode)
                st.append(np.concatenate([nb.band_levels(np.ascontiguousarray(p, F32)).view(U32) for p in planes]))
            out["structs"] = st
    return out


# ----------------------------------------------------------------------------- C++ side
def write_case(f, img, mode, table, deep):
    H, W = img.shape[:2]
    flags = (1 if table is not None else 0) | (2 if deep else 0)
    f.write(struct.pack("<iiii", W, H, mode, flags))
    if table is not None:
        f.write(struct.pack("<i", len(table[0])) + np.asarray(table[0], F64).tobytes()
                + np.asarray(table[1], F64).tobytes())
    f.write(np.ascontiguousarray(img[..., :3], F32).tobytes())


class Cursor:
    def __init__(self, data):
        self.d = data
        self.p = 0

    def arr(self, dtype, n):
        a = np.frombuffer(self.d, dtype, n, self.p)
        self.p += a.nbytes
        return a

    def i32(self, n=1):
        a = self.arr(np.int32, n)
        return int(a[0]) if n == 1 else [int(v) for v in a]


def parse_case(c):
    out = {}
    v = c.i32(11)
    out["result"] = tuple(v[:5])
    out["steps"] = tuple(v[5:11])
    out["curve"] = (c.arr(F64, 8), c.arr(F64, 8))
    rn = int(c.arr(np.int64, 1)[0])
    out["reduce"] = [c.arr(F32, rn) for _ in range(4)]
    sizes = []
    for _ in range(c.i32()):
        S, r, step = c.i32(3)
        xs, ys = c.arr(np.int64, 64), c.arr(np.int64, 64)
        means, scores = c.arr(F32, 64), c.arr(F32, 64)
        placed = []
        for _ in range(c.i32()):
            p = c.i32(4)
            placed.append((*p, c.arr(F32, 3)))
        sizes.append((S, r, step, xs, ys, means, scores, placed))
    out["sizes"] = sizes
    trace = []
    for _ in range(c.i32()):
        iv = c.i32(8)
        fv = c.arr(F32, 8)
        nw = c.i32()
        words = c.arr(U32, nw)
        trace.append((iv, fv, words))
    out["trace"] = trace
    return out


def run_exe(cases_in, jobs):
    inp = os.path.join(TMP, f"tmp_flat_{os.getpid()}.in")
    outs = {}
    with open(inp, "wb") as f:
        f.write(struct.pack("<Q", len(cases_in)))
        for spec, mode, table, deep in cases_in:
            write_case(f, make_image(spec), mode, table, deep)
    times = {}
    for j in sorted({1, jobs}):
        outp = os.path.join(TMP, f"tmp_flat_{os.getpid()}_j{j}.out")
        t0 = time.perf_counter()
        subprocess.run([EXE, "flat", inp, outp, "-j", str(j)], check=True)
        times[j] = time.perf_counter() - t0
        data = open(outp, "rb").read()
        os.remove(outp)
        c = Cursor(data)
        outs[j] = [parse_case(c) for _ in cases_in]
        if c.p != len(data):
            raise RuntimeError(f"C++ output: {len(data) - c.p} trailing bytes")
    os.remove(inp)
    return outs, times


# ----------------------------------------------------------------------------- comparison
def fbits(x):
    return np.asarray(x, F32).view(U32)


def same_f32(a, b):
    a, b = np.asarray(a, F32).ravel(), np.asarray(b, F32).ravel()
    return a.shape == b.shape and np.array_equal(a.view(U32), b.view(U32))


def compare(e, g, deep):
    """Categories that differ between the Python expectation e and the C++ result g."""
    bad = []
    if e["result"] != g["result"]:
        bad.append(f"result {g['result']} != {e['result']}")
    if e["steps"] != g["steps"]:
        bad.append(f"steps {g['steps']} != {e['steps']}")
    if e["curve"] is not None:
        xs, ys = (np.asarray(v, F64) for v in e["curve"])
        if not (np.array_equal(xs.view(np.uint64), g["curve"][0].view(np.uint64))
                and np.array_equal(ys.view(np.uint64), g["curve"][1].view(np.uint64))):
            bad.append("curve")
    if e["reduce"] is None:
        if g["reduce"][0].size:
            bad.append("reduce (unexpected)")
    elif not all(same_f32(a, b) for a, b in zip(e["reduce"], g["reduce"])):
        bad.append("reduce")
    if len(e["sizes"]) != len(g["sizes"]):
        bad.append(f"nsizes {len(g['sizes'])} != {len(e['sizes'])}")
    for es, gs in zip(e["sizes"], g["sizes"]):
        S = es[0]
        if es[:3] != gs[:3]:
            bad.append(f"size {S} r/step {gs[:3]} != {es[:3]}")
        if not (np.array_equal(es[3], gs[3]) and np.array_equal(es[4], gs[4]) and same_f32(es[5], gs[5])
                and same_f32(es[6], gs[6])):
            bad.append(f"size {S} candidates")
        if len(es[7]) != len(gs[7]):
            bad.append(f"size {S} nplaced {len(gs[7])} != {len(es[7])}")
        for k, (ep, gp) in enumerate(zip(es[7], gs[7])):
            if tuple(ep[:4]) != tuple(gp[:4]) or not same_f32(ep[4], gp[4]):
                bad.append(f"size {S} cand {k} placed/IsClip {gp[:4]} {list(gp[4])} != {ep[:4]} {ep[4]}")
                break
    et, gt = e["trace"], g["trace"]
    if len(et) != len(gt):
        bad.append(f"ntrace {len(gt)} != {len(et)}")
    for k, (t, (iv, fv, words)) in enumerate(zip(et, gt)):
        ei = [t["size"], t["i"], t["x"], t["y"], t["w"], t["h"], t["ncl"], t["pen"]]
        ef = F32([t["mean"], t["nz"], *t["fl"], t["E"], t["R"], t["score"]])
        if ei != list(iv) or not same_f32(ef, fv):
            names = ["mean", "nz", "fl0", "fl1", "fl2", "E", "R", "score"]
            dif = [n for n, a, b in zip(names, fbits(ef), fbits(fv)) if a != b]
            bad.append(f"trace {k} (size {t['size']} i {t['i']}): ints {list(iv)} vs {ei}, floats {dif}")
            break
        if deep:
            if words.size != 3 * 4096 or not np.array_equal(words, e["structs"][k]):
                nd = -1 if words.size != 3 * 4096 else int((words != e["structs"][k]).sum())
                bad.append(f"structs of trace {k}: {nd} words differ")
                break
    return bad


def summarize(e):
    n_cand = sum(int((np.asarray(s[6]) >= 0).sum()) for s in e["sizes"])
    return n_cand, len(e["trace"]), len(e["trace"]) if e.get("structs") is not None else 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true", help="3 frames per video, no 4K / table-3 / s=9 frames")
    ap.add_argument("--fuzz", type=int, default=40, help="seeded random images (default 40)")
    ap.add_argument("--jobs", type=int, default=3, help="Python processes / C++ threads (max 3)")
    ap.add_argument("--chunk", type=int, default=12, help="cases per C++ run")
    ap.add_argument("--cache", action="store_true",
                    help="reuse / store the Python expectations in TMP (for the mutation runs)")
    A = ap.parse_args()
    jobs = max(1, min(A.jobs, 3))
    if not os.path.exists(EXE):
        print(f"{EXE} not found: make -C native -f Makefile.flat")
        return 2
    os.makedirs(TMP, exist_ok=True)
    t_all = time.perf_counter()
    extract_frames()
    cases = case_list(A.quick, A.fuzz if not A.quick else min(A.fuzz, 10))
    print(f"flat: {len(cases)} cases ({sum(1 for c in cases if c[1][0] == 'frame')} real frame x mode, "
          f"{sum(1 for c in cases if c[1][0] not in ('frame', 'fuzz'))} synthetic, "
          f"{sum(1 for c in cases if c[1][0] == 'fuzz')} fuzz; {sum(1 for c in cases if c[4])} deep)")
    fails = 0
    stats = {"cand": 0, "trace": 0, "structs": 0, "status": {}}
    t_py = t_cpp = 0.0
    cpp_times = {}
    with Pool(jobs) as pool:
        for c0 in range(0, len(cases), A.chunk):
            part = cases[c0:c0 + A.chunk]
            args = [(spec, mode, table, deep) for _, spec, mode, table, deep in part]
            t0 = time.perf_counter()
            key = hashlib.sha1(repr([(c[0], c[2]) for c in part]).encode()).hexdigest()[:16]
            cache = os.path.join(TMP, f"expected_{key}.pkl")
            if A.cache and os.path.exists(cache):
                with open(cache, "rb") as f:
                    expected = pickle.load(f)
            else:
                expected = pool.map(py_case, args, chunksize=1)  # not overlapped with the C++ (<= 3 busy cores)
                if A.cache:
                    with open(cache, "wb") as f:
                        pickle.dump(expected, f)
            t_py += time.perf_counter() - t0
            outs, times = run_exe(args, jobs)
            for j, dt in times.items():
                cpp_times[j] = cpp_times.get(j, 0.0) + dt
            for k, ((name, spec, mode, table, deep), e) in enumerate(zip(part, expected)):
                nc, nt, ns = summarize(e)
                stats["cand"] += nc
                stats["trace"] += nt
                stats["structs"] += ns
                st = e["result"][4]
                stats["status"][st] = stats["status"].get(st, 0) + 1
                for j, o in outs.items():
                    bad = compare(e, o[k], deep)
                    if bad:
                        fails += 1
                        print(f"  MISMATCH [{name}] -j {j}: " + "; ".join(bad[:4]))
    n = len(cases) * len({1, jobs})
    print(f"flat: {n - fails}/{n} case runs bit-exact (result + steps + curve + reduce + candidates + placed/IsClip"
          f" + trace{' + structs' if stats['structs'] else ''})")
    print(f"  per run: {stats['cand']} candidates, {stats['trace']} scored candidates, "
          f"{stats['structs']} x 3 structs (deep); statuses {dict(sorted(stats['status'].items()))}")
    print(f"  time: Python (incl. intermediates, {jobs} procs, wall) {t_py:.1f} s; C++ "
          + ", ".join(f"-j {j} {dt:.1f} s" for j, dt in sorted(cpp_times.items())) + " (incl. blob I/O)")
    print(f"total {time.perf_counter() - t_all:.1f} s: " + ("ALL BIT-EXACT" if fails == 0 else f"{fails} MISMATCHES"))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
