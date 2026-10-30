/* nvcore_c_rgb - C ABI of the R/G/B equalizer (G1, sandbox/omo/wp_qtui_fix/DESIGN_G1.md) for the C++ Qt UI; same
 * conventions as nvcore_c.h (flat buffers, float32 planes row-major, Python floats as doubles).  Checked bit for
 * bit by native/verify_rgb_eq.py against references copied from nv_aft_v5.apply_aft (and apply_aft itself).
 *
 * nvc_rgb_weights: apply_aft's colour weights c_cj = a_j^2 m_cj / sum_j a_j^2 m_cj (nv_aft_v5._color_weights) with
 *   a_j = the model q99 (word 5, nv_band.model_levels) of the R / G / B crop of the profile frame's RGB
 *   (np.clip(yuv_to_rgb(..), 0, 255): H x W x 3), area4 = (y0, x0, h, w) (numpy slicing: clamped to the frame);
 *   cw9 = [Y / Cb / Cr][R / G / B] row-major.  Returns 0, or < 0 on bad arguments.
 * nvc_rgb_effective_curve: the equalizer's curve and sigma (qtui state rgb_effective_profile when it computes):
 *   centers[n], src[3 * n] (Y, Cb, Cr: the curve before AFT, the profile's floats), cw9 (aft_color_weights /
 *   ui_rgb_weights), levels27 (R, G, B x 9 bins), tune4 (all, Y, Cb, Cr; 1.0 when missing), area_level,
 *   tot_ratio[3]; curve_out[3 * n] = float32(float32(src) * sqrt(max(sum_j cw_cj f_j, 0))) * float32(tune) as doubles,
 *   sigma_out[3] = interp(area_level, centres, curve) * tot_ratio.  Returns 0, < 0 on bad arguments.  The
 *   "unchanged" answers (levels == the reference with tune 1, unit levels after AFT) are the caller's.
 * nvc_rgb_combine: apply_aft steps 4-6 with the 9 levels of each face given: face_out[3 * n] (R, G, B interpolated on
 *   the centres; may be NULL), curve_out[3 * n] (Y, Cb, Cr float32 as doubles).
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
