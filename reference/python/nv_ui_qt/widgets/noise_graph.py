"""Noise graph: brightness vs noise level of the profile (Y / Cb / Cr), drawn with QPainter."""
from PySide6.QtCore import QPointF, QRectF, Qt
from PySide6.QtGui import QColor, QPainter, QPainterPath, QPen
from PySide6.QtWidgets import QWidget

COLORS = {"Y": QColor(230, 230, 230), "Cr": QColor(236, 60, 60), "Cb": QColor(150, 90, 240)}   # genuine dots


class NoiseGraph(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.prof = None
        self.base = None
        self.setMinimumSize(260, 150)

    def set_profile(self, prof, base=None):
        self.prof, self.base = prof, base
        self.update()

    def paintEvent(self, e):
        p = QPainter(self)
        p.setRenderHint(QPainter.Antialiasing)
        r = QRectF(self.rect()).adjusted(34, 8, -8, -20)
        p.fillRect(self.rect(), QColor(32, 32, 32))
        p.setPen(QColor(90, 90, 90))
        p.drawRect(r)
        if not self.prof:
            p.setPen(QColor(160, 160, 160))
            p.drawText(r, Qt.AlignCenter, "no profile")
            p.end()
            return
        xs = self.prof["curve_centers"]
        curves = [self.prof["curve"]]
        if self.base is not None and self.base is not self.prof:
            curves.append(self.base["curve"])
        vmax = max(max(max(c[n]) for n in COLORS) for c in curves) * 1.1 or 1.0

        def pt(x, y):
            return QPointF(r.left() + r.width() * x / 255.0, r.bottom() - r.height() * y / vmax)

        p.setPen(QColor(150, 150, 150))
        for k in range(5):
            y = vmax * k / 4
            q = pt(0, y)
            p.drawText(QRectF(0, q.y() - 7, 32, 14), Qt.AlignRight | Qt.AlignVCenter, f"{y:.1f}")
        for x in (0, 64, 128, 192, 255):
            q = pt(x, 0)
            p.drawText(QRectF(q.x() - 16, r.bottom() + 2, 32, 16), Qt.AlignCenter, str(x))
        for i, c in enumerate(curves):
            for n, col in COLORS.items():
                path = QPainterPath()
                for j, (x, y) in enumerate(zip(xs, c[n])):
                    (path.moveTo if j == 0 else path.lineTo)(pt(x, y))
                pen = QPen(col, 2 if i == 0 else 1, Qt.SolidLine if i == 0 else Qt.DashLine)
                p.setPen(pen)
                p.drawPath(path)
        p.setPen(QColor(200, 200, 200))
        p.drawText(r.adjusted(4, 2, -4, -2), Qt.AlignTop | Qt.AlignRight,
                   "Y / Cr / Cb noise sigma vs brightness" + ("  (dashed: before tuning)" if len(curves) > 1 else ""))
        p.end()
