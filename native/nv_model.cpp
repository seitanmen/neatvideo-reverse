// nv_model - C++ port of repro/nv_model.py (model_full), repro/nv_model_special.py (ad319c) and
// repro/nv_model_bm.py (ad1ae0); see nv_model.h.  Every routine names the Python function it
// reproduces; operation order and operand order (which decides the NaN payloads) follow the Python
// code line by line.  The Python fadd / fsub / fmul / fdiv / fma32 of these modules always give the
// AArch64 FPProcessNaNs result of (a, b) (resp. (c, a, b) for fma32), i.e. what a single FADD / FSUB /
// FMUL / FDIV / FMADD instruction with that operand order returns; A / S / M / D / FMA below are
// exactly those instructions (nv_portable: inline asm on AArch64, so the compiler can neither commute
// nor fold them, or their results written out in the portable build).

#include "nv_model.h"
#include "nv_libm.h"
#include "nv_linekernel.h"
#include "nv_portable.h"

#include "nv_band.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

#include <math.h>   // atan2, __sincos_stret (libSystem)


namespace nvm {

namespace {

// ------------------------------------------------------------------------------ float helpers
using nvp::bits;
using nvp::ubits;
using nvp::opaque;   // keeps x * 1.0 / fma(x, 1.0, 0.0) as real operations
using nvp::quiet;
inline bool isnanf_(float x) { return x != x; }

// One FADD / FSUB / FMUL / FDIV / FMADD (a*b + c) / FSQRT / FRECPE / FRECPS / FMIN / FMAX with this
// operand order, or its result written out (nv_portable).
inline float A(float a, float b) { return nvp::add(a, b); }
inline float S(float a, float b) { return nvp::sub(a, b); }
inline float M(float a, float b) { return nvp::mul(a, b); }
inline float D(float a, float b) { return nvp::div(a, b); }
inline float FMA(float a, float b, float c) { return nvp::fma(a, b, c); }
inline float SQRT(float a) { return nvp::sqrt(a); }
inline float FRECPE(float x) { return nvp::frecpe(x); }
inline float FRECPS(float a, float b) { return nvp::frecps(a, b); }
// nv_pixnoise._fminmax: FMIN / FMAX (x op1, c op2), NaNs as FPProcessNaNs(x, c)
inline float fmin_arm(float x, float c) { return nvp::fmin(x, c); }
inline float fmax_arm(float x, float c) { return nvp::fmax(x, c); }

// FMAXNM(x, c) with a constant c > 0 (nv_band.fmaxnm: a NaN gives c)
inline float fmaxnm(float x, float c) { return (isnanf_(x) || x < c) ? c : x; }

// nv_model.sqrt_abs (FUN_009ed3e0): fsqrt(fabs(x))
inline float sqrt_abs(float x) { return SQRT(bits(ubits(x) & 0x7fffffffu)); }

// ------------------------------------------------------------------------------ constants (Python bits)
const float NEG_1E6 = bits(0xc9742400u);
const float EPS_1E6 = bits(0x358637bdu);
const float Q99 = bits(0x3f7d70a4u);
const float C11 = bits(0x3f8ccccdu);        // 1.1
const float C125 = bits(0x3fa00000u);       // 1.25
const float C075 = bits(0x3f400000u);
const float C05 = bits(0x3f000000u);
const float C025 = bits(0x3e800000u);
const float C1 = bits(0x3f800000u);
const float C3 = bits(0x40400000u);
const float C4 = bits(0x40800000u);
const float C0_0625 = bits(0x3d800000u);
const float LEVEL_C[5] = {bits(0x3f87ae14u), bits(0x3f800000u), bits(0x3f800000u), bits(0x3f800000u),
                          bits(0x3f800000u)};    // DAT_012445a8
const float FLT_MAX_ = bits(0x7f7fffffu);
// nv_model_special
const float C_2M10 = bits(0x3a800000u);      // 2^-10
const float C_1E3 = bits(0x3a83126fu);       // 0.001
const float C_1_24 = bits(0x3d2aaaabu);      // 1/24
const float C_SCORE = bits(0x407988e2u);     // 3.8989797
const float C_THR_TILED = bits(0x3e19999au); // 0.15
const float C_THR = bits(0x3dcccccdu);       // 0.1
const float C_064 = bits(0x3f23d70au);       // 0.64
const float C_1E4 = bits(0x38d1b717u);       // 1e-4
const float C_NINTH = bits(0x3de38e39u);     // 1/9
const float C_1_5 = bits(0x3fc00000u);
const float C_24 = bits(0x41c00000u);
const float C_255 = bits(0x437f0000u);
const float C_M1 = bits(0xbf800000u);
// nv_pixnoise.ramp
const float C_1E5 = bits(0x3727c5acu);
const float C_099999 = bits(0x3f7fff58u);
const float C_100001 = bits(0x3f800054u);
// nv_model_bm
const float Q50 = bits(0x3f000000u);
const float Q85 = bits(0x3f59999au);

// libSystem functions through volatile pointers (no constant folding / rewriting)
double (*volatile g_atan2)(double, double) = NV_LIBM(atan2, atan2);
struct __double2 (*volatile g_sincos)(double) = NV_LIBM(__sincos_stret, sincos_stret);

using Vec = std::vector<float>;

inline size_t sz(int W, int H) { return static_cast<size_t>(W) * static_cast<size_t>(H); }

// ------------------------------------------------------------------------------ the set table
const uint32_t ANG16[16] = {0x0, 0x3e7adbb0, 0x3eed6338, 0x3f24bc7d, 0x3f490fdb, 0x3f6d6338, 0x3f8db70d,
                            0x3fa9b465, 0x3fc90fdb, 0x3fe86b51, 0x40023454, 0x400db70d, 0x4016cbe4,
                            0x401fe0bb, 0x402b6374, 0x40396220};
const uint32_t ANG8[8] = {0x0, 0x3fc90fdb, 0x3f490fdb, 0x4016cbe4, 0x3eed6338, 0x40023454, 0x3f8db70d,
                          0x402b6374};
const int IDX16[16] = {0, 0, 0, 4, 4, 4, 8, 8, 8, 8, 8, 12, 12, 12, 0, 0};
const int RAD16[16] = {2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 2};

struct Fam { int r, rs, div, b; };
struct SetDef {
    int flag;
    int nd;
    const uint32_t* angles;
    int idx2[16];
    int rad2[16];
    Fam Af, Bf, Cf;
    int dc_r, dc_div, dc_b;
    int dc_twice, special;
};

SetDef make_set(int s) {
    SetDef d{};
    for (int i = 0; i < 16; ++i) d.idx2[i] = -1, d.rad2[i] = 1;
    switch (s) {
        case 0:
            break;
        case 1:
            d.nd = 4; d.angles = ANG8; d.Af = {4, 1, 6, 4};
            break;
        case 2: {
            d.nd = 8; d.angles = ANG8; d.Af = {4, 1, 8, 4};
            const int i2[8] = {-1, -1, -1, -1, 0, 1, 1, 0};
            for (int i = 0; i < 8; ++i) d.idx2[i] = i2[i];
            d.dc_r = 4; d.dc_div = 16; d.dc_b = 4;
            break;
        }
        default:
            d.flag = 1; d.nd = 16; d.angles = ANG16;
            for (int i = 0; i < 16; ++i) d.idx2[i] = IDX16[i], d.rad2[i] = RAD16[i];
            d.Af = {4, 1, 16, 4}; d.Bf = {6, 2, 24, 6};
            if (s == 4) d.Cf = {12, 2, 48, 12};
            d.dc_r = 4; d.dc_div = 36; d.dc_b = 6; d.dc_twice = 1; d.special = s == 4;
            break;
    }
    return d;
}

const int OFF_DC = 0xdc, OFF_A = 0x140, OFF_R = 0x780, OFF_B = 0x7e4, OFF_C = 0xe24, P_OFF = 0x1464;

// ------------------------------------------------------------------------------ nv_lines
// An oriented kernel with its entry-major plan (nv_linekernel, shared with nv_curve)
struct LineKernel : nvl::LineKernel, nvl::LinePlan {};

// nv_lines.LineKernel.__init__ (FUN_00990448) + nv_lines.LinePlan
void make_kernel(LineKernel& k, int W, int H, float a) {
    static_cast<nvl::LineKernel&>(k) = nvl::make_kernel(W, H, a);
    if (!k.horiz && !k.vert) static_cast<nvl::LinePlan&>(k) = nvl::make_plan(k);
}

struct KernelList {
    std::vector<LineKernel> k;
};

// nv_model.kernels: one LineKernel per angle at (Wk, Hk), cached
const KernelList& kernels(int Wk, int Hk, const uint32_t* angles, int nd) {
    static std::mutex mu;
    static std::map<std::tuple<int, int, const uint32_t*, int>, std::unique_ptr<KernelList>> cache;
    std::lock_guard<std::mutex> lock(mu);
    auto& slot = cache[std::make_tuple(Wk, Hk, angles, nd)];
    if (!slot) {
        auto kl = std::make_unique<KernelList>();
        kl->k.resize(nd);
        for (int i = 0; i < nd; ++i) make_kernel(kl->k[i], Wk, Hk, bits(angles[i]));
        slot = std::move(kl);
    }
    return *slot;
}

// nv_lines._general (mode 1), in the entry-major form of _general_bm: the entries past the end of a
// line are -0.0, and S + (-0.0) == S bit for bit (S is the result of an addition, so never signalling).
void general_filter(const LineKernel& K, const float* X, float* out, int r) {
    const float recip = 1.0f / static_cast<float>(2 * r + 1);
    const int m = K.m, L = K.L, HW = K.W * K.H;
    const size_t mL = static_cast<size_t>(m) * L;
    Vec xp(static_cast<size_t>(HW) + 1), Am(mL), Sh(mL), Sv(L, 0.0f);
    std::memcpy(xp.data(), X, sizeof(float) * HW);
    xp[HW] = -0.0f;
    for (size_t q = 0; q < mL; ++q) Am[q] = xp[K.G[q]];
    float* s = Sv.data();
    for (int j2 = 0; j2 < std::min(r, m); ++j2) {
        const float* a = Am.data() + static_cast<size_t>(j2) * L;
        for (int l = 0; l < L; ++l) s[l] = A(s[l], a[l]);
    }
    for (int j = 0; j < m; ++j) {
        const int j2 = j + r, jo = j2 - 2 * r - 1;
        if (j2 < m) {
            const float* a = Am.data() + static_cast<size_t>(j2) * L;
            for (int l = 0; l < L; ++l) s[l] = A(s[l], a[l]);
        }
        if (jo >= 0) {
            const float* a = Am.data() + static_cast<size_t>(jo) * L;
            for (int l = 0; l < L; ++l) s[l] = S(s[l], a[l]);
        }
        std::memcpy(Sh.data() + static_cast<size_t>(j) * L, s, sizeof(float) * L);
    }
    for (int q = 0; q < HW; ++q) out[q] = M(recip, Sh[K.inv[q]]);
}

// nv_lines.linefilt (mode 1)
void linefilt(const LineKernel& K, const float* X, float* out, int r) {
    const int W = K.W, H = K.H;
    if (r == 0) {
        std::memcpy(out, X, sizeof(float) * sz(W, H));
        return;
    }
    if (K.horiz) {
        nvb::box_rows(X, W, H, r, 1, out);
        return;
    }
    if (K.vert) {
        nvb::box_cols(X, W, H, r, 1, out);
        return;
    }
    general_filter(K, X, out, r);
}

// nv_model.box2: rows then columns
Vec box2(const Vec& X, int W, int H, int r, int mode = 1) {
    Vec t(X.size()), o(X.size());
    nvb::box_rows(X.data(), W, H, r, mode, t.data());
    nvb::box_cols(t.data(), W, H, r, mode, o.data());
    return o;
}

// nv_model.dfilt
Vec dfilt(const KernelList& K, const SetDef& sd, int i, const Vec& X, int r) {
    Vec f(X.size());
    linefilt(K.k[i], X.data(), f.data(), r);
    const int j = sd.idx2[i];
    if (j != -1) {
        Vec g(X.size());
        linefilt(K.k[j], f.data(), g.data(), sd.rad2[i]);
        return g;
    }
    return f;
}

// ------------------------------------------------------------------------------ element-wise pieces
Vec vsq(const Vec& x) {
    Vec o(x.size());
    for (size_t i = 0; i < x.size(); ++i) o[i] = M(x[i], x[i]);
    return o;
}
Vec vsqrt_abs(const Vec& x) {
    Vec o(x.size());
    for (size_t i = 0; i < x.size(); ++i) o[i] = sqrt_abs(x[i]);
    return o;
}
Vec vsub(const Vec& a, const Vec& b) {
    Vec o(a.size());
    for (size_t i = 0; i < a.size(); ++i) o[i] = S(a[i], b[i]);
    return o;
}
Vec vadd(const Vec& a, const Vec& b) {
    Vec o(a.size());
    for (size_t i = 0; i < a.size(); ++i) o[i] = A(a[i], b[i]);
    return o;
}

// nv_model.robust_weight (FUN_009ef0d0)
Vec robust_weight(float t, const Vec& a, const Vec& e) {
    const float inv = 1.0f / t;
    const size_t n = a.size(), m = n & ~static_cast<size_t>(3);
    Vec out(n);
    for (size_t i = 0; i < m; ++i) {
        const float z = M(inv, e[i]);
        const bool big = std::fabs(z) > 1.0f;
        const float z2 = M(z, z);
        const float est = FRECPE(z2);
        const float w = M(est, FRECPS(z2, est));
        const float sel = big ? w : 1.0f;
        out[i] = M(a[i], sel);
    }
    for (size_t i = m; i < n; ++i) {
        const float z = M(inv, e[i]);
        const bool le = (std::fabs(z) <= 1.0f) || isnanf_(z);
        const float z2 = M(z, z);
        out[i] = le ? a[i] : D(a[i], z2);
    }
    return out;
}

// nv_model.argmax_dirs (FUN_009ee5fc): idx of the per-pixel maximum over E[0..nd) (>=, start 0)
std::vector<uint8_t> argmax_dirs(const std::vector<Vec>& E, int nd, size_t n) {
    std::vector<uint8_t> idx(n, 0);
    Vec best(n, 0.0f);
    for (int j = 0; j < nd; ++j) {
        const Vec& x = E[j];
        for (size_t p = 0; p < n; ++p)
            if (x[p] >= best[p]) {
                best[p] = x[p];
                idx[p] = static_cast<uint8_t>(j);
            }
    }
    return idx;
}

// nv_model.extrap: -1e6 stays, else fac * -|prev|
float extrap(float prev, float fac) {
    const float neg = bits((ubits(prev) & 0x7fffffffu) | 0x80000000u);
    const float v = M(fac, neg);
    return prev == NEG_1E6 ? NEG_1E6 : v;
}

// nv_model.borders
void borders(int Wk, int Hk, int b, int min_area, int* pbx, int* pby, bool* psmall) {
    const int hx = (Wk - 1 < 0 ? Wk : Wk - 1) >> 1;
    const int hy = (Hk - 1 < 0 ? Hk : Hk - 1) >> 1;
    bool small = hx < b || hy < b;
    int bx = hx < b ? hx : b;
    int by = hy < b ? hy : b;
    while (static_cast<long long>(Wk - 2 * bx) * (Hk - 2 * by) < min_area && (bx != 0 || by != 0)) {
        if (bx > 0 || by > 0) small = true;
        bx -= bx > 0;
        by -= by > 0;
    }
    *pbx = bx;
    *pby = by;
    *psmall = small;
}

Vec crop(const Vec& E, int W, int H, int bx, int by) {
    Vec o;
    o.reserve(sz(W - 2 * bx, H - 2 * by));
    for (int y = by; y < H - by; ++y)
        for (int x = bx; x < W - bx; ++x) o.push_back(E[static_cast<size_t>(y) * W + x]);
    return o;
}

// nv_model.q99: fmaxnm(FUN_00997bcc(0.99, v, NULL, 1, div, n), 1e-6)
float q99(const Vec& v, int div) {
    float r = 0.0f;
    if (!nvb::quantile(v.data(), v.size(), Q99, 1, div, &r)) throw std::logic_error("quantile refused");
    return fmaxnm(r, EPS_1E6);
}

float quantile_raw(const Vec& v, float q) {
    float r = 0.0f;
    if (!nvb::quantile(v.data(), v.size(), q, 1, 1, &r)) throw std::logic_error("quantile refused");
    return r;
}

// nv_pixnoise.up2 (FUN_00993df0): (Hc, Wc) -> (2Hc + oddH, 2Wc + oddW)
Vec up2(const Vec& lo, int Wc, int Hc, int oddW, int oddH) {
    const int W2 = 2 * Wc + oddW, H2 = 2 * Hc + oddH;
    Vec out(sz(W2, H2));
    if (Hc < 2 || Wc < 4) {                      // _up2_scalar
        if (Wc < 2) throw std::logic_error("scalar up-sampling with Wc < 2");
        Vec s(Wc), a3(Wc);
        for (int rr = 0; rr < Hc; ++rr) {
            const float* x = lo.data() + static_cast<size_t>(rr) * Wc;
            for (int r0 = 0; r0 < 2; ++r0) {
                const int nb = r0 == 0 ? (rr > 0 ? rr - 1 : 0) : (rr + 1 < Hc ? rr + 1 : Hc - 1);
                const float* y = lo.data() + static_cast<size_t>(nb) * Wc;
                for (int i = 0; i < Wc; ++i) s[i] = A(M(x[i], C3), y[i]);
                for (int i = 0; i < Wc; ++i) a3[i] = M(s[i], C3);
                float* row = out.data() + static_cast<size_t>(2 * rr + r0) * W2;
                row[0] = M(M(s[0], C4), C0_0625);
                for (int i = 1; i < Wc; ++i) row[2 * i] = M(A(a3[i], s[i - 1]), C0_0625);
                for (int i = 0; i < Wc - 1; ++i) row[2 * i + 1] = M(A(a3[i], s[i + 1]), C0_0625);
                row[2 * Wc - 1] = M(M(s[Wc - 1], C4), C0_0625);
            }
        }
    } else {                                     // _up2_neon
        const int w2 = 2 * Wc;
        Vec h(sz(w2, Hc));
        for (int i = 0; i < Hc; ++i) {
            const float* l = lo.data() + static_cast<size_t>(i) * Wc;
            float* hr = h.data() + static_cast<size_t>(i) * w2;
            for (int j = 0; j < Wc; ++j) {
                const float prev = l[j > 0 ? j - 1 : 0];
                const float nxt = l[j < Wc - 1 ? j + 1 : Wc - 1];
                const float xs = M(l[j], C075);
                hr[2 * j] = A(xs, M(prev, C025));
                hr[2 * j + 1] = A(xs, M(nxt, C025));
            }
        }
        auto O = [&](int r) { return out.data() + static_cast<size_t>(r) * W2; };
        std::memcpy(O(0), h.data(), sizeof(float) * w2);
        std::memcpy(O(2 * Hc - 1), h.data() + static_cast<size_t>(Hc - 1) * w2, sizeof(float) * w2);
        for (int i = 1; i < Hc; ++i) {
            const float* hp = h.data() + static_cast<size_t>(i - 1) * w2;
            const float* hn = hp + w2;
            float* oa = O(2 * i - 1);
            float* ob = O(2 * i);
            for (int c = 0; c < w2; ++c) {
                oa[c] = A(M(hp[c], C075), M(hn[c], C025));
                ob[c] = A(M(hp[c], C025), M(hn[c], C075));
            }
        }
    }
    if (oddW)
        for (int r = 0; r < 2 * Hc; ++r) {
            float* row = out.data() + static_cast<size_t>(r) * W2;
            row[2 * Wc] = row[2 * Wc - 1];
        }
    if (oddH)
        std::memcpy(out.data() + static_cast<size_t>(2 * Hc) * W2, out.data() + static_cast<size_t>(2 * Hc - 1) * W2,
                    sizeof(float) * W2);
    return out;
}

// nv_model.blend_add(1.0, 0.0, A, B) (FUN_009ed978): NEON A + fma(B, 1, 0); tail (A + B*1) + 0
Vec blend_add_10(const Vec& Av, const Vec& Bv) {
    const size_t n = Av.size(), m = n & ~static_cast<size_t>(3);
    const float one = opaque(1.0f), zero = opaque(0.0f);
    Vec out(n);
    for (size_t i = 0; i < m; ++i) out[i] = A(Av[i], FMA(Bv[i], one, zero));
    for (size_t i = m; i < n; ++i) out[i] = A(A(Av[i], M(Bv[i], one)), zero);
    return out;
}

// nv_pixnoise.ratio_eps (FUN_009ee04c)
Vec ratio_eps(const Vec& a, const Vec& b, float eps) {
    const size_t n = a.size(), m = n & ~static_cast<size_t>(3);
    Vec out(n);
    for (size_t i = 0; i < m; ++i) {
        const float d = A(eps, b[i]);
        const float est = FRECPE(d);
        const float step = FRECPS(d, est);
        out[i] = M(a[i], M(est, step));
    }
    for (size_t i = m; i < n; ++i) out[i] = D(a[i], A(b[i], eps));
    return out;
}

// nv_pixnoise.ramp (FUN_009ee93c)
Vec ramp(float a, float b, float va, float vb, const Vec& x) {
    if (b - a <= b * C_1E5) {
        a = (a * C_099999) + (-C_1E5);
        b = (b * C_100001) + C_1E5;
    }
    const float slope = (vb - va) / (b - a);
    const float icpt = va - (a * slope);
    float lo, hi;
    if (!(vb > va)) lo = vb, hi = va;
    else lo = va, hi = vb;
    const size_t n = x.size(), m = n & ~static_cast<size_t>(3);
    Vec y(n);
    for (size_t i = 0; i < m; ++i) y[i] = fmax_arm(fmin_arm(FMA(x[i], slope, icpt), hi), lo);
    for (size_t i = m; i < n; ++i) {
        float t = A(icpt, M(slope, x[i]));
        t = t > hi ? hi : t;
        y[i] = t > lo ? t : lo;
    }
    return y;
}

// ------------------------------------------------------------------------------ nv_model.Pyramid
struct Pyramid {
    std::vector<std::array<int, 4>> sizes;
    std::vector<Vec> band, E;
    std::vector<bool> have;

    Pyramid(const float* plane, int W, int H, int levels) {
        for (int k = 0; k <= levels; ++k) sizes.push_back({W >> k, H >> k, (W >> k) & 1, (H >> k) & 1});
        band.resize(levels);
        E.resize(levels);
        have.assign(levels, false);
        Vec src(plane, plane + sz(W, H));
        for (int k = 0; k < levels; ++k) {
            const int Wk = sizes[k][0], Hk = sizes[k][1];
            const int Wn = sizes[k + 1][0], Hn = sizes[k + 1][1];
            if (Wn < 1 || Hn < 1) break;
            const Vec B = box2(src, Wk, Hk, 1, 0);
            Vec lo(sz(Wk >> 1, Hk >> 1));
            nvb::down2(B.data(), Wk, Hk, lo.data());
            const int Wc = Wk >> 1, Hc = Hk >> 1;
            if ((Hc < 2 || Wc < 4) && Wc < 2) break;     // _up2_scalar: NotImplementedError
            Vec b(sz(Wk, Hk));
            nvb::band_from(lo.data(), Wc, Hc, src.data(), Wk, Hk, b.data());
            band[k] = b;
            E[k] = vsqrt_abs(box2(vsq(b), Wk, Hk, 1, 1));
            have[k] = true;
            src.swap(lo);
        }
    }
};

struct LevelInfo {
    bool small;
    int bx, by;
    float fac;
};

// nv_model.level_info
std::vector<LevelInfo> level_info(int W, int H, const uint32_t* words, int levels) {
    std::vector<LevelInfo> out;
    float fac = C05;
    for (int k = 0; k < levels; ++k) {
        const int Wk = W >> k, Hk = H >> k;
        int bx, by;
        bool small;
        borders(Wk, Hk, 4, 32, &bx, &by, &small);
        if (!small && k) {
            const float r = std::fabs(D(bits(words[5 + k]), bits(words[4 + k])));
            if (r < C05) fac = C05;
            else fac = fmin_arm(r, C1);
        }
        out.push_back({small, bx, by, fac});
    }
    return out;
}

// ============================================================================== nv_model_special
struct Img {
    int W = 0, H = 0;
    Vec d;
    Img() = default;
    Img(int w, int h) : W(w), H(h), d(sz(w, h), 0.0f) {}
    float& at(int y, int x) { return d[static_cast<size_t>(y) * W + x]; }
    float at(int y, int x) const { return d[static_cast<size_t>(y) * W + x]; }
};
struct U8Img {
    int W = 0, H = 0;
    std::vector<uint8_t> d;
    uint8_t at(int y, int x) const { return d[static_cast<size_t>(y) * W + x]; }
};

// nv_model_special.minmax (FUN_009efdd4; the maximum is nv_band._vmax)
void minmax(const Vec& v, float* pmn, float* pmx) {
    const size_t n = v.size(), m = n & ~static_cast<size_t>(7);
    float mn = FLT_MAX_, mx = -FLT_MAX_;
    if (m) {
        float lmin[8], lmax[8];
        for (int j = 0; j < 8; ++j) {
            bool nan = false, negzero = false, poszero = false;
            float lastnan = 0.0f, lo = INFINITY, top = -INFINITY;
            for (size_t i = j; i < m; i += 8) {
                const float x = v[i];
                if (isnanf_(x)) {
                    nan = true;
                    lastnan = x;
                    continue;
                }
                if (x < lo) lo = x;
                if (x > top) top = x;
                if (x == 0.0f) (std::signbit(x) ? negzero : poszero) = true;
            }
            if (nan) {
                lmin[j] = lmax[j] = quiet(lastnan);
                continue;
            }
            if (lo > FLT_MAX_) lo = FLT_MAX_;
            if (lo == 0.0f) lo = negzero ? -0.0f : 0.0f;
            if (top < -FLT_MAX_) top = -FLT_MAX_;
            if (top == 0.0f) top = poszero ? 0.0f : -0.0f;
            lmin[j] = lo;
            lmax[j] = top;
        }
        float c[4], e[4];
        for (int j = 0; j < 4; ++j) c[j] = fmin_arm(lmin[j], lmin[j + 4]), e[j] = fmax_arm(lmax[j], lmax[j + 4]);
        mn = fmin_arm(fmin_arm(c[0], c[1]), fmin_arm(c[2], c[3]));
        mx = fmax_arm(fmax_arm(e[0], e[1]), fmax_arm(e[2], e[3]));
    }
    for (size_t i = m; i < n; ++i) {
        if (v[i] < mn) mn = v[i];
        if (v[i] > mx) mx = v[i];
    }
    *pmn = mn;
    *pmx = mx;
}

inline uint8_t u8_of(float y) {
    double r = static_cast<double>(y);
    if (r != r) return 0;
    r = std::nearbyint(r);
    if (r < 0.0) r = 0.0;
    if (r > 255.0) r = 255.0;
    return static_cast<uint8_t>(r);
}

// nv_model_special.image_u8 (minmax, then to_u8_rows / FUN_009f1110)
U8Img image_u8(const Img& Aimg) {
    U8Img u;
    u.W = Aimg.W;
    u.H = Aimg.H;
    u.d.assign(Aimg.d.size(), 0x80);
    float mn, mx;
    minmax(Aimg.d, &mn, &mx);
    if (mx == mn) return u;
    const float s = C_255 / (mx - mn);
    const float off = s * (-mn);
    const int W = Aimg.W, m = W & ~15;
    for (int y = 0; y < Aimg.H; ++y) {
        const float* x = Aimg.d.data() + static_cast<size_t>(y) * W;
        uint8_t* o = u.d.data() + static_cast<size_t>(y) * W;
        for (int i = 0; i < m; ++i) o[i] = u8_of(FMA(x[i], s, off));
        for (int i = m; i < W; ++i) o[i] = u8_of(A(M(x[i], s), off));
    }
    return u;
}

// nv_model_special.box3_fused (FUN_00990c0c, NEON 2-D path FUN_009ed014)
Img box3_fused(const Img& X) {
    const int W = X.W, H = X.H;
    if (W < 4 || H < 2) throw std::logic_error("box3_fused: separable fallback not ported");
    Img h(W, H), out(W, H);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            const float L = x > 0 ? X.at(y, x - 1) : 0.0f;
            const float R = x + 1 < W ? X.at(y, x + 1) : 0.0f;
            h.at(y, x) = A(A(X.at(y, x), L), R);
        }
    for (int x = 0; x < W; ++x) out.at(0, x) = A(A(h.at(0, x), 0.0f), h.at(1, x));
    for (int y = 1; y < H - 1; ++y)
        for (int x = 0; x < W; ++x) out.at(y, x) = A(A(h.at(y, x), h.at(y - 1, x)), h.at(y + 1, x));
    for (int x = 0; x < W; ++x) out.at(H - 1, x) = A(0.0f, A(h.at(H - 1, x), h.at(H - 2, x)));
    for (auto& v : out.d) v = M(v, C_NINTH);
    return out;
}

// nv_model_special.local_rms (r != 1): 1.0 * sqrt|box2(src^2, r, 1)|
Vec local_rms(const Img& X, int r) {
    Vec t = vsqrt_abs(box2(vsq(X.d), X.W, X.H, r, 1));
    const float one = opaque(1.0f);
    for (auto& v : t) v = M(one, v);
    return t;
}

using Off = std::pair<int, int>;   // (dx, dy)

// nv_model_special.pattern_remove_fast (FUN_00ad3c88, accum = None)
Img pattern_remove(const Img& src, const std::vector<Off>& offs) {
    const int H = src.H, W = src.W;
    const int n = static_cast<int>(offs.size());
    Img dst(W, H);
    if (H > 48) {
        const float inv = 1.0f / static_cast<float>(n);
        for (int y = 24; y < H - 24; ++y)
            for (int x = 24; x < W - 24; ++x) {
                float acc = 0.0f;
                for (const auto& o : offs) acc = A(src.at(y + o.second, x + o.first), acc);
                dst.at(y, x) = M(acc, inv);
            }
    }
    {   // border rows, middle columns
        std::vector<int> rows;
        for (int y = 0; y < 24; ++y) rows.push_back(y);
        for (int y = std::max(H, 48) - 24; y < H; ++y) rows.push_back(y);
        for (int y : rows) {
            int cnt = 0;
            for (const auto& o : offs) {
                const int yy = y + o.second;
                cnt += (y < 24) ? (yy >= 0 && yy < H) : (yy < H);
            }
            const float scale = cnt > 0 ? 1.0f / static_cast<float>(cnt) : 1.0f;
            for (int x = 24; x < W - 24; ++x) {
                float acc = dst.at(y, x);
                for (const auto& o : offs) {
                    const int yy = y + o.second;
                    const bool ok = (y < 24) ? (yy >= 0 && yy < H) : (yy < H);
                    if (ok) acc = A(src.at(std::min(std::max(yy, 0), H - 1), x + o.first), acc);
                }
                if (cnt > 0) acc = M(acc, scale);
                dst.at(y, x) = acc;
            }
        }
    }
    {   // border columns, all rows
        std::vector<int> xs;
        for (int x = 0; x < 24; ++x) xs.push_back(x);
        for (int x = std::max(W, 48) - 24; x < W; ++x) xs.push_back(x);
        std::vector<float> vals(sz(static_cast<int>(xs.size()), H));
        for (int y = 0; y < H; ++y)
            for (size_t xi = 0; xi < xs.size(); ++xi) {
                const int x = xs[xi];
                float acc = dst.at(y, x);
                int cnt = 0;
                for (const auto& o : offs) {
                    const int yy = y + o.second, xx = x + o.first;
                    const bool ok = (yy >= 0 && yy < H) && (xx < W && (x >= 24 || xx >= 0));
                    if (ok) {
                        acc = A(src.at(yy, xx), acc);
                        ++cnt;
                    }
                }
                if (cnt > 0) acc = D(acc, static_cast<float>(cnt));
                vals[static_cast<size_t>(y) * xs.size() + xi] = acc;
            }
        for (int y = 0; y < H; ++y)
            for (size_t xi = 0; xi < xs.size(); ++xi) dst.at(y, xs[xi]) = vals[static_cast<size_t>(y) * xs.size() + xi];
    }
    const Vec P = local_rms(src, 16);
    const Vec Q = local_rms(dst, 2);
    const Vec ratio = ratio_eps(P, Q, C_1E4);
    const float t = sqrt_abs(static_cast<float>(n)) * C_064;
    const Vec w = ramp(t, t * C_1_5, 1.0f, 0.0f, ratio);
    const size_t N = dst.d.size(), m = N & ~static_cast<size_t>(3);
    Img out(W, H);
    for (size_t i = 0; i < m; ++i) out.d[i] = S(src.d[i], M(w[i], dst.d[i]));
    for (size_t i = m; i < N; ++i) out.d[i] = S(src.d[i], M(dst.d[i], w[i]));
    return out;
}

// nv_model_special.offsets_ring (FUN_00acf05c list)
std::vector<Off> offsets_ring(int rmin, int r) {
    std::vector<Off> o;
    for (int dy = -r; dy <= r; ++dy)
        for (int dx = -r; dx <= r; ++dx) {
            const int q = dx * dx + dy * dy;
            if (rmin * rmin <= q && q <= (r + 1) * r) o.push_back({dx, dy});
        }
    return o;
}

const std::vector<Off>& cand24() {
    static const std::vector<Off> c = offsets_ring(4, 24);
    return c;
}

// nv_model_special.select_greedy(costs, cand, 12, 2, 24)
std::vector<Off> select_greedy(const std::vector<int64_t>& costs, const std::vector<Off>& cand) {
    const int nsel = 12, sup = 2, r = 24;
    const int side = 2 * sup + (2 * r | 1) - 2;
    std::vector<uint8_t> mask(static_cast<size_t>(side) * side, 0);
    const int center = (side * side) >> 1;
    std::vector<Off> sel;
    for (int it = 0; it < nsel; ++it) {
        size_t j = 0;
        int64_t best = 0;
        bool first = true;
        for (size_t i = 0; i < cand.size(); ++i) {
            const int mi = cand[i].first + side * cand[i].second + center;
            const int64_t c = mask[mi] ? 0x7fffffffLL : costs[i];
            if (first || c < best) {
                best = c;
                j = i;
                first = false;
            }
        }
        if (!(best < 0x7fffffffLL)) j = 0;
        const int dx = cand[j].first, dy = cand[j].second;
        sel.push_back({dx, dy});
        for (int yy = dy - (sup - 1); yy < dy + sup; ++yy) {
            const int base = center + side * yy + dx - (sup - 1);
            for (int q = 0; q < 2 * sup - 1; ++q) mask[base + q] = 1;
        }
    }
    return sel;
}

// nv_model_special._offsets_at_fast: SAD of the 32x32 block at (bx, by) for every candidate
std::vector<Off> offsets_at(const U8Img& U, int bx, int by) {
    const auto& cand = cand24();
    std::vector<int64_t> costs(cand.size());
    for (size_t i = 0; i < cand.size(); ++i) {
        const int dx = cand[i].first, dy = cand[i].second;
        int s = 0;
        for (int r = 0; r < 32; ++r) {
            const uint8_t* a = U.d.data() + static_cast<size_t>(by + r) * U.W + bx;
            const uint8_t* b = U.d.data() + static_cast<size_t>(by + dy + r) * U.W + bx + dx;
            for (int c = 0; c < 32; ++c) s += std::abs(static_cast<int>(b[c]) - static_cast<int>(a[c]));
        }
        costs[i] = s;
    }
    std::vector<Off> sel = select_greedy(costs, cand);
    std::vector<Off> out = sel;
    for (const auto& o : sel) out.push_back({-o.first, -o.second});
    return out;
}

// nv_model_special.block_sum32 (FUN_009fada0)
float block_sum32(const Img& X, int x0, int y0) {
    float acc[4][4] = {};
    for (int r = 0; r < 32; ++r) {
        const float* row = X.d.data() + static_cast<size_t>(y0 + r) * X.W + x0;
        for (int a = 0; a < 4; ++a)
            for (int l = 0; l < 4; ++l) acc[a][l] = A(acc[a][l], row[4 * a + l]);
        for (int a = 0; a < 4; ++a)
            for (int l = 0; l < 4; ++l) acc[a][l] = A(acc[a][l], row[16 + 4 * a + l]);
    }
    float v[4];
    for (int l = 0; l < 4; ++l) {
        const float v0 = A(acc[1][l], acc[0][l]);
        const float v1 = A(acc[3][l], acc[2][l]);
        v[l] = A(v1, v0);
    }
    return A(A(v[0], v[1]), A(v[2], v[3]));
}

// nv_model_special.block_std32_fast (FUN_009ef664)
float block_std32(const Img& X, int x0, int y0, float m) {
    float acc[4] = {};
    for (int r = 0; r < 32; ++r) {
        const float* row = X.d.data() + static_cast<size_t>(y0 + r) * X.W + x0;
        for (int c = 0; c < 8; ++c)
            for (int l = 0; l < 4; ++l) {
                const float d = S(row[4 * c + l], m);
                acc[l] = FMA(d, d, acc[l]);
            }
    }
    const float s = A(A(acc[0], acc[1]), A(acc[2], acc[3]));
    return sqrt_abs(M(s, C_2M10));
}

struct Cand {
    std::vector<Off> offs;
    float rms, score;
    bool has_score;
};

// nv_model_special._grid_fast
std::vector<Cand> grid_cands(const Img& Ai, const U8Img& U, const std::vector<Off>& grid) {
    std::vector<Cand> res;
    std::vector<float> acc(1024);
    for (const auto& g : grid) {
        const int bx = g.first, by = g.second;
        Cand c;
        c.offs = offsets_at(U, bx, by);
        float sd[24];
        for (int i = 0; i < 24; ++i) {
            const int x0 = bx + c.offs[i].first, y0 = by + c.offs[i].second;
            const float sm = block_sum32(Ai, x0, y0);
            sd[i] = block_std32(Ai, x0, y0, M(sm, C_2M10));
        }
        float s8 = 0.0f;
        for (int i = 0; i < 24; ++i) s8 = A(s8, M(sd[i], sd[i]));
        c.rms = D(sqrt_abs(s8), C_24);
        std::fill(acc.begin(), acc.end(), 0.0f);
        for (int i = 0; i < 24; ++i) {
            const int x0 = bx + c.offs[i].first, y0 = by + c.offs[i].second;
            for (int r = 0; r < 32; ++r)
                for (int q = 0; q < 32; ++q) acc[r * 32 + q] = A(Ai.at(y0 + r, x0 + q), acc[r * 32 + q]);
        }
        for (auto& v : acc) v = M(C_1_24, v);
        const float mean = nvb::mean_exact(acc.data(), acc.size());
        const float sdv = nvb::std_exact(acc.data(), acc.size(), mean);
        c.score = D(A(D(sdv, c.rms), C_M1), C_SCORE);
        c.has_score = !(c.rms < C_1E3);
        res.push_back(std::move(c));
    }
    return res;
}

Img tile3(const Img& X) {
    Img o(3 * X.W, 3 * X.H);
    for (int y = 0; y < o.H; ++y)
        for (int x = 0; x < o.W; ++x) o.at(y, x) = X.at(y % X.H, x % X.W);
    return o;
}

// ============================================================================== nv_model_bm
struct Cfg {
    int bs, step, pad, r1, rmin, n8, n10, n18, r28, r30, nbins;
    float fill;
};
const Cfg CFG[2] = {{16, 8, 4, 8, 1, 16, 16, 64, 8, 8, 8, bits(0x41800000u)},
                    {32, 16, 8, 16, 2, 32, 32, 128, 16, 16, 16, bits(0x42000000u)}};
const int64_t INF_COST = 0x7fffffff;

struct Table {
    std::vector<Off> list1, list2;
    Vec win;                                     // bs x bs
    std::vector<std::vector<int>> bins;
};

// nv_model_bm.window (acf05c tent window)
Vec window(int n) {
    const int h = n >> 1;
    const float d = static_cast<float>((h * n) >> 1);
    Vec w(sz(n, n));
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x) {
            const int ay = h - std::abs(y - h), ax = h - std::abs(x - h);
            w[static_cast<size_t>(y) * n + x] = static_cast<float>(ay * ax) / d;
        }
    return w;
}

// nv_model_bm.Table (FUN_00acf420, flag = 1)
Table make_table(int rmin, int r, int nbins, int winn) {
    Table t;
    t.list1 = offsets_ring(rmin, r);
    t.list2 = offsets_ring(rmin, r);
    t.win = window(winn);
    const int n = static_cast<int>(t.list1.size());
    const auto& L = t.list1;
    t.bins.assign(nbins, {});
    std::map<Off, int> pos;
    for (int j = n - 1; j >= 0; --j) pos[L[j]] = j;   // first index of (dx, dy)
    auto find = [&](int dx, int dy) {
        auto it = pos.find({dx, dy});
        return it == pos.end() ? -1 : it->second;
    };
    auto add = [&](int b, int idx) {
        auto& v = t.bins[b];
        if (std::find(v.begin(), v.end(), idx) == v.end()) v.push_back(idx);
    };
    const double PI = 3.141592653589793, NPI = -3.141592653589793;
    const double binw = PI / static_cast<double>(nbins);
    const double half = binw * 0.5;
    const double R = static_cast<double>(r / 1);
    for (int i = 0; i < n; ++i) {
        const int dx = L[i].first, dy = L[i].second;
        double a = g_atan2(static_cast<double>(dy), static_cast<double>(dx));
        a = half + (a + PI);
        a = a < PI ? a : a + NPI;
        a = a < PI ? a : a + NPI;
        const int b = static_cast<int>(a / binw);
        add(b, find(dx, dy));
        add(b, find(-dx, -dy));
        const double center = binw * (static_cast<double>(b) + 0.5);
        if (center <= a) {
            const int b2 = (b + 1) % nbins;
            add(b2, find(dx, dy));
            add(b2, find(-dx, -dy));
        }
        if (!(a > center)) {
            const int b2 = (b + nbins - 1) % nbins;
            add(b2, find(dx, dy));
            add(b2, find(-dx, -dy));
        }
        const double fy = static_cast<double>(dy), fx = static_cast<double>(dx);
        for (int l = 0; l < nbins; ++l) {
            const struct __double2 sc0 = g_sincos(binw * static_cast<double>(l) - half);
            const struct __double2 sc1 = g_sincos(binw * static_cast<double>(l + 1) - half);
            const double s0 = sc0.__sinval, c0 = sc0.__cosval, s1 = sc1.__sinval, c1 = sc1.__cosval;
            const double d15 = c0 * R, d14 = s0 * R;
            const double d2 = c1 * R, d3 = s1 * R;
            const double d4 = -(c0 * R), d5 = -(s0 * R);
            const double e1 = -(c1 * R), e0 = -(s1 * R);
            const double Aa = (e1 - d15) * (fy - d14) - (fx - d15) * (e0 - d14);
            const double Bb = (d4 - d2) * (fy - d3) - (fx - d2) * (d5 - d3);
            if (!(Aa * Bb > 0.0)) {
                add(l, find(dx, dy));
                add(l, find(-dx, -dy));
            }
        }
    }
    return t;
}

const Table& table(int c) {
    static std::mutex mu;
    static std::unique_ptr<Table> tabs[2];
    std::lock_guard<std::mutex> lock(mu);
    if (!tabs[c]) {
        const Cfg& g = CFG[c];
        tabs[c] = std::make_unique<Table>(make_table(g.rmin, g.r1, g.nbins, g.bs));
    }
    return *tabs[c];
}

// nv_model_bm.pad_mirror (np.pad mode reflect)
template <class T>
std::vector<T> pad_mirror(const std::vector<T>& X, int W, int H, int p) {
    if (p >= H || p >= W) throw std::logic_error("pad_mirror: edge-replicating branch not ported");
    const int Wp = W + 2 * p, Hp = H + 2 * p;
    auto refl = [](int i, int n) { return i < 0 ? -i : (i >= n ? 2 * (n - 1) - i : i); };
    std::vector<T> o(sz(Wp, Hp));
    for (int y = 0; y < Hp; ++y) {
        const int sy = refl(y - p, H);
        for (int x = 0; x < Wp; ++x) o[static_cast<size_t>(y) * Wp + x] = X[static_cast<size_t>(sy) * W + refl(x - p, W)];
    }
    return o;
}

std::vector<int> positions(int n, int step) {
    const int cnt = (n + step - 1) / step;
    std::vector<int> o;
    for (int g = 0; g < cnt; ++g) o.push_back(std::min(g * step, n - step));
    return o;
}

// nv_model_bm.sad_grid: costs (npos x nlist), INF where the moved block leaves the padded image
std::vector<int64_t> sad_grid(const std::vector<uint8_t>& Up, int Wp, int Hp, const std::vector<Off>& xy, int bs,
                              int pad, const std::vector<Off>& lst) {
    const size_t npos = xy.size(), nl = lst.size();
    std::vector<int64_t> out(npos * nl, INF_COST);
    std::vector<int32_t> I;
    for (size_t j = 0; j < nl; ++j) {
        const int dx = lst[j].first, dy = lst[j].second;
        bool any = false;
        for (size_t p = 0; p < npos && !any; ++p) {
            const int X = xy[p].first + pad, Y = xy[p].second + pad;
            any = X + dx >= 0 && X + dx + bs <= Wp && Y + dy >= 0 && Y + dy + bs <= Hp;
        }
        if (!any) continue;
        const int y0 = std::max(0, -dy), y1 = Hp - std::max(0, dy);
        const int x0 = std::max(0, -dx), x1 = Wp - std::max(0, dx);
        const int iw = x1 - x0 + 1, ih = y1 - y0 + 1;
        I.assign(sz(iw, ih), 0);
        for (int y = 0; y < ih - 1; ++y) {
            const uint8_t* a = Up.data() + static_cast<size_t>(y0 + y) * Wp + x0;
            const uint8_t* b = Up.data() + static_cast<size_t>(y0 + y + dy) * Wp + x0 + dx;
            int32_t* ir = I.data() + static_cast<size_t>(y + 1) * iw;
            const int32_t* ip = ir - iw;
            int32_t run = 0;
            for (int x = 0; x < iw - 1; ++x) {
                run += std::abs(static_cast<int>(b[x]) - static_cast<int>(a[x]));
                ir[x + 1] = ip[x + 1] + run;
            }
        }
        for (size_t p = 0; p < npos; ++p) {
            const int X = xy[p].first + pad, Y = xy[p].second + pad;
            if (!(X + dx >= 0 && X + dx + bs <= Wp && Y + dy >= 0 && Y + dy + bs <= Hp)) continue;
            const int yy = Y - y0, xx = X - x0;
            auto at = [&](int y, int x) { return static_cast<int64_t>(I[static_cast<size_t>(y) * iw + x]); };
            out[p * nl + j] = at(yy + bs, xx + bs) - at(yy, xx + bs) - at(yy + bs, xx) + at(yy, xx);
        }
    }
    return out;
}

// nv_model_bm.best_direction_all: false when a row has no qualifying bin (NotImplementedError)
bool best_direction_all(const std::vector<int64_t>& C, size_t npos, const Table& tab, int nsel,
                        std::vector<std::vector<Off>>* lists) {
    const size_t nl = tab.list1.size();
    const int64_t BIG = INT64_MAX;
    lists->clear();
    std::vector<int64_t> vals, srt;
    for (size_t p = 0; p < npos; ++p) {
        const int64_t* row = C.data() + p * nl;
        int64_t best = BIG, bestv = 0;
        int bsel = 0;
        for (size_t b = 0; b < tab.bins.size(); ++b) {
            const auto& idx = tab.bins[b];
            const int len = static_cast<int>(idx.size());
            int64_t sc = BIG, v = 0;
            if (len >= nsel) {
                vals.resize(len);
                int ninf = 0;
                for (int q = 0; q < len; ++q) {
                    vals[q] = row[idx[q]];
                    ninf += vals[q] == INF_COST;
                }
                srt = vals;
                std::nth_element(srt.begin(), srt.begin() + (nsel - 1), srt.end());
                v = srt[nsel - 1];
                if (nsel <= (len - ninf) / 2) {
                    int64_t s = 0, cle = 0;
                    for (int q = 0; q < len; ++q)
                        if (vals[q] <= v) s += vals[q], ++cle;
                    sc = s - v * (cle - nsel);
                }
            }
            if (sc < best) {                   // np.argmin: the first minimum
                best = sc;
                bestv = v;
                bsel = static_cast<int>(b);
            }
        }
        if (best == BIG) return false;
        const auto& idx = tab.bins[bsel];
        std::vector<Off> lst;
        for (size_t q = 0; q < idx.size(); ++q)
            if (row[idx[q]] < bestv) lst.push_back(tab.list1[idx[q]]);
        if (static_cast<int>(lst.size()) < nsel)
            for (size_t q = 0; q < idx.size(); ++q) {
                if (static_cast<int>(lst.size()) >= nsel) break;
                if (row[idx[q]] == bestv) lst.push_back(tab.list1[idx[q]]);
            }
        lists->push_back(std::move(lst));
    }
    return true;
}

// nv_model_bm._order_stat for one sorted row
int64_t order_stat(const std::vector<int64_t>& srt, int ninf, int64_t target) {
    const int64_t nval = static_cast<int64_t>(srt.size()) - ninf;
    const int64_t tgt = std::min(std::max(target, int64_t(0)), nval);
    return tgt > 0 ? srt[tgt - 1] : 0;
}

// nv_model_bm.ratio_sums_all (one row)
void ratio_sums(const int64_t* c, size_t n, int nsel, int nlarge, uint32_t* ps, uint32_t* pl) {
    std::vector<int64_t> srt(c, c + n);
    std::sort(srt.begin(), srt.end());
    int ninf = 0;
    for (size_t i = 0; i < n; ++i) ninf += c[i] == INF_COST;
    const int64_t v1 = order_stat(srt, ninf, nsel);
    const int64_t v2 = order_stat(srt, ninf, static_cast<int64_t>(n) - nlarge - ninf);
    int64_t s = 0, nlt = 0, l = 0, ngt = 0;
    for (size_t i = 0; i < n; ++i) {
        if (c[i] < v1) s += c[i], ++nlt;
        if (c[i] > v2 && c[i] != INF_COST) l += c[i], ++ngt;
    }
    s += v1 * (nsel - nlt);
    l += v2 * (nlarge - ngt);
    *ps = static_cast<uint32_t>(s & 0xffffffff);
    *pl = static_cast<uint32_t>(l & 0xffffffff);
}

// nv_model_bm.norm_image: 1 / (sum over the blocks of window * fill)
Vec norm_image(int W, int H, int c) {
    const Cfg& g = CFG[c];
    const Table& tab = table(c);
    const int bs = g.bs, pad = g.pad, Wp = W + 2 * pad, Hp = H + 2 * pad;
    Vec acc(sz(Wp, Hp), 0.0f), wf(tab.win.size());
    for (size_t i = 0; i < wf.size(); ++i) wf[i] = tab.win[i] * g.fill;
    for (int y : positions(H, g.step))
        for (int x : positions(W, g.step))
            for (int r = 0; r < bs; ++r)
                for (int q = 0; q < bs; ++q) {
                    float& a = acc[static_cast<size_t>(y + r) * Wp + x + q];
                    a = A(a, wf[static_cast<size_t>(r) * bs + q]);
                }
    Vec o(sz(W, H));
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) o[static_cast<size_t>(y) * W + x] = 1.0f / acc[static_cast<size_t>(y + pad) * Wp + x + pad];
    return o;
}

// nv_model_bm.ad1ae0 -> (quantile 0.5, quantile 0.85); false = NotImplementedError
bool ad1ae0(const Img& clean, const Img& Ai, int c, float* q0, float* q1) {
    const Cfg& g = CFG[c];
    const int bs = g.bs, step = g.step, pad = g.pad;
    const Table& tab = table(c);
    const int H = Ai.H, W = Ai.W, Wp = W + 2 * pad, Hp = H + 2 * pad;
    // nv_model_bm.pad_mirror raises NotImplementedError for pad >= side (the edge-replicating branch of
    // FUN_0097ef28, not ported); model_full never gets here (FUN_00ad319c passes sides >= 84)
    if (pad >= H || pad >= W) return false;
    // 1 - 2: direction selection on the u8 image of A
    const U8Img u = image_u8(Ai);
    const std::vector<uint8_t> U = pad_mirror(u.d, W, H, pad);
    const std::vector<int> xs = positions(W, step), ys = positions(H, step);
    std::vector<Off> grid;
    for (int y : ys)
        for (int x : xs) grid.push_back({x - pad, y - pad});
    const std::vector<int64_t> C1 = sad_grid(U, Wp, Hp, grid, bs, pad, tab.list1);
    std::vector<std::vector<Off>> lists;
    if (!best_direction_all(C1, grid.size(), tab, g.n8 - 1, &lists)) return false;
    for (auto& l : lists) l.push_back({0, 0});
    // 3: aggregation (raster order of the blocks = the per-pixel FMA order of _aggregate_waves)
    const Vec src = pad_mirror(clean.d, W, H, pad);
    Vec dst(src.size(), 0.0f);
    const Vec& win = tab.win;
    size_t bi = 0;
    for (int y : ys)
        for (int x : xs) {
            for (const auto& o : lists[bi])
                for (int r = 0; r < bs; ++r) {
                    float* drow = dst.data() + static_cast<size_t>(y + r) * Wp + x;
                    const float* srow = src.data() + static_cast<size_t>(y + o.second + r) * Wp + x + o.first;
                    const float* wrow = win.data() + static_cast<size_t>(r) * bs;
                    for (int q = 0; q < bs; ++q) drow[q] = FMA(wrow[q], srow[q], drow[q]);
                }
            ++bi;
        }
    const Vec nrm = norm_image(W, H, c);
    Img F(W, H);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            F.at(y, x) = M(dst[static_cast<size_t>(y + pad) * Wp + x + pad], nrm[static_cast<size_t>(y) * W + x]);
    // 4: ratios on the u8 image of the aggregate
    const U8Img u2 = image_u8(F);
    const std::vector<uint8_t> U2 = pad_mirror(u2.d, W, H, pad);
    const int nsel = g.n10 - 1, nlarge = g.n18;
    std::vector<Off> grid2;
    for (int gy = 0; gy < ((H + 7) >> 3); ++gy)
        for (int gx = 0; gx < ((W + 7) >> 3); ++gx)
            grid2.push_back({std::min(8 * gx, W - step) - pad, std::min(8 * gy, H - step) - pad});
    const std::vector<int64_t> C2 = sad_grid(U2, Wp, Hp, grid2, bs, pad, tab.list2);
    const size_t n2 = tab.list2.size();
    Vec v(grid2.size());
    for (size_t p = 0; p < grid2.size(); ++p) {
        uint32_t s, l;
        ratio_sums(C2.data() + p * n2, n2, nsel, nlarge, &s, &l);
        const float lf = static_cast<float>(static_cast<int64_t>(l)), sf = static_cast<float>(static_cast<int64_t>(s));
        v[p] = D(D(M(lf, static_cast<float>(nsel)), A(sf, C_1E5)), static_cast<float>(nlarge));
    }
    *q0 = quantile_raw(v, Q50);
    *q1 = quantile_raw(v, Q85);
    return true;
}

// nv_model_special.ad319c: OK, NOT_IMPLEMENTED (the Python NotImplementedError) or CANCELLED (stop() true)
template <class Stop>
int ad319c(uint32_t* words, const Vec& band_k, const Vec& M_k, int W, int H, int k, const Stop& stop) {
    if (std::min(W, H) * 3 < 83) return OK;
    Img band(W, H), Mi(W, H);
    band.d = band_k;
    Mi.d = M_k;
    float thr;
    if (W >= 89 && H > 88) {
        thr = C_THR;
    } else {
        thr = C_THR_TILED;
        band = tile3(band);
        Mi = tile3(Mi);
    }
    const int H2 = Mi.H, W2 = Mi.W;
    Img Ai = box3_fused(Mi);
    for (size_t i = 0; i < Ai.d.size(); ++i) Ai.d[i] = A(Mi.d[i], Ai.d[i]);
    const int cx = static_cast<int>(static_cast<double>(W2 - 81) / 24.0) | 3;
    const int cy = static_cast<int>(static_cast<double>(H2 - 81) / 24.0) | 3;
    const int stx = static_cast<int>(static_cast<double>(W2 - 81) / static_cast<double>(cx - 1));
    const int sty = static_cast<int>(static_cast<double>(H2 - 81) / static_cast<double>(cy - 1));
    float* out = reinterpret_cast<float*>(words);
    uint8_t* byte = reinterpret_cast<uint8_t*>(words);
    auto score_w = [&](int j) { return (P_OFF + 0x20 * k + 4 * j) / 4; };
    auto offs_b = [&](int j) { return P_OFF + 0xa0 + 0x200 * k + 0x40 * j; };
    auto read_offs = [&](int j) {
        std::vector<Off> o;
        const int8_t* p = reinterpret_cast<const int8_t*>(byte + offs_b(j));
        for (int i = 0; i < 24; ++i) o.push_back({p[2 * i], p[2 * i + 1]});
        return o;
    };
    std::vector<Off> grid;
    for (int gy = 0; gy < cy; ++gy)
        for (int gx = 0; gx < cx; ++gx) grid.push_back({24 + gx * stx, 24 + gy * sty});
    for (int j = 0; j < 5; ++j) {
        if (stop()) return CANCELLED;
        const U8Img U8 = image_u8(Ai);
        out[score_w(j)] = 0.0f;
        float best = 0.0f;
        const std::vector<Cand> cands = grid_cands(Ai, U8, grid);
        for (const auto& c : cands) {
            if (!(c.rms >= C_1E3) && !isnanf_(c.rms)) continue;
            // c.has_score is false only when rms < 0.001, which was skipped above
            if (c.score > best) {
                best = c.score;
                out[score_w(j)] = best;
                int8_t* p = reinterpret_cast<int8_t*>(byte + offs_b(j));
                for (int i = 0; i < 24; ++i) {
                    p[2 * i] = static_cast<int8_t>(c.offs[i].first);
                    p[2 * i + 1] = static_cast<int8_t>(c.offs[i].second);
                }
            }
        }
        if (best < thr) {
            out[score_w(j)] = 0.0f;
            break;
        }
        if (j > 0 && out[score_w(j - 1)] < best) {
            out[score_w(j)] = 0.0f;
            break;
        }
        Ai = pattern_remove(Ai, read_offs(j));
    }
    Img clean = band;
    if (out[score_w(0)] > 0.0f) {
        Img X = band;
        for (int j = 0; j < 8; ++j) {
            if (!(out[score_w(j)] > 0.0f)) break;
            X = pattern_remove(X, read_offs(j));
        }
        clean = X;
    }
    const int o4[2] = {0xaa0, 0xab4}, o5[2] = {0xac8, 0xadc};
    for (int c = 0; c < 2; ++c) {
        if (stop()) return CANCELLED;
        float a, b;
        if (!ad1ae0(clean, Ai, c, &a, &b)) return NOT_IMPLEMENTED;
        out[(P_OFF + o4[c] + 4 * k) / 4] = a;
        out[(P_OFF + o5[c] + 4 * k) / 4] = b;
    }
    return OK;
}

}  // namespace

// ============================================================================== nv_model_bm.ad1ae0
int bm_quantiles(const float* clean, const float* A, int W, int H, int cfg, float q[2]) {
    if (cfg < 0 || cfg > 1 || W < 1 || H < 1) return BAD_ARGUMENT;
    Img c(W, H), a(W, H);
    std::memcpy(c.d.data(), clean, sizeof(float) * c.d.size());
    std::memcpy(a.d.data(), A, sizeof(float) * a.d.size());
    return ad1ae0(c, a, cfg, &q[0], &q[1]) ? OK : NOT_IMPLEMENTED;
}

// ============================================================================== nv_model.model_full
int model_full(const float* plane, int W, int H, int set_, uint32_t* words, int levels, const Cancel* cancel) {
    if (W > 512 || H > 512 || W < 2 || H < 2 || levels < 1 || levels > LEVELS) return BAD_ARGUMENT;
    auto stop = [cancel]() { return cancel != nullptr && *cancel && (*cancel)(); };
    if (stop()) return CANCELLED;
    nvb::model_levels(plane, W, H, words, levels);
    float* out = reinterpret_cast<float*>(words);
    const std::vector<LevelInfo> info = level_info(W, H, words, levels);
    const Pyramid pyr(plane, W, H, levels);
    std::vector<int> sets;
    if (set_ < 0) sets = {0, 1, 2, 3, 4};
    else sets = {set_};
    for (int s : sets)
        if (s > 4) return BAD_ARGUMENT;
    for (int k = 0; k < levels; ++k) {
        const LevelInfo& li = info[k];
        const float fac = li.fac;
        const int Wk = W >> k, Hk = H >> k;
        const size_t n = sz(Wk, Hk);
        for (int s : sets) {
            if (stop()) return CANCELLED;
            const SetDef sd = make_set(s);
            const int nd = sd.nd;
            const KernelList* K = nullptr;
            const float q99k = out[5 + k];
            // ---------------------------------------------------------------- family A
            if (sd.Af.r >= 1) {
                const int r1 = sd.Af.r, rs = sd.Af.rs, div = sd.Af.div, b = sd.Af.b;
                int bx, by;
                bool smallA;
                borders(Wk, Hk, b, 8 * div, &bx, &by, &smallA);
                if (Wk <= 2 * r1 || Hk <= 2 * r1 || smallA) {
                    if (k) {
                        for (int d = 0; d < nd; ++d) {
                            const int o = (OFF_A + 0x140 * s + 0x40 * k + 4 * d) / 4;
                            out[o] = extrap(out[o - 16], fac);
                        }
                        for (int off : {OFF_R, OFF_DC}) {
                            const int o = (off + 0x14 * s + 4 * k) / 4;
                            out[o] = extrap(out[o - 1], fac);
                        }
                    }
                } else {
                    K = &kernels(Wk, Hk, sd.angles, nd);
                    const Vec& band = pyr.band[k];
                    std::vector<Vec> F(nd), E(nd);
                    for (int d = 0; d < nd; ++d) {
                        if (stop()) return CANCELLED;
                        Vec lf(n);
                        linefilt(K->k[d], band.data(), lf.data(), r1);
                        Vec T = vsub(band, lf);
                        float c = 1.0f;
                        if (sd.flag) {
                            T = vsqrt_abs(box2(vsq(T), Wk, Hk, 1, 1));
                            c = C075;
                        }
                        const float t = c * (LEVEL_C[k] * (std::fabs(q99k) * C11));
                        const Vec G = robust_weight(t, band, T);
                        Vec Fd = dfilt(*K, sd, d, G, r1);
                        T = vsq(Fd);
                        if (sd.flag) T = box2(box2(T, Wk, Hk, rs), Wk, Hk, rs);
                        else T = box2(T, Wk, Hk, rs);
                        const Vec Ed = vsqrt_abs(T);
                        if (sd.flag) Fd = vadd(Fd, dfilt(*K, sd, d, vsub(G, Fd), r1));
                        const float q = q99(crop(Ed, Wk, Hk, bx, by), div);
                        out[(OFF_A + 0x140 * s + 0x40 * k + 4 * d) / 4] = q;
                        F[d] = std::move(Fd);
                        const float iq = 1.0f / q;
                        E[d].resize(n);
                        for (size_t p = 0; p < n; ++p) E[d][p] = M(Ed[p], iq);
                    }
                    const std::vector<uint8_t> idx = argmax_dirs(E, nd, n);
                    Vec R(n);
                    for (size_t p = 0; p < n; ++p) R[p] = S(band[p], F[idx[p]][p]);
                    const Vec T = vsqrt_abs(box2(box2(vsq(R), Wk, Hk, 1), Wk, Hk, 1));
                    out[(OFF_R + 0x14 * s + 4 * k) / 4] = q99(crop(T, Wk, Hk, li.bx, li.by), 4);
                    // ------------------------------------------------------------ "dc"
                    int bx2, by2;
                    bool smalld;
                    borders(Wk, Hk, sd.dc_b, 8 * sd.dc_div, &bx2, &by2, &smalld);
                    const int o = (OFF_DC + 0x14 * s + 4 * k) / 4;
                    const int rdc = sd.dc_r;
                    if (!smalld && rdc >= 1 && Wk > 2 * rdc && Hk > 2 * rdc) {
                        const float t = (std::fabs(q99k) * LEVEL_C[k]) * C125;
                        Vec T2 = vsq(robust_weight(t, R, pyr.E[k]));
                        T2 = box2(T2, Wk, Hk, rdc);
                        if (sd.dc_twice) T2 = box2(T2, Wk, Hk, rdc);
                        T2 = vsqrt_abs(T2);
                        out[o] = q99(crop(T2, Wk, Hk, bx2, by2), sd.dc_div);
                    } else if (k) {
                        out[o] = extrap(out[o - 1], fac);
                    }
                }
            }
            // ---------------------------------------------------------------- families B, C
            for (int fam = 0; fam < 2; ++fam) {
                const Fam& fd = fam == 0 ? sd.Bf : sd.Cf;
                const int off = fam == 0 ? OFF_B : OFF_C;
                const int r = fd.r, rs = fd.rs, div = fd.div, b = fd.b;
                if (r < 1) continue;
                int bx, by;
                bool smallF;
                borders(Wk, Hk, b, 8 * div, &bx, &by, &smallF);
                if (Wk <= 2 * r || Hk <= 2 * r || smallF) {
                    if (k)
                        for (int d = 0; d < nd; ++d) {
                            const int o = (off + 0x140 * s + 0x40 * k + 4 * d) / 4;
                            out[o] = extrap(out[o - 16], fac);
                        }
                    continue;
                }
                if (!K) K = &kernels(Wk, Hk, sd.angles, nd);
                const Vec& band = pyr.band[k];
                Vec U;
                if (k < levels - 1 && !pyr.have[k + 1]) throw std::logic_error("missing pyramid level");
                if (k < levels - 1)
                    U = up2(pyr.band[k + 1], pyr.sizes[k + 1][0], pyr.sizes[k + 1][1], pyr.sizes[k][2], pyr.sizes[k][3]);
                else
                    U.assign(n, 0.0f);
                U = vadd(band, U);
                Vec V;
                if (k == 0) {
                    const Vec bU = box2(U, Wk, Hk, 1, 1);
                    V.resize(n);
                    for (size_t p = 0; p < n; ++p) V[p] = M(A(U[p], bU[p]), C05);
                } else {
                    V = U;
                }
                std::vector<Vec> F(nd), E(nd);
                for (int d = 0; d < nd; ++d) {
                    if (stop()) return CANCELLED;
                    const Vec Fd = dfilt(*K, sd, d, V, r);
                    const Vec Ed = vsqrt_abs(box2(box2(vsq(Fd), Wk, Hk, rs), Wk, Hk, rs));
                    out[(off + 0x140 * s + 0x40 * k + 4 * d) / 4] = q99(crop(Ed, Wk, Hk, bx, by), div);
                    E[d] = Ed;
                    if (fam == 1) F[d] = dfilt(*K, sd, d, U, (r >= 0 ? r : r + 1) >> 1);
                }
                if (fam == 1 && sd.special) {
                    const std::vector<uint8_t> idx = argmax_dirs(E, nd, n);
                    Vec Sel(n);
                    for (size_t p = 0; p < n; ++p) Sel[p] = F[idx[p]][p];
                    const Vec U2 = vadd(band, U);
                    const Vec Mv = blend_add_10(U2, Sel);
                    const int st = ad319c(words, band, Mv, Wk, Hk, k, stop);
                    if (st != OK) return st;
                }
            }
        }
    }
    return OK;
}

}  // namespace nvm
