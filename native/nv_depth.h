// nv_depth - the pixel-format conversions of repro/nv_depth.py (high bit depths: raw frame <-> the 8-bit-equivalent
// float32 planes 0..255 of the v5 core) and of repro/nv_ui_qt/ofx_bridge.py (the OFX host frames, --depth 8 / 16 /
// 32), for the C++ UI (N3c: moved out of qtui/src/core/export.cpp, N3b verification M2).  The arithmetic follows
// numpy's types and order (float32 products for the integer yuv scale and the 4:2:2 widening, float64 for the
// RGB <-> YCbCr matrices, np.clip(np.rint(..)).astype(..) for the integer outputs, NaN cast to 0 as numpy does on
// arm64); native/verify_bridge_depth.py checks it bit for bit against the Python functions.
#pragma once

#include <cstddef>
#include <cstdint>

namespace nvd {

// Format of a raw frame: family 0 yuv / 1 rgb (ffmpeg planes G, B, R), bits 8 / 10 / 12 / 16 / 32, chroma 420 / 422 /
// 444 (rgb: 444).  The legacy path is yuv 8-bit 4:2:0 (uint8 planes as read).
struct Fmt {
    int family = 0, bits = 8, is_float = 0, chroma = 420;
    bool legacy() const { return family == 0 && bits == 8 && chroma == 420; }
    int itemsize() const { return bits == 8 ? 1 : bits == 32 ? 4 : 2; }
    // (rows, cols) of the raw planes (nv_depth.PixFmt.plane_shapes)
    void raw_shape(int W, int H, int k, int* rows, int* cols) const;
    // (rows, cols) of the internal planes (to_internal: 4:2:2 chroma widened to the full width)
    void internal_shape(int W, int H, int k, int* rows, int* cols) const;
};

// nv_depth.sanitize of one float32 plane in place; idx / vals (capacity n, or null) get the flat indices and the
// original values of the replaced samples.  Returns their count.
long sanitize(float* p, long n, long* idx, float* vals);

// nv_depth.to_internal(split(raw), fmt, matrix): raw in ffmpeg's plane order (float planes already sanitized; legacy:
// the uint8 values as float32).  matrix 0 bt709, 1 bt601, 2 bt2020 (others: bt709).  Returns false for a format
// nv_depth.pixfmt does not accept.
bool to_internal(const uint8_t* raw, const Fmt& f, int W, int H, int matrix, float* Y, float* Cb, float* Cr);

// nv_depth.from_internal(planes, fmt, matrix, restore): the raw frame bytes (frame_bytes of fmt) into out.
// restore (float formats): per raw plane k, rn[k] flat indices ridx[k] and original values rvals[k] (null = none).
bool from_internal(const float* Y, const float* Cb, const float* Cr, const Fmt& f, int W, int H, int matrix,
                   const long* const* ridx, const float* const* rvals, const long* rn, uint8_t* out);

// ofx_bridge.unpack_yuv420p for depth 16 / 32 (planes Y W x H, Cb / Cr (W/2) x (H/2), float32 0..255; depth 32
// sanitizes each plane first: ridx[k] / rvals[k] capacity of plane k, rn[k] the count; null = not recorded) and
// pack_yuv420p for depth 8 / 16 / 32 (restore as from_internal, depth 32 only).
bool ofx_unpack(const uint8_t* data, int W, int H, int depth, float* Y, float* Cb, float* Cr, long* const* ridx,
                float* const* rvals, long* rn);
bool ofx_pack(const float* Y, const float* Cb, const float* Cr, int W, int H, int depth, const long* const* ridx,
              const float* const* rvals, const long* rn, uint8_t* out);

}  // namespace nvd
