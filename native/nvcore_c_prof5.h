/* nvcore_c_prof5 - C ABI of the UI's profile building (nv_prof5.h) for the C++ Qt UI (qtui/, N3a); same
 * conventions as nvcore_c.h (flat buffers, float32 planes row-major, Python floats as doubles, cancel
 * callbacks int(void) != 0 = cancelled).  Checked bit for bit against the Python code by
 * native/verify_prof5.py and native/verify_bridge_prof5.py (a group of native/verify_bridge.py).
 *
 * nvc_build_profile_v5 = repro/nv_ui_qt/backend.build_profile(frames, f, full_range, matrix, mode, area, cancel)
 * with the native core on (repro/nv_native.build_profile_v5 -> nvfilter.build_profile_v5) minus "ui_frame":
 *   planes: Y (H x W), Cb, Cr (Hc x Wc) of frame f as float32 (backend.profile_window: np.asarray(p, float32));
 *   pair: the 3 planes of frame f + 1 or NULL; more: n_more x 3 planes of frames f + 1 .. f + n_more (<= 2 in the UI;
 *   pair == more[0..2] there); mode 0 "auto", 1 "spatial", 2 "temporal"; levels 5 in the UI; matrix 0 bt709,
 *   1 bt601, 2 bt2020; area_n 0 = FindFlatArea search (cancellable: status 5), 3 = [y0, x0, size], 4 = [y0, x0, h, w].
 * Returns 0 ok, 5 cancelled (Python: ProfileCancelled; nothing written), < 0 bad arguments / error.
 *
 * out_i[NVC_P5_I]:
 *   [0] area_n (3 or 4)   [1..4] area (y0, x0, size, size) or (y0, x0, h, w)
 *   [5] mode: 0 "auto", 1 "spatial", 2 "temporal"   [6] 1 = mode + "+noise-free"
 *   [7..9] levels Y, Cb, Cr   [10] flat_status (-1 = None: a manual area)   [11] 1 = "static_fraction" present
 *   [12] sub (1 or 2)   [13] full_range (0 / 1)   [14] number of curve points (8)
 * out_d[NVC_P5_D] (doubles, the Python floats):
 *   [0..14]  band_ratio Y[5], Cb[5], Cr[5] (levels[c] values each; the rest 0)
 *   [15..17] tot_ratio Y, Cb, Cr
 *   [18..32] area_sigma Y[5], Cb[5], Cr[5] (levels[c] values each)
 *   [33..56] curve Y[8], Cb[8], Cr[8]   [57..64] curve_centers[8]
 *   [65] static_fraction (when out_i[11])   [66] dead_fraction   [67] area_level   [68..70] sigma Y, Cb, Cr
 * Python dict (key order of nvfilter.build_profile_v5, the JSON the UI writes; backend adds "ui_frame" last):
 *   version (5), sub, full_range, area, mode, levels {Y, Cb, Cr}, band_ratio {Y, Cb, Cr}, tot_ratio {Y, Cb, Cr},
 *   area_sigma {Y, Cb, Cr}, curve {Y, Cb, Cr}, curve_centers, flat_status, [static_fraction], dead_fraction,
 *   area_level, sigma {Y, Cb, Cr}.  Python types: ints for version / sub / area / levels, bool full_range,
 *   None or int flat_status, str mode, lists of floats elsewhere.
 */
#ifndef NVCORE_C_PROF5_H
#define NVCORE_C_PROF5_H

#ifdef __cplusplus
extern "C" {
#endif

#define NVC_P5_I 16
#define NVC_P5_D 72

typedef int (*nvc_p5_cancel_fn)(void);

int nvc_build_profile_v5(const float* const* planes, int H, int W, int Hc, int Wc, const float* const* pair,
                         int n_more, const float* const* more, int full_range, int matrix, int mode, int levels,
                         int area_n, const int* area, int threads, nvc_p5_cancel_fn cancel, int* out_i,
                         double* out_d);

/* nvfilter._wn_band_std(levels) (1 <= levels <= 8): out[levels]. Returns 0. */
int nvc_wn_band_std(int levels, double* out);

/* backend.hh1_sigma(Y) / the score of backend.noisiest_frame: 1 = *out written, 0 = None (H or W < 2). */
int nvc_hh1_sigma(const float* Y, int H, int W, double* out);

/* The frames backend.noisiest_frame(frames) scores, for n frames: out[<= 32] (ascending, distinct); returns the
 * count.  noisiest_frame = the first candidate with the largest nvc_hh1_sigma (s > best, best starts at -1.0;
 * the cancel query before each candidate). */
int nvc_noisiest_candidates(int n, int* out);

/* state.effective_profile(base, factors, tune)'s "curve" and "sigma": centers[n], curve[3 * n] (Y, Cb, Cr, the
 * profile's floats), factors[27] (Y, Cb, Cr x 9 bins), tune[4] (all, Y, Cb, Cr; 1.0 when missing), area_level
 * (prof.get("area_level", 128.0)), tot_ratio[3].  Returns 0 when effective_profile returns base unchanged
 * (outputs untouched), 1 when curve_out[3 * n] / sigma_out[3] hold the new values. */
int nvc_effective_curve(const double* centers, int n, const double* curve, const double* factors,
                        const double* tune, double area_level, const double* tot_ratio, double* curve_out,
                        double* sigma_out);

#ifdef __cplusplus
}
#endif

#endif
