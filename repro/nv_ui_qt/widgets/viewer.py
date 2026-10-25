"""Image viewer in the genuine layout: a primary pane plus three component panes (YCrCb + Enhanced
Contrast by default) arranged 2 x 2 / 1+3 Bottom / 1+3 Right, sharing zoom and pan; "Filtered" in
each pane's corner while the filtered preview is shown, the original while the left button is held.

Component panes (repro definitions, display only): components are computed from the displayed RGB
with the stream's matrix (Y = kr R + kg G + kb B, Cb / Cr = scaled B - Y / R - Y around 128).
"Enhanced" stretches each component linearly so that its 1% .. 99% percentiles of the original frame
span 0 .. 255 (the same stretch is used for the filtered frame, so the two compare directly).
"Frequencies" panes show Y / Cr / Cb High = c - box3(c), Mid = box3 - box9, Low = box9 - box27
around mid grey, scaled so that the original band's RMS maps to 40 levels.
"""
import numpy as np
from PySide6.QtCore import QPointF, QRectF, Qt, Signal
from PySide6.QtGui import QColor, QFont, QImage, QPainter, QPen
from PySide6.QtWidgets import (QComboBox, QFrame, QHBoxLayout, QLabel, QSizePolicy, QSlider, QToolButton,
                               QWidget)
from scipy.ndimage import uniform_filter

ZOOMS = [5, 10, 12.5, 16.7, 20, 25, 33.3, 50, 100, 200, 300, 400, 800]
MODES = [("Normal", "Normal"), ("YCrCb +", "YCrCb + Enhanced Contrast (Default)"),
         ("RGB", "RGB  (Red, Green, Blue)"), ("YCrCb", "YCrCb  (Luminance, Chrominance)"),
         ("Y Freq", "Y Channel Frequencies"), ("Cr Freq", "Cr Channel Frequencies"),
         ("Cb Freq", "Cb Channel Frequencies")]
SPLITS = ["2 x 2", "1+3 Bottom", "1+3 Right"]
PANES = {"Normal": [], "YCrCb +": ["Y+", "Cr+", "Cb+"], "RGB": ["R", "G", "B"], "YCrCb": ["Y", "Cr", "Cb"],
         "Y Freq": ["Y High", "Y Mid", "Y Low"], "Cr Freq": ["Cr High", "Cr Mid", "Cr Low"],
         "Cb Freq": ["Cb High", "Cb Mid", "Cb Low"]}
PANE_LABEL = {"Y+": "Y Enhanced", "Cr+": "Cr Enhanced", "Cb+": "Cb Enhanced"}
TINT = {"Y": (1.0, 1.0, 1.0), "Cr": (1.0, 0.80, 0.82), "Cb": (0.86, 0.84, 1.0),
        "R": (1.0, 0.78, 0.78), "G": (0.80, 1.0, 0.80), "B": (0.80, 0.82, 1.0)}
KRB = {"bt709": (0.2126, 0.0722), "bt601": (0.299, 0.114), "bt2020": (0.2627, 0.0593)}
FREQ = {"High": (0, 3), "Mid": (3, 9), "Low": (9, 27)}


def to_qimage(rgb):
    a = np.ascontiguousarray(np.clip(rgb, 0, 255).astype(np.uint8))
    h, w = a.shape[:2]
    return QImage(a.data, w, h, 3 * w, QImage.Format_RGB888).copy()


def components(rgb, matrix):
    kr, kb = KRB.get(matrix, KRB["bt709"])
    f = np.asarray(rgb, np.float32)
    R, G, B = f[..., 0], f[..., 1], f[..., 2]
    Y = kr * R + (1 - kr - kb) * G + kb * B
    return {"R": R, "G": G, "B": B, "Y": Y, "Cb": (B - Y) / (2 * (1 - kb)) + 128.0,
            "Cr": (R - Y) / (2 * (1 - kr)) + 128.0}


def _band(c, name):
    a, b = FREQ[name]
    lo = uniform_filter(c, b) if b else c
    hi = uniform_filter(c, a) if a else c
    return hi - lo


class Viewer(QWidget):
    areaSelected = Signal(int, int, int, int)      # y0, x0, h, w in coded luma pixels
    clicked = Signal()                             # press + release without dragging
    holdChanged = Signal(bool)
    cursorText = Signal(str)
    viewChanged = Signal()                         # mode / split / zoom changed

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumSize(320, 240)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.setMouseTracking(True)
        self.setFocusPolicy(Qt.StrongFocus)
        self.orig = self.filt = None               # display-oriented uint8 RGB
        self.region = None
        self.rot = 0
        self.coded_shape = None
        self.matrix = "bt709"
        self._mode, self._split, self._zoom = "YCrCb +", "1+3 Bottom", "fit"
        self.center = None
        self.brightness = 0
        self.show_filtered = False
        self.assist = False
        self.hold = False
        self.pan_mode = False
        self.square = True
        self.sel = None                            # coded (y0, x0, h, w)
        self.sel_style = "profile"                 # "profile" (green square + label) / "preview"
        self.sel_label = ""
        self.prof_region = None
        self._cache = {}
        self._stats = {}
        self._p0 = self._drag = self._pan0 = None

    # ---------------- state ----------------
    def mode(self):
        return self._mode

    def set_mode(self, m):
        if m in PANES and m != self._mode:
            self._mode = m
            self.update()
            self.viewChanged.emit()

    def split(self):
        return self._split

    def set_split(self, s):
        if s in SPLITS and s != self._split:
            self._split = s
            self.update()
            self.viewChanged.emit()

    def zoom(self):
        return self._zoom

    def set_zoom(self, z):
        """'fit' or a percentage of ZOOMS."""
        if z != self._zoom:
            self._zoom = z
            if z == "fit":
                self.center = None
            self.update()
            self.viewChanged.emit()

    def zoom_step(self, d):
        s = self._scale() * 100.0
        if d > 0:
            nxt = [z for z in ZOOMS if z > s * 1.001]
            self.set_zoom(nxt[0] if nxt else ZOOMS[-1])
        else:
            nxt = [z for z in ZOOMS if z < s * 0.999]
            self.set_zoom(nxt[-1] if nxt else ZOOMS[0])

    def set_brightness(self, b):
        self.brightness = int(b)
        self._cache = {k: v for k, v in self._cache.items() if k[1] != "rgb"}
        self.update()

    def set_matrix(self, m):
        self.matrix = m

    def set_show_filtered(self, on):
        if on != self.show_filtered:
            self.show_filtered = on
            self.update()

    def set_assist(self, on):
        self.assist = bool(on)
        self.update()

    def showing_filtered(self):
        return self.show_filtered and self.filt is not None and not self.hold

    def set_selection(self, rect, style="profile", label=""):
        """rect = coded (y0, x0, h, w) or None."""
        self.sel, self.sel_style, self.sel_label = rect, style, label
        self.update()

    def set_profile_region(self, rect):
        self.prof_region = rect
        self.update()

    # ---------------- images ----------------
    def set_images(self, orig, filt=None, region=None, rot=0):
        """orig: coded-orientation RGB of the full frame; filt: RGB of the full frame or of
        `region` (y0, x0, h, w); None = not computed yet."""
        new_orig = self.orig is None or self.coded_shape != orig.shape[:2] or rot % 4 != self.rot or \
            not np.array_equal(np.rot90(orig, rot % 4), self.orig)
        self.rot = rot % 4
        self.coded_shape = orig.shape[:2]
        if filt is not None and region is not None:
            y0, x0, h, w = region
            full = np.array(orig, copy=True)
            full[y0:y0 + h, x0:x0 + w] = filt[:h, :w]
            filt = full
        self.region = region
        if new_orig:
            self.orig = np.ascontiguousarray(np.rot90(orig, self.rot))
            self._stats = {}
        self.filt = None if filt is None else np.ascontiguousarray(np.rot90(filt, self.rot))
        self._cache = {}
        self.update()

    def _source(self):
        """'filt' / 'orig' / 'assist' for the primary pane."""
        if not self.showing_filtered():
            return "orig"
        return "assist" if self.assist else "filt"

    def _comps(self, src):
        key = ("comps", src)
        if key not in self._cache:
            self._cache[key] = components(self.orig if src == "orig" else self.filt, self.matrix)
        return self._cache[key]

    def _stat(self, name):
        """Display stretch of a component from the original frame (see module doc)."""
        if name not in self._stats:
            oc = self._comps("orig")
            if name.endswith("+"):
                c = oc[name[:-1]][::2, ::2]
                lo, hi = np.percentile(c, [1, 99])
                self._stats[name] = (float(lo), float(max(hi - lo, 1.0)))
            else:
                ch, band = name.split()
                rms = float(np.sqrt(np.mean(_band(oc[ch], band)[::2, ::2] ** 2)))
                self._stats[name] = 40.0 / max(rms, 1e-3)
        return self._stats[name]

    def pane_image(self, name, src):
        """QImage of a pane; name 'rgb' = primary pane."""
        key = (src, name)
        if key in self._cache:
            return self._cache[key]
        if name == "rgb":
            if src == "assist":
                # genuine Noise Level Check mask: shows the filtered (noise-reduced) result
                # so the user can judge if Noise Level is correct while adjusting sliders.
                # Split view: left = original, right = filtered (the mask boundary is the split).
                if self.split() != "off" and self.orig is not None and self.filt is not None:
                    sw = self.orig.shape[1] // 2
                    img = self.orig.copy()
                    img[:, sw:] = self.filt[:, sw:]
                else:
                    img = self.filt
            else:
                img = self.orig if src == "orig" else self.filt
                if self.brightness:
                    img = np.clip(img.astype(np.float32) * 2.0 ** (self.brightness / 100.0), 0, 255)
        else:
            cs = self._comps("orig" if src == "orig" else "filt")
            if name.endswith("+"):
                lo, rng = self._stat(name)
                g = (cs[name[:-1]] - lo) * (255.0 / rng)
                ch = name[:-1]
            elif " " in name:
                ch, band = name.split()
                g = 128.0 + _band(cs[ch], band) * self._stat(name)
            else:
                ch, g = name, cs[name]
            g = np.clip(g, 0, 255)
            img = g[..., None] * np.asarray(TINT[ch], np.float32)
        qi = to_qimage(img)
        self._cache[key] = qi
        return qi

    def composite(self):
        """The primary pane's image as uint8 RGB (what Save Preview PNG writes)."""
        qi = self.pane_image("rgb", self._source())
        ptr = qi.constBits()
        return np.frombuffer(ptr, np.uint8).reshape(qi.height(), qi.bytesPerLine())[:, :3 * qi.width()] \
            .reshape(qi.height(), qi.width(), 3).copy()

    # ---------------- geometry ----------------
    def panes(self):
        """[(name, QRectF)] in widget coordinates; the first is the primary pane."""
        r = QRectF(self.rect())
        comps = PANES[self._mode]
        if not comps:
            return [("rgb", r)]
        g = 3.0
        W, H = r.width(), r.height()
        if self._split == "1+3 Bottom":
            top = (H - g) * 2 / 3
            w3 = (W - 2 * g) / 3
            out = [("rgb", QRectF(0, 0, W, top))]
            out += [(c, QRectF(k * (w3 + g), top + g, w3, H - top - g)) for k, c in enumerate(comps)]
        elif self._split == "1+3 Right":
            left = (W - g) * 2 / 3
            h3 = (H - 2 * g) / 3
            out = [("rgb", QRectF(0, 0, left, H))]
            out += [(c, QRectF(left + g, k * (h3 + g), W - left - g, h3)) for k, c in enumerate(comps)]
        else:
            w2, h2 = (W - g) / 2, (H - g) / 2
            cells = [QRectF(0, 0, w2, h2), QRectF(w2 + g, 0, w2, h2), QRectF(0, h2 + g, w2, h2),
                     QRectF(w2 + g, h2 + g, w2, h2)]
            out = list(zip(["rgb"] + comps, cells))
        return out

    def _scale(self):
        if self.orig is None:
            return 1.0
        if self._zoom == "fit":
            pr = self.panes()[0][1]
            H, W = self.orig.shape[:2]
            return max(min(pr.width() / W, pr.height() / H), 0.01)
        return float(self._zoom) / 100.0

    def _center(self):
        H, W = self.orig.shape[:2]
        return self.center if self.center is not None else (W / 2.0, H / 2.0)

    def _pane_center(self, name, pr=None):
        """Component panes of the Prepare page centre on the selection (else the profile area), as the
        genuine shows the sample box in all panes; otherwise all panes share the primary's centre."""
        if name != "rgb" and self.sel_style == "profile":
            r = self.sel if self.sel is not None else self.prof_region
            if r is not None:
                x, y, w, h = self.coded_to_display(r)
                cx, cy = x + w / 2.0, y + h / 2.0
                if pr is not None:                  # keep the pane filled where the frame allows
                    s = self._scale()
                    H, W = self.orig.shape[:2]
                    hw, hh = pr.width() / (2 * s), pr.height() / (2 * s)
                    cx = min(max(cx, hw), W - hw) if W > 2 * hw else W / 2.0
                    cy = min(max(cy, hh), H - hh) if H > 2 * hh else H / 2.0
                return cx, cy
        return self._center()

    def _to_widget(self, pane, x, y, name="rgb"):
        s = self._scale()
        cx, cy = self._pane_center(name, pane)
        c = pane.center()
        return QPointF(c.x() + (x - cx) * s, c.y() + (y - cy) * s)

    def _to_image(self, pos):
        for name, pr in self.panes():
            if pr.contains(QPointF(pos)):
                s = self._scale()
                cx, cy = self._pane_center(name, pr)
                c = pr.center()
                return name, (pos.x() - c.x()) / s + cx, (pos.y() - c.y()) / s + cy
        return None, None, None

    def coded_to_display(self, rect):
        """coded (y0, x0, h, w) -> display (x0, y0, w, h) floats."""
        y0, x0, h, w = rect
        H, W = self.coded_shape
        pts = []
        for y, x in ((y0, x0), (y0 + h, x0 + w)):
            hh, ww = H, W
            for _ in range(self.rot):               # rot90 counter-clockwise
                y, x = ww - x, y
                hh, ww = ww, hh
            pts.append((y, x))
        (ya, xa), (yb, xb) = pts
        return min(xa, xb), min(ya, yb), abs(xb - xa), abs(yb - ya)

    def display_to_coded(self, x0, y0, w, h):
        H, W = self.coded_shape
        dh, dw = (W, H) if self.rot % 2 else (H, W)
        x0, y0 = int(max(0, min(x0, dw - 1))), int(max(0, min(y0, dh - 1)))
        w, h = int(max(1, min(w, dw - x0))), int(max(1, min(h, dh - y0)))
        if self.rot == 0:
            return y0, x0, h, w
        idx = np.rot90(np.indices((H, W)), self.rot, axes=(1, 2))
        ys, xs = idx[0][y0:y0 + h, x0:x0 + w], idx[1][y0:y0 + h, x0:x0 + w]
        return int(ys.min()), int(xs.min()), int(ys.max() - ys.min() + 1), int(xs.max() - xs.min() + 1)

    # ---------------- painting ----------------
    def paintEvent(self, e):
        p = QPainter(self)
        p.fillRect(self.rect(), QColor(40, 40, 40))
        if self.orig is None:
            p.setPen(QColor(170, 170, 170))
            p.drawText(self.rect(), Qt.AlignCenter, "Open a video (Effect > Open Video…)")
            p.end()
            return
        src = self._source()
        s = self._scale()
        p.setRenderHint(QPainter.SmoothPixmapTransform, s < 1.0)
        H, W = self.orig.shape[:2]
        f = QFont(self.font())
        f.setBold(True)
        p.setFont(f)
        for k, (name, pr) in enumerate(self.panes()):
            p.save()
            p.setClipRect(pr)
            p.fillRect(pr, QColor(55, 55, 55))
            qi = self.pane_image(name, src if name == "rgb" else ("orig" if src == "orig" else "filt"))
            tl = self._to_widget(pr, 0, 0, name)
            p.drawImage(QRectF(tl.x(), tl.y(), W * s, H * s), qi)
            self._overlays(p, pr, name)
            if self.showing_filtered():
                self._text(p, pr, "Assist Image" if (k == 0 and self.assist) else "Filtered", Qt.AlignTop)
            if name != "rgb":
                self._text(p, pr, PANE_LABEL.get(name, name), Qt.AlignBottom)
            p.restore()
        p.end()

    @staticmethod
    def _text(p, pr, text, valign):
        r = pr.adjusted(6, 4, -8, -4)
        p.setPen(QColor(0, 0, 0))
        p.drawText(r.translated(1, 1), Qt.AlignRight | valign, text)
        p.setPen(QColor(255, 255, 255))
        p.drawText(r, Qt.AlignRight | valign, text)

    def _wrect(self, pr, rect, name="rgb"):
        x, y, w, h = self.coded_to_display(rect)
        a = self._to_widget(pr, x, y, name)
        s = self._scale()
        return QRectF(a.x(), a.y(), w * s, h * s)

    def _overlays(self, p, pr, name):
        primary = name == "rgb"
        if self.prof_region is not None and primary and self.sel_style == "profile":
            p.setPen(QPen(QColor(80, 160, 255), 1, Qt.DashLine))
            p.setBrush(Qt.NoBrush)
            p.drawRect(self._wrect(pr, self.prof_region))
        if self.region is not None and primary and self.showing_filtered() and self.sel is None:
            p.setPen(QPen(QColor(80, 160, 255), 1, Qt.DashLine))
            p.drawRect(self._wrect(pr, self.region))
        if self.sel is not None:
            r = self._wrect(pr, self.sel, name)
            if not primary:
                p.setPen(QPen(QColor(160, 160, 160), 3))
            elif self.sel_style == "profile":
                side = max(r.width(), 1)
                p.setPen(QPen(QColor(0, 200, 0), 3 if side < 64 else 4 if side < 160 else 5))
            else:
                p.setPen(QPen(QColor(255, 255, 255), 1, Qt.DashLine))
            p.setBrush(Qt.NoBrush)
            p.drawRect(r)
            if primary and self.sel_label:
                ty = r.top() - 22 if r.top() - 22 >= pr.top() else r.bottom() + 4   # below at the top edge
                br = QRectF(r.center().x() - 120, ty, 240, 18)
                p.setPen(QColor(0, 0, 0))
                p.drawText(br.translated(1, 1), Qt.AlignCenter, self.sel_label)
                p.setPen(QColor(255, 255, 255))
                p.drawText(br, Qt.AlignCenter, self.sel_label)
        if self._drag is not None and primary:
            p.setPen(QPen(QColor(255, 80, 80), 2))
            p.drawRect(self._drag)

    # ---------------- mouse ----------------
    def _set_hold(self, on):
        if on != self.hold:
            self.hold = on
            self.holdChanged.emit(on)
            self.update()

    def mousePressEvent(self, e):
        pos = e.position()
        if e.button() == Qt.MiddleButton or (e.button() == Qt.LeftButton and self.pan_mode):
            if self.orig is not None:
                self._pan0 = (pos, self._center(), self._scale())
            return
        if e.button() == Qt.LeftButton and self.orig is not None:
            self._p0 = pos
            self._drag = None
            self._set_hold(True)

    def mouseMoveEvent(self, e):
        pos = e.position()
        if self._pan0 is not None:
            p0, (cx, cy), s = self._pan0
            if self._zoom == "fit":
                self._zoom = round(s * 100.0, 1)
                self.viewChanged.emit()
            self.center = (cx - (pos.x() - p0.x()) / s, cy - (pos.y() - p0.y()) / s)
            self.update()
            return
        if self._p0 is not None:
            dx, dy = pos.x() - self._p0.x(), pos.y() - self._p0.y()
            if self._drag is not None or max(abs(dx), abs(dy)) > 4:
                if self.square:
                    d = max(abs(dx), abs(dy))
                    dx, dy = (d if dx >= 0 else -d), (d if dy >= 0 else -d)
                self._drag = QRectF(QPointF(self._p0), QPointF(self._p0.x() + dx, self._p0.y() + dy)).normalized()
                self.update()
            return
        if self.orig is not None:
            name, x, y = self._to_image(pos)
            H, W = self.orig.shape[:2]
            if name is not None and 0 <= x < W and 0 <= y < H:
                q = self.pane_image("rgb", self._source())
                c = QColor(q.pixel(int(x), int(y)))
                self.cursorText.emit(f"R: {c.red()}  G: {c.green()}  B: {c.blue()}")
            else:
                self.cursorText.emit("R:  G:  B:")

    def mouseReleaseEvent(self, e):
        if self._pan0 is not None:
            self._pan0 = None
            return
        if self._p0 is None:
            return
        drag, self._p0, self._drag = self._drag, None, None
        self._set_hold(False)
        if drag is not None and drag.width() >= 4:
            n0, x0, y0 = self._to_image(drag.topLeft().toPoint())
            if n0 is None:
                return
            s = self._scale()
            w, h = drag.width() / s, drag.height() / s
            if self.square:
                w = h = min(w, h)
            self.areaSelected.emit(*self.display_to_coded(x0, y0, w, h))
        else:
            self.clicked.emit()

    def wheelEvent(self, e):
        if self.orig is not None:
            self.zoom_step(1 if e.angleDelta().y() > 0 else -1)

    def keyPressEvent(self, e):
        if e.key() == Qt.Key_Escape:
            self.clicked.emit()
        else:
            super().keyPressEvent(e)


class ViewerBar(QWidget):
    """Bar under the viewer: zoom, viewer mode, pan, brightness, W/H of the selection, R/G/B under
    the cursor, frame size and bit depth, info."""

    def __init__(self, viewer, parent=None):
        super().__init__(parent)
        self.v = viewer
        lay = QHBoxLayout(self)
        lay.setContentsMargins(2, 1, 2, 1)
        lay.setSpacing(6)
        self.zoom = QComboBox()
        self.zoom.addItems(["Fit"] + [f"{z:g}%" for z in ZOOMS])
        self.zoom.setToolTip("Adjust zoom level of image viewer")
        self.zoom.activated.connect(lambda i: viewer.set_zoom("fit" if i == 0 else ZOOMS[i - 1]))
        lay.addWidget(self.zoom)
        self.mode = QComboBox()
        self.mode.addItems([m for m, _ in MODES])
        for i, (_, long) in enumerate(MODES):
            self.mode.setItemData(i, long, Qt.ToolTipRole)
        self.mode.setToolTip("Adjust image viewer mode (YCrCb + Enhanced Contrast: see the viewer's "
                             "definition of the enhanced components)")
        self.mode.activated.connect(lambda i: viewer.set_mode(MODES[i][0]))
        lay.addWidget(self.mode)
        self.pan = QToolButton()
        self.pan.setText("✋")
        self.pan.setCheckable(True)
        self.pan.setToolTip("Toggle viewer panning mode (or drag with the middle button)")
        self.pan.toggled.connect(lambda on: setattr(viewer, "pan_mode", on))
        lay.addWidget(self.pan)
        lay.addWidget(self._line())
        self.bright_reset = QToolButton()
        self.bright_reset.setText("☼")
        self.bright_reset.setToolTip("Reset viewer brightness")
        self.bright = QSlider(Qt.Horizontal)
        self.bright.setRange(-100, 100)
        self.bright.setFixedWidth(80)
        self.bright.setToolTip("Adjust viewer brightness (display only)")
        self.bright.valueChanged.connect(viewer.set_brightness)
        self.bright_reset.clicked.connect(lambda: self.bright.setValue(0))
        lay.addWidget(self.bright_reset)
        lay.addWidget(self.bright)
        lay.addWidget(self._line())
        self.wh = QLabel("W:      H:     ")
        self.wh.setToolTip("Size of the selected area")
        lay.addWidget(self.wh)
        lay.addWidget(self._line())
        self.rgb = QLabel("R:  G:  B:")
        self.rgb.setMinimumWidth(140)
        lay.addWidget(self.rgb)
        lay.addStretch(1)
        lay.addWidget(self._line())
        self.size = QLabel("Frame: —")
        lay.addWidget(self.size)
        self.info = QToolButton()
        self.info.setText("ⓘ")
        self.info.setToolTip("Additional information about image")
        lay.addWidget(self.info)
        viewer.cursorText.connect(self.rgb.setText)
        viewer.viewChanged.connect(self.sync)
        self.sync()

    @staticmethod
    def _line():
        f = QFrame()
        f.setFrameShape(QFrame.VLine)
        f.setFrameShadow(QFrame.Sunken)
        return f

    def sync(self):
        z = self.v.zoom()
        if z == "fit":
            self.zoom.setCurrentIndex(0)
        else:
            i = self.zoom.findText(f"{z:g}%")
            if i < 0:
                self.zoom.setItemText(0, f"{z:g}%")
                i = 0
            else:
                self.zoom.setItemText(0, "Fit")
            self.zoom.setCurrentIndex(i)
        if z == "fit":
            self.zoom.setItemText(0, "Fit")
        self.mode.setCurrentIndex([m for m, _ in MODES].index(self.v.mode()))

    def set_selection(self, rect):
        self.wh.setText("W:      H:     " if rect is None else f"W: {rect[3]}  H: {rect[2]}")

    def set_frame_info(self, W, H, bits=8):
        self.size.setText(f"Frame: {W}x{H}, {bits}-bit")
