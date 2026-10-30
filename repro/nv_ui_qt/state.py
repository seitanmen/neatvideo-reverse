"""Application state: filter parameters (UI units), noise profile, undo/redo history.

Parameters are kept in UI units (percent for amounts / levels) under the keys of PARAM_SPECS and
converted to the nvfilter v5 parameter dict by backend_params(). Defaults follow nv.V5_DEFAULTS
(the values calibrated against the genuine output, Agents.md §6.3), not the genuine factory
values, which the binary strings do not contain.
"""
import copy
import json
import math
from dataclasses import dataclass, field

import numpy as np
from PySide6.QtCore import QObject, Signal
from PySide6.QtGui import QUndoCommand, QUndoStack

import nvfilter as nv

BANDS = [("ultra-low", "Ultra Low"), ("very-low", "Very Low"), ("low", "Low"), ("medium", "Mid"),
         ("high", "High")]
CHANNELS = [("Y", "Luminance"), ("Cb", "Cb"), ("Cr", "Cr")]
UI_CHANNELS = [("Y", "Luminance"), ("Cr", "Cr"), ("Cb", "Cb")]   # genuine panel order (Cr before Cb)
UI_BANDS = BANDS[::-1]                                          # genuine panel order: High ... Ultra Low
CH_INDEX = {c: i for i, (c, _) in enumerate(CHANNELS)}
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
                         "frame) used by the Temporal filter (1 frame ... 5 frames).")
    S["t_amount"] = Spec("pct", 100.0 * D["temporal"], 0, 100, "Temporal Noise Reduction Amount",
                         "Amount of temporal noise reduction - high values -> more reduction of detected noise")
    S["t_quality"] = Spec("choice", "Normal", label="Temporal Quality Mode", implemented=False,
                          choices=["Normal", "High"], tip=NOT_IMPLEMENTED)
    for c, name in CHANNELS:
        S[f"noise_{c}"] = Spec("pct", 100.0 * D["channel_noise"][c], 0, 250, f"{name} Noise Level",
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
            S[f"bnoise_{c}_{b}"] = Spec("pct", 100.0 * D["band_noise"][b], 0, 250, bname,
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
                    "(v5 backend semantics; the panel shows 100 - mix like the genuine Mix slider)")
    S["mix_enable"] = Spec("bool", False, label="Enable Mix with Original",
                           tip="Off: the filtered result only (backend mix 0)")
    # genuine controls without a v5 counterpart (shown disabled, kept in presets)
    na = dict(implemented=False, tip=NOT_IMPLEMENTED)
    S["t_noise"] = Spec("pct", 100.0, 0, 250, "Temporal Noise Level",
                        tip="Estimation of noise level for the Temporal filter (relative to noise profile) - "
                        "higher values -> more image elements are treated as noise.\n(repro: multiplies the "
                        "noise level of the temporal similarity only, on top of the Overall / Luminance Noise "
                        "Level; the Spatial filter is not affected)")
    S["rep_rate"] = Spec("pct", 0.0, 0, 100, "Repeat Rate", **na)
    S["dust_amount"] = Spec("pct", 100.0, 0, 100, "Amount", **na)
    S["adapt_enable"] = Spec("bool", False, label="Adapt to Changing Noise", **na)
    S["art_enable"] = Spec("bool", True, label="Enable Artifact Removal", **na)
    for k, lab in (("art_dots", "Dots"), ("art_lines", "Lines"), ("art_high", "High"), ("art_mid", "Mid"),
                   ("art_low", "Low")):
        S[k] = Spec("pct", 100.0, 0, 250, lab, **na)
    S["edge_enable"] = Spec("bool", False, label="Enable Edge Smoothing", **na)
    for b in ("high", "medium", "low"):
        S[f"sharp_range_{b}"] = Spec("float", 0.0, -1.0, 1.0, "Range", step=0.05, **na)
    for c, name in UI_CHANNELS:
        S[f"sharp_ch_{c}"] = Spec("pct", 100.0 if c == "Y" else 0.0, 0, 250, name, **na)
        S[f"mix_{c}"] = Spec("pct", 100.0, 0, 100, name, **na)
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
        similarity=ui["adv_similarity"], mix=ui["mix"] / 100.0 if ui.get("mix_enable") else 0.0)
    P["ui_channels"] = chans
    P["ui_spatial"] = bool(ui["s_enable"])
    P["t_noise"] = ui["t_noise"] / 100.0        # Temporal Noise Level (backend.temporal)
    return P


# ---------------- profile ----------------
def unit_factors():
    return [[1.0] * 9 for _ in CHANNELS]


TUNE_MIN, TUNE_MAX = 0.2, 3.0                # profile Noise Level multipliers (Profile Check and Tuning)


def effective_profile(base, factors, tune=None):
    """Manual Fine-Tune: multiply the v5 curve (absolute sigma at prof['curve_centers']) by the
    per-bin factors interpolated from the 9 genuine bins, as nv_aft_v5.apply_aft does with the
    AFT factors. tune = {"all": t, "Y": ty, "Cb": .., "Cr": ..} (profile Noise Level, repro):
    each channel's curve is further multiplied by t * t_c."""
    if base is None:
        return None
    tune = tune or {}
    tk = {c: float(tune.get("all", 1.0)) * float(tune.get(c, 1.0)) for c, _ in CHANNELS}
    if all(abs(f - 1.0) < 1e-12 for fs in factors for f in fs) and all(v == 1.0 for v in tk.values()):
        return base
    prof = copy.deepcopy(base)
    xs = np.asarray(prof["curve_centers"], np.float64)
    for (c, _), fs in zip(CHANNELS, factors):
        k = np.interp(xs, CURVE_X, fs) * tk[c]
        prof["curve"][c] = (np.asarray(prof["curve"][c], np.float32) * k.astype(np.float32)).tolist()
    lvl = prof.get("area_level", 128.0)
    prof["sigma"] = {c: float(np.interp(lvl, prof["curve_centers"], prof["curve"][c]) * prof["tot_ratio"][c])
                     for c, _ in CHANNELS}
    prof["ui_factors"] = [list(map(float, fs)) for fs in factors]
    prof["ui_tune"] = dict(tune)
    return prof


def rgb_ready(base):
    """An R/G/B equalizer can drive this profile: after AFT the curve before AFT and the AFT's colour weights,
    before AFT the build's colour weights (profiles made before G1 lack them and stay Y / Cb / Cr)."""
    if base is None:
        return False
    if base.get("aft_applied"):
        return "ui_pre_aft" in base and "aft_color_weights" in base
    return "ui_rgb_weights" in base


def rgb_reference(base):
    """The R/G/B levels the profile's own curve stands for: the AFT levels after AFT, else 1.0 (G1 §2.2); None
    for an AFT result made before G1 (no aft_levels: its curve stands for no R/G/B levels)."""
    if base is not None and base.get("aft_applied"):
        return [list(map(float, l)) for l in base["aft_levels"]] if "aft_levels" in base else None
    return unit_factors()


def rgb_effective_profile(base, levels, tune=None):
    """Manual Fine-Tune of an R/G/B profile (G1): the 9 R / G / B bin levels are combined into the Y / Cb / Cr
    curve factors as nv_aft_v5.apply_aft combines the AFT levels, f_c = sqrt(max(sum_j c_cj f_j, 0)) on the
    curve before AFT, then the profile Noise Level tune t * t_c multiplies each curve (float32, as the Y / Cb / Cr
    path). Levels equal to rgb_reference and tune 1 return base unchanged; levels 1.0 and tune 1 after AFT give
    the curve and sigma before AFT unchanged (Reset Fine-Tuning, G1 U5)."""
    if base is None:
        return None
    tune = tune or {}
    tk = {c: float(tune.get("all", 1.0)) * float(tune.get(c, 1.0)) for c, _ in CHANNELS}
    lv = [[float(v) for v in l] for l in levels]
    if lv == rgb_reference(base) and all(v == 1.0 for v in tk.values()):
        return base
    if not rgb_ready(base):
        raise ValueError("the profile has no colour weights for the R/G/B equalizer")
    aft = bool(base.get("aft_applied"))
    src = base["ui_pre_aft"]["curve"] if aft else base["curve"]
    cw = base["aft_color_weights"] if aft else base["ui_rgb_weights"]
    prof = copy.deepcopy(base)
    if aft and lv == unit_factors() and all(v == 1.0 for v in tk.values()):
        # levels 1.0 after AFT (Reset Fine-Tuning): the curve and sigma before AFT as they were
        prof["curve"] = copy.deepcopy(base["ui_pre_aft"]["curve"])
        prof["sigma"] = dict(base["ui_pre_aft"]["sigma"])
    else:
        from .backend import rgb_combine
        _, curves = rgb_combine(src, prof["curve_centers"], cw, lv)      # nv_aft_v5.apply_aft steps 4-6
        for c, _ in CHANNELS:
            prof["curve"][c] = (curves[c] * np.float32(tk[c])).tolist()
        lvl = prof.get("area_level", 128.0)
        prof["sigma"] = {c: float(np.interp(lvl, prof["curve_centers"], prof["curve"][c]) * prof["tot_ratio"][c])
                         for c, _ in CHANNELS}
    prof["ui_channels"] = "RGB"
    prof["ui_factors"] = lv
    prof["ui_tune"] = dict(tune)
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


def auto_complete_ps(ps):
    """Auto Complete of a profile state: an R/G/B profile after AFT also keeps the bins the AFT measured
    (aft_bin_flags marker false; G1 U6), a Y / Cb / Cr profile only the edited ones."""
    fixed = [set(p) for p in ps.pinned]
    b = ps.base
    if ps.channels == "RGB" and b is not None and b.get("aft_applied") and not b.get("aft_reset") \
            and b.get("aft_bin_flags"):
        for c in range(3):
            fixed[c] |= {k for k in range(9) if not b["aft_bin_flags"]["marker"][c][k]}
    return auto_complete(ps.factors, fixed)


def after_aft(ps, prof):
    """The profile state after Auto Fine-Tune: the new profile, the equalizer at the AFT's R/G/B levels (G1;
    a result without them keeps the channels and resets the factors), no edited bins."""
    n = ps.clone()
    n.base = prof
    if rgb_ready(prof) and "aft_levels" in prof:
        n.channels = "RGB"
        n.factors = [list(map(float, l)) for l in prof["aft_levels"]]
    else:
        n.factors = unit_factors()
    n.pinned = [set() for _ in CHANNELS]
    return n


def reset_fine_tune(ps):
    """Reset Fine-Tuning Results: levels / factors 1.0, no edited bins, base["aft_reset"] (quality._curve_blob).
    An R/G/B profile then runs on the curve before AFT (levels 1.0 = no AFT correction, G1 U5); a Y / Cb / Cr
    profile that can drive the R/G/B equalizer moves to it (U9)."""
    n = ps.clone()
    n.factors = unit_factors()
    n.pinned = [set() for _ in CHANNELS]
    if n.base is not None:
        n.base = copy.deepcopy(n.base)
        n.base["aft_reset"] = True
        if rgb_ready(n.base):
            n.channels = "RGB"
    return n


def check_rgb_keys(b):
    """The keys the R/G/B equalizer reads (G1), where present, in their shape with finite numbers (ValueError
    otherwise; qtui's ProfileState::from_json raises the same texts): ui_rgb_weights / aft_color_weights 3 x 3,
    aft_levels 3 x 9, ui_pre_aft {curve: Y / Cb / Cr as long as curve_centers, sigma: Y / Cb / Cr}, aft_bin_flags
    {marker, and clip / gt3 when given: 3 x 9}."""
    num = lambda v: isinstance(v, (int, float)) and math.isfinite(v)                    # noqa: E731
    grid = lambda v, r, c, ok: isinstance(v, list) and len(v) == r and all(              # noqa: E731
        isinstance(row, list) and len(row) == c and all(ok(x) for x in row) for row in v)
    for k, c in (("ui_rgb_weights", 3), ("aft_color_weights", 3), ("aft_levels", 9)):
        if k in b and not grid(b[k], 3, c, num):
            raise ValueError(f"profile {k} must be 3 x {c} finite numbers")
    if "ui_pre_aft" in b:
        p, n = b["ui_pre_aft"], len(b["curve_centers"])
        if not (isinstance(p, dict) and isinstance(p.get("curve"), dict) and isinstance(p.get("sigma"), dict) and
                all(isinstance(p["curve"].get(c), list) and len(p["curve"][c]) == n and all(num(x) for x in p["curve"][c])
                    and num(p["sigma"].get(c)) for c, _ in CHANNELS)):
            raise ValueError("profile ui_pre_aft must hold the curve and sigma of Y / Cb / Cr (finite numbers)")
    if "aft_bin_flags" in b:
        f = b["aft_bin_flags"]
        if not (isinstance(f, dict) and "marker" in f and
                all(grid(f[k], 3, 9, lambda x: isinstance(x, bool)) for k in ("marker", "clip", "gt3") if k in f)):
            raise ValueError("profile aft_bin_flags must be 3 x 9 booleans (marker, clip, gt3)")


PROFILE_KEYS = ("mode", "area", "levels", "band_ratio", "tot_ratio", "curve", "curve_centers", "sigma")
PROFILE_FORMAT, PROFILE_FORMAT_RGB = "nv_repro_profile", "nv_repro_profile_rgb"


@dataclass
class ProfileState:
    base: dict = None                     # build_profile_v5 / apply_aft result
    factors: list = field(default_factory=unit_factors)
    pinned: list = field(default_factory=lambda: [set() for _ in CHANNELS])
    device: str = "Unspecified device"
    tune: dict = field(default_factory=dict)     # profile Noise Level: "all", "Y", "Cb", "Cr" (missing = 1.0)
    channels: str = "RGB"                        # "RGB": factors = R/G/B levels (G1); "YCbCr": Y/Cb/Cr factors (older files)

    def clone(self):
        return ProfileState(copy.deepcopy(self.base), [list(f) for f in self.factors],
                            [set(p) for p in self.pinned], self.device, dict(self.tune), self.channels)

    def tune_value(self, k):
        return float(self.tune.get(k, 1.0))

    def effective(self):
        """The effective profile (state.effective_profile / rgb_effective_profile by the channels)."""
        if self.channels == "RGB":
            return rgb_effective_profile(self.base, self.factors, self.tune)
        return effective_profile(self.base, self.factors, self.tune)

    def to_json(self):
        if self.channels == "RGB":                # G1: older UIs refuse the format instead of misreading R/G/B
            return {"format": PROFILE_FORMAT_RGB, "version": 2, "channels": "RGB", "device": self.device,
                    "base": self.base, "factors": self.factors, "pinned": [sorted(p) for p in self.pinned],
                    "tune": self.tune}
        return {"format": PROFILE_FORMAT, "version": 1, "device": self.device, "base": self.base,
                "factors": self.factors, "pinned": [sorted(p) for p in self.pinned], "tune": self.tune}

    @staticmethod
    def from_json(d):
        if not isinstance(d, dict) or d.get("format") not in (PROFILE_FORMAT, PROFILE_FORMAT_RGB):
            raise ValueError("not a repro profile file (format nv_repro_profile)")
        rgb = d["format"] == PROFILE_FORMAT_RGB
        if rgb and d.get("channels") != "RGB":
            raise ValueError("profile channels must be RGB (format nv_repro_profile_rgb)")
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
        finite = lambda vs: all(isinstance(v, (int, float)) and math.isfinite(v) for v in vs)
        if not finite([v for x in f for v in x]) or not finite(b["curve_centers"]) or \
                not all(finite(b["curve"][c]) and finite([b["sigma"][c]]) for c, _ in CHANNELS):
            raise ValueError("profile factors / curve / sigma must be finite numbers")
        tune = d.get("tune", {})                 # older files: no tune = 1.0
        if not isinstance(tune, dict) or any(k not in ("all", "Y", "Cb", "Cr") for k in tune) or \
                not all(isinstance(v, (int, float)) and math.isfinite(v) for v in tune.values()):
            raise ValueError("profile tune must be a dict of all / Y / Cb / Cr with finite values")
        tune = {k: float(min(max(float(v), TUNE_MIN), TUNE_MAX)) for k, v in tune.items()}
        check_rgb_keys(b)
        if rgb and not rgb_ready(b):
            raise ValueError("R/G/B profile lacks its colour weights (ui_rgb_weights / aft_color_weights, ui_pre_aft)")
        return ProfileState(d["base"], [list(map(float, f)) for f in d.get("factors", unit_factors())],
                            [set(p) for p in d.get("pinned", [[], [], []])], d.get("device", "Unspecified device"),
                            tune, "RGB" if rgb else "YCbCr")


def preset_json(params):
    return {"format": "nv_repro_filter_preset", "version": 1, "params": params}


def params_from_json(d):
    if not isinstance(d, dict) or d.get("format") != "nv_repro_filter_preset" or not isinstance(d.get("params", {}), dict):
        raise ValueError("not a repro filter preset (format nv_repro_filter_preset)")
    P = default_params()
    raw = d.get("params", {})
    for k, v in raw.items():
        if k in PARAM_SPECS:
            P[k] = coerce(k, v)
    if "mix_enable" not in raw:                 # presets written before mix_enable: mix > 0 was active
        P["mix_enable"] = P["mix"] > 0
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

    def set_params(self, updates, text, merge=True):
        """Several keys in one undo step (Frequencies rows write Y, Cb and Cr together); drags merge."""
        new = dict(self.params)
        new.update({k: coerce(k, v) for k, v in updates.items()})
        if new != self.params:
            self.undo.push(ParamsCommand(self, dict(self.params), new, text,
                                         (tuple(sorted(updates)), self._seq) if merge else None))

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

    def set_factors_all(self, factors9, pin=None):
        """Equalizer 'All': the same bin edit on the three channels, one undo step."""
        new = self.profile.clone()
        for c in range(len(CHANNELS)):
            new.factors[c][pin] = float(min(max(factors9[pin], FACTOR_MIN), FACTOR_MAX))
            new.pinned[c].add(pin)
        self.set_profile(new, "Manual Fine-Tune", ("curve", "all", self._seq))

    def set_tune(self, k, value):
        """Profile Noise Level (k = "all" or a channel), multiplier clamped to [TUNE_MIN, TUNE_MAX]."""
        v = float(min(max(float(value), TUNE_MIN), TUNE_MAX))
        if self.profile.tune_value(k) == v:
            return
        new = self.profile.clone()
        new.tune[k] = v
        self.set_profile(new, "Profile Noise Level", ("tune", k, self._seq))

    def _apply_profile(self, ps):
        self.profile = ps.clone()
        self._eff = None
        self.profileChanged.emit()

    def effective_profile(self):
        if self._eff is None and self.profile.base is not None:
            self._eff = self.profile.effective()
        return self._eff


def dump_json(obj, path):
    with open(path, "w") as fh:
        json.dump(obj, fh, indent=1)


def load_json(path):
    with open(path) as fh:
        return json.load(fh)
