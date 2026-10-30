// MeatVideoQt - the C++ Qt UI of the reproduction (N3a..N3c).  Usage: MeatVideoQt [video]
//                                                               MeatVideoQt --ofx W H [FPS] [MAX_FRAMES] [--depth 8|16|32]
// ffmpeg / ffprobe are found as the N2b launcher finds them (packaging/main.py): $NV_FFMPEG, the setting
// prefs/ffmpeg, /opt/homebrew/bin, /usr/local/bin, /opt/local/bin, PATH.
#include <QApplication>
#include <QMessageBox>

#include "look.h"
#include "mainwindow.h"
#include "ofx.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("nv_ui_qt");
    nvqt::apply_look(app);                 // Fusion + the genuine grey palette (look.h), also on macOS
    QStringList args = QApplication::arguments().mid(1);
    const std::string dir =
        nvqt::find_ffmpeg_dir(nvqt::settings()->value("prefs/ffmpeg").toString().toStdString());
    nvqt::set_ffmpeg_dir(dir);
    // OFX host mode (N3c): frames on stdin, the current frame on stdout (ofx.h); no ffmpeg needed
    if (!args.isEmpty() && args[0] == "--ofx") return nvqt::ofx_main(args.mid(1));
    if (dir.empty()) {
        QMessageBox::critical(
            nullptr, "ffmpeg not found",
            "ffmpeg and ffprobe were not found.\n\nMeatVideo reads and writes video with them; they are not bundled. "
            "Install them (e.g. `brew install ffmpeg`), or name the ffmpeg binary in the environment variable NV_FFMPEG "
            "or as ffmpeg=<path> under [prefs] in ~/.config/NeatRepro/nv_ui_qt.ini, then start the app again.\n\n"
            "Looked in: /opt/homebrew/bin, /usr/local/bin, /opt/local/bin, PATH");
        args.clear();                      // opening a video would fail in ffprobe
    }
    nvqt::MainWindow win(args.isEmpty() ? QString() : args[0]);
    win.show();
    return QApplication::exec();
}
