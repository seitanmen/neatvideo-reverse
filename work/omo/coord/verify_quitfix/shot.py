import sys, os
from PySide6.QtWidgets import QApplication, QScrollArea
from nv_ui_qt.main_window import MainWindow
V = os.path.dirname(os.path.abspath(__file__))
def main():
    app = QApplication(sys.argv[:1]); win = MainWindow(sys.argv[1]); win.resize(1400, 900); win.show(); app.processEvents()
    win.tabs.setCurrentIndex(1)
    fp = win.filter_page.panel
    got = []
    fp.noiseLevelCheck.connect(got.append)
    nlc_calls = []
    orig = win.noise_level_check
    for adv in (False, True):
        win.set_advanced(adv); app.processEvents()
        fp.tabs.setCurrentIndex(0); app.processEvents()
        # find NL row
        from nv_ui_qt.widgets.rows import BeginRow, Header
        nl = [b for b in fp.findChildren(BeginRow) if "Noise Level Check" in b.title.text()]
        print("adv", adv, "n NL rows", len(nl))
        b = nl[0]
        print("  parent", type(b.parentWidget()).__name__, "isWindow", b.isWindow(), "visibleTo(win)", b.isVisibleTo(win), "isVisible", b.isVisible())
        # ancestor header
        p = b.parentWidget(); hdr = None
        while p is not None:
            if isinstance(p, Header): hdr = p; break
            p = p.parentWidget()
        print("  header", hdr.title_text if hdr else None, "idx in body layout", hdr.lay.indexOf(b) if hdr else None, "next row", type(hdr.lay.itemAt(1).widget()).__name__)
        if adv:
            hdr.set_expanded(True); app.processEvents()
            print("  expanded; isVisible", b.isVisible())
            b.button.click(); app.processEvents()
            print("  after click got", got, "nlc_active", getattr(win, "_nlc_active", None))
            b.button.click(); app.processEvents()
            print("  after 2nd click got", got, "nlc_active", getattr(win, "_nlc_active", None))
            win.grab().save(V + "/shot_window_adv_temporal.png")
            fp.tabs.grab().save(V + "/shot_tabs_adv_temporal.png")
    print("done")
if __name__ == "__main__": main()
