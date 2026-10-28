// nv_model_test - batch driver for the bit-exact verification of nv_model against the Python port.
//
//   nv_model_test <cases.in> <results.out>
//     Input:  uint64 LE count N, then N cases of {uint32 W, H, int32 set, uint32 levels; float32 data[W*H]}.
//     Output: per case {uint32 status (nvm::Status); uint32 words[4096]} (words are meaningful for status 0).
//     stderr: total time of nvm::model_full and the number of cases.
//   nv_model_test time <cases.in> [reps]
//     Times every case (best of reps runs) and prints one line per case: W H set seconds.
//   nv_model_test bm <cases.in> <results.out>
//     nvm::bm_quantiles (FUN_00ad1ae0 on its own, the explicit-error paths).
//     Input:  uint64 LE count N, then N cases of {uint32 W, H, int32 cfg; float32 clean[W*H], A[W*H]}.
//     Output: per case {uint32 status; float32 q0, q1} (q meaningful for status 0).

#include "nv_model.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <string>
#include <vector>

namespace {

struct Case {
    int W, H, set, levels;
    std::vector<float> data;
};

bool read_cases(const char* path, std::vector<Case>* cases) {
    std::FILE* in = std::fopen(path, "rb");
    if (!in) { std::perror("open in"); return false; }
    uint64_t n = 0;
    if (std::fread(&n, 8, 1, in) != 1) { std::fprintf(stderr, "short header\n"); std::fclose(in); return false; }
    for (uint64_t i = 0; i < n; ++i) {
        uint32_t h[4];
        if (std::fread(h, 4, 4, in) != 4) { std::fprintf(stderr, "short case %llu\n", (unsigned long long)i); std::fclose(in); return false; }
        Case c;
        c.W = static_cast<int>(h[0]);
        c.H = static_cast<int>(h[1]);
        c.set = static_cast<int>(static_cast<int32_t>(h[2]));
        c.levels = static_cast<int>(h[3]);
        c.data.resize(static_cast<size_t>(c.W) * c.H);
        if (std::fread(c.data.data(), 4, c.data.size(), in) != c.data.size()) {
            std::fprintf(stderr, "short data %llu\n", (unsigned long long)i);
            std::fclose(in);
            return false;
        }
        cases->push_back(std::move(c));
    }
    std::fclose(in);
    return true;
}

int run_one(const Case& c, uint32_t* words) {
    try {
        return nvm::model_full(c.data.data(), c.W, c.H, c.set, words, c.levels);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "exception in %dx%d set %d: %s\n", c.W, c.H, c.set, e.what());
        return 99;
    }
}

}  // namespace

static int run_bm(const char* in_path, const char* out_path) {
    std::FILE* in = std::fopen(in_path, "rb");
    if (!in) { std::perror("open in"); return 1; }
    std::FILE* out = std::fopen(out_path, "wb");
    if (!out) { std::perror("open out"); std::fclose(in); return 1; }
    uint64_t n = 0;
    bool ok = std::fread(&n, 8, 1, in) == 1;
    for (uint64_t i = 0; ok && i < n; ++i) {
        uint32_t h[3];
        if (std::fread(h, 4, 3, in) != 3) { ok = false; break; }
        const size_t np = static_cast<size_t>(h[0]) * h[1];
        std::vector<float> clean(np), A(np);
        if (std::fread(clean.data(), 4, np, in) != np || std::fread(A.data(), 4, np, in) != np) { ok = false; break; }
        float q[2] = {0.0f, 0.0f};
        uint32_t st;
        try {
            st = static_cast<uint32_t>(nvm::bm_quantiles(clean.data(), A.data(), static_cast<int>(h[0]),
                                                          static_cast<int>(h[1]), static_cast<int32_t>(h[2]), q));
        } catch (const std::exception& e) {
            std::fprintf(stderr, "exception in bm %ux%u cfg %d: %s\n", h[0], h[1], static_cast<int32_t>(h[2]), e.what());
            st = 99;
        }
        std::fwrite(&st, 4, 1, out);
        std::fwrite(q, 4, 2, out);
    }
    std::fclose(in);
    std::fclose(out);
    if (!ok) { std::fprintf(stderr, "short bm input\n"); return 1; }
    return 0;
}

int main(int argc, char** argv) {
    if (argc == 4 && std::string(argv[1]) == "bm") return run_bm(argv[2], argv[3]);
    if (argc >= 3 && std::string(argv[1]) == "time") {
        std::vector<Case> cases;
        if (!read_cases(argv[2], &cases)) return 1;
        const int reps = argc >= 4 ? std::atoi(argv[3]) : 3;
        std::vector<uint32_t> words(nvm::STRUCT_WORDS);
        for (const auto& c : cases) {
            double best = 1e30;
            for (int r = 0; r < reps; ++r) {
                const auto t0 = std::chrono::steady_clock::now();
                run_one(c, words.data());
                const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                if (s < best) best = s;
            }
            std::printf("%d %d %d %.6f\n", c.W, c.H, c.set, best);
        }
        return 0;
    }
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s <cases.in> <results.out> | %s time <cases.in> [reps]\n", argv[0], argv[0]);
        return 2;
    }
    std::vector<Case> cases;
    if (!read_cases(argv[1], &cases)) return 1;
    std::FILE* out = std::fopen(argv[2], "wb");
    if (!out) { std::perror("open out"); return 1; }
    std::vector<uint32_t> words(nvm::STRUCT_WORDS);
    double secs = 0;
    for (const auto& c : cases) {
        std::memset(words.data(), 0, words.size() * 4);
        const auto t0 = std::chrono::steady_clock::now();
        const uint32_t st = static_cast<uint32_t>(run_one(c, words.data()));
        secs += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::fwrite(&st, 4, 1, out);
        std::fwrite(words.data(), 4, words.size(), out);
    }
    std::fclose(out);
    std::fprintf(stderr, "nv_model_test: %zu cases, model_full %.3f s\n", cases.size(), secs);
    return 0;
}
