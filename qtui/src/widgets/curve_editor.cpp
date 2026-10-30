// curve_editor - see curve_editor.h (port of repro/nv_ui_qt/widgets/curve_editor.py).
#include "curve_editor.h"

#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QSizePolicy>
#include <QVBoxLayout>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "rows.h"

namespace nvqt {

namespace {
const QColor PANEL_GREY(89, 89, 89);           // genuine panel / plot background (g0.png)

// Python round(x, 3): the correctly rounded decimal (round half to even on the exact value), back to a float
double py_round3(double x) {
    char b[64];
    std::snprintf(b, sizeof(b), "%.3f", x);
    return std::strtod(b, nullptr);
}
}  // namespace

QIcon all_icon(int size, const std::vector<std::string>& keys) {
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r(1.5, 1.5, size - 3, size - 3);
    for (int k = 0; k < 3; ++k) {
        p.setPen(Qt::NoPen);
        p.setBrush(ch_color(keys[static_cast<size_t>(k)]));
        p.drawPie(r, (90 + 120 * k) * 16, 120 * 16);
    }
    p.setPen(QPen(QColor(60, 60, 60), 1));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(r);
    p.end();
    return QIcon(pm);
}

// ------------------------------------------------------------------ EqualizerPlot
EqualizerPlot::EqualizerPlot(CurveEditor* owner) : QWidget(owner), o(owner) {
    setMinimumHeight(170);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QRectF EqualizerPlot::plot_rect() const { return QRectF(rect()).adjusted(48, 8, -8, -20); }

QPointF EqualizerPlot::pt(int i, double f) const {
    const QRectF r = plot_rect();
    const double d = (f - 1.0) * 100.0;
    return QPointF(r.left() + r.width() * CURVE_X[i] / 256.0, r.top() + r.height() * (D_MAX - d) / (D_MAX - D_MIN));
}

double EqualizerPlot::factor_at(double y) const {
    const QRectF r = plot_rect();
    const double d = D_MAX - (y - r.top()) / r.height() * (D_MAX - D_MIN);
    return std::min(std::max(1.0 + d / 100.0, FACTOR_MIN), FACTOR_MAX);
}

void EqualizerPlot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), PANEL_GREY);
    const QRectF r = plot_rect();
    p.setPen(QPen(QColor(190, 190, 190), 1, Qt::DotLine));
    for (int i = 0; i < 9; ++i) {
        const double x = pt(i, 1.0).x();
        p.drawLine(QPointF(x, r.top()), QPointF(x, r.bottom()));
    }
    for (int d = 200; d >= -100; d -= 50) {
        const double y = r.top() + r.height() * (D_MAX - d) / (D_MAX - D_MIN);
        p.setPen(QPen(QColor(190, 190, 190), 1, Qt::DotLine));
        p.drawLine(QPointF(r.left(), y), QPointF(r.right(), y));
        if (d % 100 == 0) {
            p.setPen(QColor(240, 240, 240));
            p.drawText(QRectF(0, y - 7, 45, 14), Qt::AlignRight | Qt::AlignVCenter,
                       QString::asprintf("%+d%%", d));
        }
    }
    QLinearGradient g(r.left(), 0, r.right(), 0);
    g.setColorAt(0, QColor(0, 0, 0));
    g.setColorAt(1, QColor(255, 255, 255));
    const QRectF bar(r.left(), r.bottom() + 5, r.width(), 8);
    p.fillRect(bar, QBrush(g));
    p.setPen(QPen(QColor(40, 40, 40), 1));
    p.drawRect(bar);
    const std::string tgt = o->target;
    std::vector<std::string> order;
    for (const auto& kl : o->order())
        if (kl.first != tgt) order.push_back(kl.first);
    for (const auto& kl : o->order())
        if (kl.first == tgt) order.push_back(kl.first);
    for (const auto& c : order) {
        const auto& fs = o->factors[o->index(c)];
        std::vector<QPointF> pts;
        for (int i = 0; i < 9; ++i) pts.push_back(pt(i, fs[i]));
        const bool active = tgt == "all" || tgt == c;
        p.setPen(QPen(ch_color(c), active ? 2 : 1));
        for (size_t k = 0; k + 1 < pts.size(); ++k) p.drawLine(pts[k], pts[k + 1]);
        for (int i = 0; i < 9; ++i) {
            const QPointF q = pts[static_cast<size_t>(i)];
            const bool pinned = o->pinned[o->index(c)].count(i) > 0;
            p.setPen(QPen(QColor(20, 20, 20), 1));
            p.setBrush(QBrush(pinned ? ch_color(c) : QColor(250, 220, 0)));
            const double s = (active ? 6 : 5) + (o->main_bin && i == *o->main_bin ? 4 : 0);
            p.drawRect(QRectF(q.x() - s / 2, q.y() - s / 2, s, s));
        }
    }
    p.end();
}

std::optional<EqualizerPlot::Hit> EqualizerPlot::hit_(QPointF pos) const {
    std::vector<std::string> chans;
    if (o->target == "all") {
        for (const auto& kl : o->order()) chans.push_back(kl.first);
    } else {
        chans.push_back(o->target);
    }
    std::optional<Hit> best;
    for (const auto& c : chans) {
        const auto& fs = o->factors[o->index(c)];
        for (int i = 0; i < 9; ++i) {
            const QPointF q = pt(i, fs[i]);
            const double d = std::fabs(q.x() - pos.x()) + std::fabs(q.y() - pos.y());
            if (d < 10 && (!best || d < best->d)) best = Hit{d, c, i};
        }
    }
    return best;
}

void EqualizerPlot::mousePressEvent(QMouseEvent* e) {
    const auto h = hit_(e->position());
    if (h && isEnabled()) {
        drag_ = std::make_pair(h->c, h->i);
        emit o->editStarted();
    }
}

void EqualizerPlot::mouseMoveEvent(QMouseEvent* e) {
    if (drag_) o->move_knot(drag_->first, drag_->second, py_round3(factor_at(e->position().y())));
}

void EqualizerPlot::mouseReleaseEvent(QMouseEvent*) { drag_.reset(); }

// ------------------------------------------------------------------ CurveEditor
namespace {
const std::vector<std::pair<std::string, int>> ORDER_RGB = {{"R", 0}, {"G", 1}, {"B", 2}};
const std::vector<std::pair<std::string, int>> ORDER_YCC = {{"Y", 0}, {"Cr", 2}, {"Cb", 1}};   // factors Y, Cb, Cr
const std::map<std::string, QString> NAMES = {{"Y", "Luminance (Y)"}, {"Cr", "Cr"}, {"Cb", "Cb"},
                                              {"R", "Red"}, {"G", "Green"}, {"B", "Blue"}};
}  // namespace

CurveEditor::CurveEditor(QWidget* parent) : QWidget(parent) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    plot = new EqualizerPlot(this);
    v->addWidget(plot);
    auto* row = new QHBoxLayout();
    row->setSpacing(2);
    group = new QButtonGroup(this);
    for (int n = 0; n < 4; ++n) {
        auto* b = new QToolButton();
        b->setIconSize(QSize(18, 18));
        b->setToolButtonStyle(Qt::ToolButtonIconOnly);
        b->setFixedSize(26, 24);
        b->setCheckable(true);
        b->setStyleSheet("QToolButton{border:1px solid #3a3a3a; border-radius:3px; background:#6a6a6a}"
                         "QToolButton:checked{background:#6f6fd8}");
        group->addButton(b);
        connect(b, &QToolButton::clicked, this, [this, b] { set_target(b->property("key").toString().toStdString()); });
        row->addWidget(b);
        buttons_.push_back(b);
    }
    label_buttons();
    target_buttons["all"]->setChecked(true);
    buttons = row;
    v->addLayout(row);
}

const std::vector<std::pair<std::string, int>>& CurveEditor::order() const {
    return channels == "RGB" ? ORDER_RGB : ORDER_YCC;
}

int CurveEditor::index(const std::string& c) const {
    for (const auto& [k, i] : order())
        if (k == c) return i;
    return 0;
}

void CurveEditor::label_buttons() {
    const bool rgb = channels == "RGB";
    std::vector<std::string> keys;
    for (const auto& kl : order()) keys.push_back(kl.first);
    target_buttons.clear();
    for (int n = 0; n < 4; ++n) {
        const std::string key = n == 0 ? std::string("all") : keys[static_cast<size_t>(n - 1)];
        QToolButton* b = buttons_[static_cast<size_t>(n)];
        b->setProperty("key", QString::fromStdString(key));
        b->setText(key == "all" ? (rgb ? "RGB" : "All") : QString::fromStdString(key));
        b->setIcon(key == "all" ? all_icon(18, keys) : QIcon(dot_pixmap(key, 18)));
        if (rgb)
            b->setToolTip(key == "all" ? QString("Show the noise graphs (noise level vs brightness level) for all three "
                                                 "color channels")
                                       : QString("Show the noise graph (noise level vs brightness level) for the %1 "
                                                 "channel alone").arg(NAMES.at(key)));
        else
            b->setToolTip(key == "all"
                              ? QString("Show the noise graphs (noise level vs brightness level) for all three color "
                                        "channels\n(repro: Luminance, Cr and Cb of a profile made before the R/G/B "
                                        "equalizer; the genuine's channels are R, G, B)")
                              : QString("Show the noise graph (noise level vs brightness level) for the %1 channel "
                                        "alone\n(repro: a profile made before the R/G/B equalizer; the genuine's "
                                        "buttons are Red, Green, Blue)").arg(NAMES.at(key)));
        target_buttons[key] = b;
    }
}

void CurveEditor::set_target(const std::string& key) {
    target = key;
    target_buttons[key]->setChecked(true);
    plot->update();
}

int CurveEditor::channel() const { return target == "all" ? 0 : index(target); }

void CurveEditor::set_factors(const Factors& f, const std::array<std::set<int>, 3>& p, std::optional<int> mb,
                              const std::string& ch) {
    if (ch != channels) {
        channels = ch;
        label_buttons();
        set_target("all");
    }
    factors = f;
    pinned = p;
    main_bin = mb;
    plot->update();
}

void CurveEditor::move_knot(const std::string& c, int i, double f) {
    f = std::min(std::max(f, FACTOR_MIN), FACTOR_MAX);
    std::array<double, 9> fs = factors[index(c)];
    fs[i] = f;
    if (target == "all")
        emit editedAll(fs, i);
    else
        emit edited(index(c), fs, i);
}

}  // namespace nvqt
