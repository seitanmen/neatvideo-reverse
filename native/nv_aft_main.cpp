// nv_aft_test - batch driver for the bit-exact verification of nv_aft against the Python port.
//
//   nv_aft_test aft in.bin out.bin [-j N]
//       in:  u64 N, then N x {u32 W, H, mode, has_image2; i64 cancel_at; u8 prof[0xc300];
//                             f32 rgb[H*W*3]; [f32 image2[H*W*3]]}
//       out: N x {u32 status; u8 prof[0xc300]; f32 out3[3]; i32 diag[7];
//                 27 x {u32 n; f32 v[n]} (bins, channel-major)}
//   nv_aft_test sim in.bin out.bin
//       in:  u64 N, then N x {f32 t[4096]; f32 p[2005]; f32 stats[51]; f32 s_lvl, s_mult}; out: N x f32
//   nv_aft_test finish in.bin out.bin
//       in:  u64 N, then N x {u32 mode; u8 prof[0xc300]; 27 x {u32 n; f32 v[n]}}
//       out: N x {u8 prof[0xc300]; f32 out3[3]}
//   nv_aft_test shuffle NMAX out.bin            out: for n = 0..NMAX: i32 order[n]
//   nv_aft_test combine in.bin out.bin
//       in:  u64 N, then N x {u8 raw[0xc300]; u8 aft[0xc300]; i32 n; f64 centers[n]; 3 x f32 curve[n];
//                             f64 area_level; f64 tot_ratio[3]; i32 band}
//       out: N x {f64 cw[9]; 3 x f64 face[n]; 3 x f32 curve[n]; f64 sigma[3]; u8 marker[27], clip[27], gt3[27]}
//   nv_aft_test time in.bin [-j N] [reps]        times auto_fine_tune on every case of an aft file
#include "nv_aft.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

template <typename T>
bool rd(std::FILE* f, T* v, size_t n = 1) {
    return std::fread(v, sizeof(T), n, f) == n;
}
template <typename T>
void wr(std::FILE* f, const T* v, size_t n = 1) {
    std::fwrite(v, sizeof(T), n, f);
}

int fail(const char* msg) {
    std::fprintf(stderr, "nv_aft_test: %s\n", msg);
    return 1;
}

struct AftCase {
    uint32_t W, H, mode, has2;
    int64_t cancel_at;
    std::vector<uint8_t> prof;
    std::vector<float> rgb, image2;
};

bool read_bins(std::FILE* f, nva::Bins* bins) {
    for (int c = 0; c < 3; ++c)
        for (int k = 0; k < 9; ++k) {
            uint32_t n;
            if (!rd(f, &n)) return false;
            (*bins)[c][k].resize(n);
            if (n && !rd(f, (*bins)[c][k].data(), n)) return false;
        }
    return true;
}

void write_bins(std::FILE* f, const nva::Bins& bins) {
    for (int c = 0; c < 3; ++c)
        for (int k = 0; k < 9; ++k) {
            const uint32_t n = static_cast<uint32_t>(bins[c][k].size());
            wr(f, &n);
            if (n) wr(f, bins[c][k].data(), n);
        }
}

bool read_aft(const char* path, std::vector<AftCase>* cases) {
    std::FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    uint64_t n = 0;
    bool ok = rd(f, &n);
    for (uint64_t i = 0; ok && i < n; ++i) {
        AftCase c;
        uint32_t h[4];
        ok = rd(f, h, 4) && rd(f, &c.cancel_at);
        if (!ok) break;
        c.W = h[0];
        c.H = h[1];
        c.mode = h[2];
        c.has2 = h[3];
        c.prof.resize(nva::PROF);
        const size_t np = static_cast<size_t>(c.W) * c.H * 3;
        c.rgb.resize(np);
        ok = rd(f, c.prof.data(), nva::PROF) && (np == 0 || rd(f, c.rgb.data(), np));
        if (ok && c.has2) {
            c.image2.resize(np);
            ok = np == 0 || rd(f, c.image2.data(), np);
        }
        cases->push_back(std::move(c));
    }
    std::fclose(f);
    return ok;
}

int threads_arg(int argc, char** argv, int from) {
    for (int i = from; i + 1 < argc; ++i)
        if (std::string(argv[i]) == "-j") return std::atoi(argv[i + 1]);
    return 1;
}

int cmd_aft(int argc, char** argv) {
    std::vector<AftCase> cases;
    if (!read_aft(argv[2], &cases)) return fail("cannot read the aft input");
    const int nt = threads_arg(argc, argv, 4);
    std::FILE* out = std::fopen(argv[3], "wb");
    if (!out) return fail("cannot open the output");
    double total = 0.0;
    for (auto& c : cases) {
        nva::Diag d;
        nva::Bins bins;
        float out3[3] = {0, 0, 0};
        const auto t0 = std::chrono::steady_clock::now();
        const uint32_t st = static_cast<uint32_t>(nva::auto_fine_tune(
            c.rgb.data(), static_cast<int>(c.W), static_cast<int>(c.H), c.prof.data(), static_cast<int>(c.mode),
            c.has2 ? c.image2.data() : nullptr, static_cast<long>(c.cancel_at), nt, out3, &d, &bins));
        total += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        const int32_t dg[7] = {d.ntiles, d.clip_skip, d.simG_skip, d.simR_skip, d.simB_skip, d.diff_skip, d.accepted};
        wr(out, &st);
        wr(out, c.prof.data(), nva::PROF);
        wr(out, out3, 3);
        wr(out, dg, 7);
        write_bins(out, bins);
    }
    std::fclose(out);
    std::fprintf(stderr, "nv_aft_test aft: %zu cases, %.3f s (-j %d)\n", cases.size(), total, nt);
    return 0;
}

int cmd_sim(char** argv) {
    std::FILE* f = std::fopen(argv[2], "rb");
    if (!f) return fail("cannot read the sim input");
    uint64_t n = 0;
    if (!rd(f, &n)) return fail("short sim header");
    std::FILE* out = std::fopen(argv[3], "wb");
    if (!out) return fail("cannot open the output");
    std::vector<float> t(4096), p(2005), s(51);
    for (uint64_t i = 0; i < n; ++i) {
        float sc[2];
        if (!rd(f, t.data(), t.size()) || !rd(f, p.data(), p.size()) || !rd(f, s.data(), s.size()) || !rd(f, sc, 2))
            return fail("short sim case");
        const float r = nva::similarity(t.data(), p.data(), s.data(), sc[0], sc[1]);
        wr(out, &r);
    }
    std::fclose(f);
    std::fclose(out);
    return 0;
}

int cmd_finish(char** argv) {
    std::FILE* f = std::fopen(argv[2], "rb");
    if (!f) return fail("cannot read the finish input");
    uint64_t n = 0;
    if (!rd(f, &n)) return fail("short finish header");
    std::FILE* out = std::fopen(argv[3], "wb");
    if (!out) return fail("cannot open the output");
    std::vector<uint8_t> prof(nva::PROF);
    for (uint64_t i = 0; i < n; ++i) {
        uint32_t mode;
        nva::Bins bins;
        if (!rd(f, &mode) || !rd(f, prof.data(), prof.size()) || !read_bins(f, &bins)) return fail("short finish case");
        float out3[3] = {0, 0, 0};
        nva::finish(prof.data(), bins, static_cast<int>(mode), out3);
        wr(out, prof.data(), prof.size());
        wr(out, out3, 3);
    }
    std::fclose(f);
    std::fclose(out);
    return 0;
}

int cmd_shuffle(char** argv) {
    const int nmax = std::atoi(argv[2]);
    std::FILE* out = std::fopen(argv[3], "wb");
    if (!out) return fail("cannot open the output");
    for (int n = 0; n <= nmax; ++n) {
        const std::vector<int> o = nva::shuffle_order(n);
        for (int v : o) {
            const int32_t w = v;
            wr(out, &w);
        }
    }
    std::fclose(out);
    return 0;
}

int cmd_combine(char** argv) {
    std::FILE* f = std::fopen(argv[2], "rb");
    if (!f) return fail("cannot read the combine input");
    uint64_t n = 0;
    if (!rd(f, &n)) return fail("short combine header");
    std::FILE* out = std::fopen(argv[3], "wb");
    if (!out) return fail("cannot open the output");
    std::vector<uint8_t> raw(nva::PROF), aft(nva::PROF);
    for (uint64_t i = 0; i < n; ++i) {
        int32_t m, band;
        if (!rd(f, raw.data(), raw.size()) || !rd(f, aft.data(), aft.size()) || !rd(f, &m)) return fail("short combine case");
        std::vector<double> centers(m);
        std::vector<float> cv[3];
        double lvl, tot[3];
        bool ok = rd(f, centers.data(), m);
        for (int c = 0; c < 3 && ok; ++c) {
            cv[c].resize(m);
            ok = rd(f, cv[c].data(), m);
        }
        if (!ok || !rd(f, &lvl) || !rd(f, tot, 3) || !rd(f, &band)) return fail("short combine case");
        const float* curves[3] = {cv[0].data(), cv[1].data(), cv[2].data()};
        nva::Combine r;
        nva::combine(raw.data(), aft.data(), centers.data(), m, curves, lvl, tot, band, &r);
        wr(out, &r.cw[0][0], 9);
        for (int c = 0; c < 3; ++c) wr(out, r.face[c].data(), m);
        for (int c = 0; c < 3; ++c) wr(out, r.curve[c].data(), m);
        wr(out, r.sigma, 3);
        wr(out, &r.marker[0][0], 27);
        wr(out, &r.clip[0][0], 27);
        wr(out, &r.gt3[0][0], 27);
    }
    std::fclose(f);
    std::fclose(out);
    return 0;
}

int cmd_time(int argc, char** argv) {
    std::vector<AftCase> cases;
    if (!read_aft(argv[2], &cases)) return fail("cannot read the aft input");
    const int nt = threads_arg(argc, argv, 3);
    int reps = 1;
    for (int i = 3; i < argc; ++i) {
        if (std::string(argv[i]) == "-j") { ++i; continue; }
        reps = std::atoi(argv[i]);
    }
    for (auto& c : cases) {
        double best = 1e30;
        for (int r = 0; r < reps; ++r) {
            std::vector<uint8_t> prof = c.prof;
            float out3[3];
            nva::Diag d;
            const auto t0 = std::chrono::steady_clock::now();
            nva::auto_fine_tune(c.rgb.data(), static_cast<int>(c.W), static_cast<int>(c.H), prof.data(),
                                static_cast<int>(c.mode), c.has2 ? c.image2.data() : nullptr,
                                static_cast<long>(c.cancel_at), nt, out3, &d);
            const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            if (s < best) best = s;
        }
        std::printf("%u %u mode %u: %.4f s (-j %d)\n", c.W, c.H, c.mode, best, nt);
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) return fail("usage: nv_aft_test aft|sim|finish|shuffle|combine|time ...");
    const std::string cmd = argv[1];
    if (cmd == "time") return cmd_time(argc, argv);
    if (argc < 4) return fail("missing the output path");
    if (cmd == "aft") return cmd_aft(argc, argv);
    if (cmd == "sim") return cmd_sim(argv);
    if (cmd == "finish") return cmd_finish(argv);
    if (cmd == "shuffle") return cmd_shuffle(argv);
    if (cmd == "combine") return cmd_combine(argv);
    return fail("unknown command");
}
