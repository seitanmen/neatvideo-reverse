"""Device Noise Profile side panel (Prepare Noise Profile page) in the genuine order (binary
FUN_000d99d4 / FUN_000e0b18) and look (genuine 6.1.3 after Auto Profile, sandbox/omo/coord/genuine_ui2/
shots2/g0.png, g0_panel_top.png, g0_eq.png, rp_nl.png, rp_freq.png), top to bottom without scrolling:
title; icons (open, save | undo ▾, redo ▾, reset × | default (pin) ▾, info (i)); profile name ("profile
not ready" in red); Device Name and Mode with its ▸≡ button and the name; the info box (light text on a
dark box; Profiled with = "Full frame (W x H)" as the genuine prints it); the statistics in two columns
(left: Quality (n/a), Noise Levels Main Sample / Selection (genuine measure, display.noise_level); right:
a noise sample image and a green bar per band High .. Ultra Low); the equalizer with its button row
(Auto Fine-Tune, Manual Fine-Tune (n/a), Auto Complete | the four round component toggles |
completeness (n/a) | reset ×); Profile Check and Tuning: ⚙ Profile Check (?) [Begin] (toggles the
Assist Image mode of the main window; while it runs the title and the section are yellow and the
button reads Finish) and Noise Level (collapsed; Luminance -> Cr -> Cb, Frequencies collapsed).
The repro-only profile details (mode, sample area, FindFlatArea status, band ratios, sigma graph) are in
the Profile Viewer dialog (main_window.profile_viewer).

Profile Noise Level is a repro multiplier of the profile curve (state.effective_profile); the profile
curve has no bands, so the Frequencies rows are shown disabled.
Band bars (repro definition): the profile's Y band ratio of band j divided by that of white noise
(nvfilter._wn_band_std), relative to the largest band; the genuine's bar values are not decoded.
Equalizer: the genuine draws R / G / B; the reproduction's fine-tune factors multiply the Y / Cb / Cr
curves of the v5 backend (the AFT's R / G / B factors are combined into Y / Cb / Cr by nv_aft_v5), so the
editor keeps the Y / Cr / Cb lines."""
from PySide6.QtCore import QPointF, QRectF, QSize, Qt, Signal
from PySide6.QtGui import QColor, QIcon, QPainter, QPen, QPixmap
from PySide6.QtWidgets import (QComboBox, QFrame, QGridLayout, QHBoxLayout, QLabel, QLineEdit, QScrollArea,
                               QSizePolicy, QToolButton, QVBoxLayout, QWidget)

import nvfilter as nv

from ..state import TUNE_MAX, TUNE_MIN, UI_BANDS, UI_CHANNELS, Spec
from .curve_editor import CurveEditor
from .rows import ACTIVE_TITLE, BeginRow, Header, IconBar, Row, band_pixmap, panel_title

TUNE_TIP = ("Profile Noise Level (repro): multiplies the noise profile curve{} - higher values -> more "
            "image elements are treated as noise. Saved with the profile.")
DARK_BOX = "background:#3a3a3a; color:#d5d5d5; border:1px solid #262626;"
CHECK_TIP = ("Profile Check helps to find the optimal value of Noise Level using a visually enhanced preview "
             "(Assist Image).\nClick Begin to start the checking and tuning process.\n"
             "Adjust Noise Level while watching the image in preview to find the Noise Level value that "
             "completely removes noise yet preserves details.\n"
             "(repro: the Assist Image is the current frame through the spatial filter only, all amounts "
             "100%; the genuine's Assist Image processing is not decoded)")
EQ_TIPS = {
    "aft": "Use to automatically fine-tune the current noise profile using the whole input image\n"
           "(repro: nv_aft_v5.apply_aft, static port of the genuine AutoFineTune; its factors were not "
           "compared with the genuine; takes several seconds)",
    "manual": "Use to additionally fine-tune current noise profile using selected image area\n"
              "(n/a: not available in the reproduction; drag the knots of the graph instead)",
    "complete": "Use to finalize manual fine-tuning by interpolating missing values using measured data\n"
                "(repro: interpolates the bins you did not edit between the edited ones)",
}


class BandBar(QWidget):
    """One green bar of the band list (value 0..1)."""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.value = 0.0
        self.setFixedSize(44, 12)

    def set_value(self, v):
        self.value = 0.0 if v is None else float(min(max(v, 0.0), 1.0))
        self.update()

    def paintEvent(self, e):
        p = QPainter(self)
        r = QRectF(self.rect()).adjusted(0, 1, -1, -1)
        if self.value > 0:
            p.fillRect(QRectF(r.left(), r.top(), max(r.width() * self.value, 3), r.height()), QColor(76, 175, 80))
            p.setPen(QPen(QColor(30, 60, 30), 1))
            p.drawRect(QRectF(r.left(), r.top(), max(r.width() * self.value, 3), r.height()))
        p.end()


def eq_icon(kind, size=20):
    """Bar-chart icons of the equalizer buttons (Auto Fine-Tune / Manual Fine-Tune / Auto Complete)."""
    pm = QPixmap(size, size)
    pm.fill(Qt.transparent)
    p = QPainter(pm)
    p.setRenderHint(QPainter.Antialiasing)
    p.setPen(QPen(QColor(60, 60, 60), 1))
    p.setBrush(QColor(236, 236, 236))
    p.drawRect(QRectF(1.5, 1.5, size - 3, size - 3))
    hs = {"aft": (5, 9, 7, 12, 10, 14), "manual": (6, 11, 4, 9, 12, 7), "complete": (4, 7, 9, 11, 9, 13)}[kind]
    for k, h in enumerate(hs):
        x = 3.5 + k * 2.4
        dotted = kind == "manual" and k % 2
        p.setPen(QPen(QColor(40, 150, 40), 1.4, Qt.DotLine if dotted else Qt.SolidLine))
        p.drawLine(QPointF(x, size - 3.5), QPointF(x, size - 3.5 - h))
    if kind == "complete":
        p.setPen(QPen(QColor(240, 140, 20), 1.6))
        p.drawPolyline([QPointF(3, size - 8), QPointF(size / 2, 5), QPointF(size - 3, 8)])
    p.end()
    return QIcon(pm)


def band_bars(base):
    """Bar values High .. Ultra Low (see the module doc) or None without a profile."""
    if base is None:
        return None
    r = list(base["band_ratio"]["Y"])
    wn = nv._wn_band_std(len(r))
    v = [r[j] * wn[0] / wn[j] for j in range(len(r))]
    m = max(v) if v and max(v) > 0 else 1.0
    return [x / m for x in v]


def tune_spec(label, c=None):
    return Spec("float", 1.0, TUNE_MIN, TUNE_MAX, label, TUNE_TIP.format(f" of {c}" if c else ""))


class ProfilePanel(QWidget):
    autoFineTune = Signal()
    autoComplete = Signal()
    resetFineTune = Signal()
    profileCheck = Signal(bool)          # Begin / Finish of Profile Check (Assist Image mode)
    editDevice = Signal(str)
    tuneEdited = Signal(str, float)     # "all" / "Y" / "Cb" / "Cr", multiplier
    editStarted = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self._modes = []
        outer = QVBoxLayout(self)
        outer.setContentsMargins(0, 0, 0, 0)
        # one scroll area without a frame: no scroll bar while everything fits (the genuine panel does
        # not scroll), the content stays at the top
        sc = QScrollArea()
        sc.setWidgetResizable(True)
        sc.setFrameShape(QFrame.NoFrame)
        sc.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        sc.setVerticalScrollBarPolicy(Qt.ScrollBarAsNeeded)
        self.scroll = sc
        body = QWidget()
        v = QVBoxLayout(body)
        v.setContentsMargins(4, 4, 4, 4)
        v.setSpacing(3)
        sc.setWidget(body)
        outer.addWidget(sc)

        v.addWidget(panel_title("Device Noise Profile"))
        self.icons = IconBar("noise profile")
        v.addWidget(self.icons)
        self.name = QComboBox()
        v.addWidget(self.name)
        dh = QHBoxLayout()
        dh.addWidget(QLabel("Device Name and Mode:"))
        dh.addStretch(1)
        self.device_menu = QToolButton()
        self.device_menu.setText("▸≡")
        self.device_menu.setAutoRaise(True)
        self.device_menu.setEnabled(False)
        self.device_menu.setToolTip("Device name and mode options (n/a: not available in the reproduction)")
        dh.addWidget(self.device_menu)
        v.addLayout(dh)
        self.device = QLineEdit("Unspecified device")
        self.device.setToolTip("Optional device name (e.g., 'Nikon D90'); saved with the profile")
        self.device.editingFinished.connect(lambda: self.editDevice.emit(self.device.text()))
        v.addWidget(self.device)
        self.info = QLabel()
        self.info.setStyleSheet("QLabel{" + DARK_BOX + " padding:3px 5px}")
        self.info.setTextFormat(Qt.RichText)
        self.info.setAlignment(Qt.AlignLeft | Qt.AlignTop)
        self.info.setMinimumHeight(5 * self.info.fontMetrics().lineSpacing() + 10)
        v.addWidget(self.info)

        # statistics: left Quality / Noise Levels, right the bands
        self.stats = st = QWidget()
        sh = QHBoxLayout(st)
        sh.setContentsMargins(0, 2, 0, 2)
        sh.setSpacing(6)
        left = QGridLayout()
        left.setContentsMargins(2, 0, 0, 0)
        left.setVerticalSpacing(4)
        left.addWidget(QLabel("Quality"), 0, 0)
        self.quality = QLabel("n/a")
        self.quality.setAlignment(Qt.AlignCenter)
        self.quality.setFixedWidth(52)
        self.quality.setStyleSheet("QLabel{" + DARK_BOX + "}")
        self.quality.setToolTip("Overall Profile Quality: curve completeness x sample uniformity x "
                                "area size x clipping penalty (genuine measure, decoded 2026-10-26)")
        left.addWidget(self.quality, 0, 1, Qt.AlignRight)
        left.addWidget(QLabel("Completeness"), 1, 0)
        self.completeness = QLabel("n/a")
        self.completeness.setAlignment(Qt.AlignCenter)
        self.completeness.setFixedWidth(52)
        self.completeness.setStyleSheet("QLabel{" + DARK_BOX + "}")
        self.completeness.setToolTip("Completeness: the luminance-curve term g x 100 "
                                      "(genuine measure, decoded 2026-10-26)")
        left.addWidget(self.completeness, 1, 1, Qt.AlignRight)
        lt = QHBoxLayout()
        self.levels_title = QLabel("<b>Noise Levels</b>")
        lt.addWidget(self.levels_title)
        self.levels_line = QFrame()
        self.levels_line.setFrameShape(QFrame.HLine)
        self.levels_line.setFrameShadow(QFrame.Sunken)
        lt.addWidget(self.levels_line, 1)
        left.addLayout(lt, 2, 0, 1, 2)
        self.l_main = QLabel("Main Sample")
        self.main_val = QLabel("-")
        self.l_sel = QLabel("Selection")
        self.sel_val = QLabel("-")
        for w in (self.main_val, self.sel_val):
            w.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
            w.setToolTip("Noise Level (genuine measure FUN_009b01d4): sqrt of the sum over five levels and "
                         "R, G, B of the squared mean local RMS of the area (Main Sample = the profile's area)")
        left.addWidget(self.l_main, 3, 0)
        left.addWidget(self.main_val, 3, 1)
        left.addWidget(self.l_sel, 4, 0)
        left.addWidget(self.sel_val, 4, 1)
        left.setRowStretch(5, 1)
        sh.addLayout(left, 1)
        sep = QFrame()
        sep.setFrameShape(QFrame.VLine)
        sep.setFrameShadow(QFrame.Sunken)
        sh.addWidget(sep)
        right = QGridLayout()
        right.setContentsMargins(0, 0, 0, 0)
        right.setHorizontalSpacing(4)
        right.setVerticalSpacing(3)
        self.bars, self.band_samples = {}, {}
        for i, (b, name) in enumerate(UI_BANDS):
            pic = QLabel()
            pic.setPixmap(band_pixmap(b))
            pic.setToolTip(f"Noise pattern of the {name} frequency band (repro picture)")
            lab = QLabel("Medium" if name == "Mid" else name)
            bar = BandBar()
            bar.setToolTip("Band noise relative to white noise (repro definition, see the panel's doc)")
            right.addWidget(pic, i, 0)
            right.addWidget(lab, i, 1)
            right.addWidget(bar, i, 2)
            self.bars[b], self.band_samples[b] = bar, pic
        right.setColumnStretch(1, 1)
        sh.addLayout(right, 1)
        v.addWidget(st)
        for w in (self.levels_title, self.levels_line, self.l_main, self.main_val, self.l_sel, self.sel_val):
            self._reg(w, "adv")

        # equalizer: graph, then [Auto Fine-Tune, Manual, Auto Complete | 4 round toggles | n/a | x]
        eq = QFrame()
        eq.setObjectName("eq")
        eq.setStyleSheet("#eq{background:#595959; border:1px solid #3a3a3a}")
        el = QVBoxLayout(eq)
        el.setContentsMargins(2, 2, 2, 2)
        self.curve = CurveEditor()
        el.addWidget(self.curve)
        row = self.curve.buttons

        def tool(kind, text):
            b = QToolButton()
            b.setText(text)
            b.setIcon(eq_icon(kind))
            b.setIconSize(QSize(20, 20))
            b.setToolButtonStyle(Qt.ToolButtonIconOnly)
            b.setFixedSize(26, 24)
            b.setToolTip(EQ_TIPS[kind])
            return b

        self.b_aft = tool("aft", "Auto Fine-Tune")
        self.b_manual = tool("manual", "Manual Fine-Tune (n/a)")
        self.b_manual.setEnabled(False)
        self.b_complete = tool("complete", "Auto Complete")
        for k, b in enumerate((self.b_aft, self.b_manual, self.b_complete)):
            row.insertWidget(k, b)
        row.insertSpacing(3, 6)
        self.complete_lbl = QLabel("n/a")
        self.complete_lbl.setAlignment(Qt.AlignCenter)
        self.complete_lbl.setFixedWidth(56)
        self.complete_lbl.setStyleSheet("QLabel{" + DARK_BOX + "}")
        self.complete_lbl.setToolTip("Overall completeness and accuracy of fine-tuning\n(n/a: not computed in "
                                     "the reproduction)")
        self.b_reset = QToolButton()
        self.b_reset.setText("✕")
        self.b_reset.setFixedSize(26, 24)
        self.b_reset.setStyleSheet("QToolButton{color:#e02020; font-weight:bold; font-size:16px}")
        self.b_reset.setToolTip("Reset all fine-tuning")
        row.addStretch(1)
        row.addWidget(self.complete_lbl)
        row.addStretch(1)
        row.addWidget(self.b_reset)
        self.b_aft.clicked.connect(self.autoFineTune)
        self.b_complete.clicked.connect(self.autoComplete)
        self.b_reset.clicked.connect(self.resetFineTune)
        self.curve.editStarted.connect(self.editStarted)
        v.addWidget(self._reg(eq, "adv"))

        # Profile Check and Tuning
        self.tuning = h = Header("Profile Check and Tuning", collapsible=False)
        self.check = BeginRow("Profile Check", CHECK_TIP, checkable=True, dim=True)
        self.check.toggled.connect(self.profileCheck)
        h.add(self.check)
        self.tune = {}
        inline = self._tune_row("all", "")
        self.tune_header = hn = Header("Noise Level", level=1, inline=inline, collapsible=True, expanded=False)
        self._reg(hn.arrow, "adv")
        for c, name in UI_CHANNELS:
            hn.add(self._reg(self._tune_row(c, name, deco=f"dot:{c}"), "adv"))
        self.freq_header = hf = Header("Frequencies", level=2, expanded=False)
        for b, name in UI_BANDS:
            r = Row(f"tune_band_{b}", name, "tune", f"band:{b}", indent=1, srange=(-100, 150),
                    spec=Spec("float", 1.0, TUNE_MIN, TUNE_MAX, name, implemented=False,
                              tip="Not available: the reproduction's profile curve has no frequency bands"))
            hf.add(r)
        hn.add(self._reg(hf, "adv"))
        h.add(hn)
        v.addWidget(h)
        v.addStretch(1)
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

    def set_busy(self, busy):
        self.b_aft.setEnabled(not busy)

    def set_check_active(self, on):
        """Profile Check running: the row (Finish), the section title and Noise Level in yellow."""
        self.check.set_checked(on)
        for hd in (self.tuning, self.tune_header):
            t = f"<b>{hd.title_text}</b>"
            hd.title.setText(f"<span style='color:{ACTIVE_TITLE}'>{t}</span>" if on else t)

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
        main_bin = None if ps.base is None else int(min(max(round(ps.base.get("area_level", 128.0) / 32.0), 0), 8))
        self.curve.set_factors(ps.factors, ps.pinned, main_bin)
        for k, r in self.tune.items():
            r.set_value(ps.tune_value(k))
        W, H = frame_info or ("—", "—")
        bars = band_bars(ps.base)
        for i, (b, _) in enumerate(UI_BANDS):
            self.bars[b].set_value(None if bars is None or i >= len(bars) else bars[i])
        rows = [] if ps.base is None else [("Frame size:", f"{W}x{H} p"), ("Scan type:", "Progressive"),
                                           ("Gamma:", "Gamma corrected"), ("Gain:", "x1.0"),
                                           ("Profiled with:", f"Full frame ({W}x{H})")]
        self.info.setText("<table cellspacing=0 cellpadding=0>" + "".join(
            f"<tr><td>{a}&nbsp;&nbsp;</td><td>{b}</td></tr>" for a, b in rows) + "</table>")

    def set_levels(self, main=None, sel=None):
        self.main_val.setText("-" if main is None else f"{main:.1f}")
        self.sel_val.setText("-" if sel is None else f"{sel:.1f}")

    def set_quality(self, pct):
        """Overall Profile Quality (genuine measure, quality.py): percent or None = not computed."""
        self.quality.setText("n/a" if pct is None else f"{pct}%")

    def set_completeness(self, pct):
        """Completeness (genuine measure, quality.py: g x 100): percent or None = not computed."""
        self.completeness.setText("n/a" if pct is None else f"{pct}%")
