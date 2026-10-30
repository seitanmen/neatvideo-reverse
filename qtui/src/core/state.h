// Application state of the C++ UI, ported from repro/nv_ui_qt/state.py: filter parameters in UI units
// (PARAM_SPECS), the noise profile state (ProfileState) and the undo commands.  The parameter and profile
// dicts are PyObj values with Python's dict semantics, so files and derived dicts match the Python UI.
#pragma once

#include <QObject>
#include <QUndoCommand>
#include <QUndoStack>

#include <array>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "pyobj.h"

namespace nvqt {

// state.BANDS / CHANNELS / UI_CHANNELS / UI_BANDS (key, label)
using KeyLabel = std::pair<std::string, std::string>;
extern const std::vector<KeyLabel> BANDS;        // ultra-low .. high
extern const std::vector<KeyLabel> CHANNELS;     // Y, Cb, Cr
extern const std::vector<KeyLabel> UI_CHANNELS;  // Y, Cr, Cb (genuine panel order)
extern const std::vector<KeyLabel> UI_BANDS;     // high .. ultra-low
extern const std::vector<std::string> V5_BANDS;  // nvfilter.V5_BANDS: high .. ultra-low
int ch_index(const std::string& c);              // state.CH_INDEX (Y 0, Cb 1, Cr 2)
extern const double CURVE_X[9];                  // 0, 32, .., 256
constexpr double FACTOR_MIN = 0.2, FACTOR_MAX = 3.0;
constexpr double TUNE_MIN = 0.2, TUNE_MAX = 3.0;
extern const char* NOT_IMPLEMENTED;

struct Spec {
    std::string kind;          // "pct", "float", "int", "bool", "choice"
    PyObj def;
    double lo = 0.0, hi = 100.0;
    std::string label, tip;
    bool implemented = true;
    std::vector<std::string> choices;
    double step = 1.0;
};

// PARAM_SPECS in the Python insertion order
const std::vector<std::pair<std::string, Spec>>& param_specs();
const Spec& spec(const std::string& key);
PyObj default_params();                            // dict key -> default
PyObj coerce(const std::string& key, const PyObj& value);
// backend_params(ui): the nvfilter v5 parameter dict (+ ui_channels, ui_spatial, t_noise) as Python builds it
PyObj backend_params(const PyObj& ui);
// main_window.assist_params(): Assist Image (Profile Check) parameters
PyObj assist_params();

// ---------------- profile ----------------
using Factors = std::array<std::array<double, 9>, 3>;   // state.CHANNELS order (Y, Cb, Cr) x 9 bins
Factors unit_factors();

struct ProfileState {
    PyObj base;                                    // none() = no profile
    Factors factors = unit_factors();
    std::array<std::set<int>, 3> pinned;
    std::string device = "Unspecified device";
    PyObj tune = PyObj::dict();                    // "all", "Y", "Cb", "Cr" (missing = 1.0)
    // "RGB": factors = the R / G / B bin levels (G1); "YCbCr": Y / Cb / Cr factors (files made before G1)
    std::string channels = "RGB";

    bool has_profile() const { return !base.is_none(); }
    double tune_value(const std::string& k) const;
    PyObj effective() const;                        // ProfileState.effective (by the channels)
    PyObj to_json() const;
    static ProfileState from_json(const PyObj& d);   // throws std::runtime_error as Python's ValueError
};

// state.effective_profile(base, factors, tune) (the curve through native nvc_effective_curve)
PyObj effective_profile(const PyObj& base, const Factors& factors, const PyObj& tune);
// state.auto_complete(factors, pinned)
Factors auto_complete(const Factors& factors, const std::array<std::set<int>, 3>& pinned);

// G1, the R/G/B equalizer (sandbox/omo/wp_qtui_fix/DESIGN_G1.md): rgb_ready / rgb_reference (nullopt =
// None) / rgb_effective_profile (curve and sigma through nvc_rgb_effective_curve) / auto_complete_ps /
// after_aft / reset_fine_tune
bool rgb_ready(const PyObj& base);
// check_rgb_keys: the keys the R/G/B equalizer reads, where present, in shape with finite numbers (throws
// std::runtime_error naming the key)
void check_rgb_keys(const PyObj& base);
std::optional<Factors> rgb_reference(const PyObj& base);
PyObj rgb_effective_profile(const PyObj& base, const Factors& levels, const PyObj& tune);
Factors auto_complete(const ProfileState& ps);
ProfileState after_aft(const ProfileState& ps, const PyObj& prof);
ProfileState reset_fine_tune(const ProfileState& ps);

PyObj preset_json(const PyObj& params);
PyObj params_from_json(const PyObj& d);           // throws std::runtime_error

// ---------------- app state + undo ----------------
class AppState;

class ParamsCommand : public QUndoCommand {
public:
    ParamsCommand(AppState* st, PyObj old_, PyObj new_, const QString& text, std::optional<std::string> merge_key);
    int id() const override { return merge_key ? 1 : -1; }
    bool mergeWith(const QUndoCommand* other) override;
    void redo() override;
    void undo() override;

private:
    AppState* st;
    PyObj old_v, new_v;
    std::optional<std::string> merge_key;
};

class ProfileCommand : public QUndoCommand {
public:
    ProfileCommand(AppState* st, ProfileState old_, ProfileState new_, const QString& text,
                   std::optional<std::string> merge_key);
    int id() const override { return merge_key ? 2 : -1; }
    bool mergeWith(const QUndoCommand* other) override;
    void redo() override;
    void undo() override;

private:
    AppState* st;
    ProfileState old_v, new_v;
    std::optional<std::string> merge_key;
};

class AppState : public QObject {
    Q_OBJECT
public:
    explicit AppState(QObject* parent = nullptr);

    PyObj params;
    ProfileState profile;
    QUndoStack* undo;

    void new_edit() { ++seq_; }
    void set_param(const std::string& key, const PyObj& value);
    void set_params(const std::vector<std::pair<std::string, PyObj>>& updates, const QString& text, bool merge = true);
    void set_all_params(const PyObj& params, const QString& text);
    PyObj backend() const { return backend_params(params); }

    void set_profile(const ProfileState& ps, const QString& text, std::optional<std::string> merge_key = std::nullopt);
    void set_factors(int c, const std::array<double, 9>& f, std::optional<int> pin);
    void set_factors_all(const std::array<double, 9>& f, int pin);
    void set_tune(const std::string& k, double value);
    const PyObj* effective();                      // nullptr without a profile

    // used by the undo commands
    void apply_params(const PyObj& p);
    void apply_profile(const ProfileState& ps);

signals:
    void paramsChanged(QStringList keys);
    void profileChanged();

private:
    int seq_ = 0;
    std::optional<PyObj> eff_;
};

}  // namespace nvqt
