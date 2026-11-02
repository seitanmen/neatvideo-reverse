// fields - interlaced processing of the C++ UI (INT-3, .claude/tasks/interlace.md): the field split / join of
// native/nv_fields.h on the UI's frames and planes, the field pipeline of nvfp::process_frame_fields
// (native/nv_fieldproc.h; reference native/fields_ref.py) with the UI's stages (nvcore.h temporal with the half-line
// shifts, finish = per-channel spatial + Mix), and the field profiles ("Two fields" / "One field").
//
// Fields: top = the frame rows 0, 2, .., bottom = 1, 3, ..; both ceil(H/2) rows, the bottom field of an odd H ends
// with a pad row repeating its own last row (frame row H - 2; row 0 when H == 1).  Every plane is split by its own row
// parity (4:2:0 chroma row c belongs to field c & 1).
//
// The processing parameters travel in the backend parameter dict as P["interlace"] = {"first": 0 top / 1 bottom
// field first in time, "mode": "same" | "shift" | "plain", "radius": the Temporal Radius in fields}; a dict without
// "interlace" is the progressive path, unchanged.
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "nvcore.h"
#include "pyobj.h"

namespace nvqt {

enum FieldMode { FIELD_PLAIN = 0, FIELD_SHIFT = 1, FIELD_SAME = 2 };

struct Interlace {
    int first = 0;            // 0 top field first in time, 1 bottom field first
    int mode = FIELD_SAME;
    int radius = 2;           // Temporal Radius in fields
};

// P["interlace"] (nullopt: progressive); throws std::runtime_error on a malformed entry
std::optional<Interlace> interlace_of(const PyObj& P);
PyObj interlace_json(const Interlace& il);
const char* field_mode_name(int mode);            // "plain" / "shift" / "same"
int field_mode_of(const std::string& name);       // -1 for an unknown name
// the frames t - m .. t + m a field window around frame t needs (m = radius / 2 + 1)
inline int frame_margin(const Interlace& il) { return il.radius / 2 + 1; }

// the field `parity` of planes / of a stored frame (uint8 frames stay uint8)
Planes field_planes(const Planes& p, int parity);
Frame field_frame(const Frame& f, int parity);
// the frame of H luma and Hc chroma rows from its two fields
Planes join_planes(const Planes& top, const Planes& bottom, int H, int Hc);

// process_fields(window, ci, il, prof, P): the output frame ci of `window` (frames around it, truncated at the clip
// ends as for the frame path), field by field: every field filtered over its neighbours within il.radius fields
// (FIELD_SAME: of its own parity; FIELD_SHIFT: with the half-line offsets 0.5 (parity of the centre - parity of the
// neighbour) rows on both grids), temporal + finish as backend.process_frame does a frame, then joined.
// = field_finish(field_temporal(...)).
struct FieldStage {
    Planes src[2], tmp[2], rv[2];     // [0] top, [1] bottom field of frame ci: source, temporal result, residual var.
    int H = 0, Hc = 0;                // the frame's luma / chroma rows
    int n[2] = {0, 0};                // the fields each temporal stage averaged (centre included)
};
FieldStage field_temporal(const std::vector<Planes>& window, int ci, const Interlace& il, const PyObj& prof,
                          const PyObj& P);
Planes field_finish(const FieldStage& st, const PyObj& prof, const PyObj& P);
Planes process_fields(const std::vector<Planes>& window, int ci, const Interlace& il, const PyObj& prof,
                      const PyObj& P);
// the Assist Image of Profile Check field by field (finish with residual variance 1 on each field, joined)
Planes assist_fields(const Planes& frame, const PyObj& prof, const PyObj& P);

// ---------------------------------------------------------------- profiles
// prof["fields"]: "frame" (also for a profile without the key: every file made before INT-3), "two", "one";
// prof["field"]: "top" / "bottom", the profiled field (one) or the field the area is on (two, the first in time)
std::string profile_fields(const PyObj& prof);
int profile_parity(const PyObj& prof);            // 0 top / 1 bottom (-1: a frame profile)
// the image a profile was measured on: the frame itself (frame profile) or its field of profile_parity
Frame profile_image(const PyObj& prof, const Frame& f);
// Auto Fine-Tune of the profile on frame f (2026-11-02, Phase C work/omo/wp_aft_runtime/REPORT.md section 2): a frame
// profile runs on the frame and a "One field" profile on its field (profile_image; not observed in the genuine); a "Two
// fields" profile runs once on the RGB of the first field in time (profile_parity) with the RGB of the other field
// stacked below it, W x 2 ceil(H / 2) (the bottom field of an odd H keeps its pad row).  The genuine does this with the
// top field first for TFF material; the first field on top for BFF is an inference.  The area (on the first field)
// keeps its coordinates in the stacked image.
PyObj run_aft_fields(const PyObj& prof, const Frame& f, bool full_range, const std::string& matrix,
                     const CancelFn& cancel);
// build_profile on fields (Prepare page, interlaced): frames = the video, f = the profile frame, kind "two" / "one",
// parity = the field ("one": the profiled field; "two": the first field in time).  area (may be null) is in frame
// rows and is mapped to the field's rows (y0 / 2, h / 2).
//   one: the field sequence of that parity: profiling field = field of frame f, pair and the median curves = the
//        same field of frames f + 1, f + 2 (backend.build_profile on that sequence)
//   two: profiling field A = the first field of frame f; the curves of the next two fields in time (B of frame f, A
//        of frame f + 1) are median-combined with A's (both parities), the static-pixel pair is A of frame f + 1
//        (the same parity: fields of the other parity differ by the half line)
// The dict is build_profile's with "fields" / "field" added (and "ui_frame" = f).
PyObj build_profile_fields(const std::vector<Frame>& frames, int f, bool full_range, const std::string& matrix,
                           const std::string& mode, const std::vector<int>* area, const std::string& kind, int parity,
                           const CancelFn& cancel);

}  // namespace nvqt
