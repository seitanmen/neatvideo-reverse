// state - see state.h (port of repro/nv_ui_qt/state.py).
#include "state.h"

#include <QStringList>

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>

#include "nvcore_c_prof5.h"
#include "nvcore_c_ui.h"
#include "nvcore_c_rgb.h"

namespace nvqt {

const std::vector<KeyLabel> BANDS = {{"ultra-low", "Ultra Low"}, {"very-low", "Very Low"}, {"low", "Low"},
                                     {"medium", "Mid"}, {"high", "High"}};
const std::vector<KeyLabel> CHANNELS = {{"Y", "Luminance"}, {"Cb", "Cb"}, {"Cr", "Cr"}};
const std::vector<KeyLabel> UI_CHANNELS = {{"Y", "Luminance"}, {"Cr", "Cr"}, {"Cb", "Cb"}};
const std::vector<KeyLabel> UI_BANDS = {{"high", "High"}, {"medium", "Mid"}, {"low", "Low"},
                                        {"very-low", "Very Low"}, {"ultra-low", "Ultra Low"}};
const std::vector<std::string> V5_BANDS = {"high", "medium", "low", "very-low", "ultra-low"};
const double CURVE_X[9] = {0.0, 32.0, 64.0, 96.0, 128.0, 160.0, 192.0, 224.0, 256.0};
const char* NOT_IMPLEMENTED = "Not available in the reproduction: the v5 backend has no counterpart of this filter.";

int ch_index(const std::string& c) { return c == "Y" ? 0 : c == "Cb" ? 1 : 2; }

namespace {

// nvfilter.V5_DEFAULTS (the values the specs are derived from)
const double D_RADIUS = 2, D_TEMPORAL = 1.0, D_THRESHOLD = 4.0, D_SPATIAL = 2.0, D_LUMA = 0.7, D_CHROMA = 1.0,
             D_NOISE_LEVEL = 1.0, D_PYR = 1.5, D_MIX = 0.0;
const int D_BS = 16, D_SEARCH = 16;
double d_band_amount(const std::string& b) { return b == "ultra-low" ? 0.0 : 1.0; }

const std::map<std::string, std::string> BAND_TIP = {{"ultra-low", "ultra low"}, {"very-low", "very low"},
                                                     {"low", "low"}, {"medium", "medium"}, {"high", "high"}};

Spec mk(const std::string& kind, PyObj def, double lo = 0.0, double hi = 100.0, std::string label = "",
        std::string tip = "", bool implemented = true, std::vector<std::string> choices = {}, double step = 1.0) {
    Spec s;
    s.kind = kind;
    s.def = std::move(def);
    s.lo = lo;
    s.hi = hi;
    s.label = std::move(label);
    s.tip = std::move(tip);
    s.implemented = implemented;
    s.choices = std::move(choices);
    s.step = step;
    return s;
}

PyObj R(double v) { return PyObj::real(v); }
PyObj B(bool v) { return PyObj::boolean(v); }

std::vector<std::pair<std::string, Spec>> build_specs() {
    std::vector<std::pair<std::string, Spec>> S;
    auto add = [&](const std::string& k, Spec s) { S.emplace_back(k, std::move(s)); };
    const std::string NI = NOT_IMPLEMENTED;
    add("t_enable", mk("bool", B(true), 0, 100, "Enable Temporal Filter"));
    add("t_radius", mk("int", PyObj::integer(static_cast<int>(D_RADIUS)), 0, 5, "Temporal Radius",
                       "Radius determines the number of consecutive frames (before and after the current "
                       "frame) used by the Temporal filter (1 frame ... 5 frames)."));
    add("t_amount", mk("pct", R(100.0 * D_TEMPORAL), 0, 100, "Temporal Noise Reduction Amount",
                       "Amount of temporal noise reduction - high values -> more reduction of detected noise"));
    add("t_quality", mk("choice", PyObj::str("Normal"), 0, 100, "Temporal Quality Mode", NI, false, {"Normal", "High"}));
    for (const auto& [c, name] : CHANNELS)
        add("noise_" + c, mk("pct", R(100.0 * 1.0), 0, 250, name + " Noise Level",
                             "Estimation of noise level in the " + name + " component (relative to noise profile)"
                             " - higher values -> more image elements are treated as noise"));
    add("s_enable", mk("bool", B(true), 0, 100, "Enable Spatial Filter"));
    add("s_quality", mk("choice", PyObj::str("Normal"), 0, 100, "Spatial Quality Mode", NI, false, {"Normal", "High"}));
    for (const auto& [c, name] : CHANNELS) {
        const double amt = c == "Y" ? D_LUMA : D_CHROMA;
        add("amt_" + c, mk("pct", R(100.0 * amt), 0, 100, name,
                           "Noise Reduction Amount of the " + name + " component (multiplies the band amounts)"));
        for (const auto& [b, bname] : BANDS) {
            add("bamt_" + c + "_" + b,
                mk("pct", R(100.0 * d_band_amount(b)), 0, 100, bname,
                   "Amount of noise reduction in the " + BAND_TIP.at(b) + " frequency component of " + name +
                       " - high values -> more reduction of detected noise"));
            add("bnoise_" + c + "_" + b,
                mk("pct", R(100.0 * 1.0), 0, 250, bname,
                   "Estimation of noise level in " + BAND_TIP.at(b) + " frequency component (relative to noise "
                   "profile) - higher values -> more image elements are treated as noise"));
        }
    }
    add("sharp_enable", mk("bool", B(false), 0, 100, "Enable Sharpening", NI, false));
    for (const auto& [b, bname] : BANDS)
        add("sharp_" + b, mk("pct", R(0.0), 0, 100, bname + " Freq Sharpening Amount",
                             "Amount of sharpening applied to " + BAND_TIP.at(b) + " frequency component - high "
                             "values -> more sharpening\n" + NI, false));
    add("sharp_prevent", mk("bool", B(true), 0, 100, "Prevent Over-Sharpening", NI, false));
    add("jitter_enable", mk("bool", B(false), 0, 100, "Enable Jitter Filter", NI, false));
    add("jitter_thr", mk("pct", R(0.0), 0, 100, "Jitter Threshold", NI, false));
    add("dust_enable", mk("bool", B(false), 0, 100, "Enable Dust and Scratches", NI, false));
    add("dust_mode", mk("choice", PyObj::str("Normal"), 0, 100, "Mode", NI, false, {"Normal", "Aggressive"}));
    add("dust_thr", mk("pct", R(0.0), 0, 100, "Threshold", NI, false));
    add("flicker_enable", mk("bool", B(false), 0, 100, "Enable Flicker Filter", NI, false));
    add("flicker_local", mk("pct", R(0.0), 0, 100, "Local Flicker Threshold", NI, false));
    add("flicker_full", mk("pct", R(0.0), 0, 100, "Full Frame Flicker Threshold", NI, false));
    for (const auto& [k, lab] : std::vector<KeyLabel>{{"low", "Low"}, {"mid", "Mid"}, {"high", "High"}})
        add("edge_" + k, mk("pct", R(0.0), 0, 100, lab + " Freq Edge Smoothing", NI, false));
    add("mix", mk("pct", R(100.0 * D_MIX), 0, 100, "Mix with original",
                  "Mixing with the original frame - 0% = filtered result only, 100% = original only "
                  "(v5 backend semantics; the panel shows 100 - mix like the genuine Mix slider)"));
    add("mix_enable", mk("bool", B(false), 0, 100, "Enable Mix with Original", "Off: the filtered result only (backend mix 0)"));
    add("t_noise", mk("pct", R(100.0), 0, 250, "Temporal Noise Level",
                      "Estimation of noise level for the Temporal filter (relative to noise profile) - "
                      "higher values -> more image elements are treated as noise.\n(repro: multiplies the "
                      "noise level of the temporal similarity only, on top of the Overall / Luminance Noise "
                      "Level; the Spatial filter is not affected)"));
    add("rep_rate", mk("pct", R(0.0), 0, 100, "Repeat Rate", NI, false));
    add("dust_amount", mk("pct", R(100.0), 0, 100, "Amount", NI, false));
    add("adapt_enable", mk("bool", B(false), 0, 100, "Adapt to Changing Noise", NI, false));
    add("art_enable", mk("bool", B(true), 0, 100, "Enable Artifact Removal", NI, false));
    for (const auto& [k, lab] : std::vector<KeyLabel>{{"art_dots", "Dots"}, {"art_lines", "Lines"}, {"art_high", "High"},
                                                      {"art_mid", "Mid"}, {"art_low", "Low"}})
        add(k, mk("pct", R(100.0), 0, 250, lab, NI, false));
    add("edge_enable", mk("bool", B(false), 0, 100, "Enable Edge Smoothing", NI, false));
    for (const std::string b : {"high", "medium", "low"})
        add("sharp_range_" + b, mk("float", R(0.0), -1.0, 1.0, "Range", NI, false, {}, 0.05));
    for (const auto& [c, name] : UI_CHANNELS) {
        add("sharp_ch_" + c, mk("pct", R(c == "Y" ? 100.0 : 0.0), 0, 250, name, NI, false));
        add("mix_" + c, mk("pct", R(100.0), 0, 100, name, NI, false));
    }
    add("adv_threshold", mk("float", R(D_THRESHOLD), 0.25, 8.0, "Temporal threshold",
                            "v5 temporal similarity tolerance (higher -> more averaging)", true, {}, 0.05));
    add("adv_spatial", mk("float", R(D_SPATIAL), 0.25, 4.0, "Spatial strength",
                          "v5 spatial Wiener strength (multiple of the noise variance)", true, {}, 0.05));
    add("adv_noise_level", mk("pct", R(100.0 * D_NOISE_LEVEL), 25, 400, "Overall Noise Level",
                              "v5 overall noise level relative to the profile"));
    add("adv_similarity", mk("choice", PyObj::str("v5"), 0, 100, "Temporal similarity",
                             "v5: 5x5 energy weight; pyramid: FUN_0099a76c analogue (Agents.md §6.4)", true,
                             {"v5", "pyramid"}));
    return S;
}

}  // namespace

const std::vector<std::pair<std::string, Spec>>& param_specs() {
    static const auto S = build_specs();
    return S;
}

const Spec& spec(const std::string& key) {
    for (const auto& kv : param_specs())
        if (kv.first == key) return kv.second;
    throw std::runtime_error("unknown parameter " + key);
}

PyObj default_params() {
    PyObj p = PyObj::dict();
    for (const auto& [k, s] : param_specs()) p.set(k, s.def);
    return p;
}

PyObj coerce(const std::string& key, const PyObj& value) {
    const Spec& s = spec(key);
    if (s.kind == "bool") return PyObj::boolean(value.truthy());
    if (s.kind == "choice") {
        if (value.type() == PyObj::STR && std::find(s.choices.begin(), s.choices.end(), value.as_str()) != s.choices.end())
            return value;
        return s.def;
    }
    if (s.kind == "int") {
        // int(min(max(int(round(float(value))), lo), hi)) (round half to even)
        const double r = std::nearbyint(value.as_double());
        double v = std::max(r, s.lo);
        v = std::min(v, s.hi);
        return PyObj::integer(static_cast<int64_t>(v));
    }
    double v = std::max(value.as_double(), s.lo);
    v = std::min(v, s.hi);
    return PyObj::real(v);
}

PyObj backend_params(const PyObj& ui) {
    auto f = [&](const std::string& k) { return ui.get(k).as_double(); };
    // chans = {c: {"amount", "bands", "noise"}} (state.CHANNELS order, BANDS order inside)
    PyObj chans = PyObj::dict();
    for (const auto& [c, _] : CHANNELS) {
        PyObj ch = PyObj::dict();
        ch.set("amount", PyObj::real(f("amt_" + c) / 100.0));
        PyObj bands = PyObj::dict(), noise = PyObj::dict();
        for (const auto& [b, _b] : BANDS) bands.set(b, PyObj::real(f("bamt_" + c + "_" + b) / 100.0));
        for (const auto& [b, _b] : BANDS) noise.set(b, PyObj::real(f("bnoise_" + c + "_" + b) / 100.0));
        ch.set("bands", bands);
        ch.set("noise", noise);
        chans.set(c, ch);
    }
    // nv.v5_params(**kw): V5_DEFAULTS order, dict values merged into the default dicts (their key order)
    PyObj P = PyObj::dict();
    P.set("radius", PyObj::integer(ui.get("t_radius").as_int()));
    P.set("temporal", PyObj::real(ui.get("t_enable").truthy() ? f("t_amount") / 100.0 : 0.0));
    P.set("threshold", PyObj::real(f("adv_threshold")));
    P.set("spatial", PyObj::real(f("adv_spatial")));
    P.set("luma", chans.get("Y").get("amount"));
    P.set("chroma", chans.get("Cb").get("amount"));
    PyObj ba = PyObj::dict(), bn = PyObj::dict();
    for (const auto& b : V5_BANDS) {
        ba.set(b, chans.get("Y").get("bands").get(b));
        bn.set(b, chans.get("Y").get("noise").get(b));
    }
    P.set("band_amounts", ba);
    P.set("band_noise", bn);
    P.set("noise_level", PyObj::real(f("adv_noise_level") / 100.0));
    PyObj cn = PyObj::dict();
    for (const auto& [c, _] : CHANNELS) cn.set(c, PyObj::real(f("noise_" + c) / 100.0));
    P.set("channel_noise", cn);
    P.set("similarity", ui.get("adv_similarity"));
    P.set("pyramid_scale", PyObj::real(D_PYR));
    P.set("bs", PyObj::integer(D_BS));
    P.set("search", PyObj::integer(D_SEARCH));
    P.set("mix", PyObj::real(ui.get("mix_enable").truthy() ? f("mix") / 100.0 : 0.0));
    P.set("ui_channels", chans);
    P.set("ui_spatial", PyObj::boolean(ui.get("s_enable").truthy()));
    P.set("t_noise", PyObj::real(f("t_noise") / 100.0));
    return P;
}

PyObj assist_params() {
    PyObj ui = default_params();
    ui.set("t_enable", PyObj::boolean(false));
    for (const auto& [c, _] : CHANNELS) {
        ui.set("amt_" + c, PyObj::real(100.0));
        for (const auto& [b, _b] : BANDS) ui.set("bamt_" + c + "_" + b, PyObj::real(100.0));
    }
    return backend_params(ui);
}

// ---------------- profile ----------------
Factors unit_factors() {
    Factors f;
    for (auto& row : f) row.fill(1.0);
    return f;
}

double ProfileState::tune_value(const std::string& k) const {
    return tune.has(k) ? tune.get(k).as_double() : 1.0;
}

PyObj ProfileState::effective() const {
    return channels == "RGB" ? rgb_effective_profile(base, factors, tune) : effective_profile(base, factors, tune);
}

PyObj ProfileState::to_json() const {
    PyObj d = PyObj::dict();
    if (channels == "RGB") {            // G1: older UIs refuse the format instead of misreading R/G/B levels
        d.set("format", PyObj::str("nv_repro_profile_rgb"));
        d.set("version", PyObj::integer(2));
        d.set("channels", PyObj::str("RGB"));
    } else {
        d.set("format", PyObj::str("nv_repro_profile"));
        d.set("version", PyObj::integer(1));
    }
    d.set("device", PyObj::str(device));
    d.set("base", base);
    PyObj f = PyObj::list();
    for (const auto& row : factors) f.append(PyObj::reals(std::vector<double>(row.begin(), row.end())));
    d.set("factors", f);
    PyObj p = PyObj::list();
    for (const auto& s : pinned) p.append(PyObj::ints(std::vector<int>(s.begin(), s.end())));
    d.set("pinned", p);
    d.set("tune", tune);
    return d;
}

void check_rgb_keys(const PyObj& b) {
    auto num = [](const PyObj& v) {
        return (v.type() == PyObj::INT || v.type() == PyObj::FLOAT || v.type() == PyObj::BOOL) && std::isfinite(v.as_double());
    };
    auto boolean = [](const PyObj& v) { return v.type() == PyObj::BOOL; };
    auto grid = [](const PyObj& v, size_t r, size_t c, const std::function<bool(const PyObj&)>& ok) {
        if (!v.is_list() || v.size() != r) return false;
        for (size_t i = 0; i < r; ++i) {
            if (!v[i].is_list() || v[i].size() != c) return false;
            for (size_t j = 0; j < c; ++j)
                if (!ok(v[i][j])) return false;
        }
        return true;
    };
    for (const auto& [k, c] : {std::pair<const char*, size_t>{"ui_rgb_weights", 3}, {"aft_color_weights", 3},
                               {"aft_levels", 9}})
        if (b.has(k) && !grid(b.get(k), 3, c, num))
            throw std::runtime_error(std::string("profile ") + k + " must be 3 x " + std::to_string(c) + " finite numbers");
    if (b.has("ui_pre_aft")) {
        const PyObj& p = b.get("ui_pre_aft");
        const size_t n = b.get("curve_centers").size();
        bool ok = p.is_dict() && p.get("curve").is_dict() && p.get("sigma").is_dict();
        for (const auto& [c, _] : CHANNELS) {
            if (!ok) break;
            const PyObj& cv = p.get("curve").get(c);
            ok = cv.is_list() && cv.size() == n && num(p.get("sigma").get(c));
            for (size_t i = 0; ok && i < cv.size(); ++i) ok = num(cv[i]);
        }
        if (!ok) throw std::runtime_error("profile ui_pre_aft must hold the curve and sigma of Y / Cb / Cr (finite numbers)");
    }
    if (b.has("aft_bin_flags")) {
        const PyObj& f = b.get("aft_bin_flags");
        bool ok = f.is_dict() && f.has("marker");
        for (const char* k : {"marker", "clip", "gt3"})
            if (ok && f.has(k)) ok = grid(f.get(k), 3, 9, boolean);
        if (!ok) throw std::runtime_error("profile aft_bin_flags must be 3 x 9 booleans (marker, clip, gt3)");
    }
}

ProfileState ProfileState::from_json(const PyObj& d) {
    if (!d.is_dict() || d.get("format").type() != PyObj::STR ||
        (d.get("format").as_str() != "nv_repro_profile" && d.get("format").as_str() != "nv_repro_profile_rgb"))
        throw std::runtime_error("not a repro profile file (format nv_repro_profile)");
    const bool rgb = d.get("format").as_str() == "nv_repro_profile_rgb";
    if (rgb && !(d.get("channels").type() == PyObj::STR && d.get("channels").as_str() == "RGB"))
        throw std::runtime_error("profile channels must be RGB (format nv_repro_profile_rgb)");
    const PyObj& b = d.get("base");
    static const char* KEYS[] = {"mode", "area", "levels", "band_ratio", "tot_ratio", "curve", "curve_centers", "sigma"};
    std::string missing;
    for (const char* k : KEYS)
        if (!b.is_dict() || !b.has(k)) missing += (missing.empty() ? "" : ", ") + std::string(k);
    if (!missing.empty()) throw std::runtime_error("profile lacks " + missing);
    for (const auto& [c, _] : CHANNELS) {
        if (b.get("curve").get(c).size() != b.get("curve_centers").size() || !b.get("sigma").has(c) ||
            !b.get("levels").has(c))
            throw std::runtime_error("profile curve / sigma / levels of " + c + " are inconsistent");
    }
    const PyObj& f = d.has("factors") ? d.get("factors") : PyObj::none();
    ProfileState ps;
    if (d.has("factors")) {
        if (f.size() != 3) throw std::runtime_error("profile factors must be 3 x 9");
        for (size_t c = 0; c < 3; ++c)
            if (f[c].size() != 9) throw std::runtime_error("profile factors must be 3 x 9");
    }
    auto finite = [](const PyObj& v) {
        return (v.type() == PyObj::INT || v.type() == PyObj::FLOAT || v.type() == PyObj::BOOL) && std::isfinite(v.as_double());
    };
    bool ok = true;
    if (d.has("factors"))
        for (size_t c = 0; c < 3; ++c)
            for (size_t k = 0; k < 9; ++k) ok = ok && finite(f[c][k]);
    for (size_t i = 0; i < b.get("curve_centers").size(); ++i) ok = ok && finite(b.get("curve_centers")[i]);
    for (const auto& [c, _] : CHANNELS) {
        const PyObj& cv = b.get("curve").get(c);
        for (size_t i = 0; i < cv.size(); ++i) ok = ok && finite(cv[i]);
        ok = ok && finite(b.get("sigma").get(c));
    }
    if (!ok) throw std::runtime_error("profile factors / curve / sigma must be finite numbers");
    PyObj tune = d.has("tune") ? d.get("tune") : PyObj::dict();
    if (!tune.is_dict()) throw std::runtime_error("profile tune must be a dict of all / Y / Cb / Cr with finite values");
    for (const auto& k : tune.keys())
        if ((k != "all" && k != "Y" && k != "Cb" && k != "Cr") || !finite(tune.get(k)))
            throw std::runtime_error("profile tune must be a dict of all / Y / Cb / Cr with finite values");
    PyObj t2 = PyObj::dict();
    for (const auto& k : tune.keys())
        t2.set(k, PyObj::real(std::min(std::max(tune.get(k).as_double(), TUNE_MIN), TUNE_MAX)));
    check_rgb_keys(b);
    if (rgb && !rgb_ready(b))
        throw std::runtime_error("R/G/B profile lacks its colour weights (ui_rgb_weights / aft_color_weights, ui_pre_aft)");
    ps.channels = rgb ? "RGB" : "YCbCr";
    ps.base = b;
    if (d.has("factors"))
        for (size_t c = 0; c < 3; ++c)
            for (size_t k = 0; k < 9; ++k) ps.factors[c][k] = f[c][k].as_double();
    if (d.has("pinned")) {
        const PyObj& p = d.get("pinned");
        for (size_t c = 0; c < 3 && c < p.size(); ++c)
            for (size_t i = 0; i < p[c].size(); ++i) ps.pinned[c].insert(static_cast<int>(p[c][i].as_int()));
    }
    if (d.has("device") && d.get("device").type() == PyObj::STR) ps.device = d.get("device").as_str();
    ps.tune = t2;
    return ps;
}

PyObj effective_profile(const PyObj& base, const Factors& factors, const PyObj& tune) {
    if (base.is_none()) return PyObj::none();
    const PyObj& centers = base.get("curve_centers");
    const int n = static_cast<int>(centers.size());
    std::vector<double> cx = centers.as_doubles(), curve(3 * static_cast<size_t>(n)), fac(27), tot(3);
    for (int c = 0; c < 3; ++c) {
        const auto v = base.get("curve").get(CHANNELS[c].first).as_doubles();
        for (int i = 0; i < n; ++i) curve[static_cast<size_t>(c * n + i)] = v[static_cast<size_t>(i)];
        for (int k = 0; k < 9; ++k) fac[static_cast<size_t>(9 * c + k)] = factors[c][k];
        tot[static_cast<size_t>(c)] = base.get("tot_ratio").get(CHANNELS[c].first).as_double();
    }
    auto tv = [&](const char* k) { return tune.has(k) ? tune.get(k).as_double() : 1.0; };
    const double tn[4] = {tv("all"), tv("Y"), tv("Cb"), tv("Cr")};
    const double lvl = base.has("area_level") ? base.get("area_level").as_double() : 128.0;
    std::vector<double> cout(curve.size()), sig(3);
    if (nvc_effective_curve(cx.data(), n, curve.data(), fac.data(), tn, lvl, tot.data(), cout.data(), sig.data()) == 0)
        return base;
    PyObj prof = base;
    for (int c = 0; c < 3; ++c) {
        std::vector<double> v(cout.begin() + c * n, cout.begin() + (c + 1) * n);
        prof.at("curve").set(CHANNELS[c].first, PyObj::reals(v));
    }
    PyObj s = PyObj::dict();
    for (int c = 0; c < 3; ++c) s.set(CHANNELS[c].first, PyObj::real(sig[static_cast<size_t>(c)]));
    prof.set("sigma", s);
    PyObj uf = PyObj::list();
    for (const auto& row : factors) uf.append(PyObj::reals(std::vector<double>(row.begin(), row.end())));
    prof.set("ui_factors", uf);
    prof.set("ui_tune", tune);
    return prof;
}

Factors auto_complete(const Factors& factors, const std::array<std::set<int>, 3>& pinned) {
    // float(np.interp(CURVE_X[i], xs, ys)) of the bins that were not edited (nvc_interp = numpy's arr_interp,
    // native/verify_bridge_ui.py "interp"); constant beyond the outermost edited bin as np.interp
    Factors out = factors;
    for (int c = 0; c < 3; ++c) {
        if (pinned[c].empty()) continue;
        std::vector<double> xs, ys, qx;
        std::vector<int> qi;
        for (int i : pinned[c]) {                 // sorted(pinned[c]): std::set iterates in order
            xs.push_back(CURVE_X[i]);
            ys.push_back(factors[c][i]);
        }
        for (int i = 0; i < 9; ++i)
            if (!pinned[c].count(i)) {
                qx.push_back(CURVE_X[i]);
                qi.push_back(i);
            }
        if (qx.empty()) continue;
        std::vector<double> r(qx.size());
        nvc_interp(xs.data(), ys.data(), static_cast<int>(xs.size()), qx.data(), static_cast<int>(qx.size()), r.data());
        for (size_t k = 0; k < qi.size(); ++k) out[c][qi[k]] = r[k];
    }
    return out;
}

// ---------------- the R/G/B equalizer (G1) ----------------
bool rgb_ready(const PyObj& base) {
    if (base.is_none()) return false;
    if (base.get("aft_applied").truthy()) return base.has("ui_pre_aft") && base.has("aft_color_weights");
    return base.has("ui_rgb_weights");
}

std::optional<Factors> rgb_reference(const PyObj& base) {
    if (!base.is_none() && base.get("aft_applied").truthy()) {
        if (!base.has("aft_levels")) return std::nullopt;
        Factors f;
        const PyObj& l = base.get("aft_levels");
        for (size_t j = 0; j < 3; ++j)
            for (size_t k = 0; k < 9; ++k) f[j][k] = l[j][k].as_double();
        return f;
    }
    return unit_factors();
}

PyObj rgb_effective_profile(const PyObj& base, const Factors& levels, const PyObj& tune) {
    if (base.is_none()) return PyObj::none();
    auto tv = [&](const char* k) { return tune.has(k) ? tune.get(k).as_double() : 1.0; };
    const double t4[4] = {tv("all"), tv("Y"), tv("Cb"), tv("Cr")};
    const auto ref = rgb_reference(base);
    if (ref && *ref == levels && t4[0] * t4[1] == 1.0 && t4[0] * t4[2] == 1.0 && t4[0] * t4[3] == 1.0) return base;
    if (!rgb_ready(base)) throw std::runtime_error("the profile has no colour weights for the R/G/B equalizer");
    const bool aft = base.get("aft_applied").truthy();
    const PyObj& src = aft ? base.get("ui_pre_aft").get("curve") : base.get("curve");
    const PyObj& cwo = aft ? base.get("aft_color_weights") : base.get("ui_rgb_weights");
    const PyObj& centers = base.get("curve_centers");
    const int n = static_cast<int>(centers.size());
    std::vector<double> cx = centers.as_doubles(), sv(3 * static_cast<size_t>(n)), cw(9), lv(27), tot(3);
    for (int c = 0; c < 3; ++c) {
        const auto v = src.get(CHANNELS[c].first).as_doubles();
        for (int i = 0; i < n; ++i) sv[static_cast<size_t>(c * n + i)] = v[static_cast<size_t>(i)];
        for (int j = 0; j < 3; ++j) cw[static_cast<size_t>(3 * c + j)] = cwo[static_cast<size_t>(c)][static_cast<size_t>(j)].as_double();
        for (int k = 0; k < 9; ++k) lv[static_cast<size_t>(9 * c + k)] = levels[c][k];
        tot[static_cast<size_t>(c)] = base.get("tot_ratio").get(CHANNELS[c].first).as_double();
    }
    if (aft && levels == unit_factors() && t4[0] * t4[1] == 1.0 && t4[0] * t4[2] == 1.0 && t4[0] * t4[3] == 1.0) {
        // levels 1.0 after AFT (Reset Fine-Tuning): the curve and sigma before AFT as they were
        PyObj prof = base;
        prof.set("curve", base.get("ui_pre_aft").get("curve"));
        prof.set("sigma", base.get("ui_pre_aft").get("sigma"));
        prof.set("ui_channels", PyObj::str("RGB"));
        PyObj uf = PyObj::list();
        for (const auto& row : levels) uf.append(PyObj::reals(std::vector<double>(row.begin(), row.end())));
        prof.set("ui_factors", uf);
        prof.set("ui_tune", tune);
        return prof;
    }
    const double lvl = base.has("area_level") ? base.get("area_level").as_double() : 128.0;
    std::vector<double> cout(sv.size()), sig(3);
    if (nvc_rgb_effective_curve(cx.data(), n, sv.data(), cw.data(), lv.data(), t4, lvl, tot.data(), cout.data(),
                                sig.data()) != 0)
        throw std::runtime_error("nvc_rgb_effective_curve failed");
    PyObj prof = base;
    for (int c = 0; c < 3; ++c) {
        std::vector<double> v(cout.begin() + c * n, cout.begin() + (c + 1) * n);
        prof.at("curve").set(CHANNELS[c].first, PyObj::reals(v));
    }
    PyObj s = PyObj::dict();
    for (int c = 0; c < 3; ++c) s.set(CHANNELS[c].first, PyObj::real(sig[static_cast<size_t>(c)]));
    prof.set("sigma", s);
    prof.set("ui_channels", PyObj::str("RGB"));
    PyObj uf = PyObj::list();
    for (const auto& row : levels) uf.append(PyObj::reals(std::vector<double>(row.begin(), row.end())));
    prof.set("ui_factors", uf);
    prof.set("ui_tune", tune);
    return prof;
}

Factors auto_complete(const ProfileState& ps) {
    std::array<std::set<int>, 3> fixed = ps.pinned;
    const PyObj& b = ps.base;
    if (ps.channels == "RGB" && !b.is_none() && b.get("aft_applied").truthy() && !b.get("aft_reset").truthy() &&
        b.get("aft_bin_flags").truthy()) {
        const PyObj& mk = b.get("aft_bin_flags").get("marker");
        for (size_t c = 0; c < 3; ++c)
            for (size_t k = 0; k < 9; ++k)
                if (!mk[c][k].truthy()) fixed[c].insert(static_cast<int>(k));
    }
    return auto_complete(ps.factors, fixed);
}

ProfileState after_aft(const ProfileState& ps, const PyObj& prof) {
    ProfileState n = ps;
    n.base = prof;
    if (rgb_ready(prof) && prof.has("aft_levels")) {
        n.channels = "RGB";
        n.factors = *rgb_reference(prof);
    } else {
        n.factors = unit_factors();
    }
    n.pinned = {};
    return n;
}

ProfileState reset_fine_tune(const ProfileState& ps) {
    ProfileState n = ps;
    n.factors = unit_factors();
    n.pinned = {};
    if (n.has_profile()) {
        n.base.set("aft_reset", PyObj::boolean(true));
        if (rgb_ready(n.base)) n.channels = "RGB";
    }
    return n;
}

PyObj preset_json(const PyObj& params) {
    PyObj d = PyObj::dict();
    d.set("format", PyObj::str("nv_repro_filter_preset"));
    d.set("version", PyObj::integer(1));
    d.set("params", params);
    return d;
}

PyObj params_from_json(const PyObj& d) {
    if (!d.is_dict() || d.get("format").type() != PyObj::STR || d.get("format").as_str() != "nv_repro_filter_preset" ||
        (d.has("params") && !d.get("params").is_dict()))
        throw std::runtime_error("not a repro filter preset (format nv_repro_filter_preset)");
    PyObj P = default_params();
    const PyObj raw = d.has("params") ? d.get("params") : PyObj::dict();
    for (const auto& k : raw.keys()) {
        bool known = false;
        for (const auto& kv : param_specs()) known = known || kv.first == k;
        if (known) P.set(k, coerce(k, raw.get(k)));
    }
    if (!raw.has("mix_enable")) P.set("mix_enable", PyObj::boolean(P.get("mix").as_double() > 0));
    return P;
}

// ---------------- undo ----------------
ParamsCommand::ParamsCommand(AppState* s, PyObj o, PyObj n, const QString& text, std::optional<std::string> mk)
    : QUndoCommand(text), st(s), old_v(std::move(o)), new_v(std::move(n)), merge_key(std::move(mk)) {}

bool ParamsCommand::mergeWith(const QUndoCommand* other) {
    auto* o = dynamic_cast<const ParamsCommand*>(other);
    if (!o || !o->merge_key || !merge_key || *o->merge_key != *merge_key) return false;
    new_v = o->new_v;
    return true;
}
void ParamsCommand::redo() { st->apply_params(new_v); }
void ParamsCommand::undo() { st->apply_params(old_v); }

ProfileCommand::ProfileCommand(AppState* s, ProfileState o, ProfileState n, const QString& text,
                               std::optional<std::string> mk)
    : QUndoCommand(text), st(s), old_v(std::move(o)), new_v(std::move(n)), merge_key(std::move(mk)) {}

bool ProfileCommand::mergeWith(const QUndoCommand* other) {
    auto* o = dynamic_cast<const ProfileCommand*>(other);
    if (!o || !o->merge_key || !merge_key || *o->merge_key != *merge_key) return false;
    new_v = o->new_v;
    return true;
}
void ProfileCommand::redo() { st->apply_profile(new_v); }
void ProfileCommand::undo() { st->apply_profile(old_v); }

AppState::AppState(QObject* parent) : QObject(parent), params(default_params()), undo(new QUndoStack(this)) {}

void AppState::set_param(const std::string& key, const PyObj& value) {
    const PyObj v = coerce(key, value);
    if (params.get(key) == v) return;
    PyObj n = params;
    n.set(key, v);
    undo->push(new ParamsCommand(this, params, n, QString::fromStdString("Change " + spec(key).label),
                                 key + "#" + std::to_string(seq_)));
}

void AppState::set_params(const std::vector<std::pair<std::string, PyObj>>& updates, const QString& text, bool merge) {
    PyObj n = params;
    std::vector<std::string> ks;
    for (const auto& [k, v] : updates) {
        n.set(k, coerce(k, v));
        ks.push_back(k);
    }
    std::sort(ks.begin(), ks.end());
    std::string mk;
    for (const auto& k : ks) mk += k + ",";
    if (n != params)
        undo->push(new ParamsCommand(this, params, n, text,
                                     merge ? std::optional<std::string>(mk + "#" + std::to_string(seq_)) : std::nullopt));
}

void AppState::set_all_params(const PyObj& p, const QString& text) {
    PyObj n = default_params();
    for (const auto& k : p.keys()) {
        bool known = false;
        for (const auto& kv : param_specs()) known = known || kv.first == k;
        if (known) n.set(k, coerce(k, p.get(k)));
    }
    if (n != params) undo->push(new ParamsCommand(this, params, n, text, std::nullopt));
}

void AppState::apply_params(const PyObj& p) {
    QStringList changed;
    for (const auto& k : p.keys())
        if (!(p.get(k) == params.get(k)) || !params.has(k)) changed << QString::fromStdString(k);
    params = p;
    if (!changed.isEmpty()) emit paramsChanged(changed);
}

void AppState::set_profile(const ProfileState& ps, const QString& text, std::optional<std::string> merge_key) {
    undo->push(new ProfileCommand(this, profile, ps, text, std::move(merge_key)));
}

void AppState::set_factors(int c, const std::array<double, 9>& f, std::optional<int> pin) {
    ProfileState n = profile;
    for (int k = 0; k < 9; ++k) n.factors[c][k] = std::min(std::max(f[k], FACTOR_MIN), FACTOR_MAX);
    if (pin) n.pinned[c].insert(*pin);
    set_profile(n, "Manual Fine-Tune", "curve#" + std::to_string(c) + "#" + std::to_string(seq_));
}

void AppState::set_factors_all(const std::array<double, 9>& f, int pin) {
    ProfileState n = profile;
    for (int c = 0; c < 3; ++c) {
        n.factors[c][pin] = std::min(std::max(f[pin], FACTOR_MIN), FACTOR_MAX);
        n.pinned[c].insert(pin);
    }
    set_profile(n, "Manual Fine-Tune", "curve#all#" + std::to_string(seq_));
}

void AppState::set_tune(const std::string& k, double value) {
    const double v = std::min(std::max(value, TUNE_MIN), TUNE_MAX);
    if (profile.tune_value(k) == v) return;
    ProfileState n = profile;
    n.tune.set(k, PyObj::real(v));
    set_profile(n, "Profile Noise Level", "tune#" + k + "#" + std::to_string(seq_));
}

const PyObj* AppState::effective() {
    if (!eff_ && profile.has_profile()) eff_ = profile.effective();
    return eff_ ? &*eff_ : nullptr;
}

void AppState::apply_profile(const ProfileState& ps) {
    profile = ps;
    eff_.reset();
    emit profileChanged();
}

}  // namespace nvqt
