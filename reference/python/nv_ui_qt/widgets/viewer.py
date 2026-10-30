"""Image viewer in the genuine layout: a primary pane plus three component panes (YCrCb + Enhanced
Contrast, RGB, YCrCb, Y / Cr / Cb Frequencies) arranged 2 x 2 / 1+3 Bottom / 1+3 Right (rows / columns
2 : 1), sharing the zoom; "Filtered" in each pane's corner while the filtered preview is shown, the
original while the left button is held.

The pane images follow the genuine's formulas (nv_ui_qt/display.py: colour conversion, the pane mapping
FUN_00051130 with the viewer brightness 0..100 (50 = identity), BuildFreqImage / FindFreqLevs for the
Enhanced and Frequencies panes, emphasis on the Prepare page only). Enhanced / Frequencies images are
computed in a worker thread and cached per image; until they are ready the panes say "computing…".

The component panes show the same scale as the primary pane, centred on the selection (or the primary
pane's centre without one), kept inside the frame where the frame is larger than the pane (as the
genuine screenshots show at the frame's top edge).

Selection (Prepare page): a free rectangle; the press point is a corner, each side at most `sel_max`
(512) pixels from it, inside the frame, no lower limit. Dragging inside the selection moves it (size
kept); a click without dragging emits clicked (the main window clears the selection). The frame colour
follows min(w, h) (FUN_000492d4): <= 15 red (dashed), 16..127 yellow, >= 128 green, the profile's Main
Sample blue (the colours measured on the genuine screenshot (98, 171, 255) / (32, 32, 140)); 'Noise
Level: x.x' above, warnings below in yellow, a small triangle at the lower right corner opens the
Build Profile menu (menuRequested). Adjust page: the preview area (sel_max 1280) drawn in blue.

Profile Check (Prepare page, set_pcheck): every pane is made from the Assist Image the main window
computes (the bottom panes' Enhanced without emphasis, as STATIC.md 1d reads page+0x1e0), with a yellow
"⚙ Profile Check" at the top centre of each pane and a white "Assist Image" at the top right of the
image (primary pane) / of the pane (genuine pc1.png, pc1_label.png). Until the first Assist Image is
ready the original is shown; holding the mouse button does not switch to the original (the button
draws the selection on this page; the genuine's behaviour is not observed).

Noise Level Check (Adjust page, set_nlc): the primary pane shows the mask image the main window computes
(display.nlc_image: sepia where the temporal filter averages nothing, blue where it averages fully) with
a yellow "⚙ Noise Level Check" at the top centre and a white "Assist Image" at the top right of the image
(genuine g_nlc1.png); the main window switches the viewer to Normal. Holding the mouse button shows the
original (repro choice, as for the filtered preview on this page).
"""
import concurrent.futures

import numpy as np
from PySide6.QtCore import QPoint, QPointF, QRectF, Qt, Signal
from PySide6.QtGui import QAction, QActionGroup, QColor, QFont, QImage, QPainter, QPainterPath, QPen, QPolygonF
from PySide6.QtWidgets import QFrame, QHBoxLayout, QLabel, QMenu, QSizePolicy, QSlider, QToolButton, QWidget

from .. import display as D

ZOOMS = [5, 10, 12.5, 16.7, 20, 25, 33.3, 50, 100, 200, 300, 400, 800]
MODES = [("Normal", "Normal"), ("YCrCb +", "YCrCb + Enhanced Contrast (Default)"),
         ("RGB", "RGB  (Red, Green, Blue)"), ("YCrCb", "YCrCb  (Luminance, Chrominance)"),
         ("Y Freq", "Y Channel Frequencies"), ("Cr Freq", "Cr Channel Frequencies"),
         ("Cb Freq", "Cb Channel Frequencies")]
MODE_SECTIONS = {0: "Primary Modes", 2: "Channel Components", 4: "Frequency Components"}
SPLITS = ["2 x 2", "1+3 Bottom", "1+3 Right"]
PANES = {"Normal": [], "YCrCb +": ["Y+", "Cr+", "Cb+"], "RGB": ["R", "G", "B"], "YCrCb": ["Y", "Cr", "Cb"],
         "Y Freq": ["Y High", "Y Mid", "Y Low"], "Cr Freq": ["Cr High", "Cr Mid", "Cr Low"],
         "Cb Freq": ["Cb High", "Cb Mid", "Cb Low"]}
PANE_LABEL = {"Y+": "Y Enhanced", "Cr+": "Cr Enhanced", "Cb+": "Cb Enhanced"}
FREQ_KIND = {"Y+": "Y+", "Cr+": "Y+", "Cb+": "Y+", "Y High": "Y", "Y Mid": "Y", "Y Low": "Y",
             "Cr High": "Cr", "Cr Mid": "Cr", "Cr Low": "Cr", "Cb High": "Cb", "Cb Mid": "Cb", "Cb Low": "Cb"}
# selection frame colours (inner, outer) by min(w, h); Main Sample and preview area
SEL_COLORS = ((15, (255, 15, 15), (128, 0, 0)), (127, (255, 208, 15), (100, 97, 2)), (None, (15, 192, 15), (0, 94, 0)))
MAIN_COLORS = ((98, 171, 255), (32, 32, 140))
PREVIEW_COLORS = ((15, 15, 255), (0, 0, 110))
PANE_SEL_COLORS = ((192, 192, 192), (64, 64, 64))
WARN = QColor(255, 208, 15)
PCHECK_TEXT = QColor(240, 208, 0)              # "⚙ Profile Check" / "⚙ Noise Level Check" overlay (genuine pc1_label.png)


def to_qimage(rgb):
    a = np.ascontiguousarray(np.clip(rgb, 0, 255).astype(np.uint8))
    h, w = a.shape[:2]
    return QImage(a.data, w, h, 3 * w, QImage.Format_RGB888).copy()


def sel_colors(w, h):
    m = min(w, h)
    for lim, inner, outer in SEL_COLORS:
        if lim is None or m <= lim:
            return inner, outer


class Viewer(QWidget):
    areaSelected = Signal(int, int, int, int)      # y0, x0, h, w in coded luma pixels (gesture finished)
    selectionChanging = Signal(int, int, int, int)  # during a drag / move
    clicked = Signal()                             # press + release without dragging
    holdChanged = Signal(bool)
    cursorText = Signal(str)
    viewChanged = Signal()                         # mode / split / zoom changed
    menuRequested = Signal(QPoint)                 # the selection's triangle (global position)
    _computed = Signal(object)                     # worker -> GUI thread

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setMinimumSize(320, 240)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.setMouseTracking(True)
        self.setFocusPolicy(Qt.StrongFocus)
        self.orig = self.filt = None               # display-oriented uint8 RGB (rounded)
        self._f = {}                               # "orig" / "filt": display-oriented float32 RGB
        self._tok = {"orig": 0, "filt": 0, "pcheck": 0, "nlc": 0}    # image tokens of the frequency cache
        self._ntok = 0
        self.region = None
        self.rot = 0
        self.coded_shape = None
        self.matrix = "bt709"
        self._mode, self._split, self._zoom = "YCrCb +", "1+3 Bottom", "fit"
        self.center = None
        self.brightness = 50
        self.emphasis = True                       # Enhanced emphasis (Prepare page)
        self.show_filtered = False
        self.assist = False
        self.pcheck = False                        # Profile Check: panes from the Assist Image
        self.nlc = False                           # Noise Level Check: primary pane from the mask image
        self.hold = False
        self.pan_mode = False
        self.sel_max, self.sel_movable = 512, True
        self.sel = None                            # coded (y0, x0, h, w)
        self.sel_style = "profile"                 # "profile" (Prepare) / "preview" (Adjust)
        self.sel_label = ""
        self.sel_warn = []                         # lines under the frame (yellow)
        self.main_sample = None                    # coded rect of the profile's sample area
        self._cache = {}
        self._freq = {}                            # (token, kind, emphasis) -> (planes, levels) / None
        self._pool = concurrent.futures.ThreadPoolExecutor(max_workers=1)
        self._computed.connect(self._on_computed)
        self._p0 = self._g = self._pan0 = None

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

    def scale_percent(self):
        return self._scale() * 100.0

    def set_brightness(self, b):
        """Viewer brightness 0..100 (50 = no change), display only."""
        self.brightness = int(min(max(int(b), 0), 100))
        self._cache = {}
        self.update()

    def set_emphasis(self, on):
        on = bool(on)
        if on != self.emphasis:
            self.emphasis = on
            self._cache = {}
            self.update()

    def set_matrix(self, m):
        self.matrix = m

    def set_show_filtered(self, on):
        if on != self.show_filtered:
            self.show_filtered = on
            self.update()

    def set_assist(self, on):
        self.assist = bool(on)
        self._cache = {}
        self.update()

    def showing_filtered(self):
        return self.show_filtered and self.filt is not None and not self.hold

    def set_pcheck(self, on, img=None):
        """Profile Check on / off; img = coded-orientation float RGB of the Assist Image of the whole
        frame (None keeps the current one, e.g. while the next one is computed)."""
        self.pcheck = bool(on)
        if not on:
            self._f.pop("pcheck", None)
        elif img is not None:
            fo = np.ascontiguousarray(np.rot90(np.asarray(img, np.float32), self.rot))
            if "pcheck" not in self._f or self._f["pcheck"].shape != fo.shape or not np.array_equal(fo, self._f["pcheck"]):
                self._f["pcheck"] = fo
                self._ntok += 1
                self._tok["pcheck"] = self._ntok
        self._drop_freq()
        self._cache = {}
        self.update()

    def showing_pcheck(self):
        return self.pcheck and "pcheck" in self._f

    def set_nlc(self, on, img=None, keep=True):
        """Noise Level Check on / off; img = coded-orientation float RGB of the mask image of the whole
        frame (None: keep the current one while the next one is computed, or drop it if keep is False)."""
        self.nlc = bool(on)
        if not on or (img is None and not keep):
            self._f.pop("nlc", None)
        elif img is not None:
            fo = np.ascontiguousarray(np.rot90(np.asarray(img, np.float32), self.rot))
            if "nlc" not in self._f or self._f["nlc"].shape != fo.shape or not np.array_equal(fo, self._f["nlc"]):
                self._f["nlc"] = fo
                self._ntok += 1
                self._tok["nlc"] = self._ntok
        self._drop_freq()
        self._cache = {}
        self.update()

    def showing_nlc(self):
        return self.nlc and "nlc" in self._f and not self.hold

    def _drop_freq(self):
        live = {self._tok[k] for k in self._tok if k in self._f}
        self._freq = {k: v for k, v in self._freq.items() if k[0] in live}

    def set_selection(self, rect, style="profile", label="", warn=None):
        """rect = coded (y0, x0, h, w) or None."""
        self.sel, self.sel_style, self.sel_label = rect, style, label
        self.sel_warn = list(warn or [])
        self.update()

    def set_selection_text(self, label="", warn=None):
        self.sel_label, self.sel_warn = label, list(warn or [])
        self.update()

    def set_main_sample(self, rect):
        self.main_sample = None if rect is None else tuple(int(v) for v in rect)
        self.update()

    def set_profile_region(self, rect):          # older name
        self.set_main_sample(rect)

    def set_select_limits(self, max_side, movable):
        self.sel_max, self.sel_movable = int(max_side), bool(movable)

    # ---------------- images ----------------
    def set_images(self, orig, filt=None, region=None, rot=0):
        """orig: coded-orientation RGB of the full frame (float 0..255 or uint8); filt: RGB of the full
        frame or of `region` (y0, x0, h, w); None = not computed yet."""
        orig = np.asarray(orig, np.float32)
        rot = rot % 4
        if filt is not None and region is not None:
            y0, x0, h, w = region
            full = np.array(orig, copy=True)
            full[y0:y0 + h, x0:x0 + w] = np.asarray(filt, np.float32)[:h, :w]
            filt = full
        o = np.ascontiguousarray(np.rot90(orig, rot))
        new_orig = "orig" not in self._f or self.rot != rot or self._f["orig"].shape != o.shape or \
            not np.array_equal(o, self._f["orig"])
        self.rot = rot
        self.coded_shape = orig.shape[:2]
        self.region = region
        if new_orig:
            self._f["orig"] = o
            self.orig = np.clip(np.rint(o), 0, 255).astype(np.uint8)
            self._ntok += 1
            self._tok["orig"] = self._ntok
        if filt is None:
            self._f.pop("filt", None)
            self.filt = None
        else:
            fo = np.ascontiguousarray(np.rot90(np.asarray(filt, np.float32), rot))
            if "filt" not in self._f or self._f["filt"].shape != fo.shape or not np.array_equal(fo, self._f["filt"]):
                self._f["filt"] = fo
                self.filt = np.clip(np.rint(fo), 0, 255).astype(np.uint8)
                self._ntok += 1
                self._tok["filt"] = self._ntok
        self._drop_freq()
        self._cache = {}
        self.update()
        if new_orig and self._zoom == "fit":
            self.viewChanged.emit()             # the fitted scale (shown in percent) may change

    def _source(self):
        """'filt' / 'orig' / 'assist' / 'pcheck' / 'nlc' for the primary pane."""
        if self.showing_pcheck():
            return "pcheck"
        if self.showing_nlc():
            return "nlc"
        if not self.showing_filtered():
            return "orig"
        return "assist" if self.assist else "filt"

    @staticmethod
    def _plane_src(src):
        """Image the component panes use with the primary pane's source."""
        return src if src in ("orig", "pcheck", "nlc") else "filt"

    def _img(self, src):
        return self._f[self._plane_src(src)]

    def _freq_get(self, src, kind):
        """(planes, levels) of the frequency image, or None while it is being computed. The Assist
        Image of Profile Check (and the Noise Level Check image) gets no emphasis."""
        s = self._plane_src(src)
        key = (self._tok[s], kind, self.emphasis and s not in ("pcheck", "nlc") if kind == "Y+" else False)
        if key in self._freq:
            return self._freq[key]
        self._freq[key] = None
        img, emph = self._f[s], key[2]
        fut = self._pool.submit(D.freq_images, img, kind, emph)
        fut.add_done_callback(lambda f, key=key: self._computed.emit((key, f)))
        return None

    def _on_computed(self, msg):
        key, fut = msg
        if key not in self._freq:
            return                                  # image replaced meanwhile
        try:
            self._freq[key] = fut.result()
        except Exception as e:                      # noqa: BLE001 - shown in the pane
            self._freq[key] = e
        self._cache = {k: v for k, v in self._cache.items() if v is not None}
        self.update()

    def ready(self):
        """True when every pane of the current mode can be drawn (no frequency image pending)."""
        if self.orig is None:
            return True
        src = self._source()
        for name in PANES[self._mode]:
            if name in FREQ_KIND and self._freq_get(self._plane_src(src), FREQ_KIND[name]) is None:
                return False
        return True

    def pane_image(self, name, src):
        """QImage of a pane (None while its frequency image is computed); name 'rgb' = primary pane."""
        key = (src, name)
        if self._cache.get(key) is not None:
            return self._cache[key]
        b = self.brightness
        mnmx = D.minmax(self._f["orig"])
        if name == "rgb":
            if src == "assist":
                # Filter Tuning Assist (Spatial tab, repro): left half of the frame original, right half
                # filtered (self.split() is never "off", SPLITS has no such entry). Not the Noise Level
                # Check, which is the "nlc" source (set_nlc).
                img = self._f["filt"]
                if self.split() != "off" and "filt" in self._f:
                    sw = self._f["orig"].shape[1] // 2
                    img = self._f["orig"].copy()
                    img[:, sw:] = self._f["filt"][:, sw:]
            else:
                img = self._img(src)
            out = D.render_simple("rgb", img, b, mnmx)
        elif name in FREQ_KIND:
            got = self._freq_get(src, FREQ_KIND[name])
            if got is None or isinstance(got, Exception):
                return None
            planes, levels = got
            k = ("Y+", "Cr+", "Cb+").index(name) if name.endswith("+") else ("High", "Mid", "Low").index(name.split()[1])
            out = D.render_freq([name], [planes[k]], [levels[k]], b)[0]
        else:
            out = D.render_simple(name, self._img(src), b, mnmx)
        qi = to_qimage(out)
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
        """Component panes centre on the selection ((x0 + x1 + 1) / 2, (y0 + y1 + 1) / 2), else on the
        primary pane's centre; kept inside the frame where the frame is larger than the pane."""
        if name == "rgb":
            return self._center()
        if self.sel is not None:
            x, y, w, h = self.coded_to_display(self.sel)
            cx, cy = x + w / 2.0, y + h / 2.0
        else:
            cx, cy = self._center()
        if pr is not None:
            s = self._scale()
            H, W = self.orig.shape[:2]
            hw, hh = pr.width() / (2 * s), pr.height() / (2 * s)
            cx = min(max(cx, hw), W - hw) if W > 2 * hw else W / 2.0
            cy = min(max(cy, hh), H - hh) if H > 2 * hh else H / 2.0
        return cx, cy

    def _to_widget(self, pane, x, y, name="rgb"):
        s = self._scale()
        cx, cy = self._pane_center(name, pane)
        c = pane.center()
        return QPointF(c.x() + (x - cx) * s, c.y() + (y - cy) * s)

    def _to_image(self, pos, only=None):
        for name, pr in self.panes():
            if (only is None and pr.contains(QPointF(pos))) or name == only:
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
            p.drawText(self.rect(), Qt.AlignCenter, "Open a video (Effect > Open Input Video…)")
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
            qi = self.pane_image(name, src if name == "rgb" else self._plane_src(src))
            tl = self._to_widget(pr, 0, 0, name)
            ir = QRectF(tl.x(), tl.y(), W * s, H * s)
            if qi is None:
                p.setPen(QColor(200, 200, 200))
                p.drawText(pr, Qt.AlignCenter, "computing…")
            else:
                p.drawImage(ir, qi)
            self._overlays(p, pr, name)
            if src in ("pcheck", "nlc"):
                title = "⚙ Profile Check" if src == "pcheck" else "⚙ Noise Level Check"
                self._text(p, pr, title, Qt.AlignTop | Qt.AlignHCenter, PCHECK_TEXT)
                self._text(p, ir.intersected(pr) if k == 0 else pr, "Assist Image", Qt.AlignTop)
            elif self.showing_filtered():
                self._text(p, pr, "Assist Image" if (k == 0 and self.assist) else "Filtered", Qt.AlignTop)
            if name != "rgb":
                self._text(p, pr, PANE_LABEL.get(name, name), Qt.AlignBottom)
            p.restore()
        p.end()

    @staticmethod
    def _text(p, pr, text, valign, color=QColor(255, 255, 255)):
        """Shadowed label in the pane rect pr: right-aligned unless valign carries a horizontal flag."""
        r = pr.adjusted(6, 4, -8, -4)
        align = valign if valign & Qt.AlignHorizontal_Mask else Qt.AlignRight | valign
        p.setPen(QColor(0, 0, 0))
        p.drawText(r.translated(1, 1), align, text)
        p.setPen(color)
        p.drawText(r, align, text)

    def _wrect(self, pr, rect, name="rgb"):
        x, y, w, h = self.coded_to_display(rect)
        a = self._to_widget(pr, x, y, name)
        s = self._scale()
        return QRectF(a.x(), a.y(), w * s, h * s)

    @staticmethod
    def _frame(p, r, inner, outer, dashed=False):
        """Selection frame drawn outside r: a dark line, the coloured band (thicker for larger
        selections, at most 8 px), a dark line (as on the genuine screenshots)."""
        t = max(1.0, min(8.0, round(min(r.width(), r.height()) / 10.0)))
        p.setBrush(Qt.NoBrush)
        out = r.adjusted(-t / 2 - 1, -t / 2 - 1, t / 2 + 1, t / 2 + 1)
        p.setPen(QPen(QColor(*outer), t + 2, Qt.DashLine if dashed else Qt.SolidLine, Qt.SquareCap, Qt.MiterJoin))
        p.drawRect(out)
        p.setPen(QPen(QColor(*inner), t, Qt.DashLine if dashed else Qt.SolidLine, Qt.SquareCap, Qt.MiterJoin))
        p.drawRect(out)
        return out.adjusted(-t / 2 - 1, -t / 2 - 1, t / 2 + 1, t / 2 + 1)

    def triangle_rect(self, pr=None):
        """Widget rect of the selection's triangle (Prepare page) or None."""
        if self.sel is None or self.sel_style != "profile" or self.orig is None:
            return None
        pr = pr or self.panes()[0][1]
        r = self._wrect(pr, self.sel)
        t = max(1.0, min(8.0, round(min(r.width(), r.height()) / 10.0)))
        return QRectF(r.right() + t / 2 + 2, r.bottom() - 4, 12, 9)

    def _overlays(self, p, pr, name):
        primary = name == "rgb"
        if self.region is not None and primary and self.showing_filtered() and self.sel is None:
            p.setPen(QPen(QColor(*PREVIEW_COLORS[0]), 1, Qt.DashLine))
            p.drawRect(self._wrect(pr, self.region))
        if self.sel is None:
            return
        r = self._wrect(pr, self.sel, name)
        y0, x0, h, w = self.sel
        if not primary:
            self._frame(p, r, *PANE_SEL_COLORS)
            return
        if self.sel_style != "profile":
            self._frame(p, r, *PREVIEW_COLORS)
            return
        if self.main_sample is not None and tuple(self.sel) == self.main_sample:
            outer = self._frame(p, r, *MAIN_COLORS)
        else:
            inner, oc = sel_colors(w, h)
            outer = self._frame(p, r, inner, oc, dashed=min(w, h) <= 15)
        fm = p.fontMetrics()
        y = outer.bottom() + 2
        if self.sel_label:
            br = QRectF(outer.center().x() - 150, outer.top() - fm.height() - 2, 300, fm.height())
            if br.top() < pr.top():                 # no room above (frame's top edge): first line below
                br.moveTop(y)
                y += fm.height()
            self._outlined(p, br, self.sel_label, QColor(255, 255, 255))
        for line in self.sel_warn:
            br = QRectF(outer.center().x() - 200, y, 400, fm.height())
            self._outlined(p, br, line, WARN)
            y += fm.height()
        tr = self.triangle_rect(pr)
        if tr is not None:
            p.setPen(QPen(QColor(40, 40, 40), 1))
            p.setBrush(QColor(200, 200, 200))
            p.drawPolygon(QPolygonF([tr.topLeft(), tr.topRight(), QPointF(tr.center().x(), tr.bottom())]))
            p.setBrush(Qt.NoBrush)

    @staticmethod
    def _outlined(p, br, text, color):
        path = QPainterPath()
        fm = p.fontMetrics()
        x = br.center().x() - fm.horizontalAdvance(text) / 2
        path.addText(QPointF(x, br.top() + fm.ascent()), p.font(), text)
        p.setRenderHint(QPainter.Antialiasing, True)
        p.strokePath(path, QPen(QColor(0, 0, 0), 3))
        p.fillPath(path, color)
        p.setRenderHint(QPainter.Antialiasing, False)

    # ---------------- mouse ----------------
    def _set_hold(self, on):
        if on != self.hold:
            self.hold = on
            self.holdChanged.emit(on)
            self.update()

    def _disp_shape(self):
        return self.orig.shape[:2]

    def mousePressEvent(self, e):
        pos = e.position()
        if e.button() == Qt.MiddleButton or (e.button() == Qt.LeftButton and self.pan_mode):
            if self.orig is not None:
                self._pan0 = (pos, self._center(), self._scale())
            return
        if e.button() != Qt.LeftButton or self.orig is None:
            return
        tr = self.triangle_rect()
        if tr is not None and tr.adjusted(-3, -3, 3, 3).contains(pos):
            self.menuRequested.emit(self.mapToGlobal(pos.toPoint()))
            return
        name, x, y = self._to_image(pos)
        self._p0 = pos
        self._g = None
        if name == "rgb":
            H, W = self._disp_shape()
            move = False
            if self.sel is not None and self.sel_movable:
                sx, sy, sw, sh = self.coded_to_display(self.sel)
                move = sx <= x <= sx + sw and sy <= y <= sy + sh
                if move:
                    self._g = {"kind": "move", "rect": (sx, sy, sw, sh), "x": x, "y": y, "moved": False}
            if not move:
                ax, ay = int(min(max(round(x), 0), W - 1)), int(min(max(round(y), 0), H - 1))
                self._g = {"kind": "new", "ax": ax, "ay": ay, "moved": False}
        self._set_hold(True)

    def _gesture_rect(self, x, y):
        """Display rect (x0, y0, w, h) of the current gesture at image point (x, y)."""
        H, W = self._disp_shape()
        g = self._g
        if g["kind"] == "move":
            sx, sy, sw, sh = g["rect"]
            nx = int(round(min(max(sx + x - g["x"], 0), W - sw)))
            ny = int(round(min(max(sy + y - g["y"], 0), H - sh)))
            return nx, ny, int(sw), int(sh)
        m = self.sel_max
        ax, ay = g["ax"], g["ay"]
        cx, cy = int(round(x)), int(round(y))
        if cx >= ax:
            x0, x1 = ax, min(cx, ax + m, W)
        else:
            x0, x1 = max(cx, ax - m, 0), ax
        if cy >= ay:
            y0, y1 = ay, min(cy, ay + m, H)
        else:
            y0, y1 = max(cy, ay - m, 0), ay
        return x0, y0, max(x1 - x0, 1), max(y1 - y0, 1)

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
        if self._p0 is not None and self._g is not None:
            dx, dy = pos.x() - self._p0.x(), pos.y() - self._p0.y()
            if self._g["moved"] or max(abs(dx), abs(dy)) > 4:
                self._g["moved"] = True
                _, x, y = self._to_image(pos, only="rgb")
                r = self.display_to_coded(*self._gesture_rect(x, y))
                self.sel = r
                self.update()
                self.selectionChanging.emit(*r)
            return
        if self.orig is not None:
            name, x, y = self._to_image(pos)
            H, W = self.orig.shape[:2]
            if name is not None and 0 <= x < W and 0 <= y < H:
                v = self._img(self._source() if name == "rgb" else "orig")[int(y), int(x)]
                self.cursorText.emit(f"R: {v[0]:.2f}  G: {v[1]:.2f}  B: {v[2]:.2f}")
            else:
                self.cursorText.emit("R:  G:  B:")

    def mouseReleaseEvent(self, e):
        if self._pan0 is not None:
            self._pan0 = None
            return
        if self._p0 is None:
            return
        g, self._p0, self._g = self._g, None, None
        self._set_hold(False)
        if g is not None and g["moved"] and self.sel is not None:
            self.areaSelected.emit(*self.sel)
        else:
            self.clicked.emit()

    def resizeEvent(self, e):
        super().resizeEvent(e)
        if self._zoom == "fit":
            self.viewChanged.emit()             # the fitted scale (shown in percent) changed

    def wheelEvent(self, e):
        if self.orig is not None:
            self.zoom_step(1 if e.angleDelta().y() > 0 else -1)

    def keyPressEvent(self, e):
        if e.key() == Qt.Key_Escape:
            self.clicked.emit()
        else:
            super().keyPressEvent(e)


class ViewerBar(QWidget):
    """Bar under the viewer: zoom (percent), viewer mode menu (Primary Modes / Channel Components /
    Frequency Components / Viewer Split), pan, brightness 0..100, W/H of the selection, R/G/B under the
    cursor (two decimals), frame size and bit depth, info."""

    def __init__(self, viewer, parent=None):
        super().__init__(parent)
        self.v = viewer
        lay = QHBoxLayout(self)
        lay.setContentsMargins(2, 1, 2, 1)
        lay.setSpacing(6)
        self.zoom = QToolButton()
        self.zoom.setPopupMode(QToolButton.InstantPopup)
        self.zoom.setToolTip("Adjust zoom level of image viewer")
        zm = QMenu(self.zoom)
        zm.addAction("Zoom to Fit", lambda: viewer.set_zoom("fit"))
        zm.addSeparator()
        for z in ZOOMS:
            zm.addAction(f"{z:g}%", lambda z=z: viewer.set_zoom(z))
        self.zoom.setMenu(zm)
        self.zoom.setMinimumWidth(70)
        lay.addWidget(self.zoom)
        self.mode = QToolButton()
        self.mode.setPopupMode(QToolButton.InstantPopup)
        self.mode.setToolTip("Adjust image viewer mode")
        self.mode.setMinimumWidth(80)
        mm = QMenu(self.mode)
        self.mode_actions, self.split_actions = {}, {}
        g = QActionGroup(self)
        for i, (short, long) in enumerate(MODES):
            if i in MODE_SECTIONS:
                mm.addSection(MODE_SECTIONS[i])
            a = QAction(long, self)
            a.setCheckable(True)
            a.triggered.connect(lambda checked=False, short=short: viewer.set_mode(short))
            g.addAction(a)
            mm.addAction(a)
            self.mode_actions[short] = a
        mm.addSection("Viewer Split")
        g = QActionGroup(self)
        for s in SPLITS:
            a = QAction(s, self)
            a.setCheckable(True)
            a.triggered.connect(lambda checked=False, s=s: viewer.set_split(s))
            g.addAction(a)
            mm.addAction(a)
            self.split_actions[s] = a
        self.mode.setMenu(mm)
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
        self.bright.setRange(0, 100)
        self.bright.setValue(50)
        self.bright.setFixedWidth(80)
        self.bright.setToolTip("Adjust viewer brightness (display only; 50 = unchanged)")
        self.bright.valueChanged.connect(viewer.set_brightness)
        self.bright_reset.clicked.connect(lambda: self.bright.setValue(50))
        lay.addWidget(self.bright_reset)
        lay.addWidget(self.bright)
        lay.addWidget(self._line())
        self.wh = QLabel("W:      H:     ")
        self.wh.setToolTip("Size of the selected area")
        lay.addWidget(self.wh)
        lay.addWidget(self._line())
        self.rgb = QLabel("R:  G:  B:")
        self.rgb.setMinimumWidth(200)
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
        pct = self.v.scale_percent()
        self.zoom.setText(f"{pct:.1f}%".replace(".0%", "%") + " ▾")
        self.zoom.setToolTip("Adjust zoom level of image viewer" + (" (Zoom to Fit)" if self.v.zoom() == "fit" else ""))
        m = self.v.mode()
        self.mode.setText(m + " ▾")
        for k, a in self.mode_actions.items():
            a.setChecked(k == m)
        for k, a in self.split_actions.items():
            a.setChecked(k == self.v.split())
            a.setEnabled(m != "Normal")

    def resizeEvent(self, e):
        super().resizeEvent(e)
        self.sync()

    def set_selection(self, rect):
        self.wh.setText("W:      H:     " if rect is None else f"W: {rect[3]}  H: {rect[2]}")

    def set_frame_info(self, W, H, bits=8):
        self.size.setText(f"Frame: {W}x{H}, {bits}-bit")
