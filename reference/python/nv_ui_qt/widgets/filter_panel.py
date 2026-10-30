"""Filter Settings side panel (Adjust and Preview page) in the genuine order: preset buttons and
name, tabs Temporal | Spatial | General with grey section headers; Luminance -> Cr -> Cb, High ->
Ultra Low. Advanced-only rows are hidden in Beginner Mode (values are kept).

Frequencies rows are common to the three components as in the genuine panel: one row writes
bamt_{Y,Cr,Cb}_<band> (or bnoise_...) in one undo step; the per-component rows of the v5 backend
stay available in "Per-component bands (repro)".

Noise Level Check (set_nlc_active, genuine g_nlc1.png): while it runs the button reads Finish, the rows
it concerns (Temporal Filter, Radius, Noise Level Check, Noise Level, Adapt to Changing Noise) are
yellow, the other sections of the Temporal tab and the Spatial / General tabs are disabled."""
from PySide6.QtCore import Qt, Signal
from PySide6.QtWidgets import (QComboBox, QFrame, QHBoxLayout, QLabel, QPushButton, QScrollArea, QTabWidget,
                               QVBoxLayout, QWidget)

from nvfilter import V5_BANDS

from ..state import PARAM_SPECS, UI_BANDS, UI_CHANNELS
from .rows import (ACTIVE_TITLE, BeginRow, ChoiceRow, EnableCheck, Header, IconBar, Row, guide_labels,
                   mode_text, panel_title)

ADV, BEG, BOTH = {"adv"}, {"beg"}, {"adv", "beg"}
QUALITY_NAMES = ["Normal (Faster)", "High (Slower)"]
NOISE_TRACK = (-100, 150)          # genuine Noise Level sliders: -100 ... +150 %


class FilterPanel(QWidget):
    edited = Signal(str, object)
    editedMany = Signal(dict)
    editStarted = Signal()
    assistToggled = Signal(bool)
    noiseLevelCheck = Signal(bool)
    resetSpatial = Signal()
    checkSpeed = Signal()
    preferences = Signal()
    linkActivated = Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.controls = {}          # key -> primary widget
        self.extra = {}             # key -> further widgets bound to the same key
        self.common = []            # Frequencies rows (several keys)
        self.headers = {}
        self._modes = []            # (widget, modes)
        self.params = {k: s.default for k, s in PARAM_SPECS.items()}
        v = QVBoxLayout(self)
        v.setContentsMargins(4, 4, 4, 4)
        v.addWidget(panel_title("Filter Settings"))
        self.icons = IconBar("filter settings")
        v.addWidget(self.icons)
        self.name = QComboBox()
        self.name.addItem("default")
        self.name.setToolTip("Name of the current filter preset (* = changed since it was loaded)")
        v.addWidget(self.name)
        self.tabs = QTabWidget()
        v.addWidget(self.tabs, 1)
        self._temporal()
        self._spatial()
        self._general()
        self.qsg, self.mode_lbl = guide_labels()
        for l in (self.qsg, self.mode_lbl):
            l.linkActivated.connect(self.linkActivated)
            v.addWidget(l)
        self.set_mode(True)

    # ---------------- construction helpers ----------------
    def _page(self, title):
        sc = QScrollArea()
        sc.setWidgetResizable(True)
        sc.setFrameShape(QFrame.NoFrame)
        sc.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        w = QWidget()
        lay = QVBoxLayout(w)
        lay.setContentsMargins(0, 0, 0, 0)
        lay.setSpacing(1)
        sc.setWidget(w)
        self.tabs.addTab(sc, title)
        return lay

    def _reg(self, w, modes=BOTH):
        self._modes.append((w, modes))
        return w

    def _bind(self, w):
        if hasattr(w, "editedMany"):
            w.editedMany.connect(self.editedMany)
        w.edited.connect(self._edited)
        w.editStarted.connect(self.editStarted)
        k = getattr(w, "key", None)
        if isinstance(w, Row) and len(w.keys) > 1:
            self.common.append(w)
        elif k in self.controls:
            self.extra.setdefault(k, []).append(w)
        else:
            self.controls[k] = w
        return w

    def _row(self, parent, key, label=None, fmt="amount", deco=None, steps=(5, 20), indent=0, modes=BOTH):
        w = self._bind(Row(key, label, fmt, deco, steps, indent, srange=NOISE_TRACK if fmt == "rel" else None))
        parent.add(w) if isinstance(parent, Header) else parent.addWidget(w)
        return self._reg(w, modes)

    def _choice(self, parent, key, label, names=None, indent=0, modes=BOTH):
        w = self._bind(ChoiceRow(key, label, names, indent))
        parent.add(w) if isinstance(parent, Header) else parent.addWidget(w)
        return self._reg(w, modes)

    def _header(self, parent, title, enable=None, modes=BOTH, inline=None, **kw):
        en = self._bind(EnableCheck(enable)) if enable else None
        if inline is not None:
            self._bind(inline)
        h = Header(title, enable=en, inline=inline, **kw)
        parent.add(h) if isinstance(parent, Header) else parent.addWidget(h)
        self.headers[title if title not in self.headers else f"{title}#{len(self.headers)}"] = h
        return self._reg(h, modes)

    def _check_row(self, parent, key, label, indent=0, modes=BOTH):
        w = QWidget()
        lay = QHBoxLayout(w)
        lay.setContentsMargins(6 + 14 * indent, 1, 2, 1)
        na = not PARAM_SPECS[key].implemented
        lab = QLabel(label + (" (n/a)" if na else ""))
        lab.setToolTip(PARAM_SPECS[key].tip)
        lab.setWordWrap(True)
        lay.addWidget(lab, 1)
        cb = self._bind(EnableCheck(key))
        if na:
            cb.na_label = lab
        lay.addWidget(cb)
        parent.add(w)
        return self._reg(w, modes)

    def _na_begin(self, parent, title, modes=ADV):
        w = BeginRow(title, "Not available in the reproduction", enabled=False)
        parent.add(w)
        return self._reg(w, modes)

    # ---------------- tabs ----------------
    def _temporal(self):
        lay = self._page("Temporal")
        h = self._header(lay, "Temporal Filter", "t_enable", collapsible=False)
        nlc_hl = [h]                            # yellow while Noise Level Check runs
        nlc_off = [self._choice(h, "t_quality", "Quality Mode", QUALITY_NAMES, modes=ADV)]   # disabled then
        nlc_hl.append(self._row(h, "t_radius", "Radius", fmt="frames", steps=(1, 1)))
        h = self._header(lay, "Temporal Noise", modes=ADV, expanded=False)
        self.t_noise_header = h
        nl = BeginRow("Noise Level Check", "Click Begin to show the Temporal Filter mask. The mask shows the "
                      "areas where temporal noise reduction is applied. Adjust Noise Level while watching the "
                      "mask to make it sufficiently cover the noisy areas of the frame.\n(repro: the mask is "
                      "1 - the residual noise of the temporal stage, relative to full averaging)", enabled=True)
        nl.toggled.connect(lambda on: self.noiseLevelCheck.emit(on))
        h.add(nl)
        self._reg(nl, ADV)
        self.nl_check = nl
        nlc_hl.append(self._row(h, "t_noise", "Noise Level", fmt="rel", steps=(5, 20)))
        nlc_off.append(self._na_begin(h, "Local Flicker Check"))
        nlc_off.append(self._row(h, "flicker_local", "Local Flicker", steps=(1, 5)))
        nlc_off.append(self._row(h, "flicker_full", "Frame Flicker", steps=(1, 5)))
        hn = self._header(h, "Noise Reduction Amount", level=1)
        self._row(hn, "t_amount", "Amount", steps=(5, 20), indent=1)
        nlc_off.append(hn)
        h = self._header(lay, "Repeated Frames", modes=ADV, expanded=False)
        self._na_begin(h, "Repeated Frames Check")
        self._row(h, "rep_rate", "Repeat Rate", steps=(1, 20))
        nlc_off.append(h)
        h = self._header(lay, "Jitter of Details", modes=ADV, expanded=False)
        self._row(h, "jitter_thr", "Threshold")
        self._check_row(h, "jitter_enable", "Jitter Extended Filtration")
        nlc_off.append(h)
        h = self._header(lay, "Flicker", "flicker_enable", modes=BEG, collapsible=False, level=0)
        nlc_off.append(h)
        h = self._header(lay, "Dust and Scratches", "dust_enable", expanded=False)
        self._reg(h.arrow, ADV)
        self._choice(h, "dust_mode", "Mode", modes=ADV)
        self._row(h, "dust_thr", "Threshold", modes=ADV)
        self._row(h, "dust_amount", "Amount", modes=ADV)
        nlc_off.append(h)
        nlc_hl.append(self._header(lay, "Adapt to Changing Noise", "adapt_enable", modes=ADV, collapsible=False))
        self._nlc_hl, self._nlc_off, self._nlc_text, self._nlc_dis, self._nlc_on = nlc_hl, nlc_off, {}, [], False
        lay.addStretch(1)

    def _freq(self, parent, prefix, fmt, steps):
        h = self._header(parent, "Frequencies", modes=ADV, level=1)
        for b, name in UI_BANDS:
            w = self._bind(Row([f"{prefix}_{c}_{b}" for c, _ in UI_CHANNELS], name, fmt, f"band:{b}", steps, 1,
                               srange=NOISE_TRACK if fmt == "rel" else None))
            w.band = b
            w.setToolTip("Common to Luminance, Cr and Cb (writes the three components; per-component "
                         "values: Per-component bands (repro))")
            h.add(w)
        return h

    def _spatial(self):
        lay = self._page("Spatial")
        h = self._header(lay, "Spatial Filter", "s_enable", collapsible=False, reset=True)
        h.resetClicked.connect(self.resetSpatial)
        self.assist = BeginRow("Filter Tuning Assist", "Filter Tuning Assist (repro): shows the removed "
                               "component (original - filtered) x4 around mid grey in the primary viewer pane")
        self.assist.toggled.connect(self.assistToggled)
        h.add(self.assist)
        self._reg(self.assist, ADV)
        self._choice(h, "s_quality", "Quality Mode", QUALITY_NAMES, modes=ADV)
        h = self._header(lay, "Noise Reduction Amount")
        self._row(h, "amt_Y", "Luminance", deco="dot:Y", steps=(5, 10))
        hc = self._header(h, "Chrominance", modes=ADV, level=1)
        for c in ("Cr", "Cb"):
            self._row(hc, f"amt_{c}", c, deco=f"dot:{c}", steps=(5, 10), indent=1)
        self._freq(h, "bamt", "amount", (5, 10))
        inline = Row("adv_noise_level", "", fmt="rel", steps=(5, 20), srange=NOISE_TRACK)
        h = self._header(lay, "Noise Level", inline=inline)
        for c, name in UI_CHANNELS:
            self._row(h, f"noise_{c}", name, fmt="rel", deco=f"dot:{c}", modes=ADV)
        self._freq(h, "bnoise", "rel", (5, 20))
        h = self._header(lay, "Artifact Removal", "art_enable", modes=ADV, expanded=False)
        self._row(h, "art_dots", "Dots", fmt="rel")
        self._row(h, "art_lines", "Lines", fmt="rel")
        hf = self._header(h, "Frequencies", level=1)
        for k, name, b in (("art_high", "High", "high"), ("art_mid", "Mid", "medium"), ("art_low", "Low", "low")):
            self._row(hf, k, name, fmt="rel", deco=f"band:{b}", indent=1)
        h = self._header(lay, "Edge Smoothing", "edge_enable", modes=ADV, expanded=False)
        for k, name, b in (("edge_high", "High", "high"), ("edge_mid", "Mid", "medium"), ("edge_low", "Low", "low")):
            self._row(h, k, name, deco=f"band:{b}")
        h = self._header(lay, "Sharpening", "sharp_enable", expanded=False)
        self.sharp_rows = []
        for b, adv_name, beg_name in (("high", "High", "Fine"), ("medium", "Mid", "Medium"), ("low", "Low", "Large")):
            r = self._row(h, f"sharp_{b}", adv_name, deco=f"band:{b}")
            r.names = (adv_name, beg_name)
            self.sharp_rows.append(r)
            self._row(h, f"sharp_range_{b}", "Range", fmt="range", indent=1, modes=ADV)
        hc = self._header(h, "Channels", modes=ADV, level=1)
        for c, name in UI_CHANNELS:
            self._row(hc, f"sharp_ch_{c}", name, deco=f"dot:{c}", indent=1)
        self._check_row(h, "sharp_prevent", "Prevent Over-Sharpening", modes=ADV)
        h = self._header(lay, "Per-component bands (repro)", modes=ADV, expanded=False,
                         help_tip="v5 backend: separate band amounts / noise levels per component "
                                  "(the genuine panel has only the common Frequencies rows)")
        for c, name in UI_CHANNELS:
            hc = self._header(h, f"{name} amount", level=1, expanded=False)
            for b, bname in UI_BANDS:
                self._row(hc, f"bamt_{c}_{b}", bname, deco=f"band:{b}", indent=1)
        for c, name in UI_CHANNELS:
            hc = self._header(h, f"{name} noise level", level=1, expanded=False)
            for b, bname in UI_BANDS:
                self._row(hc, f"bnoise_{c}_{b}", bname, fmt="rel", deco=f"band:{b}", indent=1)
        h = self._header(lay, "Advanced (repro)", modes=ADV, expanded=False,
                         help_tip="v5 backend parameters without a genuine control")
        self._row(h, "adv_threshold", "Temporal threshold", fmt="float")
        self._row(h, "adv_spatial", "Spatial strength", fmt="float")
        self._choice(h, "adv_similarity", "Temporal similarity")
        lay.addStretch(1)

    def _general(self):
        lay = self._page("General")
        h = self._header(lay, "Mix with Original", "mix_enable", collapsible=False, modes=ADV)   # binary: Beginner = Performance only
        self.controls["mix_enable"].edited.disconnect()
        self.controls["mix_enable"].edited.connect(self._mix_enable)
        inline = Row("mix", "", fmt="inv", steps=(5, 20))
        inline.setToolTip("Mix (genuine meaning: higher = more of the filter's effect); shown as 100 - the "
                          "backend's mix with the original")
        hm = self._header(h, "Mix", level=1, inline=inline, expanded=False)
        self._reg(hm.arrow, ADV)
        for c, name in UI_CHANNELS:
            self._row(hm, f"mix_{c}", name, deco=f"dot:{c}", indent=1, modes=ADV)
        h = self._header(lay, "Performance", collapsible=False)
        self.perf = QLabel("—")
        self.perf.setTextFormat(Qt.RichText)
        self.perf.setFrameShape(QFrame.StyledPanel)
        self.perf.setAlignment(Qt.AlignTop | Qt.AlignLeft)
        self.perf.setStyleSheet("background:white; padding:4px")
        self.perf.setMinimumHeight(200)
        self.perf.setWordWrap(True)
        h.add(self.perf)
        row = QWidget()
        rl = QHBoxLayout(row)
        rl.setContentsMargins(0, 4, 0, 0)
        self.b_speed = QPushButton("Check Speed")
        self.b_speed.setToolTip("Process the current frame once with the current settings and show frames/sec")
        self.b_prefs = QPushButton("Preferences...")
        self.b_speed.clicked.connect(self.checkSpeed)
        self.b_prefs.clicked.connect(self.preferences)
        rl.addWidget(self.b_speed)
        rl.addStretch(1)
        rl.addWidget(self.b_prefs)
        h.add(row)
        self.speed = QLabel("")
        self.speed.setFrameShape(QFrame.StyledPanel)
        h.add(self.speed)
        lay.addStretch(1)

    # ---------------- behaviour ----------------
    def _edited(self, k, v):
        self.edited.emit(k, v)

    def _mix_enable(self, k, on):
        """Enable with mix 0 (filter only) starts at the genuine default 75% (backend mix 25)."""
        if on and self.params.get("mix", 0.0) == 0.0:
            self.editedMany.emit({"mix_enable": True, "mix": 25.0})
        else:
            self.edited.emit(k, on)

    def set_mode(self, advanced):
        self.advanced = advanced
        mode = "adv" if advanced else "beg"
        for w, modes in self._modes:
            w.setVisible(mode in modes)
        for r in self.sharp_rows:
            r.label.setText(r.names[0 if advanced else 1])
        self.mode_lbl.setText(mode_text(advanced))

    def set_band_availability(self, levels):
        """Disable band sliders the backend does not use for this profile: nv.spatial_v5 gives a
        plane with L levels the finest L band names for luma and the coarsest L for chroma (4:2:0
        chroma has 4 levels, so its finest band is 'Mid' and 'High' has no effect)."""
        self.used = {}
        for c, _ in UI_CHANNELS:
            L = (levels or {}).get(c, len(V5_BANDS))
            used = set(V5_BANDS[:L] if c == "Y" else V5_BANDS[len(V5_BANDS) - L:])
            self.used[c] = used
            for b, _ in UI_BANDS:
                for k in (f"bamt_{c}_{b}", f"bnoise_{c}_{b}"):
                    w = self.controls[k]
                    w.setEnabled(b in used)
                    w.slider.setToolTip(PARAM_SPECS[k].tip if b in used else
                                        f"Not used with this profile: {c} has {L} frequency bands "
                                        "(chroma at half resolution in 4:2:0)")
        for w in self.common:
            w.setEnabled(any(w.band in u for u in self.used.values()))

    def set_values(self, params, keys=None):
        self.params = dict(params)
        keys = set(keys if keys is not None else params)
        for k in keys:
            for w in [self.controls.get(k)] + self.extra.get(k, []):
                if w is not None:
                    w.set_value(params[k])
        for w in self.common:
            if keys & set(w.keys):
                vals = [params[k] for k in w.keys]
                w.set_value(vals[0], mixed=any(abs(x - vals[0]) > 1e-9 for x in vals))
        on = bool(params.get("mix_enable"))
        mix = self.controls["mix"]
        mix.setEnabled(on)
        if not on and params["mix"] == 0.0:           # disabled: show the value Enable would start with (75%)
            mix.set_value(25.0)
            mix.reset.setVisible(False)

    def set_performance(self, html):
        self.perf.setText(html)

    def set_nlc_active(self, on):
        """Noise Level Check running: Begin -> Finish, its rows yellow, the other Temporal sections and the
        Spatial / General tabs disabled (a parent's setEnabled keeps the children's own n/a state)."""
        on = bool(on)
        self.nl_check.set_checked(on)
        if on == self._nlc_on:
            return
        self._nlc_on = on
        for w in self._nlc_hl:
            lab = w.title if isinstance(w, Header) else w.label
            if on:
                self._nlc_text.setdefault(id(lab), (lab, lab.text(), lab.styleSheet()))
                if isinstance(w, Header):
                    t = f"<b>{w.title_text}</b>"
                    lab.setText(lab.text().replace(t, f"<span style='color:{ACTIVE_TITLE}'>{t}</span>", 1))
                else:
                    lab.setStyleSheet(f"color:{ACTIVE_TITLE}")
        if not on:
            for lab, text, css in self._nlc_text.values():
                lab.setText(text)
                lab.setStyleSheet(css)
            self._nlc_text = {}
        if on:                                  # disable only what is enabled by itself (n/a rows stay off)
            self._nlc_dis = [w for w in self._nlc_off if not w.testAttribute(Qt.WA_ForceDisabled)]
        for w in self._nlc_dis:
            w.setEnabled(not on)
        if not on:
            self._nlc_dis = []
        for i in (1, 2):
            self.tabs.setTabEnabled(i, not on)
        if on:
            self.tabs.setCurrentIndex(0)
            self.t_noise_header.set_expanded(True)
