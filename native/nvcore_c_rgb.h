/* nvcore_c_rgb - C ABI of the R/G/B equalizer (G1, sandbox/omo/wp_qtui_fix/DESIGN_G1.md) for the C++ Qt UI; same
 * conventions as nvcore_c.h (flat buffers, float32 planes row-major, Python floats as doubles).  Checked bit for
 * bit against the Python UI by native/verify_rgb_eq.py.
 *
 * nvc_rgb_weights = repro/nv_ui_qt/backend.rgb_weights(planes, area, full_range, matrix) on the RGB of the profile
 *   frame (np.clip(yuv_to_rgb(..), 0, 255): H x W x 3), area4 = (y0, x0, h, w) (numpy slicing: clamped to the
 *   frame); cw9 = the colour weights [Y / Cb / Cr][R / G / B] row-major.  Returns 0, or < 0 on bad arguments.
 * nvc_rgb_effective_curve = the curve and sigma of repro/nv_ui_qt/state.rgb_effective_profile(base, levels, tune)
 *   when it does not return base: centers[n], src[3 * n] (Y, Cb, Cr: the curve before AFT, the profile's floats),
 *   cw9 (aft_color_weights / ui_rgb_weights), levels27 (R, G, B x 9 bins), tune4 (all, Y, Cb, Cr; 1.0 when missing),
 *   area_level, tot_ratio[3]; curve_out[3 * n] (the float32 values as doubles), sigma_out[3].  Returns 0, < 0 on
 *   bad arguments.  The "unchanged" test (levels == the reference and tune 1) is the caller's.
 * nvc_rgb_combine = repro/nv_ui_qt/backend.rgb_combine(curve, centers, cw, levels) (nv_aft_v5.apply_aft steps 4-6):
 *   face_out[3 * n] (R, G, B on the centres; may be NULL), curve_out[3 * n] (Y, Cb, Cr float32 as doubles).
 */
#ifndef NVCORE_C_RGB_H
#define NVCORE_C_RGB_H

#ifdef __cplusplus
extern "C" {
#endif

int nvc_rgb_weights(const float* rgb, int H, int W, const int* area4, double* cw9);
int nvc_rgb_effective_curve(const double* centers, int n, const double* src, const double* cw9, const double* levels27,
                            const double* tune4, double area_level, const double* tot_ratio, double* curve_out,
                            double* sigma_out);
int nvc_rgb_combine(const double* centers, int n, const double* curve, const double* cw9, const double* levels27,
                    double* face_out, double* curve_out);

#ifdef __cplusplus
}
#endif

#endif
