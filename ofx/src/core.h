// core - the processing of MeatVideo.ofx on frames the host hands over, all through libnvcore's C ABI
// (native/nvcore_c.h, nvcore_c_depth.h).  No file, pipe or process is touched: pixels come from the host's image
// buffers, the settings and the noise profile from ClipParams (params.h; made by the window, session.h).
//
// Pixel mapping (the --depth 32 path of denoise_video_v5 / nv_depth, format gbrpf32le):
//   host RGBA float32 (OFX rows bottom-up) -> top-down G, B, R planes (the raw gbrpf32le frame, also what the window
//   receives) -> nvc_depth_sanitize per plane (NaN / +-inf recorded) -> nvc_depth_to_internal =
//   nv_depth.to_internal(raw, pixfmt("gbrpf32le"), matrix): full-range YCbCr 4:4:4 on the 0..255 scale.
//   Back: nvc_depth_from_internal (= nv_depth.from_internal(.., restore): x / 255, no clip, the non-finite samples of
//   the frame put back) -> R, G, B; alpha is the source's.
// Frame: nvc_temporal + nvc_spatial (+ nvc_mix) = backend.process_frame(window, ci, prof, P) of the UI for the
//   ClipParams state (qtui/src/core/nvcore.cpp temporal + finish), flows estimated afresh (flows=None).
#pragma once

#include <cstddef>
#include <vector>

#include "params.h"

namespace mvofx {

// float32 planes Y, Cb, Cr (4:4:4: all H x W), top-down
struct Planes {
    int H = 0, W = 0;
    std::vector<float> Y, Cb, Cr;
    const float* p(int c) const { return c == 0 ? Y.data() : c == 1 ? Cb.data() : Cr.data(); }
    float* p(int c) { return c == 0 ? Y.data() : c == 1 ? Cb.data() : Cr.data(); }
};

// the non-finite samples of the raw G, B, R planes (nv_depth.sanitize's restore info)
struct Restore {
    std::vector<long> idx[3];
    std::vector<float> vals[3];
};

// An RGBA float32 image as OFX lays it out: `base` points at the bottom-left pixel of the W x H frame, rows go up by
// row_bytes (any sign / padding).
struct RgbaView {
    const unsigned char* base = nullptr;
    std::ptrdiff_t row_bytes = 0;
    int W = 0, H = 0;
    const float* px(int top_row, int col) const {
        return reinterpret_cast<const float*>(base + static_cast<std::ptrdiff_t>(H - 1 - top_row) * row_bytes) + 4 * col;
    }
};

// host image -> the raw gbrpf32le frame: G, B, R planes, top-down, W x H each
std::vector<float> rgba_to_gbr(const RgbaView& v);

// host image -> internal planes (+ restore info when `rs`)
void rgba_to_internal(const RgbaView& v, int matrix, Planes* out, Restore* rs);

// internal planes -> RGB rows (R, G, B floats, top-down, W x H x 3) of nv_depth.from_internal with the restore info
std::vector<float> internal_to_rgb(const Planes& p, int matrix, const Restore* rs);

// backend.process_frame(window, ci, prof, P) of the ClipParams state: window = the frames t-R .. t+R inside the
// clip, ci = index of t
Planes process_frame(const std::vector<const Planes*>& window, int ci, const RenderState& s);

}  // namespace mvofx
