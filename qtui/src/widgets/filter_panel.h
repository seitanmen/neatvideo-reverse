// Filter Settings side panel (Adjust and Preview page), port of repro/nv_ui_qt/widgets/filter_panel.py: preset
// buttons and name, tabs Temporal | Spatial | General with grey section headers; Luminance -> Cr -> Cb, High ->
// Ultra Low.  Advanced-only rows are hidden in Beginner Mode (values are kept).  Frequencies rows are common to
// the three components (one row writes bamt_{Y,Cr,Cb}_<band> / bnoise_... in one undo step); the per-component
// rows of the v5 backend stay in "Per-component bands (repro)".  Noise Level Check (set_nlc_active): while it
// runs the button reads Finish, its rows are yellow, the other sections of the Temporal tab and the Spatial /
// General tabs are disabled.  Names follow the Python module.
#pragma once

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QVariant>
#include <QVariantMap>
#include <QVBoxLayout>
#include <QWidget>

#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "rows.h"
#include "state.h"

namespace nvqt {

class FilterPanel : public QWidget {
    Q_OBJECT
public:
    explicit FilterPanel(QWidget* parent = nullptr);

    void set_mode(bool advanced);
    // set_band_availability(levels): levels = eff["levels"] (Y, Cb, Cr) or nullptr (no profile)
    void set_band_availability(const PyObj* levels);
    // set_values(params, keys): keys = the changed keys (an empty list = none); nullopt = every key
    void set_values(const PyObj& params, std::optional<QStringList> keys = std::nullopt);
    void set_performance(const QString& html);
    void set_nlc_active(bool on);

    std::map<std::string, QWidget*> controls;       // key -> primary widget (Row / EnableCheck / ChoiceRow)
    std::map<std::string, std::vector<QWidget*>> extra;
    std::vector<Row*> common;                        // Frequencies rows (several keys)
    std::map<std::string, Header*> headers;
    std::map<std::string, std::set<std::string>> used;   // set_band_availability: bands the backend uses per channel
    PyObj params;
    bool advanced = true;

    IconBar* icons;
    QComboBox* name;
    QTabWidget* tabs;
    QLabel *qsg, *mode_lbl;
    Header* t_noise_header;
    BeginRow* nl_check;
    BeginRow* assist;
    std::vector<Row*> sharp_rows;
    QLabel* perf;
    QPushButton *b_speed, *b_prefs;
    QLabel* speed;

    // the widget of a key as its concrete type (nullptr when the key has another kind)
    Row* row(const std::string& key) const;
    EnableCheck* check(const std::string& key) const;
    ChoiceRow* choice(const std::string& key) const;

signals:
    void edited(QString key, QVariant value);      // bool / double / QString as the control's kind
    void editedMany(QVariantMap values);
    void editStarted();
    void assistToggled(bool on);
    void noiseLevelCheck(bool on);
    void resetSpatial();
    void checkSpeed();
    void preferences();
    void linkActivated(QString href);

private:
    QVBoxLayout* page_(const QString& title);
    QWidget* reg_(QWidget* w, const std::string& modes = "both");
    QWidget* bind_(QWidget* w);
    Row* row_(QWidget* parent, const std::string& key, std::optional<QString> label = std::nullopt,
              const std::string& fmt = "amount", const std::string& deco = "", std::pair<int, int> steps = {5, 20},
              int indent = 0, const std::string& modes = "both");
    ChoiceRow* choice_(QWidget* parent, const std::string& key, const QString& label, const std::vector<QString>& names = {},
                       int indent = 0, const std::string& modes = "both");
    Header* header_(QWidget* parent, const QString& title, const std::string& enable = "", const std::string& modes = "both",
                    QWidget* inline_ = nullptr, bool collapsible = true, bool expanded = true, int level = 0,
                    bool reset = false, const QString& help_tip = QString());
    QWidget* check_row_(QWidget* parent, const std::string& key, const QString& label, int indent = 0,
                        const std::string& modes = "both");
    BeginRow* na_begin_(QWidget* parent, const QString& title, const std::string& modes = "adv");
    Header* freq_(QWidget* parent, const std::string& prefix, const std::string& fmt, std::pair<int, int> steps);
    void temporal_();
    void spatial_();
    void general_();
    void mix_enable_(const QString& k, bool on);
    static void add_to_(QWidget* parent, QWidget* w);

    std::vector<std::pair<QWidget*, std::string>> modes_;
    std::vector<QWidget*> nlc_hl_, nlc_off_, nlc_dis_;
    std::vector<std::tuple<QLabel*, QString, QString>> nlc_text_;
    bool nlc_on_ = false;
    std::map<std::string, QMetaObject::Connection> enable_conn_;   // the edited connection of an EnableCheck
    std::map<Row*, std::string> band_of_;                            // Frequencies row -> band
};

}  // namespace nvqt
