# Extra flat cases aiming at statuses 3 (not flat) and 4 (clipped channel), new inputs.
import os, struct, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import *
import ind_flat as ifl
def mk(kind, W, H, seed):
    rng = np.random.default_rng(seed); yy, xx = np.mgrid[0:H, 0:W].astype(np.float64)
    if kind == "texture":
        img = 128 + 60 * np.sin(xx / 7.0 + 3 * np.sin(yy / 23.0))[..., None] + 30 * np.sin(yy / 5.0)[..., None] + rng.normal(0, 2, (H, W, 3))
    elif kind == "Bsat":
        img = rng.normal(110, 3, (H, W, 3)); img[..., 2] = 255.0
    elif kind == "Gblack":
        img = rng.normal(60, 2, (H, W, 3)); img[..., 1] = 0.0
    elif kind == "lowfreq":
        img = 128 + 40 * np.sin(xx / 60.0)[..., None] * np.cos(yy / 45.0)[..., None] + rng.normal(0, 0.8, (H, W, 3))
    return np.ascontiguousarray(np.clip(img, 0, 255), F32)
cs = [(k, W, H, s, m) for (k, W, H, s) in [("texture", 640, 360, 1), ("Bsat", 900, 500, 2), ("Gblack", 640, 480, 3),
      ("lowfreq", 1024, 576, 4), ("texture", 1920, 1080, 5)] for m in (0, 1, 2)]
ifl.synth = lambda kind, W, H, seed: mk(kind, W, H, seed)
bad = 0
inp, outp = os.path.join(TMP, "flatx.in"), os.path.join(TMP, "flatx.out")
exp = [ifl.py((("syn", k, W, H, s), m)) for k, W, H, s, m in cs]
with open(inp, "wb") as f:
    f.write(struct.pack("<Q", len(cs)))
    for k, W, H, s, m in cs:
        img = mk(k, W, H, s); f.write(struct.pack("<iiii", W, H, m, 0)); f.write(img.tobytes())
subprocess.run([os.path.join(BIN, "nv_flat_test"), "flat", inp, outp, "-j", "2"], check=True)
c = ifl.C(open(outp, "rb").read())
for (k, W, H, s, m), (res, t, tr) in zip(cs, exp):
    gres, (gx, gy), steps, gtr = ifl.parse(c)
    ok = gres == res and len(gtr) == len(tr) and np.array_equal(np.asarray(t[1]).view(np.uint64), gy.view(np.uint64))
    for dd, (iv, fv) in zip(tr, gtr):
        ef = np.array([dd["mean"], dd["nz"], *dd["fl"], dd["E"], dd["R"], dd["score"]], np.float64).astype(F32)
        ei = [dd["size"], dd["i"], dd["x"], dd["y"], dd["w"], dd["h"], dd["ncl"], dd["pen"]]
        ok = ok and ei == iv and np.array_equal(ef.view(U32), fv.view(U32))
    bad += not ok
    print("ok" if ok else "MISMATCH", k, W, H, "mode", m, res, len(tr))
print(f"flat extra: {len(cs)-bad}/{len(cs)} bit-exact")
