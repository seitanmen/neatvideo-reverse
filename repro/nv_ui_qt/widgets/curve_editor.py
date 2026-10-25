"""Manual Fine-Tune: per-bin noise factors (9 genuine bins) of one channel.

The curve through the 9 handles is drawn as cubic Bezier segments (Catmull-Rom tangents) in a
QGraphicsScene; handles are dragged vertically, and the 9 sliders below mirror them."""
from PySide6.QtCore import QPointF, QRectF, Qt, Signal
from PySide6.QtGui import QBrush, QColor, QPainter, QPainterPath, QPen
from PySide6.QtWidgets import (QComboBox, QGraphicsEllipseItem, QGraphicsItem, QGraphicsPathItem,
                               QGraphicsScene, QGraphicsSimpleTextItem, QGraphicsView, QHBoxLayout, QLabel,
                               QSlider, QVBoxLayout, QWidget)

from ..state import CHANNELS, CURVE_X, FACTOR_MAX, FACTOR_MIN

W, H = 360.0, 150.0


def _y(f):
    return H - H * (f - FACTOR_MIN) / (FACTOR_MAX - FACTOR_MIN)


def _f(y):
    return FACTOR_MIN + (H - y) / H * (FACTOR_MAX - FACTOR_MIN)


class Handle(QGraphicsEllipseItem):
    def __init__(self, editor, i):
        super().__init__(-5, -5, 10, 10)
        self.editor, self.i = editor, i
        self.setBrush(QBrush(QColor(255, 210, 0)))
        self.setPen(QPen(Qt.black))
        self.setFlag(QGraphicsItem.ItemIsMovable)
        self.setFlag(QGraphicsItem.ItemSendsGeometryChanges)
        self.setCursor(Qt.SizeVerCursor)
        self.setToolTip(f"bin {i}: brightness {CURVE_X[i]:.0f}")
        self._sync = False

    def itemChange(self, change, value):
        if change == QGraphicsItem.ItemPositionChange:
            return QPointF(CURVE_X[self.i] / 256.0 * W, min(max(value.y(), 0.0), H))
        if change == QGraphicsItem.ItemPositionHasChanged and not self._sync:
            self.editor._moved(self.i, _f(self.pos().y()))
        return super().itemChange(change, value)

    def mousePressEvent(self, e):
        self.editor.editStarted.emit()
        super().mousePressEvent(e)


class CurveEditor(QWidget):
    edited = Signal(int, list, int)        # channel, 9 factors, edited bin
    editStarted = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.factors = [[1.0] * 9 for _ in CHANNELS]
        self.pinned = [set() for _ in CHANNELS]
        v = QVBoxLayout(self)
        top = QHBoxLayout()
        top.addWidget(QLabel("Component"))
        self.chan = QComboBox()
        self.chan.addItems([n for _, n in CHANNELS])
        self.chan.currentIndexChanged.connect(self._redraw)
        top.addWidget(self.chan)
        top.addStretch(1)
        v.addLayout(top)
        self.scene = QGraphicsScene(-30, -10, W + 40, H + 30)
        self.view = QGraphicsView(self.scene)
        self.view.setMinimumHeight(int(H + 40))
        self.view.setRenderHint(QPainter.Antialiasing)
        self.view.setBackgroundBrush(QColor(32, 32, 32))
        v.addWidget(self.view)
        grid = QPen(QColor(80, 80, 80))
        for f in (FACTOR_MIN, 1.0, 2.0, FACTOR_MAX):
            self.scene.addLine(0, _y(f), W, _y(f), grid)
            t = QGraphicsSimpleTextItem(f"{f:g}")
            t.setBrush(QColor(170, 170, 170))
            t.setPos(-28, _y(f) - 8)
            self.scene.addItem(t)
        self.path = QGraphicsPathItem()
        self.path.setPen(QPen(QColor(230, 230, 230), 2))
        self.scene.addItem(self.path)
        self.handles = [Handle(self, i) for i in range(9)]
        for h in self.handles:
            self.scene.addItem(h)
        row = QHBoxLayout()
        self.sliders = []
        for i in range(9):
            s = QSlider(Qt.Vertical)
            s.setRange(int(FACTOR_MIN * 100), int(FACTOR_MAX * 100))
            s.setFixedHeight(70)
            s.setToolTip(f"bin {i} (brightness {CURVE_X[i]:.0f}) noise factor")
            s.sliderPressed.connect(self.editStarted)
            s.valueChanged.connect(lambda val, i=i: self._slider(i, val))
            row.addWidget(s)
            self.sliders.append(s)
        v.addLayout(row)
        self._redraw()

    def channel(self):
        return self.chan.currentIndex()

    def set_factors(self, factors, pinned):
        self.factors = [list(f) for f in factors]
        self.pinned = [set(p) for p in pinned]
        self._redraw()

    def _redraw(self, *a):
        fs = self.factors[self.channel()]
        for i, (h, s) in enumerate(zip(self.handles, self.sliders)):
            h._sync = True
            h.setPos(CURVE_X[i] / 256.0 * W, _y(fs[i]))
            h._sync = False
            h.setBrush(QBrush(QColor(255, 120, 0) if i in self.pinned[self.channel()] else QColor(255, 210, 0)))
            s.blockSignals(True)
            s.setValue(int(round(fs[i] * 100)))
            s.blockSignals(False)
        pts = [QPointF(CURVE_X[i] / 256.0 * W, _y(fs[i])) for i in range(9)]
        path = QPainterPath(pts[0])
        for i in range(8):
            p0, p1, p2, p3 = pts[max(i - 1, 0)], pts[i], pts[i + 1], pts[min(i + 2, 8)]
            c1 = p1 + (p2 - p0) / 6.0
            c2 = p2 - (p3 - p1) / 6.0
            path.cubicTo(c1, c2, p2)
        self.path.setPath(path)

    def _moved(self, i, f):
        c = self.channel()
        fs = list(self.factors[c])
        fs[i] = round(min(max(f, FACTOR_MIN), FACTOR_MAX), 3)
        self.edited.emit(c, fs, i)

    def _slider(self, i, val):
        c = self.channel()
        fs = list(self.factors[c])
        fs[i] = val / 100.0
        self.edited.emit(c, fs, i)
