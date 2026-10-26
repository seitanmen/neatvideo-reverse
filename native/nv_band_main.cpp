// nv_band_test - batch driver for the bit-exact verification of nv_band against the Python port.
//
// Input file:  uint64 LE count N, then N cases of
//                {uint32 kind, W, H, p0, p1, p2; float32 data[W * H]}
//              kind 0 = model_levels(levels = p0), 1 = band_levels(levels = p0),
//              2 = quantile(data[W*H], q = bits p0, flag = p1, div = p2),
//              3 = mean_exact / std_exact of data[W*H].
// Output file: per case, kind 0/1: 4096 uint32 words; kind 2: {uint32 ok, uint32 bits};
//              kind 3: {uint32 mean bits, uint32 std bits}.
// stderr: the time spent in the ported routines per kind.

#include "nv_band.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s <cases.in> <results.out>\n", argv[0]);
        return 2;
    }
    std::FILE* in = std::fopen(argv[1], "rb");
    if (!in) { std::perror("open in"); return 1; }
    std::FILE* out = std::fopen(argv[2], "wb");
    if (!out) { std::perror("open out"); return 1; }
    uint64_t n = 0;
    if (std::fread(&n, 8, 1, in) != 1) { std::fprintf(stderr, "short header\n"); return 1; }
    std::vector<float> data;
    std::vector<uint32_t> words(nvb::STRUCT_WORDS);
    double secs[4] = {0, 0, 0, 0};
    for (uint64_t i = 0; i < n; ++i) {
        uint32_t h[6];
        if (std::fread(h, 4, 6, in) != 6) { std::fprintf(stderr, "short case %llu\n", (unsigned long long)i); return 1; }
        const uint32_t kind = h[0];
        const int W = static_cast<int>(h[1]), H = static_cast<int>(h[2]);
        const size_t cnt = static_cast<size_t>(W) * H;
        data.resize(cnt);
        if (cnt && std::fread(data.data(), 4, cnt, in) != cnt) { std::fprintf(stderr, "short data %llu\n", (unsigned long long)i); return 1; }
        const auto t0 = std::chrono::steady_clock::now();
        if (kind == 0 || kind == 1) {
            if (kind == 0)
                nvb::model_levels(data.data(), W, H, words.data(), static_cast<int>(h[3]));
            else
                nvb::band_levels(data.data(), W, H, words.data(), static_cast<int>(h[3]));
            secs[kind] += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            std::fwrite(words.data(), 4, words.size(), out);
        } else if (kind == 2) {
            float q, r = 0.0f;
            std::memcpy(&q, &h[3], 4);
            const bool ok = nvb::quantile(data.data(), cnt, q, static_cast<int>(h[4]),
                                          static_cast<int32_t>(h[5]), &r);
            secs[2] += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            uint32_t rec[2] = {ok ? 1u : 0u, 0u};
            std::memcpy(&rec[1], &r, 4);
            std::fwrite(rec, 4, 2, out);
        } else if (kind == 3) {
            const float m = nvb::mean_exact(data.data(), cnt);
            const float s = nvb::std_exact(data.data(), cnt, m);
            secs[3] += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            uint32_t rec[2];
            std::memcpy(&rec[0], &m, 4);
            std::memcpy(&rec[1], &s, 4);
            std::fwrite(rec, 4, 2, out);
        } else {
            std::fprintf(stderr, "bad kind %u\n", kind);
            return 1;
        }
    }
    std::fclose(in);
    std::fclose(out);
    std::fprintf(stderr, "nv_band_test: model %.3f s, band %.3f s, quantile %.3f s, mean/std %.3f s\n",
                 secs[0], secs[1], secs[2], secs[3]);
    return 0;
}
