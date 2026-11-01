// qtui_fieldprof - INT-3: the field profile of the C++ UI (fields.h build_profile_fields) on a video's 8-bit yuv420p
// frames, as JSON on stdout (qtui/tools/verify_interlace.py compares it with nvfilter.build_profile_v5 on the fields).
//
//   qtui_fieldprof VIDEO FRAME two|one PARITY [auto|spatial|temporal [Y0,X0,H,W]]   (PARITY 0 top, 1 bottom; the
//   manual area in frame rows, as the Prepare page's selection)
#include <QCoreApplication>

#include <cstdio>
#include <string>

#include "fields.h"
#include "mainwindow.h"
#include "video.h"

using namespace nvqt;

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("nv_ui_qt");
    if (argc < 5) {
        std::fprintf(stderr, "usage: qtui_fieldprof VIDEO FRAME two|one PARITY [mode]\n");
        return 2;
    }
    set_ffmpeg_dir(find_ffmpeg_dir(settings()->value("prefs/ffmpeg").toString().toStdString()));
    try {
        const VideoInfo vi = probe(argv[1]);
        std::vector<Frame> frames;
        read_frames(argv[1], vi.W, vi.H, 0, -1, vi.fps, [&](int, Frame&& f) {
            frames.push_back(std::move(f));
            return true;
        });
        std::vector<int> area;
        if (argc > 6)
            for (const QString& v : QString(argv[6]).split(',')) area.push_back(v.toInt());
        const PyObj p = build_profile_fields(frames, std::stoi(argv[2]), vi.full_range, matrix_from_tags(vi.tags, vi.H),
                                             argc > 5 ? argv[5] : "auto", area.empty() ? nullptr : &area, argv[3],
                                             std::stoi(argv[4]), nullptr);
        std::printf("%s\n", p.dump(-1).c_str());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }
    return 0;
}
