"""NaN / +-inf in float input (verification round 2, finding 3).

    cd repro && python3 ../sandbox/omo/coord/highbits/test_nonfinite.py

Inputs: the verifier's verify/edge/nan.mkv (gbrpf32le, NaN and +inf) and test/r2/nf_src.mkv (made
here: NaN, +inf and -inf in single pixels, a 6x6 NaN block and an all-NaN row, 320x256, 8 frames).
For each: CLI --depth 32 normal and --mix 1; checks that every non-finite input sample comes back
unchanged (same bits), every other output sample is finite, and Mix 1 equals the input elsewhere
within the float round trip. Then the UI preview path (read_frames -> PreviewEngine.process) on the
same frames: the displayed RGB must be finite everywhere.
"""
import os
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
T = os.path.join(HERE, "test", "r2")
REPRO = os.path.abspath(os.path.join(HERE, "..", "..", "..", "..", "repro"))
sys.path.insert(0, REPRO)


def make(path, W=320, H=256, N=8):
    rng = np.random.default_rng(7)
    y, x = np.mgrid[0:H, 0:W]
    frames = []
    for i in range(N):
        b = 0.2 + 0.6 * x / W + 0.05 * np.sin((x - 2 * i) / 6.0)
        planes = [b + rng.normal(0, 0.01, (H, W)) for _ in range(3)]
        g = planes[0]
        g[10 + i, 20] = np.nan
        g[100, 50 + i] = np.inf
        planes[1][200, 300 - i] = -np.inf
        planes[2][40:46, 60:66] = np.nan
        if i == 3:
            planes[2][128, :] = np.nan
        frames.append(b"".join(np.asarray(p, "<f4").tobytes() for p in planes))
    p = subprocess.Popen(["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "gbrpf32le", "-s", f"{W}x{H}",
                          "-r", "24", "-i", "-", "-c:v", "ffv1", "-level", "4", "-strict", "experimental",
                          "-pix_fmt", "gbrpf32le", path], stdin=subprocess.PIPE)
    for f in frames:
        p.stdin.write(f)
    p.stdin.close()
    assert p.wait() == 0


def dec(path):
    return np.frombuffer(subprocess.run(["ffmpeg", "-v", "error", "-i", path, "-f", "rawvideo", "-pix_fmt",
                                         "gbrpf32le", "-"], capture_output=True, check=True).stdout, "<f4")


def main():
    os.makedirs(T, exist_ok=True)
    mine = os.path.join(T, "nf_src.mkv")
    make(mine)
    import denoise_video_v5 as dv
    import nv_depth as nd
    import nvfilter as nv
    from nv_ui_qt import backend
    for src in (os.path.join(HERE, "verify", "edge", "nan.mkv"), mine):
        a = dec(src)
        bad = ~np.isfinite(a)
        print(f"{os.path.relpath(src, HERE)}: {a.size} samples, NaN {int(np.isnan(a).sum())}, "
              f"+inf {int(np.isposinf(a).sum())}, -inf {int(np.isneginf(a).sum())}")
        for tag, extra in (("normal", []), ("mix1", ["--mix", "1"])):
            dst = os.path.join(T, f"nf_{os.path.basename(src)[:-4]}_{tag}.mkv")
            r = subprocess.run([sys.executable, "denoise_video_v5.py", src, dst, "--depth", "32", "--jobs", "3", *extra],
                               cwd=REPRO, capture_output=True, text=True)
            o = dec(dst)
            n = min(o.size, a.size)
            same_bad = np.array_equal(o[:n][bad[:n]].view("<u4"), a[:n][bad[:n]].view("<u4"))
            fin_else = bool(np.isfinite(o[:n][~bad[:n]]).all())
            line = (f"  {tag}: exit {r.returncode}, samples {o.size}, non-finite input samples restored bit for bit "
                    f"{same_bad}, all other samples finite {fin_else} (non-finite share of output "
                    f"{np.mean(~np.isfinite(o)):.6f} = input {np.mean(bad):.6f})")
            if tag == "mix1":
                line += f", max |out-in| elsewhere {np.abs(o[:n][~bad[:n]].astype(np.float64) - a[:n][~bad[:n]]).max():.3g}"
            print(line)
        # UI preview path: frames as the UI holds them (read_frames, float32 0..255), profile, preview
        info = dv.probe(src)
        fmt = nd.pixfmt("gbrpf32le")
        W, H = info["W"], info["H"]
        frames = [tuple(np.array(p) for p in f) for _, f in dv.read_frames(src, W, H, 0, None, info["fps"], fmt)]
        held = all(np.isfinite(p).all() for f in frames for p in f)
        prof = backend.build_profile(frames, 2, True, "bt709", "auto")
        eng = backend.PreviewEngine(frames, "bt709", True)
        filt, orig = eng.process(3, prof, 0, backend.nv.v5_params() if hasattr(backend, "nv") else nv.v5_params())
        print(f"  UI: frames held finite {held}, profile sigma finite {all(np.isfinite(v) for v in prof['sigma'].values())}, "
              f"preview filtered finite {bool(np.isfinite(filt).all())}, original view finite {bool(np.isfinite(orig).all())}")


if __name__ == "__main__":
    main()
