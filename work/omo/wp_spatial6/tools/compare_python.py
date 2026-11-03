#!/usr/bin/env python3
"""native/nv_spatial6 plane_filter (FUN_0098edb0) against the Python transcription work/omo/wp_spatial/src/nv_spatial.py
(plane_filter), bit for bit, on crops of the test videos and synthetic planes, with the cfg of build_cfg under several
settings (sets 1..3, levels 3..5, adaptive b8, the cfg+0x90 detail stage, cfg+0x28 / +0x3c / +0x78, ROI cuts).
The model words come from native/nv_model_test (BuildNoiseModel per plane, bit-exact with repro/nv_model.py) on a
crop of the same frame; the sigma maps are positive synthetic maps (FUN_009aae90 is not part of FUN_0098edb0).

  python3 compare_python.py [--cases N] [--seed S] [--rec DIR] [--log FILE] [--popdir P] [--popcfg P]
--popdir / --popcfg: the popcounts of nvs6::Guard passed to the C++ run (default 16).  With --popcfg != 16 the Python
reference is run with cfg sigma[k] = 0 for every level (what FUN_0098edb0 does then, 0x98f77c..0x98f788 in the level
loop), so the zeroing range is checked; --popdir has no Python counterpart (the reference keeps the multiplier 1).
--rec DIR: also records the leaf calls of every C++ run under DIR/<case>/ for tools/replay.py.
"""
import argparse, os, struct, subprocess, sys, tempfile, time
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "work", "omo", "wp_spatial", "src"))
sys.path.insert(0, os.path.join(ROOT, "reference", "python"))
import nv_spatial as nsp  # noqa: E402

PORT = os.environ.get("NV6_PORT") or os.path.join(ROOT, "native", "nv_spatial6_test")
MODEL = os.path.join(ROOT, "native", "nv_model_test")
F32 = np.float32


def frame_planes(path, n, W, H):
    raw = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-i", path, "-vf", "select=eq(n\\,%d)" % n, "-frames:v", "1",
                          "-f", "rawvideo", "-pix_fmt", "yuv420p", "-"], capture_output=True, check=True).stdout
    y = np.frombuffer(raw[:W * H], np.uint8).reshape(H, W).astype(F32)
    u = np.frombuffer(raw[W * H:W * H + W * H // 4], np.uint8).reshape(H // 2, W // 2).astype(F32)
    v = np.frombuffer(raw[W * H + W * H // 4:], np.uint8).reshape(H // 2, W // 2).astype(F32)
    return y, (u - F32(128)) * F32(0.713), (v - F32(128)) * F32(0.565)


def models(planes):
    """native/nv_model_test on each plane (set -1 = all sets, 5 levels) -> 4096 words each (status 0 required)."""
    with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR")) as td:
        fi, fo = os.path.join(td, "in.bin"), os.path.join(td, "out.bin")
        with open(fi, "wb") as f:
            f.write(struct.pack("<Q", len(planes)))
            for p in planes:
                H, W = p.shape
                f.write(struct.pack("<IIiI", W, H, -1, 5))
                f.write(np.ascontiguousarray(p, F32).tobytes())
        subprocess.run([MODEL, fi, fo], check=True, capture_output=True)
        b = open(fo, "rb").read()
    out = []
    for i in range(len(planes)):
        st = struct.unpack_from("<I", b, i * 4 * 4097)[0]
        w = np.frombuffer(b, np.uint32, 4096, i * 4 * 4097 + 4).view(F32).copy()
        out.append(w if st == 0 else None)
    return out


def hexs(v):
    return " ".join("%08x" % F32(x).view(np.uint32) for x in v)


def write_cfg(path, c):
    with open(path, "w") as f:
        for k in ("sigma", "amount", "c28", "c3c", "c78", "c90"):
            f.write("%s %s\n" % (k, hexs(c[k])))
        f.write("ca4 %s\n" % " ".join(str(int(x)) for x in c["ca4"]))
        f.write("c8c %d\nb8 %s\nlevels %d\nset %d\n" % (int(c["c8c"]), hexs([c["b8"]]), int(c["levels"]), int(c["set"])))


def settings(rng):
    s = nsp.default_settings()
    s[0x104] = int(rng.integers(0, 2))                         # quality 0 / 1 -> sets (2,1,1) / (3,2,2)
    lv = int(rng.integers(3, 6))
    s[0x1c] = tuple([1.0] * 3 + [1.0 if lv >= 4 else 0.0, 1.0 if lv >= 5 else 0.0])
    s[0x08] = tuple(F32(rng.uniform(0.5, 2.0)) for _ in range(5))
    s[0x3c] = tuple(F32(rng.choice([1.0, 0.4, 0.0, 1.0, rng.uniform(0, 1)])) for _ in range(3))
    if rng.random() < 0.3:
        s[0xcd], s[0xec], s[0xd0], s[0x128] = 1, F32(rng.uniform(0.2, 3)), int(rng.integers(1, 4)), F32(rng.uniform(0.1, 2))
    if rng.random() < 0.5:
        s[0x90], s[0x91] = 1, int(rng.integers(0, 2))
        s[0xa0] = tuple(F32(rng.uniform(0, 1)) for _ in range(3))
        s[0x154] = tuple(int(rng.integers(0, 3)) for _ in range(3))
    if rng.random() < 0.5:
        s[0x48], s[0x4c], s[0x50] = 1, F32(rng.uniform(0, 1)), F32(rng.uniform(0, 1))
    if rng.random() < 0.4:
        s[0x60] = 1
        s[0x64] = tuple(F32(rng.uniform(0, 1)) for _ in range(3))
    return s


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cases", type=int, default=40)
    ap.add_argument("--seed", type=int, default=11)
    ap.add_argument("--rec", default="")
    ap.add_argument("--log", default="")
    ap.add_argument("--popdir", type=int, default=16)
    ap.add_argument("--popcfg", type=int, default=16)
    ap.add_argument("--no-mirror", action="store_true", help="keep the Python reference at the default cfg under --popcfg")
    ap.add_argument("--maxside", type=int, default=512)
    ap.add_argument("--set3", type=float, default=0.25, help="probability of forcing set 3")
    a = ap.parse_args()
    log = open(a.log, "w") if a.log else None

    def say(m):
        print(m, flush=True)
        if log:
            log.write(m + "\n"); log.flush()

    rng = np.random.default_rng(a.seed)
    frames = []
    for path, W, H, ns in ((os.path.join(ROOT, "test_v02.mp4"), 1920, 1080, (100, 312, 576)),
                           (os.path.join(ROOT, "test.mp4"), 640, 360, (0, 300, 450))):
        for n in ns:
            frames.append(("%s:%d" % (os.path.basename(path), n), frame_planes(path, n, W, H)))
    t0 = time.time()
    agg = dict(cases=0, ok=0, notimpl_both=0, mismatch=0, words=0, dirlevels=0)
    with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR")) as td:
        for ci in range(a.cases):
            name, (Y, U, V) = frames[int(rng.integers(len(frames)))]
            pi = int(rng.integers(3))
            src = (Y, U, V)[pi]
            h = int(rng.choice([48, 64, 96, 128, 160, 200, 256])) + int(rng.integers(0, 8))
            w = int(rng.choice([48, 64, 96, 128, 160, 200, 256])) + int(rng.integers(0, 8))
            h, w = min(h, src.shape[0], a.maxside), min(w, src.shape[1], a.maxside)
            y0, x0 = int(rng.integers(0, src.shape[0] - h + 1)), int(rng.integers(0, src.shape[1] - w + 1))
            P = np.ascontiguousarray(src[y0:y0 + h, x0:x0 + w], F32)
            if rng.random() < 0.2:                               # synthetic: noise, edges, a ramp
                yy, xx = np.mgrid[0:h, 0:w]
                P = (F32(100) + F32(40) * (xx > w // 2) + F32(0.3) * yy + rng.standard_normal((h, w)) * rng.uniform(0.5, 8)).astype(F32)
            # the model: a 128 x 128 (or smaller) crop of the same source plane
            mh, mw = min(128, src.shape[0]), min(128, src.shape[1])
            my, mx = int(rng.integers(0, src.shape[0] - mh + 1)), int(rng.integers(0, src.shape[1] - mw + 1))
            model = models([np.ascontiguousarray(src[my:my + mh, mx:mx + mw], F32)])[0]
            if model is None:
                say("case %d: model failed, skipped" % ci); continue
            cfg = nsp.build_cfg(settings(rng))[pi]
            if rng.random() < a.set3:                            # set 3 (16 directions, family B) for any plane
                cfg = dict(cfg, set=3)
            L = int(cfg["levels"])
            sig = []
            for k in range(L):
                hk, wk = h >> k, w >> k
                kind = rng.integers(3)
                if kind == 0:
                    s = np.ones((hk, wk), F32)
                elif kind == 1:
                    s = (F32(1) + F32(0.2) * rng.standard_normal((hk, wk))).astype(F32)
                else:
                    s = np.full((hk, wk), F32(rng.uniform(0.3, 3)), F32)
                sig.append(np.abs(s).astype(F32))
            roi = None
            if rng.random() < 0.35:
                rh, rw = int(rng.integers(8, h + 1)), int(rng.integers(8, w + 1))
                roi = (int(rng.integers(0, h - rh + 1)), int(rng.integers(0, w - rw + 1)), rh, rw)
            # Python
            try:
                pcfg = dict(cfg, sigma=tuple(F32(0) for _ in cfg["sigma"])) if a.popcfg != 16 and not a.no_mirror else cfg
                ref = nsp.plane_filter(P, sig, pcfg, model, roi=roi)
                pst = 0
            except NotImplementedError:
                ref, pst = None, 3
            # C++
            fP, fs, fm, fc, fo = (os.path.join(td, x) for x in ("P.f32", "s.f32", "m.f32", "cfg.txt", "o.f32"))
            P.tofile(fP); np.concatenate([x.ravel() for x in sig]).tofile(fs); model[:2005].tofile(fm); write_cfg(fc, cfg)
            cmd = [PORT, "plane", str(w), str(h), fP, fs, fm, fc, fo]
            if roi:
                cmd += ["roi", *map(str, roi)]
            if a.rec:
                d = os.path.join(a.rec, "case%03d" % ci)
                os.makedirs(d, exist_ok=True)
                cmd += ["rec", d]
            if a.popdir != 16:
                cmd += ["popdir", str(a.popdir)]
            if a.popcfg != 16:
                cmd += ["popcfg", str(a.popcfg)]
            r = subprocess.run(cmd, capture_output=True, text=True)
            agg["cases"] += 1
            sets = cfg["set"]
            st = nsp.SET_TABLE[sets]
            sizes = nsp.level_sizes(w, h, L)
            rects = (nsp.level_rects(roi, sizes, nsp.SET_MARGIN[sets], L) if roi is not None
                     else [(0, 0, sizes[k][1], sizes[k][0], False) for k in range(L)])
            ndir = sum(1 for k in range(L) if st["ndir"] and model[80 + 80 * sets + 16 * k] != F32(-1e6)
                       and 2 * st["rA"] < rects[k][2] and 2 * st["rA"] < rects[k][3])
            desc = "case %3d %-14s plane %d %3dx%-3d set %d L %d b8 %.3g amount %s c90 %s roi %s" % (
                ci, name, pi, w, h, sets, L, float(cfg["b8"]), "/".join("%.3g" % float(x) for x in cfg["amount"][:L]),
                "on" if any(F32(x) != 0 for x in cfg["c90"]) else "off", roi)
            if r.returncode == 3 and pst == 3:
                agg["notimpl_both"] += 1
                say(desc + "  NOT_IMPLEMENTED (both)")
                continue
            if r.returncode != 0 or pst:
                agg["mismatch"] += 1
                say(desc + "  STATUS C++ %d / Python %d %s" % (r.returncode, pst, r.stderr[-200:]))
                continue
            out = np.fromfile(fo, F32)
            bad = int((out.view(np.uint32) != ref.ravel().view(np.uint32)).sum())
            agg["words"] += out.size
            agg["dirlevels"] += ndir
            if bad:
                agg["mismatch"] += 1
                say(desc + "  MISMATCH %d / %d words (max |d| %g)" % (bad, out.size, float(np.nanmax(np.abs(out - ref.ravel())))))
            else:
                agg["ok"] += 1
                say(desc + "  OK (%d words, directional levels %d)" % (out.size, ndir))
    say("TOTAL %s (%.0f s)" % (agg, time.time() - t0))
    sys.exit(1 if agg["mismatch"] else 0)


if __name__ == "__main__":
    main()
