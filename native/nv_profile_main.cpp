// nv_profile_test - batch driver for the bit-exact verification of nv_profile against repro/nv_profile.py.
//
//   nv_profile_test [-j N] [--floor PATH] <cases.in> <results.out>
//     Input:  uint64 LE count, then per case {uint32 W, H; int32 mode, x, y, w, h; uint32 flags;
//             float32 rgb[H * W * 3] (R, G, B interleaved)}; flags bit 0 = with_floor, bit 1 = lzma.
//     Output: per case {uint32 status (nvp::Status); uint8 profile[0xc300]}.
//     The floor (with_floor) is read from PATH (default <dir of the binary>/../data/ref_floor.npy).
//     stderr: total time of nvp::build_profile.
//   nv_profile_test [-j N] time <cases.in> [reps]     best-of-reps seconds per case (one line each)
//   nv_profile_test [-j N] floor <out.bin>            nvp::compute_reference_floor -> 51 float32
//   nv_profile_test noise <n> <seed> <out.bin>         nvp::reference_noise -> n float32

#include "nv_profile.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>
#include <vector>

namespace {

struct Case {
    int W, H, mode, x, y, w, h;
    uint32_t flags;
    std::vector<float> rgb;
};

bool read_cases(const char* path, std::vector<Case>* cases) {
    std::FILE* in = std::fopen(path, "rb");
    if (!in) { std::perror("open in"); return false; }
    uint64_t n = 0;
    if (std::fread(&n, 8, 1, in) != 1) { std::fprintf(stderr, "short header\n"); std::fclose(in); return false; }
    for (uint64_t i = 0; i < n; ++i) {
        int32_t hd[8];
        if (std::fread(hd, 4, 8, in) != 8) {
            std::fprintf(stderr, "short case %llu\n", (unsigned long long)i);
            std::fclose(in);
            return false;
        }
        Case c{hd[0], hd[1], hd[2], hd[3], hd[4], hd[5], hd[6], static_cast<uint32_t>(hd[7]), {}};
        c.rgb.resize(size_t(c.W) * c.H * 3);
        if (std::fread(c.rgb.data(), 4, c.rgb.size(), in) != c.rgb.size()) {
            std::fprintf(stderr, "short data %llu\n", (unsigned long long)i);
            std::fclose(in);
            return false;
        }
        cases->push_back(std::move(c));
    }
    std::fclose(in);
    return true;
}

int run_one(const Case& c, const float* floor, uint8_t* prof, int threads) {
    try {
        return nvp::build_profile(c.rgb.data(), c.W, c.H, 3, c.mode, c.x, c.y, c.w, c.h,
                                  (c.flags & 1u) ? floor : nullptr, prof, (c.flags & 2u) != 0, threads);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "exception in %dx%d area (%d,%d,%d,%d): %s\n", c.W, c.H, c.x, c.y, c.w, c.h, e.what());
        return 99;
    }
}

bool write_floats(const char* path, const float* v, size_t n) {
    std::FILE* f = std::fopen(path, "wb");
    if (!f) { std::perror("open out"); return false; }
    const bool ok = std::fwrite(v, 4, n, f) == n;
    return std::fclose(f) == 0 && ok;
}

}  // namespace

int main(int argc, char** argv) {
    int threads = 1;
    std::string floor_path;
    {
        const std::string self = argv[0];
        const size_t s = self.rfind('/');
        floor_path = (s == std::string::npos ? std::string(".") : self.substr(0, s)) + "/../data/ref_floor.npy";
    }
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "-j" && i + 1 < argc) threads = std::atoi(argv[++i]);
        else if (a == "--floor" && i + 1 < argc) floor_path = argv[++i];
        else args.push_back(a);
    }
    if (args.size() == 4 && args[0] == "noise") {
        std::vector<float> v(std::strtoull(args[1].c_str(), nullptr, 0));
        nvp::reference_noise(v.size(), static_cast<uint32_t>(std::strtoul(args[2].c_str(), nullptr, 0)), v.data());
        return write_floats(args[3].c_str(), v.data(), v.size()) ? 0 : 1;
    }
    if (args.size() == 2 && args[0] == "floor") {
        float fl[nvp::DEV_WORDS];
        const auto t0 = std::chrono::steady_clock::now();
        const int st = nvp::compute_reference_floor(fl, threads);
        std::fprintf(stderr, "compute_reference_floor: status %d, %.3f s\n", st,
                     std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
        return st == 0 && write_floats(args[1].c_str(), fl, nvp::DEV_WORDS) ? 0 : 1;
    }
    float floor[nvp::DEV_WORDS];
    const bool have_floor = nvp::load_reference_floor(floor_path.c_str(), floor);
    std::vector<uint8_t> prof(nvp::PROF_BYTES);
    if (args.size() >= 2 && args[0] == "time") {
        std::vector<Case> cases;
        if (!read_cases(args[1].c_str(), &cases)) return 1;
        const int reps = args.size() >= 3 ? std::atoi(args[2].c_str()) : 3;
        for (const auto& c : cases) {
            if ((c.flags & 1u) && !have_floor) { std::fprintf(stderr, "cannot read %s\n", floor_path.c_str()); return 1; }
            double best = 1e30;
            for (int r = 0; r < reps; ++r) {
                const auto t0 = std::chrono::steady_clock::now();
                run_one(c, floor, prof.data(), threads);
                const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                if (s < best) best = s;
            }
            std::printf("%d %d %d %d %d %.6f\n", c.W, c.H, c.mode, c.w, c.h, best);
        }
        return 0;
    }
    if (args.size() != 2) {
        std::fprintf(stderr, "usage: %s [-j N] [--floor PATH] <cases.in> <results.out> | time <cases.in> [reps] | "
                             "floor <out.bin> | noise <n> <seed> <out.bin>\n", argv[0]);
        return 2;
    }
    std::vector<Case> cases;
    if (!read_cases(args[0].c_str(), &cases)) return 1;
    std::FILE* out = std::fopen(args[1].c_str(), "wb");
    if (!out) { std::perror("open out"); return 1; }
    double total = 0;
    for (const auto& c : cases) {
        if ((c.flags & 1u) && !have_floor) {
            std::fprintf(stderr, "cannot read the reference floor %s\n", floor_path.c_str());
            std::fclose(out);
            return 1;
        }
        std::memset(prof.data(), 0, prof.size());
        const auto t0 = std::chrono::steady_clock::now();
        const uint32_t st = static_cast<uint32_t>(run_one(c, floor, prof.data(), threads));
        total += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        if (std::fwrite(&st, 4, 1, out) != 1 || std::fwrite(prof.data(), 1, prof.size(), out) != prof.size()) {
            std::perror("write");
            std::fclose(out);
            return 1;
        }
    }
    std::fclose(out);
    std::fprintf(stderr, "%zu cases, build_profile %.3f s (-j %d)\n", cases.size(), total, threads);
    return 0;
}
