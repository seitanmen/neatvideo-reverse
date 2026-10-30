// mainwindow - see mainwindow.h (port of repro/nv_ui_qt/main_window.py).
#include "mainwindow.h"

#include <QActionGroup>
#include <QTextEdit>
#include <QSpinBox>
#include <QListWidget>
#include <QLineEdit>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QDialog>
#include <QCheckBox>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTextStream>
#include <QThread>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>

#include <QRegularExpression>

#include "noise_graph.h"
#include "profile_panel.h"
#include "rows.h"
#include "timeline.h"

namespace nvqt {

namespace {

const char* N_A = "Not available in the reproduction";
const QString PROFILE_FILTER = "Repro noise profile (*.nvprof.json);;JSON (*.json)";
const QString PRESET_FILTER = "Repro filter preset (*.nvpreset.json);;JSON (*.json)";
const std::vector<std::string> PROFILE_MODES = {"auto", "spatial", "temporal"};
// Preferences > Bit depth: (label, --depth value); 8-bit = the original yuv420p path (main_window.DEPTHS)
const std::vector<std::pair<std::string, std::string>> DEPTHS = {
    {"8-bit (legacy yuv420p)", "8"}, {"Source depth", "source"}, {"10-bit", "10"}, {"12-bit", "12"},
    {"16-bit", "16"}, {"32-bit float (RGB)", "32"}};
const std::vector<std::string> SPATIAL_PREFIXES = {"s_", "amt_", "bamt_", "bnoise_", "noise_", "adv_noise_level", "art_",
                                                   "edge_", "sharp_", "adv_spatial"};

const std::map<int, std::string> FLAT_STATUS = {{0, "ok"},
                                                {1, "no square found (fallback)"},
                                                {2, "small square (< 128 px)"},
                                                {3, "not flat (details in the square)"},
                                                {4, "clipped pixels in the square"},
                                                {5, "cancelled (no profile built)"}};

QString U(const char* s) { return QString::fromUtf8(s); }
QString Q(const std::string& s) { return QString::fromStdString(s); }

std::string f1(double v) {
    char b[64];
    std::snprintf(b, sizeof(b), "%.1f", v);
    return b;
}
std::string f2(double v) {
    char b[64];
    std::snprintf(b, sizeof(b), "%.2f", v);
    return b;
}

// main_window.GUIDES
const std::map<std::string, std::pair<std::string, std::string>> GUIDES = {
    {"filter",
     {"Filter Settings Check",
      "On the Adjust and Preview page, hold the left mouse button in the viewer to see the original "
      "frame; release it to see the filtered one. Check a detailed and a flat part of the frame "
      "(also in the Y / Cr / Cb Enhanced panes).\n\n"
      "• Residual noise in flat areas → raise Noise Level (Overall, Luminance / Cr / Cb or per band) "
      "or the Noise Reduction Amount.\n"
      "• Lost detail or waxy texture → lower the Noise Reduction Amount of the High / Mid frequencies, "
      "or the Noise Level.\n"
      "• Filter Tuning Assist (Spatial tab) shows what is removed: structure (edges, texture) in the "
      "Assist image means the filter is too strong.\n\n(Guide text written for the reproduction.)"}},
    {"tuning",
     {"Quick Start Guide",
      "1. Prepare Noise Profile: Auto Profile (a frame with a flat, unclipped area), or drag a rectangle "
      "over a flat area (at least 128 x 128 recommended, 32 x 32 minimum) and Build Profile.\n"
      "2. Profile Check (Begin): the viewer shows the Assist Image (repro: the frame through the "
      "spatial filter at 100%); adjust Noise Level until the noise is gone while details remain. "
      "Profile > Profile Viewer… shows the noise curve (sigma should vary smoothly with brightness).\n"
      "3. Advanced Mode: Auto Fine-Tune adjusts the per-brightness factors (static port of the genuine "
      "AFT); drag the knots of the equalizer for Manual Fine-Tune, Auto Complete interpolates the "
      "bins you did not touch.\n"
      "4. Adjust and Preview: tune Temporal / Spatial settings; hold the mouse button in the viewer "
      "to compare with the original; Apply processes the full video.\n\n"
      "(Guide text written for the reproduction.)"}}};

Rect area_rect(const PyObj& area) {
    const int y = static_cast<int>(area[0].as_int()), x = static_cast<int>(area[1].as_int()),
              h = static_cast<int>(area[2].as_int());
    return {y, x, h, area.size() == 4 ? static_cast<int>(area[3].as_int()) : h};
}

std::string rect_key(const std::optional<Rect>& r) {
    if (!r) return "None";
    return std::to_string(r->y0) + "," + std::to_string(r->x0) + "," + std::to_string(r->h) + "," + std::to_string(r->w);
}

QToolButton* tool(const QString& text, const QString& tip, bool checkable = false, QStyle* st = nullptr,
                  QStyle::StandardPixmap icon = QStyle::SP_CustomBase) {
    auto* b = new QToolButton();
    b->setText(text);
    b->setToolTip(tip);
    b->setCheckable(checkable);
    if (st && icon != QStyle::SP_CustomBase) {
        b->setIcon(st->standardIcon(icon));
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    }
    b->setStyleSheet("QToolButton{padding:3px 8px}");
    return b;
}

bool write_file(const QString& path, const std::string& text) {
    std::ofstream f(path.toStdString(), std::ios::binary);
    f << text;
    return static_cast<bool>(f);
}

PyObj read_json(const QString& path) {
    std::ifstream f(path.toStdString(), std::ios::binary);
    if (!f) throw std::runtime_error("[Errno 2] No such file or directory: '" + path.toStdString() + "'");
    std::stringstream ss;
    ss << f.rdbuf();
    return PyObj::parse(ss.str());
}

}  // namespace

std::unique_ptr<QSettings> settings() {
    const QByteArray p = qgetenv("NV_UI_QT_SETTINGS");
    if (!p.isEmpty()) return std::make_unique<QSettings>(QString::fromLocal8Bit(p), QSettings::IniFormat);
    return std::make_unique<QSettings>(QSettings::IniFormat, QSettings::UserScope, "NeatRepro", "nv_ui_qt");
}

// =================================================================== construction
MainWindow::MainWindow(const QString& video, QWidget* parent) : QMainWindow(parent) {
    state = new AppState(this);
    runner = new TaskRunner(this);
    load_prefs();
    preset_clean = default_params();
    setWindowTitle(U("Neat Video v6 — reproduction UI (export using ") + QString::number(prefs["jobs"].toInt()) +
                   " CPU processes)");
    resize(1400, 860);
    QStyle* st = style();

    auto* central = new QWidget();
    auto* v = new QVBoxLayout(central);
    v->setContentsMargins(4, 2, 4, 2);
    v->setSpacing(2);
    auto* top = new QHBoxLayout();
    tabs = new QTabBar();
    tabs->setExpanding(false);
    tabs->addTab(st->standardIcon(QStyle::SP_FileDialogContentsView), "Prepare Noise Profile");
    tabs->addTab(st->standardIcon(QStyle::SP_FileDialogDetailedView), "Adjust and Preview");
    tabs->setStyleSheet("QTabBar::tab{padding:5px 14px; font-weight:bold}");
    top->addWidget(tabs);
    top->addStretch(1);
    v->addLayout(top);

    auto* split = new QSplitter(Qt::Horizontal);
    auto* left = new QWidget();
    auto* lv = new QVBoxLayout(left);
    lv->setContentsMargins(0, 0, 0, 0);
    lv->setSpacing(2);
    tb_stack = new QStackedWidget();
    tb_stack->addWidget(prepare_toolbar());
    tb_stack->addWidget(adjust_toolbar());
    tb_stack->setFixedHeight(tb_stack->sizeHint().height());
    lv->addWidget(tb_stack);
    viewer = new Viewer();
    lv->addWidget(viewer, 1);
    viewer_bar = new ViewerBar(viewer);
    lv->addWidget(viewer_bar);
    split->addWidget(left);
    stack = new QStackedWidget();
    profile_panel = new ProfilePanel();
    filter_panel = new FilterPanel();
    {
        auto* pp = new QWidget();
        auto* l = new QVBoxLayout(pp);
        l->setContentsMargins(0, 0, 0, 0);
        l->addWidget(profile_panel);
        stack->addWidget(pp);
        auto* fp = new QWidget();
        auto* l2 = new QVBoxLayout(fp);
        l2->setContentsMargins(0, 0, 0, 0);
        l2->addWidget(filter_panel);
        stack->addWidget(fp);
    }
    stack->setMinimumWidth(320);
    split->addWidget(stack);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 0);
    split->setSizes({1040, 350});
    v->addWidget(split, 1);

    auto* bottom = new QHBoxLayout();
    bottom->setSpacing(4);
    const std::vector<std::pair<QString, QString>> bb = {
        {U("P ▾"), "Video Scan Type: Progressive (n/a: the reproduction processes progressive frames only)"},
        {U("γ ▾"), "Input Data Gamma: Gamma Corrected (n/a)"},
        {U("x1.0 ▾"), "Input Data Gain: x1.0 (n/a: the v5 backend has no input gain)"}};
    for (const auto& [t, tip] : bb) {
        auto* b = new QToolButton();
        b->setText(t);
        b->setToolTip(tip);
        b->setEnabled(false);
        bottom->addWidget(b, 0, Qt::AlignBottom);
    }
    timeline = new Timeline();
    timeline->thumbs->provider = [this](int i) { return thumb(i); };
    bottom->addWidget(timeline, 1);
    b_cancel = new QPushButton("Cancel");
    b_cancel->setToolTip("Do not apply any changes and return (repro: closes the window)");
    connect(b_cancel, &QPushButton::clicked, this, &QWidget::close);
    b_apply = new QPushButton("Apply");
    b_apply->setDefault(true);
    b_apply->setToolTip("Apply the current filter settings to the preview (genuine OFX: return to host with filtered frame)");
    connect(b_apply, &QPushButton::clicked, this, [this] {
        invalidate();
        schedule();
        say("filter settings applied to preview");
    });
    for (QPushButton* b : {b_cancel, b_apply}) {
        b->setMinimumWidth(90);
        bottom->addWidget(b, 0, Qt::AlignBottom);
    }
    v->addLayout(bottom);
    setCentralWidget(central);

    QStatusBar* sb = statusBar();
    msg = new QLabel(U("Open a video (Effect > Open Input Video…)"));
    sb->addWidget(msg, 1);
    prog = new QProgressBar();
    prog->setMaximumWidth(200);
    prog->setVisible(false);
    sb->addPermanentWidget(prog);
    cancel_btn = new QPushButton("Cancel export");
    cancel_btn->setVisible(false);
    sb->addPermanentWidget(cancel_btn);
    video_lbl = new QLabel("");
    sb->addPermanentWidget(video_lbl);
    depth_lbl = new QLabel("");
    sb->addPermanentWidget(depth_lbl);
    prof_lbl = new QLabel("no profile");
    sb->addPermanentWidget(prof_lbl);

    build_menus();
    addAction(actions["undo"]);
    addAction(actions["redo"]);

    for (IconBar* ib : {filter_panel->icons, profile_panel->icons})
        ib->bind_undo(actions["undo"], actions["redo"], state->undo);
    FilterPanel* fp = filter_panel;
    connect(fp, &FilterPanel::linkActivated, this, [this](QString h) { link(h); });
    connect(fp, &FilterPanel::edited, this, [this](QString k, QVariant v) {
        PyObj val;
        if (v.typeId() == QMetaType::Bool)
            val = PyObj::boolean(v.toBool());
        else if (v.typeId() == QMetaType::QString)
            val = PyObj::str(v.toString().toStdString());
        else
            val = PyObj::real(v.toDouble());
        state->set_param(k.toStdString(), val);
    });
    connect(fp, &FilterPanel::editedMany, this, [this](QVariantMap d) {
        std::vector<std::pair<std::string, PyObj>> u;
        for (auto it = d.begin(); it != d.end(); ++it)
            u.emplace_back(it.key().toStdString(), it.value().typeId() == QMetaType::Bool ? PyObj::boolean(it.value().toBool())
                                                                                           : PyObj::real(it.value().toDouble()));
        state->set_params(u, "Change filter settings");
    });
    connect(fp, &FilterPanel::editStarted, this, [this] { state->new_edit(); });
    connect(fp, &FilterPanel::assistToggled, this, [this](bool on) { set_assist(on); });
    connect(fp, &FilterPanel::noiseLevelCheck, this, [this](bool on) { set_noise_level_check(on); });
    connect(fp, &FilterPanel::resetSpatial, this, [this] { reset_spatial(); });
    connect(fp, &FilterPanel::checkSpeed, this, [this] { check_speed(); });
    connect(fp, &FilterPanel::preferences, this, [this] { preferences(); });
    IconBar* fi = filter_panel->icons;
    connect(fi->open, &QToolButton::clicked, this, [this] { load_preset(); });
    connect(fi->save, &QToolButton::clicked, this, [this] { save_preset(); });
    connect(fi->reset, &QToolButton::clicked, this, [this] { reset_filter(); });
    connect(fi->info, &QToolButton::clicked, this, [this] { preset_description(); });
    for (const char* k : {"f_assign", "f_reset_default", "f_change_default"}) fi->default_menu->addAction(actions[k]);
    IconBar* pi = profile_panel->icons;
    connect(pi->open, &QToolButton::clicked, this, [this] { load_profile(); });
    connect(pi->save, &QToolButton::clicked, this, [this] { save_profile(); });
    connect(pi->reset, &QToolButton::clicked, this, [this] { reset_profile(); });
    connect(pi->info, &QToolButton::clicked, this, [this] { profile_viewer(); });
    for (const char* k : {"p_assign", "p_reset_default", "p_change_default"}) pi->default_menu->addAction(actions[k]);
    fp->set_values(state->params);
    ProfilePanel* pp = profile_panel;
    connect(pp, &ProfilePanel::autoFineTune, this, [this] { auto_fine_tune(); });
    connect(pp, &ProfilePanel::autoComplete, this, [this] { auto_complete(); });
    connect(pp, &ProfilePanel::resetFineTune, this, [this] { reset_fine_tune(); });
    connect(pp, &ProfilePanel::profileCheck, this, [this](bool on) { set_profile_check(on); });
    connect(pp, &ProfilePanel::editDevice, this, [this](QString n) { edit_device(n); });
    connect(pp, &ProfilePanel::tuneEdited, this, [this](QString k, double val) { state->set_tune(k.toStdString(), val); });
    connect(pp, &ProfilePanel::editStarted, this, [this] { state->new_edit(); });
    connect(pp->curve, &CurveEditor::edited, this,
            [this](int c, std::array<double, 9> fs, int i) { state->set_factors(c, fs, i); });
    connect(pp->curve, &CurveEditor::editedAll, this, [this](std::array<double, 9> fs, int i) { state->set_factors_all(fs, i); });
    connect(pp->curve, &CurveEditor::editStarted, this, [this] { state->new_edit(); });
    connect(state, &AppState::paramsChanged, this, &MainWindow::on_params);
    connect(state, &AppState::profileChanged, this, &MainWindow::on_profile);
    connect(tabs, &QTabBar::currentChanged, this, &MainWindow::on_page);
    connect(timeline, &Timeline::frameChanged, this, &MainWindow::on_frame);
    connect(viewer, &Viewer::areaSelected, this, &MainWindow::viewer_area);
    connect(viewer, &Viewer::selectionChanging, this, &MainWindow::viewer_changing);
    connect(viewer, &Viewer::menuRequested, this, [this](QPoint p) { sel_menu(p); });
    connect(viewer, &Viewer::clicked, this, &MainWindow::viewer_click);
    connect(viewer, &Viewer::viewChanged, this, &MainWindow::sync_view_actions);
    connect(viewer_bar->info, &QToolButton::clicked, this, [this] { image_info(); });
    timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(300);
    connect(timer, &QTimer::timeout, this, &MainWindow::request_preview);
    sel_timer = new QTimer(this);
    sel_timer->setSingleShot(true);
    sel_timer->setInterval(100);
    connect(sel_timer, &QTimer::timeout, this, &MainWindow::sync_selection);
    pc_timer = new QTimer(this);
    pc_timer->setSingleShot(true);
    pc_timer->setInterval(100);
    connect(pc_timer, &QTimer::timeout, this, &MainWindow::request_pcheck);
    nlc_timer = new QTimer(this);            // mask after a temporal setting change (thinned out)
    nlc_timer->setSingleShot(true);
    nlc_timer->setInterval(100);
    connect(nlc_timer, &QTimer::timeout, this, &MainWindow::request_nlc);
    connect(cancel_btn, &QPushButton::clicked, this, [this] { cancel_export(); });

    const QVariant dp = settings()->value("default_preset");
    if (dp.isValid() && !dp.toString().isEmpty()) {
        try {
            state->params = params_from_json(PyObj::parse(dp.toString().toStdString()));
            preset_clean = state->params;
            fp->set_values(state->params);
        } catch (const std::exception& e) {
            say(Q(std::string("default preset ignored: ") + e.what()));
        }
    }
    const QVariant adv = settings()->value("ui/advanced");
    set_advanced(!adv.isValid() || adv.toString().toLower() == "1" || adv.toString().toLower() == "true");
    sync_view_actions();
    QStringList all;
    for (const auto& kv : param_specs()) all << Q(kv.first);
    on_params(all);
    on_profile();
    on_page(0);
    if (!video.isEmpty()) open_video(video);
}

MainWindow::~MainWindow() { runner->wait_all(); }

void MainWindow::load_prefs() {
    const unsigned hc = std::thread::hardware_concurrency();
    prefs = {{"jobs", std::max(1, static_cast<int>(hc ? hc : 2) - 2)}, {"crf", 16}, {"audio", "copy"},
             {"preview_area", 0.5}, {"auto_fine_tune", true}, {"mem_gb", 4.0}, {"auto_profile", true},
             {"depth", "8"}, {"profile_mode", "auto"}, {"native", true}};
    auto s = settings();
    for (auto& [k, d] : prefs) {
        const QVariant v = s->value(Q("prefs/" + k));
        if (!v.isValid()) continue;
        if (d.typeId() == QMetaType::Bool) {
            d = v.toString().toLower() == "1" || v.toString().toLower() == "true";
        } else if (d.typeId() == QMetaType::Int) {
            bool ok = false;
            const int x = v.toString().toInt(&ok);
            if (ok) d = x;
        } else if (d.typeId() == QMetaType::Double) {
            bool ok = false;
            const double x = v.toString().toDouble(&ok);
            if (ok) d = x;
        } else {
            d = v.toString();
        }
    }
}

std::string MainWindow::profile_mode() const {
    const std::string m = prefs.at("profile_mode").toString().toStdString();
    return std::find(PROFILE_MODES.begin(), PROFILE_MODES.end(), m) != PROFILE_MODES.end() ? m : "auto";
}

// ---------------------------------------------------------------- toolbars
QWidget* MainWindow::prepare_toolbar() {
    QStyle* st = style();
    auto* w = new QWidget();
    auto* h = new QHBoxLayout(w);
    h->setContentsMargins(0, 0, 0, 0);
    auto_btn = tool("Auto Profile", "Automatically build a device noise profile using the input image (click: Most "
                    "Noisy Frame; menu: Current Frame)", false, st, QStyle::SP_BrowserReload);
    auto_btn->setPopupMode(QToolButton::MenuButtonPopup);
    auto_menu = new QMenu(auto_btn);
    auto_btn->setMenu(auto_menu);
    connect(auto_btn, &QToolButton::clicked, this, &MainWindow::auto_clicked);
    load_btn = tool("Load Profile", "Load a noise profile (repro JSON)", false, st, QStyle::SP_DirOpenIcon);
    connect(load_btn, &QToolButton::clicked, this, [this] { load_profile(); });
    generic_btn = tool("Generic Profile", U("repro: no genuine generic profile — loads the Default Profile (Profile "
                                            "> Assign As Default Profile), or resets the profile if none"),
                       false, st, QStyle::SP_FileIcon);
    connect(generic_btn, &QToolButton::clicked, this, [this] { generic_profile(); });
    check_btn = tool("Profile Check", "Profile Check: shows the Assist Image (a visually enhanced, overprocessed preview) "
                     "to find the optimal Noise Level; click again to finish (repro: spatial filter only, 100% amounts)",
                     true, st, QStyle::SP_FileDialogInfoView);
    check_btn->setStyleSheet("QToolButton{padding:3px 8px} QToolButton:checked{background:#3d5fd0; "
                             "color:white; border:1px solid #2a3f90; border-radius:3px}");
    connect(check_btn, &QToolButton::toggled, this, [this](bool on) { set_profile_check(on); });
    for (QToolButton* b : {auto_btn, generic_btn, load_btn}) h->addWidget(b);
    h->addStretch(1);
    h->addWidget(check_btn);
    variants_btn = tool("Variants", QString(N_A) + " (Variants: alternative filter settings)");
    variants_btn->setEnabled(false);
    h->addWidget(variants_btn);
    update_profile_button();
    return w;
}

QWidget* MainWindow::adjust_toolbar() {
    QStyle* st = style();
    auto* w = new QWidget();
    auto* h = new QHBoxLayout(w);
    h->setContentsMargins(0, 0, 0, 0);
    area_btn = tool(U("⛶"), "Preview area: process only the selected rectangle (drag in the viewer); without a "
                    "selection the central part (Preferences: Preview Area) is selected. Off = the whole frame", true);
    connect(area_btn, &QToolButton::toggled, this, &MainWindow::area_toggled);
    play_btn = tool("Play All", "Start/Stop Playback (preview is not filtered while playing)", true, st, QStyle::SP_MediaPlay);
    play_btn->setPopupMode(QToolButton::MenuButtonPopup);
    auto* m = new QMenu(play_btn);
    m->addAction("&Play", [this] { play_btn->setChecked(true); });
    m->addAction("Stop (&Current Frame)", [this] { play_btn->setChecked(false); });
    m->addAction("Stop (Initia&l Frame)", [this] { stop_initial(); });
    play_btn->setMenu(m);
    connect(play_btn, &QToolButton::toggled, this, &MainWindow::play);
    preview_btn = tool("Preview", "Show preview/original image", true, st, QStyle::SP_DesktopIcon);
    preview_btn->setChecked(true);
    connect(preview_btn, &QToolButton::toggled, this, [this](bool) {
        display();
        schedule();
    });
    fsc_btn = tool("Filter Settings Check", "Filter Settings Check tools (repro: Noise Level Check and Spatial Filter "
                   "Tuning = Filter Tuning Assist are available)", true, st, QStyle::SP_FileDialogInfoView);
    fsc_btn->setStyleSheet(check_btn->styleSheet());
    fsc_btn->setPopupMode(QToolButton::InstantPopup);
    connect(fsc_btn, &QToolButton::toggled, this, [this](bool) { sync_nlc_widgets(); });   // the state is nlc, not a click
    m = new QMenu(fsc_btn);
    fsc_nlc = m->addAction("&1. Noise Level Check");
    fsc_nlc->setCheckable(true);
    connect(fsc_nlc, &QAction::toggled, this, [this](bool on) { set_noise_level_check(on); });
    int i = 2;
    for (const char* name : {"Local Flicker Check", "Repeated Frames Check"}) {
        QAction* a = m->addAction(QString("&%1. %2").arg(i++).arg(name));
        a->setEnabled(false);
        a->setToolTip(N_A);
    }
    fsc_assist = m->addAction("&4. Spatial Filter Tuning");
    fsc_assist->setCheckable(true);
    connect(fsc_assist, &QAction::toggled, this, [this](bool on) { set_assist(on); });
    m->addSeparator();
    m->addAction("&Finish", [this] {
        set_assist(false);
        set_noise_level_check(false);
    });
    fsc_btn->setMenu(m);
    for (QToolButton* b : {area_btn, preview_btn, play_btn}) h->addWidget(b);
    h->addStretch(1);
    h->addWidget(fsc_btn);
    auto* vb = tool("Variants", QString(N_A) + " (Variants: alternative filter settings)");
    vb->setEnabled(false);
    h->addWidget(vb);
    return w;
}

void MainWindow::update_profile_button() {
    const bool build = sel_area.has_value();
    const QString what = build ? "Build Profile" : "Auto Profile";
    auto_btn->setText(what);
    auto_menu->clear();
    auto_menu->addAction(what + " (Most Noisy Frame)", [this, build] { build ? (void)build_profile(true) : auto_profile(true); });
    auto_menu->addAction(what + " (Current Frame)", [this, build] { build ? (void)build_profile(false) : auto_profile(false); });
    const bool small = build && std::min(sel_area->h, sel_area->w) < 32;
    auto_btn->setEnabled(!small && busy_slots.empty());
    auto_btn->setToolTip(build ? "Build a device noise profile from the selected area (click: Current Frame)"
                               : "Automatically build a device noise profile using the input image (click: Most Noisy "
                                 "Frame; menu: Current Frame)");
}

void MainWindow::auto_clicked() {
    if (sel_area)
        build_profile(false);
    else
        auto_profile(true);
}

// ---------------------------------------------------------------- menus (menus.py)
void MainWindow::build_menus() {
    QMenuBar* mb = menuBar();
    auto add = [&](QMenu* menu, const std::string& key, const QString& text, std::function<void()> slot,
                   const QKeySequence& sc = QKeySequence(), bool enabled = true, const QString& tip = QString(),
                   bool checkable = false, std::function<void(bool)> tslot = nullptr) {
        auto* a = new QAction(text, this);
        if (!sc.isEmpty()) a->setShortcut(sc);
        if (!tip.isEmpty()) {
            a->setStatusTip(tip);
            a->setToolTip(tip);
        }
        a->setEnabled(enabled);
        a->setCheckable(checkable);
        if (checkable && tslot)
            connect(a, &QAction::toggled, this, tslot);
        else if (slot)
            connect(a, &QAction::triggered, this, [slot](bool) { slot(); });
        menu->addAction(a);
        actions[key] = a;
        return a;
    };
    auto na = [&](QMenu* menu, const QString& text, const QString& shortcut = QString(), const QString& why = N_A) {
        QAction* a = menu->addAction(text);
        a->setEnabled(false);
        a->setToolTip(why);
        a->setStatusTip(why);
        if (!shortcut.isEmpty()) a->setText(text + "\t" + shortcut);
        return a;
    };

    QMenu* m = mb->addMenu("E&ffect");
    add(m, "copy_both", "Copy Profile and Preset", [this] { copy_both(); }, QKeySequence("Shift+Ctrl+C"));
    add(m, "paste_both", "Paste Profile and Preset", [this] { paste_both(); }, QKeySequence("Shift+Ctrl+V"));
    na(m, "Skip Neat Video Processing");
    add(m, "apply", "Apply\tReturn", [this] { b_apply->click(); }, QKeySequence(), true, b_apply->toolTip());
    add(m, "cancel", "Cancel", [this] { b_cancel->click(); }, QKeySequence(), true,
        "Close the window (the genuine plug-in returns to the host)");
    m->addSeparator();
    m->addSection("Reproduction");
    add(m, "open", U("Open Input Video…"), [this] { open_video(); }, QKeySequence(QKeySequence::Open), true,
        "repro: load a video file");
    add(m, "save_frame", U("Save Output Image As…"), [this] { save_frame(); }, QKeySequence(), true,
        "Save the primary viewer pane as PNG");
    add(m, "export", "Export Full Video...", [this] { process_video(); }, QKeySequence("Ctrl+E"), true,
        "Process the full video with the current settings");

    QAction* u = state->undo->createUndoAction(this, "Undo Last Change");
    u->setShortcut(QKeySequence::Undo);
    QAction* r = state->undo->createRedoAction(this, "Redo Last Change");
    r->setShortcuts({QKeySequence("Shift+Ctrl+Z"), QKeySequence("Ctrl+Y")});
    connect(state->undo, &QUndoStack::undoTextChanged, u, [u](const QString& t) {
        u->setText("Undo Last Change");
        u->setToolTip(t);
    });
    connect(state->undo, &QUndoStack::redoTextChanged, r, [r](const QString& t) {
        r->setText("Redo Last Change");
        r->setToolTip(t);
    });
    actions["undo"] = u;
    actions["redo"] = r;

    m = mb->addMenu("&View");
    QMenu* z = m->addMenu("&Zoom");
    add(z, "zoom_fit", "Zoom to Fit", [this] { viewer->set_zoom(0); }, QKeySequence("Ctrl+0"));
    add(z, "zoom_in", "Zoom In", [this] { viewer->zoom_step(1); }, QKeySequence("Ctrl++"));
    add(z, "zoom_out", "Zoom Out", [this] { viewer->zoom_step(-1); }, QKeySequence("Ctrl+-"));
    z->addSeparator();
    for (double p : ZOOMS) {
        char b[32];
        std::snprintf(b, sizeof(b), "%g", p);
        add(z, std::string("zoom_") + b, QString(b) + "%", [this, p] { viewer->set_zoom(p); },
            p == 100 ? QKeySequence("Ctrl+Alt+0") : QKeySequence());
    }
    QMenu* vm = m->addMenu("&Viewer Mode");
    auto* g = new QActionGroup(this);
    for (size_t i = 0; i < MODES.size(); ++i) {
        auto sec = MODE_SECTIONS.find(static_cast<int>(i));
        if (sec != MODE_SECTIONS.end()) vm->addSection(Q(sec->second));
        const std::string sh = MODES[i].first;
        QAction* a = add(vm, "mode_" + sh, Q(MODES[i].second), [this, sh] { viewer->set_mode(sh); },
                         QKeySequence(QString("Ctrl+%1").arg(i + 1)));
        a->setCheckable(true);
        g->addAction(a);
    }
    vm->addSection("Viewer Split");
    g = new QActionGroup(this);
    for (const auto& s : SPLITS) {
        QAction* a = add(vm, "split_" + s, Q(s), [this, s] { viewer->set_split(s); });
        a->setCheckable(true);
        g->addAction(a);
    }
    add(vm, "split_next", "Next Split Mode", [this] { cycle_split(1); }, QKeySequence("Ctrl+]"));
    add(vm, "split_prev", "Previous Split Mode", [this] { cycle_split(-1); }, QKeySequence("Ctrl+["));
    m->addSeparator();
    add(m, "show_preview", "Show Preview", [this] { preview_btn->toggle(); }, QKeySequence("Alt+H"), true, "Toggle Auto Preview");
    add(m, "pan", "Toggle Panning Mode", [this] { viewer_bar->pan->toggle(); }, QKeySequence("Ctrl+D"));
    add(m, "play", "Start Playback", [this] { play_btn->toggle(); }, QKeySequence("Ctrl+P"));

    m = mb->addMenu("&Profile");
    m->addAction(u);
    m->addAction(r);
    m->addSeparator();
    add(m, "p_load", U("&Load Profile…"), [this] { load_profile(); });
    add(m, "p_save", U("&Save As…"), [this] { save_profile(); });
    add(m, "p_reset", "&Reset Profile", [this] { reset_profile(); });
    m->addSeparator();
    add(m, "p_copy", "Copy Profile", [this] { copy_profile(); }, QKeySequence(QKeySequence::Copy));
    add(m, "p_paste", "Paste Profile", [this] { paste_profile(); }, QKeySequence(QKeySequence::Paste));
    m->addSeparator();
    na(m, U("Save As Default Profile…"));
    add(m, "p_assign", "Assign As Default Profile", [this] { assign_default_profile(); });
    add(m, "p_reset_default", "Reset Default Profile", [this] { reset_default_profile(); });
    add(m, "p_change_default", U("Change Default Profile…"), [this] { change_default_profile(); });
    m->addSeparator();
    add(m, "p_auto_noisy", "Auto Profile (Most Noisy Frame)", [this] { auto_profile(true); }, QKeySequence("F2"));
    add(m, "p_auto", "Auto Profile (Current Frame)", [this] { auto_profile(false); }, QKeySequence("Shift+F2"));
    add(m, "p_generic", "Generic Profile", [this] { generic_profile(); }, QKeySequence("F6"));
    m->addSeparator();
    add(m, "p_viewer", U("Profile Viewer…"), [this] { profile_viewer(); }, QKeySequence("Ctrl+Shift+I"), true,
        "Profile viewer: details of current noise profile");
    m->addSeparator();
    m->addSection("Reproduction");
    add(m, "p_build_noisy", "Build Profile (Most Noisy Frame)", [this] { build_profile(true); });
    add(m, "p_build", "Build Profile (Current Frame)", [this] { build_profile(false); });
    add(m, "p_aft", "Auto Fine-Tune", [this] { auto_fine_tune(); }, QKeySequence("Ctrl+F"));
    add(m, "p_complete", "Auto Com&plete", [this] { auto_complete(); });
    add(m, "p_reset_ft", "R&eset Fine-Tuning Results", [this] { reset_fine_tune(); });
    add(m, "p_check", "Profile Check", nullptr, QKeySequence("Ctrl+K"), true,
        "Profile Check: Assist Image preview to find the optimal Noise Level (same toggle as Begin)", true,
        [this](bool on) { set_profile_check(on); });
    add(m, "p_match", "Auto Match (n/a)", nullptr, QKeySequence("Ctrl+M"), false, "Not available in the reproduction");
    add(m, "p_converter", U("Profile Converter…"), nullptr, QKeySequence(), false,
        "Not available: genuine .dnp profiles are not read or written by the reproduction");

    m = mb->addMenu("F&ilter");
    m->addAction(u);
    m->addAction(r);
    m->addSeparator();
    add(m, "f_load", U("&Load Filter Preset…"), [this] { load_preset(); });
    add(m, "f_save", U("&Save As Filter Preset…"), [this] { save_preset(); });
    add(m, "f_reset", "&Reset Filter Settings", [this] { reset_filter(); });
    add(m, "f_copy", "Copy Filter Settings", [this] { copy_filter(); });
    add(m, "f_paste", "Paste Filter Settings", [this] { paste_filter(); });
    m->addSeparator();
    add(m, "f_assign", "Assign As Default Preset", [this] { assign_default_preset(); });
    add(m, "f_reset_default", "Reset Default Preset", [this] { reset_default_preset(); });
    add(m, "f_change_default", U("Change Default Preset…"), [this] { change_default_preset(); });
    add(m, "f_desc", U("Preset &Description…"), [this] { preset_description(); });
    m->addSeparator();
    add(m, "f_assist", "F&ilter Tuning Assist", nullptr, QKeySequence(), true, QString(), true,
        [this](bool on) { set_assist(on); });
    add(m, "f_check", "Filter Settings &Check", [this] { guide("filter"); });

    m = mb->addMenu("V&ariants");
    na(m, "Variants (alternative filter settings)");

    m = mb->addMenu("T&ools");
    add(m, "batch", U("&Batch Profiler…"), [this] { batch_profiler(); });
    m->addSeparator();
    add(m, "prefs", U("Preferences…"), [this] { preferences(); }, QKeySequence("F10"));
    add(m, "perf", "Performance tab", [this] { performance_tab(); }, QKeySequence("Shift+Ctrl+P"));
    m->addSeparator();
    add(m, "nlc", "Noise Level Check", nullptr, QKeySequence(), true,
        "Noise Level Check: Temporal Filter mask in the viewer (same toggle as Begin on the Temporal tab and Filter "
        "Settings Check > 1. Noise Level Check)", true, [this](bool on) { set_noise_level_check(on); });

    m = mb->addMenu("&Help");
    add(m, "help", "Help", [this] { guide("tuning"); }, QKeySequence("F1"), true, "repro: shows the Quick Start Guide text");
    na(m, "Read Me");
    na(m, "What's New");
    add(m, "qsg", "&Quick Start Guide", [this] { guide("tuning"); });
    na(m, "How to Use Neat Video");
    m->addSeparator();
    na(m, "Web Page");
    na(m, "Community Forum");
    na(m, "Report a Bug Online");
    na(m, "Support via E-mail");
    m->addSeparator();
    add(m, "about", U("About…"), [this] { about(); }, QKeySequence("Alt+F1"));
    add(m, "about_qt", "About Qt", [] { QApplication::aboutQt(); }, QKeySequence(), true,
        "Qt version and licence (the C++ UI uses Qt under the GNU LGPL v3)");

    auto* mode = new QToolButton();
    mode->setPopupMode(QToolButton::InstantPopup);
    mode->setAutoRaise(true);
    mode->setStyleSheet("QToolButton{font-weight:bold; padding:2px 8px}");
    auto* mm = new QMenu(mode);
    g = new QActionGroup(this);
    for (const auto& [key, text, adv] : std::vector<std::tuple<std::string, QString, bool>>{
             {"beginner", "&Beginner Mode", false}, {"advanced", "&Advanced Mode", true}}) {
        auto* a = new QAction(text, this);
        a->setCheckable(true);
        const bool av = adv;
        connect(a, &QAction::triggered, this, [this, av](bool) { set_advanced(av); });
        g->addAction(a);
        mm->addAction(a);
        actions[key] = a;
    }
    mode->setMenu(mm);
    mb->setCornerWidget(mode);
    mode_button_ = mode;
}

// ---------------------------------------------------------------- helpers
void MainWindow::say(const QString& text) { msg->setText(text); }

void MainWindow::link(const QString& href) {
    if (href == "qsg")
        guide("tuning");
    else if (href == "mode")
        set_advanced(!advanced);
}

void MainWindow::set_advanced(bool adv) {
    advanced = adv;
    profile_panel->set_mode(advanced);
    filter_panel->set_mode(advanced);
    actions[advanced ? "advanced" : "beginner"]->setChecked(true);
    mode_button_->setText(QString(advanced ? "Advanced" : "Beginner") + U(" Mode ▾"));
    settings()->setValue("ui/advanced", advanced);
}

int MainWindow::page() const { return tabs->currentIndex(); }

void MainWindow::update_radius() {
    const PyObj& P = state->params;
    timeline->set_radius(P.get("t_enable").truthy() && page() == 1 ? static_cast<int>(P.get("t_radius").as_int()) : 0);
}

void MainWindow::on_page(int i) {
    if (i != 0 && pcheck) set_profile_check(false);       // Profile Check belongs to the Prepare page
    if (i != 1 && nlc) set_noise_level_check(false);      // Noise Level Check belongs to the Adjust page
    update_radius();
    stack->setCurrentIndex(i);
    tb_stack->setCurrentIndex(i);
    if (i != page_) {
        view_modes[page_] = viewer->mode();
        page_ = i;
    }
    viewer->set_mode(view_modes[i]);
    viewer->set_emphasis(i == 0);
    viewer->set_select_limits(i == 0 ? 512 : 1280, i == 0);
    sync_selection();
    display();
    schedule();
}

void MainWindow::sync_view_actions() {
    for (auto& [k, a] : actions) {
        if (k.rfind("mode_", 0) == 0 && k != "mode_button") {
            a->setChecked(k == "mode_" + viewer->mode());
        } else if (k.rfind("split_", 0) == 0) {
            const std::string s = k.substr(6);
            if (std::find(SPLITS.begin(), SPLITS.end(), s) != SPLITS.end()) a->setChecked(k == "split_" + viewer->split());
            a->setEnabled(viewer->mode() != "Normal");
        }
    }
}

void MainWindow::cycle_split(int d) {
    const int n = static_cast<int>(SPLITS.size());
    const int i = static_cast<int>(std::find(SPLITS.begin(), SPLITS.end(), viewer->split()) - SPLITS.begin());
    viewer->set_split(SPLITS[static_cast<size_t>(((i + d) % n + n) % n)]);
}

QString MainWindow::ask_open(const QString& title, const QString& filt) {
    return QFileDialog::getOpenFileName(this, title, "", filt);
}

QString MainWindow::ask_save(const QString& title, const QString& filt, const QString& name) {
    return QFileDialog::getSaveFileName(this, title, name, filt);
}

QWidget* MainWindow::info_box(const QString& title, const QString& text) {
    auto* box = new QMessageBox(QMessageBox::Information, title, text, QMessageBox::Ok, this);
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->open();
    return box;
}

// ---------------------------------------------------------------- video
std::string MainWindow::depth() const {
    // Preferences > Bit depth as a --depth value (an unknown stored value falls back to 8)
    const std::string d = prefs.at("depth").toString().toStdString();
    for (const auto& [label, v] : DEPTHS)
        if (v == d) return d;
    return "8";
}

void MainWindow::open_video(const QString& path_in) {
    const QString path = path_in.isEmpty()
                             ? ask_open("Open video", "Video (*.mp4 *.mov *.mkv *.m4v *.avi *.mts *.ts);;All (*)")
                             : path_in;
    if (path.isEmpty()) return;
    VideoInfo vi;
    try {
        vi = probe(path.toStdString());
    } catch (const std::exception& e) {
        say(Q(std::string("error (load): ") + e.what()));
        return;
    }
    const int W = vi.W, H = vi.H;
    const std::string dep = depth();
    PixFmt pf;
    try {
        pf = choose_format(source_format(vi.pix_fmt), dep);
    } catch (const std::exception& e) {
        say(Q(std::string("error (load): ") + e.what()));
        return;
    }
    const std::string mat = matrix_from_tags(vi.tags, H);
    const std::string vf = pf.legacy() ? std::string() : read_filter(vi, pf, mat);
    const long long fs = pf.internal_bytes(W, H);           // bytes of one frame in memory (uint8 or float32 planes)
    const long long cap = std::max(3LL, static_cast<long long>(prefs["mem_gb"].toDouble() * 1073741824.0) / fs);
    const int n = vi.n;
    const int hi = static_cast<int>(n ? std::min<long long>(n, cap) : cap);
    say(U("loading ") + QFileInfo(path).fileName() + U(" …"));
    prog->setVisible(true);
    const std::string p = path.toStdString();
    const std::string fps = vi.fps;
    runner->submit(
        "load", QString("load"),
        [p, W, H, hi, fps, n, pf, mat, vf](Task& task) -> std::any {
            auto fr = std::make_shared<std::vector<Frame>>();
            auto report = [&](int i) {
                if (i % 10 == 0)
                    task.report(100.0 * i / std::max(n ? std::min(n, hi) : hi, 1), QString("loading frame %1").arg(i));
            };
            if (pf.legacy()) {
                read_frames(p, W, H, 0, hi, fps, [&](int i, Frame&& f) {
                    fr->push_back(std::move(f));
                    report(i);
                    return !task.cancelled.load();
                });
            } else {
                read_frames_fmt(p, W, H, 0, hi, fps, pf, mat, vf, [&](int i, Planes&& pl) {
                    fr->push_back(frame_from_planes(std::move(pl)));
                    report(i);
                    return !task.cancelled.load();
                });
            }
            return fr;
        },
        [this, path, vi, mat, pf, dep](const std::any&, std::any& res) {
            prog->setVisible(false);
            auto fr = std::any_cast<std::shared_ptr<std::vector<Frame>>>(res);
            if (!fr || fr->empty()) {
                say("no decodable video frames");
                return;
            }
            set_profile_check(false);
            pc_cache.clear();
            pc_order.clear();
            set_noise_level_check(false);
            nlc_cache.clear();
            nlc_order.clear();
            src = path;
            info = vi;
            frames = std::move(*fr);
            matrix = mat;
            fmt = pf;
            depth_used = dep;
            ofx_depth = 0;
            full_range = full_range_internal(vi.full_range, pf);
            install_frames();
            const int nf = static_cast<int>(frames.size());
            const bool capped = vi.n && nf < vi.n;
            video_lbl->setText(QString("%1x%2 · preview %3").arg(vi.W).arg(vi.H).arg(nf) +
                               (capped ? QString("/%1 frames (memory cap)").arg(vi.n) : QString(" frames")));
            video_lbl->setStyleSheet(capped ? "color:#d08000" : "");
            viewer_bar->set_frame_info(vi.W, vi.H, pf.bits);
            depth_lbl->setText(depth_text());
            say(QFileInfo(path).fileName() +
                Q(": " + std::to_string(vi.W) + "x" + std::to_string(vi.H) + " " + std::to_string(nf) + "f " + vi.fps +
                  " fps " + matrix) +
                (capped ? QString(" (preview holds the first %1 of %2 frames; export uses all)").arg(nf).arg(vi.n)
                        : QString()));
            invalidate();
            on_profile();
            on_params({});
            emit videoLoaded();
            const QVariant dprof = settings()->value("default_profile");
            if (dprof.isValid() && !dprof.toString().isEmpty()) {
                try {
                    state->set_profile(ProfileState::from_json(PyObj::parse(dprof.toString().toStdString())),
                                       "Default profile");
                    return;
                } catch (const std::exception& e) {
                    say(Q(std::string("default profile ignored: ") + e.what()));
                }
            }
            if (prefs["auto_profile"].toBool()) auto_profile(false);
        },
        [this](const std::any&, const QString& m) { failed("load", m); },
        [this](double pct, const QString& text) { progress(pct, text); });
}

void MainWindow::install_frames() {
    // the part of open_video's done() shared with the OFX host mode: the engine and the caches of the new frames
    viewer->set_matrix(matrix);
    engine = std::make_unique<PreviewEngine>(&frames, matrix, full_range);
    ++engine_id;
    cur_rgb.reset();
    nl_cache.clear();
    nu_cache.clear();
    thumb_cache.clear();
    thumb_order.clear();
    state->undo->clear();
    sel_area.reset();
    preview_sel.reset();
    update_profile_button();
    timeline->set_count(static_cast<int>(frames.size()), info->fps_value());
    const int nf = static_cast<int>(frames.size());
    timeline->set_frame(std::min(nf / 3, std::max(nf - 3, 0)));
}

QString MainWindow::depth_text() const {
    // the status bar: source pix_fmt / bit depth and the processing depth (main_window._depth_text)
    if (!fmt) return QString();
    const PixFmt& f = *fmt;
    const std::string proc = std::to_string(f.bits) + "-bit" + (f.is_float ? " float RGB" : f.family == "rgb" ? " RGB" : "");
    const std::string srcd = !info->pix_fmt.empty() ? describe_source(info->pix_fmt) : "OFX host";
    return Q("source " + srcd + " · processing " + proc);
}

void MainWindow::progress(double pct, const QString& text) {
    if (pct >= 0) {
        prog->setVisible(true);
        prog->setValue(static_cast<int>(pct));
    }
    if (!text.isEmpty()) say(text);
}

void MainWindow::failed(const QString& tag, const QString& m) {
    prog->setVisible(false);
    say("error (" + tag + "): " + m);
}

QImage MainWindow::thumb(int i) {
    if (frames.empty() || i < 0 || i >= static_cast<int>(frames.size())) return QImage();
    auto it = thumb_cache.find(i);
    if (it != thumb_cache.end()) return it->second;
    const Frame& f = frames[static_cast<size_t>(i)];
    const int s = std::max(2, f.H / 36 / 2 * 2);
    const int cs = (f.Hc != f.H || f.Wc != f.W) ? s / 2 : s;
    const int yh = (f.H + s - 1) / s, yw = (f.W + s - 1) / s, ch = (f.Hc + cs - 1) / cs, cw = (f.Wc + cs - 1) / cs;
    const int h = std::min(yh, ch), w = std::min(yw, cw);
    Planes p;
    p.H = p.Hc = h;
    p.W = p.Wc = w;
    p.Y.resize(static_cast<size_t>(h) * w);
    p.Cb.resize(p.Y.size());
    p.Cr.resize(p.Y.size());
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            const size_t o = static_cast<size_t>(y) * w + x;
            p.Y[o] = f.at(0, static_cast<size_t>(y * s) * f.W + x * s);
            p.Cb[o] = f.at(1, static_cast<size_t>(y * cs) * f.Wc + x * cs);
            p.Cr[o] = f.at(2, static_cast<size_t>(y * cs) * f.Wc + x * cs);
        }
    const std::vector<float> rgb = engine->to_rgb(p);
    const RgbImage r = rot90(rgb, h, w, (info->rotation / 90) % 4);
    QImage q = to_qimage_f(r.d, r.H, r.W);
    thumb_cache[i] = q;
    thumb_order.push_back(i);
    while (thumb_order.size() > 64) {
        thumb_cache.erase(thumb_order.front());
        thumb_order.pop_front();
    }
    return q;
}

// ---------------------------------------------------------------- preview
void MainWindow::invalidate() {
    ++gen;
    filt_cache.clear();
    filt_order.clear();
}

bool MainWindow::previewing() const { return page() == 1 && preview_btn->isChecked(); }

void MainWindow::schedule() {
    if (!timeline->playing() && previewing()) timer->start();
}

std::optional<Crop> MainWindow::crop() const {
    if (frames.empty() || page() != 1 || !preview_sel) return std::nullopt;
    return Crop{preview_sel->y0, preview_sel->x0, preview_sel->h, preview_sel->w};
}

const std::vector<float>& MainWindow::frame_rgb(int t) {
    if (!cur_rgb || cur_rgb->first != t) {
        cur_rgb = std::make_pair(t, engine->to_rgb(to_planes(frames[static_cast<size_t>(t)])));
    }
    return cur_rgb->second;
}

void MainWindow::display() {
    if (frames.empty()) return;
    const int t = timeline->frame();
    const std::vector<float>& orig = frame_rgb(t);
    const auto cr = crop();
    std::optional<Rect> crr;
    if (cr) crr = Rect{cr->y0, cr->x0, cr->h, cr->w};
    const std::vector<float>* filt = nullptr;
    if (previewing()) {
        auto it = filt_cache.find(std::to_string(t) + "|" + std::to_string(gen) + "|" + rect_key(crr));
        if (it != filt_cache.end()) filt = &it->second;
    }
    viewer->set_images(orig, frames[0].H, frames[0].W, filt, filt ? crr : std::nullopt, (info->rotation / 90) % 4);
    viewer->set_show_filtered(previewing());
}

void MainWindow::request_preview() {
    if (frames.empty() || !previewing()) return;
    const PyObj* eff = state->effective();
    if (!eff) {
        say("no profile yet: run Auto Profile or Build Profile (Prepare Noise Profile page)");
        return;
    }
    const int t = timeline->frame();
    const long g = gen;
    const auto cr = crop();
    std::optional<Rect> crr;
    if (cr) crr = Rect{cr->y0, cr->x0, cr->h, cr->w};
    const std::string key = std::to_string(t) + "|" + std::to_string(g) + "|" + rect_key(crr);
    if (filt_cache.count(key)) {
        display();
        return;
    }
    if (nlc) return;                        // the viewer shows the mask; Finish schedules the preview
    const PyObj P = state->backend();
    const PyObj prof = *eff;
    const long token = prof_token;
    PreviewEngine* eng = engine.get();
    say(QString("filtering frame %1…").arg(t));
    runner->submit(
        "preview", key,
        [eng, t, prof, token, P, cr](Task&) -> std::any { return eng->process(t, prof, token, P, cr).first; },
        [this, t, g, key](const std::any&, std::any& res) {
            if (g != gen) return;
            filt_cache[key] = std::any_cast<std::vector<float>>(res);
            filt_order.push_back(key);
            while (filt_order.size() > 8) {
                filt_cache.erase(filt_order.front());
                filt_order.pop_front();
            }
            if (t == timeline->frame()) display();
            say(QString("frame %1 filtered").arg(t));
        },
        [this](const std::any&, const QString& m) { failed("preview", m); });
}

void MainWindow::on_frame(int) {
    display();
    sync_selection();
    schedule();
    request_pcheck();
    request_nlc();
}

void MainWindow::on_params(const QStringList& keys) {
    const PyObj& P = state->params;
    filter_panel->set_values(P, keys);
    update_radius();
    bool dirty = false;
    for (const auto& k : P.keys()) dirty = dirty || !(P.get(k) == preset_clean.get(k));
    filter_panel->name->setItemText(0, preset_name + (dirty ? " *" : ""));
    update_performance();
    if (!keys.isEmpty()) {
        invalidate();
        display();
        schedule();
        if (nlc) nlc_timer->start();          // temporal settings: new mask (same key = from the cache)
    }
}

void MainWindow::update_performance() {
    const PyObj& P = state->params;
    const QString W = info ? QString::number(info->W) : U("—"), H = info ? QString::number(info->H) : U("—");
    QStringList bands;
    for (const auto& [b, n] : std::vector<std::pair<std::string, QString>>{
             {"high", "High"}, {"medium", "Mid"}, {"low", "Low"}, {"very-low", "Very Low"}, {"ultra-low", "Ultra Low"}}) {
        bool any = false;
        for (const char* c : {"Y", "Cb", "Cr"}) any = any || P.get(std::string("bamt_") + c + "_" + b).as_double() > 0;
        if (any) bands << n;
    }
    // the processing format of the loaded frames, the OFX host's depth, or 8-bit before a video is loaded
    const QString bits = fmt ? Q(std::to_string(fmt->bits) + "-bit" + (fmt->is_float ? " float" : "") + " (" + fmt->name + ")")
                         : ofx_depth ? QString("%1-bit (OFX host)").arg(ofx_depth)
                                     : QString("8-bit");
    const unsigned hc = std::thread::hardware_concurrency();
    const std::vector<std::pair<QString, QString>> rows = {
        {"Frame Size:", W + "x" + H + " progressive"},
        {"Frame Bitdepth:", bits},
        {"Floating Point:", "float32"},
        {"Temporal Filter:", P.get("t_enable").truthy() ? "Enabled" : "Disabled"},
        {"&nbsp;&nbsp;Radius:", QString("%1 frames").arg(P.get("t_radius").as_int())},
        {"Spatial Filter:", P.get("s_enable").truthy() ? "Enabled" : "Disabled"},
        {"&nbsp;&nbsp;Frequencies:", bands.isEmpty() ? QString("none") : bands.join(", ")},
        {"", ""},
        {"Using:", ""},
        {"&nbsp;&nbsp;CPU:", QString("preview 1 thread, export %1 processes (%2 cores)").arg(prefs["jobs"].toInt()).arg(hc ? hc : 1)}};
    QString html = "<table cellspacing=0 cellpadding=0>";
    for (const auto& [a, b] : rows) html += "<tr><td>" + a + "&nbsp;&nbsp;</td><td>" + b + "</td></tr>";
    filter_panel->set_performance(html + "</table>");
}

void MainWindow::check_speed() {
    // Check Speed: process the current frame once (as the preview does) and show frames/sec
    const PyObj* eff = state->effective();
    if (frames.empty() || !eff) {
        say("Check Speed needs a video and a profile");
        return;
    }
    const int t = timeline->frame();
    const PyObj P = state->backend(), prof = *eff;
    PreviewEngine* eng = engine.get();
    filter_panel->speed->setText(U("measuring…"));
    runner->submit(
        "speed", QString("speed"),
        [eng, t, P, prof](Task&) -> std::any {
            auto [idx, win] = eng->window(t, static_cast<int>(P.get("radius").as_int()), std::nullopt);
            const int ci = static_cast<int>(std::find(idx.begin(), idx.end(), t) - idx.begin());
            const auto t0 = std::chrono::steady_clock::now();
            Planes tmp, rv;
            temporal(win, ci, prof, P, nullptr, &tmp, &rv);
            finish(win[static_cast<size_t>(ci)], tmp, rv, prof, P);
            return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        },
        [this](const std::any&, std::any& res) {
            speed_result = 1.0 / std::max(std::any_cast<double>(res), 1e-9);
            filter_panel->speed->setText(QString("%1 frames/sec (1 frame, 1 thread)").arg(QString::fromStdString(f2(speed_result))));
        },
        [this](const std::any&, const QString& m) { failed("speed", m); });
}

void MainWindow::performance_tab() {
    tabs->setCurrentIndex(1);
    filter_panel->tabs->setCurrentIndex(2);
}

void MainWindow::reset_spatial() {
    // ↺ of the Spatial Filter header: the Spatial tab's settings back to the default preset
    const PyObj D = default_preset();
    std::vector<std::pair<std::string, PyObj>> u;
    for (const auto& kv : param_specs())
        for (const auto& pre : SPATIAL_PREFIXES)
            if (kv.first.rfind(pre, 0) == 0) {
                u.emplace_back(kv.first, D.get(kv.first));
                break;
            }
    state->set_params(u, "Reset Spatial Settings", false);
}

std::string MainWindow::profile_sig() const { return state->profile.to_json().dump(-1, true); }

void MainWindow::update_quality() {
    const ProfileState& ps = state->profile;
    if (!ps.has_profile() || frames.empty()) {
        ++quality_req;
        quality_key.reset();
        quality_pct.reset();
        completeness_pct.reset();
        profile_panel->set_quality(std::nullopt);
        profile_panel->set_completeness(std::nullopt);
        return;
    }
    const std::string key = profile_sig() + "|" + matrix + "|" + (full_range ? "1" : "0") + "|" +
                            std::to_string(frames.size()) + "|" + src.toStdString();
    if (quality_key && *quality_key == key) return;
    quality_key = key;
    const long req = ++quality_req;
    const ProfileState psc = ps;
    const std::vector<Frame>* fr = &frames;
    const bool frr = full_range;
    const std::string mat = matrix;
    runner->submit(
        "quality", req,
        [psc, fr, frr, mat](Task&) -> std::any { return profile_numbers(psc, *fr, frr, mat); },
        [this](const std::any& tag, std::any& res) {
            if (std::any_cast<long>(tag) != quality_req) return;
            auto pair = std::any_cast<std::optional<std::pair<int, int>>>(res);
            quality_pct = pair ? std::optional<int>(pair->first) : std::nullopt;
            completeness_pct = pair ? std::optional<int>(pair->second) : std::nullopt;
            profile_panel->set_quality(quality_pct);
            profile_panel->set_completeness(completeness_pct);
        },
        [this](const std::any&, const QString& m) { failed("quality", m); });
}

void MainWindow::on_profile() {
    ++prof_token;
    const ProfileState& ps = state->profile;
    const PyObj* eff = state->effective();
    std::optional<std::pair<int, int>> fi;
    if (info) fi = std::make_pair(info->W, info->H);
    profile_panel->show_profile(ps, eff, fi);
    update_quality();
    if (!ps.has_profile())
        profile_panel->set_name(std::nullopt, false);
    else
        profile_panel->set_name(fi ? QString("Noise profile (%1x%2p)").arg(fi->first).arg(fi->second) : QString("Noise profile"),
                                profile_sig() != prof_clean);
    check_btn->setVisible(ps.has_profile());
    profile_panel->check->setEnabled(ps.has_profile());
    actions["p_check"]->setEnabled(ps.has_profile());
    if (pcheck) {
        if (!ps.has_profile())
            set_profile_check(false);
        else
            pc_timer->start();
    }
    filter_panel->nl_check->setEnabled(ps.has_profile());
    fsc_nlc->setEnabled(ps.has_profile());
    actions["nlc"]->setEnabled(ps.has_profile());
    if (nlc) {
        if (!ps.has_profile())
            set_noise_level_check(false);
        else
            nlc_timer->start();
    }
    filter_panel->set_band_availability(eff && eff->has("levels") ? &eff->get("levels") : nullptr);
    if (!eff) {
        prof_lbl->setText("no profile");
    } else {
        std::string t = ps.device + " · " + eff->get("mode").as_str() + " · σ ";
        const PyObj& sg = eff->get("sigma");
        for (size_t i = 0; i < sg.keys().size(); ++i)
            t += (i ? " " : "") + sg.keys()[i] + "=" + f2(sg.get(sg.keys()[i]).as_double());
        if (!frames.empty()) {
            const int sub = (frames[0].Hc == frames[0].H && frames[0].Wc == frames[0].W) ? 1 : 2;
            if (eff->has("sub") && eff->get("sub").as_int() != sub) {
                t += " · chroma format differs: rebuild the profile";
                say("the profile was made at another chroma format (bit-depth setting); export will refuse it");
            }
        }
        prof_lbl->setText(Q(t));
    }
    sync_selection();
    invalidate();
    display();
    schedule();
}

std::optional<double> MainWindow::nl(int f, std::optional<Rect> rect) {
    if (frames.empty() || f < 0 || f >= static_cast<int>(frames.size()) || !rect || std::min(rect->h, rect->w) < 16)
        return std::nullopt;
    const std::string key = std::to_string(f) + "|" + rect_key(rect) + "|" + std::to_string(engine_id);
    auto it = nl_cache.find(key);
    if (it != nl_cache.end()) return it->second;
    int ch, cw;
    const auto c = crop_rgb(frame_rgb(f), frames[0].H, frames[0].W, rect->y0, rect->x0, rect->h, rect->w, &ch, &cw);
    const double v = noise_level(c, ch, cw);
    nl_cache[key] = v;
    nl_order.push_back(key);
    while (nl_order.size() > 64) {
        nl_cache.erase(nl_order.front());
        nl_order.pop_front();
    }
    return v;
}

void MainWindow::sync_selection() {
    if (frames.empty()) return;
    const int t = timeline->frame();
    const PyObj& base = state->profile.base;
    std::optional<Rect> main_rect;
    std::optional<double> mainv, selv;
    if (state->profile.has_profile()) {
        main_rect = area_rect(base.get("area"));
        mainv = nl(base.has("ui_frame") ? static_cast<int>(base.get("ui_frame").as_int()) : t, main_rect);
    }
    std::optional<Rect> rect;
    if (page() == 0) {
        rect = sel_area;
        QString label;
        QStringList warn;
        if (rect) {
            selv = nl(t, rect);
            if (selv) label = Q("Noise Level: " + f1(*selv));
            if (selv && mainv && *mainv != 0.0) {
                const int pct = static_cast<int>((*selv / *mainv - 1.0) * 100.0);   // int(): toward zero
                label += QString(" (%1%2%)").arg(pct >= 0 ? "+" : "").arg(pct);
            }
            if (std::min(rect->h, rect->w) >= 16) {
                const std::string key = std::to_string(t) + "|" + rect_key(rect) + "|" + std::to_string(engine_id);
                auto it = nu_cache.find(key);
                if (it == nu_cache.end()) {
                    int ch, cw;
                    const auto c = crop_rgb(frame_rgb(t), frames[0].H, frames[0].W, rect->y0, rect->x0, rect->h, rect->w,
                                            &ch, &cw);
                    std::vector<std::string> bad, clip;
                    not_uniform(c, ch, cw, &bad, &clip);
                    if (nu_cache.size() >= 64) nu_cache.clear();
                    it = nu_cache.emplace(key, std::make_pair(bad, clip)).first;
                }
                auto join = [](const std::vector<std::string>& v) {
                    std::string s;
                    for (size_t i = 0; i < v.size(); ++i) s += (i ? ", " : "") + v[i];
                    return Q(s);
                };
                if (!it->second.first.empty()) warn << "Not Uniform: " + join(it->second.first);
                if (!it->second.second.empty()) warn << "Clipping: " + join(it->second.second);
            }
        }
        viewer->set_selection(rect, "profile", label, warn);
        viewer->setToolTip("Drag a rectangle over a uniform area (each side up to 512 px), drag inside it to move it, "
                           "click to remove it. Noise Level = genuine measure of the selection's R, G, B (five levels)");
        const bool same_frame = state->profile.has_profile() && base.has("ui_frame") && base.get("ui_frame").as_int() == t;
        viewer->set_main_sample(same_frame ? main_rect : std::nullopt);
    } else {
        rect = preview_sel;
        viewer->set_selection(rect, "preview");
        viewer->setToolTip("Hold the left mouse button to see the original frame; drag to select the preview area");
        viewer->set_main_sample(std::nullopt);
    }
    viewer_bar->set_selection(rect);
    profile_panel->set_levels(mainv, selv);
}

void MainWindow::viewer_area(int y, int x, int h, int w) {
    if (page() == 0)
        on_area(y, x, h, w);
    else
        set_preview_area(y, x, h, w);
}

void MainWindow::viewer_changing(int y, int x, int h, int w) {
    if (page() == 0) {
        sel_area = Rect{y, x, h, w};
        viewer_bar->set_selection(sel_area);
        if (!sel_timer->isActive()) sel_timer->start();
    } else {
        viewer_bar->set_selection(Rect{y, x, h, w});
    }
}

void MainWindow::viewer_click() {
    if (page() == 0 && sel_area) {
        sel_area.reset();
        update_profile_button();
        sync_selection();
    }
}

QMenu* MainWindow::sel_menu(QPoint pos) {
    if (page() != 0 || !sel_area) return nullptr;
    auto* m = new QMenu(this);
    m->setAttribute(Qt::WA_DeleteOnClose);
    m->addAction("Build Profile (Most Noisy Frame)", [this] { build_profile(true); });
    m->addAction("Build Profile (Current Frame)", [this] { build_profile(false); });
    m->popup(pos);
    return m;
}

void MainWindow::on_area(int y, int x, int h, int w) {
    sel_area = Rect{y, x, h, w};
    update_profile_button();
    sync_selection();
}

void MainWindow::set_preview_area(int y, int x, int h, int w) {
    const int H = frames[0].H, W = frames[0].W;
    h = std::min(std::max(48, h), 1280) / 2 * 2;
    w = std::min(std::max(48, w), 1280) / 2 * 2;
    h = std::min(h, H / 2 * 2);
    w = std::min(w, W / 2 * 2);
    y = std::min(std::max(0, y), H - h) / 2 * 2;
    x = std::min(std::max(0, x), W - w) / 2 * 2;
    preview_sel = Rect{y, x, h, w};
    area_btn->blockSignals(true);
    area_btn->setChecked(true);
    area_btn->blockSignals(false);
    sync_selection();
    display();
    schedule();
}

void MainWindow::area_toggled(bool on) {
    if (on && !frames.empty() && !preview_sel) {
        const int H = frames[0].H, W = frames[0].W;
        const double fr = std::min(std::max(prefs["preview_area"].toDouble(), 0.1), 1.0);
        const int h = static_cast<int>(H * fr) / 2 * 2, w = static_cast<int>(W * fr) / 2 * 2;
        set_preview_area((H - h) / 4 * 2, (W - w) / 4 * 2, h, w);
        return;
    }
    if (!on) preview_sel.reset();
    sync_selection();
    display();
    schedule();
}

void MainWindow::play(bool on) {
    if (on) play_start = timeline->frame();
    timeline->set_playing(on);
    if (!on) schedule();
}

void MainWindow::stop_initial() {
    play_btn->setChecked(false);
    timeline->set_frame(play_start, true);
}

void MainWindow::set_assist(bool on) {
    if (on && nlc) set_noise_level_check(false);   // one Filter Settings Check tool at a time
    viewer->set_assist(on);
    filter_panel->assist->set_checked(on);
    for (QAction* a : {actions["f_assist"], fsc_assist}) {
        a->blockSignals(true);
        a->setChecked(on);
        a->blockSignals(false);
    }
}

// ---------------------------------------------------------------- Noise Level Check
void MainWindow::set_noise_level_check(bool on_in) {
    // Begin / Finish from the panel, the toolbar menu or Tools (one state, three displays).  Needs a profile;
    // switches to the Adjust page and the viewer to Normal (restored on Finish unless the mode was changed meanwhile).
    const bool on = on_in && state->profile.has_profile() && !frames.empty();
    const bool was = nlc;
    nlc = on;
    if (on) {
        set_assist(false);
        if (page() != 1) tabs->setCurrentIndex(1);
    }
    if (on && !was) {
        nlc_mode = viewer->mode();
        viewer->set_mode("Normal");
    } else if (was && !on) {
        if (nlc_mode && viewer->mode() == "Normal") viewer->set_mode(*nlc_mode);
        nlc_mode.reset();
    }
    sync_nlc_widgets();
    viewer->set_nlc(on);
    if (on) {
        nlc_shown.reset();
        say(U("Noise Level Check: Temporal Filter mask (repro definition) — adjust Noise Level so that the mask "
              "sufficiently covers the noisy areas"));
        request_nlc();
    } else if (was) {
        nlc_timer->stop();
        runner->cancel("nlc");
        say("Noise Level Check: finished");
        display();
        schedule();
    }
}

void MainWindow::sync_nlc_widgets() {
    filter_panel->set_nlc_active(nlc);
    for (QObject* w : {static_cast<QObject*>(fsc_nlc), static_cast<QObject*>(actions["nlc"]), static_cast<QObject*>(fsc_btn)})
        w->blockSignals(true);
    fsc_nlc->setChecked(nlc);
    actions["nlc"]->setChecked(nlc);
    fsc_btn->setChecked(nlc);
    for (QObject* w : {static_cast<QObject*>(fsc_nlc), static_cast<QObject*>(actions["nlc"]), static_cast<QObject*>(fsc_btn)})
        w->blockSignals(false);
    fsc_btn->setText(nlc ? "Noise Level Check" : "Filter Settings Check");
}

std::string MainWindow::nlc_key() const {
    // (frame, id(engine), prof_token) + backend.temporal_sig(P)
    return std::to_string(timeline->frame()) + "|" + std::to_string(engine_id) + "|" + std::to_string(prof_token) + "|" +
           temporal_sig(state->backend());
}

void MainWindow::request_nlc() {
    // Mask image of the current frame (cached; computed in the "nlc" worker slot, a newer request replaces a queued
    // one).  A result is shown only if its key is still the current one; another frame's image is removed at once,
    // a changed setting keeps the image until the new one is ready.
    if (!nlc || frames.empty()) return;
    const PyObj* eff = state->effective();
    if (!eff) return;
    const std::string key = nlc_key();
    auto it = nlc_cache.find(key);
    if (it != nlc_cache.end()) {
        nlc_order.remove(key);
        nlc_order.push_back(key);
        viewer->set_nlc(true, &it->second.first);
        nlc_shown = key;
        return;
    }
    auto head = [](const std::string& k) { return k.substr(0, k.find('|', k.find('|') + 1)); };   // (frame, engine)
    if (!nlc_shown || head(*nlc_shown) != head(key)) {
        viewer->set_nlc(true, nullptr, false);
        nlc_shown.reset();
    }
    const int t = timeline->frame();
    PreviewEngine* eng = engine.get();
    const long token = prof_token;
    const PyObj P = state->backend(), prof = *eff;
    say(QString("Noise Level Check: computing the mask of frame %1…").arg(t));
    runner->submit(
        "nlc", key,
        [eng, t, prof, token, P](Task&) -> std::any {
            const std::vector<float> m = eng->mask(t, prof, token, P);
            const Planes pl = eng->crop_planes(eng->frames()[static_cast<size_t>(t)], std::nullopt);
            const std::vector<float> rgb = eng->to_rgb(pl);
            return std::make_pair(nlc_image(rgb, m, pl.H, pl.W), mean_f32(m));
        },
        [this, key](const std::any&, std::any& res) {
            auto pr = std::any_cast<std::pair<std::vector<float>, double>>(res);
            nlc_cache[key] = pr;
            nlc_order.push_back(key);
            while (nlc_order.size() > 4) {
                nlc_cache.erase(nlc_order.front());
                nlc_order.pop_front();
            }
            if (nlc && key == nlc_key()) {
                viewer->set_nlc(true, &nlc_cache[key].first);
                nlc_shown = key;
                char b[64];
                std::snprintf(b, sizeof(b), "%.3f", pr.second);
                say(QString("Noise Level Check: mask of frame %1 (mean %2)").arg(timeline->frame()).arg(b));
            }
        },
        [this](const std::any&, const QString& m) { failed("nlc", m); });
}

// ---------------------------------------------------------------- Profile Check
void MainWindow::set_profile_check(bool on) {
    on = on && state->profile.has_profile() && !frames.empty();
    const bool was = pcheck;
    pcheck = on;
    if (on && page() != 0) tabs->setCurrentIndex(0);
    profile_panel->set_check_active(on);
    for (QObject* w : {static_cast<QObject*>(check_btn), static_cast<QObject*>(actions["p_check"])}) w->blockSignals(true);
    check_btn->setChecked(on);
    actions["p_check"]->setChecked(on);
    for (QObject* w : {static_cast<QObject*>(check_btn), static_cast<QObject*>(actions["p_check"])}) w->blockSignals(false);
    viewer->set_pcheck(on);
    if (on) {
        say("Profile Check: Assist Image (repro: spatial filter only, 100% amounts)");
        request_pcheck();
    } else if (was) {
        pc_timer->stop();
        runner->cancel("pcheck");
        say("Profile Check: finished");
    }
}

bool MainWindow::pc_cached() const {
    return pc_cache.count(std::to_string(timeline->frame()) + "|" + std::to_string(engine_id) + "|" + profile_sig()) > 0;
}

void MainWindow::request_pcheck() {
    if (!pcheck || frames.empty()) return;
    const PyObj* eff = state->effective();
    if (!eff) return;
    const int t = timeline->frame();
    const std::string key = std::to_string(t) + "|" + std::to_string(engine_id) + "|" + profile_sig();
    auto it = pc_cache.find(key);
    if (it != pc_cache.end()) {
        pc_order.remove(key);
        pc_order.push_back(key);
        viewer->set_pcheck(true, &it->second);
        return;
    }
    const PyObj prof = *eff;
    const PyObj P = assist_params();
    PreviewEngine* eng = engine.get();
    say(QString("Profile Check: computing the Assist Image of frame %1…").arg(t));
    runner->submit(
        "pcheck", key, [eng, t, prof, P](Task&) -> std::any { return eng->assist(t, prof, P); },
        [this, key](const std::any&, std::any& res) {
            pc_cache[key] = std::any_cast<std::vector<float>>(res);
            pc_order.push_back(key);
            while (pc_order.size() > 4) {
                pc_cache.erase(pc_order.front());
                pc_order.pop_front();
            }
            const int t2 = timeline->frame();
            if (pcheck && key == std::to_string(t2) + "|" + std::to_string(engine_id) + "|" + profile_sig()) {
                viewer->set_pcheck(true, &pc_cache[key]);
                say("Profile Check: Assist Image (repro: spatial filter only, 100% amounts)");
            }
        },
        [this](const std::any&, const QString& m) { failed("pcheck", m); });
}

// ---------------------------------------------------------------- profile
void MainWindow::profile_job(bool noisy, std::optional<Rect> area, const QString& text) {
    if (frames.empty()) {
        say("open a video first");
        return;
    }
    const long req = ++prof_req;
    const std::vector<Frame>* fr = &frames;
    const bool frr = full_range;
    const std::string mat = matrix, mode = profile_mode();
    const int cur = timeline->frame();
    busy("profile", true);
    say(text + U("…"));
    runner->cancel("profile");
    runner->submit(
        "profile", req,
        [this, req, fr, frr, mat, mode, cur, noisy, area](Task& task) -> std::any {
            CancelFn cancel = [this, req, &task] { return req != prof_req.load() || task.cancelled.load(); };
            const int f = noisy ? noisiest_frame(*fr, cancel) : cur;
            std::vector<int> a;
            if (area) a = {area->y0, area->x0, area->h, area->w};
            return ::nvqt::build_profile(*fr, f, frr, mat, mode, area ? &a : nullptr, cancel);
        },
        [this, req, text](const std::any& tag, std::any& res) {
            if (std::any_cast<long>(tag) != req || req != prof_req.load()) return;
            busy("profile", false);
            ProfileState n;
            n.base = std::any_cast<PyObj>(res);
            n.device = state->profile.device;
            state->set_profile(n, text);
            const int uf = static_cast<int>(n.base.get("ui_frame").as_int());
            if (uf != timeline->frame()) {
                timeline->set_frame(uf);
                display();
                sync_selection();
            }
            const PyObj& st = n.base.get("flat_status");
            QString warn;
            if (!st.is_none() && st.as_int() != 0 && st.as_int() != 2) {
                auto it = FLAT_STATUS.find(static_cast<int>(st.as_int()));
                warn = U(" — warning: ") + Q(it == FLAT_STATUS.end() ? std::to_string(st.as_int()) : it->second);
            }
            say(text + QString(": frame %1, area ").arg(uf) + Q(n.base.get("area").dump()) + warn);
            emit profileBuilt();
            if (prefs["auto_fine_tune"].toBool()) auto_fine_tune();
        },
        [this, req](const std::any& tag, const QString& m) {
            if (std::any_cast<long>(tag) != req) return;
            busy("profile", false);
            if (m.contains("ProfileCancelled")) return;
            failed("profile", m);
        });
}

void MainWindow::auto_profile(bool noisy) {
    profile_job(noisy, std::nullopt, QString("Auto Profile") + (noisy ? " (Most Noisy Frame)" : " (Current Frame)"));
}

QWidget* MainWindow::build_profile(bool noisy) {
    if (!sel_area) {
        say("Build Profile: drag a rectangle over a flat area in the preview first");
        return nullptr;
    }
    const Rect area = *sel_area;
    const QString text = QString("Build Profile") + (noisy ? " (Most Noisy Frame)" : " (Current Frame)");
    const int side = std::min(area.h, area.w);
    if (side < 32) {
        auto* box = new QMessageBox(QMessageBox::Warning, "Build Profile",
                                    "Too small area for building a noise profile; it should be at least 32x32 pixels.",
                                    QMessageBox::Ok, this);
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->open();
        return box;
    }
    if (side < 128) {
        auto* box = new QMessageBox(QMessageBox::Question, "Build Profile",
                                    QString("The selected area (%1x%2) is small; an area of at least 128x128 pixels is "
                                            "recommended for building a noise profile.")
                                        .arg(area.w)
                                        .arg(area.h),
                                    QMessageBox::NoButton, this);
        QPushButton* use = box->addButton("Use the small area", QMessageBox::AcceptRole);
        box->addButton("Cancel", QMessageBox::RejectRole);
        box->setAttribute(Qt::WA_DeleteOnClose);
        connect(box, &QMessageBox::buttonClicked, this, [this, use, noisy, area, text](QAbstractButton* b) {
            if (b == use) profile_job(noisy, area, text);
        });
        box->open();
        return box;
    }
    profile_job(noisy, area, text);
    return nullptr;
}

void MainWindow::auto_fine_tune() {
    if (!state->profile.has_profile() || frames.empty()) {
        say("Auto Fine-Tune needs a profile and the video");
        return;
    }
    if (runner->busy("aft")) {
        say("Auto Fine-Tune is already running");
        return;
    }
    const PyObj base = state->profile.base;
    const Frame* fr = &frames[static_cast<size_t>(aft_frame(base, frames.size()))];
    const bool frr = full_range;
    const std::string mat = matrix;
    busy("aft", true);
    say(U("Auto Fine-Tune (static AFT port, several seconds)…"));
    runner->submit(
        "aft", QString("aft"),
        [base, fr, frr, mat](Task& task) -> std::any {
            if (task.cancelled) throw Cancelled();
            PyObj res = run_aft(base, *fr, frr, mat, [&task] { return task.cancelled.load(); });
            if (task.cancelled) throw Cancelled();
            return res;
        },
        [this, base](const std::any&, std::any& res) {
            busy("aft", false);
            if (!(state->profile.base == base)) {
                say("Auto Fine-Tune result discarded: the profile changed while it ran");
                return;
            }
            state->set_profile(after_aft(state->profile, std::any_cast<PyObj>(res)), "Auto Fine-Tune");
            say("Auto Fine-Tune applied");
            emit aftFinished();
        },
        [this](const std::any&, const QString& m) {
            busy("aft", false);
            if (!m.contains("Cancelled")) failed("aft", m);
            emit aftFinished();
        });
}

void MainWindow::busy(const std::string& slot, bool on) {
    if (on)
        busy_slots.insert(slot);
    else
        busy_slots.erase(slot);
    const bool b = !busy_slots.empty();
    profile_panel->set_busy(b);
    actions["p_aft"]->setEnabled(!b);
    update_profile_button();
}

void MainWindow::auto_complete() {
    ProfileState n = state->profile;
    n.factors = nvqt::auto_complete(n);
    state->set_profile(n, "Auto Complete");
}

void MainWindow::edit_device(const QString& name_in) {
    QString name = name_in;
    if (name.isNull()) {
        bool ok = false;
        name = QInputDialog::getText(this, "Device Name and Mode", "Name:", QLineEdit::Normal,
                                     Q(state->profile.device), &ok);
        if (!ok) return;
    }
    if (name.toStdString() == state->profile.device) return;
    ProfileState n = state->profile;
    n.device = name.toStdString();
    state->set_profile(n, "Rename device");
}

void MainWindow::reset_fine_tune() {
    // an R/G/B profile then runs on the curve before AFT (levels 1.0, nvqt::reset_fine_tune)
    state->set_profile(nvqt::reset_fine_tune(state->profile), "Reset Fine-Tuning Results");
}

void MainWindow::generic_profile() {
    const QVariant dprof = settings()->value("default_profile");
    if (dprof.isValid() && !dprof.toString().isEmpty()) {
        try {
            state->set_profile(ProfileState::from_json(PyObj::parse(dprof.toString().toStdString())),
                               "Generic Profile (default)");
            say("Generic Profile: Default Profile loaded (repro)");
            return;
        } catch (const std::exception& e) {
            say(Q(std::string("default profile ignored: ") + e.what()));
        }
    }
    reset_profile();
    say("Generic Profile: no Default Profile assigned, profile reset (repro)");
}

void MainWindow::save_profile(const QString& path_in) {
    if (!state->profile.has_profile()) {
        say("no profile to save");
        return;
    }
    const QString path = path_in.isEmpty() ? ask_save("Save Device Noise Profile", PROFILE_FILTER, "profile.nvprof.json") : path_in;
    if (path.isEmpty()) return;
    write_file(path, state->profile.to_json().dump(1));
    prof_clean = profile_sig();
    on_profile();
    say("profile saved: " + path);
}

void MainWindow::load_profile(const QString& path_in) {
    const QString path = path_in.isEmpty() ? ask_open("Load profile", PROFILE_FILTER) : path_in;
    if (path.isEmpty()) return;
    try {
        const ProfileState ps = ProfileState::from_json(read_json(path));
        prof_clean = ps.to_json().dump(-1, true);
        state->set_profile(ps, "Load Profile");
        say("profile loaded: " + path);
    } catch (const std::exception& e) {
        say(Q(std::string("cannot load profile: ") + e.what()));
    }
}

void MainWindow::reset_profile() { state->set_profile(ProfileState(), "Reset Profile"); }

void MainWindow::copy_profile() {
    if (state->profile.has_profile()) {
        QGuiApplication::clipboard()->setText(Q(state->profile.to_json().dump()));
        say("profile copied");
    }
}

void MainWindow::paste_profile() {
    try {
        state->set_profile(ProfileState::from_json(PyObj::parse(QGuiApplication::clipboard()->text().toStdString())),
                           "Paste Profile");
    } catch (const std::exception& e) {
        say(Q(std::string("clipboard holds no profile: ") + e.what()));
    }
}

void MainWindow::reset_default_profile() {
    settings()->remove("default_profile");
    say("default profile removed");
}

void MainWindow::assign_default_profile() {
    if (state->profile.has_profile()) {
        settings()->setValue("default_profile", Q(state->profile.to_json().dump()));
        say("current profile assigned as default (used when a video is opened)");
    }
}

void MainWindow::change_default_profile(const QString& path_in) {
    const QString path = path_in.isEmpty() ? ask_open("Default profile", PROFILE_FILTER) : path_in;
    if (path.isEmpty()) return;
    try {
        const ProfileState ps = ProfileState::from_json(read_json(path));
        settings()->setValue("default_profile", Q(ps.to_json().dump()));
        say("default profile: " + path);
    } catch (const std::exception& e) {
        say(Q(std::string("cannot use as default: ") + e.what()));
    }
}

// ---------------------------------------------------------------- presets
void MainWindow::save_preset(const QString& path_in) {
    const QString path = path_in.isEmpty() ? ask_save("Save Filter Preset", PRESET_FILTER, "preset.nvpreset.json") : path_in;
    if (path.isEmpty()) return;
    write_file(path, preset_json(state->params).dump(1));
    preset_named(QFileInfo(path).fileName(), state->params);
    say("preset saved: " + path);
}

void MainWindow::load_preset(const QString& path_in) {
    const QString path = path_in.isEmpty() ? ask_open("Load Filter Preset", PRESET_FILTER) : path_in;
    if (path.isEmpty()) return;
    try {
        const PyObj P = params_from_json(read_json(path));
        preset_named(QFileInfo(path).fileName(), P);
        state->set_all_params(P, "Load Filter Preset");
        say("preset loaded: " + path);
    } catch (const std::exception& e) {
        say(Q(std::string("cannot load preset: ") + e.what()));
    }
}

PyObj MainWindow::default_preset() {
    const QVariant dp = settings()->value("default_preset");
    if (dp.isValid() && !dp.toString().isEmpty()) {
        try {
            return params_from_json(PyObj::parse(dp.toString().toStdString()));
        } catch (const std::exception&) {
        }
    }
    return default_params();
}

void MainWindow::preset_named(const QString& name_in, const PyObj& params) {
    QString name = name_in;
    for (const QString& suf : {QString(".nvpreset.json"), QString(".json")})
        if (name.endsWith(suf)) {
            name.chop(suf.size());
            break;
        }
    preset_name = name;
    preset_clean = params;
    on_params({});
}

void MainWindow::reset_filter() {
    const PyObj P = default_preset();
    preset_named("default", P);
    state->set_all_params(P, "Reset Filter Settings");
}

QWidget* MainWindow::preset_description() {
    const PyObj D = default_params();
    QStringList ch;
    for (const auto& k : state->params.keys())
        if (!(state->params.get(k) == D.get(k)))
            ch << Q(spec(k).label + ": " + (D.get(k).type() == PyObj::STR ? D.get(k).as_str() : D.get(k).dump()) + " → " +
                    (state->params.get(k).type() == PyObj::STR ? state->params.get(k).as_str() : state->params.get(k).dump()));
    return info_box("Preset Description", "Preset: " + preset_name + "\n\n" +
                                              (ch.isEmpty() ? QString("Factory settings (no changes).")
                                                            : "Changed from the factory settings:\n" + ch.join("\n")));
}

void MainWindow::copy_both() {
    PyObj d = PyObj::dict();
    d.set("profile", state->profile.has_profile() ? state->profile.to_json() : PyObj::none());
    d.set("preset", preset_json(state->params));
    QGuiApplication::clipboard()->setText(Q(d.dump()));
    say("profile and preset copied");
}

void MainWindow::paste_both() {
    PyObj P;
    std::optional<ProfileState> ps;
    try {
        const PyObj d = PyObj::parse(QGuiApplication::clipboard()->text().toStdString());
        if (!d.has("preset")) throw std::runtime_error("'preset'");
        P = params_from_json(d.get("preset"));
        if (!d.get("profile").is_none()) ps = ProfileState::from_json(d.get("profile"));
    } catch (const std::exception& e) {
        say(Q(std::string("clipboard holds no profile and preset: ") + e.what()));
        return;
    }
    state->undo->beginMacro("Paste Profile and Preset");
    if (ps) state->set_profile(*ps, "Paste Profile");
    state->set_all_params(P, "Paste Filter Settings");
    state->undo->endMacro();
}

void MainWindow::copy_filter() {
    QGuiApplication::clipboard()->setText(Q(preset_json(state->params).dump()));
    say("filter settings copied");
}

void MainWindow::paste_filter() {
    try {
        state->set_all_params(params_from_json(PyObj::parse(QGuiApplication::clipboard()->text().toStdString())),
                              "Paste Filter Settings");
    } catch (const std::exception& e) {
        say(Q(std::string("clipboard holds no filter settings: ") + e.what()));
    }
}

void MainWindow::assign_default_preset() {
    settings()->setValue("default_preset", Q(preset_json(state->params).dump()));
    say("current filter settings assigned as default");
}

void MainWindow::reset_default_preset() {
    settings()->remove("default_preset");
    say("default preset removed (factory settings are used)");
}

void MainWindow::change_default_preset(const QString& path_in) {
    const QString path = path_in.isEmpty() ? ask_open("Default filter preset", PRESET_FILTER) : path_in;
    if (path.isEmpty()) return;
    try {
        const PyObj P = params_from_json(read_json(path));
        settings()->setValue("default_preset", Q(preset_json(P).dump()));
        say("default preset: " + path);
    } catch (const std::exception& e) {
        say(Q(std::string("cannot use as default: ") + e.what()));
    }
}

// ---------------------------------------------------------------- tools (N3c)
namespace {

// Preferences: accept() (OK, or a call) runs `check` first and stays open when it fails (the ffmpeg field; the
// Python UI's _PrefsDialog)
class PrefsDialog : public QDialog {
public:
    using QDialog::QDialog;
    std::function<bool()> check;
    void accept() override {
        if (check && !check()) return;
        QDialog::accept();
    }
};

}  // namespace

QDialog* MainWindow::preferences() {
    // Tools > Preferences… (F10), the Preferences… link of the General tab (main_window.preferences): the same rows,
    // keys (prefs/<key> of the shared settings file) and defaults as the Python UI, the last two rows Auto Fine-Tune
    // after a profile build (prefs/auto_fine_tune) and the ffmpeg location (prefs/ffmpeg, which the N2b launcher
    // and both UIs read at start; a path without ffmpeg and ffprobe keeps the dialog open with a warning)
    auto* dlg = new PrefsDialog(this);
    dlg->setWindowTitle("Preferences");
    auto* f = new QFormLayout(dlg);
    auto* jobs = new QSpinBox();
    jobs->setRange(1, 64);
    jobs->setValue(prefs["jobs"].toInt());
    auto* crf = new QSpinBox();
    crf->setRange(0, 51);
    crf->setValue(prefs["crf"].toInt());
    auto* audio = new QComboBox();
    audio->addItems({"copy", "aac", "none"});
    audio->setCurrentText(prefs["audio"].toString());
    auto* area = new QDoubleSpinBox();
    area->setRange(0.1, 1.0);
    area->setSingleStep(0.05);
    area->setValue(prefs["preview_area"].toDouble());
    auto* mem = new QDoubleSpinBox();
    mem->setRange(0.25, 64.0);
    mem->setValue(prefs["mem_gb"].toDouble());
    auto* autop = new QCheckBox("Auto Profile when a video is opened");
    autop->setChecked(prefs["auto_profile"].toBool());
    auto* native = new QCheckBox("Use the native core (libnvcore.dylib; same results, faster)");
    native->setChecked(true);
    native->setEnabled(false);       // the C++ UI has no Python path: the stored choice is kept for the Python UI
    native->setToolTip("The C++ UI always processes through libnvcore (native/, bit-exact with the Python code); the "
                       "setting (prefs/native) is kept for the Python UI");
    auto* depthb = new QComboBox();
    for (const auto& [label, val] : DEPTHS) depthb->addItem(Q(label), Q(val));
    depthb->setCurrentIndex(std::max(depthb->findData(Q(depth())), 0));
    depthb->setToolTip("Bit depth of loading, profiling and export (nv_depth.py). 8-bit = yuv420p as before; 10/12/16 "
                       "keep the source's chroma format (RGB sources: gbrp), 32 = RGB float. Changing it reloads the video");
    auto* aft = new QCheckBox("Auto Fine-Tune after Auto / Build Profile");
    aft->setChecked(prefs["auto_fine_tune"].toBool());
    aft->setToolTip("prefs/auto_fine_tune (the genuine runs AutoFineTune after a profile build, DefAutoFineTine = 1)");
    auto* ff = new QLineEdit(settings()->value("prefs/ffmpeg").toString());
    ff->setPlaceholderText(Q(ffmpeg_dir().empty() ? std::string("(not found)") : ffmpeg_dir()));
    ff->setToolTip(FFMPEG_TOOLTIP);
    auto* ff_browse = new QPushButton("Browse…");
    ff_browse->setToolTip("Choose the ffmpeg binary");
    auto* ffw = new QWidget();
    auto* ffl = new QHBoxLayout(ffw);
    ffl->setContentsMargins(0, 0, 0, 0);
    ffl->addWidget(ff, 1);
    ffl->addWidget(ff_browse);
    connect(ff_browse, &QPushButton::clicked, dlg, [this, ff] {
        const QString p = ask_open("ffmpeg binary", "All files (*)");
        if (!p.isEmpty()) ff->setText(p);
    });
    f->addRow("Computing: export processes (jobs)", jobs);
    f->addRow("Output: x264 CRF", crf);
    f->addRow("Output: audio", audio);
    f->addRow("Preview Area (fraction of the frame)", area);
    f->addRow("Preview cache: frames in memory (GB)", mem);
    f->addRow("Bit depth", depthb);
    f->addRow(autop);
    f->addRow(native);
    f->addRow(aft);
    f->addRow("ffmpeg (directory or binary)", ffw);
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, dlg, &QDialog::reject);
    f->addRow(bb);
    dlg->check = [dlg, ff] {
        const QString fp = ff->text().trimmed();
        if (fp.isEmpty() || ffmpeg_tools_in(fp.toStdString())) return true;
        auto* box = new QMessageBox(QMessageBox::Warning, "ffmpeg not found", Q(ffmpeg_missing_text(fp.toStdString())),
                                    QMessageBox::Ok, dlg);
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->open();
        return false;
    };
    connect(dlg, &QDialog::accepted, this, [this, jobs, crf, audio, area, mem, autop, depthb, aft, ff] {
        const bool reload = depthb->currentData().toString() != prefs["depth"].toString();
        prefs["jobs"] = jobs->value();
        prefs["crf"] = crf->value();
        prefs["audio"] = audio->currentText();
        prefs["preview_area"] = area->value();
        prefs["mem_gb"] = mem->value();
        prefs["auto_profile"] = autop->isChecked();
        prefs["depth"] = depthb->currentData().toString();
        prefs["auto_fine_tune"] = aft->isChecked();
        auto st = settings();
        for (const auto& [k, v] : prefs) st->setValue(Q("prefs/" + k), v);
        const QString fp = ff->text().trimmed();
        if (fp.isEmpty())
            st->remove("prefs/ffmpeg");
        else
            st->setValue("prefs/ffmpeg", fp);
        const std::string dir = find_ffmpeg_dir(fp.toStdString());
        if (!dir.empty()) set_ffmpeg_dir(dir);
        if (reload && !src.isEmpty() && !export_ && !ofx_depth) {
            open_video(src);              // frames are held at the processing depth
            return;
        }
        invalidate();
        schedule();
    });
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->open();
    return dlg;
}

namespace {
std::string pyfmt(const char* f, double v) {
    char b[64];
    std::snprintf(b, sizeof(b), f, v);
    return b;
}
// str() of a Python value in a message (ints, None, lists of ints as Python prints them)
std::string pystr(const PyObj& v) {
    if (v.is_none()) return "None";
    if (v.type() == PyObj::STR) return v.as_str();
    return v.dump();
}
std::string flat_name(const PyObj& st) {
    if (st.is_none()) return "manual";
    auto it = FLAT_STATUS.find(static_cast<int>(st.as_int()));
    return it == FLAT_STATUS.end() ? std::to_string(st.as_int()) : it->second;
}
}  // namespace

QDialog* MainWindow::profile_viewer() {
    // Profile > Profile Viewer… (main_window.profile_viewer): Profile mode (the build_profile_v5 mode of Auto / Build
    // Profile, kept in the preferences), sample area, FindFlatArea status, band ratios, the sigma graph and values
    const PyObj* eff = state->effective();
    const PyObj& base = state->profile.base;
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle("Profile Viewer");
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    auto* v = new QVBoxLayout(dlg);
    auto* f = new QFormLayout();
    auto* mode = new QComboBox();
    mode->setObjectName("mode");
    for (const auto& m : PROFILE_MODES) mode->addItem(Q(m));
    mode->setCurrentText(Q(profile_mode()));
    mode->setToolTip("build_profile_v5 mode of Auto / Build Profile (repro): auto = single-frame curve + band shape from "
                     "static pixels (default), spatial = genuine-style single frame, temporal = frame differences");
    connect(mode, &QComboBox::currentTextChanged, this, [this](const QString& m) {
        prefs["profile_mode"] = m;
        settings()->setValue("prefs/profile_mode", m);
    });
    f->addRow("Profile mode (repro):", mode);
    std::string area_s, match, ratios;
    if (!state->profile.has_profile()) {
        area_s = "(no profile)";
        match = U("—").toStdString();
        ratios = U("—").toStdString();
    } else {
        const Rect r = area_rect(base.get("area"));
        area_s = std::to_string(r.w) + "x" + std::to_string(r.h) + " at (x " + std::to_string(r.x0) + ", y " +
                 std::to_string(r.y0) + ") \xc2\xb7 frame " + (base.has("ui_frame") ? pystr(base.get("ui_frame")) : "?");
        const PyObj& st = base.get("flat_status");
        match = st.is_none() ? "manual area" : flat_name(st);
        if (base.get("aft_applied").truthy()) match += " \xc2\xb7 AFT applied";
        const PyObj& br = base.get("band_ratio").get("Y");
        for (size_t i = 0; i < br.size(); ++i) ratios += (i ? " " : "") + pyfmt("%.2f", br[i].as_double());
    }
    auto label = [](const std::string& t, const char* name) {
        auto* l = new QLabel(Q(t));
        l->setObjectName(name);
        return l;
    };
    f->addRow("Sample area:", label(area_s, "area"));
    f->addRow("Match:", label(match, "match"));
    f->addRow("Band ratios (Y, repro):", label(ratios, "ratios"));
    v->addLayout(f);
    auto* g = new NoiseGraph();
    g->setMinimumSize(520, 260);
    g->set_profile(eff, state->profile.has_profile() ? &base : nullptr);
    v->addWidget(g);
    auto* t = new QTextEdit();
    t->setObjectName("details");
    t->setReadOnly(true);
    if (!eff) {
        t->setPlainText("no profile");
    } else {
        const PyObj& st = eff->get("flat_status");
        const PyObj& ar = eff->get("area");
        std::vector<std::string> lines;
        lines.push_back("device: " + state->profile.device);
        lines.push_back("mode: " + eff->get("mode").as_str() + "  frame: " + pystr(eff->get("ui_frame")) + "  area (" +
                        (ar.size() == 4 ? "y, x, h, w" : "y, x, size") + "): " + ar.dump() + "  [" + flat_name(st) + "]");
        const double sf = eff->has("static_fraction") ? eff->get("static_fraction").as_double() : 0.0;
        lines.push_back("static pixels: " + pyfmt("%.1f", sf * 100.0) + "%  AFT: " +
                        (eff->get("aft_applied").truthy() ? "True" : "False"));
        std::string sg = U("σ total @area: ").toStdString();
        const PyObj& sig = eff->get("sigma");
        for (size_t i = 0; i < sig.keys().size(); ++i)
            sg += (i ? ", " : "") + sig.keys()[i] + "=" + pyfmt("%.3f", sig.get(sig.keys()[i]).as_double());
        lines.push_back(sg);
        std::string nl = "profile Noise Level: ";
        int i = 0;
        for (const char* k : {"all", "Y", "Cr", "Cb"})
            nl += std::string(i++ ? ", " : "") + k + " " + pyfmt("%+.0f", (state->profile.tune_value(k) - 1) * 100) + "%";
        lines.push_back(nl);
        for (const char* n : {"Y", "Cb", "Cr"}) {
            std::string l = std::string(n) + " band ratio";
            const PyObj& br = eff->get("band_ratio").get(n);
            for (size_t j = 0; j < br.size(); ++j) l += " " + pyfmt("%.3f", br[j].as_double());
            lines.push_back(l);
        }
        std::string bins = "brightness bins";
        const PyObj& cc = eff->get("curve_centers");
        for (size_t j = 0; j < cc.size(); ++j) bins += " " + pyfmt("%.0f", cc[j].as_double());
        lines.push_back(bins);
        for (const char* n : {"Y", "Cb", "Cr"}) {
            std::string l = std::string(n) + U(" σ1 curve").toStdString();
            const PyObj& cv = eff->get("curve").get(n);
            for (size_t j = 0; j < cv.size(); ++j) l += " " + pyfmt("%.3f", cv[j].as_double());
            lines.push_back(l);
        }
        lines.push_back("Overall Profile Quality: " + (quality_pct ? std::to_string(*quality_pct) + "%" : std::string("n/a")) +
                        " (genuine measure, quality.py)");
        lines.push_back("Completeness: " + (completeness_pct ? std::to_string(*completeness_pct) + "%" : std::string("n/a")) +
                        " (genuine measure, quality.py)");
        std::string all;
        for (size_t j = 0; j < lines.size(); ++j) all += (j ? "\n" : "") + lines[j];
        t->setPlainText(Q(all));
    }
    v->addWidget(t);
    dlg->resize(640, 620);
    dlg->show();
    return dlg;
}

QDialog* MainWindow::batch_profiler(const QStringList* files, const QString& outdir, const QString& mode) {
    // Tools > Batch Profiler… (main_window.batch_profiler): profiles each video (frame N//3, like denoise_video_v5)
    // and saves <stem>.nvprof.json; with files and outdir it runs at once (no dialog)
    if (files && !outdir.isEmpty()) {
        run_batch(*files, outdir, mode.isEmpty() ? QString("auto") : mode);
        return nullptr;
    }
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle("Batch Profiler");
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    auto* v = new QVBoxLayout(dlg);
    auto* lst = new QListWidget();
    lst->setObjectName("files");
    v->addWidget(lst);
    auto* row = new QHBoxLayout();
    auto* add = new QPushButton(U("Add videos…"));
    connect(add, &QPushButton::clicked, dlg, [dlg, lst] { lst->addItems(QFileDialog::getOpenFileNames(dlg, "Videos", "", "Video (*)")); });
    row->addWidget(add);
    auto* mbox = new QComboBox();
    mbox->setObjectName("mode");
    mbox->addItems({"auto", "spatial", "temporal"});
    row->addWidget(new QLabel("mode"));
    row->addWidget(mbox);
    v->addLayout(row);
    auto* run = new QPushButton("Choose output folder and run");
    v->addWidget(run);
    connect(run, &QPushButton::clicked, dlg, [this, dlg, lst, mbox] {
        const QString d = QFileDialog::getExistingDirectory(dlg, "Output folder");
        if (d.isEmpty()) return;
        QStringList fs;
        for (int i = 0; i < lst->count(); ++i) fs << lst->item(i)->text();
        run_batch(fs, d, mbox->currentText());
        dlg->accept();
    });
    dlg->open();
    return dlg;
}

void MainWindow::run_batch(const QStringList& files, const QString& outdir, const QString& mode) {
    batch_result.reset();
    const std::string dep = depth();                 // same bit-depth setting as loading / export
    const std::vector<QString> fl(files.begin(), files.end());
    const std::string md = mode.toStdString();
    runner->submit(
        "batch", QString("batch"),
        [fl, outdir, md, dep](Task& task) -> std::any {
            auto out = std::make_shared<std::vector<QString>>();
            for (size_t k = 0; k < fl.size(); ++k) {
                const std::string path = fl[k].toStdString();
                task.report(100.0 * static_cast<double>(k) / std::max<double>(static_cast<double>(fl.size()), 1.0),
                            "batch: profiling " + QFileInfo(fl[k]).fileName());
                const VideoInfo vi = probe(path);
                const int W = vi.W, H = vi.H;
                const PixFmt pf = choose_format(source_format(vi.pix_fmt), dep);
                const std::string mat = matrix_from_tags(vi.tags, H);
                const std::string vf = pf.legacy() ? std::string() : read_filter(vi, pf, mat);
                const int n = vi.n;
                const int p0 = std::max(std::min(n / 3, n - 3), 0);
                auto read = [&](int lo, int hi) {
                    std::vector<Frame> fr;
                    if (pf.legacy())
                        read_frames(path, W, H, lo, hi, vi.fps, [&](int, Frame&& f) {
                            fr.push_back(std::move(f));
                            return true;
                        });
                    else
                        read_frames_fmt(path, W, H, lo, hi, vi.fps, pf, mat, vf, [&](int, Planes&& p) {
                            fr.push_back(frame_from_planes(std::move(p)));
                            return true;
                        });
                    return fr;
                };
                std::vector<Frame> fr = read(p0, p0 + 3);
                if (fr.empty()) fr = read(0, 3);
                PyObj prof = ::nvqt::build_profile(fr, 0, full_range_internal(vi.full_range, pf), mat, md, nullptr, nullptr);
                prof.set("ui_frame", PyObj::integer(p0));
                const QString dst = QDir(outdir).filePath(QFileInfo(fl[k]).completeBaseName() + ".nvprof.json");
                ProfileState ps;
                ps.base = prof;
                ps.device = QFileInfo(fl[k]).fileName().toStdString();
                std::ofstream o(dst.toStdString(), std::ios::binary);
                o << ps.to_json().dump(1);
                out->push_back(dst);
            }
            return out;
        },
        [this, outdir](const std::any&, std::any& res) {
            prog->setVisible(false);
            batch_result = *std::any_cast<std::shared_ptr<std::vector<QString>>>(res);
            say(QString("batch profiler: wrote %1 profiles to ").arg(batch_result->size()) + outdir);
        },
        [this](const std::any&, const QString& m) { failed("batch", m); },
        [this](double pct, const QString& text) { progress(pct, text); });
}

// ---------------------------------------------------------------- misc
void MainWindow::save_frame(const QString& path_in) {
    if (!viewer->has_image()) return;
    const QString path = path_in.isEmpty() ? ask_save("Save output image", "PNG (*.png)", "frame.png") : path_in;
    if (path.isEmpty()) return;
    viewer->composite().save(path);
    say("saved " + path);
}

QWidget* MainWindow::guide(const std::string& which) {
    const auto& g = GUIDES.at(which);
    return info_box(Q(g.first), Q(g.second));
}

QWidget* MainWindow::about() {
    return info_box("About",
                    "Reproduction UI of Neat Video v6 (C++ / Qt " + QString(qVersion()) + ").\n"
                    "Backend: libnvcore (native/, the C++ ports of repro/nvfilter.py v5 and the static AFT port, "
                    "bit-exact with the Python code).\n"
                    "Not affiliated with ABSoft; profiles and presets are JSON, not .dnp / .nfp.\n\n"
                    "This program uses the Qt framework (Qt " + QString(qVersion()) + "), licensed under the GNU "
                    "Lesser General Public License v3 (LGPL-3.0); Qt is linked dynamically and can be replaced. "
                    "Qt source code and licence: https://www.qt.io/download-open-source and Help > About Qt.");
}

QWidget* MainWindow::image_info() {
    if (!info) return nullptr;
    const VideoInfo& i = *info;
    const PixFmt f = fmt ? *fmt : pixfmt("yuv420p");
    return info_box("Image information",
                    QFileInfo(src).fileName() + Q("\n" + std::to_string(i.W) + "x" + std::to_string(i.H) + ", source " +
                                                  describe_source(i.pix_fmt) + ", read as " + describe(f) + "\nfps " +
                                                  i.fps + ", matrix " + matrix + ", " +
                                                  (i.full_range ? "full" : "limited") + " range" +
                                                  (f.family == "rgb" ? " (processed as full-range YCbCr)" : "") +
                                                  ", rotation " + std::to_string(i.rotation)));
}

// ---------------------------------------------------------------- export
void MainWindow::process_video(const QString& dst_in, std::optional<int> start, std::optional<int> nframes) {
    if (src.isEmpty()) {
        say("open a video first");
        return;
    }
    const PyObj* eff = state->effective();
    if (!eff) {
        say("no profile: run Auto Profile first");
        return;
    }
    if (export_) {
        say("an export is already running");
        return;
    }
    QString dst = dst_in;
    if (dst.isEmpty()) {
        const QFileInfo fi(src);
        const auto [filt, ext] = export_filter();
        dst = ask_save("Process full video", filt, QDir(fi.absolutePath()).filePath(fi.completeBaseName() + "_nvqt" + ext));
        if (dst.isEmpty()) return;
        if (dst.toLower().endsWith(".exr") && !QFileInfo(dst).fileName().contains('%'))
            dst = dst.left(dst.size() - 4) + "_%05d.exr";      // EXR sequence: frame-number pattern
    }
    dst = QFileInfo(dst).absoluteFilePath();
    const QString base = Q(sidecar_stem(dst.toStdString()));   // without %05d for an EXR sequence
    const QString ppath = base + ".ui_params.json", fpath = base + ".ui_profile.json";
    const PyObj P = state->backend();
    write_file(ppath, P.dump(1));
    write_file(fpath, eff->dump(1));
    ExportOptions o;
    o.src = src.toStdString();
    o.dst = dst.toStdString();
    o.prof = *eff;
    o.params = P;
    o.jobs = prefs["jobs"].toInt();
    o.crf = prefs["crf"].toInt();
    o.audio = prefs["audio"].toString().toStdString();
    o.depth = depth_used;
    o.start = start;
    o.frames = nframes;
    export_ = new ExportJob(o, this);
    connect(export_, &ExportJob::progress, this, [this](double pct, QString text) { progress(pct, text); });
    connect(export_, &ExportJob::finished, this, [this](bool ok, QString text) { export_done(ok, text); });
    prog->setVisible(true);
    prog->setValue(0);
    cancel_btn->setVisible(true);
    actions["export"]->setEnabled(false);
    b_apply->setEnabled(false);
    export_dst = dst;
    export_cancelled = false;
    export_files = {dst, base + ".json", ppath, fpath};    // denoise_video_v5 writes <stem>.json
    export_->start();
    say(U("exporting to ") + dst + U(" …"));
}

void MainWindow::export_done(bool ok, const QString& text) {
    prog->setVisible(false);
    cancel_btn->setVisible(false);
    actions["export"]->setEnabled(true);
    b_apply->setEnabled(true);
    ExportJob* j = export_;
    export_ = nullptr;
    if (j) j->deleteLater();
    export_ok = ok;
    if (!ok && export_cancelled) remove_export_files(export_files);   // remove what this export wrote
    say(ok ? "wrote " + export_dst : export_cancelled ? QString("export cancelled") : "export failed: " + text);
}

std::pair<QString, QString> MainWindow::export_filter() const {
    // the save-dialog filter and default extension for the processing depth (16 / 32 bits: .mkv, 32 bits also an
    // EXR sequence; main_window._export_filter)
    const PixFmt f = fmt ? *fmt : pixfmt("yuv420p");
    if (f.bits >= 16 || f.family == "rgb")
        return {QString("Matroska FFV1 (*.mkv)") + (f.is_float ? ";;EXR sequence (*.exr)" : ""), ".mkv"};
    if (f.legacy()) return {"MP4 (*.mp4)", ".mp4"};
    return {"MP4 (*.mp4);;QuickTime (*.mov);;Matroska FFV1 (*.mkv)", ".mp4"};
}

void MainWindow::remove_export_files(const std::vector<QString>& files) {
    for (const QString& p : files) remove_output(p.toStdString());
}

void MainWindow::cancel_export() {
    if (export_) {
        export_cancelled = true;
        export_->kill();
    }
}

void MainWindow::closeEvent(QCloseEvent* e) {
    if (close_hook) close_hook(e);
    ++prof_req;                               // cancels a running profile search
    if (export_) {
        const std::vector<QString> files = export_files;
        cancel_export();
        delete export_;                       // waits for the export thread
        export_ = nullptr;
        remove_export_files(files);           // same clean-up as a cancelled export
    }
    runner->wait_all();
    QMainWindow::closeEvent(e);
    if (after_close_hook) after_close_hook(e);
}

// ---------------------------------------------------------------- OFX host mode
void MainWindow::set_ofx_mode(bool ofx) {
    // main_window._set_ofx_mode: disable the standalone-only controls when running as an OFX host plugin backend
    for (const char* k : {"open", "export"}) actions[k]->setEnabled(!ofx);
    load_btn->setEnabled(!ofx);
    generic_btn->setEnabled(!ofx);
    b_apply->setToolTip(ofx ? "Apply: process the current frame and emit to the host"
                            : "Apply the current filter settings to the preview");
}

void MainWindow::set_ofx_frames(std::vector<Frame>&& fr, int W, int H, double fps, int depth) {
    // main.py _ofx_main: inject the host-provided frames into the UI (as the Python UI does: the viewer is drawn on
    // the first frame / profile / setting change)
    src = "<OFX host>";
    VideoInfo vi;
    vi.W = W;
    vi.H = H;
    vi.fps = py_float_str(fps);
    vi.n = static_cast<int>(fr.size());
    info = vi;
    frames = std::move(fr);
    matrix = "bt709";
    full_range = false;
    fmt.reset();
    ofx_depth = depth;
    engine = std::make_unique<PreviewEngine>(&frames, matrix, full_range);
    ++engine_id;
    cur_rgb.reset();
    const int n = static_cast<int>(frames.size());
    timeline->set_count(n, fps);
    timeline->set_frame(std::min(n / 3, std::max(n - 3, 0)));
    video_lbl->setText(QString("OFX host: %1x%2 · %3 frames · %4 fps").arg(W).arg(H).arg(n).arg(Q(py_float_str(fps))));
    depth_lbl->setText(QString("source OFX host (%1-bit 4:2:0) · processing %1-bit").arg(depth));
    viewer_bar->set_frame_info(W, H, depth);
    update_performance();
    set_ofx_mode(true);
    say("OFX host: Auto Profile (Prepare Noise Profile), adjust, then Apply to return the frame to the host");
}

QString MainWindow::dump_state() {
    // the values the Python UI shows for the same state (qtui/tools/compare_ui.py builds the same dict there)
    PyObj d = PyObj::dict();
    ProfilePanel* pp = profile_panel;
    d.set("quality", PyObj::str(pp->quality->text().toStdString()));
    d.set("completeness", PyObj::str(pp->completeness->text().toStdString()));
    d.set("main_val", PyObj::str(pp->main_val->text().toStdString()));
    d.set("sel_val", PyObj::str(pp->sel_val->text().toStdString()));
    d.set("name", PyObj::str(pp->name->currentText().toStdString()));
    d.set("info", PyObj::str(pp->info->text().toStdString()));
    PyObj bars = PyObj::list();
    for (const auto& [b, _] : UI_BANDS) bars.append(PyObj::real(pp->bars.at(b)->value));
    d.set("bars", bars);
    d.set("prof_lbl", PyObj::str(prof_lbl->text().toStdString()));
    d.set("video_lbl", PyObj::str(video_lbl->text().toStdString()));
    d.set("depth_lbl", PyObj::str(depth_lbl->text().toStdString()));
    d.set("sel_label", PyObj::str(viewer->sel_label().toStdString()));
    PyObj warn = PyObj::list();
    for (const QString& w : viewer->sel_warn()) warn.append(PyObj::str(w.toStdString()));
    d.set("sel_warn", warn);
    d.set("wh", PyObj::str(viewer_bar->wh->text().toStdString()));
    d.set("frame_info", PyObj::str(viewer_bar->size->text().toStdString()));
    d.set("auto_btn", PyObj::str(auto_btn->text().toStdString()));
    d.set("check_visible", PyObj::boolean(!check_btn->isHidden()));
    d.set("frame", PyObj::integer(timeline->frame()));
    d.set("timeline_label", PyObj::str(timeline->label->text().toStdString()));
    PyObj tune = PyObj::dict();
    for (const char* k : {"all", "Y", "Cr", "Cb"}) tune.set(k, PyObj::str(pp->tune.at(k)->val->text().toStdString()));
    d.set("tune_rows", tune);
    d.set("profile", state->profile.has_profile() ? state->profile.to_json() : PyObj::none());
    return QString::fromStdString(d.dump(1));
}

}  // namespace nvqt
