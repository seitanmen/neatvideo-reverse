// nvcore_c_rgb - see nvcore_c_rgb.h.  Argument unpacking only; exceptions do not cross the ABI.
#include "nvcore_c_rgb.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include "nv_aft.h"
#include "nv_band.h"

namespace {

void unpack(const double* cw9, const double* levels27, double cw[3][3], double lv[3][9]) {
    for (int c = 0; c < 3; ++c)
        for (int j = 0; j < 3; ++j) cw[c][j] = cw9[3 * c + j];
    for (int j = 0; j < 3; ++j)
        for (int k = 0; k < 9; ++k) lv[j][k] = levels27[9 * j + k];
}

}  // namespace

extern "C" {

int nvc_rgb_weights(const float* rgb, int H, int W, const int* area4, double* cw9) {
    try {
        if (!rgb || !area4 || !cw9 || H < 1 || W < 1) return -2;
        // rgb[y0:y0 + h, x0:x0 + w] (numpy slicing of non-negative starts: clamped to the frame)
        const int y0 = std::min(std::max(area4[0], 0), H), x0 = std::min(std::max(area4[1], 0), W);
        const int y1 = std::min(std::max(area4[0] + area4[2], y0), H), x1 = std::min(std::max(area4[1] + area4[3], x0), W);
        const int h = y1 - y0, w = x1 - x0;
        if (h < 1 || w < 1) return -2;
        std::vector<float> plane(static_cast<size_t>(h) * w);
        std::vector<uint32_t> st(nvb::STRUCT_WORDS);
        float q99[3];
        for (int j = 0; j < 3; ++j) {
            for (int y = 0; y < h; ++y)
                for (int x = 0; x < w; ++x)
                    plane[static_cast<size_t>(y) * w + x] = rgb[(static_cast<size_t>(y0 + y) * W + (x0 + x)) * 3 + j];
            nvb::model_levels(plane.data(), w, h, st.data());
            std::memcpy(&q99[j], &st[5], 4);                 // word 5: the finest band's q99
        }
        double cw[3][3];
        nva::color_weights(q99, cw);
        for (int c = 0; c < 3; ++c)
            for (int j = 0; j < 3; ++j) cw9[3 * c + j] = cw[c][j];
        return 0;
    } catch (...) {
        return -1;
    }
}

int nvc_rgb_effective_curve(const double* centers, int n, const double* src, const double* cw9, const double* levels27,
                            const double* tune4, double area_level, const double* tot_ratio, double* curve_out,
                            double* sigma_out) {
    try {
        if (!centers || n < 1 || !src || !cw9 || !levels27 || !tune4 || !tot_ratio || !curve_out || !sigma_out)
            return -2;
        double cw[3][3], lv[3][9];
        unpack(cw9, levels27, cw, lv);
        std::vector<float> s[3], out[3];
        const float* sp[3];
        double tk[3];
        for (int c = 0; c < 3; ++c) {
            s[c].resize(n);
            for (int i = 0; i < n; ++i) s[c][i] = static_cast<float>(src[c * n + i]);   // np.array(src, float32)
            sp[c] = s[c].data();
            tk[c] = tune4[0] * tune4[1 + c];
        }
        double sig[3];
        nva::rgb_effective(centers, n, sp, cw, lv, tk, area_level, tot_ratio, out, sig);
        for (int c = 0; c < 3; ++c) {
            for (int i = 0; i < n; ++i) curve_out[c * n + i] = static_cast<double>(out[c][i]);
            sigma_out[c] = sig[c];
        }
        return 0;
    } catch (...) {
        return -1;
    }
}

int nvc_rgb_combine(const double* centers, int n, const double* curve, const double* cw9, const double* levels27,
                    double* face_out, double* curve_out) {
    try {
        if (!centers || n < 1 || !curve || !cw9 || !levels27 || !curve_out) return -2;
        double cw[3][3], lv[3][9];
        unpack(cw9, levels27, cw, lv);
        std::vector<float> s[3], out[3];
        std::vector<double> face[3];
        const float* sp[3];
        for (int c = 0; c < 3; ++c) {
            s[c].resize(n);
            for (int i = 0; i < n; ++i) s[c][i] = static_cast<float>(curve[c * n + i]);
            sp[c] = s[c].data();
        }
        nva::combine_levels(cw, lv, centers, n, sp, face, out);
        for (int c = 0; c < 3; ++c)
            for (int i = 0; i < n; ++i) {
                curve_out[c * n + i] = static_cast<double>(out[c][i]);
                if (face_out) face_out[c * n + i] = face[c][i];
            }
        return 0;
    } catch (...) {
        return -1;
    }
}

}  // extern "C"
