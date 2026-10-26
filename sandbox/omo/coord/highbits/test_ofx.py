"""OFX bridge round trip at --depth 8 / 16 / 32 (SPEC.md §4.5), off-screen.

    cd repro && python3 ../sandbox/omo/coord/highbits/test_ofx.py

Writes 5 frames of test.mp4 (n=300..304, 640x360) to `python3 -m nv_ui_qt.main --ofx ...` through
ofx_wrap (the window closes itself 0.5 s after it is shown; closing emits the current frame), and
checks the size and value range of the frame on stdout. Settings go to test/ofx_settings.ini.
"""
import os
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
T = os.path.join(HERE, "test")
REPRO = os.path.abspath(os.path.join(HERE, "..", "..", "..", "..", "repro"))
W, H, N = 640, 360, 5


def main():
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", os.path.join(REPRO, "..", "test.mp4"), "-vf",
                          "select=between(n\\,300\\,304)", "-fps_mode", "passthrough", "-f", "rawvideo",
                          "-pix_fmt", "yuv420p", "-"], capture_output=True, check=True).stdout
    a8 = np.frombuffer(raw, np.uint8)
    inputs = {8: a8.tobytes(), 16: (a8.astype("<u2") * 256).tobytes(),
              32: (a8.astype(np.float32) / np.float32(255)).astype("<f4").tobytes()}
    env = dict(os.environ, QT_QPA_PLATFORM="offscreen", NV_UI_QT_SETTINGS=os.path.join(T, "ofx_settings.ini"))
    fs8 = W * H + 2 * (W // 2) * (H // 2)
    outs = {}
    for depth, data in inputs.items():
        r = subprocess.run([sys.executable, os.path.join(HERE, "ofx_wrap.py"), "--ofx", str(W), str(H), "30", "0",
                            "--depth", str(depth)], input=data, capture_output=True, cwd=REPRO, env=env, timeout=600)
        dt, size = {8: (np.uint8, 1), 16: ("<u2", 2), 32: ("<f4", 4)}[depth]
        o = np.frombuffer(r.stdout[:len(r.stdout) // size * size], dt)
        outs[depth] = o
        err = [ln for ln in r.stderr.decode(errors="replace").strip().splitlines()
               if "propagateSizeHints" not in ln and not ln.startswith("qt.qpa.fonts")]
        print(f"depth {depth}: exit {r.returncode}, stdout {len(r.stdout)} bytes (expected 1 frame = {fs8 * size}), "
              f"min {o.min() if o.size else '-'} max {o.max() if o.size else '-'}"
              + (f", stderr: {err[-3:]}" if err else ""))
    if all(outs[d].size == fs8 for d in outs):
        o8 = outs[8].astype(np.float64)
        print(f"|out16/256 - out8| max {np.abs(outs[16] / 256.0 - o8).max():.4f}, "
              f"|out32*255 - out8| max {np.abs(outs[32].astype(np.float64) * 255 - o8).max():.4f}, "
              f"out16 values not multiple of 256: {np.mean(outs[16] % 256 != 0):.4f}, "
              f"out32 values off the 8-bit grid (|x*255 - rint| > 1e-3): "
              f"{np.mean(np.abs(outs[32] * 255.0 - np.rint(outs[32] * 255.0)) > 1e-3):.4f}")


def nonfinite():
    """--depth 32 with NaN / +-inf in the host frames: written back unchanged, the rest finite."""
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", os.path.join(REPRO, "..", "test.mp4"), "-vf",
                          "select=between(n\\,300\\,304)", "-fps_mode", "passthrough", "-f", "rawvideo",
                          "-pix_fmt", "yuv420p", "-"], capture_output=True, check=True).stdout
    a = (np.frombuffer(raw, np.uint8).astype(np.float32) / np.float32(255)).astype("<f4").reshape(N, -1)
    fs8 = W * H + 2 * (W // 2) * (H // 2)
    for k in range(N):
        a[k, 1000 + k] = np.nan
        a[k, 5000] = np.inf
        a[k, W * H + 300] = -np.inf          # a Cb sample
        a[k, 20000:20010] = np.nan
    env = dict(os.environ, QT_QPA_PLATFORM="offscreen", NV_UI_QT_SETTINGS=os.path.join(T, "ofx_settings.ini"))
    r = subprocess.run([sys.executable, os.path.join(HERE, "ofx_wrap.py"), "--ofx", str(W), str(H), "30", "0",
                        "--depth", "32"], input=a.tobytes(), capture_output=True, cwd=REPRO, env=env, timeout=600)
    o = np.frombuffer(r.stdout, "<f4")
    t = min(N // 3, max(N - 3, 0))                # the frame the UI starts on and emits
    bad = ~np.isfinite(a[t])
    ok = o.size == fs8 and np.array_equal(o[bad].view("<u4"), a[t][bad].view("<u4")) and np.isfinite(o[~bad]).all()
    print(f"depth 32 with {int(bad.sum())} non-finite samples in frame {t}: exit {r.returncode}, "
          f"{o.size} samples, non-finite restored and the rest finite: {bool(ok)}")


if __name__ == "__main__":
    main()
    nonfinite()
