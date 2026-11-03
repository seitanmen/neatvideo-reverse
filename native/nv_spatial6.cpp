// nv_spatial6 - see nv_spatial6.h.  Every routine names the genuine function (arm64 slice) and the line of
// work/omo/wp_spatial/src/nv_spatial.py it reproduces; operand order follows the disassembly (it decides the NaN
// payloads), A / S / M / D / FMA are single AArch64 instructions (nv_portable).

#include "nv_spatial6.h"

#include "nv_band.h"
#include "nv_linekernel.h"
#include "nv_portable.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <tuple>

namespace nvs6 {

namespace {

using nvp::bits;
using nvp::ubits;
using Vec = std::vector<float>;
using U8 = std::vector<uint8_t>;

inline float A(float a, float b) { return nvp::add(a, b); }
inline float S(float a, float b) { return nvp::sub(a, b); }
inline float M(float a, float b) { return nvp::mul(a, b); }
inline float D(float a, float b) { return nvp::div(a, b); }
// FMLA acc + x * y (FPMulAdd(acc, x, y): NaN order acc, x, y) = FMADD x * y + acc
inline float FMLA(float acc, float x, float y) { return nvp::fma(x, y, acc); }
inline float SQRT(float a) { return nvp::sqrt(a); }
inline float FABS(float a) { return bits(ubits(a) & 0x7fffffffu); }
inline float FNEG(float a) { return bits(ubits(a) ^ 0x80000000u); }
inline size_t n4of(size_t n) { return n & ~static_cast<size_t>(3); }

const float C1 = 1.0f;
const float C_NINTH = bits(0x3de38e39u);     // 1/9 of FUN_009ed014 / FUN_009ed164
const float C_EPS_W = bits(0x38d1b717u);     // 1e-4: 0x98f86c / 0x98fae8 (whitening)
const float C_1E6 = bits(0x358637bdu);       // 1e-6: 0x9915a8
const float C_1P1 = bits(0x3f8ccccdu);       // 1.1: 0x99167c / 0x991ab4
const float C_1P05 = bits(0x3f866666u);      // 1.05: 0x992034
const float C_1E3 = bits(0x3a83126fu);       // 0.001: the detail stage ratio
const float C_1P33 = bits(0x3faa3d71u);      // 1.33: the detail stage
const float C_008 = bits(0x3da3d70au);       // S+0x1e0 (sets 1..4)
const float C_0053 = bits(0x3d591687u);      // S+0x200 (sets 3, 4)
const float C_01 = bits(0x3dcccccdu);        // 0.1 of the cfg+0x28 stage
const float NEG_1E6 = bits(0xc9742400u);
const float C_E5 = bits(0x3727c5acu), C_ME5 = bits(0xb727c5acu);   // FUN_009ee93c: 1e-5, -1e-5
const float C_LO = bits(0x3f7fff58u), C_HI = bits(0x3f800054u);     // 0.99999, 1.00001

Recorder* g_rec = nullptr;

}  // namespace

// Sensitivity mutants for work/omo/wp_spatial6/tools/verify_leaves.py --mutants (`make nv_spatial6_mut MUT=k`); 0 = none
#ifndef NV6_MUT
#define NV6_MUT 0
#endif

// ================================================================== leaves
void recip(float eps, const float* x, float* out, size_t n) {   // nv_spatial.leaf_recip
    const size_t m = n4of(n);
    for (size_t i = 0; i < m; ++i) {
        const float d = A(eps, x[i]);
        const float e = nvp::frecpe(d);
        out[i] = NV6_MUT == 1 ? D(C1, d) : M(e, nvp::frecps(d, e));
    }
    for (size_t i = m; i < n; ++i) out[i] = D(C1, A(x[i], eps));
}

void mul(const float* a, const float* b, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) out[i] = NV6_MUT == 2 ? M(b[i], a[i]) : M(a[i], b[i]);
}

void sqr(const float* x, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) out[i] = NV6_MUT == 3 ? M(FABS(x[i]), FABS(x[i])) : M(x[i], x[i]);
}

void sub(const float* a, const float* b, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) out[i] = NV6_MUT == 4 ? FNEG(S(b[i], a[i])) : S(a[i], b[i]);
}

void add(const float* a, const float* b, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) out[i] = A(a[i], b[i]);
}

void sqrtk(float k, const float* x, float* out, size_t n) {   // NEON k * sqrt|x|, tail sqrt|x| * k
    const size_t m = n4of(n);
    for (size_t i = 0; i < m; ++i) out[i] = NV6_MUT == 5 ? M(SQRT(FABS(x[i])), k) : M(k, SQRT(FABS(x[i])));
    for (size_t i = m; i < n; ++i) out[i] = M(SQRT(FABS(x[i])), k);
}

void sqrtabs(const float* x, float* out, size_t n) {
    for (size_t i = 0; i < n; ++i) out[i] = SQRT(FABS(x[i]));
}

void fabs(const float* x, float* out, size_t n) {   // fabs.4s / fabs: the sign bit cleared (NaN payloads kept)
    for (size_t i = 0; i < n; ++i) out[i] = FABS(x[i]);
}

void rstep(float s, const float* x0, const float* x1, float* out, size_t n) {   // nv_spatial.leaf_rstep
    const float si = D(C1, s);
    const size_t m = n4of(n);
    for (size_t i = 0; i < m; ++i) {
        const float v = M(si, FABS(S(x1[i], x0[i])));    // FABD x1, x0
        const bool big = FABS(v) > 1.0f;
        const float q = M(v, v);
        const float e = nvp::frecpe(q);
        const float r = M(e, nvp::frecps(q, e));
        out[i] = M(x0[i], big ? r : C1);
    }
    for (size_t i = m; i < n; ++i) {
        const float v = M(si, S(x1[i], x0[i]));
        const bool keep = NV6_MUT == 6 ? FABS(v) <= 1.0f : !(FABS(v) > 1.0f);   // fcmp; b.le: also taken when unordered
        const float x = x0[i];
        out[i] = keep ? x : D(x, M(v, v));
    }
}

void robust(float t, const float* a, const float* e, float* out, size_t n) {   // nv_model.robust_weight
    const float inv = D(C1, t);
    const size_t m = n4of(n);
    for (size_t i = 0; i < m; ++i) {
        const float z = M(inv, e[i]);
        const bool big = FABS(z) > 1.0f;
        const float z2 = M(z, z);
        const float est = nvp::frecpe(z2);
        const float w = M(est, nvp::frecps(z2, est));
        out[i] = M(a[i], big ? w : C1);
    }
    for (size_t i = m; i < n; ++i) {
        const float z = M(inv, e[i]);
        const bool le = !(FABS(z) > 1.0f);               // b.le, unordered included
        const float x = a[i];
        if (NV6_MUT == 7) {
            const float z2 = M(z, z), est = nvp::frecpe(z2);
            out[i] = le ? x : M(x, M(est, nvp::frecps(z2, est)));
            continue;
        }
        out[i] = le ? x : D(x, M(z, z));
    }
}

void rinv(float s, const float* x0, const float* x1, float* out, size_t n) {   // nv_spatial.leaf_rinv
    const float si = D(C1, s);
    const size_t m = n4of(n);
    for (size_t i = 0; i < m; ++i) {
        const float v = FABS(M(si, x1[i]));
        const float e = nvp::frecpe(v);
        const float r = M(e, nvp::frecps(v, e));
        out[i] = M(x0[i], v > 1.0f ? r : C1);
    }
    for (size_t i = m; i < n; ++i) {
        const float v = FABS(M(si, x1[i]));
        const float x = x0[i];
        out[i] = v > 1.0f ? D(x, v) : x;
    }
}

void scale(float s, const float* x, float* out, size_t n) {   // NEON s * x, tail x * s
    const size_t m = n4of(n);
    for (size_t i = 0; i < m; ++i) out[i] = M(s, x[i]);
    for (size_t i = m; i < n; ++i) out[i] = NV6_MUT == 8 ? M(s, x[i]) : M(x[i], s);
}

void clampmul(float a, float b, float lo, float hi, const float* x, const float* y, float* out, size_t n) {
    const size_t m = n4of(n);   // NEON FMIN(FMLA(b, x, a), hi) -> FMAX(., lo) -> * y
    for (size_t i = 0; i < m; ++i)
        out[i] = M(nvp::fmax(nvp::fmin(NV6_MUT == 9 ? A(M(x[i], a), b) : FMLA(b, x[i], a), hi), lo), y[i]);
    for (size_t i = m; i < n; ++i) {                     // v = x*a + b; v > hi ? hi : v; v > lo ? v : lo; y * v
        float v = A(M(x[i], a), b);
        v = v > hi ? hi : v;
        v = v > lo ? v : lo;
        out[i] = M(y[i], v);
    }
}

void affine(float a, float b, const float* x, float* out, size_t n) {   // NEON FMLA(b, x, a), tail x*a + b
    const size_t m = n4of(n);
    for (size_t i = 0; i < m; ++i) out[i] = FMLA(b, x[i], a);
    for (size_t i = m; i < n; ++i) out[i] = A(M(x[i], a), b);
}

void acc(float s, const float* a, const float* b, const float* c, float* out, size_t n) {
    const size_t m = n4of(n);   // NEON FMLA(c, s, a*b), tail c + ((a*b)*s)
    for (size_t i = 0; i < m; ++i) out[i] = FMLA(c[i], s, M(a[i], b[i]));
    for (size_t i = m; i < n; ++i) out[i] = A(c[i], M(M(a[i], b[i]), s));
}

void blend(float a, float b, const float* x, const float* y, float* out, size_t n) {
    const size_t m = n4of(n);   // NEON FMLA(a*x, b, y), tail x*a + y*b
    for (size_t i = 0; i < m; ++i) out[i] = FMLA(M(a, x[i]), b, y[i]);
    for (size_t i = m; i < n; ++i) out[i] = A(M(x[i], a), M(y[i], b));
}

void bladd(float c, float d, const float* Av, const float* Bv, float* out, size_t n) {   // nv_model.blend_add
    const size_t m = n4of(n);   // NEON A + FMLA(d, c, B) (0x9ed9b4: fmla v5(d), v2(c), v4(B)), tail (A + B*c) + d
    for (size_t i = 0; i < m; ++i) out[i] = A(Av[i], FMLA(d, c, Bv[i]));
    for (size_t i = m; i < n; ++i) out[i] = A(A(Av[i], M(Bv[i], c)), d);
}

void ratio(float eps, const float* a, const float* b, float* out, size_t n) {   // nv_pixnoise.ratio_eps
    const size_t m = n4of(n);
    for (size_t i = 0; i < m; ++i) {
        const float d = A(eps, b[i]);
        const float est = nvp::frecpe(d);
        out[i] = M(a[i], M(est, nvp::frecps(d, est)));
    }
    for (size_t i = m; i < n; ++i) out[i] = D(a[i], A(b[i], eps));
}

namespace {

// FUN_009ee93c / FUN_009eebf8 head (0x9ee93c..0x9ee9b4): coefficients and clamp bounds
struct RampK { float slope, ic, lo, hi; };
RampK ramp_k(float a, float b, float c, float d) {
    const float s4 = S(b, a), s6 = M(b, C_E5);
    const float s7 = A(M(a, C_LO), C_ME5);
    const float s5 = A(M(b, C_HI), C_E5);
    const bool keep = !(s4 <= s6);                        // fcsel hi: also taken when unordered
    const float b2 = keep ? b : s5, a2 = keep ? a : s7;
    const float slope = D(S(d, c), S(b2, a2));
    const float ic = S(c, M(a2, slope));
    const bool le = !(d > c);                             // fcmp s3, s2; b.le (unordered included)
    return le ? RampK{slope, ic, d, c} : RampK{slope, ic, c, d};
}

inline float ramp_neon(const RampK& k, float x) { return nvp::fmax(nvp::fmin(FMLA(k.ic, x, k.slope), k.hi), k.lo); }
inline float ramp_tail(const RampK& k, float x) {
    float v = A(k.ic, M(k.slope, x));
    v = v > k.hi ? k.hi : v;
    return v > k.lo ? v : k.lo;
}

}  // namespace

void ramp(float a, float b, float c, float d, const float* x, float* out, size_t n) {
    const RampK k = ramp_k(a, b, c, d);
    const size_t m = n4of(n);
    for (size_t i = 0; i < m; ++i) out[i] = ramp_neon(k, x[i]);
    for (size_t i = m; i < n; ++i) out[i] = ramp_tail(k, x[i]);
}

void wmul(float a, float b, float c, float d, const float* r, const float* prev, float* out, size_t n) {
    const RampK k = ramp_k(a, b, c, d);   // NEON ramp * prev, tail prev * ramp
    const size_t m = n4of(n);
    for (size_t i = 0; i < m; ++i) out[i] = M(ramp_neon(k, r[i]), prev[i]);
    for (size_t i = m; i < n; ++i) out[i] = M(prev[i], ramp_tail(k, r[i]));
}

void pairw(const float* a, const float* b, const float* c, float* out, size_t n) {   // c * (max(a, b) - b)
    const size_t m = n4of(n);
    for (size_t i = 0; i < m; ++i) out[i] = M(c[i], S(nvp::fmax(a[i], b[i]), b[i]));
    for (size_t i = m; i < n; ++i) out[i] = M(c[i], S(a[i] > b[i] ? a[i] : b[i], b[i]));
}

void argmax(const float* maps, uint8_t* idx, float* mx, size_t n, size_t stride, int count) {
    for (size_t i = 0; i < n; ++i) {   // NEON fcmge x, best; tail fcsel lt (unordered keeps best): the same.  best
                                       // starts at 0.0, x >= best updates, so a tie goes to the later map
        float best = 0.0f;
        uint8_t j0 = 0;
        for (int j = 0; j < count; ++j) {
            const float x = maps[static_cast<size_t>(j) * stride + i];
            if (x >= best) best = x, j0 = static_cast<uint8_t>(j);
        }
        mx[i] = best;
        idx[i] = j0;
    }
}

void gather(const uint8_t* idx, const float* maps, float* out, size_t stride, size_t n) {
    for (size_t i = 0; i < n; ++i) std::memcpy(out + i, maps + static_cast<size_t>(idx[i]) * stride + i, 4);
}

void sel2(const float* a, float* b, const uint8_t* i3, uint8_t* i4, float* out, float* best, const float* src,
          const float* wt, int flag, size_t stride, size_t n) {   // nv_spatial.leaf_sel2
    const size_t m = n4of(n);
    for (size_t i = 0; i < n; ++i) {
        const float v = i < m ? FMLA(C1, b[i], a[i]) : A(M(a[i], b[i]), C1);
        const size_t k4 = static_cast<size_t>(i4[i]) * stride + i, k3 = static_cast<size_t>(i3[i]) * stride + i;
        const float f = M(wt[k4], v);
        if (f > best[i]) {
            best[i] = f;
            out[i] = src[k4];
        } else {
            out[i] = src[k3];
        }
        if (flag & 1) b[i] = a[i], i4[i] = i3[i];
    }
}

void strip_box3(const float* x, int W, int H, int mode, float* out) {   // nv_spatial.strip_box3 (W >= 4, H >= 2)
    const size_t w = static_cast<size_t>(W);
    Vec h(w * H);
    for (int r = 0; r < H; ++r) {
        const float* p = x + r * w;
        float* q = h.data() + r * w;
        for (int c = 0; c < W; ++c) {
            const float L = c > 0 ? p[c - 1] : (mode == 0 ? p[0] : 0.0f);
            const float R = c < W - 1 ? p[c + 1] : (mode == 0 ? p[W - 1] : 0.0f);
            q[c] = A(A(p[c], L), R);
        }
    }
    for (int c = 0; c < W; ++c) {
        const float e0 = mode == 0 ? h[c] : 0.0f, el = mode == 0 ? h[(H - 1) * w + c] : 0.0f;
        out[c] = M(A(A(h[c], e0), h[w + c]), C_NINTH);
        for (int r = 1; r < H - 1; ++r)
            out[r * w + c] = M(A(A(h[r * w + c], h[(r - 1) * w + c]), h[(r + 1) * w + c]), C_NINTH);
        out[(H - 1) * w + c] = M(A(el, A(h[(H - 1) * w + c], h[(H - 2) * w + c])), C_NINTH);
    }
}

void strip_rms3(const float* x, int W, int H, int mode, float s, float* out) {   // nv_spatial.strip_rms3
    const size_t w = static_cast<size_t>(W);
    const float k = M(M(s, C_NINTH), s);
    Vec h(w * H);
    for (int r = 0; r < H; ++r) {
        const float* p = x + r * w;
        float* q = h.data() + r * w;
        for (int c = 0; c < W; ++c) {
            const float L = c > 0 ? p[c - 1] : (mode == 0 ? p[0] : 0.0f);
            const float R = c < W - 1 ? p[c + 1] : (mode == 0 ? p[W - 1] : 0.0f);
            q[c] = A(A(M(p[c], p[c]), M(L, L)), M(R, R));
        }
    }
    for (int c = 0; c < W; ++c) {
        const float e0 = mode == 0 ? h[c] : 0.0f, el = mode == 0 ? h[(H - 1) * w + c] : 0.0f;
        out[c] = SQRT(M(A(A(h[c], e0), h[w + c]), k));
        for (int r = 1; r < H - 1; ++r)
            out[r * w + c] = SQRT(M(k, A(A(h[r * w + c], h[(r - 1) * w + c]), h[(r + 1) * w + c])));
        out[(H - 1) * w + c] = SQRT(M(k, A(el, A(h[(H - 1) * w + c], h[(H - 2) * w + c]))));
    }
}

void down2(const float* x, int W, int H, float* out) {   // nv_spatial.halve / down2_row
    const int h2 = H >> 1, w2 = W >> 1, w8 = W & ~7;
    const float q = 0.25f;
    for (int y = 0; y < h2; ++y) {
        const float* t = x + static_cast<size_t>(2 * y) * W;
        const float* b = t + W;
        float* o = out + static_cast<size_t>(y) * w2;
        for (int j = 0; j < w2; ++j) {
            const int c = 2 * j;
            o[j] = c < w8 ? M(A(A(t[c], t[c + 1]), A(b[c], b[c + 1])), q) : M(A(A(A(t[c], t[c + 1]), b[c]), b[c + 1]), q);
        }
    }
}

// ================================================================== recorder
Recorder::Recorder(const std::string& d) : dir(d) {
    index = std::fopen((dir + "/calls.txt").c_str(), "w");
    if (!index) throw std::runtime_error("recorder: cannot open " + dir + "/calls.txt");
}
Recorder::~Recorder() {
    if (index) std::fclose(index);
}
void set_recorder(Recorder* r) { g_rec = r; }

namespace {

struct Arr { const char* tag; const void* p; size_t bytes; };

void log_call(const std::string& head, const std::vector<Arr>& arrs) {
    if (!g_rec) return;
    const long id = g_rec->next++;
    std::fprintf(g_rec->index, "%ld %s", id, head.c_str());
    for (const Arr& a : arrs) {
        std::fprintf(g_rec->index, " %s", a.tag);
        const std::string nm = g_rec->dir + "/" + std::to_string(id) + "." + a.tag + ".bin";
        FILE* f = std::fopen(nm.c_str(), "wb");
        if (!f) throw std::runtime_error("recorder: cannot write " + nm);
        if (a.bytes) std::fwrite(a.p, 1, a.bytes, f);
        std::fclose(f);
    }
    std::fprintf(g_rec->index, "\n");
}

std::string hx(float v) {
    char b[16];
    std::snprintf(b, sizeof b, "%08x", ubits(v));
    return b;
}
std::string elem_head(const char* nm, size_t n, float s0 = 0, float s1 = 0, float s2 = 0, float s3 = 0) {
    return std::string("elem ") + nm + " " + std::to_string(n) + " " + hx(s0) + " " + hx(s1) + " " + hx(s2) + " " + hx(s3);
}
Arr arr(const char* tag, const Vec& v) { return {tag, v.data(), v.size() * 4}; }
Arr arr(const char* tag, const U8& v) { return {tag, v.data(), v.size()}; }

// ---- recorded wrappers (inputs by value semantics: new output vectors) -----------------------------------------
Vec Recip(float eps, const Vec& x) { Vec o(x.size()); recip(eps, x.data(), o.data(), x.size()); log_call(elem_head("recip", x.size(), eps), {arr("in0", x), arr("out", o)}); return o; }
Vec Mul(const Vec& a, const Vec& b) { Vec o(a.size()); mul(a.data(), b.data(), o.data(), a.size()); log_call(elem_head("mul", a.size()), {arr("in0", a), arr("in1", b), arr("out", o)}); return o; }
Vec Sqr(const Vec& x) { Vec o(x.size()); sqr(x.data(), o.data(), x.size()); log_call(elem_head("sqr", x.size()), {arr("in0", x), arr("out", o)}); return o; }
Vec Sub(const Vec& a, const Vec& b) { Vec o(a.size()); sub(a.data(), b.data(), o.data(), a.size()); log_call(elem_head("sub", a.size()), {arr("in0", a), arr("in1", b), arr("out", o)}); return o; }
Vec Add(const Vec& a, const Vec& b) { Vec o(a.size()); add(a.data(), b.data(), o.data(), a.size()); log_call(elem_head("add", a.size()), {arr("in0", a), arr("in1", b), arr("out", o)}); return o; }
Vec Sqrtk(float k, const Vec& x) { Vec o(x.size()); sqrtk(k, x.data(), o.data(), x.size()); log_call(elem_head("sqrtk", x.size(), k), {arr("in0", x), arr("out", o)}); return o; }
Vec Fabs(const Vec& x) { Vec o(x.size()); fabs(x.data(), o.data(), x.size()); log_call(elem_head("fabs", x.size()), {arr("in0", x), arr("out", o)}); return o; }
Vec Sqrtabs(const Vec& x) { Vec o(x.size()); sqrtabs(x.data(), o.data(), x.size()); log_call(elem_head("sqrtabs", x.size()), {arr("in0", x), arr("out", o)}); return o; }
Vec Rstep(float s, const Vec& x0, const Vec& x1) { Vec o(x0.size()); rstep(s, x0.data(), x1.data(), o.data(), x0.size()); log_call(elem_head("rstep", x0.size(), s), {arr("in0", x0), arr("in1", x1), arr("out", o)}); return o; }
Vec Robust(float t, const Vec& a, const Vec& e) { Vec o(a.size()); robust(t, a.data(), e.data(), o.data(), a.size()); log_call(elem_head("robust", a.size(), t), {arr("in0", a), arr("in1", e), arr("out", o)}); return o; }
Vec Rinv(float s, const Vec& x0, const Vec& x1) { Vec o(x0.size()); rinv(s, x0.data(), x1.data(), o.data(), x0.size()); log_call(elem_head("rinv", x0.size(), s), {arr("in0", x0), arr("in1", x1), arr("out", o)}); return o; }
Vec Scale(float s, const Vec& x) { Vec o(x.size()); scale(s, x.data(), o.data(), x.size()); log_call(elem_head("scale", x.size(), s), {arr("in0", x), arr("out", o)}); return o; }
Vec Clampmul(float a, float b, float lo, float hi, const Vec& x, const Vec& y) { Vec o(x.size()); clampmul(a, b, lo, hi, x.data(), y.data(), o.data(), x.size()); log_call(elem_head("clampmul", x.size(), a, b, lo, hi), {arr("in0", x), arr("in1", y), arr("out", o)}); return o; }
Vec Affine(float a, float b, const Vec& x) { Vec o(x.size()); affine(a, b, x.data(), o.data(), x.size()); log_call(elem_head("affine", x.size(), a, b), {arr("in0", x), arr("out", o)}); return o; }
Vec Acc(float s, const Vec& a, const Vec& b, const Vec& c) { Vec o(a.size()); acc(s, a.data(), b.data(), c.data(), o.data(), a.size()); log_call(elem_head("acc", a.size(), s), {arr("in0", a), arr("in1", b), arr("in2", c), arr("out", o)}); return o; }
Vec Blend(float a, float b, const Vec& x, const Vec& y) { Vec o(x.size()); blend(a, b, x.data(), y.data(), o.data(), x.size()); log_call(elem_head("blend", x.size(), a, b), {arr("in0", x), arr("in1", y), arr("out", o)}); return o; }
Vec Bladd(float c, float d, const Vec& Av, const Vec& Bv) { Vec o(Av.size()); bladd(c, d, Av.data(), Bv.data(), o.data(), Av.size()); log_call(elem_head("bladd", Av.size(), c, d), {arr("in0", Av), arr("in1", Bv), arr("out", o)}); return o; }
Vec Ratio(float eps, const Vec& a, const Vec& b) { Vec o(a.size()); ratio(eps, a.data(), b.data(), o.data(), a.size()); log_call(elem_head("ratio", a.size(), eps), {arr("in0", a), arr("in1", b), arr("out", o)}); return o; }
Vec Ramp(float a, float b, float c, float d, const Vec& x) { Vec o(x.size()); ramp(a, b, c, d, x.data(), o.data(), x.size()); log_call(elem_head("ramp4", x.size(), a, b, c, d), {arr("in0", x), arr("out", o)}); return o; }
Vec Wmul(float a, float b, float c, float d, const Vec& r, const Vec& p) { Vec o(r.size()); wmul(a, b, c, d, r.data(), p.data(), o.data(), r.size()); log_call(elem_head("wmul4", r.size(), a, b, c, d), {arr("in0", r), arr("in1", p), arr("out", o)}); return o; }
Vec Pairw(const Vec& a, const Vec& b, const Vec& c) { Vec o(a.size()); pairw(a.data(), b.data(), c.data(), o.data(), a.size()); log_call("pairw " + std::to_string(a.size()), {arr("in0", a), arr("in1", b), arr("in2", c), arr("out", o)}); return o; }

std::string wh(int W, int H) { return std::to_string(W) + " " + std::to_string(H); }

// nv_lines.box_rows / box_cols (FUN_0099ba54 / FUN_0099bf40, the separable path; FUN_009944c0's axis-aligned kernels)
Vec BoxRows(const Vec& x, int W, int H, int r, int mode) {
    Vec o(x.size());
    nvb::box_rows(x.data(), W, H, r, mode, o.data());
    log_call("linefilt " + wh(W, H) + " 00000000 " + std::to_string(r) + " " + std::to_string(mode), {arr("in0", x), arr("out", o)});
    return o;
}
Vec BoxCols(const Vec& x, int W, int H, int r, int mode) {
    Vec o(x.size());
    nvb::box_cols(x.data(), W, H, r, mode, o.data());
    log_call("linefilt " + wh(W, H) + " 3fc90fdb " + std::to_string(r) + " " + std::to_string(mode), {arr("in0", x), arr("out", o)});
    return o;
}
Vec Box2(const Vec& x, int W, int H, int r, int mode) { return BoxCols(BoxRows(x, W, H, r, mode), W, H, r, mode); }

// FUN_009ed014 strips (0x98f378 / 0x991900), or the separable fallback when W < 4 or H < 2 (radius 1)
Vec Box3(const Vec& x, int W, int H, int mode) {
    if (W < 4 || H < 2) return Box2(x, W, H, 1, mode);
    Vec o(x.size());
    strip_box3(x.data(), W, H, mode, o.data());
    log_call("strip3 " + wh(W, H) + " box " + std::to_string(mode) + " 00000000", {arr("in0", x), arr("out", o)});
    return o;
}
// FUN_009950f4(s, ..., r = 1, mode): FUN_009ed164 strips, or x^2 -> rows -> columns -> FUN_009ed4e0(s)
Vec Rms3(const Vec& x, int W, int H, int mode, float s) {
    if (W < 4 || H < 2) return Sqrtk(s, Box2(Sqr(x), W, H, 1, mode));
    Vec o(x.size());
    strip_rms3(x.data(), W, H, mode, s, o.data());
    log_call("strip3 " + wh(W, H) + " sq " + std::to_string(mode) + " " + hx(s), {arr("in0", x), arr("out", o)});
    return o;
}

Vec Down2(const Vec& x, int W, int H) {
    Vec o(static_cast<size_t>(W >> 1) * (H >> 1));
    down2(x.data(), W, H, o.data());
    log_call("down2 " + wh(W, H), {arr("in0", x), arr("out", o)});
    return o;
}

// ---- the line kernels (nv_linekernel) and the general line filter (nv_model.general_filter, mode 1) -------------
struct LineKernel : nvl::LineKernel, nvl::LinePlan { float angle = 0; };

const LineKernel& kernel(int W, int H, uint32_t abits) {
    static std::mutex mu;
    static std::map<std::tuple<int, int, uint32_t>, std::unique_ptr<LineKernel>> cache;
    std::lock_guard<std::mutex> lock(mu);
    auto& slot = cache[std::make_tuple(W, H, abits)];
    if (!slot) {
        auto k = std::make_unique<LineKernel>();
        static_cast<nvl::LineKernel&>(*k) = nvl::make_kernel(W, H, bits(abits));
        if (!k->horiz && !k->vert) static_cast<nvl::LinePlan&>(*k) = nvl::make_plan(*k);
        k->angle = bits(abits);
        slot = std::move(k);
    }
    return *slot;
}

void general_filter(const LineKernel& K, const float* X, float* out, int r) {   // nv_model.cpp general_filter
    const float recip_ = 1.0f / static_cast<float>(2 * r + 1);
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
    for (int q = 0; q < HW; ++q) out[q] = M(recip_, Sh[K.inv[q]]);
}

// FUN_009944c0(kernel, src, dst, r, mode 1) (nv_lines.linefilt)
Vec Line(const LineKernel& K, const Vec& X, int r) {
    Vec o(X.size());
    if (r == 0) o = X;
    else if (K.horiz) nvb::box_rows(X.data(), K.W, K.H, r, 1, o.data());
    else if (K.vert) nvb::box_cols(X.data(), K.W, K.H, r, 1, o.data());
    else general_filter(K, X.data(), o.data(), r);
    log_call("linefilt " + wh(K.W, K.H) + " " + hx(K.angle) + " " + std::to_string(r) + " 1", {arr("in0", X), arr("out", o)});
    return o;
}

// ---- the set table 0x1243700 + 0x260 * set (S = +0x280; static __TEXT __const data, work/omo/wp_spatial6/REPORT.md)
const uint32_t ANG16[16] = {0x0, 0x3e7adbb0, 0x3eed6338, 0x3f24bc7d, 0x3f490fdb, 0x3f6d6338, 0x3f8db70d,
                            0x3fa9b465, 0x3fc90fdb, 0x3fe86b51, 0x40023454, 0x400db70d, 0x4016cbe4,
                            0x401fe0bb, 0x402b6374, 0x40396220};
const uint32_t ANG8[8] = {0x0, 0x3fc90fdb, 0x3f490fdb, 0x4016cbe4, 0x3eed6338, 0x40023454, 0x3f8db70d, 0x402b6374};
const int IDX16[16] = {0, 0, 0, 4, 4, 4, 8, 8, 8, 8, 8, 12, 12, 12, 0, 0};
const int RAD16[16] = {2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 2};
const int IDX8[8] = {-1, -1, -1, -1, 0, 1, 1, 0};

struct SetDef {
    int flag;            // S+0 (byte)
    float f984;          // S+4
    int nd;              // S+0x258
    const uint32_t* ang; // S+8
    int idx2[16], rad2[16];   // S+0xc8 / S+0x148
    int rA, smooth, rB;  // S+0x1c8 / S+0x1d0 / S+0x1e8
    int rdc, dc_twice;   // 0x12438f8[set] / 0x1243920[set]
    int margin;          // 0x12438d0[set]
};

SetDef set_def(int s) {
    SetDef d{};
    for (int i = 0; i < 16; ++i) d.idx2[i] = -1, d.rad2[i] = 1;
    d.f984 = s >= 3 ? 0.5f : 1.0f;
    const int margins[5] = {2, 4, 4, 8, 32};
    d.margin = margins[s];
    switch (s) {
        case 0: break;
        case 1: d.nd = 4; d.ang = ANG8; d.rA = 4; d.smooth = 1; break;
        case 2:
            d.nd = 8; d.ang = ANG8; d.rA = 4; d.smooth = 1; d.rdc = 4;
            for (int i = 0; i < 8; ++i) d.idx2[i] = IDX8[i];
            break;
        default:
            d.flag = 1; d.nd = 16; d.ang = ANG16; d.rA = 4; d.smooth = 1; d.rB = 6; d.rdc = 4; d.dc_twice = 1;
            for (int i = 0; i < 16; ++i) d.idx2[i] = IDX16[i], d.rad2[i] = RAD16[i];
            break;
    }
    return d;
}

// nv_model.dfilt: radius r along kernel d, then rad2[d] along kernel idx2[d]
Vec Dfilt(const SetDef& sd, int W, int H, int d, const Vec& X, int r) {
    Vec f = Line(kernel(W, H, sd.ang[d]), X, r);
    const int j = sd.idx2[d];
    if (j != -1) f = Line(kernel(W, H, sd.ang[j]), f, sd.rad2[d]);
    return f;
}

struct NotImpl {};

// nv_pixnoise.up2 (FUN_00993df0): (Hc, Wc) -> (2Hc + oddH, 2Wc + oddW)  (nv_model.cpp up2)
Vec Up2(const Vec& lo, int Wc, int Hc, int oddW, int oddH) {
    const float C3 = 3.0f, C4 = 4.0f, C0_0625 = 0.0625f, C075 = 0.75f, C025 = 0.25f;
    const int W2 = 2 * Wc + oddW, H2 = 2 * Hc + oddH;
    Vec out(static_cast<size_t>(W2) * H2);
    if (Hc < 2 || Wc < 4) {
        if (Wc < 2) throw NotImpl{};
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
    } else {
        const int w2 = 2 * Wc;
        Vec h(static_cast<size_t>(w2) * Hc);
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

// FUN_00990da8 (nv_band.band_from): src - up2(lo)
Vec BandFrom(const Vec& lo, int Wc, int Hc, const Vec& src, int W2, int H2) {
    if ((Hc < 2 || Wc < 4) && Wc < 2) throw NotImpl{};
    Vec o(src.size());
    nvb::band_from(lo.data(), Wc, Hc, src.data(), W2, H2, o.data());
    return o;
}

// ================================================================== FUN_00990f74
struct Plane { Vec v; int w, h; };

// cfg+0x90 != 0: the detail stage 0x99185c..0x992608 (nv_spatial._detail)
Vec detail(const Vec& Wf, const Vec& X, const Vec& Rf, const Vec& dst, float c90, const Cfg& cfg, int k, int h, int w) {
    const int nb = cfg.ca4[k];
    auto boxn = [&](const Vec& x) {
        Vec y = Box3(x, w, h, 1);
        if (nb >= 1) y = Box3(y, w, h, 1);
        if (nb >= 2) y = Box3(Box3(y, w, h, 1), w, h, 1);
        return y;
    };
    const Vec Dv = Sub(Wf, X);
    const Vec Bx = boxn(Dv);
    Vec G = Sub(Dv, Bx);
    const Vec Ab = Fabs(Bx), Ad = Fabs(Dv);                                       // FUN_009ee50c 0x992214 / 0x992264
    const Vec Q = boxn(Ad);
    const Vec r = Ratio(C_1E3, Q, Ab);
    G = Wmul(1.0f, 2.0f, 0.0f, 1.0f, r, G);
    if (cfg.c8c) {
        const Vec q = Ratio(C_EPS_W, G, Rf);
        const float sc = D(C_1P33, A(c90, C1));
        G = Rinv(sc, G, q);
    }
    return Bladd(c90, 0.0f, dst, G);
}

// B family (0x992cac..0x993284, set 3) (nv_spatial._family_b)
Vec family_b(const Vec& X, Vec Y, float t2, float c3c, const Cfg& cfg, const float* model, int k, const SetDef& sd,
             int h, int w) {
    const int sid = cfg.set;
    const size_t n = static_cast<size_t>(h) * w;
    const int rB = sd.rB, rBh = rB >> 1, nd = sd.nd;
    if (k == 0) Y = Blend(0.5f, 0.5f, Y, Box3(Y, w, h, 1));
    Vec E(static_cast<size_t>(nd) * n), Fd(static_cast<size_t>(nd) * n);
    for (int d = 0; d < nd; ++d) {
        const Vec F = Dfilt(sd, w, h, d, Y, rB);
        const Vec q = Box2(Box2(Sqr(F), w, h, 2, 1), w, h, 2, 1);
        const float Bw = model[505 + 80 * sid + 16 * k + d];
        const Vec e = Sqrtk(D(C1, FABS(Bw)), q);
        std::copy(e.begin(), e.end(), E.begin() + static_cast<long>(d * n));
        Vec F2 = Dfilt(sd, w, h, d, X, rB);
        F2 = Add(F2, Dfilt(sd, w, h, d, Sub(X, F2), rBh));
        std::copy(F2.begin(), F2.end(), Fd.begin() + static_cast<long>(d * n));
    }
    U8 idx(n);
    Vec bestB(n);
    argmax(E.data(), idx.data(), bestB.data(), n, n, nd);
    log_call("argmax " + std::to_string(n) + " " + std::to_string(nd) + " " + std::to_string(n), {arr("in0", E), arr("out", idx), arr("out1", bestB)});
    Vec SB(n);
    gather(idx.data(), Fd.data(), SB.data(), n, n);
    log_call("gather " + std::to_string(n) + " " + std::to_string(n), {arr("in0", idx), arr("in1", Fd), arr("out", SB)});
    const float tm = M(t2, C1);
    Vec mB = Ramp(tm, A(tm, tm), 0.0f, 1.0f, bestB);
    if (c3c > 0) {
        const Vec sm = Box2(mB, w, h, 12, 1);
        const float a7 = M(M(C_0053, c3c), 0.5f);
        mB = Wmul(a7, A(a7, a7), 0.0f, 1.0f, sm, mB);
    }
    return Acc(-1.0f, mB, SB, X);
}

// directional path 0x991aac..0x993d18 (sets 1..3) (nv_spatial._directional): X, and B / I / A18 updated in place
Vec directional(const Vec& Wf, const Vec& Rf, float t, float s13, const Cfg& cfg, const float* model, int k,
                Vec& Bb, U8& Ib, Vec& A18, int h, int w) {
    const int sid = cfg.set;
    const SetDef sd = set_def(sid);
    if (sid >= 4) throw NotImpl{};
    const size_t n = static_cast<size_t>(h) * w;
    const float c28 = cfg.c28[k], c3c = cfg.c3c[k], c78 = cfg.c78[k];
    const Vec Wc = Rinv(M(t, 4.5f), Wf, Rf);                                       // 0x991ac0..0x991adc
    const int nd = sd.nd, rA = sd.rA;
    Vec E(static_cast<size_t>(nd) * n), Fd(static_cast<size_t>(nd) * n);
    for (int d = 0; d < nd; ++d) {
        const float A_d = model[80 + 80 * sid + 16 * k + d];
        const Vec f = Line(kernel(w, h, sd.ang[d]), Wc, rA);                        // 0x991bb8
        Vec F, e;
        if (!sd.flag) {
            const Vec g = Rstep(t, Wc, f);                                         // 0x991c34
            F = Dfilt(sd, w, h, d, g, rA);
            e = Rms3(F, w, h, 1, D(C1, FABS(A_d)));                                // 0x991f28..0x991f5c
        } else {
            const Vec Rr = Rms3(Sub(Wc, f), w, h, 1, C1);                          // 0x991bcc..0x991c04
            const Vec g = Robust(M(t, 0.75f), Wc, Rr);                              // 0x991c1c
            F = Dfilt(sd, w, h, d, g, rA);
            const Vec q = Box3(Box3(Sqr(F), w, h, 1), w, h, 1);                     // 0x991cc4..0x991e6c (radius S+0x1d0 = 1)
            e = Sqrtk(D(C1, FABS(A_d)), q);                                        // 0x991e88..0x991ea0
            const Vec D2 = Sub(g, F);                                              // 0x991eb4
            F = Add(F, Dfilt(sd, w, h, d, D2, rA));                                // 0x991b70
        }
        std::copy(e.begin(), e.end(), E.begin() + static_cast<long>(d * n));
        std::copy(F.begin(), F.end(), Fd.begin() + static_cast<long>(d * n));
    }
    U8 idx(n);
    Vec best(n);
    argmax(E.data(), idx.data(), best.data(), n, n, nd);                            // 0x992000
    log_call("argmax " + std::to_string(n) + " " + std::to_string(nd) + " " + std::to_string(n), {arr("in0", E), arr("out", idx), arr("out1", best)});
    const float t1 = M(s13, C_1P05);
    Vec Sv(n);
    if (sd.f984 > 0) {                                                              // 0x992020..
        const Vec wv = Ramp(t1, A(t1, t1), 1.0f, 0.0f, best);
        const Vec a_in = wv, b_in = Bb, best_in = best;
        const U8 i4_in = Ib;
        sel2(wv.data(), Bb.data(), idx.data(), Ib.data(), Sv.data(), best.data(), Fd.data(), E.data(), k != 0, n, n);
        log_call("sel2 " + std::to_string(n) + " " + std::to_string(k != 0) + " " + std::to_string(n),
                 {arr("in0", a_in), arr("in1", b_in), arr("in2", idx), arr("in3", i4_in), arr("in4", best_in),
                  arr("in5", Fd), arr("in6", E), arr("out", Sv), arr("out1", best), arr("out2", Bb), arr("out3", Ib)});
    } else {
        gather(idx.data(), Fd.data(), Sv.data(), n, n);                             // FUN_0098ed34
    }
    const int rB = sd.rB;
    const float bword = rB ? model[505 + 80 * sid + 16 * k] : NEG_1E6;
    const bool bvalid = rB && bword != NEG_1E6 && 2 * rB < w && 2 * rB < h;        // 0x99145c..0x99148c
    float s0 = C1;
    if (bvalid) s0 = A(M(c78, 0.25f), C1);                                          // 0x9926e4..0x992700
    const float t2 = M(s13, C_1P05);
    const float tm = M(t2, s0);
    Vec m = Ramp(tm, A(tm, tm), 0.0f, 1.0f, best);
    if (c3c > 0) {                                                                  // 0x9927a4..0x992838
        const Vec sm = Box2(m, w, h, 8, 1);
        const float a7 = M(M(C_008, c3c), 0.5f);
        m = Wmul(a7, A(a7, a7), 0.0f, 1.0f, sm, m);
    }
    const Vec Xd = Acc(-1.0f, m, Sv, Wc);                                           // 0x992854
    Vec Mb;
    if (bvalid || sd.flag) {                                                        // 0x992868..0x9928a4
        const float a9 = M(t2, 1.75f);
        Mb = Ramp(a9, A(a9, a9), 0.0f, -1.0f, best);
        Mb = Acc(1.0f, Mb, Sv, Wc);
    }
    const Vec Dv = Sub(Wc, Sv);                                                     // 0x9928b8
    const Vec q = Box3(Box3(Sqr(Xd), w, h, 1), w, h, 1);                            // 0x9928cc..0x992a38
    Vec Rx = Sqrtabs(q);
    if (c78 > 0) Rx = Mul(Rx, Affine(FNEG(c78), C1, m));                            // 0x992a78..0x992ab8
    const float Rw = model[480 + 5 * sid + k];
    const float t4 = M(s13, M(FABS(Rw), C_1P1));
    const Vec wR = Ramp(t4, A(t4, t4), 1.0f, 0.0f, Rx);                             // 0x992b50
    Vec Pc;
    const bool havePc = c28 > 0;
    if (havePc) {                                                                   // 0x992b60..0x992c10
        Vec sw = Box2(wR, w, h, 5, 1);
        const Vec m1 = Affine(-1.0f, C1, m);
        const float s1 = M(c28, C_01);
        const float a8 = S(C1, s1);
        const float b8_ = A(M(s1, -0.5f), C1);
        sw = Wmul(a8, b8_, 0.0f, 1.0f, sw, m1);
        Pc = Pairw(sw, wR, Xd);
    }
    Vec X = Mul(wR, Xd);                                                            // 0x992c24
    Vec Y;
    if (bvalid || sd.flag) {                                                        // 0x992c38..0x992c88
        if (A18.empty()) A18.assign(n, 0.0f);
        Y = Acc(1.0f, wR, Mb, A18);
        A18 = Robust(M(t2, 1.75f), Sv, best);
    }
    if (bvalid) X = family_b(X, Y, t2, c3c, cfg, model, k, sd, h, w);
    const int rdc = sd.rdc;
    const float dcw = model[55 + 5 * sid + k];
    if (rdc >= 1 && dcw != NEG_1E6 && 2 * rdc < w && 2 * rdc < h) {                // 0x993a10..0x993bbc
        Vec qd = Robust(M(t, 1.25f), Dv, Rf);
        qd = Box2(Sqr(qd), w, h, rdc, 1);
        if (sd.dc_twice) qd = Box2(qd, w, h, rdc, 1);
        const Vec Qd = Sqrtabs(qd);
        const float a6 = M(s13, FABS(dcw));
        X = Wmul(a6, M(a6, 1.5f), 1.0f, 0.0f, Qd, X);
    }
    if (havePc) X = Add(X, Pc);                                                     // 0x993bc0..0x993bdc
    return X;
}

// FUN_00990f74 without the var output (nv_spatial.shrink)
Vec shrink(const Vec& Wb, const Vec& inv, const Cfg& cfg, const float* model, int k, Vec& Bb, U8& Ib, Vec& A18,
           bool useB, int h, int w, const Guard& guard) {
    const SetDef sd = set_def(cfg.set);
    const size_t n = static_cast<size_t>(h) * w;
    const float a_word0 = sd.nd ? model[80 + 80 * cfg.set + 16 * k] : NEG_1E6;
    const bool dir0 = sd.nd && a_word0 != NEG_1E6 && 2 * sd.rA < h && 2 * sd.rA < w;
    if (w >= 0x201 && !(guard.allow_wide && !dir0)) throw NotImpl{};
    const Vec R = Rms3(Wb, w, h, 1, C1);                                            // 0x9915a4
    const float s13 = A(cfg.sigma[k], C_1E6);                                       // 0x991638
    const float t = M(s13, M(FABS(model[5 + k]), C_1P1));                           // 0x991688..0x99168c
    const float a_word = sd.nd ? model[80 + 80 * cfg.set + 16 * k] : NEG_1E6;
    const bool dir = sd.nd && a_word != NEG_1E6 && 2 * sd.rA < h && 2 * sd.rA < w; // 0x991644..0x991670
    Vec X;
    uint64_t x20 = 0;
    if (!dir) {
        X = Wmul(t, A(t, t), 1.0f, 0.0f, R, Wb);                                    // 0x9916a4
        if (useB && sd.f984 > 0) Bb.assign(n, 1.0f);                                // 0x9916a8..0x9916dc
    } else {
        X = directional(Wb, R, t, s13, cfg, model, k, Bb, Ib, A18, h, w);
        x20 = static_cast<uint32_t>(guard.dir_popcount - 16);                      // 0x993cd8 sub w20 (zero-extended)
    }
    const float amt = cfg.amount[k], b8 = cfg.b8;
    Vec dst;
    if (b8 > 0 && amt != 1.0f) {                                                    // 0x99174c..0x991794
        const float a = M(b8, S(C1, amt));
        dst = Clampmul(a, -1.0f, -1.0f, FNEG(amt), inv, X);
    } else {                                                                        // 0x9917a0..0x9917c0
        const float f = static_cast<float>(static_cast<int64_t>(1 - x20));        // 0x9917a4..0x9917ac: sub x8; scvtf
        dst = Scale(M(f, FNEG(amt)), X);
    }
    const float c90 = cfg.c90[k];
    if (c90 != 0) dst = detail(Wb, X, R, dst, c90, cfg, k, h, w);                 // 0x991860..0x991874
    return dst;
}

}  // namespace

// ================================================================== FUN_0098edb0 (nv_spatial.plane_filter)
int plane_filter(const float* P, int W, int H, const std::vector<const float*>& sigma, const Cfg& cfg_in,
                 const float* model, const int* roi, float* out, const Guard& guard, Cfg* cfg_after) {
    if (W < 1 || H < 1 || cfg_in.levels < 1 || cfg_in.levels > 5 || cfg_in.set < 0 || cfg_in.set > 4 ||
        static_cast<int>(sigma.size()) < cfg_in.levels)
        return BAD_ARGUMENT;
    Cfg cfg = cfg_in;
    const int L = cfg.levels;
    const size_t N = static_cast<size_t>(W) * H;
    bool any = cfg.b8 > 0;                                                          // 0x98ee90..0x98eed8
    for (int k = 0; k < L; ++k) any = any || !(cfg.amount[k] == 0) || !(cfg.c90[k] == 0);
    if (!any) {
        std::memcpy(out, P, N * 4);                                                 // 0x98ef04: FUN_009f14e4
        if (cfg_after) *cfg_after = cfg;
        return OK;
    }
    const SetDef sd = set_def(cfg.set);
    const float s10 = sd.f984;
    const bool useB = s10 > 0, useA18 = sd.flag != 0;
    try {
        // level sizes (FUN_0099c3a4) and the decomposition 0x98f184..0x98f368
        int sw[6], sh[6];
        for (int k = 0; k <= L; ++k) sw[k] = W >> k, sh[k] = H >> k;
        std::vector<Vec> bands(L);
        Vec cur(P, P + N);
        for (int k = 0; k < L; ++k) {
            const Vec blur = Box3(cur, sw[k], sh[k], 0);
            if (sh[k] < 2) throw NotImpl{};
            const Vec coarse = Down2(blur, sw[k], sh[k]);
            bands[k] = BandFrom(coarse, sw[k] >> 1, sh[k] >> 1, cur, sw[k], sh[k]);
            cur = coarse;
        }
        // level rects (FUN_0099c694, nv_spatial.level_rects)
        struct Rect { int y0, x0, h, w; bool tiled; };
        std::vector<Rect> rects(L);
        for (int k = 0; k < L; ++k) {
            if (!roi) {
                rects[k] = {0, 0, sh[k], sw[k], false};
                continue;
            }
            const int y = roi[0], x = roi[1], ylast = roi[0] + roi[2] - 1, xlast = roi[1] + roi[3] - 1;
            const int mg = sd.margin;
            int cnt = 0;
            int x0 = (x >> k) - mg;
            cnt += x0 < 1;
            x0 = std::max(x0, 0);
            int x1 = (xlast >> k) + mg;
            if (sw[k] - 1 <= x1) x1 = sw[k] - 1, ++cnt;
            int y0 = (y >> k) - mg;
            cnt += y0 < 1;
            y0 = std::max(y0, 0);
            int y1 = (ylast >> k) + mg;
            if (sh[k] - 1 <= y1) y1 = sh[k] - 1, ++cnt;
            rects[k] = {y0, x0, y1 - y0 + 1, x1 - x0 + 1, cnt < 4};
        }
        // reconstruction 0x98f774..0x9902c4
        int k = L - 1;
        Vec accv(static_cast<size_t>(sw[k]) * sh[k], 0.0f);                       // 0x98f468
        Vec Bf, Af;
        U8 If;
        if (useB) Bf.assign(accv.size(), 1.0f), If.assign(accv.size(), 0);           // 0x98f4c0 / 0x98f4e8
        if (useA18) Af.assign(accv.size(), 0.0f);                                   // 0x98f498
        for (k = L - 1; k >= 0; --k) {
            if (guard.cfg_popcount != 16) cfg.sigma[k] = 0.0f;                     // 0x98f77c..0x98f788 (every level)
            const Rect rc = rects[k];
            const int wk = sw[k], hk = sh[k];
            const float* sg = sigma[k];
            const size_t nr = static_cast<size_t>(rc.h) * rc.w;
            Vec inv(nr), Wb(nr), Bc, Ac;
            U8 Ic;
            if (rc.tiled) {
                if (useB) Bc.resize(nr), Ic.resize(nr);
                if (useA18) Ac.resize(nr);
                for (int r = 0; r < rc.h; ++r) {
                    const size_t off = static_cast<size_t>(rc.y0 + r) * wk + rc.x0;
                    const Vec srow(sg + off, sg + off + rc.w);
                    const Vec ir = Recip(C_EPS_W, srow);                            // 0x98f880
                    const Vec brow(bands[k].begin() + static_cast<long>(off), bands[k].begin() + static_cast<long>(off + rc.w));
                    const Vec wr = Mul(brow, ir);                                   // 0x98f8a4
                    std::copy(ir.begin(), ir.end(), inv.begin() + static_cast<long>(r) * rc.w);
                    std::copy(wr.begin(), wr.end(), Wb.begin() + static_cast<long>(r) * rc.w);
                    if (useB) {
                        const Vec bsrc(Bf.begin() + static_cast<long>(off), Bf.begin() + static_cast<long>(off + rc.w));
                        const Vec br = Affine(FNEG(s10), s10, bsrc);
                        std::copy(br.begin(), br.end(), Bc.begin() + static_cast<long>(r) * rc.w);
                        std::copy(If.begin() + static_cast<long>(off), If.begin() + static_cast<long>(off + rc.w),
                                  Ic.begin() + static_cast<long>(r) * rc.w);
                    }
                    if (useA18)
                        std::copy(Af.begin() + static_cast<long>(off), Af.begin() + static_cast<long>(off + rc.w),
                                  Ac.begin() + static_cast<long>(r) * rc.w);
                }
            } else {
                inv = Recip(C_EPS_W, Vec(sg, sg + nr));                             // 0x98faf4
                Wb = Mul(bands[k], inv);
                if (useB) Bc = Affine(FNEG(s10), s10, Bf), Ic = If;
                if (useA18) Ac = Af;
            }
            const Vec dst = shrink(Wb, inv, cfg, model, k, Bc, Ic, Ac, useB, rc.h, rc.w, guard);
            if (rc.tiled) {
                for (int r = 0; r < rc.h; ++r) {
                    const size_t off = static_cast<size_t>(rc.y0 + r) * wk + rc.x0;
                    const Vec drow(dst.begin() + static_cast<long>(r) * rc.w, dst.begin() + static_cast<long>(r + 1) * rc.w);
                    const Vec srow(sg + off, sg + off + rc.w);
                    const Vec arow(accv.begin() + static_cast<long>(off), accv.begin() + static_cast<long>(off + rc.w));
                    const Vec o = Acc(1.0f, drow, srow, arow);
                    std::copy(o.begin(), o.end(), accv.begin() + static_cast<long>(off));
                }
            } else {
                accv = Acc(1.0f, dst, Vec(sg, sg + nr), accv);                      // 0x98fc68
            }
            if (k) {
                const int oW = sw[k - 1] & 1, oH = sh[k - 1] & 1;
                accv = Up2(accv, wk, hk, oW, oH);                                   // 0x98fcb4..0x98fce8
                auto expand = [&](const auto& c, auto zero) {
                    std::vector<decltype(zero)> full(static_cast<size_t>(wk) * hk, zero);
                    if (!rc.tiled) return std::vector<decltype(zero)>(c.begin(), c.end());
                    for (int r = 0; r < rc.h; ++r)
                        std::copy(c.begin() + static_cast<long>(r) * rc.w, c.begin() + static_cast<long>(r + 1) * rc.w,
                                  full.begin() + static_cast<long>(static_cast<size_t>(rc.y0 + r) * wk + rc.x0));
                    return full;
                };
                if (useA18) Af = Up2(expand(Ac, 0.0f), wk, hk, oW, oH);             // 0x98fd50..0x98fdd0
                if (useB) {
                    Bf = Up2(expand(Bc, 0.0f), wk, hk, oW, oH);
                    const U8 Ik = expand(Ic, static_cast<uint8_t>(0));             // 0x98fe98..0x98ffe4: nearest, bytes
                    const int W2 = 2 * wk + oW, H2 = 2 * hk + oH;
                    If.assign(static_cast<size_t>(W2) * H2, 0);
                    for (int y = 0; y < H2; ++y)
                        for (int x = 0; x < W2; ++x)
                            If[static_cast<size_t>(y) * W2 + x] = Ik[static_cast<size_t>(std::min(y >> 1, hk - 1)) * wk + std::min(x >> 1, wk - 1)];
                }
            }
        }
        const Vec o = Add(Vec(P, P + N), accv);                                     // FUN_009ed844
        std::memcpy(out, o.data(), N * 4);
        if (cfg_after) *cfg_after = cfg;
    } catch (const NotImpl&) {
        return NOT_IMPLEMENTED;
    }
    return OK;
}

}  // namespace nvs6
