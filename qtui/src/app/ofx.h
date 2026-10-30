// ofx - the OFX host mode of the C++ UI (repro/nv_ui_qt/main.py _ofx_main + ofx_bridge.OfxFrameSource):
//   MeatVideoQt --ofx W H [FPS] [MAX_FRAMES] [--depth 8|16|32]
// reads raw 4:2:0 frames from stdin until EOF (or MAX_FRAMES), shows the UI with them, and writes the current frame,
// processed with the current settings (with an Auto Profile of it when no profile was made), to stdout: on Apply,
// and on closing unless the same frame / settings / profile were written last; Cancel closes without writing.
// Sample types and layout as ofx_bridge.py's docstring (8: uint8; 16: uint16 LE = 8-bit value x 256; 32: float32
// LE = 8-bit value / 255, NaN / +-inf written back unchanged).  The conversions are libnvcore's (depth.h).
#pragma once

#include <QStringList>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "depth.h"
#include "engine.h"

namespace nvqt {

class MainWindow;

// ofx_bridge.unpack_yuv420p: one frame of stdin (frame_size bytes) as the UI holds it (restore receives the
// non-finite samples of a depth-32 frame)
Frame ofx_frame(const uint8_t* data, int w, int h, int depth, RestoreMap* restore);
// main.py _emit after its signature check: frame t processed with P and the effective profile `eff` (the Auto Profile
// of frame t for nullptr) and packed for stdout; throws what the Python UI raises (e.what() = its message)
std::vector<uint8_t> ofx_process(const std::vector<Frame>& frames, int t, const PyObj& P, const PyObj* eff,
                                 bool full_range, const std::string& matrix, const PreviewEngine& engine, int depth,
                                 const RestoreMap* restore);

// rest = the arguments after --ofx; on_shown (tests: qtui/tests/ofx_drv.cpp) runs once the event loop has started,
// after the window is set up.  Returns the exit code (1 with the usage, "no frames received on stdin", or the
// traceback of a frame size Python cannot read (MemoryError / OverflowError) on stderr).  The numbers are read as
// Python's int() / float() read them (pynum.h).
int ofx_main(const QStringList& rest, const std::function<void(MainWindow*)>& on_shown = {});

}  // namespace nvqt
