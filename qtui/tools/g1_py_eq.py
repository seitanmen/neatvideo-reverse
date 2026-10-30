"""G1 (R/G/B equalizer) comparison driver of the PySide6 UI: the steps of qtui_g1eq (qtui/tests/g1_eq.cpp) on
repro/nv_ui_qt (state.AppState + backend), with the same output files.

    QT_QPA_PLATFORM=offscreen NV_NATIVE=1 python3 qtui/tools/g1_py_eq.py VIDEO OUTDIR F CASE[,CASE...]

Reads the repro code; writes only into OUTDIR.
"""
import json
import os
import sys

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
HERE = os.path.dirname(os.path.abspath(__file__))
REPRO = os.path.normpath(os.path.join(HERE, "..", "..", "repro"))
sys.path.insert(0, REPRO)
sys.dont_write_bytecode = True

import numpy as np  # noqa: E402
from PySide6.QtGui import QGuiApplication  # noqa: E402

G1_BEFORE = os.environ.get("G1_BEFORE") == "1"   # baseline run of the code before G1


def helpers():
    from nv_ui_qt import state as S
    if not G1_BEFORE:
        return S.after_aft, S.reset_fine_tune, S.auto_complete_ps
    import copy

    def after_aft(ps, prof):                # the behaviour before G1 (MainWindow), baseline run only
        n = ps.clone()
        n.base = prof
        n.factors = [[1.0] * 9 for _ in range(3)]
        n.pinned = [set() for _ in range(3)]
        return n

    def reset_fine_tune(ps):
        n = ps.clone()
        n.factors = [[1.0] * 9 for _ in range(3)]
        n.pinned = [set() for _ in range(3)]
        if n.base is not None:
            n.base = copy.deepcopy(n.base)
            n.base["aft_reset"] = True
        return n

    return after_aft, reset_fine_tune, lambda ps: S.auto_complete(ps.factors, ps.pinned)


def with_(fs, k, v):
    o = list(fs)
    o[k] = v
    return o


def apply_case(st, name, h):
    _, reset_fine_tune, auto_complete_ps = h
    f = lambda c: st.profile.factors[c]    # noqa: E731
    if name == "default":
        return
    if name == "r4":
        st.new_edit()
        st.set_factors(0, with_(f(0), 4, 1.2), pin=4)
    elif name == "gall":
        for k in range(9):
            st.new_edit()
            st.set_factors(1, with_(f(1), k, 1.2), pin=k)
    elif name == "all2":
        st.new_edit()
        st.set_factors_all(with_(f(0), 2, 1.2), pin=2)
    elif name == "b6tune":
        st.new_edit()
        st.set_factors(2, with_(f(2), 6, 0.7), pin=6)
        st.new_edit()
        st.set_tune("all", 1.1)
    elif name == "reset":
        st.set_profile(reset_fine_tune(st.profile), "Reset Fine-Tuning Results")
    elif name == "r4ac":
        apply_case(st, "r4", h)
        n = st.profile.clone()
        n.factors = auto_complete_ps(n)
        st.set_profile(n, "Auto Complete")
    else:
        raise SystemExit(f"unknown case {name}")


def main():
    video, out, F, cases = sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4].split(",")
    os.makedirs(out, exist_ok=True)
    app = QGuiApplication([sys.argv[0]])  # noqa: F841
    import denoise_video_v5 as dv
    import nvfilter as nv
    from nv_ui_qt import backend, quality
    from nv_ui_qt.state import AppState, ProfileState
    h = helpers()
    info = dv.probe(video)
    W, H = info["W"], info["H"]
    matrix = nv.matrix_from_tags(info["tags"], H)
    full_range = bool(info["full_range"])
    lo = F - 2
    frames = [tuple(np.array(p) for p in fr) for _, fr in dv.read_frames(video, W, H, lo, F + 3, info["fps"])]
    ci = F - lo
    prof = backend.build_profile(frames, ci, full_range, matrix, "auto")
    a = backend.run_aft(prof, frames[backend.aft_frame(prof, frames)], full_range, matrix)
    for name in cases:
        st = AppState()
        st.set_profile(ProfileState(prof), "Auto Profile")
        st.set_profile(h[0](st.profile, a), "Auto Fine-Tune")
        apply_case(st, name, h)
        eff = st.effective_profile()
        P = st.backend()
        json.dump(st.profile.to_json(), open(os.path.join(out, name + ".nvprof.json"), "w"), indent=1)
        json.dump(eff, open(os.path.join(out, name + ".ui_profile.json"), "w"), indent=1)
        json.dump(P, open(os.path.join(out, name + ".ui_params.json"), "w"), indent=1)
        R = int(P["radius"])
        i0, i1 = max(0, ci - R), min(len(frames), ci + R + 1)
        win = [tuple(np.asarray(p, np.float32) for p in frames[i]) for i in range(i0, i1)]
        res = backend.process_frame(win, ci - i0, eff, P)
        with open(os.path.join(out, name + ".planes.f32"), "wb") as fh:
            for p in res:
                fh.write(np.ascontiguousarray(p, np.float32).tobytes())
        q = quality.profile_numbers(st.profile, frames, full_range, matrix)
        json.dump(None if q[0] is None else list(q), open(os.path.join(out, name + ".quality.json"), "w"))
        print(name, "done", flush=True)


if __name__ == "__main__":
    main()
