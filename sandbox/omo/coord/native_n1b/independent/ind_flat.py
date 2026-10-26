# Independent check of native nv_flat (N1d) vs repro/nv_flat.find_flat_area_genuine on NEW inputs.
# Compared bit for bit: result (x, y, w, h, status), the C++ own curve (8 f64 x 2), every trace entry
# (8 ints + 8 f32). Reduced planes / candidates are parsed (skipped) only.
import os, struct, subprocess, sys, time
from multiprocessing import Pool
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import *  # noqa
import nv_flat as nf, nv_curve as ncv

FR = [("test_v02.mp4", n) for n in (30, 333, 640)] + [("test.mp4", n) for n in (60, 510, 800)]

def synth(kind, W, H, seed):
    rng = np.random.default_rng(seed)
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float64)
    if kind == "blocks":          # tiles with different noise levels; flat tiles
        img = np.zeros((H, W, 3))
        for by in range(0, H, 90):
            for bx in range(0, W, 90):
                img[by:by+90, bx:bx+90] = rng.uniform(30, 220) + rng.normal(0, rng.uniform(0.3, 9), (min(90, H-by), min(90, W-bx), 3))
    elif kind == "vignette":
        r = np.hypot(yy - H/2, xx - W/2) / max(H, W)
        img = (200 * (1 - r))[..., None] + rng.normal(0, 2.0, (H, W, 3)) * np.array([1.0, 0.7, 1.4])
    elif kind == "clipband":     # top third saturated white, rest noisy
        img = rng.normal(90, 6, (H, W, 3)); img[:H//3] = 255.0; img[:, :W//10, 2] = 0.0
    elif kind == "nanrows":
        img = rng.normal(140, 3, (H, W, 3)).astype(F32)
        img[::37] = np.nan
        u = img.view(U32); u[5:9, 100:140, 1] = 0x7fa12345   # sNaN payload
        img[H//2:H//2+3, :, 0] = -np.inf
        return np.ascontiguousarray(img, F32)
    elif kind == "hdr":          # values above 255 and negative values
        img = rng.normal(128, 80, (H, W, 3))
    img = np.clip(img, 0, 255) if kind != "hdr" else img
    return np.ascontiguousarray(img, F32)

SYN = [("blocks", 1280, 720, 1), ("vignette", 720, 480, 2), ("clipband", 1440, 1080, 3), ("nanrows", 800, 600, 4),
       ("hdr", 300, 170, 5), ("blocks", 2560, 1440, 6), ("vignette", 640, 96, 7), ("hdr", 52, 333, 8)]

def cases():
    out = []
    for v, n in FR:
        for m in (0, 1, 2):
            out.append((f"{v} n={n} mode {m}", ("frame", v, n), m))
    for k, W, H, s in SYN:
        for m in ((0, 1, 2, 3) if W * H <= 800 * 600 else (1,)):
            out.append((f"{k} {W}x{H} mode {m}", ("syn", k, W, H, s), m))
    return out

def image(spec):
    return rgb_frame(spec[1], spec[2]) if spec[0] == "frame" else synth(*spec[1:])

def py(args):
    spec, mode = args
    img = image(spec)
    tr, hold = [], {}
    with np.errstate(all="ignore"):
        def curve():
            t = ncv.genuine_noise_curve(img); hold["t"] = t; return ncv.noise_at(t)
        res = nf.find_flat_area_genuine(img, None, mode, trace=tr, curve=curve)
    return tuple(int(v) for v in res), hold.get("t"), tr

class C:
    def __init__(s, d): s.d, s.p = d, 0
    def a(s, dt, n):
        x = np.frombuffer(s.d, dt, n, s.p); s.p += x.nbytes; return x
    def i(s, n=1):
        x = s.a(np.int32, n); return int(x[0]) if n == 1 else [int(v) for v in x]

def parse(c):
    v = c.i(11)
    cx, cy = c.a(np.float64, 8), c.a(np.float64, 8)
    rn = int(c.a(np.int64, 1)[0]); [c.a(F32, rn) for _ in range(4)]
    for _ in range(c.i()):
        c.i(3); c.a(np.int64, 64); c.a(np.int64, 64); c.a(F32, 64); c.a(F32, 64)
        for _ in range(c.i()):
            c.i(4); c.a(F32, 3)
    tr = []
    for _ in range(c.i()):
        iv = c.i(8); fv = c.a(F32, 8); nw = c.i(); c.a(U32, nw); tr.append((iv, fv))
    return tuple(v[:5]), (cx, cy), v[5:], tr

def main():
    cs = cases()
    t0 = time.time()
    with Pool(2) as pool:
        exp = pool.map(py, [(s, m) for _, s, m in cs], chunksize=1)
    tpy = time.time() - t0
    inp = os.path.join(TMP, "flat.in")
    with open(inp, "wb") as f:
        f.write(struct.pack("<Q", len(cs)))
        for _, s, m in cs:
            img = image(s); H, W = img.shape[:2]
            f.write(struct.pack("<iiii", W, H, m, 0)); f.write(np.ascontiguousarray(img, F32).tobytes())
    bad = 0; ntr = 0; stat = {}
    for j in (1, 2):
        outp = os.path.join(TMP, f"flat_j{j}.out")
        subprocess.run([os.path.join(BIN, "nv_flat_test"), "flat", inp, outp, "-j", str(j)], check=True)
        c = C(open(outp, "rb").read())
        for (name, s, m), (res, t, tr) in zip(cs, exp):
            gres, (gx, gy), steps, gtr = parse(c)
            errs = []
            if gres != res: errs.append(f"result {gres} vs {res}")
            if t is not None:
                if not (np.array_equal(np.asarray(t[0], np.float64).view(np.uint64), gx.view(np.uint64)) and
                        np.array_equal(np.asarray(t[1], np.float64).view(np.uint64), gy.view(np.uint64))):
                    errs.append("curve")
            elif steps[4]:
                errs.append("C++ ran curve, Python did not")
            if len(gtr) != len(tr): errs.append(f"ntrace {len(gtr)} vs {len(tr)}")
            for k, (d, (iv, fv)) in enumerate(zip(tr, gtr)):
                ei = [d["size"], d["i"], d["x"], d["y"], d["w"], d["h"], d["ncl"], d["pen"]]
                ef = np.array([d["mean"], d["nz"], *d["fl"], d["E"], d["R"], d["score"]], np.float64).astype(F32)
                if ei != iv or not np.array_equal(ef.view(U32), fv.view(U32)):
                    errs.append(f"trace {k}"); break
            if j == 1:
                ntr += len(tr); stat[res[4]] = stat.get(res[4], 0) + 1
            if errs:
                bad += 1; print(f"MISMATCH -j{j} [{name}]: {errs[:3]}")
            elif j == 1:
                print(f"ok [{name}] {res} trace {len(tr)}")
        assert c.p == len(c.d), "trailing bytes"
        os.remove(outp)
    print(f"flat independent: {2*len(cs)-bad}/{2*len(cs)} runs bit-exact ({len(cs)} cases x -j1/-j2), "
          f"{ntr} trace entries/run, statuses {stat}, python {tpy:.0f}s")
    return 1 if bad else 0

if __name__ == "__main__":
    sys.exit(main())
