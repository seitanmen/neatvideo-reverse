// nv_model - C++ port of the per-plane model of BuildNoiseModel (FUN_00995250): all 4096 words of
// the struct `nvh model W H plane [set]` dumps.
//
// Source of truth: repro/nv_model.py (model_full) + repro/nv_model_special.py (FUN_00ad319c, the
// self-similar pattern part of set 4) + repro/nv_model_bm.py (FUN_00ad1ae0, the block-matching
// quantiles), with the default (FAST = True) paths of the Python port.  Words 0..54 come from
// nvb::model_levels (N1a).  native/verify_model.py checks every word bit for bit.
//
// Build with -ffp-contract=off -fno-fast-math.  On AArch64 the float operations whose operands can
// both be NaN are inline asm in the Python operand order (see nv_model.cpp); the direction bins of
// FUN_00acf420 call libSystem's atan2 / __sincos_stret (the functions the Python port calls through
// ctypes) through volatile function pointers.
#pragma once

#include <cstdint>

namespace nvm {

enum : int {
    LEVELS = 5,
    STRUCT_WORDS = 0x4000 / 4,
};

enum Status : int {
    OK = 0,
    NOT_IMPLEMENTED = 1,   // the Python port raises NotImplementedError (FUN_00ad11b0 without a bin,
                           // pad_mirror with pad >= side: bm_quantiles only)
    BAD_ARGUMENT = 2,      // the Python port raises ValueError (sides outside 2..512, levels outside 1..5)
};

// nv_model.model_full(plane, set_, levels=levels): plane is H x W row-major float32;
// set_ < 0: sets 0..4, else that set only.  out receives STRUCT_WORDS words (also on
// NOT_IMPLEMENTED the words written so far, which the caller must not use).
int model_full(const float* plane, int W, int H, int set_, uint32_t* out, int levels = LEVELS);

// nv_model_bm.ad1ae0(clean, A, cfg) (FUN_00ad1ae0) on its own, for the checks of its explicit-error
// paths (native/verify_model.py): clean and A are H x W row-major float32, q receives the quantiles
// 0.5 / 0.85 of the configuration (0: 16 px blocks, 1: 32 px blocks).  NOT_IMPLEMENTED where the
// Python port raises NotImplementedError: a side <= the padding (pad_mirror, the edge-replicating
// branch of FUN_0097ef28) or a block without a qualifying direction bin (FUN_00ad11b0 leaves the
// genuine's list uninitialised).  model_full reaches neither: FUN_00ad319c calls it with sides >= 84,
// where both are impossible (sandbox/omo/wp_native_model_ub/REPORT.md).
int bm_quantiles(const float* clean, const float* A, int W, int H, int cfg, float q[2]);

}  // namespace nvm
