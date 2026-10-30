# The default flow of the PySide6 UI (reference) without the window: Auto Profile on frame F (frames F-2..F+2 as
# qtui_g1eq reads them) + Auto Fine-Tune, then (Quality %, Completeness %) and the AFT bin flags.
#   NV_NATIVE=0|1 python3 py_quality.py VIDEO F
import json
import os
import sys

ROOT = "/Users/seitan/Desktop/work/reverse"
sys.path[:0] = [os.path.join(ROOT, "repro"), os.path.join(ROOT, "repro", "nv_ui_qt")]
import denoise_video_v5 as dv           # noqa: E402
import nvfilter as nv                   # noqa: E402
from nv_ui_qt import backend, quality   # noqa: E402
from nv_ui_qt.state import ProfileState  # noqa: E402

video, F = sys.argv[1], int(sys.argv[2])
info = dv.probe(video)
W, H = info["W"], info["H"]
matrix = nv.matrix_from_tags(info.get("tags", {}), H)
full_range = bool(info.get("full_range", False))
lo = F - 2
frames = [fr for _, fr in dv.read_frames(video, W, H, lo, F + 3, fps=info.get("fps"))]
prof = backend.build_profile(frames, F - lo, full_range, matrix, "auto")
ps = ProfileState(base=prof)
print("before AFT", quality.profile_numbers(ps, frames, full_range, matrix))
aft = backend.run_aft(prof, frames[backend.aft_frame(prof, frames)], full_range, matrix)
ps2 = ProfileState(base=aft)
qc = quality.profile_numbers(ps2, frames, full_range, matrix)
flags = aft.get("aft_bin_flags")
print(json.dumps({"native": backend.native_enabled(), "area": prof.get("area"), "quality_completeness": qc,
                  "aft_bin_flags": flags}))
