"""Entry point: python3 -m nv_ui_qt.main [video]   (run from repro/)."""
import sys

from PySide6.QtWidgets import QApplication

from nv_ui_qt.main_window import MainWindow


def main(argv=None):
    argv = sys.argv if argv is None else argv
    app = QApplication(argv)
    app.setApplicationName("nv_ui_qt")
    win = MainWindow(argv[1] if len(argv) > 1 else None)
    win.show()
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
