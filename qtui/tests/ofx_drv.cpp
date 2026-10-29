// qtui_ofx_drv - test driver of the OFX host mode (qtui/tools/compare_ofx.py): runs nvqt::ofx_main with the arguments
// after the program name (--ofx W H ...) and, once the window is up, the steps of $NV_OFX_SCRIPT (";"-separated, the
// same script as qtui/tools/py_ofx_drv.py for the Python UI).  After every step it waits until the UI is idle and
// prints "MSG: <status bar text>" to stderr.  Steps: wait | apply | cancel | close | auto (Auto Profile, Current
// Frame) | frame N | tab N | set KEY VALUE (a float parameter) | setb KEY 0|1 | tune KEY VALUE (profile Noise Level) |
// dump DIR (menus_ofx.json, state_ofx.json and win_ofx.png of the window, as py_ofx_drv.py writes them).
#include <QApplication>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

#include <cstdio>
#include <memory>

#include "dump_util.h"
#include "mainwindow.h"
#include "ofx.h"
#include "timeline.h"

using namespace nvqt;

namespace {

bool c_is_close(const QStringList& w) { return !w.isEmpty() && (w[0] == "close" || w[0] == "cancel"); }

void run_steps(MainWindow* win, std::shared_ptr<QStringList> steps) {
    auto tick = std::make_shared<std::function<void()>>();
    *tick = [win, steps, tick] {
        if (!win->runner->idle() || !win->timers_idle()) {
            QTimer::singleShot(20, win, *tick);
            return;
        }
        if (steps->isEmpty()) return;
        const QStringList w = steps->takeFirst().trimmed().split(' ', Qt::SkipEmptyParts);
        if (!w.isEmpty()) {
            const QString& c = w[0];
            if (c == "apply")
                win->b_apply->click();
            else if (c == "cancel")
                win->b_cancel->click();
            else if (c == "close")
                win->close();
            else if (c == "auto")
                win->auto_profile(false);
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
            else if (c == "dump" && w.size() > 1) {
                QFile mf(w[1] + "/menus_ofx.json");
                if (mf.open(QIODevice::WriteOnly)) mf.write(QByteArray::fromStdString(dump::menus(*win).dump(1)));
                PyObj ex = dump::extras(*win);
                ex.set("apply_tip", dump::S(win->b_apply->toolTip()));
                ex.set("load_enabled", PyObj::boolean(win->load_btn->isEnabled()));
                ex.set("generic_enabled", PyObj::boolean(win->generic_btn->isEnabled()));
                QFile sf(w[1] + "/state_ofx.json");
                if (sf.open(QIODevice::WriteOnly)) sf.write(QByteArray::fromStdString(dump::state_with(*win, ex)));
                win->grab().save(w[1] + "/win_ofx.png");
            }
        }
        if (c_is_close(w)) return;
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
        QTimer::singleShot(20, win, *after);
    };
    (*tick)();
}

}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("nv_ui_qt");
    QStringList args = QApplication::arguments().mid(1);
    if (args.isEmpty() || args[0] != "--ofx") {
        std::fprintf(stderr, "usage: qtui_ofx_drv --ofx W H [FPS] [MAX_FRAMES] [--depth D]  (script: $NV_OFX_SCRIPT)\n");
        return 2;
    }
    auto steps = std::make_shared<QStringList>(qEnvironmentVariable("NV_OFX_SCRIPT").split(';', Qt::SkipEmptyParts));
    return ofx_main(args.mid(1), [steps](MainWindow* win) { run_steps(win, steps); });
}
