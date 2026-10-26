// nv_band - C++ port of repro/nv_band.py (see nv_band.h).  Every routine names the Python
// function it reproduces; the operation order (and the operand order, which decides NaN
// payloads) follows the Python code line by line.

#include "nv_band.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace nvb {

namespace {

inline float bits(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }
inline uint32_t ubits(float f) { uint32_t u; std::memcpy(&u, &f, 4); return u; }

// A value the compiler cannot see through: keeps x * 1.0 / fma(x, 1.0, c) as real operations
// (LLVM folds them, which changes signalling-NaN quieting and NaN operand priority).
inline float opaque(float x) {
#if defined(__aarch64__)
    __asm__ volatile("" : "+w"(x));
#else
    __asm__ volatile("" : "+x"(x));
#endif
    return x;
}

// a + b / a * b with the operand order kept.  LLVM may commute the operands of fadd / fmul
// (Reassociate's operand canonicalisation runs without fast-math), and the operand order decides
// which NaN propagates when both are NaN (FPProcessNaNs: first operand first, as numpy computes
// it).  Used wherever both operands can be NaN; products with a constant or a square are plain.
inline float A(float a, float b) {
#if defined(__aarch64__)
    float r;
    __asm__("fadd %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b));
    return r;
#else
    return a + b;
#endif
}
inline float M(float a, float b) {
#if defined(__aarch64__)
    float r;
    __asm__("fmul %s0, %s1, %s2" : "=w"(r) : "w"(a), "w"(b));
    return r;
#else
    return a * b;
#endif
}
inline double AD(double a, double b) {
#if defined(__aarch64__)
    double r;
    __asm__("fadd %d0, %d1, %d2" : "=w"(r) : "w"(a), "w"(b));
    return r;
#else
    return a + b;
#endif
}

// numpy float32 *scalar* arithmetic (the scalar code of nv_band.quantile / probit) propagates the
// second operand's NaN: with numpy 2.1.3 on arm64, np.float32 a + b and a * b behave as
// fadd / fmul (b, a) (array operations keep (a, b)).  These helpers give that order.
inline float AS(float a, float b) { return A(b, a); }
inline float MS(float a, float b) { return M(b, a); }

// Constants, from the Python bit patterns.
const float NEG_1E6 = bits(0xc9742400u);   // -1e6, "not measured"
const float EPS_1E6 = bits(0x358637bdu);   // 1e-6 (fmaxnm clamp)
const float Q99 = bits(0x3f7d70a4u);       // 0.99
const float Q50 = bits(0x3f000000u);       // 0.5
const float FMAX = bits(0x7f7fffffu);      // FLT_MAX
const float C0_25 = bits(0x3e800000u);
const float C0_75 = bits(0x3f400000u);
const float C3 = bits(0x40400000u);
const float C4 = bits(0x40800000u);
const float C0_0625 = bits(0x3d800000u);
const float CM0_5 = bits(0xbf000000u);     // -0.5
const float C0_5 = bits(0x3f000000u);
const float C1 = bits(0x3f800000u);
const float C100 = bits(0x42c80000u);
const float THIRD = bits(0x3eaaaaabu);     // nv_lines._THIRD (FUN_009ece90)
const size_t BLOCK = 0x800;                // FUN_009ef3fc / FUN_009ef520 block length

// FUN_00997bcc small-sample factors UNK_01244678 (nv_band.GQ_SMALL).
const uint32_t GQ_SMALL_BITS[12] = {0x0, 0x0, 0x3fd872b0, 0x3f9ef9db, 0x3f90e560, 0x3f8a7efa,
                                    0x3f86e979, 0x3f849ba6, 0x3f83126f, 0x3f81eb85, 0x3f810625,
                                    0x3f800000};
// DAT_01248e48 probit table (nv_band.PROBIT, 101 words + the word read past it at q = 1).
const uint32_t PROBIT_BITS[102] = {
    0xc0400000, 0xc014dd2f, 0xc00374bc, 0xbff0c49c, 0xbfe020c5, 0xbfd28f5c, 0xbfc70a3d, 0xbfbced91,
    0xbfb3d70a, 0xbfaba5e3, 0xbfa41893, 0xbf9d0e56, 0xbf966666, 0xbf9020c5, 0xbf8a3d71, 0xbf849ba6,
    0xbf7e76c9, 0xbf743958, 0xbf6a3d71, 0xbf60c49c, 0xbf578d50, 0xbf4e5604, 0xbf45a1cb, 0xbf3d2f1b,
    0xbf34bc6a, 0xbf2c8b44, 0xbf249ba6, 0xbf1ced91, 0xbf153f7d, 0xbf0d9168, 0xbf0624dd, 0xbefdf3b6,
    0xbeef9db2, 0xbee147ae, 0xbed2f1aa, 0xbec51eb8, 0xbeb74bc7, 0xbea9fbe7, 0xbe9c28f6, 0xbe8ed917,
    0xbe818937, 0xbe6978d5, 0xbe4ed917, 0xbe343958, 0xbe1a9fbe, 0xbe010625, 0xbdcccccd, 0xbd99999a,
    0xbd4ccccd, 0xbccccccd, 0x00000000, 0x3ccccccd, 0x3d4ccccd, 0x3d99999a, 0x3dcccccd, 0x3e010625,
    0x3e1a9fbe, 0x3e343958, 0x3e4ed917, 0x3e6978d5, 0x3e818937, 0x3e8ed917, 0x3e9c28f6, 0x3ea9fbe7,
    0x3eb74bc7, 0x3ec51eb8, 0x3ed2f1aa, 0x3ee147ae, 0x3eef9db2, 0x3efdf3b6, 0x3f0624dd, 0x3f0d9168,
    0x3f153f7d, 0x3f1ced91, 0x3f249ba6, 0x3f2c8b44, 0x3f34bc6a, 0x3f3d2f1b, 0x3f45a1cb, 0x3f4e5604,
    0x3f578d50, 0x3f60c49c, 0x3f6a3d71, 0x3f743958, 0x3f7e76c9, 0x3f849ba6, 0x3f8a3d71, 0x3f9020c5,
    0x3f966666, 0x3f9d0e56, 0x3fa41893, 0x3faba5e3, 0x3fb3d70a, 0x3fbced91, 0x3fc70a3d, 0x3fd28f5c,
    0x3fe020c5, 0x3ff0c49c, 0x400374bc, 0x4014dd2f, 0x40400000, 0x54494e38};

// nv_band.fmaxnm with a constant c > 0: a NaN x gives c.
inline float fmaxnm(float x, float c) { return (std::isnan(x) || x < c) ? c : x; }

// nv_band._reduce8: ((l4+l0)+(l5+l1)) + ((l6+l2)+(l7+l3)).
inline float reduce8(const float* a) {
    const float b0 = A(a[4], a[0]), b1 = A(a[5], a[1]), b2 = A(a[6], a[2]), b3 = A(a[7], a[3]);
    return A(A(b0, b1), A(b2, b3));
}

// nv_lines._lanes_run: running box of radius r along n steps; step k of the input is the
// L-lane vector X + k*xs, of the output O + k*os.  S: L floats of scratch.
void lanes_run(const float* X, ptrdiff_t xs, float* O, ptrdiff_t os, int n, int L, int r,
               int mode, bool prefix4, float* S) {
    const float recip = C1 / static_cast<float>(2 * r + 1);
    const float fr = opaque(static_cast<float>(r));
    const float fr1 = opaque(static_cast<float>(r + 1));
    const float frn = opaque(static_cast<float>(r + 1 - n));
    auto Xk = [&](int k) { return X + static_cast<ptrdiff_t>(k) * xs; };
    auto Ok = [&](int k) { return O + static_cast<ptrdiff_t>(k) * os; };
    const float* first = Xk(0);
    const float* last = Xk(n - 1);
    for (int l = 0; l < L; ++l) S[l] = 0.0f;
    if (n <= r + 1) {
        for (int k = 0; k < n; ++k) {
            const float* x = Xk(k);
            for (int l = 0; l < L; ++l) S[l] = A(S[l], x[l]);
        }
        if (mode == 0) {
            for (int l = 0; l < L; ++l) {
                float t = first[l] * fr;
                t = A(t, last[l] * frn);
                S[l] = A(S[l], t);
            }
            for (int j = 0; j < n; ++j) {
                float* o = Ok(j);
                for (int l = 0; l < L; ++l) {
                    o[l] = recip * S[l];
                    S[l] = A(last[l], S[l]);
                    S[l] = S[l] - first[l];
                }
            }
        } else {
            for (int j = 0; j < n; ++j) {
                float* o = Ok(j);
                for (int l = 0; l < L; ++l) o[l] = recip * S[l];
            }
        }
        return;
    }
    for (int k = 0; k < r; ++k) {
        const float* x = Xk(k);
        for (int l = 0; l < L; ++l) S[l] = A(S[l], x[l]);
    }
    if (mode == 0)
        for (int l = 0; l < L; ++l) S[l] = A(first[l] * fr1, S[l]);
    const int g = std::min(r, n - r - 1);
    for (int j = 0; j <= g; ++j) {                         // growing: right end inside
        const float* x = Xk(j + r);
        float* o = Ok(j);
        for (int l = 0; l < L; ++l) {
            S[l] = A(S[l], x[l]);
            if (mode == 0) S[l] = S[l] - first[l];
            o[l] = recip * S[l];
        }
    }
    if (n <= 2 * r) {
        for (int j = n - r; j <= r; ++j) {                 // both ends outside
            float* o = Ok(j);
            for (int l = 0; l < L; ++l) {
                if (mode == 0) {
                    S[l] = A(last[l], S[l]);
                    S[l] = S[l] - first[l];
                }
                o[l] = recip * S[l];
            }
        }
    } else {
        int j = r + 1;
        const int M = n - 2 * r - 1;
        if (prefix4) {
            for (int s = 0; s < M % 4; ++s, ++j) {
                const float* xa = Xk(j + r);
                const float* xb = Xk(j - r - 1);
                float* o = Ok(j);
                for (int l = 0; l < L; ++l) {
                    S[l] = A(S[l], xa[l]);
                    S[l] = S[l] - xb[l];
                    o[l] = recip * S[l];
                }
            }
            for (; j <= n - r - 1; j += 4) {
                const float *a0 = Xk(j + r), *a1 = Xk(j + r + 1), *a2 = Xk(j + r + 2),
                            *a3 = Xk(j + r + 3);
                const float *b0 = Xk(j - r - 1), *b1 = Xk(j - r), *b2 = Xk(j - r + 1),
                            *b3 = Xk(j - r + 2);
                float *o0 = Ok(j), *o1 = Ok(j + 1), *o2 = Ok(j + 2), *o3 = Ok(j + 3);
                for (int l = 0; l < L; ++l) {
                    const float d0 = a0[l] - b0[l];
                    const float d1 = a1[l] - b1[l];
                    const float d2 = a2[l] - b2[l];
                    const float d3 = a3[l] - b3[l];
                    const float p1 = A(d0, d1);
                    const float p2 = A(d2, p1);
                    const float t = A(d2, d3);
                    const float p3 = A(p1, t);
                    o0[l] = recip * A(S[l], d0);
                    o1[l] = recip * A(S[l], p1);
                    o2[l] = recip * A(S[l], p2);
                    o3[l] = recip * A(S[l], p3);
                    S[l] = A(S[l], p3);
                }
            }
        } else {
            for (; j <= n - r - 1; ++j) {
                const float* xa = Xk(j + r);
                const float* xb = Xk(j - r - 1);
                float* o = Ok(j);
                for (int l = 0; l < L; ++l) {
                    S[l] = A(S[l], xa[l]);
                    S[l] = S[l] - xb[l];
                    o[l] = recip * S[l];
                }
            }
        }
    }
    for (int j = std::max(n - r, r + 1); j < n; ++j) {    // left end inside only
        const float* xb = Xk(j - r - 1);
        float* o = Ok(j);
        for (int l = 0; l < L; ++l) {
            if (mode == 0) {
                S[l] = A(last[l], S[l]);
                S[l] = S[l] - xb[l];
            } else {
                S[l] = S[l] - xb[l];
            }
            o[l] = recip * S[l];
        }
    }
}

// nv_lines._box3_rows_bm on one row: ((x + prev) + next) * (1/3).
void box3_row(const float* X, int W, int mode, float* O) {
    for (int i = 1; i < W; ++i) O[i] = A(X[i], X[i - 1]);
    O[0] = A(X[0], mode == 0 ? X[0] : 0.0f);
    for (int i = 0; i < W - 1; ++i) O[i] = A(O[i], X[i + 1]);
    O[W - 1] = A(O[W - 1], mode == 0 ? X[W - 1] : 0.0f);
    for (int i = 0; i < W; ++i) O[i] = O[i] * THIRD;
}

// nv_pixnoise._up2_scalar: (Hc, Wc) -> (2Hc + oddH, 2Wc + oddW).  Wc >= 2.
void up2_scalar(const float* lo, int Wc, int Hc, int oddW, int oddH, float* out) {
    const int W2 = 2 * Wc + oddW;
    std::vector<float> s(Wc), a3(Wc);
    for (int rr = 0; rr < Hc; ++rr) {
        const float* x = lo + static_cast<ptrdiff_t>(rr) * Wc;
        for (int r0 = 0; r0 < 2; ++r0) {
            const int nb = r0 == 0 ? (rr > 0 ? rr - 1 : 0) : (rr + 1 < Hc ? rr + 1 : Hc - 1);
            const float* y = lo + static_cast<ptrdiff_t>(nb) * Wc;
            for (int i = 0; i < Wc; ++i) s[i] = A(x[i] * C3, y[i]);
            for (int i = 0; i < Wc; ++i) a3[i] = s[i] * C3;
            float* row = out + static_cast<ptrdiff_t>(2 * rr + r0) * W2;
            row[0] = (s[0] * C4) * C0_0625;
            for (int i = 1; i < Wc; ++i) row[2 * i] = A(a3[i], s[i - 1]) * C0_0625;
            for (int i = 0; i < Wc - 1; ++i) row[2 * i + 1] = A(a3[i], s[i + 1]) * C0_0625;
            row[2 * Wc - 1] = (s[Wc - 1] * C4) * C0_0625;
        }
    }
    if (oddW)
        for (int r = 0; r < 2 * Hc; ++r) {
            float* row = out + static_cast<ptrdiff_t>(r) * W2;
            row[2 * Wc] = row[2 * Wc - 1];
        }
    if (oddH)
        std::memcpy(out + static_cast<ptrdiff_t>(2 * Hc) * W2,
                    out + static_cast<ptrdiff_t>(2 * Hc - 1) * W2, sizeof(float) * W2);
}

// nv_band._borders.
void borders(int Wk, int Hk, int b, int* pbx, int* pby, bool* psmall) {
    int bx = Wk > 2 * b ? b : (((Wk - 1) >= 0 ? (Wk - 1) : Wk) >> 1);
    int by = Hk > 2 * b ? b : (((Hk - 1) >= 0 ? (Hk - 1) : Hk) >> 1);
    bool small = Wk < 2 * b + 1 || Hk < 2 * b + 1;
    while (!(static_cast<long long>(Wk - 2 * bx) * (Hk - 2 * by) > 31 || (bx == 0 && by == 0))) {
        if (bx > 0 || by > 0) small = true;
        bx -= bx > 0;
        by -= by > 0;
    }
    *pbx = bx;
    *pby = by;
    *psmall = small;
}

// Count of v <= t over a sorted copy (nv_band.quantile.count): NaN is never counted; a NaN t
// gives n for n <= 4096 (numpy searchsorted puts NaN last) and 0 above (count_nonzero).
// The sort is an LSD radix sort (3 passes of 11 bits) of the order-preserving keys of the
// non-NaN values (-0 sorts before +0, which counts with <= do not distinguish).
struct Counter {
    std::vector<float> s;
    size_t n = 0;
    bool built = false;
    void build(const float* v, size_t n_) {
        n = n_;
        built = true;
        std::vector<uint32_t> k, tmp;
        k.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            if (std::isnan(v[i])) continue;
            const uint32_t u = ubits(v[i]);
            k.push_back((u & 0x80000000u) ? ~u : (u | 0x80000000u));
        }
        const size_t nn = k.size();
        tmp.resize(nn);
        for (int pass = 0; pass < 3; ++pass) {
            const int sh = 11 * pass;
            size_t hist[2049] = {0};
            for (size_t i = 0; i < nn; ++i) ++hist[((k[i] >> sh) & 0x7ffu) + 1];
            for (int b = 0; b < 2048; ++b) hist[b + 1] += hist[b];
            for (size_t i = 0; i < nn; ++i) tmp[hist[(k[i] >> sh) & 0x7ffu]++] = k[i];
            k.swap(tmp);
        }
        s.resize(nn);
        for (size_t i = 0; i < nn; ++i) {
            const uint32_t key = k[i];
            s[i] = bits((key & 0x80000000u) ? (key & 0x7fffffffu) : ~key);
        }
    }
    size_t count(float t) const {
        if (std::isnan(t)) return n <= 4096 ? n : 0;
        return static_cast<size_t>(std::upper_bound(s.begin(), s.end(), t) - s.begin());
    }
};

long long fcvtzs_d(double x) {
    if (x != x) return 0;
    if (x >= 9223372036854775808.0) return INT64_MAX;
    if (x < -9223372036854775808.0) return INT64_MIN;
    return static_cast<long long>(x);
}

// nv_band._local_rms_crop on one plane: lo (Hk>>1 x Wk>>1) and the cropped local RMS map.
void local_rms_crop(const float* src, int Wk, int Hk, int bx, int by, std::vector<float>& lo,
                    std::vector<float>& crop, std::vector<float>& a, std::vector<float>& b) {
    const size_t n = static_cast<size_t>(Wk) * Hk;
    a.resize(n);
    b.resize(n);
    box_rows(src, Wk, Hk, 1, 0, a.data());
    box_cols(a.data(), Wk, Hk, 1, 0, b.data());           // box3 in b
    lo.resize(static_cast<size_t>(Wk >> 1) * (Hk >> 1));
    down2(b.data(), Wk, Hk, lo.data());
    band_from(lo.data(), Wk >> 1, Hk >> 1, src, Wk, Hk, a.data());   // band in a
    for (size_t i = 0; i < n; ++i) a[i] = a[i] * a[i];
    box_rows(a.data(), Wk, Hk, 1, 1, b.data());
    box_cols(b.data(), Wk, Hk, 1, 1, a.data());           // local mean square in a
    for (size_t i = 0; i < n; ++i) a[i] = std::sqrt(std::fabs(a[i]));
    const int h = Hk - 2 * by, w = Wk - 2 * bx;
    crop.resize(static_cast<size_t>(w) * h);
    for (int y = 0; y < h; ++y)
        std::memcpy(crop.data() + static_cast<size_t>(y) * w,
                    a.data() + static_cast<size_t>(y + by) * Wk + bx, sizeof(float) * w);
}

bool quantile_impl(const float* v, size_t n, float q, int flag, int div, float* result, float m,
                   float sd, Counter& cnt);

// nv_band._measure_bm on one plane: words k, 5+k, 10+k, 15+8k .. 22+8k.
void measure(const std::vector<float>& crop, int w, int h, uint32_t* out, int k) {
    const size_t n = static_cast<size_t>(w) * h;
    const float* V = crop.data();
    const float m = mean_exact(V, n);
    out[k] = ubits(fmaxnm(m, EPS_1E6));
    const float sd = std_exact(V, n, m);
    float q99 = 0.0f, q50 = 0.0f;
    Counter cnt;                               // one sorted copy for both quantiles
    quantile_impl(V, n, Q99, 1, 4, &q99, m, sd, cnt);
    quantile_impl(V, n, Q50, 1, 4, &q50, m, sd, cnt);
    out[5 + k] = ubits(fmaxnm(q99, EPS_1E6));
    out[10 + k] = ubits(fmaxnm(q50, EPS_1E6));
    float c[8];
    cv(V, w, h, m, c);
    for (int j = 0; j < 8; ++j) out[15 + 8 * k + j] = ubits(c[j]);
}

// nv_band._extrapolate: fac < 0 selects the FUN_009980e8 form |prev| * -0.5.
void extrapolate(uint32_t* out, int k, const float* fac) {
    for (int base = 0; base <= 10; base += 5) {
        const float prev = bits(out[base + k - 1]);
        float val;
        if (!fac)
            val = std::fabs(prev) * CM0_5;
        else
            val = M(*fac, -std::fabs(prev));
        out[base + k] = ubits(prev == NEG_1E6 ? NEG_1E6 : val);
    }
    for (int j = 0; j < 8; ++j) out[15 + 8 * k + j] = out[15 + 8 * (k - 1) + j];
}

}  // namespace

// ------------------------------------------------------------------------------ public parts

void init_struct(uint32_t* out) {
    static const struct { int i, n; uint32_t v; } RUNS[] = {
        {0x000, 1305, 0xc9742400u}, {0x519, 40, 0xbf800000u}, {0x541, 640, 0xffffffffu},
        {0x7c1, 20, 0xbf800000u}};
    std::memset(out, 0, sizeof(uint32_t) * STRUCT_WORDS);
    for (const auto& r : RUNS)
        for (int j = 0; j < r.n; ++j) out[r.i + j] = r.v;
}

float mean_exact(const float* v, size_t n) {
    double d = 0.0;
    const size_t nfull = n / BLOCK;
    for (size_t bk = 0; bk < nfull; ++bk) {
        const float* x = v + bk * BLOCK;
        float acc[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        for (size_t i = 0; i < BLOCK / 8; ++i)
            for (int j = 0; j < 8; ++j) acc[j] = A(acc[j], x[8 * i + j]);
        d = AD(d, static_cast<double>(reduce8(acc)));
    }
    const size_t rem = n - nfull * BLOCK;
    if (rem) {
        const float* T = v + nfull * BLOCK;
        const size_t m8 = rem & ~static_cast<size_t>(7);
        float acc[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        for (size_t i = 0; i < m8 / 8; ++i)
            for (int j = 0; j < 8; ++j) acc[j] = A(acc[j], T[8 * i + j]);
        float s = reduce8(acc);
        for (size_t j = m8; j < rem; ++j) s = A(s, T[j]);
        d = AD(d, static_cast<double>(s));
    }
    return static_cast<float>(d / static_cast<double>(n));
}

float std_exact(const float* v, size_t n, float m) {
    double d = 0.0;
    const size_t nfull = n / BLOCK;
    for (size_t bk = 0; bk < nfull; ++bk) {
        const float* x = v + bk * BLOCK;
        float acc[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        for (size_t i = 0; i < BLOCK / 8; ++i)
            for (int j = 0; j < 8; ++j) {
                const float dd = x[8 * i + j] - m;
                acc[j] = std::fma(dd, dd, acc[j]);         // FMLA
            }
        d = AD(d, static_cast<double>(reduce8(acc)));
    }
    const size_t rem = n - nfull * BLOCK;
    if (rem) {
        const float* T = v + nfull * BLOCK;
        const size_t m8 = rem & ~static_cast<size_t>(7);
        float acc[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        for (size_t i = 0; i < m8 / 8; ++i)
            for (int j = 0; j < 8; ++j) {
                const float dd = T[8 * i + j] - m;
                acc[j] = std::fma(dd, dd, acc[j]);
            }
        float s = reduce8(acc);
        for (size_t j = m8; j < rem; ++j) {
            const float t = T[j] - m;
            s = A(s, t * t);
        }
        d = AD(d, static_cast<double>(s));
    }
    return static_cast<float>(std::sqrt(d / static_cast<double>(n)));
}

float probit(float q) {
    float x;
    if (std::isnan(q)) {
        x = q;
    } else {                                   // Python min(max(float(q), 0.0), 1.0)
        double qd = static_cast<double>(q);
        if (0.0 > qd) qd = 0.0;
        if (1.0 < qd) qd = 1.0;
        x = static_cast<float>(qd);
    }
    x = x * C100;
    const int i = std::isnan(x) ? 0 : static_cast<int>(x);
    const float f = x - static_cast<float>(i);
    return AS((C1 - f) * bits(PROBIT_BITS[i]), f * bits(PROBIT_BITS[i + 1]));
}

bool quantile(const float* v, size_t n, float q, int flag, int div, float* result, const float* pm,
              const float* psd) {
    if (q >= C1) return false;                 // FUN_009efdd4 maximum: not ported
    if (div < 0) return false;
    const float m = pm ? *pm : mean_exact(v, n);
    const float sd = psd ? *psd : std_exact(v, n, m);
    Counter cnt;
    return quantile_impl(v, n, q, flag, div, result, m, sd, cnt);
}

namespace {

// nv_band.quantile after m / sd; cnt is built on first use (shared between calls on v).
bool quantile_impl(const float* v, size_t n, float q, int flag, int div, float* result, float m,
                   float sd, Counter& cnt) {
    if (q >= C1) return false;
    if (div < 0) return false;
    if (sd == 0.0f) {
        *result = 0.0f;
        return true;
    }
    if (!cnt.built) cnt.build(v, n);
    const float nf = static_cast<float>(n);
    const float target = MS(nf, q);
    float t = m, step = sd;
    int evals = 0;
    bool done = false;
    for (int ph = 0; ph < 8; ++ph) {
        if (step > 0.0f) {
            for (;;) {
                if (evals > 99) { done = true; break; }
                ++evals;
                t = AS(step, t);
                if (target <= static_cast<float>(cnt.count(t))) break;      // b.ls
            }
        } else if (step < 0.0f) {
            for (;;) {
                if (evals > 99) { done = true; break; }
                ++evals;
                t = AS(step, t);
                const float c = static_cast<float>(cnt.count(t));
                if (!(target < c || std::isnan(target))) break;             // b.lt loops
            }
        } else {                               // zero or NaN step
            if (evals <= 99) t = AS(step, t);
            done = true;
        }
        if (done) break;
        step = step * CM0_5;
    }
    if (n <= 49) {
        if (n < 1) {
            t = -FMAX;
        } else {
            float upper = FMAX, lower = -FMAX;
            for (size_t i = 0; i < n; ++i) {
                const float x = v[i];
                if (x >= t && upper > x) upper = x;
                if (x < t && lower < x) lower = x;
            }
            if (upper == FMAX) {
                t = lower;
            } else if (lower == -FMAX) {
                t = upper;
            } else {
                const float c_up = static_cast<float>(cnt.count(upper));
                size_t kk = 0;
                for (size_t i = 0; i < n; ++i)
                    if (v[i] > lower && v[i] <= upper) ++kk;
                const float k = static_cast<float>(kk);
                const float slope = (upper - lower) / k;
                t = AS(MS(slope * nf, q), upper - slope * c_up);
            }
        }
    }
    const long long d = div ? static_cast<long long>(n) / div : 0;
    long long lim = fcvtzs_d(1.0 / (1.0 - static_cast<double>(q)));
    lim = lim > 10 ? lim : 10;
    const float g = AS(MS(probit(q), sd) * bits(GQ_SMALL_BITS[d < 11 ? d : 11]), m);
    *result = (d < lim && (flag & 1) && g > t) ? g : t;
    return true;
}

}  // namespace

void cv(const float* E, int w, int h, float m, float* out8) {
    for (int j = 0; j < 8; ++j) out8[j] = NEG_1E6;
    if (m == 0.0f) return;                     // fcmp m, #0; b.eq (a NaN m goes on)
    std::vector<float> X(E, E + static_cast<size_t>(w) * h), d, nxt;
    const float one = opaque(C1);
    const float nm = -m;
    for (int j = 0; j < 8; ++j) {
        if (static_cast<long long>(h) * w < 4) break;
        const size_t n = static_cast<size_t>(h) * w;
        d.resize(n);
        const size_t k = n & ~static_cast<size_t>(3);
        for (size_t i = 0; i < k; ++i) d[i] = std::fma(X[i], one, nm);      // FUN_009ee3fc FMLA
        for (size_t i = k; i < n; ++i) d[i] = A(X[i] * one, nm);
        for (size_t i = 0; i < n; ++i) d[i] = d[i] * d[i];
        out8[j] = std::sqrt(mean_exact(d.data(), n)) / m;
        if (j < 7) {
            nxt.resize(static_cast<size_t>(h >> 1) * (w >> 1));
            down2(X.data(), w, h, nxt.data());
            X.swap(nxt);
            w >>= 1;
            h >>= 1;
        }
    }
}

void box_rows(const float* X, int W, int H, int r, int mode, float* out) {
    bool prefix4 = false;
    if (W > 2 * r) {
        if (r == 1 && W >= 8 && W % 4 == 0) {
            for (int y = 0; y < H; ++y)
                box3_row(X + static_cast<ptrdiff_t>(y) * W, W, mode,
                         out + static_cast<ptrdiff_t>(y) * W);
            return;
        }
        prefix4 = true;
    }
    float S;
    for (int y = 0; y < H; ++y)
        lanes_run(X + static_cast<ptrdiff_t>(y) * W, 1, out + static_cast<ptrdiff_t>(y) * W, 1, W, 1,
                  r, mode, prefix4, &S);
}

void box_cols(const float* X, int W, int H, int r, int mode, float* out) {
    std::vector<float> S(W);
    lanes_run(X, W, out, W, H, W, r, mode, false, S.data());
}

void down2(const float* x, int W, int H, float* out) {
    const int h = H >> 1, w = W >> 1;
    const int c = (W & ~7) >> 1;
    for (int r = 0; r < h; ++r) {
        const float* r0 = x + static_cast<ptrdiff_t>(2 * r) * W;
        const float* r1 = r0 + W;
        float* o = out + static_cast<ptrdiff_t>(r) * w;
        for (int i = 0; i < c; ++i)
            o[i] = A(A(r0[2 * i], r0[2 * i + 1]), A(r1[2 * i], r1[2 * i + 1])) * C0_25;
        for (int i = c; i < w; ++i)
            o[i] = A(A(A(r0[2 * i], r0[2 * i + 1]), r1[2 * i]), r1[2 * i + 1]) * C0_25;
    }
}

void band_from(const float* lo, int Wc, int Hc, const float* src, int W2, int H2, float* out) {
    const int oddW = W2 - 2 * Wc, oddH = H2 - 2 * Hc;
    const size_t N = static_cast<size_t>(W2) * H2;
    if (Hc < 2 || Wc < 4) {                    // nv_pixnoise.band_from_bm scalar path
        std::vector<float> up(N);
        up2_scalar(lo, Wc, Hc, oddW, oddH, up.data());
        for (size_t i = 0; i < N; ++i) out[i] = src[i] - up[i];
        return;
    }
    const int W2c = 2 * Wc;
    std::vector<float> hbuf(static_cast<size_t>(Hc) * W2c);
    for (int r = 0; r < Hc; ++r) {             // columns
        const float* l = lo + static_cast<ptrdiff_t>(r) * Wc;
        float* hr = hbuf.data() + static_cast<ptrdiff_t>(r) * W2c;
        for (int i = 0; i < Wc; ++i) {
            const float xs = l[i] * C0_75;
            const float ql = l[i > 0 ? i - 1 : 0] * C0_25;
            hr[2 * i] = A(xs, ql);
            const float qr = l[i + 1 < Wc ? i + 1 : Wc - 1] * C0_25;
            hr[2 * i + 1] = A(xs, qr);
        }
    }
    auto U = [&](int r) { return out + static_cast<ptrdiff_t>(r) * W2; };
    auto Hh = [&](int r) { return hbuf.data() + static_cast<ptrdiff_t>(r) * W2c; };
    std::memcpy(U(0), Hh(0), sizeof(float) * W2c);
    std::memcpy(U(2 * Hc - 1), Hh(Hc - 1), sizeof(float) * W2c);
    for (int i = 0; i < Hc - 1; ++i) {         // rows
        const float* hp = Hh(i);
        const float* hn = Hh(i + 1);
        float* odd = U(2 * i + 1);
        float* even = U(2 * i + 2);
        for (int c = 0; c < W2c; ++c) {
            odd[c] = A(hp[c] * C0_75, hn[c] * C0_25);
            even[c] = A(hp[c] * C0_25, hn[c] * C0_75);
        }
    }
    for (int r = 0; r < 2 * Hc; ++r) {
        float* o = U(r);
        const float* s = src + static_cast<ptrdiff_t>(r) * W2;
        for (int c = 0; c < W2c; ++c) o[c] = s[c] - o[c];
    }
    if (oddW) {
        const int c = W2c;
        for (int r = 0; r < 2 * Hc; ++r) {
            float* o = U(r);
            const float* s = src + static_cast<ptrdiff_t>(r) * W2;
            o[c] = A(o[c - 1] - s[c - 1], s[c]);
        }
    }
    if (oddH) {
        const int r = 2 * Hc;
        float* o = U(r);
        const float* op = U(r - 1);
        const float* s = src + static_cast<ptrdiff_t>(r) * W2;
        const float* sp = src + static_cast<ptrdiff_t>(r - 1) * W2;
        for (int c = 0; c < W2; ++c) o[c] = A(op[c] - sp[c], s[c]);
    }
}

// ------------------------------------------------------------------------------ the routines

void model_levels(const float* plane, int W, int H, uint32_t* out, int levels) {
    init_struct(out);
    if (levels < 1 || levels > LEVELS) return;
    float fac = C0_5;
    std::vector<float> src(plane, plane + static_cast<size_t>(W) * H), lo, crop, a, b;
    for (int k = 0; k < levels; ++k) {
        const int Wk = W >> k, Hk = H >> k;
        int bx, by;
        bool small;
        borders(Wk, Hk, 4, &bx, &by, &small);
        if (small) {
            if (k) extrapolate(out, k, &fac);
            continue;
        }
        local_rms_crop(src.data(), Wk, Hk, bx, by, lo, crop, a, b);
        measure(crop, Wk - 2 * bx, Hk - 2 * by, out, k);
        if (k) {                               // 0x995db0: fdiv, fabs, fmin 1.0, fcmp 0.5 / fcsel mi
            const float r = std::fabs(bits(out[5 + k]) / bits(out[4 + k]));
            fac = r < C0_5 ? C0_5 : (std::isnan(r) ? r : (r < C1 ? r : C1));
        }
        src.swap(lo);
    }
}

void band_levels(const float* plane, int W, int H, uint32_t* out, int levels) {
    init_struct(out);
    if (levels < 1 || levels > LEVELS) return;
    std::vector<float> src(plane, plane + static_cast<size_t>(W) * H), lo, crop, a, b;
    for (int k = 0; k < levels; ++k) {
        const int Wk = W >> k, Hk = H >> k;
        int bx, by;
        bool small;
        borders(Wk, Hk, 2, &bx, &by, &small);
        if (Hk < 3 || Wk < 3 || small) {
            if (k) extrapolate(out, k, nullptr);
            continue;
        }
        local_rms_crop(src.data(), Wk, Hk, bx, by, lo, crop, a, b);
        measure(crop, Wk - 2 * bx, Hk - 2 * by, out, k);
        src.swap(lo);
    }
}

}  // namespace nvb
