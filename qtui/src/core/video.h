// video - ffmpeg / ffprobe for the C++ UI: the tool search of the N2b launcher (packaging/main.py), the probe and
// the 8-bit frame reader of repro/denoise_video_v5.py (probe, read_frames with fmt None = yuv420p uint8), the
// matrix choice of nvfilter.matrix_from_tags and nv_depth.source_format / describe_source.
#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "nvcore.h"

namespace nvqt {

// The directory holding both ffmpeg and ffprobe: $NV_FFMPEG (file or directory), the setting prefs/ffmpeg,
// /opt/homebrew/bin, /usr/local/bin, /opt/local/bin, then PATH.  Empty when none is found.
std::string find_ffmpeg_dir(const std::string& pref = "");
std::string ffmpeg_dir();          // the cached find_ffmpeg_dir() of the process (set_ffmpeg_dir overrides)
void set_ffmpeg_dir(const std::string& d);
std::string tool_path(const char* name);   // "<ffmpeg_dir()>/<name>", or the bare name when no directory is known
// A prefs/ffmpeg value (a binary or its directory, ~ expanded) naming a directory that holds both tools
bool ffmpeg_tools_in(const std::string& pref);
// Preferences: the warning for a path without both tools (ffmpeg_path.missing_text of the Python UI), and the
// tooltip of the field
std::string ffmpeg_missing_text(const std::string& path);
extern const char* FFMPEG_TOOLTIP;

struct VideoInfo {
    int W = 0, H = 0;
    std::string fps = "24/1";      // r_frame_rate
    int n = 0;                     // approximate frame count (0 = unknown)
    std::map<std::string, std::string> tags;    // color_space, color_primaries, color_transfer, color_range
    std::string pix_fmt;
    int bits_raw = 0;
    bool full_range = false;
    int rotation = 0;              // degrees counter-clockwise
    double fps_value() const;      // num / den (24 on errors), as main_window.open_video
};

// denoise_video_v5.probe (the fields the UI uses; the first decoded frame's time is not read: export only)
VideoInfo probe(const std::string& path);

// denoise_video_v5.read_frames(path, W, H, lo, hi, fps) (yuv420p uint8, constant-rate timeline when fps is
// given): calls on_frame(index, frame) until it returns false or the stream ends.
void read_frames(const std::string& path, int W, int H, int lo, int hi, const std::string& fps,
                 const std::function<bool(int, Frame&&)>& on_frame);

// nvfilter.matrix_from_tags(tags, height)
std::string matrix_from_tags(const std::map<std::string, std::string>& tags, int height);

struct SrcFmt {
    std::string family;  // "yuv" / "rgb"
    int bits = 8;
    bool is_float = false;
    std::string chroma;  // "420", "422", "444"
};
SrcFmt source_format(const std::string& name);       // nv_depth.source_format
std::string describe_source(const std::string& name); // nv_depth.describe_source

}  // namespace nvqt
