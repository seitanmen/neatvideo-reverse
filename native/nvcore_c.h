/* nvcore_c - C ABI of the N1 C++ ports for ctypes (libnvcore.dylib, N2a; repro/nv_native.py).
 *
 * Thin wrappers only: every function calls the N1 routine that native/verify_*.py checked bit for bit
 * against the Python implementation, on flat buffers (no structs cross the ABI).  Planes are row-major
 * float32, RGB images are H x W x 3 float32 (HWC).  repro/nv_native.py builds the arguments the way the
 * Python path computes its inputs; native/verify_bridge.py compares the two paths end to end.
 *
 * Profile / parameter blocks of the v5 kernels (nvs::V5Profile / V5Params), Python floats as doubles
 * (the kernels cast them to float32 before each operation, as numpy does with weak scalars):
 *   prof_i[NVC_PROF_I]: ncurve (1..16), levels Y, Cb, Cr
 *   prof_d[NVC_PROF_D]: centers[16], curve Y[16], Cb[16], Cr[16], band_ratio Y[5], Cb[5], Cr[5], tot_ratio[3]
 *   par_i[NVC_PAR_I]  : radius, bs, search, similarity (0 "v5", 1 "pyramid")
 *   par_d[NVC_PAR_D]  : temporal, threshold, spatial, luma, chroma, noise_level, pyramid_scale, mix,
 *                       band_amounts[5], band_noise[5] (V5_BANDS order: high .. ultra-low), channel_noise[3]
 *
 * Cancel callbacks: int cancel(void), non-zero = cancelled; NULL = never asked.  They are called from the
 * calling thread or, with threads > 1, from worker threads (one query at a time, under a lock).
 */
#ifndef NVCORE_C_H
#define NVCORE_C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NVC_ABI_VERSION 2       /* 2 (G7/G8): nvc_profile_build_cancel; repro/nv_native.py calls nvc_build_profile_v5 */
#define NVC_PROF_I 4
#define NVC_PROF_D 82
#define NVC_PAR_I 4
#define NVC_PAR_D 21
#define NVC_PROF_BYTES 0xc300

typedef int (*nvc_cancel_fn)(void);

int nvc_abi_version(void);

/* nvc::genuine_noise_curve (nv_curve.genuine_noise_curve): 1 = ok (xs, ys: 8 doubles each), 0 = no curve
 * (W or H < 48, or cancelled). */
int nvc_noise_curve(const float* rgb, int W, int H, int threads, nvc_cancel_fn cancel, double* xs, double* ys);

/* nvf::find_flat_area_genuine with the curve run inside (nv_flat.find_flat_area_genuine with nvfilter's
 * curve="genuine" callback): out5 = x, y, w, h, status. */
void nvc_flat_area(const float* rgb, int W, int H, int mode, int threads, nvc_cancel_fn cancel, int* out5);

/* nvp::build_profile (nv_profile.build_profile(rgb, mode, (x, y, w, h), with_floor, lzma)): floor = the 51
 * reference-floor words (with_floor = True) or NULL.  out: NVC_PROF_BYTES.  Returns nvp::Status (0 ok). */
int nvc_profile_build(const float* rgb, int W, int H, int mode, int x, int y, int w, int h, const float* floor,
                      int lzma, int threads, uint8_t* out);

/* nvc_profile_build with a cancel query (G7): nvp::build_profile asks it before each plane model and inside the
 * models (every level x direction set, every direction, every pattern round), before each tile model of the
 * tile deviations and before the LZMA part.  Returns 3 (nvp::CANCELLED) soon after a true answer (out is then
 * not meaningful); without one the result is that of nvc_profile_build. */
int nvc_profile_build_cancel(const float* rgb, int W, int H, int mode, int x, int y, int w, int h,
                             const float* floor, int lzma, int threads, nvc_cancel_fn cancel, uint8_t* out);

/* nva::auto_fine_tune (nv_aft.auto_fine_tune(rgb, prof, mode, image2=None, cancel)) on prof in place.
 * out3: the mode-2 values (written for mode 2 with status 0).  Returns nva::Status (0 ok, 1 too small,
 * 2 cancelled, 3 model error). */
int nvc_aft(const float* rgb, int W, int H, uint8_t* prof, int mode, int threads, nvc_cancel_fn cancel,
            float* out3);

/* nva::combine (the part of nv_aft_v5.apply_aft after the AFT call).  curves: Y, Cb, Cr float32 [n] each
 * (np.array(prof["curve"][c], float32)).  Outputs: cw[9], face[3 * n], curve_out[3 * n], sigma[3],
 * flags[81] = marker[27], clip[27], gt3[27] (face-major: R, G, B x 9 bins). */
void nvc_aft_combine(const uint8_t* raw, const uint8_t* aft, const double* centers, int n, const float* curves,
                     double area_level, const double* tot_ratio, int band, double* cw, double* face,
                     float* curve_out, double* sigma, uint8_t* flags);

/* nvt::temporal_v5 (nvfilter.temporal_v5(window, ci, prof, P, flows)).  planes: nf * 3 pointers (frame-major
 * Y, Cb, Cr); Y planes H x W, chroma planes Hc x Wc.  Flows (block vectors, nby x nbx = H / bs x W / bs):
 * nin given ones (offsets off_in[i] = k - ci; flow_in[2i] = vy, flow_in[2i + 1] = vx).  flow_out: 2 * nf
 * buffers; for every window index k whose flow was estimated here, the vectors go to flow_out[2k],
 * flow_out[2k + 1] and new_flow[k] = 1 (else 0).  out / rv: 3 planes each (Y on H x W, chroma Hc x Wc).
 * Returns 0. */
int nvc_temporal(int nf, int ci, int H, int W, int Hc, int Wc, const float* const* planes, const int* prof_i,
                 const double* prof_d, const int* par_i, const double* par_d, int nin, const int* off_in,
                 const float* const* flow_in, float* const* flow_out, uint8_t* new_flow, float* const* out,
                 float* const* rv);

/* nvs::spatial_v5 (nvfilter.spatial_v5(planes, prof, rv, P)): planes / rv / out = Y (H x W), Cb, Cr (Hc x Wc).
 * mask: bit c set = compute plane c (out[c] untouched otherwise).  Returns 0. */
int nvc_spatial(int H, int W, int Hc, int Wc, const float* const* planes, const float* const* rv,
                const int* prof_i, const double* prof_d, const int* par_i, const double* par_d, int mask,
                float* const* out);

#ifdef __cplusplus
}
#endif

#endif
