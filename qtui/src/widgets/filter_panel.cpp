// filter_panel - see filter_panel.h (port of repro/nv_ui_qt/widgets/filter_panel.py).
#include "filter_panel.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QScrollArea>

#include <cmath>

namespace nvqt {

namespace {
const std::vector<QString> QUALITY_NAMES = {"Normal (Faster)", "High (Slower)"};
const std::pair<double, double> NOISE_TRACK = {-100.0, 150.0};   // genuine Noise Level sliders: -100 ... +150 %
}  // namespace

FilterPanel::FilterPanel(QWidget* parent) : QWidget(parent), params(default_params()) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(4, 4, 4, 4);
    v->addWidget(panel_title("Filter Settings"));
    icons = new IconBar("filter settings");
    v->addWidget(icons);
    name = new QComboBox();
    name->addItem("default");
    name->setToolTip("Name of the current filter preset (* = changed since it was loaded)");
    v->addWidget(name);
    tabs = new QTabWidget();
    v->addWidget(tabs, 1);
    temporal_();
    spatial_();
    general_();
    auto gl = guide_labels();
    qsg = gl.first;
    mode_lbl = gl.second;
    for (QLabel* l : {qsg, mode_lbl}) {
        connect(l, &QLabel::linkActivated, this, &FilterPanel::linkActivated);
        v->addWidget(l);
    }
    set_mode(true);
}

// ---------------- construction helpers ----------------
QVBoxLayout* FilterPanel::page_(const QString& title) {
    auto* sc = new QScrollArea();
    sc->setWidgetResizable(true);
    sc->setFrameShape(QFrame::NoFrame);
    sc->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* w = new QWidget();
    auto* lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(1);
    sc->setWidget(w);
    tabs->addTab(sc, title);
    return lay;
}

QWidget* FilterPanel::reg_(QWidget* w, const std::string& modes) {
    modes_.emplace_back(w, modes);
    return w;
}

void FilterPanel::add_to_(QWidget* parent, QWidget* w) {
    if (auto* h = qobject_cast<Header*>(parent))
        h->add(w);
    else
        qobject_cast<QBoxLayout*>(parent->layout())->addWidget(w);
}

QWidget* FilterPanel::bind_(QWidget* w) {
    std::string k;
    if (auto* r = qobject_cast<Row*>(w)) {
        connect(r, &Row::editedMany, this, &FilterPanel::editedMany);
        connect(r, &Row::edited, this, [this](QString key, double v) { emit edited(key, QVariant(v)); });
        connect(r, &Row::editStarted, this, &FilterPanel::editStarted);
        if (r->keys.size() > 1) {
            common.push_back(r);
            return w;
        }
        k = r->key;
    } else if (auto* e = qobject_cast<EnableCheck*>(w)) {
        enable_conn_[e->key] = connect(e, &EnableCheck::edited, this, [this](QString key, bool v) { emit edited(key, QVariant(v)); });
        connect(e, &EnableCheck::editStarted, this, &FilterPanel::editStarted);
        k = e->key;
    } else if (auto* c = qobject_cast<ChoiceRow*>(w)) {
        connect(c, &ChoiceRow::edited, this, [this](QString key, QString v) { emit edited(key, QVariant(v)); });
        connect(c, &ChoiceRow::editStarted, this, &FilterPanel::editStarted);
        k = c->key;
    }
    if (controls.count(k))
        extra[k].push_back(w);
    else
        controls[k] = w;
    return w;
}

Row* FilterPanel::row_(QWidget* parent, const std::string& key, std::optional<QString> label, const std::string& fmt,
                       const std::string& deco, std::pair<int, int> steps, int indent, const std::string& modes) {
    auto* w = new Row({key}, label, fmt, deco, steps, indent, nullptr,
                      fmt == "rel" ? std::optional<std::pair<double, double>>(NOISE_TRACK) : std::nullopt);
    bind_(w);
    add_to_(parent, w);
    reg_(w, modes);
    return w;
}

ChoiceRow* FilterPanel::choice_(QWidget* parent, const std::string& key, const QString& label, const std::vector<QString>& names,
                                int indent, const std::string& modes) {
    auto* w = new ChoiceRow(key, label, names, indent);
    bind_(w);
    add_to_(parent, w);
    reg_(w, modes);
    return w;
}

Header* FilterPanel::header_(QWidget* parent, const QString& title, const std::string& enable, const std::string& modes,
                             QWidget* inline_, bool collapsible, bool expanded, int level, bool reset, const QString& help_tip) {
    EnableCheck* en = nullptr;
    if (!enable.empty()) {
        en = new EnableCheck(enable);
        bind_(en);
    }
    if (inline_) bind_(inline_);
    auto* h = new Header(title, en, expanded, collapsible, level, inline_, reset, help_tip);
    add_to_(parent, h);
    const std::string t = title.toStdString();
    headers[headers.count(t) ? t + "#" + std::to_string(headers.size()) : t] = h;
    reg_(h, modes);
    return h;
}

QWidget* FilterPanel::check_row_(QWidget* parent, const std::string& key, const QString& label, int indent, const std::string& modes) {
    auto* w = new QWidget();
    auto* lay = new QHBoxLayout(w);
    lay->setContentsMargins(6 + 14 * indent, 1, 2, 1);
    const bool na = !spec(key).implemented;
    auto* lab = new QLabel(label + (na ? " (n/a)" : ""));
    lab->setToolTip(QString::fromStdString(spec(key).tip));
    lab->setWordWrap(true);
    lay->addWidget(lab, 1);
    auto* cb = new EnableCheck(key);
    bind_(cb);
    if (na) cb->na_label = lab;
    lay->addWidget(cb);
    add_to_(parent, w);
    reg_(w, modes);
    return w;
}

BeginRow* FilterPanel::na_begin_(QWidget* parent, const QString& title, const std::string& modes) {
    auto* w = new BeginRow(title, "Not available in the reproduction", true, false);
    add_to_(parent, w);
    reg_(w, modes);
    return w;
}

// ---------------- tabs ----------------
void FilterPanel::temporal_() {
    QVBoxLayout* lay = page_("Temporal");
    QWidget* page = lay->parentWidget();
    Header* h = header_(page, "Temporal Filter", "t_enable", "both", nullptr, false);
    nlc_hl_ = {h};                                   // yellow while Noise Level Check runs
    nlc_off_ = {choice_(h, "t_quality", "Quality Mode", QUALITY_NAMES, 0, "adv")};   // disabled then
    nlc_hl_.push_back(row_(h, "t_radius", "Radius", "frames", "", {1, 1}));
    h = header_(page, "Temporal Noise", "", "adv", nullptr, true, false);
    t_noise_header = h;
    auto* nl = new BeginRow("Noise Level Check",
                            "Click Begin to show the Temporal Filter mask. The mask shows the areas where temporal noise "
                            "reduction is applied. Adjust Noise Level while watching the mask to make it sufficiently cover "
                            "the noisy areas of the frame.\n(repro: the mask is 1 - the residual noise of the temporal "
                            "stage, relative to full averaging)",
                            true, true);
    connect(nl, &BeginRow::toggled, this, [this](bool on) { emit noiseLevelCheck(on); });
    h->add(nl);
    reg_(nl, "adv");
    nl_check = nl;
    nlc_hl_.push_back(row_(h, "t_noise", "Noise Level", "rel", "", {5, 20}));
    nlc_off_.push_back(na_begin_(h, "Local Flicker Check"));
    nlc_off_.push_back(row_(h, "flicker_local", "Local Flicker", "amount", "", {1, 5}));
    nlc_off_.push_back(row_(h, "flicker_full", "Frame Flicker", "amount", "", {1, 5}));
    Header* hn = header_(h, "Noise Reduction Amount", "", "both", nullptr, true, true, 1);
    row_(hn, "t_amount", "Amount", "amount", "", {5, 20}, 1);
    nlc_off_.push_back(hn);
    h = header_(page, "Repeated Frames", "", "adv", nullptr, true, false);
    na_begin_(h, "Repeated Frames Check");
    row_(h, "rep_rate", "Repeat Rate", "amount", "", {1, 20});
    nlc_off_.push_back(h);
    h = header_(page, "Jitter of Details", "", "adv", nullptr, true, false);
    row_(h, "jitter_thr", "Threshold");
    check_row_(h, "jitter_enable", "Jitter Extended Filtration");
    nlc_off_.push_back(h);
    h = header_(page, "Flicker", "flicker_enable", "beg", nullptr, false, true, 0);
    nlc_off_.push_back(h);
    h = header_(page, "Dust and Scratches", "dust_enable", "both", nullptr, true, false);
    reg_(h->arrow, "adv");
    choice_(h, "dust_mode", "Mode", {}, 0, "adv");
    row_(h, "dust_thr", "Threshold", "amount", "", {5, 20}, 0, "adv");
    row_(h, "dust_amount", "Amount", "amount", "", {5, 20}, 0, "adv");
    nlc_off_.push_back(h);
    nlc_hl_.push_back(header_(page, "Adapt to Changing Noise", "adapt_enable", "adv", nullptr, false));
    lay->addStretch(1);
}

Header* FilterPanel::freq_(QWidget* parent, const std::string& prefix, const std::string& fmt, std::pair<int, int> steps) {
    Header* h = header_(parent, "Frequencies", "", "adv", nullptr, true, true, 1);
    for (const auto& [b, bname] : UI_BANDS) {
        std::vector<std::string> keys;
        for (const auto& [c, _] : UI_CHANNELS) keys.push_back(prefix + "_" + c + "_" + b);
        auto* w = new Row(keys, QString::fromStdString(bname), fmt, "band:" + b, steps, 1, nullptr,
                          fmt == "rel" ? std::optional<std::pair<double, double>>(NOISE_TRACK) : std::nullopt);
        bind_(w);
        band_of_[w] = b;
        w->setToolTip("Common to Luminance, Cr and Cb (writes the three components; per-component values: "
                      "Per-component bands (repro))");
        h->add(w);
    }
    return h;
}

void FilterPanel::spatial_() {
    QVBoxLayout* lay = page_("Spatial");
    QWidget* page = lay->parentWidget();
    Header* h = header_(page, "Spatial Filter", "s_enable", "both", nullptr, false, true, 0, true);
    connect(h, &Header::resetClicked, this, &FilterPanel::resetSpatial);
    assist = new BeginRow("Filter Tuning Assist", "Filter Tuning Assist (repro): shows the removed component "
                                                  "(original - filtered) x4 around mid grey in the primary viewer pane");
    connect(assist, &BeginRow::toggled, this, &FilterPanel::assistToggled);
    h->add(assist);
    reg_(assist, "adv");
    choice_(h, "s_quality", "Quality Mode", QUALITY_NAMES, 0, "adv");
    h = header_(page, "Noise Reduction Amount");
    row_(h, "amt_Y", "Luminance", "amount", "dot:Y", {5, 10});
    Header* hc = header_(h, "Chrominance", "", "adv", nullptr, true, true, 1);
    for (const char* c : {"Cr", "Cb"}) row_(hc, std::string("amt_") + c, QString(c), "amount", std::string("dot:") + c, {5, 10}, 1);
    freq_(h, "bamt", "amount", {5, 10});
    auto* inline_ = new Row({"adv_noise_level"}, QString(""), "rel", "", {5, 20}, 0, nullptr, NOISE_TRACK);
    h = header_(page, "Noise Level", "", "both", inline_);
    for (const auto& [c, nm] : UI_CHANNELS) row_(h, "noise_" + c, QString::fromStdString(nm), "rel", "dot:" + c, {5, 20}, 0, "adv");
    freq_(h, "bnoise", "rel", {5, 20});
    h = header_(page, "Artifact Removal", "art_enable", "adv", nullptr, true, false);
    row_(h, "art_dots", "Dots", "rel");
    row_(h, "art_lines", "Lines", "rel");
    Header* hf = header_(h, "Frequencies", "", "both", nullptr, true, true, 1);
    for (const auto& [k, nm, b] : std::vector<std::tuple<std::string, QString, std::string>>{
             {"art_high", "High", "high"}, {"art_mid", "Mid", "medium"}, {"art_low", "Low", "low"}})
        row_(hf, k, nm, "rel", "band:" + b, {5, 20}, 1);
    h = header_(page, "Edge Smoothing", "edge_enable", "adv", nullptr, true, false);
    for (const auto& [k, nm, b] : std::vector<std::tuple<std::string, QString, std::string>>{
             {"edge_high", "High", "high"}, {"edge_mid", "Mid", "medium"}, {"edge_low", "Low", "low"}})
        row_(h, k, nm, "amount", "band:" + b);
    h = header_(page, "Sharpening", "sharp_enable", "both", nullptr, true, false);
    sharp_rows.clear();
    for (const auto& [b, adv_name, beg_name] : std::vector<std::tuple<std::string, QString, QString>>{
             {"high", "High", "Fine"}, {"medium", "Mid", "Medium"}, {"low", "Low", "Large"}}) {
        Row* r = row_(h, "sharp_" + b, adv_name, "amount", "band:" + b);
        r->setProperty("adv_name", adv_name);
        r->setProperty("beg_name", beg_name);
        sharp_rows.push_back(r);
        row_(h, "sharp_range_" + b, "Range", "range", "", {5, 20}, 1, "adv");
    }
    hc = header_(h, "Channels", "", "adv", nullptr, true, true, 1);
    for (const auto& [c, nm] : UI_CHANNELS) row_(hc, "sharp_ch_" + c, QString::fromStdString(nm), "amount", "dot:" + c, {5, 20}, 1);
    check_row_(h, "sharp_prevent", "Prevent Over-Sharpening", 0, "adv");
    h = header_(page, "Per-component bands (repro)", "", "adv", nullptr, true, false, 0, false,
                "v5 backend: separate band amounts / noise levels per component (the genuine panel has only the common "
                "Frequencies rows)");
    for (const auto& [c, nm] : UI_CHANNELS) {
        hc = header_(h, QString::fromStdString(nm) + " amount", "", "both", nullptr, true, false, 1);
        for (const auto& [b, bname] : UI_BANDS) row_(hc, "bamt_" + c + "_" + b, QString::fromStdString(bname), "amount", "band:" + b, {5, 20}, 1);
    }
    for (const auto& [c, nm] : UI_CHANNELS) {
        hc = header_(h, QString::fromStdString(nm) + " noise level", "", "both", nullptr, true, false, 1);
        for (const auto& [b, bname] : UI_BANDS) row_(hc, "bnoise_" + c + "_" + b, QString::fromStdString(bname), "rel", "band:" + b, {5, 20}, 1);
    }
    h = header_(page, "Advanced (repro)", "", "adv", nullptr, true, false, 0, false,
                "v5 backend parameters without a genuine control");
    row_(h, "adv_threshold", "Temporal threshold", "float");
    row_(h, "adv_spatial", "Spatial strength", "float");
    choice_(h, "adv_similarity", "Temporal similarity");
    lay->addStretch(1);
}

void FilterPanel::general_() {
    QVBoxLayout* lay = page_("General");
    QWidget* page = lay->parentWidget();
    Header* h = header_(page, "Mix with Original", "mix_enable", "adv", nullptr, false);   // binary: Beginner = Performance only
    // self.controls["mix_enable"].edited.disconnect(); ... connect(self._mix_enable)
    disconnect(enable_conn_["mix_enable"]);
    connect(check("mix_enable"), &EnableCheck::edited, this, [this](QString k, bool on) { mix_enable_(k, on); });
    auto* inline_ = new Row({"mix"}, QString(""), "inv", "", {5, 20});
    inline_->setToolTip("Mix (genuine meaning: higher = more of the filter's effect); shown as 100 - the backend's mix "
                        "with the original");
    Header* hm = header_(h, "Mix", "", "both", inline_, true, false, 1);
    reg_(hm->arrow, "adv");
    for (const auto& [c, nm] : UI_CHANNELS) row_(hm, "mix_" + c, QString::fromStdString(nm), "amount", "dot:" + c, {5, 20}, 1, "adv");
    h = header_(page, "Performance", "", "both", nullptr, false);
    perf = new QLabel(QString::fromUtf8("—"));
    perf->setTextFormat(Qt::RichText);
    perf->setFrameShape(QFrame::StyledPanel);
    perf->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    perf->setStyleSheet("background:white; padding:4px");
    perf->setMinimumHeight(200);
    perf->setWordWrap(true);
    h->add(perf);
    auto* row = new QWidget();
    auto* rl = new QHBoxLayout(row);
    rl->setContentsMargins(0, 4, 0, 0);
    b_speed = new QPushButton("Check Speed");
    b_speed->setToolTip("Process the current frame once with the current settings and show frames/sec");
    b_prefs = new QPushButton("Preferences...");
    connect(b_speed, &QPushButton::clicked, this, &FilterPanel::checkSpeed);
    connect(b_prefs, &QPushButton::clicked, this, &FilterPanel::preferences);
    rl->addWidget(b_speed);
    rl->addStretch(1);
    rl->addWidget(b_prefs);
    h->add(row);
    speed = new QLabel("");
    speed->setFrameShape(QFrame::StyledPanel);
    h->add(speed);
    lay->addStretch(1);
}

// ---------------- behaviour ----------------
Row* FilterPanel::row(const std::string& key) const {
    auto it = controls.find(key);
    return it == controls.end() ? nullptr : qobject_cast<Row*>(it->second);
}
EnableCheck* FilterPanel::check(const std::string& key) const {
    auto it = controls.find(key);
    return it == controls.end() ? nullptr : qobject_cast<EnableCheck*>(it->second);
}
ChoiceRow* FilterPanel::choice(const std::string& key) const {
    auto it = controls.find(key);
    return it == controls.end() ? nullptr : qobject_cast<ChoiceRow*>(it->second);
}

void FilterPanel::mix_enable_(const QString& k, bool on) {
    // Enable with mix 0 (filter only) starts at the genuine default 75% (backend mix 25)
    if (on && params.get("mix").as_double() == 0.0) {
        QVariantMap m;
        m["mix_enable"] = true;
        m["mix"] = 25.0;
        emit editedMany(m);
    } else {
        emit edited(k, QVariant(on));
    }
}

void FilterPanel::set_mode(bool adv) {
    advanced = adv;
    const std::string mode = adv ? "adv" : "beg";
    for (const auto& [w, modes] : modes_) w->setVisible(modes.find(mode) != std::string::npos || modes == "both");
    for (Row* r : sharp_rows) r->label->setText(r->property(adv ? "adv_name" : "beg_name").toString());
    mode_lbl->setText(mode_text(adv));
}

void FilterPanel::set_band_availability(const PyObj* levels) {
    // nv.spatial_v5 gives a plane with L levels the finest L band names for luma and the coarsest L for chroma
    used.clear();
    const int nb = static_cast<int>(V5_BANDS.size());
    for (const auto& [c, _] : UI_CHANNELS) {
        const int L = levels && levels->has(c) ? static_cast<int>(levels->get(c).as_int()) : nb;
        std::set<std::string> u;
        for (int j = 0; j < nb; ++j)
            if (c == "Y" ? j < L : j >= nb - L) u.insert(V5_BANDS[static_cast<size_t>(j)]);
        used[c] = u;
        for (const auto& [b, _b] : UI_BANDS)
            for (const std::string& k : {"bamt_" + c + "_" + b, "bnoise_" + c + "_" + b}) {
                Row* w = row(k);
                w->setEnabled(u.count(b) != 0);
                w->slider->setToolTip(u.count(b) ? QString::fromStdString(spec(k).tip)
                                                 : QString("Not used with this profile: %1 has %2 frequency bands (chroma at "
                                                           "half resolution in 4:2:0)")
                                                       .arg(QString::fromStdString(c))
                                                       .arg(L));
            }
    }
    for (Row* w : common) {
        bool any = false;
        for (const auto& [c, u] : used) any = any || u.count(band_of_[w]);
        w->setEnabled(any);
    }
}

void FilterPanel::set_values(const PyObj& p, std::optional<QStringList> keys_in) {
    params = p;
    std::set<std::string> keys;
    if (!keys_in) {
        for (const auto& k : p.keys()) keys.insert(k);
    } else {
        for (const QString& k : *keys_in) keys.insert(k.toStdString());
    }
    for (const auto& k : keys) {
        std::vector<QWidget*> ws;
        if (controls.count(k)) ws.push_back(controls[k]);
        if (extra.count(k)) ws.insert(ws.end(), extra[k].begin(), extra[k].end());
        for (QWidget* w : ws) {
            if (auto* r = qobject_cast<Row*>(w))
                r->set_value(p.get(k).as_double());
            else if (auto* e = qobject_cast<EnableCheck*>(w))
                e->set_value(p.get(k).truthy());
            else if (auto* c = qobject_cast<ChoiceRow*>(w))
                c->set_value(QString::fromStdString(p.get(k).as_str()));
        }
    }
    for (Row* w : common) {
        bool hit = false;
        for (const auto& k : w->keys) hit = hit || keys.count(k);
        if (!hit) continue;
        std::vector<double> vals;
        for (const auto& k : w->keys) vals.push_back(p.get(k).as_double());
        bool mixed = false;
        for (double x : vals) mixed = mixed || std::fabs(x - vals[0]) > 1e-9;
        w->set_value(vals[0], mixed);
    }
    const bool on = p.get("mix_enable").truthy();
    Row* mix = row("mix");
    mix->setEnabled(on);
    if (!on && p.get("mix").as_double() == 0.0) {   // disabled: show the value Enable would start with (75%)
        mix->set_value(25.0);
        mix->reset->setVisible(false);
    }
}

void FilterPanel::set_performance(const QString& html) { perf->setText(html); }

void FilterPanel::set_nlc_active(bool on) {
    nl_check->set_checked(on);
    if (on == nlc_on_) return;
    nlc_on_ = on;
    for (QWidget* w : nlc_hl_) {
        auto* hd = qobject_cast<Header*>(w);
        QLabel* lab = hd ? hd->title : qobject_cast<Row*>(w)->label;
        if (on) {
            bool known = false;
            for (const auto& t : nlc_text_) known = known || std::get<0>(t) == lab;
            if (!known) nlc_text_.emplace_back(lab, lab->text(), lab->styleSheet());
            if (hd) {
                const QString t = "<b>" + hd->title_text + "</b>";
                QString txt = lab->text();
                const int i = txt.indexOf(t);
                if (i >= 0) txt.replace(i, t.size(), QString("<span style='color:%1'>%2</span>").arg(ACTIVE_TITLE, t));
                lab->setText(txt);
            } else {
                lab->setStyleSheet(QString("color:%1").arg(ACTIVE_TITLE));
            }
        }
    }
    if (!on) {
        for (const auto& [lab, text, css] : nlc_text_) {
            lab->setText(text);
            lab->setStyleSheet(css);
        }
        nlc_text_.clear();
    }
    if (on) {                                   // disable only what is enabled by itself (n/a rows stay off)
        nlc_dis_.clear();
        for (QWidget* w : nlc_off_)
            if (!w->testAttribute(Qt::WA_ForceDisabled)) nlc_dis_.push_back(w);
    }
    for (QWidget* w : nlc_dis_) w->setEnabled(!on);
    if (!on) nlc_dis_.clear();
    for (int i : {1, 2}) tabs->setTabEnabled(i, !on);
    if (on) {
        tabs->setCurrentIndex(0);
        t_noise_header->set_expanded(true);
    }
}

}  // namespace nvqt
