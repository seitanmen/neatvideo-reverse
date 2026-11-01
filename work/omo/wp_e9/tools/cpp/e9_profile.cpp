// e9_profile - the C++ UI's default profile of a clip, as MainWindow builds it after loading a video with the
// default Preferences (auto_profile on: Auto Profile (Current Frame) on the current frame, then Auto Fine-Tune
// because auto_fine_tune is on), written as the --ui-profile / --params files of qtui_export.
// The code path is the one of qtui/tests/g1_eq.cpp (case "default"), except that every frame of the clip is read
// (as the UI holds them) so that the profile's ui_frame is the clip's frame number.
//
//   e9_profile VIDEO OUTDIR F            (F = the profile frame; "noisy" = Auto Profile (Most Noisy Frame))
//
// Writes OUTDIR/ui_profile.json (ProfileState::effective), ui_params.json (AppState::backend of the default
// parameters), nvprof.json (ProfileState::to_json, the UI's profile file) and profile_info.json.
#include <QCoreApplication>

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

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
    if (argc != 4) {
        std::fprintf(stderr, "usage: e9_profile VIDEO OUTDIR F|noisy\n");
        return 2;
    }
    const std::string video = argv[1], out = argv[2], fsel = argv[3];
    try {
        const VideoInfo vi = probe(video);
        const std::string matrix = matrix_from_tags(vi.tags, vi.H);
        const bool full_range = vi.full_range;
        std::vector<Frame> frames;
        read_frames(video, vi.W, vi.H, 0, 1 << 30, vi.fps, [&](int, Frame&& fr) {
            frames.push_back(std::move(fr));
            return true;
        });
        if (frames.empty()) throw std::runtime_error("no frames read");
        const int f = fsel == "noisy" ? noisiest_frame(frames, CancelFn()) : std::stoi(fsel);
        PyObj prof = build_profile(frames, f, full_range, matrix, "auto", nullptr, CancelFn());
        AppState st;
        ProfileState ps;
        ps.base = prof;
        st.set_profile(ps, "Auto Profile");
        const int af = aft_frame(st.profile.base, frames.size());
        PyObj a = run_aft(st.profile.base, frames[static_cast<size_t>(af)], full_range, matrix, CancelFn());
        st.set_profile(after_aft(st.profile, a), "Auto Fine-Tune");
        const PyObj eff = *st.effective();
        const PyObj P = st.backend();
        write_file(out + "/ui_profile.json", eff.dump(1));
        write_file(out + "/ui_params.json", P.dump(1));
        write_file(out + "/nvprof.json", st.profile.to_json().dump(1));
        PyObj info = PyObj::dict();
        info.set("video", PyObj::str(video));
        info.set("frames_read", PyObj::integer(static_cast<long long>(frames.size())));
        info.set("profile_select", PyObj::str(fsel == "noisy" ? "Most Noisy Frame" : "Current Frame"));
        info.set("profile_frame", PyObj::integer(f));
        info.set("aft_frame", PyObj::integer(af));
        info.set("matrix", PyObj::str(matrix));
        info.set("full_range", PyObj::boolean(full_range));
        info.set("area", prof.get("area"));
        info.set("flat_status", prof.get("flat_status"));
        info.set("mode", prof.get("mode"));
        write_file(out + "/profile_info.json", info.dump(1));
        std::printf("profile frame %d, aft frame %d, area %s, flat_status %s\n", f, af, prof.get("area").dump().c_str(),
                    prof.get("flat_status").dump().c_str());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    return 0;
}
