import sys, subprocess, itertools, numpy as np
rng = np.random.default_rng(1)
cases = []
for r in range(2, 9):
    for idx in itertools.combinations(range(8), r):
        for deg in (1, 2):
            if deg == 2 and r < 3: continue
            for t in range(int(sys.argv[2]) if len(sys.argv) > 2 else 20):
                x = np.array(idx, float) + 0.0
                y = rng.uniform(0.2, 8.0, r) * (10.0 ** rng.integers(-2, 2))
                cases.append((x, y, deg))
inp = "".join(f"{len(x)} {d} " + " ".join(v.hex() for v in x) + " " + " ".join(float(v).hex() for v in y) + "\n" for x, y, d in cases)
out = subprocess.run([sys.argv[1]], input=inp, capture_output=True, text=True).stdout.split("\n")
bad = 0
for (x, y, d), line in zip(cases, out):
    c = np.polyfit(x, y, d)
    got = [float.fromhex(t) for t in line.split()]
    if any(np.float64(a).tobytes() != np.float64(b).tobytes() for a, b in zip(c, got)):
        bad += 1
print(sys.argv[1], "cases", len(cases), "mismatch", bad)
