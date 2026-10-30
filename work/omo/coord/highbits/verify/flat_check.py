"""Noise std in the flat patch (rows < H/3, cols < W/2, inner margin) input vs processed, per plane, frame 4."""
import os, sys, json, subprocess
import numpy as np
sys.path.insert(0, "../../../../../repro"); import nv_depth as nd
S, O = "syn", "out"
cases = {"y420p10_odd": ("yuv420p10le", 259, 195), "y422p10_odd": ("yuv422p10le", 323, 257), "y444p12": ("yuv444p12le", 300, 222),
         "y444p16": ("yuv444p16le", 288, 256), "gbrp12": ("gbrp12le", 270, 210), "gbrpf32": ("gbrpf32le", 320, 240)}
for n, (pix, W, H) in cases.items():
    f = nd.pixfmt(pix); fb = f.frame_bytes(W, H)
    a = np.load(f"{S}/{n}_raw.npy").tobytes()
    b = subprocess.run(["ffmpeg", "-v", "error", "-i", f"{O}/{n}_proc.mkv", "-f", "rawvideo", "-pix_fmt", pix, "-"], capture_output=True).stdout
    k = 4
    pa = nd.split(a[k * fb:(k + 1) * fb], f, W, H); pb = nd.split(b[k * fb:(k + 1) * fb], f, W, H)
    row = [n]
    for i, (x, y) in enumerate(zip(pa, pb)):
        h, w = x.shape
        sl = (slice(4, h // 3 - 4), slice(4, w // 2 - 4)) if w == W else (slice(4, h // 3 - 4), slice(4, w // 2 - 4))
        if i > 0 and h < H:
            sl = (slice(2, h // 3 - 2), slice(2, w // 2 - 2))
        row.append(f"p{i} std {x[sl].astype(float).std():.3f}->{y[sl].astype(float).std():.3f}")
    # HDR block for float: r plane rows 5:40 cols 5:60 (r is plane 2)
    if f.is_float:
        r_in, r_out = pa[2][8:37, 8:57].astype(float), pb[2][8:37, 8:57].astype(float)
        row.append(f"HDR block R mean {r_in.mean():.4f}->{r_out.mean():.4f} std {r_in.std():.4f}->{r_out.std():.4f}")
        bl_in, bl_out = pa[1][-27:-3, -47:-3].astype(float), pb[1][-27:-3, -47:-3].astype(float)
        row.append(f"neg block B mean {bl_in.mean():.4f}->{bl_out.mean():.4f}")
    js = json.load(open(f"{O}/{n}_proc.json"))
    row.append("sigma " + json.dumps(js["profile"].get("sigma")) + f" area {js['profile'].get('area')} flat_status {js['profile'].get('flat_status')}")
    print(*row)
