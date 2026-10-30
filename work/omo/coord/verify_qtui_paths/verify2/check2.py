"""Check OUTDIR: split stdout.bin into frames, compare with refs/previews.  usage: check2.py OUTDIR W H DEPTH EXPECT_COUNT"""
import json, sys, numpy as np
sys.path.insert(0, "/Users/seitan/Desktop/work/reverse/repro")
out, W, H, D, exp = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5])
import nvfilter as nv
steps = json.load(open(out + "/steps.json"))
data = open(out + "/stdout.bin", "rb").read()
bpp = {8: 1, 16: 2, 32: 4}[D]
fs = (W * H + 2 * (W // 2) * (H // 2)) * bpp
print(f"stdout bytes {len(data)}  frame_size {fs}  frames {len(data)/fs}  expected_frames {exp}  size_ok={len(data) == exp*fs}")
def dec(b):
    dt = {8: np.uint8, 16: "<u2", 32: "<f4"}[D]
    a = np.frombuffer(b, dt)
    ys = a[:W*H].reshape(H, W); cb = a[W*H:W*H+(W//2)*(H//2)].reshape(H//2, W//2); cr = a[W*H+(W//2)*(H//2):].reshape(H//2, W//2)
    return [ys, cb, cr]
prev_size = 0
n = 0
ok = len(data) == exp * fs
for i, s in enumerate(steps):
    if "size" not in s: continue
    delta = s["size"] - prev_size
    if s['name']=='exit' and i>0:
        s = dict(steps[i-1], size=s['size'], name='exit', rc=s.get('rc'))
    prev_size = s["size"]
    tag = ""
    if delta:
        if delta % fs: ok = False; tag = "PARTIAL-FRAME!"
        else:
            k = delta // fs
            frm = dec(data[s["size"] - fs: s["size"]])
            if "ref" in s:
                ref = np.load(s["ref"]); R = [ref[f"arr_{j}"] for j in range(3)]
                if D == 8: exp_arr = [np.clip(np.rint(r), 0, 255).astype(np.uint8) for r in R]; cmp = [int(np.abs(f.astype(int) - e.astype(int)).max()) for f, e in zip(frm, exp_arr)]
                elif D == 16: exp_arr = [np.clip(np.rint(r*256), 0, 65535) for r in R]; cmp = [int(np.abs(f.astype(int) - e.astype(int)).max()) for f, e in zip(frm, exp_arr)]
                else: cmp = [float(np.abs(f.astype(np.float64) - r.astype(np.float64)/255).max()) for f, r in zip(frm, R)]
                tag += f" maxdiff_vs_ref(Y,Cb,Cr)={cmp}"
                # preview comparison (rgb)
                if "prev" in s:
                    sc = {8: 1.0, 16: 1/256, 32: 255.0}[D]
                    pl = [f.astype(np.float32) * np.float32(sc) for f in frm]
                    rgb = np.clip(nv.yuv_to_rgb(pl, "bt709", False), 0, 255)
                    pv = np.load(s["prev"])
                    rr = np.clip(nv.yuv_to_rgb([r.astype(np.float32) for r in R], "bt709", False), 0, 255)
                    tag += f" refrgb_vs_preview max={np.abs(rr-pv).max():.3g}"
                    tag += f" rgb_vs_preview max={np.abs(rgb-pv).max():.3f} mean={np.abs(rgb-pv).mean():.4f}"
                else: tag += " (no preview)"
            else: tag += " (no ref)"
            tag = f"frames+{k} " + tag
    print(f"{i:2d} {s['name']:<22} delta={delta:>8} {tag}  | frame={s.get('frame')} {('rc=%s'%s['rc']) if 'rc' in s else ''}")
print("RESULT size/partial ok:", ok)
