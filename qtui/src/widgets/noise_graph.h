// Noise graph (port of repro/nv_ui_qt/widgets/noise_graph.py): brightness vs noise level of the profile
// (Y / Cb / Cr), drawn with QPainter; the base curve dashed when the effective profile differs.
#pragma once

#include <QWidget>

#include "pyobj.h"

namespace nvqt {

class NoiseGraph : public QWidget {
public:
    explicit NoiseGraph(QWidget* parent = nullptr);
    // prof: the effective profile (nullptr = none); base: the profile before tuning (dashed when not prof)
    void set_profile(const PyObj* prof, const PyObj* base = nullptr);

protected:
    void paintEvent(QPaintEvent* e) override;

private:
    PyObj prof_, base_;
    bool has_prof_ = false, has_base_ = false;
};

}  // namespace nvqt
