// nv_aft - C++ port of the AutoFineTune body (FUN_009a775c, worker FUN_009b56f4) and of the
// colour combination that repro/nv_aft_v5.apply_aft applies to the v5 curves.
//
// Source of truth: neatvideo_ofx_614/analysis/noise/aft_port/src/nv_aft.py (a static port that was
// compared with the genuine only for the mode-2 tile order; see its README) and repro/nv_aft_v5.py.
// The Python implementation is the reference here; native/verify_aft.py checks every output byte /
// word BIT FOR BIT against it.
//
// Parts reused from the earlier ports: nvb::mean_exact / nvb::quantile (N1a), nvc::is_clip_genuine
// (N1b), nvm::model_full (N1e) and nvn::Interp (numpy.interp, N1g).
//
// Build with -ffp-contract=off -fno-fast-math.  On AArch64 the float operations whose operands can
// both be NaN are inline asm in the operand order of the Python side (numpy float32 scalars: a + b
// and a * b evaluate as (b, a); CPython floats and numpy arrays: (a, b)).
#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace nva {

enum : int {
    PROF = 0xc300,          // profile size
    MODEL = 0x1f54,         // one plane model in the profile
    NWORDS = 0x4000 / 4,    // nvm::model_full output
};

enum Status : int {
    OK = 0,            // tiles ran, the curves were post-processed
    TOO_SMALL = 1,     // W or H < 48: _run_tiles returns False (only byte 0x2d is written)
    CANCELLED = 2,     // the cancel query answered true (_run_tiles returns None; only 0x2d)
    MODEL_ERROR = 3,   // nvm::model_full failed (the Python port would raise)
};

// Per channel (R, G, B) x 9 brightness bins: the ratios _run_tiles appends, in tile order.
using Bins = std::vector<float>[3][9];

struct Diag {   // the counters of nv_aft._run_tiles(diag=...)
    int ntiles = 0, clip_skip = 0, simG_skip = 0, simR_skip = 0, simB_skip = 0, diff_skip = 0, accepted = 0;
};

// FUN_009d49b4(x, s, -0.25f) (nv_aft._penalty)
float penalty(float x, float s);

// FUN_009b6338 (nv_aft.similarity): t = tile model (>= 492 words), p = profile model (>= 492 words),
// stats = 51 words of tile_dev.
float similarity(const float* t, const float* p, const float* stats, float s_lvl, float s_mult);

// FUN_009d39ec (nv_aft.shuffle_order)
std::vector<int> shuffle_order(int n);

// nv_profile.curve_bin (the AFT brightness bin, 0..8)
int curve_bin(float mean);

// FUN_009d3e8c (nv_aft.polyfit_aft; ncoef >= 1) and FUN_009d4890 (nv_aft.polyval_aft)
std::vector<double> polyfit_aft(const float* xs, const float* ys, int npts, int ncoef);
double polyval_aft(double x, const double* c, int ncoef);

// The parent's post-processing (nv_aft._finish) on a 0xc300-byte profile, in place.
// mode < 2: FUN_009b4e08 x 3 then FUN_009bdf30(0.2, 3.0, 0) x 3; mode 2: FUN_009b5408 x 3 into out3.
void finish(uint8_t* prof, const Bins& bins, int mode, float out3[3]);

// The cancel query of nv_aft._run_tiles (`cancel`; true = cancelled).  Empty: never asked.
using Cancel = std::function<bool()>;

// FUN_009b56f4 one-thread behaviour (nv_aft._run_tiles, fast = False).  rgb: H x W x 3 float32
// (HWC); image2: the same layout or nullptr.  cancel_at: the 1-based index of the cancel query that
// answers true (< 1: never).  threads > 1 runs the tiles in parallel; the bins are still appended in
// slot order and the mode-2 skip is decided in that order, so the result does not depend on it.
// cancel (used when cancel_at < 1): asked before each tile is taken, as nv_aft._run_tiles asks it
// (ntiles queries without a cancel; none after the first true answer).  With threads > 1 the query and
// the taking of the tile are one step under a lock (the same number of queries as one thread).  A true
// answer gives CANCELLED (the tiles taken before it run and count in diag; the bins are dropped).
int run_tiles(const float* rgb, int W, int H, const uint8_t* prof, int mode, const float* image2,
              long cancel_at, int threads, Bins* bins, Diag* diag, const Cancel& cancel = Cancel());

// nv_aft.auto_fine_tune: prof (0xc300 bytes) is rewritten in place (curves + byte 0x2d).
// out3 receives the three FUN_009b5408 values for mode 2 with status OK (else untouched).
// bins_out (optional) receives the bins of _run_tiles.
int auto_fine_tune(const float* rgb, int W, int H, uint8_t* prof, int mode, const float* image2,
                   long cancel_at, int threads, float out3[3], Diag* diag, Bins* bins_out = nullptr,
                   const Cancel& cancel = Cancel());

// repro/nv_aft_v5.apply_aft after the AFT call: the colour weights from the raw profile's model q99,
// the 9 curve words of each face interpolated onto the v5 centres, f_c = sqrt(max(sum_j cw_cj f_j, 0)),
// the v5 curves multiplied by f_c, the sigma estimate and the bin flags.
struct Combine {
    double cw[3][3];                    // prof["aft_color_weights"]
    std::vector<double> face[3];        // prof["aft_face_curves"]
    std::vector<float> curve[3];        // prof["curve"] Y, Cb, Cr after the product
    double sigma[3];                    // prof["sigma"] Y, Cb, Cr
    uint8_t marker[3][9], clip[3][9], gt3[3][9];   // prof["aft_bin_flags"]
};
void combine(const uint8_t* raw, const uint8_t* aft, const double* centers, int n, const float* const curves[3],
             double area_level, const double tot_ratio[3], int band, Combine* out);

// the two parts of combine: _color_weights of the faces' model q99 (a_j), and the levels -> curves step with the
// 9 levels of each face given (face[j] = prof["aft_face_curves"][j], curve_out[c] = the float32 curves).
void color_weights(const float q99[3], double cw[3][3]);
void combine_levels(const double cw[3][3], const double levels[3][9], const double* centers, int n,
                    const float* const curves[3], std::vector<double> face[3], std::vector<float> curve_out[3]);

// G1 (qtui state rgb_effective_profile, computed path): combine_levels on the curve src (before AFT), each curve then
// times float32(tk[c]) in float32, sigma = interp(area_level, centres, curve) * tot_ratio.
void rgb_effective(const double* centers, int n, const float* const src[3], const double cw[3][3],
                   const double levels[3][9], const double tk[3], double area_level, const double tot_ratio[3],
                   std::vector<float> curve_out[3], double sigma[3]);

}  // namespace nva
