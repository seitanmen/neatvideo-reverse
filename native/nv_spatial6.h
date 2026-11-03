// nv_spatial6 - C++ port of the genuine per-plane spatial filter FUN_0098edb0 (arm64 slice) with its shrink kernel
// FUN_00990f74 (fast path, directional path of sets 1..3, the cfg+0x90 detail stage), for the v6 spatial stage.
//
// Source of truth: the disassembly (work/omo/wp_spatial6/REPORT.md lists the addresses) and the Python transcription
// work/omo/wp_spatial/src/nv_spatial.py (plane_filter / shrink / _directional / _family_b / _detail), which this port
// follows operation by operation.  The leaves (element-wise helpers) reproduce the genuine functions bit for bit,
// NaN payloads included, with the NEON byte 0x14a67a8 = 1: the first n & ~3 elements take the NEON form, the rest the
// scalar form (work/omo/wp_spatial6: `nvh elem|argmax|gather|sel2|strip3|down2` against them).
//
// Not ported (Status NOT_IMPLEMENTED): set 4 (family C and the self-similar pattern protection FUN_00ad06ec.. of
// quality 2), a level whose height is < 2 (FUN_009f15b4 writes nothing), the scalar up-sampling with a width < 2,
// rect widths > 512 (FUN_00990f74 enters an unread hash block at 0x9915c0 and the line kernels hold 512 entries).
// The var output (10th argument of FUN_0098edb0) is not produced.
//
// Build with -ffp-contract=off -fno-fast-math (nv_portable: one AArch64 instruction per float operation).
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace nvs6 {

enum Status : int { OK = 0, NOT_IMPLEMENTED = 1, BAD_ARGUMENT = 2 };

// ------------------------------------------------------------------ leaves (n elements; NEON form on the first n & ~3)
void recip(float eps, const float* x, float* out, size_t n);                                // FUN_009ed5f8
void mul(const float* a, const float* b, float* out, size_t n);                             // FUN_009edc38
void sqr(const float* x, float* out, size_t n);                                             // FUN_009ed2f0
void sub(const float* a, const float* b, float* out, size_t n);                             // FUN_009ed710
void add(const float* a, const float* b, float* out, size_t n);                             // FUN_009ed844
void sqrtk(float k, const float* x, float* out, size_t n);                                  // FUN_009ed4e0
void sqrtabs(const float* x, float* out, size_t n);                                         // FUN_009ed3e0
void fabs(const float* x, float* out, size_t n);                                            // FUN_009ee50c
void rstep(float s, const float* x0, const float* x1, float* out, size_t n);                // FUN_009ef260
void robust(float t, const float* a, const float* e, float* out, size_t n);                 // FUN_009ef0d0
void rinv(float s, const float* x0, const float* x1, float* out, size_t n);                 // FUN_009eef5c
void scale(float s, const float* x, float* out, size_t n);                                  // FUN_009ee304
void clampmul(float a, float b, float lo, float hi, const float* x, const float* y, float* out, size_t n);  // FUN_009ee7bc
void affine(float a, float b, const float* x, float* out, size_t n);                        // FUN_009ee3fc
void acc(float s, const float* a, const float* b, const float* c, float* out, size_t n);    // FUN_009edeb8
void blend(float a, float b, const float* x, const float* y, float* out, size_t n);         // FUN_009ee1a4
void bladd(float c, float d, const float* A, const float* B, float* out, size_t n);         // FUN_009ed978
void ratio(float eps, const float* a, const float* b, float* out, size_t n);                // FUN_009ee04c
void ramp(float a, float b, float c, float d, const float* x, float* out, size_t n);        // FUN_009ee93c
void wmul(float a, float b, float c, float d, const float* r, const float* prev, float* out, size_t n);  // FUN_009eebf8
void pairw(const float* a, const float* b, const float* c, float* out, size_t n);           // FUN_009efb1c
// FUN_009ee5fc(maps, idx, max, n, stride, count): per element best = 0.0, idx = 0, then for j = 0..count-1
// x_j >= best replaces both (a tie goes to the later map; a NaN never wins)
void argmax(const float* maps, uint8_t* idx, float* mx, size_t n, size_t stride, int count);
// FUN_0098ed34(idx, maps, out, stride, n): out[i] = maps[idx[i] * stride + i]
void gather(const uint8_t* idx, const float* maps, float* out, size_t stride, size_t n);
// FUN_0098eba8(a, b, i3, i4, out, best, src, wt, flag, stride, n) (b, i4, best are updated in place)
void sel2(const float* a, float* b, const uint8_t* i3, uint8_t* i4, float* out, float* best, const float* src,
          const float* wt, int flag, size_t stride, size_t n);
// FUN_009ed014 / FUN_009ed164 run over the 4-column strips of the callers (W >= 4, H >= 2): the 3x3 box
// (mode 0 = replicated border, 1 = zero border) and the local RMS sqrt(k * box(x^2)), k = (s * 1/9) * s
void strip_box3(const float* x, int W, int H, int mode, float* out);
void strip_rms3(const float* x, int W, int H, int mode, float s, float* out);
// FUN_009f15b4 per row pair: (H >> 1) x (W >> 1)
void down2(const float* x, int W, int H, float* out);

// ------------------------------------------------------------------ the cfg of one plane (FUN_0099ead0 layout)
struct Cfg {
    float sigma[5] = {1.06f, 1, 1, 1, 1};   // +0x00 cfg[k]
    float amount[5] = {1, 1, 1, 1, 1};      // +0x14
    float c28[5] = {1, 1, 1, 1, 1};         // +0x28
    float c3c[5] = {1, 1, 1, 1, 1};         // +0x3c
    float c78[5] = {0, 0, 0, 0, 0};         // +0x78
    int c8c = 1;                            // +0x8c (byte)
    float c90[5] = {1, 1, 1, 0, 0};         // +0x90
    int ca4[5] = {1, 1, 1, 1, 1};           // +0xa4 (int)
    float b8 = 0;                           // +0xb8
    int levels = 4;                         // +0xc0
    int set = 2;                            // +0xc8
};

// Values the genuine takes from tamper-check state words (static analysis cannot fix them on a legitimate host): the
// popcounts of the two state words w25.  16 (the default) is the value under which neither has an effect.
struct Guard {
    // FUN_00990f74 0x993be0..0x993cd8: w20 = popcount - 16 as a 32-bit result (`sub w20`, zero-extended to x20); the
    // amount multiplier of a directional level is (float)(int64)(1 - x20) (0x9917a4..0x9917ac: sub x8, scvtf s1, x8),
    // i.e. 1 - (popcount - 16) for popcount >= 16 and 1 - (2^32 - (16 - popcount)) below
    int dir_popcount = 16;
    // FUN_0098edb0 0x98f590..0x98f718 computes [sp+0xbc] = popcount - 0x1f0 once; inside the level loop (0x98f774..,
    // back edges 0x98f754 / 0x9902c4) 0x98f77c..0x98f788 writes 0 to cfg[k] (the sigma word +4k of the caller's cfg)
    // before level k whenever popcount != 16, i.e. every level L-1..0 is processed with cfg[k] = 0
    int cfg_popcount = 16;
    // Not a genuine value: true lets a level wider than 512 run when it takes the fast path (no directional kernel),
    // as the v5-compatible whole-plane entry nv_spatial.spatial_genuine does (guard_w=False).  The genuine itself
    // only sees chunk-sized planes (its width >= 513 block 0x9915c0.. is not reached then).  Default false.
    bool allow_wide = false;
};

// Optional call log for the replay against the genuine leaves (work/omo/wp_spatial6/tools/replay.py): every leaf call
// of plane_filter is written as one line of dir/calls.txt plus its arrays dir/<id>.<name>.bin.
struct Recorder {
    std::string dir;
    long next = 0;
    FILE* index = nullptr;
    explicit Recorder(const std::string& d);
    ~Recorder();
};
void set_recorder(Recorder* r);

// FUN_0098edb0 on one plane (H x W row-major float32): sigma[k] is the sigma map of level k ((W >> k) x (H >> k)),
// model the plane's model words (>= 2005 floats; words 5+k, 55+5s+k, 80+80s+16k+d, 480+5s+k, 505+80s+16k+d are
// read), roi (y0, x0, h, w) of level 0 or nullptr (whole plane, no level is cut).  out: H x W.
// cfg_after (may be null) receives the cfg as the genuine leaves it: FUN_0098edb0 writes into the caller's cfg (the
// per-plane cfg of the context, ctx+0x3d10 / +0x3de0 / +0x3eb0), so with guard.cfg_popcount != 16 the zeroed sigma
// words persist for the later calls with the same cfg.  cfg itself is not modified; on NOT_IMPLEMENTED cfg_after is
// not written.
int plane_filter(const float* P, int W, int H, const std::vector<const float*>& sigma, const Cfg& cfg,
                 const float* model, const int* roi, float* out, const Guard& guard = Guard(), Cfg* cfg_after = nullptr);

}  // namespace nvs6
