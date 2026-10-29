# N3a: the "prof5" group of native/verify_bridge.py - the UI's profile building through nvcore_c_prof5.h
# (nvc_build_profile_v5, nvc_noisiest_candidates + nvc_hh1_sigma, nvc_effective_curve) against
# repro/nv_ui_qt/backend.py with the native core ON (backend.build_profile -> nv_native.build_profile_v5,
# backend.noisiest_frame, state.effective_profile), bit for bit on every leaf (verify_bridge.compare).
#
#   run(tmp, jobs, quick) -> (ok, total)        (verify_bridge.py calls it; also: python3 native/verify_bridge_prof5.py)
#
# Frames: verify_bridge.frames (test_v02 n = 306..320 BT.709, test.mp4 n = 298..308 BT.601, as the UI loads
# them).  backend.build_profile(frames, f, ...) takes frame f, pair = frame f + 1 and more = f + 1, f + 2
# (backend.profile_window); the C side gets the same planes and adds "ui_frame" = f as the backend does.
import os
import sys
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
REPRO = os.path.join(ROOT, "repro")
for p in (REPRO, HERE):
    if p not in sys.path:
        sys.path.insert(0, p)
sys.dont_write_bytecode = True
os.environ["NV_NATIVE"] = "1"        # backend with the native core (verify_prof5 would default it to 0)
F32 = np.float32


def c_build_profile(frames, f, full_range, matrix, mode, area=None, cancel=None):
    """backend.build_profile through the C ABI (the backend's profile_window, then nvc_build_profile_v5)."""
    import verify_prof5 as vp
    f = max(0, min(f, len(frames) - 1))
    planes = tuple(np.asarray(p, F32) for p in frames[f])
    more = [tuple(np.asarray(p, F32) for p in frames[i]) for i in range(f + 1, min(f + 3, len(frames)))]
    prof = vp.c_build(planes, full_range, area, more[0] if more else None, mode, 5, more, matrix, cancel)
    if isinstance(prof, dict):
        prof["ui_frame"] = f
    return prof


def _cases(quick):
    out = []
    for video, mat in (("v02", "bt709"), ("t", "bt601")):
        for f in ((2, 6) if quick else (0, 2, 4, 6, 8, 9, 10)):
            for mode, area in (("auto", None), ("spatial", None), ("temporal", None), ("auto", (40, 60, 216, 512)),
                               ("auto", (100, 120, 64, 64))):
                if quick and mode == "temporal":
                    continue
                out.append((video, mat, f, mode, area))
    return out


def _run_profile(case):
    video, mat, f, mode, area = case
    import verify_bridge as vb
    from nv_ui_qt import backend
    backend.set_native(True)
    frames, _ = vb.frame_list(video)
    t0 = time.perf_counter()
    ref = backend.build_profile(frames, f, False, mat, mode, area)
    t1 = time.perf_counter()
    got = c_build_profile(frames, f, False, mat, mode, area)
    t2 = time.perf_counter()
    return f"profile {video} f={f} {mode} {area}", vb.compare(ref, got), t1 - t0, t2 - t1


def run(tmp, jobs=2, quick=False, verbose=True):
    """The prof5 group: (ok, total)."""
    os.environ["NV_BRIDGE_TMP"] = tmp
    import verify_bridge as vb
    vb.TMP = tmp
    os.makedirs(tmp, exist_ok=True)
    import verify_prof5 as vp
    from nv_ui_qt import backend, state
    backend.set_native(True)
    if not backend.native_enabled():
        raise RuntimeError("native core not available (libnvcore.dylib)")
    rows = []
    for v in ("v02", "t"):
        vb.frames(v)                                  # fill the frame cache before the workers start
    from concurrent.futures import ProcessPoolExecutor
    with ProcessPoolExecutor(max(1, jobs)) as ex:
        rows += list(ex.map(_run_profile, _cases(quick)))
    for video in ("v02", "t"):
        frames, _ = vb.frame_list(video)
        for n in ((len(frames),) if quick else (1, 2, 3, 4, 5, len(frames))):
            fr = frames[:n]
            rows.append((f"noisiest_frame {video} n={n}", vb.compare(backend.noisiest_frame(fr), vp.c_noisiest(fr)),
                         None, None))
        for f in (0, 5):
            rows.append((f"hh1_sigma {video} f={f}", vb.compare(backend.hh1_sigma(frames[f][0]),
                                                                vp.c_hh1(np.asarray(frames[f][0], F32))), None, None))
    # Manual Fine-Tune / profile Noise Level on a backend profile and on its AFT result
    frames, _ = vb.frame_list("v02")
    base = backend.build_profile(frames, 6, False, "bt709", "auto")
    aft = backend.run_aft(base, frames[6], False, "bt709")
    rng = np.random.default_rng(11)
    for t in range(4 if quick else 12):
        factors = [list(map(float, rng.uniform(0.2, 3.0, 9))) for _ in range(3)] if t % 4 else state.unit_factors()
        tune = {k: float(rng.uniform(0.2, 3.0)) for k in ("all", "Y", "Cb", "Cr") if rng.random() < 0.5} if t % 3 else {}
        for name, b in (("auto", base), ("aft", aft)):
            ref = state.effective_profile(b, factors, tune)
            got = vp.c_effective(b, factors, tune)
            cmp = vb.compare(ref, got)
            if (ref is b) != (got is b):
                cmp = [cmp[0] + 1, cmp[1] + 1, cmp[2] + ["identity of base differs"]]
            rows.append((f"effective_profile {name} #{t}", cmp, None, None))
    ok = 0
    for label, (w, d, first), tp, tc in rows:
        ok += d == 0
        if verbose and (d or tp is not None):
            tm = f"  (py {tp:.2f} s, C {tc:.2f} s)" if tp is not None else ""
            print(f"{'ok  ' if d == 0 else 'DIFF'} prof5 {label}: {w} words{tm}" + (f" {first}" if d else ""))
    if verbose:
        print(f"prof5: {ok}/{len(rows)} cases bit-exact")
    return ok, len(rows)


if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--tmp", default=os.path.join(HERE, "tmp_bridge"))
    a = ap.parse_args()
    ok, total = run(a.tmp, a.jobs, a.quick)
    sys.exit(0 if ok == total else 1)
