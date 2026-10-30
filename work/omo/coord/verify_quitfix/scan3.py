import sys
from PySide6.QtWidgets import QApplication
from nv_ui_qt.main_window import MainWindow
from collections import Counter
def main():
    app = QApplication(sys.argv[:1])
    print("baseline parentless:", [w for w in app.allWidgets() if w.parent() is None])
    win = MainWindow(sys.argv[1]); win.show(); app.processEvents()
    c = Counter((w.metaObject().className(), w.objectName(), w.windowFlags().value & 0xff) for w in app.allWidgets() if w.parent() is None and w is not win)
    print(c)
    l=[w for w in app.allWidgets() if w.parent() is None and w is not win]
    print([ (w.sizeHint().width(), w.styleSheet()[:30], w.windowOpacity()) for w in l[:3]])
if __name__=="__main__": main()
