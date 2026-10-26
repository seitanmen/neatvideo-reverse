"""OFX mode after the genuine-UI change: Apply writes a frame, Cancel writes nothing, closing writes one.
    cd repro && python3 ../sandbox/omo/coord/genuine_ui/impl/test_ofx.py"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPRO = os.path.abspath(os.path.join(HERE, "..", "..", "..", "..", "..", "repro"))
W, H = 640, 360
FS = W * H + 2 * (W // 2) * (H // 2)


def main():
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", os.path.join(REPRO, "..", "test.mp4"), "-vf",
                          "select=between(n\\,300\\,304)", "-fps_mode", "passthrough", "-f", "rawvideo",
                          "-pix_fmt", "yuv420p", "-"], capture_output=True, check=True).stdout
    ok = True
    for act, want in (("close", 1), ("cancel", 0), ("apply_cancel", 1), ("menu_apply_cancel", 1)):
        env = dict(os.environ, QT_QPA_PLATFORM="offscreen", OFX_ACTION=act,
                   NV_UI_QT_SETTINGS=os.path.join(HERE, "ofx", "ofx.ini"))
        r = subprocess.run([sys.executable, os.path.join(HERE, "ofx_wrap.py"), "--ofx", str(W), str(H), "30"],
                           input=raw, capture_output=True, cwd=REPRO, env=env, timeout=900)
        n = len(r.stdout) / FS
        good = r.returncode == 0 and n == want
        ok &= good
        print(f"[{'PASS' if good else 'FAIL'}] {act}: exit {r.returncode}, {n:g} frames written (expected {want})")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
