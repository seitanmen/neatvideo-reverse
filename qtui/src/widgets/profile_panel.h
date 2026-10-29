// Device Noise Profile side panel (Prepare Noise Profile page), port of repro/nv_ui_qt/widgets/profile_panel.py:
// the genuine order (title; icons; profile name; Device Name and Mode; the dark info box; Quality /
// Completeness / Noise Levels and the band bars; the equalizer with its button row; Profile Check and
// Tuning with the profile Noise Level rows).  Names follow the Python module.
#pragma once

#include <QComboBox>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QToolButton>
#include <QWidget>

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "curve_editor.h"
#include "rows.h"
#include "state.h"

namespace nvqt {

class BandBar : public QWidget {
public:
    explicit BandBar(QWidget* parent = nullptr);
    void set_value(std::optional<double> v);
    double value = 0.0;

protected:
    void paintEvent(QPaintEvent* e) override;
};

QIcon eq_icon(const std::string& kind, int size = 20);      // "aft", "manual", "complete"
// bar values High .. Ultra Low (the profile's Y band ratio relative to white noise, max = 1) or nullopt
std::optional<std::vector<double>> band_bars(const PyObj& base);
Spec tune_spec(const QString& label, const QString& c = QString());

class ProfilePanel : public QWidget {
    Q_OBJECT
public:
    explicit ProfilePanel(QWidget* parent = nullptr);

    void set_mode(bool advanced);
    void set_busy(bool busy);
    void set_check_active(bool on);
    void set_name(std::optional<QString> text, bool dirty = false);
    void show_profile(const ProfileState& ps, const PyObj* eff, std::optional<std::pair<int, int>> frame_info);
    void set_levels(std::optional<double> main, std::optional<double> sel);
    void set_quality(std::optional<int> pct);
    void set_completeness(std::optional<int> pct);

    QScrollArea* scroll;
    IconBar* icons;
    QComboBox* name;
    QToolButton* device_menu;
    QLineEdit* device;
    QLabel* info;
    QWidget* stats;
    QLabel *quality, *completeness, *levels_title, *l_main, *main_val, *l_sel, *sel_val;
    QFrame* levels_line;
    std::map<std::string, BandBar*> bars;
    std::map<std::string, QLabel*> band_samples;
    CurveEditor* curve;
    QToolButton *b_aft, *b_manual, *b_complete, *b_reset;
    QLabel* complete_lbl;
    Header* tuning;
    BeginRow* check;
    std::map<std::string, Row*> tune;             // "all", "Y", "Cr", "Cb"
    Header* tune_header;
    Header* freq_header;

signals:
    void autoFineTune();
    void autoComplete();
    void resetFineTune();
    void profileCheck(bool on);                   // Begin / Finish of Profile Check (Assist Image mode)
    void editDevice(QString name);
    void tuneEdited(QString key, double value);   // "all" / "Y" / "Cb" / "Cr", multiplier
    void editStarted();

private:
    QWidget* reg_(QWidget* w, const std::string& modes);
    Row* tune_row_(const std::string& k, const QString& label, const std::string& deco = "");
    std::vector<std::pair<QWidget*, std::string>> modes_;
};

}  // namespace nvqt
