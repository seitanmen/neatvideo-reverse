"""Test wrapper (run with cwd = repro/): nv_ui_qt.main --ofx off-screen; 0.5 s after the window is shown
it performs OFX_ACTION: 'close' (window close), 'cancel' (Cancel button), 'apply_cancel' (Apply, then
Cancel), 'menu_apply_cancel' (Effect > Apply, then Effect > Cancel). UI code unchanged."""
import os
import sys

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
sys.path.insert(0, os.getcwd())

from PySide6.QtCore import QTimer                      # noqa: E402

from nv_ui_qt import main as M                         # noqa: E402
from nv_ui_qt.main_window import MainWindow            # noqa: E402

_show = MainWindow.show
ACT = os.environ.get("OFX_ACTION", "close")


def show(self):
    _show(self)

    def act():
        if ACT == "close":
            self.close()
        elif ACT == "cancel":
            self.b_cancel.click()
        elif ACT == "apply_cancel":
            self.b_apply.click()
            self.b_cancel.click()
        elif ACT == "menu_apply_cancel":
            self.actions["apply"].trigger()
            self.actions["cancel"].trigger()
    QTimer.singleShot(500, act)


MainWindow.show = show
if __name__ == "__main__":
    sys.exit(M.main([sys.argv[0]] + sys.argv[1:]))
