// Noise profile equalizer (Manual Fine-Tune), port of repro/nv_ui_qt/widgets/curve_editor.py: the
// per-brightness noise factors of the three components drawn together (Luminance white, Cr red, Cb purple),
// vertical axis (factor - 1) x 100% from -100% to +200%, knots as small yellow squares (edited ones filled
// with the component's colour), the knot of the profile's sample-brightness bin larger; round toggles choose
// the edit target (all / Y / Cr / Cb).
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

#include "state.h"

namespace nvqt {

constexpr double D_MIN = -100.0, D_MAX = 200.0;

QIcon all_icon(int size = 18);

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
    int channel() const;                         // state.CHANNELS index of the target (all -> 0)
    void set_factors(const Factors& factors, const std::array<std::set<int>, 3>& pinned,
                     std::optional<int> main_bin = std::nullopt);
    void move_knot(const std::string& c, int i, double f);

    Factors factors = unit_factors();
    std::array<std::set<int>, 3> pinned;
    std::string target = "all";
    std::optional<int> main_bin;                 // bin of the profile's sample brightness (larger knot)
    EqualizerPlot* plot;
    QButtonGroup* group;
    std::map<std::string, QToolButton*> target_buttons;
    QHBoxLayout* buttons;

signals:
    void edited(int c, std::array<double, 9> factors, int bin);   // channel index (state.CHANNELS)
    void editedAll(std::array<double, 9> factors, int bin);       // same bin edit on all components
    void editStarted();
};

}  // namespace nvqt
