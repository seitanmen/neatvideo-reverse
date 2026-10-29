// nv_curve.cpp - see nv_curve.h.  Every function names the Python function it ports; the
// operation order (and operand order, which decides NaN payloads) follows the Python line by line.
#include "nv_curve.h"
#include "nv_libm.h"
#include "nv_linekernel.h"
#include "nv_portable.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>


namespace nvc {

namespace {

using nvp::bits;
using nvp::ubits;

// float32 constants, copied from the Python side's bit patterns
const float F_THIRD = bits(0x3eaaaaabu);      // 1/3 of FUN_009ece90
const float F_0_75 = bits(0x3f400000u);
const float F_0_25 = bits(0x3e800000u);
const float F_0_0625 = bits(0x3d800000u);
const float F_3 = bits(0x40400000u);
const float F_4 = bits(0x40800000u);
const float F_0_8 = bits(0x3f4ccccdu);
const float F_1_2 = bits(0x3f99999au);
const float F_1_1 = bits(0x3f8ccccdu);
const float F_1_5 = bits(0x3fc00000u);
const float F_1EM4 = bits(0x38d1b717u);       // 1e-4 (eps of FUN_009ee04c in FUN_00998d98)
const float F_1EM5 = bits(0x3727c5acu);       // 1e-05
const float F_0_99999 = bits(0x3f7fff58u);
const float F_1_00001 = bits(0x3f800054u);
const float F_CLIP_THR = bits(0x3ea8f5c3u);   // 0.33 (DAT_01244588)
const float F_INV256 = bits(0x3b800000u);     // 0.00390625
const float F_HALF = bits(0x3f000000u);
const float FLT_MAX_ = std::numeric_limits<float>::max();

const uint32_t LUMA_BITS[3] = {0x3e99096cu, 0x3f162b6bu, 0x3dea7efau};  // 0.2989 0.5866 0.1145

// nv_lines.ANGLES (table at 0x12440a8), IDX2 / RAD2 (DAT_01244168 / DAT_012441e8)
const uint32_t ANGLE_BITS[16] = {0x0, 0x3e7adbb0, 0x3eed6338, 0x3f24bc7d, 0x3f490fdb, 0x3f6d6338,
                                 0x3f8db70d, 0x3fa9b465, 0x3fc90fdb, 0x3fe86b51, 0x40023454,
                                 0x400db70d, 0x4016cbe4, 0x401fe0bb, 0x402b6374, 0x40396220};
const int IDX2[16] = {0, 0, 0, 4, 4, 4, 8, 8, 8, 8, 8, 12, 12, 12, 0, 0};
const int RAD2[16] = {2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 2};

// nv_pixnoise.REF_NORM (white-noise reference model, set 3, levels 0 / 1)
const uint32_t REF_BITS[2][16] = {
    {0x413d6846, 0x4105ad92, 0x40f8eb0a, 0x41077463, 0x413b18fe, 0x410ae91e, 0x410006ea, 0x41060937,
     0x413dd31f, 0x4104a534, 0x40fb1afd, 0x41043ba1, 0x413c2996, 0x410622ce, 0x40fe630e, 0x4104b3f3},
    {0x406e220a, 0x4028b230, 0x4024f222, 0x402406d4, 0x403b0d7d, 0x401e9e1b, 0x4026d658, 0x4033a0cf,
     0x406c5975, 0x4030bbf8, 0x4029441c, 0x4026107a, 0x4039f6b7, 0x4026c12e, 0x4023c705, 0x402dacc0}};

// libSystem functions through volatile pointers: the compiler must not constant-fold or rewrite
// them (pow(x, 2.0) -> x*x).
double (*volatile g_pow)(double, double) = NV_LIBM(pow, pow);

// ------------------------------------------------------------------ float helpers (nv_portable)
// float64 operations of the curve stage with the NaN choice of the Python side made explicit (so neither
// the compiler's operand order nor the ISA decides it).  When both operands are NaN:
//   add1 / mul1: the first operand wins (AArch64 FPProcessNaNs(a, b)): CPython float ops, numpy array ops,
//                np.add.at;
//   add2:        the second operand wins: numpy float64 SCALAR + and * (numpy's scalar math evaluates
//                commutative ops as b op a; observed with numpy 2.1.3 / Python 3.13.5, arm64).
// Only quiet NaNs reach this stage (the float32 -> float64 conversion quiets).
inline double add1(double a, double b) { return nvp::add(a, b); }
inline double mul1(double a, double b) { return nvp::mul(a, b); }
inline double add2(double a, double b) { return nvp::add(b, a); }
inline double div1(double a, double b) { return nvp::div(a, b); }
inline double sub1(double a, double b) { return nvp::sub(a, b); }

using nvp::fcvtzs;   // float -> int64 with saturation: NaN -> 0
// FMLA / FMADD: a*b + c with one rounding (FPMulAdd(addend = c, a, b))
inline float fma32(float a, float b, float c) { return nvp::fma(a, b, c); }

// ------------------------------------------------------------------ ordered float32 operations
// clang -O2 (without fast-math) may swap the operands of a commutative fadd / fmul.  AArch64 returns the
// first operand's NaN when both are NaN, so a swap changes the NaN bits whenever two NaNs of different
// payload / sign meet.  Every add / sub / mul of the pixel pipeline therefore goes through FA / FS / FM:
// nv_portable's add / sub / mul, i.e. one FADD / FSUB / FMUL in exactly the operand order of the Python
// (numpy array ops: first operand first) that the compiler cannot commute or fold (x*1.0 stays a
// multiply), or its result written out in the portable build.  F = float, V = 4 lanes (AArch64 build
// only, NVP_ASM); for_lanes runs a lane body over [0, L) in 4-wide chunks and a scalar remainder.
template <class T> inline T FA(T a, T b) { return nvp::add(a, b); }
template <class T> inline T FS(T a, T b) { return nvp::sub(a, b); }
template <class T> inline T FM(T a, T b) { return nvp::mul(a, b); }
#if NVP_ASM
using V = nvp::V4;
inline V LD(const float* p, V) { return nvp::load(p); }
inline void ST(float* p, V x) { nvp::store(p, x); }
inline V DUP(float c, V) { return nvp::dup(c); }
#endif
inline float LD(const float* p, float) { return *p; }
inline void ST(float* p, float x) { *p = x; }
inline float DUP(float c, float) { return c; }

template <class Fn>
inline void for_lanes(int L, Fn&& f) {
    int l = 0;
#if NVP_ASM
    for (; l + 4 <= L; l += 4) f(l, V{});
#endif
    for (; l < L; ++l) f(l, 0.0f);
}

// ------------------------------------------------------------------ planes
struct Plane {
    int W = 0, H = 0;
    std::vector<float> d;
    Plane() = default;
    Plane(int w, int h) : W(w), H(h), d(static_cast<size_t>(w) * h, 0.0f) {}
    float* p() { return d.data(); }
    const float* p() const { return d.data(); }
    size_t n() const { return d.size(); }
};

// ------------------------------------------------------------------ nv_lines
// LineKernel / make_kernel and LinePlan / make_plan: nv_linekernel (shared with nv_model)
using nvl::LineKernel;
using nvl::LinePlan;
using nvl::make_kernel;
using nvl::make_plan;

// nv_lines._box_lanes on L lanes at once: X and O are n x L (lane-contiguous), the running sum of every
// lane goes along the n axis with exactly the per-lane operations of the reference (vectorised over lanes).
void box_lanes(const float* X, float* O, int n, int L, int r, int mode, bool prefix4) {
    const float recip = 1.0f / static_cast<float>(2 * r + 1);
    const size_t Ls = static_cast<size_t>(L);
#define XR(k) (X + static_cast<size_t>(k) * Ls)
#define OR(k) (O + static_cast<size_t>(k) * Ls)
    thread_local std::vector<float> sbuf;
    if (sbuf.size() < Ls) sbuf.resize(Ls);
    float* S = sbuf.data();
    const float* first = XR(0);
    const float* last = XR(n - 1);
    for (int l = 0; l < L; ++l) S[l] = 0.0f;
    auto add_row = [&](const float* x) {
        for_lanes(L, [&](int l, auto t) { ST(S + l, FA(LD(S + l, t), LD(x + l, t))); });
    };
    auto out_row = [&](float* o) {
        for_lanes(L, [&](int l, auto t) { ST(o + l, FM(DUP(recip, t), LD(S + l, t))); });
    };
    if (n <= r + 1) {
        for (int k = 0; k < n; ++k) add_row(XR(k));
        if (mode == 0) {
            const float fr = static_cast<float>(r), fl = static_cast<float>(r + 1 - n);
            for_lanes(L, [&](int l, auto t) {
                const auto a = FM(LD(first + l, t), DUP(fr, t));
                const auto b = FM(LD(last + l, t), DUP(fl, t));
                ST(S + l, FA(LD(S + l, t), FA(a, b)));
            });
            for (int j = 0; j < n; ++j) {
                float* o = OR(j);
                for_lanes(L, [&](int l, auto t) {
                    ST(o + l, FM(DUP(recip, t), LD(S + l, t)));
                    ST(S + l, FS(FA(LD(last + l, t), LD(S + l, t)), LD(first + l, t)));
                });
            }
        } else {
            for (int j = 0; j < n; ++j) out_row(OR(j));
        }
        return;
    }
    for (int k = 0; k < r; ++k) add_row(XR(k));
    if (mode == 0) {
        const float fr1 = static_cast<float>(r + 1);
        for_lanes(L, [&](int l, auto t) { ST(S + l, FA(FM(LD(first + l, t), DUP(fr1, t)), LD(S + l, t))); });
    }
    const int grow_end = std::min(r, n - r - 1);
    for (int j = 0; j <= grow_end; ++j) {
        const float* x = XR(j + r);
        float* o = OR(j);
        if (mode == 0)
            for_lanes(L, [&](int l, auto t) {
                const auto v = FS(FA(LD(S + l, t), LD(x + l, t)), LD(first + l, t));
                ST(S + l, v);
                ST(o + l, FM(DUP(recip, t), v));
            });
        else
            for_lanes(L, [&](int l, auto t) {
                const auto v = FA(LD(S + l, t), LD(x + l, t));
                ST(S + l, v);
                ST(o + l, FM(DUP(recip, t), v));
            });
    }
    if (n <= 2 * r) {
        for (int j = n - r; j <= r; ++j) {
            float* o = OR(j);
            if (mode == 0)
                for_lanes(L, [&](int l, auto t) {
                    const auto v = FS(FA(LD(last + l, t), LD(S + l, t)), LD(first + l, t));
                    ST(S + l, v);
                    ST(o + l, FM(DUP(recip, t), v));
                });
            else
                out_row(o);
        }
    } else {
        int j = r + 1;
        const int M = n - 2 * r - 1;
        auto step = [&](int jj) {
            const float* xi = XR(jj + r);
            const float* xo = XR(jj - r - 1);
            float* o = OR(jj);
            for_lanes(L, [&](int l, auto t) {
                const auto v = FS(FA(LD(S + l, t), LD(xi + l, t)), LD(xo + l, t));
                ST(S + l, v);
                ST(o + l, FM(DUP(recip, t), v));
            });
        };
        if (prefix4) {
            for (int q = 0; q < M % 4; ++q) step(j++);
            while (j <= n - r - 1) {
                const float *i0 = XR(j + r), *i1 = XR(j + r + 1), *i2 = XR(j + r + 2), *i3 = XR(j + r + 3);
                const float *u0 = XR(j - r - 1), *u1 = XR(j - r), *u2 = XR(j - r + 1), *u3 = XR(j - r + 2);
                float *o0 = OR(j), *o1 = OR(j + 1), *o2 = OR(j + 2), *o3 = OR(j + 3);
                for_lanes(L, [&](int l, auto t) {
                    const auto d0 = FS(LD(i0 + l, t), LD(u0 + l, t));
                    const auto d1 = FS(LD(i1 + l, t), LD(u1 + l, t));
                    const auto d2 = FS(LD(i2 + l, t), LD(u2 + l, t));
                    const auto d3 = FS(LD(i3 + l, t), LD(u3 + l, t));
                    const auto p1 = FA(d0, d1);
                    const auto p2 = FA(d2, p1);
                    const auto p3 = FA(p1, FA(d2, d3));
                    const auto sv = LD(S + l, t);
                    const auto rc = DUP(recip, t);
                    ST(o0 + l, FM(rc, FA(sv, d0)));
                    ST(o1 + l, FM(rc, FA(sv, p1)));
                    ST(o2 + l, FM(rc, FA(sv, p2)));
                    ST(o3 + l, FM(rc, FA(sv, p3)));
                    ST(S + l, FA(sv, p3));
                });
                j += 4;
            }
        } else {
            for (j = r + 1; j < n - r; ++j) step(j);
        }
    }
    for (int j = std::max(n - r, r + 1); j < n; ++j) {
        const float* xo = XR(j - r - 1);
        float* o = OR(j);
        if (mode == 0)
            for_lanes(L, [&](int l, auto t) {
                const auto v = FS(FA(LD(last + l, t), LD(S + l, t)), LD(xo + l, t));
                ST(S + l, v);
                ST(o + l, FM(DUP(recip, t), v));
            });
        else
            for_lanes(L, [&](int l, auto t) {
                const auto v = FS(LD(S + l, t), LD(xo + l, t));
                ST(S + l, v);
                ST(o + l, FM(DUP(recip, t), v));
            });
    }
#undef XR
#undef OR
}

void transpose(const float* X, int rows, int cols, float* out) {  // X rows x cols -> out cols x rows
    for (int i = 0; i < rows; ++i)
        for (int j = 0; j < cols; ++j) out[static_cast<size_t>(j) * rows + i] = X[static_cast<size_t>(i) * cols + j];
}

// nv_lines.box_rows (FUN_0099ba54, SIMD build)
void box_rows(const float* X, int W, int H, int r, int mode, float* out) {
    if (W > 2 * r && r == 1 && W >= 8 && W % 4 == 0) {  // _box3_rows (FUN_009ece90): ((x + prev) + next) / 3
        const float pad = 0.0f;
        for (int y = 0; y < H; ++y) {
            const float* x = X + static_cast<size_t>(y) * W;
            float* o = out + static_cast<size_t>(y) * W;
            o[0] = FM(FA(FA(x[0], mode == 0 ? x[0] : pad), x[1]), F_THIRD);
            for_lanes(W - 2, [&](int l, auto t) {
                const int j = l + 1;
                ST(o + j, FM(FA(FA(LD(x + j, t), LD(x + j - 1, t)), LD(x + j + 1, t)), DUP(F_THIRD, t)));
            });
            o[W - 1] = FM(FA(FA(x[W - 1], x[W - 2]), mode == 0 ? x[W - 1] : pad), F_THIRD);
        }
        return;
    }
    const bool prefix4 = W > 2 * r;
    thread_local std::vector<float> t1, t2;
    const size_t N = static_cast<size_t>(W) * H;
    if (t1.size() < N) t1.resize(N);
    if (t2.size() < N) t2.resize(N);
    transpose(X, H, W, t1.data());                     // W x H: lanes = rows
    box_lanes(t1.data(), t2.data(), W, H, r, mode, prefix4);
    transpose(t2.data(), W, H, out);
}

// nv_lines.box_cols (FUN_0099bf40)
void box_cols(const float* X, int W, int H, int r, int mode, float* out) {
    box_lanes(X, out, H, W, r, mode, false);
}

// nv_pixnoise.box2: rows then columns
void box2(const float* X, int W, int H, int r, int mode, float* out, float* tmp) {
    box_rows(X, W, H, r, mode, tmp);
    box_cols(tmp, W, H, r, mode, out);
}

// nv_lines._general, mode 1 (the only mode FUN_00998d98 uses on oriented kernels), vectorised over the
// lines as nv_lines._general_bm: the entries past the end of a line are -0.0, and S + (-0.0) == S bit for
// bit, so they leave every valid output exactly as the per-line running sum of the reference.
void general_filter(const LinePlan& P, const float* X, int HW, float* out, int r) {
    const float recip = 1.0f / static_cast<float>(2 * r + 1);
    const int m = P.m, L = P.L;
    thread_local std::vector<float> xp, A, Sh, S;
    xp.resize(static_cast<size_t>(HW) + 1);
    std::memcpy(xp.data(), X, sizeof(float) * HW);
    xp[HW] = -0.0f;
    const size_t mL = static_cast<size_t>(m) * L;
    if (A.size() < mL) A.resize(mL);
    if (Sh.size() < mL) Sh.resize(mL);
    if (S.size() < static_cast<size_t>(L)) S.resize(L);
    for (size_t q = 0; q < mL; ++q) A[q] = xp[P.G[q]];
    float* s = S.data();
    for (int l = 0; l < L; ++l) s[l] = 0.0f;
    for (int j2 = 0; j2 < std::min(r, m); ++j2) {
        const float* a = A.data() + static_cast<size_t>(j2) * L;
        for_lanes(L, [&](int l, auto t) { ST(s + l, FA(LD(s + l, t), LD(a + l, t))); });
    }
    for (int j = 0; j < m; ++j) {
        const int j2 = j + r, jo = j2 - 2 * r - 1;
        float* h = Sh.data() + static_cast<size_t>(j) * L;
        if (j2 < m) {
            const float* a = A.data() + static_cast<size_t>(j2) * L;
            for_lanes(L, [&](int l, auto t) { ST(s + l, FA(LD(s + l, t), LD(a + l, t))); });
        }
        if (jo >= 0) {
            const float* a = A.data() + static_cast<size_t>(jo) * L;
            for_lanes(L, [&](int l, auto t) { ST(s + l, FS(LD(s + l, t), LD(a + l, t))); });
        }
        std::memcpy(h, s, sizeof(float) * L);
    }
    for (int q = 0; q < HW; ++q) out[q] = FM(recip, Sh[P.inv[q]]);
}

struct KernelSet {
    LineKernel k[16];
    LinePlan p[16];
};

const KernelSet& kernels_for(int W, int H) {
    static std::mutex mu;
    static std::map<std::pair<int, int>, std::unique_ptr<KernelSet>> cache;
    std::lock_guard<std::mutex> lock(mu);
    auto& slot = cache[{W, H}];
    if (!slot) {
        auto ks = std::make_unique<KernelSet>();
        for (int i = 0; i < 16; ++i) {
            ks->k[i] = make_kernel(W, H, bits(ANGLE_BITS[i]));
            if (!ks->k[i].horiz && !ks->k[i].vert) ks->p[i] = make_plan(ks->k[i]);
        }
        slot = std::move(ks);
    }
    return *slot;
}

// nv_lines.linefilt (mode 1)
void linefilt(const LineKernel& K, const LinePlan& P, const float* X, float* out, int r) {
    const int W = K.W, H = K.H;
    if (r == 0) {
        std::memcpy(out, X, sizeof(float) * static_cast<size_t>(W) * H);
        return;
    }
    if (K.horiz) {
        box_rows(X, W, H, r, 1, out);
        return;
    }
    if (K.vert) {
        box_cols(X, W, H, r, 1, out);
        return;
    }
    general_filter(P, X, W * H, out, r);
}

// nv_lines.direction_filter: radius 4 along kernel i, then RAD2[i] along kernel IDX2[i]
void direction_filter(const KernelSet& ks, int i, const float* X, float* out, float* tmp) {
    linefilt(ks.k[i], ks.p[i], X, tmp, 4);
    linefilt(ks.k[IDX2[i]], ks.p[IDX2[i]], tmp, out, RAD2[i]);
}

// ------------------------------------------------------------------ nv_pixnoise pieces
// down2 (FUN_009f15b4 per row pair)
void down2(const float* x, int W, int H, float* out) {
    const int h = H >> 1, w = W >> 1;
    const int c = (W & ~7) >> 1;
    for (int i = 0; i < h; ++i) {
        const float* r0 = x + static_cast<size_t>(2 * i) * W;
        const float* r1 = r0 + W;
        float* o = out + static_cast<size_t>(i) * w;
        for (int j = 0; j < w; ++j) {
            const float x0 = r0[2 * j], x1 = r0[2 * j + 1], y0 = r1[2 * j], y1 = r1[2 * j + 1];
            if (j < c)
                o[j] = FM(FA(FA(x0, x1), FA(y0, y1)), F_0_25);
            else
                o[j] = FM(FA(FA(FA(x0, x1), y0), y1), F_0_25);
        }
    }
}

// _up2_neon: (Hc, Wc) -> (2Hc, 2Wc) written into out with row stride ow
void up2_neon(const float* lo, int Wc, int Hc, float* out, int ow) {
    std::vector<float> h(static_cast<size_t>(Hc) * 2 * Wc);
    for (int i = 0; i < Hc; ++i) {
        const float* l = lo + static_cast<size_t>(i) * Wc;
        float* hr = h.data() + static_cast<size_t>(i) * 2 * Wc;
        for (int j = 0; j < Wc; ++j) {
            const float prev = l[j > 0 ? j - 1 : 0];
            const float nxt = l[j < Wc - 1 ? j + 1 : Wc - 1];
            const float xs = FM(l[j], F_0_75);
            hr[2 * j] = FA(xs, FM(prev, F_0_25));
            hr[2 * j + 1] = FA(xs, FM(nxt, F_0_25));
        }
    }
    const int w2 = 2 * Wc;
    std::memcpy(out, h.data(), sizeof(float) * w2);
    std::memcpy(out + static_cast<size_t>(2 * Hc - 1) * ow, h.data() + static_cast<size_t>(Hc - 1) * w2,
                sizeof(float) * w2);
    for (int i = 1; i < Hc; ++i) {
        const float* hp = h.data() + static_cast<size_t>(i - 1) * w2;
        const float* hn = hp + w2;
        float* oa = out + static_cast<size_t>(2 * i - 1) * ow;
        float* ob = out + static_cast<size_t>(2 * i) * ow;
        for_lanes(w2, [&](int j, auto t) {
            const auto p = LD(hp + j, t), q = LD(hn + j, t);
            ST(oa + j, FA(FM(p, DUP(F_0_75, t)), FM(q, DUP(F_0_25, t))));
            ST(ob + j, FA(FM(p, DUP(F_0_25, t)), FM(q, DUP(F_0_75, t))));
        });
    }
}

// _up2_scalar: (Hc, Wc) -> (2Hc + oddH, 2Wc + oddW)
void up2_scalar(const float* lo, int Wc, int Hc, int oddW, int oddH, float* out) {
    if (Wc < 2) throw std::runtime_error("the genuine scalar up-sampling reads past the row for Wc < 2");
    const int OW = 2 * Wc + oddW;
    std::vector<float> src(static_cast<size_t>(Hc) * Wc), a3(src.size());
    for (int pass = 0; pass < 2; ++pass) {
        for (int i = 0; i < Hc; ++i) {
            const float* l = lo + static_cast<size_t>(i) * Wc;
            const int ni = pass == 0 ? (i > 0 ? i - 1 : 0) : (i < Hc - 1 ? i + 1 : Hc - 1);
            const float* nb = lo + static_cast<size_t>(ni) * Wc;
            for (int j = 0; j < Wc; ++j) src[static_cast<size_t>(i) * Wc + j] = FA(FM(l[j], F_3), nb[j]);
        }
        for (size_t q = 0; q < src.size(); ++q) a3[q] = FM(src[q], F_3);
        for (int i = 0; i < Hc; ++i) {
            const float* sr = src.data() + static_cast<size_t>(i) * Wc;
            const float* a = a3.data() + static_cast<size_t>(i) * Wc;
            float* o = out + static_cast<size_t>(2 * i + pass) * OW;
            o[0] = FM(FM(sr[0], F_4), F_0_0625);
            for (int j = 1; j < Wc; ++j) o[2 * j] = FM(FA(a[j], sr[j - 1]), F_0_0625);
            for (int j = 0; j < Wc - 1; ++j) o[2 * j + 1] = FM(FA(a[j], sr[j + 1]), F_0_0625);
            o[2 * Wc - 1] = FM(FM(sr[Wc - 1], F_4), F_0_0625);
        }
    }
    if (oddW)
        for (int i = 0; i < 2 * Hc; ++i) out[static_cast<size_t>(i) * OW + 2 * Wc] = out[static_cast<size_t>(i) * OW + 2 * Wc - 1];
    if (oddH)
        std::memcpy(out + static_cast<size_t>(2 * Hc) * OW, out + static_cast<size_t>(2 * Hc - 1) * OW, sizeof(float) * OW);
}

// nv_pixnoise.up2 (FUN_00993df0)
void up2(const float* lo, int Wc, int Hc, int oddW, int oddH, float* out) {
    oddW &= 1;
    oddH &= 1;
    if (Hc < 2 || Wc < 4) {
        up2_scalar(lo, Wc, Hc, oddW, oddH, out);
        return;
    }
    const int OW = 2 * Wc + oddW;
    up2_neon(lo, Wc, Hc, out, OW);
    if (oddW)
        for (int i = 0; i < 2 * Hc; ++i) out[static_cast<size_t>(i) * OW + 2 * Wc] = out[static_cast<size_t>(i) * OW + 2 * Wc - 1];
    if (oddH)
        std::memcpy(out + static_cast<size_t>(2 * Hc) * OW, out + static_cast<size_t>(2 * Hc - 1) * OW, sizeof(float) * OW);
}

// nv_pixnoise.band_from (FUN_00990da8): src - up2(lo)
void band_from(const float* lo, const float* src, int Wc, int Hc, int oddW, int oddH, float* out) {
    oddW &= 1;
    oddH &= 1;
    const int H2 = 2 * Hc + oddH, W2 = 2 * Wc + oddW;
    const size_t N = static_cast<size_t>(H2) * W2;
    if (Hc < 2 || Wc < 4) {
        std::vector<float> u(N);
        up2_scalar(lo, Wc, Hc, oddW, oddH, u.data());
        for (size_t q = 0; q < N; ++q) out[q] = FS(src[q], u[q]);
        return;
    }
    std::vector<float> u(static_cast<size_t>(2 * Hc) * 2 * Wc);
    up2_neon(lo, Wc, Hc, u.data(), 2 * Wc);
    for (int i = 0; i < 2 * Hc; ++i) {
        const float* sr = src + static_cast<size_t>(i) * W2;
        const float* ur = u.data() + static_cast<size_t>(i) * 2 * Wc;
        float* o = out + static_cast<size_t>(i) * W2;
        for_lanes(2 * Wc, [&](int j, auto t) { ST(o + j, FS(LD(sr + j, t), LD(ur + j, t))); });
    }
    if (oddW) {
        const int c = 2 * Wc;
        for (int i = 0; i < 2 * Hc; ++i) {
            const size_t r = static_cast<size_t>(i) * W2;
            out[r + c] = FA(FS(out[r + c - 1], src[r + c - 1]), src[r + c]);
        }
    }
    if (oddH) {
        const size_t r = static_cast<size_t>(2 * Hc) * W2, rp = r - W2;
        for (int j = 0; j < W2; ++j) out[r + j] = FA(FS(out[rp + j], src[rp + j]), src[r + j]);
    }
}

// nv_pixnoise.ratio_eps (FUN_009ee04c): a / (b + eps); NEON part FRECPE + one FRECPS step, the last n & 3
// elements a true division.  For a NaN d, est (d quieted) and step (FRECPS negates op1: -d) are NaNs of
// opposite sign: the product keeps the order est * step of the genuine / numpy (FM).
void ratio_eps(const float* a, const float* b, float eps, size_t n, float* out) {
    const size_t m = n & ~static_cast<size_t>(3);
#if NVP_ASM
    const V ve = nvp::dup(eps);
    for (size_t i = 0; i < m; i += 4) {
        const V d = FA(ve, nvp::load(b + i));
        const V est = nvp::frecpe(d);
        const V step = nvp::frecps(d, est);
        nvp::store(out + i, FM(nvp::load(a + i), FM(est, step)));
    }
#else
    for (size_t i = 0; i < m; ++i) {
        const float d = FA(eps, b[i]);
        const float est = nvp::frecpe(d);
        const float step = nvp::frecps(d, est);
        out[i] = FM(a[i], FM(est, step));
    }
#endif
    for (size_t i = m; i < n; ++i) out[i] = nvp::div(a[i], FA(b[i], eps));
}

// nv_pixnoise.ramp (FUN_009ee93c)
void ramp(float a, float b, float va, float vb, const float* x, size_t n, float* y) {
    if ((b - a) <= (b * F_1EM5)) {
        a = (a * F_0_99999) + (-F_1EM5);
        b = (b * F_1_00001) + F_1EM5;
    }
    const float slope = (vb - va) / (b - a);
    const float icpt = va - (a * slope);
    float lo, hi;
    if (!(vb > va)) {
        lo = vb;
        hi = va;
    } else {
        lo = va;
        hi = vb;
    }
    const size_t m = n & ~static_cast<size_t>(3);
#if NVP_ASM
    const V vs = nvp::dup(slope), vi = nvp::dup(icpt), vh = nvp::dup(hi), vl = nvp::dup(lo);
    for (size_t i = 0; i < m; i += 4)  // FMLA (addend icpt), FMIN hi, FMAX lo
        nvp::store(y + i, nvp::fmax(nvp::fmin(nvp::fma(nvp::load(x + i), vs, vi), vh), vl));
#else
    for (size_t i = 0; i < m; ++i) y[i] = nvp::fmax(nvp::fmin(fma32(x[i], slope, icpt), hi), lo);
#endif
    for (size_t i = m; i < n; ++i) {
        float t = FA(icpt, FM(slope, x[i]));
        t = t > hi ? hi : t;
        y[i] = t > lo ? t : lo;
    }
}

// element-wise a op b (array op array: the first operand's NaN wins), a * c (c a constant)
void vadd(const float* a, const float* b, float* o, size_t n) {
    for_lanes(static_cast<int>(n), [&](int i, auto t) { ST(o + i, FA(LD(a + i, t), LD(b + i, t))); });
}
void vsub(const float* a, const float* b, float* o, size_t n) {
    for_lanes(static_cast<int>(n), [&](int i, auto t) { ST(o + i, FS(LD(a + i, t), LD(b + i, t))); });
}
void vmul(const float* a, const float* b, float* o, size_t n) {
    for_lanes(static_cast<int>(n), [&](int i, auto t) { ST(o + i, FM(LD(a + i, t), LD(b + i, t))); });
}
void vmulc(const float* a, float c, float* o, size_t n) {
    for_lanes(static_cast<int>(n), [&](int i, auto t) { ST(o + i, FM(LD(a + i, t), DUP(c, t))); });
}

inline void sqrt_abs(float* x, size_t n) {
    for (size_t i = 0; i < n; ++i) x[i] = std::sqrt(std::fabs(x[i]));
}

// nv_pixnoise.weighted_sum3 / luma_601 (FUN_009f11fc with the BT.601 weights)
void luma_601(const float* R, const float* G, const float* B, size_t n, float* out) {
    const float a = bits(LUMA_BITS[0]), b = bits(LUMA_BITS[1]), c = bits(LUMA_BITS[2]);
    const size_t m = n & ~static_cast<size_t>(3);
    for (size_t i = 0; i < m; ++i) {
        const float ar = FM(a, R[i]);
        out[i] = fma32(B[i], c, fma32(G[i], b, ar));
    }
    for (size_t i = m; i < n; ++i) {
        const float sm = FA(FM(R[i], a), FM(G[i], b));
        out[i] = FA(sm, FM(B[i], c));
    }
}

}  // namespace

// ------------------------------------------------------------------ nv_pixnoise.pixnoise
void pixnoise(const float* luma, int W, int H, float* weight, float* noise, float* bright) {
    if (W < 2 || H < 2) throw std::invalid_argument("the tile must be at least 2x2");
    Plane src(W, H);
    std::memcpy(src.p(), luma, sizeof(float) * src.n());
    const int sW[2] = {W, W >> 1}, sH[2] = {H, H >> 1};
    const int oW[2] = {W & 1, (W >> 1) & 1}, oH[2] = {H & 1, (H >> 1) & 1};
    const size_t N0 = static_cast<size_t>(W) * H;
    for (int k = 0; k < 2; ++k) {
        const int Wk = sW[k], Hk = sH[k];
        if (Wk < 1 || Hk < 1) throw std::invalid_argument("the tile is too small for two levels");
        const size_t n = static_cast<size_t>(Wk) * Hk;
        Plane Bx(Wk, Hk), tmp(Wk, Hk);
        box2(src.p(), Wk, Hk, 1, 0, Bx.p(), tmp.p());
        if (k == 0) std::memcpy(bright, Bx.p(), sizeof(float) * n);
        Plane lo(Wk >> 1, Hk >> 1);
        down2(Bx.p(), Wk, Hk, lo.p());
        Plane band(Wk, Hk);
        band_from(lo.p(), src.p(), Wk >> 1, Hk >> 1, oW[k], oH[k], band.p());
        const KernelSet& ks = kernels_for(Wk, Hk);
        Plane f(Wk, Hk), g(Wk, Hk), ff(Wk, Hk), e(Wk, Hk), d(Wk, Hk), S(Wk, Hk), best(Wk, Hk), Fi_(Wk, Hk);
        for (int i = 0; i < 16; ++i) {
            direction_filter(ks, i, band.p(), f.p(), tmp.p());
            vmul(f.p(), f.p(), ff.p(), n);
            box2(ff.p(), Wk, Hk, 1, 1, e.p(), tmp.p());
            const float nrm = 1.0f / std::fabs(bits(REF_BITS[k][i]));
            sqrt_abs(e.p(), n);
            vmulc(e.p(), nrm, e.p(), n);
            vsub(band.p(), f.p(), d.p(), n);
            direction_filter(ks, i, d.p(), g.p(), tmp.p());
            // F_i = f + g; argmax_maps: start 0 / index 0, update on best <= E_i (later index wins)
            float* Sp = S.p();
            float* bp = best.p();
            const float* fp = f.p();
            const float* gp = g.p();
            const float* ep = e.p();
            float* Fp = Fi_.p();
            vadd(fp, gp, Fp, n);
            if (i == 0)
                for (size_t q = 0; q < n; ++q) {
                    const float Fi = Fp[q];
                    const bool hit = bp[q] <= ep[q];
                    bp[q] = hit ? ep[q] : bp[q];
                    Sp[q] = Fi;
                }
            else
                for (size_t q = 0; q < n; ++q) {
                    const float Fi = Fp[q];
                    const bool hit = bp[q] <= ep[q];
                    bp[q] = hit ? ep[q] : bp[q];
                    Sp[q] = hit ? Fi : Sp[q];
                }
        }
        Plane R(Wk, Hk), RR(Wk, Hk), SS(Wk, Hk);
        vsub(band.p(), S.p(), R.p(), n);
        vmul(R.p(), R.p(), RR.p(), n);
        if (k == 0) {
            box2(RR.p(), Wk, Hk, 1, 1, noise, tmp.p());
            sqrt_abs(noise, n);
        }
        Plane ER(Wk, Hk), ES(Wk, Hk), E10(Wk, Hk), rat(Wk, Hk), w1(Wk, Hk), w2(Wk, Hk);
        box2(RR.p(), Wk, Hk, 2, 1, ER.p(), tmp.p());
        sqrt_abs(ER.p(), n);
        vmul(S.p(), S.p(), SS.p(), n);
        box2(SS.p(), Wk, Hk, 2, 1, ES.p(), tmp.p());
        sqrt_abs(ES.p(), n);
        ratio_eps(ES.p(), ER.p(), F_1EM4, n, rat.p());
        ramp(F_0_8, F_1_2, 1.0f, 0.0f, rat.p(), n, w1.p());
        box2(RR.p(), Wk, Hk, 10, 0, E10.p(), tmp.p());
        sqrt_abs(E10.p(), n);
        ratio_eps(ER.p(), E10.p(), F_1EM4, n, rat.p());
        ramp(F_1_1, F_1_5, 1.0f, 0.0f, rat.p(), n, w2.p());
        if (k == 0) {
            vmul(weight, w1.p(), weight, N0);
            vmul(weight, w2.p(), weight, N0);
        } else {
            Plane u(W, H);
            up2(w1.p(), Wk, Hk, oW[0], oH[0], u.p());
            vmul(weight, u.p(), weight, N0);
            up2(w2.p(), Wk, Hk, oW[0], oH[0], u.p());
            vmul(weight, u.p(), weight, N0);
        }
        src = std::move(lo);
    }
}

// ------------------------------------------------------------------ nv_curve (a) tiles
namespace {
std::array<int, 11> tile_desc(int x, int y, int w, int h, int W, int H, int margin, int min_size) {
    int lx = x < margin ? x : margin;
    int rx = (margin + w + x <= W) ? margin : (W - x) - w;
    int ty = y < margin ? y : margin;
    int by = (margin + h + y <= H) ? margin : (H - y) - h;
    int tw = lx + w + rx;
    int th = ty + h + by;
    if (tw < min_size) {
        const int lx2 = (min_size - tw) + lx;
        lx = lx2 > x ? x : lx2;
        tw = lx + w + rx;
        if (tw < min_size) {
            const int rx2 = (min_size - tw) + rx;
            rx = (rx2 + w + x <= W) ? rx2 : (W - x) - w;
            tw = rx + lx + w;
        }
    }
    if (th < min_size) {
        const int ty2 = (min_size - th) + ty;
        ty = ty2 > y ? y : ty2;
        th = ty + h + by;
        if (th < min_size) {
            const int by2 = (min_size - th) + by;
            by = (by2 + h + y <= H) ? by2 : (H - y) - h;
            th = by + ty + h;
        }
    }
    return {x - lx, y - ty, tw, th, x, y, w, h, lx, ty, 0};
}
}  // namespace

std::vector<std::array<int, 11>> tile_layout(int W, int H) {
    std::vector<std::array<int, 11>> out;
    if (W < MIN_TILE || H < MIN_TILE) return out;
    const int ncols = (W + TILE - 1) / TILE, nrows = (H + TILE - 1) / TILE;
    int y = 0;
    for (int r = 0; r < nrows; ++r) {
        const int h = std::min(TILE, H - y);
        int x = 0;
        for (int c = 0; c < ncols; ++c) {
            const int w = std::min(TILE, W - x);
            out.push_back(tile_desc(x, y, w, h, W, H, MARGIN, MIN_TILE));
            x += w;
        }
        y += h;
    }
    return out;
}

// ------------------------------------------------------------------ nv_curve (b) IsClip
namespace {
// nv_curve._minmax_genuine (FUN_009efdd4, SIMD build)
void minmax_genuine(const float* v, size_t n, float* omn, float* omx) {
    const size_t m = n & ~static_cast<size_t>(7);
    float mn = FLT_MAX_, mx = -FLT_MAX_;
    if (m) {
        bool nan = false;
        float bmn = v[0], bmx = v[0];
        for (size_t i = 0; i < m; ++i) {
            const float x = v[i];
            if (std::isnan(x)) {
                nan = true;
                break;
            }
            if (x < bmn) bmn = x;
            if (x > bmx) bmx = x;
        }
        if (nan) {
            mn = mx = std::numeric_limits<float>::quiet_NaN();
        } else {
            mn = bmn < mn ? bmn : mn;  // Python min(mn, bmn)
            mx = bmx > mx ? bmx : mx;  // Python max(mx, bmx)
        }
    }
    if (m < n) {
        bool has = false;
        float tmn = 0.0f, tmx = 0.0f;
        for (size_t i = m; i < n; ++i) {
            const float x = v[i];
            if (std::isnan(x)) continue;
            if (!has) {
                tmn = tmx = x;
                has = true;
            } else {
                if (x < tmn) tmn = x;
                if (x > tmx) tmx = x;
            }
        }
        if (has) {
            if (!std::isnan(mx)) mx = tmx > mx ? tmx : mx;
            if (!std::isnan(mn)) mn = tmn < mn ? tmn : mn;
        }
    }
    *omn = mn;
    *omx = mx;
}
}  // namespace

float is_clip_genuine(const float* v, size_t n) {
    float mn, mx;
    minmax_genuine(v, n, &mn, &mx);
    const float rng = mx - mn;
    const float nbf = (rng * 8.0f) + 1.0f;
    int64_t nb = fcvtzs(nbf);
    if (nb < 2) return 1.0f;
    nb = std::min<int64_t>(nb, 0x4000);
    const float k = static_cast<float>(nb - 1) / rng;
    // histogram of the bin numbers (h = bincount); the mode is the lowest bin of maximal count (argmax)
    thread_local std::vector<int32_t> idx;
    thread_local std::vector<int64_t> h;
    if (idx.size() < n) idx.resize(n);
    if (h.size() < 0x4000) h.assign(0x4000, 0);
    for (size_t i = 0; i < n; ++i) {
        const float dv = v[i] - mn;
        const float p = k * dv;
        int64_t b = fcvtzs(std::rint(p));  // fsub, fmul, frintn, fcvtzs
        if (b < 0) b = 0;
        if (b > nb - 1) b = nb - 1;
        idx[i] = static_cast<int32_t>(b);
        h[static_cast<size_t>(b)] += 1;
    }
    int64_t peak = 0, m = 0;
    for (size_t i = 0; i < n; ++i) {
        const int64_t c = h[idx[i]];
        if (c > peak || (c == peak && idx[i] < m)) {
            peak = c;
            m = idx[i];
        }
    }
    int64_t below = 0;
    for (size_t i = 0; i < n; ++i) below += idx[i] < m ? 1 : 0;
    for (size_t i = 0; i < n; ++i) h[idx[i]] = 0;
    const int64_t above = static_cast<int64_t>(n) - below - peak;
    const int64_t nn = static_cast<int64_t>(n);
    if (nn < 2 * peak || below * 10 < above || above * 10 < below)
        return static_cast<float>(peak) / static_cast<float>(nn);
    return 0.0f;
}

namespace {
// nv_curve.clip_weight: 16x16 blocks of R and B (G is never read)
void clip_weight(const float* R, const float* B, int w, int h, float* weight) {
    std::fill(weight, weight + static_cast<size_t>(w) * h, 1.0f);
    const int nrow = (h + BLOCK - 1) / BLOCK, ncol = (w + BLOCK - 1) / BLOCK;
    float blk[BLOCK * BLOCK];
    for (int r = 0; r < nrow; ++r) {
        const int by = BLOCK <= h - r * BLOCK ? r * BLOCK : h - BLOCK;
        const int by0 = std::max(by, 0), bh = std::min(by0 + BLOCK, h) - by0;
        for (int c = 0; c < ncol; ++c) {
            const int bx = BLOCK <= w - c * BLOCK ? c * BLOCK : w - BLOCK;
            const int bx0 = std::max(bx, 0), bw = std::min(bx0 + BLOCK, w) - bx0;
            float cr, cb;
            const float* planes[2] = {R, B};
            float res[2];
            for (int p = 0; p < 2; ++p) {
                size_t q = 0;
                for (int i = 0; i < bh; ++i)
                    for (int j = 0; j < bw; ++j) blk[q++] = planes[p][static_cast<size_t>(by0 + i) * w + bx0 + j];
                res[p] = is_clip_genuine(blk, q);
            }
            cr = res[0];
            cb = res[1];
            if (cr > F_CLIP_THR || cb > F_CLIP_THR) {
                for (int i = 0; i < bh; ++i)
                    for (int j = 0; j < bw; ++j) weight[static_cast<size_t>(by0 + i) * w + bx0 + j] = 0.0f;
            }
        }
    }
}

// bin_index: (long)(8 * 0.00390625f * bright) clipped to [0, nbins-1] (NaN -> 0)
inline int bin_index(float bright) {
    const float scale = static_cast<float>(NBINS) * F_INV256;
    const float v = std::trunc(scale * bright);
    if (std::isnan(v)) return 0;
    if (v < 0.0f) return 0;
    if (v > static_cast<float>(NBINS - 1)) return NBINS - 1;
    return static_cast<int>(v);
}

struct Kept {
    std::vector<uint8_t> bins;
    std::vector<float> noise;
};

// one tile of FUN_009b76b0: clip weight, luma, pixnoise, kept core pixels (row-major)
void process_tile(const float* rgb, int W, int stride_c, const std::array<int, 11>& d, Kept* out) {
    (void)W;
    const int x = d[0], y = d[1], w = d[2], h = d[3];
    const size_t n = static_cast<size_t>(w) * h;
    std::vector<float> R(n), G(n), B(n);
    for (int i = 0; i < h; ++i)
        for (int j = 0; j < w; ++j) {
            const float* px = rgb + (static_cast<size_t>(y + i) * W + (x + j)) * stride_c;
            const size_t q = static_cast<size_t>(i) * w + j;
            R[q] = px[0];
            G[q] = px[1];
            B[q] = px[2];
        }
    std::vector<float> weight(n), luma(n), noise(n), bright(n);
    clip_weight(R.data(), B.data(), w, h, weight.data());
    luma_601(R.data(), G.data(), B.data(), n, luma.data());
    pixnoise(luma.data(), w, h, weight.data(), noise.data(), bright.data());
    const int cx = d[8], cy = d[9], cw = d[6], ch = d[7];
    out->bins.clear();
    out->noise.clear();
    for (int i = cy; i < cy + ch; ++i)
        for (int j = cx; j < cx + cw; ++j) {
            const size_t q = static_cast<size_t>(i) * w + j;
            if (weight[q] < F_HALF) continue;  // fcmp ... b.mi: NaN weights are counted
            out->bins.push_back(static_cast<uint8_t>(bin_index(bright[q])));
            out->noise.push_back(noise[q]);
        }
}

// ------------------------------------------------------------------ nv_curve (c) polynomial
double det(const std::vector<std::vector<double>>& m, int n) {
    if (n == 1) return m[0][0];
    if (n < 1) return 0.0;
    double dt = 0.0, sign = 1.0;
    for (int i = 0; i < n; ++i) {
        std::vector<std::vector<double>> minor;
        for (int k = 0; k < n; ++k) {
            if (k == i) continue;
            minor.emplace_back(m[k].begin() + 1, m[k].begin() + n);
        }
        double t = mul1(m[i][0], det(minor, n - 1));
        t = mul1(sign, t);
        dt = add1(dt, t);
        sign = -sign;
    }
    return dt;
}

std::vector<double> cramer(const std::vector<std::vector<double>>& a, const std::vector<double>& b, int n) {
    const double dd = det(a, n);
    std::vector<double> out;
    for (int k = 0; k < n; ++k) {
        auto mk = a;
        for (int r = 0; r < n; ++r) mk[r][k] = b[r];
        out.push_back(div1(det(mk, n), dd));
    }
    return out;
}

std::vector<double> polyfit(const double* xs, const double* ys, int npts, int ncoef) {
    if (ncoef < 1) return {};
    std::vector<double> b(ncoef, 0.0);
    std::vector<std::vector<double>> a(ncoef, std::vector<double>(ncoef, 0.0));
    for (int i = 0; i < ncoef; ++i) {
        double s = 0.0;
        for (int p = 0; p < npts; ++p) s = add1(s, mul1(ys[p], g_pow(xs[p], static_cast<double>(i))));
        b[i] = s;
        for (int j = 0; j < ncoef; ++j) {
            s = 0.0;
            for (int p = 0; p < npts; ++p) s = add1(s, g_pow(xs[p], static_cast<double>(i + j)));
            a[i][j] = s;
        }
    }
    return cramer(a, b, ncoef);
}

double polyval(double x, const double* c, int ncoef) {
    double s = 0.0;
    for (int i = 0; i < ncoef; ++i) s = add1(s, mul1(g_pow(x, static_cast<double>(i)), c[i]));
    return s;
}

void fill_curve(double* m) {
    const int nbins = NBINS;
    std::vector<double> xs, ys;
    int first = -1, last = -1;
    for (int i = 0; i < nbins; ++i) {
        const double v = m[i];
        if (!(v <= 0.0)) {
            if (first < 0) first = i;
            xs.push_back(static_cast<double>(i));
            ys.push_back(v);
            last = i;
        }
    }
    const int npts = static_cast<int>(xs.size());
    if (npts < 2) {
        const double v = npts == 1 ? ys[0] : 1.0;
        for (int i = 0; i < nbins; ++i) m[i] = v;
        return;
    }
    if (npts == nbins) return;
    const int ncoef = npts >= 3 ? 3 : 2;
    std::vector<double> c = polyfit(xs.data(), ys.data(), npts, ncoef);
    for (int i = first + 1; i < last; ++i)
        if (!(m[i] > 0.0)) m[i] = polyval(static_cast<double>(i), c.data(), ncoef);
    if (first > 0) {
        const int third = nbins / 3;
        int nfit, nc;
        if (first < third) {
            nfit = npts > 2 ? 3 : 2;
            nc = 2;
        } else {
            nfit = 1;
            nc = 1;
        }
        c = polyfit(xs.data(), ys.data(), nfit, nc);
        for (int i = first - 1; i >= 0; --i) {
            const double v = polyval(static_cast<double>(i), c.data(), nc);
            m[i] = v;
            if (v <= 0.0) m[i] = m[i + 1];
        }
    }
    if (last + 1 < nbins) {
        const int two3 = (2 * nbins) / 3;
        int nfit, nc;
        if (last >= two3) {
            nfit = npts > 2 ? 3 : 2;
            nc = 2;
        } else {
            nfit = 1;
            nc = 1;
        }
        c = polyfit(xs.data() + (npts - nfit), ys.data() + (npts - nfit), nfit, nc);
        for (int i = last + 1; i < nbins; ++i) {
            const double v = polyval(static_cast<double>(i), c.data(), nc);
            m[i] = v;
            if (v <= 0.0) m[i] = m[i - 1];
        }
    }
}

}  // namespace

// ------------------------------------------------------------------ driver
bool genuine_noise_curve(const float* rgb, int W, int H, int stride_c, CurveResult* out, int threads,
                         const Cancel& cancel) {
    if (W < MIN_TILE || H < MIN_TILE || stride_c < 3) return false;
    const auto tiles = tile_layout(W, H);
    const size_t nt = tiles.size();
    std::vector<Kept> kept(nt);
    // pre-build the kernel sets of every tile size (levels 0 and 1) outside the workers
    for (const auto& d : tiles) {
        kernels_for(d[2], d[3]);
        kernels_for(d[2] >> 1, d[3] >> 1);
    }
    double sums[NBINS] = {0};
    int64_t counts[NBINS] = {0};
    auto add = [&](const Kept& k) {
        for (size_t q = 0; q < k.bins.size(); ++q) {
            sums[k.bins[q]] = add1(sums[k.bins[q]], static_cast<double>(k.noise[q]));  // np.add.at
            counts[k.bins[q]] += 1;
        }
    };
    if (threads <= 1) {
        Kept k;
        for (size_t t = 0; t < nt; ++t) {
            if (cancel && cancel()) return false;   // nv_curve._cancelled before each tile
            process_tile(rgb, W, stride_c, tiles[t], &k);
            add(k);
        }
    } else {
        std::mutex mu;                               // guards next / cancelled / the cancel query
        size_t next = 0;
        bool cancelled = false;
        std::vector<std::thread> pool;
        for (int i = 0; i < threads; ++i)
            pool.emplace_back([&]() {
                for (;;) {
                    size_t t;
                    {
                        std::lock_guard<std::mutex> lk(mu);
                        if (cancelled || next >= nt) break;
                        if (cancel && cancel()) {
                            cancelled = true;
                            break;
                        }
                        t = next++;
                    }
                    process_tile(rgb, W, stride_c, tiles[t], &kept[t]);
                }
            });
        for (auto& th : pool) th.join();
        if (cancelled) return false;
        for (size_t t = 0; t < nt; ++t) add(kept[t]);  // scheduler order, row-major within the tile
    }
    if (cancel && cancel()) return false;            // the query that finds the scheduler empty
    // curve_means
    const int64_t thr = (static_cast<int64_t>(W) * H) >> 10;
    double m[NBINS];
    for (int i = 0; i < NBINS; ++i) m[i] = counts[i] >= thr ? div1(sums[i], static_cast<double>(counts[i])) : -1.0;
    fill_curve(m);
    // smooth_curve
    double ys[NBINS];
    ys[0] = m[0];
    ys[NBINS - 1] = m[NBINS - 1];
    for (int i = 1; i < NBINS - 1; ++i)  // numpy float64 scalars (add2)
        ys[i] = add2(m[i], add2(m[i - 1], m[i + 1]) * 0.5) * 0.5;
    // curve_xs
    const float step = 256.0f / static_cast<float>(NBINS);
    for (int i = 0; i < NBINS; ++i) {
        out->xs[i] = static_cast<double>(step * (static_cast<float>(i) + F_HALF));
        out->ys[i] = ys[i];
        out->sums[i] = sums[i];
        out->counts[i] = counts[i];
    }
    return true;
}

// ------------------------------------------------------------------ nv_curve (d) interpolator
CurveInterp::CurveInterp(const double* xs, const double* ys, int n) : xs_(xs, xs + n), ys_(ys, ys + n), n_(n) {
    if (n < 2) throw std::invalid_argument("the genuine interpolator needs n >= 2 nodes");
    const std::vector<double> lo = polyfit(xs_.data(), ys_.data(), 2, 2);
    const std::vector<double> hi = polyfit(xs_.data() + (n - 2), ys_.data() + (n - 2), 2, 2);
    lo_[0] = lo[0];
    lo_[1] = lo[1];
    hi_[0] = hi[0];
    hi_[1] = hi[1];
}

double CurveInterp::operator()(double x) const {
    const double* xs = xs_.data();
    const double* ys = ys_.data();
    const int n = n_;
    if (x < xs[0]) return polyval(x, lo_, 2);
    if (xs[n - 1] <= x) return polyval(x, hi_, 2);
    int j = 0, k;
    bool check = xs[0] <= x;
    for (;;) {
        if (check && xs[j + 1] > x) {
            k = j + 1;
            break;
        }
        if (j == n - 2) return std::numeric_limits<double>::quiet_NaN();
        ++j;
        check = xs[j] <= x;
    }
    const double x0 = xs[k - 1];
    const double t = div1(sub1(x, x0), sub1(xs[k], x0));
    return add1(mul1(ys[k - 1], sub1(1.0, t)), mul1(t, ys[k]));  // Python floats
}

// software FRECPE / FRECPS, exported for the self-test of the test driver
// (the hardware forms are the CPU instructions on AArch64 in every build, the software forms elsewhere)
float frecpe_software(float x) { return nvp::frecpe_soft(x); }
float frecps_software(float a, float b) { return nvp::frecps_soft(a, b); }
float frecpe_hardware(float x) { return nvp::frecpe_hw(x); }
float frecps_hardware(float a, float b) { return nvp::frecps_hw(a, b); }

// nv_pixnoise.up2 / ratio_eps for nv_ui (the viewer's BuildFreqImage, N3a): the functions above, unchanged
void up2_ext(const float* lo, int Wc, int Hc, int oddW, int oddH, float* out) { up2(lo, Wc, Hc, oddW, oddH, out); }
void ratio_eps_ext(const float* a, const float* b, float eps, size_t n, float* out) { ratio_eps(a, b, eps, n, out); }

}  // namespace nvc
