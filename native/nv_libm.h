// nv_libm.h - the libm entry points of the ports (W2a).
//
// The ports call five libm functions through volatile function pointers (no constant folding, no
// pow(x, 2) -> x*x): pow (nv_curve, nv_aft), tanf (nv_linekernel), atan2 and __sincos_stret (nv_model),
// expf (nv_ndimage).  NV_LIBM(sys, alt) is the initialiser of those pointers:
//   default build      : &::sys, the system libm (libSystem on macOS), exactly as before W2a;
//   -DNV_LIBM_ALT build: &nvlibm::alt, the dispatcher of libm_alt.cpp (`make libm-alt`), which picks the
//                        system libm, openlibm, musl or a naive implementation at run time (NV_LIBM_ALT).
#pragma once

#ifdef NV_LIBM_ALT
#include "libm_alt.h"
#define NV_LIBM(sys, alt) (&::nvlibm::alt)
#else
#define NV_LIBM(sys, alt) (&::sys)
#endif
