"""Application state: filter parameters (UI units), noise profile, undo/redo history.

Parameters are kept in UI units (percent for amounts / levels) under the keys of PARAM_SPECS and
converted to the nvfilter v5 parameter dict by backend_params(). Defaults follow nv.V5_DEFAULTS
(the values calibrated against the genuine output, Agents.md §6.3), not the genuine factory
values, which the binary strings do not contain.
"""
import copy
import json
from dataclasses import dataclass, field

import numpy as np
from PySide6.QtCore import QObject, Signal
from PySide6.QtGui import QUndoCommand, QUndoStack

import nvfilter as nv

BANDS = [("ultra-low", "Ultra Low"), ("very-low", "Very Low"), ("low", "Low"), ("medium", "Mid"),
         ("high", "High")]
CHANNELS = [("Y", "Luminance"), ("Cb", "Cb"), ("Cr", "Cr")]
CURVE_X = [32.0 * k for k in range(9)]       # genuine profile curve bins (centres 0, 32, ..., 256)
FACTOR_MIN, FACTOR_MAX = 0.2, 3.0            # AutoFineTune clamps its factors to this range (§6.5)

NOT_IMPLEMENTED = "Not available in the reproduction: the v5 backend has no counterpart of this filter."
_BAND_TIP = {"ultra-low": "ultra low", "very-low": "very low", "low": "low", "medium": "medium",
             "high": "high"}


@dataclass
class Spec:
    kind: str                 # "pct" (percent float), "float", "int", "bool", "choice"
    default: object
    lo: float = 0.0
    hi: float = 100.0
    label: str = ""
    tip: str = ""
    implemented: bool = True
    choices: list = field(default_factory=list)
    step: float = 1.0


def _specs():
    D = nv.V5_DEFAULTS
    S = {}
    S["t_enable"] = Spec("bool", True, label="Enable Temporal Filter")
    S["t_radius"] = Spec("int", int(D["radius"]), 0, 5, "Temporal Radius",
                         "Radius determines the number of consecutive frames (before and after the current "
                         "frame) used by the Temporal filter.")
    S["t_amount"] = Spec("pct", 100.0 * D["temporal"], 0, 100, "Temporal Noise Reduction Amount",
                         "Amount of temporal noise reduction - high values -> more reduction of detected noise")
    S["t_quality"] = Spec("choice", "Normal", label="Temporal Quality Mode", implemented=False,
                          choices=["Normal", "High"], tip=NOT_IMPLEMENTED)
    for c, name in CHANNELS:
        S[f"noise_{c}"] = Spec("pct", 100.0 * D["channel_noise"][c], 0, 200, f"{name} Noise Level",
                               f"Estimation of noise level in the {name} component (relative to noise profile)"
                               " - higher values -> more image elements are treated as noise")
    S["s_enable"] = Spec("bool", True, label="Enable Spatial Filter")
    S["s_quality"] = Spec("choice", "Normal", label="Spatial Quality Mode", implemented=False,
                          choices=["Normal", "High"], tip=NOT_IMPLEMENTED)
    for c, name in CHANNELS:
        amt = D["luma"] if c == "Y" else D["chroma"]
        S[f"amt_{c}"] = Spec("pct", 100.0 * amt, 0, 100, name,
                             f"Noise Reduction Amount of the {name} component (multiplies the band amounts)")
        for b, bname in BANDS:
            S[f"bamt_{c}_{b}"] = Spec("pct", 100.0 * D["band_amounts"][b], 0, 100, bname,
                                      f"Amount of noise reduction in the {_BAND_TIP[b]} frequency component "
                                      f"of {name} - high values -> more reduction of detected noise")
            S[f"bnoise_{c}_{b}"] = Spec("pct", 100.0 * D["band_noise"][b], 0, 200, bname,
                                        f"Estimation of noise level in {_BAND_TIP[b]} frequency component "
                                        "(relative to noise profile) - higher values -> more image elements "
                                        "are treated as noise")
    S["sharp_enable"] = Spec("bool", False, label="Enable Sharpening", implemented=False, tip=NOT_IMPLEMENTED)
    for b, bname in BANDS:
        S[f"sharp_{b}"] = Spec("pct", 0.0, 0, 100, f"{bname} Freq Sharpening Amount", implemented=False,
                               tip="Amount of sharpening applied to " + _BAND_TIP[b] + " frequency component - "
                               "high values -> more sharpening\n" + NOT_IMPLEMENTED)
    S["sharp_prevent"] = Spec("bool", True, label="Prevent Over-Sharpening", implemented=False, tip=NOT_IMPLEMENTED)
    S["jitter_enable"] = Spec("bool", False, label="Enable Jitter Filter", implemented=False, tip=NOT_IMPLEMENTED)
    S["jitter_thr"] = Spec("pct", 0.0, 0, 100, "Jitter Threshold", implemented=False, tip=NOT_IMPLEMENTED)
    S["dust_enable"] = Spec("bool", False, label="Enable Dust and Scratches", implemented=False, tip=NOT_IMPLEMENTED)
    S["dust_mode"] = Spec("choice", "Normal", label="Mode", implemented=False, choices=["Normal", "Aggressive"],
                          tip=NOT_IMPLEMENTED)
    S["dust_thr"] = Spec("pct", 0.0, 0, 100, "Threshold", implemented=False, tip=NOT_IMPLEMENTED)
    S["flicker_enable"] = Spec("bool", False, label="Enable Flicker Filter", implemented=False, tip=NOT_IMPLEMENTED)
    S["flicker_local"] = Spec("pct", 0.0, 0, 100, "Local Flicker Threshold", implemented=False, tip=NOT_IMPLEMENTED)
    S["flicker_full"] = Spec("pct", 0.0, 0, 100, "Full Frame Flicker Threshold", implemented=False,
                             tip=NOT_IMPLEMENTED)
    for k, lab in (("low", "Low"), ("mid", "Mid"), ("high", "High")):
        S[f"edge_{k}"] = Spec("pct", 0.0, 0, 100, f"{lab} Freq Edge Smoothing", implemented=False,
                              tip=NOT_IMPLEMENTED)
    S["mix"] = Spec("pct", 100.0 * D["mix"], 0, 100, "Mix with original",
                    "Mixing with the original frame - 0% = filtered result only, 100% = original only "
                    "(v5 backend semantics)")
    # backend parameters without a genuine control (shown in the "Advanced (repro)" section)
    S["adv_threshold"] = Spec("float", float(D["threshold"]), 0.25, 8.0, "Temporal threshold",
                              "v5 temporal similarity tolerance (higher -> more averaging)", step=0.05)
    S["adv_spatial"] = Spec("float", float(D["spatial"]), 0.25, 4.0, "Spatial strength",
                            "v5 spatial Wiener strength (multiple of the noise variance)", step=0.05)
    S["adv_noise_level"] = Spec("pct", 100.0 * D["noise_level"], 25, 400, "Overall Noise Level",
                                "v5 overall noise level relative to the profile")
    S["adv_similarity"] = Spec("choice", D["similarity"], label="Temporal similarity",
                               choices=["v5", "pyramid"],
                               tip="v5: 5x5 energy weight; pyramid: FUN_0099a76c analogue (Agents.md §6.4)")
    return S


PARAM_SPECS = _specs()


def default_params():
    return {k: s.default for k, s in PARAM_SPECS.items()}


def coerce(key, value):
    s = PARAM_SPECS[key]
    if s.kind == "bool":
        return bool(value)
    if s.kind == "choice":
        return value if value in s.choices else s.default
    if s.kind == "int":
        return int(min(max(int(round(float(value))), s.lo), s.hi))
    return float(min(max(float(value), s.lo), s.hi))


def backend_params(ui):
    """UI values -> nvfilter v5 parameter dict (plus the per-channel spatial settings that
    backend.spatial_ui applies; with all channels on the same band settings and Cb = Cr this is
    exactly nv.process_frame_v5)."""
    chans = {c: {"amount": ui[f"amt_{c}"] / 100.0,
                 "bands": {b: ui[f"bamt_{c}_{b}"] / 100.0 for b, _ in BANDS},
                 "noise": {b: ui[f"bnoise_{c}_{b}"] / 100.0 for b, _ in BANDS}} for c, _ in CHANNELS}
    P = nv.v5_params(
        radius=int(ui["t_radius"]),
        temporal=ui["t_amount"] / 100.0 if ui["t_enable"] else 0.0,
        threshold=float(ui["adv_threshold"]), spatial=float(ui["adv_spatial"]),
        luma=chans["Y"]["amount"], chroma=chans["Cb"]["amount"],
        band_amounts=dict(chans["Y"]["bands"]), band_noise=dict(chans["Y"]["noise"]),
        noise_level=ui["adv_noise_level"] / 100.0,
        channel_noise={c: ui[f"noise_{c}"] / 100.0 for c, _ in CHANNELS},
        similarity=ui["adv_similarity"], mix=ui["mix"] / 100.0)
    P["ui_channels"] = chans
    P["ui_spatial"] = bool(ui["s_enable"])
    return P


# ---------------- profile ----------------
def unit_factors():
    return [[1.0] * 9 for _ in CHANNELS]


def effective_profile(base, factors):
    """Manual Fine-Tune: multiply the v5 curve (absolute sigma at prof['curve_centers']) by the
    per-bin factors interpolated from the 9 genuine bins, as nv_aft_v5.apply_aft does with the
    AFT factors."""
    if base is None:
        return None
    if all(abs(f - 1.0) < 1e-12 for fs in factors for f in fs):
        return base
    prof = copy.deepcopy(base)
    xs = np.asarray(prof["curve_centers"], np.float64)
    for (c, _), fs in zip(CHANNELS, factors):
        k = np.interp(xs, CURVE_X, fs)
        prof["curve"][c] = (np.asarray(prof["curve"][c], np.float32) * k.astype(np.float32)).tolist()
    lvl = prof.get("area_level", 128.0)
    prof["sigma"] = {c: float(np.interp(lvl, prof["curve_centers"], prof["curve"][c]) * prof["tot_ratio"][c])
                     for c, _ in CHANNELS}
    prof["ui_factors"] = [list(map(float, fs)) for fs in factors]
    return prof


def auto_complete(factors, pinned):
    """Auto Complete (repro): bins that were not edited by hand are linearly interpolated
    between the edited ones (constant beyond the outermost edited bin)."""
    out = []
    for c, fs in enumerate(factors):
        pins = sorted(pinned[c])
        if not pins:
            out.append(list(fs))
            continue
        xs = [CURVE_X[i] for i in pins]
        ys = [fs[i] for i in pins]
        out.append([fs[i] if i in pinned[c] else float(np.interp(CURVE_X[i], xs, ys)) for i in range(9)])
    return out


PROFILE_KEYS = ("mode", "area", "levels", "band_ratio", "tot_ratio", "curve", "curve_centers", "sigma")


@dataclass
class ProfileState:
    base: dict = None                     # build_profile_v5 / apply_aft result
    factors: list = field(default_factory=unit_factors)
    pinned: list = field(default_factory=lambda: [set() for _ in CHANNELS])
    device: str = "Unnamed device"

    def clone(self):
        return ProfileState(copy.deepcopy(self.base), [list(f) for f in self.factors],
                            [set(p) for p in self.pinned], self.device)

    def to_json(self):
        return {"format": "nv_repro_profile", "version": 1, "device": self.device, "base": self.base,
                "factors": self.factors, "pinned": [sorted(p) for p in self.pinned]}

    @staticmethod
    def from_json(d):
        if not isinstance(d, dict) or d.get("format") != "nv_repro_profile":
            raise ValueError("not a repro profile file (format nv_repro_profile)")
        b = d.get("base")
        missing = [k for k in PROFILE_KEYS if not isinstance(b, dict) or k not in b]
        if missing:
            raise ValueError(f"profile lacks {', '.join(missing)}")
        for c, _ in CHANNELS:
            if len(b["curve"][c]) != len(b["curve_centers"]) or c not in b["sigma"] or c not in b["levels"]:
                raise ValueError(f"profile curve / sigma / levels of {c} are inconsistent")
        f = d.get("factors", unit_factors())
        if len(f) != 3 or any(len(x) != 9 for x in f):
            raise ValueError("profile factors must be 3 x 9")
        return ProfileState(d["base"], [list(map(float, f)) for f in d.get("factors", unit_factors())],
                            [set(p) for p in d.get("pinned", [[], [], []])], d.get("device", "Unnamed device"))


def preset_json(params):
    return {"format": "nv_repro_filter_preset", "version": 1, "params": params}


def params_from_json(d):
    if not isinstance(d, dict) or d.get("format") != "nv_repro_filter_preset" or not isinstance(d.get("params", {}), dict):
        raise ValueError("not a repro filter preset (format nv_repro_filter_preset)")
    P = default_params()
    for k, v in d.get("params", {}).items():
        if k in PARAM_SPECS:
            P[k] = coerce(k, v)
    return P


# ---------------- undo commands ----------------
class ParamsCommand(QUndoCommand):
    def __init__(self, state, old, new, text, merge_key=None):
        super().__init__(text)
        self.state, self.old, self.new, self.merge_key = state, old, new, merge_key

    def id(self):
        return 1 if self.merge_key is not None else -1

    def mergeWith(self, other):
        if other.merge_key is None or other.merge_key != self.merge_key:
            return False
        self.new = other.new
        return True

    def redo(self):
        self.state._apply_params(self.new)

    def undo(self):
        self.state._apply_params(self.old)


class ProfileCommand(QUndoCommand):
    def __init__(self, state, old, new, text, merge_key=None):
        super().__init__(text)
        self.state, self.old, self.new, self.merge_key = state, old, new, merge_key

    def id(self):
        return 2 if self.merge_key is not None else -1

    def mergeWith(self, other):
        if other.merge_key is None or other.merge_key != self.merge_key:
            return False
        self.new = other.new
        return True

    def redo(self):
        self.state._apply_profile(self.new)

    def undo(self):
        self.state._apply_profile(self.old)


class AppState(QObject):
    paramsChanged = Signal(set)          # changed keys
    profileChanged = Signal()

    def __init__(self):
        super().__init__()
        self.params = default_params()
        self.profile = ProfileState()
        self.undo = QUndoStack(self)
        self._seq = 0
        self._eff = None

    # --- parameters ---
    def new_edit(self):
        """Start a new undo step (slider press); later moves of the same drag merge into it."""
        self._seq += 1

    def set_param(self, key, value):
        value = coerce(key, value)
        if self.params[key] == value:
            return
        new = dict(self.params)
        new[key] = value
        self.undo.push(ParamsCommand(self, dict(self.params), new, f"Change {PARAM_SPECS[key].label}",
                                     (key, self._seq)))

    def set_all_params(self, params, text):
        new = default_params()
        new.update({k: coerce(k, v) for k, v in params.items() if k in PARAM_SPECS})
        if new != self.params:
            self.undo.push(ParamsCommand(self, dict(self.params), new, text))

    def _apply_params(self, p):
        changed = {k for k in p if p[k] != self.params.get(k)}
        self.params = dict(p)
        if changed:
            self.paramsChanged.emit(changed)

    def backend(self):
        return backend_params(self.params)

    # --- profile ---
    def set_profile(self, new, text, merge_key=None):
        self.undo.push(ProfileCommand(self, self.profile.clone(), new.clone(), text, merge_key))

    def set_factors(self, c, factors9, pin=None):
        new = self.profile.clone()
        new.factors[c] = [float(min(max(f, FACTOR_MIN), FACTOR_MAX)) for f in factors9]
        if pin is not None:
            new.pinned[c].add(pin)
        self.set_profile(new, "Manual Fine-Tune", ("curve", c, self._seq))

    def _apply_profile(self, ps):
        self.profile = ps.clone()
        self._eff = None
        self.profileChanged.emit()

    def effective_profile(self):
        if self._eff is None and self.profile.base is not None:
            self._eff = effective_profile(self.profile.base, self.profile.factors)
        return self._eff


def dump_json(obj, path):
    with open(path, "w") as fh:
        json.dump(obj, fh, indent=1)


def load_json(path):
    with open(path) as fh:
        return json.load(fh)
