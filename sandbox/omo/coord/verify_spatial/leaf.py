import os, sys, subprocess, numpy as np
sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
NVH = "/Users/seitan/Desktop/work/reverse/neatvideo_ofx_614/analysis/harness/nvh"
sys.path.insert(0, HERE)
import nv_spatial_copy as S
r = subprocess.run([NVH, "selftest"], capture_output=True, text=True)
assert r.returncode == 3 and "genuine alert suppressed" in r.stderr, r.returncode
T = os.path.join(HERE, "tmp")
f32 = np.float32
def wr(n, a): p = os.path.join(T, n); np.ascontiguousarray(a, f32).tofile(p); return p
def nv(*a):
    r = subprocess.run([NVH, *map(str, a)], capture_output=True, text=True, timeout=120)
    if r.returncode: raise RuntimeError((a, r.returncode, r.stderr[-300:]))
def rd(n, k): return np.fromfile(os.path.join(T, n), f32, k)
def bits(a): return np.ascontiguousarray(a, f32).view(np.uint32)
def eq(a, b):  # bit equal, NaN payload-insensitive? report both
    ba, bb = bits(a), bits(b)
    return int((ba != bb).sum()), int((~(np.isnan(a) & np.isnan(b)) & (ba != bb)).sum())
rng = np.random.default_rng(7)
def inputs(t, n):
    L = []
    L.append(rng.uniform(0, 3*t, n))
    L.append(rng.normal(0, 2*t, n))
    L.append(np.abs(rng.normal(0, t, n)))
    sp = np.array([0, -0.0, t, 2*t, np.nextafter(f32(t), f32(9)), np.nextafter(f32(t), f32(-9)), np.nextafter(f32(2*t), f32(9)), np.nextafter(f32(2*t), f32(0)),
                   1e-30, 1e30, np.inf, -np.inf, np.nan, 1.5*t, 3*t, -t, 1e-45], f32)
    base = np.resize(sp, n); L.append(base)
    return [f32(x) for x in L]
tot = {"ramp": [0,0,0], "ramp_np": [0,0,0], "wmul": [0,0,0], "wmul_np": [0,0,0], "down2": [0,0,0]}
cases = 0
ts = [1.1, 1.1*1.5, 0.0123, 3.3, 1e-6*1.1, 7.7, 2.0, 1.0, 0.0, 1e-8]
for t in ts:
    for n in [1,2,3,4,5,6,7,8,9, 13, 64, 1000, 4099]:
        for k, x in enumerate(inputs(f32(t), n)):
            x = x[:n]
            p = wr("in.f32", x); q = wr("prev.f32", rng.normal(0, 3, n))
            prev = np.fromfile(q, f32)
            nv("ramp", repr(float(f32(t))), p, n, os.path.join(T, "o.f32")); g = rd("o.f32", n)
            tt = f32(t)
            with np.errstate(all="ignore"):
                ours = np.clip((f32(2)*tt - x)/tt, 0, 1).astype(f32)
                # candidate emulation of 009ee93c: slope=(0-1)/(b-a), icpt = 1 - a*slope
                a = tt; b = f32(2)*tt
                if (b - a) <= b*f32(1e-5): a = a*f32(0.99999)+f32(-1e-5); b = b*f32(1.00001)+f32(1e-5)
                slope = f32(f32(0)-f32(1))/f32(b-a); icpt = f32(1)-f32(a*slope)
                emu = np.minimum(np.maximum(icpt + x*slope, 0), 1).astype(f32)  # fmin(.,c) then fmax(.,d)
            c = eq(g, ours); tot["ramp_np"][0] += 1; tot["ramp_np"][1] += c[0]; tot["ramp_np"][2] += c[1]
            c = eq(g, emu); tot["ramp"][0] += 1; tot["ramp"][1] += c[0]; tot["ramp"][2] += c[1]
            nv("wmul", repr(float(tt)), p, q, n, os.path.join(T, "o2.f32")); g2 = rd("o2.f32", n)
            with np.errstate(all="ignore"):
                ours2 = (prev*ours).astype(f32); emu2 = (prev*emu).astype(f32)
            c = eq(g2, ours2); tot["wmul_np"][0]+=1; tot["wmul_np"][1]+=c[0]; tot["wmul_np"][2]+=c[1]
            c = eq(g2, emu2); tot["wmul"][0]+=1; tot["wmul"][1]+=c[0]; tot["wmul"][2]+=c[1]
            cases += 1
print("cases", cases)
for k, v in tot.items():
    if k != "down2": print(k, "runs", v[0], "elem_mismatch(bits)", v[1], "non-NaN-pair mismatch", v[2])
# down2
bad = 0; runs = 0
for (W, H) in [(2,2),(3,3),(4,4),(5,7),(8,6),(9,9),(16,16),(33,17),(100,57),(257,129),(64,3),(3,64),(2,9),(11,2)]:
    for kind in range(4):
        if kind == 0: a = rng.normal(0, 50, W*H)
        elif kind == 1: a = rng.uniform(-1e30, 1e30, W*H)
        elif kind == 2:
            a = rng.normal(0, 5, W*H); a[rng.integers(0, W*H, max(1, W*H//7))] = rng.choice([np.nan, np.inf, -np.inf, 1e38, -1e38, 1e-45], max(1, W*H//7))
        else: a = np.round(rng.normal(0, 100, W*H)) 
        a = f32(a)
        p = wr("pl.f32", a)
        nv("down2", W, H, p, os.path.join(T, "d.f32"))
        w2, h2 = W//2, H//2
        g = rd("d.f32", w2*h2)
        with np.errstate(all="ignore"): o = S.halve(a.reshape(H, W)).reshape(-1)
        c = eq(g, o); runs += 1; bad += c[1]
        if c[1]: print("down2 mismatch", W, H, kind, c)
print("down2 runs", runs, "mismatching", bad)
