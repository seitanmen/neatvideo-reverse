# Negative controls: the comparisons of ind_*.py must FAIL when the C++ input is perturbed by 1 ulp.
import os, struct, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from common import *  # noqa
import ind_model as im, ind_flat as ifl, ind_v5 as iv
res = {}
# model: 1 ulp in one pixel of the C++ input
name, p = im.planes()[0]
st, w = im.py((p, 0))
q = p.copy(); q.view(U32)[80, 80] += 1
inp, outp = os.path.join(TMP, "neg_model.in"), os.path.join(TMP, "neg_model.out")
with open(inp, "wb") as f:
    f.write(struct.pack("<Q", 1)); f.write(struct.pack("<IIiI", 160, 160, 0, 5)); f.write(q.tobytes())
subprocess.run([os.path.join(BIN, "nv_model_test"), inp, outp], check=True, capture_output=True)
row = np.fromfile(outp, U32)
res["model 1ulp"] = int(np.count_nonzero(row[1:] != w))
# flat: 1 ulp in one pixel inside the chosen square
spec = ("frame", "test.mp4", 60)
r, t, tr = ifl.py((spec, 1))
img = ifl.image(spec).copy(); img.view(U32)[r[1] + 10, r[0] + 10, 1] += 1
inp = os.path.join(TMP, "neg_flat.in"); outp = os.path.join(TMP, "neg_flat.out")
with open(inp, "wb") as f:
    f.write(struct.pack("<Q", 1)); f.write(struct.pack("<iiii", img.shape[1], img.shape[0], 1, 0)); f.write(img.tobytes())
subprocess.run([os.path.join(BIN, "nv_flat_test"), "flat", inp, outp], check=True)
c = ifl.C(open(outp, "rb").read()); gres, _, _, gtr = ifl.parse(c)
nd = 0
for dd, (ivv, fv) in zip(tr, gtr):
    ef = np.array([dd["mean"], dd["nz"], *dd["fl"], dd["E"], dd["R"], dd["score"]], np.float64).astype(F32)
    nd += int(np.count_nonzero(ef.view(U32) != fv.view(U32)))
res["flat 1ulp trace floats differing"] = nd
# v5: threshold +1 ulp (double) in the C++ params only
cs = [c for c in iv.build_cases() if c["label"].startswith("rand NaN 70x90 444 nf=5 temporal")]
c0 = cs[0]; e = iv.expected(c0)
P2 = dict(c0["P"], threshold=float(np.nextafter(F32(c0["P"]["threshold"]), F32(10))))
blob = vc = None
import v5_cases as vc
inp, outp = os.path.join(TMP, "neg_v5.in"), os.path.join(TMP, "neg_v5.out")
planes = [pp for fr in c0["win"] for pp in fr]
vc.write_cases(inp, [vc.encode_case("temporal", c0["prof"], P2, ints=[c0["ci"], len(c0["win"]), 0], planes=planes)])
subprocess.run([os.path.join(BIN, "nv_v5_test"), inp, outp], check=True)
g = iv.read(outp, 1)[0]
res["v5 threshold+1ulp differing words"] = sum(int(np.count_nonzero(np.asarray(x, F32).view(U32) != y.view(U32))) for x, y in zip(e, g))
print(res)
