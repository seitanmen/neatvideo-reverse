/* nvcore_c_ui - C ABI of the UI numerics (nv_ui.h) for the C++ Qt UI (qtui/, N3a); same conventions as
 * nvcore_c.h (flat buffers, float32 planes row-major, RGB images H x W x 3 float32).  Checked bit for bit
 * against the Python UI code (repro/nv_ui_qt/display.py, quality.py, nvfilter.yuv_to_rgb) by
 * native/verify_bridge_ui.py (a group of native/verify_bridge.py). */
#ifndef NVCORE_C_UI_H
#define NVCORE_C_UI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* nvfilter.yuv_to_rgb((Y, Cb, Cr), matrix, full_range) with clip 0 (none), 1 (np.clip 0..255, the UI's
 * to_rgb) or 2 (nan_to_num + clip, FindFlatArea's input); matrix 0 bt709, 1 bt601, 2 bt2020.  out: H*W*3. */
void nvc_yuv_to_rgb(const float* Y, const float* Cb, const float* Cr, int H, int W, int Hc, int Wc, int matrix,
                    int full_range, int clip, float* out);

/* display.minmax: float(a.min()), float(a.max()) of n float32 values. */
void nvc_minmax(const float* v, size_t n, double* mn, double* mx);

/* display.render_simple(name, rgb, b, (mn, mx)): name 0 "rgb" (Normal / the primary pane), 1 R, 2 G, 3 B,
 * 4 Y, 5 Cr, 6 Cb; b = viewer brightness 0..100.  out: H*W*3 uint8. */
void nvc_render_simple(int name, const float* rgb, int H, int W, int b, double mn, double mx, uint8_t* out);

/* display.freq_images(rgb, kind, emphasis): kind 0 "Y+" (Enhanced: planes Y, Cr, Cb), 1 "Y", 2 "Cr", 3 "Cb"
 * (that channel's High, Mid, Low).  out16: 3*H*W int16; levels6: (L, H) of the three planes (float32 values). */
void nvc_freq_images(const float* rgb, int H, int W, int kind, int emphasis, int16_t* out16, float* levels6);

/* display.render_freq for one pane: range_kind = pane_kind of the pane name (0 Y, 4 Cr, 5 Cb; display.RANGES
 * order Y, R, G, B, Cr, Cb), one int16 plane and its (L, H).  out: H*W*3 uint8. */
void nvc_render_freq(int range_kind, const int16_t* p16, int H, int W, float L, float Hh, int b, uint8_t* out);

/* display.noise_level(rgb_crop) of an h x w x 3 crop. */
double nvc_noise_level(const float* rgb, int h, int w);

/* display.not_uniform(rgb_crop): bad[3] (Y, Cr, Cb not uniform), clip[3] (R, G, B clipped), 0 / 1. */
void nvc_not_uniform(const float* rgb, int h, int w, int* bad3, int* clip3);

/* quality.profile_numbers(ps, frames, full_range, matrix): rgb = np.clip(yuv_to_rgb(profile frame), 0, 255)
 * (H x W x 3), area4 = (y0, x0, h, w) of base["area"], factors27 = ps.factors (Y, Cb, Cr x 9), tune4 = ps.tune
 * all, Y, Cb, Cr (1.0 when missing), aft / reset = base["aft_applied"] / base["aft_reset"], flags81 = the
 * stored aft_bin_flags marker[3][9], clip[3][9], gt3[3][9] (NULL with aft: an older profile without them).
 * Returns 1 and the two percents (qRound(q * 100), qRound(g * 100)), or 0 (None). */
int nvc_quality_numbers(const float* rgb, int H, int W, const int* area4, const double* factors27,
                        const double* tune4, int aft, int reset, const uint8_t* flags81, int* q_pct, int* c_pct);

/* backend.finish's Mix with original (nvs::mix_v5 = nvfilter.process_frame_v5's): out[c] = o + m * (src - o) on
 * the planes Y (H x W), Cb, Cr (Hc x Wc), in place; nothing for m <= 0. */
void nvc_mix(float* const* out, const float* const* src, int H, int W, int Hc, int Wc, double m);

/* The 51 reference-floor words of the AFT's BuildNoiseModel (nv_profile.reference_floor()): read from npy_path
 * (data/ref_floor.npy, '<f4' (51,)) when it is given and valid (returns 1), else computed (about 7 s; returns 2,
 * with threads workers).  out51: 51 floats. */
int nvc_reference_floor(const char* npy_path, int threads, float* out51);

/* N3b: backend.temporal_mask(rv, n, a) on the H x W luma residual variance; out H x W float32. */
void nvc_temporal_mask(const float* rv, int H, int W, int n, double a, float* out);

/* N3b: display.nlc_image(rgb, m): rgb H x W x 3, m H x W -> out H x W x 3 (float32). */
void nvc_nlc_image(const float* rgb, const float* m, int H, int W, float* out);

/* N3b: np.interp(x, xp, fp) for m queries (xp strictly increasing, n >= 1); state.auto_complete's interpolation. */
void nvc_interp(const double* xp, const double* fp, int n, const double* x, int m, double* out);

/* N3b: float(np.mean(a)) of n float32 values (the "mean" of the Noise Level Check status line). */
float nvc_mean_f32(const float* a, size_t n);

#ifdef __cplusplus
}
#endif

#endif
