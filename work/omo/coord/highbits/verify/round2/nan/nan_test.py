"""Verifier's non-finite inputs (gbrpf32le FFV1, 256x192, 8 frames) through the CLI; checks restore and finiteness."""
import os, subprocess, sys, numpy as np
from concurrent.futures import ThreadPoolExecutor
N = os.path.dirname(os.path.abspath(__file__)); REPRO = os.path.abspath(os.path.join(N, "../../../../../../../repro"))
W, H, F = 256, 192, 8
rng = np.random.default_rng(99)
def base():
    y, x = np.mgrid[0:H, 0:W]
    s = 0.2 + 0.6 * x / W; s[:64, :128] = 0.35
    return np.stack([np.stack([s * g + rng.normal(0, 0.012, (H, W)) for g in (1.0, 0.9, 1.1)]) for _ in range(F)]).astype("<f4")  # (F, 3[G,B,R], H, W)
def enc(a, path):
    p = subprocess.Popen(["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "gbrpf32le", "-s", f"{W}x{H}", "-r", "24", "-i", "-",
                          "-c:v", "ffv1", "-level", "4", "-strict", "experimental", "-pix_fmt", "gbrpf32le", path], stdin=subprocess.PIPE)
    p.stdin.write(a.tobytes()); p.stdin.close(); assert p.wait() == 0
    back = dec(path)
    assert back.tobytes() == a.tobytes(), "ffv1 not bit-lossless for this input"
def dec(path):
    return np.frombuffer(subprocess.run(["ffmpeg", "-v", "error", "-i", path, "-f", "rawvideo", "-pix_fmt", "gbrpf32le", "-"],
                                        capture_output=True).stdout, "<f4").reshape(-1, 3, H, W)
cases = {}
a = base(); a[3, 0] = np.nan; a[4, 2] = np.inf; a[5, 1] = -np.inf                     # whole planes non-finite
cases["whole_plane"] = (a, [])
a = base(); m = rng.random((H, W)) < 0.01
a[3, 0][m] = np.inf; a[3, 0][rng.random((H, W)) < 0.002] = 3.0e38; a[3, 2][rng.random((H, W)) < 0.005] = -np.inf
a[3, 2][rng.random((H, W)) < 0.002] = -3.0e38; a[4, 1][10:20, 10:20] = 1e30                  # huge finite + inf mixed
cases["huge_mix"] = (a, [])
a = base(); a[4, 1][10:20, 10:20] = 1e30
cases["huge_finite_only"] = (a, [])
a = base()
for k in (1, 2, 5, 6): a[k, :, 50:60, 50:60] = np.nan; a[k, 0, 100, :] = np.inf             # context frames only
cases["context_only"] = (a, ["--start", "3", "--frames", "2"])
a = base(); sel = rng.random(a.shape) < 0.001; v = rng.choice(np.array([np.nan, np.inf, -np.inf], "<f4"), sel.sum()); a[sel] = v
nanpay = np.array([0x7fc00123, 0xffc00001], "<u4").view("<f4"); a[2, 0, 0, 0], a[2, 1, 0, 1] = nanpay    # NaN payloads
cases["scatter"] = (a, [])
cases["scatter_mix1"] = (a, ["--mix", "1"])
cases["whole_plane_mix1"] = (cases["whole_plane"][0], ["--mix", "1"])
def run(item):
    name, (a, args) = item
    src, dst = os.path.join(N, name + "_in.mkv"), os.path.join(N, name + "_out.mkv")
    if not os.path.exists(src): enc(a, src)
    r = subprocess.run([sys.executable, "denoise_video_v5.py", src, dst, "--depth", "32", "--jobs", "2", *args], cwd=REPRO, capture_output=True, text=True)
    open(os.path.join(N, name + ".log"), "w").write(r.stdout + r.stderr)
    return name, a, args, r
def summarize(name, a, args, r):
    if r.returncode:
        return f"{name}: rc={r.returncode} {(r.stdout + r.stderr).strip().splitlines()[-1][:200]}"
    o = dec(os.path.join(N, name + "_out.mkv"))
    lo = int(args[args.index("--start") + 1]) if "--start" in args else 0
    i = a[lo:lo + o.shape[0]]
    nf = ~np.isfinite(i)
    bits_ok = np.array_equal(i.view("<u4")[nf], o.view("<u4")[nf])
    others_finite = bool(np.isfinite(o[~nf]).all())
    s = f"{name} {' '.join(args)}: rc=0 frames={o.shape[0]} in_nonfinite={int(nf.sum())} out_nonfinite={int((~np.isfinite(o)).sum())} restored_bitexact={bits_ok} others_finite={others_finite}"
    if "--mix" in args:
        d = np.abs(o[~nf].astype(np.float64) - i[~nf].astype(np.float64)); rel = d / np.maximum(np.abs(i[~nf]), 1)
        s += f" mix1 max|diff| on finite={d.max():.3g} (max rel {rel.max():.3g})"
    else:
        fin = ~nf & (np.abs(i) < 10)
        s += f" normal-range out max|Δ| {np.abs(o[fin] - i[fin]).max():.3g}; out range finite [{np.nanmin(np.where(np.isfinite(o), o, np.nan)):.3g},{np.nanmax(np.where(np.isfinite(o), o, np.nan)):.3g}]"
        k = 0 if name != "context_only" else None
    warn = [l for l in (r.stdout + r.stderr).splitlines() if "Warning" in l or "warning" in l]
    return s + (f" warnings={warn[:2]}" if warn else "")
with ThreadPoolExecutor(3) as ex:
    for name, a, args, r in ex.map(run, cases.items()):
        print(summarize(name, a, args, r), flush=True)
