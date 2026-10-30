"""The N3c comparison scenario of qtui_dump_n3c (qtui/tests/dump_n3c.cpp) on the PySide6 UI (repro/nv_ui_qt, native
core on): Preferences, Profile Viewer, Batch Profiler and the Bit depth setting (loading, display, export at 10 bits,
4:2:2 and 32-bit float), with the same output files as the C++ side (state_*.json, menus.json, panes/*.rgb +
index.json, *.png grabs, batch_*.nvprof.json, export_*.{mp4,mkv} + side files, settings.ini).

    QT_QPA_PLATFORM=offscreen NV_NATIVE=1 python3 qtui/tools/py_dump_n3c.py OUTDIR SRC10 SRC422

SRC10 / SRC422: 10-bit 4:2:0 / 4:2:2 sources (qtui/tools/compare_n3c.py makes them).  Writes only into OUTDIR.
"""
import json
import os
import shutil
import sys
import time

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
os.environ.setdefault("NV_NATIVE", "1")
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
REPRO = os.path.join(ROOT, "repro")
sys.path.insert(0, REPRO)
sys.path.insert(0, HERE)
sys.dont_write_bytecode = True

from PySide6.QtCore import QSettings, Qt  # noqa: E402
from PySide6.QtWidgets import (QApplication, QCheckBox, QComboBox, QDialogButtonBox, QDoubleSpinBox,  # noqa: E402
                               QFormLayout, QLabel, QLineEdit, QListWidget, QPushButton, QSpinBox, QTextEdit,
                               QWidget)

from py_dump import dump_state, qimage_bytes, wait  # noqa: E402


def widget_info(w):
    """The displayed state of a form widget (the same keys as dump_n3c.cpp)."""
    if w is None:
        return {"kind": "none"}
    d = {"enabled": w.isEnabled(), "tip": w.toolTip()}
    if isinstance(w, QDoubleSpinBox):
        d.update(kind="dspin", value=w.value(), text=w.text(), min=w.minimum(), max=w.maximum(), step=w.singleStep())
    elif isinstance(w, QSpinBox):
        d.update(kind="spin", value=w.value(), text=w.text(), min=w.minimum(), max=w.maximum())
    elif isinstance(w, QComboBox):
        d.update(kind="combo", text=w.currentText(), items=[w.itemText(i) for i in range(w.count())],
                 data=[str(w.itemData(i)) if w.itemData(i) is not None else None for i in range(w.count())])
    elif isinstance(w, QCheckBox):
        d.update(kind="check", text=w.text(), checked=w.isChecked())
    elif isinstance(w, QLineEdit):
        d.update(kind="line", text=w.text(), placeholder=w.placeholderText())
    elif isinstance(w, QLabel):
        d.update(kind="label", text=w.text())
    elif isinstance(w, QDialogButtonBox):
        d.update(kind="buttons", texts=[b.text() for b in w.buttons()])
    elif isinstance(w, QPushButton):
        d.update(kind="button", text=w.text())
    else:
        d.update(kind=type(w).__name__)
        kids = w.findChildren(QWidget, options=Qt.FindDirectChildrenOnly)
        if kids:                    # a composite field (the ffmpeg row: line edit + Browse…)
            d["children"] = [widget_info(c) for c in kids]
    return d


def form_rows(form):
    rows = []
    for i in range(form.rowCount()):
        li = form.itemAt(i, QFormLayout.LabelRole)
        fi = form.itemAt(i, QFormLayout.FieldRole) or form.itemAt(i, QFormLayout.SpanningRole)
        rows.append({"label": li.widget().text() if li is not None and li.widget() is not None else None,
                     **widget_info(fi.widget() if fi is not None else None)})
    return rows


def menus(win):
    out = []
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
        out.append({"title": top.text(), "items": items})
    return out


def extras(win):
    """Main-window strings of the N3c features: Performance table, Image information, export dialog defaults."""
    box = win.image_info()
    info = box.text() if box is not None else None
    if box is not None:
        box.close()
    fp = win.filter_page.panel
    return {"perf": fp.perf.text(), "image_info": info, "export_filter": list(win._export_filter()),
            "title": win.windowTitle(), "depth_used": str(win.depth_used), "msg": win.msg.text()}


def state_with(win, **kw):
    d = json.loads(dump_state(win))
    d.update(kw)
    return json.dumps(d, indent=1)


def main():
    out, src10, src422 = sys.argv[1], sys.argv[2], sys.argv[3]
    video = os.path.join(ROOT, "test.mp4")
    os.makedirs(os.path.join(out, "panes"), exist_ok=True)
    ini = os.path.join(out, "settings.ini")
    if os.path.exists(ini):
        os.remove(ini)
    os.environ["NV_UI_QT_SETTINGS"] = ini
    s = QSettings(ini, QSettings.IniFormat)
    s.setValue("prefs/mem_gb", 0.05)
    s.sync()
    app = QApplication([sys.argv[0]])
    app.setApplicationName("nv_ui_qt")
    from nv_ui_qt.main_window import MainWindow
    win = MainWindow(video)
    win.show()
    st = win.state
    idle = lambda: not win.runner.running and not win.runner.queued   # noqa: E731
    settled = lambda: idle() and not win.timer.isActive() and not win.pc_timer.isActive() and win.viewer.ready()  # noqa: E731

    def loaded(path):
        return lambda: (win.src == path and st.profile.base is not None and st.profile.base.get("aft_applied")
                        and idle())

    print("t8:", "ok" if wait(app, loaded(video)) else "TIMEOUT", flush=True)
    wait(app, settled, 120)
    open(os.path.join(out, "state_t8.json"), "w").write(state_with(win, **extras(win)))
    json.dump(menus(win), open(os.path.join(out, "menus.json"), "w"), indent=1)
    win.grab().save(os.path.join(out, "win_t8.png"))

    # Preferences
    dlg = win.preferences()
    app.processEvents()
    form = dlg.findChild(QFormLayout)
    open(os.path.join(out, "prefs_rows.json"), "w").write(json.dumps({"title": dlg.windowTitle(),
                                                                     "rows": form_rows(form), "profile": None}, indent=1))
    dlg.grab().save(os.path.join(out, "dlg_prefs.png"))
    [w for w in dlg.findChildren(QSpinBox) if not isinstance(w, QDoubleSpinBox)][0].setValue(7)   # rejected: kept
    dlg.reject()
    app.processEvents()

    # Profile Viewer (the mode combo writes prefs/profile_mode at once)
    dlg = win.profile_viewer()
    app.processEvents()
    form = dlg.findChild(QFormLayout)
    pv = {"title": dlg.windowTitle(), "rows": form_rows(form), "details": dlg.findChild(QTextEdit).toPlainText(),
          "profile": None}
    dlg.grab().save(os.path.join(out, "dlg_pviewer.png"))
    combo = dlg.findChild(QComboBox)
    combo.setCurrentText("spatial")
    pv["profile_mode_after_change"] = str(QSettings(ini, QSettings.IniFormat).value("prefs/profile_mode"))
    pv["prefs_profile_mode"] = win.prefs["profile_mode"]
    combo.setCurrentText("auto")
    open(os.path.join(out, "state_pviewer.json"), "w").write(json.dumps(pv, indent=1))
    dlg.close()
    app.processEvents()
    win.reset_profile()                     # the viewer without a profile
    dlg = win.profile_viewer()
    app.processEvents()
    form = dlg.findChild(QFormLayout)
    open(os.path.join(out, "state_pviewer_none.json"), "w").write(json.dumps(
        {"rows": form_rows(form), "details": dlg.findChild(QTextEdit).toPlainText(), "profile": None}, indent=1))
    dlg.grab().save(os.path.join(out, "dlg_pviewer_none.png"))
    dlg.close()
    st.undo.undo()
    app.processEvents()

    # Batch Profiler: the dialog, then runs (depth 8; mode spatial on the 4:2:2 source)
    dlg = win.batch_profiler()
    app.processEvents()
    open(os.path.join(out, "state_batchdlg.json"), "w").write(json.dumps(
        {"title": dlg.windowTitle(), "buttons": [b.text() for b in dlg.findChildren(QPushButton)],
         "modes": [dlg.findChild(QComboBox).itemText(i) for i in range(dlg.findChild(QComboBox).count())],
         "list": dlg.findChild(QListWidget).count(), "profile": None}, indent=1))
    dlg.grab().save(os.path.join(out, "dlg_batch.png"))
    dlg.reject()
    app.processEvents()

    def batch(files, tag, mode=None):
        d = os.path.join(out, "batch_" + tag)
        os.makedirs(d, exist_ok=True)
        win.batch_result = None
        win.batch_profiler(files, d, mode)
        wait(app, lambda: idle() and win.batch_result is not None, 600)
        for p in win.batch_result:
            shutil.copy(p, os.path.join(out, f"batch_{tag}_" + os.path.basename(p)))
        print(f"batch {tag}: {len(win.batch_result)} profiles; {win.msg.text().replace(out, '<OUT>')}", flush=True)
        return win.msg.text().replace(out, "<OUT>")

    bmsg = [batch([video, src10], "d8"), batch([src422], "d8s", "spatial")]

    exports = {}

    def export(tag, ext, start, frames):
        dst = os.path.join(out, f"export_{tag}{ext}")
        win.process_video(dst, start, frames)
        wait(app, lambda: win.export is None, 600)
        exports[tag] = bool(getattr(win, "export_ok", False))
        print(f"export {tag}: {exports[tag]} {win.msg.text()}", flush=True)

    def prefs_depth(val):
        d = win.preferences()
        app.processEvents()
        cb = [c for c in d.findChildren(QComboBox) if c.findData(val) >= 0][0]
        cb.setCurrentIndex(cb.findData(val))
        d.accept()
        app.processEvents()

    def panes(tag, modes=("Normal", "YCrCb +")):
        vw = win.viewer
        for mode in modes:
            vw.set_mode(mode)
            wait(app, vw.ready, 120)
            src = vw._source()
            for name, _r in vw.panes():
                qi = vw.pane_image(name, src if name == "rgb" else vw._plane_src(src))
                data, h, w = qimage_bytes(qi)
                fn = f"{tag}_{mode}_{name}".replace(" ", "_").replace("+", "P")
                open(os.path.join(out, "panes", fn + ".rgb"), "wb").write(data)
                index.append({"file": fn, "H": h, "W": w})

    index = []
    # Bit depth 10: test.mp4 (8-bit source) is reloaded as yuv420p10le
    prefs_depth("10")
    print("t10:", "ok" if wait(app, lambda: win.depth_used == "10" and loaded(video)()) else "TIMEOUT", flush=True)
    wait(app, settled, 120)
    open(os.path.join(out, "state_t10.json"), "w").write(state_with(win, **extras(win)))
    panes("t10")
    win.viewer.set_mode("YCrCb +")
    wait(app, settled, 120)
    win.grab().save(os.path.join(out, "win_t10.png"))

    # a 10-bit source at 10 bits: Prepare, Adjust (preview), export (libx264 High 10 mp4)
    win.open_video(src10)
    print("s10:", "ok" if wait(app, loaded(src10)) else "TIMEOUT", flush=True)
    wait(app, settled, 120)
    open(os.path.join(out, "state_s10.json"), "w").write(state_with(win, **extras(win)))
    panes("s10")
    win.tabs.setCurrentIndex(1)
    wait(app, lambda: idle() and win.viewer.showing_filtered(), 600)
    wait(app, settled, 120)
    panes("s10adj", ("Normal",))
    win.grab().save(os.path.join(out, "win_s10_adjust.png"))
    win.tabs.setCurrentIndex(0)
    wait(app, settled, 120)
    export("s10", ".mp4", 2, 3)
    bmsg.append(batch([src10], "d10"))

    # Source depth: the 4:2:2 source as yuv422p10le (widened to 4:4:4), export libx264 4:2:2
    prefs_depth("source")
    wait(app, lambda: win.depth_used == "source" and loaded(src10)(), 600)
    win.open_video(src422)
    print("s422:", "ok" if wait(app, loaded(src422)) else "TIMEOUT", flush=True)
    wait(app, settled, 120)
    open(os.path.join(out, "state_s422.json"), "w").write(state_with(win, **extras(win)))
    panes("s422")
    export("s422", ".mp4", 1, 3)

    # 32-bit float: the 4:2:2 source as gbrpf32le, export FFV1 .mkv
    prefs_depth("32")
    print("s422_32:", "ok" if wait(app, lambda: win.depth_used == "32" and loaded(src422)()) else "TIMEOUT", flush=True)
    wait(app, settled, 120)
    open(os.path.join(out, "state_s422_32.json"), "w").write(state_with(win, **extras(win)))
    panes("s422_32")
    win.grab().save(os.path.join(out, "win_s422_32.png"))
    export("s422_32", ".mkv", 1, 3)
    bmsg.append(batch([src422], "d32"))

    json.dump(index, open(os.path.join(out, "panes", "index.json"), "w"), indent=1)
    json.dump({"exports": exports, "batch_msgs": bmsg}, open(os.path.join(out, "runs.json"), "w"), indent=1)
    print("done", flush=True)
    win.close()


if __name__ == "__main__":
    main()
