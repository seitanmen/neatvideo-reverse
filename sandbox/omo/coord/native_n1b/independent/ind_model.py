# Independent check of native nv_model (N1e) vs repro/nv_model.model_full on NEW planes (no caches).
import os, struct, subprocess, sys, time
from multiprocessing import Pool
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import *  # noqa

NANS = np.array([0x7fc00000, 0xffc00000, 0x7fd0beef, 0x7f800123, 0xff9abcde], U32)

def planes():
    rng = np.random.default_rng(0xC0FFEE)
    out = []
    def add(name, p): out.append((name, np.ascontiguousarray(p, F32)))
    add("normal 160x160", rng.normal(77, 6.5, (160, 160)))
    add("normal 75x130 (W75,H130)", rng.normal(30, 2, (130, 75)))
    add("uniform 130x75", rng.uniform(-20, 300, (75, 130)))
    c = rng.normal(0, 8, (115, 204)); c = (c[:-4, :-4] + c[4:, 4:] + c[2:-2, 2:-2]) / 3 + 120
    add("coloured 200x111", c)
    y, x = np.mgrid[0:47, 0:311]
    add("edges 311x47", np.where(np.sin((x * 0.6 + y * 0.8) / 3.3) > 0, 180.0, 60.0) + rng.normal(0, 1.5, (47, 311)))
    tile = rng.normal(0, 4, (6, 11))
    add("periodic 150x150", np.tile(tile, (26, 14))[:150, :150] + rng.normal(90, 1, (150, 150)))
    p = rng.normal(100, 4, (143, 97)).astype(F32); u = p.view(U32)
    idx = rng.integers(0, p.size, 60); u.ravel()[idx] = NANS[rng.integers(0, 5, 60)]
    add("payload NaN 97x143", p)
    add("normal 512x33", rng.normal(10, 3, (33, 512)))
    p = rng.normal(50, 1, (33, 33)).astype(F32); u = p.view(U32); u[3, 4] = 0x7f800001; u[30, 31] = 0xff800000; u[16, 16] = 0x7f800000
    add("snan/inf 33x33", p)
    p = rng.normal(200, 10, (89, 61)).astype(F32); p[rng.random(p.shape) < 0.01] = np.inf; p[-1, -1] = -np.inf
    add("inf 61x89", p)
    # real crops (frames the workers did not use)
    import nv_flat as nf
    f = rgb_frame("test_v02.mp4", 640)
    cr = f[700:860, 1500:1660]
    add("test_v02 n=640 Y 160^2 @(1500,700)", nf.transform(*(np.ascontiguousarray(cr[..., k]) for k in range(3)), 1)[0])
    f = rgb_frame("test.mp4", 510)
    add("test n=510 G 75x130 @(400,200)", f[200:330, 400:475, 1])
    f = rgb_frame("test_v02.mp4", 30)
    cr = f[900:990, 300:440]
    add("test_v02 n=30 Cb 140x90 @(300,900)", nf.transform(*(np.ascontiguousarray(cr[..., k]) for k in range(3)), 1)[2])
    return out

SETS = (-1, 0, 1, 2, 3, 4)

def py(job):
    p, s = job
    import warnings; warnings.simplefilter("ignore")
    import nv_model as nm
    try:
        with np.errstate(all="ignore"):
            w = nm.model_full(p, s)
        assert w.dtype == U32 and w.size == 4096, (w.dtype, w.size)
        return 0, np.asarray(w)
    except NotImplementedError:
        return 1, None
    except ValueError:
        return 2, None

def main():
    pl = planes()
    jobs = [(p, s) for _, p in pl for s in SETS]
    t0 = time.time()
    with Pool(2) as pool:
        exp = pool.map(py, jobs, chunksize=1)
    tpy = time.time() - t0
    inp, outp = os.path.join(TMP, "model.in"), os.path.join(TMP, "model.out")
    with open(inp, "wb") as f:
        f.write(struct.pack("<Q", len(jobs)))
        for p, s in jobs:
            f.write(struct.pack("<IIiI", p.shape[1], p.shape[0], s, 5)); f.write(p.tobytes())
    t1 = time.time()
    subprocess.run([os.path.join(BIN, "nv_model_test"), inp, outp], check=True)
    tc = time.time() - t1
    res = np.fromfile(outp, U32).reshape(len(jobs), 4097)
    bad = 0; k = 0; nonzero = 0
    for name, p in pl:
        for s in SETS:
            st, w = exp[k]; row = res[k]; k += 1
            ok = int(row[0]) == st and (st != 0 or np.array_equal(row[1:], w))
            if st == 0:
                nonzero += int(np.count_nonzero(w))
            if not ok:
                bad += 1
                d = np.nonzero(row[1:] != w)[0] if st == 0 else []
                print(f"MISMATCH {name} set {s}: status {int(row[0])}/{st}, {len(d)} words, first {list(d[:5])}")
            else:
                print(f"ok {name} set {s} status {st}")
    print(f"model independent: {len(jobs)-bad}/{len(jobs)} bit-exact ({len(pl)} planes x 6 sets, "
          f"{nonzero} nonzero expected words), python {tpy:.0f}s, C++ {tc:.1f}s")
    return 1 if bad else 0

if __name__ == "__main__":
    sys.exit(main())
