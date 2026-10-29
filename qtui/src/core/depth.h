// depth - the pixel formats of repro/nv_depth.py (PixFmt, pixfmt, choose_format, describe, full_range_internal) and
// the frame conversions of nv_depth (split + sanitize + to_internal, from_internal) and of the OFX bridge
// (repro/nv_ui_qt/ofx_bridge.py unpack_yuv420p / pack_yuv420p) for the C++ UI.  The numerics are libnvcore's C ABI
// (native/nvcore_c_depth.h, checked bit for bit by native/verify_bridge_depth.py); this file only moves buffers.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "nvcore.h"
#include "video.h"

namespace nvqt {

// nv_depth.PixFmt: a raw-video format the pipeline reads and writes
struct PixFmt {
    std::string name;      // ffmpeg rawvideo pix_fmt (yuv420p, yuv420p10le, .., gbrp10le, gbrpf32le)
    std::string family;    // "yuv" / "rgb"
    int bits = 8;          // 8 / 10 / 12 / 16 / 32
    bool is_float = false;
    std::string chroma;    // "420" / "422" / "444"
    int itemsize = 1;      // bytes per sample (1 / 2 / 4)
    bool legacy() const { return name == "yuv420p"; }     // the original 8-bit yuv420p path
    // (rows, cols) of the three planes in ffmpeg's order (yuv: Y, U, V; rgb: G, B, R)
    void plane_shapes(int W, int H, int rows[3], int cols[3]) const;
    long long frame_bytes(int W, int H) const;             // bytes of one raw frame
    long long internal_bytes(int W, int H) const;          // bytes of one frame as the UI holds it
};
PixFmt pixfmt(const std::string& name);                               // nv_depth.pixfmt (throws std::runtime_error)
PixFmt choose_format(const SrcFmt& src, const std::string& depth);    // nv_depth.choose_format ("8".."32"/"source")
std::string describe(const PixFmt& f);                                // nv_depth.describe
bool full_range_internal(bool stream_full_range, const PixFmt& f);    // nv_depth.full_range_internal

struct Restore {                            // nv_depth.sanitize's info of one plane
    std::vector<long> idx;
    std::vector<float> vals;
};
using RestoreMap = std::map<int, Restore>;   // raw plane index -> the original non-finite samples (none = no entry)

// nv_depth.split + sanitize + to_internal: raw frame bytes -> float32 (Y, Cb, Cr) planes (legacy: the uint8 values
// as float32); info (float formats, may be null) receives the restore info of the planes that had NaN / +-inf
Planes to_internal(const uint8_t* raw, const PixFmt& fmt, int W, int H, const std::string& matrix, RestoreMap* info);
// nv_depth.from_internal: the raw frame bytes of fmt
std::vector<uint8_t> from_internal(const Planes& p, const PixFmt& fmt, const std::string& matrix, const RestoreMap* restore);

// ofx_bridge (--depth 8 / 16 / 32): frame_size, unpack_yuv420p for 16 / 32 (float32 0..255 planes, 4:2:0 with
// (W/2) x (H/2) chroma; 32 sanitizes and records the restore info), pack_yuv420p for 8 / 16 / 32
long long ofx_frame_size(int W, int H, int depth);
Planes ofx_unpack(const uint8_t* data, int W, int H, int depth, RestoreMap* restore);
std::vector<uint8_t> ofx_pack(const Planes& p, int depth, const RestoreMap* restore);

}  // namespace nvqt
