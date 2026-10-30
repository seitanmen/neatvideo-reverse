"""Compare the OFX frame processing of the C++ UI (qtui_emit_sizes = nvqt::ofx_frame + ofx_process) with the Python
UI's (nv_ui_qt.main._emit: ofx_bridge.unpack_yuv420p, backend.build_profile(.., "auto"), PreviewEngine.window,
backend.process_frame, ofx_bridge.pack_yuv420p) over frame sizes, mostly the small ones where the Python UI raises
(N3c verification I-6: np.gradient below 2 px, the motion search below 16 px) or runs nvfilter.temporal_v5 itself.

    NV_NATIVE=1 python3 qtui/tools/compare_emit_sizes.py OUTDIR [--exe qtui/build_main/qtui_emit_sizes] [--quick]

Configurations: 5 frames with the default parameters (frame 1, the OFX window's first frame), one frame (Temporal
Amount 100% and 37%), 5 frames with the Temporal filter off, 5 frames with the pyramid similarity; depth 8 everywhere, 16 and 32 on a subset.
Compared per job: the error message (the "OFX emit error" text) or the output bytes.
"""
import argparse
import json
import os
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "repro"))

from nv_ui_qt import backend, state as st                       # noqa: E402
from nv_ui_qt.ofx_bridge import frame_size, pack_yuv420p, unpack_yuv420p   # noqa: E402

CONFIGS = {"default": (5, {}), "one_frame": (1, {}), "one_frame_a37": (1, {"t_amount": 37.0}),
           "t_off": (5, {"t_enable": False}), "pyramid": (5, {"adv_similarity": "pyramid"})}


def py_emit(frames, t, P, depth, restore):
    """main._emit's processing (no UI profile): the bytes, or the message of the exception."""
    try:
        prof = backend.build_profile(frames, t, False, "bt709", "auto")
        eng = backend.PreviewEngine(frames, "bt709", False)
        idx, planes = eng.window(t, int(P["radius"]), None)
        out = backend.process_frame(planes, idx.index(t), prof, P)
        return pack_yuv420p(out, depth, restore[t] if t < len(restore) else None)
    except Exception as e:      # noqa: BLE001 - _emit catches everything
        return f"ERR {e}"


def sizes(quick):
    small = [1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17] if quick else list(range(1, 21)) + [31, 32, 33, 47, 48, 63, 64, 65]
    res = [(W, H) for W in small for H in small]
    res += [(15, 300), (300, 15), (2, 640), (640, 3), (1, 640), (640, 1), (17, 17), (33, 65), (100, 70), (321, 181)]
    return res


def frames_for(W, H, n, depth, seed):
    rng = np.random.default_rng(seed)
    fs8 = frame_size(W, H)
    a8 = rng.integers(16, 236, n * fs8).astype(np.uint8)
    if depth == 8:
        raw = a8.tobytes()
    elif depth == 16:
        raw = (a8.astype("<u2") * 256 + rng.integers(0, 256, a8.size).astype("<u2")).astype("<u2").tobytes()
    else:
        f = (a8.astype(np.float32) + rng.random(a8.size, dtype=np.float32) - np.float32(0.5)) / np.float32(255)
        f[::97] = -0.0                   # signed zeros and non-finite samples (sanitize / restore)
        f[5::131] = np.nan
        f[7::173] = np.inf
        raw = f.astype("<f4").tobytes()
    fs = frame_size(W, H, depth)
    restore, frames = [], []
    for k in range(n):
        frames.append(unpack_yuv420p(raw[k * fs:(k + 1) * fs], W, H, depth, restore))
    while len(restore) < n:
        restore.append(None)
    return raw, frames, restore


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--exe", default=os.path.join(ROOT, "qtui", "build_main", "qtui_emit_sizes"))
    ap.add_argument("--quick", action="store_true")
    A = ap.parse_args()
    os.makedirs(A.out, exist_ok=True)
    jobs, expect = [], []
    for cname, (n, ui_over) in CONFIGS.items():
        ui = st.default_params()
        ui.update(ui_over)
        P = st.backend_params(ui)
        ppath = os.path.join(A.out, f"P_{cname}.json")
        with open(ppath, "w") as fh:
            json.dump(P, fh)
        for W, H in sizes(A.quick):
            for depth in (8, 16, 32) if (W <= 9 and H <= 9) or (W, H) in ((15, 300), (17, 17)) else (8,):
                tag = f"{cname}_{W}x{H}_d{depth}"
                raw, frames, restore = frames_for(W, H, n, depth, W * 7919 + H * 104729 + depth)
                fpath = os.path.join(A.out, tag + ".in")
                with open(fpath, "wb") as fh:
                    fh.write(raw)
                t = min(n // 3, max(n - 3, 0))
                jobs.append(f"{W} {H} {depth} {n} {t} {fpath} {ppath} {os.path.join(A.out, tag + '.cpp')}")
                expect.append((tag, py_emit(frames, t, P, depth, restore)))
    with open(os.path.join(A.out, "jobs.txt"), "w") as fh:
        fh.write("\n".join(jobs) + "\n")
    r = subprocess.run([A.exe, os.path.join(A.out, "jobs.txt")], capture_output=True, text=True, timeout=3600)
    got = r.stdout.splitlines()
    if r.returncode != 0 or len(got) != len(expect):
        print(f"qtui_emit_sizes: exit {r.returncode}, {len(got)} of {len(expect)} results\n{r.stderr[-2000:]}")
        return 1
    bad, kinds = 0, {}
    for (tag, py), line in zip(expect, got):
        if line.startswith("OK"):
            cp = open(os.path.join(A.out, tag + ".cpp"), "rb").read()
        else:
            cp = line
        same = py == cp
        kind = "bytes" if isinstance(py, bytes) else py[4:60]
        k = kinds.setdefault(kind, [0, 0])
        k[0] += same
        k[1] += 1
        if not same:
            bad += 1
            if bad <= 30:
                pv = py if isinstance(py, str) else f"{len(py)} bytes"
                cv = cp if isinstance(cp, str) else f"{len(cp)} bytes"
                print(f"DIFF {tag}: py {pv!r:.120} / cpp {cv!r:.120}")
    for kind, (a, b) in sorted(kinds.items()):
        print(f"  {a}/{b} same: {kind}")
    print(f"{len(expect)} jobs, {bad} differ")
    print("RESULT:", "ALL SAME" if not bad else "DIFFERENCES")
    for f in os.listdir(A.out):          # the inputs and outputs are regenerated each run
        if f.endswith((".in", ".cpp")):
            os.remove(os.path.join(A.out, f))
    return 0 if not bad else 1


if __name__ == "__main__":
    sys.exit(main())
