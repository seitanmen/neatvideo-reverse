"""Synthetic / converted test clips for the high-bit-depth tests (SPEC.md §4). Writes into test/.

    cd repro && python3 ../sandbox/omo/coord/highbits/make_synth.py

t2_src8.mkv / t2_src10.mkv : test.mp4 frames 296..315 as yuv420p (8 bit) and yuv420p10le (= 8-bit x 4), FFV1
syn_<pix_fmt>.mkv          : 320x256, 8 frames, gradient + moving texture + Gaussian noise, FFV1
                             (yuv420p10le, yuv422p10le, yuv444p12le, yuv444p16le)
syn_gbrpf32le.mkv          : same picture as RGB float with values above 1.0 and below 0, FFV1
syn_exr/in_%05d.exr        : the same float frames as an EXR sequence
"""
import os
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "test")
REPRO = os.path.join(HERE, "..", "..", "..", "..", "repro")
sys.path.insert(0, REPRO)
import nv_depth as nd                                   # noqa: E402

TAGS = ["-colorspace", "bt709", "-color_primaries", "bt709", "-color_trc", "bt709", "-color_range", "tv"]


def encode(path, pix_fmt, W, H, frames, tags=TAGS, codec=("ffv1", "-level", "3"), extra=()):
    cmd = ["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", pix_fmt, "-s", f"{W}x{H}", "-r", "24",
           *tags, "-i", "-", "-c:v", *codec, "-pix_fmt", pix_fmt, *tags, *extra, path]
    p = subprocess.Popen(cmd, stdin=subprocess.PIPE)
    for f in frames:
        p.stdin.write(f)
    p.stdin.close()
    assert p.wait() == 0, cmd


def picture(W, H, k, rng):
    """Base picture on the 0..1 scale: gradient, a moving sine texture and a flat patch."""
    y, x = np.mgrid[0:H, 0:W].astype(np.float64)
    base = 0.15 + 0.6 * x / W + 0.1 * np.sin((x - 3 * k) / 7.0) * np.sin(y / 9.0) * (y > H / 2)
    base[:H // 3, W // 2:] = 0.5
    return base


def main():
    os.makedirs(OUT, exist_ok=True)
    # --- test 2: test.mp4 frames 296..315, 8 bit and x4 at 10 bit
    W, H = 640, 360
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", os.path.join(REPRO, "..", "test.mp4"), "-vf",
                          "select=between(n\\,296\\,315)", "-fps_mode", "passthrough", "-f", "rawvideo",
                          "-pix_fmt", "yuv420p", "-"], capture_output=True, check=True).stdout
    fs = W * H * 3 // 2
    f8 = [raw[i:i + fs] for i in range(0, len(raw), fs)]
    assert len(f8) == 20, len(f8)
    encode(os.path.join(OUT, "t2_src8.mkv"), "yuv420p", W, H, f8)
    encode(os.path.join(OUT, "t2_src10.mkv"), "yuv420p10le", W, H,
           [(np.frombuffer(b, np.uint8).astype("<u2") * 4).tobytes() for b in f8])

    # --- test 3: integer yuv clips with noise
    rng = np.random.default_rng(20261026)
    W, H, N = 320, 256, 8
    for name in ("yuv420p10le", "yuv422p10le", "yuv444p12le", "yuv444p16le"):
        fmt = nd.pixfmt(name)
        top, k = 2 ** fmt.bits - 1, 2 ** (fmt.bits - 8)
        frames = []
        for i in range(N):
            Y = (16 + 219 * picture(W, H, i, rng)) * k + rng.normal(0, 2.5 * k, (H, W))
            ch, cw = fmt.plane_shapes(W, H)[1]
            ys, xs = np.mgrid[0:ch, 0:cw]
            U = (128 + 20 * np.sin(xs / 11.0)) * k + rng.normal(0, 1.5 * k, (ch, cw))
            V = (128 - 15 * np.cos(ys / 13.0)) * k + rng.normal(0, 1.5 * k, (ch, cw))
            frames.append(b"".join(np.clip(np.rint(p), 0, top).astype("<u2").tobytes() for p in (Y, U, V)))
        encode(os.path.join(OUT, f"syn_{name}.mkv"), name, W, H, frames)

    # --- test 4: RGB float with values > 1 and < 0
    frames = []
    for i in range(N):
        b = picture(W, H, i, rng)
        b[H // 3:H // 3 + 24, :48] = 1.3              # highlight above 1.0
        b[-20:, -40:] = -0.03                         # slightly negative patch
        g = b + rng.normal(0, 0.01, (H, W))
        bl = 0.9 * b + rng.normal(0, 0.012, (H, W))
        r = 1.05 * b + rng.normal(0, 0.012, (H, W))
        frames.append(b"".join(np.asarray(p, "<f4").tobytes() for p in (g, bl, r)))   # G, B, R planes
    ftags = ["-color_primaries", "bt709", "-color_trc", "linear"]
    encode(os.path.join(OUT, "syn_gbrpf32le.mkv"), "gbrpf32le", W, H, frames, tags=ftags,
           codec=("ffv1", "-level", "4", "-strict", "experimental"))
    os.makedirs(os.path.join(OUT, "syn_exr"), exist_ok=True)
    encode(os.path.join(OUT, "syn_exr", "in_%05d.exr"), "gbrpf32le", W, H, frames, tags=ftags,
           codec=("exr",), extra=("-f", "image2"))
    np.save(os.path.join(OUT, "syn_gbrpf32le_frames.npy"),
            np.stack([np.frombuffer(f, "<f4") for f in frames]))
    print("ok")


if __name__ == "__main__":
    main()
