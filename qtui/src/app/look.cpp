// look - the application look (look.h)
#include "look.h"

#include <QApplication>
#include <QPainter>
#include <QPalette>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOptionComplex>
#include <QWidget>

#include <algorithm>

namespace nvqt {

namespace {

// a trailing " ▾" of a button text (the menu mark of the labels): removed, true if there was one
bool strip_arrow(QString& t) {
    static const QString mark = QString::fromUtf8("▾");
    if (!t.endsWith(mark)) return false;
    t.chop(mark.size());
    while (t.endsWith(' ')) t.chop(1);
    return true;
}

class LookStyle : public QProxyStyle {
public:
    LookStyle() : QProxyStyle(QStyleFactory::create("Fusion")) {}

    int styleHint(StyleHint h, const QStyleOption* opt, const QWidget* w, QStyleHintReturn* ret) const override {
        if (h == SH_UnderlineShortcut) return 0;     // genuine (macOS): no mnemonic underlines in the menu bar
        return QProxyStyle::styleHint(h, opt, w, ret);
    }

    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex* opt, QPainter* p,
                            const QWidget* w) const override {
        if (cc == CC_ToolButton) {
            if (const auto* tb = qstyleoption_cast<const QStyleOptionToolButton*>(opt)) {
                QStyleOptionToolButton o = *tb;
                bool arrow = (o.features & (QStyleOptionToolButton::HasMenu | QStyleOptionToolButton::MenuButtonPopup)) != 0;
                if (strip_arrow(o.text)) arrow = true;
                if (w && w->property("nv_arrow").isValid() && !w->property("nv_arrow").toBool()) arrow = false;
                if (o.features & QStyleOptionToolButton::MenuButtonPopup) {
                    // one bevel over the whole button (the menu part is only a hit area, subControlRect below)
                    o.features &= ~QStyleOptionToolButton::MenuButtonPopup;
                    o.subControls &= ~SC_ToolButtonMenu;
                    if (o.activeSubControls & SC_ToolButtonMenu) o.activeSubControls |= SC_ToolButton;
                    o.activeSubControls &= ~SC_ToolButtonMenu;
                }
                if (arrow)
                    o.features |= QStyleOptionToolButton::HasMenu;
                else
                    o.features &= ~QStyleOptionToolButton::HasMenu;
                QProxyStyle::drawComplexControl(cc, &o, p, w);
                return;
            }
        }
        if (cc == CC_Slider) {
            const auto* sl = qstyleoption_cast<const QStyleOptionSlider*>(opt);
            if (sl && sl->orientation == Qt::Horizontal) {
                // genuine: a light flat groove with a dark top/left edge and a flat light grey handle (no highlight)
                const bool on = sl->state & State_Enabled;
                p->save();
                p->setRenderHint(QPainter::Antialiasing, false);
                if (sl->subControls & SC_SliderGroove) {
                    const QRect g = subControlRect(cc, sl, SC_SliderGroove, w);
                    const int gh = std::min(8, std::max(4, sl->rect.height() - 6));
                    const QRect r(g.left(), sl->rect.center().y() - gh / 2, g.width(), gh);
                    p->fillRect(r, on ? QColor(115, 115, 115) : QColor(100, 100, 100));
                    p->setPen(QColor(57, 57, 57));
                    p->drawLine(r.topLeft(), r.topRight());
                    p->drawLine(r.topLeft(), r.bottomLeft());
                }
                if (sl->subControls & SC_SliderTickmarks) {
                    QStyleOptionSlider o = *sl;
                    o.subControls = SC_SliderTickmarks;
                    QProxyStyle::drawComplexControl(cc, &o, p, w);
                }
                if (sl->subControls & SC_SliderHandle) {
                    const QRect h = subControlRect(cc, sl, SC_SliderHandle, w);
                    const int hh = std::min(15, sl->rect.height() - 1), hw = 10;
                    const QRectF r(h.center().x() - hw / 2.0 + 0.5, sl->rect.center().y() - hh / 2.0 + 0.5, hw - 1, hh - 1);
                    p->setRenderHint(QPainter::Antialiasing, true);
                    p->setPen(QColor(57, 57, 57));
                    p->setBrush(!on ? QColor(120, 120, 120)
                                : (sl->state & State_Sunken) ? QColor(175, 175, 175) : QColor(155, 155, 155));
                    p->drawRoundedRect(r, 2.5, 2.5);
                }
                p->restore();
                return;
            }
        }
        if (cc == CC_ComboBox) {
            const auto* cb = qstyleoption_cast<const QStyleOptionComboBox*>(opt);
            if (cb && !cb->editable) {
                // genuine: a flat sunken field with one white ▼ at the right (the label is CE_ComboBoxLabel)
                const bool on = cb->state & State_Enabled;
                const QRect r = cb->rect.adjusted(0, 0, -1, -1);
                p->save();
                p->fillRect(r, cb->palette.color(QPalette::Window));
                p->setPen(QColor(53, 53, 53));
                p->drawLine(r.topLeft(), r.topRight());
                p->drawLine(r.topLeft(), r.bottomLeft());
                p->setPen(QColor(110, 110, 110));
                p->drawLine(r.bottomLeft(), r.bottomRight());
                p->drawLine(r.topRight(), r.bottomRight());
                if (cb->state & State_HasFocus) {
                    p->setPen(cb->palette.color(QPalette::Highlight));
                    p->drawRect(r.adjusted(1, 1, -1, -1));
                }
                const QRect a = subControlRect(cc, cb, SC_ComboBoxArrow, w);
                const QPointF c = QRectF(a).center();
                p->setRenderHint(QPainter::Antialiasing, true);
                p->setPen(Qt::NoPen);
                p->setBrush(on ? QColor(240, 240, 240) : QColor(140, 140, 140));
                p->drawPolygon(QPolygonF({QPointF(c.x() - 4.5, c.y() - 2.5), QPointF(c.x() + 4.5, c.y() - 2.5),
                                          QPointF(c.x(), c.y() + 2.5)}));
                p->restore();
                return;
            }
        }
        QProxyStyle::drawComplexControl(cc, opt, p, w);
    }
};

QPalette look_palette() {
    QPalette pal;
    const QColor text(232, 232, 232), dis(140, 140, 140);
    pal.setColor(QPalette::Window, QColor(89, 89, 89));
    pal.setColor(QPalette::WindowText, text);
    pal.setColor(QPalette::Base, QColor(58, 58, 58));
    pal.setColor(QPalette::AlternateBase, QColor(68, 68, 68));
    pal.setColor(QPalette::Text, text);
    pal.setColor(QPalette::PlaceholderText, QColor(160, 160, 160));
    pal.setColor(QPalette::Button, QColor(100, 100, 100));     // Fusion: 124% .. 102% gradient = genuine 119 .. 100
    pal.setColor(QPalette::ButtonText, QColor(240, 240, 240));
    pal.setColor(QPalette::BrightText, Qt::white);
    pal.setColor(QPalette::Light, QColor(130, 130, 130));
    pal.setColor(QPalette::Midlight, QColor(112, 112, 112));
    pal.setColor(QPalette::Mid, QColor(72, 72, 72));
    pal.setColor(QPalette::Dark, QColor(53, 53, 53));
    pal.setColor(QPalette::Shadow, QColor(30, 30, 30));
    pal.setColor(QPalette::Highlight, QColor(52, 120, 220));
    pal.setColor(QPalette::HighlightedText, Qt::white);
    pal.setColor(QPalette::Link, QColor(110, 168, 255));
    pal.setColor(QPalette::LinkVisited, QColor(110, 168, 255));
    pal.setColor(QPalette::ToolTipBase, QColor(240, 240, 225));
    pal.setColor(QPalette::ToolTipText, QColor(20, 20, 20));
    for (QPalette::ColorRole r : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
        pal.setColor(QPalette::Disabled, r, dis);
    pal.setColor(QPalette::Disabled, QPalette::Button, QColor(92, 92, 92));
    pal.setColor(QPalette::Disabled, QPalette::Highlight, QColor(110, 110, 110));
    return pal;
}

}  // namespace

void apply_look(QApplication& app) {
    QApplication::setStyle(new LookStyle());
    app.setPalette(look_palette());
    // the macOS theme gives some classes the small system font (tool buttons 11 pt); genuine: one font everywhere
    const QFont f = QApplication::font();
    for (const char* cls : {"QToolButton", "QPushButton", "QComboBox", "QTabBar", "QMenuBar", "QMenu", "QLabel",
                            "QLineEdit", "QCheckBox", "QRadioButton", "QAbstractSpinBox", "QGroupBox"})
        QApplication::setFont(f, cls);
}

}  // namespace nvqt
