"""Independent check of R1/R2: old (backup) vs new backend+state at t_noise 100%, t_noise != 100 consistency,
cache keys, mask formula. Run from repro/: python3 ../sandbox/.../nlc_verify/v_r1.py VIDEO START"""
import importlib.machinery, importlib.util, os, sys, json
sys.path.insert(0, os.getcwd())
import numpy as np
from denoise_video_v5 import probe, read_frames
import nvfilter as nv
from nv_ui_qt import backend as new
from nv_ui_qt import state as nst
B = "/Users/seitan/Desktop/work/reverse/sandbox/omo/coord/backup/"
def load(name, f):
    ld = importlib.machinery.SourceFileLoader(name, B + f)
    sp = importlib.util.spec_from_loader(name, ld); m = importlib.util.module_from_spec(sp); ld.exec_module(m); return m
old = load("backend_old", "backend.py.20261026_pre_nlc")
ost = load("state_old", "state.py.20261026_pre_nlc")

video, s0 = sys.argv[1], int(sys.argv[2])
crop_area = None
info = probe(video)
frames = [tuple(np.array(p) for p in f) for _, f in read_frames(video, info["W"], info["H"], s0, s0 + 9, info["fps"])]
print(video, info["W"], info["H"], len(frames))
matrix = "bt709" if info["H"] >= 720 else "smpte170m"
prof = new.build_profile(frames, 4, False, matrix, "auto")
# JSON round trip like export
prof = json.loads(json.dumps(prof, default=lambda o: o.tolist() if hasattr(o, "tolist") else str(o)))
fails = []
def eq(a, b):
    return all(np.array_equal(x, y) for x, y in zip(a, b)) and len(a) == len(b)

Dn, Do = nst.default_params(), ost.default_params()
assert set(Dn) == set(Do), set(Dn) ^ set(Do)
sets = {
 "default": {},
 "nonuniform": dict(bamt_Cb_low=30.0, bamt_Y_high=80.0),
 "radius3_thr": dict(t_radius=3, adv_threshold=6.0) if "adv_threshold" in Dn else dict(t_radius=3),
 "amount50": dict(t_amount=50.0),
 "toff": dict(t_enable=False),
 "pyr": dict(adv_similarity="pyramid") if "adv_similarity" in Dn else {},
 "noise180_mix": dict(adv_noise_level=180.0, mix_enable=True, mix=30.0),
 "radius0": dict(t_radius=0),
}
for name, ch in sets.items():
    un, uo = dict(Dn, **ch), dict(Do, **ch)
    Pn, Po = nst.backend_params(un), ost.backend_params(uo)
    extra = set(Pn) - set(Po)
    same_other = all((Pn[k] == Po[k]) for k in Po)
    if extra != {"t_noise"} or not same_other or Pn["t_noise"] != 1.0:
        fails.append(("params", name, extra, same_other))
    R = int(Pn["radius"])
    for t in (0, 4, 8):
        idx = [i for i in range(t - R, t + R + 1) if 0 <= i < len(frames)]
        win = [tuple(p.astype(np.float32) for p in frames[i]) for i in idx]
        a = new.process_frame(win, idx.index(t), prof, Pn); b = old.process_frame(win, idx.index(t), prof, Po)
        new._init(prof, Pn); _, wa = new._work((0, [frames[i] for i in idx], idx.index(t)))
        old._init(prof, Po); _, wb = old._work((0, [frames[i] for i in idx], idx.index(t)))
        en, eo = new.PreviewEngine(frames, matrix, False), old.PreviewEngine(frames, matrix, False)
        ea, eb = en.process(t, prof, 1, Pn), eo.process(t, prof, 1, Po)
        ca, cb = en.process(t, prof, 1, Pn, (32, 48, 96, 160)), eo.process(t, prof, 1, Po, (32, 48, 96, 160))
        r = (eq(a, b), eq(wa, wb), eq(ea, eb), eq(ca, cb), eq(wa, a))
        if not all(r): fails.append(("100%", name, t, r))
    print(f"100% {name}: done")

# t_noise != 100
for tn in (0.0, 40.0, 137.0, 250.0):
    for name in ("default", "nonuniform", "noise180_mix"):
        u = dict(Dn, t_noise=tn, **sets[name]); P = nst.backend_params(u)
        assert P["t_noise"] == tn / 100
        R = int(P["radius"]); t = 4
        idx = [i for i in range(t - R, t + R + 1) if 0 <= i < len(frames)]
        win = [tuple(p.astype(np.float32) for p in frames[i]) for i in idx]; ci = idx.index(t)
        a = new.process_frame(win, ci, prof, P)
        new._init(prof, P); _, w = new._work((0, [frames[i] for i in idx], ci))
        # reference built from the OLD module: temporal with noise_level*tn, spatial with the original noise_level
        Q = dict(P, noise_level=P["noise_level"] * P["t_noise"])
        tmp, rv = old.temporal(win, ci, prof, Q)
        ref = old.finish(win, ci, tmp, rv, prof, P)
        en = new.PreviewEngine(frames, matrix, False)
        e = en.process(t, prof, 7, P)[0]
        er = en.to_rgb([np.clip(o, 0, 255) for o in a])
        r = (eq(a, ref), eq(w, a), np.array_equal(e, er), not eq(a, old.process_frame(win, ci, prof, P)) or tn == 100)
        if not all(r): fails.append(("tn", tn, name, r))
    print(f"t_noise {tn}: done")

# cache key: same engine, process at 100 then 40 then 100 again, compare with fresh engines
en = new.PreviewEngine(frames, matrix, False)
P100, P40 = nst.backend_params(Dn), nst.backend_params(dict(Dn, t_noise=40.0))
x100 = en.process(4, prof, 1, P100); x40 = en.process(4, prof, 1, P40); x100b = en.process(4, prof, 1, P100)
f40 = new.PreviewEngine(frames, matrix, False).process(4, prof, 1, P40)
m40 = en.mask(4, prof, 1, P40); m100 = en.mask(4, prof, 1, P100)
f_m40 = new.PreviewEngine(frames, matrix, False).mask(4, prof, 1, P40)
print("cache: 40 != 100", not eq(x40, x100), "40 == fresh", eq(x40, f40), "100 again same", eq(x100, x100b),
      "mask40 == fresh", np.array_equal(m40, f_m40), "mask40 != mask100", not np.array_equal(m40, m100))
if eq(x40, x100) or not eq(x40, f40) or not eq(x100, x100b) or not np.array_equal(m40, f_m40): fails.append("cache")
print("temporal cache keys:", [k[3:] for k in en._temporal.keys()][:3])

# mask formula (independent, float64)
def mref(rv, n, a):
    if a <= 0 or n <= 1: return np.zeros_like(rv, dtype=np.float64)
    rmin = ((1 - a) + a / n) ** 2 + a * a * 0.64 * (n - 1) / n ** 2
    return np.clip((1 / rv.astype(np.float64) - 1) / (1 / rmin - 1), 0, 1)
for tn in (0.0, 40.0, 100.0, 250.0):
    for amt in (100.0, 50.0, 0.0):
        for R in (2, 1, 0):
            P = nst.backend_params(dict(Dn, t_noise=tn, t_amount=amt, t_radius=R))
            for t in (0, 4):
                m = en.mask(t, prof, 1, P)
                idx = [i for i in range(t - R, t + R + 1) if 0 <= i < len(frames)]
                win = [tuple(p.astype(np.float32) for p in frames[i]) for i in idx]
                _, rv = new.temporal(win, idx.index(t), prof, P)
                mr = mref(rv[0], len(idx), P["temporal"])
                d = float(np.abs(m - mr).max())
                ok = m.dtype == np.float32 and m.min() >= 0 and m.max() <= 1 and not np.isnan(m).any() and d < 1e-5 \
                     and m.shape == frames[t][0].shape
                if amt == 0 or R == 0: ok = ok and not m.any()
                if not ok: fails.append(("mask", tn, amt, R, t, d))
            if R == 2 and amt == 100: print(f"mask t_noise {tn:5.1f}: mean m(t=4) {float(m.mean()):.4f}")
# repeated frame
for n_, a in ((5, 1.0), (3, 1.0), (5, 0.5), (11, 0.8), (2, 1.0)):
    pl = tuple(p.astype(np.float32) for p in frames[4])
    for tn in (100.0, 0.0, 250.0):
        P = nst.backend_params(dict(Dn, t_noise=tn, t_amount=a * 100, t_radius=5))
        _, rv = new.temporal([pl] * n_, n_ // 2, prof, P)
        m = new.temporal_mask(rv[0], n_, P["temporal"])
        if m.min() < 0.9999: fails.append(("repeat", n_, a, tn, float(m.min())))
    print(f"repeat n={n_} a={a}: min m {float(m.min()):.6f}")
print("FAILS:", fails)
print("RESULT", "OK" if not fails else "FAIL")
