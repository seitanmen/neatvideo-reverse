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
    // INT-1 (probe(path, true) only; otherwise "" / -1): the stream's field_order (progressive / tt / bb / tb / bt;
    // "" when not reported) and the first decoded frame's interlaced_frame / top_field_first (-1: no frame decoded)
    std::string field_order;
    int frame_interlaced = -1, frame_tff = -1;
    double fps_value() const;      // num / den (24 on errors), as main_window.open_video
};

// denoise_video_v5.probe (the fields the UI uses; the first decoded frame's time is not read: export only).
// with_field_order (INT-1, the UI's open_video): the stream's field_order in the same ffprobe call, and a second
// ffprobe that decodes the first frame (-read_intervals %+#1) for its interlaced_frame / top_field_first.  The export
// keeps the call without them (its command is the one of the Python export).
VideoInfo probe(const std::string& path, bool with_field_order = false);

// The scan of a video (INT-1, display only; INT-2 takes the field order from here).  The first frame's flags decide:
// interlaced_frame 1 -> top_field_first 1 top field first / 0 bottom field first.  Without the flag (0, or no frame)
// the stream's field_order is used: tt top first, bb bottom first, progressive progressive; tb / bt only say
// "interlaced": ffmpeg 8.0.1 writes tb for top-field-first frames and bt for bottom-field-first ones in ffv1 / prores
// (mkv, mov), against AVFieldOrder's "tb = bottom displayed first", so they do not give the order.
enum class Scan { Unknown, Progressive, TopFirst, BottomFirst, Interlaced };
Scan scan_of(const std::string& field_order, int frame_interlaced, int frame_tff);
// the first field in time: 0 top, 1 bottom, -1 (progressive, unknown, or interlaced without a known order)
int first_field(Scan s);
// The status bar text: "progressive", "interlaced, top field first", "interlaced, bottom field first", "interlaced,
// field order unknown" or "scan unknown", followed by "(...)" when the two sources do not agree: "stream field_order:
// X" when the stream's value is not the one of the result (tt / bb / progressive), "first frame not flagged
// interlaced" when the result is interlaced from the stream alone
std::string scan_text(const std::string& field_order, int frame_interlaced, int frame_tff);


// denoise_video_v5.read_frames(path, W, H, lo, hi, fps) (yuv420p uint8, constant-rate timeline when fps is
// given): calls on_frame(index, frame) until it returns false or the stream ends.  vf (INT-3): an extra filter after
// the selection ("" = none: the command of the Python reader), e.g. interlaced_vf's scale=interl=1
void read_frames(const std::string& path, int W, int H, int lo, int hi, const std::string& fps,
                 const std::function<bool(int, Frame&&)>& on_frame, const std::string& vf = "");

// INT-3: the read filter of an interlaced clip: vf (the progressive one: "" or nv_depth.read_filter's
// scale=in_color_matrix=..:in_range=..) with swscale's interlaced mode (interl=1) when the conversion changes the
// vertical chroma sampling (a 4:2:0 source to 4:2:2 / 4:4:4 / RGB processing, or another source to 4:2:0), so that
// the fields' chroma rows are not mixed: an existing scale gets ":interl=1", else "scale=interl=1" is added.
// out_chroma: the processing format's chroma ("420" / "422" / "444"; RGB formats "444").
std::string interlaced_vf(const std::string& vf, const std::string& src_pix_fmt, const std::string& out_chroma);

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
