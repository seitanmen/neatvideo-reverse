/* nvcore_c_depth - C ABI of the pixel-format conversions (nv_depth.h) for the C++ Qt UI (qtui/, N3c): the high-bit-
 * depth reading / writing of repro/nv_depth.py and the OFX host frames of repro/nv_ui_qt/ofx_bridge.py.  Same
 * conventions as nvcore_c.h (flat buffers, float32 planes row-major).  Checked bit for bit against the Python
 * functions by native/verify_bridge_depth.py (a group of native/verify_bridge.py: --only depth).
 *
 * A format is (family 0 yuv / 1 rgb, bits 8 / 10 / 12 / 16 / 32, is_float, chroma 420 / 422 / 444); the formats are
 * those of nv_depth.pixfmt (yuv420p, yuv{420,422,444}p{10,12,16}le, gbrp{10,12,16}le, gbrpf32le).  Raw frames hold
 * the planes in ffmpeg's order (yuv: Y, U, V; rgb: G, B, R), little-endian; internal planes are Y (H x W) and Cb /
 * Cr (4:2:0: ((H+1)/2) x ((W+1)/2), 4:2:2 widened and 4:4:4: H x W).  matrix 0 bt709, 1 bt601, 2 bt2020. */
#ifndef NVCORE_C_DEPTH_H
#define NVCORE_C_DEPTH_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* nv_depth.sanitize of one float32 plane of n values, in place: NaN -> 0, +inf -> the largest finite value of the
 * plane, -inf -> the smallest (0 when there is none).  idx / vals (capacity n, or NULL) receive the flat indices
 * and the original values (bit patterns kept).  Returns the number of replaced samples. */
long nvc_depth_sanitize(float* p, long n, long* idx, float* vals);

/* nv_depth.to_internal(nv_depth.split(raw), fmt, matrix): the internal planes (float planes of raw sanitized
 * beforehand, as denoise_video_v5.read_frames does; the legacy yuv420p: the uint8 values as float32).
 * Returns 0, or -1 for a format nv_depth.pixfmt does not accept. */
int nvc_depth_to_internal(const uint8_t* raw, int family, int bits, int is_float, int chroma, int W, int H, int matrix,
                          float* Y, float* Cb, float* Cr);

/* nv_depth.from_internal(planes, fmt, matrix, restore): out = the raw frame (fmt.frame_bytes(W, H) bytes).
 * restore (float formats; NULL = none): for raw plane k, rn[k] flat indices ridx[k] and original values rvals[k]
 * (ridx[k] NULL = none for that plane).  Returns 0 or -1. */
int nvc_depth_from_internal(const float* Y, const float* Cb, const float* Cr, int family, int bits, int is_float,
                            int chroma, int W, int H, int matrix, const long* const* ridx, const float* const* rvals,
                            const long* rn, uint8_t* out);

/* ofx_bridge.unpack_yuv420p for --depth 16 / 32: data (frame_size(W, H, depth) bytes) -> float32 0..255 planes Y
 * (W x H), Cb / Cr ((W/2) x (H/2)).  Depth 32 sanitizes each plane first (ridx[k] / rvals[k] with the capacity of
 * plane k, or NULL; rn[k] = the count).  Returns 0, or -1 for another depth (depth 8 holds the uint8 planes as
 * they are). */
int nvc_ofx_unpack(const uint8_t* data, int W, int H, int depth, float* Y, float* Cb, float* Cr, long* const* ridx,
                   float* const* rvals, long* rn);

/* ofx_bridge.pack_yuv420p for --depth 8 / 16 / 32 (restore as nvc_depth_from_internal, depth 32 only): out =
 * frame_size(W, H, depth) bytes.  Returns 0 or -1. */
int nvc_ofx_pack(const float* Y, const float* Cb, const float* Cr, int W, int H, int depth, const long* const* ridx,
                 const float* const* rvals, const long* rn, uint8_t* out);

#ifdef __cplusplus
}
#endif

#endif
