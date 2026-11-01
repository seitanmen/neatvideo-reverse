// nv_temporal - C++ port of the v5 temporal kernel of repro/nvfilter.py (N1g):
//   temporal_v5 (similarity "v5" and "pyramid"), estimate_flow_v5 (_search / _refine / _down2 /
//   gaussian / median), dense_flow, warp_plane, pyramid_weight, and process_frame_v5
//   (temporal -> spatial -> mix).  Same scalar-type assumptions as nv_spatial.h.
#pragma once

#include <map>
#include <vector>

#include "nv_spatial.h"

namespace nvt {

using nvn::Plane;
using nvs::V5Params;
using nvs::V5Profile;

struct Frame {
    Plane p[3];     // Y, Cb, Cr (float32)
};

struct Flow {
    Plane vy, vx;   // block vectors (nby, nbx), float32
};

// estimate_flow_v5(ref, cur, bs, search)
Flow estimate_flow_v5(const Plane& ref, const Plane& cur, int bs, int search);
// dense_flow(vy, vx, shape, bs) -> (FY, FX)
void dense_flow(const Flow& f, int H, int W, int bs, Plane& FY, Plane& FX);
// warp_plane(img, FY, FX)
Plane warp_plane(const Plane& img, const Plane& FY, const Plane& FX);
// pyramid_weight(d, s1, ratios, scale)
Plane pyramid_weight(const Plane& d, const Plane& s1, const double* ratios, double scale);

// temporal_v5(window, ci, prof, P, flows): `flows` maps frame offset (k - ci) to block vectors; a
// missing entry is estimated and stored (as the Python dict cache does).  Returns out (3 planes)
// and rv (3 planes).
// shift (INT-2, the field pipeline nv_fieldproc.h; native/fields_ref.py temporal_shift): nullptr is temporal_v5.
// Else shift[2k], shift[2k + 1] = the known vertical offset of window frame k against the centre, in rows of the
// luma plane and of the chroma planes (a field of the other parity: +-0.5 on both grids).  For a frame with a
// non-zero luma offset dy the flow is estimated against warp_plane(neighbour Y, dy, 0) and the luma is warped by
// FY + dy; a non-zero chroma offset dc is added to the chroma flow (FY, or to_grid(FY) / 2 for 4:2:0) after the
// motion.  Frames with both offsets 0 take exactly the temporal_v5 operations.
void temporal_v5(const std::vector<Frame>& window, int ci, const V5Profile& prof, const V5Params& P,
                 std::map<int, Flow>* flows, std::vector<Plane>& out, std::vector<Plane>& rv,
                 const double* shift = nullptr);

// process_frame_v5(window, ci, prof, P, flows)
std::vector<Plane> process_frame_v5(const std::vector<Frame>& window, int ci, const V5Profile& prof,
                                    const V5Params& P, std::map<int, Flow>* flows);

// test entry points (verify_temporal.py): the _search block sum and _refine
namespace detail {
void block_sum13(const float* d, int nby, int nbx, int blk, Plane& s);
void refine_test(const Plane& cur, const Plane& ref, Plane& vy, Plane& vx, int blk, std::vector<Plane>& cost);
}  // namespace detail

}  // namespace nvt
