# quick check of native/nv_model_test against repro/nv_model.model_full on a few planes
import os, struct, subprocess, sys, time
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "repro"))
sys.dont_write_bytecode = True
import numpy as np
import nv_model as nm
F32 = np.float32; U32 = np.uint32
EXE = os.path.join(ROOT, "native", "nv_model_test")

def region(w):
    if w < 55: return "levels"
    for name, lo, hi in (("dc", 0xdc, 0x140), ("A", 0x140, 0x780), ("R", 0x780, 0x7e4), ("B", 0x7e4, 0xe24),
                         ("C", 0xe24, 0x1464), ("score", 0x1464, 0x1504), ("offs", 0x1504, 0x1f04), ("bmq", 0x1f04, 0x1f54)):
        if lo <= 4 * w < hi: return name
    return "rest"

def run(cases):
    inp = os.path.join(HERE, "tmp", "q_in.bin"); outp = os.path.join(HERE, "tmp", "q_out.bin")
    with open(inp, "wb") as f:
        f.write(struct.pack("<Q", len(cases)))
        for p, s in cases:
            H, W = p.shape
            f.write(struct.pack("<IIiI", W, H, s, 5)); f.write(np.ascontiguousarray(p, F32).tobytes())
    t = time.time(); r = subprocess.run([EXE, inp, outp], capture_output=True, text=True); tc = time.time() - t
    print(r.stderr.strip(), f"wall {tc:.2f}")
    res = np.fromfile(outp, U32).reshape(len(cases), 4097)
    for (p, s), row in zip(cases, res):
        t = time.time()
        try:
            e = nm.model_full(p, s); st = 0
        except NotImplementedError:
            e = None; st = 1
        tp = time.time() - t
        if st or row[0]:
            print(p.shape, s, "status py", st, "c++", row[0]); continue
        d = np.nonzero(row[1:] != e)[0]
        regs = {}
        for w in d: regs[region(w)] = regs.get(region(w), 0) + 1
        print(p.shape, s, f"py {tp:.2f}s", "ndiff", d.size, regs, [ (int(w), hex(row[1+w]), hex(e[w])) for w in d[:4]])

rng = np.random.default_rng(int(sys.argv[1]) if len(sys.argv) > 1 else 1)
cases = []
for (H, W) in [(64, 64), (128, 128), (45, 37), (100, 90)]:
    p = rng.normal(100, 5, (H, W)).astype(F32)
    for s in (-1,):
        cases.append((p, s))
run(cases)
