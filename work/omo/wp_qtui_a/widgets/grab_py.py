"""Python side of the widget comparison: repro/nv_ui_qt widgets offscreen in fixed states -> shots/py_*.png.
Writes shots/profile.json (a ProfileState of test.mp4 n=300, Python path, + AFT) used by both sides.

    QT_QPA_PLATFORM=offscreen python3 sandbox/omo/wp_qtui_a/widgets/grab_py.py
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "repro"))
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
from PySide6.QtGui import QImage  # noqa: E402
from PySide6.QtWidgets import QApplication  # noqa: E402

app = QApplication([])
import numpy as np  # noqa: E402
from nv_ui_qt import backend  # noqa: E402
from nv_ui_qt.state import ProfileState, effective_profile  # noqa: E402
from nv_ui_qt.widgets.curve_editor import CurveEditor  # noqa: E402
from nv_ui_qt.widgets.noise_graph import NoiseGraph  # noqa: E402
from nv_ui_qt.widgets.profile_panel import ProfilePanel  # noqa: E402
from nv_ui_qt.widgets.timeline import Timeline  # noqa: E402

SHOTS = os.path.join(HERE, "shots")
os.makedirs(SHOTS, exist_ok=True)
pj = os.path.join(SHOTS, "profile.json")
if not os.path.exists(pj):
    z = np.load(os.path.join(ROOT, "native", "tmp_bridge", "frames_t_298_309.npz"))
    fr = [tuple(z[f"{c}{n}"] for c in "ybr") for n in range(298, 309)]
    base = backend.build_profile(fr, 2, False, "bt601", "auto")
    base = backend.run_aft(base, fr[2], False, "bt601")
    ps = ProfileState(base=base, device="Test camera")
    ps.factors[0][3] = 1.6
    ps.factors[2][5] = 0.55
    ps.pinned[0].add(3)
    ps.pinned[2].add(5)
    ps.tune = {"all": 1.2, "Cr": 0.8}
    with open(pj, "w") as fh:
        json.dump(ps.to_json(), fh, indent=1)
ps = ProfileState.from_json(json.load(open(pj)))
eff = effective_profile(ps.base, ps.factors, ps.tune)


def grab(w, name, size):
    w.resize(*size)
    w.show()
    app.processEvents()
    w.grab().toImage().convertToFormat(QImage.Format_RGB888).save(os.path.join(SHOTS, f"py_{name}.png"))
    w.hide()


p = ProfilePanel()
grab(p, "panel_none", (350, 900))
p = ProfilePanel()
p.show_profile(ps, eff, (640, 360))
p.set_name("Noise profile (640x360p)", True)
p.set_quality(43)
p.set_completeness(70)
p.set_levels(3.25, 2.95)
grab(p, "panel_adv", (350, 900))
p.set_mode(False)
grab(p, "panel_beg", (350, 900))
p.set_mode(True)
p.tune_header.set_expanded(True)
p.freq_header.set_expanded(True)
p.set_check_active(True)
grab(p, "panel_check", (350, 1100))
c = CurveEditor()
c.set_factors(ps.factors, ps.pinned, 4)
c.set_target("Cb")
grab(c, "curve", (330, 220))
t = Timeline()
t.thumbs.provider = lambda i: (lambda q: (q.fill(0xff000000 | (i * 2) * 0x010101), q)[1])(QImage(64, 36, QImage.Format_RGB32))
t.set_count(100, 25.0)
t.set_frame(50)
t.set_radius(2)
grab(t, "timeline", (800, 60))
g = NoiseGraph()
g.set_profile(eff, ps.base)
grab(g, "graph", (520, 260))
print("ok")
