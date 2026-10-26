// nv_flat_test - verification / timing driver for nv_flat (native/verify_flat.py).
//
//   nv_flat_test flat IN OUT [-j N]      run every case of IN, write the results and diagnostics to OUT
//   nv_flat_test time IN [-j N] [REPS]   time find_flat_area_genuine (curve included) on the cases of IN
//
// IN:  u64 ncases, then per case: i32 W, H, mode, flags (bit 0: an explicit noise table follows,
//      bit 1: keep the band-measure structs of the scored candidates); [bit 0: i32 n, n f64 xs,
//      n f64 ys]; W*H*3 f32 (R, G, B interleaved, row-major).
// OUT: per case: i32 x, y, w, h, status, s, rw, rh, i0, curve_run, curve_ok; 8 f64 curve xs, 8 f64 ys;
//      i64 rn, then rn f32 each of R, G, B, luma (the reduced planes; rn = 0 if not reached);
//      i32 nsizes, per size: i32 size, r, step; 64 i64 xs; 64 i64 ys; 64 f32 means; 64 f32 scores;
//        i32 nplaced, per placed candidate: i32 x0, y0, cw, ch; 3 f32 IsClip;
//      i32 ntrace, per scored candidate: i32 size, i, x, y, w, h, ncl, pen; 8 f32 mean, nz, fl0..2, E, R,
//        score; i32 nwords, nwords u32 (3 x 4096 struct words with flag bit 1, else 0).
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "nv_flat.h"

namespace {

struct Case {
    int W, H, mode, flags;
    std::vector<double> txs, tys;
    std::vector<float> rgb;
};

// Streaming reader: one case in memory at a time (the inputs hold several 4K / 8K frames).
struct Input {
    FILE* f = nullptr;
    uint64_t left = 0;
    explicit Input(const char* path) {
        f = std::fopen(path, "rb");
        if (!f) { std::perror(path); std::exit(2); }
        read(&left, sizeof left);
    }
    ~Input() { if (f) std::fclose(f); }
    void read(void* dst, size_t n) {
        if (n && std::fread(dst, 1, n, f) != n) { std::fprintf(stderr, "truncated input\n"); std::exit(2); }
    }
    template <class T> T get() { T v; read(&v, sizeof v); return v; }
    bool next(Case& c) {
        if (!left) return false;
        --left;
        c.W = get<int32_t>();
        c.H = get<int32_t>();
        c.mode = get<int32_t>();
        c.flags = get<int32_t>();
        c.txs.clear();
        c.tys.clear();
        if (c.flags & 1) {
            const int m = get<int32_t>();
            c.txs.resize(m);
            c.tys.resize(m);
            read(c.txs.data(), sizeof(double) * m);
            read(c.tys.data(), sizeof(double) * m);
        }
        c.rgb.assign(static_cast<size_t>(c.W) * c.H * 3, 0.0f);
        read(c.rgb.data(), sizeof(float) * c.rgb.size());
        return true;
    }
};

template <class T> void put(FILE* f, T v) { std::fwrite(&v, sizeof(T), 1, f); }
void putn(FILE* f, const void* p, size_t n) { if (n) std::fwrite(p, 1, n, f); }

int run_flat(const char* in, const char* out, int threads) {
    Input inp(in);
    FILE* f = std::fopen(out, "wb");
    if (!f) { std::perror(out); return 2; }
    Case c;
    while (inp.next(c)) {
        nvf::Diag d;
        d.keep_structs = (c.flags & 2) != 0;
        std::unique_ptr<nvc::CurveInterp> table;
        if (c.flags & 1) table.reset(new nvc::CurveInterp(c.txs.data(), c.tys.data(), static_cast<int>(c.txs.size())));
        const nvf::Result res = nvf::find_flat_area_genuine(c.rgb.data(), c.W, c.H, 3, c.mode, table.get(), threads, &d);
        for (int v : {res.x, res.y, res.w, res.h, res.status, d.s, d.rw, d.rh, d.i0, int(d.curve_run), int(d.curve_ok)})
            put<int32_t>(f, v);
        putn(f, d.curve_xs, sizeof d.curve_xs);
        putn(f, d.curve_ys, sizeof d.curve_ys);
        put<int64_t>(f, static_cast<int64_t>(d.R.size()));
        for (const auto* p : {&d.R, &d.G, &d.B, &d.luma}) putn(f, p->data(), sizeof(float) * p->size());
        put<int32_t>(f, static_cast<int32_t>(d.sizes.size()));
        for (const auto& sd : d.sizes) {
            put<int32_t>(f, sd.size);
            put<int32_t>(f, sd.r);
            put<int32_t>(f, sd.step);
            putn(f, sd.xs, sizeof sd.xs);
            putn(f, sd.ys, sizeof sd.ys);
            putn(f, sd.means, sizeof sd.means);
            putn(f, sd.scores, sizeof sd.scores);
            put<int32_t>(f, static_cast<int32_t>(sd.placed.size()));
            for (const auto& p : sd.placed) {
                for (int v : {p.x0, p.y0, p.cw, p.ch}) put<int32_t>(f, v);
                putn(f, p.clip, sizeof p.clip);
            }
        }
        put<int32_t>(f, static_cast<int32_t>(d.trace.size()));
        for (const auto& t : d.trace) {
            for (int v : {t.size, t.i, t.x, t.y, t.w, t.h, t.ncl, t.pen}) put<int32_t>(f, v);
            for (float v : {t.mean, t.nz, t.fl[0], t.fl[1], t.fl[2], t.E, t.R, t.score}) put<float>(f, v);
            put<int32_t>(f, static_cast<int32_t>(t.structs.size()));
            putn(f, t.structs.data(), sizeof(uint32_t) * t.structs.size());
        }
    }
    std::fclose(f);
    return 0;
}

int run_time(const char* in, int threads, int reps) {
    Input inp(in);
    Case c;
    while (inp.next(c)) {
        double best = 1e30, sum = 0;
        nvf::Result res{};
        for (int k = 0; k < reps; ++k) {
            const auto t0 = std::chrono::steady_clock::now();
            res = nvf::find_flat_area_genuine(c.rgb.data(), c.W, c.H, 3, c.mode, nullptr, threads, nullptr);
            const double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            best = dt < best ? dt : best;
            sum += dt;
        }
        double cbest = 1e30;                                   // the curve alone (EstimateNoiseLevelCurve)
        for (int k = 0; k < reps; ++k) {
            nvc::CurveResult cr;
            const auto t0 = std::chrono::steady_clock::now();
            nvc::genuine_noise_curve(c.rgb.data(), c.W, c.H, 3, &cr, threads);
            const double dt = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            cbest = dt < cbest ? dt : cbest;
        }
        std::printf("%dx%d mode %d -j %d: best %.3f s, mean %.3f s over %d (curve alone best %.3f s) "
                    "(region %d,%d %dx%d status %d)\n", c.W, c.H, c.mode, threads, best, sum / reps, reps, cbest,
                    res.x, res.y, res.w, res.h, res.status);
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: %s flat IN OUT [-j N] | time IN [-j N] [REPS]\n", argv[0]);
        return 2;
    }
    const std::string cmd = argv[1];
    int threads = 1;
    std::vector<const char*> pos;
    for (int i = 2; i < argc; ++i) {
        if (!std::strcmp(argv[i], "-j") && i + 1 < argc)
            threads = std::atoi(argv[++i]);
        else
            pos.push_back(argv[i]);
    }
    if (cmd == "flat" && pos.size() == 2) return run_flat(pos[0], pos[1], threads);
    if (cmd == "time" && !pos.empty()) return run_time(pos[0], threads, pos.size() > 1 ? std::atoi(pos[1]) : 3);
    std::fprintf(stderr, "bad arguments\n");
    return 2;
}
