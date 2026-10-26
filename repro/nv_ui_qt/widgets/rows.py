"""Side-panel building blocks in the genuine style: grey section headers ("▼ Name ...... Enable ☑")
and value rows ("Label  (dot / band pattern)  slider  value"), the value underlined with dots and
editable by a click. A row can drive several keys at once (Frequencies rows write Y, Cr and Cb)."""
import numpy as np
from PySide6.QtCore import QPointF, QRectF, QSize, Qt, Signal
from PySide6.QtGui import QBrush, QColor, QIcon, QImage, QPainter, QPen, QPixmap, QPolygonF, QRadialGradient
from PySide6.QtWidgets import (QAbstractSlider, QCheckBox, QComboBox, QDoubleSpinBox, QHBoxLayout, QLabel, QPushButton,
                               QSizePolicy, QSlider, QStackedWidget, QToolButton, QVBoxLayout, QWidget)

from ..state import PARAM_SPECS

CH_COLORS = {"Y": QColor(255, 255, 255), "Cr": QColor(236, 60, 60), "Cb": QColor(150, 90, 240),
             "R": QColor(236, 60, 60), "G": QColor(60, 190, 60), "B": QColor(60, 90, 236)}
BAND_SCALE = {"high": 1, "medium": 2, "low": 4, "very-low": 8, "ultra-low": 16}
HEADER_CSS = "background:#6e6e6e;"            # genuine section band (110, 110, 110), rp_freq.png
HEADER_TEXT = "#e9e9e9"                         # its bold title (233, 233, 233)


def _frames(v):
    return str(int(round(v)))          # genuine row shows "2" ("2 frames" is in the tooltip)


# display format: (to display, from display, text, slider scale)
FORMATS = {
    "amount": (lambda v: v, lambda d: d, lambda d: "off" if round(d) == 0 else f"{d:.0f}%", 1),
    "rel": (lambda v: v - 100.0, lambda d: d + 100.0, lambda d: f"{d:+.0f}%", 1),
    "inv": (lambda v: 100.0 - v, lambda d: 100.0 - d, lambda d: f"{d:.0f}%", 1),
    "tune": (lambda v: (v - 1.0) * 100.0, lambda d: 1.0 + d / 100.0, lambda d: f"{d:+.0f}%", 1),
    "frames": (lambda v: v, lambda d: d, _frames, 1),
    "float": (lambda v: v, lambda d: d, lambda d: f"{d:.2f}", 100),
    "range": (lambda v: v, lambda d: d, lambda d: f"{d:+.2f}", 100),
}


def dot_pixmap(c, size=14):
    """Round component marker, lit from the upper left like the genuine's (white / pink / lilac)."""
    pm = QPixmap(size, size)
    pm.fill(Qt.transparent)
    p = QPainter(pm)
    p.setRenderHint(QPainter.Antialiasing)
    p.setPen(QPen(QColor(90, 90, 90), 1))
    g = QRadialGradient(QPointF(size * 0.35, size * 0.3), size * 0.75)
    g.setColorAt(0, QColor(255, 255, 255))
    g.setColorAt(1, CH_COLORS[c].lighter(115) if c == "Y" else CH_COLORS[c])
    p.setBrush(QBrush(g))
    p.drawEllipse(QRectF(1.5, 1.5, size - 3, size - 3))
    p.end()
    return pm


def glyph_icon(text, color=QColor(220, 220, 220), size=20, bold=False):
    """QIcon of one text glyph (↶, ↷, ...) drawn in `color`."""
    pm = QPixmap(size, size)
    pm.fill(Qt.transparent)
    p = QPainter(pm)
    p.setRenderHint(QPainter.Antialiasing)
    f = p.font()
    f.setPixelSize(int(size * 0.8))
    f.setBold(bold)
    p.setFont(f)
    p.setPen(color)
    p.drawText(QRectF(0, 0, size, size), Qt.AlignCenter, text)
    p.end()
    return QIcon(pm)


def info_icon(size=18):
    """Blue round (i) of the panel's top bar."""
    pm = QPixmap(size, size)
    pm.fill(Qt.transparent)
    p = QPainter(pm)
    p.setRenderHint(QPainter.Antialiasing)
    p.setPen(QPen(QColor(20, 60, 140), 1))
    p.setBrush(QColor(40, 120, 230))
    p.drawEllipse(QRectF(1.5, 1.5, size - 3, size - 3))
    f = p.font()
    f.setPixelSize(int(size * 0.7))
    f.setBold(True)
    p.setFont(f)
    p.setPen(QColor(255, 255, 255))
    p.drawText(QRectF(0, 0, size, size), Qt.AlignCenter, "i")
    p.end()
    return QIcon(pm)


def pin_icon(size=18):
    """Blue push pin (Default Profile / Preset menu)."""
    pm = QPixmap(size, size)
    pm.fill(Qt.transparent)
    p = QPainter(pm)
    p.setRenderHint(QPainter.Antialiasing)
    p.setPen(QPen(QColor(200, 200, 200), 1.6))
    p.drawLine(QPointF(size * 0.22, size * 0.82), QPointF(size * 0.48, size * 0.52))
    p.setPen(QPen(QColor(20, 60, 140), 1))
    p.setBrush(QColor(60, 140, 240))
    p.drawPolygon(QPolygonF([QPointF(size * 0.45, size * 0.25), QPointF(size * 0.78, size * 0.58),
                             QPointF(size * 0.62, size * 0.62), QPointF(size * 0.40, size * 0.40)]))
    p.drawEllipse(QRectF(size * 0.50, size * 0.12, size * 0.36, size * 0.36))
    p.end()
    return QIcon(pm)


def square_icon(color, size=12):
    pm = QPixmap(size, size)
    pm.fill(Qt.transparent)
    p = QPainter(pm)
    p.setPen(QPen(QColor(20, 40, 90), 1))
    p.setBrush(color)
    p.drawRect(QRectF(1.5, 1.5, size - 3, size - 3))
    p.end()
    return QIcon(pm)


def triangle_icon(color=QColor(190, 190, 190), size=12):
    pm = QPixmap(size, size)
    pm.fill(Qt.transparent)
    p = QPainter(pm)
    p.setRenderHint(QPainter.Antialiasing)
    p.setPen(Qt.NoPen)
    p.setBrush(color)
    p.drawPolygon(QPolygonF([QPointF(3, 1.5), QPointF(size - 2, size / 2), QPointF(3, size - 1.5)]))
    p.end()
    return QIcon(pm)


_BAND_PM = {}


def band_pixmap(band, w=22, h=16):
    """Noise pattern of one frequency band (repro picture, deterministic): white noise blurred with a
    Gaussian of the band scale (the genuine's samples look like this: fine grain -> smooth blobs)."""
    if band not in _BAND_PM:
        from scipy.ndimage import gaussian_filter
        s = BAND_SCALE[band]
        rng = np.random.default_rng(7 + s)
        m = 2 * s + 2
        a = rng.standard_normal((h + 2 * m, w + 2 * m))
        if s > 1:
            a = gaussian_filter(a, 0.6 * s, mode="wrap")
        a = a[m:m + h, m:m + w]
        a = (a - a.mean()) / (a.std() + 1e-6)
        k = {1: 50, 2: 45, 4: 40, 8: 32, 16: 24}[s]          # coarser bands: softer (as the genuine's)
        g = np.ascontiguousarray(np.clip(128 + k * a, 0, 255).astype(np.uint8))
        _BAND_PM[band] = QPixmap.fromImage(QImage(g.data, w, h, w, QImage.Format_Grayscale8).copy())
    return _BAND_PM[band]


class ValueLabel(QLabel):
    """Value text with a dotted underline; a click asks for an exact value."""
    clicked = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setAlignment(Qt.AlignRight | Qt.AlignVCenter)
        self.setCursor(Qt.PointingHandCursor)

    def mousePressEvent(self, e):
        if e.button() == Qt.LeftButton and self.isEnabled():
            self.clicked.emit()

    def paintEvent(self, e):
        super().paintEvent(e)
        p = QPainter(self)
        p.setPen(QPen(self.palette().windowText().color(), 1, Qt.DotLine))
        fm = self.fontMetrics()
        w = fm.horizontalAdvance(self.text())
        y = self.height() // 2 + fm.ascent() // 2 + 2
        p.drawLine(self.width() - w, y, self.width() - 1, y)
        p.end()


class Row(QWidget):
    """Slider row bound to one or more keys of `spec` (default PARAM_SPECS[keys[0]]).
    edited(key, value) for a single key, editedMany({key: value}) for several keys."""
    edited = Signal(str, object)
    editedMany = Signal(dict)
    editStarted = Signal()

    def __init__(self, keys, label=None, fmt="amount", deco=None, steps=(5, 20), indent=0, spec=None,
                 srange=None, parent=None):
        """srange = (lo, hi) of the slider in display units (genuine track); the exact-value entry
        keeps the spec's full range."""
        super().__init__(parent)
        self.keys = [keys] if isinstance(keys, str) else list(keys)
        self.key = self.keys[0]
        self.spec = s = spec or PARAM_SPECS[self.key]
        self.to_d, self.from_d, self.text, self.sc = FORMATS[fmt]
        self.internal = s.default
        lay = QHBoxLayout(self)
        lay.setContentsMargins(6 + 14 * indent, 1, 2, 1)
        lay.setSpacing(4)
        self.label = QLabel(label if label is not None else s.label)
        if label != "":
            self.label.setMinimumWidth(max(70 - 14 * indent, 40))
            lay.addWidget(self.label)
        self.deco = QLabel()
        self.deco.setFixedSize(QSize(24, 16))
        if deco:
            self.deco.setPixmap(dot_pixmap(deco[4:], 16) if deco.startswith("dot:") else band_pixmap(deco[5:]))
            lay.addWidget(self.deco)
        self.slider = QSlider(Qt.Horizontal)
        lo, hi = sorted((self.to_d(s.lo), self.to_d(s.hi)))
        slo, shi = srange or (lo, hi)
        self.slider.setRange(int(round(slo * self.sc)), int(round(shi * self.sc)))
        self.slider.setSingleStep(int(steps[0] * self.sc) if self.sc == 1 else 5)
        self.slider.setPageStep(int(steps[1] * self.sc) if self.sc == 1 else 20)
        self.slider.setMinimumWidth(70)
        lay.addWidget(self.slider, 1)
        self.stack = QStackedWidget()
        self.stack.setFixedWidth(58)
        self.val = ValueLabel()
        self.spin = QDoubleSpinBox()
        self.spin.setRange(lo, hi)
        self.spin.setDecimals(0 if self.sc == 1 else 2)
        self.spin.setKeyboardTracking(False)
        self.spin.setButtonSymbols(QDoubleSpinBox.NoButtons)
        self.stack.addWidget(self.val)
        self.stack.addWidget(self.spin)
        lay.addWidget(self.stack)
        self.reset = QToolButton()
        self.reset.setText("⋈")
        self.reset.setAutoRaise(True)
        self.reset.setFixedSize(16, 16)
        self.reset.setStyleSheet("color:#e0409a; border:none; font-weight:bold")
        self.reset.setToolTip("Reset to the default value")
        sp = self.reset.sizePolicy()
        sp.setRetainSizeWhenHidden(True)
        self.reset.setSizePolicy(sp)
        self.reset.setVisible(False)
        lay.addWidget(self.reset)
        tip = s.tip or s.label
        for w in (self.label, self.slider, self.val, self.deco):
            w.setToolTip(tip)
        self.implemented = s.implemented
        self.setEnabled(s.implemented)
        self._block = False
        self.slider.sliderPressed.connect(self.editStarted)
        # only a real drag (handle held: SliderMove while isSliderDown) merges into the press's undo step;
        # wheel steps (also SliderMove), keys and page steps each start a new one (D6)
        self.slider.actionTriggered.connect(self._action)
        self.slider.valueChanged.connect(self._from_slider)
        self.val.clicked.connect(self._edit_exact)
        self.spin.editingFinished.connect(self._exact_done)
        self.reset.clicked.connect(self._reset)
        self._show(s.default, False)

    def _action(self, a):
        drag = QAbstractSlider.SliderAction(a) == QAbstractSlider.SliderAction.SliderMove and self.slider.isSliderDown()
        if not drag:
            self.editStarted.emit()

    # --- values ---
    def value(self):
        return self.internal

    def set_value(self, v, mixed=False):
        self.internal = float(v) if self.spec.kind != "int" else int(v)
        self._block = True
        self.slider.setValue(int(round(self.to_d(float(v)) * self.sc)))
        self._block = False
        self._show(v, mixed)

    def _show(self, v, mixed):
        d = self.to_d(float(v))
        self.val.setText("n/a" if not self.implemented else "mixed" if mixed else self.text(d))
        self.reset.setVisible(self.implemented and (mixed or abs(float(v) - float(self.spec.default)) > 1e-9))
        self.val.update()

    def _emit(self, v):
        v = min(max(v, self.spec.lo), self.spec.hi)      # the slider track may extend past the spec range
        if self.spec.kind == "int":
            v = int(round(v))
        self.internal = v
        self._show(v, False)
        if len(self.keys) == 1:
            self.edited.emit(self.key, v)
        else:
            self.editedMany.emit({k: v for k in self.keys})

    def _from_slider(self, iv):
        if not self._block:
            self._emit(self.from_d(iv / self.sc))

    def _edit_exact(self):
        self.spin.setValue(self.to_d(float(self.internal)))
        self.stack.setCurrentIndex(1)
        self.spin.setFocus()
        self.spin.selectAll()

    def _exact_done(self):
        if self.stack.currentIndex() != 1:
            return
        self.stack.setCurrentIndex(0)
        v = self.from_d(self.spin.value())
        if abs(v - float(self.internal)) > 1e-12:
            self.editStarted.emit()
            self.set_value(v)
            self._emit(v)

    def _reset(self):
        self.editStarted.emit()
        self.set_value(self.spec.default)
        self._emit(self.spec.default)


class EnableCheck(QCheckBox):
    """'Enable ☑' of a section header, bound to a bool key."""
    edited = Signal(str, object)
    editStarted = Signal()

    def __init__(self, key, parent=None):
        self.key, self.spec = key, PARAM_SPECS[key]
        super().__init__("Enable", parent)
        self.na_label = None                 # the header / row label that shows "(n/a)" for this box
        self.setLayoutDirection(Qt.RightToLeft)
        self.setToolTip(self.spec.tip or self.spec.label)
        self.setEnabled(self.spec.implemented)
        self._block = False
        self.toggled.connect(self._t)

    def value(self):
        return self.isChecked()

    def set_value(self, v):
        self._block = True
        self.setChecked(bool(v))
        self._block = False

    def _t(self, v):
        if not self._block:
            self.editStarted.emit()
            self.edited.emit(self.key, bool(v))


class ChoiceRow(QWidget):
    """'Label  [combo]' row (Quality Mode, Dust and Scratches Mode); `names` are display texts."""
    edited = Signal(str, object)
    editStarted = Signal()

    def __init__(self, key, label, names=None, indent=0, parent=None):
        super().__init__(parent)
        self.key, self.spec = key, PARAM_SPECS[key]
        lay = QHBoxLayout(self)
        lay.setContentsMargins(6 + 14 * indent, 1, 2, 1)
        self.label = lab = QLabel(label if self.spec.implemented else f"{label} (n/a)")
        lab.setMinimumWidth(90)
        lay.addWidget(lab)
        self.box = QComboBox()
        self.choices = list(self.spec.choices)
        self.box.addItems(names or self.choices)
        lay.addWidget(self.box, 1)
        lay.addSpacing(20)
        for w in (lab, self.box):
            w.setToolTip(self.spec.tip or self.spec.label)
        self.setEnabled(self.spec.implemented)
        self._block = False
        self.box.currentIndexChanged.connect(self._c)

    def value(self):
        return self.choices[self.box.currentIndex()]

    def set_value(self, v):
        self._block = True
        self.box.setCurrentIndex(self.choices.index(v) if v in self.choices else 0)
        self._block = False

    def _c(self, i):
        if not self._block and i >= 0:
            self.editStarted.emit()
            self.edited.emit(self.key, self.choices[i])


ACTIVE_TITLE = "#f0d000"                 # genuine: title of a running check tool in yellow (pc1_panel.png)


class BeginRow(QWidget):
    """'⚙ Name (?)   [▷ Begin]' check tool (Profile Check, Filter Tuning Assist, ...); while it runs the
    title is yellow and the button reads '■ Finish' (blue square), as the genuine shows. dim = grey title
    while not running (genuine Profile Check)."""
    toggled = Signal(bool)

    def __init__(self, title, tip="", checkable=True, enabled=True, indent=0, dim=False, parent=None):
        super().__init__(parent)
        lay = QHBoxLayout(self)
        lay.setContentsMargins(6 + 14 * indent, 2, 22, 2)
        self.name, self.dim = title, dim
        self.title = QLabel()
        self.title.setToolTip(tip)
        self.title.setWordWrap(True)
        self.title.setMinimumWidth(0)
        self.title.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Preferred)
        lay.addWidget(self.title, 1)
        self.button = QPushButton("Begin" if enabled else "Begin (n/a)")
        self.button.setCheckable(checkable)
        self.button.setFixedWidth(80)
        self.button.setToolTip(tip)
        lay.addWidget(self.button)
        self.button.toggled.connect(self._t)
        self.button.clicked.connect(lambda: None if checkable else self.toggled.emit(True))
        self.setEnabled(enabled)
        self._look(False)

    def _look(self, on):
        c = ACTIVE_TITLE if on else "#9a9a9a" if self.dim else None
        name = f"⚙ {self.name}" if c is None else f"<span style='color:{c}'>⚙ {self.name}</span>"
        self.title.setText(f"{name} <span style='color:#2a6fd6'>(?)</span>")
        if self.button.text() != "Begin (n/a)":
            self.button.setText("Finish" if on else "Begin")
        self.button.setIcon(square_icon(QColor(70, 150, 255)) if on else triangle_icon())

    def _t(self, on):
        self._look(on)
        self.toggled.emit(on)

    def set_checked(self, on):
        self.button.blockSignals(True)
        self.button.setChecked(on)
        self._look(on)
        self.button.blockSignals(False)


class Header(QWidget):
    """Section: header band (arrow, title, optional inline slider row, optional Enable check box
    and ↺) and a body for the rows. level 0 = grey band, 1 / 2 = nested sub-groups."""
    resetClicked = Signal()

    def __init__(self, title, enable=None, expanded=True, collapsible=True, level=0, inline=None,
                 reset=False, help_tip=None, parent=None):
        super().__init__(parent)
        v = QVBoxLayout(self)
        v.setContentsMargins(0, 0, 0, 0)
        v.setSpacing(0)
        self.band = QWidget()
        self.band.setObjectName("band")
        if level == 0:
            self.band.setStyleSheet("#band{" + HEADER_CSS + "}")
            self.band.setAttribute(Qt.WA_StyledBackground, True)
        h = QHBoxLayout(self.band)
        h.setContentsMargins(2 + 14 * max(level - 1, 0), 3, 2, 3)
        h.setSpacing(4)
        self.arrow = QToolButton()
        self.arrow.setAutoRaise(True)
        self.arrow.setFixedSize(14, 14)
        self.arrow.setStyleSheet("border:none")
        sp = self.arrow.sizePolicy()
        sp.setRetainSizeWhenHidden(True)                # titles stay aligned when Beginner hides the arrow
        self.arrow.setSizePolicy(sp)
        self.collapsible = collapsible
        if collapsible:
            self.arrow.setArrowType(Qt.DownArrow if expanded else Qt.RightArrow)
            self.arrow.clicked.connect(self.toggle)
        else:
            self.arrow.setText("•" if level else "")
        h.addWidget(self.arrow)
        na = enable is not None and not enable.spec.implemented
        self.title = QLabel(f"<b>{title}</b>" + (" <span style='color:#2a6fd6'>(?)</span>" if help_tip else "")
                            + (f" <span style='color:{'#c8c8c8' if level == 0 else '#808080'}'>(n/a)</span>"
                               if na else ""))
        if level == 0:
            self.title.setStyleSheet(f"color:{HEADER_TEXT}")
        if na:
            self.title.setToolTip(enable.spec.tip)
            enable.na_label = self.title
        if help_tip:
            self.title.setToolTip(help_tip)
        self.title.mousePressEvent = lambda e: self.toggle() if collapsible else None
        h.addWidget(self.title)
        self.inline = inline
        if inline is not None:
            inline.layout().setContentsMargins(0, 0, 0, 0)
            h.addWidget(inline, 1)
        else:
            h.addStretch(1)
        self.enable = enable
        if enable is not None:
            h.addWidget(enable)
        self.reset_btn = None
        if reset:
            self.reset_btn = QToolButton()
            self.reset_btn.setText("↺")
            self.reset_btn.setAutoRaise(True)
            self.reset_btn.setToolTip("Reset all settings of this tab to the default preset values")
            self.reset_btn.clicked.connect(self.resetClicked)
            h.addWidget(self.reset_btn)
        v.addWidget(self.band)
        self.body = QWidget()
        self.lay = QVBoxLayout(self.body)
        self.lay.setContentsMargins(0, 0, 0, 2)
        self.lay.setSpacing(0)
        self.body.setVisible(expanded)
        v.addWidget(self.body)
        self.title_text = title

    def toggle(self):
        self.set_expanded(self.body.isHidden())

    def set_expanded(self, on):
        if self.collapsible:
            self.arrow.setArrowType(Qt.DownArrow if on else Qt.RightArrow)
        self.body.setVisible(on)

    def add(self, w):
        self.lay.addWidget(w)
        return w


def separator():
    w = QWidget()
    w.setFixedHeight(6)
    w.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
    return w


class IconBar(QWidget):
    """Top buttons of a side panel: open · save | undo ▾ · redo ▾ (curved arrows; ▾ = the undo history)
    · reset (red ×) | default (pin) ▾ · info (blue (i)). Undo / redo get the QUndoStack actions later
    (bind_undo)."""

    def __init__(self, what, parent=None):
        super().__init__(parent)
        from PySide6.QtWidgets import QMenu, QStyle
        lay = QHBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 0)
        lay.setSpacing(2)
        st = self.style()

        def btn(icon, tip, text=None):
            b = QToolButton()
            if icon is not None:
                b.setIcon(st.standardIcon(icon))
            if text:
                b.setText(text)
            b.setToolTip(tip)
            b.setAutoRaise(True)
            lay.addWidget(b)
            return b

        self.open = btn(QStyle.SP_DirOpenIcon, f"Load {what} from a file…")
        self.save = btn(QStyle.SP_DialogSaveButton, f"Save current {what} to a file…")
        lay.addSpacing(10)
        self.undo = btn(None, "Undo last change")
        self.redo = btn(None, "Redo")
        self.reset = btn(None, f"Reset {what}", "✕")
        self.reset.setStyleSheet("QToolButton{color:#d02020; font-weight:bold}")
        lay.addStretch(1)
        self.default = btn(None, f"Default {what}")
        self.default.setIcon(pin_icon())
        self.default.setPopupMode(QToolButton.InstantPopup)
        self.default_menu = QMenu(self.default)
        self.default.setMenu(self.default_menu)
        self.info = btn(None, f"Description of the current {what}")
        self.info.setIcon(info_icon())

    def bind_undo(self, undo, redo, stack=None):
        """undo / redo: the QUndoStack actions; stack: the QUndoStack whose history the ▾ menus list
        (choosing an entry undoes / redoes up to it)."""
        from PySide6.QtWidgets import QMenu
        for b, a, glyph, back in ((self.undo, undo, "↶", True), (self.redo, redo, "↷", False)):
            if a.icon().isNull():
                a.setIcon(glyph_icon(glyph, QColor(80, 150, 255), bold=True))
            b.setDefaultAction(a)
            b.setToolButtonStyle(Qt.ToolButtonIconOnly)
            if stack is not None:
                m = QMenu(b)
                m.aboutToShow.connect(lambda m=m, back=back: self._history(m, stack, back))
                b.setMenu(m)
                b.setPopupMode(QToolButton.MenuButtonPopup)

    @staticmethod
    def _history(menu, stack, back):
        menu.clear()
        i = stack.index()
        steps = range(i - 1, max(i - 11, -1), -1) if back else range(i, min(i + 10, stack.count()))
        for k in steps:
            menu.addAction(stack.text(k), lambda k=k: stack.setIndex(k if back else k + 1))
        if menu.isEmpty():
            menu.addAction("(no history)").setEnabled(False)


def panel_title(text):
    lab = QLabel(f"<b>{text}</b>")
    lab.setAlignment(Qt.AlignCenter)
    return lab


def guide_labels():
    """'How to use: Quick Start Guide.' + mode comment (links: 'qsg', 'mode')."""
    qsg = QLabel("How to use: <a href='qsg'>Quick Start Guide</a>.")
    mode = QLabel()
    for l in (qsg, mode):
        l.setAlignment(Qt.AlignCenter)
        l.setWordWrap(True)
    return qsg, mode


def mode_text(advanced):
    other = "Beginner" if advanced else "Advanced"
    more = "simpler settings" if advanced else "more control"
    return (f"Currently working in <b>{'Advanced' if advanced else 'Beginner'} Mode</b>.<br>"
            f"For {more} switch to <a href='mode'>{other} Mode</a>.")
