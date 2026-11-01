// params - the parameters of MeatVideo.ofx (N4d) and the ClipParams string they keep.
//
// The inspector shows what the genuine's Describe shows (reference/genuine/analysis/prof_0xa22c.c), page "Controls":
//   PushButton "Prepare Profile..." (label "Prepare Noise Profile") and "Adjust Filters..." (label "Adjust Filter
//   Settings", disabled until ClipParams holds a noise profile), the secret Custom parameter "ClipParams" and the
//   secret Integer parameters "ParamsHash1..3".  The buttons open the window (the helper MeatVideoQt --ofx-session,
//   session.h); its Apply writes ClipParams.
//
// ClipParams (JSON text, written by qtui/src/app/ofx_session.cpp clip_params_json):
//   {"format": "meatvideo_ofx_clip_params", "version": 1,
//    "frame_size": [W, H],
//    "preset": <the filter preset as the UI saves it (format nv_repro_filter_preset)>,
//    "profile": <the noise profile as the UI saves it (nv_repro_profile[_rgb])> | null,
//    "render": {"matrix": "bt709" | "bt601" | "bt2020" | "auto", "full_range": true,
//               "P": <state.backend_params(ui)>, "profile": <ProfileState.effective()>} | null}
// "preset" and "profile" restore the window; the render reads "render" only (render is null without a profile).
// A new instance gets the ClipParams of the Default Profile / Preset (session.h request_defaults; matrix "auto",
// frame_size null).
// The mapping of "render" to the v5 parameter blocks of libnvcore is qtui/src/core/nvcore.cpp's (v5_blocks, uniform,
// channel_params, temporal's t_noise), so a render equals the C++ UI's / the Python UI's backend.process_frame for
// the same frames and state.
#pragma once

#include <exception>
#include <string>
#include <utility>

#include "nvcore_c.h"

namespace mvofx {

// The plug-in's exception type.  Not std::runtime_error / logic_error: destroying the first of those makes macOS's
// libc++abi ask dlopen("/usr/lib/libstdc++.6.dylib", RTLD_NOLOAD) (__refstring_imp, a GCC compatibility probe; no
// file is read), which the file-access monitor of the test host records; a std::string member does not.
class Error : public std::exception {
public:
    explicit Error(std::string m) : msg_(std::move(m)) {}
    const char* what() const noexcept override { return msg_.c_str(); }

private:
    std::string msg_;
};

// parameter names (and script names) of the genuine
constexpr const char* PARAM_PAGE = "Controls";
constexpr const char* PARAM_PREPARE = "Prepare Profile...";
constexpr const char* PARAM_ADJUST = "Adjust Filters...";
constexpr const char* PARAM_CLIP = "ClipParams";
constexpr const char* PARAM_HASH[3] = {"ParamsHash1", "ParamsHash2", "ParamsHash3"};

// the v5 parameter blocks of one libnvcore call (nvcore_c.h prof_i / prof_d / par_i / par_d)
struct V5Blocks {
    int pi[NVC_PROF_I] = {0};
    double pd[NVC_PROF_D] = {0};
    int qi[NVC_PAR_I] = {0};
    double qd[NVC_PAR_D] = {0};
};

// what a render needs, read from ClipParams' "render"
struct RenderState {
    bool has_profile = false;
    int matrix = 0;                 // nvcore_c code: 0 bt709, 1 bt601, 2 bt2020; -1 "auto" (by the frame height:
                                    // BT.709 from 720 lines up, else BT.601 - the window's rule for the host's frames)
    std::string matrix_name;
    int W = 0, H = 0;               // frame_size the state was made on (informative)
    int radius = 0;
    V5Blocks temporal;              // backend.temporal: P with noise_level x t_noise
    bool spatial = true;            // P["ui_spatial"]
    bool uniform = true;            // backend._uniform(P): one spatial_v5 call on all planes
    V5Blocks spatial_all;           // uniform: v5_blocks(prof, P)
    V5Blocks spatial_ch[3];         // else: v5_blocks(prof, _channel_params(P, c)), c = Y, Cb, Cr
    double mix = 0;                 // P["mix"]
};

// ClipParams -> RenderState; "" -> has_profile false.  Throws Error for text that is not a ClipParams of this
// version or whose "render" lacks what the v5 blocks need.
RenderState parse_clip_params(const std::string& s);

// ParamsHash1..3 of a ClipParams text: FNV-1a 64 of the text, its low and high 31 bits, and the length (31 bits)
void params_hash(const std::string& s, int out[3]);

}  // namespace mvofx
