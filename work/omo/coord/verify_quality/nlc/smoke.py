"""Quick off-screen check of Noise Level Check (no window shown).

    cd repro && QT_QPA_PLATFORM=offscreen NV_UI_QT_SETTINGS=<ini> python3 ../sandbox/omo/coord/verify_quality/nlc/smoke.py VIDEO
"""
import os
import sys
import time

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
sys.path.insert(0, os.getcwd())

import numpy as np
from PySide6.QtWidgets import QApplication


def main(video):
    app = QApplication([sys.argv[0]])
    from nv_ui_qt import backend
    from nv_ui_qt.main_window import MainWindow
    win = MainWindow(video)
    win.show()
    st = win.state
    idle = lambda: not win.runner.running and not win.runner.queued

    def wait(c, to=300):
        t0 = time.time()
        while not c():
            app.processEvents()
            time.sleep(0.01)
            if time.time() - t0 > to:
                return False
        app.processEvents()
        return True

    print("profile", wait(lambda: st.profile.base is not None and idle()))
    win.tabs.setCurrentIndex(1)
    wait(idle)
    t0 = time.time()
    win.filter_page.panel.nl_check.button.click()
    ok = wait(lambda: win.viewer.showing_nlc() and idle())
    print("nlc on", ok, win.nlc, win.viewer._source(), win.viewer.mode(), win.fsc_btn.text(), win.fsc_btn.isChecked(),
          f"{time.time() - t0:.2f}s", win.msg.text())
    t = win.timeline.frame()
    P = st.backend()
    m = win.engine.mask(t, st.effective_profile(), win.prof_token, P)
    print("mask mean", float(m.mean()), "median", float(np.median(m)), "p10/p90", np.percentile(m, [10, 90]))
    for v in (40.0, 0.0):
        st.set_param("t_noise", v)
        wait(lambda: idle() and not win.nlc_timer.isActive())
        P = st.backend()
        m = win.engine.mask(t, st.effective_profile(), win.prof_token, P)
        print("t_noise", v, "mask mean", float(m.mean()), "max", float(m.max()))
    win.filter_page.panel.nl_check.button.click()
    app.processEvents()
    print("nlc off", win.nlc, win.viewer._source(), win.viewer.mode(), win.fsc_btn.text())
    win.close()


if __name__ == "__main__":
    main(sys.argv[1])
