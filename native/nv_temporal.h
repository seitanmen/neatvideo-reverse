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
void temporal_v5(const std::vector<Frame>& window, int ci, const V5Profile& prof, const V5Params& P,
                 std::map<int, Flow>* flows, std::vector<Plane>& out, std::vector<Plane>& rv);

// process_frame_v5(window, ci, prof, P, flows)
std::vector<Plane> process_frame_v5(const std::vector<Frame>& window, int ci, const V5Profile& prof,
                                    const V5Params& P, std::map<int, Flow>* flows);

// test entry points (verify_temporal.py): the _search block sum and _refine
namespace detail {
void block_sum13(const float* d, int nby, int nbx, int blk, Plane& s);
void refine_test(const Plane& cur, const Plane& ref, Plane& vy, Plane& vx, int blk, std::vector<Plane>& cost);
}  // namespace detail

}  // namespace nvt
