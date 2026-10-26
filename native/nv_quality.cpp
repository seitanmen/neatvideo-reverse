#include "nv_quality.h"

#include <cmath>
#include <cstring>

namespace nvq {

namespace {

inline float bits(uint32_t b) {
    float f;
    std::memcpy(&f, &b, 4);
    return f;
}

const float NEG27 = bits(0xBD17B426u);  // -1/27
const float POS27 = bits(0x3D17B426u);  // +1/27
const float NEG81 = bits(0xBC4A4588u);  // -1/81
const float POS81 = bits(0x3C4A4588u);  // +1/81
const float HALF  = 0.5f;

// FUN_009d49b4(x, s=2.0, c=-0.25) = clamp(2.5 - x, 0, 1), computed in the genuine's op order.
inline float penalty(float x) {
    const float s = bits(0x40000000u);  // 2.0
    const float c = bits(0xBE800000u);  // -0.25
    float a = (0.5f / s) / c;
    a = a * x;
    float b = (c + -1.0f) * 0.5f;
    b = b / c;
    float v = a + b;
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

// Size term: min side on the 16/32/64/128/256 ladder (0 below 16, 5 at 256+), linear in between.
float size_term(int32_t W, int32_t H, int32_t fallback) {
    if (W == 0 || H == 0) return static_cast<float>(fallback);
    int m = W < H ? W : H;
    const int32_t T[5] = {16, 32, 64, 128, 256};
    if (m < T[0]) return 0.0f;
    if (m >= T[4]) return 5.0f;
    int k = 1;
    while (k < 4 && m >= T[k]) ++k;
    const float fm = static_cast<float>(m);
    const float lo = static_cast<float>(T[k - 1]);
    const float hi = static_cast<float>(T[k]);
    return static_cast<float>(k) + (fm - lo) / (hi - lo);
}

}  // namespace

float genuine_g(const uint8_t* p, size_t len) {
    (void)len;
    float s = 0.0f;
    for (int k = 0; k < 9; ++k) {
        for (int c = 0; c < 3; ++c) {
            const uint8_t* b = p + 0xbf44 + 0x120 * c + 0x20 * k;
            if (b[0x1e] != 0) {            // level > 3.0 flag
                s = s + NEG27;
            } else if (b[0x1d] != 0) {     // clip flag
                s = s + NEG27;
            } else {
                float w1;
                std::memcpy(&w1, b + 4, 4);            // word at +0x4 (first scale)
                if (w1 == 0.0f) {
                    s = s + NEG81;
                } else {
                    uint32_t marker;
                    std::memcpy(&marker, b + 0x1c, 4); // AFT marker word
                    if (marker != 0) {
                        s = s + POS27;
                    } else if (b[0x20] != 0) {         // next bin's level byte 0
                        s = s + POS81;                 // (ch2 bin8 reads 0xc2a4)
                    }
                }
            }
        }
    }
    return s > 0.0f ? s : 0.0f;  // fmax(s, 0): NaN compares false -> 0
}

float flatness(const float* s, bool luma) {
    const float c0 = bits(0x3F19999Au);   // 0.6  (chroma)
    const float c1 = bits(0x3E99999Au);   // 0.3  (luma)
    const float lim0 = bits(0x402CCCCCu);
    const float lim1 = bits(0x3FE66666u); // 1.8
    const float eps = bits(0x3727C5ACu);  // 1e-5
    const int k0 = luma ? 1 : 2;
    const float c = luma ? c1 : c0;
    const float lim = luma ? lim1 : lim0;

    float thr[7];
    thr[0] = c;
    for (int i = 1; i < 7; ++i) thr[i] = thr[i - 1] * 0.6f;

    float tot = 0.0f;
    int n = 0;
    for (int k = k0; k < 5; ++k) {
        const float q99 = s[5 + k];
        if (q99 < eps) break;          // NaN < eps is false: the chain continues (as in Python)
        const float* cv = s + 15 + 8 * k;
        float f = 1.0f;
        if (!(cv[1] < 0.0f)) {
            // fmaxnm(|cv[1]|/thr[0], 1): NaN v -> 1.0
            const float v1 = std::fabs(cv[1]) / thr[0];
            f = (std::isnan(v1) || v1 < 1.0f) ? 1.0f : v1;
            for (int j = 2; j < 8; ++j) {
                if (cv[j] < 0.0f) break;
                const float v = std::fabs(cv[j]) / thr[j - 1];
                if (v > f) f = v;      // NaN v compares false: skipped (as in Python)
            }
        }
        const float r = std::fabs(q99 / s[10 + k]) / lim;
        if (r > f) f = r;
        tot = tot + f;
        ++n;
    }
    return n ? tot / static_cast<float>(n) : 1.0f;
}

bool quality_parts(const uint8_t* p, size_t len, QualityParts* out) {
    if (len < PROF_SIZE || p[OFF_VALID] == 0) return false;
    int32_t mode;
    std::memcpy(&mode, p + OFF_MODE, 4);
    if (mode < 0 || mode > 3) mode = 1;
    static const uint8_t LUMA_FLAGS[12] = {1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0};

    float u[3];
    for (int i = 0; i < 3; ++i) {
        const int off = OFF_FACE3 + i * MODEL;
        float words[55];
        for (int w = 0; w < 55; ++w) std::memcpy(&words[w], p + off + 4 * w, 4);
        const bool luma = LUMA_FLAGS[mode * 3 + i] != 0;
        float f = flatness(words, luma);
        float v = penalty(f);
        if (!luma) v = v * HALF + HALF;
        u[i] = v;
    }
    float P = ((u[0] + HALF) * (u[1] + HALF)) * (u[2] + HALF);
    P = P + 0.25f;
    P = P / 3.625f;
    float g = genuine_g(p, len);
    float s11 = g * HALF + HALF;
    float q = s11 * P;
    int32_t W, H, cls;
    std::memcpy(&W, p + OFF_W, 4);
    std::memcpy(&H, p + OFF_H, 4);
    std::memcpy(&cls, p + OFF_SIZECLS, 4);
    const float S = size_term(W, H, cls);
    q = q * (S / 5.0f);
    for (int c = 0; c < 3; ++c)
        if (p[OFF_CLIPS + c] != 0) q = q * HALF;
    if (out) {
        for (int i = 0; i < 3; ++i) out->u[i] = u[i];
        out->P = P;
        out->s11 = s11;
        out->S = S;
        out->g = g;
        out->q = q;
    }
    return true;
}

bool quality_numbers(const uint8_t* p, size_t len, float* out_quality, float* out_completeness) {
    QualityParts part;
    if (!quality_parts(p, len, &part)) return false;
    if (out_quality) *out_quality = part.q;
    if (out_completeness) *out_completeness = part.g;
    return true;
}

}  // namespace nvq
