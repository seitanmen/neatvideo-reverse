"""Quick offscreen smoke run: open a video, wait for the profile, grab the window per viewer mode.
    cd repro && QT_QPA_PLATFORM=offscreen python3 ../sandbox/omo/coord/genuine_ui/impl/smoke.py VIDEO OUTDIR
"""
import os
import sys
import time

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
sys.path.insert(0, os.getcwd())                 # run from repro/


def main(video, outdir):
    os.makedirs(outdir, exist_ok=True)
    os.environ["NV_UI_QT_SETTINGS"] = os.path.join(outdir, "smoke.ini")
    from PySide6.QtWidgets import QApplication
    app = QApplication([sys.argv[0]])
    from nv_ui_qt.main_window import MainWindow, settings
    s = settings()
    s.setValue("prefs/auto_fine_tune", False)
    s.sync()
    win = MainWindow(video)
    win.resize(1800, 1095)
    win.show()
    idle = lambda: not win.runner.running and not win.runner.queued

    def wait(cond, t=600):
        t0 = time.time()
        while not cond() and time.time() - t0 < t:
            app.processEvents()
            time.sleep(0.01)
        app.processEvents()
        return cond()

    print("profile", wait(lambda: win.state.profile.base is not None and idle()))
    win.timeline.set_frame(0, emit=True)
    for m in ("YCrCb +", "YCrCb", "RGB", "Y Freq", "Normal"):
        win.viewer.set_mode(m)
        t0 = time.time()
        wait(win.viewer.ready)
        win.viewer.grab()
        print(m, f"{time.time() - t0:.2f}s")
        win.grab().save(os.path.join(outdir, f"smoke_{m.replace(' ', '').replace('+', 'plus')}.png"))
    win.close()


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
