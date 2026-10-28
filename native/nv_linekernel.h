// nv_linekernel - the line kernels of FUN_00990448 (repro/nv_lines.py LineKernel and the entry-major
// LinePlan), shared by nv_curve (N1b, EstimateNoiseLevelCurve's pixel noise) and nv_model (N1e, the
// direction filters of BuildNoiseModel).  Both used to carry their own copy of this code.
//
// Source of truth: repro/nv_lines.py.  The offsets are frintn(t * i + 0.26f) with t = tanf of the
// folded angle (libSystem's tanf through a volatile pointer: no constant folding).  Build with
// -ffp-contract=off -fno-fast-math (t * i and + 0.26f stay two roundings).
#pragma once

#include <cstdint>
#include <vector>

namespace nvl {

struct LineKernel {
    int W = 0, H = 0;
    bool horiz = false, vert = false;
    int flag = 0;
    std::vector<int32_t> idx;                       // entries
    std::vector<int32_t> loff, lstart, lcount;      // lines
};

// Entry-major plan of an oriented kernel (nv_lines.LinePlan): G[j*L + l] = pixel of entry j of line l
// (HW = the -0.0 padding slot past the end of a line), inv[pixel] = j*L + l.
struct LinePlan {
    int m = 0, L = 0;
    std::vector<int32_t> G, inv;
};

// nv_lines.LineKernel.__init__ (FUN_00990448) for a W x H plane and the angle a (float32 radians).
// Throws std::invalid_argument for a side outside 1..512 (the genuine's index table holds 512 entries)
// or non-monotone offsets.  Horizontal / vertical kernels only set horiz / vert.
LineKernel make_kernel(int W, int H, float a);

// nv_lines.LinePlan of an oriented kernel (neither horiz nor vert).  Throws std::logic_error when the
// lines do not partition the plane.
LinePlan make_plan(const LineKernel& K);

}  // namespace nvl
