"""Noise profile equalizer (Manual Fine-Tune) in the genuine look: the per-brightness noise levels of the
three channels drawn together, vertical axis (level - 1) x 100% from -100% to +200%, on the panel grey with a
dotted grid (every bin, every 50%), a black -> white brightness bar under the horizontal axis. Knots are small
yellow squares (edited ones filled with the channel's colour); the knot of the bin holding the profile's sample
brightness is drawn larger, as the genuine draws one knot larger (g0_eq.png; that this is the sample's bin is an
inference). The edit target is chosen with round toggles (all channels = three-colour disc, then one per
channel).
An R/G/B profile (G1) shows the genuine's R / G / B levels (red, green, blue; toggles RGB / R / G / B): after
Auto Fine-Tune its curves are the AFT's levels. A Y / Cb / Cr profile of an older file keeps the reproduction's
former Y / Cr / Cb factors (Luminance white, Cr red, Cb purple)."""
from PySide6.QtCore import QPointF, QRectF, QSize, Qt, Signal
from PySide6.QtGui import QBrush, QColor, QIcon, QLinearGradient, QPainter, QPen, QPixmap
from PySide6.QtWidgets import QButtonGroup, QHBoxLayout, QSizePolicy, QToolButton, QVBoxLayout, QWidget

from ..state import CH_INDEX, CURVE_X, FACTOR_MAX, FACTOR_MIN, UI_CHANNELS
from .rows import CH_COLORS, dot_pixmap

D_MIN, D_MAX = -100.0, 200.0
PANEL_GREY = QColor(89, 89, 89)              # genuine panel / plot background (g0.png)


# the channels of the equalizer in drawing / toggle order with their index in ProfileState.factors
ORDER = {"RGB": [("R", 0), ("G", 1), ("B", 2)],
         "YCbCr": [(c, CH_INDEX[c]) for c, _ in UI_CHANNELS]}          # Y, Cr, Cb (factors Y, Cb, Cr)
NAMES = {"Y": "Luminance (Y)", "Cr": "Cr", "Cb": "Cb", "R": "Red", "G": "Green", "B": "Blue"}


def all_icon(size=18, keys=("Y", "Cr", "Cb")):
    """Three-colour disc of the 'all channels' toggle."""
    pm = QPixmap(size, size)
    pm.fill(Qt.transparent)
    p = QPainter(pm)
    p.setRenderHint(QPainter.Antialiasing)
    r = QRectF(1.5, 1.5, size - 3, size - 3)
    for k, c in enumerate(keys):
        p.setPen(Qt.NoPen)
        p.setBrush(CH_COLORS[c])
        p.drawPie(r, (90 + 120 * k) * 16, 120 * 16)
    p.setPen(QPen(QColor(60, 60, 60), 1))
    p.setBrush(Qt.NoBrush)
    p.drawEllipse(r)
    p.end()
    return QIcon(pm)


class EqualizerPlot(QWidget):
    def __init__(self, owner):
        super().__init__(owner)
        self.o = owner
        self.setMinimumHeight(170)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self._drag = None

    def plot_rect(self):
        return QRectF(self.rect()).adjusted(48, 8, -8, -20)

    def pt(self, i, f):
        r = self.plot_rect()
        d = (f - 1.0) * 100.0
        return QPointF(r.left() + r.width() * CURVE_X[i] / 256.0, r.top() + r.height() * (D_MAX - d) / (D_MAX - D_MIN))

    def factor_at(self, y):
        r = self.plot_rect()
        d = D_MAX - (y - r.top()) / r.height() * (D_MAX - D_MIN)
        return min(max(1.0 + d / 100.0, FACTOR_MIN), FACTOR_MAX)

    def paintEvent(self, e):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        p.fillRect(self.rect(), PANEL_GREY)
        r = self.plot_rect()
        p.setPen(QPen(QColor(190, 190, 190), 1, Qt.DotLine))
        for i in range(9):
            x = self.pt(i, 1.0).x()
            p.drawLine(QPointF(x, r.top()), QPointF(x, r.bottom()))
        for d in range(200, -101, -50):
            y = r.top() + r.height() * (D_MAX - d) / (D_MAX - D_MIN)
            p.setPen(QPen(QColor(190, 190, 190), 1, Qt.DotLine))
            p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y))
            if d % 100 == 0:
                p.setPen(QColor(240, 240, 240))
                p.drawText(QRectF(0, y - 7, 45, 14), Qt.AlignRight | Qt.AlignVCenter, f"{d:+d}%")
        g = QLinearGradient(r.left(), 0, r.right(), 0)
        g.setColorAt(0, QColor(0, 0, 0))
        g.setColorAt(1, QColor(255, 255, 255))
        bar = QRectF(r.left(), r.bottom() + 5, r.width(), 8)
        p.fillRect(bar, QBrush(g))
        p.setPen(QPen(QColor(40, 40, 40), 1))
        p.drawRect(bar)
        tgt = self.o.target
        order = [c for c, _ in self.o.order()]
        order = [c for c in order if c != tgt] + ([tgt] if tgt in order else [])
        for c in order:
            fs = self.o.factors[self.o.index(c)]
            pts = [self.pt(i, fs[i]) for i in range(9)]
            active = tgt in ("all", c)
            p.setPen(QPen(CH_COLORS[c], 2 if active else 1))
            for a, b in zip(pts, pts[1:]):
                p.drawLine(a, b)
            for i, q in enumerate(pts):
                pinned = i in self.o.pinned[self.o.index(c)]
                p.setPen(QPen(QColor(20, 20, 20), 1))
                p.setBrush(QBrush(CH_COLORS[c] if pinned else QColor(250, 220, 0)))
                s = (6 if active else 5) + (4 if i == self.o.main_bin else 0)
                p.drawRect(QRectF(q.x() - s / 2, q.y() - s / 2, s, s))
        p.end()

    def _hit(self, pos):
        chans = [c for c, _ in self.o.order()] if self.o.target == "all" else [self.o.target]
        best = None
        for c in chans:
            fs = self.o.factors[self.o.index(c)]
            for i in range(9):
                q = self.pt(i, fs[i])
                d = abs(q.x() - pos.x()) + abs(q.y() - pos.y())
                if d < 10 and (best is None or d < best[0]):
                    best = (d, c, i)
        return best

    def mousePressEvent(self, e):
        h = self._hit(e.position())
        if h is not None and self.isEnabled():
            self._drag = (h[1], h[2])
            self.o.editStarted.emit()

    def mouseMoveEvent(self, e):
        if self._drag is not None:
            c, i = self._drag
            self.o.move_knot(c, i, round(self.factor_at(e.position().y()), 3))

    def mouseReleaseEvent(self, e):
        self._drag = None


class CurveEditor(QWidget):
    edited = Signal(int, list, int)        # factor index (R/G/B, or state.CHANNELS), 9 levels, edited bin
    editedAll = Signal(list, int)          # same bin edit on all channels
    editStarted = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.factors = [[1.0] * 9 for _ in UI_CHANNELS]
        self.pinned = [set() for _ in UI_CHANNELS]
        self.channels = "RGB"
        self.target = "all"
        self.main_bin = None                 # bin of the profile's sample brightness (larger knot)
        v = QVBoxLayout(self)
        v.setContentsMargins(0, 0, 0, 0)
        self.plot = EqualizerPlot(self)
        v.addWidget(self.plot)
        row = QHBoxLayout()
        row.setSpacing(2)
        self.group = QButtonGroup(self)
        self.target_buttons = {}
        self._buttons = []
        for _ in range(4):
            b = QToolButton()
            b.setIconSize(QSize(18, 18))
            b.setToolButtonStyle(Qt.ToolButtonIconOnly)
            b.setFixedSize(26, 24)
            b.setCheckable(True)
            b.setStyleSheet("QToolButton{border:1px solid #3a3a3a; border-radius:3px; background:#6a6a6a}"
                            "QToolButton:checked{background:#6f6fd8}")
            self.group.addButton(b)
            b.clicked.connect(lambda _=False, b=b: self.set_target(b.property("key")))
            row.addWidget(b)
            self._buttons.append(b)
        self._label_buttons()
        self.target_buttons["all"].setChecked(True)
        self.buttons = row
        v.addLayout(row)

    def order(self):
        return ORDER[self.channels]

    def index(self, c):
        return dict(self.order())[c]

    def _label_buttons(self):
        """The toggles of the current channels: all, then one per channel in the panel order."""
        rgb = self.channels == "RGB"
        keys = [c for c, _ in self.order()]
        self.target_buttons = {}
        for b, key in zip(self._buttons, ["all"] + keys):
            b.setProperty("key", key)
            b.setText("RGB" if key == "all" and rgb else "All" if key == "all" else key)
            b.setIcon(all_icon(keys=keys) if key == "all" else QIcon(dot_pixmap(key, 18)))
            if rgb:
                b.setToolTip("Show the noise graphs (noise level vs brightness level) for all three color channels"
                             if key == "all" else
                             f"Show the noise graph (noise level vs brightness level) for the {NAMES[key]} channel alone")
            else:
                b.setToolTip("Show the noise graphs (noise level vs brightness level) for all three color channels\n"
                             "(repro: Luminance, Cr and Cb of a profile made before the R/G/B equalizer; the "
                             "genuine's channels are R, G, B)" if key == "all" else
                             f"Show the noise graph (noise level vs brightness level) for the {NAMES[key]} channel "
                             f"alone\n(repro: a profile made before the R/G/B equalizer; the genuine's buttons are "
                             f"Red, Green, Blue)")
            self.target_buttons[key] = b

    def set_target(self, key):
        self.target = key
        self.target_buttons[key].setChecked(True)
        self.plot.update()

    def channel(self):
        return 0 if self.target == "all" else self.index(self.target)

    def set_factors(self, factors, pinned, main_bin=None, channels="RGB"):
        if channels != self.channels:
            self.channels = channels
            self._label_buttons()
            self.set_target("all")
        self.factors = [list(f) for f in factors]
        self.pinned = [set(p) for p in pinned]
        self.main_bin = main_bin
        self.plot.update()

    def move_knot(self, c, i, f):
        f = min(max(f, FACTOR_MIN), FACTOR_MAX)
        fs = list(self.factors[self.index(c)])
        fs[i] = f
        if self.target == "all":
            self.editedAll.emit(fs, i)
        else:
            self.edited.emit(self.index(c), fs, i)
