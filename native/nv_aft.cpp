// nv_aft - see nv_aft.h.  Every function names the Python function it reproduces; the comments give
// the operand order of the Python expression where both operands can be NaN.
#include "nv_aft.h"

#include "nv_band.h"
#include "nv_curve.h"
#include "nv_model.h"
#include "nv_ndimage.h"

#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <thread>

namespace nva {

namespace {

inline float bits(uint32_t u) { float f; std::memcpy(&f, &u, 4); return f; }

// ------------------------------------------------------------------ operand-ordered arithmetic
// fadd / fmul with the operand order kept (LLVM may commute them; the order decides which NaN
// propagates when both operands are NaN: FPProcessNaNs takes the first operand first).
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
inline double MD(double a, double b) {
#if defined(__aarch64__)
    double r;
    __asm__("fmul %d0, %d1, %d2" : "=w"(r) : "w"(a), "w"(b));
    return r;
#else
    return a * b;
#endif
}
// numpy float32 / float64 scalars: a + b and a * b evaluate as (b, a) (numpy 2.1.3, arm64;
// native/MIGRATION.md).  Python floats and numpy arrays keep (a, b) (A / M / AD / MD).
inline float AS(float a, float b) { return A(b, a); }
inline float MS(float a, float b) { return M(b, a); }
inline double MDS(double a, double b) { return MD(b, a); }

// libSystem's pow through a volatile pointer (nv_curve._pow; no constant folding, no pow(x, 2) -> x*x)
double (*volatile g_pow)(double, double) = &::pow;

const float NEG_C = bits(0xbe800000u);    // -0.25f
const float M_ONE = bits(0xbf800000u);    // -1.0f
const float ONE = bits(0x3f800000u);      // 1.0f (also COEF1)
const float HALF = bits(0x3f000000u);     // 0.5f
const float ZERO = 0.0f;
const float TEN4 = bits(0x461c4000u);     // 10000.0f
const float THREE = bits(0x40400000u);    // 3.0f
const float CLIP = bits(0x3ea8f5c3u);     // 0.33f
#if defined(NVA_MUT_Q66)                  // negative controls (verify_aft.py --mutants)
const float Q66 = bits(0x3f2b851fu);      // 0.67f
#else
const float Q66 = bits(0x3f28f5c3u);      // 0.66f
#endif
const float LO_FILL = bits(0x3e4ccccdu);  // 0.2f
const float HI_FILL = bits(0x40400000u);  // 3.0f
const float F16 = bits(0x41800000u);      // 16.0f
const float F1_32 = bits(0x3d000000u);    // 0.03125f
const float F8 = bits(0x41000000u);       // 8.0f
const float S_MULT[3] = {bits(0x40800000u), bits(0x40400000u), bits(0x40800000u)};   // 4, 3, 4
const float S_LVL[3] = {bits(0x40400000u), bits(0x40000000u), bits(0x40400000u)};    // 3, 2, 3

const int FACE[3] = {0x4c, 0x1fa0, 0x3ef4};
const int STATS[3] = {0xbc44, 0xbd10, 0xbddc};
const int CURVE[3] = {0xbf44, 0xc064, 0xc184};
const int NDIR = 8;

// ------------------------------------------------------------------ profile view (Prof)
struct View {
    uint8_t* b;
    float f(int w) const { float v; std::memcpy(&v, b + 4 * w, 4); return v; }
    void setf(int w, float v) { std::memcpy(b + 4 * w, &v, 4); }
    int32_t i(int w) const { int32_t v; std::memcpy(&v, b + 4 * w, 4); return v; }
    void seti(int w, int32_t v) { std::memcpy(b + 4 * w, &v, 4); }
};

// nv_aft._dev: |x| <= |y| ? |y|/|x| : |x|/|y|, then + (-1.0f) (numpy scalar: (-1, r))
float dev(float x, float y) {
    const float ax = std::fabs(x), ay = std::fabs(y);
    const float r = (ax > ay) ? ax / ay : ay / ax;
    return AS(r, M_ONE);
}

float ratio_dev(float a, float b) {
    if (a == ZERO || b == ZERO) return TEN4;
    return dev(a, b);
}

// nv_aft._cl: t = (v <= hi) ? v : hi; (lo <= v) ? t : lo
float cl(float v, float lo, float hi) {
    const float t = (v <= hi) ? v : hi;
    return (lo <= v) ? t : lo;
}

// nv_curve._det (FUN_009d4708) / _cramer (FUN_009d41b0) in Python floats
double det(const std::vector<std::vector<double>>& m, int n) {
    if (n == 1) return m[0][0];
    if (n < 1) return 0.0;
    double d = 0.0, sign = 1.0;
    for (int i = 0; i < n; ++i) {
        std::vector<std::vector<double>> minor;
        for (int k = 0; k < n; ++k) {
            if (k == i) continue;
            minor.emplace_back(m[k].begin() + 1, m[k].begin() + n);
        }
        double t = MD(m[i][0], det(minor, n - 1));
        t = MD(sign, t);
        d = AD(d, t);
        sign = -sign;
    }
    return d;
}

std::vector<double> cramer(const std::vector<std::vector<double>>& a, const std::vector<double>& b, int n) {
    const double d = det(a, n);
    std::vector<double> out;
    for (int k = 0; k < n; ++k) {
        auto mk = a;
        for (int r = 0; r < n; ++r) mk[r][k] = b[r];
        out.push_back(det(mk, n) / d);     // _fdiv: numpy float64 division (a, b)
    }
    return out;
}

// ------------------------------------------------------------------ curve view helpers
inline int cbase(int ch) { return CURVE[ch] >> 2; }
inline int cbyte(int ch, int k) { return CURVE[ch] + 0x20 * k; }

// nv_aft.curve_post (FUN_009b4e08)
void curve_post(const std::vector<float>* bins, View& P, int ch, int mode) {
    const int base0 = cbase(ch);
    const size_t thr = mode == 0 ? 4 : 8;
    bool flags[9];
    float q[9] = {0};
    for (int k = 0; k < 9; ++k) {
        flags[k] = bins[k].size() >= thr;
        if (flags[k]) nvb::quantile(bins[k].data(), bins[k].size(), Q66, 0, 1, &q[k]);
    }
    float ratio_sum = ZERO;
    int ratio_n = 0;
    for (int k = 0; k < 9; ++k) {                     // phase 2: bins 0..8
        if (!flags[k]) continue;
        const int wb = base0 + 8 * k;
        const int o = cbyte(ch, k);
        if (P.i(wb + 6) > 0) {                        // the old count (read before writing)
            ratio_sum = AS(ratio_sum, q[k] / P.f(wb));
            ++ratio_n;
        }
        P.setf(wb, q[k]);
        for (int j = 1; j < 6; ++j) P.setf(wb + j, ONE);
        P.seti(wb + 6, 1);
        P.b[o + 0x1c] = 0;
        P.b[o + 0x1d] = 0;
        P.b[o + 0x1e] = (THREE < q[k]) ? 1 : 0;
    }
    if (ratio_n == 0) return;
    const float avg = ratio_sum / static_cast<float>(ratio_n);
    auto body = [&](int k) {
        const int wb = base0 + 8 * k;
        const int o = cbyte(ch, k);
        if (P.i(wb + 6) > 0 || P.b[o + 0x1c] != 0) {
            P.setf(wb, MS(avg, P.f(wb)));
            for (int j = 1; j < 6; ++j) P.setf(wb + j, ONE);
            P.seti(wb + 6, 0);
            P.b[o + 0x1c] = 1;
            P.b[o + 0x1e] = (THREE < P.f(wb)) ? 1 : 0;
        }
    };
    if (!flags[0]) body(0);
    for (int k = 1; k <= 8; ++k)
        if (!flags[k]) body(k);
}

// nv_aft.curve_fill (FUN_009bdf30)
void curve_fill(float lo, float hi, View& P, int ch, int flag) {
    const int base0 = cbase(ch);
    auto knot_ok = [&](int k) {
        const int wb = base0 + 8 * k, o = cbyte(ch, k);
        return P.i(wb + 6) != 0 && P.b[o + 0x1d] == 0 && P.b[o + 0x1e] == 0 && P.b[o + 0x1c] == 0;
    };
    std::vector<float> xs, ys;
    int first = -1, last = -1;
    for (int k = 0; k < 9; ++k) {
        if (knot_ok(k)) {
            if (first < 0) first = k;
            xs.push_back(static_cast<float>(k));
            ys.push_back(P.f(base0 + 8 * k));
            last = k;
        }
    }
    const int n = static_cast<int>(xs.size());
    if (n == 0) return;
    auto fillable = [&](int k) {
        const int wb = base0 + 8 * k, o = cbyte(ch, k);
        const bool c = P.i(wb + 6) == 0 || P.b[o + 0x1d] != 0 || P.b[o + 0x1e] != 0 || P.b[o + 0x1c] != 0;
        if (flag & 1) return c;
        return c && P.b[o + 0x1c] == 0;
    };
    auto write = [&](int k, float v) {
        const int wb = base0 + 8 * k, o = cbyte(ch, k);
        P.setf(wb, v);
        for (int j = 1; j < 6; ++j) P.setf(wb + j, ONE);
        P.seti(wb + 6, 0);
        P.b[o + 0x1c] = 1;
#if defined(NVA_MUT_GT3)
        P.b[o + 0x1e] = (THREE <= v) ? 1 : 0;
#else
        P.b[o + 0x1e] = (THREE < v) ? 1 : 0;
#endif
    };
    if (n == 1) {
        const float v = cl(ys[0], lo, hi);
        for (int k = 0; k < 9; ++k)
            if (fillable(k)) write(k, v);
        return;
    }
    const int ncoef = n > 2 ? 3 : 2;
    const std::vector<double> c = polyfit_aft(xs.data(), ys.data(), n, ncoef);
    for (int k = first + 1; k < last; ++k)
        if (fillable(k)) write(k, cl(static_cast<float>(polyval_aft(k, c.data(), ncoef)), lo, hi));
    if (first > 0) {
        const int n2 = first < 3 ? 2 : 1;
        const std::vector<double> c2 = polyfit_aft(xs.data(), ys.data(), n2, n2);
        for (int k = 0; k < first; ++k)
            if (fillable(k)) write(k, cl(static_cast<float>(polyval_aft(k, c2.data(), n2)), lo, hi));
    }
    if (last < 8) {
#if defined(NVA_MUT_UPPER)
        const int n2 = last > 4 ? 2 : 1;
#else
        const int n2 = last > 5 ? 2 : 1;
#endif
        const std::vector<double> c3 = polyfit_aft(xs.data() + (n - n2), ys.data() + (n - n2), n2, n2);
        for (int k = last + 1; k < 9; ++k)
            if (fillable(k)) write(k, cl(static_cast<float>(polyval_aft(k, c3.data(), n2)), lo, hi));
    }
}

// nv_aft.mode2_post (FUN_009b5408)
float mode2_post(const std::vector<float>* bins, View& P, int ch) {
    const int base0 = cbase(ch);
    float total = ZERO;
    int n = 0;
    for (int k = 0; k < 9; ++k) {
        const int wb = base0 + 8 * k;
        if (P.f(wb) == ZERO || bins[k].size() < 9) continue;
        float qk = 0.0f;
        nvb::quantile(bins[k].data(), bins[k].size(), Q66, 0, 1, &qk);
        total = AS(total, qk / P.f(wb));
        ++n;
    }
    if (n < 2) return M_ONE;
    return total / static_cast<float>(n);
}

// ------------------------------------------------------------------ one tile of _run_tiles
enum TileOutcome : int { T_CLIP, T_SIMG, T_SIMR, T_SIMB, T_DIFF, T_REJECT, T_ACCEPT, T_ERROR };

struct TileResult {
    int outcome = T_ERROR;
    int bin[3] = {0, 0, 0};
    float val[3] = {0, 0, 0};
};

struct Ctx {
    const float* rgb;
    const float* image2;
    int W, H, mode;
    const float* faces[3];
    const float* stats[3];
    float fw0[3];
    float s_lvl, s_mult;
};

// Stage 1 = clip test, means and bins (the mode-2 skip needs the bins before the models);
// stage 2 = models, similarities, the image2 test and the ratios to append.
void tile_stage1(const Ctx& c, int y, int x, float* R, float* G, float* B, TileResult* r) {
    for (int i = 0; i < 32; ++i)
        for (int j = 0; j < 32; ++j) {
            const float* px = c.rgb + (static_cast<size_t>(y + i) * c.W + (x + j)) * 3;
            R[i * 32 + j] = px[0];
            G[i * 32 + j] = px[1];
            B[i * 32 + j] = px[2];
        }
    const float cR = nvc::is_clip_genuine(R, 1024);
    const float cG = nvc::is_clip_genuine(G, 1024);
    const float cB = nvc::is_clip_genuine(B, 1024);
    if (cR > CLIP || cG > CLIP || cB > CLIP) {
        r->outcome = T_CLIP;
        return;
    }
    r->bin[0] = curve_bin(nvb::mean_exact(R, 1024));
    r->bin[1] = curve_bin(nvb::mean_exact(G, 1024));
    r->bin[2] = curve_bin(nvb::mean_exact(B, 1024));
    r->outcome = T_ACCEPT;     // provisional: stage 2 decides
}

void tile_stage2(const Ctx& c, int y, int x, const float* R, const float* G, const float* B, TileResult* r) {
    std::vector<uint32_t> wR(NWORDS), wG(NWORDS), wB(NWORDS);
    if (nvm::model_full(G, 32, 32, 2, wG.data(), 2) != nvm::OK) { r->outcome = T_ERROR; return; }
    const float* mG = reinterpret_cast<const float*>(wG.data());
    const float simG = similarity(mG, c.faces[1], c.stats[1], c.s_lvl, c.s_mult);
    if (simG < HALF) { r->outcome = T_SIMG; return; }
    if (nvm::model_full(R, 32, 32, 2, wR.data(), 2) != nvm::OK) { r->outcome = T_ERROR; return; }
    const float* mR = reinterpret_cast<const float*>(wR.data());
    const float simR = similarity(mR, c.faces[0], c.stats[0], c.s_lvl, c.s_mult);
    float prod = MS(simG, simR);
    if (prod < HALF) { r->outcome = T_SIMR; return; }
    if (nvm::model_full(B, 32, 32, 2, wB.data(), 2) != nvm::OK) { r->outcome = T_ERROR; return; }
    const float* mB = reinterpret_cast<const float*>(wB.data());
    const float simB = similarity(mB, c.faces[2], c.stats[2], c.s_lvl, c.s_mult);
    prod = MS(MS(simG, simR), simB);
    if (prod < HALF) { r->outcome = T_SIMB; return; }
    if (c.image2) {
        std::vector<uint32_t> dm[3];
        const float* planes[3] = {R, G, B};
        float d[1024];
        for (int ch = 0; ch < 3; ++ch) {
            for (int i = 0; i < 32; ++i)
                for (int j = 0; j < 32; ++j)
                    d[i * 32 + j] = c.image2[(static_cast<size_t>(y + i) * c.W + (x + j)) * 3 + ch] - planes[ch][i * 32 + j];
            dm[ch].resize(NWORDS);
            if (nvm::model_full(d, 32, 32, 0, dm[ch].data(), 2) != nvm::OK) { r->outcome = T_ERROR; return; }
        }
        const float* mm[3] = {mR, mG, mB};
        const int order[6][2] = {{0, 5}, {1, 5}, {2, 5}, {0, 6}, {1, 6}, {2, 6}};
        for (const auto& cw : order) {
            const float a = std::fabs(reinterpret_cast<const float*>(dm[cw[0]].data())[cw[1]]);
            const float b = std::fabs(mm[cw[0]][cw[1]]);
            if (a < b || a > AS(b, b)) { r->outcome = T_DIFF; return; }
        }
    }
    if (prod > HALF) {
        r->val[0] = std::fabs(mR[0]) / c.fw0[0];
        r->val[1] = std::fabs(mG[0]) / c.fw0[1];
        r->val[2] = std::fabs(mB[0]) / c.fw0[2];
        r->outcome = T_ACCEPT;
    } else {
        r->outcome = T_REJECT;
    }
}

}  // namespace

// ------------------------------------------------------------------ public functions
float penalty(float x, float s) {
    if (s == ZERO) {
        const float t = MS(x, NEG_C);
        return t > ZERO ? ONE : ZERO;
    }
    const float a = MS((HALF / s) / NEG_C, x);
    const float b = MS(AS(NEG_C, M_ONE), HALF) / NEG_C;
    const float v = AS(a, b);
    if (v < ZERO) return ZERO;
    if (v > ONE) return ONE;
    return v;
}

float similarity(const float* t, const float* p, const float* stats, float s_lvl, float s_mult) {
    float prod = MS(penalty(std::fabs(t[5] / p[5]), s_lvl), penalty(std::fabs(t[6] / p[6]), s_lvl));
    prod = MS(prod, penalty(ratio_dev(t[6] / t[5], p[6] / p[5]), MS(stats[0], s_mult)));
    for (int i = 0; i < NDIR; ++i) {
        const float a0 = t[240 + i] / p[240 + i];
        const float b0 = t[490] / p[490];
        const float a1 = t[256 + i] / p[256 + i];
        const float b1 = t[491] / p[491];
        prod = MS(prod, penalty(ratio_dev(a0, b0), MS(stats[1 + i], s_mult)));
        prod = MS(prod, penalty(ratio_dev(a1, b1), MS(stats[0x11 + i], s_mult)));
    }
    for (int l = 0; l < 2; ++l) {
        const int base = 16 + 8 * l, sb = 34 + 8 * l;
        for (int m = 0; m < 7; ++m) {
            const float wt = t[base + m], wp = p[base + m];
            if (wt < ZERO) break;
            if (wp < ZERO) break;
            const float tol = stats[sb + m];
#if defined(NVA_MUT_CVTOL)
            if (tol != ZERO) prod = MS(prod, penalty(ratio_dev(wt, wp), MS(MS(tol, s_mult), s_lvl)));
#else
            if (tol != ZERO) prod = MS(prod, penalty(ratio_dev(wt, wp), MS(tol, s_mult)));
#endif
        }
        const float qa = t[l] / t[5 + l];
        const float qb = p[l] / p[5 + l];
        prod = MS(prod, penalty(ratio_dev(qa, qb), MS(stats[0x31 + l], s_mult)));
    }
    return prod;
}

std::vector<int> shuffle_order(int n) {
    std::vector<int> out;
    if (n <= 0) return out;
    int bl = 0;
    while ((n >> bl) != 0) ++bl;
    const uint32_t mask = (bl >= 32) ? 0xffffffffu : ((1u << bl) - 1u);
    out.resize(n);
    for (int i = 0; i < n; ++i) out[i] = i;
    uint32_t s = 0x7b;
    for (int i = 0; i < n; ++i) {
        const int j = static_cast<int>((s & mask) % static_cast<uint32_t>(n));
        std::swap(out[i], out[j]);
        const uint32_t t = s + (s >> 16);
        uint32_t u = t + (t << 8);
        u = u + (s >> 24);
        s = (u << 16) | (s >> 16);
    }
    return out;
}

int curve_bin(float mean) {
    const float v = (mean + F16) * F1_32;
    if (!(v > ZERO)) return 0;
    if (!std::isfinite(v) || v >= F8) return 8;
    return static_cast<int>(v);
}

std::vector<double> polyfit_aft(const float* xs, const float* ys, int npts, int ncoef) {
    std::vector<double> b(ncoef, 0.0);
    std::vector<std::vector<double>> a(ncoef, std::vector<double>(ncoef, 0.0));
    for (int i = 0; i < ncoef; ++i) {
        double s = 0.0;
        if (npts >= 1) {
            if (i == 0) {
                for (int k = 0; k < npts; ++k) {
                    const float x = xs[k];
                    const double term = (x == 0.0f) ? 1.0 : g_pow(static_cast<double>(x), 0.0);
                    s = AD(s, MD(term, static_cast<double>(ys[k])));
                }
            } else {
                for (int k = 0; k < npts; ++k)
                    s = AD(s, MD(g_pow(static_cast<double>(xs[k]), static_cast<double>(i)), static_cast<double>(ys[k])));
            }
        }
        b[i] = s;
        for (int j = 0; j < ncoef; ++j) {
            double s2 = 0.0;
            const int e = i + j;
            for (int k = 0; k < npts; ++k) {
                const float x = xs[k];
                if (e == 0)
                    s2 = AD(s2, (x == 0.0f) ? 1.0 : g_pow(static_cast<double>(x), 0.0));
                else
                    s2 = AD(s2, g_pow(static_cast<double>(x), static_cast<double>(e)));
            }
            a[i][j] = s2;
        }
    }
    return cramer(a, b, ncoef);
}

double polyval_aft(double x, const double* c, int ncoef) {
    double s = 0.0;
    for (int i = 0; i < ncoef; ++i) s = AD(s, MD(g_pow(x, static_cast<double>(i)), c[i]));
    return s;
}

void finish(uint8_t* prof, const Bins& bins, int mode, float out3[3]) {
    View P{prof};
    if (mode < 2) {
        for (int c = 0; c < 3; ++c) curve_post(bins[c], P, c, mode);
        for (int c = 0; c < 3; ++c) curve_fill(LO_FILL, HI_FILL, P, c, 0);
        return;
    }
    for (int c = 0; c < 3; ++c) out3[c] = mode2_post(bins[c], P, c);
}

int run_tiles(const float* rgb, int W, int H, const uint8_t* prof, int mode, const float* image2,
              long cancel_at, int threads, Bins* bins, Diag* diag, const Cancel& cancel) {
    if (W < 48 || H < 48) return TOO_SMALL;
    const int nx = W / 32, ny = H / 32, ntiles = nx * ny;
    std::vector<int> sy(ntiles), sx(ntiles);           // the slots (nv_aft._tiles)
    if (mode == 2) {
        const std::vector<int> order = shuffle_order(ntiles);
        int i = 0;
        for (int ty = 0; ty < ny; ++ty)
            for (int tx = 0; tx < nx; ++tx, ++i) { sy[order[i]] = 32 * ty; sx[order[i]] = 32 * tx; }
    } else {
        int i = 0;
        for (int ty = 0; ty < ny; ++ty)
            for (int tx = 0; tx < nx; ++tx, ++i) { sy[i] = 32 * ty; sx[i] = 32 * tx; }
    }
    // faces / stats: float copies of the profile words (nv_aft reads them before any write)
    std::vector<float> fbuf[3], sbuf[3];
    Ctx c;
    c.rgb = rgb;
    c.image2 = image2;
    c.W = W;
    c.H = H;
    c.mode = mode;
    for (int ch = 0; ch < 3; ++ch) {
        fbuf[ch].resize(MODEL / 4);
        std::memcpy(fbuf[ch].data(), prof + FACE[ch], MODEL);
        sbuf[ch].resize(51);
        std::memcpy(sbuf[ch].data(), prof + STATS[ch], 51 * 4);
        c.faces[ch] = fbuf[ch].data();
        c.stats[ch] = sbuf[ch].data();
        c.fw0[ch] = std::fabs(fbuf[ch][0]);
    }
    c.s_lvl = S_LVL[mode];
    c.s_mult = S_MULT[mode];
    if (diag) diag->ntiles = ntiles;
    for (int ch = 0; ch < 3; ++ch)
        for (int k = 0; k < 9; ++k) (*bins)[ch][k].clear();
    // the cancel query is asked once before every tile: query cancel_at answers true, so the tiles
    // before it still run (and count in diag) and the bins are dropped
    bool cancelled = cancel_at >= 1 && cancel_at <= ntiles;
    int nrun = cancelled ? static_cast<int>(cancel_at - 1) : ntiles;
    const bool ask = cancel_at < 1 && static_cast<bool>(cancel);   // the cancel callback

    std::vector<TileResult> res(ntiles);
    std::vector<char> run2(ntiles, 0);
    std::vector<int> cnt(27, 0);           // accepted ratios per (channel, bin) so far, in slot order
    auto full = [&](const TileResult& r) {   // the mode-2 skip
        return mode == 2 && cnt[r.bin[0]] >= 32 && cnt[9 + r.bin[1]] >= 32 && cnt[18 + r.bin[2]] >= 32;
    };
    const int nt = threads < 1 ? 1 : threads;
    if (nt == 1) {
        float R[1024], G[1024], B[1024];
        for (int s = 0; s < nrun; ++s) {
            if (ask && cancel()) { cancelled = true; nrun = s; break; }
            tile_stage1(c, sy[s], sx[s], R, G, B, &res[s]);
            if (res[s].outcome == T_CLIP || full(res[s])) continue;
            tile_stage2(c, sy[s], sx[s], R, G, B, &res[s]);
            run2[s] = 1;
            if (res[s].outcome == T_ACCEPT)
                for (int ch = 0; ch < 3; ++ch) ++cnt[9 * ch + res[s].bin[ch]];
        }
    } else {
        // every non-clipped tile is modelled in parallel (stage 2 does not depend on the order);
        // the mode-2 skip is then decided in slot order and drops the results it skips
        std::atomic<int> next{0};
        std::mutex mu;                      // the cancel query + the taking of a tile (ask only)
        int taken = 0;
        bool stop = false;
        auto take = [&]() -> int {
            if (!ask) {
                const int s = next.fetch_add(1);
                return s < nrun ? s : -1;
            }
            std::lock_guard<std::mutex> lk(mu);
            if (stop || taken >= nrun) return -1;
            if (cancel()) { stop = true; return -1; }
            return taken++;
        };
        std::vector<std::thread> pool;
        for (int t = 0; t < nt; ++t)
            pool.emplace_back([&] {
                float R[1024], G[1024], B[1024];
                for (int s; (s = take()) >= 0;) {
                    tile_stage1(c, sy[s], sx[s], R, G, B, &res[s]);
                    if (res[s].outcome != T_CLIP) tile_stage2(c, sy[s], sx[s], R, G, B, &res[s]);
                }
            });
        for (auto& th : pool) th.join();
        if (stop) { cancelled = true; nrun = taken; }
        for (int s = 0; s < nrun; ++s) {
            if (res[s].outcome == T_CLIP || full(res[s])) continue;
            run2[s] = 1;
            if (res[s].outcome == T_ACCEPT)
                for (int ch = 0; ch < 3; ++ch) ++cnt[9 * ch + res[s].bin[ch]];
        }
    }
    // the one-thread loop's bookkeeping in slot order
    for (int s = 0; s < nrun; ++s) {
        const TileResult& r = res[s];
        if (r.outcome == T_CLIP) { if (diag) ++diag->clip_skip; continue; }
        if (!run2[s]) continue;                  // mode-2 skip
        switch (r.outcome) {
            case T_ERROR: return MODEL_ERROR;
            case T_SIMG: if (diag) ++diag->simG_skip; break;
            case T_SIMR: if (diag) ++diag->simR_skip; break;
            case T_SIMB: if (diag) ++diag->simB_skip; break;
            case T_DIFF: if (diag) ++diag->diff_skip; break;
            case T_ACCEPT:
                for (int ch = 0; ch < 3; ++ch) (*bins)[ch][r.bin[ch]].push_back(r.val[ch]);
                if (diag) ++diag->accepted;
                break;
            default: break;
        }
    }
    return cancelled ? CANCELLED : OK;
}

int auto_fine_tune(const float* rgb, int W, int H, uint8_t* prof, int mode, const float* image2,
                   long cancel_at, int threads, float out3[3], Diag* diag, Bins* bins_out, const Cancel& cancel) {
    Bins local;
    Bins* bins = bins_out ? bins_out : &local;
    const int st = run_tiles(rgb, W, H, prof, mode, image2, cancel_at, threads, bins, diag, cancel);
    if (st == MODEL_ERROR) return st;
    if (st == OK) finish(prof, *bins, mode, out3);
    prof[0x2d] = 0;
    return st;
}

// ------------------------------------------------------------------ nv_aft_v5.apply_aft (combination)
void combine(const uint8_t* raw, const uint8_t* aft, const double* centers, int n, const float* const curves[3],
             double area_level, const double tot_ratio[3], int band, Combine* out) {
    // _read_model_q99 / _color_weights (Python floats; q ** 2 and the folded constants are libm pow)
    const double MSQ[3][3] = {
        {g_pow(0.299, 2.0), g_pow(0.587, 2.0), g_pow(0.114, 2.0)},
        {g_pow(0.169, 2.0), g_pow(0.331, 2.0), g_pow(0.500, 2.0)},
        {g_pow(0.500, 2.0), g_pow(0.419, 2.0), g_pow(0.081, 2.0)},
    };
    double a_sq[3];
    for (int j = 0; j < 3; ++j) {
        float q;
        std::memcpy(&q, raw + (FACE[j] / 4 + 5 + band) * 4, 4);
        a_sq[j] = g_pow(static_cast<double>(q), 2.0);
    }
    for (int ci = 0; ci < 3; ++ci) {
        // sum() of the three Python floats: 0 + x0 (the int start), then CPython 3.12+'s
        // Neumaier-compensated loop over x1, x2 (builtin_sum_impl), the compensation added at the end
        double x[3];
        for (int j = 0; j < 3; ++j) x[j] = MD(a_sq[j], MSQ[ci][j]);
        double denom = AD(0.0, x[0]), comp = 0.0;
        for (int j = 1; j < 3; ++j) {
            const double t = AD(denom, x[j]);
            if (std::fabs(denom) >= std::fabs(x[j])) comp = AD(comp, AD(denom - t, x[j]));
            else comp = AD(comp, AD(x[j] - t, denom));
            denom = t;
        }
        if (comp != 0.0 && std::isfinite(comp)) denom = AD(denom, comp);
        for (int j = 0; j < 3; ++j)
            out->cw[ci][j] = (denom <= 0) ? 1.0 / 3.0 : MD(a_sq[j], MSQ[ci][j]) / denom;
    }
    // the 9 curve words of each face on np.linspace(0, 256, 9), interpolated at the v5 centres
    double raw_c[9];
    for (int k = 0; k < 9; ++k) raw_c[k] = 32.0 * k;
    for (int j = 0; j < 3; ++j) {
        double vals[9];
        for (int k = 0; k < 9; ++k) {
            float v;
            std::memcpy(&v, aft + CURVE[j] + 0x20 * k, 4);
            vals[k] = static_cast<double>(v);
        }
        nvn::Interp it(raw_c, vals, 9);
        out->face[j].resize(n);
        for (int i = 0; i < n; ++i) out->face[j][i] = it(centers[i], false);
    }
    // f_c = sqrt(maximum(0 + cw0*f0 + cw1*f1 + cw2*f2, 0)); curve * f_c in float64 -> float32
    for (int ci = 0; ci < 3; ++ci) {
        out->curve[ci].resize(n);
        for (int i = 0; i < n; ++i) {
            double ws = 0.0;
            for (int j = 0; j < 3; ++j) ws = AD(ws, MD(out->cw[ci][j], out->face[j][i]));
            const double mx = (ws != ws) ? ws : (ws >= 0.0 ? ws : 0.0);
            const double f = std::sqrt(mx);
            out->curve[ci][i] = static_cast<float>(MD(static_cast<double>(curves[ci][i]), f));
        }
    }
    // prof["sigma"] = interp(area_level, centres, curve) * tot_ratio (numpy float64 scalar product)
    for (int ci = 0; ci < 3; ++ci) {
        std::vector<double> cv(n);
        for (int i = 0; i < n; ++i) cv[i] = static_cast<double>(out->curve[ci][i]);
        nvn::Interp it(centers, cv.data(), n);
        out->sigma[ci] = MDS(it(area_level, false), tot_ratio[ci]);
    }
    for (int j = 0; j < 3; ++j)
        for (int k = 0; k < 9; ++k) {
            const int o = CURVE[j] + 0x20 * k;
            uint32_t mk;
            std::memcpy(&mk, aft + o + 0x1c, 4);
            out->marker[j][k] = mk != 0;
            out->clip[j][k] = aft[o + 0x1d] != 0;
            out->gt3[j][k] = aft[o + 0x1e] != 0;
        }
}

}  // namespace nva
