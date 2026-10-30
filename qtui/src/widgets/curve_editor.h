// Noise profile equalizer (Manual Fine-Tune), port of repro/nv_ui_qt/widgets/curve_editor.py: the
// per-brightness noise levels of the three channels drawn together, vertical axis (level - 1) x 100% from -100% to
// +200%, knots as small yellow squares (edited ones filled with the channel's colour), the knot of the profile's
// sample-brightness bin larger; round toggles choose the edit target (all, then one per channel).  An R/G/B
// profile (G1) shows the R / G / B levels (toggles RGB / R / G / B); a Y / Cb / Cr profile of an older file the
// former Y / Cr / Cb factors.
#pragma once

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QIcon>
#include <QToolButton>
#include <QWidget>

#include <array>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "state.h"

namespace nvqt {

constexpr double D_MIN = -100.0, D_MAX = 200.0;

QIcon all_icon(int size = 18, const std::vector<std::string>& keys = {"Y", "Cr", "Cb"});

class CurveEditor;

class EqualizerPlot : public QWidget {
public:
    explicit EqualizerPlot(CurveEditor* owner);
    QRectF plot_rect() const;
    QPointF pt(int i, double f) const;
    double factor_at(double y) const;

protected:
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    struct Hit { double d; std::string c; int i; };
    std::optional<Hit> hit_(QPointF pos) const;
    CurveEditor* o;
    std::optional<std::pair<std::string, int>> drag_;
};

class CurveEditor : public QWidget {
    Q_OBJECT
public:
    explicit CurveEditor(QWidget* parent = nullptr);
    void set_target(const std::string& key);
    int channel() const;                         // factor index of the target (all -> 0)
    void set_factors(const Factors& factors, const std::array<std::set<int>, 3>& pinned,
                     std::optional<int> main_bin = std::nullopt, const std::string& channels = "RGB");
    void move_knot(const std::string& c, int i, double f);
    // the channels in drawing / toggle order with their index in ProfileState::factors
    const std::vector<std::pair<std::string, int>>& order() const;
    int index(const std::string& c) const;

    Factors factors = unit_factors();
    std::array<std::set<int>, 3> pinned;
    std::string channels = "RGB";
    std::string target = "all";
    std::optional<int> main_bin;                 // bin of the profile's sample brightness (larger knot)
    EqualizerPlot* plot;
    QButtonGroup* group;
    std::map<std::string, QToolButton*> target_buttons;
    QHBoxLayout* buttons;

private:
    void label_buttons();
    std::vector<QToolButton*> buttons_;

signals:
    void edited(int c, std::array<double, 9> factors, int bin);   // factor index (R/G/B or state.CHANNELS)
    void editedAll(std::array<double, 9> factors, int bin);       // same bin edit on all components
    void editStarted();
};

}  // namespace nvqt
