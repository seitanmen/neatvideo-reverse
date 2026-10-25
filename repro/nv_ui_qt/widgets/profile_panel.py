"""Device Noise Profile side panel (Prepare Noise Profile page) in the genuine order (binary
FUN_000d99d4 / FUN_000e0b18): profile buttons, name, device name, info; then statistics (Quality /
Match, Advanced: Main Sample / Selection / Noise Levels), the equalizer (Advanced: Auto Fine-Tune,
Manual Fine-Tune, Auto Complete, component switch, reset) and Profile Check and Tuning (Profile
Check, Noise Level: Overall, Advanced: Luminance -> Cr -> Cb, Frequencies). Profile Noise Level is a
repro multiplier of the profile curve (state.effective_profile); the profile curve has no bands, so
the Frequencies rows are shown disabled."""
from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (QComboBox, QFrame, QGridLayout, QHBoxLayout, QLabel, QLineEdit, QProgressBar,
                               QPushButton, QScrollArea, QSizePolicy, QToolButton, QVBoxLayout, QWidget)

import nvfilter as nv

from ..state import TUNE_MAX, TUNE_MIN, UI_BANDS, UI_CHANNELS, Spec
from .curve_editor import CurveEditor
from .noise_graph import NoiseGraph
from .rows import BeginRow, Header, IconBar, Row, guide_labels, mode_text, panel_title

TUNE_TIP = ("Profile Noise Level (repro): multiplies the noise profile curve{} - higher values -> more "
            "image elements are treated as noise. Saved with the profile.")


def tune_spec(label, c=None):
    return Spec("float", 1.0, TUNE_MIN, TUNE_MAX, label, TUNE_TIP.format(f" of {c}" if c else ""))


class ProfilePanel(QWidget):
    autoFineTune = Signal()
    autoComplete = Signal()
    resetFineTune = Signal()
    profileCheck = Signal()
    editDevice = Signal(str)
    tuneEdited = Signal(str, float)     # "all" / "Y" / "Cb" / "Cr", multiplier
    editStarted = Signal()
    linkActivated = Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._modes = []
        v = QVBoxLayout(self)
        v.setContentsMargins(4, 4, 4, 4)
        v.addWidget(panel_title("Device Noise Profile"))
        self.icons = IconBar("noise profile")
        v.addWidget(self.icons)
        self.name = QComboBox()
        v.addWidget(self.name)
        v.addWidget(QLabel("Device Name and Mode:"))
        self.device = QLineEdit("Unnamed device")
        self.device.setToolTip("Optional device name (e.g., 'Nikon D90'); saved with the profile")
        self.device.editingFinished.connect(lambda: self.editDevice.emit(self.device.text()))
        v.addWidget(self.device)
        self.info = QLabel()
        self.info.setFrameShape(QFrame.StyledPanel)
        self.info.setStyleSheet("background:white; padding:3px")
        self.info.setTextFormat(Qt.RichText)
        v.addWidget(self.info)

        sc = QScrollArea()
        sc.setWidgetResizable(True)
        sc.setFrameShape(QFrame.NoFrame)
        sc.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        body = QWidget()
        bl = QVBoxLayout(body)
        bl.setContentsMargins(0, 4, 0, 0)
        bl.setSpacing(2)
        sc.setWidget(body)
        v.addWidget(sc, 1)

        # statistics
        st = QWidget()
        g = QGridLayout(st)
        g.setContentsMargins(2, 0, 2, 0)
        g.addWidget(QLabel("Quality"), 0, 0)
        self.quality = QProgressBar()
        self.quality.setRange(0, 100)
        self.quality.setValue(0)
        self.quality.setFormat("n/a")
        self.quality.setEnabled(False)
        self.quality.setMaximumWidth(80)
        self.quality.setToolTip("Overall Profile Quality: the genuine quality measure is not decoded; not shown")
        g.addWidget(self.quality, 0, 1)
        g.addWidget(QLabel("Match"), 0, 2)
        self.match = QLabel("—")
        self.match.setToolTip("FindFlatArea status of the profile's sample area (genuine status codes; repro "
                              "shows the status instead of the genuine Match percentage)")
        g.addWidget(self.match, 0, 3, 1, 2)
        self.l_main = QLabel("Main Sample")
        self.main_val = QLabel("—")
        self.l_sel = QLabel("Selection")
        self.sel_val = QLabel("—")
        for w in (self.main_val, self.sel_val):
            w.setToolTip("Noise level (repro estimate): robust luma noise sigma median(|HH1|)/0.6745 of the "
                         "area (Haar diagonal band, the estimator of Most Noisy Frame)")
        g.addWidget(self.l_main, 1, 0)
        g.addWidget(self.main_val, 1, 1)
        g.addWidget(self.l_sel, 1, 2)
        g.addWidget(self.sel_val, 1, 3)
        self.levels = QLabel("Noise Levels: —")
        self.levels.setToolTip("Band ratios of the profile's Y noise (fine -> coarse, repro), instead of the "
                               "genuine per-band accuracy histogram")
        self.levels.setWordWrap(True)
        self.match.setWordWrap(True)
        g.addWidget(self.levels, 2, 0, 1, 5)
        bl.addWidget(st)
        for w in (self.l_main, self.main_val, self.l_sel, self.sel_val, self.levels):
            self._reg(w, "adv")

        # equalizer
        eq = QFrame()
        eq.setFrameShape(QFrame.StyledPanel)
        el = QVBoxLayout(eq)
        el.setContentsMargins(2, 2, 2, 2)
        self.curve = CurveEditor()
        el.addWidget(self.curve)
        row = QHBoxLayout()
        self.b_aft = QPushButton("Auto Fine-Tune")
        self.b_aft.setToolTip("nv_aft_v5.apply_aft: static port of the genuine AutoFineTune (its factors were "
                              "not compared with the genuine); takes several seconds")
        self.b_manual = QPushButton("Manual (n/a)")
        self.b_manual.setEnabled(False)
        self.b_manual.setToolTip("Not available in the reproduction (fine-tuning from the selected area); "
                                 "drag the knots of the graph instead")
        self.b_complete = QPushButton("Auto Complete")
        self.b_complete.setToolTip("Interpolate the bins you did not edit between the edited ones (repro)")
        for b in (self.b_aft, self.b_manual, self.b_complete):
            b.setStyleSheet("padding:2px 2px")
            b.setMinimumWidth(0)
            b.setSizePolicy(QSizePolicy.Ignored, QSizePolicy.Fixed)
            row.addWidget(b)
        el.addLayout(row)
        self.complete_lbl = QLabel("n/a")
        self.complete_lbl.setToolTip("Overall completeness of fine-tuning: not computed in the reproduction")
        self.b_reset = QToolButton()
        self.b_reset.setText("✕")
        self.b_reset.setStyleSheet("QToolButton{color:#d02020; font-weight:bold}")
        self.b_reset.setToolTip("Reset all fine-tuning (factors back to 1)")
        self.curve.buttons.addStretch(1)
        self.curve.buttons.addWidget(self.complete_lbl)
        self.curve.buttons.addWidget(self.b_reset)
        self.b_aft.clicked.connect(self.autoFineTune)
        self.b_complete.clicked.connect(self.autoComplete)
        self.b_reset.clicked.connect(self.resetFineTune)
        self.curve.editStarted.connect(self.editStarted)
        bl.addWidget(self._reg(eq, "adv"))

        # Profile Check and Tuning
        h = Header("Profile Check and Tuning", collapsible=False,
                   help_tip="Check the profile and adjust its noise level")
        self.check = BeginRow("Profile Check", "Profile Check: noise graph and profile values (repro dialog)",
                              checkable=False)
        self.check.toggled.connect(lambda _: self.profileCheck.emit())
        h.add(self.check)
        self.tune = {}
        inline = self._tune_row("all", "")
        hn = Header("Noise Level", level=1, inline=inline, collapsible=True)
        self._reg(hn.arrow, "adv")
        for c, name in UI_CHANNELS:
            hn.add(self._reg(self._tune_row(c, name, deco=f"dot:{c}"), "adv"))
        hf = Header("Frequencies", level=2)
        for b, name in UI_BANDS:
            r = Row(f"tune_band_{b}", name, "tune", f"band:{b}", indent=1, srange=(-100, 150),
                    spec=Spec("float", 1.0, TUNE_MIN, TUNE_MAX, name, implemented=False,
                              tip="Not available: the reproduction's profile curve has no frequency bands"))
            hf.add(r)
        hn.add(self._reg(hf, "adv"))
        h.add(hn)
        bl.addWidget(h)

        # repro extras
        hr = Header("Repro", level=0, expanded=True, help_tip="Reproduction-only profile settings")
        w = QWidget()
        gl = QGridLayout(w)
        gl.setContentsMargins(6, 2, 2, 2)
        gl.addWidget(QLabel("Profile mode:"), 0, 0)
        self.mode = QComboBox()
        self.mode.addItems(["auto", "spatial", "temporal"])
        self.mode.setToolTip("build_profile_v5 mode: auto = single-frame curve + band shape from static "
                             "pixels (default), spatial = genuine-style single frame, temporal = frame differences")
        gl.addWidget(self.mode, 0, 1)
        self.area = QLabel("Sample area: (none selected)")
        self.area.setWordWrap(True)
        gl.addWidget(self.area, 1, 0, 1, 2)
        hr.add(w)
        self.graph = NoiseGraph()
        hr.add(self.graph)
        bl.addWidget(self._reg(hr, "adv"))
        bl.addStretch(1)

        self.qsg, self.mode_lbl = guide_labels()
        for l in (self.qsg, self.mode_lbl):
            l.linkActivated.connect(self.linkActivated)
            v.addWidget(l)
        self.set_name(None)
        self.set_mode(True)

    def _reg(self, w, modes):
        self._modes.append((w, modes))
        return w

    def _tune_row(self, k, label, deco=None):
        r = Row(f"tune_{k}", label, "tune", deco, (5, 20), srange=(-100, 150), spec=tune_spec("Noise Level" if k == "all" else label,
                                                                          None if k == "all" else label))
        r.edited.connect(lambda key, v, k=k: self.tuneEdited.emit(k, float(v)))
        r.editStarted.connect(self.editStarted)
        self.tune[k] = r
        return r

    def set_mode(self, advanced):
        for w, m in self._modes:
            w.setVisible(("adv" if advanced else "beg") in m or m == "both")
        self.mode_lbl.setText(mode_text(advanced))

    def set_busy(self, busy):
        self.b_aft.setEnabled(not busy)

    def set_name(self, text, dirty=False):
        self.name.clear()
        if text is None:
            self.name.addItem("profile not ready")
            self.name.setStyleSheet("QComboBox{color:#d01010}")
            self.name.setToolTip("Build a noise profile by analysing a uniform area or load a ready-made profile.")
        else:
            self.name.addItem(text + (" *" if dirty else ""))
            self.name.setStyleSheet("")
            self.name.setToolTip("Current noise profile (* = not saved)")

    def show_profile(self, ps, eff, frame_info=None):
        if self.device.text() != ps.device:
            self.device.setText(ps.device)
        self.curve.set_factors(ps.factors, ps.pinned)
        self.graph.set_profile(eff, ps.base)
        for k, r in self.tune.items():
            r.set_value(ps.tune_value(k))
        W, H = frame_info or ("—", "—")
        if ps.base is None:
            prof_with = "—"
            self.match.setText("—")
            self.area.setText("Sample area: (no profile)")
            self.levels.setText("Noise Levels: —")
        else:
            b = ps.base
            y, x, s = b["area"][:3]
            prof_with = f"Area {s}x{s} at ({x}, {y}), frame {b.get('ui_frame', '?')}"
            st = b.get("flat_status")
            txt = "manual area" if st is None else nv.FLAT_STATUS.get(st, str(st))
            if b.get("aft_applied"):
                txt += " · AFT applied"
            self.match.setText(txt)
            self.area.setText(f"Profile area (y, x, size): {y}, {x}, {s} · frame {b.get('ui_frame', '?')}")
            self.levels.setText("Noise Levels (Y band ratio, repro): " +
                                " ".join(f"{r:.2f}" for r in b["band_ratio"]["Y"]))
        rows = [("Frame size:", f"{W}x{H} p"), ("Scan type:", "Progressive"), ("Gamma:", "Gamma corrected"),
                ("Gain:", "x1.0"), ("Profiled with:", prof_with)]
        self.info.setText("<table cellspacing=0 cellpadding=0>" + "".join(
            f"<tr><td>{a}&nbsp;&nbsp;</td><td>{b}</td></tr>" for a, b in rows) + "</table>")

    def set_levels(self, main=None, sel=None):
        self.main_val.setText("—" if main is None else f"{main:.2f}")
        self.sel_val.setText("—" if sel is None else f"{sel:.2f}")

    def set_selection(self, area):
        if area is not None:
            self.area.setText(f"Selected area (y, x, size): {area[0]}, {area[1]}, {area[2]}")
