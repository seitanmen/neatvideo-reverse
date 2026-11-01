// e9_profile_fields - the C++ UI's (INT-3) profile and processing parameters of an interlaced clip, as MainWindow
// builds them after loading it with the Video Scan Type set and the default Preferences: Auto Profile (Current Frame)
// on the fields (fields.h build_profile_fields, "Profiled with" Two fields / One field), Auto Fine-Tune on the
// profile's field image (profile_image), and MainWindow::proc_params = AppState::backend + P["interlace"].
// The frames are read as open_video reads an interlaced clip (read_filter + interlaced_vf).
//
//   e9_profile_fields VIDEO OUTDIR F FIRST KIND VARIANT...
//     F = the profile frame; FIRST = 0 top / 1 bottom field first; KIND = two | one0 | one1 (the UI's fprof_box)
//     VARIANT = NAME:MODE:RADIUS_IN_FIELDS, e.g. same_frames:same:4 (the parameters of each variant)
//
// Writes OUTDIR/ui_profile.json (ProfileState::effective), nvprof.json (ProfileState::to_json), profile_info.json and
// OUTDIR/params_<NAME>.json for every variant.
#include <QCoreApplication>

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "fields.h"
#include "nvcore.h"
#include "pyobj.h"
#include "state.h"
#include "video.h"

using namespace nvqt;

namespace {

void write_file(const std::string& path, const std::string& s) {
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot write " + path);
    f << s;
}

}  // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc < 7) {
        std::fprintf(stderr, "usage: e9_profile_fields VIDEO OUTDIR F FIRST two|one0|one1 NAME:MODE:RADIUS...\n");
        return 2;
    }
    const std::string video = argv[1], out = argv[2], kindarg = argv[5];
    const int F = std::stoi(argv[3]), first = std::stoi(argv[4]);
    try {
        const VideoInfo vi = probe(video);
        const std::string matrix = matrix_from_tags(vi.tags, vi.H);
        const bool full_range = vi.full_range;
        // open_video: an 8-bit 4:2:0 clip (legacy format, no read filter) + interlaced_vf
        const std::string vf = interlaced_vf("", vi.pix_fmt, "420");
        std::vector<Frame> frames;
        read_frames(video, vi.W, vi.H, 0, -1, vi.fps, [&](int, Frame&& fr) {
            frames.push_back(std::move(fr));
            return true;
        }, vf);
        if (frames.empty()) throw std::runtime_error("no frames read");
        const std::string kind = kindarg == "two" ? "two" : "one";
        const int parity = kindarg == "one1" ? 1 : kindarg == "one0" ? 0 : first;
        PyObj base = build_profile_fields(frames, F, full_range, matrix, "auto", nullptr, kind, parity, CancelFn());
        AppState st;
        ProfileState ps;
        ps.base = base;
        st.set_profile(ps, "Auto Profile");
        const int af = aft_frame(st.profile.base, frames.size());
        const Frame img = profile_image(st.profile.base, frames[static_cast<size_t>(af)]);
        PyObj a = run_aft(st.profile.base, img, full_range, matrix, CancelFn());
        st.set_profile(after_aft(st.profile, a), "Auto Fine-Tune");
        write_file(out + "/ui_profile.json", st.effective()->dump(1));
        write_file(out + "/nvprof.json", st.profile.to_json().dump(1));
        for (int i = 6; i < argc; ++i) {
            const std::string v = argv[i];
            const size_t c1 = v.find(':'), c2 = v.rfind(':');
            if (c1 == std::string::npos || c2 == c1) throw std::runtime_error("bad variant " + v);
            Interlace il;
            il.first = first;
            il.mode = field_mode_of(v.substr(c1 + 1, c2 - c1 - 1));
            il.radius = std::stoi(v.substr(c2 + 1));
            if (il.mode < 0) throw std::runtime_error("bad mode in " + v);
            PyObj P = st.backend();
            P.set("interlace", interlace_json(il));
            write_file(out + "/params_" + v.substr(0, c1) + ".json", P.dump(1));
        }
        PyObj info = PyObj::dict();
        info.set("video", PyObj::str(video));
        info.set("frames_read", PyObj::integer(static_cast<long long>(frames.size())));
        info.set("read_filter", PyObj::str(vf));
        info.set("profile_select", PyObj::str("Current Frame"));
        info.set("profile_frame", PyObj::integer(F));
        info.set("aft_frame", PyObj::integer(af));
        info.set("first", PyObj::integer(first));
        info.set("profiled_with", PyObj::str(kind == "two" ? "Two fields" : "One field"));
        info.set("fields", base.get("fields"));
        info.set("field", base.get("field"));
        info.set("matrix", PyObj::str(matrix));
        info.set("full_range", PyObj::boolean(full_range));
        info.set("area_field_rows", base.get("area"));
        info.set("flat_status", base.get("flat_status"));
        info.set("mode", base.get("mode"));
        write_file(out + "/profile_info.json", info.dump(1));
        std::printf("profile frame %d (%s, field %s), aft frame %d, area %s, mode %s, flat_status %s\n", F,
                    kind.c_str(), base.get("field").dump().c_str(), af, base.get("area").dump().c_str(),
                    base.get("mode").dump().c_str(), base.get("flat_status").dump().c_str());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    return 0;
}
