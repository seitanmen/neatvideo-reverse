"""Prepare Profile side panel (Device Noise Profile, Auto/Build Profile, Fine-Tune, quality)."""
from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (QComboBox, QGridLayout, QGroupBox, QHBoxLayout, QLabel, QProgressBar,
                               QPushButton, QVBoxLayout, QWidget)

import nvfilter as nv

from .curve_editor import CurveEditor
from .noise_graph import NoiseGraph


class DeviceLabel(QLabel):
    doubleClicked = Signal()

    def mouseDoubleClickEvent(self, e):
        self.doubleClicked.emit()


class ProfilePanel(QWidget):
    autoProfile = Signal(bool)          # most noisy frame?
    buildProfile = Signal(bool)
    autoFineTune = Signal()
    autoComplete = Signal()
    profileCheck = Signal()
    tuningGuide = Signal()
    editDevice = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        v = QVBoxLayout(self)
        g = QGroupBox("Device Noise Profile")
        gl = QGridLayout(g)
        gl.addWidget(QLabel("Device Name and Mode:"), 0, 0)
        self.device = DeviceLabel("Unnamed device")
        self.device.setToolTip("Double click to edit")
        self.device.setStyleSheet("font-weight:bold")
        self.device.doubleClicked.connect(self.editDevice)
        gl.addWidget(self.device, 0, 1)
        gl.addWidget(QLabel("Input Data Gain:"), 1, 0)
        self.gain = QPushButton("x1.0 (n/a)")
        self.gain.setEnabled(False)
        self.gain.setToolTip("Not available in the reproduction (the v5 backend has no input gain)")
        gl.addWidget(self.gain, 1, 1)
        gl.addWidget(QLabel("Profile mode (repro):"), 2, 0)
        self.mode = QComboBox()
        self.mode.addItems(["auto", "spatial", "temporal"])
        self.mode.setToolTip("build_profile_v5 mode: auto = single-frame curve + band shape from static "
                             "pixels (default), spatial = genuine-style single frame, temporal = frame differences")
        gl.addWidget(self.mode, 2, 1)
        v.addWidget(g)

        g = QGroupBox("Profile")
        gl = QVBoxLayout(g)
        self.b_auto = QPushButton("Auto Profile (Current Frame)")
        self.b_auto_noisy = QPushButton("Auto Profile (Most Noisy Frame)")
        self.b_build = QPushButton("Build Profile (Current Frame)")
        self.b_build_noisy = QPushButton("Build Profile (Most Noisy Frame)")
        self.b_auto.clicked.connect(lambda: self.autoProfile.emit(False))
        self.b_auto_noisy.clicked.connect(lambda: self.autoProfile.emit(True))
        self.b_build.clicked.connect(lambda: self.buildProfile.emit(False))
        self.b_build_noisy.clicked.connect(lambda: self.buildProfile.emit(True))
        self.b_auto.setToolTip("FindFlatArea (genuine port) picks the sample area on the current frame")
        self.b_auto_noisy.setToolTip("Same on the frame with the largest luma noise estimate "
                                     "(repro heuristic: median |HH1| of up to 32 sampled frames)")
        self.b_build.setToolTip("Profile from the area dragged in the preview")
        self.b_build_noisy.setToolTip("Profile from the dragged area on the most noisy frame")
        for b in (self.b_auto, self.b_auto_noisy, self.b_build, self.b_build_noisy):
            gl.addWidget(b)
        self.area = QLabel("Sample area: (none selected)")
        gl.addWidget(self.area)
        v.addWidget(g)

        g = QGroupBox("Fine-Tune")
        gl = QVBoxLayout(g)
        row = QHBoxLayout()
        self.b_aft = QPushButton("Auto Fine-Tune")
        self.b_aft.setToolTip("nv_aft_v5.apply_aft: static port of the genuine AutoFineTune (its factors were "
                              "not compared with the genuine); takes several seconds")
        self.b_complete = QPushButton("Auto Complete")
        self.b_complete.setToolTip("Interpolate the bins you did not edit between the edited ones (repro)")
        self.b_aft.clicked.connect(self.autoFineTune)
        self.b_complete.clicked.connect(self.autoComplete)
        row.addWidget(self.b_aft)
        row.addWidget(self.b_complete)
        gl.addLayout(row)
        gl.addWidget(QLabel("Manual Fine-Tune (noise factor per brightness bin):"))
        self.curve = CurveEditor()
        gl.addWidget(self.curve)
        self.graph = NoiseGraph()
        gl.addWidget(self.graph)
        v.addWidget(g)

        g = QGroupBox("Profile Quality")
        gl = QGridLayout(g)
        gl.addWidget(QLabel("Overall Profile Quality:"), 0, 0)
        self.quality = QProgressBar()
        self.quality.setRange(0, 100)
        self.quality.setValue(0)
        self.quality.setFormat("n/a (not computed)")
        self.quality.setEnabled(False)
        self.quality.setToolTip("The genuine quality measure is not decoded; not shown")
        gl.addWidget(self.quality, 0, 1)
        gl.addWidget(QLabel("Profile-image match:"), 1, 0)
        self.match = QLabel("—")
        self.match.setToolTip("FindFlatArea status of the sample area (genuine status codes)")
        gl.addWidget(self.match, 1, 1)
        row = QHBoxLayout()
        b = QPushButton("Profile Check")
        b.clicked.connect(self.profileCheck)
        row.addWidget(b)
        b = QPushButton("Tuning Guide")
        b.clicked.connect(self.tuningGuide)
        row.addWidget(b)
        gl.addLayout(row, 2, 0, 1, 2)
        v.addWidget(g)
        v.addStretch(1)

    def set_busy(self, busy):
        for b in (self.b_auto, self.b_auto_noisy, self.b_build, self.b_build_noisy, self.b_aft):
            b.setEnabled(not busy)

    def show_profile(self, ps, eff):
        self.device.setText(ps.device)
        self.curve.set_factors(ps.factors, ps.pinned)
        self.graph.set_profile(eff, ps.base)
        if ps.base is None:
            self.match.setText("—")
            self.area.setText("Sample area: (no profile)")
            return
        st = ps.base.get("flat_status")
        txt = "manual area" if st is None else nv.FLAT_STATUS.get(st, str(st))
        if ps.base.get("aft_applied"):
            txt += "  ·  AFT applied"
        self.match.setText(txt)
        y, x, s = ps.base["area"][:3]
        self.area.setText(f"Profile area (y, x, size): {y}, {x}, {s}  ·  frame {ps.base.get('ui_frame', '?')}")

    def set_selection(self, area):
        if area is not None:
            self.area.setText(f"Selected area (y, x, size): {area[0]}, {area[1]}, {area[2]}")
