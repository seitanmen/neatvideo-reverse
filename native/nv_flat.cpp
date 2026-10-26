// nv_flat - C++ port of repro/nv_flat.py (see nv_flat.h).  The operation order and the operand
// order (which decides NaN payloads) follow the Python code line by line:
//   numpy array op array  -> first operand's NaN  (A / M below)
//   numpy float32 scalar a + b, a * b -> second operand's NaN (AS / MS; observed with numpy 2.1.3
//   on arm64, sandbox/omo/wp_native_band/REPORT.md section 1.1), a - b and a / b -> first.

#include "nv_flat.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>
#include <thread>

#include "nv_band.h"

namespace nvf {

namespace {

inline float bits(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }

// fadd / fmul with the operand order kept (LLVM commutes fadd / fmul operands even without
// fast-math; the first operand's NaN propagates on AArch64).
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
// numpy float32 scalar arithmetic: the second operand's NaN.
inline float AS(float a, float b) { return A(b, a); }
inline float MS(float a, float b) { return M(b, a); }

// Constants (bit patterns printed from the Python port).
const float MASKED = bits(0x461c4000u);      // 10000 (nv_flat.MASKED)
const float FLT_MAXF = bits(0x7f7fffffu);
const float CLIP_THR = bits(0x3ea8f5c3u);    // 0.33
const double CLIP_PENALTY = 0.1;             // DAT_0127a7a0 (double, nv_flat.CLIP_PENALTY)
const float EPS_1E6 = bits(0x358637bdu);     // nv_band.EPS_1E6
const float CR_K = bits(0x3f3691fcu);        // 0.71316504
const float CB_K = bits(0x3f108d15u);        // 0.56465274
const float C0_75 = bits(0x3f400000u);
const float C3 = bits(0x40400000u);
const float FL_C_CHROMA = bits(0x3f19999au); // 0.6
const float FL_C_LUMA = bits(0x3e99999au);   // 0.3
const float FL_LIM_CHROMA = bits(0x402ccccc);
const float FL_LIM_LUMA = bits(0x3fe66666u);
const float FL_EPS = bits(0x3727c5acu);      // 1e-5
const float C0_6 = bits(0x3f19999au);
const float C1 = bits(0x3f800000u);
const float C2 = bits(0x40000000u);
const float C4 = bits(0x40800000u);
const float C8 = bits(0x41000000u);
const float C16 = bits(0x41800000u);
const float C0_5 = bits(0x3f000000u);
const float CM1 = bits(0xbf800000u);
const float CM96 = bits(0xc2c00000u);
const float C1_512 = bits(0x3b000000u);      // 0.001953125
const float C1_256 = bits(0x3b800000u);      // 0.00390625
const float C256 = bits(0x43800000u);
const uint32_t LUMA_BITS[3] = {0x3e99096cu, 0x3f162b6bu, 0x3dea7efau};   // 0.2989 0.5866 0.1145

// FindFlatArea size tables (nv_flat.TABLES).
struct Table {
    long long lim;                 // max W*H (-1: no limit)
    int n;
    int sizes[7];
    uint32_t factors[7];
};
const Table TABLES[3] = {
    {0x280000, 6, {320, 192, 128, 96, 64, 48, 0},
     {0x3f800000, 0x3f933333, 0x3faa3d71, 0x40000000, 0x40800000, 0x40c00000, 0}},
    {0xA00000, 7, {512, 320, 192, 128, 96, 64, 48},
     {0x3f800000, 0x3f933333, 0x3fa00000, 0x40000000, 0x40800000, 0x40c00000, 0x41000000}},
    {-1, 7, {512, 320, 192, 128, 96, 64, 48},
     {0x3f933333, 0x3faa3d71, 0x40000000, 0x40800000, 0x40c00000, 0x41000000, 0x41200000}},
};
// DAT_0147dfc8 (nv_flat.LUMA_FLAGS)
const int LUMA_FLAGS[4][3] = {{1, 1, 1}, {1, 0, 0}, {1, 0, 0}, {0, 0, 0}};

// nv_flat._box: radius 0 copies, otherwise nv_lines.box_rows / box_cols.
void box(const float* X, int W, int H, int r, int mode, bool rows, float* out) {
    if (r == 0) {
        std::memcpy(out, X, sizeof(float) * static_cast<size_t>(W) * H);
        return;
    }
    if (rows)
        nvb::box_rows(X, W, H, r, mode, out);
    else
        nvb::box_cols(X, W, H, r, mode, out);
}

// nv_flat._fcvtzs (float32 -> int64: toward zero, saturating, NaN -> 0).
int64_t fcvtzs(float x) {
    const double d = x;
    if (d != d) return 0;
    if (d >= 9223372036854775808.0) return std::numeric_limits<int64_t>::max();
    if (d < -9223372036854775808.0) return std::numeric_limits<int64_t>::min();
    return static_cast<int64_t>(d);
}

// nv_flat._place: (start, length) on one axis.
void place(int64_t c, int s, int size, int side, int* start, int* len) {
    int64_t x0 = c * s - (size >> 1);
    x0 = x0 >= 0 ? x0 : 0;
    const int64_t end = x0 + size;
    const int64_t xs = end > side ? (side >= size ? side - size : x0) : x0;
    const int64_t xe = end > side ? side : end;
    *start = static_cast<int>(xs);
    *len = static_cast<int>(xe - xs);
}

inline float word_f(const uint32_t* st, int k) { return bits(st[k]); }

}  // namespace

// ------------------------------------------------------------------------------- pieces
void reduce_planes(const float* rgb, int W, int H, int stride_c, int s, float* R, float* G, float* B) {
    const int h = H / s, w = W / s;
    const float recip = C1 / static_cast<float>(s * s);
    float* outs[3] = {R, G, B};
    for (int c = 0; c < 3; ++c) {
        float* o = outs[c];
        for (int by = 0; by < h; ++by)
            for (int bx = 0; bx < w; ++bx) {
                float acc = 0.0f;
                for (int r = 0; r < s; ++r) {
                    const float* row = rgb + (static_cast<ptrdiff_t>(by * s + r) * W + bx * s) * stride_c + c;
                    for (int q = 0; q < s; ++q) acc = A(acc, row[static_cast<ptrdiff_t>(q) * stride_c]);
                }
                o[static_cast<ptrdiff_t>(by) * w + bx] = M(recip, acc);
            }
    }
}

void luma_601(const float* R, const float* G, const float* B, size_t n, float* out) {
    const float a = bits(LUMA_BITS[0]), b = bits(LUMA_BITS[1]), c = bits(LUMA_BITS[2]);
    const size_t m = n & ~static_cast<size_t>(3);
    for (size_t i = 0; i < m; ++i) out[i] = std::fma(B[i], c, std::fma(G[i], b, M(a, R[i])));
    for (size_t i = m; i < n; ++i) out[i] = A(A(M(R[i], a), M(G[i], b)), M(B[i], c));
}

void candidates(const float* luma, int W, int H, int rx, int ry, int step, int64_t* xs, int64_t* ys,
                float* means, float* scores, int nbins, int per_bin) {
    const size_t n = static_cast<size_t>(W) * H;
    const int total = nbins * per_bin;
    for (int i = 0; i < total; ++i) {
        xs[i] = -1;
        ys[i] = -1;
        means[i] = CM1;
        scores[i] = CM1;
    }
    std::vector<float> Abuf(n), T(n), sc(n), Mb(n);
    box(luma, W, H, step, 0, true, T.data());
    box(T.data(), W, H, step, 0, false, Abuf.data());                 // A
    for (size_t i = 0; i < n; ++i) T[i] = luma[i] - Abuf[i];          // L - A
    box(T.data(), W, H, 1, 1, true, sc.data());
    box(sc.data(), W, H, 1, 1, false, T.data());                      // t
    for (size_t i = 0; i < n; ++i) T[i] = T[i] * T[i];                // t * t
    box(T.data(), W, H, rx, 1, true, Abuf.data());
    box(Abuf.data(), W, H, ry, 1, false, sc.data());
    for (size_t i = 0; i < n; ++i) sc[i] = std::sqrt(std::fabs(sc[i]));
    box(luma, W, H, rx, 0, true, T.data());
    box(T.data(), W, H, ry, 0, false, Mb.data());                     // M
    if (total < 1) return;
    if (H - 1 - ry < ry || W - 1 - rx < rx) return;
    std::vector<int64_t> counts(std::max(nbins, 1), 0);
    const float width = C256 / static_cast<float>(nbins);
    const float scale = M(static_cast<float>(nbins), C1_256);
    const float inf = std::numeric_limits<float>::infinity();
    for (int i = 0; i < total; ++i) {
        // np.argmin over the reversed region with NaN -> inf: the first strict minimum in the scan
        // order (rows H-1-ry .. ry, columns W-1-rx .. rx); all keys equal -> the first position.
        int bby = H - 1 - ry, bbx = W - 1 - rx;
        float bkey = inf;
        bool first = true;
        for (int by = H - 1 - ry; by >= ry; --by) {
            const float* row = sc.data() + static_cast<ptrdiff_t>(by) * W;
            for (int bx = W - 1 - rx; bx >= rx; --bx) {
                const float v = row[bx];
                const float key = std::isnan(v) ? inf : v;
                if (first) {
                    bkey = key;
                    first = false;
                } else if (key < bkey) {
                    bkey = key;
                    bby = by;
                    bbx = bx;
                }
            }
        }
        const float v = sc[static_cast<ptrdiff_t>(bby) * W + bbx];
        if (!(v < MASKED)) break;
        const float mv = Mb[static_cast<ptrdiff_t>(bby) * W + bbx];
        xs[i] = bbx;
        ys[i] = bby;
        scores[i] = v;
        means[i] = mv;
        const int x1 = bbx + rx < W ? bbx + rx : W - 1;
        const int y0 = std::max(bby - ry, 0);
        const int y1 = bby + ry < H ? bby + ry : H - 1;
        const int x0 = std::max(bbx - rx, 0);
        if (y0 <= y1 && x0 <= x1)
            for (int y = y0; y <= y1; ++y)
                for (int x = x0; x <= x1; ++x) sc[static_cast<ptrdiff_t>(y) * W + x] = MASKED;
        int64_t b = fcvtzs(MS(scale, mv));
        b = b >= 0 ? (b < nbins ? b : nbins - 1) : 0;
        counts[b] += 1;
        const float lo = b == 0 ? -FLT_MAXF : MS(width, static_cast<float>(b));
        const float hi = b == nbins - 1 ? FLT_MAXF : MS(width, static_cast<float>(b + 1));
        if (counts[b] >= per_bin && n >= 1)
            for (size_t k = 0; k < n; ++k)
                if (Mb[k] >= lo && Mb[k] <= hi) sc[k] = MASKED;
    }
}

void transform(float* p0, float* p1, float* p2, size_t n, int mode) {
    if (mode == 1) {
        std::vector<float> Y(n);
        luma_601(p0, p1, p2, n, Y.data());
        for (size_t i = 0; i < n; ++i) {
            const float y = Y[i];
            const float cr = (p0[i] - y) * CR_K;
            const float cb = (p2[i] - y) * CB_K;
            p0[i] = y;
            p1[i] = cr;
            p2[i] = cb;
        }
    } else if (mode == 2) {
        for (size_t i = 0; i < n; ++i) {
            const float y = A(A(p0[i], p1[i]), p2[i]) / C3;
            const float d0 = (p0[i] - y) * C0_75;
            const float d2 = (p2[i] - y) * C0_75;
            p0[i] = y;
            p1[i] = d0;
            p2[i] = d2;
        }
    }
}

float flatness(const uint32_t* st, bool luma) {
    const int k0 = luma ? 1 : 2;
    const float c = luma ? FL_C_LUMA : FL_C_CHROMA;
    const float lim = luma ? FL_LIM_LUMA : FL_LIM_CHROMA;
    float thr[7];
    thr[0] = c;
    for (int j = 1; j < 7; ++j) thr[j] = MS(thr[j - 1], C0_6);
    float tot = 0.0f;
    int n = 0;
    for (int k = k0; k < 5; ++k) {
        const float q99 = word_f(st, 5 + k);
        if (q99 < FL_EPS) break;
        float cv[8];
        for (int j = 0; j < 8; ++j) cv[j] = word_f(st, 15 + 8 * k + j);
        float f = C1;
        if (!(cv[1] < 0.0f)) {
            const float v = std::fabs(cv[1]) / thr[0];
            f = (std::isnan(v) || v < C1) ? C1 : v;           // fmaxnm(v, 1)
            for (int j = 2; j < 8; ++j) {
                if (cv[j] < 0.0f) break;
                const float vj = std::fabs(cv[j]) / thr[j - 1];
                if (vj > f) f = vj;
            }
        }
        const float r = std::fabs(q99 / word_f(st, 10 + k)) / lim;
        if (r > f) f = r;
        tot = AS(tot, f);
        n += 1;
    }
    return n ? tot / static_cast<float>(n) : C1;
}

void level_terms(const uint32_t* const st[3], float* Eo, float* Ro) {
    float m[3][5];
    for (int p = 0; p < 3; ++p)
        for (int k = 0; k < 5; ++k) m[p][k] = word_f(st[p], k);
    float E = 0.0f, R = 0.0f;
    if (m[0][0] < 0.0f || m[1][0] < 0.0f || m[2][0] < 0.0f) {
        *Eo = 0.0f;
        *Ro = 0.0f;
        return;
    }
    float Lp = AS(AS(std::fabs(m[0][0]), std::fabs(m[1][0])), std::fabs(m[2][0]));
    E = AS(Lp, 0.0f);
    int n = 1;
    const float weights[5] = {0.0f, 0.0f, C4, C8, C16};
    for (int k = 1; k < 5; ++k) {
        if (m[0][k] < 0.0f || m[1][k] < 0.0f || m[2][k] < 0.0f) break;
        const float Lk = AS(AS(std::fabs(m[0][k]), std::fabs(m[1][k])), std::fabs(m[2][k]));
        if (k == 1) {
            E = AS(E, AS(Lk, Lk));
            R = AS(Lp / Lk, 0.0f);
        } else {
            E = AS(E, MS(Lk, weights[k]));
            R = AS(R, Lp / Lk);
        }
        Lp = Lk;
        n = k + 1;
    }
    if (n > 1) R = R / static_cast<float>(n - 1);
    E = E / static_cast<float>(n);
    *Eo = E;
    *Ro = R;
}

float score(float E, float R, float nz, const float fl[3], float b, int nclip, int penalty, float factor) {
    float t = MS(MS(fl[0], fl[1]), fl[2]);
    t = MS(t, E / nz);
    t = MS(t, AS(MS(std::fabs(AS(b, CM96)), C1_512), C1));
    t = MS(t, AS(static_cast<float>(penalty), C1));
    t = MS(AS(MS(static_cast<float>(nclip), C4), C1), t);
    t = MS(factor, t);
    float r = C2 - R;
    r = (std::isnan(r) || r < 0.0f) ? 0.0f : r;            // fmaxnm(r, 0)
    return MS(AS(AS(r, r), C1), t);
}

// ------------------------------------------------------------------------------- the routine
namespace {

struct Cand {
    int i, x0, y0, cw, ch, ncl, pen;
    float nz;
};

void copy_plane(const float* rgb, int W, int stride_c, int x0, int y0, int cw, int ch, int c, float* out) {
    for (int y = 0; y < ch; ++y) {
        const float* row = rgb + (static_cast<ptrdiff_t>(y0 + y) * W + x0) * stride_c + c;
        float* o = out + static_cast<ptrdiff_t>(y) * cw;
        for (int x = 0; x < cw; ++x) o[x] = row[static_cast<ptrdiff_t>(x) * stride_c];
    }
}

// transform + band_levels of one candidate square: 3 x STRUCT_WORDS words.
void measure(const float* rgb, int W, int stride_c, const Cand& c, int mode, uint32_t* out) {
    const size_t n = static_cast<size_t>(c.cw) * c.ch;
    std::vector<float> p(3 * n);
    for (int ch = 0; ch < 3; ++ch) copy_plane(rgb, W, stride_c, c.x0, c.y0, c.cw, c.ch, ch, p.data() + ch * n);
    transform(p.data(), p.data() + n, p.data() + 2 * n, n, mode);
    for (int ch = 0; ch < 3; ++ch) nvb::band_levels(p.data() + ch * n, c.cw, c.ch, out + ch * STRUCT_WORDS);
}

}  // namespace

Result find_flat_area_genuine(const float* rgb, int W, int H, int stride_c, int mode,
                              const nvc::CurveInterp* noise, int threads, Diag* diag) {
    if (mode < 0 || mode > 3) return {0, 0, 0, 0, -1};
    int s = 7;
    long long w = 0, h = 0;
    while (true) {
        s += 1;
        w = W / s;
        h = H / s;
        if (!(h >= 5 && w >= 5 && w * h > 0x40000)) break;
    }
    if (diag) {
        diag->s = s;
        diag->rw = static_cast<int>(w);
        diag->rh = static_cast<int>(h);
    }
    if (w == 0 || h == 0) return {0, 0, 0, 0, 1};
    const long long area = static_cast<long long>(W) * H;
    const Table* tb = &TABLES[2];
    for (int t = 0; t < 3; ++t)
        if (TABLES[t].lim < 0 || area <= TABLES[t].lim) {
            tb = &TABLES[t];
            break;
        }
    int i0 = -1;
    for (int i = 0; i < tb->n; ++i)
        if (tb->sizes[i] <= W && tb->sizes[i] <= H) {
            i0 = i;
            break;
        }
    if (diag) diag->i0 = i0;
    if (i0 < 0) return {0, 0, 0, 0, 1};
    nvc::CurveResult cres;
    std::vector<nvc::CurveInterp> own;
    if (!noise) {
        if (diag) diag->curve_run = true;
        if (!nvc::genuine_noise_curve(rgb, W, H, stride_c, &cres, threads)) return {0, 0, 0, 0, 5};
        if (diag) {
            diag->curve_ok = true;
            std::memcpy(diag->curve_xs, cres.xs, sizeof cres.xs);
            std::memcpy(diag->curve_ys, cres.ys, sizeof cres.ys);
        }
        own.emplace_back(cres.xs, cres.ys, nvc::NBINS);
        noise = &own[0];
    }
    const size_t rn = static_cast<size_t>(w) * h;
    std::vector<float> R(rn), G(rn), B(rn), luma(rn);
    reduce_planes(rgb, W, H, stride_c, s, R.data(), G.data(), B.data());
    luma_601(R.data(), G.data(), B.data(), rn, luma.data());
    if (diag) {
        diag->R = R;
        diag->G = G;
        diag->B = B;
        diag->luma = luma;
    }
    const int* flags = LUMA_FLAGS[mode];
    float best_sc = MASKED;
    bool have_best = false;
    int best[4] = {0, 0, 0, 0};
    bool flat_ok = false, clipped = false;
    int step = 16 / s;
    step = step > 1 ? step : 1;
    int64_t xs[NCAND], ys[NCAND];
    float means[NCAND], scores[NCAND];
    std::vector<float> plane;
    std::vector<uint32_t> structs;
    for (int si = i0; si < tb->n; ++si) {
        const int S = tb->sizes[si];
        int r = static_cast<int>(M(A(static_cast<float>(S) / static_cast<float>(s), CM1), C0_5));
        r = r > 1 ? r : 1;
        candidates(luma.data(), static_cast<int>(w), static_cast<int>(h), r, r, step, xs, ys, means, scores);
        SizeDiag* sd = nullptr;
        if (diag) {
            diag->sizes.emplace_back();
            sd = &diag->sizes.back();
            sd->size = S;
            sd->r = r;
            sd->step = step;
            std::memcpy(sd->xs, xs, sizeof xs);
            std::memcpy(sd->ys, ys, sizeof ys);
            std::memcpy(sd->means, means, sizeof means);
            std::memcpy(sd->scores, scores, sizeof scores);
        }
        std::vector<Cand> cand;
        for (int i = 0; i < NCAND; ++i) {
            if (scores[i] < 0.0f) break;
            Cand c;
            c.i = i;
            place(xs[i], s, S, W, &c.x0, &c.cw);
            place(ys[i], s, S, H, &c.y0, &c.ch);
            const size_t n = static_cast<size_t>(c.cw) * c.ch;
            plane.resize(n);
            float clip[3];
            for (int ch = 0; ch < 3; ++ch) {
                copy_plane(rgb, W, stride_c, c.x0, c.y0, c.cw, c.ch, ch, plane.data());
                clip[ch] = nvc::is_clip_genuine(plane.data(), n);
            }
            if (sd) sd->placed.push_back({c.x0, c.y0, c.cw, c.ch, {clip[0], clip[1], clip[2]}});
            c.ncl = int(clip[0] > CLIP_THR) + int(clip[1] > CLIP_THR) + int(clip[2] > CLIP_THR);
            if (c.ncl > 1) continue;
            c.nz = static_cast<float>((*noise)(static_cast<double>(means[i])));
            if (c.nz < EPS_1E6) continue;
            c.pen = int(static_cast<double>(clip[0]) > CLIP_PENALTY || static_cast<double>(clip[1]) > CLIP_PENALTY ||
                        static_cast<double>(clip[2]) > CLIP_PENALTY);
            cand.push_back(c);
        }
        if (cand.empty()) continue;
        const size_t per = 3 * static_cast<size_t>(STRUCT_WORDS);
        structs.assign(per * cand.size(), 0u);
        const int nt = std::max(1, std::min<int>(threads, static_cast<int>(cand.size())));
        if (nt == 1) {
            for (size_t j = 0; j < cand.size(); ++j) measure(rgb, W, stride_c, cand[j], mode, structs.data() + per * j);
        } else {
            std::atomic<size_t> next(0);
            std::vector<std::thread> pool;
            for (int t = 0; t < nt; ++t)
                pool.emplace_back([&]() {
                    for (size_t j; (j = next.fetch_add(1)) < cand.size();)
                        measure(rgb, W, stride_c, cand[j], mode, structs.data() + per * j);
                });
            for (auto& th : pool) th.join();
        }
        const float factor = bits(tb->factors[si]);
        for (size_t j = 0; j < cand.size(); ++j) {
            const Cand& c = cand[j];
            const uint32_t* st[3] = {structs.data() + per * j, structs.data() + per * j + STRUCT_WORDS,
                                     structs.data() + per * j + 2 * STRUCT_WORDS};
            float fl[3];
            for (int p = 0; p < 3; ++p) fl[p] = flatness(st[p], flags[p] != 0);
            float E, Rr;
            level_terms(st, &E, &Rr);
            const float sc = score(E, Rr, c.nz, fl, means[c.i], c.ncl, c.pen, factor);
            if (diag) {
                Trace tr;
                tr.size = S;
                tr.i = c.i;
                tr.x = c.x0;
                tr.y = c.y0;
                tr.w = c.cw;
                tr.h = c.ch;
                tr.ncl = c.ncl;
                tr.pen = c.pen;
                tr.mean = means[c.i];
                tr.nz = c.nz;
                for (int p = 0; p < 3; ++p) tr.fl[p] = fl[p];
                tr.E = E;
                tr.R = Rr;
                tr.score = sc;
                if (diag->keep_structs) tr.structs.assign(st[0], st[0] + per);
                diag->trace.push_back(std::move(tr));
            }
            if (sc < best_sc) {
                best_sc = sc;
                have_best = true;
                best[0] = c.x0;
                best[1] = c.y0;
                best[2] = c.cw;
                best[3] = c.ch;
                flat_ok = fl[0] < C2 && fl[1] < C2 && fl[2] < C2;
                clipped = c.ncl != 0;
            }
        }
    }
    if (!have_best) return {0, 0, 0, 0, 1};
    int x0 = best[0], y0 = best[1], bw = best[2], bh = best[3];
    if (bw > 0x200) {
        x0 += (bw - 0x200) >> 1;
        bw = 0x200;
    }
    if (bh > 0x200) {
        y0 += (bh - 0x200) >> 1;
        bh = 0x200;
    }
    int status;
    if (!flat_ok)
        status = 3;
    else if (clipped)
        status = 4;
    else
        status = (bh >= 0x80 && bw >= 0x80) ? 0 : 2;
    return {x0, y0, bw, bh, status};
}

}  // namespace nvf
