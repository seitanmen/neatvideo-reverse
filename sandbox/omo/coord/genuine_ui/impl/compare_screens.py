"""§5.2: offscreen grabs of the repro UI on test_v02 frame 0 next to the genuine screenshots (shots/).

    cd repro && QT_QPA_PLATFORM=offscreen python3 ../sandbox/omo/coord/genuine_ui/impl/compare_screens.py

Writes impl/screens/ours_*.png (1800 x 1095 grabs) and impl/cmp_*.png (genuine window | repro, 0.6x).
The genuine runs on a 3840 x 2160 timeline (test_v02 up-scaled 2x by Resolve), so its selection
512 x 216 covers twice the image of the repro's 512 x 216 on the 1920 x 1080 source.
"""
import os
import sys
import time

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
sys.path.insert(0, os.getcwd())                 # run from repro/
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", "..", "..", ".."))
SHOTS = os.path.join(HERE, "..", "shots")
OUT = os.path.join(HERE, "screens")


def genuine_window(name):
    from PIL import Image
    a = Image.open(os.path.join(SHOTS, name)).convert("RGB")
    a = a.crop((0, 60, 3600, 2250)).resize((1800, 1095), Image.LANCZOS)    # plug-in window (0, 30) pt
    return a


def side_by_side(gname, ours, dst):
    from PIL import Image
    g = genuine_window(gname)
    o = Image.open(ours).convert("RGB").resize((1800, 1095), Image.LANCZOS)
    c = Image.new("RGB", (3600, 1095))
    c.paste(g, (0, 0))
    c.paste(o, (1800, 0))
    c.resize((2160, 657), Image.LANCZOS).save(dst)


def main():
    os.makedirs(OUT, exist_ok=True)
    os.environ["NV_UI_QT_SETTINGS"] = os.path.join(HERE, "screens.ini")
    if os.path.exists(os.environ["NV_UI_QT_SETTINGS"]):
        os.remove(os.environ["NV_UI_QT_SETTINGS"])
    from PySide6.QtWidgets import QApplication
    app = QApplication([sys.argv[0]])
    from nv_ui_qt.main_window import MainWindow, settings
    s = settings()
    s.setValue("prefs/auto_profile", False)
    s.setValue("prefs/mem_gb", 0.1)             # ~34 frames of test_v02 in memory
    s.sync()
    win = MainWindow(os.path.join(ROOT, "test_v02.mp4"))
    win.resize(1800, 1095)
    win.show()
    idle = lambda: not win.runner.running and not win.runner.queued

    def wait(cond, t=900):
        t0 = time.time()
        while not cond() and time.time() - t0 < t:
            app.processEvents()
            time.sleep(0.01)
        app.processEvents()
        return cond()

    def shot(name):
        t0 = time.time()
        wait(win.viewer.ready)
        dt = time.time() - t0
        for _ in range(3):
            app.processEvents()
        p = os.path.join(OUT, f"ours_{name}.png")
        win.grab().save(p)
        print(f"{name}: panes ready {dt:.2f} s", flush=True)
        return p

    wait(lambda: win.frames is not None and idle())
    win.timeline.set_frame(0, emit=True)
    pairs = []
    win.viewer.set_mode("Y Freq")
    pairs.append(("m_yfreq.png", shot("prep_YFreq_noprofile"), "prep_YFreq"))
    win.viewer.set_mode("YCrCb +")
    pairs.append(("main.png", shot("prep_YCrCbplus_noprofile"), "prep_YCrCbplus_noprofile"))
    win.on_area(0, 704, 216, 512)
    pairs.append(("sel1.png", shot("prep_selection"), "prep_selection"))
    t0 = time.time()
    win.build_profile(False)
    wait(lambda: win.state.profile.base is not None and idle() and win.state.profile.base.get("aft_applied"))
    print(f"Build Profile + AFT: {time.time() - t0:.1f} s, area {win.state.profile.base['area']}", flush=True)
    win.timeline.set_frame(0, emit=True)
    pairs.append(("g_yccplus.png", shot("prep_YCrCbplus"), "prep_YCrCbplus"))
    win.viewer.set_mode("RGB")
    pairs.append(("bp_after.png", shot("prep_RGB_after"), "prep_RGB"))
    win._viewer_click()
    win.viewer.set_mode("YCrCb")
    pairs.append(("g_ycc2.png", shot("prep_YCrCb"), "prep_YCrCb"))
    # switching modes on the 1080p frame (computed once per image, then cached)
    for m in ("YCrCb +", "Y Freq", "Cr Freq", "RGB", "YCrCb +"):
        t0 = time.time()
        win.viewer.set_mode(m)
        wait(win.viewer.ready)
        win.viewer.grab()
        print(f"switch to {m}: {time.time() - t0:.2f} s", flush=True)
    win.tabs.setCurrentIndex(1)
    wait(lambda: idle() and win.viewer.showing_filtered())
    fp = win.filter_page.panel
    for i, (g, n) in enumerate((("adj1.png", "adj_temporal"), ("adj_sp.png", "adj_spatial"),
                                ("adj_gen.png", "adj_general"))):
        fp.tabs.setCurrentIndex(i)
        pairs.append((g, shot(n), n))
    win.close()
    app.processEvents()
    for g, o, n in pairs:
        side_by_side(g, o, os.path.join(HERE, f"cmp_{n}.png"))
    print("wrote", len(pairs), "comparisons")


if __name__ == "__main__":
    main()
