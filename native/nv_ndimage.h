// nv_ndimage - bit-exact C++ reproductions of the scipy.ndimage / numpy building blocks that
// repro/nvfilter.py's v5 kernels use (N1g / N1h).  Each routine reproduces the arithmetic of the
// versions installed here (scipy 1.15.3, numpy 2.1.3, arm64 build by conda):
//
//   gauss1          scipy.ndimage.gaussian_filter(a, 1.0)  (radius 4, mode "reflect"; NI_Correlate1D
//                   symmetric branch: o = x0*w0, then o = fmadd(x[-k] + x[+k], w[k], o) for k = 4..1,
//                   in double; axis 0 then axis 1, float32 between the passes)
//   uniform         scipy.ndimage.uniform_filter(a, size, mode=...)  (NI_UniformFilter1D running sum
//                   in double: tmp = 0 + x..; tmp += (x_in - x_out); out = tmp / size)
//   map_coords1     scipy.ndimage.map_coordinates(img, [cy, cx], order=1, mode="nearest")
//   median3         scipy.ndimage.median_filter(a, 3, mode="nearest")  (NI_Select on double buffers)
//   interp          numpy.interp (arr_interp: fmadd(slope, x - xp[j], fp[j]) and the NaN retry)
//   b3 / atrous     nvfilter._b3 / atrous (numpy float32 elementwise, symmetric padding)
//   down2 / fit / to_grid   nvfilter._down2 (numpy mean over (1,3): (+0 + ((-0+a)+b)) + ((-0+c)+d), / 4),
//                   _fit, _to_grid
//
// Operand order: every float add / mul whose operands can both be NaN is written as an AArch64
// instruction (inline asm) in the operand order of the library code (numpy array ops: first
// operand; scipy: the order of the compiled loops, read from the disassembly of _nd_image.so),
// because clang -O2 commutes fadd / fmul operands and the order decides the NaN payload.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace nvn {

inline float bits(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }
inline uint32_t ubits(float f) { uint32_t u; std::memcpy(&u, &f, 4); return u; }
inline double dbits(uint64_t u) { double f; std::memcpy(&f, &u, 8); return f; }

// ---------------------------------------------------------------- operand-order-fixed arithmetic
#if defined(__aarch64__)
inline float A(float a, float b) { float r; __asm__("fadd %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline float S(float a, float b) { float r; __asm__("fsub %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline float M(float a, float b) { float r; __asm__("fmul %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline float D(float a, float b) { float r; __asm__("fdiv %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline float SQRT(float a) { float r; __asm__("fsqrt %s0, %s1" : "=w"(r) : "w"(a)); return r; }
inline double AD(double a, double b) { double r; __asm__("fadd %d0, %d1, %d2" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline double SD(double a, double b) { double r; __asm__("fsub %d0, %d1, %d2" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline double MD(double a, double b) { double r; __asm__("fmul %d0, %d1, %d2" : "=w"(r) : "w"(a), "w"(b)); return r; }
inline double DD(double a, double b) { double r; __asm__("fdiv %d0, %d1, %d2" : "=w"(r) : "w"(a), "w"(b)); return r; }
// fmadd d, n, m, a  =  a + n*m (one rounding); NaN priority as the instruction defines it.
inline double FMAD(double n, double m, double a) {
    double r; __asm__("fmadd %d0, %d1, %d2, %d3" : "=w"(r) : "w"(n), "w"(m), "w"(a)); return r;
}
#else
inline float A(float a, float b) { return a + b; }
inline float S(float a, float b) { return a - b; }
inline float M(float a, float b) { return a * b; }
inline float D(float a, float b) { return a / b; }
float SQRT(float a);
inline double AD(double a, double b) { return a + b; }
inline double SD(double a, double b) { return a - b; }
inline double MD(double a, double b) { return a * b; }
inline double DD(double a, double b) { return a / b; }
double FMAD(double n, double m, double a);
#endif

inline bool isnanf_(float x) { return x != x; }
inline bool isnand_(double x) { return x != x; }

// np.maximum(x, scalar) / np.minimum (the scalar loop of loops_minmax: (A >= B || isnan(A)) ? A : B;
// numpy has no SIMD path for a broadcast scalar on NEON)
inline float np_max(float a, float b) { return (a >= b || isnanf_(a)) ? a : b; }
inline float np_min(float a, float b) { return (a <= b || isnanf_(a)) ? a : b; }
// np.clip(x, lo, hi) with constant non-NaN bounds (clip.cpp: if x < lo: x = lo; if x > hi: x = hi)
inline float np_clip(float x, float lo, float hi) {
    if (x < lo) x = lo;
    if (x > hi) x = hi;
    return x;
}
// Python's built-in max(a, b) / min(a, b) on floats (keeps a unless b compares greater / less)
inline double py_max(double a, double b) { return (b > a) ? b : a; }
inline double py_min(double a, double b) { return (b < a) ? b : a; }
// float32(x) of a Python float (numpy's weak-scalar cast: round to nearest)
inline float f32(double x) { return (float)x; }

// np.exp on float32 = libm expf (checked bit for bit on 400k inputs; called through a volatile
// pointer so that no compile-time folding happens)
float np_expf(float x);

// ------------------------------------------------------------------------------------------ plane
struct Plane {
    int H = 0, W = 0;
    std::vector<float> d;
    Plane() = default;
    Plane(int h, int w) : H(h), W(w), d((size_t)h * w) {}
    Plane(int h, int w, float v) : H(h), W(w), d((size_t)h * w, v) {}
    float* row(int y) { return d.data() + (size_t)y * W; }
    const float* row(int y) const { return d.data() + (size_t)y * W; }
    float& at(int y, int x) { return d[(size_t)y * W + x]; }
    float at(int y, int x) const { return d[(size_t)y * W + x]; }
    size_t size() const { return d.size(); }
    bool same_shape(const Plane& o) const { return H == o.H && W == o.W; }
};

enum Mode { REFLECT = 0, NEAREST = 1 };

// index mapping of the borders: "reflect" (scipy) == "symmetric" (numpy): period 2n; "nearest": clamp
long ext_index(long i, long n, Mode m);

// scipy.ndimage.gaussian_filter(a, 1.0) on a float32 plane (mode "reflect", truncate 4.0)
Plane gauss1(const Plane& a);
// scipy.ndimage.uniform_filter(a, size, mode=mode) on a float32 plane
Plane uniform(const Plane& a, int size, Mode mode);
// scipy.ndimage.map_coordinates(img, [cy, cx], order=1, mode="nearest") with float32 coordinate
// planes (cy, cx of one shape; output float32 of that shape)
Plane map_coords1(const Plane& img, const Plane& cy, const Plane& cx);
// scipy.ndimage.median_filter(a, 3, mode="nearest")
Plane median3(const Plane& a);
// numpy.interp(x (float32 -> double), xp, fp) -> double, then .astype(float32) by the caller
struct Interp {
    std::vector<double> xp, fp, slopes;
    Interp(const double* xp_, const double* fp_, int n);
    double operator()(double x, bool use_slopes) const;
};
// nvfilter.curve_sigma: np.interp(lum, centers, curve).astype(float32)
Plane curve_sigma(const Plane& lum, const double* centers, const double* curve, int n);

// nvfilter._b3(a, step, axis) and atrous(img, levels)
Plane b3(const Plane& a, int step, int axis);
std::vector<Plane> atrous(const Plane& img, int levels);
// nvfilter._down2, _fit, _to_grid
Plane down2(const Plane& a);
Plane fit(const Plane& a, int H, int W);
Plane to_grid(const Plane& a, int H, int W);

}  // namespace nvn
