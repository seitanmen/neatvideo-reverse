// ofx_session - the window of the OFX plug-in's two buttons (N4d):
//   MeatVideoQt --ofx-session
// reads the request of ofx/src/session.h on stdin (one JSON line, then the frames as raw gbrpf32le), shows the main
// window on those frames (Prepare Noise Profile or Adjust and Preview page, the current frame selected, the state of
// the request's ClipParams restored) and, when the window closes, writes the reply to stdout: "APPLY <n>\n" + the new
// ClipParams (Apply) or "CANCEL\n" (Cancel, the window closed).  Nothing else is written to stdout (fd 1 is pointed
// at stderr for the rest of the process).  The frames are converted as a gbrpf32le video at --depth 32 is
// (depth.h to_internal: full-range YCbCr 4:4:4 of the request's matrix).  ClipParams' format: ofx/src/params.h.
// With an empty ClipParams (a new instance) the window starts from the Default Profile and Default Preset, and its
// Reset Profile / Reset Filter Settings return to them.  MeatVideoQt --ofx-default writes the ClipParams a new
// instance gets from them (no window).  The existing --ofx host mode (ofx.h) is separate and unchanged.
#pragma once

#include <QStringList>

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "depth.h"
#include "fields.h"
#include "state.h"

namespace nvqt {

class MainWindow;

struct OfxSession {
    std::string page;                  // "prepare" / "adjust"
    int W = 0, H = 0;
    double fps = 24.0;
    std::string matrix;
    std::vector<double> times;         // host times of the frames (frame i of the window = times[i])
    int current = 0;
    std::string clip_params;           // the request's ClipParams
    int field_order = -1;              // INT-4: the host's field order (0 top / 1 bottom first; -1 none / not given)
    std::vector<RestoreMap> restore;   // per frame: the non-finite samples of the raw planes
};

// ClipParams of a state: the filter preset and the noise profile as the UI saves them, and what the plug-in's render
// needs (matrix ("auto": by the frame height), backend_params, the effective profile); frame_size null for W, H <= 0
// INT-4: il (an interlaced clip, the window's Video Scan Type) writes version 2 with "scan": "tff" | "bff" and the
// field processing (render.P["interlace"], fields.h); without it the text is the progressive version 1, unchanged
std::string clip_params_text(const PyObj& params, const ProfileState& ps, const std::string& matrix, bool full_range,
                             int W, int H, const std::optional<Interlace>& il = std::nullopt);
// ... of the window's state
std::string clip_params_json(MainWindow* win);

// the Default Profile / Default Preset of the UI's settings (Assign As Default / Change Default…): each nullopt when
// not assigned; *_error says why an assigned one cannot be loaded
inline const char* const DEFAULT_PROFILE_ERROR = "Selected default profile cannot be loaded.";
inline const char* const DEFAULT_PRESET_ERROR = "Selected default preset cannot be loaded.";
struct Defaults {
    std::optional<ProfileState> profile;
    std::optional<PyObj> preset;
    std::string profile_error, preset_error;
    QString message() const;           // the genuine's messages of the defaults that cannot be loaded ("" = none)
};
Defaults read_defaults();

// MeatVideoQt --ofx-default (no window, ofx/src/session.h): the ClipParams of a new instance from the defaults
int ofx_default_main();
// the window's state from a ClipParams text (throws std::runtime_error)
void apply_clip_params(MainWindow* win, const std::string& text);

// the session: returns the exit code (1 for a malformed request, with the reason on stderr).  on_shown (tests:
// qtui/tests/ofx_session_drv.cpp) runs once the event loop has started, after the window is set up.
int ofx_session_main(const QStringList& rest, const std::function<void(MainWindow*, const OfxSession&)>& on_shown = {});

}  // namespace nvqt
