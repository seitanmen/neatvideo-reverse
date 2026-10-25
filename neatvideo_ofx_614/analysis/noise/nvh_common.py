"""Shared access to the genuine-bundle harness (../harness/nvh) for the verification scripts.

Every script calls require_guard() first: `nvh selftest` must end with exit code 3 (the
genuine assertion handler was intercepted), otherwise nothing of the bundle is run."""
import os
import subprocess
import tempfile

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
NVH = os.path.join(HERE, "..", "harness", "nvh")
_OK = False


def require_guard():
    global _OK
    if not _OK:
        r = subprocess.run([NVH, "selftest"], capture_output=True, text=True, timeout=120)
        if r.returncode != 3 or "genuine alert suppressed" not in r.stderr:
            raise SystemExit(f"nvh selftest failed (exit {r.returncode}): rebuild with `make -C ../harness nvh`"
                             f" - refusing to run genuine code\n{r.stderr[-400:]}")
        _OK = True


def run(args, arrays, timeout=600):
    """Write `arrays` (float32, concatenated) to a temp file, run `nvh args[0] W H file args[1:]`."""
    require_guard()
    with tempfile.NamedTemporaryFile(suffix=".f32", delete=False) as f:
        for a in arrays:
            np.ascontiguousarray(a, np.float32).tofile(f)
        name = f.name
    try:
        r = subprocess.run([NVH, args[0], *map(str, args[1:3]), name, *map(str, args[3:])],
                           capture_output=True, text=True, timeout=timeout)
    finally:
        os.unlink(name)
    if r.returncode:
        raise RuntimeError(f"nvh {args[0]} exit {r.returncode}: {r.stderr[-400:]}")
    return r.stdout


def genuine_flat(rgb, mode=1, sel=0):
    """rgb (H, W, 3) float 0..255 -> genuine FindFlatArea (x, y, w, h, status)."""
    H, W, _ = rgb.shape
    t = run(["flat", W, H, mode, sel], [np.moveaxis(rgb, -1, 0)]).split()
    kv = dict(s.split("=") for s in t if "=" in s)
    return int(kv["x"]), int(kv["y"]), int(kv["w"]), int(kv["h"]), int(t[1])


def genuine_struct(mode, plane):
    """`nvh levels|model` on one plane -> the dumped float struct."""
    out = run([mode, plane.shape[1], plane.shape[0]], [plane])
    return np.array([float(l.split()[1]) for l in out.splitlines() if l.strip()])
