// qtui_g1eq - G1 (R/G/B equalizer) comparison driver of the C++ UI: the default flow of the UI on one clip (Auto
// Profile on frame F + Auto Fine-Tune, as MainWindow does after loading) and the equalizer edits of CASE through
// AppState, then the processed frame F (backend.process_frame: temporal radius from the parameters, spatial, mix).
// qtui/tools/g1_compare.py runs the same steps on the PySide6 UI (g1_py_eq.py) and compares.
//
//   qtui_g1eq VIDEO OUTDIR F CASE
//
// Writes OUTDIR/CASE.nvprof.json (ProfileState::to_json), CASE.ui_profile.json (the effective profile),
// CASE.ui_params.json (the backend parameters), CASE.planes.f32 (Y, Cb, Cr float32 of the processed frame) and
// CASE.quality.json ([Quality %, Completeness %] or null).
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
    f << s;
}

std::array<double, 9> with(const std::array<double, 9>& fs, int k, double v) {
    std::array<double, 9> o = fs;
    o[static_cast<size_t>(k)] = v;
    return o;
}

// the equalizer edits of a case (factor index c of the profile's channel order; "+20%" = level 1.2)
void apply_case(AppState& st, const std::string& name) {
    auto f = [&](int c) { return st.profile.factors[static_cast<size_t>(c)]; };
    if (name == "default") return;
    if (name == "r4") {                         // R (or the first channel) bin 4 to +20%
        st.new_edit();
        st.set_factors(0, with(f(0), 4, 1.2), 4);
    } else if (name == "gall") {                // G (second channel) every bin to +20%
        for (int k = 0; k < 9; ++k) {
            st.new_edit();
            st.set_factors(1, with(f(1), k, 1.2), k);
        }
    } else if (name == "all2") {                // All: bin 2 of the three channels to +20%
        st.new_edit();
        st.set_factors_all(with(f(0), 2, 1.2), 2);
    } else if (name == "b6tune") {              // B (third channel) bin 6 to -30%, Overall Noise Level +10%
        st.new_edit();
        st.set_factors(2, with(f(2), 6, 0.7), 6);
        st.new_edit();
        st.set_tune("all", 1.1);
    } else if (name == "reset") {               // Reset Fine-Tuning Results
        st.set_profile(reset_fine_tune(st.profile), "Reset Fine-Tuning Results");
    } else if (name == "r4ac") {                // r4 then Auto Complete
        apply_case(st, "r4");
        ProfileState n = st.profile;
        n.factors = auto_complete(n);
        st.set_profile(n, "Auto Complete");
    } else {
        throw std::runtime_error("unknown case " + name);
    }
}

}  // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc != 5) {
        std::fprintf(stderr, "usage: qtui_g1eq VIDEO OUTDIR F CASE\n");
        return 2;
    }
    const std::string video = argv[1], out = argv[2], cname = argv[4];
    const int F = std::stoi(argv[3]);
    try {
        const VideoInfo vi = probe(video);
        const std::string matrix = matrix_from_tags(vi.tags, vi.H);
        const bool full_range = vi.full_range;
        const int lo = F - 2;
        std::vector<Frame> frames;
        read_frames(video, vi.W, vi.H, lo, F + 3, vi.fps, [&](int, Frame&& fr) {
            frames.push_back(std::move(fr));
            return true;
        });
        const int ci = F - lo;
        PyObj prof = build_profile(frames, ci, full_range, matrix, "auto", nullptr, CancelFn());
        AppState st;
        ProfileState ps;
        ps.base = prof;
        st.set_profile(ps, "Auto Profile");
        PyObj a = run_aft(st.profile.base, frames[static_cast<size_t>(aft_frame(st.profile.base, frames.size()))],
                          full_range, matrix, CancelFn());
        st.set_profile(after_aft(st.profile, a), "Auto Fine-Tune");
        apply_case(st, cname);
        const PyObj eff = *st.effective();
        const PyObj P = st.backend();
        write_file(out + "/" + cname + ".nvprof.json", st.profile.to_json().dump(1));
        write_file(out + "/" + cname + ".ui_profile.json", eff.dump(1));
        write_file(out + "/" + cname + ".ui_params.json", P.dump(1));
        const int R = static_cast<int>(P.get("radius").as_int());
        std::vector<Planes> win;
        for (int i = std::max(0, ci - R); i < std::min(static_cast<int>(frames.size()), ci + R + 1); ++i)
            win.push_back(to_planes(frames[static_cast<size_t>(i)]));
        const int wci = ci - std::max(0, ci - R);
        Flows flows;
        Planes tmp, rv;
        temporal(win, wci, eff, P, &flows, &tmp, &rv);
        const Planes res = finish(win[static_cast<size_t>(wci)], tmp, rv, eff, P);
        std::string bytes;
        for (int c = 0; c < 3; ++c) {
            const auto& v = c == 0 ? res.Y : c == 1 ? res.Cb : res.Cr;
            bytes.append(reinterpret_cast<const char*>(v.data()), v.size() * sizeof(float));
        }
        write_file(out + "/" + cname + ".planes.f32", bytes);
        const auto q = profile_numbers(st.profile, frames, full_range, matrix);
        PyObj qj = PyObj::none();
        if (q) {
            qj = PyObj::list();
            qj.append(PyObj::integer(q->first));
            qj.append(PyObj::integer(q->second));
        }
        write_file(out + "/" + cname + ".quality.json", qj.dump());
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    return 0;
}
