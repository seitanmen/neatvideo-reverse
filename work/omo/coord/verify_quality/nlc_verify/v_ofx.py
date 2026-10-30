"""OFX mode: real nv_ui_qt.main --ofx path with frames on stdin; Auto Profile, t_noise -60%, NLC Begin/Finish,
Apply, close. Compares stdout frame to backend.process_frame. stdout of the UI goes to OUT file."""
import json, os, sys, time
os.environ["QT_QPA_PLATFORM"] = "offscreen"
REPRO = "/Users/seitan/Desktop/work/reverse/repro"; sys.path.insert(0, REPRO); os.chdir(REPRO)
OUTD = os.path.dirname(os.path.abspath(__file__))
def log(**kw): print("DRV " + json.dumps(kw, default=str), file=sys.stderr, flush=True)
def main():
    os.environ["NV_UI_QT_SETTINGS"] = os.path.join(OUTD, "settings_ofx.ini")
    import numpy as np
    from PySide6.QtCore import QTimer, Qt
    from PySide6.QtTest import QTest
    from nv_ui_qt import backend, ofx_bridge
    from nv_ui_qt.main_window import MainWindow
    emits = []
    orig = ofx_bridge.OfxFrameSource.emit_frame
    def emit_frame(self, planes, index=None):
        emits.append((index, [np.asarray(p).copy() for p in planes])); return orig(self, planes, index)
    ofx_bridge.OfxFrameSource.emit_frame = emit_frame
    orig_show = MainWindow.show
    def show(win):
        orig_show(win); QTimer.singleShot(0, lambda: drive(win))
    MainWindow.show = show
    def drive(win):
        from PySide6.QtWidgets import QApplication
        app = QApplication.instance(); st = win.state
        idle = lambda: not win.runner.running and not win.runner.queued
        def wait(c, to=300):
            t0 = time.time()
            while not c():
                app.processEvents(); time.sleep(0.01)
                if time.time() - t0 > to: return False
            app.processEvents(); return True
        win.tabs.setCurrentIndex(1); app.processEvents()
        QTest.mouseClick(win.auto_btn, Qt.LeftButton)
        log(ev="profile", ok=wait(lambda: st.profile.base is not None and idle()))
        st.set_param("t_noise", 40.0)
        win.filter_page.panel.nl_check.button.click()
        ok = wait(lambda: idle() and not win.nlc_timer.isActive() and win.viewer.showing_nlc())
        log(ev="nlc", ok=ok, nlc=win.nlc, btn=win.fsc_btn.text(), status=win.msg.text())
        win.filter_page.panel.nl_check.button.click()
        log(ev="finish", nlc=win.nlc)
        wait(lambda: idle())
        QTest.mouseClick(win.b_apply, Qt.LeftButton); wait(idle)
        t = win.timeline.frame(); P = st.backend(); prof = st.effective_profile()
        idx, planes = win.engine.window(t, int(P["radius"]), None)
        ref = backend.process_frame(planes, idx.index(t), prof, P)
        ref1 = backend.process_frame(planes, idx.index(t), prof, dict(P, t_noise=1.0))
        same = bool(emits) and all(np.array_equal(a, b) for a, b in zip(emits[-1][1], ref))
        diff1 = bool(emits) and not all(np.array_equal(a, b) for a, b in zip(emits[-1][1], ref1))
        log(ev="apply", emitted=len(emits), t_noise=P.get("t_noise"), equals_process_frame=same, differs_from_100=diff1)
        win.close()
    from nv_ui_qt import main as m
    rc = m.main(["nv_ui_qt.main", "--ofx"] + sys.argv[1:])
    log(ev="exit", rc=rc, emits=[e[0] for e in emits])
    return rc
if __name__ == "__main__":
    sys.exit(main() or 0)
