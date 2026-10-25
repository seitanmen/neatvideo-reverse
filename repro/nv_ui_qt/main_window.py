"""Main window: shared preview + Prepare Profile / Filter Settings pages, menus, background jobs."""
import collections
import json
import os

import numpy as np
from PySide6.QtCore import QSettings, Qt, QTimer
from PySide6.QtGui import QGuiApplication
from PySide6.QtWidgets import (QCheckBox, QComboBox, QDialog, QDialogButtonBox, QDoubleSpinBox, QFileDialog,
                               QFormLayout, QHBoxLayout, QInputDialog, QLabel, QListWidget, QMainWindow,
                               QMessageBox, QProgressBar, QPushButton, QSpinBox, QSplitter, QStackedWidget,
                               QTabBar, QTextEdit, QVBoxLayout, QWidget)

import nvfilter as nv
from denoise_video_v5 import probe, read_frames

from . import backend
from .menus import build_menus
from .pages.filter_page import FilterPage
from .pages.profile_page import ProfilePage
from .state import (CHANNELS, AppState, ProfileState, auto_complete, dump_json, load_json, params_from_json,
                    preset_json, default_params)
from .widgets.noise_graph import NoiseGraph
from .widgets.preview import PreviewWidget, to_qimage
from .worker import ExportProcess, TaskRunner

REPRO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROFILE_FILTER = "Repro noise profile (*.nvprof.json);;JSON (*.json)"
PRESET_FILTER = "Repro filter preset (*.nvpreset.json);;JSON (*.json)"
PREFS = {"jobs": max(1, (os.cpu_count() or 2) - 2), "crf": 16, "audio": "copy", "preview_area": 0.5,
         "mem_gb": 4.0, "auto_profile": True}


def settings():
    """QSettings (INI); NV_UI_QT_SETTINGS=<file> selects another file (tests)."""
    path = os.environ.get("NV_UI_QT_SETTINGS")
    if path:
        return QSettings(path, QSettings.IniFormat)
    return QSettings(QSettings.IniFormat, QSettings.UserScope, "NeatRepro", "nv_ui_qt")


GUIDES = {
    "filter": ("Filter Settings Check",
               "Compare Original / Filtered / Split on a detailed and a flat part of the frame.\n\n"
               "• Residual noise in flat areas → raise Noise Level (Components or per band) or the "
               "Noise Reduction Amount.\n"
               "• Lost detail or waxy texture → lower the Noise Reduction Amount of the high / mid bands, "
               "or the Noise Level.\n"
               "• Use Assist to see what is removed: structure (edges, texture) in the Assist image means "
               "the filter is too strong.\n\n(Guide text written for the reproduction.)"),
    "noise": ("Noise Level Check",
              "The Noise Level sliders scale the profile's noise estimate (100% = profile).\n"
              "Set them where flat areas just become clean in the Filtered view; going further treats "
              "image detail as noise.\n\n(Guide text written for the reproduction.)"),
    "tuning": ("Tuning Guide",
               "1. Auto Profile on a frame with a flat, unclipped area (or drag an area and Build Profile).\n"
               "2. Check the noise graph (Profile Check): sigma should vary smoothly with brightness.\n"
               "3. Auto Fine-Tune adjusts the per-brightness factors (static port of the genuine AFT).\n"
               "4. Manual Fine-Tune: drag the 9 points (bins 0, 32, …, 256) of each component; "
               "Auto Complete interpolates the bins you did not touch.\n\n"
               "(Guide text written for the reproduction.)"),
}


class MainWindow(QMainWindow):
    def __init__(self, video=None):
        super().__init__()
        self.setWindowTitle("Neat Video v6 (Reduce Noise v6) — reproduction UI")
        self.resize(1400, 860)
        self.state = AppState()
        self.runner = TaskRunner(self)
        self.frames = self.info = self.engine = self.src = None
        self.matrix = "bt709"
        self.sel_area = None
        self.prof_req = 0
        self.gen = 0
        self.prof_token = 0
        self.filt_cache = collections.OrderedDict()
        self.export = None
        self._busy_slots = set()
        self.prefs = self._load_prefs()

        central = QWidget()
        v = QVBoxLayout(central)
        v.setContentsMargins(4, 4, 4, 0)
        self.tabs = QTabBar()
        self.tabs.addTab("Prepare Profile")
        self.tabs.addTab("Filter Settings")
        v.addWidget(self.tabs)
        split = QSplitter(Qt.Horizontal)
        self.preview = PreviewWidget()
        split.addWidget(self.preview)
        self.stack = QStackedWidget()
        self.profile_page = ProfilePage()
        self.filter_page = FilterPage()
        self.stack.addWidget(self.profile_page)
        self.stack.addWidget(self.filter_page)
        self.stack.setMinimumWidth(440)
        split.addWidget(self.stack)
        split.setStretchFactor(0, 1)
        v.addWidget(split, 1)
        self.setCentralWidget(central)
        self.tabs.currentChanged.connect(self.stack.setCurrentIndex)

        sb = self.statusBar()
        self.msg = QLabel("Open a video (File > Open Video…)")
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
        self.prof_lbl = QLabel("no profile")
        sb.addPermanentWidget(self.prof_lbl)

        self.actions = build_menus(self)

        fp, pp = self.filter_page.panel, self.profile_page.panel
        fp.edited.connect(self.state.set_param)
        fp.editStarted.connect(self.state.new_edit)
        fp.apply.connect(self.process_video)
        fp.set_values(self.state.params)
        pp.autoProfile.connect(self.auto_profile)
        pp.buildProfile.connect(self.build_profile)
        pp.autoFineTune.connect(self.auto_fine_tune)
        pp.autoComplete.connect(self.auto_complete)
        pp.profileCheck.connect(self.profile_check)
        pp.tuningGuide.connect(lambda: self.guide("tuning"))
        pp.editDevice.connect(self.edit_device)
        pp.curve.edited.connect(lambda c, fs, i: self.state.set_factors(c, fs, pin=i))
        pp.curve.editStarted.connect(self.state.new_edit)
        self.state.paramsChanged.connect(self.on_params)
        self.state.profileChanged.connect(self.on_profile)
        self.preview.frameChanged.connect(self.on_frame)
        self.preview.areaSelected.connect(self.on_area)
        self.preview.previewAreaToggled.connect(lambda on: self.schedule())
        self.preview.play.toggled.connect(lambda on: None if on else self.schedule())
        self.timer = QTimer(self)
        self.timer.setSingleShot(True)
        self.timer.setInterval(300)
        self.timer.timeout.connect(self.request_preview)

        dp = settings().value("default_preset")
        if dp:
            try:
                self.state.params = params_from_json(json.loads(dp))
                fp.set_values(self.state.params)
            except (ValueError, KeyError, TypeError, AttributeError) as e:
                self.say(f"default preset ignored: {e}")
        self.on_profile()
        if video:
            self.open_video(video)

    # ---------------- helpers ----------------
    def say(self, text):
        self.msg.setText(text)

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
    def open_video(self, path=None):
        path = path or self._ask_open("Open video", "Video (*.mp4 *.mov *.mkv *.m4v *.avi *.mts *.ts);;All (*)")
        if not path:
            return
        info = probe(path)
        W, H = info["W"], info["H"]
        fs = W * H + 2 * ((W + 1) // 2) * ((H + 1) // 2)
        cap = max(3, int(self.prefs["mem_gb"] * 2 ** 30 // fs))
        n = info["n"] or 0
        hi = min(n, cap) if n else cap
        self.say(f"loading {os.path.basename(path)} …")
        self.prog.setVisible(True)

        def job(task):
            frames = []
            for i, f in read_frames(path, W, H, 0, hi, fps=info["fps"]):
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
            self.matrix = nv.matrix_from_tags(info["tags"], H)
            self.engine = backend.PreviewEngine(frames, self.matrix, info["full_range"])
            self.state.undo.clear()
            self.sel_area = None
            self.preview.set_selection(None)
            fps = info["fps"]
            try:
                num, den = (fps.split("/") + ["1"])[:2]
                fpsf = float(num) / float(den)
            except (ValueError, ZeroDivisionError):
                fpsf = 24.0
            self.preview.set_count(len(frames), fpsf)
            pf = min(len(frames) // 3, max(len(frames) - 3, 0))
            self.preview.set_frame(pf)
            capped = n and len(frames) < n
            self.video_lbl.setText(f"{W}x{H} · preview {len(frames)}" + (f"/{n} frames (memory cap)" if capped else " frames"))
            self.video_lbl.setStyleSheet("color:#d08000" if capped else "")
            self.say(f"{os.path.basename(path)}: {W}x{H} {len(frames)}f {fps} fps {self.matrix}"
                     + (f" (preview holds the first {len(frames)} of {n} frames; export uses all)" if capped else ""))
            self.invalidate()
            self.display()
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

    def _progress(self, pct, text):
        if pct >= 0:
            self.prog.setVisible(True)
            self.prog.setValue(int(pct))
        if text:
            self.say(text)

    def _failed(self, tag, msg):
        self.prog.setVisible(False)
        self.say(f"error ({tag if isinstance(tag, str) else tag[0]}): {msg}")

    # ---------------- preview ----------------
    def invalidate(self):
        self.gen += 1
        self.filt_cache.clear()

    def schedule(self):
        if not self.preview.playing():
            self.timer.start()

    def crop(self):
        if not self.preview.area_cb.isChecked() or self.frames is None:
            return None
        H, W = self.frames[0][0].shape
        fr = min(max(float(self.prefs["preview_area"]), 0.1), 1.0)
        h, w = int(H * fr) // 2 * 2, int(W * fr) // 2 * 2
        return ((H - h) // 4 * 2, (W - w) // 4 * 2, h, w)

    def display(self):
        if self.frames is None:
            return
        t = self.preview.frame()
        orig = self.engine.to_rgb(tuple(np.asarray(p, np.float32) for p in self.frames[t]))
        crop = self.crop()
        filt = self.filt_cache.get((t, self.gen, crop))
        self.preview.set_images(orig.astype(np.uint8), None if filt is None else filt.astype(np.uint8),
                                crop if filt is not None else None, (self.info["rotation"] // 90) % 4)

    def request_preview(self):
        if self.frames is None:
            return
        prof = self.state.effective_profile()
        if prof is None:
            self.say("no profile yet: run Auto Profile or Build Profile (Prepare Profile page)")
            return
        t, gen, crop = self.preview.frame(), self.gen, self.crop()
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
            if t2 == self.preview.frame():
                self.display()
            self.say(f"frame {t2} filtered")

        self.runner.submit("preview", (t, gen, crop), job, done, self._failed)

    def on_frame(self, t):
        self.display()
        self.schedule()

    def on_params(self, keys):
        self.filter_page.panel.set_values(self.state.params, keys)
        self.invalidate()
        self.display()
        self.schedule()

    def on_profile(self):
        self.prof_token += 1
        ps = self.state.profile
        eff = self.state.effective_profile()
        self.profile_page.panel.show_profile(ps, eff)
        self.filter_page.panel.set_band_availability(None if eff is None else eff.get("levels"))
        if eff is None:
            self.prof_lbl.setText("no profile")
        else:
            self.prof_lbl.setText(f"{ps.device} · {eff['mode']} · σ " +
                                  " ".join(f"{k}={v:.2f}" for k, v in eff["sigma"].items()))
        self.invalidate()
        self.display()
        self.schedule()

    def on_area(self, y, x, s):
        self.sel_area = (y, x, s)
        self.preview.set_selection(self.sel_area)
        self.profile_page.panel.set_selection(self.sel_area)

    def save_frame(self, path=None):
        if self.preview.orig is None:
            return
        path = path or self._ask_save("Save preview frame", "PNG (*.png)", "frame.png")
        if path:
            self.preview.label.pixmap().save(path)
            self.say(f"saved {path}")

    # ---------------- profile ----------------
    def _profile_job(self, noisy, area, text):
        if self.frames is None:
            self.say("open a video first")
            return
        self.prof_req += 1
        req = self.prof_req
        frames, full_range, matrix = self.frames, self.info["full_range"], self.matrix
        mode = self.profile_page.panel.mode.currentText()
        cur = self.preview.frame()
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
            if prof["ui_frame"] != self.preview.frame():
                self.preview.set_frame(prof["ui_frame"])
                self.display()
            st = prof.get("flat_status")
            warn = "" if st in (None, 0, 2) else f" — warning: {nv.FLAT_STATUS.get(st, st)}"
            self.say(f"{text}: frame {prof['ui_frame']}, area {prof['area']}{warn}")

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
        full_range, matrix = self.info["full_range"], self.matrix
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
        self.profile_page.panel.set_busy(bool(self._busy_slots))

    def auto_complete(self):
        new = self.state.profile.clone()
        new.factors = auto_complete(new.factors, new.pinned)
        self.state.set_profile(new, "Auto Complete")

    def edit_device(self, name=None):
        if name is None:
            name, ok = QInputDialog.getText(self, "Device Name and Mode", "Name:", text=self.state.profile.device)
            if not ok:
                return
        new = self.state.profile.clone()
        new.device = name
        self.state.set_profile(new, "Rename device")

    def save_profile(self, path=None):
        if self.state.profile.base is None:
            self.say("no profile to save")
            return
        path = path or self._ask_save("Save Device Noise Profile", PROFILE_FILTER, "profile.nvprof.json")
        if path:
            dump_json(self.state.profile.to_json(), path)
            self.say(f"profile saved: {path}")

    def load_profile(self, path=None):
        path = path or self._ask_open("Load profile", PROFILE_FILTER)
        if path:
            try:
                self.state.set_profile(ProfileState.from_json(load_json(path)), "Load Profile")
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
                     "σ total @area: " + ", ".join(f"{k}={v:.3f}" for k, v in eff["sigma"].items())]
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
            self.say(f"preset saved: {path}")

    def load_preset(self, path=None):
        path = path or self._ask_open("Load Filter Preset", PRESET_FILTER)
        if path:
            try:
                self.state.set_all_params(params_from_json(load_json(path)), "Load Filter Preset")
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

    def reset_filter(self):
        self.state.set_all_params(self._default_preset(), "Reset Filter Settings")

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
            dst = self._ask_save("Process full video", "MP4 (*.mp4)",
                                 os.path.join(os.path.dirname(self.src), f"{stem}_nvqt.mp4"))
            if not dst:
                return
        dst = os.path.abspath(dst)              # the export runs with repro/ as working directory
        base = os.path.splitext(dst)[0]
        ppath, fpath = base + ".ui_params.json", base + ".ui_profile.json"
        dump_json(self.state.backend(), ppath)
        dump_json(prof, fpath)
        args = [self.src, dst, "--ui-profile", fpath, "--params", ppath, "--jobs", str(self.prefs["jobs"]),
                "--crf", str(self.prefs["crf"]), "--audio", self.prefs["audio"]]
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
        self.filter_page.panel.b_apply.setEnabled(False)
        self.export_dst = dst
        self.export_cancelled = False
        self.export_files = [dst, base + ".json", ppath, fpath]   # denoise_video_v5 writes <stem>.json
        self.export.start()
        self.say(f"exporting to {dst} …")

    def _export_done(self, ok, text):
        self.prog.setVisible(False)
        self.cancel_btn.setVisible(False)
        self.actions["export"].setEnabled(True)
        self.filter_page.panel.b_apply.setEnabled(True)
        self.export = None
        self.export_ok = ok
        if not ok and self.export_cancelled:      # remove what this export wrote (partial mp4, JSON)
            for p in self.export_files:
                try:
                    os.remove(p)
                except OSError:
                    pass
        self.say(f"wrote {self.export_dst}" if ok else
                 "export cancelled" if self.export_cancelled else f"export failed: {text}")

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
        f.addRow("Computing: export processes (jobs)", jobs)
        f.addRow("Output: x264 CRF", crf)
        f.addRow("Output: audio", audio)
        f.addRow("Preview Area (fraction of the frame)", area)
        f.addRow("Preview cache: frames in memory (GB)", mem)
        f.addRow(auto)
        bb = QDialogButtonBox(QDialogButtonBox.Ok | QDialogButtonBox.Cancel)
        bb.accepted.connect(dlg.accept)
        bb.rejected.connect(dlg.reject)
        f.addRow(bb)

        def accept():
            self.prefs.update(jobs=jobs.value(), crf=crf.value(), audio=audio.currentText(),
                              preview_area=area.value(), mem_gb=mem.value(), auto_profile=auto.isChecked())
            s = settings()
            for k, v in self.prefs.items():
                s.setValue(f"prefs/{k}", v)
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

        def job(task):
            out = []
            for k, path in enumerate(files):
                task.report(100.0 * k / max(len(files), 1), f"batch: profiling {os.path.basename(path)}")
                info = probe(path)
                W, H = info["W"], info["H"]
                pf = max(min((info["n"] or 0) // 3, (info["n"] or 0) - 3), 0)
                fr = [tuple(np.array(p) for p in f) for _, f in read_frames(path, W, H, pf, pf + 3, info["fps"])]
                if not fr:
                    fr = [tuple(np.array(p) for p in f) for _, f in read_frames(path, W, H, 0, 3, info["fps"])]
                prof = backend.build_profile(fr, 0, info["full_range"], nv.matrix_from_tags(info["tags"], H), mode)
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
            for p in files:                 # same clean-up as a cancelled export
                try:
                    os.remove(p)
                except OSError:
                    pass
        self.runner.wait_all()
        super().closeEvent(e)
