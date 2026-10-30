"""Verifier's own synthetic sources (fresh, independent of the implementer's make_synth.py).
Writes raw planes with numpy -> ffmpeg ffv1 / exr, then decodes back and checks the encode was lossless."""
import os, subprocess, sys
import numpy as np
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "syn")
rng = np.random.default_rng(20261026)
NF = 8

def scene(H, W, t):
    y, x = np.mgrid[0:H, 0:W].astype(np.float64)
    base = 0.25 + 0.5 * x / W                     # ramp
    base[: H // 3, : W // 2] = 0.30               # flat patch (for FindFlatArea)
    base += 0.08 * np.sin((x + 3 * t) / 9.0) * (y > H // 2)   # moving texture
    return base

def planes_yuv(H, W, chroma, bits, t, sig=0.012):
    top = 2 ** bits - 1
    s = 2 ** (bits - 8)
    Y = (16 + 219 * scene(H, W, t)) * s + rng.normal(0, sig * 219 * s, (H, W))
    ch = {"420": ((H + 1) // 2, (W + 1) // 2), "422": (H, (W + 1) // 2), "444": (H, W)}[chroma]
    U = 128 * s + 20 * s * np.sin(np.arange(ch[1]) / 17.0)[None, :] + rng.normal(0, sig * 224 * s, ch)
    V = 128 * s - 15 * s * np.cos(np.arange(ch[0]) / 13.0)[:, None] + rng.normal(0, sig * 224 * s, ch)
    return [np.clip(np.rint(p), 0, top).astype("<u2") for p in (Y, U, V)]

def planes_gbr(H, W, bits, t, flt):
    sc = scene(H, W, t)
    r = sc * 1.1 + rng.normal(0, 0.012, (H, W)); g = sc + rng.normal(0, 0.012, (H, W)); b = sc * 0.9 + rng.normal(0, 0.012, (H, W))
    if flt:
        r[5:40, 5:60] = 1.6 + rng.normal(0, 0.01, (35, 55))      # HDR > 1 block
        b[-30:, -50:] = -0.05 + rng.normal(0, 0.01, (30, 50))     # negative block
        return [p.astype("<f4") for p in (g, b, r)]
    top = 2 ** bits - 1
    return [np.clip(np.rint(p * top), 0, top).astype("<u2") for p in (g, b, r)]

def enc(name, pix, W, H, frames, args=("-c:v", "ffv1", "-level", "3"), out=None):
    out = out or os.path.join(OUT, name + ".mkv")
    p = subprocess.Popen(["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", pix, "-s", f"{W}x{H}", "-r", "24",
                          "-i", "-", *args, "-pix_fmt", pix, out], stdin=subprocess.PIPE)
    raw = b"".join(b"".join(pl.tobytes() for pl in f) for f in frames)
    p.stdin.write(raw); p.stdin.close(); assert p.wait() == 0
    dec = subprocess.run(["ffmpeg", "-v", "error", "-i", out, "-f", "rawvideo", "-pix_fmt", pix, "-"], capture_output=True).stdout
    print(name, pix, W, H, "lossless" if dec == raw else f"NOT LOSSLESS {len(dec)} vs {len(raw)}", flush=True)
    np.save(os.path.join(OUT, name + "_raw.npy"), np.frombuffer(raw, np.uint8))

cases = [("y420p10_odd", "yuv420p10le", "420", 10, 259, 195),
         ("y422p10_odd", "yuv422p10le", "422", 10, 323, 257),
         ("y422p12", "yuv422p12le", "422", 12, 320, 240),
         ("y444p12", "yuv444p12le", "444", 12, 300, 222),
         ("y444p16", "yuv444p16le", "444", 16, 288, 256),
         ("y420p16", "yuv420p16le", "420", 16, 256, 256),
         ("y422p10_even", "yuv422p10le", "422", 10, 320, 256)]
for name, pix, ch, bits, W, H in cases:
    enc(name, pix, W, H, [planes_yuv(H, W, ch, bits, t) for t in range(NF)])
enc("gbrp12", "gbrp12le", 270, 210, [planes_gbr(210, 270, 12, t, False) for t in range(NF)])
fr = [planes_gbr(240, 320, 32, t, True) for t in range(NF)]
enc("gbrpf32", "gbrpf32le", 320, 240, fr, ("-c:v", "ffv1", "-level", "4", "-strict", "experimental"))
os.makedirs(os.path.join(OUT, "exr"), exist_ok=True)
enc("exrseq", "gbrpf32le", 320, 240, fr, ("-c:v", "exr", "-f", "image2"), os.path.join(OUT, "exr", "in_%04d.exr"))
