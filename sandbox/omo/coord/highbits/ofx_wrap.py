"""Test wrapper: runs nv_ui_qt.main with the given arguments off-screen; the main window closes itself
0.5 s after it is shown (test only; the UI code is unchanged). Run with cwd = repro/."""
import os
import sys

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
sys.path.insert(0, os.getcwd())

from PySide6.QtCore import QTimer                      # noqa: E402

from nv_ui_qt import main as M                         # noqa: E402
from nv_ui_qt.main_window import MainWindow            # noqa: E402

_show = MainWindow.show


def show(self):
    _show(self)
    QTimer.singleShot(500, self.close)


MainWindow.show = show
sys.exit(M.main([sys.argv[0]] + sys.argv[1:]))
