// profile_panel - see profile_panel.h (port of repro/nv_ui_qt/widgets/profile_panel.py).
#include "profile_panel.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QSizePolicy>
#include <QVBoxLayout>

#include <cmath>
#include <cstdio>

#include "nvcore_c_prof5.h"

namespace nvqt {

namespace {

const char* TUNE_TIP = "Profile Noise Level (repro): multiplies the noise profile curve%1 - higher values -> more "
                       "image elements are treated as noise. Saved with the profile.";
const char* DARK_BOX = "background:#3a3a3a; color:#d5d5d5; border:1px solid #262626;";
const char* CHECK_TIP =
    "Profile Check helps to find the optimal value of Noise Level using a visually enhanced preview "
    "(Assist Image).\nClick Begin to start the checking and tuning process.\n"
    "Adjust Noise Level while watching the image in preview to find the Noise Level value that "
    "completely removes noise yet preserves details.\n"
    "(repro: the Assist Image is the current frame through the spatial filter only, all amounts "
    "100%; the genuine's Assist Image processing is not decoded)";

QString eq_tip(const std::string& kind) {
    if (kind == "aft")
        return "Use to automatically fine-tune the current noise profile using the whole input image\n"
               "(repro: nv_aft_v5.apply_aft, static port of the genuine AutoFineTune; its factors were not "
               "compared with the genuine; takes several seconds)";
    if (kind == "manual")
        return "Use to additionally fine-tune current noise profile using selected image area\n"
               "(n/a: not available in the reproduction; drag the knots of the graph instead)";
    return "Use to finalize manual fine-tuning by interpolating missing values using measured data\n"
           "(repro: interpolates the bins you did not edit between the edited ones)";
}

// f"{v:.1f}": the C printf (correctly rounded, half to even on the exact value as Python);
// QString::asprintf rounds 3.25 to 3.3
QString fmt1(double v) {
    char b[64];
    std::snprintf(b, sizeof(b), "%.1f", v);
    return QString::fromUtf8(b);
}

}  // namespace

// ------------------------------------------------------------------ BandBar
BandBar::BandBar(QWidget* parent) : QWidget(parent) { setFixedSize(44, 12); }

void BandBar::set_value(std::optional<double> v) {
    value = v ? std::min(std::max(*v, 0.0), 1.0) : 0.0;
    update();
}

void BandBar::paintEvent(QPaintEvent*) {
    QPainter p(this);
    const QRectF r = QRectF(rect()).adjusted(0, 1, -1, -1);
    if (value > 0) {
        const QRectF b(r.left(), r.top(), std::max(r.width() * value, 3.0), r.height());
        p.fillRect(b, QColor(76, 175, 80));
        p.setPen(QPen(QColor(30, 60, 30), 1));
        p.drawRect(b);
    }
    p.end();
}

QIcon eq_icon(const std::string& kind, int size) {
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(60, 60, 60), 1));
    p.setBrush(QColor(236, 236, 236));
    p.drawRect(QRectF(1.5, 1.5, size - 3, size - 3));
    static const int AFT[6] = {5, 9, 7, 12, 10, 14}, MAN[6] = {6, 11, 4, 9, 12, 7}, COM[6] = {4, 7, 9, 11, 9, 13};
    const int* hs = kind == "aft" ? AFT : kind == "manual" ? MAN : COM;
    for (int k = 0; k < 6; ++k) {
        const double x = 3.5 + k * 2.4;
        const bool dotted = kind == "manual" && (k % 2);
        p.setPen(QPen(QColor(40, 150, 40), 1.4, dotted ? Qt::DotLine : Qt::SolidLine));
        p.drawLine(QPointF(x, size - 3.5), QPointF(x, size - 3.5 - hs[k]));
    }
    if (kind == "complete") {
        p.setPen(QPen(QColor(240, 140, 20), 1.6));
        const QPointF pts[3] = {QPointF(3, size - 8), QPointF(size / 2.0, 5), QPointF(size - 3, 8)};
        p.drawPolyline(pts, 3);
    }
    p.end();
    return QIcon(pm);
}

std::optional<std::vector<double>> band_bars(const PyObj& base) {
    if (base.is_none()) return std::nullopt;
    const std::vector<double> r = base.get("band_ratio").get("Y").as_doubles();
    const int n = static_cast<int>(r.size());
    std::vector<double> wn(static_cast<size_t>(std::max(n, 1)));
    if (n >= 1) nvc_wn_band_std(n, wn.data());
    std::vector<double> v;
    for (int j = 0; j < n; ++j) v.push_back(r[static_cast<size_t>(j)] * wn[0] / wn[static_cast<size_t>(j)]);
    double m = 1.0;
    if (!v.empty()) {
        double mx = v[0];
        for (double x : v)
            if (x > mx) mx = x;                  // Python max(v)
        if (mx > 0) m = mx;
    }
    for (double& x : v) x = x / m;
    return v;
}

Spec tune_spec(const QString& label, const QString& c) {
    Spec s;
    s.kind = "float";
    s.def = PyObj::real(1.0);
    s.lo = TUNE_MIN;
    s.hi = TUNE_MAX;
    s.label = label.toStdString();
    s.tip = QString(TUNE_TIP).arg(c.isEmpty() ? QString() : " of " + c).toStdString();
    return s;
}

// ------------------------------------------------------------------ ProfilePanel
ProfilePanel::ProfilePanel(QWidget* parent) : QWidget(parent) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    // one scroll area without a frame: no scroll bar while everything fits (the genuine panel does not scroll)
    scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    auto* body = new QWidget();
    auto* v = new QVBoxLayout(body);
    v->setContentsMargins(4, 4, 4, 4);
    v->setSpacing(3);
    scroll->setWidget(body);
    outer->addWidget(scroll);

    v->addWidget(panel_title("Device Noise Profile"));
    icons = new IconBar("noise profile");
    v->addWidget(icons);
    name = new QComboBox();
    v->addWidget(name);
    auto* dh = new QHBoxLayout();
    dh->addWidget(new QLabel("Device Name and Mode:"));
    dh->addStretch(1);
    device_menu = new QToolButton();
    device_menu->setText(QString::fromUtf8("▸≡"));
    device_menu->setAutoRaise(true);
    device_menu->setEnabled(false);
    device_menu->setToolTip("Device name and mode options (n/a: not available in the reproduction)");
    dh->addWidget(device_menu);
    v->addLayout(dh);
    device = new QLineEdit("Unspecified device");
    device->setToolTip("Optional device name (e.g., 'Nikon D90'); saved with the profile");
    connect(device, &QLineEdit::editingFinished, this, [this] { emit editDevice(device->text()); });
    v->addWidget(device);
    info = new QLabel();
    info->setStyleSheet(QString("QLabel{") + DARK_BOX + " padding:3px 5px}");
    info->setTextFormat(Qt::RichText);
    info->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    info->setMinimumHeight(5 * info->fontMetrics().lineSpacing() + 10);
    v->addWidget(info);

    // statistics: left Quality / Noise Levels, right the bands
    stats = new QWidget();
    auto* sh = new QHBoxLayout(stats);
    sh->setContentsMargins(0, 2, 0, 2);
    sh->setSpacing(6);
    auto* left = new QGridLayout();
    left->setContentsMargins(2, 0, 0, 0);
    left->setVerticalSpacing(4);
    left->addWidget(new QLabel("Quality"), 0, 0);
    quality = new QLabel("n/a");
    quality->setAlignment(Qt::AlignCenter);
    quality->setFixedWidth(52);
    quality->setStyleSheet(QString("QLabel{") + DARK_BOX + "}");
    quality->setToolTip("Overall Profile Quality: curve completeness x sample uniformity x "
                        "area size x clipping penalty (genuine measure, decoded 2026-10-26)");
    left->addWidget(quality, 0, 1, Qt::AlignRight);
    left->addWidget(new QLabel("Completeness"), 1, 0);
    completeness = new QLabel("n/a");
    completeness->setAlignment(Qt::AlignCenter);
    completeness->setFixedWidth(52);
    completeness->setStyleSheet(QString("QLabel{") + DARK_BOX + "}");
    completeness->setToolTip("Completeness: the luminance-curve term g x 100 (genuine measure, decoded 2026-10-26)");
    left->addWidget(completeness, 1, 1, Qt::AlignRight);
    auto* lt = new QHBoxLayout();
    levels_title = new QLabel("<b>Noise Levels</b>");
    lt->addWidget(levels_title);
    levels_line = new QFrame();
    levels_line->setFrameShape(QFrame::HLine);
    levels_line->setFrameShadow(QFrame::Sunken);
    lt->addWidget(levels_line, 1);
    left->addLayout(lt, 2, 0, 1, 2);
    l_main = new QLabel("Main Sample");
    main_val = new QLabel("-");
    l_sel = new QLabel("Selection");
    sel_val = new QLabel("-");
    for (QLabel* w : {main_val, sel_val}) {
        w->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        w->setToolTip("Noise Level (genuine measure FUN_009b01d4): sqrt of the sum over five levels and "
                      "R, G, B of the squared mean local RMS of the area (Main Sample = the profile's area)");
    }
    left->addWidget(l_main, 3, 0);
    left->addWidget(main_val, 3, 1);
    left->addWidget(l_sel, 4, 0);
    left->addWidget(sel_val, 4, 1);
    left->setRowStretch(5, 1);
    sh->addLayout(left, 1);
    auto* sep = new QFrame();
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);
    sh->addWidget(sep);
    auto* right = new QGridLayout();
    right->setContentsMargins(0, 0, 0, 0);
    right->setHorizontalSpacing(4);
    right->setVerticalSpacing(3);
    int i = 0;
    for (const auto& [b, nm] : UI_BANDS) {
        const QString bname = QString::fromStdString(nm);
        auto* pic = new QLabel();
        pic->setPixmap(band_pixmap(b));
        pic->setToolTip(QString("Noise pattern of the %1 frequency band (repro picture)").arg(bname));
        auto* lab = new QLabel(nm == "Mid" ? QString("Medium") : bname);
        auto* bar = new BandBar();
        bar->setToolTip("Band noise relative to white noise (repro definition, see the panel's doc)");
        right->addWidget(pic, i, 0);
        right->addWidget(lab, i, 1);
        right->addWidget(bar, i, 2);
        bars[b] = bar;
        band_samples[b] = pic;
        ++i;
    }
    right->setColumnStretch(1, 1);
    sh->addLayout(right, 1);
    v->addWidget(stats);
    for (QWidget* w : {static_cast<QWidget*>(levels_title), static_cast<QWidget*>(levels_line),
                       static_cast<QWidget*>(l_main), static_cast<QWidget*>(main_val), static_cast<QWidget*>(l_sel),
                       static_cast<QWidget*>(sel_val)})
        reg_(w, "adv");

    // equalizer: graph, then [Auto Fine-Tune, Manual, Auto Complete | 4 round toggles | n/a | x]
    auto* eq = new QFrame();
    eq->setObjectName("eq");
    eq->setStyleSheet("#eq{background:#595959; border:1px solid #3a3a3a}");
    auto* el = new QVBoxLayout(eq);
    el->setContentsMargins(2, 2, 2, 2);
    curve = new CurveEditor();
    el->addWidget(curve);
    QHBoxLayout* row = curve->buttons;
    auto tool = [&](const std::string& kind, const QString& text) {
        auto* b = new QToolButton();
        b->setText(text);
        b->setIcon(eq_icon(kind));
        b->setIconSize(QSize(20, 20));
        b->setToolButtonStyle(Qt::ToolButtonIconOnly);
        b->setFixedSize(26, 24);
        b->setToolTip(eq_tip(kind));
        return b;
    };
    b_aft = tool("aft", "Auto Fine-Tune");
    b_manual = tool("manual", "Manual Fine-Tune (n/a)");
    b_manual->setEnabled(false);
    b_complete = tool("complete", "Auto Complete");
    int k = 0;
    for (QToolButton* b : {b_aft, b_manual, b_complete}) row->insertWidget(k++, b);
    row->insertSpacing(3, 6);
    complete_lbl = new QLabel("n/a");
    complete_lbl->setAlignment(Qt::AlignCenter);
    complete_lbl->setFixedWidth(56);
    complete_lbl->setStyleSheet(QString("QLabel{") + DARK_BOX + "}");
    complete_lbl->setToolTip("Overall completeness and accuracy of fine-tuning\n(n/a: not computed in the reproduction)");
    b_reset = new QToolButton();
    b_reset->setText(QString::fromUtf8("✕"));
    b_reset->setFixedSize(26, 24);
    b_reset->setStyleSheet("QToolButton{color:#e02020; font-weight:bold; font-size:16px}");
    b_reset->setToolTip("Reset all fine-tuning");
    row->addStretch(1);
    row->addWidget(complete_lbl);
    row->addStretch(1);
    row->addWidget(b_reset);
    connect(b_aft, &QToolButton::clicked, this, &ProfilePanel::autoFineTune);
    connect(b_complete, &QToolButton::clicked, this, &ProfilePanel::autoComplete);
    connect(b_reset, &QToolButton::clicked, this, &ProfilePanel::resetFineTune);
    connect(curve, &CurveEditor::editStarted, this, &ProfilePanel::editStarted);
    v->addWidget(reg_(eq, "adv"));

    // Profile Check and Tuning
    tuning = new Header("Profile Check and Tuning", nullptr, true, false);
    check = new BeginRow("Profile Check", CHECK_TIP, true, true, 0, true);
    connect(check, &BeginRow::toggled, this, &ProfilePanel::profileCheck);
    tuning->add(check);
    Row* inl = tune_row_("all", "");
    tune_header = new Header("Noise Level", nullptr, false, true, 1, inl);
    reg_(tune_header->arrow, "adv");
    for (const auto& [c, nm] : UI_CHANNELS)
        tune_header->add(reg_(tune_row_(c, QString::fromStdString(nm), "dot:" + c), "adv"));
    freq_header = new Header("Frequencies", nullptr, false, true, 2);
    for (const auto& [b, nm] : UI_BANDS) {
        Spec s;
        s.kind = "float";
        s.def = PyObj::real(1.0);
        s.lo = TUNE_MIN;
        s.hi = TUNE_MAX;
        s.label = nm;
        s.implemented = false;
        s.tip = "Not available: the reproduction's profile curve has no frequency bands";
        auto* r = new Row({"tune_band_" + b}, QString::fromStdString(nm), "tune", "band:" + b, {5, 20}, 1, &s,
                          std::make_pair(-100.0, 150.0));
        freq_header->add(r);
    }
    tune_header->add(reg_(freq_header, "adv"));
    tuning->add(tune_header);
    v->addWidget(tuning);
    v->addStretch(1);
    set_name(std::nullopt);
    set_mode(true);
}

QWidget* ProfilePanel::reg_(QWidget* w, const std::string& modes) {
    modes_.emplace_back(w, modes);
    return w;
}

Row* ProfilePanel::tune_row_(const std::string& k, const QString& label, const std::string& deco) {
    const Spec s = tune_spec(k == "all" ? QString("Noise Level") : label, k == "all" ? QString() : label);
    auto* r = new Row({"tune_" + k}, label, "tune", deco, {5, 20}, 0, &s, std::make_pair(-100.0, 150.0));
    const QString key = QString::fromStdString(k);
    connect(r, &Row::edited, this, [this, key](QString, double v) { emit tuneEdited(key, v); });
    connect(r, &Row::editStarted, this, &ProfilePanel::editStarted);
    tune[k] = r;
    return r;
}

void ProfilePanel::set_mode(bool advanced) {
    for (const auto& [w, m] : modes_)
        w->setVisible(m.find(advanced ? "adv" : "beg") != std::string::npos || m == "both");
}

void ProfilePanel::set_busy(bool busy) { b_aft->setEnabled(!busy); }

void ProfilePanel::set_check_active(bool on) {
    check->set_checked(on);
    for (Header* hd : {tuning, tune_header}) {
        const QString t = "<b>" + hd->title_text + "</b>";
        hd->title->setText(on ? QString("<span style='color:%1'>%2</span>").arg(ACTIVE_TITLE, t) : t);
    }
}

void ProfilePanel::set_name(std::optional<QString> text, bool dirty) {
    name->clear();
    if (!text) {
        name->addItem("profile not ready");
        name->setStyleSheet("QComboBox{color:#ff6060}");
        name->setToolTip("Build a noise profile by analysing a uniform area or load a ready-made profile.");
    } else {
        name->addItem(*text + (dirty ? " *" : ""));
        name->setStyleSheet("");
        name->setToolTip("Current noise profile (* = not saved)");
    }
}

void ProfilePanel::show_profile(const ProfileState& ps, const PyObj*, std::optional<std::pair<int, int>> frame_info) {
    const QString dev = QString::fromStdString(ps.device);
    if (device->text() != dev) device->setText(dev);
    std::optional<int> main_bin;
    if (ps.has_profile()) {
        const double lvl = ps.base.has("area_level") ? ps.base.get("area_level").as_double() : 128.0;
        main_bin = static_cast<int>(std::min(std::max(std::nearbyint(lvl / 32.0), 0.0), 8.0));
    }
    curve->set_factors(ps.factors, ps.pinned, main_bin, ps.channels);
    for (auto& [kk, r] : tune) r->set_value(ps.tune_value(kk));
    const QString W = frame_info ? QString::number(frame_info->first) : QString::fromUtf8("—");
    const QString H = frame_info ? QString::number(frame_info->second) : QString::fromUtf8("—");
    const auto bv = band_bars(ps.base);
    int i = 0;
    for (const auto& [b, nm] : UI_BANDS) {
        bars[b]->set_value(!bv || i >= static_cast<int>(bv->size()) ? std::nullopt
                                                                     : std::optional<double>((*bv)[static_cast<size_t>(i)]));
        ++i;
    }
    QString rows;
    if (ps.has_profile()) {
        const std::pair<QString, QString> rs[5] = {{"Frame size:", QString("%1x%2 p").arg(W, H)},
                                                   {"Scan type:", "Progressive"},
                                                   {"Gamma:", "Gamma corrected"},
                                                   {"Gain:", "x1.0"},
                                                   {"Profiled with:", QString("Full frame (%1x%2)").arg(W, H)}};
        for (const auto& [a, b] : rs) rows += QString("<tr><td>%1&nbsp;&nbsp;</td><td>%2</td></tr>").arg(a, b);
    }
    info->setText("<table cellspacing=0 cellpadding=0>" + rows + "</table>");
}

void ProfilePanel::set_levels(std::optional<double> main, std::optional<double> sel) {
    main_val->setText(main ? fmt1(*main) : QString("-"));
    sel_val->setText(sel ? fmt1(*sel) : QString("-"));
}

void ProfilePanel::set_quality(std::optional<int> pct) {
    quality->setText(pct ? QString("%1%").arg(*pct) : QString("n/a"));
}

void ProfilePanel::set_completeness(std::optional<int> pct) {
    completeness->setText(pct ? QString("%1%").arg(*pct) : QString("n/a"));
}

}  // namespace nvqt
