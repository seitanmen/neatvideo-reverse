// noise_graph - see noise_graph.h (port of repro/nv_ui_qt/widgets/noise_graph.py).
#include "noise_graph.h"

#include <QPainter>
#include <QPainterPath>

#include <algorithm>
#include <cstdio>
#include <utility>
#include <vector>

namespace nvqt {

namespace {
// COLORS = {"Y": .., "Cr": .., "Cb": ..} (genuine dots), in that order
const std::vector<std::pair<std::string, QColor>>& colors() {
    static const std::vector<std::pair<std::string, QColor>> C = {
        {"Y", QColor(230, 230, 230)}, {"Cr", QColor(236, 60, 60)}, {"Cb", QColor(150, 90, 240)}};
    return C;
}
}  // namespace

NoiseGraph::NoiseGraph(QWidget* parent) : QWidget(parent) { setMinimumSize(260, 150); }

void NoiseGraph::set_profile(const PyObj* prof, const PyObj* base) {
    has_prof_ = prof != nullptr && !prof->is_none() && prof->size() > 0;   // Python: `if not self.prof`
    prof_ = has_prof_ ? *prof : PyObj();
    // Python draws the base dashed when `base is not prof`; an effective profile equal to its base is the
    // base object itself there (state.effective_profile returns it unchanged)
    has_base_ = has_prof_ && base != nullptr && !base->is_none() && base != prof && !(*base == *prof);
    base_ = has_base_ ? *base : PyObj();
    update();
}

void NoiseGraph::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QRectF r = QRectF(rect()).adjusted(34, 8, -8, -20);
    p.fillRect(rect(), QColor(32, 32, 32));
    p.setPen(QColor(90, 90, 90));
    p.drawRect(r);
    if (!has_prof_) {
        p.setPen(QColor(160, 160, 160));
        p.drawText(r, Qt::AlignCenter, "no profile");
        p.end();
        return;
    }
    const std::vector<double> xs = prof_.get("curve_centers").as_doubles();
    std::vector<const PyObj*> curves{&prof_.get("curve")};
    if (has_base_) curves.push_back(&base_.get("curve"));
    double vmax = 0.0;
    bool first = true;
    for (const PyObj* c : curves) {
        double m = 0.0;
        bool f2 = true;
        for (const auto& [n, col] : colors()) {
            for (double v : c->get(n).as_doubles()) {
                if (f2 || v > m) m = v;
                f2 = false;
            }
        }
        if (first || m > vmax) vmax = m;
        first = false;
    }
    vmax *= 1.1;
    if (vmax == 0.0) vmax = 1.0;                  // `... * 1.1 or 1.0`
    auto pt = [&](double x, double y) {
        return QPointF(r.left() + r.width() * x / 255.0, r.bottom() - r.height() * y / vmax);
    };
    p.setPen(QColor(150, 150, 150));
    for (int k = 0; k < 5; ++k) {
        const double y = vmax * k / 4;
        const QPointF q = pt(0, y);
        char b[64];
        std::snprintf(b, sizeof(b), "%.1f", y);
        p.drawText(QRectF(0, q.y() - 7, 32, 14), Qt::AlignRight | Qt::AlignVCenter, QString::fromUtf8(b));
    }
    for (int x : {0, 64, 128, 192, 255}) {
        const QPointF q = pt(x, 0);
        p.drawText(QRectF(q.x() - 16, r.bottom() + 2, 32, 16), Qt::AlignCenter, QString::number(x));
    }
    for (size_t i = 0; i < curves.size(); ++i) {
        for (const auto& [n, col] : colors()) {
            QPainterPath path;
            const std::vector<double> ys = curves[i]->get(n).as_doubles();
            for (size_t j = 0; j < xs.size() && j < ys.size(); ++j) {
                if (j == 0)
                    path.moveTo(pt(xs[j], ys[j]));
                else
                    path.lineTo(pt(xs[j], ys[j]));
            }
            p.setPen(QPen(col, i == 0 ? 2 : 1, i == 0 ? Qt::SolidLine : Qt::DashLine));
            p.drawPath(path);
        }
    }
    p.setPen(QColor(200, 200, 200));
    p.drawText(r.adjusted(4, 2, -4, -2), Qt::AlignTop | Qt::AlignRight,
               QString("Y / Cr / Cb noise sigma vs brightness") +
                   (curves.size() > 1 ? "  (dashed: before tuning)" : ""));
    p.end();
}

}  // namespace nvqt
