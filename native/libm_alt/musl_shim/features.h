/* W2a: stand-in for musl's src/include/features.h (the internal macros the math sources use) on a
   non-musl host; musl's public headers are not used (the system <math.h>, <stdint.h>, <float.h> are). */
#ifndef NV_MUSL_FEATURES_H
#define NV_MUSL_FEATURES_H
#define weak __attribute__((__weak__))
#define hidden __attribute__((__visibility__("hidden")))
#endif
