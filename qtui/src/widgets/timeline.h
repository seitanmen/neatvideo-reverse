// Bottom row (port of repro/nv_ui_qt/widgets/timeline.py): frame thumbnails (current frame = blue frame, the
// temporal filter's window = orange, click = go to that frame) and, because the reproduction runs stand-alone
// without a host timeline, a thin timeline slider with the frame number (repro).
#pragma once

#include <QImage>
#include <QLabel>
#include <QSlider>
#include <QTimer>
#include <QWidget>

#include <functional>
#include <optional>
#include <utility>
#include <vector>

namespace nvqt {

constexpr int N_THUMBS = 11;

class ThumbStrip : public QWidget {
    Q_OBJECT
public:
    explicit ThumbStrip(QWidget* parent = nullptr);
    QSize sizeHint() const override;
    void set_state(int n, int cur, int radius);
    // [(frame index or nullopt, rect)] of the 11 thumbnails, the current frame in the middle
    std::vector<std::pair<std::optional<int>, QRectF>> slots_() const;
    std::optional<QColor> frame_color(int i) const;

    int n = 0, cur = 0, radius = 0;
    std::function<QImage(int)> provider;          // frame -> thumbnail (a null QImage = none)

signals:
    void clickedFrame(int i);

protected:
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
};

class Timeline : public QWidget {
    Q_OBJECT
public:
    explicit Timeline(QWidget* parent = nullptr);
    void set_count(int n, double fps = 24.0);
    int frame() const { return slider->value(); }
    void set_frame(int i, bool emit_ = false);
    void set_radius(int r);
    bool playing() const { return timer->isActive(); }
    void set_playing(bool on);

    QSlider* slider;
    QLabel* label;
    ThumbStrip* thumbs;
    QTimer* timer;
    int n = 0;
    double fps = 24.0;

signals:
    void frameChanged(int i);

private:
    void update_(int i);
    void seek_(int i);
};

}  // namespace nvqt
