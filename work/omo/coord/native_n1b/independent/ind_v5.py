# Independent check of native nv_v5_test (N1g temporal / N1h spatial) vs repro/nvfilter.py on NEW windows
# and parameter sets. Profiles are used directly from build_profile_v5 (no JSON round trip).
# Python outputs must be float32 (dtype checked) and are compared as uint32 words.
import os, struct, subprocess, sys, time, warnings
from multiprocessing import Pool
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import *  # noqa
sys.path.insert(0, os.path.join(ROOT, "native"))
import v5_cases as vc   # only the blob encoder (encode_case / write_cases)
import nvfilter as nv

d = nv.v5_params
PSETS = {
    "default": d(),
    "pyr1.2-thr6": d(similarity="pyramid", pyramid_scale=1.2, threshold=6.0, temporal=0.8, bs=16, search=6,
                     noise_level=0.9, channel_noise={"Y": 1.1, "Cb": 0.9, "Cr": 1.3}),
    "v5-bs24": d(threshold=3.0, bs=24, search=10, temporal=1.15, luma=0.55, chroma=0.8, spatial=1.5),
    "r1-mix": d(radius=1, mix=0.15, luma=0.55, chroma=0.8, spatial=1.5,
                band_amounts={"high": 0.9, "medium": 0.6, "low": 0.4, "very-low": 0.2, "ultra-low": 0.1}),
    "r3-pyr": d(radius=3, similarity="pyramid", pyramid_scale=0.7, band_noise={"high": 1.4, "medium": 0.8,
                "low": 1.2, "very-low": 0.6, "ultra-low": 2.0}, noise_level=1.4,
                channel_noise={"Y": 1.0, "Cb": 0.7, "Cr": 1.6}),
}

def window(video, lo, hi, fmt):
    return [tuple(yuv_frame(video, k, fmt)) for k in range(lo, hi)]

def profile(video, n, fmt):
    p0, p1 = yuv_frame(video, n, fmt), yuv_frame(video, n + 1, fmt)
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        return nv.build_profile_v5(p0, False, None, pair=p1, mode="auto",
                                   matrix="bt709" if p0[0].shape[0] >= 720 else "bt601")

def rand_window(seed, H, W, nf, grid):
    rng = np.random.default_rng(seed)
    base = rng.normal(120, 25, (H + 8, W + 8))
    base = nv.gaussian_filter(base, 2.0) * 3
    fr = []
    for k in range(nf):
        dy, dx = rng.integers(-3, 4, 2)
        Y = base[4 + dy:4 + dy + H, 4 + dx:4 + dx + W] + rng.normal(0, 3, (H, W))
        if grid == "420":
            C = [Y[::2, ::2] * 0.2 + 90 + rng.normal(0, 2, ((H + 1) // 2, (W + 1) // 2)) for _ in range(2)]
        else:
            C = [Y * 0.2 + 90 + rng.normal(0, 2, (H, W)) for _ in range(2)]
        pl = [np.ascontiguousarray(p, F32) for p in [Y] + C]
        for p in pl:
            u = p.view(U32)
            idx = rng.integers(0, p.size, max(1, p.size // 150))
            u.ravel()[idx] = np.array([0x7fc00000, 0xffc00000, 0x7fc0abcd, 0x7fa00001], U32)[rng.integers(0, 4, idx.size)]
        fr.append(tuple(pl))
    return fr

def rand_prof(seed, Lc):
    rng = np.random.default_rng(seed)
    return {"curve_centers": [16.0 + 32 * i for i in range(8)],
            "curve": {n: [float(v) for v in rng.uniform(0.5, 5, 8)] for n in ("Y", "Cb", "Cr")},
            "levels": {"Y": 5, "Cb": Lc, "Cr": Lc},
            "band_ratio": {"Y": [1.0, 0.55, 0.3, 0.18, 0.1], "Cb": [1.0, 0.6, 0.35, 0.2, 0.1][:Lc],
                           "Cr": [1.0, 0.5, 0.3, 0.15, 0.08][:Lc]},
            "tot_ratio": {"Y": 1.7, "Cb": 1.4, "Cr": 1.5}}

def build_cases():
    cs = []
    real = [("test_v02.mp4", 200, 207, "yuv420p"), ("test.mp4", 600, 607, "yuv420p"), ("test.mp4", 600, 607, "yuv444p")]
    for video, lo, hi, fmt in real:
        win = window(video, lo, hi, fmt)
        prof = profile(video, lo + 3, fmt)
        for name in ("default", "pyr1.2-thr6", "v5-bs24"):
            cs.append(dict(label=f"{video} {fmt} temporal n={lo+3} {name}", kind="temporal", win=win[1:6], ci=2,
                           prof=prof, P=PSETS[name]))
        for name in ("r1-mix", "r3-pyr"):
            cs.append(dict(label=f"{video} {fmt} frame n={lo+3} {name}", kind="frame", win=win, ci=3, prof=prof,
                           P=PSETS[name]))
        cs.append(dict(label=f"{video} {fmt} frame n={lo+1} edge ci=1 default", kind="frame", win=win, ci=1,
                       prof=prof, P=PSETS["default"]))
        cs.append(dict(label=f"{video} {fmt} spatial n={lo+3} r3-pyr", kind="spatial", win=win[1:6], ci=2,
                       prof=prof, P=PSETS["r3-pyr"]))
    # 4:2:2 temporal (not covered by the worker)
    win = window("test.mp4", 600, 605, "yuv422p")
    prof = profile("test.mp4", 602, "yuv422p")
    cs.append(dict(label="test.mp4 yuv422p temporal n=602 default", kind="temporal", win=win, ci=2, prof=prof, P=PSETS["default"]))
    cs.append(dict(label="test.mp4 yuv422p frame n=602 r1-mix", kind="frame", win=win, ci=2, prof=prof, P=PSETS["r1-mix"]))
    # random NaN windows
    for i, (H, W, nf, grid, name) in enumerate([(70, 90, 5, "444", "default"), (70, 90, 5, "420", "pyr1.2-thr6"),
                                                 (64, 97, 4, "444", "pyr1.2-thr6"), (81, 64, 6, "420", "r3-pyr")]):
        win = rand_window(100 + i, H, W, nf, grid)
        prof = rand_prof(200 + i, 5 if grid == "444" else 4)
        cs.append(dict(label=f"rand NaN {H}x{W} {grid} nf={nf} temporal {name}", kind="temporal", win=win, ci=nf // 2,
                       prof=prof, P=PSETS[name]))
        cs.append(dict(label=f"rand NaN {H}x{W} {grid} nf={nf} frame {name}", kind="frame", win=win, ci=nf // 2,
                       prof=prof, P=dict(PSETS[name], mix=0.3)))
    return cs

def expected(c):
    with warnings.catch_warnings(), np.errstate(all="ignore"):
        warnings.simplefilter("ignore")
        flows = {}
        if c["kind"] == "temporal":
            o, rv = nv.temporal_v5(c["win"], c["ci"], c["prof"], c["P"], flows)
            res = list(o) + list(rv)
        elif c["kind"] == "frame":
            res = list(nv.process_frame_v5(c["win"], c["ci"], c["prof"], c["P"], flows))
        else:   # spatial on the temporal output of the window (default temporal params)
            o, rv = nv.temporal_v5(c["win"], c["ci"], c["prof"], d(), {})
            c["sp_in"] = (o, rv)
            return [list(nv.spatial_v5(o, c["prof"], rv, c["P"])), o, rv]
        for off in sorted(flows):
            res += list(flows[off])
        return res

def enc(c, sp_in=None):
    if c["kind"] == "spatial":
        o, rv = sp_in
        return vc.encode_case("spatial", c["prof"], c["P"], planes=list(o) + list(rv))
    planes = [p for f in c["win"] for p in f]
    return vc.encode_case(c["kind"], c["prof"], c["P"], ints=[c["ci"], len(c["win"]), 0], planes=planes)

def read(path, n):
    data = open(path, "rb").read(); off = 0; out = []
    for _ in range(n):
        (k,) = struct.unpack_from("<I", data, off); off += 4; pl = []
        for _ in range(k):
            H, W = struct.unpack_from("<ii", data, off); off += 8
            pl.append(np.frombuffer(data, F32, H * W, off).reshape(H, W)); off += 4 * H * W
        out.append(pl)
    assert off == len(data)
    return out

def main():
    cs = build_cases()
    t0 = time.time()
    with Pool(2) as pool:
        exp = pool.map(expected, cs, chunksize=1)
    tpy = time.time() - t0
    blobs = []
    for c, e in zip(cs, exp):
        if c["kind"] == "spatial":
            blobs.append(enc(c, (e[1], e[2]))); 
        else:
            blobs.append(enc(c))
    inp, outp = os.path.join(TMP, "v5.in"), os.path.join(TMP, "v5.out")
    vc.write_cases(inp, blobs)
    subprocess.run([os.path.join(BIN, "nv_v5_test"), inp, outp, "-j", "2"], check=True)
    got = read(outp, len(cs))
    bad = 0; words = 0; nanw = 0
    for c, e, g in zip(cs, exp, got):
        if c["kind"] == "spatial":
            e = e[0]
        errs = []
        if len(e) != len(g): errs.append(f"nplanes {len(g)} vs {len(e)}")
        for i, (x, y) in enumerate(zip(e, g)):
            x = np.asarray(x)
            if x.dtype != F32 and x.dtype.kind == "f":
                errs.append(f"plane {i}: Python dtype {x.dtype}")
                continue
            if x.dtype != F32:                      # integer arrays (none expected): value compare
                x = x.astype(F32)
            ok, msg = bits_equal(x, y)
            words += x.size; nanw += int(np.isnan(x).sum())
            if not ok: errs.append(f"plane {i}: {msg}")
        if errs:
            bad += 1; print(f"MISMATCH [{c['label']}]: {errs[:4]}")
        else:
            print(f"ok [{c['label']}] {len(e)} planes")
    print(f"v5 independent: {len(cs)-bad}/{len(cs)} bit-exact ({words} words, {nanw} NaN words), python {tpy:.0f}s")
    return 1 if bad else 0

if __name__ == "__main__":
    sys.exit(main())
