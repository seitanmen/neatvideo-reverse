/* W2a: <endian.h> for musl's libm.h on macOS (little-endian targets only). */
#ifndef NV_MUSL_ENDIAN_H
#define NV_MUSL_ENDIAN_H
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#define __BYTE_ORDER __LITTLE_ENDIAN
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "big-endian target"
#endif
#endif
