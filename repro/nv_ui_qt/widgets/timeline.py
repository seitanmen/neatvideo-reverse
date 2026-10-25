"""Bottom row: frame thumbnails (current frame = blue frame, the temporal filter's window = orange,
click = go to that frame) and, because the reproduction runs stand-alone without a host timeline,
a thin timeline slider with the frame number (repro)."""
from PySide6.QtCore import QRectF, QSize, Qt, QTimer, Signal
from PySide6.QtGui import QColor, QPainter, QPen
from PySide6.QtWidgets import QHBoxLayout, QLabel, QSlider, QVBoxLayout, QWidget

N_THUMBS = 11


class ThumbStrip(QWidget):
    clickedFrame = Signal(int)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.n, self.cur, self.radius = 0, 0, 0
        self.provider = None                   # callable(i) -> QImage
        self.setFixedHeight(40)
        self.setMinimumWidth(N_THUMBS * 30)

    def sizeHint(self):
        return QSize(N_THUMBS * 60, 40)

    def set_state(self, n, cur, radius):
        self.n, self.cur, self.radius = n, cur, radius
        self.update()

    def slots(self):
        """[(frame index or None, QRectF)] of the 11 thumbnails, the current frame in the middle."""
        h = self.height() - 6
        w = min(h * 16 / 9, (self.width() - 4) / N_THUMBS - 6)
        x0 = 3.0
        out = []
        for k in range(N_THUMBS):
            i = self.cur - N_THUMBS // 2 + k
            out.append((i if 0 <= i < self.n else None, QRectF(x0 + k * (w + 6), 3, w, h)))
        return out

    def frame_color(self, i):
        if i == self.cur:
            return QColor(40, 120, 230)
        if abs(i - self.cur) <= self.radius:
            return QColor(240, 130, 40)
        return None

    def paintEvent(self, e):
        p = QPainter(self)
        p.setRenderHint(QPainter.SmoothPixmapTransform)
        for i, r in self.slots():
            if i is None:
                continue
            img = self.provider(i) if self.provider else None
            if img is not None:
                p.drawImage(r, img)
            else:
                p.fillRect(r, QColor(90, 90, 90))
            col = self.frame_color(i)
            if col is not None:
                p.setPen(QPen(col, 3))
                p.drawRect(r.adjusted(-1.5, -1.5, 1.5, 1.5))
        p.end()

    def mousePressEvent(self, e):
        for i, r in self.slots():
            if i is not None and r.adjusted(-3, -3, 3, 3).contains(e.position()):
                self.clickedFrame.emit(i)
                return


class Timeline(QWidget):
    """Thin timeline slider + frame number above the thumbnail strip; owns playback."""
    frameChanged = Signal(int)

    def __init__(self, parent=None):
        super().__init__(parent)
        v = QVBoxLayout(self)
        v.setContentsMargins(0, 0, 0, 0)
        v.setSpacing(1)
        row = QHBoxLayout()
        row.setContentsMargins(0, 0, 0, 0)
        self.slider = QSlider(Qt.Horizontal)
        self.slider.setRange(0, 0)
        self.slider.setFixedHeight(14)
        self.slider.setToolTip("Timeline (repro: the genuine plug-in follows the host's timeline)")
        self.label = QLabel("Frame 0 / 0")
        self.label.setStyleSheet("color:#606060")
        self.label.setMinimumWidth(110)
        row.addWidget(self.slider, 1)
        row.addWidget(self.label)
        v.addLayout(row)
        self.thumbs = ThumbStrip()
        v.addWidget(self.thumbs)
        self.n, self.fps = 0, 24.0
        self.slider.valueChanged.connect(self._seek)
        self.thumbs.clickedFrame.connect(lambda i: self.set_frame(i, emit=True))
        self.timer = QTimer(self)
        self.timer.timeout.connect(lambda: self.set_frame((self.slider.value() + 1) % max(self.n, 1), emit=True))

    def set_count(self, n, fps=24.0):
        self.n, self.fps = n, fps
        self.slider.blockSignals(True)
        self.slider.setRange(0, max(n - 1, 0))
        self.slider.setValue(0)
        self.slider.blockSignals(False)
        self._update(0)

    def frame(self):
        return self.slider.value()

    def set_frame(self, i, emit=False):
        i = max(0, min(int(i), max(self.n - 1, 0)))
        self.slider.blockSignals(not emit)
        self.slider.setValue(i)
        self.slider.blockSignals(False)
        self._update(i)

    def set_radius(self, r):
        self.thumbs.radius = r
        self.thumbs.update()

    def _update(self, i):
        self.label.setText(f"Frame {i} / {self.n}")
        self.thumbs.set_state(self.n, i, self.thumbs.radius)

    def _seek(self, i):
        self._update(i)
        self.frameChanged.emit(i)

    def playing(self):
        return self.timer.isActive()

    def set_playing(self, on):
        if on:
            self.timer.start(int(1000 / max(self.fps, 1.0)))
        else:
            self.timer.stop()
