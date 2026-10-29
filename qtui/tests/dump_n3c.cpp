// qtui_dump_n3c - the N3c comparison scenario of qtui/tools/py_dump_n3c.py on the C++ main window (the same steps,
// the same output files): Preferences, Profile Viewer, Batch Profiler and the Bit depth setting (loading, display,
// export at 10 bits, 4:2:2 and 32-bit float).  qtui/tools/compare_n3c.py runs both and compares them.
//
//   QT_QPA_PLATFORM=offscreen qtui_dump_n3c OUTDIR SRC10 SRC422
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTextEdit>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>

#include "filter_panel.h"
#include "mainwindow.h"
#include "dump_util.h"
#include "pyobj.h"

using namespace nvqt;
using namespace nvqt::dump;

namespace {

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

void write(const QString& path, const std::string& text) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return;
    f.write(QByteArray::fromStdString(text));
}

}  // namespace

int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", qgetenv("QT_QPA_PLATFORM").isEmpty() ? QByteArray("offscreen") : qgetenv("QT_QPA_PLATFORM"));
    if (argc < 4) {
        std::fprintf(stderr, "usage: qtui_dump_n3c OUTDIR SRC10 SRC422\n");
        return 2;
    }
    const QString out = argv[1], src10 = argv[2], src422 = argv[3];
    const QString video = QString(NV_SOURCE_ROOT) + "/test.mp4";
    QDir().mkpath(out + "/panes");
    const QString ini = out + "/settings.ini";
    QFile::remove(ini);
    qputenv("NV_UI_QT_SETTINGS", ini.toLocal8Bit());
    {
        QSettings s(ini, QSettings::IniFormat);
        s.setValue("prefs/mem_gb", 0.05);
        s.sync();
    }
    QApplication app(argc, argv);
    QApplication::setApplicationName("nv_ui_qt");
    MainWindow win(QDir::cleanPath(video));
    win.show();
    auto idle = [&] { return win.runner->idle(); };
    auto settled = [&] { return idle() && !win.preview_pending() && win.timers_idle() && win.viewer->ready(); };
    auto loaded = [&](const QString& path) {
        return [&win, path, idle] {
            return win.src == path && win.state->profile.has_profile() &&
                   win.state->profile.base.get("aft_applied").truthy() && idle();
        };
    };
    const QString v = QDir::cleanPath(video);
    std::printf("t8: %s\n", wait_until(loaded(v)) ? "ok" : "TIMEOUT");
    std::fflush(stdout);
    wait_until(settled, 120);
    write(out + "/state_t8.json", state_with(win, extras(win)));
    write(out + "/menus.json", menus(win).dump(1));
    win.grab().save(out + "/win_t8.png");

    // Preferences
    {
        QDialog* dlg = win.preferences();
        QCoreApplication::processEvents();
        auto* form = dlg->findChild<QFormLayout*>();
        PyObj d = PyObj::dict();
        d.set("title", S(dlg->windowTitle()));
        d.set("rows", form_rows(form));
        d.set("profile", PyObj::none());
        write(out + "/prefs_rows.json", d.dump(1));
        dlg->grab().save(out + "/dlg_prefs.png");
        for (QSpinBox* s : dlg->findChildren<QSpinBox*>())
            if (!qobject_cast<QDoubleSpinBox*>(s)) {
                s->setValue(7);            // rejected: kept
                break;
            }
        dlg->reject();
        QCoreApplication::processEvents();
    }
    // Profile Viewer (the mode combo writes prefs/profile_mode at once)
    {
        QDialog* dlg = win.profile_viewer();
        QCoreApplication::processEvents();
        PyObj pv = PyObj::dict();
        pv.set("title", S(dlg->windowTitle()));
        pv.set("rows", form_rows(dlg->findChild<QFormLayout*>()));
        pv.set("details", S(dlg->findChild<QTextEdit*>()->toPlainText()));
        pv.set("profile", PyObj::none());
        dlg->grab().save(out + "/dlg_pviewer.png");
        auto* combo = dlg->findChild<QComboBox*>();
        combo->setCurrentText("spatial");
        pv.set("profile_mode_after_change", S(QSettings(ini, QSettings::IniFormat).value("prefs/profile_mode").toString()));
        pv.set("prefs_profile_mode", S(win.prefs["profile_mode"].toString()));
        combo->setCurrentText("auto");
        write(out + "/state_pviewer.json", pv.dump(1));
        dlg->close();
        QCoreApplication::processEvents();
        win.reset_profile();
        dlg = win.profile_viewer();
        QCoreApplication::processEvents();
        PyObj pn = PyObj::dict();
        pn.set("rows", form_rows(dlg->findChild<QFormLayout*>()));
        pn.set("details", S(dlg->findChild<QTextEdit*>()->toPlainText()));
        pn.set("profile", PyObj::none());
        write(out + "/state_pviewer_none.json", pn.dump(1));
        dlg->grab().save(out + "/dlg_pviewer_none.png");
        dlg->close();
        win.state->undo->undo();
        QCoreApplication::processEvents();
    }
    // Batch Profiler
    {
        QDialog* dlg = win.batch_profiler();
        QCoreApplication::processEvents();
        PyObj d = PyObj::dict();
        d.set("title", S(dlg->windowTitle()));
        PyObj bt = PyObj::list();
        for (QPushButton* b : dlg->findChildren<QPushButton*>()) bt.append(S(b->text()));
        d.set("buttons", bt);
        PyObj modes = PyObj::list();
        auto* cb = dlg->findChild<QComboBox*>();
        for (int i = 0; i < cb->count(); ++i) modes.append(S(cb->itemText(i)));
        d.set("modes", modes);
        d.set("list", PyObj::integer(dlg->findChild<QListWidget*>()->count()));
        d.set("profile", PyObj::none());
        write(out + "/state_batchdlg.json", d.dump(1));
        dlg->grab().save(out + "/dlg_batch.png");
        dlg->reject();
        QCoreApplication::processEvents();
    }
    PyObj bmsg = PyObj::list();
    auto batch = [&](const QStringList& files, const QString& tag, const QString& mode = QString()) {
        const QString d = out + "/batch_" + tag;
        QDir().mkpath(d);
        win.batch_result.reset();
        win.batch_profiler(&files, d, mode);
        wait_until([&] { return idle() && win.batch_result.has_value(); }, 600);
        for (const QString& p : *win.batch_result) {
            const QString to = out + "/batch_" + tag + "_" + QFileInfo(p).fileName();
            QFile::remove(to);
            QFile::copy(p, to);
        }
        const QString m = QString(win.msg->text()).replace(out, "<OUT>");
        std::printf("batch %s: %d profiles; %s\n", tag.toUtf8().constData(), static_cast<int>(win.batch_result->size()),
                    m.toUtf8().constData());
        std::fflush(stdout);
        bmsg.append(S(m));
    };
    batch({v, src10}, "d8");
    batch({src422}, "d8s", "spatial");

    PyObj exports = PyObj::dict();
    auto do_export = [&](const QString& tag, const QString& ext, int start, int frames) {
        win.process_video(out + "/export_" + tag + ext, start, frames);
        wait_until([&] { return win.export_ == nullptr; }, 600);
        exports.set(tag.toStdString(), PyObj::boolean(win.export_ok));
        std::printf("export %s: %d %s\n", tag.toUtf8().constData(), win.export_ok ? 1 : 0, win.msg->text().toUtf8().constData());
        std::fflush(stdout);
    };
    auto prefs_depth = [&](const QString& val) {
        QDialog* d = win.preferences();
        QCoreApplication::processEvents();
        for (QComboBox* c : d->findChildren<QComboBox*>())
            if (c->findData(val) >= 0) {
                c->setCurrentIndex(c->findData(val));
                break;
            }
        d->accept();
        QCoreApplication::processEvents();
    };
    PyObj index = PyObj::list();
    auto panes = [&](const std::string& tag, std::vector<std::string> modes) {
        for (const auto& mode : modes) {
            win.viewer->set_mode(mode);
            wait_until([&] { return win.viewer->ready(); }, 120);
            const std::string src = win.viewer->source();
            for (const auto& [name, rect] : win.viewer->panes()) {
                (void)rect;
                int H = 0, W = 0;
                const auto px = win.viewer->pane_pixels(
                    name, name == "rgb" ? src : (src == "orig" || src == "pcheck" || src == "nlc" ? src : "filt"), &H, &W);
                std::string fn = tag + "_" + mode + "_" + name;
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
    };
    const std::vector<std::string> two = {"Normal", "YCrCb +"};

    prefs_depth("10");
    std::printf("t10: %s\n", wait_until([&] { return win.depth_used == "10" && loaded(v)(); }) ? "ok" : "TIMEOUT");
    std::fflush(stdout);
    wait_until(settled, 120);
    write(out + "/state_t10.json", state_with(win, extras(win)));
    panes("t10", two);
    win.viewer->set_mode("YCrCb +");
    wait_until(settled, 120);
    win.grab().save(out + "/win_t10.png");

    win.open_video(src10);
    std::printf("s10: %s\n", wait_until(loaded(src10)) ? "ok" : "TIMEOUT");
    std::fflush(stdout);
    wait_until(settled, 120);
    write(out + "/state_s10.json", state_with(win, extras(win)));
    panes("s10", two);
    win.tabs->setCurrentIndex(1);
    wait_until([&] { return idle() && win.viewer->showing_filtered(); }, 600);
    wait_until(settled, 120);
    panes("s10adj", {"Normal"});
    win.grab().save(out + "/win_s10_adjust.png");
    win.tabs->setCurrentIndex(0);
    wait_until(settled, 120);
    do_export("s10", ".mp4", 2, 3);
    batch({src10}, "d10");

    prefs_depth("source");
    wait_until([&] { return win.depth_used == "source" && loaded(src10)(); }, 600);
    win.open_video(src422);
    std::printf("s422: %s\n", wait_until(loaded(src422)) ? "ok" : "TIMEOUT");
    std::fflush(stdout);
    wait_until(settled, 120);
    write(out + "/state_s422.json", state_with(win, extras(win)));
    panes("s422", two);
    do_export("s422", ".mp4", 1, 3);

    prefs_depth("32");
    std::printf("s422_32: %s\n", wait_until([&] { return win.depth_used == "32" && loaded(src422)(); }) ? "ok" : "TIMEOUT");
    std::fflush(stdout);
    wait_until(settled, 120);
    write(out + "/state_s422_32.json", state_with(win, extras(win)));
    panes("s422_32", two);
    win.grab().save(out + "/win_s422_32.png");
    do_export("s422_32", ".mkv", 1, 3);
    batch({src422}, "d32");

    write(out + "/panes/index.json", index.dump(1));
    PyObj runs = PyObj::dict();
    runs.set("exports", exports);
    runs.set("batch_msgs", bmsg);
    write(out + "/runs.json", runs.dump(1));
    std::printf("done\n");
    win.close();
    return 0;
}
