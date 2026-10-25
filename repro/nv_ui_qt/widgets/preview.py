"""Preview: Original / Filtered / Split / Assist, zoom, frame navigation, sample-area selection."""
import numpy as np
from PySide6.QtCore import QPoint, QRect, QSize, Qt, QTimer, Signal
from PySide6.QtGui import QColor, QImage, QPainter, QPen, QPixmap
from PySide6.QtWidgets import (QButtonGroup, QCheckBox, QComboBox, QHBoxLayout, QLabel, QPushButton,
                               QRadioButton, QScrollArea, QSizePolicy, QSlider, QVBoxLayout, QWidget)

MODES = ("Original", "Filtered", "Split", "Assist")


def to_qimage(rgb):
    a = np.ascontiguousarray(np.clip(rgb, 0, 255).astype(np.uint8))
    h, w = a.shape[:2]
    return QImage(a.data, w, h, 3 * w, QImage.Format_RGB888).copy()


class ImageLabel(QLabel):
    """Shows the composed preview; a left-button drag selects a square in display pixels."""
    dragged = Signal(QRect)

    def __init__(self):
        super().__init__()
        self.setAlignment(Qt.AlignLeft | Qt.AlignTop)
        self.setMouseTracking(False)
        self._p0 = None
        self._rect = None
        self.sel = None            # QRect (display pixels of the scaled image) drawn as overlay
        self.region = None         # filtered sub-region (Preview Area), display pixels

    def mousePressEvent(self, e):
        if e.button() == Qt.LeftButton:
            self._p0 = e.position().toPoint()
            self._rect = None

    def mouseMoveEvent(self, e):
        if self._p0 is not None:
            p = e.position().toPoint()
            d = max(abs(p.x() - self._p0.x()), abs(p.y() - self._p0.y()))
            x = self._p0.x() if p.x() >= self._p0.x() else self._p0.x() - d
            y = self._p0.y() if p.y() >= self._p0.y() else self._p0.y() - d
            self._rect = QRect(x, y, d, d)
            self.update()

    def mouseReleaseEvent(self, e):
        if self._p0 is not None and self._rect is not None and self._rect.width() >= 4:
            self.dragged.emit(self._rect)
        self._p0 = None
        self._rect = None
        self.update()

    def paintEvent(self, e):
        super().paintEvent(e)
        p = QPainter(self)
        if self.region is not None:
            p.setPen(QPen(QColor(80, 160, 255), 1, Qt.DashLine))
            p.drawRect(self.region)
        for r, col in ((self.sel, QColor(255, 210, 0)), (self._rect, QColor(255, 80, 80))):
            if r is not None:
                p.setPen(QPen(col, 2))
                p.drawRect(r)
        p.end()


class PreviewWidget(QWidget):
    frameChanged = Signal(int)
    areaSelected = Signal(int, int, int)       # y0, x0, size in coded luma pixels
    modeChanged = Signal(str)
    previewAreaToggled = Signal(bool)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.orig = self.filt = None           # display-oriented uint8 RGB
        self.region = None                     # (y0, x0, h, w) of filt inside the frame (coded coords)
        self.rot = 0
        self.coded_shape = None
        self.scale = 1.0
        self.n = 0
        v = QVBoxLayout(self)
        bar = QHBoxLayout()
        self.group = QButtonGroup(self)
        for i, m in enumerate(MODES):
            b = QRadioButton(m)
            b.setToolTip({"Assist": "Assist Image (repro): removed component (original - filtered) x4 "
                                    "around mid grey"}.get(m, m))
            self.group.addButton(b, i)
            bar.addWidget(b)
        self.group.button(1).setChecked(True)
        self.group.idToggled.connect(self._mode)
        bar.addWidget(QLabel("Split"))
        self.split = QSlider(Qt.Horizontal)
        self.split.setRange(0, 100)
        self.split.setValue(50)
        self.split.setMaximumWidth(120)
        self.split.valueChanged.connect(self.compose)
        bar.addWidget(self.split)
        bar.addWidget(QLabel("Zoom"))
        self.zoom = QComboBox()
        self.zoom.addItems(["Fit", "50%", "100%", "200%"])
        self.zoom.currentTextChanged.connect(self.compose)
        bar.addWidget(self.zoom)
        self.area_cb = QCheckBox("Preview Area")
        self.area_cb.setToolTip("Filter only the central part of the frame (faster preview)")
        self.area_cb.toggled.connect(self.previewAreaToggled)
        bar.addWidget(self.area_cb)
        bar.addStretch(1)
        v.addLayout(bar)
        self.label = ImageLabel()
        self.label.dragged.connect(self._dragged)
        self.scroll = QScrollArea()
        self.scroll.setWidget(self.label)
        self.scroll.setWidgetResizable(False)
        self.scroll.setAlignment(Qt.AlignCenter)
        self.scroll.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.scroll.setStyleSheet("background:#202020")
        v.addWidget(self.scroll, 1)
        self.hint = QLabel("Drag in the preview to select the noise sample area (Build Profile)")
        self.hint.setStyleSheet("color:gray")
        v.addWidget(self.hint)
        nav = QHBoxLayout()
        self.play = QPushButton("▶")
        self.play.setCheckable(True)
        self.play.setFixedWidth(40)
        self.play.toggled.connect(self._play)
        self.prev = QPushButton("◀")
        self.next = QPushButton("▶|")
        for b in (self.prev, self.next):
            b.setFixedWidth(40)
        self.prev.clicked.connect(lambda: self.set_frame(self.slider.value() - 1, emit=True))
        self.next.clicked.connect(lambda: self.set_frame(self.slider.value() + 1, emit=True))
        self.slider = QSlider(Qt.Horizontal)
        self.slider.setRange(0, 0)
        self.slider.valueChanged.connect(self._seek)
        self.flabel = QLabel("f 0 / 0")
        self.flabel.setMinimumWidth(90)
        nav.addWidget(self.play)
        nav.addWidget(self.prev)
        nav.addWidget(self.slider, 1)
        nav.addWidget(self.next)
        nav.addWidget(self.flabel)
        v.addLayout(nav)
        self.timer = QTimer(self)
        self.timer.timeout.connect(lambda: self.set_frame((self.slider.value() + 1) % max(self.n, 1), emit=True))
        self.fps = 24.0

    # ---- state ----
    def mode(self):
        return MODES[self.group.checkedId()]

    def set_mode(self, m):
        self.group.button(MODES.index(m)).setChecked(True)

    def set_count(self, n, fps=24.0):
        self.n = n
        self.fps = fps
        self.slider.blockSignals(True)
        self.slider.setRange(0, max(n - 1, 0))
        self.slider.setValue(0)
        self.slider.blockSignals(False)
        self.flabel.setText(f"f 0 / {n}")

    def frame(self):
        return self.slider.value()

    def set_frame(self, i, emit=False):
        i = max(0, min(int(i), max(self.n - 1, 0)))
        self.slider.blockSignals(not emit)
        self.slider.setValue(i)
        self.slider.blockSignals(False)
        self.flabel.setText(f"f {i} / {self.n}")

    def playing(self):
        return self.play.isChecked()

    def _play(self, on):
        self.play.setText("■" if on else "▶")
        if on:
            self.timer.start(int(1000 / max(self.fps, 1.0)))
        else:
            self.timer.stop()

    def _seek(self, i):
        self.flabel.setText(f"f {i} / {self.n}")
        self.frameChanged.emit(i)

    def _mode(self, i, checked):
        if checked:
            self.compose()
            self.modeChanged.emit(MODES[i])

    # ---- images ----
    def set_images(self, orig, filt=None, region=None, rot=0):
        """orig: coded-orientation RGB of the full frame; filt: RGB of the full frame or of
        `region` (y0, x0, h, w); None = not computed yet."""
        self.rot = rot % 4
        self.coded_shape = orig.shape[:2]
        if filt is not None and region is not None:
            y0, x0, h, w = region
            full = np.array(orig, copy=True)
            full[y0:y0 + h, x0:x0 + w] = filt[:h, :w]
            filt = full
        self.region = region
        self.orig = np.rot90(orig, self.rot)
        self.filt = None if filt is None else np.rot90(filt, self.rot)
        self.compose()

    def set_selection(self, area):
        """area (y0, x0, size) in coded coordinates, or None."""
        self._sel_area = area
        self.compose()

    def _display_rect(self, y0, x0, h, w):
        H, W = self.coded_shape
        pts = np.array([[y0, x0], [y0 + h, x0 + w]], np.float64)
        # coded -> display for rot90 counter-clockwise k times
        out = []
        for y, x in pts:
            for _ in range(self.rot):
                y, x = W - x, y
                H, W = W, H
            H, W = self.coded_shape
            out.append((y, x))
        (ya, xa), (yb, xb) = out
        r = QRect(int(min(xa, xb) * self.scale), int(min(ya, yb) * self.scale),
                  int(abs(xb - xa) * self.scale), int(abs(yb - ya) * self.scale))
        return r

    def _dragged(self, r):
        if self.coded_shape is None:
            return
        H, W = self.coded_shape
        dh, dw = (W, H) if self.rot % 2 else (H, W)
        x0 = int(r.x() / self.scale)
        y0 = int(r.y() / self.scale)
        s = int(r.width() / self.scale)
        x0, y0 = max(0, min(x0, dw - 1)), max(0, min(y0, dh - 1))
        s = max(8, min(s, dw - x0, dh - y0))
        idx = np.rot90(np.indices((H, W)), self.rot, axes=(1, 2))
        ys = idx[0][y0:y0 + s, x0:x0 + s]
        xs = idx[1][y0:y0 + s, x0:x0 + s]
        self.areaSelected.emit(int(ys.min()), int(xs.min()), int(s))

    def compose(self, *a):
        if self.orig is None:
            return
        m = self.mode()
        o = self.orig
        f = self.filt
        if m == "Original" or f is None:
            img = o
        elif m == "Filtered":
            img = f
        elif m == "Split":
            x = int(o.shape[1] * self.split.value() / 100)
            img = np.concatenate([o[:, :x], f[:, x:]], axis=1)
        else:
            d = o.astype(np.float32) - f.astype(np.float32)
            img = np.clip(128 + 4 * d.mean(axis=2, keepdims=True).repeat(3, axis=2), 0, 255)
        qi = to_qimage(img)
        z = self.zoom.currentText()
        if z == "Fit":
            vs = self.scroll.viewport().size()
            self.scale = min(vs.width() / qi.width(), vs.height() / qi.height()) if qi.width() else 1.0
            self.scale = max(self.scale, 0.05)
        else:
            self.scale = float(z.rstrip("%")) / 100.0
        pm = QPixmap.fromImage(qi)
        if self.scale != 1.0:
            pm = pm.scaled(QSize(max(1, int(qi.width() * self.scale)), max(1, int(qi.height() * self.scale))),
                           Qt.IgnoreAspectRatio, Qt.SmoothTransformation)
        if m == "Split" and f is not None:
            p = QPainter(pm)
            p.setPen(QPen(QColor(255, 255, 255), 1))
            x = int(pm.width() * self.split.value() / 100)
            p.drawLine(QPoint(x, 0), QPoint(x, pm.height()))
            p.end()
        self.label.setPixmap(pm)
        self.label.resize(pm.size())
        self.label.region = self._display_rect(*self.region) if self.region is not None else None
        sel = getattr(self, "_sel_area", None)
        self.label.sel = self._display_rect(sel[0], sel[1], sel[2], sel[2]) if sel else None
        self.label.update()

    def resizeEvent(self, e):
        super().resizeEvent(e)
        if self.zoom.currentText() == "Fit":
            self.compose()
