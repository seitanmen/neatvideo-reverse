"""Clicks Noise Level Check Begin (filter panel) offscreen and records what happens."""
import os, sys, time, traceback
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../../../.."))
sys.path.insert(0, os.path.join(ROOT, "repro"))


def main():
    errs = []
    sys.excepthook = lambda *a: errs.append("".join(traceback.format_exception(*a)))
    from PySide6.QtWidgets import QApplication
    app = QApplication.instance() or QApplication([])
    from nv_ui_qt.main_window import MainWindow
    win = MainWindow(os.path.join(ROOT, "test.mp4")); win.show()
    idle = lambda: not win.runner.running and not win.runner.queued
    t0 = time.time()
    while not (win.state.profile.base is not None and idle()) and time.time() - t0 < 300:
        app.processEvents(); time.sleep(0.01)
    win.tabs.setCurrentIndex(1)
    t0 = time.time()
    while not (idle() and win.viewer.filt is not None) and time.time() - t0 < 300:
        app.processEvents(); time.sleep(0.01)
    before = win.viewer.filt.copy()
    win.filter_page.panel.noiseLevelCheck.emit(True)
    t0 = time.time()
    while time.time() - t0 < 60:
        app.processEvents(); time.sleep(0.01)
        if idle() and time.time() - t0 > 3: break
    after = win.viewer.filt
    print("nlc_active:", getattr(win, "_nlc_active", None), "nlc_params:", getattr(win, "_nlc_params", "unset") is not None)
    print("preview changed:", None if after is None else bool((after != before).any()))
    print("exceptions:", len(errs)); [print(e) for e in errs]
    win.close()


if __name__ == "__main__":
    main()
