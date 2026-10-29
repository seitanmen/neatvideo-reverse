"""polyfit of nv_lstsq (lsq_mine) vs numpy.polyfit on every _fill_curve system: all index subsets of 0..7
(deg 1 for 2 points, 2 for >= 3) and the end fits (deg 1 on 2 / 3 points), with several value laws."""
import sys, subprocess, itertools, numpy as np
rng = np.random.default_rng(int(sys.argv[3]) if len(sys.argv) > 3 else 2)
per = int(sys.argv[2])
def vals(r, k):
    law = k % 7
    if law == 0: return rng.uniform(0.05, 20, r)
    if law == 1: return 10.0 ** rng.uniform(-6, 6, r)
    if law == 2: return rng.integers(0, 5, r).astype(float)
    if law == 3: return np.full(r, rng.uniform(0.1, 9))
    if law == 4: return rng.normal(0, 3, r)
    if law == 5: return rng.uniform(0.5, 4, r) * 1.07
    return np.round(rng.uniform(0, 30, r), 3)
cases = []
for r in range(2, 9):
    for idx in itertools.combinations(range(8), r):
        for deg in ((1,) if r == 2 else (2, 1)):
            for t in range(per):
                cases.append((np.array(idx, float), vals(r, t), deg))
inp = "".join(f"{len(x)} {d} " + " ".join(v.hex() for v in x) + " " + " ".join(float(v).hex() for v in y) + "\n" for x, y, d in cases)
out = subprocess.run([sys.argv[1]], input=inp, capture_output=True, text=True).stdout.split("\n")
bad = 0
import warnings
warnings.simplefilter("ignore")
for (x, y, d), line in zip(cases, out):
    c = np.polyfit(x, y, d)
    got = [float.fromhex(t) for t in line.split()] if line != "FAIL" else []
    if len(got) != len(c) or any(np.float64(a).tobytes() != np.float64(b).tobytes() for a, b in zip(c, got)):
        bad += 1
        if bad <= 5: print("mismatch", x, y.tolist(), d, c.tolist(), got)
print("cases", len(cases), "mismatch", bad)
