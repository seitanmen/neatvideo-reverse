// libm_alt.h - alternative libm implementations for the libm sensitivity harness (W2a).
//
// Only the -DNV_LIBM_ALT build (`make -C native libm-alt`, binaries in native/libm_alt/bin/) uses this;
// the default build calls the system libm directly (nv_libm.h).
//
// Run-time selection (read once, at the first call):
//   NV_LIBM_ALT=0|1|2|3     0 / unset: system libm (the plumbing check: must equal the default build)
//                           1: openlibm 0.8.8  (native/openlibm/, FreeBSD msun / FDLIBM 5.3)
//                           2: musl 1.2.6      (native/musl/, ARM optimized-routines pow / expf)
//                           3: naive formulas in double (arm64 macOS: long double == double), see libm_alt.cpp
//   NV_LIBM_ALT_FUNCS=list  comma separated subset of pow,tanf,atan2,sincos,expf (default: all); the
//                           functions not listed stay on the system libm (to attribute a difference)
//   NV_LIBM_LOG=dir         record the distinct arguments of every call; written at exit to
//                           dir/libm_args_<pid>.bin (read by native/libm_alt/libm_ulp)
#pragma once

#include <math.h>   // struct __double2 (libSystem)

namespace nvlibm {

double pow(double x, double y);
float tanf(float x);
double atan2(double y, double x);
struct __double2 sincos_stret(double x);   // {sin(x), cos(x)}, as libSystem's __sincos_stret
float expf(float x);

// The implementations by index (0 system, 1 openlibm, 2 musl, 3 naive), for libm_ulp.
double pow_impl(int alt, double x, double y);
float tanf_impl(int alt, float x);
double atan2_impl(int alt, double y, double x);
struct __double2 sincos_impl(int alt, double x);
double sin_impl(int alt, double x);   // the library's sin / cos (to check sincos == (sin, cos))
double cos_impl(int alt, double x);
float expf_impl(int alt, float x);

}  // namespace nvlibm
