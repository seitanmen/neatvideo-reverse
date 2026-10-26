// nv_spatial - C++ port of the v5 spatial kernel of repro/nvfilter.py (N1h):
//   spatial_v5(planes, prof, rv, P)  and the Mix step of process_frame_v5.
// Bit-exact with the Python code under numpy 2.1.3 / scipy 1.15.3 (see nv_ndimage.h), provided
// the profile and parameter values are Python floats / ints (numpy's weak scalars: they are cast
// to float32 before each operation).  A numpy float64 scalar in P or prof would promote the
// arrays to float64 in Python (NEP 50) and is not reproduced.
#pragma once

#include <vector>

#include "nv_ndimage.h"

namespace nvs {

using nvn::Plane;

// V5_BANDS order: 0 high, 1 medium, 2 low, 3 very-low, 4 ultra-low
constexpr int NBANDS = 5;

struct V5Profile {
    int ncurve = 8;
    double centers[16] = {};      // prof["curve_centers"]
    double curve[3][16] = {};     // prof["curve"]["Y" / "Cb" / "Cr"]
    int levels[3] = {5, 5, 5};    // prof["levels"]
    double band_ratio[3][NBANDS] = {};
    double tot_ratio[3] = {1, 1, 1};
};

struct V5Params {
    int radius = 2;
    double temporal = 1.0, threshold = 4.0, spatial = 2.0, luma = 0.7, chroma = 1.0;
    double band_amounts[NBANDS] = {1, 1, 1, 1, 0};
    double band_noise[NBANDS] = {1, 1, 1, 1, 1};
    double noise_level = 1.0;
    double channel_noise[3] = {1, 1, 1};
    int similarity = 0;           // 0 "v5", 1 "pyramid"
    double pyramid_scale = 1.5;
    int bs = 16, search = 16;
    double mix = 0.0;
};

// spatial_v5: planes = (Y, Cb, Cr) float32 (chroma on any grid), rv = residual variance per plane
std::vector<Plane> spatial_v5(const Plane planes[3], const V5Profile& prof, const Plane rv[3],
                              const V5Params& P);
// process_frame_v5's Mix: out = o + m * (src - o) when m > 0 (in place)
void mix_v5(std::vector<Plane>& out, const Plane src[3], double m);

}  // namespace nvs
