// nv_band - C++ port of the 5-level Laplacian band noise measure.
//
// Source of truth: repro/nv_band.py (bit-exact numpy port of the genuine FUN_009980e8 and the
// level part of FUN_00995250), with its helpers from repro/nv_lines.py (running box filters,
// FUN_009944c0 axis-aligned fast paths) and repro/nv_pixnoise.py (2x2 mean FUN_009f15b4,
// (3,1)/4 up-sampling FUN_00993df0 / FUN_00990da8, fma32).  The SIMD build (DAT_014a67a8 = 1,
// nv_lines.SIMD = True) is the only one ported.
//
// native/verify_band.py checks the outputs word for word against the Python port.
//
// Rules: float32 arithmetic in the same order as the Python port; no fast-math; no FMA
// contraction (-ffp-contract=off).  Explicit fusions (FMLA of FUN_009ef520 and FUN_009ee3fc)
// use std::fma.

#pragma once

#include <cstddef>
#include <cstdint>

namespace nvb {

enum : int {
    LEVELS = 5,
    STRUCT_WORDS = 0x4000 / 4,   // what `nvh levels|model` dumps
    MODEL_WORDS = 0x1f54 / 4,    // the part FUN_009979fc initialises
};

// The struct after FUN_009979fc (INIT_RUNS; zero from MODEL_WORDS on).  out: STRUCT_WORDS words.
void init_struct(uint32_t* out);

// FUN_00995250 level part (nv_band.model_levels): words 0..54 as the genuine writes them; the
// rest keep the init pattern.  plane: H x W row-major float32.  out: STRUCT_WORDS words.
void model_levels(const float* plane, int W, int H, uint32_t* out, int levels = LEVELS);

// FUN_009980e8 (nv_band.band_levels): the whole 4096-word struct.
void band_levels(const float* plane, int W, int H, uint32_t* out, int levels = LEVELS);

// ---- parts (exposed for the later ports and for the verification driver) ----

// FUN_009ef3fc mean (nv_band.mean_exact) and FUN_009ef520 std (nv_band.std_exact).
float mean_exact(const float* v, size_t n);
float std_exact(const float* v, size_t n, float m);

// FUN_00997bcc (nv_band.quantile) for q < 1 (q >= 1 = FUN_009efdd4 maximum is not ported: the
// function returns false).  m / sd: pass nullptr to compute them as the Python port does.
bool quantile(const float* v, size_t n, float q, int flag, int div, float* result,
              const float* m = nullptr, const float* sd = nullptr);

// FUN_009d3e40 (nv_band.probit).
float probit(float q);

// FUN_00997f88 (nv_band.cv_bm for one plane): 8 words (NEG_1E6 where nothing is written).
void cv(const float* E, int w, int h, float m, float* out8);

// nv_lines.box_rows / box_cols (radius r, mode 0 = border replicated, 1 = zero padded) on an
// H x W plane; out must not overlap X.
void box_rows(const float* X, int W, int H, int r, int mode, float* out);
void box_cols(const float* X, int W, int H, int r, int mode, float* out);

// nv_pixnoise.down2 (FUN_009f15b4 on every row pair): out is (H >> 1) x (W >> 1).
void down2(const float* x, int W, int H, float* out);

// out = src - up2(lo) (nv_band._band_into / nv_pixnoise.band_from); lo is Hc x Wc, src and out
// are (2Hc + oddH) x (2Wc + oddW) with W2, H2 the src size.
void band_from(const float* lo, int Wc, int Hc, const float* src, int W2, int H2, float* out);

}  // namespace nvb
