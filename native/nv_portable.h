// nv_portable - the float32 / float64 operations of the bit-exact ports (N1a nv_band, N1b nv_curve,
// N1e nv_model) with the AArch64 result made explicit, on every architecture.
//
// The ports reproduce the Python ports, which compute (through numpy and ctypes) exactly what one
// AArch64 instruction with the given operand order returns (FPCR: round to nearest, FZ = 0, DN = 0):
//   * when an operand is NaN: FPProcessNaNs(op1, op2) - the first signalling NaN (quieted), else the
//     first quiet NaN; FMADD a*b + c uses FPProcessNaNs3(c, a, b) and gives the default NaN for a quiet
//     NaN c with inf * 0;
//   * a NaN made from non-NaN operands (inf - inf, 0 * inf, 0 / 0, sqrt(-1), ...) is the default NaN
//     0x7fc00000 / 0x7ff8000000000000 (x86 SSE makes 0xffc00000 / 0xfff8000000000000);
//   * FRECPE / FRECPS (reciprocal estimate / step), FMIN / FMAX (NaN-propagating, -0 < +0).
// Two implementations with the same results:
//   * AArch64 (default): one instruction per operation as inline asm (the compiler can neither commute
//     the operands - clang -O2 does that for fadd / fmul without fast-math - nor fold x * 1.0), NEON for
//     the 4-lane forms (NVP_ASM = 1);
//   * portable (any other architecture, or -DNV_PORTABLE on AArch64): plain C++ for the non-NaN cases
//     and the rules above written out (NVP_ASM = 0, no 4-lane forms).  FRECPE / FRECPS are the software
//     FPRecipEstimate / FPRecipStepFused of nv_pixnoise.frecpe32 / frecps32.
// native/verify_band.py / verify_curve.py / verify_model.py check both builds against Python;
// native/verify_x86.sh compares an x86_64 build with the arm64 build byte for byte.
// Build with -ffp-contract=off -fno-fast-math.
#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

#if defined(NVC_PORTABLE) && !defined(NV_PORTABLE)
#define NV_PORTABLE 1   // the former nv_curve switch
#endif

#if defined(__aarch64__)
#include <arm_neon.h>
#endif
#if defined(__aarch64__) && !defined(NV_PORTABLE)
#define NVP_ASM 1
#else
#define NVP_ASM 0
#endif

namespace nvp {

// ------------------------------------------------------------------ bits
inline float bits(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }
inline uint32_t ubits(float f) { uint32_t u; std::memcpy(&u, &f, 4); return u; }
inline double bitsd(uint64_t u) { double d; std::memcpy(&d, &u, 8); return d; }
inline uint64_t ubitsd(double d) { uint64_t u; std::memcpy(&u, &d, 8); return u; }

constexpr uint32_t QUIET = 0x00400000u;
constexpr uint64_t QUIET_D = 0x0008000000000000ULL;
constexpr uint32_t DEFAULT_NAN = 0x7fc00000u;
constexpr uint64_t DEFAULT_NAN_D = 0x7ff8000000000000ULL;

// NaN tests on the bits (no floating-point compare involved)
inline bool isnan_(float x) { return (ubits(x) & 0x7fffffffu) > 0x7f800000u; }
inline bool isnan_(double x) { return (ubitsd(x) & 0x7fffffffffffffffULL) > 0x7ff0000000000000ULL; }
inline bool is_snan(float x) { return isnan_(x) && !(ubits(x) & QUIET); }
inline bool is_snan(double x) { return isnan_(x) && !(ubitsd(x) & QUIET_D); }
inline float quiet(float x) { return bits(ubits(x) | QUIET); }
inline double quiet(double x) { return bitsd(ubitsd(x) | QUIET_D); }

// FPProcessNaNs(a, b) / FPProcessNaNs3(a, b, c) with DN = 0, for operands of which one is a NaN
template <class T>
inline T process_nans(T a, T b) {
    if (is_snan(a)) return quiet(a);
    if (is_snan(b)) return quiet(b);
    return isnan_(a) ? a : b;
}
template <class T>
inline T process_nans3(T a, T b, T c) {
    if (is_snan(a)) return quiet(a);
    if (is_snan(b)) return quiet(b);
    if (is_snan(c)) return quiet(c);
    return isnan_(a) ? a : (isnan_(b) ? b : c);
}

// A value the compiler cannot see through (keeps x * 1.0 / fma(x, 1.0, c) as real operations).
inline float opaque(float x) {
#if defined(__aarch64__)
    __asm__ volatile("" : "+w"(x));
#elif defined(__x86_64__) || defined(__i386__)
    __asm__ volatile("" : "+x"(x));
#else
    volatile float v = x;
    x = v;
#endif
    return x;
}

// ------------------------------------------------------------------ portable forms (all builds)
namespace portable {

inline float dn(float r) { return isnan_(r) ? bits(DEFAULT_NAN) : r; }       // a NaN from non-NaN operands
inline double dn(double r) { return isnan_(r) ? bitsd(DEFAULT_NAN_D) : r; }

template <class T> inline T add(T a, T b) { return (isnan_(a) || isnan_(b)) ? process_nans(a, b) : dn(a + b); }
template <class T> inline T sub(T a, T b) { return (isnan_(a) || isnan_(b)) ? process_nans(a, b) : dn(a - b); }
template <class T> inline T mul(T a, T b) { return (isnan_(a) || isnan_(b)) ? process_nans(a, b) : dn(a * b); }
template <class T> inline T div(T a, T b) { return (isnan_(a) || isnan_(b)) ? process_nans(a, b) : dn(a / b); }
// FMADD a*b + c (FPMulAdd(addend c, a, b)), one rounding
inline float fma(float a, float b, float c) {
    if (isnan_(a) || isnan_(b) || isnan_(c)) {
        const bool inv = (std::isinf(a) && b == 0.0f) || (a == 0.0f && std::isinf(b));
        if (!is_snan(c) && isnan_(c) && inv) return bits(DEFAULT_NAN);
        return process_nans3(c, a, b);
    }
    return dn(std::fma(a, b, c));
}
inline float sqrt(float x) { return isnan_(x) ? quiet(x) : dn(std::sqrt(x)); }
inline double sqrt(double x) { return isnan_(x) ? quiet(x) : dn(std::sqrt(x)); }
// FMIN / FMAX (NaN-propagating, -0 < +0)
inline float fmin(float a, float b) {
    if (isnan_(a) || isnan_(b)) return process_nans(a, b);
    if (a == 0.0f && b == 0.0f) return (std::signbit(a) || std::signbit(b)) ? -0.0f : 0.0f;
    return a < b ? a : b;
}
inline float fmax(float a, float b) {
    if (isnan_(a) || isnan_(b)) return process_nans(a, b);
    if (a == 0.0f && b == 0.0f) return (std::signbit(a) && std::signbit(b)) ? -0.0f : 0.0f;
    return a > b ? a : b;
}

}  // namespace portable

// software FPRecipEstimate / FPRecipStepFused (nv_pixnoise.frecpe32 / frecps32; nv_portable.cpp)
float frecpe_soft(float x);
float frecps_soft(float a, float b);

// the CPU's FRECPE / FRECPS on AArch64 in every build (the self-test of the software forms); elsewhere the
// software forms
#if defined(__aarch64__)
inline float frecpe_hw(float x) { return vrecpes_f32(x); }
inline float frecps_hw(float a, float b) { return vrecpss_f32(a, b); }
#else
inline float frecpe_hw(float x) { return frecpe_soft(x); }
inline float frecps_hw(float a, float b) { return frecps_soft(a, b); }
#endif

// ------------------------------------------------------------------ the operations
#if NVP_ASM
inline float add(float a, float b) { float r; __asm__("fadd %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline float sub(float a, float b) { float r; __asm__("fsub %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline float mul(float a, float b) { float r; __asm__("fmul %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline float div(float a, float b) { float r; __asm__("fdiv %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline float fma(float a, float b, float c) {   // FMADD: a*b + c
    float r;
    __asm__("fmadd %s0, %s1, %s2, %s3" : "=w"(r) : "w"(a), "w"(b), "w"(c));
    return r;
}
inline float sqrt(float x) { float r; __asm__("fsqrt %s0, %s1" : "=w"(r) : "w"(x)); return r; }
inline float fmin(float a, float b) { float r; __asm__("fmin %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline float fmax(float a, float b) { float r; __asm__("fmax %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline float frecpe(float x) { return vrecpes_f32(x); }
inline float frecps(float a, float b) { return vrecpss_f32(a, b); }
inline double add(double a, double b) { double r; __asm__("fadd %d0, %d1, %d2" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline double sub(double a, double b) { double r; __asm__("fsub %d0, %d1, %d2" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline double mul(double a, double b) { double r; __asm__("fmul %d0, %d1, %d2" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline double div(double a, double b) { double r; __asm__("fdiv %d0, %d1, %d2" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline double sqrt(double x) { double r; __asm__("fsqrt %d0, %d1" : "=w"(r) : "w"(x)); return r; }

// 4-lane forms (NEON; lane-wise the same results as the scalar forms)
using V4 = float32x4_t;
inline V4 add(V4 a, V4 b) { V4 r; __asm__("fadd %0.4s, %1.4s, %2.4s" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline V4 sub(V4 a, V4 b) { V4 r; __asm__("fsub %0.4s, %1.4s, %2.4s" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline V4 mul(V4 a, V4 b) { V4 r; __asm__("fmul %0.4s, %1.4s, %2.4s" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline V4 fma(V4 a, V4 b, V4 c) { return vfmaq_f32(c, a, b); }   // FMLA c + a*b
inline V4 fmin(V4 a, V4 b) { return vminq_f32(a, b); }
inline V4 fmax(V4 a, V4 b) { return vmaxq_f32(a, b); }
inline V4 frecpe(V4 x) { return vrecpeq_f32(x); }
inline V4 frecps(V4 a, V4 b) { return vrecpsq_f32(a, b); }
inline V4 load(const float* p) { return vld1q_f32(p); }
inline void store(float* p, V4 x) { vst1q_f32(p, x); }
inline V4 dup(float c) { return vdupq_n_f32(c); }
#else
inline float add(float a, float b) { return portable::add(a, b); }
inline float sub(float a, float b) { return portable::sub(a, b); }
inline float mul(float a, float b) { return portable::mul(a, b); }
inline float div(float a, float b) { return portable::div(a, b); }
inline float fma(float a, float b, float c) { return portable::fma(a, b, c); }
inline float sqrt(float x) { return portable::sqrt(x); }
inline float fmin(float a, float b) { return portable::fmin(a, b); }
inline float fmax(float a, float b) { return portable::fmax(a, b); }
inline float frecpe(float x) { return frecpe_soft(x); }
inline float frecps(float a, float b) { return frecps_soft(a, b); }
inline double add(double a, double b) { return portable::add(a, b); }
inline double sub(double a, double b) { return portable::sub(a, b); }
inline double mul(double a, double b) { return portable::mul(a, b); }
inline double div(double a, double b) { return portable::div(a, b); }
inline double sqrt(double x) { return portable::sqrt(x); }
#endif

// fcvtzs float / double -> int64 (round toward zero, saturating, NaN -> 0; a C++ cast is undefined there)
inline int64_t fcvtzs(double d) {
    if (isnan_(d)) return 0;
    if (d >= 9223372036854775808.0) return std::numeric_limits<int64_t>::max();
    if (d < -9223372036854775808.0) return std::numeric_limits<int64_t>::min();
    return static_cast<int64_t>(d);
}
inline int64_t fcvtzs(float v) { return fcvtzs(static_cast<double>(v)); }

}  // namespace nvp
