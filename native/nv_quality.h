// nv_quality - C++ port of the genuine Overall Profile Quality / Completeness
// (RS 0xa4acf8 quality + RS 0xa4b03c g + Demo 0x9b096c flatness + RS 0x9d49b4 penalty).
//
// Source of truth: repro/nv_ui_qt/quality.py and repro/nv_flat.py (bit-exact ports of the
// genuine functions; decode record in sandbox/omo/coord/verify_quality/static/QUALITY.md).
// The C++ code must produce bit-identical float32 results - native/verify_quality.py checks
// exactly that against the Python implementation.
//
// Rules: float32 arithmetic in the same order as the Python port; no fast-math; no FMA
// contraction (build with -ffp-contract=off).

#pragma once

#include <cstddef>
#include <cstdint>

namespace nvq {

// Blob offsets (nv_profile.build_profile layout, 0xc300 bytes).
enum : int {
    PROF_SIZE    = 0xc300,
    OFF_MODE     = 0x48,    // i32 colour mode (1 = YCrCb)
    MODEL        = 0x1f54,  // one model face
    OFF_FACE3    = 0x4c + 3 * MODEL,
    OFF_VALID    = 0xbf34,  // u8 profile-valid flag
    OFF_SIZECLS  = 0xbf38,  // i32 sample size class (used when W or H is 0)
    OFF_CLIPS    = 0xbf3c,  // u8 R/G/B IsClip > 0.33 flags
    OFF_CURVE    = 0xbf40,  // mode word; bins from 0xbf44
    OFF_LZMA     = 0xc2a4,  // u8 LZMA success flag (read as ch2 bin8's next byte)
    OFF_W        = 0xc2c0,  // i32 sample width
    OFF_H        = 0xc2c4,  // i32 sample height
};

// The two genuine percents. Returns false when p[0xbf34] == 0 (no profile).
// quality = s11 * P * (S/5) * 0.5^clips with s11 = 0.5*g + 0.5;
// completeness = g. Both are raw float32 (callers apply qRound(x*100)).
bool quality_numbers(const uint8_t* p, size_t len, float* out_quality, float* out_completeness);

// Intermediate parts for debugging bit differences (same order as quality_numbers).
struct QualityParts {
    float u[3];       // penalised flatness (chroma halved)
    float P;
    float s11;
    float S;          // raw size term (0..5)
    float g;
    float q;          // s11 * P * (S/5) * 0.5^clips
};
bool quality_parts(const uint8_t* p, size_t len, QualityParts* out);

// Curve completeness (max of the 27-bin float32 sum, in (bin, channel) order).
float genuine_g(const uint8_t* p, size_t len);

// FUN_009b096c: flatness of a model face from its words 0..54 (mean/q99/q50/cv per level).
float flatness(const float* words55, bool luma);

}  // namespace nvq
