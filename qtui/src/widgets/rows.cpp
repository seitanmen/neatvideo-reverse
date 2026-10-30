// rows - see rows.h (port of repro/nv_ui_qt/widgets/rows.py).
#include "rows.h"

#include <QAbstractSlider>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPolygonF>
#include <QRadialGradient>
#include <QSizePolicy>
#include <QStyle>

#include <cmath>
#include <cstdio>

namespace nvqt {

namespace {
#include "band_pixmaps.inc"

QString fmtc(const char* f, double d) {
    char b[64];
    std::snprintf(b, sizeof(b), f, d);
    return QString::fromUtf8(b);
}
}  // namespace

QColor ch_color(const std::string& c) {
    if (c == "Y") return QColor(255, 255, 255);
    if (c == "Cr" || c == "R") return QColor(236, 60, 60);
    if (c == "Cb") return QColor(150, 90, 240);
    if (c == "G") return QColor(60, 190, 60);
    return QColor(60, 90, 236);                      // B
}

int band_scale(const std::string& band) {
    if (band == "high") return 1;
    if (band == "medium") return 2;
    if (band == "low") return 4;
    if (band == "very-low") return 8;
    return 16;
}

const char* HEADER_CSS = "background:#6e6e6e;";       // genuine section band (110, 110, 110), rp_freq.png
const char* HEADER_TEXT = "#e9e9e9";                  // its bold title (233, 233, 233)
const char* ACTIVE_TITLE = "#f0d000";                 // genuine: title of a running check tool in yellow

// ------------------------------------------------------------------ FORMATS
namespace {
double ident(double v) { return v; }
double rel_to(double v) { return v - 100.0; }
double rel_from(double d) { return d + 100.0; }
double inv(double v) { return 100.0 - v; }
double tune_to(double v) { return (v - 1.0) * 100.0; }
double tune_from(double d) { return 1.0 + d / 100.0; }
QString t_amount(double d) { return std::nearbyint(d) == 0 ? QString("off") : fmtc("%.0f%%", d); }
QString t_rel(double d) { return fmtc("%+.0f%%", d); }
QString t_pct(double d) { return fmtc("%.0f%%", d); }
QString t_frames(double v) { return QString::number(static_cast<long long>(std::nearbyint(v))); }
QString t_float(double d) { return fmtc("%.2f", d); }
QString t_range(double d) { return fmtc("%+.2f", d); }
}  // namespace

const Format& format(const std::string& fmt) {
    static const Format AMOUNT{ident, ident, t_amount, 1}, REL{rel_to, rel_from, t_rel, 1}, INV{inv, inv, t_pct, 1},
        TUNE{tune_to, tune_from, t_rel, 1}, FRAMES{ident, ident, t_frames, 1}, FLOAT{ident, ident, t_float, 100},
        RANGE{ident, ident, t_range, 100};
    if (fmt == "rel") return REL;
    if (fmt == "inv") return INV;
    if (fmt == "tune") return TUNE;
    if (fmt == "frames") return FRAMES;
    if (fmt == "float") return FLOAT;
    if (fmt == "range") return RANGE;
    return AMOUNT;
}

// ------------------------------------------------------------------ icons
QPixmap dot_pixmap(const std::string& c, int size) {
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(90, 90, 90), 1));
    QRadialGradient g(QPointF(size * 0.35, size * 0.3), size * 0.75);
    g.setColorAt(0, QColor(255, 255, 255));
    g.setColorAt(1, c == "Y" ? ch_color(c).lighter(115) : ch_color(c));
    p.setBrush(QBrush(g));
    p.drawEllipse(QRectF(1.5, 1.5, size - 3, size - 3));
    p.end();
    return pm;
}

QIcon glyph_icon(const QString& text, QColor color, int size, bool bold) {
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    QFont f = p.font();
    f.setPixelSize(static_cast<int>(size * 0.8));
    f.setBold(bold);
    p.setFont(f);
    p.setPen(color);
    p.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, text);
    p.end();
    return QIcon(pm);
}

QIcon info_icon(int size) {
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(20, 60, 140), 1));
    p.setBrush(QColor(40, 120, 230));
    p.drawEllipse(QRectF(1.5, 1.5, size - 3, size - 3));
    QFont f = p.font();
    f.setPixelSize(static_cast<int>(size * 0.7));
    f.setBold(true);
    p.setFont(f);
    p.setPen(QColor(255, 255, 255));
    p.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, "i");
    p.end();
    return QIcon(pm);
}

QIcon pin_icon(int size) {
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(200, 200, 200), 1.6));
    p.drawLine(QPointF(size * 0.22, size * 0.82), QPointF(size * 0.48, size * 0.52));
    p.setPen(QPen(QColor(20, 60, 140), 1));
    p.setBrush(QColor(60, 140, 240));
    p.drawPolygon(QPolygonF({QPointF(size * 0.45, size * 0.25), QPointF(size * 0.78, size * 0.58),
                             QPointF(size * 0.62, size * 0.62), QPointF(size * 0.40, size * 0.40)}));
    p.drawEllipse(QRectF(size * 0.50, size * 0.12, size * 0.36, size * 0.36));
    p.end();
    return QIcon(pm);
}

QIcon square_icon(QColor color, int size) {
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setPen(QPen(QColor(20, 40, 90), 1));
    p.setBrush(color);
    p.drawRect(QRectF(1.5, 1.5, size - 3, size - 3));
    p.end();
    return QIcon(pm);
}

QIcon triangle_icon(QColor color, int size) {
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawPolygon(QPolygonF({QPointF(3, 1.5), QPointF(size - 2, size / 2.0), QPointF(3, size - 1.5)}));
    p.end();
    return QIcon(pm);
}

QPixmap band_pixmap(const std::string& band, int w, int h) {
    static std::map<std::string, QPixmap> cache;
    auto it = cache.find(band);
    if (it != cache.end()) return it->second;
    const unsigned char* d = band == "high" ? BAND_PM_high : band == "medium" ? BAND_PM_medium
                             : band == "low" ? BAND_PM_low : band == "very-low" ? BAND_PM_very_low
                                                                                : BAND_PM_ultra_low;
    // the embedded pictures are 22 x 16 (rows.band_pixmap's defaults, the only size the UI uses)
    QImage img(d, 22, 16, 22, QImage::Format_Grayscale8);
    QPixmap pm = QPixmap::fromImage(img.copy());
    if (w != 22 || h != 16) pm = pm.scaled(w, h);
    cache[band] = pm;
    return pm;
}

// ------------------------------------------------------------------ ValueLabel
ValueLabel::ValueLabel(QWidget* parent) : QLabel(parent) {
    setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    setCursor(Qt::PointingHandCursor);
}

void ValueLabel::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && isEnabled()) emit clicked();
}

void ValueLabel::paintEvent(QPaintEvent* e) {
    QLabel::paintEvent(e);
    QPainter p(this);
    p.setPen(QPen(palette().windowText().color(), 1, Qt::DotLine));
    QFontMetrics fm = fontMetrics();
    const int w = fm.horizontalAdvance(text());
    const int y = height() / 2 + fm.ascent() / 2 + 2;
    p.drawLine(width() - w, y, width() - 1, y);
    p.end();
}

// ------------------------------------------------------------------ Row
Row::Row(std::vector<std::string> keys_, std::optional<QString> label_, const std::string& fmt, const std::string& deco_,
         std::pair<int, int> steps, int indent, const Spec* spec_, std::optional<std::pair<double, double>> srange,
         QWidget* parent)
    : QWidget(parent), keys(std::move(keys_)), key(keys[0]), spec(spec_ ? *spec_ : nvqt::spec(keys[0])),
      fmtf(format(fmt)), internal(spec.def.as_double()) {
    const Spec& s = spec;
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(6 + 14 * indent, 1, 2, 1);
    lay->setSpacing(4);
    label = new QLabel(label_ ? *label_ : QString::fromStdString(s.label));
    if (!label_ || !label_->isEmpty()) {
        label->setMinimumWidth(std::max(70 - 14 * indent, 40));
        lay->addWidget(label);
    }
    deco = new QLabel();
    deco->setFixedSize(QSize(24, 16));
    if (!deco_.empty()) {
        deco->setPixmap(deco_.rfind("dot:", 0) == 0 ? dot_pixmap(deco_.substr(4), 16) : band_pixmap(deco_.substr(5)));
        lay->addWidget(deco);
    }
    slider = new QSlider(Qt::Horizontal);
    double lo = fmtf.to_d(s.lo), hi = fmtf.to_d(s.hi);
    if (lo > hi) std::swap(lo, hi);
    const double slo = srange ? srange->first : lo, shi = srange ? srange->second : hi;
    slider->setRange(static_cast<int>(std::nearbyint(slo * fmtf.sc)), static_cast<int>(std::nearbyint(shi * fmtf.sc)));
    slider->setSingleStep(fmtf.sc == 1 ? steps.first * fmtf.sc : 5);
    slider->setPageStep(fmtf.sc == 1 ? steps.second * fmtf.sc : 20);
    slider->setMinimumWidth(70);
    lay->addWidget(slider, 1);
    stack = new QStackedWidget();
    stack->setFixedWidth(58);
    val = new ValueLabel();
    spin = new QDoubleSpinBox();
    spin->setRange(lo, hi);
    spin->setDecimals(fmtf.sc == 1 ? 0 : 2);
    spin->setKeyboardTracking(false);
    spin->setButtonSymbols(QDoubleSpinBox::NoButtons);
    stack->addWidget(val);
    stack->addWidget(spin);
    lay->addWidget(stack);
    reset = new QToolButton();
    reset->setText(QString::fromUtf8("⋈"));
    reset->setAutoRaise(true);
    reset->setFixedSize(16, 16);
    reset->setStyleSheet("color:#e0409a; border:none; font-weight:bold");
    reset->setToolTip("Reset to the default value");
    QSizePolicy sp = reset->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    reset->setSizePolicy(sp);
    reset->setVisible(false);
    lay->addWidget(reset);
    const QString tip = QString::fromStdString(s.tip.empty() ? s.label : s.tip);
    for (QWidget* w : {static_cast<QWidget*>(label), static_cast<QWidget*>(slider), static_cast<QWidget*>(val),
                       static_cast<QWidget*>(deco)})
        w->setToolTip(tip);
    implemented = s.implemented;
    setEnabled(s.implemented);
    connect(slider, &QSlider::sliderPressed, this, &Row::editStarted);
    // only a real drag (handle held) merges into the press's undo step; wheel / keys / page steps start new ones
    connect(slider, &QSlider::actionTriggered, this, [this](int a) { action_(a); });
    connect(slider, &QSlider::valueChanged, this, [this](int iv) { from_slider_(iv); });
    connect(val, &ValueLabel::clicked, this, [this] { edit_exact_(); });
    connect(spin, &QDoubleSpinBox::editingFinished, this, [this] { exact_done_(); });
    connect(reset, &QToolButton::clicked, this, [this] { reset_(); });
    show_(s.def.as_double(), false);
}

void Row::action_(int a) {
    const bool drag = a == QAbstractSlider::SliderMove && slider->isSliderDown();
    if (!drag) emit editStarted();
}

void Row::set_value(double v, bool mixed) {
    internal = spec.kind == "int" ? static_cast<double>(static_cast<long long>(v)) : v;
    block_ = true;
    slider->setValue(static_cast<int>(std::nearbyint(fmtf.to_d(v) * fmtf.sc)));
    block_ = false;
    show_(v, mixed);
}

void Row::show_(double v, bool mixed) {
    const double d = fmtf.to_d(v);
    val->setText(!implemented ? QString("n/a") : mixed ? QString("mixed") : fmtf.text(d));
    reset->setVisible(implemented && (mixed || std::fabs(v - spec.def.as_double()) > 1e-9));
    val->update();
}

void Row::emit_(double v) {
    v = std::min(std::max(v, spec.lo), spec.hi);      // the slider track may extend past the spec range
    if (spec.kind == "int") v = std::nearbyint(v);
    internal = v;
    show_(v, false);
    if (keys.size() == 1) {
        emit edited(QString::fromStdString(key), v);
    } else {
        QVariantMap m;
        for (const auto& k : keys) m[QString::fromStdString(k)] = v;
        emit editedMany(m);
    }
}

void Row::from_slider_(int iv) {
    if (!block_) emit_(fmtf.from_d(static_cast<double>(iv) / fmtf.sc));
}

void Row::edit_exact_() {
    spin->setValue(fmtf.to_d(internal));
    stack->setCurrentIndex(1);
    spin->setFocus();
    spin->selectAll();
}

void Row::exact_done_() {
    if (stack->currentIndex() != 1) return;
    stack->setCurrentIndex(0);
    const double v = fmtf.from_d(spin->value());
    if (std::fabs(v - internal) > 1e-12) {
        emit editStarted();
        set_value(v);
        emit_(v);
    }
}

void Row::reset_() {
    emit editStarted();
    const double d = spec.def.as_double();
    set_value(d);
    emit_(d);
}

// ------------------------------------------------------------------ EnableCheck
EnableCheck::EnableCheck(const std::string& key_, QWidget* parent)
    : QCheckBox("Enable", parent), key(key_), spec(nvqt::spec(key_)) {
    setLayoutDirection(Qt::RightToLeft);
    setToolTip(QString::fromStdString(spec.tip.empty() ? spec.label : spec.tip));
    setEnabled(spec.implemented);
    connect(this, &QCheckBox::toggled, this, [this](bool v) {
        if (!block_) {
            emit editStarted();
            emit edited(QString::fromStdString(key), v);
        }
    });
}

void EnableCheck::set_value(bool v) {
    block_ = true;
    setChecked(v);
    block_ = false;
}

// ------------------------------------------------------------------ ChoiceRow
ChoiceRow::ChoiceRow(const std::string& key_, const QString& label_, const std::vector<QString>& names, int indent,
                     QWidget* parent)
    : QWidget(parent), key(key_), spec(nvqt::spec(key_)), choices(spec.choices) {
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(6 + 14 * indent, 1, 2, 1);
    label = new QLabel(spec.implemented ? label_ : label_ + " (n/a)");
    label->setMinimumWidth(90);
    lay->addWidget(label);
    box = new QComboBox();
    if (!names.empty()) {
        for (const auto& n : names) box->addItem(n);
    } else {
        for (const auto& c : choices) box->addItem(QString::fromStdString(c));
    }
    lay->addWidget(box, 1);
    lay->addSpacing(20);
    const QString tip = QString::fromStdString(spec.tip.empty() ? spec.label : spec.tip);
    label->setToolTip(tip);
    box->setToolTip(tip);
    setEnabled(spec.implemented);
    connect(box, &QComboBox::currentIndexChanged, this, [this](int i) {
        if (!block_ && i >= 0) {
            emit editStarted();
            emit edited(QString::fromStdString(key), QString::fromStdString(choices[static_cast<size_t>(i)]));
        }
    });
}

QString ChoiceRow::value() const {
    return QString::fromStdString(choices[static_cast<size_t>(box->currentIndex())]);
}

void ChoiceRow::set_value(const QString& v) {
    block_ = true;
    int idx = 0;
    for (size_t i = 0; i < choices.size(); ++i)
        if (QString::fromStdString(choices[i]) == v) {
            idx = static_cast<int>(i);
            break;
        }
    box->setCurrentIndex(idx);
    block_ = false;
}

// ------------------------------------------------------------------ BeginRow
BeginRow::BeginRow(const QString& title_, const QString& tip, bool checkable, bool enabled, int indent, bool dim_,
                   QWidget* parent)
    : QWidget(parent), name(title_), dim(dim_) {
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(6 + 14 * indent, 2, 22, 2);
    title = new QLabel();
    title->setToolTip(tip);
    title->setWordWrap(true);
    title->setMinimumWidth(0);
    title->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    lay->addWidget(title, 1);
    button = new QPushButton(enabled ? "Begin" : "Begin (n/a)");
    button->setCheckable(checkable);
    button->setFixedWidth(80);
    button->setToolTip(tip);
    lay->addWidget(button);
    connect(button, &QPushButton::toggled, this, [this](bool on) {
        look_(on);
        emit toggled(on);
    });
    connect(button, &QPushButton::clicked, this, [this, checkable] {
        if (!checkable) emit toggled(true);
    });
    setEnabled(enabled);
    look_(false);
}

void BeginRow::look_(bool on) {
    const QString c = on ? QString(ACTIVE_TITLE) : dim ? QString("#9a9a9a") : QString();
    const QString gear = QString::fromUtf8("⚙ ") + name;
    const QString nm = c.isEmpty() ? gear : QString("<span style='color:%1'>%2</span>").arg(c, gear);
    title->setText(nm + " <span style='color:#6ea8ff'>(?)</span>");
    if (button->text() != "Begin (n/a)") button->setText(on ? "Finish" : "Begin");
    button->setIcon(on ? square_icon(QColor(70, 150, 255)) : triangle_icon());
}

void BeginRow::set_checked(bool on) {
    button->blockSignals(true);
    button->setChecked(on);
    look_(on);
    button->blockSignals(false);
}

// ------------------------------------------------------------------ Header
Header::Header(const QString& title_, EnableCheck* enable_, bool expanded, bool collapsible_, int level,
               QWidget* inline__, bool reset, const QString& help_tip, QWidget* parent)
    : QWidget(parent), collapsible(collapsible_), inline_w(inline__), enable(enable_), title_text(title_) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);
    band = new QWidget();
    band->setObjectName("band");
    if (level == 0) {
        band->setStyleSheet(QString("#band{") + HEADER_CSS + "}");
        band->setAttribute(Qt::WA_StyledBackground, true);
    }
    auto* h = new QHBoxLayout(band);
    h->setContentsMargins(2 + 14 * std::max(level - 1, 0), 3, 2, 3);
    h->setSpacing(4);
    arrow = new QToolButton();
    arrow->setAutoRaise(true);
    arrow->setFixedSize(14, 14);
    arrow->setStyleSheet("border:none");
    QSizePolicy sp = arrow->sizePolicy();
    sp.setRetainSizeWhenHidden(true);                  // titles stay aligned when Beginner hides the arrow
    arrow->setSizePolicy(sp);
    if (collapsible) {
        arrow->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
        connect(arrow, &QToolButton::clicked, this, [this] { toggle(); });
    } else {
        arrow->setText(level ? QString::fromUtf8("•") : QString());
    }
    h->addWidget(arrow);
    const bool na = enable != nullptr && !enable->spec.implemented;
    QString t = "<b>" + title_ + "</b>";
    if (!help_tip.isEmpty()) t += " <span style='color:#6ea8ff'>(?)</span>";
    if (na) t += QString(" <span style='color:%1'>(n/a)</span>").arg(level == 0 ? "#c8c8c8" : "#a8a8a8");
    title = new QLabel(t);
    if (level == 0) title->setStyleSheet(QString("color:") + HEADER_TEXT);
    if (na) {
        title->setToolTip(QString::fromStdString(enable->spec.tip));
        enable->na_label = title;
    }
    if (!help_tip.isEmpty()) title->setToolTip(help_tip);
    title->installEventFilter(this);
    h->addWidget(title);
    if (inline_w != nullptr) {
        inline_w->layout()->setContentsMargins(0, 0, 0, 0);
        h->addWidget(inline_w, 1);
    } else {
        h->addStretch(1);
    }
    if (enable != nullptr) h->addWidget(enable);
    if (reset) {
        reset_btn = new QToolButton();
        reset_btn->setText(QString::fromUtf8("↺"));
        reset_btn->setAutoRaise(true);
        reset_btn->setToolTip("Reset all settings of this tab to the default preset values");
        connect(reset_btn, &QToolButton::clicked, this, &Header::resetClicked);
        h->addWidget(reset_btn);
    }
    v->addWidget(band);
    body = new QWidget();
    lay = new QVBoxLayout(body);
    lay->setContentsMargins(0, 0, 0, 2);
    lay->setSpacing(0);
    body->setVisible(expanded);
    v->addWidget(body);
}

bool Header::eventFilter(QObject* o, QEvent* e) {
    // title.mousePressEvent = lambda e: self.toggle() if collapsible else None
    if (o == title && e->type() == QEvent::MouseButtonPress) {
        if (collapsible) toggle();
        return true;
    }
    return QWidget::eventFilter(o, e);
}

void Header::toggle() { set_expanded(body->isHidden()); }

void Header::set_expanded(bool on) {
    if (collapsible) arrow->setArrowType(on ? Qt::DownArrow : Qt::RightArrow);
    body->setVisible(on);
}

QWidget* Header::add(QWidget* w) {
    lay->addWidget(w);
    return w;
}

QWidget* separator() {
    auto* w = new QWidget();
    w->setFixedHeight(6);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return w;
}

// ------------------------------------------------------------------ IconBar
IconBar::IconBar(const QString& what, QWidget* parent) : QWidget(parent) {
    auto* lay = new QHBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(2);
    QStyle* st = style();
    auto btn = [&](std::optional<QStyle::StandardPixmap> icon, const QString& tip, const QString& text = QString()) {
        auto* b = new QToolButton();
        if (icon) b->setIcon(st->standardIcon(*icon));
        if (!text.isEmpty()) b->setText(text);
        b->setToolTip(tip);
        lay->addWidget(b);                      // raised (genuine: every icon button has its bevel)
        return b;
    };
    open = btn(QStyle::SP_DirOpenIcon, QString("Load %1 from a file…").arg(what));
    save = btn(QStyle::SP_DialogSaveButton, QString("Save current %1 to a file…").arg(what));
    lay->addSpacing(10);
    undo = btn(std::nullopt, "Undo last change");
    redo = btn(std::nullopt, "Redo");
    reset = btn(std::nullopt, QString("Reset %1").arg(what), QString::fromUtf8("✕"));
    reset->setStyleSheet("QToolButton{color:#d02020; font-weight:bold}");
    lay->addStretch(1);
    default_ = btn(std::nullopt, QString("Default %1").arg(what));
    default_->setIcon(pin_icon());
    default_->setPopupMode(QToolButton::InstantPopup);
    default_menu = new QMenu(default_);
    default_->setMenu(default_menu);
    info = btn(std::nullopt, QString("Description of the current %1").arg(what));
    info->setIcon(info_icon());
}

void IconBar::bind_undo(QAction* u, QAction* r, QUndoStack* stack) {
    struct B { QToolButton* b; QAction* a; const char* glyph; bool back; };
    for (const B& x : {B{undo, u, "↶", true}, B{redo, r, "↷", false}}) {
        if (x.a->icon().isNull()) x.a->setIcon(glyph_icon(QString::fromUtf8(x.glyph), QColor(80, 150, 255), 20, true));
        x.b->setDefaultAction(x.a);
        x.b->setToolButtonStyle(Qt::ToolButtonIconOnly);
        if (stack != nullptr) {
            auto* m = new QMenu(x.b);
            const bool back = x.back;
            connect(m, &QMenu::aboutToShow, m, [m, stack, back] { history_(m, stack, back); });
            x.b->setMenu(m);
            x.b->setPopupMode(QToolButton::MenuButtonPopup);
        }
    }
}

void IconBar::history_(QMenu* menu, QUndoStack* stack, bool back) {
    menu->clear();
    const int i = stack->index();
    std::vector<int> steps;
    if (back) {
        for (int k = i - 1; k > std::max(i - 11, -1); --k) steps.push_back(k);
    } else {
        for (int k = i; k < std::min(i + 10, stack->count()); ++k) steps.push_back(k);
    }
    for (int k : steps)
        menu->addAction(stack->text(k), menu, [stack, k, back] { stack->setIndex(back ? k : k + 1); });
    if (menu->isEmpty()) menu->addAction("(no history)")->setEnabled(false);
}

QLabel* panel_title(const QString& text) {
    auto* lab = new QLabel("<b>" + text + "</b>");
    lab->setAlignment(Qt::AlignCenter);
    return lab;
}

std::pair<QLabel*, QLabel*> guide_labels() {
    auto* qsg = new QLabel("How to use: <a href='qsg'>Quick Start Guide</a>.");
    auto* mode = new QLabel();
    for (QLabel* l : {qsg, mode}) {
        l->setAlignment(Qt::AlignCenter);
        l->setWordWrap(true);
    }
    return {qsg, mode};
}

QString mode_text(bool advanced) {
    const QString other = advanced ? "Beginner" : "Advanced";
    const QString more = advanced ? "simpler settings" : "more control";
    return QString("Currently working in <b>%1 Mode</b>.<br>For %2 switch to <a href='mode'>%3 Mode</a>.")
        .arg(advanced ? "Advanced" : "Beginner", more, other);
}

}  // namespace nvqt
