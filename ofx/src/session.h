// session - the window of the two buttons (N4d): the helper "MeatVideoQt --ofx-session" in a separate process.
//
// The plug-in starts the helper with posix_spawn (no shell, no file opened by the plug-in), writes the request to its
// stdin and waits until the window is closed:
//   one line of JSON (UTF-8, ended by "\n"):
//     {"protocol": "meatvideo-ofx-session", "version": 1, "page": "prepare" | "adjust",
//      "width": W, "height": H, "fps": F, "matrix": "bt709" | "bt601", "format": "gbrpf32le",
//      "times": [t0, t1, ..], "current": k, "clip_params": "<the current ClipParams, may be empty>"}
//   then len(times) frames, each the raw gbrpf32le frame (G, B, R planes of W x H float32 LE, top-down rows) of the
//   host's RGBA image at that time (the same mapping as the render, core.h).
// The helper answers on its stdout (and nothing else goes there) with exactly one of
//     "APPLY <n>\n" followed by n bytes of the new ClipParams    (Apply)
//     "CANCEL\n"                                                  (Cancel / the window closed)
// and exits 0.  Anything else (another reply, a non-zero exit, a signal) leaves ClipParams unchanged.
//
// The helper is <the OFX bundle>/Contents/Helpers/MeatVideoQt.app/Contents/MacOS/MeatVideoQt, located from the
// plug-in's own image path (dladdr: dyld's in-memory list, no file access); the environment variable
// MEATVIDEO_OFX_HELPER names another executable (tests).  The child gets the host's environment without DYLD_* and
// the Qt plug-in path variables (the host's Qt must not leak into the helper's), fds 0 / 1 on the pipes, fd 2
// inherited, every other descriptor closed (POSIX_SPAWN_CLOEXEC_DEFAULT), default signal handlers.
#pragma once

#include <string>
#include <vector>

namespace mvofx {

struct SessionRequest {
    std::string page;                       // "prepare" / "adjust"
    int W = 0, H = 0;
    double fps = 24.0;
    std::string matrix;                     // "bt709" / "bt601"
    std::vector<double> times;
    int current = 0;                        // index into times
    std::string clip_params;
    std::vector<std::vector<float>> frames; // raw gbrpf32le frames (3 x W x H floats each), as times
};

struct SessionResult {
    enum Kind { APPLY, CANCEL, FAILED } kind = FAILED;
    std::string clip_params;                // APPLY
    std::string error;                      // FAILED: why
};

// The defaults of a new instance (N4d): the helper run as "MeatVideoQt --ofx-default" (no window, nothing on its
// stdin) answers "DEFAULTS <n> <profile> <preset>\n" + n bytes: the ClipParams the Default Profile and Default Preset
// of the UI's settings make (empty when neither is assigned), <profile> / <preset> each none / ok / error (assigned
// but not loadable, left out).  Its "render" has matrix "auto" (the frames are not known yet).
struct DefaultsResult {
    enum Status { NONE, OK, ERROR };
    Status profile = NONE, preset = NONE;
    std::string clip_params;
    std::string error;                      // the helper could not be asked ("" = answered)
};

std::string helper_path();                  // MEATVIDEO_OFX_HELPER or the bundled helper ("" when unknown)

// run the window and wait for it (blocks until the helper exits)
SessionResult run_session(const SessionRequest& rq);

// ask the helper for the defaults (blocks until it exits; no window)
DefaultsResult request_defaults();

}  // namespace mvofx
