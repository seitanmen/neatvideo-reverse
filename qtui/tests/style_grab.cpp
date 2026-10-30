// qtui_style_grab - pictures of the main window for the look comparison with the genuine Neat Video window
// (sandbox/omo/wp_qtui_style/ref/genuine.png).  The window is not shown: QWidget::grab() paints it off screen, so it
// runs on the native platform (the look the user sees) as well as on offscreen; only NV_GRAB_SHOW (offscreen only)
// shows it, on the offscreen platform.
//
//   qtui/build_main/qtui_style_grab <out_dir> [video]          (video: $NV_TEST_VIDEO or <repo>/test_v02.mp4)
//   QT_QPA_PLATFORM=offscreen NV_GRAB_SHOW=1 qtui/build_main/qtui_style_grab <out_dir>   (shown off screen, see below)
//   NV_GRAB_NO_LOOK=1: without apply_look (the look before look.h)
//
// Writes <out_dir>/prepare.png (Prepare page: Auto Profile + Auto Fine-Tune of the loaded frames, a selection as in
// genuine.png) and <out_dir>/adjust.png (Adjust and Preview page) (NV_GRAB_SHOW: also preferences.png and
// profile_viewer.png), plus <out_dir>/settings.ini (a fresh settings file,
// so the user's ~/.config/NeatRepro/nv_ui_qt.ini is neither read nor written).
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QElapsedTimer>
#include <QMenuBar>
#include <QScreen>
#include <QSettings>
#include <QTabBar>

#include <cstdio>
#include <functional>

#include "look.h"
#include "mainwindow.h"
#include "viewer.h"

using namespace nvqt;

namespace {

bool wait_until(const std::function<bool()>& cond, double timeout_s = 300) {
    QElapsedTimer t;
    t.start();
    while (!cond()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 20);
        if (t.elapsed() > timeout_s * 1000) return false;
    }
    for (int i = 0; i < 5; ++i) QCoreApplication::processEvents();
    return true;
}

void settle(int ms) {                            // the viewer panes are filled by the workers after "idle"
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms) QCoreApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents, 20);
}

bool save(QWidget* w, const QString& path) {
    if (qobject_cast<MainWindow*>(w)) w->resize(1800, 1010);   // again: on cocoa the hidden window falls back to its first size once it has a platform window
    w->grab();                   // the first paint asks the workers for the viewer panes (pane_image)
    settle(2500);
    const QPixmap px = w->grab();
    std::printf("%s %dx%d dpr %.1f (widget %dx%d)\n", qPrintable(path), px.width(), px.height(), px.devicePixelRatio(),
                w->width(), w->height());
    return px.save(path);
}

}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("nv_ui_qt");
    if (!qEnvironmentVariableIsSet("NV_GRAB_NO_LOOK")) apply_look(app);   // as MeatVideoQt (main.cpp)
    if (argc < 2) {
        std::fprintf(stderr, "usage: qtui_style_grab <out_dir> [video]\n");
        return 2;
    }
    const QString out = QString::fromLocal8Bit(argv[1]);
    QDir().mkpath(out);
    const QByteArray ev = qgetenv("NV_TEST_VIDEO");
    const QString video = argc > 2 ? QString::fromLocal8Bit(argv[2])
                          : ev.isEmpty() ? QString(NV_SOURCE_ROOT) + "/test_v02.mp4"
                                         : QString::fromLocal8Bit(ev);
    const QString ini = QDir(out).filePath("settings.ini");
    QFile::remove(ini);
    qputenv("NV_UI_QT_SETTINGS", ini.toLocal8Bit());
    {
        QSettings s(ini, QSettings::IniFormat);
        s.setValue("prefs/mem_gb", 0.05);       // 1080p: the first 16 frames
        s.sync();
    }
    set_ffmpeg_dir(find_ffmpeg_dir(""));
    std::printf("platform %s, style %s\n", qPrintable(QGuiApplication::platformName()),
                qPrintable(QApplication::style()->name()));

    MainWindow win(video);
    // NV_GRAB_SHOW=1 (offscreen only, never on a real screen): show() the window, so that the parts laid out on the
    // show event (the menu bar's corner widget "Advanced Mode") are placed as in the running app
    if (qEnvironmentVariableIsSet("NV_GRAB_SHOW")) {
        if (QGuiApplication::platformName() != "offscreen") {
            std::fprintf(stderr, "NV_GRAB_SHOW needs QT_QPA_PLATFORM=offscreen\n");
            return 2;
        }
        win.show();
    }
    win.resize(1800, 1010);                      // the client area of genuine.png (3600x2076 at 2x, less the title)
    if (QWidget* cw = win.menuBar()->cornerWidget())
        std::printf("menu bar native %d hidden %d, corner hidden %d visibleTo %d geom %d,%d %dx%d\n",
                    win.menuBar()->isNativeMenuBar(), win.menuBar()->isHidden(), cw->isHidden(), cw->isVisibleTo(&win),
                    cw->x(), cw->y(), cw->width(), cw->height());
    std::printf("window %dx%d, screen %dx%d\n", win.width(), win.height(), win.screen()->availableGeometry().width(),
                win.screen()->availableGeometry().height());
    const auto done = [&] {
        return win.runner->idle() && win.state->profile.has_profile() &&
               win.state->profile.base.get("aft_applied").truthy() && win.timers_idle();
    };
    if (!wait_until(done)) {
        std::fprintf(stderr, "timeout: profile / Auto Fine-Tune\n");
        return 1;
    }
    const int W = win.info ? win.info->W : 1920, H = win.info ? win.info->H : 1080;
    win.on_area(H * 875 / 1080, W * 769 / 1920, H * 43 / 1080, W * 208 / 1920);   // the selection of genuine.png
    wait_until([&] { return win.runner->idle() && win.timers_idle(); });
    if (!save(&win, QDir(out).filePath("prepare.png"))) return 1;
    win.tabs->setCurrentIndex(1);
    wait_until([&] { return win.runner->idle() && win.timers_idle() && win.viewer->showing_filtered(); }, 120);
    if (!save(&win, QDir(out).filePath("adjust.png"))) return 1;
    win.tabs->setCurrentIndex(0);
    wait_until([&] { return win.runner->idle() && win.timers_idle(); });
    if (win.isVisible()) {                       // NV_GRAB_SHOW (offscreen): the dialogs too (they show themselves)
        QDialog* d = win.preferences();
        if (!save(d, QDir(out).filePath("preferences.png"))) return 1;
        d->reject();
        d = win.profile_viewer();
        if (!save(d, QDir(out).filePath("profile_viewer.png"))) return 1;
        d->reject();
    }
    return 0;
}
