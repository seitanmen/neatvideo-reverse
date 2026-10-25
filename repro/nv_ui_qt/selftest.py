"""Off-screen self-test of the Qt UI (no window is shown).

    cd repro && QT_QPA_PLATFORM=offscreen python3 -m nv_ui_qt.selftest VIDEO OUTDIR

Writes only into OUTDIR (settings, presets, profiles, a short lossless export). Checks: load +
preview, Auto Profile (current / most noisy frame), Build Profile, slider -> preview update,
per-channel band settings, undo/redo, AFT, Manual Fine-Tune + Auto Complete, preset / profile
save + load, batch profiler, and that the export equals nv.process_frame_v5 bit for bit (CRF 0).
Layout checks: panel order (Temporal / Spatial / General, Luminance -> Cr -> Cb, High -> Ultra Low),
common Frequencies rows, Mix with Original Enable, profile Noise Level, hold-for-original, viewer
panes and splits, thumbnails, Beginner / Advanced Mode, Auto / Build Profile button.
"""
import json
import os
import subprocess
import sys
import time

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

import numpy as np
from PySide6.QtCore import Qt
from PySide6.QtTest import QTest
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
    win.tabs.setCurrentIndex(1)                 # Adjust and Preview: the filtered preview is computed
    check("preview filtered", wait(app, lambda: idle() and win.filt_cache and win.viewer.filt is not None))

    # defaults = V5_DEFAULTS
    P = st.backend()
    D = nv.V5_DEFAULTS
    same = all(P[k] == D[k] for k in D)
    check("UI defaults map to nv.V5_DEFAULTS", same)

    # bit-exact vs process_frame_v5 (default, uniform channels)
    t = win.timeline.frame()
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
    deps = {"mix": {"mix_enable": True}, "mix_enable": {"mix": 25.0}}   # mix acts only with its Enable
    for k, sp in PARAM_SPECS.items():
        if not sp.implemented:
            continue
        ui = default_params()
        ui.update(deps.get(k, {}))
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

    # profile: AFT (wait for the auto-AFT triggered by the earlier Auto Profile to finish first)
    wait(app, lambda: idle() and not win.runner.busy("aft"), 120)
    # auto-AFT already changed the curve; the explicit AFT button re-runs it (same result)
    win.auto_fine_tune()
    check("Auto Fine-Tune runs and curve is AFT-adjusted",
          wait(app, lambda: idle() and st.profile.base.get("aft_applied")))
    idx_before = st.undo.index()
    st.undo.undo()
    check("undo AFT", st.undo.index() < idx_before and st.profile.base is not None
          and "curve" in st.profile.base)
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

    # views: viewer modes, Assist, save frame, preview area
    from nv_ui_qt.widgets.viewer import MODES, SPLITS
    win.tabs.setCurrentIndex(1)
    wait(app, lambda: idle() and win.viewer.showing_filtered())
    for m, _ in MODES:
        win.viewer.set_mode(m)
        win.viewer.grab()
    win.set_assist(True)
    win.viewer.grab()
    win.set_assist(False)
    win.viewer.set_mode("YCrCb +")
    png = os.path.join(outdir, "frame.png")
    win.save_frame(png)
    check("preview modes + save frame", os.path.getsize(png) > 0)
    win.area_btn.setChecked(True)
    check("Preview Area (crop) filtering", wait(app, lambda: idle() and any(k[2] is not None for k in win.filt_cache)))
    win.area_btn.setChecked(False)
    check("preview area off -> whole frame", win.crop() is None and win.preview_sel is None)
    win.profile_check().close()
    win.guide("filter").close()

    # ---------------- layout checks ----------------
    fpn, ppn = win.filter_page.panel, win.profile_page.panel
    from nv_ui_qt.widgets.rows import Header, Row

    def page_headers(i):
        lay = fpn.tabs.widget(i).widget().layout()
        return [lay.itemAt(j).widget().title_text for j in range(lay.count())
                if isinstance(lay.itemAt(j).widget(), Header)]

    def body_labels(h):
        return [h.lay.itemAt(j).widget().label.text() for j in range(h.lay.count())
                if isinstance(h.lay.itemAt(j).widget(), Row)]

    def body_headers(h):
        return {h.lay.itemAt(j).widget().title_text: h.lay.itemAt(j).widget() for j in range(h.lay.count())
                if isinstance(h.lay.itemAt(j).widget(), Header)}

    tabs = [fpn.tabs.tabText(i) for i in range(fpn.tabs.count())]
    nl = fpn.headers["Noise Level"]
    nra = [h for k, h in fpn.headers.items() if k.startswith("Noise Reduction Amount") and
           "Chrominance" in body_headers(h)][0]
    order_ok = (tabs == ["Temporal", "Spatial", "General"]
                and page_headers(0) == ["Temporal Filter", "Temporal Noise", "Repeated Frames", "Jitter of Details",
                                        "Flicker", "Dust and Scratches", "Adapt to Changing Noise"]
                and "Noise Reduction Amount" in body_headers(fpn.headers["Temporal Noise"])
                and page_headers(1) == ["Spatial Filter", "Noise Reduction Amount", "Noise Level", "Artifact Removal",
                                        "Edge Smoothing", "Sharpening", "Per-component bands (repro)", "Advanced (repro)"]
                and page_headers(2) == ["Mix with Original", "Performance"]
                and body_labels(nl) == ["Luminance", "Cr", "Cb"]
                and body_labels(body_headers(nl)["Frequencies"]) == ["High", "Mid", "Low", "Very Low", "Ultra Low"]
                and body_labels(nra) == ["Luminance"]
                and body_labels(body_headers(nra)["Chrominance"]) == ["Cr", "Cb"]
                and body_labels(body_headers(nra)["Frequencies"]) == ["High", "Mid", "Low", "Very Low", "Ultra Low"])
    pbody = ppn.curve.parentWidget().parentWidget().layout()
    idx = lambda w: pbody.indexOf(w)
    tuning = [w for w in ppn.findChildren(Header) if w.title_text == "Profile Check and Tuning"][0]
    stats = ppn.quality.parentWidget()
    prep_ok = idx(stats) < idx(ppn.curve.parentWidget()) < idx(tuning) and \
        [w.label.text() for w in (ppn.tune["Y"], ppn.tune["Cr"], ppn.tune["Cb"])] == ["Luminance", "Cr", "Cb"]
    check("(a) panel order: Temporal / Spatial / General, Luminance -> Cr -> Cb, High -> Ultra Low",
          order_ok and prep_ok, f"{page_headers(1)}")

    # (b) common Frequencies row: three components, one undo step
    row = [w for w in fpn.common if w.keys[0] == "bamt_Y_high"][0]
    keys = row.keys
    n0 = st.undo.index()
    st.new_edit()
    row.slider.setValue(60)
    row.slider.setValue(55)
    one = st.undo.index() == n0 + 1 and all(st.params[k] == 55.0 for k in keys)
    st.undo.undo()
    back = all(st.params[k] == 100.0 for k in keys) and row.value() == 100.0
    st.set_param("bamt_Cb_medium", 50.0)
    mrow = [w for w in fpn.common if w.keys[0] == "bamt_Y_medium"][0]
    mixed = mrow.val.text() == "mixed"
    st.undo.undo()
    check("(b) Frequencies row writes Y / Cr / Cb in one undo step", one and back and mixed and
          sorted(keys) == ["bamt_Cb_high", "bamt_Cr_high", "bamt_Y_high"] and mrow.val.text() == "100%")

    # (c) Mix with Original: Enable, display 100 - mix, old presets
    me = fpn.controls["mix_enable"]
    off = st.backend()["mix"] == 0.0 and not st.params["mix_enable"]
    n0 = st.undo.index()
    me.setChecked(True)
    on = (st.params["mix_enable"] and st.params["mix"] == 25.0 and st.backend()["mix"] == 0.25
          and fpn.controls["mix"].val.text() == "75%" and st.undo.index() == n0 + 1)
    me.setChecked(False)
    dis = st.backend()["mix"] == 0.0 and st.params["mix"] == 25.0
    st.undo.undo()
    st.undo.undo()
    old = os.path.join(outdir, "old_mix.json")
    json.dump({"format": "nv_repro_filter_preset", "version": 1, "params": {"mix": 30.0}}, open(old, "w"))
    win.load_preset(old)
    old_on = st.params["mix_enable"] and st.backend()["mix"] == 0.3
    json.dump({"format": "nv_repro_filter_preset", "version": 1, "params": {"mix": 0.0}}, open(old, "w"))
    win.load_preset(old)
    old_off = not st.params["mix_enable"]
    win.reset_filter()
    check("(c) mix_enable: off by default, Enable -> 75%, old presets", off and on and dis and old_on and old_off
          and st.params == default_params(), f"{off} {on} {dis} {old_on} {old_off} "
          f"{[k for k in st.params if st.params[k] != default_params()[k]]}")

    # (d) profile Noise Level (tune): multiplies the curve, undo, JSON
    base_c = list(st.effective_profile()["curve"]["Y"])
    st.new_edit()
    ppn.tune["all"].slider.setValue(50)
    eff2 = st.effective_profile()
    scaled = st.profile.tune_value("all") == 1.5 and np.allclose(eff2["curve"]["Y"], np.asarray(base_c) * 1.5, rtol=1e-5)
    st.new_edit()
    ppn.tune["Cr"].slider.setValue(-50)
    cr_ok = np.allclose(st.effective_profile()["curve"]["Cr"],
                        np.asarray(st.profile.base["curve"]["Cr"]) * 0.75 * np.interp(
                            st.profile.base["curve_centers"], [32.0 * k for k in range(9)], st.profile.factors[2]),
                        rtol=1e-5)
    tp = os.path.join(outdir, "tune.nvprof.json")
    win.save_profile(tp)
    saved = json.load(open(tp))["tune"] == {"all": 1.5, "Cr": 0.5}
    st.undo.undo()
    st.undo.undo()
    undone = st.profile.tune_value("all") == 1.0 and st.effective_profile()["curve"]["Y"] == base_c
    from nv_ui_qt.state import ProfileState
    d = json.load(open(tp))
    d.pop("tune")
    legacy = ProfileState.from_json(d).tune_value("all") == 1.0
    win.load_profile(tp)
    loaded = st.profile.tune_value("Cr") == 0.5 and ppn.tune["Cr"].val.text() == "-50%"
    st.undo.undo()
    check("(d) profile Noise Level scales the curve, undo, saved in JSON", scaled and cr_ok and saved and undone
          and legacy and loaded)

    # (e) hold the left button = original, "Filtered" otherwise
    win.tabs.setCurrentIndex(1)
    wait(app, lambda: idle() and win.viewer.showing_filtered())
    vw = win.viewer
    c = vw.panes()[0][1].center().toPoint()
    shown = vw.showing_filtered()
    QTest.mousePress(vw, Qt.LeftButton, Qt.NoModifier, c)
    held = not vw.showing_filtered() and vw.hold
    QTest.mouseRelease(vw, Qt.LeftButton, Qt.NoModifier, c)
    check("(e) press-and-hold shows the original ('Filtered' label off)", shown and held and vw.showing_filtered())

    # (f) YCrCb+ panes and splits
    vw.set_mode("YCrCb +")
    geo = {}
    for sp in SPLITS:
        vw.set_split(sp)
        geo[sp] = [r for _, r in vw.panes()]
    img = vw.grab().toImage()
    b, r2 = geo["1+3 Bottom"], geo["1+3 Right"]
    g2 = geo["2 x 2"]
    names = [n for n, _ in vw.panes()]
    vw.set_mode("Normal")
    normal = len(vw.panes()) == 1
    vw.set_mode("YCrCb +")
    vw.set_split("1+3 Bottom")
    check("(f) YCrCb+ draws 4 panes; split changes the arrangement",
          names == ["rgb", "Y+", "Cr+", "Cb+"] and normal and not img.isNull()
          and all(q.top() > b[0].bottom() for q in b[1:]) and all(q.left() > r2[0].right() for q in r2[1:])
          and abs(g2[0].width() - g2[1].width()) < 1 and abs(g2[0].height() - g2[2].height()) < 1
          and abs(b[0].height() / (b[0].height() + b[1].height()) - 2 / 3) < 0.02)

    # (g) thumbnails follow the Radius; click selects the frame
    th = win.timeline.thumbs
    cur = win.timeline.frame()
    blue, orange = th.frame_color(cur), th.frame_color(cur + 2)
    st.set_param("t_radius", 1)
    r1 = th.frame_color(cur + 1) is not None and th.frame_color(cur + 2) is None and th.frame_color(cur - 1) is not None
    st.set_param("t_enable", False)
    r0 = th.frame_color(cur + 1) is None
    st.undo.undo()
    st.undo.undo()
    tgt = [rr for i, rr in th.slots() if i == cur + 3][0]
    QTest.mouseClick(th, Qt.LeftButton, Qt.NoModifier, tgt.center().toPoint())
    moved = win.timeline.frame() == cur + 3
    win.timeline.set_frame(cur, emit=True)
    check("(g) thumbnails: blue = current, orange = Radius window, click = go to frame",
          blue is not None and orange is not None and blue != orange and r1 and r0 and moved)

    # (h) Beginner / Advanced: rows hidden, values kept
    st.set_param("noise_Cr", 120.0)
    win.set_advanced(False)
    beg = (fpn.controls["noise_Cr"].isHidden() and not fpn.controls["amt_Y"].isHidden()
           and fpn.headers["Mix with Original"].isHidden() and not fpn.headers["Performance"].isHidden()
           and not fpn.controls["adv_noise_level"].isHidden() and ppn.curve.parentWidget().isHidden()
           and fpn.sharp_rows[0].label.text() == "Fine" and st.params["noise_Cr"] == 120.0)
    win.set_advanced(True)
    adv = not fpn.controls["noise_Cr"].isHidden() and fpn.controls["noise_Cr"].value() == 120.0 and \
        not ppn.curve.parentWidget().isHidden() and fpn.sharp_rows[0].label.text() == "High"
    st.undo.undo()
    check("(h) Beginner / Advanced Mode hides rows and keeps values", beg and adv)

    # drag = one undo step, page / wheel steps = separate steps (D6); both slider kinds
    from PySide6.QtWidgets import QAbstractSlider
    drags = []
    for w, get in ((fpn.controls["amt_Y"], lambda: st.params["amt_Y"]),
                   (ppn.tune["all"], lambda: st.profile.tune_value("all"))):
        sl = w.slider
        i0 = st.undo.index()
        v0 = sl.value()
        sl.setSliderDown(True)                  # sliderPressed -> new undo step
        for dv in (3, 6, 9, 12, 15):            # setSliderPosition while down -> actionTriggered(SliderMove)
            sl.setSliderPosition(v0 + dv)
        sl.setSliderDown(False)
        one = st.undo.index() == i0 + 1 and get() != (70.0 if w is fpn.controls["amt_Y"] else 1.0)
        sl.triggerAction(QAbstractSlider.SliderAction.SliderPageStepSub)
        sl.triggerAction(QAbstractSlider.SliderAction.SliderPageStepSub)
        steps = st.undo.index() == i0 + 3
        for _ in range(3):
            st.undo.undo()
        drags.append(one and steps and st.undo.index() == i0)
    check("slider drag = one undo step, page steps = separate steps", all(drags), str(drags))

    # wheel steps never merge into a drag or into each other (D6)
    from PySide6.QtCore import QPoint, QPointF
    from PySide6.QtGui import QWheelEvent

    def wheel(sl):
        c = QPointF(sl.width() / 2, sl.height() / 2)
        QApplication.sendEvent(sl, QWheelEvent(c, sl.mapToGlobal(c), QPoint(0, 0), QPoint(0, 120), Qt.NoButton,
                                               Qt.NoModifier, Qt.NoScrollPhase, False))
        app.processEvents()

    sl = fpn.controls["amt_Y"].slider
    i0 = st.undo.index()
    sl.setSliderDown(True)
    for dv in (-5, -10, -20):
        sl.setSliderPosition(70 + dv)
    sl.setSliderDown(False)
    drag1 = st.undo.index() == i0 + 1 and st.params["amt_Y"] == 50.0
    wheel(sl)
    drag_wheel = st.undo.index() == i0 + 2 and st.params["amt_Y"] != 50.0
    time.sleep(0.3)
    wheel(sl)
    two_wheels = st.undo.index() == i0 + 3
    st.undo.undo()
    st.undo.undo()
    back_to_drag = st.params["amt_Y"] == 50.0
    st.undo.undo()
    check("wheel: drag = 1, drag + wheel = 2, two wheel gestures = 2 undo steps",
          drag1 and drag_wheel and two_wheels and back_to_drag and st.params["amt_Y"] == 70.0,
          f"{drag1} {drag_wheel} {two_wheels} {back_to_drag}")

    # no side-panel row is wider than its viewport at the minimum window width (all sections open)
    from PySide6.QtWidgets import QScrollArea
    size0 = win.size()
    for hh in win.findChildren(Header):
        hh.set_expanded(True)
    win.resize(win.minimumSizeHint().width(), size0.height())
    over = []
    for adv in (True, False):
        win.set_advanced(adv)
        for page in (0, 1):
            win.tabs.setCurrentIndex(page)
            panel = win.stack.currentWidget().panel
            scs = [panel.tabs.widget(i) for i in range(panel.tabs.count())] if page else \
                panel.findChildren(QScrollArea)
            for i, sc in enumerate(scs):
                if page:
                    panel.tabs.setCurrentIndex(i)
                for _ in range(3):
                    app.processEvents()
                if sc.widget().minimumSizeHint().width() > sc.viewport().width():
                    over.append((adv, page, i, sc.widget().minimumSizeHint().width(), sc.viewport().width()))
    win.set_advanced(True)
    win.resize(size0)
    app.processEvents()
    check("side panels fit at the minimum window width (all sections expanded, both modes)", not over,
          f"min width {win.minimumSizeHint().width()} {over}")

    # non-finite profile values are rejected
    good = json.dumps(st.profile.to_json())
    bad = os.path.join(outdir, "bad_tune.json")
    rej = []
    for mut in ("tune", "factor", "curve"):
        e = json.loads(good)
        if mut == "tune":
            e["tune"] = {"all": float("nan")}
        elif mut == "factor":
            e["factors"][0][3] = float("inf")
        else:
            e["base"]["curve"]["Y"][0] = float("nan")
        json.dump(e, open(bad, "w"))
        win.load_profile(bad)
        rej.append(json.dumps(st.profile.to_json()) == good)
    check("profile JSON with NaN / inf tune, factors or curve rejected", all(rej), str(rej))

    # every non-implemented control shows visible "n/a"
    from nv_ui_qt.widgets.rows import ChoiceRow, EnableCheck
    def na_text(w):
        if isinstance(w, Row):
            return w.val.text()
        if isinstance(w, EnableCheck):
            return w.na_label.text() if w.na_label is not None else w.text()
        if isinstance(w, ChoiceRow):
            return w.label.text()
        return ""
    widgets = [w for k, w in fpn.controls.items()] + [w for ws in fpn.extra.values() for w in ws]
    no_na = [w.key for w in widgets if not getattr(w, "spec", PARAM_SPECS.get(getattr(w, "key", ""))).implemented
             and "n/a" not in na_text(w)]
    check("non-implemented controls show 'n/a'", not no_na and "n/a" in win.match_btn.text()
          and "n/a" in ppn.b_manual.text(), str(no_na))

    # genuine slider tracks / texts
    nr, ov = fpn.controls["noise_Y"].slider, fpn.controls["adv_noise_level"]
    pos0 = (0 - nr.minimum()) / (nr.maximum() - nr.minimum())
    mix_txt = fpn.controls["mix"].val.text()
    win.tabs.setCurrentIndex(0)
    prep_r = win.timeline.thumbs.radius
    win.tabs.setCurrentIndex(1)
    check("Noise Level track -100..+150, Mix off shows 75%, Radius '2', no orange frames on Prepare",
          (nr.minimum(), nr.maximum()) == (-100, 150) and abs(pos0 - 0.4) < 1e-9
          and (ov.slider.minimum(), ov.slider.maximum()) == (-100, 150) and ov.spin.maximum() == 300
          and PARAM_SPECS["noise_Y"].hi == 250 and mix_txt == "75%" and not st.params["mix_enable"]
          and fpn.controls["t_radius"].val.text() == "2" and prep_r == 0 and win.timeline.thumbs.radius == 2)

    # (i) Auto Profile button becomes Build Profile with a selection
    win.tabs.setCurrentIndex(0)
    win.on_area(16, 16, 64)
    named = win.auto_btn.text() == "Build Profile" and win.viewer.sel_label.startswith("Noise Level:")
    win._viewer_click()
    check("(i) Auto Profile <-> Build Profile with the selection", named and win.auto_btn.text() == "Auto Profile"
          and win.sel_area is None)
    win.tabs.setCurrentIndex(1)

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
