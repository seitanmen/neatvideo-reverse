import os, subprocess, sys, numpy as np
HERE = os.path.dirname(os.path.abspath(__file__)); WRAP = os.path.abspath(os.path.join(HERE, "../../ofx/wrap.py"))
REPRO = os.path.abspath(os.path.join(HERE, "../../../../../../../repro"))
W, H, N = 320, 192, 5
rng = np.random.default_rng(5); y, x = np.mgrid[0:H, 0:W]
env = dict(os.environ, QT_QPA_PLATFORM="offscreen", NV_UI_QT_SETTINGS=os.path.join(HERE, "settings.ini"))
fs = W * H + 2 * (W // 2) * (H // 2)
def frames(kind):
    out = []
    for t in range(N):
        Y = (40 + 180 * x / W + rng.normal(0, 3, (H, W))) / 255; Cb = (128 + rng.normal(0, 2, (H // 2, W // 2))) / 255; Cr = (128 + rng.normal(0, 2, (H // 2, W // 2))) / 255
        Y, Cb, Cr = Y.astype("<f4"), Cb.astype("<f4"), Cr.astype("<f4")
        if kind == "scatter":
            for p in (Y, Cb, Cr):
                m = rng.random(p.shape) < 0.002; p[m] = rng.choice(np.array([np.nan, np.inf, -np.inf], "<f4"), m.sum())
        elif kind == "wholeY" and t == 1:
            Y[:] = np.nan
        elif kind == "other_frames" and t != 1:
            Y[20:30, 20:30] = np.nan
        out.append((Y, Cb, Cr))
    return out
for kind in ("scatter", "wholeY", "other_frames"):
    fr = frames(kind)
    data = b"".join(p.tobytes() for f in fr for p in f)
    r = subprocess.run([sys.executable, WRAP, "--ofx", str(W), str(H), "25", "--depth", "32"], input=data, capture_output=True, cwd=REPRO, env=env, timeout=600)
    o = np.frombuffer(r.stdout, "<f4"); i = np.concatenate([p.ravel() for p in fr[1]])     # emitted frame = 1
    nf = ~np.isfinite(i)
    err = [l for l in r.stderr.decode(errors="replace").splitlines() if "propagateSizeHints" not in l and not l.startswith("qt.qpa")]
    print(f"{kind}: rc {r.returncode} bytes {len(r.stdout)} (expect {fs*4}) in_nf {int(nf.sum())} out_nf {int((~np.isfinite(o)).sum()) if o.size == i.size else '-'} "
          f"restored_bitexact {o.size == i.size and np.array_equal(i.view('<u4')[nf], o.view('<u4')[nf])} others_finite {o.size == i.size and bool(np.isfinite(o[~nf]).all())} stderr {err[-2:]}", flush=True)
