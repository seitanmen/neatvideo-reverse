// nvcore_c_depth - see nvcore_c_depth.h.
#include "nvcore_c_depth.h"

#include "nv_depth.h"

namespace {
nvd::Fmt fmt(int family, int bits, int is_float, int chroma) {
    nvd::Fmt f;
    f.family = family;
    f.bits = bits;
    f.is_float = is_float;
    f.chroma = chroma;
    return f;
}
}  // namespace

extern "C" {

long nvc_depth_sanitize(float* p, long n, long* idx, float* vals) { return nvd::sanitize(p, n, idx, vals); }

int nvc_depth_to_internal(const uint8_t* raw, int family, int bits, int is_float, int chroma, int W, int H, int matrix,
                          float* Y, float* Cb, float* Cr) {
    return nvd::to_internal(raw, fmt(family, bits, is_float, chroma), W, H, matrix, Y, Cb, Cr) ? 0 : -1;
}

int nvc_depth_from_internal(const float* Y, const float* Cb, const float* Cr, int family, int bits, int is_float,
                            int chroma, int W, int H, int matrix, const long* const* ridx, const float* const* rvals,
                            const long* rn, uint8_t* out) {
    return nvd::from_internal(Y, Cb, Cr, fmt(family, bits, is_float, chroma), W, H, matrix, ridx, rvals, rn, out) ? 0 : -1;
}

int nvc_ofx_unpack(const uint8_t* data, int W, int H, int depth, float* Y, float* Cb, float* Cr, long* const* ridx,
                   float* const* rvals, long* rn) {
    return nvd::ofx_unpack(data, W, H, depth, Y, Cb, Cr, ridx, rvals, rn) ? 0 : -1;
}

int nvc_ofx_pack(const float* Y, const float* Cb, const float* Cr, int W, int H, int depth, const long* const* ridx,
                 const float* const* rvals, const long* rn, uint8_t* out) {
    return nvd::ofx_pack(Y, Cb, Cr, W, H, depth, ridx, rvals, rn, out) ? 0 : -1;
}

}  // extern "C"
