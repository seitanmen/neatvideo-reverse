// nv_fieldproc - the field pipeline of interlaced frames (INT-2, .claude/tasks/interlace.md): every frame of a window
// is split into its two fields (nv_fields.h), the fields form one sequence in time, each field is filtered as
// process_frame_v5 filters a frame (temporal -> spatial -> Mix) over its neighbours in that sequence, and the two
// filtered fields are joined into the output frame.  The Python reference is native/fields_ref.py
// (process_frame_fields), checked bit for bit by native/verify_fieldproc.py.  The progressive path (process_frame_v5)
// is not touched.
//
// Field sequence: field 2k + j of window frame k has parity j ^ first (first = nvfield::TOP: the top field is the
// first in time; BOTTOM: the bottom one).  The Temporal Radius P.radius counts fields: the neighbours of field s are
// the fields s - R .. s + R inside the window (the genuine's Temporal Radius counts fields when it processes fields,
// docs/INTERLACE_PROGRESSIVE.md).
//
// The fields of the other parity sit half a field line from the centre field: a bottom field row i is the frame row
// 2i + 1, i.e. row i + 0.5 in the top field's rows; in 4:2:0 the chroma rows split the same way (chroma row c belongs
// to field c & 1), so the bottom field's chroma is half a chroma field row (one luma field row) below the top
// field's.  The modes:
//   PLAIN  the fields of both parities as they are (the motion estimate has to find the half line)
//   SHIFT  both parities, the half line given as a known offset (nvt::temporal_v5's shift: 0.5 * (parity of the
//          centre - parity of the neighbour) rows on the luma grid and the same on the chroma grid)
//   SAME   the fields of the centre's parity only (the neighbours at even distances <= R)
#pragma once

#include <vector>

#include "nv_temporal.h"

namespace nvfp {

using nvn::Plane;
using nvs::V5Params;
using nvs::V5Profile;
using nvt::Frame;

enum Mode { PLAIN = 0, SHIFT = 1, SAME = 2 };

// The two fields of a frame (each plane split by its own row parity, nv_fields.h; odd heights: the bottom field's
// pad row repeats its own last row)
void split_fields(const Frame& f, Frame* top, Frame* bottom);
// The frame of H luma rows and Hc chroma rows from its two fields
Frame join_fields(const Frame& top, const Frame& bottom, int H, int Hc);

// One field: `fields` = a field sequence, parity[k] of field k, ci = the centre.  The neighbours within P.radius
// fields (SAME: of the centre's parity only), temporal_v5 (SHIFT: with the half-line offsets), spatial_v5 and the
// Mix with fields[ci].
std::vector<Plane> process_field_v5(const std::vector<Frame>& fields, const std::vector<int>& parity, int ci, int mode,
                                    const V5Profile& prof, const V5Params& P);

// The output frame ci of `window` (frames around it, truncated at the clip ends as for process_frame_v5) processed
// field by field (first: TOP / BOTTOM, mode: PLAIN / SHIFT / SAME).  std::invalid_argument for another first or mode.
std::vector<Plane> process_frame_fields(const std::vector<Frame>& window, int ci, int first, int mode,
                                        const V5Profile& prof, const V5Params& P);

}  // namespace nvfp
