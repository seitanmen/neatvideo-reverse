// nv_portable.cpp - see nv_portable.h (the software FRECPE / FRECPS moved from nv_curve.cpp).
#include "nv_portable.h"

namespace nvp {

// nv_pixnoise.frecpe32 (software FPRecipEstimate, FPCR: RN, FZ = 0, DN = 0, AH = 0)
float frecpe_soft(float x) {
    const int64_t b = ubits(x);
    const int64_t sign = b & 0x80000000LL;
    int64_t exp = (b >> 23) & 0xff;
    const int64_t frac = b & 0x7fffff;
    if (exp == 255 && frac != 0) return bits(static_cast<uint32_t>(b | 0x400000));
    if (exp == 255) return bits(static_cast<uint32_t>(sign));
    if (exp == 0 && frac < (1 << 21)) return bits(static_cast<uint32_t>(sign | 0x7f800000));  // 0 and tiny
    const int64_t mask52 = (int64_t(1) << 52) - 1;
    int64_t fraction = frac << 29;
    if (exp == 0) {
        if ((fraction >> 51) & 1) {
            fraction = (fraction << 1) & mask52;
        } else {
            fraction = (fraction << 2) & mask52;
            exp = -1;
        }
    }
    const int64_t scaled = (1 << 8) | ((fraction >> 44) & 0xff);
    int64_t result_exp = 253 - exp;
    const int64_t est = ((int64_t(1) << 19) / (scaled * 2 + 1) + 1) / 2;
    int64_t fr = (est & 0xff) << 44;
    if (result_exp == 0) fr = (int64_t(1) << 51) | (fr >> 1);
    if (result_exp == -1) {
        fr = (int64_t(1) << 50) | (fr >> 2);
        result_exp = 0;
    }
    return bits(static_cast<uint32_t>(sign | ((result_exp & 0xff) << 23) | (fr >> 29)));
}

// nv_pixnoise.frecps32 (software FPRecipStepFused 2 - a*b, one rounding; inf*0 -> 2)
float frecps_soft(float a, float b) {
    const float na = bits(ubits(a) ^ 0x80000000u);
    if ((std::isinf(na) && b == 0.0f) || (na == 0.0f && std::isinf(b))) return 2.0f;
    if (isnan_(na) || isnan_(b)) return process_nans(na, b);  // addend 2.0 is never NaN
    return std::fma(na, b, 2.0f);
}

}  // namespace nvp
