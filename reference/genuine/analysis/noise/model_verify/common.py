"""Shared helpers of the FUN_00995250 / BuildNoiseModel verification scripts (merged from work/dirfilt on
2026-10-16).  The genuine bundle is only reached through the guarded harness (../../harness/nvh);
nvh_common.require_guard() checks that `nvh selftest` exits with 3 before anything of the bundle is run.
Ports are imported from repro/; temporary files and the genuine-result cache go to ./tmp, cached frames to ./data."""
import os
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = HERE
NOISE = os.path.dirname(HERE)
WS = os.path.abspath(os.path.join(NOISE, "..", "..", ".."))
SRC = os.path.join(WS, "reference", "python")
TMP = os.path.join(HERE, "tmp")
DATA = os.path.join(HERE, "data")
for p in (SRC, NOISE):
    if p not in sys.path:
        sys.path.insert(0, p)

import numpy as np  # noqa: E402

import nvh_common  # noqa: E402

NVH = os.path.join(WS, "reference/genuine", "analysis", "harness", "nvh")
nvh_common.NVH = NVH
require_guard = nvh_common.require_guard
F32 = np.float32
STRUCT_WORDS = 4096


class Rejected(Exception):
    pass


def _tmpfile(suffix):
    os.makedirs(TMP, exist_ok=True)
    f = tempfile.NamedTemporaryFile(suffix=suffix, dir=TMP, delete=False)
    return f


def genuine_model(plane, set_=None, timeout=1800):
    """`nvh model W H plane [set]` -> uint32[4096] from the bit-pattern column."""
    require_guard()
    plane = np.ascontiguousarray(plane, F32)
    H, W = plane.shape
    if W > 512 or H > 512 or W < 2 or H < 2:
        raise ValueError(f"refusing to pass a {W}x{H} plane to nvh model")
    f = _tmpfile(".f32")
    plane.tofile(f)
    f.close()
    args = [NVH, "model", str(W), str(H), f.name] + ([] if set_ is None else [str(set_)])
    try:
        r = subprocess.run(args, capture_output=True, text=True, timeout=timeout)
    finally:
        os.unlink(f.name)
    if r.returncode == 3:
        raise Rejected(r.stderr.strip().splitlines()[-1][:160] if r.stderr.strip() else "exit 3")
    if r.returncode:
        raise RuntimeError(f"nvh model exit {r.returncode}: {r.stderr[-300:]}")
    w = np.array([int(l.split()[2], 16) for l in r.stdout.splitlines() if l.strip()], np.uint32)
    if w.size != STRUCT_WORDS:
        raise RuntimeError(f"nvh model: {w.size} words")
    return w


def genuine_profile(rgb, mode, area, timeout=3600):
    """`nvh profile W H rgb mode out x y w h` (area given: FindFlatArea is not called) -> (bytes, line)."""
    require_guard()
    rgb = np.ascontiguousarray(rgb, F32)
    H, W, _ = rgb.shape
    f = _tmpfile(".f32")
    np.moveaxis(rgb, -1, 0).astype(F32).tofile(f)
    f.close()
    out = f.name + ".prof"
    x, y, w, h = area
    try:
        r = subprocess.run([NVH, "profile", str(W), str(H), f.name, str(mode), out, str(x), str(y), str(w), str(h)],
                           capture_output=True, text=True, timeout=timeout)
        if r.returncode == 3:
            raise Rejected(r.stderr.strip().splitlines()[-1][:160] if r.stderr.strip() else "exit 3")
        if r.returncode:
            raise RuntimeError(f"nvh profile exit {r.returncode}: {r.stderr[-300:]}")
        data = open(out, "rb").read()
    finally:
        os.unlink(f.name)
        if os.path.exists(out):
            os.unlink(out)
    return data, r.stdout.strip()


def read_frame(path, n, pix="rgb24"):
    """Frame n of a clip (r_frame_rate grid from the first frame) as uint8 (H, W, 3) RGB."""
    probe = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries", "stream=width,height",
                            "-of", "csv=p=0", path], capture_output=True, text=True, check=True).stdout.strip().split(",")
    W, H = int(probe[0]), int(probe[1])
    r = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-i", path, "-vf", f"select=eq(n\\,{n})", "-frames:v", "1",
                        "-f", "rawvideo", "-pix_fmt", pix, "-"], capture_output=True, check=True)
    return np.frombuffer(r.stdout, np.uint8)[:W * H * 3].reshape(H, W, 3)


def frame_cached(name, n):
    """Cached RGB frame (data/<clip>_<n>.npy, uint8)."""
    os.makedirs(DATA, exist_ok=True)
    p = os.path.join(DATA, f"{os.path.splitext(name)[0]}_{n}.npy")
    if os.path.exists(p):
        return np.load(p)
    a = read_frame(os.path.join(WS, name), n)
    np.save(p, a)
    return a
