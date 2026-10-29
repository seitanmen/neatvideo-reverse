// engine - backend.PreviewEngine of the PySide6 UI: one frame (optionally a crop) for the preview, with the
// motion vectors cached per centre frame and the temporal result per temporal-parameter set (the same arrays
// process_frame computes), all processing through nvcore.h.
#pragma once

#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "nvcore.h"
#include "pyobj.h"

namespace nvqt {

struct Crop {
    int y0 = 0, x0 = 0, h = 0, w = 0;
    bool operator==(const Crop& o) const { return y0 == o.y0 && x0 == o.x0 && h == o.h && w == o.w; }
};

// backend.temporal_sig(P) as a string key (the parameters the temporal stage depends on)
std::string temporal_sig(const PyObj& P);

class PreviewEngine {
public:
    PreviewEngine(const std::vector<Frame>* frames, std::string matrix, bool full_range)
        : frames_(frames), matrix_(std::move(matrix)), full_range_(full_range) {}

    const std::vector<Frame>& frames() const { return *frames_; }
    const std::string& matrix() const { return matrix_; }
    bool full_range() const { return full_range_; }

    Planes crop_planes(const Frame& f, const std::optional<Crop>& crop) const;
    // window(t, R, crop): the frame indices and their (cropped) float32 planes
    std::pair<std::vector<int>, std::vector<Planes>> window(int t, int R, const std::optional<Crop>& crop) const;
    // process(t, prof, token, P, crop) -> (filtered RGB, original RGB) of the frame / crop (H x W x 3 float32)
    std::pair<std::vector<float>, std::vector<float>> process(int t, const PyObj& prof, long token, const PyObj& P,
                                                              const std::optional<Crop>& crop);
    // assist(t, prof, P): Profile Check Assist Image (spatial stage only, residual variance 1), RGB of the frame
    std::vector<float> assist(int t, const PyObj& prof, const PyObj& P);
    // mask(t, prof, token, P): the Noise Level Check mask (H x W float32 0..1, coded orientation) of the whole
    // frame t from the luma residual variance of the cached temporal stage (backend.temporal_mask)
    std::vector<float> mask(int t, const PyObj& prof, long token, const PyObj& P);
    // _temporal_stage(t, prof, prof_token, P, crop): (window, ci, tmp, rv), the temporal result cached per temporal_sig
    struct Stage {
        std::vector<int> idx;
        std::vector<Planes> win;
        int ci = 0;
        Planes tmp, rv;
    };
    Stage temporal_stage(int t, const PyObj& prof, long token, const PyObj& P, const std::optional<Crop>& crop);
    // to_rgb(planes) = np.clip(yuv_to_rgb(clip(planes, 0, 255)?...)): the planes as given
    std::vector<float> to_rgb(const Planes& p) const { return nvqt::to_rgb(p, matrix_, full_range_, 1); }

private:
    struct TempEntry {
        std::string key;
        Planes tmp, rv;
    };
    struct FlowEntry {
        std::string key;
        Flows flows;
    };

    const std::vector<Frame>* frames_;
    std::string matrix_;
    bool full_range_;
    std::mutex mu_;
    std::list<FlowEntry> flows_;     // most recent last, at most 16 (OrderedDict of the Python engine)
    std::list<TempEntry> temporal_;  // at most 6
};

// np.clip(p, 0, 255) of every plane (NaN kept), before the RGB conversion of the preview
Planes clip_planes(const Planes& p);

}  // namespace nvqt
