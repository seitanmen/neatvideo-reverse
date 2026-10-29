// nv_ui - the numerics of the PySide6 UI (repro/nv_ui_qt) that the C++ Qt UI (qtui/, N3a) needs besides the
// N1 kernels: the RGB conversion of the viewer / profile frame (nvfilter.yuv_to_rgb), the viewer display
// (nv_ui_qt/display.py), the selection measures (display.noise_level / not_uniform) and the Quality numbers
// (nv_ui_qt/quality.py profile_numbers).  Bit-exact with the Python code (native/verify_bridge_ui.py).
//
// Rules as the N1 ports: float32 in the Python operation order (numpy weak scalars cast to float32 before the
// operation), the add / mul / div of nv_ndimage (operand order fixed by inline asm on AArch64), no FMA.
#pragma once

#include <cstddef>
#include <cstdint>

namespace nvu {

enum Matrix { BT709 = 0, BT601 = 1, BT2020 = 2 };

// clip modes of yuv_to_rgb
enum RgbClip {
    CLIP_NONE = 0,     // nvfilter.yuv_to_rgb as is
    CLIP_0_255 = 1,    // np.clip(yuv_to_rgb(..), 0, 255)  (PreviewEngine.to_rgb, quality._curve_blob, run_aft)
    CLIP_FINITE = 2,   // np.clip(np.nan_to_num(yuv_to_rgb(..), nan=0, posinf=255, neginf=0), 0, 255)
                       // (nvfilter.find_flat_area / nv_native.find_flat_area)
};

// nvfilter.yuv_to_rgb((Y, Cb, Cr), matrix, full_range): Y H x W, Cb / Cr Hc x Wc (float32 row-major; a chroma
// plane of another shape than Y is repeated 2 x 2 and cropped / edge-padded to H x W as nvfilter._fit does).
// out: H x W x 3 float32 (R, G, B).
void yuv_to_rgb(const float* Y, const float* Cb, const float* Cr, int H, int W, int Hc, int Wc, int matrix,
                bool full_range, int clip, float* out);

// ------------------------------------------------------------------ display.py
// pane output-range kinds (display.RANGES)
enum RangeKind { RK_Y = 0, RK_R = 1, RK_G = 2, RK_B = 3, RK_CR = 4, RK_CB = 5 };
// simple panes (display.render_simple names)
enum SimplePane { SP_RGB = 0, SP_R = 1, SP_G = 2, SP_B = 3, SP_Y = 4, SP_CR = 5, SP_CB = 6 };
// frequency image kinds (display.freq_images kind)
enum FreqKind { FK_ENHANCED = 0, FK_Y = 1, FK_CR = 2, FK_CB = 3 };

// display.minmax: float(min), float(max) of n float32 values (NaN propagates as numpy's reduction).
void minmax(const float* v, size_t n, double* mn, double* mx);

// display.render_simple(name, rgb, b, (mn, mx)): rgb H x W x 3 float32 -> out H x W x 3 uint8.
void render_simple(int name, const float* rgb, int H, int W, int b, double mn, double mx, uint8_t* out);

// display.freq_images(rgb, kind, emphasis): out16 = 3 int16 planes H x W (Enhanced: Y, Cr, Cb; a channel's
// Frequencies: High, Mid, Low), levels = the display ranges (L, H) x 3 as float32 (the Python floats of
// freq_levels are float32 values).
void freq_images(const float* rgb, int H, int W, int kind, bool emphasis, int16_t* out16, float* levels6);

// display.render_freq for one pane: apply(p16 * 0.0625, *map_b(kind, b, L, Hh)) -> out H x W x 3 uint8.
void render_freq(int range_kind, const int16_t* p16, int H, int W, float L, float Hh, int b, uint8_t* out);

// display.noise_level(rgb_crop) (crop h x w x 3 float32): float(np.sqrt(tot)).
double noise_level(const float* rgb, int h, int w);

// display.not_uniform(rgb_crop): bad[3] = Y, Cr, Cb not uniform (flatness >= 2.0 or NaN), clip[3] = R, G, B
// with IsClip > 0.33.
void not_uniform(const float* rgb, int h, int w, int bad[3], int clip[3]);

// ------------------------------------------------------------------ Noise Level Check (N3b)
// backend.temporal_mask(rv, n, a): rv = the luma residual variance of temporal_v5 (n_px float32 values), n frames
// of the window, a = Amount (Python float): m = clip((1 / rv - 1) / (1 / rv_min - 1), 0, 1) as float32, 0
// everywhere for a <= 0 or n <= 1.  out: n_px float32.
void temporal_mask(const float* rv, size_t n_px, int n, double a, float* out);

// display.nlc_image(rgb, m): rgb H x W x 3 float32, m H x W float32 (0..1) -> out H x W x 3 float32.
void nlc_image(const float* rgb, const float* m, int H, int W, float* out);

// np.interp(x, xp, fp) (numpy arr_interp: binary search, slope * (x - xp[j]) + fp[j] fused, the NaN retries;
// xp strictly increasing, n >= 1): out[m] for the m queries x.  (N3b: state.auto_complete's np.interp.)
void np_interp(const double* xp, const double* fp, int n, const double* x, int m, double* out);

// ------------------------------------------------------------------ quality.py
// quality.profile_numbers(ps, frames, full_range, matrix) from the pieces it reads:
//   rgb: the profile frame as np.clip(yuv_to_rgb(...), 0, 255) float32 (H x W x 3);
//   area (y0, x0, h, w) of base["area"];
//   factors[27] = ps.factors (state.CHANNELS order Y, Cb, Cr x 9 bins);
//   tune[4] = ps.tune all, Y, Cb, Cr (1.0 when missing);
//   aft = base["aft_applied"], reset = base["aft_reset"];
//   flags[81] = the stored bin flags marker[27], clip[27], gt3[27] ([c][k], c-major) used when aft and not reset
//   (nullptr with aft = the older profiles without "aft_bin_flags": every marker set, no clip / gt3).
// Returns false when quality_from_blob gives None; else *q_pct = qRound(q * 100), *c_pct = qRound(g * 100) as
// profile_numbers computes them (floor(x * 100 + 0.5) in double).
bool quality_numbers(const float* rgb, int H, int W, const int area[4], const double factors[27],
                     const double tune[4], bool aft, bool reset, const uint8_t* flags, int* q_pct, int* c_pct);

}  // namespace nvu
