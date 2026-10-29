"""The comparison scenario of qtui_dump (qtui/tests/dump.cpp) on the PySide6 UI (repro/nv_ui_qt, native core on):
the same steps, the same output files (state_*.json, panes/*.rgb + index.json, *.png grabs).

    QT_QPA_PLATFORM=offscreen NV_NATIVE=1 python3 qtui/tools/py_dump.py VIDEO OUTDIR Y,X,H,W [MEM_GB]

Reads the repro code; writes only into OUTDIR.
"""
import json
import os
import sys
import time

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
os.environ.setdefault("NV_NATIVE", "1")
HERE = os.path.dirname(os.path.abspath(__file__))
REPRO = os.path.normpath(os.path.join(HERE, "..", "..", "repro"))
sys.path.insert(0, REPRO)
sys.dont_write_bytecode = True

import numpy as np  # noqa: E402
from PySide6.QtCore import QSettings  # noqa: E402
from PySide6.QtWidgets import QApplication  # noqa: E402


def wait(app, cond, timeout=600.0):
    t0 = time.time()
    while not cond():
        app.processEvents()
        time.sleep(0.01)
        if time.time() - t0 > timeout:
            return False
    app.processEvents()
    return True


def qimage_bytes(qi):
    """RGB888 bytes of a QImage (rows without padding)."""
    qi = qi.convertToFormat(qi.Format.Format_RGB888)
    w, h, bpl = qi.width(), qi.height(), qi.bytesPerLine()
    a = np.frombuffer(qi.constBits(), np.uint8, count=bpl * h).reshape(h, bpl)[:, :3 * w]
    return a.tobytes(), h, w


def dump_state(win):
    """The same dict as MainWindow::dump_state of the C++ UI."""
    pp = win.profile_page.panel
    from nv_ui_qt.state import UI_BANDS
    d = {"quality": pp.quality.text(), "completeness": pp.completeness.text(), "main_val": pp.main_val.text(),
         "sel_val": pp.sel_val.text(), "name": pp.name.currentText(), "info": pp.info.text(),
         "bars": [float(pp.bars[b].value) for b, _ in UI_BANDS], "prof_lbl": win.prof_lbl.text(),
         "video_lbl": win.video_lbl.text(), "depth_lbl": win.depth_lbl.text(), "sel_label": win.viewer.sel_label,
         "sel_warn": list(win.viewer.sel_warn), "wh": win.viewer_bar.wh.text(), "frame_info": win.viewer_bar.size.text(),
         "auto_btn": win.auto_btn.text(), "check_visible": not win.check_btn.isHidden(), "frame": win.timeline.frame(),
         "timeline_label": win.timeline.label.text(),
         "tune_rows": {k: pp.tune[k].val.text() for k in ("all", "Y", "Cr", "Cb")},
         "profile": win.state.profile.to_json() if win.state.profile.base is not None else None}
    return json.dumps(d, indent=1)


def main():
    video, out, rect = sys.argv[1], sys.argv[2], [int(v) for v in sys.argv[3].split(",")]
    mem = float(sys.argv[4]) if len(sys.argv) > 4 else 4.0
    os.makedirs(os.path.join(out, "panes"), exist_ok=True)
    ini = os.path.join(out, "settings.ini")
    if os.path.exists(ini):
        os.remove(ini)
    os.environ["NV_UI_QT_SETTINGS"] = ini
    s = QSettings(ini, QSettings.IniFormat)
    s.setValue("prefs/mem_gb", mem)
    s.sync()
    app = QApplication([sys.argv[0]])
    app.setApplicationName("nv_ui_qt")
    from nv_ui_qt.main_window import MainWindow
    from nv_ui_qt.widgets.viewer import MODES
    win = MainWindow(video)
    win.show()
    st = win.state
    idle = lambda: not win.runner.running and not win.runner.queued   # noqa: E731
    ok = wait(app, lambda: st.profile.base is not None and st.profile.base.get("aft_applied") and idle())
    print("load + auto profile + AFT:", "ok" if ok else "TIMEOUT", flush=True)
    wait(app, lambda: idle() and win.viewer.ready(), 120)
    open(os.path.join(out, "state_auto.json"), "w").write(dump_state(win))
    menus = []
    for top in win.menuBar().actions():
        items = []

        def walk(menu, pre):
            for a in menu.actions():
                items.append({"text": pre + a.text(), "sep": a.isSeparator(), "enabled": a.isEnabled(),
                              "checkable": a.isCheckable(), "shortcut": a.shortcut().toString()})
                if a.menu():
                    walk(a.menu(), pre + a.text() + " > ")
        if top.menu():
            walk(top.menu(), "")
        menus.append({"title": top.text(), "items": items})
    json.dump(menus, open(os.path.join(out, "menus.json"), "w"), indent=1)
    from PySide6.QtCore import Qt
    from PySide6.QtWidgets import QToolButton
    tb = []
    for i in range(win.tb_stack.count()):
        for b in win.tb_stack.widget(i).findChildren(QToolButton, options=Qt.FindDirectChildrenOnly):
            tb.append({"page": i, "text": b.text(), "enabled": b.isEnabled(), "checkable": b.isCheckable(),
                       "tip": b.toolTip()})
    json.dump(tb, open(os.path.join(out, "toolbars.json"), "w"), indent=1)
    win.grab().save(os.path.join(out, "win_prepare_auto.png"))

    ry, rx, rh, rw = rect
    win.on_area(ry, rx, rh, rw)
    wait(app, lambda: idle() and win.viewer.ready(), 120)
    open(os.path.join(out, "state_sel.json"), "w").write(dump_state(win))
    win.grab().save(os.path.join(out, "win_prepare_sel.png"))
    win.viewer.grab().save(os.path.join(out, "viewer_sel.png"))

    win.build_profile(False)

    def built():
        b = st.profile.base
        return b is not None and b.get("aft_applied") and b["area"][0] == ry and b["area"][1] == rx and idle()
    print("build profile + AFT:", "ok" if wait(app, built) else "TIMEOUT", flush=True)
    wait(app, lambda: idle() and win.viewer.ready(), 120)
    open(os.path.join(out, "state_build.json"), "w").write(dump_state(win))
    win.grab().save(os.path.join(out, "win_prepare_build.png"))
    win.viewer.grab().save(os.path.join(out, "viewer_build.png"))

    index = []

    def dump_panes(tag):
        vw = win.viewer
        src = vw._source()
        for mode, _ in MODES:
            vw.set_mode(mode)
            for b in (50, 23, 77):
                win.viewer_bar.bright.setValue(b)
                wait(app, vw.ready, 120)
                for name, _r in vw.panes():
                    qi = vw.pane_image(name, src if name == "rgb" else vw._plane_src(src))
                    data, h, w = qimage_bytes(qi)
                    fn = f"{tag}_{mode}_{b}_{name}".replace(" ", "_").replace("+", "P")
                    open(os.path.join(out, "panes", fn + ".rgb"), "wb").write(data)
                    index.append({"file": fn, "H": h, "W": w})
        win.viewer_bar.bright.setValue(50)

    dump_panes("prepare")
    win.viewer.set_mode("YCrCb +")
    wait(app, win.viewer.ready, 120)
    win.viewer.grab().save(os.path.join(out, "viewer_ycc_build.png"))

    win.set_profile_check(True)
    wait(app, lambda: win.viewer.showing_pcheck() and idle() and win.viewer.ready(), 300)
    dump_panes("pcheck")
    win.viewer.set_mode("YCrCb +")
    wait(app, win.viewer.ready, 120)
    win.grab().save(os.path.join(out, "win_pcheck.png"))
    win.set_profile_check(False)

    win.tabs.setCurrentIndex(1)
    wait(app, lambda: idle() and win.viewer.showing_filtered(), 600)
    wait(app, lambda: idle() and win.viewer.ready(), 120)
    dump_panes("adjust")
    win.viewer.set_mode("Normal")
    open(os.path.join(out, "state_adjust.json"), "w").write(dump_state(win))
    win.grab().save(os.path.join(out, "win_adjust.png"))
    win.area_btn.setChecked(True)
    wait(app, lambda: idle() and win.viewer.showing_filtered() and win.preview_sel is not None, 600)
    wait(app, idle, 600)
    data, h, w = qimage_bytes(win.viewer.pane_image("rgb", win.viewer._source()))
    open(os.path.join(out, "panes", "adjust_area_rgb.rgb"), "wb").write(data)
    index.append({"file": "adjust_area_rgb", "H": h, "W": w})
    win.grab().save(os.path.join(out, "win_adjust_area.png"))
    adjust_steps(app, win, out, index, idle)
    json.dump(index, open(os.path.join(out, "panes", "index.json"), "w"), indent=1)
    print("done", flush=True)
    win.close()


def filter_dump(win, out):
    """The Filter Settings panel as the C++ dump writes it (filter_<tag>.json)."""
    from nv_ui_qt.state import PARAM_SPECS
    from nv_ui_qt.widgets.rows import ChoiceRow, EnableCheck, Row
    fpn = win.filter_page.panel
    ctl = {}
    for k in PARAM_SPECS:
        w = fpn.controls.get(k)
        if w is None:
            continue
        v = w.val.text() if isinstance(w, Row) else str(w.isChecked()) if isinstance(w, EnableCheck) else \
            w.box.currentText() if isinstance(w, ChoiceRow) else "?"
        ctl[k] = {"val": v, "enabled": w.isEnabled(), "hidden": w.isHidden(),
                  "reset": (not w.reset.isHidden()) if isinstance(w, Row) else None,
                  "slider": w.slider.value() if isinstance(w, Row) else None}
    common = [{"keys": list(w.keys), "val": w.val.text(), "enabled": w.isEnabled()} for w in fpn.common]
    headers = sorted([{"title": t, "text": h.title.text(), "collapsed": h.body.isHidden(), "hidden": h.isHidden(),
                       "enabled": h.isEnabled()} for t, h in fpn.headers.items()], key=lambda d: d["title"])
    d = {"controls": ctl, "common": common, "headers": headers,
         "tabs_enabled": [fpn.tabs.isTabEnabled(i) for i in range(3)], "tab": fpn.tabs.currentIndex(),
         "nl_check": fpn.nl_check.button.text(), "nl_title": fpn.nl_check.title.text(),
         "assist": fpn.assist.button.text(), "fsc_btn": win.fsc_btn.text(), "fsc_checked": win.fsc_btn.isChecked(),
         "nlc_action": win.actions["nlc"].isChecked(), "perf": fpn.perf.text(), "name": fpn.name.currentText(),
         "mode_lbl": fpn.mode_lbl.text(), "sharp_labels": [r.label.text() for r in fpn.sharp_rows],
         "msg": win.msg.text().replace(out, "<OUT>"), "prof_lbl": win.prof_lbl.text(), "undo_index": win.state.undo.index(),
         "undo_text": win.state.undo.undoText(), "radius": win.timeline.thumbs.radius,
         "viewer_mode": win.viewer.mode(), "params": win.state.params}
    return json.dumps(d, indent=1)


def adjust_steps(app, win, out, index, idle):
    """N3b: the Adjust page (Filter Settings values, Beginner mode, edits + undo / redo, presets, Noise Level
    Check, thumbnails, Performance tab) and two exports."""
    st, vw, fpn = win.state, win.viewer, win.filter_page.panel

    def pane(name, fn):
        qi = vw.pane_image(name, vw._source() if name == "rgb" else vw._plane_src(vw._source()))
        data, h, w = qimage_bytes(qi)
        open(os.path.join(out, "panes", fn + ".rgb"), "wb").write(data)
        index.append({"file": fn, "H": h, "W": w})

    def w(fn):
        open(os.path.join(out, fn), "w").write(filter_dump(win, out))

    win.area_btn.setChecked(False)
    wait(app, lambda: idle() and vw.showing_filtered(), 600)
    w("filter_adjust.json")
    for i, nm in ((1, "spatial"), (2, "general")):
        fpn.tabs.setCurrentIndex(i)
        app.processEvents()
        win.grab().save(os.path.join(out, f"win_adjust_{nm}.png"))
    fpn.tabs.setCurrentIndex(0)
    win.set_advanced(False)
    app.processEvents()
    win.grab().save(os.path.join(out, "win_adjust_beginner.png"))
    w("filter_beginner.json")
    win.set_advanced(True)
    app.processEvents()
    # edits through the state (the same undo steps as the panel's rows produce)
    st.new_edit()
    st.set_param("amt_Y", 50.0)
    st.new_edit()
    st.set_params({"bamt_Y_medium": 60.0, "bamt_Cr_medium": 60.0, "bamt_Cb_medium": 60.0}, "Change filter settings")
    st.new_edit()
    st.set_param("bamt_Cr_low", 30.0)
    st.new_edit()
    st.set_params({"mix_enable": True, "mix": 25.0}, "Change filter settings")
    st.new_edit()
    st.set_param("t_noise", 60.0)
    st.new_edit()
    st.set_param("t_radius", 1)
    st.new_edit()
    st.set_param("adv_noise_level", 150.0)
    wait(app, lambda: idle() and vw.showing_filtered() and not win.timer.isActive(), 600)
    w("filter_edited.json")
    open(os.path.join(out, "state_edited.json"), "w").write(dump_state(win))
    pane("rgb", "adjust_edit_rgb")
    fpn.tabs.setCurrentIndex(1)
    app.processEvents()
    win.grab().save(os.path.join(out, "win_adjust_edit.png"))
    fpn.tabs.setCurrentIndex(0)
    st.undo.undo()
    st.undo.undo()
    app.processEvents()
    w("filter_undo2.json")
    st.undo.redo()
    app.processEvents()
    w("filter_redo1.json")
    win.save_preset(os.path.join(out, "preset.nvpreset.json"))
    win.save_profile(os.path.join(out, "profile.nvprof.json"))
    w("filter_saved.json")
    # Noise Level Check
    win.set_noise_level_check(True)
    wait(app, lambda: idle() and not win.nlc_timer.isActive() and vw.showing_nlc() and win._nlc_shown == win._nlc_key(), 600)
    pane("rgb", "nlc_rgb")
    win.grab().save(os.path.join(out, "win_nlc.png"))
    w("filter_nlc.json")
    st.new_edit()
    st.set_param("t_noise", 40.0)
    wait(app, lambda: idle() and not win.nlc_timer.isActive() and vw.showing_nlc() and win._nlc_shown == win._nlc_key(), 600)
    pane("rgb", "nlc_rgb40")
    w("filter_nlc40.json")
    win.set_noise_level_check(False)
    wait(app, lambda: idle() and vw.showing_filtered(), 600)
    w("filter_nlc_off.json")
    # thumbnails
    n = len(win.frames)
    for i in sorted({0, win.timeline.frame(), n - 1}):
        data, h, ww = qimage_bytes(win._thumb(i))
        open(os.path.join(out, "panes", f"thumb_{i}.rgb"), "wb").write(data)
        index.append({"file": f"thumb_{i}", "H": h, "W": ww})
    win.timeline.grab().save(os.path.join(out, "timeline.png"))
    win.performance_tab()
    app.processEvents()
    win.grab().save(os.path.join(out, "win_perf.png"))
    fpn.tabs.setCurrentIndex(0)
    # exports: (a) the current (edited) settings, CRF 0, 4 frames; (b) the defaults, CRF 16, 12 frames
    win.prefs["crf"] = 0
    win.process_video(os.path.join(out, "export_a.mp4"), start=300, frames=4)
    ok_a = wait(app, lambda: win.export is None, 1800) and getattr(win, "export_ok", False)
    win.reset_filter()
    win.prefs["crf"] = 16
    win.process_video(os.path.join(out, "export_b.mp4"), start=300, frames=12)
    ok_b = wait(app, lambda: win.export is None, 1800) and getattr(win, "export_ok", False)
    json.dump({"export_a": ok_a, "export_b": ok_b, "msg": win.msg.text()}, open(os.path.join(out, "exports.json"), "w"), indent=1)
    w("filter_final.json")


if __name__ == "__main__":
    main()
