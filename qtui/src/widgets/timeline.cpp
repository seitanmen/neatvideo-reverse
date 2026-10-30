// timeline - see timeline.h (port of repro/nv_ui_qt/widgets/timeline.py).
#include "timeline.h"

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

#include <algorithm>

namespace nvqt {

ThumbStrip::ThumbStrip(QWidget* parent) : QWidget(parent) {
    setFixedHeight(40);
    setMinimumWidth(N_THUMBS * 30);
}

QSize ThumbStrip::sizeHint() const { return QSize(N_THUMBS * 60, 40); }

void ThumbStrip::set_state(int n_, int cur_, int radius_) {
    n = n_;
    cur = cur_;
    radius = radius_;
    update();
}

std::vector<std::pair<std::optional<int>, QRectF>> ThumbStrip::slots_() const {
    const double h = height() - 6;
    const double w = std::min(h * 16 / 9, (width() - 4) / static_cast<double>(N_THUMBS) - 6);
    const double x0 = 3.0;
    std::vector<std::pair<std::optional<int>, QRectF>> out;
    for (int k = 0; k < N_THUMBS; ++k) {
        const int i = cur - N_THUMBS / 2 + k;
        out.emplace_back(0 <= i && i < n ? std::optional<int>(i) : std::nullopt, QRectF(x0 + k * (w + 6), 3, w, h));
    }
    return out;
}

std::optional<QColor> ThumbStrip::frame_color(int i) const {
    if (i == cur) return QColor(40, 120, 230);
    if (std::abs(i - cur) <= radius) return QColor(240, 130, 40);
    return std::nullopt;
}

void ThumbStrip::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    for (const auto& [i, r] : slots_()) {
        if (!i) continue;
        const QImage img = provider ? provider(*i) : QImage();
        if (!img.isNull())
            p.drawImage(r, img);
        else
            p.fillRect(r, QColor(90, 90, 90));
        const auto col = frame_color(*i);
        if (col) {
            p.setPen(QPen(*col, 3));
            p.drawRect(r.adjusted(-1.5, -1.5, 1.5, 1.5));
        }
    }
    p.end();
}

void ThumbStrip::mousePressEvent(QMouseEvent* e) {
    for (const auto& [i, r] : slots_()) {
        if (i && r.adjusted(-3, -3, 3, 3).contains(e->position())) {
            emit clickedFrame(*i);
            return;
        }
    }
}

Timeline::Timeline(QWidget* parent) : QWidget(parent) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(1);
    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 0);
    slider->setFixedHeight(14);
    slider->setToolTip("Timeline (repro: the genuine plug-in follows the host's timeline)");
    label = new QLabel("Frame 0 / 0");
    label->setStyleSheet("color:#b4b4b4");      // light grey on the grey page (look.h), as the genuine dim labels
    label->setMinimumWidth(110);
    row->addWidget(slider, 1);
    row->addWidget(label);
    v->addLayout(row);
    thumbs = new ThumbStrip();
    v->addWidget(thumbs);
    connect(slider, &QSlider::valueChanged, this, [this](int i) { seek_(i); });
    connect(thumbs, &ThumbStrip::clickedFrame, this, [this](int i) { set_frame(i, true); });
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this] { set_frame((slider->value() + 1) % std::max(n, 1), true); });
}

void Timeline::set_count(int n_, double fps_) {
    n = n_;
    fps = fps_;
    slider->blockSignals(true);
    slider->setRange(0, std::max(n - 1, 0));
    slider->setValue(0);
    slider->blockSignals(false);
    update_(0);
}

void Timeline::set_frame(int i, bool emit_) {
    i = std::max(0, std::min(i, std::max(n - 1, 0)));
    slider->blockSignals(!emit_);
    slider->setValue(i);
    slider->blockSignals(false);
    update_(i);
}

void Timeline::set_radius(int r) {
    thumbs->radius = r;
    thumbs->update();
}

void Timeline::update_(int i) {
    label->setText(QString("Frame %1 / %2").arg(i).arg(n));
    thumbs->set_state(n, i, thumbs->radius);
}

void Timeline::seek_(int i) {
    update_(i);
    emit frameChanged(i);
}

void Timeline::set_playing(bool on) {
    if (on)
        timer->start(static_cast<int>(1000 / std::max(fps, 1.0)));
    else
        timer->stop();
}

}  // namespace nvqt
