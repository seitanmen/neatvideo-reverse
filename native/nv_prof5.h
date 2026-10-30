// nv_prof5 - the Python part of the UI's profile building (repro/nvfilter.py build_profile_v5 with the
// FindFlatArea step of repro/nv_native.find_flat_area, repro/nv_ui_qt/backend.noisiest_frame /
// hh1_sigma, repro/nv_ui_qt/state.effective_profile, nvfilter._wn_band_std) in C++, bit-exact with numpy
// 2.1.3 / scipy 1.15.3 on this machine (native/verify_prof5.py, native/verify_bridge_prof5.py).
//
// numpy semantics reproduced (each checked through the tests): float32 elementwise ops with Python
// scalars cast to float32 (NEP 50), np.mean (float32 pairwise sum + identity, then float32(sum / n) in
// double), np.median (exact order statistics, mean of the two middle values), np.percentile 'linear'
// (q / float32(100), virtual index (n - 1) * q in float32, _lerp), np.polyfit (nv_lstsq), np.interp
// (nvn::Interp), scipy uniform_filter / gaussian_filter (nv_ndimage, plus sigma 2.0 here),
// np.gradient, nvfilter.atrous (nvn::atrous).
#pragma once

#include <functional>
#include <string>
#include <vector>

namespace nvp5 {

// (Y, Cb, Cr) float32 planes of one frame: Y H x W, chroma Hc x Wc (4:2:0 or 4:4:4)
struct Frame {
    const float* p[3];
    int H, W, Hc, Wc;
};

enum Mode { MODE_AUTO = 0, MODE_SPATIAL = 1, MODE_TEMPORAL = 2 };

// prof of build_profile_v5 (backend.build_profile without "ui_frame").  Python key order:
// version, sub, full_range, area, mode, levels, band_ratio, tot_ratio, area_sigma, curve, curve_centers,
// flat_status, [static_fraction], dead_fraction, area_level, sigma; the per-plane dicts in Y, Cb, Cr order.
struct Profile {
    int sub = 1;
    bool full_range = false;
    int area_n = 3;                    // 3: [y0, x0, size], 4: [y0, x0, h, w]
    int area[4] = {0, 0, 0, 0};
    int mode = MODE_SPATIAL;           // the "mode" string: "spatial" / "auto" / "temporal" ...
    bool noise_free = false;           // ... + "+noise-free"
    int levels[3] = {0, 0, 0};
    std::vector<double> band_ratio[3];
    double tot_ratio[3] = {0, 0, 0};
    std::vector<double> area_sigma[3];
    std::vector<double> curve[3];
    std::vector<double> centers;
    int flat_status = -1;              // -1 = None (manual area)
    bool has_static = false;
    double static_fraction = 0.0;
    double dead_fraction = 0.0;
    double area_level = 0.0;
    double sigma[3] = {0, 0, 0};
    std::string mode_name() const;
};

using Cancel = std::function<bool()>;

// Agreement with the Python code holds for finite planes (the UI's uint8 / normalised high-bit frames); with
// NaN / inf in the planes Python propagates NaN into area_sigma / band_ratio / curve where this port returns finite
// values (coord_n3a/VERDICT_n3a.md L1: 8 of 21 non-finite cases differ).  nv_depth.sanitize keeps such values out
// of the processing path.

enum Status { OK = 0, CANCELLED = 5, BAD_ARGUMENT = -2 };

// backend.build_profile's call of build_profile_v5 (native core on; BAD_ARGUMENT for H or W < 2, where Python
// raises ValueError): area_n 0 = FindFlatArea (nv_native.
// find_flat_area: nvc_flat_area on the CLIP_FINITE RGB from 64 px, else / on status 1 / a side < 8
// find_flat_area_gradient), 3 = square (y0, x0, size), 4 = rectangle (y0, x0, h, w).  pair: the next frame
// (or null), more: the following frames (more[0] == pair in the UI).  matrix: nvu::Matrix.
int build_profile_v5(const Frame& f, const Frame* pair, const Frame* more, int n_more, bool full_range,
                     int matrix, int mode, int levels, int area_n, const int* area, int threads,
                     const Cancel& cancel, Profile* out);

// nvfilter.find_flat_area_gradient(planes, full_range, matrix=matrix) -> (y0, x0, size)
void flat_area_gradient(const Frame& f, bool full_range, int matrix, int out3[3]);

// nvfilter._wn_band_std(levels)
std::vector<double> wn_band_std(int levels);

// float(np.mean(a)) of a contiguous float32 array of n values (N3b: the mask mean of Noise Level Check)
float mean_f32(const float* a, size_t n);

// backend.hh1_sigma(Y) (also the score of noisiest_frame); false when H or W < 2 (None)
bool hh1_sigma(const float* Y, int H, int W, double* out);

// the candidate frames of backend.noisiest_frame(frames) for n frames (sorted, distinct)
std::vector<int> noisiest_candidates(int n);

// state.effective_profile's curve and sigma: centers / curve (3 x n, Y, Cb, Cr), factors 3 x 9, tune
// (all, Y, Cb, Cr).  Returns false when effective_profile returns base unchanged (then the outputs are
// not written).
bool effective_curve(const double* centers, int n, const double* curve, const double* factors,
                     const double* tune, double area_level, const double* tot_ratio, double* curve_out,
                     double* sigma_out);

}  // namespace nvp5
