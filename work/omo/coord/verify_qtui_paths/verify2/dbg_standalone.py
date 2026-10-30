import os, sys, time, json
os.environ["QT_QPA_PLATFORM"] = "offscreen"
os.environ["NV_UI_QT_SETTINGS"] = "/Users/seitan/Desktop/work/reverse/sandbox/omo/coord/verify_qtui_paths/verify2/st_dbg_sa.ini"
sys.path.insert(0, "/Users/seitan/Desktop/work/reverse/repro"); os.chdir("/Users/seitan/Desktop/work/reverse/repro")
def main():
    from PySide6.QtWidgets import QApplication
    from PySide6.QtTest import QTest
    from PySide6.QtCore import Qt
    from nv_ui_qt.main_window import MainWindow
    app = QApplication(sys.argv)
    win = MainWindow("../test.mp4"); win.show()
    def wait(cond, timeout=120):
        t0 = time.time()
        while not cond():
            app.processEvents(); time.sleep(0.01)
            if time.time() - t0 > timeout: return False
        app.processEvents(); return True
    idle = lambda: not win.runner.running and not win.runner.queued
    wait(lambda: win.frames is not None and idle())
    win.tabs.setCurrentIndex(1); app.processEvents()
    QTest.mouseClick(win.auto_btn, Qt.LeftButton)
    wait(lambda: win.state.profile.base is not None); wait(lambda: win.state.profile.base.get("aft_applied") and idle())
    t = win.timeline.frame()
    wait(lambda: idle() and (t, win.gen, None) in win.filt_cache)
    QTest.mouseClick(win.b_apply, Qt.LeftButton); wait(idle)
    wait(lambda: idle() and (t, win.gen, None) in win.filt_cache)
    for k in range(3):
        t1 = t + 2 if k % 2 == 0 else t
        win.timeline.set_frame(t1); app.processEvents()
        ok = wait(lambda: idle() and (t1, win.gen, None) in win.filt_cache, 15)
        print(json.dumps(dict(k=k, frame=t1, ok=ok, keys=[list(map(str, kk)) for kk in win.filt_cache], gen=win.gen, status=win.msg.text())), flush=True)
        QTest.mouseClick(win.b_apply, Qt.LeftButton); wait(idle)
        wait(lambda: idle() and (t1, win.gen, None) in win.filt_cache, 15)
    win.close()
if __name__ == "__main__":
    main()
