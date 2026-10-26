import sys, os
from PySide6.QtWidgets import QApplication, QWidget, QMenu
from PySide6.QtCore import Qt
from nv_ui_qt.main_window import MainWindow

def main():
    app = QApplication(sys.argv[:1])
    win = MainWindow(sys.argv[1])
    win.show(); app.processEvents()
    def scan(tag):
        app.processEvents()
        allw = win.findChildren(QWidget)
        # 1. descendants whose isWindow() (own top-level, incl. menus/dialogs)
        wins = [w for w in allw if w.isWindow()]
        print(tag, "descendant isWindow:", [(type(w).__name__, w.isVisible()) for w in wins])
        # 2. all app top-level widgets other than win
        tl = [w for w in app.topLevelWidgets() if w is not win]
        print(tag, "app.topLevelWidgets other:", [(type(w).__name__, w.objectName(), w.isVisible(), w.parent() is not None) for w in tl])
        # 3. parentless widgets that are visible
        vis = [w for w in app.allWidgets() if w.parent() is None and w is not win and w.isVisible()]
        print(tag, "visible parentless:", [(type(w).__name__) for w in vis])
        # 4. widgets not in any layout but visible-intended: count children w/o layout containing them
        orphan=[]
        for w in allw:
            p=w.parentWidget()
            if p is None: continue
            lay=p.layout()
            if lay is None: continue
            if lay.indexOf(w)<0 and not isinstance(w,(QMenu,)) :
                # may be parented but not in layout (e.g. scroll viewport, internal) - list class
                orphan.append(type(w).__name__)
        from collections import Counter
        print(tag, "parent-has-layout-but-widget-not-in-it:", Counter(orphan))
    # exercise all pages / tabs
    for adv in (False, True):
        win.set_advanced(adv); app.processEvents()
        # iterate through QTabWidgets and QStackedWidgets
        from PySide6.QtWidgets import QTabWidget, QStackedWidget, QToolBox
        for tw in win.findChildren(QTabWidget):
            for i in range(tw.count()):
                tw.setCurrentIndex(i); app.processEvents()
                scan(f"adv={adv} tab={tw.tabText(i)}")
        for sw in win.findChildren(QStackedWidget):
            for i in range(sw.count()):
                sw.setCurrentIndex(i); app.processEvents()
                scan(f"adv={adv} stack{i}")
        scan(f"adv={adv} final")
if __name__=="__main__": main()
