import sys
from PySide6.QtWidgets import QApplication, QWidget, QLabel
from nv_ui_qt.main_window import MainWindow
def main():
    app = QApplication(sys.argv[:1]); win = MainWindow(sys.argv[1]); win.show()
    for adv in (False, True):
        win.set_advanced(adv); app.processEvents()
        print("adv", adv)
        for w in app.allWidgets():
            if w.parent() is None and w is not win:
                print("  parentless", type(w).__name__, repr(getattr(w,'text',lambda:'')())[:60], "vis", w.isVisible(), "win", w.isWindow(), w.windowFlags().value & 0xff)
        # non-layout children whose parent has layout, with visible state
        for w in win.findChildren(QWidget):
            p = w.parentWidget()
            if p and p.layout() and p.layout().indexOf(w) < 0 and w.isVisible() and w.isWindow():
                print("  VISIBLE-WINDOW-CHILD", type(w).__name__)
    print("done")
if __name__=="__main__": main()
