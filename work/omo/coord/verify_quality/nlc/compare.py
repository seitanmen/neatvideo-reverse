"""Off-screen window grabs of Noise Level Check in the genuine screens' composition (test_v02, Adjust page,
Advanced Mode, Temporal tab, 1800 x 1125 = the genuine 3600 x 2250 Retina shots at half size) and
side-by-side images genuine | repro. Writes only into this folder.

    cd repro && QT_QPA_PLATFORM=offscreen NV_UI_QT_SETTINGS=<this folder>/settings_cmp.ini \
        python3 ../sandbox/omo/coord/verify_quality/nlc/compare.py ../test_v02.mp4
"""
import json
import os
import sys
import time

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
sys.path.insert(0, os.getcwd())

import numpy as np
from PIL import Image
from PySide6.QtWidgets import QApplication

HERE = os.path.dirname(os.path.abspath(__file__))
SHOTS = os.path.join(HERE, "..", "shots")
# (genuine shot, Temporal Noise Level in %, output name)
CASES = [("g_adjust_normal.png", None, "adjust_normal"), ("g_nlc1.png", 100.0, "nlc_0"),
         ("g_nlc_mid_1636.png", 40.0, "nlc_m60"), ("g_nlc_low.png", 0.0, "nlc_m100")]


def main(video):
    s_path = os.environ["NV_UI_QT_SETTINGS"]
    app = QApplication([sys.argv[0]])
    from nv_ui_qt.main_window import MainWindow, settings
    s = settings()
    s.setValue("prefs/mem_gb", 0.15)            # about 48 frames of 1080p 4:2:0 (memory cap of the preview)
    s.setValue("ui/advanced", True)
    s.sync()
    win = MainWindow(video)
    win.resize(1800, 1125)
    win.show()
    st = win.state
    idle = lambda: not win.runner.running and not win.runner.queued

    def wait(c, to=600):
        t0 = time.time()
        while not c():
            app.processEvents()
            time.sleep(0.01)
            if time.time() - t0 > to:
                return False
        for _ in range(5):
            app.processEvents()
        return True

    assert wait(lambda: st.profile.base is not None and idle()), "no profile"
    win.tabs.setCurrentIndex(1)
    assert wait(lambda: idle() and win.viewer.showing_filtered()), "no preview"
    stats = {"video": video, "frame": win.timeline.frame(), "size": list(win.frames[0][0].shape)}
    for shot, tn, name in CASES:
        if tn is not None:
            if not win.nlc:
                win.filter_page.panel.nl_check.button.click()
            st.set_param("t_noise", tn)
            assert wait(lambda: idle() and not win.nlc_timer.isActive() and win.viewer.showing_nlc()
                        and win._nlc_shown == win._nlc_key()), name
            m = win.engine.mask(win.timeline.frame(), st.effective_profile(), win.prof_token, st.backend())
            stats[name] = {"t_noise_pct": tn, "display": f"{tn - 100:+.0f}%", "mean": float(m.mean()),
                           "median": float(np.median(m)), "p10": float(np.percentile(m, 10)),
                           "p90": float(np.percentile(m, 90))}
        path = os.path.join(HERE, f"q_{name}.png")
        win.grab().save(path)
        ours = Image.open(path).convert("RGB")
        gen = Image.open(os.path.join(SHOTS, shot)).convert("RGB").resize(ours.size, Image.LANCZOS)
        both = Image.new("RGB", (ours.width * 2 + 8, ours.height), (255, 255, 255))
        both.paste(gen, (0, 0))
        both.paste(ours, (ours.width + 8, 0))
        both.save(os.path.join(HERE, f"cmp_{name}.png"))
    win.filter_page.panel.nl_check.button.click()
    stats["finished"] = {"nlc": win.nlc, "button": win.fsc_btn.text(), "mode": win.viewer.mode()}
    json.dump(stats, open(os.path.join(HERE, "compare.json"), "w"), indent=1)
    print(json.dumps(stats, indent=1))
    win.close()


if __name__ == "__main__":
    main(sys.argv[1])
