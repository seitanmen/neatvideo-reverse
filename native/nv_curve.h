// nv_curve - C++ port of repro/nv_curve.py + nv_pixnoise.py + nv_lines.py
// (EstimateNoiseLevelCurve FUN_009b6c8c, its per-pixel noise FUN_00998d98, the line kernels
// FUN_00990448 / FUN_009944c0, IsClip FUN_009c6f88 and the interpolator FUN_00aedb18).
//
// Source of truth: the Python port (bit-exact against the genuine through the harness).
// This port reproduces the Python reference path (tile_histogram, pixnoise, linefilt) op for op
// in float32 / float64; native/verify_curve.py checks it BIT FOR BIT against genuine_noise_curve.
// Build with -ffp-contract=off -fno-fast-math (no FMA contraction, no reassociation).
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace nvc {

constexpr int NBINS = 8;      // DAT_012445a0
constexpr int TILE = 96;      // DAT_01244570
constexpr int MARGIN = 12;    // DAT_01244578
constexpr int MIN_TILE = 48;  // FUN_00ac1194 arg 8 (0x30)
constexpr int BLOCK = 16;     // DAT_01244580

struct CurveResult {
    double xs[NBINS];
    double ys[NBINS];
    double sums[NBINS];     // per-bin double sums of the kept noise values
    int64_t counts[NBINS];  // per-bin pixel counts
};

// rgb: H x W pixels, `stride_c` floats per pixel (>= 3; channels 0..2 = R, G, B), row-major.
// Returns false (and leaves *out untouched) when W or H < 48 (the genuine scheduler asserts).
// threads > 1 processes tiles in parallel; the double sums are still added in scheduler order
// (tile order, row-major inside the tile), so the result does not depend on `threads`.
bool genuine_noise_curve(const float* rgb, int W, int H, int stride_c, CurveResult* out, int threads = 1);

// FUN_009c6f88 (IsClip of a float plane of n values, row-major as nv_curve.is_clip_genuine ravels it).
float is_clip_genuine(const float* v, size_t n);

// FUN_00998d98 on one tile (debug / verification entry): luma W x H, weight in/out (W*H).
// noise and bright receive W*H floats each.
void pixnoise(const float* luma, int W, int H, float* weight, float* noise, float* bright);

// Tile descriptors of FUN_00abfc50 / FUN_00ac03c0 (11 ints each, as nv_curve.tile_layout).
std::vector<std::array<int, 11>> tile_layout(int W, int H);

// BaseInterpolator (FUN_00aeda08 / FUN_00aedb18): nv_curve.CurveInterp / noise_at.
class CurveInterp {
public:
    CurveInterp(const double* xs, const double* ys, int n);  // n >= 2 (caller checks)
    double operator()(double x) const;

private:
    std::vector<double> xs_, ys_;
    int n_;
    double lo_[2], hi_[2];
};

}  // namespace nvc
