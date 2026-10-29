// qtui_dump - drives the C++ main window off-screen through the comparison scenario of qtui/tools/py_dump.py
// (the same steps on the PySide6 UI) and writes what both UIs show: the panel / status strings and the profile
// JSON (state_*.json), the raw pane images of every viewer mode at three brightnesses (panes/*.rgb + index.json)
// and window / viewer grabs (*.png).  qtui/tools/compare_ui.py compares the two output directories.
//
//   QT_QPA_PLATFORM=offscreen qtui_dump VIDEO OUTDIR Y,X,H,W [MEM_GB]
#include <QApplication>
#include <QMenu>
#include <QMenuBar>
#include <QToolButton>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QSettings>
#include <QTextStream>
#include <QThread>

#include <cstdio>
#include <cstring>
#include <functional>
#include <fstream>
#include <set>

#include "filter_panel.h"
#include "mainwindow.h"
#include "profile_panel.h"
#include "pyobj.h"
#include "rows.h"
#include "timeline.h"

using namespace nvqt;

namespace {

// RGB888 bytes of a QImage (rows without padding), as py_dump.qimage_bytes
std::vector<uint8_t> qimage_bytes(const QImage& qi_in, int* H, int* W) {
    const QImage qi = qi_in.convertToFormat(QImage::Format_RGB888);
    *H = qi.height();
    *W = qi.width();
    std::vector<uint8_t> out(static_cast<size_t>(*H) * *W * 3);
    for (int y = 0; y < *H; ++y) std::memcpy(out.data() + static_cast<size_t>(y) * *W * 3, qi.constScanLine(y), static_cast<size_t>(*W) * 3);
    return out;
}

// the Filter Settings panel as py_dump.filter_dump writes it (filter_<tag>.json)
QString filter_dump(MainWindow& win, const QString& out) {
    FilterPanel* fpn = win.filter_panel;
    PyObj d = PyObj::dict(), ctl = PyObj::dict();
    for (const auto& [k, sp] : param_specs()) {
        auto it = fpn->controls.find(k);
        if (it == fpn->controls.end()) continue;
        QWidget* w = it->second;
        PyObj e = PyObj::dict();
        QString v = "?";
        if (auto* r = qobject_cast<Row*>(w))
            v = r->val->text();
        else if (auto* c = qobject_cast<EnableCheck*>(w))
            v = c->isChecked() ? "True" : "False";
        else if (auto* ch = qobject_cast<ChoiceRow*>(w))
            v = ch->box->currentText();
        e.set("val", PyObj::str(v.toStdString()));
        e.set("enabled", PyObj::boolean(w->isEnabled()));
        e.set("hidden", PyObj::boolean(w->isHidden()));
        auto* r = qobject_cast<Row*>(w);
        e.set("reset", r ? PyObj::boolean(!r->reset->isHidden()) : PyObj::none());
        e.set("slider", r ? PyObj::integer(r->slider->value()) : PyObj::none());
        ctl.set(k, e);
    }
    d.set("controls", ctl);
    PyObj common = PyObj::list();
    for (Row* w : fpn->common) {
        PyObj e = PyObj::dict();
        PyObj ks = PyObj::list();
        for (const auto& k : w->keys) ks.append(PyObj::str(k));
        e.set("keys", ks);
        e.set("val", PyObj::str(w->val->text().toStdString()));
        e.set("enabled", PyObj::boolean(w->isEnabled()));
        common.append(e);
    }
    d.set("common", common);
    PyObj headers = PyObj::list();                      // std::map: sorted by title, as the Python side sorts
    for (const auto& [t, h] : fpn->headers) {
        PyObj e = PyObj::dict();
        e.set("title", PyObj::str(t));
        e.set("text", PyObj::str(h->title->text().toStdString()));
        e.set("collapsed", PyObj::boolean(h->body->isHidden()));
        e.set("hidden", PyObj::boolean(h->isHidden()));
        e.set("enabled", PyObj::boolean(h->isEnabled()));
        headers.append(e);
    }
    d.set("headers", headers);
    PyObj te = PyObj::list();
    for (int i = 0; i < 3; ++i) te.append(PyObj::boolean(fpn->tabs->isTabEnabled(i)));
    d.set("tabs_enabled", te);
    d.set("tab", PyObj::integer(fpn->tabs->currentIndex()));
    d.set("nl_check", PyObj::str(fpn->nl_check->button->text().toStdString()));
    d.set("nl_title", PyObj::str(fpn->nl_check->title->text().toStdString()));
    d.set("assist", PyObj::str(fpn->assist->button->text().toStdString()));
    d.set("fsc_btn", PyObj::str(win.fsc_btn->text().toStdString()));
    d.set("fsc_checked", PyObj::boolean(win.fsc_btn->isChecked()));
    d.set("nlc_action", PyObj::boolean(win.actions["nlc"]->isChecked()));
    d.set("perf", PyObj::str(fpn->perf->text().toStdString()));
    d.set("name", PyObj::str(fpn->name->currentText().toStdString()));
    d.set("mode_lbl", PyObj::str(fpn->mode_lbl->text().toStdString()));
    PyObj sl = PyObj::list();
    for (Row* r : fpn->sharp_rows) sl.append(PyObj::str(r->label->text().toStdString()));
    d.set("sharp_labels", sl);
    d.set("msg", PyObj::str(QString(win.msg->text()).replace(out, "<OUT>").toStdString()));   // paths: the output dir
    d.set("prof_lbl", PyObj::str(win.prof_lbl->text().toStdString()));
    d.set("undo_index", PyObj::integer(win.state->undo->index()));
    d.set("undo_text", PyObj::str(win.state->undo->undoText().toStdString()));
    d.set("radius", PyObj::integer(win.timeline->thumbs->radius));
    d.set("viewer_mode", PyObj::str(win.viewer->mode()));
    d.set("params", win.state->params);
    return QString::fromStdString(d.dump(1));
}

bool wait_until(const std::function<bool()>& cond, double timeout_s = 600) {
    QElapsedTimer t;
    t.start();
    while (!cond()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 20);
        if (t.elapsed() > timeout_s * 1000) return false;
    }
    QCoreApplication::processEvents();
    return true;
}

void write(const QString& path, const QString& text) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return;
    f.write(text.toUtf8());
}

}  // namespace

int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", qgetenv("QT_QPA_PLATFORM").isEmpty() ? QByteArray("offscreen") : qgetenv("QT_QPA_PLATFORM"));
    if (argc < 4) {
        std::fprintf(stderr, "usage: qtui_dump VIDEO OUTDIR Y,X,H,W [MEM_GB]\n");
        return 2;
    }
    const QString video = argv[1], out = argv[2];
    const QStringList r = QString(argv[3]).split(',');
    const double mem = argc > 4 ? QString(argv[4]).toDouble() : 4.0;
    QDir().mkpath(out + "/panes");
    const QString ini = out + "/settings.ini";
    QFile::remove(ini);
    qputenv("NV_UI_QT_SETTINGS", ini.toLocal8Bit());
    {
        QSettings s(ini, QSettings::IniFormat);
        s.setValue("prefs/mem_gb", mem);
        s.sync();
    }
    QApplication app(argc, argv);
    QApplication::setApplicationName("nv_ui_qt");
    MainWindow win(video);
    win.show();
    auto idle = [&] { return win.runner->idle(); };
    auto aft_done = [&] { return win.state->profile.has_profile() && win.state->profile.base.get("aft_applied").truthy() && idle(); };
    std::printf("load + auto profile + AFT: %s\n", wait_until(aft_done) ? "ok" : "TIMEOUT");
    wait_until([&] { return idle() && win.viewer->ready(); }, 120);
    write(out + "/state_auto.json", win.dump_state());
    {
        // menu bar structure: titles, items (text, enabled, checkable, shortcut), separators / sections
        PyObj menus = PyObj::list();
        for (QAction* top : win.menuBar()->actions()) {
            PyObj m = PyObj::dict();
            m.set("title", PyObj::str(top->text().toStdString()));
            PyObj items = PyObj::list();
            std::function<void(QMenu*, const std::string&)> walk = [&](QMenu* menu, const std::string& pre) {
                for (QAction* a : menu->actions()) {
                    PyObj it = PyObj::dict();
                    it.set("text", PyObj::str(pre + a->text().toStdString()));
                    it.set("sep", PyObj::boolean(a->isSeparator()));
                    it.set("enabled", PyObj::boolean(a->isEnabled()));
                    it.set("checkable", PyObj::boolean(a->isCheckable()));
                    it.set("shortcut", PyObj::str(a->shortcut().toString().toStdString()));
                    items.append(it);
                    if (a->menu()) walk(a->menu(), pre + a->text().toStdString() + " > ");
                }
            };
            if (top->menu()) walk(top->menu(), "");
            m.set("items", items);
            menus.append(m);
        }
        write(out + "/menus.json", QString::fromStdString(menus.dump(1)));
        // toolbar buttons of both pages (text, enabled, checkable)
        PyObj tb = PyObj::list();
        for (int i = 0; i < win.tb_stack->count(); ++i)
            for (QToolButton* b : win.tb_stack->widget(i)->findChildren<QToolButton*>(Qt::FindDirectChildrenOnly)) {
                PyObj it = PyObj::dict();
                it.set("page", PyObj::integer(i));
                it.set("text", PyObj::str(b->text().toStdString()));
                it.set("enabled", PyObj::boolean(b->isEnabled()));
                it.set("checkable", PyObj::boolean(b->isCheckable()));
                it.set("tip", PyObj::str(b->toolTip().toStdString()));
                tb.append(it);
            }
        write(out + "/toolbars.json", QString::fromStdString(tb.dump(1)));
    }
    win.grab().save(out + "/win_prepare_auto.png");

    const int ry = r[0].toInt(), rx = r[1].toInt(), rh = r[2].toInt(), rw = r[3].toInt();
    win.on_area(ry, rx, rh, rw);
    wait_until([&] { return idle() && win.viewer->ready(); }, 120);
    write(out + "/state_sel.json", win.dump_state());
    win.grab().save(out + "/win_prepare_sel.png");
    win.viewer->grab().save(out + "/viewer_sel.png");

    win.build_profile(false);
    auto built = [&] {
        const PyObj& b = win.state->profile.base;
        return win.state->profile.has_profile() && b.get("aft_applied").truthy() && b.get("area").size() >= 3 &&
               b.get("area")[1].as_int() == rx && b.get("area")[0].as_int() == ry && idle();
    };
    std::printf("build profile + AFT: %s\n", wait_until(built) ? "ok" : "TIMEOUT");
    wait_until([&] { return idle() && win.viewer->ready(); }, 120);
    write(out + "/state_build.json", win.dump_state());
    win.grab().save(out + "/win_prepare_build.png");
    win.viewer->grab().save(out + "/viewer_build.png");

    // pane images of every mode at three brightnesses (Prepare page: emphasis on)
    PyObj index = PyObj::list();
    auto dump_panes = [&](const std::string& tag) {
        const std::string src = win.viewer->source();
        for (const auto& [mode, _] : MODES) {
            win.viewer->set_mode(mode);
            for (int b : {50, 23, 77}) {
                win.viewer_bar->bright->setValue(b);
                wait_until([&] { return win.viewer->ready(); }, 120);
                for (const auto& [name, rect] : win.viewer->panes()) {
                    (void)rect;
                    int H = 0, W = 0;
                    const auto px = win.viewer->pane_pixels(name, name == "rgb" ? src : (src == "orig" || src == "pcheck" || src == "nlc" ? src : "filt"), &H, &W);
                    std::string fn = tag + "_" + mode + "_" + std::to_string(b) + "_" + name;
                    for (auto& c : fn)
                        if (c == ' ' || c == '+') c = c == ' ' ? '_' : 'P';
                    std::ofstream f((out + "/panes/" + QString::fromStdString(fn) + ".rgb").toStdString(), std::ios::binary);
                    f.write(reinterpret_cast<const char*>(px.data()), static_cast<std::streamsize>(px.size()));
                    PyObj e = PyObj::dict();
                    e.set("file", PyObj::str(fn));
                    e.set("H", PyObj::integer(H));
                    e.set("W", PyObj::integer(W));
                    index.append(e);
                }
            }
        }
        win.viewer_bar->bright->setValue(50);
    };
    dump_panes("prepare");
    win.viewer->set_mode("YCrCb +");
    wait_until([&] { return win.viewer->ready(); }, 120);
    win.viewer->grab().save(out + "/viewer_ycc_build.png");

    // Profile Check (Assist Image)
    win.set_profile_check(true);
    wait_until([&] { return win.viewer->showing_pcheck() && idle() && win.viewer->ready(); }, 300);
    dump_panes("pcheck");
    win.viewer->set_mode("YCrCb +");
    wait_until([&] { return win.viewer->ready(); }, 120);
    win.grab().save(out + "/win_pcheck.png");
    win.set_profile_check(false);

    // Adjust and Preview: the filtered preview (whole frame, then the preview area)
    win.tabs->setCurrentIndex(1);
    wait_until([&] { return idle() && win.viewer->showing_filtered(); }, 600);
    wait_until([&] { return idle() && win.viewer->ready(); }, 120);
    dump_panes("adjust");
    win.viewer->set_mode("Normal");
    write(out + "/state_adjust.json", win.dump_state());
    win.grab().save(out + "/win_adjust.png");
    win.area_btn->setChecked(true);
    wait_until([&] { return idle() && win.viewer->showing_filtered() && win.preview_sel; }, 600);
    wait_until([&] { return idle(); }, 600);
    {
        int H = 0, W = 0;
        const auto px = win.viewer->pane_pixels("rgb", win.viewer->source(), &H, &W);
        std::ofstream f((out + "/panes/adjust_area_rgb.rgb").toStdString(), std::ios::binary);
        f.write(reinterpret_cast<const char*>(px.data()), static_cast<std::streamsize>(px.size()));
        PyObj e = PyObj::dict();
        e.set("file", PyObj::str("adjust_area_rgb"));
        e.set("H", PyObj::integer(H));
        e.set("W", PyObj::integer(W));
        index.append(e);
    }
    win.grab().save(out + "/win_adjust_area.png");

    // ---- N3b: the Adjust page (py_dump.adjust_steps)
    AppState* st = win.state;
    FilterPanel* fpn = win.filter_panel;
    auto pane = [&](const std::string& name, const std::string& fn) {
        int H = 0, W = 0;
        const auto px = win.viewer->pane_pixels(name, win.viewer->source(), &H, &W);
        std::ofstream f((out + "/panes/" + QString::fromStdString(fn) + ".rgb").toStdString(), std::ios::binary);
        f.write(reinterpret_cast<const char*>(px.data()), static_cast<std::streamsize>(px.size()));
        PyObj e = PyObj::dict();
        e.set("file", PyObj::str(fn));
        e.set("H", PyObj::integer(H));
        e.set("W", PyObj::integer(W));
        index.append(e);
    };
    auto wf = [&](const char* fn) { write(out + "/" + fn, filter_dump(win, out)); };
    auto filtered = [&] { return idle() && win.viewer->showing_filtered(); };
    win.area_btn->setChecked(false);
    wait_until(filtered, 600);
    wf("filter_adjust.json");
    for (const auto& [i, nm] : std::vector<std::pair<int, QString>>{{1, "spatial"}, {2, "general"}}) {
        fpn->tabs->setCurrentIndex(i);
        QCoreApplication::processEvents();
        win.grab().save(out + "/win_adjust_" + nm + ".png");
    }
    fpn->tabs->setCurrentIndex(0);
    win.set_advanced(false);
    QCoreApplication::processEvents();
    win.grab().save(out + "/win_adjust_beginner.png");
    wf("filter_beginner.json");
    win.set_advanced(true);
    QCoreApplication::processEvents();
    st->new_edit();
    st->set_param("amt_Y", PyObj::real(50.0));
    st->new_edit();
    st->set_params({{"bamt_Y_medium", PyObj::real(60.0)}, {"bamt_Cr_medium", PyObj::real(60.0)}, {"bamt_Cb_medium", PyObj::real(60.0)}},
                   "Change filter settings");
    st->new_edit();
    st->set_param("bamt_Cr_low", PyObj::real(30.0));
    st->new_edit();
    st->set_params({{"mix_enable", PyObj::boolean(true)}, {"mix", PyObj::real(25.0)}}, "Change filter settings");
    st->new_edit();
    st->set_param("t_noise", PyObj::real(60.0));
    st->new_edit();
    st->set_param("t_radius", PyObj::integer(1));
    st->new_edit();
    st->set_param("adv_noise_level", PyObj::real(150.0));
    wait_until([&] { return filtered() && !win.preview_pending(); }, 600);
    wf("filter_edited.json");
    write(out + "/state_edited.json", win.dump_state());
    pane("rgb", "adjust_edit_rgb");
    fpn->tabs->setCurrentIndex(1);
    QCoreApplication::processEvents();
    win.grab().save(out + "/win_adjust_edit.png");
    fpn->tabs->setCurrentIndex(0);
    st->undo->undo();
    st->undo->undo();
    QCoreApplication::processEvents();
    wf("filter_undo2.json");
    st->undo->redo();
    QCoreApplication::processEvents();
    wf("filter_redo1.json");
    win.save_preset(out + "/preset.nvpreset.json");
    win.save_profile(out + "/profile.nvprof.json");
    wf("filter_saved.json");
    // Noise Level Check
    win.set_noise_level_check(true);
    auto nlc_shown = [&] { return idle() && win.viewer->showing_nlc() && win.nlc_ready(); };
    wait_until(nlc_shown, 600);
    pane("rgb", "nlc_rgb");
    win.grab().save(out + "/win_nlc.png");
    wf("filter_nlc.json");
    st->new_edit();
    st->set_param("t_noise", PyObj::real(40.0));
    wait_until(nlc_shown, 600);
    pane("rgb", "nlc_rgb40");
    wf("filter_nlc40.json");
    win.set_noise_level_check(false);
    wait_until(filtered, 600);
    wf("filter_nlc_off.json");
    // thumbnails
    {
        const int n = static_cast<int>(win.frames.size());
        std::set<int> ids = {0, win.timeline->frame(), n - 1};
        for (int i : ids) {
            int H = 0, W = 0;
            const auto px = qimage_bytes(win.thumb(i), &H, &W);
            const std::string fn = "thumb_" + std::to_string(i);
            std::ofstream f((out + "/panes/" + QString::fromStdString(fn) + ".rgb").toStdString(), std::ios::binary);
            f.write(reinterpret_cast<const char*>(px.data()), static_cast<std::streamsize>(px.size()));
            PyObj e = PyObj::dict();
            e.set("file", PyObj::str(fn));
            e.set("H", PyObj::integer(H));
            e.set("W", PyObj::integer(W));
            index.append(e);
        }
    }
    win.timeline->grab().save(out + "/timeline.png");
    win.performance_tab();
    QCoreApplication::processEvents();
    win.grab().save(out + "/win_perf.png");
    fpn->tabs->setCurrentIndex(0);
    // exports: (a) the current (edited) settings, CRF 0, 4 frames; (b) the defaults, CRF 16, 12 frames
    win.prefs["crf"] = 0;
    win.process_video(out + "/export_a.mp4", 300, 4);
    const bool ok_a = wait_until([&] { return win.export_ == nullptr; }, 1800) && win.export_ok;
    win.reset_filter();
    win.prefs["crf"] = 16;
    win.process_video(out + "/export_b.mp4", 300, 12);
    const bool ok_b = wait_until([&] { return win.export_ == nullptr; }, 1800) && win.export_ok;
    {
        PyObj e = PyObj::dict();
        e.set("export_a", PyObj::boolean(ok_a));
        e.set("export_b", PyObj::boolean(ok_b));
        e.set("msg", PyObj::str(win.msg->text().toStdString()));
        write(out + "/exports.json", QString::fromStdString(e.dump(1)));
    }
    wf("filter_final.json");
    write(out + "/panes/index.json", QString::fromStdString(index.dump(1)));
    std::printf("done\n");
    win.close();
    return 0;
}
