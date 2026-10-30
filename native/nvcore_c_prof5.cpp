// nvcore_c_prof5 - see nvcore_c_prof5.h.  Argument unpacking only; exceptions do not cross the ABI.
#include "nvcore_c_prof5.h"

#include "nv_prof5.h"

namespace {

nvp5::Frame frame(const float* const* p, int H, int W, int Hc, int Wc) {
    nvp5::Frame f;
    for (int c = 0; c < 3; ++c) f.p[c] = p[c];
    f.H = H;
    f.W = W;
    f.Hc = Hc;
    f.Wc = Wc;
    return f;
}

}  // namespace

extern "C" {

int nvc_build_profile_v5(const float* const* planes, int H, int W, int Hc, int Wc, const float* const* pair,
                         int n_more, const float* const* more, int full_range, int matrix, int mode, int levels,
                         int area_n, const int* area, int threads, nvc_p5_cancel_fn cancel, int* out_i,
                         double* out_d) {
    try {
        // a luma side under 2 px: Python raises ValueError (np.gradient of the FindFlatArea fallback, or _b3 of an
        // empty chroma crop), the port is not defined there
        if (!planes || H < 2 || W < 2 || Hc < 1 || Wc < 1 || n_more < 0 || (n_more > 0 && !more) ||
            mode < 0 || mode > 2 || levels < 2 || levels > 5 || (area_n != 0 && area_n != 3 && area_n != 4) ||
            (area_n != 0 && !area))
            return -2;
        const nvp5::Frame f = frame(planes, H, W, Hc, Wc);
        nvp5::Frame pf;
        if (pair) pf = frame(pair, H, W, Hc, Wc);
        std::vector<nvp5::Frame> mf;
        for (int k = 0; k < n_more; ++k) mf.push_back(frame(more + 3 * k, H, W, Hc, Wc));
        nvp5::Cancel cb;
        if (cancel) cb = [cancel]() { return cancel() != 0; };
        nvp5::Profile P;
        const int st = nvp5::build_profile_v5(f, pair ? &pf : nullptr, mf.data(), n_more, full_range != 0, matrix,
                                              mode, levels, area_n, area, threads, cb, &P);
        if (st != 0) return st;
        for (int i = 0; i < NVC_P5_I; ++i) out_i[i] = 0;
        for (int i = 0; i < NVC_P5_D; ++i) out_d[i] = 0.0;
        out_i[0] = P.area_n;
        for (int i = 0; i < 4; ++i) out_i[1 + i] = P.area[i];
        out_i[5] = P.mode;
        out_i[6] = P.noise_free ? 1 : 0;
        for (int c = 0; c < 3; ++c) out_i[7 + c] = P.levels[c];
        out_i[10] = P.flat_status;
        out_i[11] = P.has_static ? 1 : 0;
        out_i[12] = P.sub;
        out_i[13] = P.full_range ? 1 : 0;
        out_i[14] = static_cast<int>(P.centers.size());
        for (int c = 0; c < 3; ++c) {
            for (size_t j = 0; j < P.band_ratio[c].size() && j < 5; ++j) out_d[5 * c + j] = P.band_ratio[c][j];
            out_d[15 + c] = P.tot_ratio[c];
            for (size_t j = 0; j < P.area_sigma[c].size() && j < 5; ++j) out_d[18 + 5 * c + j] = P.area_sigma[c][j];
            for (int i = 0; i < 8; ++i) out_d[33 + 8 * c + i] = P.curve[c][i];
            out_d[68 + c] = P.sigma[c];
        }
        for (int i = 0; i < 8; ++i) out_d[57 + i] = P.centers[i];
        out_d[65] = P.static_fraction;
        out_d[66] = P.dead_fraction;
        out_d[67] = P.area_level;
        return 0;
    } catch (...) {
        return -1;
    }
}

int nvc_wn_band_std(int levels, double* out) {
    try {
        if (levels < 1 || levels > 8) return -2;
        const std::vector<double> w = nvp5::wn_band_std(levels);
        for (int j = 0; j < levels; ++j) out[j] = w[j];
        return 0;
    } catch (...) {
        return -1;
    }
}

int nvc_hh1_sigma(const float* Y, int H, int W, double* out) {
    try {
        return nvp5::hh1_sigma(Y, H, W, out) ? 1 : 0;
    } catch (...) {
        return -1;
    }
}

int nvc_noisiest_candidates(int n, int* out) {
    try {
        const std::vector<int> c = nvp5::noisiest_candidates(n);
        for (size_t i = 0; i < c.size(); ++i) out[i] = c[i];
        return static_cast<int>(c.size());
    } catch (...) {
        return -1;
    }
}

int nvc_effective_curve(const double* centers, int n, const double* curve, const double* factors,
                        const double* tune, double area_level, const double* tot_ratio, double* curve_out,
                        double* sigma_out) {
    try {
        if (n < 1) return -2;
        return nvp5::effective_curve(centers, n, curve, factors, tune, area_level, tot_ratio, curve_out, sigma_out)
                   ? 1
                   : 0;
    } catch (...) {
        return -1;
    }
}

}  // extern "C"
