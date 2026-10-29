// qtui_export - the export of the C++ UI from the command line, with the arguments of
// `python3 -m nv_ui_qt.export` (repro/nv_ui_qt/export.py + denoise_video_v5.py):
//
//   qtui_export SRC DST --ui-profile P.json --params Q.json [--jobs J] [--crf C] [--audio copy|aac|none]
//               [--depth 8|10|12|16|32|source] [--codec auto|x264|x265|ffv1|prores|exr] [--start S] [--frames N]
//   qtui_export --selftest-cancel SRC DST --ui-profile P.json --params Q.json   (ExportJob::kill() check)
//
// Every line the Python export prints goes to stdout; the exit code is the Python one (0 ok, 1 error).
// NV_EXPORT_TRACE=<file> appends every spawned ffmpeg / ffprobe command as a JSON list.
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTimer>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "export.h"
#include "pyobj.h"
#include "video.h"
#include "mainwindow.h"

using namespace nvqt;

namespace {

PyObj read_json(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot read " + path);
    std::stringstream ss;
    ss << f.rdbuf();
    return PyObj::parse(ss.str());
}

int usage() {
    std::fprintf(stderr, "usage: qtui_export SRC DST --ui-profile P.json --params Q.json [--jobs J] [--crf C] "
                         "[--audio copy|aac|none] [--depth D] [--codec X] [--start S] [--frames N]\n");
    return 2;
}

}  // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("nv_ui_qt");
    set_ffmpeg_dir(find_ffmpeg_dir(settings()->value("prefs/ffmpeg").toString().toStdString()));
    ExportOptions o;
    std::string prof_path, params_path;
    bool selftest_cancel = false;
    std::vector<std::string> pos;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&](std::string* out) {
            if (i + 1 >= argc) throw std::runtime_error("missing value for " + a);
            *out = argv[++i];
        };
        try {
            std::string v;
            if (a == "--ui-profile") next(&prof_path);
            else if (a == "--params") next(&params_path);
            else if (a == "--jobs") { next(&v); o.jobs = std::stoi(v); }
            else if (a == "--crf") { next(&v); o.crf = std::stoi(v); }
            else if (a == "--audio") next(&o.audio);
            else if (a == "--depth") next(&o.depth);
            else if (a == "--codec") next(&o.codec);
            else if (a == "--start") { next(&v); o.start = std::stoi(v); }
            else if (a == "--frames") { next(&v); o.frames = std::stoi(v); }
            else if (a == "--selftest-cancel") selftest_cancel = true;
            else if (!a.empty() && a[0] == '-') { std::fprintf(stderr, "unknown option %s\n", a.c_str()); return usage(); }
            else pos.push_back(a);
        } catch (const std::exception& e) {
            std::fprintf(stderr, "%s\n", e.what());
            return usage();
        }
    }
    if (pos.size() != 2 || prof_path.empty() || params_path.empty()) return usage();
    o.src = pos[0];
    o.dst = pos[1];
    try {
        o.prof = read_json(prof_path);
        o.params = read_json(params_path);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }
    if (!selftest_cancel) {
        const int code = run_export(o, [](const std::string& line) {
            std::printf("%s\n", line.c_str());
            std::fflush(stdout);
        }, nullptr);
        return code;
    }
    // ExportJob through its signals: kill() 0.8 s after the start; expects finished(false) and no output file left
    // by the job itself (the UI removes it as the Python UI does; here we check that the children are gone and
    // the job reports failure)
    auto* job = new ExportJob(o);
    int result = 3;
    QObject::connect(job, &ExportJob::progress, [](double pct, QString line) {
        std::printf("progress %.1f %s\n", pct, line.toUtf8().constData());
        std::fflush(stdout);
    });
    QObject::connect(job, &ExportJob::finished, [&](bool ok, QString text) {
        std::printf("finished ok=%d running=%d text=%s\n", ok ? 1 : 0, job->running() ? 1 : 0, text.toUtf8().constData());
        result = ok ? 1 : 0;
        app.quit();
    });
    const int ms = qgetenv("NV_CANCEL_MS").isEmpty() ? 800 : qgetenv("NV_CANCEL_MS").toInt();
    QTimer::singleShot(ms, [&] {
        std::printf("kill\n");
        std::fflush(stdout);
        job->kill();
    });
    QTimer::singleShot(60000, [&] { result = 4; app.quit(); });
    job->start();
    app.exec();
    delete job;
    return result;
}
