"""Off-screen self-test of the Qt UI (no window is shown).

    cd repro && QT_QPA_PLATFORM=offscreen python3 -m nv_ui_qt.selftest VIDEO OUTDIR

Writes only into OUTDIR (settings, presets, profiles, a short lossless export). Checks: load +
preview, Auto Profile (current / most noisy frame), Build Profile, slider -> preview update,
per-channel band settings, undo/redo, AFT, Manual Fine-Tune + Auto Complete, preset / profile
save + load, batch profiler, and that the export equals nv.process_frame_v5 bit for bit (CRF 0).
"""
import json
import os
import subprocess
import sys
import time

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

import numpy as np
from PySide6.QtWidgets import QApplication

RESULTS = []


def check(name, ok, detail=""):
    RESULTS.append((name, bool(ok)))
    print(f"[{'PASS' if ok else 'FAIL'}] {name} {detail}", flush=True)


def wait(app, cond, timeout=600.0):
    t0 = time.time()
    while not cond():
        app.processEvents()
        time.sleep(0.01)
        if time.time() - t0 > timeout:
            return False
    app.processEvents()
    return True


def main(video, outdir):
    os.makedirs(outdir, exist_ok=True)
    os.environ["NV_UI_QT_SETTINGS"] = os.path.join(outdir, "settings.ini")
    if os.path.exists(os.environ["NV_UI_QT_SETTINGS"]):
        os.remove(os.environ["NV_UI_QT_SETTINGS"])
    app = QApplication([sys.argv[0]])
    import nvfilter as nv
    from denoise_video_v5 import probe, read_frames
    from nv_ui_qt import backend
    from nv_ui_qt.main_window import MainWindow, settings
    from nv_ui_qt.state import PARAM_SPECS, default_params

    s = settings()
    s.setValue("prefs/crf", 0)
    s.setValue("prefs/jobs", 3)
    s.sync()
    win = MainWindow(video)
    win.show()
    st = win.state
    idle = lambda: not win.runner.running and not win.runner.queued
    check("load + auto profile", wait(app, lambda: st.profile.base is not None and idle()),
          f"{len(win.frames or [])} frames, area {st.profile.base and st.profile.base['area']}")
    check("preview filtered", wait(app, lambda: idle() and win.filt_cache and win.preview.filt is not None))

    # defaults = V5_DEFAULTS
    P = st.backend()
    D = nv.V5_DEFAULTS
    same = all(P[k] == D[k] for k in D)
    check("UI defaults map to nv.V5_DEFAULTS", same)

    # bit-exact vs process_frame_v5 (default, uniform channels)
    t = win.preview.frame()
    prof = st.effective_profile()
    R = int(P["radius"])
    idx, win_planes = win.engine.window(t, R, None)
    ci = idx.index(t)
    ref = nv.process_frame_v5(win_planes, ci, prof, nv.v5_params())
    got = backend.process_frame(win_planes, ci, prof, P)
    check("backend.process_frame == nv.process_frame_v5 (defaults)",
          all(np.array_equal(a, b) for a, b in zip(ref, got)))

    # per-channel band setting: only Cb changes
    ui = default_params()
    ui["bamt_Cb_medium"] = 50.0
    from nv_ui_qt.state import backend_params
    got2 = backend.process_frame(win_planes, ci, prof, backend_params(ui))
    check("per-channel band: Y and Cr unchanged, Cb changed",
          np.array_equal(got2[0], ref[0]) and np.array_equal(got2[2], ref[2]) and not np.array_equal(got2[1], ref[1]))

    sub420 = win.frames[0][1].shape != win.frames[0][0].shape
    fpc = win.filter_page.panel.controls
    check("chroma 'High' band sliders follow the profile's band count",
          (not fpc["bamt_Cb_high"].isEnabled() and not fpc["bnoise_Cr_high"].isEnabled() and fpc["bamt_Y_high"].isEnabled())
          if sub420 and prof["levels"]["Cb"] < 5 else fpc["bamt_Cb_high"].isEnabled())

    # slider -> state -> preview update
    gen0 = win.gen
    w = win.filter_page.panel.controls["amt_Y"]
    w.slider.setValue(40)
    check("slider edits state", abs(st.params["amt_Y"] - 40.0) < 1e-9)
    check("preview recomputed after slider", wait(app, lambda: idle() and win.gen > gen0 and
                                                  any(k[1] == win.gen for k in win.filt_cache)))
    # every implemented control reaches the backend dict
    base_P = backend_params(default_params())
    missing = []
    for k, sp in PARAM_SPECS.items():
        if not sp.implemented:
            continue
        ui = default_params()
        if sp.kind == "bool":
            ui[k] = not ui[k]
        elif sp.kind == "choice":
            ui[k] = [c for c in sp.choices if c != ui[k]][0]
        elif sp.kind == "int":
            ui[k] = int(sp.lo) if ui[k] != sp.lo else int(sp.hi)
        else:
            ui[k] = sp.lo if ui[k] != sp.lo else sp.hi
        if json.dumps(backend_params(ui), sort_keys=True, default=str) == json.dumps(base_P, sort_keys=True, default=str):
            missing.append(k)
    check("every implemented control changes the backend parameters", not missing, str(missing))
    disabled = [k for k, sp in PARAM_SPECS.items() if not sp.implemented]
    check("unimplemented controls are disabled",
          all(not win.filter_page.panel.controls[k].isEnabled() for k in disabled if k in win.filter_page.panel.controls),
          f"{len(disabled)} controls")

    # undo / redo
    st.undo.undo()
    check("undo restores value and widget", st.params["amt_Y"] == 70.0 and abs(w.value() - 70.0) < 1e-9)
    st.undo.redo()
    check("redo", st.params["amt_Y"] == 40.0 and abs(w.value() - 40.0) < 1e-9)
    st.undo.undo()

    # presets
    st.set_param("noise_Cr", 150.0)
    pp = os.path.join(outdir, "p.nvpreset.json")
    win.save_preset(pp)
    win.reset_filter()
    check("reset filter", st.params["noise_Cr"] == 100.0)
    win.load_preset(pp)
    check("preset save/load", st.params["noise_Cr"] == 150.0 and
          abs(win.filter_page.panel.controls["noise_Cr"].value() - 150.0) < 1e-9)
    win.assign_default_preset()
    win.reset_filter()
    check("reset uses assigned default preset", st.params["noise_Cr"] == 150.0)
    settings().remove("default_preset")
    win.reset_filter()

    # profile: AFT
    before = json.dumps(st.profile.base["curve"])
    win.auto_fine_tune()
    check("Auto Fine-Tune changes the curve", wait(app, lambda: idle() and st.profile.base.get("aft_applied"))
          and json.dumps(st.profile.base["curve"]) != before)
    st.undo.undo()
    check("undo AFT", json.dumps(st.profile.base["curve"]) == before)
    st.undo.redo()
    once = json.dumps(st.profile.base["curve"])
    win.auto_fine_tune()
    check("Auto Fine-Tune twice == once (no squared factors)",
          wait(app, lambda: idle()) and json.dumps(st.profile.base["curve"]) == once)
    # profile replaced while AFT runs -> stale AFT result discarded
    saved = st.profile.clone()
    win.auto_fine_tune()
    wait(app, lambda: win.runner.busy("aft"), 10)
    win.reset_profile()
    wait(app, lambda: idle())
    check("stale AFT result discarded", st.profile.base is None)
    st.undo.undo()
    check("AFT buttons re-enabled", win.profile_page.panel.b_aft.isEnabled())
    # cancel: a running AFT stops promptly
    win.auto_fine_tune()
    wait(app, lambda: win.runner.busy("aft"), 10)
    t0 = time.time()
    win.runner.wait_all(30000)
    app.processEvents()
    check("AFT cancellable (child process terminated)", time.time() - t0 < 5 and not win.runner.busy("aft"),
          f"{time.time() - t0:.1f}s")
    wait(app, lambda: idle())
    # malformed files are rejected without changing the profile
    bad = os.path.join(outdir, "bad.json")
    good = json.dumps(st.profile.to_json())
    for content in ("[1, 2]", "3", json.dumps({"format": "nv_repro_profile", "base": {"x": 1}})):
        open(bad, "w").write(content)
        win.load_profile(bad)
        win.load_preset(bad)
    check("malformed profile / preset files rejected", json.dumps(st.profile.to_json()) == good)

    # manual fine-tune + auto complete
    ed = win.profile_page.panel.curve
    st.new_edit()
    ed.edited.emit(0, [1.0, 1.0, 1.5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0], 2)
    st.new_edit()
    ed.edited.emit(0, [1.0, 1.0, 1.5, 1.0, 1.0, 1.0, 0.5, 1.0, 1.0], 6)
    win.auto_complete()
    f = st.profile.factors[0]
    check("manual fine-tune + auto complete", abs(f[2] - 1.5) < 1e-9 and abs(f[6] - 0.5) < 1e-9 and
          abs(f[4] - 1.0) < 1e-9 and abs(f[0] - 1.5) < 1e-9 and abs(f[8] - 0.5) < 1e-9, str(f))
    eff = st.effective_profile()
    check("fine-tune changes the effective curve", eff["curve"]["Y"] != st.profile.base["curve"]["Y"])
    pf = os.path.join(outdir, "prof.nvprof.json")
    win.save_profile(pf)
    win.reset_profile()
    check("reset profile", st.profile.base is None)
    win.load_profile(pf)
    check("profile save/load", st.profile.factors[0] == f and st.effective_profile()["curve"] == eff["curve"])

    # build profile from a dragged area / most noisy frame
    win.on_area(8, 8, 64)
    win.build_profile(False)
    check("Build Profile (Current Frame)", wait(app, lambda: idle() and st.profile.base["area"][:3] == [8, 8, 64]))
    win.auto_profile(True)
    nf = backend.noisiest_frame(win.frames)
    check("Auto Profile (Most Noisy Frame)", wait(app, lambda: idle() and st.profile.base["ui_frame"] == nf
                                                  and st.profile.base.get("flat_status") is not None), f"frame {nf}")

    # views
    for m in ("Original", "Split", "Assist", "Filtered"):
        win.preview.set_mode(m)
    png = os.path.join(outdir, "frame.png")
    win.save_frame(png)
    check("preview modes + save frame", os.path.getsize(png) > 0)
    win.preview.area_cb.setChecked(True)
    check("Preview Area (crop) filtering", wait(app, lambda: idle() and any(k[2] is not None for k in win.filt_cache)))
    win.preview.area_cb.setChecked(False)
    win.profile_check().close()
    win.guide("filter").close()

    # batch profiler
    win.batch_profiler([video], outdir, "auto")
    check("batch profiler", wait(app, lambda: idle() and getattr(win, "batch_result", None)))

    # export: lossless, compared with process_frame_v5 using the UI profile
    st.set_param("bamt_Cr_medium", 60.0)        # non-uniform channels exercise backend._work
    dst = os.path.join(outdir, "export.mp4")
    lo, n = 300, 4
    win.process_video(dst, start=lo, frames=n)
    ok = wait(app, lambda: win.export is None, 1800)
    check("export finished", ok and getattr(win, "export_ok", False))
    info = probe(dst)
    got = [f for _, f in read_frames(dst, info["W"], info["H"], fps=info["fps"])]
    sinfo = probe(video)
    P = st.backend()
    prof = json.load(open(os.path.splitext(dst)[0] + ".ui_profile.json"))
    R = int(P["radius"])
    src = {i: tuple(p.astype(np.float32) for p in f)
           for i, f in read_frames(video, sinfo["W"], sinfo["H"], lo - R, lo + n + R, sinfo["fps"])}
    exact = len(got) == n
    for k in range(min(n, len(got))):
        c = lo + k
        ids = [i for i in sorted(src) if abs(i - c) <= R]
        out = backend.process_frame([src[i] for i in ids], ids.index(c), prof, P)
        exp = [np.clip(np.rint(o), 0, 255).astype(np.uint8) for o in out]
        exact &= all(np.array_equal(a, b) for a, b in zip(exp, got[k]))
    check("export (CRF 0) == backend.process_frame bit for bit", exact, f"{len(got)} frames")

    win.close()
    app.processEvents()
    n_fail = sum(1 for _, ok in RESULTS if not ok)
    print(f"{len(RESULTS) - n_fail}/{len(RESULTS)} passed")
    return 1 if n_fail else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2]))
