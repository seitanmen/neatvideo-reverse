# Diagnostic: the c90c-approx (a,b) of Y for centre t vs neighbour t+off, on the noisy clip and on the clean clip
# (same v5 flow from the noisy luma), plus the warped-vs-centre PSNR. Single process.
import os, sys, subprocess, numpy as np
ROOT = "/Users/seitan/Desktop/work/reverse"
sys.path.insert(0, ROOT + "/reference/python")
import nvfilter as nv
E9 = ROOT + "/work/omo/wp_e9/clips/"
clip = sys.argv[1]; PL = int(sys.argv[3]) if len(sys.argv) > 3 else 0; frames = [int(x) for x in sys.argv[2].split(",")]
W, H = 1920, 1080
def read(path, n):
    raw = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-i", path, "-vf", "select=eq(n\\,%d)" % n, "-frames:v", "1",
                          "-f", "rawvideo", "-pix_fmt", "yuv420p", "-"], capture_output=True, check=True).stdout
    y = np.frombuffer(raw[:W * H], np.uint8).reshape(H, W).astype(np.float32)
    v = np.frombuffer(raw[W * H * 5 // 4:], np.uint8).reshape(H // 2, W // 2).astype(np.float32)
    return (y, v)
def wmeans(p, g=16):
    H, W = p.shape
    S = np.zeros((H + 1, W + 1)); S[1:, 1:] = np.cumsum(np.cumsum(p, 0, dtype=np.float64), 1)
    ys = np.arange(H // g) * g - g // 2; xs = np.arange(W // g) * g - g // 2
    ok = ((ys >= 0) & (ys + 2 * g <= H))[:, None] & ((xs >= 0) & (xs + 2 * g <= W))[None, :]
    ys, xs = np.clip(ys, 0, H - 2 * g)[:, None], np.clip(xs, 0, W - 2 * g)[None, :]
    w = 2 * g
    return ((S[ys + w, xs + w] - S[ys, xs + w] - S[ys + w, xs] + S[ys, xs]) / (w * w)), ok
def ab(x, y):
    n = x.size; den = n * (y * y).sum() - y.sum() ** 2
    a = (n * (x * y).sum() - y.sum() * x.sum()) / den
    return a, (x.sum() - a * y.sum()) / n
for t in frames:
    yc_n, yc_c = read(E9 + clip + "/noisy_p_y3.y4m", t), read(E9 + clip + "/clean_p.y4m", t)
    for off in (-2, -1, 1, 2):
        yn_n, yn_c = read(E9 + clip + "/noisy_p_y3.y4m", t + off), read(E9 + clip + "/clean_p.y4m", t + off)
        vy, vx = nv.estimate_flow_v5(yn_n[0], yc_n[0], 16, 16)
        FY, FX = nv.dense_flow(vy, vx, (H, W), 16)
        FYc, FXc = nv._to_grid(FY, (H // 2, W // 2)) / 2, nv._to_grid(FX, (H // 2, W // 2)) / 2
        out = []
        for c, n in ((yc_n[PL], yn_n[PL]), (yc_c[PL], yn_c[PL])):
            wp = nv.warp_plane(n, FY, FX) if PL == 0 else nv.warp_plane(n, FYc, FXc)
            g = 16 if PL == 0 else 8
            mx, ok = wmeans(c, g); my, _ = wmeans(wp, g)
            x, y = mx[ok], my[ok]
            a, b = ab(x, y)
            r = np.abs(x - y)
            q = r < np.percentile(r, 50)
            a2, b2 = ab(x[q], y[q])
            psnr = 10 * np.log10(255 ** 2 / ((wp - c) ** 2).mean())
            out.append("a=%.3f b=%+.2f |a,b on best-half blocks| a=%.3f b=%+.2f mean|d|=%.2f psnr=%.2f" % (a, b, a2, b2, r.mean(), psnr))
        print("%s t=%d off=%+d noisy: %s\n%s           clean: %s" % (clip, t, off, out[0], " " * len(clip), out[1]), flush=True)
