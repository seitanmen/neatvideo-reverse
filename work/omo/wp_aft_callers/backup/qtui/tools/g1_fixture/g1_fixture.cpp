// g1_fixture - writes qtui/tests/fixtures/g1_before_combine.json, the expected values of qtui_selftest
// g1EditMatchesAftCombine, with the code BEFORE G1 (built by make_g1_fixture.sh from the base commit 5dabc1f; only
// functions that existed there are used): Auto Profile on frame F of the video + the C++ AFT (nvcore run_aft), the
// raw profile and AFT bytes of that run (nvc_profile_build + nvc_aft), then for each edit the R / G / B levels written
// as float32 into the AFT bytes' curve words and combined into Y / Cb / Cr by nvc_aft_combine (the C path of
// nv_aft_v5.apply_aft that the C++ UI's AFT used before G1); the Y / Cb / Cr curves and sigma, and the FNV-1a 64
// hash of the frame processed with that profile (nvcore temporal + finish, default parameters).
//
//   g1_fixture VIDEO F OUT.json
#include <QCoreApplication>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>

#include "nvcore.h"
#include "nvcore_c.h"
#include "pyobj.h"
#include "state.h"
#include "video.h"

using namespace nvqt;

namespace {

uint64_t fnv1a(const Planes& p) {
    uint64_t h = 1469598103934665603ULL;
    for (int c = 0; c < 3; ++c) {
        const auto& v = c == 0 ? p.Y : c == 1 ? p.Cb : p.Cr;
        const auto* b = reinterpret_cast<const uint8_t*>(v.data());
        for (size_t i = 0; i < v.size() * sizeof(float); ++i) h = (h ^ b[i]) * 1099511628211ULL;
    }
    return h;
}

}  // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc != 4) {
        std::fprintf(stderr, "usage: g1_fixture VIDEO F OUT.json\n");
        return 2;
    }
    const std::string video = argv[1];
    const int F = std::stoi(argv[2]);
    const VideoInfo vi = probe(video);
    const std::string matrix = matrix_from_tags(vi.tags, vi.H);
    const bool full_range = vi.full_range;
    std::vector<Frame> fr;
    read_frames(video, vi.W, vi.H, 0, F + 3, vi.fps, [&](int, Frame&& f) {
        fr.push_back(std::move(f));
        return true;
    });
    const PyObj fresh = build_profile(fr, F, full_range, matrix, "auto", nullptr, CancelFn());
    const PyObj aftp = run_aft(fresh, fr[static_cast<size_t>(F)], full_range, matrix, CancelFn());
    const Planes pl = to_planes(fr[static_cast<size_t>(F)]);
    const std::vector<float> rgb = to_rgb(pl, matrix, full_range, 1);
    const PyObj& a = fresh.get("area");
    const int ay = static_cast<int>(a[0].as_int()), ax = static_cast<int>(a[1].as_int()),
              ah = static_cast<int>(a[2].as_int()), aw = a.size() == 4 ? static_cast<int>(a[3].as_int()) : ah;
    std::vector<uint8_t> raw(NVC_PROF_BYTES, 0);
    if (nvc_profile_build(rgb.data(), pl.W, pl.H, 1, ax, ay, aw, ah, reference_floor().data(), 1, native_threads(),
                          raw.data()) != 0)
        return 1;
    std::vector<uint8_t> aft = raw;
    float out3[3];
    if (nvc_aft(rgb.data(), pl.W, pl.H, aft.data(), 0, native_threads(), nullptr, out3) != 0) return 1;
    auto word = [](const std::vector<uint8_t>& b, int j, int k) {
        float v;
        std::memcpy(&v, b.data() + 0xbf44 + 0x120 * j + 0x20 * k, 4);
        return v;
    };
    const double up = static_cast<double>(1.2f), down = static_cast<double>(0.7f);
    struct Edit {
        const char* name;
        std::vector<std::tuple<int, int, double>> set;
    };
    std::vector<Edit> edits = {{"R bin 4 +20%", {{0, 4, up}}}, {"G all bins +20%", {}}, {"All bin 2 +20%", {}},
                               {"B bin 6 -30%", {{2, 6, down}}}};
    for (int k = 0; k < 9; ++k) edits[1].set.push_back({1, k, up});
    for (int j = 0; j < 3; ++j) edits[2].set.push_back({j, 2, up});
    const PyObj P = backend_params(default_params());
    const int R = static_cast<int>(P.get("radius").as_int());
    std::vector<Planes> wn;
    for (int i = std::max(0, F - R); i <= std::min(static_cast<int>(fr.size()) - 1, F + R); ++i)
        wn.push_back(to_planes(fr[static_cast<size_t>(i)]));
    const int wci = F - std::max(0, F - R);
    const PyObj& cc = aftp.get("curve_centers");
    const int n = static_cast<int>(cc.size());
    const std::vector<double> centers = cc.as_doubles();
    const char* NM[3] = {"Y", "Cb", "Cr"};
    std::vector<float> pre(3 * static_cast<size_t>(n));
    double tot[3];
    for (int c = 0; c < 3; ++c) {
        const auto v = aftp.get("ui_pre_aft").get("curve").get(NM[c]).as_doubles();
        for (int i = 0; i < n; ++i) pre[static_cast<size_t>(c * n + i)] = static_cast<float>(v[static_cast<size_t>(i)]);
        tot[c] = aftp.get("tot_ratio").get(NM[c]).as_double();
    }
    PyObj out = PyObj::dict();
    out.set("made_by", PyObj::str("qtui/tools/g1_fixture/make_g1_fixture.sh (code before G1: 5dabc1f)"));
    out.set("video", PyObj::str("test.mp4"));
    out.set("frame", PyObj::integer(F));
    PyObj lv = PyObj::list();          // the AFT levels of the run (the test's input fingerprint)
    for (int j = 0; j < 3; ++j) {
        std::vector<double> row;
        for (int k = 0; k < 9; ++k) row.push_back(static_cast<double>(word(aft, j, k)));
        lv.append(PyObj::reals(row));
    }
    out.set("aft_levels", lv);
    out.set("curve_after_aft", aftp.get("curve"));
    PyObj ej = PyObj::list();
    for (const Edit& e : edits) {
        std::vector<uint8_t> mod = aft;
        PyObj set = PyObj::list();
        for (const auto& [j, k, v] : e.set) {
            const float fv = static_cast<float>(v);
            std::memcpy(mod.data() + 0xbf44 + 0x120 * j + 0x20 * k, &fv, 4);
            PyObj t = PyObj::list();
            t.append(PyObj::integer(j));
            t.append(PyObj::integer(k));
            t.append(PyObj::real(v));
            set.append(t);
        }
        std::vector<double> cw(9), face(3 * static_cast<size_t>(n)), sig(3);
        std::vector<float> cv(3 * static_cast<size_t>(n));
        std::vector<uint8_t> flags(81);
        nvc_aft_combine(raw.data(), mod.data(), centers.data(), n, pre.data(), aftp.get("area_level").as_double(), tot,
                        0, cw.data(), face.data(), cv.data(), sig.data(), flags.data());
        PyObj prof = aftp, curve = PyObj::dict(), sigma = PyObj::dict();
        for (int c = 0; c < 3; ++c) {
            curve.set(NM[c], PyObj::reals(std::vector<double>(cv.begin() + c * n, cv.begin() + (c + 1) * n)));
            sigma.set(NM[c], PyObj::real(sig[static_cast<size_t>(c)]));
        }
        prof.set("curve", curve);
        prof.set("sigma", sigma);
        Flows flows;
        Planes tmp, rv;
        temporal(wn, wci, prof, P, &flows, &tmp, &rv);
        const Planes res = finish(wn[static_cast<size_t>(wci)], tmp, rv, prof, P);
        char hx[32];
        std::snprintf(hx, sizeof(hx), "%016llx", static_cast<unsigned long long>(fnv1a(res)));
        PyObj o = PyObj::dict();
        o.set("name", PyObj::str(e.name));
        o.set("set", set);
        o.set("curve", curve);
        o.set("sigma", sigma);
        o.set("frame_fnv1a64", PyObj::str(hx));
        ej.append(o);
    }
    out.set("edits", ej);
    std::ofstream(argv[3], std::ios::binary) << out.dump(1) << "\n";
    return 0;
}
