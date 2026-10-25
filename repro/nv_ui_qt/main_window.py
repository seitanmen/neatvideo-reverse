"""Main window in the genuine layout: menu bar with the Beginner / Advanced switch, page tabs
(Prepare Noise Profile | Adjust and Preview), left column = page toolbar -> viewer -> viewer bar,
right = side panel, bottom row = scan type / gamma / gain (n/a), thumbnails, Cancel, Apply."""
import collections
import json
import os
import time

import numpy as np
from PySide6.QtCore import QSettings, Qt, QTimer
from PySide6.QtGui import QGuiApplication
from PySide6.QtWidgets import (QCheckBox, QComboBox, QDialog, QDialogButtonBox, QDoubleSpinBox, QFileDialog,
                               QFormLayout, QHBoxLayout, QInputDialog, QLabel, QListWidget, QMainWindow, QMenu,
                               QMessageBox, QProgressBar, QPushButton, QSpinBox, QSplitter, QStackedWidget,
                               QStyle, QTabBar, QTextEdit, QToolButton, QVBoxLayout, QWidget)

import nv_depth as nd
import nvfilter as nv
from denoise_video_v5 import probe, read_frames, remove_output, sidecar_stem

from . import backend
from .menus import build_menus
from .pages.filter_page import FilterPage
from .pages.profile_page import ProfilePage
from .state import (CHANNELS, PARAM_SPECS, AppState, ProfileState, auto_complete, default_params, dump_json,
                    load_json, params_from_json, preset_json)
from .widgets.noise_graph import NoiseGraph
from .widgets.timeline import Timeline
from .widgets.viewer import SPLITS, Viewer, ViewerBar, to_qimage
from .worker import ExportProcess, TaskRunner

REPRO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROFILE_FILTER = "Repro noise profile (*.nvprof.json);;JSON (*.json)"
PRESET_FILTER = "Repro filter preset (*.nvpreset.json);;JSON (*.json)"
PREFS = {"jobs": max(1, (os.cpu_count() or 2) - 2), "crf": 16, "audio": "copy", "preview_area": 0.5,
         "auto_fine_tune": True,
         "mem_gb": 4.0, "auto_profile": True, "depth": "8"}
# Preferences > Bit depth: (label, --depth value); 8-bit = the original yuv420p path
DEPTHS = (("8-bit (legacy yuv420p)", "8"), ("Source depth", "source"), ("10-bit", "10"), ("12-bit", "12"),
          ("16-bit", "16"), ("32-bit float (RGB)", "32"))
SPATIAL_PREFIXES = ("s_", "amt_", "bamt_", "bnoise_", "noise_", "adv_noise_level", "art_", "edge_", "sharp_",
                    "adv_spatial")
N_A = "Not available in the reproduction"


def settings():
    """QSettings (INI); NV_UI_QT_SETTINGS=<file> selects another file (tests)."""
    path = os.environ.get("NV_UI_QT_SETTINGS")
    if path:
        return QSettings(path, QSettings.IniFormat)
    return QSettings(QSettings.IniFormat, QSettings.UserScope, "NeatRepro", "nv_ui_qt")


GUIDES = {
    "filter": ("Filter Settings Check",
               "On the Adjust and Preview page, hold the left mouse button in the viewer to see the original "
               "frame; release it to see the filtered one. Check a detailed and a flat part of the frame "
               "(also in the Y / Cr / Cb Enhanced panes).\n\n"
               "• Residual noise in flat areas → raise Noise Level (Overall, Luminance / Cr / Cb or per band) "
               "or the Noise Reduction Amount.\n"
               "• Lost detail or waxy texture → lower the Noise Reduction Amount of the High / Mid frequencies, "
               "or the Noise Level.\n"
               "• Filter Tuning Assist (Spatial tab) shows what is removed: structure (edges, texture) in the "
               "Assist image means the filter is too strong.\n\n(Guide text written for the reproduction.)"),
    "noise": ("Noise Level Check",
              "The Noise Level sliders scale the profile's noise estimate (+0% = profile).\n"
              "Set them where flat areas just become clean in the filtered view; going further treats "
              "image detail as noise.\n\n(Guide text written for the reproduction.)"),
    "tuning": ("Quick Start Guide",
               "1. Prepare Noise Profile: Auto Profile (a frame with a flat, unclipped area), or drag a square "
               "over a flat area and Build Profile.\n"
               "2. Check the profile (Profile Check): sigma should vary smoothly with brightness.\n"
               "3. Advanced Mode: Auto Fine-Tune adjusts the per-brightness factors (static port of the genuine "
               "AFT); drag the knots of the equalizer for Manual Fine-Tune, Auto Complete interpolates the "
               "bins you did not touch.\n"
               "4. Adjust and Preview: tune Temporal / Spatial settings; hold the mouse button in the viewer "
               "to compare with the original; Apply processes the full video.\n\n"
               "(Guide text written for the reproduction.)"),
}


def _tool(text, tip, checkable=False, icon=None, style=None):
    b = QToolButton()
    b.setText(text)
    b.setToolTip(tip)
    b.setCheckable(checkable)
    if icon is not None:
        b.setIcon(style.standardIcon(icon))
        b.setToolButtonStyle(Qt.ToolButtonTextBesideIcon)
    b.setStyleSheet("QToolButton{padding:3px 8px}")
    return b


class MainWindow(QMainWindow):
    def __init__(self, video=None):
        super().__init__()
        self.state = AppState()
        self.runner = TaskRunner(self)
        self.frames = self.info = self.engine = self.src = None
        self.matrix = "bt709"
        self.fmt, self.full_range, self.depth_used = None, False, "8"   # processing format of the frames
        self.sel_area = None                 # Prepare page: square (y0, x0, size), coded pixels
        self.preview_sel = None              # Adjust page: preview area (y0, x0, h, w), coded pixels
        self.prof_req = 0
        self.gen = 0
        self.prof_token = 0
        self.filt_cache = collections.OrderedDict()
        self.thumb_cache = collections.OrderedDict()
        self.export = None
        self._busy_slots = set()
        self.prefs = self._load_prefs()
        self.preset_name, self.preset_clean = "default", default_params()
        self.prof_clean = None
        self.play_start = 0
        self.setWindowTitle(f"Neat Video v6 — reproduction UI (export using {self.prefs['jobs']} CPU processes)")
        self.resize(1400, 860)
        st = self.style()

        central = QWidget()
        v = QVBoxLayout(central)
        v.setContentsMargins(4, 2, 4, 2)
        v.setSpacing(2)
        top = QHBoxLayout()
        self.tabs = QTabBar()
        self.tabs.setExpanding(False)
        self.tabs.addTab(st.standardIcon(QStyle.SP_FileDialogContentsView), "Prepare Noise Profile")
        self.tabs.addTab(st.standardIcon(QStyle.SP_FileDialogDetailedView), "Adjust and Preview")
        self.tabs.setStyleSheet("QTabBar::tab{padding:5px 14px; font-weight:bold}")
        top.addWidget(self.tabs)
        top.addStretch(1)
        v.addLayout(top)

        split = QSplitter(Qt.Horizontal)
        left = QWidget()
        lv = QVBoxLayout(left)
        lv.setContentsMargins(0, 0, 0, 0)
        lv.setSpacing(2)
        self.tb_stack = QStackedWidget()
        self.tb_stack.addWidget(self._prepare_toolbar(st))
        self.tb_stack.addWidget(self._adjust_toolbar(st))
        self.tb_stack.setFixedHeight(self.tb_stack.sizeHint().height())
        lv.addWidget(self.tb_stack)
        self.viewer = Viewer()
        lv.addWidget(self.viewer, 1)
        self.viewer_bar = ViewerBar(self.viewer)
        lv.addWidget(self.viewer_bar)
        split.addWidget(left)
        self.stack = QStackedWidget()
        self.profile_page = ProfilePage()
        self.filter_page = FilterPage()
        self.stack.addWidget(self.profile_page)
        self.stack.addWidget(self.filter_page)
        self.stack.setMinimumWidth(320)
        split.addWidget(self.stack)
        split.setStretchFactor(0, 1)
        split.setStretchFactor(1, 0)
        split.setSizes([1040, 350])
        v.addWidget(split, 1)

        bottom = QHBoxLayout()
        bottom.setSpacing(4)
        for text, tip in (("P ▾", "Video Scan Type: Progressive (n/a: the reproduction processes progressive "
                                   "frames only)"),
                          ("γ ▾", "Input Data Gamma: Gamma Corrected (n/a)"),
                          ("x1.0 ▾", "Input Data Gain: x1.0 (n/a: the v5 backend has no input gain)")):
            b = QToolButton()
            b.setText(text)
            b.setToolTip(tip)
            b.setEnabled(False)
            bottom.addWidget(b, 0, Qt.AlignBottom)
        self.timeline = Timeline()
        self.timeline.thumbs.provider = self._thumb
        bottom.addWidget(self.timeline, 1)
        self.b_cancel = QPushButton("Cancel")
        self.b_cancel.setToolTip("Do not apply any changes and return (repro: closes the window)")
        self.b_cancel.clicked.connect(self.close)
        self.b_apply = QPushButton("Apply")
        self.b_apply.setDefault(True)
        self.b_apply.setToolTip("Apply the current filter settings to the preview (genuine OFX: return to host with filtered frame)")
        self.b_apply.clicked.connect(lambda: (self.invalidate(), self.schedule(), self.say("filter settings applied to preview")))
        for b in (self.b_cancel, self.b_apply):
            b.setMinimumWidth(90)
            bottom.addWidget(b, 0, Qt.AlignBottom)
        v.addLayout(bottom)
        self.setCentralWidget(central)

        sb = self.statusBar()
        self.msg = QLabel("Open a video (Effect > Open Input Video…)")
        sb.addWidget(self.msg, 1)
        self.prog = QProgressBar()
        self.prog.setMaximumWidth(200)
        self.prog.setVisible(False)
        sb.addPermanentWidget(self.prog)
        self.cancel_btn = QPushButton("Cancel export")
        self.cancel_btn.setVisible(False)
        self.cancel_btn.clicked.connect(self.cancel_export)
        sb.addPermanentWidget(self.cancel_btn)
        self.video_lbl = QLabel("")
        sb.addPermanentWidget(self.video_lbl)
        self.depth_lbl = QLabel("")
        sb.addPermanentWidget(self.depth_lbl)
        self.prof_lbl = QLabel("no profile")
        sb.addPermanentWidget(self.prof_lbl)

        self.actions = build_menus(self)
        for k in ("undo", "redo"):
            self.addAction(self.actions[k])

        fp, pp = self.filter_page.panel, self.profile_page.panel
        for panel in (fp, pp):
            panel.icons.bind_undo(self.actions["undo"], self.actions["redo"])
            panel.linkActivated.connect(self._link)
        fp.edited.connect(self.state.set_param)
        fp.editedMany.connect(lambda d: self.state.set_params(d, "Change filter settings"))
        fp.editStarted.connect(self.state.new_edit)
        fp.assistToggled.connect(self.set_assist)
        fp.noiseLevelCheck.connect(self.noise_level_check)
        fp.resetSpatial.connect(self.reset_spatial)
        fp.checkSpeed.connect(self.check_speed)
        fp.preferences.connect(self.preferences)
        fi = fp.icons
        fi.open.clicked.connect(lambda: self.load_preset())
        fi.save.clicked.connect(lambda: self.save_preset())
        fi.reset.clicked.connect(self.reset_filter)
        fi.info.clicked.connect(self.preset_description)
        for k in ("f_assign", "f_reset_default", "f_change_default"):
            fi.default_menu.addAction(self.actions[k])
        pi = pp.icons
        pi.open.clicked.connect(lambda: self.load_profile())
        pi.save.clicked.connect(lambda: self.save_profile())
        pi.reset.clicked.connect(self.reset_profile)
        pi.info.clicked.connect(self.profile_check)
        for k in ("p_assign", "p_reset_default", "p_change_default"):
            pi.default_menu.addAction(self.actions[k])
        fp.set_values(self.state.params)
        pp.autoFineTune.connect(self.auto_fine_tune)
        pp.autoComplete.connect(self.auto_complete)
        pp.resetFineTune.connect(self.reset_fine_tune)
        pp.profileCheck.connect(self.profile_check)
        pp.editDevice.connect(self.edit_device)
        pp.tuneEdited.connect(self.state.set_tune)
        pp.editStarted.connect(self.state.new_edit)
        pp.curve.edited.connect(lambda c, fs, i: self.state.set_factors(c, fs, pin=i))
        pp.curve.editedAll.connect(lambda fs, i: self.state.set_factors_all(fs, pin=i))
        pp.curve.editStarted.connect(self.state.new_edit)
        self.state.paramsChanged.connect(self.on_params)
        self.state.profileChanged.connect(self.on_profile)
        self.tabs.currentChanged.connect(self.on_page)
        self.timeline.frameChanged.connect(self.on_frame)
        self.viewer.areaSelected.connect(self._viewer_area)
        self.viewer.clicked.connect(self._viewer_click)
        self.viewer.viewChanged.connect(self._sync_view_actions)
        self.viewer_bar.info.clicked.connect(self.image_info)
        self.timer = QTimer(self)
        self.timer.setSingleShot(True)
        self.timer.setInterval(300)
        self.timer.timeout.connect(self.request_preview)

        dp = settings().value("default_preset")
        if dp:
            try:
                self.state.params = params_from_json(json.loads(dp))
                self.preset_clean = dict(self.state.params)
                fp.set_values(self.state.params)
            except (ValueError, KeyError, TypeError, AttributeError) as e:
                self.say(f"default preset ignored: {e}")
        adv = settings().value("ui/advanced")
        self.set_advanced(adv is None or str(adv).lower() in ("1", "true"))
        self._sync_view_actions()
        self.on_params(set(PARAM_SPECS))
        self.on_profile()
        self.on_page(0)
        if video:
            self.open_video(video)

    # ---------------- toolbars ----------------
    def _prepare_toolbar(self, st):
        w = QWidget()
        h = QHBoxLayout(w)
        h.setContentsMargins(0, 0, 0, 0)
        self.auto_btn = _tool("Auto Profile", "Automatically build a device noise profile using the input "
                              "image (click: Most Noisy Frame; menu: Current Frame)", icon=QStyle.SP_BrowserReload,
                              style=st)
        self.auto_btn.setPopupMode(QToolButton.MenuButtonPopup)
        self.auto_menu = QMenu(self.auto_btn)
        self.auto_btn.setMenu(self.auto_menu)
        self.auto_btn.clicked.connect(self._auto_clicked)
        self.load_btn = _tool("Load Profile", "Load a noise profile (repro JSON)", icon=QStyle.SP_DirOpenIcon,
                              style=st)
        self.load_btn.clicked.connect(lambda: self.load_profile())
        self.match_btn = _tool("Auto Match (n/a)", N_A + " (Auto Match picks a ready-made profile)")
        self.match_btn.setEnabled(False)
        self.generic_btn = _tool("Generic Profile", "repro: no genuine generic profile — loads the Default "
                                 "Profile (Profile > Assign As Default Profile), or resets the profile if none",
                                 icon=QStyle.SP_FileIcon, style=st)
        self.generic_btn.clicked.connect(self.generic_profile)
        self.check_btn = _tool("Profile Check", "Profile Check: noise graph and profile values (repro dialog)",
                               icon=QStyle.SP_FileDialogInfoView, style=st)
        self.check_btn.clicked.connect(self.profile_check)
        for b in (self.auto_btn, self.load_btn, self.match_btn, self.generic_btn):
            h.addWidget(b)
        h.addStretch(1)
        h.addWidget(self.check_btn)
        self._update_profile_button()
        return w

    def _adjust_toolbar(self, st):
        w = QWidget()
        h = QHBoxLayout(w)
        h.setContentsMargins(0, 0, 0, 0)
        self.area_btn = _tool("⛶", "Preview area: process only the selected rectangle (drag in the viewer); "
                              "without a selection the central part (Preferences: Preview Area) is selected. "
                              "Off = the whole frame", checkable=True)
        self.area_btn.toggled.connect(self._area_toggled)
        self.play_btn = _tool("Play All", "Start/Stop Playback (preview is not filtered while playing)",
                              checkable=True, icon=QStyle.SP_MediaPlay, style=st)
        self.play_btn.setPopupMode(QToolButton.MenuButtonPopup)
        m = QMenu(self.play_btn)
        m.addAction("&Play", lambda: self.play_btn.setChecked(True))
        m.addAction("Stop (&Current Frame)", lambda: self.play_btn.setChecked(False))
        m.addAction("Stop (Initia&l Frame)", self._stop_initial)
        self.play_btn.setMenu(m)
        self.play_btn.toggled.connect(self._play)
        self.preview_btn = _tool("Preview", "Show preview/original image", checkable=True,
                                 icon=QStyle.SP_DesktopIcon, style=st)
        self.preview_btn.setChecked(True)
        self.preview_btn.toggled.connect(lambda on: (self.display(), self.schedule()))
        self.fsc_btn = _tool("Filter Settings Check", "Filter Settings Check tools (repro: only Spatial Filter "
                             "Tuning = Filter Tuning Assist is available)", icon=QStyle.SP_FileDialogInfoView,
                             style=st)
        self.fsc_btn.setPopupMode(QToolButton.InstantPopup)
        m = QMenu(self.fsc_btn)
        for i, name in enumerate(("Noise Level Check", "Local Flicker Check", "Repeated Frames Check"), 1):
            a = m.addAction(f"&{i}. {name}")
            a.setEnabled(False)
            a.setToolTip(N_A)
        self.fsc_assist = m.addAction("&4. Spatial Filter Tuning")
        self.fsc_assist.setCheckable(True)
        self.fsc_assist.toggled.connect(self.set_assist)
        m.addSeparator()
        m.addAction("&Finish", lambda: self.set_assist(False))
        self.fsc_btn.setMenu(m)
        for b in (self.area_btn, self.play_btn, self.preview_btn):
            h.addWidget(b)
        h.addStretch(1)
        h.addWidget(self.fsc_btn)
        return w

    def _update_profile_button(self):
        build = self.sel_area is not None
        what = "Build Profile" if build else "Auto Profile"
        self.auto_btn.setText(what)
        self.auto_menu.clear()
        f = self.build_profile if build else self.auto_profile
        self.auto_menu.addAction(f"{what} (Current Frame)", lambda: f(False))
        self.auto_menu.addAction(f"{what} (Most Noisy Frame)", lambda: f(True))
        self.auto_btn.setToolTip("Build a device noise profile from the selected area (click: Current Frame)"
                                 if build else "Automatically build a device noise profile using the input image "
                                 "(click: Most Noisy Frame; menu: Current Frame)")

    def _auto_clicked(self):
        if self.sel_area is not None:
            self.build_profile(False)
        else:
            self.auto_profile(True)

    # ---------------- helpers ----------------
    def say(self, text):
        self.msg.setText(text)

    def _link(self, href):
        if href == "qsg":
            self.guide("tuning")
        elif href == "mode":
            self.set_advanced(not self.advanced)

    def set_advanced(self, adv):
        self.advanced = bool(adv)
        self.profile_page.panel.set_mode(self.advanced)
        self.filter_page.panel.set_mode(self.advanced)
        self.actions["advanced" if self.advanced else "beginner"].setChecked(True)
        self.actions["mode_button"].setText(("Advanced" if self.advanced else "Beginner") + " Mode ▾")
        settings().setValue("ui/advanced", self.advanced)

    def page(self):
        return self.tabs.currentIndex()

    def _update_radius(self):
        """Orange thumbnail frames = temporal window, on the Adjust page only (genuine Prepare: blue only)."""
        P = self.state.params
        self.timeline.set_radius(int(P["t_radius"]) if P["t_enable"] and self.page() == 1 else 0)

    def on_page(self, i):
        self._update_radius()
        self.stack.setCurrentIndex(i)
        self.tb_stack.setCurrentIndex(i)
        self.viewer.square = i == 0
        self._sync_selection()
        self.display()
        self.schedule()

    def _sync_view_actions(self):
        for k, a in self.actions.items():
            if k.startswith("mode_") and k != "mode_button":
                a.setChecked(k == f"mode_{self.viewer.mode()}")
            elif k.startswith("split_") and k[6:] in SPLITS:
                a.setChecked(k == f"split_{self.viewer.split()}")

    def cycle_split(self, d):
        i = SPLITS.index(self.viewer.split())
        self.viewer.set_split(SPLITS[(i + d) % len(SPLITS)])

    def _load_prefs(self):
        s = settings()
        P = dict(PREFS)
        for k, d in PREFS.items():
            v = s.value(f"prefs/{k}")
            if v is not None:
                try:
                    P[k] = type(d)(v) if not isinstance(d, bool) else str(v).lower() in ("1", "true")
                except (TypeError, ValueError):
                    pass
        return P

    def _depth(self):
        """Preferences > Bit depth as a --depth value (an unknown stored value falls back to 8)."""
        d = str(self.prefs["depth"])
        return d if d in [v for _, v in DEPTHS] else "8"

    def _ask_open(self, title, filt):
        return QFileDialog.getOpenFileName(self, title, "", filt)[0] or None

    def _ask_save(self, title, filt, name=""):
        return QFileDialog.getSaveFileName(self, title, name, filt)[0] or None

    def _info_box(self, title, text):
        box = QMessageBox(QMessageBox.Information, title, text, QMessageBox.Ok, self)
        box.setAttribute(Qt.WA_DeleteOnClose)
        box.open()
        return box

    # ---------------- video ----------------
    def _set_ofx_mode(self, ofx):
        """Disable standalone-only controls when running as an OFX host plugin backend."""
        # menu actions
        for key in ("open", "export"):
            a = self.actions.get(key)
            if a:
                a.setEnabled(not ofx)
        # toolbar buttons
        self.load_btn.setEnabled(not ofx)
        self.generic_btn.setEnabled(not ofx)
        # apply button tooltip
        if hasattr(self, "b_apply"):
            self.b_apply.setToolTip("Apply: process the current frame and emit to the host"
                                    if ofx else
                                    "Apply the current filter settings to the preview")

    def open_video(self, path=None):
        path = path or self._ask_open("Open video", "Video (*.mp4 *.mov *.mkv *.m4v *.avi *.mts *.ts);;All (*)")
        if not path:
            return
        info = probe(path)
        W, H = info["W"], info["H"]
        depth = self._depth()
        fmt = nd.choose_format(nd.source_format(info["pix_fmt"]), depth)
        matrix = nv.matrix_from_tags(info["tags"], H)
        rfmt, vf = (None, None) if fmt.legacy else (fmt, nd.read_filter(info, fmt, matrix))
        fs = fmt.internal_bytes(W, H)          # bytes of one frame in memory (uint8 or float32 planes)
        cap = max(3, int(self.prefs["mem_gb"] * 2 ** 30 // fs))
        n = info["n"] or 0
        hi = min(n, cap) if n else cap
        self.say(f"loading {os.path.basename(path)} …")
        self.prog.setVisible(True)

        def job(task):
            frames = []
            for i, f in read_frames(path, W, H, 0, hi, info["fps"], rfmt, matrix, vf):
                frames.append(tuple(np.array(p) for p in f))
                if i % 10 == 0:
                    task.report(100.0 * i / max(hi if not n else min(n, hi), 1), f"loading frame {i}")
                if task.cancelled:
                    break
            return frames

        def done(tag, frames):
            self.prog.setVisible(False)
            if not frames:
                self.say("no decodable video frames")
                return
            self.src, self.info, self.frames = path, info, frames
            self.matrix = matrix
            self.fmt, self.depth_used = fmt, depth
            self.full_range = nd.full_range_internal(info, fmt)
            self.viewer.set_matrix(self.matrix)
            self.engine = backend.PreviewEngine(frames, self.matrix, self.full_range)
            self.thumb_cache.clear()
            self.state.undo.clear()
            self.sel_area = self.preview_sel = None
            self._update_profile_button()
            fps = info["fps"]
            try:
                num, den = (fps.split("/") + ["1"])[:2]
                fpsf = float(num) / float(den)
            except (ValueError, ZeroDivisionError):
                fpsf = 24.0
            self.timeline.set_count(len(frames), fpsf)
            pf = min(len(frames) // 3, max(len(frames) - 3, 0))
            self.timeline.set_frame(pf)
            capped = n and len(frames) < n
            self.video_lbl.setText(f"{W}x{H} · preview {len(frames)}" + (f"/{n} frames (memory cap)" if capped else " frames"))
            self.video_lbl.setStyleSheet("color:#d08000" if capped else "")
            self.viewer_bar.set_frame_info(W, H, fmt.bits)
            self.depth_lbl.setText(self._depth_text())
            self.say(f"{os.path.basename(path)}: {W}x{H} {len(frames)}f {fps} fps {self.matrix}"
                     + (f" (preview holds the first {len(frames)} of {n} frames; export uses all)" if capped else ""))
            self.invalidate()
            self.on_profile()
            self.on_params(set())
            dprof = settings().value("default_profile")
            if dprof:
                try:
                    self.state.set_profile(ProfileState.from_json(json.loads(dprof)), "Default profile")
                    return
                except (ValueError, KeyError, TypeError, AttributeError) as e:
                    self.say(f"default profile ignored: {e}")
            if self.prefs["auto_profile"]:
                self.auto_profile(False)

        self.runner.submit("load", "load", job, done, self._failed, self._progress)

    def _depth_text(self):
        """Status bar: source pix_fmt / bit depth and the processing depth."""
        if self.fmt is None:
            return ""
        f = self.fmt
        proc = f"{f.bits}-bit" + (" float RGB" if f.is_float else " RGB" if f.family == "rgb" else "")
        src = nd.describe_source(self.info.get("pix_fmt")) if self.info.get("pix_fmt") else "OFX host"
        return f"source {src} · processing {proc}"

    def _progress(self, pct, text):
        if pct >= 0:
            self.prog.setVisible(True)
            self.prog.setValue(int(pct))
        if text:
            self.say(text)

    def _failed(self, tag, msg):
        self.prog.setVisible(False)
        self.say(f"error ({tag if isinstance(tag, str) else tag[0]}): {msg}")

    def _thumb(self, i):
        """Thumbnail QImage of frame i (sub-sampled planes -> RGB, display orientation)."""
        if self.frames is None or not 0 <= i < len(self.frames):
            return None
        q = self.thumb_cache.get(i)
        if q is None:
            Y, Cb, Cr = self.frames[i]
            s = max(2, Y.shape[0] // 36 // 2 * 2)
            cs = s // 2 if Cb.shape != Y.shape else s
            ys, cb, cr = Y[::s, ::s], Cb[::cs, ::cs], Cr[::cs, ::cs]
            h, w = min(ys.shape[0], cb.shape[0]), min(ys.shape[1], cb.shape[1])
            rgb = self.engine.to_rgb((ys[:h, :w], cb[:h, :w], cr[:h, :w]))
            q = to_qimage(np.rot90(rgb, (self.info["rotation"] // 90) % 4))
            self.thumb_cache[i] = q
            while len(self.thumb_cache) > 64:
                self.thumb_cache.popitem(last=False)
        return q

    # ---------------- preview ----------------
    def invalidate(self):
        self.gen += 1
        self.filt_cache.clear()

    def previewing(self):
        """Filtered preview wanted: Adjust and Preview page with Preview pressed."""
        return self.page() == 1 and self.preview_btn.isChecked()

    def schedule(self):
        if not self.timeline.playing() and self.previewing():
            self.timer.start()

    def crop(self):
        if self.frames is None or self.page() != 1 or self.preview_sel is None:
            return None
        return self.preview_sel

    def display(self):
        if self.frames is None:
            return
        t = self.timeline.frame()
        orig = self.engine.to_rgb(tuple(np.asarray(p, np.float32) for p in self.frames[t]))
        crop = self.crop()
        filt = self.filt_cache.get((t, self.gen, crop)) if self.previewing() else None
        self.viewer.set_images(orig.astype(np.uint8), None if filt is None else filt.astype(np.uint8),
                               crop if filt is not None else None, (self.info["rotation"] // 90) % 4)
        self.viewer.set_show_filtered(self.previewing())

    def request_preview(self):
        if self.frames is None or not self.previewing():
            return
        prof = self.state.effective_profile()
        if prof is None:
            self.say("no profile yet: run Auto Profile or Build Profile (Prepare Noise Profile page)")
            return
        t, gen, crop = self.timeline.frame(), self.gen, self.crop()
        if (t, gen, crop) in self.filt_cache:
            self.display()
            return
        P = self.state.backend()
        token = self.prof_token
        self.say(f"filtering frame {t}…")

        def job(task):
            filt, _ = self.engine.process(t, prof, token, P, crop)
            return filt

        def done(tag, filt):
            t2, gen2, crop2 = tag
            if gen2 != self.gen:
                return                      # settings changed meanwhile; a newer request is queued
            self.filt_cache[tag] = filt
            while len(self.filt_cache) > 8:
                self.filt_cache.popitem(last=False)
            if t2 == self.timeline.frame():
                self.display()
            self.say(f"frame {t2} filtered")

        self.runner.submit("preview", (t, gen, crop), job, done, self._failed)

    def on_frame(self, t):
        self.display()
        self._sync_selection()
        self.schedule()

    def on_params(self, keys):
        P = self.state.params
        self.filter_page.panel.set_values(P, keys)
        self._update_radius()
        dirty = any(P[k] != self.preset_clean.get(k) for k in P)
        self.filter_page.panel.name.setItemText(0, self.preset_name + (" *" if dirty else ""))
        self._update_performance()
        if keys:
            self.invalidate()
            self.display()
            self.schedule()

    def _profile_sig(self):
        return json.dumps(self.state.profile.to_json(), sort_keys=True, default=str)

    def on_profile(self):
        self.prof_token += 1
        ps = self.state.profile
        eff = self.state.effective_profile()
        fi = (self.info["W"], self.info["H"]) if self.info else None
        pp = self.profile_page.panel
        pp.show_profile(ps, eff, fi)
        pp.set_name(None if ps.base is None else f"Noise profile ({fi[0]}x{fi[1]}p)" if fi else "Noise profile",
                    ps.base is not None and self._profile_sig() != self.prof_clean)
        self.check_btn.setVisible(ps.base is not None)
        self.filter_page.panel.set_band_availability(None if eff is None else eff.get("levels"))
        if eff is None:
            self.prof_lbl.setText("no profile")
        else:
            self.prof_lbl.setText(f"{ps.device} · {eff['mode']} · σ " +
                                  " ".join(f"{k}={v:.2f}" for k, v in eff["sigma"].items()))
            sub = None if not self.frames else 1 if self.frames[0][1].shape == self.frames[0][0].shape else 2
            if sub is not None and eff.get("sub", sub) != sub:      # e.g. a 4:2:0 profile on 4:4:4 frames
                self.prof_lbl.setText(self.prof_lbl.text() + " · chroma format differs: rebuild the profile")
                self.say("the profile was made at another chroma format (bit-depth setting); export will refuse it")
        self._sync_selection()
        self.invalidate()
        self.display()
        self.schedule()

    def _sigma(self, f, y, x, h, w):
        if self.frames is None or not 0 <= f < len(self.frames):
            return None
        return backend.hh1_sigma(self.frames[f][0][y:y + h, x:x + w])

    def _sync_selection(self):
        """Selection overlay, its Noise Level label and the Noise Levels values of the panel."""
        if self.frames is None:
            return
        t = self.timeline.frame()
        base = self.state.profile.base
        main = None
        if base is not None:
            y, x, s = base["area"][:3]
            main = self._sigma(int(base.get("ui_frame", t)), y, x, s, s)
        sel = None
        if self.page() == 0:
            rect = None if self.sel_area is None else (self.sel_area[0], self.sel_area[1], self.sel_area[2],
                                                        self.sel_area[2])
            label = ""
            if rect is not None:
                sel = self._sigma(t, *rect)
                tune = self.state.profile.tune_value("all")
                label = f"Noise Level: {sel:.1f} ({(tune - 1) * 100:+.0f}%)" if sel is not None else ""
            self.viewer.set_selection(rect, "profile", label)
            self.viewer.setToolTip("Noise Level label (repro): robust luma noise sigma of the selection, "
                                   "median(|HH1|)/0.6745, and the profile's Overall Noise Level")
            self.viewer.set_profile_region(None if base is None else (base["area"][0], base["area"][1],
                                                                      base["area"][2], base["area"][2]))
        else:
            rect = self.preview_sel
            self.viewer.set_selection(rect, "preview")
            self.viewer.setToolTip("Hold the left mouse button to see the original frame; drag to select "
                                   "the preview area")
            self.viewer.set_profile_region(None)
        self.viewer_bar.set_selection(rect)
        self.profile_page.panel.set_levels(main, sel)

    def _viewer_area(self, y, x, h, w):
        if self.page() == 0:
            self.on_area(y, x, min(h, w))
        else:
            self.set_preview_area(y, x, h, w)

    def _viewer_click(self):
        if self.page() == 0 and self.sel_area is not None:
            self.sel_area = None
            self._update_profile_button()
            self._sync_selection()

    def on_area(self, y, x, s):
        self.sel_area = (y, x, s)
        self._update_profile_button()
        self.profile_page.panel.set_selection(self.sel_area)
        self._sync_selection()

    def set_preview_area(self, y, x, h, w):
        """Preview area on the Adjust page (even coordinates, at least 64 px, inside the frame)."""
        H, W = self.frames[0][0].shape
        h, w = max(64, int(h)) // 2 * 2, max(64, int(w)) // 2 * 2
        h, w = min(h, H // 2 * 2), min(w, W // 2 * 2)
        y, x = min(max(0, int(y)), H - h) // 2 * 2, min(max(0, int(x)), W - w) // 2 * 2
        self.preview_sel = (y, x, h, w)
        self.area_btn.blockSignals(True)
        self.area_btn.setChecked(True)
        self.area_btn.blockSignals(False)
        self._sync_selection()
        self.display()
        self.schedule()

    def _area_toggled(self, on):
        if on and self.frames is not None and self.preview_sel is None:
            H, W = self.frames[0][0].shape
            fr = min(max(float(self.prefs["preview_area"]), 0.1), 1.0)
            h, w = int(H * fr) // 2 * 2, int(W * fr) // 2 * 2
            self.set_preview_area((H - h) // 4 * 2, (W - w) // 4 * 2, h, w)
            return
        if not on:
            self.preview_sel = None
        self._sync_selection()
        self.display()
        self.schedule()

    def _play(self, on):
        if on:
            self.play_start = self.timeline.frame()
        self.timeline.set_playing(on)
        if not on:
            self.schedule()

    def _stop_initial(self):
        self.play_btn.setChecked(False)
        self.timeline.set_frame(self.play_start, emit=True)

    def set_assist(self, on):
        on = bool(on)
        self.viewer.set_assist(on)
        self.filter_page.panel.assist.set_checked(on)
        for a in (self.actions["f_assist"], self.fsc_assist):
            a.blockSignals(True)
            a.setChecked(on)
            a.blockSignals(False)

    def noise_level_check(self, on):
        """Genuine: Begin → shows the noise-reduced frame in the preview (mask mode).
        The user adjusts Noise Level while watching the result in real time."""
        on = bool(on)
        if on:
            # process the current frame and show the filtered result
            self.say("Noise Level Check: adjusting Noise Level updates the preview in real time")
            self.invalidate()
            self.schedule()
        else:
            self.say("Noise Level Check: off")
        # reuse the assist pipeline for the preview update on every slider change
        self._nlc_active = on

    def _update_performance(self):
        P = self.state.params
        W, H = (self.info["W"], self.info["H"]) if self.info else ("—", "—")
        bands = [n for b, n in (("high", "High"), ("medium", "Mid"), ("low", "Low"), ("very-low", "Very Low"),
                                ("ultra-low", "Ultra Low")) if any(P[f"bamt_{c}_{b}"] > 0 for c in ("Y", "Cb", "Cr"))]
        f = self.fmt
        bits = f"{f.bits}-bit" + (" float" if f.is_float else "") + f" ({f.name})" if f is not None else \
            f"{self.info['ofx_depth']}-bit (OFX host)" if self.info and self.info.get("ofx_depth") else "8-bit"
        rows = [("Frame Size:", f"{W}x{H} progressive"), ("Frame Bitdepth:", bits),
                ("Floating Point:", "float32"),
                ("Temporal Filter:", "Enabled" if P["t_enable"] else "Disabled"),
                ("&nbsp;&nbsp;Radius:", f"{P['t_radius']} frames"),
                ("Spatial Filter:", "Enabled" if P["s_enable"] else "Disabled"),
                ("&nbsp;&nbsp;Frequencies:", ", ".join(bands) or "none"),
                ("", ""), ("Using:", ""),
                ("&nbsp;&nbsp;CPU:", f"preview 1 thread, export {self.prefs['jobs']} processes "
                                    f"({os.cpu_count()} cores)")]
        self.filter_page.panel.set_performance("<table cellspacing=0 cellpadding=0>" + "".join(
            f"<tr><td>{a}&nbsp;&nbsp;</td><td>{b}</td></tr>" for a, b in rows) + "</table>")

    def check_speed(self):
        """Check Speed: process the current frame once (as the preview does) and show frames/sec."""
        prof = self.state.effective_profile()
        if self.frames is None or prof is None:
            self.say("Check Speed needs a video and a profile")
            return
        t, P = self.timeline.frame(), self.state.backend()
        engine = self.engine

        def job(task):
            idx, win = engine.window(t, int(P["radius"]), None)
            t0 = time.perf_counter()
            backend.process_frame(win, idx.index(t), prof, P)
            return time.perf_counter() - t0

        def done(tag, dt):
            self.speed_result = 1.0 / max(dt, 1e-9)
            self.filter_page.panel.speed.setText(f"{self.speed_result:.2f} frames/sec (1 frame, 1 thread)")

        self.filter_page.panel.speed.setText("measuring…")
        self.runner.submit("speed", "speed", job, done, self._failed)

    def performance_tab(self):
        self.tabs.setCurrentIndex(1)
        self.filter_page.panel.tabs.setCurrentIndex(2)

    def image_info(self):
        if self.info is None:
            return None
        i = self.info
        f = self.fmt or nd.pixfmt("yuv420p")
        return self._info_box("Image information", f"{os.path.basename(self.src)}\n{i['W']}x{i['H']}, source "
                              f"{nd.describe_source(i.get('pix_fmt'))}, read as {nd.describe(f)}\n"
                              f"fps {i['fps']}, matrix {self.matrix}, "
                              f"{'full' if i['full_range'] else 'limited'} range"
                              + (" (processed as full-range YCbCr)" if f.family == "rgb" else "")
                              + f", rotation {i['rotation']}")

    def save_frame(self, path=None):
        if self.viewer.orig is None:
            return
        path = path or self._ask_save("Save output image", "PNG (*.png)", "frame.png")
        if path:
            to_qimage(self.viewer.composite()).save(path)
            self.say(f"saved {path}")

    # ---------------- profile ----------------
    def _profile_job(self, noisy, area, text):
        if self.frames is None:
            self.say("open a video first")
            return
        self.prof_req += 1
        req = self.prof_req
        frames, full_range, matrix = self.frames, self.full_range, self.matrix
        mode = self.profile_page.panel.mode.currentText()
        cur = self.timeline.frame()
        self._busy("profile", True)
        self.say(f"{text}…")

        def job(task):
            cancel = lambda: req != self.prof_req or task.cancelled
            f = backend.noisiest_frame(frames, cancel=cancel) if noisy else cur
            return backend.build_profile(frames, f, full_range, matrix, mode, area, cancel=cancel)

        def done(tag, prof):
            if tag != req:
                return                      # superseded; the newer request clears the busy state
            self._busy("profile", False)
            new = ProfileState(base=prof, device=self.state.profile.device)
            self.state.set_profile(new, text)
            if prof["ui_frame"] != self.timeline.frame():
                self.timeline.set_frame(prof["ui_frame"])
                self.display()
                self._sync_selection()
            st = prof.get("flat_status")
            warn = "" if st in (None, 0, 2) else f" — warning: {nv.FLAT_STATUS.get(st, st)}"
            self.say(f"{text}: frame {prof['ui_frame']}, area {prof['area']}{warn}")
            # genuine Neat Video runs AutoFineTune automatically after profile build
            # (DefAutoFineTine = 1 is the default; Preferences lets the user turn it off)
            if self.prefs.get("auto_fine_tune", True):
                self.auto_fine_tune()

        def failed(tag, msg):
            if tag != req:
                return
            self._busy("profile", False)
            if "ProfileCancelled" in msg:
                return
            self._failed("profile", msg)

        self.runner.cancel("profile")
        self.runner.submit("profile", req, job, done, failed)

    def auto_profile(self, noisy=False):
        self._profile_job(noisy, None, "Auto Profile" + (" (Most Noisy Frame)" if noisy else " (Current Frame)"))

    def build_profile(self, noisy=False):
        if self.sel_area is None:
            self.say("Build Profile: drag a square over a flat area in the preview first")
            return
        self._profile_job(noisy, self.sel_area,
                          "Build Profile" + (" (Most Noisy Frame)" if noisy else " (Current Frame)"))

    def auto_fine_tune(self):
        base = self.state.profile.base
        if base is None or self.frames is None:
            self.say("Auto Fine-Tune needs a profile and the video")
            return
        if self.runner.busy("aft"):
            self.say("Auto Fine-Tune is already running")
            return
        planes = self.frames[backend.aft_frame(base, self.frames)]
        full_range, matrix = self.full_range, self.matrix
        self._busy("aft", True)
        self.say("Auto Fine-Tune (static AFT port, several seconds)…")

        def job(task):
            return backend.run_in_child(task, backend.run_aft, base, planes, full_range, matrix)

        def done(tag, prof):
            self._busy("aft", False)
            if self.state.profile.base != base:
                self.say("Auto Fine-Tune result discarded: the profile changed while it ran")
                return
            new = self.state.profile.clone()
            new.base = prof
            new.factors = [[1.0] * 9 for _ in CHANNELS]
            new.pinned = [set() for _ in CHANNELS]
            self.state.set_profile(new, "Auto Fine-Tune")
            self.say("Auto Fine-Tune applied")

        def failed(tag, msg):
            self._busy("aft", False)
            if "Cancelled" not in msg:
                self._failed("aft", msg)

        self.runner.submit("aft", "aft", job, done, failed)

    def _busy(self, slot, on):
        (self._busy_slots.add if on else self._busy_slots.discard)(slot)
        busy = bool(self._busy_slots)
        self.profile_page.panel.set_busy(busy)
        for w in (self.auto_btn, self.actions["p_aft"]):
            w.setEnabled(not busy)

    def auto_complete(self):
        new = self.state.profile.clone()
        new.factors = auto_complete(new.factors, new.pinned)
        self.state.set_profile(new, "Auto Complete")

    def edit_device(self, name=None):
        if name is None:
            name, ok = QInputDialog.getText(self, "Device Name and Mode", "Name:", text=self.state.profile.device)
            if not ok:
                return
        if name == self.state.profile.device:
            return
        new = self.state.profile.clone()
        new.device = name
        self.state.set_profile(new, "Rename device")

    def reset_fine_tune(self):
        new = self.state.profile.clone()
        new.factors = [[1.0] * 9 for _ in CHANNELS]
        new.pinned = [set() for _ in CHANNELS]
        self.state.set_profile(new, "Reset Fine-Tuning Results")

    def generic_profile(self):
        """No genuine generic profile in the reproduction: the Default Profile if one is assigned,
        otherwise no profile (Reset Profile)."""
        dprof = settings().value("default_profile")
        if dprof:
            try:
                self.state.set_profile(ProfileState.from_json(json.loads(dprof)), "Generic Profile (default)")
                self.say("Generic Profile: Default Profile loaded (repro)")
                return
            except (ValueError, KeyError, TypeError, AttributeError) as e:
                self.say(f"default profile ignored: {e}")
        self.reset_profile()
        self.say("Generic Profile: no Default Profile assigned, profile reset (repro)")

    def save_profile(self, path=None):
        if self.state.profile.base is None:
            self.say("no profile to save")
            return
        path = path or self._ask_save("Save Device Noise Profile", PROFILE_FILTER, "profile.nvprof.json")
        if path:
            dump_json(self.state.profile.to_json(), path)
            self.prof_clean = self._profile_sig()
            self.on_profile()
            self.say(f"profile saved: {path}")

    def load_profile(self, path=None):
        path = path or self._ask_open("Load profile", PROFILE_FILTER)
        if path:
            try:
                ps = ProfileState.from_json(load_json(path))
                self.prof_clean = json.dumps(ps.to_json(), sort_keys=True, default=str)
                self.state.set_profile(ps, "Load Profile")
                self.say(f"profile loaded: {path}")
            except (ValueError, KeyError, TypeError, AttributeError, OSError) as e:
                self.say(f"cannot load profile: {e}")

    def reset_profile(self):
        self.state.set_profile(ProfileState(), "Reset Profile")

    def copy_profile(self):
        if self.state.profile.base is not None:
            QGuiApplication.clipboard().setText(json.dumps(self.state.profile.to_json()))
            self.say("profile copied")

    def paste_profile(self):
        try:
            self.state.set_profile(ProfileState.from_json(json.loads(QGuiApplication.clipboard().text())),
                                   "Paste Profile")
        except (ValueError, KeyError, TypeError, AttributeError) as e:
            self.say(f"clipboard holds no profile: {e}")

    def reset_default_profile(self):
        settings().remove("default_profile")
        self.say("default profile removed")

    def assign_default_profile(self):
        if self.state.profile.base is not None:
            settings().setValue("default_profile", json.dumps(self.state.profile.to_json()))
            self.say("current profile assigned as default (used when a video is opened)")

    def change_default_profile(self, path=None):
        path = path or self._ask_open("Default profile", PROFILE_FILTER)
        if path:
            try:
                ps = ProfileState.from_json(load_json(path))
            except (ValueError, KeyError, TypeError, AttributeError, OSError) as e:
                self.say(f"cannot use as default: {e}")
                return
            settings().setValue("default_profile", json.dumps(ps.to_json()))
            self.say(f"default profile: {path}")

    def profile_check(self):
        eff = self.state.effective_profile()
        dlg = QDialog(self)
        dlg.setWindowTitle("Profile Check")
        dlg.setAttribute(Qt.WA_DeleteOnClose)
        v = QVBoxLayout(dlg)
        g = NoiseGraph()
        g.setMinimumSize(520, 260)
        g.set_profile(eff, self.state.profile.base)
        v.addWidget(g)
        t = QTextEdit()
        t.setReadOnly(True)
        if eff is None:
            t.setPlainText("no profile")
        else:
            st = eff.get("flat_status")
            lines = [f"device: {self.state.profile.device}", f"mode: {eff['mode']}  frame: {eff.get('ui_frame')}  "
                     f"area (y, x, size): {eff['area']}  [{'manual' if st is None else nv.FLAT_STATUS.get(st, st)}]",
                     f"static pixels: {eff.get('static_fraction', 0):.1%}  AFT: {bool(eff.get('aft_applied'))}",
                     "σ total @area: " + ", ".join(f"{k}={v:.3f}" for k, v in eff["sigma"].items()),
                     "profile Noise Level: " + ", ".join(f"{k} {(self.state.profile.tune_value(k) - 1) * 100:+.0f}%"
                                                         for k in ("all", "Y", "Cr", "Cb"))]
            for n in ("Y", "Cb", "Cr"):
                lines.append(f"{n} band ratio " + " ".join(f"{r:.3f}" for r in eff["band_ratio"][n]))
            lines.append("brightness bins " + " ".join(f"{c:.0f}" for c in eff["curve_centers"]))
            for n in ("Y", "Cb", "Cr"):
                lines.append(f"{n} σ1 curve " + " ".join(f"{v:.3f}" for v in eff["curve"][n]))
            lines.append("Overall Profile Quality: not computed (genuine measure not decoded)")
            t.setPlainText("\n".join(lines))
        v.addWidget(t)
        dlg.resize(640, 560)
        dlg.show()
        return dlg

    def guide(self, which):
        title, text = GUIDES[which]
        return self._info_box(title, text)

    # ---------------- filter presets ----------------
    def save_preset(self, path=None):
        path = path or self._ask_save("Save Filter Preset", PRESET_FILTER, "preset.nvpreset.json")
        if path:
            dump_json(preset_json(self.state.params), path)
            self._preset_named(os.path.basename(path), self.state.params)
            self.say(f"preset saved: {path}")

    def load_preset(self, path=None):
        path = path or self._ask_open("Load Filter Preset", PRESET_FILTER)
        if path:
            try:
                P = params_from_json(load_json(path))
                self._preset_named(os.path.basename(path), P)
                self.state.set_all_params(P, "Load Filter Preset")
                self.say(f"preset loaded: {path}")
            except (ValueError, KeyError, TypeError, AttributeError, OSError) as e:
                self.say(f"cannot load preset: {e}")

    def _default_preset(self):
        dp = settings().value("default_preset")
        if dp:
            try:
                return params_from_json(json.loads(dp))
            except (ValueError, KeyError, TypeError, AttributeError):
                pass
        return default_params()

    def _preset_named(self, name, params):
        for suf in (".nvpreset.json", ".json"):
            if name.endswith(suf):
                name = name[:-len(suf)]
                break
        self.preset_name, self.preset_clean = name, dict(params)
        self.on_params(set())

    def reset_filter(self):
        P = self._default_preset()
        self._preset_named("default", P)
        self.state.set_all_params(P, "Reset Filter Settings")

    def reset_spatial(self):
        """↺ of the Spatial Filter header: the Spatial tab's settings back to the default preset."""
        D = self._default_preset()
        self.state.set_params({k: D[k] for k in PARAM_SPECS if k.startswith(SPATIAL_PREFIXES)},
                              "Reset Spatial Settings", merge=False)

    def reset_default_preset(self):
        settings().remove("default_preset")
        self.say("default preset removed (factory settings are used)")

    def preset_description(self):
        D = default_params()
        ch = [f"{PARAM_SPECS[k].label}: {D[k]} → {v}" for k, v in self.state.params.items() if v != D[k]]
        return self._info_box("Preset Description", f"Preset: {self.preset_name}\n\n" + (
            "Changed from the factory settings:\n" + "\n".join(ch) if ch else "Factory settings (no changes)."))

    def copy_both(self):
        QGuiApplication.clipboard().setText(json.dumps({"profile": self.state.profile.to_json()
                                                        if self.state.profile.base is not None else None,
                                                        "preset": preset_json(self.state.params)}))
        self.say("profile and preset copied")

    def paste_both(self):
        try:
            d = json.loads(QGuiApplication.clipboard().text())
            P = params_from_json(d["preset"])
            ps = ProfileState.from_json(d["profile"]) if d.get("profile") is not None else None
        except (ValueError, KeyError, TypeError, AttributeError) as e:
            self.say(f"clipboard holds no profile and preset: {e}")
            return
        self.state.undo.beginMacro("Paste Profile and Preset")
        if ps is not None:
            self.state.set_profile(ps, "Paste Profile")
        self.state.set_all_params(P, "Paste Filter Settings")
        self.state.undo.endMacro()

    def copy_filter(self):
        QGuiApplication.clipboard().setText(json.dumps(preset_json(self.state.params)))
        self.say("filter settings copied")

    def paste_filter(self):
        try:
            self.state.set_all_params(params_from_json(json.loads(QGuiApplication.clipboard().text())),
                                      "Paste Filter Settings")
        except (ValueError, KeyError, TypeError, AttributeError) as e:
            self.say(f"clipboard holds no filter settings: {e}")

    def assign_default_preset(self):
        settings().setValue("default_preset", json.dumps(preset_json(self.state.params)))
        self.say("current filter settings assigned as default")

    def change_default_preset(self, path=None):
        path = path or self._ask_open("Default filter preset", PRESET_FILTER)
        if path:
            try:
                P = params_from_json(load_json(path))
            except (ValueError, KeyError, TypeError, AttributeError, OSError) as e:
                self.say(f"cannot use as default: {e}")
                return
            settings().setValue("default_preset", json.dumps(preset_json(P)))
            self.say(f"default preset: {path}")

    # ---------------- export ----------------
    def process_video(self, dst=None, start=None, frames=None):
        if self.src is None:
            self.say("open a video first")
            return
        prof = self.state.effective_profile()
        if prof is None:
            self.say("no profile: run Auto Profile first")
            return
        if self.export is not None:
            self.say("an export is already running")
            return
        if dst is None:
            stem = os.path.splitext(os.path.basename(self.src))[0]
            filt, ext = self._export_filter()
            dst = self._ask_save("Process full video", filt,
                                 os.path.join(os.path.dirname(self.src), f"{stem}_nvqt{ext}"))
            if not dst:
                return
            if dst.lower().endswith(".exr") and "%" not in os.path.basename(dst):
                dst = dst[:-4] + "_%05d.exr"    # EXR sequence: frame-number pattern
        dst = os.path.abspath(dst)              # the export runs with repro/ as working directory
        base = sidecar_stem(dst)                # without %05d for an EXR sequence
        ppath, fpath = base + ".ui_params.json", base + ".ui_profile.json"
        dump_json(self.state.backend(), ppath)
        dump_json(prof, fpath)
        args = [self.src, dst, "--ui-profile", fpath, "--params", ppath, "--jobs", str(self.prefs["jobs"]),
                "--crf", str(self.prefs["crf"]), "--audio", self.prefs["audio"], "--depth", self.depth_used]
        if start is not None:
            args += ["--start", str(int(start))]
        if frames is not None:
            args += ["--frames", str(int(frames))]
        self.export = ExportProcess(args, REPRO, self)
        self.export.progress.connect(self._progress)
        self.export.finished.connect(self._export_done)
        self.prog.setVisible(True)
        self.prog.setValue(0)
        self.cancel_btn.setVisible(True)
        self.actions["export"].setEnabled(False)
        self.b_apply.setEnabled(False)
        self.export_dst = dst
        self.export_cancelled = False
        self.export_files = [dst, base + ".json", ppath, fpath]   # denoise_video_v5 writes <stem>.json
        self.export.start()
        self.say(f"exporting to {dst} …")

    def _export_done(self, ok, text):
        self.prog.setVisible(False)
        self.cancel_btn.setVisible(False)
        self.actions["export"].setEnabled(True)
        self.b_apply.setEnabled(True)
        self.export = None
        self.export_ok = ok
        if not ok and self.export_cancelled:      # remove what this export wrote (partial mp4, JSON)
            self._remove_export_files(self.export_files)
        self.say(f"wrote {self.export_dst}" if ok else
                 "export cancelled" if self.export_cancelled else f"export failed: {text}")

    def _export_filter(self):
        """Save-dialog filter and default extension for the processing depth (16 / 32 bits: .mkv,
        32 bits also an EXR sequence; denoise_video_v5.pick_codec)."""
        f = self.fmt or nd.pixfmt("yuv420p")
        if f.bits >= 16 or f.family == "rgb":
            return ("Matroska FFV1 (*.mkv)" + (";;EXR sequence (*.exr)" if f.is_float else ""), ".mkv")
        if f.legacy:
            return "MP4 (*.mp4)", ".mp4"
        return "MP4 (*.mp4);;QuickTime (*.mov);;Matroska FFV1 (*.mkv)", ".mp4"

    @staticmethod
    def _remove_export_files(files):
        """Remove a cancelled export's files; an EXR pattern removes the frames it numbers."""
        for p in files:
            remove_output(p)

    def cancel_export(self):
        if self.export is not None:
            self.export_cancelled = True
            self.export.kill()

    # ---------------- tools ----------------
    def preferences(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("Preferences")
        f = QFormLayout(dlg)
        jobs = QSpinBox()
        jobs.setRange(1, 64)
        jobs.setValue(int(self.prefs["jobs"]))
        crf = QSpinBox()
        crf.setRange(0, 51)
        crf.setValue(int(self.prefs["crf"]))
        audio = QComboBox()
        audio.addItems(["copy", "aac", "none"])
        audio.setCurrentText(self.prefs["audio"])
        area = QDoubleSpinBox()
        area.setRange(0.1, 1.0)
        area.setSingleStep(0.05)
        area.setValue(float(self.prefs["preview_area"]))
        mem = QDoubleSpinBox()
        mem.setRange(0.25, 64.0)
        mem.setValue(float(self.prefs["mem_gb"]))
        auto = QCheckBox("Auto Profile when a video is opened")
        auto.setChecked(bool(self.prefs["auto_profile"]))
        depth = QComboBox()
        for label, val in DEPTHS:
            depth.addItem(label, val)
        depth.setCurrentIndex(max(depth.findData(self._depth()), 0))
        depth.setToolTip("Bit depth of loading, profiling and export (nv_depth.py). 8-bit = yuv420p as before; "
                         "10/12/16 keep the source's chroma format (RGB sources: gbrp), 32 = RGB float. "
                         "Changing it reloads the video")
        f.addRow("Computing: export processes (jobs)", jobs)
        f.addRow("Output: x264 CRF", crf)
        f.addRow("Output: audio", audio)
        f.addRow("Preview Area (fraction of the frame)", area)
        f.addRow("Preview cache: frames in memory (GB)", mem)
        f.addRow("Bit depth", depth)
        f.addRow(auto)
        bb = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        bb.accepted.connect(dlg.accept)
        bb.rejected.connect(dlg.reject)
        f.addRow(bb)

        def accept():
            reload = str(depth.currentData()) != str(self.prefs["depth"])
            self.prefs.update(jobs=jobs.value(), crf=crf.value(), audio=audio.currentText(),
                              preview_area=area.value(), mem_gb=mem.value(), auto_profile=auto.isChecked(),
                              depth=str(depth.currentData()))
            s = settings()
            for k, v in self.prefs.items():
                s.setValue(f"prefs/{k}", v)
            if reload and self.src is not None and self.export is None and self.src != "<OFX host>":
                self.open_video(self.src)       # frames are held at the processing depth
                return
            self.invalidate()
            self.schedule()

        dlg.accepted.connect(accept)
        dlg.setAttribute(Qt.WA_DeleteOnClose)
        dlg.open()
        return dlg

    def batch_profiler(self, files=None, outdir=None, mode=None):
        """Profiles each video (frame N//3, like denoise_video_v5) and saves <stem>.nvprof.json."""
        if files is not None and outdir is not None:
            return self._run_batch(list(files), outdir, mode or "auto")
        dlg = QDialog(self)
        dlg.setWindowTitle("Batch Profiler")
        dlg.setAttribute(Qt.WA_DeleteOnClose)
        v = QVBoxLayout(dlg)
        lst = QListWidget()
        v.addWidget(lst)
        row = QHBoxLayout()
        add = QPushButton("Add videos…")
        add.clicked.connect(lambda: lst.addItems(QFileDialog.getOpenFileNames(dlg, "Videos", "", "Video (*)")[0]))
        row.addWidget(add)
        mbox = QComboBox()
        mbox.addItems(["auto", "spatial", "temporal"])
        row.addWidget(QLabel("mode"))
        row.addWidget(mbox)
        v.addLayout(row)
        run = QPushButton("Choose output folder and run")
        v.addWidget(run)

        def go():
            d = QFileDialog.getExistingDirectory(dlg, "Output folder")
            if d:
                self._run_batch([lst.item(i).text() for i in range(lst.count())], d, mbox.currentText())
                dlg.accept()

        run.clicked.connect(go)
        dlg.open()
        return dlg

    def _run_batch(self, files, outdir, mode):
        self.batch_result = None
        depth = self._depth()                       # same bit-depth setting as loading / export

        def job(task):
            out = []
            for k, path in enumerate(files):
                task.report(100.0 * k / max(len(files), 1), f"batch: profiling {os.path.basename(path)}")
                info = probe(path)
                W, H = info["W"], info["H"]
                fmt = nd.choose_format(nd.source_format(info["pix_fmt"]), depth)
                matrix = nv.matrix_from_tags(info["tags"], H)
                rd = (None, matrix, None) if fmt.legacy else (fmt, matrix, nd.read_filter(info, fmt, matrix))
                pf = max(min((info["n"] or 0) // 3, (info["n"] or 0) - 3), 0)
                fr = [tuple(np.array(p) for p in f) for _, f in read_frames(path, W, H, pf, pf + 3, info["fps"], *rd)]
                if not fr:
                    fr = [tuple(np.array(p) for p in f) for _, f in read_frames(path, W, H, 0, 3, info["fps"], *rd)]
                prof = backend.build_profile(fr, 0, nd.full_range_internal(info, fmt), matrix, mode)
                prof["ui_frame"] = pf
                dst = os.path.join(outdir, os.path.splitext(os.path.basename(path))[0] + ".nvprof.json")
                dump_json(ProfileState(base=prof, device=os.path.basename(path)).to_json(), dst)
                out.append(dst)
            return out

        def done(tag, out):
            self.prog.setVisible(False)
            self.batch_result = out
            self.say(f"batch profiler: wrote {len(out)} profiles to {outdir}")

        self.runner.submit("batch", "batch", job, done, self._failed, self._progress)

    def about(self):
        return self._info_box("About", "Reproduction UI of Neat Video v6 (PySide6).\n"
                              "Backend: repro/nvfilter.py (v5 pipeline), nv_aft_v5.py (static AFT port).\n"
                              "Not affiliated with ABSoft; profiles and presets are JSON, not .dnp / .nfp.")

    def closeEvent(self, e):
        self.prof_req += 1                  # cancels a running profile search
        if self.export is not None:
            files = list(self.export_files)
            self.cancel_export()
            self.export.p.waitForFinished(5000)
            self._remove_export_files(files)    # same clean-up as a cancelled export
        self.runner.wait_all()
        super().closeEvent(e)
