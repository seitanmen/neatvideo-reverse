// nv_v5_test - driver for the bit-exact checks of nv_spatial / nv_temporal / nv_ndimage
// (native/verify_spatial.py, native/verify_temporal.py).
//
//   nv_v5_test CASES.bin RESULTS.bin [-j N]        run every case, write the result planes
//   nv_v5_test time CASES.bin REPEAT                time the first case (stderr)
//
// CASES.bin (little endian): u32 ncase, then per case
//   u32 kind; profile; params; i32 nint, i32[nint]; i32 ndbl, f64[ndbl]; i32 nplane, planes
//   profile: i32 n; f64 centers[n]; f64 curve[3][n]; i32 levels[3]; f64 band_ratio[3][5]; f64 tot_ratio[3]
//   params : i32 radius, bs, search, similarity; f64 temporal, threshold, spatial, luma, chroma,
//            noise_level, pyramid_scale, mix; f64 band_amounts[5]; f64 band_noise[5]; f64 channel_noise[3]
//   plane  : i32 H, i32 W, f32[H*W]
// RESULTS.bin: per case u32 nplane, planes.
//
// kinds: 1 spatial_v5 (planes Y Cb Cr rv0 rv1 rv2 -> 3); 4 spatial_v5 + Mix (+ src0 src1 src2 -> 3)
//        2 temporal_v5 / 3 process_frame_v5 (ints: ci, nframes, nflows, flow offsets...; planes:
//          nframes*3 then (vy, vx) per given flow -> temporal: out 3, rv 3; frame: out 3; then
//          (vy, vx) for every flow of the final cache in increasing offset order)
//        10 gauss1, 11 uniform (ints: size, mode), 12 map_coords1 (img, cy, cx), 13 median3,
//        14 curve_sigma (uses the profile curve ints[0]), 15 atrous (ints: levels), 16 down2,
//        17 estimate_flow_v5 (ref, cur; ints: bs, search), 18 pyramid_weight (d, s1; dbls: ratios 3, scale),
//        19 np.exp, 20 to_grid (ints: H, W), 21 dense_flow (vy, vx; ints: H, W, bs),
//        22 warp_plane (img, FY, FX), 23 _search block sum (d; ints nby, nbx, blk),
//        24 _refine (cur, ref, vy, vx; ints blk -> vy, vx, cost[9])
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "nv_ndimage.h"
#include "nv_spatial.h"
#include "nv_temporal.h"

using nvn::Plane;

namespace {

struct Reader {
    const unsigned char* p;
    const unsigned char* e;
    template <class T> T get() {
        if (p + sizeof(T) > e) { std::fprintf(stderr, "truncated input\n"); std::exit(2); }
        T v; std::memcpy(&v, p, sizeof(T)); p += sizeof(T); return v;
    }
    Plane plane() {
        int H = get<int32_t>(), W = get<int32_t>();
        Plane pl(H, W);
        size_t n = (size_t)H * W * 4;
        if (p + n > e) { std::fprintf(stderr, "truncated plane\n"); std::exit(2); }
        std::memcpy(pl.d.data(), p, n);
        p += n;
        return pl;
    }
};

struct Case {
    uint32_t kind = 0;
    nvs::V5Profile prof;
    nvs::V5Params P;
    std::vector<int32_t> ints;
    std::vector<double> dbls;
    std::vector<Plane> planes;
    std::vector<Plane> out;
};

Case read_case(Reader& r) {
    Case c;
    c.kind = r.get<uint32_t>();
    auto& pr = c.prof;
    pr.ncurve = r.get<int32_t>();
    if (pr.ncurve < 1 || pr.ncurve > 16) { std::fprintf(stderr, "bad ncurve\n"); std::exit(2); }
    for (int i = 0; i < pr.ncurve; ++i) pr.centers[i] = r.get<double>();
    for (int k = 0; k < 3; ++k) for (int i = 0; i < pr.ncurve; ++i) pr.curve[k][i] = r.get<double>();
    for (int k = 0; k < 3; ++k) pr.levels[k] = r.get<int32_t>();
    for (int k = 0; k < 3; ++k) for (int i = 0; i < 5; ++i) pr.band_ratio[k][i] = r.get<double>();
    for (int k = 0; k < 3; ++k) pr.tot_ratio[k] = r.get<double>();
    auto& P = c.P;
    P.radius = r.get<int32_t>(); P.bs = r.get<int32_t>(); P.search = r.get<int32_t>();
    P.similarity = r.get<int32_t>();
    P.temporal = r.get<double>(); P.threshold = r.get<double>(); P.spatial = r.get<double>();
    P.luma = r.get<double>(); P.chroma = r.get<double>(); P.noise_level = r.get<double>();
    P.pyramid_scale = r.get<double>(); P.mix = r.get<double>();
    for (int i = 0; i < 5; ++i) P.band_amounts[i] = r.get<double>();
    for (int i = 0; i < 5; ++i) P.band_noise[i] = r.get<double>();
    for (int i = 0; i < 3; ++i) P.channel_noise[i] = r.get<double>();
    int ni = r.get<int32_t>();
    for (int i = 0; i < ni; ++i) c.ints.push_back(r.get<int32_t>());
    int nd = r.get<int32_t>();
    for (int i = 0; i < nd; ++i) c.dbls.push_back(r.get<double>());
    int np_ = r.get<int32_t>();
    for (int i = 0; i < np_; ++i) c.planes.push_back(r.plane());
    return c;
}

void run_window(Case& c, bool frame) {
    const int ci = c.ints[0], nf = c.ints[1], nfl = c.ints[2];
    std::vector<nvt::Frame> win(nf);
    size_t k = 0;
    for (int f = 0; f < nf; ++f)
        for (int q = 0; q < 3; ++q) win[f].p[q] = c.planes[k++];
    std::map<int, nvt::Flow> flows;
    for (int i = 0; i < nfl; ++i) {
        nvt::Flow fl;
        fl.vy = c.planes[k++];
        fl.vx = c.planes[k++];
        flows[c.ints[3 + i]] = fl;
    }
    if (frame) {
        c.out = nvt::process_frame_v5(win, ci, c.prof, c.P, &flows);
    } else {
        std::vector<Plane> o, rv;
        nvt::temporal_v5(win, ci, c.prof, c.P, &flows, o, rv);
        c.out = o;
        for (auto& p : rv) c.out.push_back(p);
    }
    for (auto& kv : flows) { c.out.push_back(kv.second.vy); c.out.push_back(kv.second.vx); }
}

void run_case(Case& c) {
    auto& pl = c.planes;
    switch (c.kind) {
        case 1: {
            Plane p[3] = {pl[0], pl[1], pl[2]}, rv[3] = {pl[3], pl[4], pl[5]};
            c.out = nvs::spatial_v5(p, c.prof, rv, c.P);
            break;
        }
        case 4: {   // spatial_v5 then process_frame_v5's Mix (planes Y Cb Cr rv0 rv1 rv2 src0 src1 src2)
            Plane p[3] = {pl[0], pl[1], pl[2]}, rv[3] = {pl[3], pl[4], pl[5]}, src[3] = {pl[6], pl[7], pl[8]};
            c.out = nvs::spatial_v5(p, c.prof, rv, c.P);
            nvs::mix_v5(c.out, src, c.P.mix);
            break;
        }
        case 2: run_window(c, false); break;
        case 3: run_window(c, true); break;
        case 10: c.out.push_back(nvn::gauss1(pl[0])); break;
        case 11: c.out.push_back(nvn::uniform(pl[0], c.ints[0], (nvn::Mode)c.ints[1])); break;
        case 12: c.out.push_back(nvn::map_coords1(pl[0], pl[1], pl[2])); break;
        case 13: c.out.push_back(nvn::median3(pl[0])); break;
        case 14:
            c.out.push_back(nvn::curve_sigma(pl[0], c.prof.centers, c.prof.curve[c.ints[0]], c.prof.ncurve));
            break;
        case 15: c.out = nvn::atrous(pl[0], c.ints[0]); break;
        case 16: c.out.push_back(nvn::down2(pl[0])); break;
        case 17: {
            nvt::Flow f = nvt::estimate_flow_v5(pl[0], pl[1], c.ints[0], c.ints[1]);
            c.out.push_back(f.vy);
            c.out.push_back(f.vx);
            break;
        }
        case 18: c.out.push_back(nvt::pyramid_weight(pl[0], pl[1], c.dbls.data(), c.dbls[3])); break;
        case 19: {
            Plane o = pl[0];
            for (auto& v : o.d) v = nvn::np_expf(v);
            c.out.push_back(o);
            break;
        }
        case 20: c.out.push_back(nvn::to_grid(pl[0], c.ints[0], c.ints[1])); break;
        case 21: {
            nvt::Flow f{pl[0], pl[1]};
            Plane FY, FX;
            nvt::dense_flow(f, c.ints[0], c.ints[1], c.ints[2], FY, FX);
            c.out.push_back(FY);
            c.out.push_back(FX);
            break;
        }
        case 22: c.out.push_back(nvt::warp_plane(pl[0], pl[1], pl[2])); break;
        case 23: {   // _search's block sum (d plane; ints nby, nbx, blk)
            Plane s(c.ints[0], c.ints[1]);
            nvt::detail::block_sum13(pl[0].d.data(), c.ints[0], c.ints[1], c.ints[2], s);
            c.out.push_back(s);
            break;
        }
        case 24: {   // _refine (cur, ref, vy, vx as float planes; ints blk) -> vy, vx, 9 cost planes
            Plane vy = pl[2], vx = pl[3];
            std::vector<Plane> cost;
            nvt::detail::refine_test(pl[0], pl[1], vy, vx, c.ints[0], cost);
            c.out.push_back(vy);
            c.out.push_back(vx);
            for (auto& p : cost) c.out.push_back(p);
            break;
        }
        default: std::fprintf(stderr, "unknown kind %u\n", c.kind); std::exit(2);
    }
}

std::vector<unsigned char> slurp(const char* path) {
    FILE* f = std::fopen(path, "rb");
    if (!f) { std::perror(path); std::exit(2); }
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::vector<unsigned char> b((size_t)n);
    if (n && std::fread(b.data(), 1, (size_t)n, f) != (size_t)n) { std::perror(path); std::exit(2); }
    std::fclose(f);
    return b;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc >= 4 && std::string(argv[1]) == "time") {
        auto buf = slurp(argv[2]);
        Reader r{buf.data(), buf.data() + buf.size()};
        r.get<uint32_t>();
        Case c0 = read_case(r);
        int rep = std::atoi(argv[3]);
        for (int i = 0; i < rep; ++i) {
            Case c = c0;
            auto t0 = std::chrono::steady_clock::now();
            run_case(c);
            auto t1 = std::chrono::steady_clock::now();
            std::fprintf(stderr, "kind %u: %.3f s\n", c.kind, std::chrono::duration<double>(t1 - t0).count());
        }
        return 0;
    }
    if (argc < 3) {
        std::fprintf(stderr, "usage: nv_v5_test CASES.bin RESULTS.bin [-j N] | time CASES.bin REPEAT\n");
        return 2;
    }
    int jobs = 1;
    if (argc >= 5 && std::string(argv[3]) == "-j") jobs = std::max(1, std::atoi(argv[4]));
    auto buf = slurp(argv[1]);
    Reader r{buf.data(), buf.data() + buf.size()};
    uint32_t n = r.get<uint32_t>();
    std::vector<Case> cases;
    for (uint32_t i = 0; i < n; ++i) cases.push_back(read_case(r));
    std::atomic<size_t> next{0};
    auto work = [&]() {
        for (;;) {
            size_t i = next++;
            if (i >= cases.size()) return;
            run_case(cases[i]);
            cases[i].planes.clear();
        }
    };
    std::vector<std::thread> th;
    for (int j = 0; j < jobs; ++j) th.emplace_back(work);
    for (auto& t : th) t.join();
    FILE* f = std::fopen(argv[2], "wb");
    if (!f) { std::perror(argv[2]); return 2; }
    for (auto& c : cases) {
        uint32_t np_ = (uint32_t)c.out.size();
        std::fwrite(&np_, 4, 1, f);
        for (auto& p : c.out) {
            int32_t hw[2] = {p.H, p.W};
            std::fwrite(hw, 4, 2, f);
            std::fwrite(p.d.data(), 4, p.d.size(), f);
        }
    }
    std::fclose(f);
    return 0;
}
