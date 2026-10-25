"""Noise profile equalizer (Manual Fine-Tune) in the genuine look: the per-brightness noise factors of
the three components drawn together (Luminance white, Cr red, Cb purple; the genuine shows R / G /
B, the reproduction's factors are Y / Cb / Cr), vertical axis (factor - 1) x 100% from -100% to
+200%, a black -> white brightness bar under the horizontal axis. Knots edited by hand are filled
with the component's colour, the others yellow. The edit target is All or one component."""
from PySide6.QtCore import QPointF, QRectF, Qt, Signal
from PySide6.QtGui import QBrush, QColor, QLinearGradient, QPainter, QPen
from PySide6.QtWidgets import QButtonGroup, QHBoxLayout, QSizePolicy, QToolButton, QVBoxLayout, QWidget

from ..state import CH_INDEX, CURVE_X, FACTOR_MAX, FACTOR_MIN, UI_CHANNELS
from .rows import CH_COLORS

D_MIN, D_MAX = -100.0, 200.0


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
        p.fillRect(self.rect(), QColor(244, 244, 244))
        r = self.plot_rect()
        p.fillRect(r, QColor(60, 60, 60))
        p.setPen(QPen(QColor(110, 110, 110), 1, Qt.DotLine))
        for i in range(9):
            x = self.pt(i, 1.0).x()
            p.drawLine(QPointF(x, r.top()), QPointF(x, r.bottom()))
        for d in (200, 100, 0, -100):
            y = r.top() + r.height() * (D_MAX - d) / (D_MAX - D_MIN)
            p.setPen(QPen(QColor(200, 200, 200) if d == 0 else QColor(110, 110, 110), 1,
                          Qt.SolidLine if d == 0 else Qt.DotLine))
            p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y))
            p.setPen(QColor(60, 60, 60))
            p.drawText(QRectF(0, y - 7, 45, 14), Qt.AlignRight | Qt.AlignVCenter, f"{d:+d}%")
        g = QLinearGradient(r.left(), 0, r.right(), 0)
        g.setColorAt(0, QColor(0, 0, 0))
        g.setColorAt(1, QColor(255, 255, 255))
        p.fillRect(QRectF(r.left(), r.bottom() + 5, r.width(), 8), QBrush(g))
        tgt = self.o.target
        order = [c for c, _ in UI_CHANNELS]
        order = [c for c in order if c != tgt] + ([tgt] if tgt in order else [])
        for c in order:
            fs = self.o.factors[CH_INDEX[c]]
            pts = [self.pt(i, fs[i]) for i in range(9)]
            active = tgt in ("all", c)
            p.setPen(QPen(CH_COLORS[c], 2 if active else 1))
            for a, b in zip(pts, pts[1:]):
                p.drawLine(a, b)
            for i, q in enumerate(pts):
                pinned = i in self.o.pinned[CH_INDEX[c]]
                p.setPen(QPen(QColor(20, 20, 20), 1))
                p.setBrush(QBrush(CH_COLORS[c] if pinned else QColor(250, 220, 0)))
                s = 7 if active else 5
                p.drawRect(QRectF(q.x() - s / 2, q.y() - s / 2, s, s))
        p.end()

    def _hit(self, pos):
        chans = [c for c, _ in UI_CHANNELS] if self.o.target == "all" else [self.o.target]
        best = None
        for c in chans:
            fs = self.o.factors[CH_INDEX[c]]
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
    edited = Signal(int, list, int)        # channel index (state.CHANNELS), 9 factors, edited bin
    editedAll = Signal(list, int)          # same bin edit on all components
    editStarted = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.factors = [[1.0] * 9 for _ in UI_CHANNELS]
        self.pinned = [set() for _ in UI_CHANNELS]
        self.target = "all"
        v = QVBoxLayout(self)
        v.setContentsMargins(0, 0, 0, 0)
        self.plot = EqualizerPlot(self)
        v.addWidget(self.plot)
        row = QHBoxLayout()
        row.setSpacing(2)
        self.group = QButtonGroup(self)
        self.target_buttons = {}
        for key, text in (("all", "All"), ("Y", "Y"), ("Cr", "Cr"), ("Cb", "Cb")):
            b = QToolButton()
            b.setText(text)
            b.setCheckable(True)
            b.setToolTip("Edit all components together" if key == "all" else f"Edit the {text} component")
            if key != "all":
                b.setStyleSheet(f"QToolButton:checked{{background:{CH_COLORS[key].name()}; color:black}}")
            self.group.addButton(b)
            b.clicked.connect(lambda _=False, key=key: self.set_target(key))
            row.addWidget(b)
            self.target_buttons[key] = b
        self.target_buttons["all"].setChecked(True)
        self.buttons = row
        v.addLayout(row)

    def set_target(self, key):
        self.target = key
        self.target_buttons[key].setChecked(True)
        self.plot.update()

    def channel(self):
        return 0 if self.target == "all" else CH_INDEX[self.target]

    def set_factors(self, factors, pinned):
        self.factors = [list(f) for f in factors]
        self.pinned = [set(p) for p in pinned]
        self.plot.update()

    def move_knot(self, c, i, f):
        f = min(max(f, FACTOR_MIN), FACTOR_MAX)
        fs = list(self.factors[CH_INDEX[c]])
        fs[i] = f
        if self.target == "all":
            self.editedAll.emit(fs, i)
        else:
            self.edited.emit(CH_INDEX[c], fs, i)
