// nv_profile - C++ port of repro/nv_profile.py build_profile: BuildNoiseModel (FUN_009af718) as
// `nvh profile W H rgb mode out x y w h` runs it (area given, FindFlatArea not called) - the
// 0xc300-byte profile object.
//
// Source of truth: repro/nv_profile.py (+ nv_mcstats.py for FUN_0098718c, nv_refnoise.py for the
// white-noise generator FUN_009c3bc4).  Parts reused from the earlier ports:
//   nvf::transform (N1d)      the colour transform DAT_0147df50[mode] of the crop planes
//   nvm::model_full (N1e)     FUN_00995250 of the six planes (set -1, 5 levels) and of the 32 x 32
//                             tiles of tile_dev (set 2, 2 levels)
//   nvb::mean_exact (N1a)     FUN_009ef3fc channel means
//   nvc::is_clip_genuine (N1b) IsClip of the channels
//   nvlz::plane_int16 / blob (N1c) the LZMA part FUN_009b41c4 (LZMA SDK 9.20)
// native/verify_profile.py compares all 0xc300 bytes with the Python port.
//
// The scalar float operations of tile_dev reproduce numpy's float32 scalar semantics explicitly
// (numpy 2.1.3 / arm64: a + b and a * b of two NaNs give b's payload, a / b gives a's; a signalling
// NaN takes precedence and is quieted, the AArch64 rule; a NaN made from non-NaN operands is the
// default NaN 0x7fc00000), so the bits do not depend on clang's operand order or on the target.
//
// Build with -ffp-contract=off -fno-fast-math.
#pragma once

#include <cstddef>
#include <cstdint>

namespace nvp {

constexpr size_t PROF_BYTES = 0xc300;
constexpr int MODEL_WORDS = 0x1f54 / 4;      // words of a plane model stored in the profile
constexpr int DEV_WORDS = 51;                // FUN_009b2f34 output (0xcc bytes)
constexpr int MC_WORDS = 32;                 // FUN_0098718c output

enum Status : int {
    OK = 0,
    NOT_IMPLEMENTED = 1,   // the Python port raises NotImplementedError (a side < 32 in tile_dev,
                           // FUN_00ad11b0 without a direction bin in nv_model)
    BAD_ARGUMENT = 2,      // ValueError in nv_model (sides outside 2..512), or an area outside the
                           // frame / a mode outside 0..3 (the Python port does not check these)
};

// data/ref_floor.npy ('<f4', shape (51,)): nv_profile.reference_floor's cache.  false on any error.
bool load_reference_floor(const char* path, float out[DEV_WORDS]);

// FUN_009c3bc4 (nv_refnoise.noise): n values of the white-noise generator.
void reference_noise(size_t n, uint32_t seed, float* out);

// nv_profile.reference_floor without the cache: tile_dev of the 256 x 256 reference noise
// (seed 0x12345678) against its own model (set -1, 5 levels), no floor.
int compute_reference_floor(float out[DEV_WORDS], int threads = 1);

// FUN_009b2f34 (nv_profile.tile_dev): plane H x W row-major, model >= MODEL_WORDS words of the
// plane's model; floor may be nullptr (no floor).  threads > 1 computes the tile models in
// parallel (the accumulation stays in the Python order: same bits).
int tile_dev(const float* plane, int W, int H, const uint32_t* model, const float* floor,
             float out[DEV_WORDS], int threads = 1);

// FUN_0098718c (nv_mcstats.mc_stats) of the H x W planes R, G, B.
void mc_stats(const float* R, const float* G, const float* B, int W, int H, float out[MC_WORDS]);

// nv_profile.size_class / curve_bin.
int size_class(int m);
int curve_bin(float mean);

// nv_profile.build_profile(rgb, mode, (x, y, w, h), with_floor, lzma): rgb is H x W pixels with
// stride_c floats per pixel (channels 0..2 = R, G, B).  floor: the 51 reference-floor words
// (with_floor = True) or nullptr (with_floor = False).  out receives PROF_BYTES bytes; the heap
// pointers 0x00..0x17 stay 0.  Returns a Status (out is meaningful for OK only).
int build_profile(const float* rgb, int W, int H, int stride_c, int mode, int x, int y, int w, int h,
                  const float* floor, uint8_t* out, bool lzma = true, int threads = 1);

}  // namespace nvp
