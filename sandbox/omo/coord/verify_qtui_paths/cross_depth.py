"""Cross-checks of the test4 outputs: processed != input, and depth 8 / 16 / 32 agree."""
import sys
import subprocess
import numpy as np
if __name__ == "__main__":
    d = sys.argv[1]
    W, H = 192, 108
    n8 = W * H * 3 // 2
    inp = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-i", "/Users/seitan/Desktop/work/reverse/test.mp4",
                          "-vf", f"scale={W}:{H}", "-pix_fmt", "yuv420p", "-frames:v", "1", "-f", "rawvideo", "-"],
                         capture_output=True).stdout
    i8 = np.frombuffer(inp, np.uint8).astype(np.float64)
    o8 = np.frombuffer(open(f"{d}/apply_close_192_108_8_12/stdout.bin", "rb").read(), np.uint8).astype(np.float64)
    o16 = np.frombuffer(open(f"{d}/apply_close_192_108_16_12/stdout.bin", "rb").read(), "<u2").astype(np.float64)
    o32 = np.frombuffer(open(f"{d}/apply_close_192_108_32_12/stdout.bin", "rb").read(), "<f4").astype(np.float64)
    print(f"depth 8 frame 0: RMS(out - in) Y {np.sqrt(((o8 - i8)[:W*H] ** 2).mean()):.4f}, "
          f"CbCr {np.sqrt(((o8 - i8)[W*H:] ** 2).mean()):.4f}  (code values)")
    p = {k: [np.load(f"{d}/apply_close_192_108_{k}_12/emit_0.npz")[f"arr_{i}"].astype(np.float64) for i in range(3)]
         for k in (8, 16, 32)}
    for k in (16, 32):
        m = max(np.abs(a - b).max() for a, b in zip(p[8], p[k]))
        print(f"emitted planes (0..255 scale) depth {k} vs 8: max |diff| {m:.3g}")
    print(f"stdout depth16/256 vs depth8: max |diff| {np.abs(o16 / 256 - o8).max():.4f}")
    print(f"stdout depth32*255 vs depth8: max |diff| {np.abs(o32 * 255 - o8).max():.4f}")
