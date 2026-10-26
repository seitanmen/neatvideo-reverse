"""OFX round trip with synthetic frames: sizes, low-bit use, HDR (>1, <0) kept at depth 32, ragged stdin."""
import os, subprocess, sys, numpy as np
HERE = os.path.dirname(os.path.abspath(__file__)); REPRO = os.path.abspath(os.path.join(HERE, "../../../../../../repro"))
W, H, N = 320, 192, 5
rng = np.random.default_rng(7)
y, x = np.mgrid[0:H, 0:W]
frames = []
for t in range(N):
    Y = 40 + 180 * x / W + rng.normal(0, 3, (H, W)); Y[:40, :80] = 90 + rng.normal(0, 3, (40, 80))
    Y[150:, 250:] = 250 + rng.normal(0, 3, (42, 70))     # above 235 (superwhite)
    Cb = 128 + rng.normal(0, 2, (H // 2, W // 2)); Cr = 128 + rng.normal(0, 2, (H // 2, W // 2))
    frames.append((Y, Cb, Cr))
env = dict(os.environ, QT_QPA_PLATFORM="offscreen", NV_UI_QT_SETTINGS=os.path.join(HERE, "settings.ini"))
fs8 = W * H + 2 * (W // 2) * (H // 2)
def run(depth, data, extra=()):
    r = subprocess.run([sys.executable, os.path.join(HERE, "wrap.py"), "--ofx", str(W), str(H), "25", *extra, "--depth", str(depth)],
                       input=data, capture_output=True, cwd=REPRO, env=env, timeout=600)
    return r
res = {}
for depth in (8, 16, 32):
    if depth == 8:
        data = b"".join(np.clip(np.rint(p), 0, 255).astype(np.uint8).tobytes() for f in frames for p in f)
    elif depth == 16:
        data = b"".join(np.clip(np.rint(p * 256), 0, 65535).astype("<u2").tobytes() for f in frames for p in f)
    else:
        fr = [list(f) for f in frames]
        fr[2][0] = fr[2][0].copy(); fr[2][0][:10, 100:140] = -20.0       # negative Y in the current frame (t = N//3 = 1? see below)
        data = b"".join((np.asarray(p) / 255).astype("<f4").tobytes() for f in fr for p in f)
    r = run(depth, data)
    dt = {8: np.uint8, 16: "<u2", 32: "<f4"}[depth]; sz = np.dtype(dt).itemsize
    o = np.frombuffer(r.stdout[: len(r.stdout) // sz * sz], dt)
    err = [l for l in r.stderr.decode(errors="replace").splitlines() if "propagateSizeHints" not in l and not l.startswith("qt.qpa")]
    print(f"depth {depth}: rc {r.returncode} stdout {len(r.stdout)} B (expect {fs8 * sz}) min {o.min():.6g} max {o.max():.6g} stderr {err[-3:]}")
    res[depth] = o
    if depth == 16: print("   out16 % 256 != 0:", np.mean(o % 256 != 0))
    if depth == 32:
        Yo = o[:W * H].reshape(H, W) * 255
        print("   out32 Y superwhite block mean (8-bit units):", Yo[155:187, 255:315].mean(), "max", Yo.max(), " >235 frac", np.mean(Yo > 235.5))
# which frame was emitted? pf = min(N//3, N-3) = 1 for N=5
print("emitted frame index per main.py: pf =", min(N // 3, max(N - 3, 0)))
o8 = res[8].astype(float); print("|out16/256-out8| max", np.abs(res[16] / 256 - o8).max(), " |out32*255-out8| max", np.abs(res[32].astype(float) * 255 - o8).max())
# ragged stdin (half a frame extra) and MAX_FRAMES
data = b"".join(np.clip(np.rint(p * 256), 0, 65535).astype("<u2").tobytes() for f in frames for p in f)
r = run(16, data + data[: fs8], ("2",))
print("ragged+MAX_FRAMES=2: rc", r.returncode, "stdout", len(r.stdout))
r = run(24, data)
print("--depth 24: rc", r.returncode, "stderr tail", r.stderr.decode()[-200:].strip().splitlines()[-1:] )
r = subprocess.run([sys.executable, os.path.join(HERE, "wrap.py"), "--ofx", "320"], capture_output=True, cwd=REPRO, env=env)
print("--ofx 320 (missing H): rc", r.returncode, r.stderr.decode().strip().splitlines()[-1:])
