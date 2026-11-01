// qtui_ofx_session_drv - test driver of the OFX plug-in's window (N4d, ofx/tools/verify_ofx.py): runs
// nvqt::ofx_session_main (the request on stdin, the reply on stdout, as MeatVideoQt --ofx-session) and, once the
// window is up, the steps of $NV_OFX_SESSION_SCRIPT (";"-separated).  After every step it waits until the UI is idle
// and prints "MSG: <status bar text>" to stderr.  Steps:
//   wait | apply | cancel | close | auto (Auto Profile, Current Frame) | aft (Auto Fine-Tune) | frame N | tab N |
//   resetprofile (Profile > Reset Profile) | resetfilter (Filter > Reset Filter Settings) | sleep MS |
//   set KEY VALUE (a float parameter) | setb KEY 0|1 | tune KEY VALUE (profile Noise Level) |
//   state PATH (clip_params_json of the current state) |
//   process PATH (the C++ UI's processing of the current frame with the current state - the window of the frames it
//                 holds, temporal + finish as ofx.cpp ofx_process - as the raw gbrpf32le frame of nv_depth.from_internal
//                 with the frame's restore info) |
//   frames PATH (the frames as the window holds them: Y, Cb, Cr float32 planes of each frame)
// With --ofx-default it is nvqt::ofx_default_main (no window), as MeatVideoQt --ofx-default.
// Used as the plug-in's helper through MEATVIDEO_OFX_HELPER (tests only).
#include <QAction>
#include <QApplication>
#include <QPushButton>
#include <QTimer>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>

#include "depth.h"
#include "mainwindow.h"
#include "ofx_session.h"
#include "timeline.h"

using namespace nvqt;

namespace {

bool c_is_close(const QStringList& w) { return !w.isEmpty() && (w[0] == "close" || w[0] == "cancel" || w[0] == "apply"); }

void write_file(const QString& path, const void* data, size_t n) {
    std::ofstream f(path.toStdString(), std::ios::binary);
    f.write(static_cast<const char*>(data), static_cast<std::streamsize>(n));
}

void process(MainWindow* win, const OfxSession& s, const QString& path) {
    const int t = win->timeline->frame();
    const PyObj P = win->state->backend();
    const PyObj* eff = win->state->effective();
    if (!eff) {
        std::fprintf(stderr, "process: no profile\n");
        return;
    }
    auto [idx, planes] = win->engine->window(t, static_cast<int>(P.get("radius").as_int()), std::nullopt);
    const int ci = static_cast<int>(std::find(idx.begin(), idx.end(), t) - idx.begin());
    Planes tmp, rv;
    temporal(planes, ci, *eff, P, nullptr, &tmp, &rv);
    const Planes out = finish(planes[static_cast<size_t>(ci)], tmp, rv, *eff, P);
    const RestoreMap& rs = s.restore[static_cast<size_t>(t)];
    const std::vector<uint8_t> bytes = from_internal(out, pixfmt("gbrpf32le"), win->matrix, rs.empty() ? nullptr : &rs);
    write_file(path, bytes.data(), bytes.size());
    std::fprintf(stderr, "PROCESS: frame %d (time %g) window [%d..%d] -> %s\n", t, s.times[static_cast<size_t>(t)],
                 idx.front(), idx.back(), path.toUtf8().constData());
}

void run_steps(MainWindow* win, std::shared_ptr<QStringList> steps, std::shared_ptr<OfxSession> s) {
    auto tick = std::make_shared<std::function<void()>>();
    *tick = [win, steps, tick, s] {
        if (!win->runner->idle() || !win->timers_idle()) {
            QTimer::singleShot(20, win, *tick);
            return;
        }
        if (steps->isEmpty()) return;
        const QStringList w = steps->takeFirst().trimmed().split(' ', Qt::SkipEmptyParts);
        if (!w.isEmpty()) {
            const QString& c = w[0];
            std::fprintf(stderr, "STEP: %s\n", w.join(' ').toUtf8().constData());
            if (c == "apply")
                win->b_apply->click();
            else if (c == "cancel")
                win->b_cancel->click();
            else if (c == "close")
                win->close();
            else if (c == "auto")
                win->auto_profile(false);
            else if (c == "aft")
                win->auto_fine_tune();
            else if (c == "resetprofile")
                win->actions.at("p_reset")->trigger();
            else if (c == "resetfilter")
                win->actions.at("f_reset")->trigger();
            else if (c == "frame" && w.size() > 1)
                win->timeline->set_frame(w[1].toInt(), true);
            else if (c == "tab" && w.size() > 1)
                win->tabs->setCurrentIndex(w[1].toInt());
            else if (c == "set" && w.size() > 2)
                win->state->set_param(w[1].toStdString(), PyObj::real(w[2].toDouble()));
            else if (c == "setb" && w.size() > 2)
                win->state->set_param(w[1].toStdString(), PyObj::boolean(w[2].toInt() != 0));
            else if (c == "tune" && w.size() > 2)
                win->state->set_tune(w[1].toStdString(), w[2].toDouble());
            else if (c == "state" && w.size() > 1) {
                const std::string cp = clip_params_json(win);
                write_file(w[1], cp.data(), cp.size());
            } else if (c == "process" && w.size() > 1) {
                process(win, *s, w[1]);
            } else if (c == "frames" && w.size() > 1) {
                std::ofstream f(w[1].toStdString(), std::ios::binary);
                for (const Frame& fr : win->frames)
                    for (const std::vector<float>* p : {&fr.fY, &fr.fCb, &fr.fCr})
                        f.write(reinterpret_cast<const char*>(p->data()), static_cast<std::streamsize>(p->size() * 4));
            }
        }
        if (c_is_close(w)) return;
        // sleep MS: the next step that much later (the event loop runs meanwhile)
        const int delay = !w.isEmpty() && w[0] == "sleep" && w.size() > 1 ? w[1].toInt() : 20;
        auto after = std::make_shared<std::function<void()>>();
        *after = [win, tick, after] {
            if (!win->runner->idle() || !win->timers_idle()) {
                QTimer::singleShot(20, win, *after);
                return;
            }
            std::fprintf(stderr, "MSG: %s\n", win->msg->text().toUtf8().constData());
            std::fflush(stderr);
            (*tick)();
        };
        QTimer::singleShot(delay, win, *after);
    };
    (*tick)();
}

}  // namespace

int main(int argc, char** argv) {
    if (argc >= 2 && std::string(argv[1]) == "--ofx-default") {
        QCoreApplication app(argc, argv);
        QCoreApplication::setApplicationName("nv_ui_qt");
        return ofx_default_main();
    }
    QApplication app(argc, argv);
    QApplication::setApplicationName("nv_ui_qt");
    QStringList args = QApplication::arguments().mid(1);
    if (args.isEmpty() || args[0] != "--ofx-session") {
        std::fprintf(stderr, "usage: qtui_ofx_session_drv --ofx-session  (script: $NV_OFX_SESSION_SCRIPT)\n");
        return 2;
    }
    auto steps = std::make_shared<QStringList>(qEnvironmentVariable("NV_OFX_SESSION_SCRIPT").split(';', Qt::SkipEmptyParts));
    return ofx_session_main(args.mid(1), [steps](MainWindow* win, const OfxSession& s) {
        run_steps(win, steps, std::make_shared<OfxSession>(s));
    });
}
