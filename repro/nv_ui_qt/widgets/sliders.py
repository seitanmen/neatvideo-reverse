"""Parameter controls bound to AppState keys (slider + spin box, check box, combo box)."""
from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import QCheckBox, QComboBox, QDoubleSpinBox, QHBoxLayout, QLabel, QSlider, QWidget

from ..state import PARAM_SPECS


def _na(text, spec):
    return text if spec.implemented else f"{text} (n/a)"


class ParamSlider(QWidget):
    """QSlider + QDoubleSpinBox for a "pct" / "float" parameter."""
    edited = Signal(str, object)
    editStarted = Signal()

    def __init__(self, key, label=None, parent=None):
        super().__init__(parent)
        s = PARAM_SPECS[key]
        self.key, self.spec = key, s
        self.scale = 1.0 / s.step if s.kind == "float" else 1.0
        lay = QHBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 0)
        self.label = QLabel(_na(label if label is not None else s.label, s))
        self.label.setMinimumWidth(130)
        self.slider = QSlider(Qt.Horizontal)
        self.slider.setRange(int(round(s.lo * self.scale)), int(round(s.hi * self.scale)))
        self.spin = QDoubleSpinBox()
        self.spin.setRange(s.lo, s.hi)
        self.spin.setDecimals(0 if s.kind == "pct" else 2)
        self.spin.setSingleStep(s.step)
        self.spin.setSuffix(" %" if s.kind == "pct" else "")
        self.spin.setKeyboardTracking(False)
        self.spin.setFixedWidth(80)
        lay.addWidget(self.label)
        lay.addWidget(self.slider, 1)
        lay.addWidget(self.spin)
        for w in (self.label, self.slider, self.spin):
            w.setToolTip(s.tip)
        self.setEnabled(s.implemented)
        self.slider.sliderPressed.connect(self.editStarted)
        # wheel / keyboard steps are separate undo steps (a drag is merged into one)
        self.slider.actionTriggered.connect(
            lambda a: None if a == QSlider.SliderMove else self.editStarted.emit())
        self.slider.valueChanged.connect(self._from_slider)
        self.spin.valueChanged.connect(self._from_spin)
        self._block = False

    def set_value(self, v):
        self._block = True
        self.slider.setValue(int(round(float(v) * self.scale)))
        self.spin.setValue(float(v))
        self._block = False

    def value(self):
        return self.spin.value()

    def _from_slider(self, iv):
        if self._block:
            return
        v = iv / self.scale
        self._block = True
        self.spin.setValue(v)
        self._block = False
        self.edited.emit(self.key, v)

    def _from_spin(self, v):
        if self._block:
            return
        self.editStarted.emit()
        self._block = True
        self.slider.setValue(int(round(v * self.scale)))
        self._block = False
        self.edited.emit(self.key, v)


class ParamCheck(QCheckBox):
    edited = Signal(str, object)
    editStarted = Signal()

    def __init__(self, key, parent=None):
        s = PARAM_SPECS[key]
        super().__init__(_na(s.label, s), parent)
        self.key = key
        self.setToolTip(s.tip)
        self.setEnabled(s.implemented)
        self.toggled.connect(self._t)
        self._block = False

    def set_value(self, v):
        self._block = True
        self.setChecked(bool(v))
        self._block = False

    def _t(self, v):
        if not self._block:
            self.editStarted.emit()
            self.edited.emit(self.key, bool(v))


class ParamCombo(QWidget):
    """Combo box for "choice" and "int" parameters (Temporal Radius is a drop-down)."""
    edited = Signal(str, object)
    editStarted = Signal()

    def __init__(self, key, parent=None):
        super().__init__(parent)
        s = PARAM_SPECS[key]
        self.key, self.spec = key, s
        lay = QHBoxLayout(self)
        lay.setContentsMargins(0, 0, 0, 0)
        self.label = QLabel(_na(s.label, s))
        self.label.setMinimumWidth(130)
        self.box = QComboBox()
        self.box.addItems([str(i) for i in range(int(s.lo), int(s.hi) + 1)] if s.kind == "int" else s.choices)
        self.box.currentTextChanged.connect(self._c)
        lay.addWidget(self.label)
        lay.addWidget(self.box, 1)
        for w in (self.label, self.box):
            w.setToolTip(s.tip)
        self.setEnabled(s.implemented)
        self._block = False

    def set_value(self, v):
        self._block = True
        self.box.setCurrentText(str(int(v)) if self.spec.kind == "int" else str(v))
        self._block = False

    def _c(self, v):
        if not self._block:
            self.editStarted.emit()
            self.edited.emit(self.key, v)


def make_control(key):
    k = PARAM_SPECS[key].kind
    if k in ("pct", "float"):
        return ParamSlider(key)
    if k == "bool":
        return ParamCheck(key)
    return ParamCombo(key)
