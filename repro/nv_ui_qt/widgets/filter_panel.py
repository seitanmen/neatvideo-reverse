"""Filter Settings side panel: collapsible sections bound to AppState parameter keys."""
from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (QFrame, QLabel, QPushButton, QSizePolicy, QToolButton, QVBoxLayout,
                               QWidget)

from nvfilter import V5_BANDS

from ..state import BANDS, CHANNELS, PARAM_SPECS
from .sliders import ParamSlider, make_control


class Section(QWidget):
    """Accordion section: a header button that shows / hides its content."""

    def __init__(self, title, expanded=True, parent=None):
        super().__init__(parent)
        v = QVBoxLayout(self)
        v.setContentsMargins(0, 0, 0, 0)
        v.setSpacing(0)
        self.btn = QToolButton()
        self.btn.setText(title)
        self.btn.setCheckable(True)
        self.btn.setChecked(expanded)
        self.btn.setToolButtonStyle(Qt.ToolButtonTextBesideIcon)
        self.btn.setArrowType(Qt.DownArrow if expanded else Qt.RightArrow)
        self.btn.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        self.btn.setStyleSheet("QToolButton{font-weight:bold; text-align:left; padding:4px}")
        self.btn.toggled.connect(self._toggle)
        self.body = QFrame()
        self.body.setFrameShape(QFrame.StyledPanel)
        self.lay = QVBoxLayout(self.body)
        self.lay.setContentsMargins(8, 4, 4, 6)
        self.body.setVisible(expanded)
        v.addWidget(self.btn)
        v.addWidget(self.body)

    def _toggle(self, on):
        self.btn.setArrowType(Qt.DownArrow if on else Qt.RightArrow)
        self.body.setVisible(on)

    def add(self, w):
        self.lay.addWidget(w)
        return w

    def heading(self, text):
        lab = QLabel(f"── {text} ──")
        lab.setStyleSheet("color:gray; margin-top:4px")
        self.lay.addWidget(lab)


class FilterPanel(QWidget):
    edited = Signal(str, object)
    editStarted = Signal()
    apply = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.controls = {}
        v = QVBoxLayout(self)
        v.setSpacing(2)

        s = self._section(v, "Temporal Filter")
        for k in ("t_enable", "t_radius", "t_amount", "t_quality"):
            self._add(s, k)
        s.heading("Components")
        for c, _ in CHANNELS:
            self._add(s, f"noise_{c}")

        s = self._section(v, "Spatial Filter")
        for k in ("s_enable", "s_quality"):
            self._add(s, k)
        s.heading("Noise Reduction Amount")
        for c, name in CHANNELS:
            s.add(QLabel(f"{name}:"))
            self._add(s, f"amt_{c}", label="  Overall")
            for b, bname in BANDS:
                self._add(s, f"bamt_{c}_{b}", label=f"  {bname}")
        s.heading("Noise Level")
        for c, name in CHANNELS:
            s.add(QLabel(f"{name}:"))
            for b, bname in BANDS:
                self._add(s, f"bnoise_{c}_{b}", label=f"  {bname}")

        s = self._section(v, "Sharpening — not available in the reproduction", False)
        self._add(s, "sharp_enable")
        for b, _ in BANDS:
            self._add(s, f"sharp_{b}")
        self._add(s, "sharp_prevent")

        s = self._section(v, "Jitter — not available in the reproduction", False)
        for k in ("jitter_enable", "jitter_thr"):
            self._add(s, k)
        s = self._section(v, "Dust and Scratches — not available in the reproduction", False)
        for k in ("dust_enable", "dust_mode", "dust_thr"):
            self._add(s, k)
        s = self._section(v, "Flicker — not available in the reproduction", False)
        for k in ("flicker_enable", "flicker_local", "flicker_full"):
            self._add(s, k)
        s = self._section(v, "Edge Smoothing — not available in the reproduction", False)
        for k in ("edge_low", "edge_mid", "edge_high"):
            self._add(s, k)

        s = self._section(v, "Output")
        self._add(s, "mix")
        self.b_apply = QPushButton("Apply (Process full video…)")
        self.b_apply.clicked.connect(self.apply)
        s.add(self.b_apply)

        s = self._section(v, "Advanced (repro)", False)
        lab = QLabel("v5 backend parameters without a genuine control")
        lab.setStyleSheet("color:gray")
        s.add(lab)
        for k in ("adv_threshold", "adv_spatial", "adv_noise_level", "adv_similarity"):
            self._add(s, k)
        v.addStretch(1)

    def _section(self, v, title, expanded=True):
        s = Section(title, expanded)
        v.addWidget(s)
        return s

    def _add(self, section, key, label=None):
        w = ParamSlider(key, label) if label is not None else make_control(key)
        w.edited.connect(self.edited)
        w.editStarted.connect(self.editStarted)
        self.controls[key] = w
        section.add(w)
        return w

    def set_band_availability(self, levels):
        """Disable band sliders the backend does not use for this profile: nv.spatial_v5 gives a
        plane with L levels the finest L band names for luma and the coarsest L for chroma (4:2:0
        chroma has 4 levels, so its finest band is 'Mid' and 'High' has no effect)."""
        for c, _ in CHANNELS:
            L = (levels or {}).get(c, len(V5_BANDS))
            used = set(V5_BANDS[:L] if c == "Y" else V5_BANDS[len(V5_BANDS) - L:])
            for b, _ in BANDS:
                for k in (f"bamt_{c}_{b}", f"bnoise_{c}_{b}"):
                    w = self.controls[k]
                    w.setEnabled(b in used)
                    w.setToolTip(PARAM_SPECS[k].tip if b in used else
                                 f"Not used with this profile: {c} has {L} frequency bands "
                                 "(chroma at half resolution in 4:2:0)")

    def set_values(self, params, keys=None):
        for k in (keys if keys is not None else params):
            if k in self.controls:
                self.controls[k].set_value(params[k])
