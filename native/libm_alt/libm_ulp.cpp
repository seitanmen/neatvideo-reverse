// libm_ulp - the function-level comparison of the W2a harness: every alternative libm (libm_alt.h:
// 1 openlibm, 2 musl, 3 naive) against the system libm (0), in ULP.
//
//   libm_ulp args LOG...          the distinct arguments recorded by NV_LIBM_LOG (libm_args_<pid>.bin),
//                                 merged; one JSON object per (function, alt) on stdout
//   libm_ulp sweep F LO HI [-j N] every float in [LO, HI] (F = tanf | expf); one JSON object per alt
//
// Distance: the two results as ordered integers (IEEE order, -0 == +0), |a - b|.  "bitdiff" counts the
// results whose bits differ (it includes -0 / +0 and NaN payloads, which have distance 0); "nan" counts the
// pairs where exactly one side is NaN.
#include "../libm_alt.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

uint64_t b64(double x) { uint64_t u; std::memcpy(&u, &x, 8); return u; }
uint32_t b32(float x) { uint32_t u; std::memcpy(&u, &x, 4); return u; }
double d64(uint64_t u) { double x; std::memcpy(&x, &u, 8); return x; }
float f32(uint32_t u) { float x; std::memcpy(&x, &u, 4); return x; }

__int128 key64(double x) {
    const int64_t k = static_cast<int64_t>(b64(x));
    return k < 0 ? static_cast<__int128>(INT64_MIN) - k : static_cast<__int128>(k);
}
int64_t key32(float x) {
    const int32_t k = static_cast<int32_t>(b32(x));
    return k < 0 ? static_cast<int64_t>(INT32_MIN) - k : static_cast<int64_t>(k);
}

// distance buckets: 0, 1, 2, 3-4, 5-16, 17-256, 257-65536, >65536
const int NB = 8;
const char* BUCKET[NB] = {"0", "1", "2", "3-4", "5-16", "17-256", "257-65536", ">65536"};
int bucket(unsigned __int128 d) {
    if (d == 0) return 0;
    if (d == 1) return 1;
    if (d == 2) return 2;
    if (d <= 4) return 3;
    if (d <= 16) return 4;
    if (d <= 256) return 5;
    if (d <= 65536) return 6;
    return 7;
}

struct Stat {
    uint64_t n = 0, bitdiff = 0, nan = 0;
    uint64_t hist[NB] = {};
    unsigned __int128 maxd = 0;
    std::string arg_at_max, sys_at_max, alt_at_max;
    double max_rel = 0.0;   // |alt - sys| / |sys| (finite, sys != 0)

    void merge(const Stat& o) {
        n += o.n;
        bitdiff += o.bitdiff;
        nan += o.nan;
        for (int i = 0; i < NB; ++i) hist[i] += o.hist[i];
        if (o.maxd > maxd) {
            maxd = o.maxd;
            arg_at_max = o.arg_at_max;
            sys_at_max = o.sys_at_max;
            alt_at_max = o.alt_at_max;
        }
        max_rel = std::max(max_rel, o.max_rel);
    }
};

std::string hexd(double x) {
    char b[64];
    std::snprintf(b, sizeof b, "%a", x);
    return b;
}

std::string argstr(const char* f, double a0, double a1, bool two) {
    return std::string(f) + "(" + hexd(a0) + (two ? ", " + hexd(a1) : std::string()) + ")";
}

void add64(Stat& s, double sys, double alt, const char* f, double a0, double a1 = 0.0, bool two = false) {
    s.n++;
    if (b64(sys) != b64(alt)) s.bitdiff++;
    if (std::isnan(sys) != std::isnan(alt)) {
        s.nan++;
        return;
    }
    if (std::isnan(sys)) { s.hist[0]++; return; }
    const __int128 a = key64(sys), b = key64(alt);
    const unsigned __int128 d = static_cast<unsigned __int128>(a > b ? a - b : b - a);
    s.hist[bucket(d)]++;
    if (std::isfinite(sys) && std::isfinite(alt) && sys != 0.0)
        s.max_rel = std::max(s.max_rel, std::fabs((alt - sys) / sys));
    if (d > s.maxd) {
        s.maxd = d;
        s.arg_at_max = argstr(f, a0, a1, two);
        s.sys_at_max = hexd(sys);
        s.alt_at_max = hexd(alt);
    }
}

void add32(Stat& s, float sys, float alt, const char* f, double a0) {
    s.n++;
    if (b32(sys) != b32(alt)) s.bitdiff++;
    if (std::isnan(sys) != std::isnan(alt)) {
        s.nan++;
        return;
    }
    if (std::isnan(sys)) { s.hist[0]++; return; }
    const int64_t a = key32(sys), b = key32(alt);
    const unsigned __int128 d = static_cast<unsigned __int128>(a > b ? a - b : b - a);
    s.hist[bucket(d)]++;
    if (std::isfinite(sys) && std::isfinite(alt) && sys != 0.0f)
        s.max_rel = std::max(s.max_rel, std::fabs((static_cast<double>(alt) - sys) / sys));
    if (d > s.maxd) {
        s.maxd = d;
        s.arg_at_max = argstr(f, a0, 0.0, false);
        s.sys_at_max = hexd(sys);
        s.alt_at_max = hexd(alt);
    }
}

void print(const char* func, int alt, const Stat& s, const std::string& extra) {
    std::printf("{\"func\": \"%s\", \"alt\": %d, \"n\": %llu, \"bitdiff\": %llu, \"nan\": %llu, \"max_ulp\": %llu, "
                "\"max_rel\": %.6g, \"hist\": {",
                func, alt, (unsigned long long)s.n, (unsigned long long)s.bitdiff, (unsigned long long)s.nan,
                (unsigned long long)s.maxd, s.max_rel);
    for (int i = 0; i < NB; ++i) std::printf("%s\"%s\": %llu", i ? ", " : "", BUCKET[i], (unsigned long long)s.hist[i]);
    std::printf("}, \"arg_at_max\": \"%s\", \"sys_at_max\": \"%s\", \"alt_at_max\": \"%s\"%s}\n", s.arg_at_max.c_str(),
                s.sys_at_max.c_str(), s.alt_at_max.c_str(), extra.c_str());
}

// distinct arguments: appended from every log, then sorted and deduplicated (the logs of a frame run hold
// tens of millions of expf arguments; a node-based set would not fit)
struct Args {
    std::vector<std::pair<uint64_t, uint64_t>> pow, atan2;
    std::vector<uint64_t> sincos;
    std::vector<uint32_t> tanf, expf;
    uint64_t calls[5] = {};
    void finish() {
        auto su = [](auto& v) {
            std::sort(v.begin(), v.end());
            v.erase(std::unique(v.begin(), v.end()), v.end());
        };
        su(pow), su(atan2), su(sincos), su(tanf), su(expf);
    }
};

bool read_log(const char* path, Args& a) {
    FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    char magic[8];
    bool ok = std::fread(magic, 1, 8, f) == 8 && std::memcmp(magic, "NVLIBMLG", 8) == 0;
    for (int k = 0; ok && k < 5; ++k) {
        char nm[9] = {};
        uint64_t calls = 0, n = 0;
        ok = std::fread(nm, 1, 8, f) == 8 && std::fread(&calls, 8, 1, f) == 1 && std::fread(&n, 8, 1, f) == 1;
        if (!ok) break;
        a.calls[k] += calls;
        const std::string name(nm);
        for (uint64_t i = 0; ok && i < n; ++i) {
            if (name == "pow" || name == "atan2") {
                uint64_t p[2];
                ok = std::fread(p, 8, 2, f) == 2;
                (name == "pow" ? a.pow : a.atan2).push_back({p[0], p[1]});
            } else if (name == "sincos") {
                uint64_t p;
                ok = std::fread(&p, 8, 1, f) == 1;
                a.sincos.push_back(p);
            } else {
                uint32_t p;
                ok = std::fread(&p, 4, 1, f) == 1;
                (name == "tanf" ? a.tanf : a.expf).push_back(p);
            }
        }
    }
    std::fclose(f);
    return ok;
}

std::string range_d(double lo, double hi) {
    return ", \"arg_min\": \"" + hexd(lo) + "\", \"arg_max\": \"" + hexd(hi) + "\"";
}

int cmd_args(int argc, char** argv) {
    Args a;
    for (int i = 2; i < argc; ++i)
        if (!read_log(argv[i], a)) {
            std::fprintf(stderr, "libm_ulp: bad log %s\n", argv[i]);
            return 1;
        }
    a.finish();
    auto calls = [&](int k) { return ", \"calls\": " + std::to_string(a.calls[k]); };
    for (int alt = 1; alt <= 3; ++alt) {
        Stat s;
        double lo = INFINITY, hi = -INFINITY, ylo = INFINITY, yhi = -INFINITY;
        for (const auto& p : a.pow) {
            const double x = d64(p.first), y = d64(p.second);
            lo = std::min(lo, x); hi = std::max(hi, x); ylo = std::min(ylo, y); yhi = std::max(yhi, y);
            add64(s, nvlibm::pow_impl(0, x, y), nvlibm::pow_impl(alt, x, y), "pow", x, y, true);
        }
        print("pow", alt, s, calls(0) + range_d(lo, hi) + ", \"y_min\": \"" + hexd(ylo) + "\", \"y_max\": \"" + hexd(yhi) + "\"");
    }
    for (int alt = 1; alt <= 3; ++alt) {
        Stat s;
        double lo = INFINITY, hi = -INFINITY;
        for (uint32_t u : a.tanf) {
            const float x = f32(u);
            lo = std::min(lo, static_cast<double>(x)); hi = std::max(hi, static_cast<double>(x));
            add32(s, nvlibm::tanf_impl(0, x), nvlibm::tanf_impl(alt, x), "tanf", x);
        }
        // what the ports do with tanf: the line-kernel offsets frintn(f32(t * i) + 0.26f) for i < 512
        // (nv_linekernel.cpp; kernel sides are at most 512): how many (argument, i) pairs change
        uint64_t koff = 0, kargs = 0;
        for (uint32_t u : a.tanf) {
            const float x = f32(u);
            const float t0 = nvlibm::tanf_impl(0, x), t1 = nvlibm::tanf_impl(alt, x);
            bool any = false;
            for (int i = 0; i < 512; ++i) {
                volatile float v0 = t0 * static_cast<float>(i), v1 = t1 * static_cast<float>(i);
                volatile float w0 = v0 + 0.26f, w1 = v1 + 0.26f;
                if (std::rint(w0) != std::rint(w1)) { koff++; any = true; }
            }
            kargs += any;
        }
        print("tanf", alt, s, calls(1) + range_d(lo, hi) + ", \"kernel_offsets_changed\": " + std::to_string(koff) +
                                  ", \"kernel_args_changed\": " + std::to_string(kargs));
    }
    for (int alt = 1; alt <= 3; ++alt) {
        Stat s;
        for (const auto& p : a.atan2) {
            const double y = d64(p.first), x = d64(p.second);
            add64(s, nvlibm::atan2_impl(0, y, x), nvlibm::atan2_impl(alt, y, x), "atan2", y, x, true);
        }
        print("atan2", alt, s, calls(2));
    }
    // sincos: sin and cos parts separately, and whether each library's sincos equals its sin / cos
    for (int alt = 0; alt <= 3; ++alt) {
        Stat ss, sc;
        uint64_t self_mismatch = 0;
        double lo = INFINITY, hi = -INFINITY;
        for (uint64_t u : a.sincos) {
            const double x = d64(u);
            lo = std::min(lo, x); hi = std::max(hi, x);
            const struct __double2 r0 = nvlibm::sincos_impl(0, x), r = nvlibm::sincos_impl(alt, x);
            if (b64(r.__sinval) != b64(nvlibm::sin_impl(alt, x)) || b64(r.__cosval) != b64(nvlibm::cos_impl(alt, x)))
                self_mismatch++;
            add64(ss, r0.__sinval, r.__sinval, "sin", x);
            add64(sc, r0.__cosval, r.__cosval, "cos", x);
        }
        const std::string ex = calls(3) + range_d(lo, hi) + ", \"sincos_ne_sin_cos\": " + std::to_string(self_mismatch);
        if (alt == 0) {
            std::printf("{\"func\": \"sincos_self\", \"alt\": 0, \"n\": %zu, \"sincos_ne_sin_cos\": %llu}\n", a.sincos.size(),
                        (unsigned long long)self_mismatch);
            continue;
        }
        print("sincos.sin", alt, ss, ex);
        print("sincos.cos", alt, sc, ex);
    }
    for (int alt = 1; alt <= 3; ++alt) {
        Stat s;
        double lo = INFINITY, hi = -INFINITY;
        for (uint32_t u : a.expf) {
            const float x = f32(u);
            lo = std::min(lo, static_cast<double>(x)); hi = std::max(hi, static_cast<double>(x));
            add32(s, nvlibm::expf_impl(0, x), nvlibm::expf_impl(alt, x), "expf", x);
        }
        print("expf", alt, s, calls(4) + range_d(lo, hi));
    }
    return 0;
}

// every float from lo to hi (in IEEE order; -0 is not visited, +0 is), split over threads
int cmd_sweep(int argc, char** argv) {
    if (argc < 5) return 2;
    const std::string func = argv[2];
    const float lo = std::strtof(argv[3], nullptr), hi = std::strtof(argv[4], nullptr);
    int nt = 2;
    for (int i = 5; i + 1 < argc; ++i)
        if (std::string(argv[i]) == "-j") nt = std::max(1, std::atoi(argv[i + 1]));
    if (func != "tanf" && func != "expf") return 2;
    const int64_t k0 = key32(lo), k1 = key32(hi);
    auto from_key = [](int64_t k) {
        const uint32_t u = k < 0 ? static_cast<uint32_t>(static_cast<int32_t>(INT32_MIN - k)) : static_cast<uint32_t>(k);
        return f32(u);
    };
    for (int alt = 1; alt <= 3; ++alt) {
        std::vector<Stat> part(nt);
        std::vector<std::thread> th;
        const int64_t total = k1 - k0 + 1;
        for (int t = 0; t < nt; ++t)
            th.emplace_back([&, t] {
                const int64_t a = k0 + total * t / nt, b = k0 + total * (t + 1) / nt;
                for (int64_t k = a; k < b; ++k) {
                    const float x = from_key(k);
                    if (func == "tanf") add32(part[t], nvlibm::tanf_impl(0, x), nvlibm::tanf_impl(alt, x), "tanf", x);
                    else add32(part[t], nvlibm::expf_impl(0, x), nvlibm::expf_impl(alt, x), "expf", x);
                }
            });
        for (auto& x : th) x.join();
        Stat s;
        for (const auto& p : part) s.merge(p);
        print((func + ".sweep").c_str(), alt, s, range_d(lo, hi));
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc >= 2 && std::string(argv[1]) == "args") return cmd_args(argc, argv);
    if (argc >= 2 && std::string(argv[1]) == "sweep") return cmd_sweep(argc, argv);
    std::fprintf(stderr, "usage: libm_ulp args LOG... | libm_ulp sweep tanf|expf LO HI [-j N]\n");
    return 2;
}
