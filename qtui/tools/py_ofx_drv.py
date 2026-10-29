"""Test driver of the Python UI's OFX host mode (the counterpart of qtui/tests/ofx_drv.cpp, qtui/tools/compare_ofx.py):
runs nv_ui_qt.main with the given arguments (--ofx W H ...) off-screen and, once the window is up, the steps of
$NV_OFX_SCRIPT (";"-separated); after every step it waits until the UI is idle and prints "MSG: <status text>" to
stderr.  The UI code is unchanged (MainWindow.show is wrapped to start the script).  Run with cwd = repro/."""
import os
import sys

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
sys.path.insert(0, os.getcwd())
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.dont_write_bytecode = True

from PySide6.QtCore import QTimer                      # noqa: E402

from nv_ui_qt import main as M                         # noqa: E402
from nv_ui_qt.main_window import MainWindow            # noqa: E402

_show = MainWindow.show
STEPS = [s for s in os.environ.get("NV_OFX_SCRIPT", "").split(";") if s.strip()]


def idle(win):
    return (not win.runner.running and not win.runner.queued and not win.timer.isActive()
            and not win.sel_timer.isActive() and not win.pc_timer.isActive() and not win.nlc_timer.isActive())


def run_steps(win):
    def tick():
        if not idle(win):
            QTimer.singleShot(20, tick)
            return
        if not STEPS:
            return
        w = STEPS.pop(0).split()
        c = w[0] if w else ""
        if c == "apply":
            win.b_apply.click()
        elif c == "cancel":
            win.b_cancel.click()
        elif c == "close":
            win.close()
        elif c == "auto":
            win.auto_profile(False)
        elif c == "frame":
            win.timeline.set_frame(int(w[1]), emit=True)
        elif c == "tab":
            win.tabs.setCurrentIndex(int(w[1]))
        elif c == "set":
            win.state.set_param(w[1], float(w[2]))
        elif c == "setb":
            win.state.set_param(w[1], bool(int(w[2])))
        elif c == "tune":
            win.state.set_tune(w[1], float(w[2]))
        elif c == "dump":
            import json
            from py_dump_n3c import extras, menus, state_with
            json.dump(menus(win), open(os.path.join(w[1], "menus_ofx.json"), "w"), indent=1)
            ex = dict(extras(win), apply_tip=win.b_apply.toolTip(), load_enabled=win.load_btn.isEnabled(),
                      generic_enabled=win.generic_btn.isEnabled())
            open(os.path.join(w[1], "state_ofx.json"), "w").write(state_with(win, **ex))
            win.grab().save(os.path.join(w[1], "win_ofx.png"))
        if c in ("close", "cancel"):
            return

        def after():
            if not idle(win):
                QTimer.singleShot(20, after)
                return
            print(f"MSG: {win.msg.text()}", file=sys.stderr, flush=True)
            tick()

        QTimer.singleShot(20, after)

    tick()


def show(self):
    _show(self)
    QTimer.singleShot(0, lambda: run_steps(self))


MainWindow.show = show
sys.exit(M.main([sys.argv[0]] + sys.argv[1:]))
