// nvcore_c_ui - see nvcore_c_ui.h.  Thin wrappers of nv_ui.h; C++ exceptions do not cross the ABI.
#include "nvcore_c_ui.h"

#include <cstring>
#include <vector>

#include "nv_ndimage.h"
#include "nv_prof5.h"
#include "nv_profile.h"
#include "nv_spatial.h"
#include "nv_ui.h"

extern "C" {

void nvc_yuv_to_rgb(const float* Y, const float* Cb, const float* Cr, int H, int W, int Hc, int Wc, int matrix,
                    int full_range, int clip, float* out) {
    nvu::yuv_to_rgb(Y, Cb, Cr, H, W, Hc, Wc, matrix, full_range != 0, clip, out);
}

void nvc_minmax(const float* v, size_t n, double* mn, double* mx) { nvu::minmax(v, n, mn, mx); }

void nvc_render_simple(int name, const float* rgb, int H, int W, int b, double mn, double mx, uint8_t* out) {
    nvu::render_simple(name, rgb, H, W, b, mn, mx, out);
}

void nvc_freq_images(const float* rgb, int H, int W, int kind, int emphasis, int16_t* out16, float* levels6) {
    try {
        nvu::freq_images(rgb, H, W, kind, emphasis != 0, out16, levels6);
    } catch (...) {
        for (int i = 0; i < 6; ++i) levels6[i] = 0.0f;
    }
}

void nvc_render_freq(int range_kind, const int16_t* p16, int H, int W, float L, float Hh, int b, uint8_t* out) {
    nvu::render_freq(range_kind, p16, H, W, L, Hh, b, out);
}

double nvc_noise_level(const float* rgb, int h, int w) { return nvu::noise_level(rgb, h, w); }

void nvc_not_uniform(const float* rgb, int h, int w, int* bad3, int* clip3) {
    nvu::not_uniform(rgb, h, w, bad3, clip3);
}

int nvc_quality_numbers(const float* rgb, int H, int W, const int* area4, const double* factors27,
                        const double* tune4, int aft, int reset, const uint8_t* flags81, int* q_pct, int* c_pct) {
    try {
        return nvu::quality_numbers(rgb, H, W, area4, factors27, tune4, aft != 0, reset != 0, flags81, q_pct, c_pct)
                   ? 1 : 0;
    } catch (...) {
        return -1;
    }
}

void nvc_mix(float* const* out, const float* const* src, int H, int W, int Hc, int Wc, double m) {
    std::vector<nvn::Plane> o;
    nvn::Plane s[3];
    for (int c = 0; c < 3; ++c) {
        const int h = c ? Hc : H, w = c ? Wc : W;
        o.emplace_back(h, w);
        s[c] = nvn::Plane(h, w);
        std::memcpy(o[c].d.data(), out[c], sizeof(float) * o[c].size());
        std::memcpy(s[c].d.data(), src[c], sizeof(float) * s[c].size());
    }
    nvs::mix_v5(o, s, m);
    for (int c = 0; c < 3; ++c) std::memcpy(out[c], o[c].d.data(), sizeof(float) * o[c].size());
}

int nvc_reference_floor(const char* npy_path, int threads, float* out51) {
    try {
        if (npy_path && nvp::load_reference_floor(npy_path, out51)) return 1;
        return nvp::compute_reference_floor(out51, threads < 1 ? 1 : threads) == 0 ? 2 : 0;
    } catch (...) {
        return 0;
    }
}

void nvc_temporal_mask(const float* rv, int H, int W, int n, double a, float* out) {
    nvu::temporal_mask(rv, static_cast<size_t>(H) * W, n, a, out);
}

void nvc_nlc_image(const float* rgb, const float* m, int H, int W, float* out) { nvu::nlc_image(rgb, m, H, W, out); }

void nvc_interp(const double* xp, const double* fp, int n, const double* x, int m, double* out) {
    if (n < 1) return;
    nvu::np_interp(xp, fp, n, x, m, out);
}

float nvc_mean_f32(const float* a, size_t n) { return nvp5::mean_f32(a, n); }

}  // extern "C"
