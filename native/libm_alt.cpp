// libm_alt.cpp - see libm_alt.h.  Built only by `make -C native libm-alt` (Makefile.libm_alt).
#include "libm_alt.h"

#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <mutex>
#include <set>
#include <string>
#include <unistd.h>
#include <unordered_set>
#include <utility>

// openlibm 0.8.8 and musl 1.2.6, compiled from the unmodified sources with the public names renamed
// (libm_alt/openlibm_rename.h, libm_alt/musl_rename.h) and every other symbol made local (ld -r).
extern "C" {
double nv_ol_pow(double, double);
float nv_ol_tanf(float);
double nv_ol_atan2(double, double);
void nv_ol_sincos(double, double*, double*);
double nv_ol_sin(double);
double nv_ol_cos(double);
float nv_ol_expf(float);

double nv_mu_pow(double, double);
float nv_mu_tanf(float);
double nv_mu_atan2(double, double);
void nv_mu_sincos(double, double*, double*);
double nv_mu_sin(double);
double nv_mu_cos(double);
float nv_mu_expf(float);
}

namespace nvlibm {

namespace {

// The system libm through volatile pointers, as the default build (no constant folding / rewriting).
double (*volatile s_pow)(double, double) = &::pow;
float (*volatile s_tanf)(float) = &::tanf;
double (*volatile s_atan2)(double, double) = &::atan2;
struct __double2 (*volatile s_sincos)(double) = &::__sincos_stret;
double (*volatile s_sin)(double) = &::sin;
double (*volatile s_cos)(double) = &::cos;
float (*volatile s_expf)(float) = &::expf;
// the system functions the naive formulas (alt 3) are built from
double (*volatile s_exp)(double) = &::exp;
double (*volatile s_log)(double) = &::log;
double (*volatile s_atan)(double) = &::atan;

enum : unsigned { F_POW = 1, F_TANF = 2, F_ATAN2 = 4, F_SINCOS = 8, F_EXPF = 16, F_ALL = 31 };

[[noreturn]] void die(const char* msg, const char* v) {
    std::fprintf(stderr, "libm_alt: %s: '%s'\n", msg, v ? v : "");
    std::abort();
}

struct Config {
    int alt = 0;
    unsigned mask = F_ALL;
    std::string logdir;
};

Config read_config() {
    Config c;
    if (const char* v = std::getenv("NV_LIBM_ALT"); v && *v) {
        if (std::strlen(v) != 1 || v[0] < '0' || v[0] > '3') die("NV_LIBM_ALT must be 0, 1, 2 or 3", v);
        c.alt = v[0] - '0';
    }
    if (const char* v = std::getenv("NV_LIBM_ALT_FUNCS"); v && *v) {
        c.mask = 0;
        std::string s(v);
        size_t p = 0;
        while (p <= s.size()) {
            size_t q = s.find(',', p);
            if (q == std::string::npos) q = s.size();
            const std::string f = s.substr(p, q - p);
            if (f == "pow") c.mask |= F_POW;
            else if (f == "tanf") c.mask |= F_TANF;
            else if (f == "atan2") c.mask |= F_ATAN2;
            else if (f == "sincos") c.mask |= F_SINCOS;
            else if (f == "expf") c.mask |= F_EXPF;
            else die("unknown function in NV_LIBM_ALT_FUNCS", f.c_str());
            p = q + 1;
        }
    }
    if (const char* v = std::getenv("NV_LIBM_LOG"); v && *v) c.logdir = v;
    return c;
}

const Config& cfg() {
    static const Config c = read_config();
    return c;
}

int sel(unsigned f) { return (cfg().mask & f) ? cfg().alt : 0; }

uint64_t b64(double x) {
    uint64_t u;
    std::memcpy(&u, &x, 8);
    return u;
}
uint32_t b32(float x) {
    uint32_t u;
    std::memcpy(&u, &x, 4);
    return u;
}

// ---------------------------------------------------------------------------------- argument log
struct PairHash {
    size_t operator()(const std::pair<uint64_t, uint64_t>& p) const {
        return std::hash<uint64_t>()(p.first * 0x9E3779B97F4A7C15ull ^ p.second);
    }
};

struct ArgLog {
    std::mutex m;
    std::unordered_set<std::pair<uint64_t, uint64_t>, PairHash> pow, atan2;
    std::unordered_set<uint64_t> sincos;
    std::unordered_set<uint32_t> tanf, expf;
    std::atomic<uint64_t> calls[5] = {};
    ~ArgLog() { dump(); }

    void dump() {
        const std::string& dir = cfg().logdir;
        if (dir.empty()) return;
        const std::string path = dir + "/libm_args_" + std::to_string(static_cast<long>(getpid())) + ".bin";
        FILE* f = std::fopen(path.c_str(), "wb");
        if (!f) {
            std::fprintf(stderr, "libm_alt: cannot write %s (errno %d)\n", path.c_str(), errno);
            return;
        }
        // "NVLIBMLG", then per function: name[8], calls u64, n u64, n records
        std::fwrite("NVLIBMLG", 1, 8, f);
        auto head = [&](const char* name, int k, uint64_t n) {
            char nm[8] = {};
            std::strncpy(nm, name, 8);
            std::fwrite(nm, 1, 8, f);
            const uint64_t c = calls[k].load();
            std::fwrite(&c, 8, 1, f);
            std::fwrite(&n, 8, 1, f);
        };
        auto pairs = [&](const char* name, int k, const auto& s) {
            head(name, k, s.size());
            for (const auto& p : s) {
                std::fwrite(&p.first, 8, 1, f);
                std::fwrite(&p.second, 8, 1, f);
            }
        };
        auto singles = [&](const char* name, int k, const auto& s, size_t sz) {
            head(name, k, s.size());
            for (const auto& v : s) std::fwrite(&v, sz, 1, f);
        };
        pairs("pow", 0, pow);
        singles("tanf", 1, tanf, 4);
        pairs("atan2", 2, atan2);
        singles("sincos", 3, sincos, 8);
        singles("expf", 4, expf, 4);
        std::fclose(f);
    }
};

ArgLog& arglog() {
    static ArgLog l;
    return l;
}

bool logging() { return !cfg().logdir.empty(); }

// ---------------------------------------------------------------------- alt 3: naive formulas
// arm64 macOS has long double == double (53-bit significand), so "through long double" is not available;
// alt 3 is instead the textbook formulas evaluated in double: pow = exp(y log x), atan2 = atan(y / x) +
// quadrant, sin / cos = one-constant pi/2 reduction + Taylor series, and the float functions in double
// rounded once to float (tanf = sin / cos, expf = exp).  They show how far a careless implementation moves
// the outputs (an upper estimate of the libm sensitivity), not a candidate libm.
const double PI_D = 3.141592653589793;
const double HALF_PI_D = 1.5707963267948966;
const double TWO_OVER_PI_D = 0.6366197723675814;

double naive_pow(double x, double y) {
    if (y == 0.0) return 1.0;
    if (std::isnan(x) || std::isnan(y)) return x + y;
    if (x == 1.0) return 1.0;
    const bool yint = std::isfinite(y) && y == std::floor(y);
    const bool yodd = yint && std::fabs(y) < 9007199254740992.0 && std::fmod(y, 2.0) != 0.0;
    if (x < 0.0 && std::isfinite(x) && !yint) return NAN;
    const double ax = std::fabs(x);
    double r;
    if (ax == 0.0) r = y > 0.0 ? 0.0 : INFINITY;
    else if (std::isinf(ax)) r = y > 0.0 ? INFINITY : 0.0;
    else r = s_exp(y * s_log(ax));
    return (std::signbit(x) && yodd) ? -r : r;
}

double naive_atan2(double y, double x) {
    if (std::isnan(x) || std::isnan(y)) return x + y;
    if (std::isinf(x) || std::isinf(y)) return s_atan2(y, x);   // the special values: no formula to be naive about
    if (x == 0.0 && y == 0.0) return std::signbit(x) ? std::copysign(PI_D, y) : std::copysign(0.0, y);
    if (x == 0.0) return y > 0.0 ? HALF_PI_D : -HALF_PI_D;
    const double a = s_atan(y / x);
    if (x > 0.0) return a;
    return std::signbit(y) ? a - PI_D : a + PI_D;
}

// Taylor series of sin / cos around 0 (|r| <= pi/4 + a little), coefficients 1/n! computed in double.
void naive_sincos_reduced(double r, double* s, double* c) {
    static const struct Coef {
        double s[12], c[12];
        Coef() {
            double f = 1.0;
            for (int n = 1; n <= 24; ++n) {
                f *= static_cast<double>(n);
                if (n % 2 == 1) s[(n - 1) / 2] = ((n / 2) % 2 ? -1.0 : 1.0) / f;
                else c[n / 2 - 1] = ((n / 2) % 2 ? -1.0 : 1.0) / f;
            }
        }
    } k;
    const double r2 = r * r;
    double ps = k.s[11], pc = k.c[11];
    for (int i = 10; i >= 0; --i) ps = ps * r2 + k.s[i];
    for (int i = 10; i >= 0; --i) pc = pc * r2 + k.c[i];
    *s = r * ps;
    *c = 1.0 + r2 * pc;
}

void naive_sincos(double x, double* s, double* c) {
    if (!std::isfinite(x)) {
        *s = *c = x - x;   // NaN
        return;
    }
    const double kq = std::nearbyint(x * TWO_OVER_PI_D);
    const double r = x - kq * HALF_PI_D;
    double sr, cr;
    naive_sincos_reduced(r, &sr, &cr);
    const long q = static_cast<long>(std::fmod(kq, 4.0) + 4.0) % 4;
    switch (q) {
        case 0: *s = sr; *c = cr; break;
        case 1: *s = cr; *c = -sr; break;
        case 2: *s = -sr; *c = -cr; break;
        default: *s = -cr; *c = sr; break;
    }
}

// tan in double from the system sin / cos, rounded once to float
float naive_tanf(float x) {
    return static_cast<float>(s_sin(static_cast<double>(x)) / s_cos(static_cast<double>(x)));
}

float naive_expf(float x) { return static_cast<float>(s_exp(static_cast<double>(x))); }

struct __double2 mk2(double s, double c) {
    struct __double2 r;
    r.__sinval = s;
    r.__cosval = c;
    return r;
}

}  // namespace

// ------------------------------------------------------------------------------- implementations
double pow_impl(int alt, double x, double y) {
    switch (alt) {
        case 1: return nv_ol_pow(x, y);
        case 2: return nv_mu_pow(x, y);
        case 3: return naive_pow(x, y);
        default: return s_pow(x, y);
    }
}

float tanf_impl(int alt, float x) {
    switch (alt) {
        case 1: return nv_ol_tanf(x);
        case 2: return nv_mu_tanf(x);
        case 3: return naive_tanf(x);
        default: return s_tanf(x);
    }
}

double atan2_impl(int alt, double y, double x) {
    switch (alt) {
        case 1: return nv_ol_atan2(y, x);
        case 2: return nv_mu_atan2(y, x);
        case 3: return naive_atan2(y, x);
        default: return s_atan2(y, x);
    }
}

struct __double2 sincos_impl(int alt, double x) {
    double s, c;
    switch (alt) {
        case 1: nv_ol_sincos(x, &s, &c); return mk2(s, c);
        case 2: nv_mu_sincos(x, &s, &c); return mk2(s, c);
        case 3: naive_sincos(x, &s, &c); return mk2(s, c);
        default: return s_sincos(x);
    }
}

double sin_impl(int alt, double x) {
    switch (alt) {
        case 1: return nv_ol_sin(x);
        case 2: return nv_mu_sin(x);
        case 3: { double s, c; naive_sincos(x, &s, &c); return s; }
        default: return s_sin(x);
    }
}

double cos_impl(int alt, double x) {
    switch (alt) {
        case 1: return nv_ol_cos(x);
        case 2: return nv_mu_cos(x);
        case 3: { double s, c; naive_sincos(x, &s, &c); return c; }
        default: return s_cos(x);
    }
}

float expf_impl(int alt, float x) {
    switch (alt) {
        case 1: return nv_ol_expf(x);
        case 2: return nv_mu_expf(x);
        case 3: return naive_expf(x);
        default: return s_expf(x);
    }
}

// ------------------------------------------------------------------------- dispatchers (the ports)
double pow(double x, double y) {
    if (logging()) {
        ArgLog& l = arglog();
        l.calls[0]++;
        std::lock_guard<std::mutex> g(l.m);
        l.pow.insert({b64(x), b64(y)});
    }
    return pow_impl(sel(F_POW), x, y);
}

float tanf(float x) {
    if (logging()) {
        ArgLog& l = arglog();
        l.calls[1]++;
        std::lock_guard<std::mutex> g(l.m);
        l.tanf.insert(b32(x));
    }
    return tanf_impl(sel(F_TANF), x);
}

double atan2(double y, double x) {
    if (logging()) {
        ArgLog& l = arglog();
        l.calls[2]++;
        std::lock_guard<std::mutex> g(l.m);
        l.atan2.insert({b64(y), b64(x)});
    }
    return atan2_impl(sel(F_ATAN2), y, x);
}

struct __double2 sincos_stret(double x) {
    if (logging()) {
        ArgLog& l = arglog();
        l.calls[3]++;
        std::lock_guard<std::mutex> g(l.m);
        l.sincos.insert(b64(x));
    }
    return sincos_impl(sel(F_SINCOS), x);
}

float expf(float x) {
    if (logging()) {
        ArgLog& l = arglog();
        l.calls[4]++;
        std::lock_guard<std::mutex> g(l.m);
        l.expf.insert(b32(x));
    }
    return expf_impl(sel(F_EXPF), x);
}

}  // namespace nvlibm
