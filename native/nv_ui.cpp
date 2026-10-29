// nv_ui - see nv_ui.h.
#include "nv_ui.h"

#include <cfloat>
#include <cmath>
#include <cstring>
#include <vector>

#include "nv_band.h"
#include "nv_curve.h"
#include "nv_flat.h"
#include "nv_ndimage.h"
#include "nv_prof5.h"
#include "nv_profile.h"
#include "nv_quality.h"

namespace nvu {

using nvn::A;
using nvn::D;
using nvn::M;
using nvn::S;
using nvn::SQRT;

namespace {

// nvfilter._KRB (an unknown matrix: BT.709, as _KRB.get(matrix, _KRB["bt709"]))
void krb(int matrix, double* kr, double* kb) {
    switch (matrix) {
        case BT601: *kr = 0.299; *kb = 0.114; break;
        case BT2020: *kr = 0.2627; *kb = 0.0593; break;
        default: *kr = 0.2126; *kb = 0.0722; break;
    }
}

// numpy float32 np.clip(x, 0, 255): max then min, NaN propagates
inline float clip255(float x) {
    if (x != x) return x;
    if (x < 0.0f) x = 0.0f;
    if (x > 255.0f) x = 255.0f;
    return x;
}

// np.nan_to_num(x, nan=0, posinf=255, neginf=0)
inline float finite255(float x) {
    if (x != x) return 0.0f;
    if (std::isinf(x)) return x > 0 ? 255.0f : 0.0f;
    return x;
}

using Vec = std::vector<float>;

// R, G, B planes of an H x W x 3 image (np.ascontiguousarray(f[..., c]))
void split3(const float* rgb, size_t n, Vec* R, Vec* G, Vec* B) {
    R->resize(n);
    G->resize(n);
    B->resize(n);
    for (size_t i = 0; i < n; ++i) {
        (*R)[i] = rgb[3 * i];
        (*G)[i] = rgb[3 * i + 1];
        (*B)[i] = rgb[3 * i + 2];
    }
}

// display.ycc: nv_flat.transform(R, G, B, 1) -> (Y, Cr, Cb), in place on copies of the planes
void ycc(const float* rgb, size_t n, Vec* Y, Vec* Cr, Vec* Cb) {
    split3(rgb, n, Y, Cr, Cb);
    nvf::transform(Y->data(), Cr->data(), Cb->data(), n, 1);
}

// ------------------------------------------------------------------ pane mappings (display.map_a / b / c)
const float RANGE_LO[6][3] = {{-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f},
                              {-0.5f, -0.5f, -0.5f}, {31.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 15.5f}};
const float RANGE_HI[6][3] = {{255.5f, 255.5f, 255.5f}, {255.5f, 200.5f, 200.5f}, {200.5f, 255.5f, 200.5f},
                              {200.5f, 200.5f, 255.5f}, {255.5f, 215.5f, 225.5f}, {235.5f, 225.5f, 255.5f}};

struct Map {
    float gain[3], off[3];
};

Map map_a(int kind, int b, double mn_d, double mx_d) {
    const float* lo_o = RANGE_LO[kind];
    const float* hi_o = RANGE_HI[kind];
    const float mn = static_cast<float>(mn_d > -2048.0 ? mn_d : -2048.0);   // F32(max(mn, -2048.0))
    const float mx = static_cast<float>(mx_d < 2048.0 ? mx_d : 2048.0);     // F32(min(mx, 2048.0))
    float f, lo, hi;
    if (b > 50) {
        f = static_cast<float>((b - 50) / 50.0);
        lo = M(f, mn);
        hi = M(f, A(mn, M(S(mx, mn), 0.33f)));
    } else {
        f = static_cast<float>((50 - b) / 50.0);
        lo = M(f, A(mx, M(S(mx, mn), -0.33f)));
        hi = M(mx, f);
    }
    hi = A(hi, M(S(1.0f, f), 255.0f));
    Map r;
    if (hi == lo) {
        for (int c = 0; c < 3; ++c) {
            r.gain[c] = 1.0f;
            r.off[c] = lo_o[c];
        }
        return r;
    }
    lo = S(lo, 0.5f);
    const float span = S(A(hi, 0.5f), lo);
    for (int c = 0; c < 3; ++c) {
        const float m = M(A(hi_o[c], lo_o[c]), 0.5f), h = M(S(hi_o[c], lo_o[c]), 0.5f);
        float bot, top;
        if (b > 50) {
            bot = S(A(m, M(f, 32.0f)), h);
            top = A(m, h);
        } else {
            bot = S(m, h);
            top = A(M(S(1.0f, f), A(m, h)), M(f, S(A(m, h), 32.0f)));
        }
        r.gain[c] = D(S(top, bot), span);
        r.off[c] = S(bot, M(lo, r.gain[c]));
    }
    return r;
}

Map map_b(int kind, int b, float L, float H) {
    const float* lo_o = RANGE_LO[kind];
    const float* hi_o = RANGE_HI[kind];
    const float fb = static_cast<float>(b);
    const float s = b <= 50 ? A(M(fb, 0.013333334f), 0.33333334f) : S(M(fb, 0.04f), 1.0f);
    if (L == H) {
        L = A(L, 1.0f);
        H = S(H, 1.0f);
    }
    const float span = S(H, L);
    Map r;
    for (int c = 0; c < 3; ++c) {
        const float m = M(A(hi_o[c], lo_o[c]), 0.5f), h = M(M(S(hi_o[c], lo_o[c]), s), 0.5f);
        const float bot = S(m, h);
        r.gain[c] = D(S(A(m, h), bot), span);
        r.off[c] = S(bot, M(r.gain[c], L));
    }
    return r;
}

Map map_c(int kind, int b) {
    const float* lo_o = RANGE_LO[kind];
    const float* hi_o = RANGE_HI[kind];
    const float s = static_cast<float>(b / 50.0);
    Map r;
    for (int c = 0; c < 3; ++c) {
        const float m = M(A(hi_o[c], lo_o[c]), 0.5f), h = M(M(S(hi_o[c], lo_o[c]), s), 0.5f);
        const float bot = S(m, h);
        r.gain[c] = D(S(A(m, h), bot), 129.0f);
        r.off[c] = A(bot, M(r.gain[c], 64.5f));
    }
    return r;
}

// display.apply: np.clip(np.nan_to_num(np.rint(v * gain + off), nan=0), 0, 255).astype(uint8)
inline uint8_t apply1(float v, float gain, float off) {
    float o = std::nearbyint(A(M(v, gain), off));
    if (o != o) return 0;
    if (o < 0.0f) o = 0.0f;
    if (o > 255.0f) o = 255.0f;
    return static_cast<uint8_t>(o);
}

void apply_plane(const float* v, size_t n, const Map& m, uint8_t* out) {
    for (size_t i = 0; i < n; ++i)
        for (int c = 0; c < 3; ++c) out[3 * i + c] = apply1(v[i], m.gain[c], m.off[c]);
}

// ------------------------------------------------------------------ frequency images (display.decompose)
struct Img {
    int H = 0, W = 0;
    Vec d;
    Img() = default;
    Img(int h, int w) : H(h), W(w), d(static_cast<size_t>(h) * w) {}
};

Img box3(const Img& x) {
    Img a(x.H, x.W), b(x.H, x.W);
    nvb::box_rows(x.d.data(), x.W, x.H, 1, 0, a.d.data());
    nvb::box_cols(a.d.data(), x.W, x.H, 1, 0, b.d.data());
    return b;
}

Img down2(const Img& x) {
    Img o(x.H >> 1, x.W >> 1);
    nvb::down2(x.d.data(), x.W, x.H, o.d.data());
    return o;
}

// _up_to: up2 through the level sizes (from the next larger level down to level 0)
Img up_to(Img x, const std::vector<std::pair<int, int>>& chain) {
    for (const auto& hw : chain) {
        const int H2 = hw.first, W2 = hw.second;
        Img o(H2, W2);
        nvc::up2_ext(x.d.data(), x.W, x.H, W2 - 2 * x.W, H2 - 2 * x.H, o.d.data());
        x = std::move(o);
    }
    return x;
}

// display.to16: rint(16 nan_to_num(x)) saturated to int16
void to16(const Img& x, int16_t* out) {
    for (size_t i = 0; i < x.d.size(); ++i) {
        float v = x.d[i];
        if (v != v) v = 0.0f;
        else if (std::isinf(v)) v = v > 0 ? FLT_MAX : -FLT_MAX;
        float r = std::nearbyint(M(v, 16.0f));
        if (r < -32768.0f) r = -32768.0f;
        if (r > 32767.0f) r = 32767.0f;
        out[i] = static_cast<int16_t>(r);
    }
}

// decompose(x, enhanced, emphasis): Enhanced -> one int16 plane in out16; Frequencies -> High, Mid, Low
void decompose(const Img& x, bool enhanced, bool emphasis, int16_t* out16) {
    const size_t N = static_cast<size_t>(x.H) * x.W;
    std::vector<std::pair<int, int>> shapes{{x.H, x.W}};
    int nfreq = 0;
    Img Sacc;
    bool have_s = false;
    float w = 1.0f;
    Img xk = x;
    for (int k = 0; k < 5; ++k) {
        if (xk.H < 4 || xk.W < 4) break;
        Img lo = down2(box3(xk));
        Img band(xk.H, xk.W);
        nvb::band_from(lo.d.data(), lo.W, lo.H, xk.d.data(), xk.W, xk.H, band.d.data());
        std::vector<std::pair<int, int>> chain;
        if (k)
            for (int i = static_cast<int>(shapes.size()) - 2; i >= 0; --i) chain.push_back(shapes[i]);
        if (!enhanced) {
            if (k < 3) to16(up_to(band, chain), out16 + N * nfreq++);
        } else {
            if (emphasis) {
                Img sq(band.H, band.W);
                for (size_t i = 0; i < sq.d.size(); ++i) sq.d[i] = M(band.d[i], band.d[i]);
                Img r = box3(box3(sq));
                for (float& v : r.d) v = SQRT(std::fabs(SQRT(std::fabs(v))));
                Img e(band.H, band.W);
                nvc::ratio_eps_ext(band.d.data(), r.d.data(), 0.01f, band.d.size(), e.d.data());
                band = std::move(e);
            }
            Img up = up_to(band, chain);
            if (!have_s) {
                Sacc = Img(up.H, up.W);
                for (size_t i = 0; i < up.d.size(); ++i) Sacc.d[i] = M(up.d[i], w);
                have_s = true;
            } else {
                for (size_t i = 0; i < up.d.size(); ++i) Sacc.d[i] = A(Sacc.d[i], M(w, up.d[i]));
            }
            if (emphasis) w = M(w, 1.25f);
        }
        shapes.push_back({lo.H, lo.W});
        xk = std::move(lo);
    }
    if (!enhanced) {
        for (; nfreq < 3; ++nfreq) std::memset(out16 + N * nfreq, 0, sizeof(int16_t) * N);
        return;
    }
    if (!have_s) {
        std::memset(out16, 0, sizeof(int16_t) * N);
        return;
    }
    to16(Sacc, out16);
}

// display.hist_levels: (value at qa, value at qb, normal-model qa, normal-model qb)
void hist_levels(const int16_t* v, size_t n, float qa, float qb, float out[4]) {
    std::vector<int64_t> cnt(65536, 0);
    for (size_t i = 0; i < n; ++i) cnt[static_cast<size_t>(static_cast<int>(v[i]) + 32768)] += 1;
    // int(F32(n * F32(q))): the Python int times the float32 scalar in float32
    const int64_t ta = static_cast<int64_t>(M(static_cast<float>(n), qa));
    const int64_t tb = static_cast<int64_t>(M(static_cast<float>(n), qb));
    // np.searchsorted(cum, t, side="right"): the first index with cum > t (65536 if none)
    int64_t ia = 65536, ib = 65536, cum = 0;
    for (int i = 0; i < 65536; ++i) {
        cum += cnt[i];
        if (ia == 65536 && cum > ta) ia = i;
        if (ib == 65536 && cum > tb) ib = i;
    }
    // s = dot(cnt, vals) and var = dot(cnt, (vals - mi)^2) in float64: sums of integers, exact (as numpy's
    // whenever its sum is exact: below 2^53)
    __int128 si = 0;
    for (int i = 0; i < 65536; ++i) si += static_cast<__int128>(cnt[i]) * (i - 32768);
    const double s = static_cast<double>(si);
    const int64_t mi = n ? static_cast<int64_t>(s / static_cast<double>(n)) : 0;   // int(s / n)
    __int128 vi = 0;
    for (int i = 0; i < 65536; ++i) {
        const __int128 d = static_cast<__int128>(i - 32768) - mi;
        vi += static_cast<__int128>(cnt[i]) * d * d;
    }
    const double var = static_cast<double>(vi) / static_cast<double>(n > 1 ? n - 1 : 1);
    const float mu = M(D(static_cast<float>(s), static_cast<float>(n)), 0.0625f);
    const float sd = static_cast<float>(std::sqrt(var) * 0.0625);
    out[0] = static_cast<float>(((ia < 65535 ? ia : 65535) - 32768) * 0.0625);
    out[1] = static_cast<float>(((ib < 65535 ? ib : 65535) - 32768) * 0.0625);
    out[2] = A(mu, M(sd, nvb::probit(qa)));
    out[3] = A(mu, M(sd, nvb::probit(qb)));
}

// display.freq_levels
void freq_levels(const int16_t* planes, size_t n, bool enhanced, float out6[6]) {
    const float m = enhanced ? 1.0f : 0.25f;
    for (int i = 0; i < 3; ++i) {
        float h4[4];
        hist_levels(planes + n * i, n, 0.3f, 0.7f, h4);
        float q30 = h4[0], q70 = h4[1];
        const float g30 = h4[2], g70 = h4[3];
        float d = std::fabs(S(q70, q30));
        if (M(d, 20.0f) < std::fabs(S(g70, g30))) {
            q30 = M(A(q30, g30), 0.5f);
            q70 = M(A(q70, g70), 0.5f);
            d = std::fabs(S(q70, q30));
        }
        if (m > d) d = m;                                  // Python max(d, m): d unless m > d
        const float c = M(A(q70, q30), 0.5f);
        float half = M(M(d, 0.5f), 8.0f);
        if (enhanced && i > 0) half = A(half, half);
        out6[2 * i] = S(c, half);
        out6[2 * i + 1] = A(c, half);
    }
    if (enhanced) {
        float h4[4];
        hist_levels(planes, n, 0.01f, 0.99f, h4);
        const float p1 = h4[0], p99 = h4[1];
        if (std::fabs(S(p99, p1)) > 0.01f) {
            if (p1 > out6[0]) out6[0] = p1;                // max(out[0][0], p1)
            if (p99 < out6[1]) out6[1] = p99;              // min(out[0][1], p99)
        }
    }
}

}  // namespace

void yuv_to_rgb(const float* Y, const float* Cb, const float* Cr, int H, int W, int Hc, int Wc, int matrix,
                bool full_range, int clip, float* out) {
    double kr, kb;
    krb(matrix, &kr, &kb);
    // Python scalars as numpy's weak scalars: the double expression, then float32 for the array operation
    const float c2r = static_cast<float>(2 * (1 - kr));
    const float c2b = static_cast<float>(2 * (1 - kb));
    const float fkr = static_cast<float>(kr), fkb = static_cast<float>(kb);
    const float k3 = static_cast<float>(1 - kr - kb);
    const bool same = Hc == H && Wc == W;
    for (int y = 0; y < H; ++y) {
        // _fit(c.repeat(2, 0).repeat(2, 1), (H, W)): crop to H x W, edge-pad beyond 2 Hc x 2 Wc
        const int cy = same ? y : (y < 2 * Hc ? y : 2 * Hc - 1) / 2;
        for (int x = 0; x < W; ++x) {
            const int cx = same ? x : (x < 2 * Wc ? x : 2 * Wc - 1) / 2;
            const float Yv = Y[static_cast<size_t>(y) * W + x];
            const float Bv = Cb[static_cast<size_t>(cy) * Wc + cx];
            const float Rv = Cr[static_cast<size_t>(cy) * Wc + cx];
            float yy, cb, cr;
            if (full_range) {
                yy = Yv;
                cb = S(Bv, 128.0f);
                cr = S(Rv, 128.0f);
            } else {
                yy = D(M(S(Yv, 16.0f), 255.0f), 219.0f);
                cb = D(M(S(Bv, 128.0f), 255.0f), 224.0f);
                cr = D(M(S(Rv, 128.0f), 255.0f), 224.0f);
            }
            const float r = A(yy, M(c2r, cr));
            const float b = A(yy, M(c2b, cb));
            const float g = D(S(S(yy, M(fkr, r)), M(fkb, b)), k3);
            float* o = out + (static_cast<size_t>(y) * W + x) * 3;
            o[0] = r;
            o[1] = g;
            o[2] = b;
            if (clip == CLIP_FINITE)
                for (int c = 0; c < 3; ++c) o[c] = finite255(o[c]);
            if (clip != CLIP_NONE)
                for (int c = 0; c < 3; ++c) o[c] = clip255(o[c]);
        }
    }
}

void minmax(const float* v, size_t n, double* mn, double* mx) {
    float lo = n ? v[0] : 0.0f, hi = lo;
    for (size_t i = 1; i < n; ++i) {
        const float x = v[i];
        if (lo == lo) lo = (x != x || x < lo) ? x : lo;
        if (hi == hi) hi = (x != x || x > hi) ? x : hi;
    }
    *mn = lo;
    *mx = hi;
}

void render_simple(int name, const float* rgb, int H, int W, int b, double mn, double mx, uint8_t* out) {
    const size_t n = static_cast<size_t>(H) * W;
    if (name == SP_RGB) {
        const Map m = map_a(RK_Y, b, mn, mx);
        for (size_t i = 0; i < n; ++i)
            for (int c = 0; c < 3; ++c) out[3 * i + c] = apply1(rgb[3 * i + c], m.gain[c], m.off[c]);
        return;
    }
    if (name == SP_R || name == SP_G || name == SP_B) {
        const int c = name - SP_R;
        Vec p(n);
        for (size_t i = 0; i < n; ++i) p[i] = rgb[3 * i + c];
        apply_plane(p.data(), n, map_a(RK_R + c, b, mn, mx), out);
        return;
    }
    Vec Y, Cr, Cb;
    ycc(rgb, n, &Y, &Cr, &Cb);
    if (name == SP_Y)
        apply_plane(Y.data(), n, map_a(RK_Y, b, mn, mx), out);
    else if (name == SP_CR)
        apply_plane(Cr.data(), n, map_c(RK_CR, b), out);
    else
        apply_plane(Cb.data(), n, map_c(RK_CB, b), out);
}

void freq_images(const float* rgb, int H, int W, int kind, bool emphasis, int16_t* out16, float* levels6) {
    const size_t n = static_cast<size_t>(H) * W;
    Vec Y, Cr, Cb;
    ycc(rgb, n, &Y, &Cr, &Cb);
    Img p[3];
    for (int i = 0; i < 3; ++i) p[i] = Img(H, W);
    p[0].d = std::move(Y);
    p[1].d = std::move(Cr);
    p[2].d = std::move(Cb);
    if (kind == FK_ENHANCED) {
        for (int i = 0; i < 3; ++i) decompose(p[i], true, emphasis, out16 + n * i);
        freq_levels(out16, n, true, levels6);
        return;
    }
    decompose(p[kind - FK_Y], false, false, out16);
    freq_levels(out16, n, false, levels6);
}

void render_freq(int range_kind, const int16_t* p16, int H, int W, float L, float Hh, int b, uint8_t* out) {
    const size_t n = static_cast<size_t>(H) * W;
    Vec v(n);
    for (size_t i = 0; i < n; ++i) v[i] = M(static_cast<float>(p16[i]), 0.0625f);
    apply_plane(v.data(), n, map_b(range_kind, b, L, Hh), out);
}

double noise_level(const float* rgb, int h, int w) {
    const size_t n = static_cast<size_t>(h) * w;
    Vec P[3];
    split3(rgb, n, &P[0], &P[1], &P[2]);
    float means[3][5];
    std::vector<uint32_t> st(nvb::STRUCT_WORDS);
    for (int c = 0; c < 3; ++c) {
        nvb::model_levels(P[c].data(), w, h, st.data());
        for (int k = 0; k < 5; ++k) means[c][k] = nvn::bits(st[k]);
    }
    float tot = 0.0f;
    for (int k = 0; k < 5; ++k)
        for (int c = 0; c < 3; ++c) tot = A(tot, M(means[c][k], means[c][k]));
    return SQRT(tot);
}

void not_uniform(const float* rgb, int h, int w, int bad[3], int clip[3]) {
    const size_t n = static_cast<size_t>(h) * w;
    Vec R, G, B;
    split3(rgb, n, &R, &G, &B);
    Vec T[3] = {R, G, B};
    nvf::transform(T[0].data(), T[1].data(), T[2].data(), n, 1);
    std::vector<uint32_t> st(nvb::STRUCT_WORDS);
    for (int i = 0; i < 3; ++i) {
        nvb::band_levels(T[i].data(), w, h, st.data());
        bad[i] = !(nvf::flatness(st.data(), i == 0) < 2.0f);
    }
    const Vec* rgbp[3] = {&R, &G, &B};
    for (int c = 0; c < 3; ++c) clip[c] = nvc::is_clip_genuine(rgbp[c]->data(), n) > 0.33f;
}

bool quality_numbers(const float* rgb, int H, int W, const int area[4], const double factors[27],
                     const double tune[4], bool aft, bool reset, const uint8_t* flags, int* q_pct, int* c_pct) {
    const int y0 = area[0], x0 = area[1], h = area[2], w = area[3];
    // rgb[y0:y0 + h, x0:x0 + w] with numpy's slice clamping
    auto clampi = [](int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; };
    const int ya = clampi(y0, 0, H), yb = clampi(y0 + h, 0, H), xa = clampi(x0, 0, W), xb = clampi(x0 + w, 0, W);
    const int ch = yb > ya ? yb - ya : 0, cw = xb > xa ? xb - xa : 0;
    const size_t n = static_cast<size_t>(ch) * cw;
    Vec P[3];
    for (int c = 0; c < 3; ++c) P[c].resize(n);
    for (int y = 0; y < ch; ++y)
        for (int x = 0; x < cw; ++x)
            for (int c = 0; c < 3; ++c)
                P[c][static_cast<size_t>(y) * cw + x] = rgb[(static_cast<size_t>(ya + y) * W + xa + x) * 3 + c];
    Vec T[3] = {P[0], P[1], P[2]};
    nvf::transform(T[0].data(), T[1].data(), T[2].data(), n, 1);
    std::vector<uint8_t> buf(nvq::PROF_SIZE, 0);
    auto put_u32 = [&](size_t off, uint32_t v) { std::memcpy(buf.data() + off, &v, 4); };
    auto put_f32 = [&](size_t off, float v) { std::memcpy(buf.data() + off, &v, 4); };
    put_u32(0x48, 1);                                          // colour mode 1
    std::vector<uint32_t> st(nvb::STRUCT_WORDS);
    for (int i = 0; i < 3; ++i) {                               // faces 3..5, words 0..54
        nvb::model_levels(T[i].data(), cw, ch, st.data());
        std::memcpy(buf.data() + 0x4c + (3 + i) * nvq::MODEL, st.data(), 55 * 4);
    }
    buf[0xbf34] = 1;
    put_u32(0xbf38, static_cast<uint32_t>(nvp::size_class(w < h ? w : h)));
    bool clips[3];
    for (int c = 0; c < 3; ++c) {
        clips[c] = nvc::is_clip_genuine(P[c].data(), n) > 0.33f;
        buf[0xbf3c + c] = clips[c];
    }
    put_u32(0xc2c0, static_cast<uint32_t>(w));
    put_u32(0xc2c4, static_cast<uint32_t>(h));
    int filled[3];
    for (int c = 0; c < 3; ++c) filled[c] = nvp::curve_bin(nvb::mean_exact(P[c].data(), n));
    // the stored flags: after Reset all clear; after AFT the run's flags (or every marker for older profiles)
    std::vector<uint8_t> stored;
    const bool have_stored = reset || aft;
    if (reset) {
        stored.assign(81, 0);
    } else if (aft) {
        if (flags) {
            stored.assign(flags, flags + 81);
        } else {
            stored.assign(81, 0);
            for (int i = 0; i < 27; ++i) stored[i] = 1;
        }
    }
    buf[0xc2a4] = reset ? 0 : 1;
    // tk with the repro channel order ("Y", "Cr", "Cb") of _curve_blob (tune[] holds all, Y, Cb, Cr)
    const double tk[3] = {tune[0] * tune[1], tune[0] * tune[3], tune[0] * tune[2]};
    for (int c = 0; c < 3; ++c) {
        for (int k = 0; k < 9; ++k) {
            const size_t e = 0xbf44 + 0x120 * c + 0x20 * k;
            const float lvl = M(static_cast<float>(factors[9 * c + k]), static_cast<float>(tk[c]));
            for (int j = 0; j < 6; ++j) put_f32(e + 4 * j, 1.0f);
            put_f32(e, lvl);
            if (have_stored) {
                if (stored[9 * c + k]) buf[e + 0x1c] = 1;
                if (stored[27 + 9 * c + k]) buf[e + 0x1d] = 1;
                if (stored[54 + 9 * c + k]) buf[e + 0x1e] = 1;
            } else if (lvl > 3.0f) {
                buf[e + 0x1e] = 1;
            }
        }
        if (!aft && !reset) {
            const size_t e = 0xbf44 + 0x120 * c + 0x20 * filled[c];
            put_u32(e + 0x18, 1);
            buf[e + 0x1d] = clips[c];
        }
    }
    float q, g;
    if (!nvq::quality_numbers(buf.data(), buf.size(), &q, &g)) return false;
    *q_pct = static_cast<int>(std::floor(static_cast<double>(q) * 100.0 + 0.5));
    *c_pct = static_cast<int>(std::floor(static_cast<double>(g) * 100.0 + 0.5));
    return true;
}

// ------------------------------------------------------------------ Noise Level Check (N3b)
void temporal_mask(const float* rv, size_t n_px, int n, double a, float* out) {
    // backend.temporal_mask: rv float32; a / n Python floats (weak scalars -> float32 at each array operation)
    if (a <= 0 || n <= 1) {
        for (size_t i = 0; i < n_px; ++i) out[i] = 0.0f;
        return;
    }
    const double x = (1.0 - a) + a / n;
    const double rv_min = std::pow(x, 2.0) + a * a * 0.64 * (n - 1) / static_cast<double>(n * n);
    const float den = static_cast<float>(1.0 / rv_min - 1.0);        // (1.0 / rv_min - 1.0): one Python float
    for (size_t i = 0; i < n_px; ++i) {
        float v = D(S(D(1.0f, rv[i]), 1.0f), den);
        if (v == v) {                                                // np.clip(.., 0.0, 1.0): NaN kept
            if (v < 0.0f) v = 0.0f;
            if (v > 1.0f) v = 1.0f;
        }
        out[i] = v;
    }
}

void nlc_image(const float* rgb, const float* m, int H, int W, float* out) {
    // i = f.mean(axis=2, dtype=F32): ((r + g) + b) / 3 (float32 division); w = m[..., None];
    // clip((1 - w) * (i * SEPIA) + w * (i * BLUE + OFF), 0, 255)
    static const float SEPIA[3] = {1.0f, static_cast<float>(0.9), static_cast<float>(0.75)};
    static const float BLUE[3] = {static_cast<float>(0.3), static_cast<float>(0.45), static_cast<float>(0.5)};
    static const float OFF[3] = {0.0f, 0.0f, 126.5f};
    const size_t n = static_cast<size_t>(H) * W;
    for (size_t p = 0; p < n; ++p) {
        const float* f = rgb + 3 * p;
        const float i = D(A(A(f[0], f[1]), f[2]), 3.0f);
        const float w = m[p];
        const float omw = S(1.0f, w);
        for (int c = 0; c < 3; ++c) {
            const float sep = M(omw, M(i, SEPIA[c]));
            const float blu = M(w, A(M(i, BLUE[c]), OFF[c]));
            out[3 * p + c] = clip255(A(sep, blu));
        }
    }
}

void np_interp(const double* xp, const double* fp, int n, const double* x, int m, double* out) {
    nvn::Interp it(xp, fp, n);
    for (int i = 0; i < m; ++i) out[i] = it(x[i], true);
}

}  // namespace nvu
