// nv_curve_test - batch driver for the bit-exact verification of nv_curve against the Python port.
//
//   nv_curve_test curve  in.bin out.bin [-j N]   in: u64 N, then N x {i32 W, i32 H, W*H*3 float32 (HWC RGB)}
//                                                 out: N x {u8 ok, 8 f64 xs, 8 f64 ys, 8 f64 sums, 8 i64 counts}
//   nv_curve_test clip   in.bin out.bin          in: u64 N, then N x {u64 n, n float32}; out: N float32
//   nv_curve_test interp in.bin out.bin          in: u64 N, then N x {i32 n, n f64 xs, n f64 ys, i32 q, q f64}
//                                                 out: per case q f64
//   nv_curve_test tile   in.bin out.bin          in: u64 N, then N x {i32 W, i32 H, W*H f32 luma, W*H f32 weight}
//                                                 out: per case noise, weight, bright (W*H f32 each)
//   nv_curve_test time   in.bin [-j N] [reps]    times genuine_noise_curve on the first image of a curve file
//   nv_curve_test recip-selftest [N]             software FRECPE / FRECPS vs the CPU instructions (aarch64)
#include "nv_curve.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

namespace nvc {
float frecpe_software(float x);
float frecps_software(float a, float b);
float frecpe_hardware(float x);
float frecps_hardware(float a, float b);
}  // namespace nvc

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
    std::fprintf(stderr, "%s\n", msg);
    return 1;
}

int jobs_arg(int argc, char** argv, int from) {
    for (int i = from; i + 1 < argc; ++i)
        if (std::strcmp(argv[i], "-j") == 0) return std::atoi(argv[i + 1]);
    return 1;
}

int run_curve(const char* inp, const char* outp, int jobs) {
    std::FILE* in = std::fopen(inp, "rb");
    std::FILE* out = std::fopen(outp, "wb");
    if (!in || !out) return fail("open");
    uint64_t n = 0;
    if (!rd(in, &n)) return fail("short header");
    std::vector<float> img;
    for (uint64_t c = 0; c < n; ++c) {
        int32_t wh[2];
        if (!rd(in, wh, 2)) return fail("short case header");
        img.resize(static_cast<size_t>(wh[0]) * wh[1] * 3);
        if (!rd(in, img.data(), img.size())) return fail("short image");
        nvc::CurveResult r;
        std::memset(&r, 0, sizeof r);
        const uint8_t ok = nvc::genuine_noise_curve(img.data(), wh[0], wh[1], 3, &r, jobs) ? 1 : 0;
        wr(out, &ok);
        wr(out, r.xs, nvc::NBINS);
        wr(out, r.ys, nvc::NBINS);
        wr(out, r.sums, nvc::NBINS);
        wr(out, r.counts, nvc::NBINS);
    }
    std::fclose(in);
    std::fclose(out);
    return 0;
}

int run_clip(const char* inp, const char* outp) {
    std::FILE* in = std::fopen(inp, "rb");
    std::FILE* out = std::fopen(outp, "wb");
    if (!in || !out) return fail("open");
    uint64_t n = 0;
    if (!rd(in, &n)) return fail("short header");
    std::vector<float> v;
    for (uint64_t c = 0; c < n; ++c) {
        uint64_t m = 0;
        if (!rd(in, &m)) return fail("short case header");
        v.resize(m);
        if (m && !rd(in, v.data(), m)) return fail("short plane");
        const float r = nvc::is_clip_genuine(v.data(), m);
        wr(out, &r);
    }
    std::fclose(in);
    std::fclose(out);
    return 0;
}

int run_interp(const char* inp, const char* outp) {
    std::FILE* in = std::fopen(inp, "rb");
    std::FILE* out = std::fopen(outp, "wb");
    if (!in || !out) return fail("open");
    uint64_t n = 0;
    if (!rd(in, &n)) return fail("short header");
    for (uint64_t c = 0; c < n; ++c) {
        int32_t k = 0, q = 0;
        if (!rd(in, &k)) return fail("short case");
        std::vector<double> xs(k), ys(k);
        if (!rd(in, xs.data(), k) || !rd(in, ys.data(), k) || !rd(in, &q)) return fail("short table");
        std::vector<double> qs(q), res(q);
        if (q && !rd(in, qs.data(), q)) return fail("short queries");
        nvc::CurveInterp f(xs.data(), ys.data(), k);
        for (int i = 0; i < q; ++i) res[i] = f(qs[i]);
        wr(out, res.data(), res.size());
    }
    std::fclose(in);
    std::fclose(out);
    return 0;
}

int run_tile(const char* inp, const char* outp) {
    std::FILE* in = std::fopen(inp, "rb");
    std::FILE* out = std::fopen(outp, "wb");
    if (!in || !out) return fail("open");
    uint64_t n = 0;
    if (!rd(in, &n)) return fail("short header");
    for (uint64_t c = 0; c < n; ++c) {
        int32_t wh[2];
        if (!rd(in, wh, 2)) return fail("short case header");
        const size_t m = static_cast<size_t>(wh[0]) * wh[1];
        std::vector<float> luma(m), weight(m), noise(m), bright(m);
        if (!rd(in, luma.data(), m) || !rd(in, weight.data(), m)) return fail("short tile");
        nvc::pixnoise(luma.data(), wh[0], wh[1], weight.data(), noise.data(), bright.data());
        wr(out, noise.data(), m);
        wr(out, weight.data(), m);
        wr(out, bright.data(), m);
    }
    std::fclose(in);
    std::fclose(out);
    return 0;
}

int run_time(const char* inp, int jobs, int reps) {
    std::FILE* in = std::fopen(inp, "rb");
    if (!in) return fail("open");
    uint64_t n = 0;
    int32_t wh[2];
    if (!rd(in, &n) || !rd(in, wh, 2)) return fail("short header");
    std::vector<float> img(static_cast<size_t>(wh[0]) * wh[1] * 3);
    if (!rd(in, img.data(), img.size())) return fail("short image");
    std::fclose(in);
    nvc::CurveResult r;
    for (int i = 0; i < reps; ++i) {
        const auto t0 = std::chrono::steady_clock::now();
        nvc::genuine_noise_curve(img.data(), wh[0], wh[1], 3, &r, jobs);
        const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::printf("%dx%d -j %d: %.3f s\n", wh[0], wh[1], jobs, s);
    }
    return 0;
}

uint32_t ub(float f) {
    uint32_t u;
    std::memcpy(&u, &f, 4);
    return u;
}
float fb(uint32_t u) {
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

int run_recip_selftest(long count) {
#if !defined(__aarch64__)
    std::printf("recip-selftest: not aarch64, nothing to compare\n");
    (void)count;
    return 0;
#else
    std::mt19937_64 rng(12345);
    const uint32_t specials[] = {0x0, 0x80000000u, 0x7f800000u, 0xff800000u, 0x7fc00000u, 0xffc00000u,
                                 0x7f800001u, 0xff812345u, 0x00000001u, 0x80000001u, 0x001fffffu,
                                 0x00200000u, 0x00400000u, 0x007fffffu, 0x00800000u, 0x7f7fffffu,
                                 0x3f800000u, 0xbf800000u, 0x7e800000u, 0x7f000000u, 0x40000000u};
    long bad = 0, total = 0;
    auto check = [&](uint32_t a, uint32_t b) {
        ++total;
        const float fa = fb(a), fbv = fb(b);
        if (ub(nvc::frecpe_software(fa)) != ub(nvc::frecpe_hardware(fa))) {
            if (bad++ < 5) std::printf("frecpe %08x: soft %08x hw %08x\n", a, ub(nvc::frecpe_software(fa)), ub(nvc::frecpe_hardware(fa)));
        }
        if (ub(nvc::frecps_software(fa, fbv)) != ub(nvc::frecps_hardware(fa, fbv))) {
            if (bad++ < 5)
                std::printf("frecps %08x %08x: soft %08x hw %08x\n", a, b, ub(nvc::frecps_software(fa, fbv)),
                            ub(nvc::frecps_hardware(fa, fbv)));
        }
    };
    for (uint32_t a : specials)
        for (uint32_t b : specials) check(a, b);
    for (long i = 0; i < count; ++i) {
        const uint32_t a = static_cast<uint32_t>(rng());
        // b: the hardware estimate of a (the pattern ratio_eps uses) or random bits
        const uint32_t b = (i & 1) ? ub(nvc::frecpe_hardware(fb(a))) : static_cast<uint32_t>(rng());
        check(a, b);
    }
    std::printf("recip-selftest: %ld operand sets, %ld differ\n", total, bad);
    return bad ? 1 : 0;
#endif
}

}  // namespace

int main(int argc, char** argv) {
    if (argc >= 2) {
        const std::string cmd = argv[1];
        if (cmd == "curve" && argc >= 4) return run_curve(argv[2], argv[3], jobs_arg(argc, argv, 4));
        if (cmd == "clip" && argc == 4) return run_clip(argv[2], argv[3]);
        if (cmd == "interp" && argc == 4) return run_interp(argv[2], argv[3]);
        if (cmd == "tile" && argc == 4) return run_tile(argv[2], argv[3]);
        if (cmd == "time" && argc >= 3) {
            int reps = 3;
            for (int i = 3; i < argc; ++i)
                if (std::strcmp(argv[i], "-j") == 0) ++i;
                else reps = std::atoi(argv[i]);
            return run_time(argv[2], jobs_arg(argc, argv, 3), reps);
        }
        if (cmd == "recip-selftest") return run_recip_selftest(argc >= 3 ? std::atol(argv[2]) : 2000000);
    }
    std::fprintf(stderr, "usage: see the header of nv_curve_main.cpp\n");
    return 2;
}
