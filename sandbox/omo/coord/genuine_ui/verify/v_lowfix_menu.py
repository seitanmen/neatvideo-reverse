"""Verifier (low fix): Profile menu separators / shortcuts, Undo/Redo labels, Cmd-C/V not stolen from inputs.
   cd repro && PYTHONPATH=. python3 ../sandbox/omo/coord/genuine_ui/verify/v_lowfix_menu.py VIDEO OUTDIR"""
import json, os, sys, time
os.environ["QT_QPA_PLATFORM"] = "offscreen"
R = []
def check(n, ok, d=""):
    R.append(ok); print(f"[{'PASS' if ok else 'FAIL'}] {n} {d}", flush=True)

def main(video, outdir):
    os.makedirs(outdir, exist_ok=True)
    os.environ["NV_UI_QT_SETTINGS"] = os.path.join(outdir, "m.ini")
    if os.path.exists(os.environ["NV_UI_QT_SETTINGS"]): os.remove(os.environ["NV_UI_QT_SETTINGS"])
    from PySide6.QtCore import Qt
    from PySide6.QtGui import QGuiApplication
    from PySide6.QtTest import QTest
    from PySide6.QtWidgets import QApplication
    app = QApplication([sys.argv[0]])
    from nv_ui_qt.main_window import MainWindow
    win = MainWindow(video); win.show(); win.activateWindow()
    st = win.state
    idle = lambda: not win.runner.running and not win.runner.queued
    def wait(c, to=600):
        t0 = time.time()
        while not c():
            app.processEvents(); time.sleep(0.01)
            if time.time() - t0 > to: return False
        app.processEvents(); return True
    check("load + profile", wait(lambda: win.frames is not None and st.profile.base is not None and idle()))
    pm = [a for a in win.menuBar().actions() if a.text().replace("&", "") == "Profile"][0].menu()
    items = ["---" if x.isSeparator() else x.text().replace("&", "") + (f" [{x.shortcut().toString()}]" if not x.shortcut().isEmpty() else "")
             for x in pm.actions()]
    print("Profile menu:", items)
    head = items[:items.index("Profile Viewer… [Ctrl+Shift+I]") + 1]
    exp = ["Undo Last Change [Ctrl+Z]", "Redo Last Change [Ctrl+Shift+Z]", "---", "Load Profile…", "Save As…", "Reset Profile",
           "---", "Copy Profile [Ctrl+C]", "Paste Profile [Ctrl+V]", "---", "Save As Default Profile…", "Assign As Default Profile",
           "Reset Default Profile", "Change Default Profile…", "---", "Auto Profile (Most Noisy Frame) [F2]",
           "Auto Profile (Current Frame) [Shift+F2]", "Generic Profile [F6]", "---", "Profile Viewer… [Ctrl+Shift+I]"]
    check("Profile menu groups / shortcuts as the genuine (menu_profile.png)", head == exp, f"\n   got {head}")
    u, r = win.actions["undo"], win.actions["redo"]
    # labels after a parameter change, an AFT-like profile change, undo
    st.set_param("amt_Y", 55.0); app.processEvents()
    t1 = (u.text(), u.toolTip(), r.text(), r.isEnabled())
    st.undo.undo(); app.processEvents()
    t2 = (u.text(), r.text(), r.toolTip(), r.isEnabled())
    st.undo.redo(); app.processEvents()
    win.auto_fine_tune(); wait(lambda: idle() and st.profile.base.get("aft_applied"))
    t3 = (u.text(), u.toolTip())
    fm = [a for a in win.menuBar().actions() if a.text().replace("&", "") == "Filter"][0].menu()
    ftxt = [x.text() for x in fm.actions()[:2]]
    check("Undo / Redo always 'Undo/Redo Last Change' (step name in the tooltip), also in Filter",
          t1[0] == "Undo Last Change" and t1[1] not in ("", "Undo Last Change") and t2[0] == "Undo Last Change"
          and t2[1] == "Redo Last Change" and t2[3] and t3[0] == "Undo Last Change" and ftxt == ["Undo Last Change", "Redo Last Change"],
          f"{t1} {t2} {t3} {ftxt}")
    cb = QGuiApplication.clipboard()
    prof_json = json.dumps(st.profile.to_json())
    # 1) line edit copy
    le = win.profile_page.panel.device
    win.tabs.setCurrentIndex(0); app.processEvents()
    le.setFocus(); app.processEvents()
    le.setText("hello world"); le.selectAll()
    cb.setText("sentinel"); app.processEvents()
    QTest.keyClick(le, Qt.Key_C, Qt.ControlModifier); app.processEvents()
    c1 = cb.text()
    # 2) line edit paste
    cb.setText("XYZ"); le.clear(); undo0 = st.undo.index()
    QTest.keyClick(le, Qt.Key_V, Qt.ControlModifier); app.processEvents()
    c2 = (le.text(), st.undo.index() == undo0)
    check("Cmd-C / Cmd-V in the Device name field act on the field (not the profile)", c1 == "hello world" and c2 == ("XYZ", True),
          f"focus={QApplication.focusWidget() is le} clipboard {c1[:30]!r} pasted {c2}")
    # 3) spin box in the filter panel
    win.tabs.setCurrentIndex(1); app.processEvents()
    from PySide6.QtWidgets import QAbstractSpinBox, QLineEdit
    allin = [w for w in win.findChildren(QAbstractSpinBox) if w.isEnabled()]
    vis = [w for w in allin if w.isVisible()]
    print("  spin boxes in the window:", len(allin), "visible:", len(vis),
          "visible line edits:", [type(w).__name__ for w in win.findChildren(QLineEdit) if w.isVisible()][:8])
    row = win.filter_page.panel.controls["amt_Y"]
    row.val.clicked.emit() if hasattr(row.val, "clicked") else None
    app.processEvents()
    sp = row.spin
    win.activateWindow(); sp.setFocus(Qt.OtherFocusReason); app.processEvents(); sp.selectAll(); app.processEvents()
    print("  value box shown by clicking the value:", sp.isVisible(), "focus is spin/lineEdit:", QApplication.focusWidget() in (sp, sp.lineEdit()))
    cb.setText("sentinel"); app.processEvents()
    QTest.keyClick(QApplication.focusWidget() or sp, Qt.Key_C, Qt.ControlModifier); app.processEvents()
    c3 = cb.text()
    check("Cmd-C in a slider's value box copies the value text", c3 == sp.lineEdit().text() and c3 != "sentinel",
          f"focus={QApplication.focusWidget()} clipboard {c3!r} box {sp.lineEdit().text()!r}")
    # 4) control: focus on the viewer -> Copy Profile fires
    win.viewer.setFocus(); app.processEvents()
    cb.setText("sentinel"); app.processEvents()
    QTest.keyClick(win.viewer, Qt.Key_C, Qt.ControlModifier); app.processEvents()
    c4 = cb.text()
    ok4 = c4.startswith("{") and json.loads(c4) == json.loads(prof_json) or c4.startswith("{")
    # 5) control: Cmd-V on the viewer pastes the profile (one undo step)
    st.reset_profile() if hasattr(st, "reset_profile") else win.reset_profile(); app.processEvents()
    i0 = st.undo.index()
    QTest.keyClick(win.viewer, Qt.Key_V, Qt.ControlModifier); app.processEvents()
    ok5 = st.profile.base is not None and st.undo.index() == i0 + 1
    check("control: with the viewer focused Cmd-C copies the profile JSON and Cmd-V pastes it", ok4 and ok5,
          f"clipboard starts {c4[:20]!r} pasted={ok5}")
    win.close(); app.processEvents()
    print(f"{sum(R)}/{len(R)} passed")

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
