// nvcore - the C++ UI's processing, all through libnvcore's C ABI (native/nvcore_c.h, nvcore_c_ui.h,
// nvcore_c_prof5.h).  Each function reproduces the Python UI function named in its comment with the native
// core on (repro/nv_ui_qt/backend.py + repro/nv_native.py); the numerics are the C ABI's (bit-exact with the
// Python code, native/verify_bridge*.py), this file only moves buffers and builds the dicts.
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "pyobj.h"

namespace nvqt {

// A frame as the UI holds it in memory (denoise_video_v5.read_frames): the uint8 planes of the 8-bit yuv420p path
// (and of the OFX host at --depth 8), or at a high bit depth the float32 planes on the 0..255 scale
// (nv_depth.to_internal; 4:2:2 chroma widened to 4:4:4, RGB sources as full-range YCbCr 4:4:4).
struct Planes;
struct Frame {
    int H = 0, W = 0, Hc = 0, Wc = 0;
    std::vector<uint8_t> Y, Cb, Cr;        // 8-bit
    std::vector<float> fY, fCb, fCr;       // high bit depth (is_float)
    bool is_float() const { return !fY.empty(); }
    // sample i of plane c (0 Y, 1 Cb, 2 Cr) as float32 (np.asarray(p, np.float32))
    float at(int c, size_t i) const {
        if (is_float()) return c == 0 ? fY[i] : c == 1 ? fCb[i] : fCr[i];
        return static_cast<float>(c == 0 ? Y[i] : c == 1 ? Cb[i] : Cr[i]);
    }
    size_t bytes() const { return Y.size() + Cb.size() + Cr.size() + 4 * (fY.size() + fCb.size() + fCr.size()); }
};
Frame frame_from_planes(Planes&& p);       // a high-bit-depth frame holding the float32 planes

// float32 planes (np.asarray(p, np.float32))
struct Planes {
    int H = 0, W = 0, Hc = 0, Wc = 0;
    std::vector<float> Y, Cb, Cr;
    const float* p(int c) const { return c == 0 ? Y.data() : c == 1 ? Cb.data() : Cr.data(); }
    float* p(int c) { return c == 0 ? Y.data() : c == 1 ? Cb.data() : Cr.data(); }
    std::vector<float>& v(int c) { return c == 0 ? Y : c == 1 ? Cb : Cr; }
};
Planes to_planes(const Frame& f);

int matrix_code(const std::string& matrix);       // "bt709" 0, "bt601" 1, "bt2020" 2 (others bt709)

// RGB H x W x 3 float32: nvfilter.yuv_to_rgb with clip 0 none / 1 np.clip(.., 0, 255) (PreviewEngine.to_rgb)
std::vector<float> to_rgb(const Planes& p, const std::string& matrix, bool full_range, int clip = 1);

using CancelFn = std::function<bool()>;

struct Cancelled : std::runtime_error {
    Cancelled() : std::runtime_error("Cancelled: cancelled") {}
};
struct ProfileCancelled : std::runtime_error {
    ProfileCancelled() : std::runtime_error("ProfileCancelled: profiling cancelled (FindFlatArea status 5)") {}
};

int native_threads();                            // nv_native.THREADS (NV_NATIVE_THREADS, default min(4, cpu))

// backend.build_profile(frames, f, full_range, matrix, mode, area, cancel) -> the profile dict (+ "ui_frame")
PyObj build_profile(const std::vector<Frame>& frames, int f, bool full_range, const std::string& matrix,
                    const std::string& mode, const std::vector<int>* area, const CancelFn& cancel);

// build_profile of planes pl with the static-pixel pair (may be null) and the frames whose single-frame curves are
// median-combined (more; build_profile passes frames f + 1, f + 2 and pair = &more[0]); "ui_frame" = f
PyObj build_profile_planes(const Planes& pl, const Planes* pair, const std::vector<Planes>& more, int f,
                           bool full_range, const std::string& matrix, const std::string& mode,
                           const std::vector<int>* area, const CancelFn& cancel);

// backend.noisiest_frame(frames, cancel=cancel)
int noisiest_frame(const std::vector<Frame>& frames, const CancelFn& cancel);

// backend.aft_frame(base, frames)
int aft_frame(const PyObj& base, size_t nframes);

// backend.run_aft(base, planes, full_range, matrix) through the C path (nv_native.apply_aft): a new profile;
// throws Cancelled, or std::runtime_error where the Python UI would fall back to the Python AFT.
PyObj run_aft(const PyObj& base, const Frame& frame, bool full_range, const std::string& matrix,
              const CancelFn& cancel);
// run_aft on an RGB image (H x W x 3 float32, to_rgb clip 1) in place of a frame (fields.h run_aft_fields: the stacked
// fields of a "Two fields" profile).  Both pass the RGB through nvc_aft_input16 (nv_aft_v5.to_internal16) first.
PyObj run_aft_rgb(const PyObj& base, std::vector<float> rgb, int H, int W, const CancelFn& cancel);

// N4e (repro Auto Match, native/nvcore_c_match.h): the match model words (faces R, G, B x 492, stats R, G, B x 51)
// of base's area on frame (nvc_profile_build_cancel, colour mode 1, as run_aft builds the AFT's profile; no LZMA part);
// nullopt where the native BuildNoiseModel does not take the area (a side < 32).  Throws Cancelled.
std::optional<std::vector<float>> match_model_of(const PyObj& base, const Frame& frame, bool full_range,
                                                 const std::string& matrix, const CancelFn& cancel);
// the frame's own match model, the reference of the score: FindFlatArea of the frame (nvc_flat_area, colour mode 1, on
// the CLIP_FINITE RGB) and its square as build_profile_v5's auto area makes it, then match_model_of that square.
// words empty when there is none (a side < 64 px, FindFlatArea status 1 "not found", or a square the native
// BuildNoiseModel does not take); status = the FindFlatArea status (-1: not searched).  Throws Cancelled.
struct RefModel {
    std::vector<float> words;
    int area[4] = {0, 0, 0, 0};       // x, y, w, h
    int status = -1;
};
RefModel reference_model(const Frame& frame, bool full_range, const std::string& matrix, const CancelFn& cancel);
// nvc_match_score of models (each MATCH_WORDS floats) against frame's RGB (to_rgb clip 1, as the AFT) with the
// reference ref (MATCH_WORDS floats): status (0 ok, 1 too small, 2 cancelled -> throws Cancelled, 3 model error,
// 4 every tile clipped), scores, accept counts, tile counts
struct MatchScores {
    int status = 0;
    std::vector<double> scores;
    std::vector<int> accept;
    int tiles = 0, used = 0, ref_accept = 0;
};
MatchScores match_scores(const Frame& frame, bool full_range, const std::string& matrix, const std::vector<float>& ref,
                         const std::vector<std::vector<float>>& models, const CancelFn& cancel);

// nvfilter._wn_band_std(levels)
std::vector<double> wn_band_std(int levels);

// ---------------------------------------------------------------- v5 kernels (backend.temporal / spatial_ui)
// flows cache of PreviewEngine: window offset -> (vy, vx) block vectors
struct Flow {
    int nby = 0, nbx = 0;
    std::vector<float> vy, vx;
};
using Flows = std::map<int, Flow>;

// backend.temporal(window, ci, prof, P, flows): out (3 planes) and rv (3 planes).  shift (INT-3, the fields of
// fields.h): nullptr = nvc_temporal; else 2 offsets (luma, chroma rows) per window frame, through nvc_temporal_shift
void temporal(const std::vector<Planes>& window, int ci, const PyObj& prof, const PyObj& P, Flows* flows,
              Planes* out, Planes* rv, const double* shift = nullptr);
// backend.finish(window, ci, tmp, rv, prof, P): spatial_ui + mix
Planes finish(const Planes& src, const Planes& tmp, const Planes& rv, const PyObj& prof, const PyObj& P);

// the reference floor (nv_profile.reference_floor, data/ref_floor.npy or computed once)
const std::vector<float>& reference_floor();

// ---------------------------------------------------------------- display (display.py through nvcore_c_ui)
std::pair<double, double> minmax(const std::vector<float>& rgb);
std::vector<uint8_t> render_simple(int name, const std::vector<float>& rgb, int H, int W, int b, double mn, double mx);
struct FreqImages {
    int H = 0, W = 0;
    std::vector<int16_t> planes;    // 3 x H x W
    float levels[6];
};
FreqImages freq_images(const std::vector<float>& rgb, int H, int W, int kind, bool emphasis);
std::vector<uint8_t> render_freq(int range_kind, const FreqImages& fi, int plane, int b);
// crop (y0, x0, h, w) of an H x W x 3 image (numpy slicing: clamped)
std::vector<float> crop_rgb(const std::vector<float>& rgb, int H, int W, int y0, int x0, int h, int w, int* ch, int* cw);
double noise_level(const std::vector<float>& crop, int h, int w);
// display.nlc_image(rgb, m) (N3b) and float(m.mean()) (nvc_mean_f32)
std::vector<float> nlc_image(const std::vector<float>& rgb, const std::vector<float>& m, int H, int W);
double mean_f32(const std::vector<float>& m);
void not_uniform(const std::vector<float>& crop, int h, int w, std::vector<std::string>* bad,
                 std::vector<std::string>* clip);
// quality.profile_numbers(ps, frames, full_range, matrix) -> (Quality %, Completeness %) or nullopt
struct ProfileState;
std::optional<std::pair<int, int>> profile_numbers(const ProfileState& ps, const std::vector<Frame>& frames,
                                                   bool full_range, const std::string& matrix);
// the same on the profile frame itself (INT-3: the field image of a field profile, fields.h profile_image)
std::optional<std::pair<int, int>> profile_numbers(const ProfileState& ps, const Frame& frame, bool full_range,
                                                   const std::string& matrix);

}  // namespace nvqt
