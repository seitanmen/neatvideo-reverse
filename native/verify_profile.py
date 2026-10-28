# Bit-exact verification of the native nv_profile port (N1f) against the Python implementation.
#
#   make -C native -f Makefile.profile && python3 native/verify_profile.py [--quick] [--jobs N] [-j N]
#                                                                          [--exe PATH] [--no-neg]
#
# Builds the 0xc300-byte profile of repro/nv_profile.build_profile (BuildNoiseModel as `nvh profile` runs
# it: area given) for real frames (test_v02.mp4 / test.mp4, 128^2 and 192^2 areas, colour modes 0..3,
# non-square areas) and synthetic frames (NaN / inf, saturation, constant planes, incompressible noise
# for the LZMA failure path, the small-area branches of mc_stats, a side < 32 = NotImplementedError,
# with_floor / lzma off), runs native/nv_profile_test over the same inputs and compares the status and
# ALL 0xc300 bytes (the heap pointers 0x00..0x17 are 0 on both sides).  On a mismatch it names the byte
# ranges (parts) that differ.  Also checked: the C++ reference noise (FUN_009c3bc4) against
# nv_refnoise.noise and the C++ reference floor against data/ref_floor.npy; negative controls (a pixel
# inside the area changed by one ulp must change the profile, one outside must not).
#
# Each case is a window of the frame (the area plus a margin of up to 7 px) so that the area offset is
# exercised without shipping whole frames.  The Python results are cached in
# sandbox/omo/wp_native_profile/tmp/pycache, keyed by the window bytes, the parameters and the SHA-1 of
# the repro sources build_profile runs (and of data/ref_floor.npy).  Real frames are read from the N1a
# frame cache (sandbox/omo/wp_native_band/tmp/*.npy, read only) or cut with ffmpeg into tmp/.
# The LZMA input of length 0 (repro/nv_lzma.py differs from the SDK there) cannot occur: areas are >= 1 px
# (and any side < 32 stops in tile_dev before the LZMA part).
import argparse
import hashlib
import json
import os
import struct
import subprocess
import sys
import time
from multiprocessing import Pool

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
REPRO = os.path.join(ROOT, "repro")
TMP = os.path.join(ROOT, "sandbox", "omo", "wp_native_profile", "tmp")
PYCACHE = os.path.join(TMP, "pycache")
BAND_FRAMES = os.path.join(ROOT, "sandbox", "omo", "wp_native_band", "tmp")
FLOOR = os.path.join(ROOT, "data", "ref_floor.npy")
sys.path.insert(0, REPRO)
sys.dont_write_bytecode = True

import numpy as np  # noqa: E402

F32 = np.float32
U32 = np.uint32
EXE = os.path.join(HERE, "nv_profile_test")
PROF = 0xc300
STATUS = {0: "ok", 1: "NotImplementedError", 2: "ValueError"}
SOURCES = ("nv_profile.py", "nv_mcstats.py", "nv_refnoise.py", "nv_flat.py", "nv_model.py", "nv_model_special.py",
           "nv_model_bm.py", "nv_band.py", "nv_curve.py", "nv_lzma.py", "nv_pixnoise.py", "nv_lines.py")


def source_hash():
    h = hashlib.sha1()
    for name in SOURCES:
        with open(os.path.join(REPRO, name), "rb") as f:
            h.update(name.encode() + b"\0" + f.read())
    with open(FLOOR, "rb") as f:
        h.update(f.read())
    return h.hexdigest()[:16]


# ------------------------------------------------------------------------------- byte ranges (parts)
def part_of(off):
    if off < 0x18:
        return "heap pointers"
    if off < 0x24:
        return "LZMA sizes"
    if off < 0x2c:
        return "LZMA W/h"
    if off < 0x4c:
        return "mode / pad"
    if off < 0xbc44:
        i, w = divmod(off - 0x4c, 0x1f54)
        return f"model {('R', 'G', 'B', 'T0', 'T1', 'T2')[i]} word {'0..54' if w // 4 < 55 else '55..'}"
    for name, lo, hi in (("tile_dev R", 0xbc44, 0xbd10), ("tile_dev G", 0xbd10, 0xbddc),
                         ("tile_dev B", 0xbddc, 0xbea8), ("mc_stats", 0xbea8, 0xbf28), ("means", 0xbf28, 0xbf34),
                         ("0xbf34", 0xbf34, 0xbf38), ("size class", 0xbf38, 0xbf3c), ("clip flags", 0xbf3c, 0xbf40),
                         ("curve", 0xbf40, 0xc2a4), ("LZMA flag", 0xc2a4, 0xc2a8), ("pad", 0xc2a8, 0xc2c0),
                         ("W/h", 0xc2c0, 0xc2c8), ("constants", 0xc2c8, 0xc2f0)):
        if lo <= off < hi:
            return name
    return "rest"


def describe(a, b):
    d = np.nonzero(a != b)[0]
    parts = {}
    for o in d:
        p = part_of(int(o))
        parts[p] = parts.get(p, 0) + 1
    return f"{d.size} bytes differ, first at {hex(int(d[0]))}; by part {parts}"


# ------------------------------------------------------------------------------- frames
FRAMES = [("test_v02.mp4", n) for n in (100, 312, 576)] + [("test.mp4", n) for n in (0, 300, 450)]


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


def window(rgb, x, y, w, h, m=7):
    """(window, x, y) - the area with a margin of up to m px (clipped to the frame)."""
    H, W = rgb.shape[:2]
    x0, y0 = max(0, x - m), max(0, y - m)
    x1, y1 = min(W, x + w + m), min(H, y + h + m)
    return np.ascontiguousarray(rgb[y0:y1, x0:x1], F32), x - x0, y - y0


def case(group, label, win, mode, x, y, w, h, floor=True, lzma=True):
    return dict(group=group, label=label, rgb=win, mode=mode, area=(x, y, w, h), flags=int(floor) | 2 * int(lzma))


def real_cases(quick):
    out = []
    frames = FRAMES if not quick else FRAMES[1:2] + FRAMES[4:5]
    for k, (video, n) in enumerate(frames):
        rgb = frame(video, n)
        H, W = rgb.shape[:2]
        # 128^2 and 192^2 at frame-dependent positions (colour mode 1, the genuine's default)
        for size, (fx, fy) in ((128, (0.15 + 0.11 * k, 0.08 + 0.07 * k)), (192, (0.62 - 0.09 * k, 0.5 - 0.05 * k))):
            x, y = int(fx * (W - size)), int(fy * (H - size))
            win, ax, ay = window(rgb, x, y, size, size)
            out.append(case("real", f"{video} n={n} {size}^2 @({x},{y}) mode 1", win, 1, ax, ay, size, size))
    # colour modes 0 / 2 / 3, non-square areas (real frames)
    extra = [("test_v02.mp4", 312, 0, (832, 0, 128, 128)), ("test_v02.mp4", 312, 2, (832, 0, 128, 128)),
             ("test.mp4", 300, 0, (200, 120, 128, 128)), ("test.mp4", 450, 2, (400, 160, 128, 128)),
             ("test.mp4", 0, 3, (100, 100, 96, 96)),
             ("test_v02.mp4", 576, 1, (300, 700, 160, 96)), ("test.mp4", 300, 1, (50, 210, 101, 77)),
             ("test_v02.mp4", 100, 2, (1500, 40, 64, 200)), ("test_v02.mp4", 312, 1, (600, 300, 320, 200))]
    if quick:
        extra = extra[:1] + extra[5:6]
    for video, n, mode, (x, y, w, h) in extra:
        rgb = frame(video, n)
        win, ax, ay = window(rgb, x, y, w, h)
        out.append(case("real2", f"{video} n={n} {w}x{h} @({x},{y}) mode {mode}", win, mode, ax, ay, w, h))
    return out


def synthetic_cases(seed, quick):
    rng = np.random.default_rng(seed)
    out = []

    def noisy(H, W, mu=100.0, sd=4.0):
        return np.clip(rng.normal(mu, sd, (H, W, 3)), 0, 255).astype(F32)

    def add(label, rgb, mode=1, area=None, floor=True, lzma=True):
        H, W = rgb.shape[:2]
        x, y, w, h = area if area else (0, 0, W, H)
        out.append(case("synth", label, np.ascontiguousarray(rgb, F32), mode, x, y, w, h, floor, lzma))

    p = noisy(140, 140)
    u = p.view(U32)
    u[rng.random(p.shape) < 0.003] = 0x7fc00000
    u[rng.random(p.shape) < 0.002] = 0xffc12345
    u[rng.random(p.shape) < 0.001] = 0x7fa00001            # signalling
    add("NaN payloads 128^2", p, 1, (5, 6, 128, 128))
    # one NaN per 16 x 16 block with its own payload (quiet and signalling, both signs): NaNs reach the
    # CV words of the tile models and of the plane models, so tile_dev accumulates NaNs
    p = noisy(128, 128)
    u = p.view(U32)
    for by in range(8):
        for bx in range(8):
            k = 8 * by + bx + 1
            u[16 * by + 5, 16 * bx + 7, k % 3] = (0x7fc00000, 0xffa00000, 0x7f800000 | k << 8)[k % 3] + k
    add("NaN payload grid 128^2, no floor (a floor replaces NaNs)", p, 1, None, False, True)
    p = noisy(130, 130)
    u = p.view(U32)
    u[rng.random(p.shape) < 0.002] = 0x7f800000
    u[rng.random(p.shape) < 0.002] = 0xff800000
    add("+-inf 128^2", p, 2, (1, 1, 128, 128))
    p = noisy(128, 128, 240, 30)                          # saturated (clip flags)
    p[:, :, 0] = np.minimum(p[:, :, 0] + 40, 255)
    p = np.clip(p, 0, 255).astype(F32)
    add("saturated R 128^2", p, 1)
    p = noisy(128, 128, 20, 3)
    p[:64, :, :] = 255.0                                  # half white
    add("half white 128^2", p, 0)
    add("constant 128^2", np.full((128, 128, 3), 128.0, F32), 1)
    c = noisy(96, 96)
    c[:, :, 1] = 0.0                                      # one constant plane
    add("constant G 96^2", c, 1)
    # full-entropy int16 samples (rint(x * 16) spans -32768..32767): the stream exceeds len + 0x80
    add("int16 noise 128^2 (LZMA overflow)", rng.uniform(-2048, 2048, (128, 128, 3)).astype(F32), 1)
    lz = rng.uniform(0, 255, (128, 128, 3)).astype(F32)
    lz[:, :, 1] = rng.uniform(-2048, 2048, (128, 128))     # only G overflows: the chain stops at G
    add("G-only int16 noise 128^2 (LZMA overflow at G)", lz, 1)
    add("64x36 (mc 32-stage fails)", noisy(36, 64), 1)
    add("32x32 minimum", noisy(32, 32), 1)
    add("48x40", noisy(40, 48, 60, 10), 2)
    add("31x64 side < 32 (NotImplementedError)", noisy(64, 31), 1)
    add("no floor, no LZMA 128^2", noisy(128, 128, 80, 6), 1, None, False, False)
    add("no floor 96^2", noisy(96, 96, 150, 2), 1, None, False, True)
    if not quick:
        p = noisy(200, 200, 128, 50)
        p.view(U32)[rng.random(p.shape) < 0.01] = 0x80000000   # -0
        add("wide noise with -0 192^2", p, 1, (4, 3, 192, 192))
        add("negative / huge values 64^2", rng.normal(0, 1e6, (64, 64, 3)).astype(F32), 1)
        add("ramp 100x60", (np.mgrid[0:60, 0:100][1][..., None] * np.array([2.0, 1.5, 1.0]) +
                            rng.normal(0, 1, (60, 100, 3))).astype(F32), 0)
    return out


# ------------------------------------------------------------------------------- expected values
_SRC = None


def key(c):
    h = hashlib.sha1(c["rgb"].tobytes())
    h.update(struct.pack("<8i", c["rgb"].shape[1], c["rgb"].shape[0], c["mode"], *c["area"], c["flags"]))
    return os.path.join(PYCACHE, f"{h.hexdigest()}_{_SRC}.npy")


def expected(job):
    """(status, profile bytes, seconds) of nv_profile.build_profile; cached as uint8[4 + 0xc300]."""
    path, rgb, mode, area, flags = job
    if os.path.exists(path):
        r = np.load(path)
        meta = path[:-4] + ".json"
        dt = json.load(open(meta))["seconds"] if os.path.exists(meta) else float("nan")
        return int(r[:4].view(U32)[0]), r[4:], dt
    import warnings
    warnings.simplefilter("ignore")
    import nv_profile as npf
    x, y, w, h = area
    t0 = time.time()
    try:
        with np.errstate(all="ignore"):
            buf, _ = npf.build_profile(rgb, mode, (x, y, w, h), with_floor=bool(flags & 1), lzma=bool(flags & 2))
        st = 0
        buf = np.asarray(buf, np.uint8)
    except NotImplementedError:
        buf, st = np.zeros(PROF, np.uint8), 1
    except ValueError:
        buf, st = np.zeros(PROF, np.uint8), 2
    dt = time.time() - t0
    r = np.concatenate([np.array([st], U32).view(np.uint8), buf])
    tmp = path + f".{os.getpid()}.tmp.npy"
    np.save(tmp, r)
    os.replace(tmp, path)
    with open(path[:-4] + ".json", "w") as f:
        json.dump({"seconds": dt}, f)
    return st, buf, dt


def run_cpp(exe, cases, tag, threads):
    os.makedirs(TMP, exist_ok=True)
    inp = os.path.join(TMP, f"profile_cases_{tag}_{os.getpid()}.bin")
    outp = os.path.join(TMP, f"profile_results_{tag}_{os.getpid()}.bin")
    with open(inp, "wb") as f:
        f.write(struct.pack("<Q", len(cases)))
        for c in cases:
            H, W = c["rgb"].shape[:2]
            f.write(struct.pack("<8i", W, H, c["mode"], *c["area"], c["flags"]))
            f.write(np.ascontiguousarray(c["rgb"], F32).tobytes())
    t1 = time.time()
    r = subprocess.run([exe, "-j", str(threads), "--floor", FLOOR, inp, outp], capture_output=True, text=True)
    t = time.time() - t1
    if r.returncode:
        raise SystemExit(f"{exe} failed ({r.returncode}): {r.stderr[-500:]}")
    res = np.fromfile(outp, np.uint8).reshape(len(cases), 4 + PROF)
    os.remove(inp)
    os.remove(outp)
    return [(int(row[:4].view(U32)[0]), row[4:]) for row in res], t, r.stderr.strip()


def check_reference(exe):
    """C++ FUN_009c3bc4 vs nv_refnoise.noise, C++ reference floor vs data/ref_floor.npy."""
    import nv_refnoise as rn
    os.makedirs(TMP, exist_ok=True)
    ok = True
    for n, seed in ((65536, 0x12345678), (1000, 0xdeadbeef), (37, 0)):
        p = os.path.join(TMP, f"noise_{os.getpid()}.bin")
        subprocess.run([exe, "noise", str(n), hex(seed), p], check=True)
        a = np.fromfile(p, F32)
        os.remove(p)
        eq = np.array_equal(a.view(U32), rn.noise(n, seed).view(U32))
        ok &= eq
        print(f"  reference noise n={n} seed={seed:#x}: {'bit-exact' if eq else 'MISMATCH'}")
    p = os.path.join(TMP, f"floor_{os.getpid()}.bin")
    r = subprocess.run([exe, "floor", p], check=True, capture_output=True, text=True)
    a = np.fromfile(p, F32)
    os.remove(p)
    eq = np.array_equal(a.view(U32), np.load(FLOOR).view(U32))
    ok &= eq
    print(f"  reference floor (C++ computed) vs data/ref_floor.npy: {'51/51 bit-exact' if eq else 'MISMATCH'}"
          f"  ({r.stderr.strip()})")
    return ok


def negative_controls(exe, cases, exp, threads):
    """A pixel inside the area changed by one ulp -> the profile changes; one outside -> unchanged."""
    picks = [i for i, c in enumerate(cases) if c["group"] == "real" and exp[i][0] == 0][:2]
    trials = []
    for i in picks:
        c = cases[i]
        x, y, w, h = c["area"]
        inside = c["rgb"].copy()
        inside.view(U32)[y + h // 2, x + w // 3, 1] += 1
        trials.append((i, "inside +1 ulp (G)", dict(c, rgb=inside), True))
        if x > 0:
            outside = c["rgb"].copy()
            outside[y + h // 2, x - 1, :] += F32(37.0)
            trials.append((i, "outside +37 (left margin)", dict(c, rgb=outside), False))
    if not trials:
        return True
    res, _, _ = run_cpp(exe, [t[2] for t in trials], "neg", threads)
    ok = True
    for (i, what, _, must_differ), (st, b) in zip(trials, res):
        differs = st != exp[i][0] or not np.array_equal(b, exp[i][1])
        good = differs == must_differ
        ok &= good
        detail = describe(b, exp[i][1]) if differs and st == exp[i][0] else ("identical" if not differs else "status")
        print(f"  negative control {cases[i]['label']}: {what}: {detail} -> {'as expected' if good else 'UNEXPECTED'}")
    return ok


def main():
    global _SRC
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--jobs", type=int, default=3, help="Python processes (at most 3)")
    ap.add_argument("-j", type=int, default=1, dest="threads", help="threads of the C++ driver (at most 3)")
    ap.add_argument("--seed", type=int, default=0xF11F)
    ap.add_argument("--exe", default=EXE)
    ap.add_argument("--no-neg", action="store_true")
    a = ap.parse_args()
    threads = max(1, min(a.threads, 3))
    _SRC = source_hash()
    os.makedirs(PYCACHE, exist_ok=True)
    print("reference parts:")
    ok = check_reference(a.exe)
    cases = real_cases(a.quick) + synthetic_cases(a.seed, a.quick)
    jobs = [(key(c), c["rgb"], c["mode"], c["area"], c["flags"]) for c in cases]
    todo = [j for j in jobs if not os.path.exists(j[0])]
    t0 = time.time()
    if todo:
        nj = max(1, min(a.jobs, 3))
        print(f"Python: {len(todo)} of {len(jobs)} profiles not cached, computing with {nj} processes ...", flush=True)
        with Pool(nj) as pool:
            for k, r in enumerate(pool.imap_unordered(expected, todo, chunksize=1)):
                print(f"  {k + 1}/{len(todo)} done ({r[2]:.1f} s)", flush=True)
    exp = [expected(j) for j in jobs]
    t_py = time.time() - t0
    res, t_cpp, msg = run_cpp(a.exe, cases, "main", threads)
    stats = {}
    bad = 0
    for c, (est, eb, _), (st, b) in zip(cases, exp, res):
        s = stats.setdefault(c["group"], [0, 0, 0])
        s[0] += 1
        s[2] += est != 0
        good = st == est and (est != 0 or np.array_equal(b, eb))
        if est == 0 and good:
            good = not b[:0x18].any() and not eb[:0x18].any()
        tag = "ok" if good else "MISMATCH"
        info = STATUS.get(est, est) if est else f"LZMA {'ok' if eb[0xc2a4] else 'failed/off'}"
        if not good:
            s[1] += 1
            bad += 1
            why = f"status C++ {st} vs Py {est}" if st != est else describe(b, eb)
            print(f"{tag} [{c['group']}] {c['label']}: {why}")
        else:
            print(f"{tag:8s} [{c['group']:5s}] {c['label']}  ({info})")
    for g, (n, nb, nst) in stats.items():
        extra = f" (status != 0 in Python: {nst})" if nst else ""
        print(f"  {g:6s}: {n - nb}/{n} bit-exact (all 0xc300 bytes){extra}")
    print(f"{len(cases) - bad}/{len(cases)} profiles bit-exact" + ("" if bad == 0 else f"  ({bad} MISMATCHES)"))
    py_sum = sum(e[2] for e in exp if e[2] == e[2])
    print(f"time: Python build_profile {py_sum:.1f} s in total (cached runs included; {len(todo)} computed now, "
          f"wall {t_py:.1f} s), C++ {t_cpp:.2f} s wall ({msg})")
    if not a.no_neg:
        ok &= negative_controls(a.exe, cases, exp, threads)
    ok &= bad == 0
    print("PASS" if ok else "FAIL")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
