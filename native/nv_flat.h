// nv_flat - C++ port of repro/nv_flat.py: FindFlatArea (FUN_009b1a5c) on the Auto Profile path
// (param_3 = 0) and its colour transform (DAT_0147df50).
//
// Source of truth: the Python port (bit-exact against the genuine through the harness).  Every
// routine names the Python function it reproduces; native/verify_flat.py compares the result
// (region, status) and the intermediates (reduced planes, luma, the candidate lists of every size,
// placement and IsClip of every candidate, the scored candidates' flatness / E / R / score, and
// optionally the band-measure structs) BIT FOR BIT with the Python port.
//
// Uses nv_band (N1a: band_levels, box_rows / box_cols) and nv_curve (N1b: genuine_noise_curve,
// is_clip_genuine, CurveInterp).  Build with -ffp-contract=off -fno-fast-math.
//
// The cancel query (status 5, nv_flat's `cancel` / `curve` arguments) is ported: see
// find_flat_area_genuine.  Not ported: the trace dict's Python types.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "nv_curve.h"

namespace nvf {

constexpr int NBINS = 16;        // FUN_009999cc nbins as FindFlatArea passes it
constexpr int PER_BIN = 4;
constexpr int NCAND = NBINS * PER_BIN;
constexpr int STRUCT_WORDS = 0x4000 / 4;

// FUN_009b7e58 (nv_flat.reduce_planes): (W/s) x (H/s) block means of R, G, B.
// rgb: H x W pixels with stride_c floats per pixel (channels 0..2 = R, G, B).
void reduce_planes(const float* rgb, int W, int H, int stride_c, int s, float* R, float* G, float* B);

// FUN_009f11fc with the BT.601 weights (nv_pixnoise.luma_601): fused on the first n & ~3 values.
void luma_601(const float* R, const float* G, const float* B, size_t n, float* out);

// FUN_009999cc (nv_flat.candidates) on an H x W luma plane: NBINS * PER_BIN entries
// (-1 / -1.0f after the last candidate).
void candidates(const float* luma, int W, int H, int rx, int ry, int step, int64_t* xs, int64_t* ys,
                float* means, float* scores, int nbins = NBINS, int per_bin = PER_BIN);

// DAT_0147df50[mode] (nv_flat.transform) in place on three planes of n values (one crop plane
// each; the fused / scalar split of mode 1 is at n & ~3).  mode 0 / 3: unchanged.
void transform(float* p0, float* p1, float* p2, size_t n, int mode);

// FUN_009b096c (nv_flat.flatness) on a 4096-word band-measure struct.
float flatness(const uint32_t* st, bool luma);

// nv_flat._level_terms on the three planes' structs: E and R of the score.
void level_terms(const uint32_t* const st[3], float* E, float* R);

// nv_flat._score.
float score(float E, float R, float nz, const float fl[3], float b, int nclip, int penalty, float factor);

// ---- diagnostics (filled when a Diag is passed) ----
struct CandInfo {
    int x0, y0, cw, ch;   // placed square (nv_flat._place)
    float clip[3];        // IsClip of R, G, B
};
struct SizeDiag {
    int size, r, step;
    int64_t xs[NCAND], ys[NCAND];
    float means[NCAND], scores[NCAND];
    std::vector<CandInfo> placed;   // candidates before the first negative score
};
struct Trace {                       // one per scored candidate, as nv_flat's `trace` dicts
    int size, i, x, y, w, h, ncl, pen;
    float mean, nz, fl[3], E, R, score;
    std::vector<uint32_t> structs;   // 3 x STRUCT_WORDS (only with Diag::keep_structs)
};
struct Diag {
    bool keep_structs = false;
    int s = 0, rw = 0, rh = 0, i0 = -1;
    bool curve_run = false, curve_ok = false;
    double curve_xs[nvc::NBINS] = {}, curve_ys[nvc::NBINS] = {};
    std::vector<float> R, G, B, luma;   // reduced planes (rw * rh each)
    std::vector<SizeDiag> sizes;
    std::vector<Trace> trace;
};

struct Result {
    int x, y, w, h, status;          // status as nv_flat (0 ok, 1 nothing found, 2 small, 3 not
                                     // flat, 4 clipped, 5 cancelled); -1 for a mode outside 0..3
};

// find_flat_area_genuine(rgb, noise_at, mode, cancel=..., curve=...).  noise: the interpolator to use,
// or nullptr to run nvc::genuine_noise_curve on rgb where the genuine runs EstimateNoiseLevelCurve
// (after the size table; as nv_flat's `curve` argument), with `threads` threads (the curve's bits do
// not depend on it).  threads > 1 also measures the candidate squares of a size in parallel (the
// selection stays sequential: same result).
// cancel (empty: never asked): with noise == nullptr the curve asks it tiles + 1 times
// (nvc::genuine_noise_curve); a cancel there gives (0, 0, 0, 0, 5).  Then it is asked at the start of
// every size from i0 on; a cancel there gives status 5 with the best square of the earlier sizes as
// it stands (before the cut of sides over 512), or zeros if there is none yet (nv_flat: "outputs as
// written so far").  No query follows the first true answer; nothing is asked on the paths that end
// before the curve (status 1).
Result find_flat_area_genuine(const float* rgb, int W, int H, int stride_c, int mode,
                              const nvc::CurveInterp* noise = nullptr, int threads = 1,
                              Diag* diag = nullptr, const nvc::Cancel& cancel = nvc::Cancel());

}  // namespace nvf
