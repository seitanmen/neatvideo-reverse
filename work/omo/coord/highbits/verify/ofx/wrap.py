"""Verifier's wrapper: run nv_ui_qt.main off-screen; close the window 0.8 s after show (cwd = repro/)."""
import os, sys
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen"); sys.path.insert(0, os.getcwd())
from PySide6.QtCore import QTimer
from nv_ui_qt import main as M
from nv_ui_qt.main_window import MainWindow
_show = MainWindow.show
def show(self):
    _show(self); QTimer.singleShot(800, self.close)
MainWindow.show = show
sys.exit(M.main([sys.argv[0]] + sys.argv[1:]))
