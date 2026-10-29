// nv_prof5 - see nv_prof5.h.  Line-by-line port of the Python functions named there; every numpy call is
// written out with its reduction / rounding order (comments name the Python expression).
#include "nv_prof5.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <thread>

#include "nv_flat.h"
#include "nv_lstsq.h"
#include "nv_ndimage.h"
#include "nv_ui.h"

namespace nvp5 {

using nvn::A;
using nvn::D;
using nvn::M;
using nvn::Plane;
using nvn::S;

namespace {

const int CURVE_BINS = 8;
const float F_CLIP = static_cast<float>(0.33);       // CLIP_FRAC as a float32 weak scalar
const float F_DEAD = static_cast<float>(1e-6);
const float F_014826 = static_cast<float>(1.4826);
const float F_002 = static_cast<float>(0.02);
const double CHROMA_GAIN[3] = {1.0, 3.0, 4.0};       // Y unchanged, Cb 3.0, Cr 4.0

// ---------------------------------------------------------------------------------- numpy helpers
// numpy FLOAT_pairwise_sum (as native/nv_temporal.cpp)
float pairwise(const float* a, long n, long stride) {
    if (n < 8) {
        if (n <= 0) return nvn::bits(0x80000000u);
        float res = a[0];
        for (long i = 1; i < n; ++i) res = A(res, a[i * stride]);
        return res;
    } else if (n <= 128) {
        float r[8];
        for (int j = 0; j < 8; ++j) r[j] = a[j * stride];
        long i;
        for (i = 8; i < n - (n % 8); i += 8)
            for (int j = 0; j < 8; ++j) r[j] = A(r[j], a[(i + j) * stride]);
        auto T = [](float x, float y) { return A(y, x); };
        float res = T(T(T(r[0], r[1]), T(r[2], r[3])), T(T(r[4], r[5]), T(r[6], r[7])));
        for (; i < n; ++i) res = A(res, a[i * stride]);
        return res;
    } else {
        long n2 = n / 2;
        n2 -= n2 % 8;
        return A(pairwise(a, n2, stride), pairwise(a + n2 * stride, n - n2, stride));
    }
}

// numpy add.reduce of a whole float32 array (axis=None): the reduction iterator buffers 8192 values at a
// time (the flattened C-order data, also for non-contiguous views): out = +0; out = pw(chunk) + out
float sum_all_f32(const float* a, size_t n) {
    float s = 0.0f;
    for (size_t k = 0; k < n; k += 8192) s = A(pairwise(a + k, static_cast<long>(std::min<size_t>(8192, n - k)), 1), s);
    return s;
}

// np.mean of a whole float32 array: float32(sum / n) (the division in double)
float mean_all_f32(const float* a, size_t n) {
    return static_cast<float>(static_cast<double>(sum_all_f32(a, n)) / static_cast<double>(n));
}

// x.mean(-1) of a reduction along the last axis (n contiguous values per output): (+0) + pw, / n
float mean_axis_f32(const float* a, size_t n) {
    const float s = A(pairwise(a, static_cast<long>(n), 1), 0.0f);
    return static_cast<float>(static_cast<double>(s) / static_cast<double>(n));
}

// np.mean of a 2-D float32 view (h rows of w values, row stride `stride`)
float mean_view_f32(const float* a, int h, int w, long stride) {
    std::vector<float> flat(static_cast<size_t>(h) * w);
    for (int y = 0; y < h; ++y) std::memcpy(&flat[static_cast<size_t>(y) * w], a + y * stride, sizeof(float) * w);
    return mean_all_f32(flat.data(), flat.size());
}

// np.median of n float32 values (v is reordered): mean of the middle value(s)
float median_f32(float* v, size_t n) {
    if (n % 2 == 1) {
        std::nth_element(v, v + n / 2, v + n);
        return v[n / 2];
    }
    const size_t k = n / 2;
    std::nth_element(v, v + k, v + n);
    const float hi = v[k];
    const float lo = *std::max_element(v, v + k);
    const float s = A(A(lo, hi), 0.0f);
    return static_cast<float>(static_cast<double>(s) / 2.0);
}

// np.median of n doubles
double median_f64(double* v, size_t n) {
    if (n % 2 == 1) {
        std::nth_element(v, v + n / 2, v + n);
        return v[n / 2];
    }
    const size_t k = n / 2;
    std::nth_element(v, v + k, v + n);
    const double hi = v[k];
    const double lo = *std::max_element(v, v + k);
    return ((lo + hi) + 0.0) / 2.0;
}

// np.percentile(v, q) ('linear') of n >= 1 float32 values (v is reordered): float32 result
float percentile_f32(float* v, size_t n, int q) {
    const float q32 = static_cast<float>(q) / 100.0f;
    const float vi = static_cast<float>(n - 1) * q32;
    long prev, next;
    if (vi >= static_cast<float>(n - 1)) {
        prev = next = -1;
    } else {
        const float fl = std::floor(vi);
        prev = static_cast<long>(fl);
        next = static_cast<long>(fl + 1.0f);
    }
    const float gamma = static_cast<float>(static_cast<double>(vi) - static_cast<double>(prev));
    auto at = [&](long i) {
        const size_t k = i < 0 ? n - 1 : static_cast<size_t>(i);
        std::nth_element(v, v + k, v + n);
        return v[k];
    };
    const float a = at(prev);
    const float b = at(next);
    const float diff = S(b, a);
    float r = A(a, M(diff, gamma));
    if (gamma >= 0.5f) r = S(b, M(diff, S(1.0f, gamma)));
    return r;
}

// nvfilter._blocks(a, blk) of an H x W plane: (nby, nbx, blk * blk) row-major blocks
std::vector<float> blocks(const float* a, int H, int W, int blk, int* nby, int* nbx) {
    *nby = H / blk;
    *nbx = W / blk;
    const size_t bb = static_cast<size_t>(blk) * blk;
    std::vector<float> out(static_cast<size_t>(*nby) * *nbx * bb);
    for (int by = 0; by < *nby; ++by)
        for (int bx = 0; bx < *nbx; ++bx) {
            float* o = &out[(static_cast<size_t>(by) * *nbx + bx) * bb];
            for (int r = 0; r < blk; ++r)
                std::memcpy(o + static_cast<size_t>(r) * blk, a + static_cast<size_t>(by * blk + r) * W + bx * blk,
                            sizeof(float) * blk);
        }
    return out;
}

Plane to_plane(const float* p, int H, int W) {
    Plane pl(H, W);
    std::memcpy(pl.d.data(), p, sizeof(float) * pl.size());
    return pl;
}

Plane crop(const Plane& a, int y0, int y1, int x0, int x1) {
    // numpy slice a[y0:y1, x0:x1] (bounds clipped as numpy does)
    y0 = std::max(0, std::min(y0, a.H));
    y1 = std::max(y0, std::min(y1, a.H));
    x0 = std::max(0, std::min(x0, a.W));
    x1 = std::max(x0, std::min(x1, a.W));
    Plane o(y1 - y0, x1 - x0);
    for (int y = 0; y < o.H; ++y) std::memcpy(o.row(y), a.row(y0 + y) + x0, sizeof(float) * o.W);
    return o;
}

// ---------------------------------------------------------------------------------- nvfilter pieces
// _dead(Y): uniform_filter(b1 * b1, 5) < 1e-6, b1 = Y - _b3(_b3(Y, 1, 0), 1, 1)
std::vector<uint8_t> dead(const Plane& Y) {
    const Plane s = nvn::b3(nvn::b3(Y, 1, 0), 1, 1);
    Plane sq(Y.H, Y.W);
    for (size_t i = 0; i < Y.size(); ++i) {
        const float b1 = S(Y.d[i], s.d[i]);
        sq.d[i] = M(b1, b1);
    }
    const Plane u = nvn::uniform(sq, 5, nvn::REFLECT);
    std::vector<uint8_t> m(Y.size());
    for (size_t i = 0; i < Y.size(); ++i) m[i] = u.d[i] < F_DEAD;
    return m;
}

// _clip_masks(planes, full_range, matrix): 3 masks (R, G, B) on the luma grid
void clip_masks(const Frame& f, bool full_range, int matrix, std::vector<uint8_t> masks[3]) {
    const size_t n = static_cast<size_t>(f.H) * f.W;
    std::vector<float> rgb(n * 3);
    nvu::yuv_to_rgb(f.p[0], f.p[1], f.p[2], f.H, f.W, f.Hc, f.Wc, matrix, full_range, nvu::CLIP_NONE, rgb.data());
    for (int c = 0; c < 3; ++c) {
        masks[c].resize(n);
        for (size_t i = 0; i < n; ++i) {
            const float v = rgb[i * 3 + c];
            masks[c][i] = (v <= 0.5f) | (v >= 254.5f);
        }
    }
}

// scipy.ndimage.gaussian_filter(a, 2.0) (truncate 4: radius 8; symmetric branch of NI_Correlate1D)
const uint64_t G2_BITS[17] = {
    0x3f118aad19e4159bull, 0x3f3c98b8c5d0dda5ull, 0x3f6227362b5fc92dull, 0x3f81f30504e20207ull,
    0x3f9ba4d4125ffd2aull, 0x3fb0941b71ceef37ull, 0x3fbef9093fc46e5aull, 0x3fc68856f9ab1983ull,
    0x3fc98862a07ae7b4ull, 0x3fc68856f9ab1983ull, 0x3fbef9093fc46e5aull, 0x3fb0941b71ceef37ull,
    0x3f9ba4d4125ffd2aull, 0x3f81f30504e20207ull, 0x3f6227362b5fc92dull, 0x3f3c98b8c5d0dda5ull,
    0x3f118aad19e4159bull};

void gauss2_axis(const Plane& in, Plane& out, int axis) {
    const int R = 8;
    double fwv[17];
    for (int i = 0; i < 17; ++i) fwv[i] = nvn::dbits(G2_BITS[i]);
    const double* fw = fwv + R;
    const int H = in.H, W = in.W;
    if (axis == 0) {
        std::vector<long> ix(H + 2 * R);
        for (long i = -R; i < H + R; ++i) ix[i + R] = nvn::ext_index(i, H, nvn::REFLECT);
        for (int y = 0; y < H; ++y) {
            float* o = out.row(y);
            const float* r0 = in.row(static_cast<int>(ix[y + R]));
            for (int x = 0; x < W; ++x) {
                double acc = nvn::MD(static_cast<double>(r0[x]), fw[0]);
                for (int jj = -R; jj < 0; ++jj) {
                    const double s = nvn::AD(static_cast<double>(in.row(static_cast<int>(ix[y + R + jj]))[x]),
                                             static_cast<double>(in.row(static_cast<int>(ix[y + R - jj]))[x]));
                    acc = nvn::FMAD(s, fw[jj], acc);
                }
                o[x] = static_cast<float>(acc);
            }
        }
    } else {
        std::vector<double> buf(W + 2 * R);
        std::vector<long> ix(W + 2 * R);
        for (long i = -R; i < W + R; ++i) ix[i + R] = nvn::ext_index(i, W, nvn::REFLECT);
        for (int y = 0; y < H; ++y) {
            const float* r = in.row(y);
            for (int i = 0; i < W + 2 * R; ++i) buf[i] = static_cast<double>(r[ix[i]]);
            const double* il = buf.data() + R;
            float* o = out.row(y);
            for (int x = 0; x < W; ++x) {
                double acc = nvn::MD(il[x], fw[0]);
                for (int jj = -R; jj < 0; ++jj) acc = nvn::FMAD(nvn::AD(il[x + jj], il[x - jj]), fw[jj], acc);
                o[x] = static_cast<float>(acc);
            }
        }
    }
}

Plane gauss2(const Plane& a) {
    Plane t(a.H, a.W), o(a.H, a.W);
    if (a.size() == 0) return o;
    gauss2_axis(a, t, 0);
    gauss2_axis(t, o, 1);
    return o;
}

// np.gradient(f) (spacing 1, edge_order 1) of a float32 plane: (d/dy, d/dx)
void gradient(const Plane& f, Plane& gy, Plane& gx) {
    const int H = f.H, W = f.W;
    gy = Plane(H, W);
    gx = Plane(H, W);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            float vy, vx;
            if (y == 0) vy = D(S(f.at(1, x), f.at(0, x)), 1.0f);
            else if (y == H - 1) vy = D(S(f.at(H - 1, x), f.at(H - 2, x)), 1.0f);
            else vy = D(S(f.at(y + 1, x), f.at(y - 1, x)), 2.0f);
            if (x == 0) vx = D(S(f.at(y, 1), f.at(y, 0)), 1.0f);
            else if (x == W - 1) vx = D(S(f.at(y, W - 1), f.at(y, W - 2)), 1.0f);
            else vx = D(S(f.at(y, x + 1), f.at(y, x - 1)), 2.0f);
            gy.at(y, x) = vy;
            gx.at(y, x) = vx;
        }
}

// Python's built-in min(a, b) / max(a, b)
inline double pmin(double a, double b) { return b < a ? b : a; }
inline double pmax(double a, double b) { return b > a ? b : a; }

// _mad_sigma(x) of a float32 plane
double mad_sigma(const Plane& x) {
    std::vector<float> v(x.d);
    const float med = median_f32(v.data(), v.size());
    for (size_t i = 0; i < v.size(); ++i) v[i] = std::fabs(S(x.d[i], med));
    const float m = median_f32(v.data(), v.size());
    return static_cast<double>(M(F_014826, m));
}

// _robust_sigma(band, blk)
double robust_sigma(const Plane& band, int blk) {
    const int n = std::min(band.H, band.W) / blk;
    if (n < 2) return mad_sigma(band);
    int nby, nbx;
    std::vector<float> r = blocks(band.d.data(), band.H, band.W, blk, &nby, &nbx);
    const size_t bb = static_cast<size_t>(blk) * blk, nb = static_cast<size_t>(nby) * nbx;
    std::vector<float> v(nb), tmp(bb);
    for (size_t k = 0; k < nb; ++k) {
        const float* blkv = &r[k * bb];
        std::memcpy(tmp.data(), blkv, sizeof(float) * bb);
        const float med = median_f32(tmp.data(), bb);
        for (size_t i = 0; i < bb; ++i) tmp[i] = std::fabs(S(blkv[i], med));
        v[k] = M(F_014826, median_f32(tmp.data(), bb));
    }
    std::vector<float> live;
    for (float x : v)
        if (x > F_002) live.push_back(x);
    std::vector<float>& src = live.empty() ? v : live;
    return static_cast<double>(percentile_f32(src.data(), src.size(), 25));
}

// _trim_rms(x) of float32 values
double trim_rms(const std::vector<float>& x) {
    if (x.empty()) return 0.0;
    std::vector<float> a(x.size());
    for (size_t i = 0; i < x.size(); ++i) a[i] = std::fabs(x[i]);
    std::vector<float> t(a);
    const float p = percentile_f32(t.data(), t.size(), 99);
    std::vector<float> sq;
    sq.reserve(a.size());
    for (float v : a)
        if (v <= p) sq.push_back(M(v, v));
    const float m = mean_all_f32(sq.data(), sq.size());     // (a * a).mean() of the kept values
    const float r = M(static_cast<float>(1.04), std::sqrt(m));
    return static_cast<double>(r);
}

// _noise_shape(sig, tot, decay): (ratios, total)
void noise_shape(const std::vector<double>& sig, double tot, double decay, std::vector<double>* r, double* total) {
    const std::vector<double> wn = wn_band_std(static_cast<int>(sig.size()));
    r->clear();
    auto rms = [](const std::vector<double>& v) {
        // float(np.sqrt(np.sum(np.square(v)))): pairwise over < 8 doubles from the first value, + 0
        double s = v.empty() ? -0.0 : v[0] * v[0];
        for (size_t i = 1; i < v.size(); ++i) s = s + v[i] * v[i];
        s = s + 0.0;
        return std::sqrt(s);
    };
    if (sig[0] < 0.02) {
        for (double w : wn) r->push_back(w / wn[0]);
        *total = rms(*r);
        return;
    }
    r->push_back(1.0);
    for (size_t i = 1; i < sig.size(); ++i) r->push_back(pmin(sig[i] / sig[0], decay * r->back()));
    *total = pmin(pmax(tot / sig[0], 1.0), 2.0 * rms(*r));
}

// area_band_sigma(plane, area, levels, sub): band sigmas and the total
void area_band_sigma(const Plane& plane, const int area[4], int levels, int sub, std::vector<double>* sig,
                     double* tot) {
    const int y0 = area[0], x0 = area[1], h = area[2], w = area[3];
    const Plane a = crop(plane, y0 / sub, (y0 + h) / sub, x0 / sub, (x0 + w) / sub);
    const std::vector<Plane> b = nvn::atrous(a, levels);
    const std::vector<double> wn = wn_band_std(levels);
    const int n = std::min(a.H, a.W);
    sig->clear();
    for (int j = 0; j < levels; ++j) {
        const double q = static_cast<double>(n) / static_cast<double>(1 << j);
        if (j == 0 || q * q >= 16.0)
            sig->push_back(robust_sigma(b[j], std::max(16, 8 * (1 << j))));
        else
            sig->push_back((sig->back() * wn[j]) / wn[j - 1]);
    }
    Plane d(a.H, a.W);
    for (size_t i = 0; i < a.size(); ++i) d.d[i] = S(a.d[i], b[levels].d[i]);
    *tot = robust_sigma(d, 32);
}

// _fill_curve(vals, valid) -> false = None
bool fill_curve(const std::vector<double>& vals, const std::vector<uint8_t>& valid, std::vector<double>* out) {
    const int n = static_cast<int>(vals.size());
    std::vector<double> x(n), v(vals);
    for (int i = 0; i < n; ++i) x[i] = static_cast<double>(i);
    std::vector<int> idx;
    for (int i = 0; i < n; ++i)
        if (valid[i]) idx.push_back(i);
    if (idx.empty()) return false;
    if (idx.size() == 1) {
        const double c = v[idx[0]];
        for (double& t : v) t = c;
    } else {
        auto fit = [&](const std::vector<int>& id, int deg, double* p) {
            std::vector<double> xs, ys;
            for (int i : id) {
                xs.push_back(x[i]);
                ys.push_back(v[i]);
            }
            return nvl5::polyfit(xs.data(), ys.data(), static_cast<int>(id.size()), deg, p);
        };
        double poly[3], pl[2], ph[2];
        const int deg = idx.size() >= 3 ? 2 : 1;
        if (!fit(idx, deg, poly)) return false;
        for (int i = idx.front() + 1; i < idx.back(); ++i)
            if (!valid[i]) {
                const double e = nvl5::polyval(poly, deg + 1, x[i]);
                v[i] = e > 0 ? e : v[i - 1];
            }
        const std::vector<int> lo(idx.begin(), idx.begin() + std::min<size_t>(3, idx.size()));
        const std::vector<int> hi(idx.end() - std::min<size_t>(3, idx.size()), idx.end());
        if (!fit(lo, 1, pl) || !fit(hi, 1, ph)) return false;
        for (int i = idx.front() - 1; i >= 0; --i) {
            const double e = nvl5::polyval(pl, 2, x[i]);
            v[i] = e > 0 ? e : v[i + 1];
        }
        for (int i = idx.back() + 1; i < n; ++i) {
            const double e = nvl5::polyval(ph, 2, x[i]);
            v[i] = e > 0 ? e : v[i - 1];
        }
    }
    out->assign(v.begin(), v.end());
    for (int i = 1; i < n - 1; ++i) (*out)[i] = ((v[i - 1] + 2 * v[i]) + v[i + 1]) / 4;
    return true;
}

// noise_curve_spatial(planes, ratio2, full_range, matrix=matrix): curves (has[c] = not None)
void noise_curve_spatial(const Frame& f, const double ratio2[3], bool full_range, int matrix,
                         std::vector<double> curves[3], bool has[3]) {
    const int blk = 16;
    const int H = f.H, W = f.W;
    const int nby = H / blk, nbx = W / blk;
    for (int c = 0; c < 3; ++c) has[c] = false;
    if (nby == 0 || nbx == 0) return;
    const Plane Y = to_plane(f.p[0], H, W);
    const size_t nb = static_cast<size_t>(nby) * nbx, bb = static_cast<size_t>(blk) * blk;
    int by_, bx_;
    std::vector<float> yb = blocks(Y.d.data(), H, W, blk, &by_, &bx_);
    std::vector<int> bins(nb);
    for (size_t k = 0; k < nb; ++k) {
        const float m = mean_axis_f32(&yb[k * bb], bb);
        const float t = D(M(m, 8.0f), 256.0f);
        long b = static_cast<long>(t);
        bins[k] = static_cast<int>(std::min<long>(std::max<long>(b, 0), CURVE_BINS - 1));
    }
    std::vector<uint8_t> masks[3];
    clip_masks(f, full_range, matrix, masks);
    std::vector<int> nclip(nb, 0);
    for (int c = 0; c < 3; ++c) {
        std::vector<float> mf(masks[c].begin(), masks[c].end());
        std::vector<float> mb = blocks(mf.data(), H, W, blk, &by_, &bx_);
        for (size_t k = 0; k < nb; ++k) nclip[k] += mean_axis_f32(&mb[k * bb], bb) > F_CLIP;
    }
    std::vector<uint8_t> live(nb);
    {
        const std::vector<uint8_t> dm = dead(Y);
        std::vector<float> df(dm.begin(), dm.end());
        std::vector<float> db = blocks(df.data(), H, W, blk, &by_, &bx_);
        for (size_t k = 0; k < nb; ++k) live[k] = mean_axis_f32(&db[k * bb], bb) < static_cast<float>(0.1);
    }
    std::vector<uint8_t> flat;
    for (int c = 0; c < 3; ++c) {
        const int s = (c == 0 || (f.Hc == H && f.Wc == W)) ? 1 : 2;
        const int ph = c == 0 ? H : f.Hc, pw = c == 0 ? W : f.Wc;
        const Plane p = to_plane(f.p[c], ph, pw);
        const std::vector<Plane> b = nvn::atrous(p, 2);
        auto med_map = [&](const Plane& band) {
            int cy, cx;
            const int bs = blk / s;
            std::vector<float> bl = blocks(band.d.data(), band.H, band.W, bs, &cy, &cx);
            const size_t bbs = static_cast<size_t>(bs) * bs;
            Plane m(cy, cx);
            std::vector<float> tmp(bbs);
            for (size_t k = 0; k < static_cast<size_t>(cy) * cx; ++k) {
                for (size_t i = 0; i < bbs; ++i) tmp[i] = std::fabs(bl[k * bbs + i]);
                m.d[k] = M(F_014826, median_f32(tmp.data(), bbs));
            }
            return nvn::fit(m, nby, nbx);
        };
        const Plane m1 = med_map(b[0]), m2 = med_map(b[1]);
        const float k2 = static_cast<float>(1.5 * ratio2[c]);
        std::vector<uint8_t> ok(nb);
        for (size_t k = 0; k < nb; ++k)
            ok[k] = (nclip[k] < 2) && live[k] && (m1.d[k] > F_002) && (m2.d[k] < M(k2, m1.d[k]));
        if (flat.empty())
            flat = ok;
        else
            for (size_t k = 0; k < nb; ++k) ok[k] = ok[k] && flat[k];
        std::vector<double> vals(CURVE_BINS);
        std::vector<uint8_t> valid(CURVE_BINS);
        for (int i = 0; i < CURVE_BINS; ++i) {
            std::vector<float> sel;
            for (size_t k = 0; k < nb; ++k)
                if (ok[k] && bins[k] == i) sel.push_back(m1.d[k]);
            vals[i] = sel.empty() ? 0.0 : 1.07 * static_cast<double>(percentile_f32(sel.data(), sel.size(), 25));
            const long cnt = static_cast<long>(sel.size());
            valid[i] = static_cast<double>(cnt * blk * blk) >= static_cast<double>(H) * W / 1024.0;
        }
        has[c] = fill_curve(vals, valid, &curves[c]);
    }
}

// noise_curve_temporal(p0, p1, levels): curves, ratios, totals (per plane, ok[c] = in the dicts), still
void noise_curve_temporal(const Frame& f0, const Frame& f1, int levels, std::vector<double> curves[3],
                          bool has_curve[3], std::vector<double> ratios[3], double tots[3], bool has_ratio[3],
                          double* still_frac) {
    const int H = f0.H, W = f0.W;
    const Plane Y0 = to_plane(f0.p[0], H, W), Y1 = to_plane(f1.p[0], H, W);
    const Plane u1 = nvn::uniform(Y1, 15, nvn::REFLECT), u0 = nvn::uniform(Y0, 15, nvn::REFLECT);
    const std::vector<uint8_t> d0 = dead(Y0);
    const size_t n = Y0.size();
    std::vector<uint8_t> still(n);
    size_t nstill = 0;
    for (size_t i = 0; i < n; ++i) {
        still[i] = (std::fabs(S(u1.d[i], u0.d[i])) < 0.5f) && !d0[i];
        nstill += still[i];
    }
    Plane lum(H, W);
    for (size_t i = 0; i < n; ++i) lum.d[i] = D(A(Y0.d[i], Y1.d[i]), 2.0f);
    const double sqrt2 = std::sqrt(2.0);
    for (int c = 0; c < 3; ++c) {
        has_curve[c] = has_ratio[c] = false;
        const int ph = c == 0 ? H : f0.Hc, pw = c == 0 ? W : f0.Wc;
        const size_t pn = static_cast<size_t>(ph) * pw;
        std::vector<double> d(pn);
        Plane df(ph, pw);
        for (size_t i = 0; i < pn; ++i) {
            d[i] = static_cast<double>(S(f1.p[c][i], f0.p[c][i])) / sqrt2;
            df.d[i] = static_cast<float>(d[i]);
        }
        const bool same = ph == H && pw == W;
        std::vector<uint8_t> m(pn);
        if (same) {
            m = still;
        } else {
            Plane sf(H, W);
            for (size_t i = 0; i < n; ++i) sf.d[i] = still[i] ? 1.0f : 0.0f;
            const Plane g = nvn::to_grid(sf, ph, pw);
            for (size_t i = 0; i < pn; ++i) m[i] = g.d[i] > static_cast<float>(0.99);
        }
        const Plane lu = nvn::to_grid(lum, ph, pw);
        const int L = same ? levels : levels - 1;
        const std::vector<Plane> b = nvn::atrous(df, L);
        size_t msum = 0;
        for (uint8_t v : m) msum += v;
        if (msum < 64) continue;            // curves[name] = None; no ratios / totals for this plane
        std::vector<double> sig;
        std::vector<float> sel;
        for (int j = 0; j < L; ++j) {
            sel.clear();
            for (size_t i = 0; i < pn; ++i)
                if (m[i]) sel.push_back(b[j].d[i]);
            sig.push_back(trim_rms(sel));
        }
        sel.clear();
        for (size_t i = 0; i < pn; ++i)
            if (m[i]) sel.push_back(static_cast<float>(d[i] - static_cast<double>(b[L].d[i])));
        noise_shape(sig, trim_rms(sel), 0.9, &ratios[c], &tots[c]);
        has_ratio[c] = true;
        std::vector<int> bins(pn);
        for (size_t i = 0; i < pn; ++i) {
            const float t = D(M(lu.d[i], 8.0f), 256.0f);
            bins[i] = static_cast<int>(std::min<long>(std::max<long>(static_cast<long>(t), 0), CURVE_BINS - 1));
        }
        std::vector<double> vals(CURVE_BINS);
        std::vector<uint8_t> valid(CURVE_BINS);
        const double ratio = static_cast<double>(H) * W / static_cast<double>(pn);
        for (int i = 0; i < CURVE_BINS; ++i) {
            sel.clear();
            for (size_t k = 0; k < pn; ++k)
                if (m[k] && bins[k] == i) sel.push_back(b[0].d[k]);
            vals[i] = sel.empty() ? 0.0 : trim_rms(sel);
            valid[i] = static_cast<double>(sel.size()) * ratio >= static_cast<double>(H) * W / 1024.0;
        }
        has_curve[c] = fill_curve(vals, valid, &curves[c]);
    }
    *still_frac = static_cast<double>(nstill) / static_cast<double>(n);
}

}  // namespace

std::string Profile::mode_name() const {
    std::string s = mode == MODE_AUTO ? "auto" : mode == MODE_TEMPORAL ? "temporal" : "spatial";
    if (noise_free) s += "+noise-free";
    return s;
}

std::vector<double> wn_band_std(int levels) {
    const int n = 8 * (1 << levels) + 1;
    Plane d(n, n, 0.0f);
    d.at(n / 2, n / 2) = 1.0f;
    const std::vector<Plane> b = nvn::atrous(d, levels);
    std::vector<double> out;
    std::vector<float> sq(d.size());
    for (int j = 0; j < levels; ++j) {
        for (size_t i = 0; i < sq.size(); ++i) sq[i] = M(b[j].d[i], b[j].d[i]);
        const float s = sum_all_f32(sq.data(), sq.size());
        out.push_back(static_cast<double>(std::sqrt(s)));
    }
    return out;
}

void flat_area_gradient(const Frame& f, bool full_range, int matrix, int out3[3]) {
    const int H = f.H, W = f.W;
    const Plane Y = to_plane(f.p[0], H, W);
    const double fw = std::nearbyint(0.18 * W);     // round(frac * W) (half to even)
    const int S_ = static_cast<int>(std::min(std::min(std::max(64.0, fw), static_cast<double>(H)), static_cast<double>(W)));
    Plane gy, gx;
    gradient(gauss2(Y), gy, gx);
    Plane e(H, W);
    for (size_t i = 0; i < e.size(); ++i) e.d[i] = A(M(gy.d[i], gy.d[i]), M(gx.d[i], gx.d[i]));
    const Plane score = nvn::uniform(e, S_, nvn::NEAREST);
    const int step = std::max(S_ / 8, 2);
    std::vector<int> ys, xs;
    for (int v = S_ / 2; v < H - S_ + S_ / 2 + 1; v += step) ys.push_back(v);
    for (int v = S_ / 2; v < W - S_ + S_ / 2 + 1; v += step) xs.push_back(v);
    const size_t ny = ys.size(), nx = xs.size();
    std::vector<int> nbad(ny * nx, 0);
    std::vector<uint8_t> masks[3];
    clip_masks(f, full_range, matrix, masks);
    for (int c = 0; c < 3; ++c) {
        Plane mf(H, W);
        for (size_t i = 0; i < mf.size(); ++i) mf.d[i] = masks[c][i] ? 1.0f : 0.0f;
        const Plane u = nvn::uniform(mf, S_, nvn::NEAREST);
        for (size_t a = 0; a < ny; ++a)
            for (size_t b = 0; b < nx; ++b) nbad[a * nx + b] += u.at(ys[a], xs[b]) > F_CLIP;
    }
    const std::vector<uint8_t> dm = dead(Y);
    Plane df(H, W);
    for (size_t i = 0; i < df.size(); ++i) df.d[i] = dm[i] ? 1.0f : 0.0f;
    const Plane ud = nvn::uniform(df, S_, nvn::NEAREST);
    std::vector<float> sc(ny * nx);
    std::vector<uint8_t> ok(ny * nx);
    bool any = false;
    for (size_t a = 0; a < ny; ++a)
        for (size_t b = 0; b < nx; ++b) {
            const bool dd = ud.at(ys[a], xs[b]) > static_cast<float>(0.05);
            ok[a * nx + b] = nbad[a * nx + b] < 2 && !dd;
            any = any || ok[a * nx + b];
            sc[a * nx + b] = score.at(ys[a], xs[b]);
        }
    if (any)
        for (size_t k = 0; k < sc.size(); ++k)
            if (!ok[k]) sc[k] = std::numeric_limits<float>::infinity();
    // np.argmin: the first NaN, else the first minimum
    size_t best = 0;
    for (size_t k = 0; k < sc.size(); ++k) {
        if (std::isnan(sc[k])) {
            best = k;
            break;
        }
        if (sc[k] < sc[best]) best = k;
    }
    out3[0] = ys[best / nx] - S_ / 2;
    out3[1] = xs[best % nx] - S_ / 2;
    out3[2] = S_;
}

bool hh1_sigma(const float* Yp, int Hh, int Ww, double* out) {
    const int H = Hh / 2 * 2, W = Ww / 2 * 2;
    if (H < 2 || W < 2) return false;
    const size_t n = static_cast<size_t>(H / 2) * (W / 2);
    std::vector<float> v(n);
    size_t k = 0;
    for (int y = 0; y < H; y += 2)
        for (int x = 0; x < W; x += 2) {
            const float a = Yp[static_cast<size_t>(y) * Ww + x], b = Yp[static_cast<size_t>(y) * Ww + x + 1];
            const float c = Yp[static_cast<size_t>(y + 1) * Ww + x], d = Yp[static_cast<size_t>(y + 1) * Ww + x + 1];
            v[k++] = D(std::fabs(A(S(S(a, b), c), d)), 2.0f);
        }
    *out = static_cast<double>(median_f32(v.data(), n)) / 0.6745;
    return true;
}

std::vector<int> noisiest_candidates(int n) {
    std::vector<int> out;
    if (n <= 0) return out;
    const int last = std::max(n - 3, 0);
    const int num = std::min(32, last + 1);
    // np.linspace(0, last, num) (start / stop as float arrays: y = arange(num) * step + 0; y[-1] = stop)
    std::vector<double> y(num);
    const double delta = static_cast<double>(last) - 0.0;
    if (num - 1 > 0) {
        const double step = delta / static_cast<double>(num - 1);
        for (int i = 0; i < num; ++i) y[i] = (step == 0.0) ? (static_cast<double>(i) / (num - 1)) * delta
                                                           : static_cast<double>(i) * step;
        for (int i = 0; i < num; ++i) y[i] = y[i] + 0.0;
        y[num - 1] = static_cast<double>(last);
    } else {
        for (int i = 0; i < num; ++i) y[i] = static_cast<double>(i) * delta + 0.0;
    }
    for (double v : y) out.push_back(static_cast<int>(std::nearbyint(v)));
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

bool effective_curve(const double* centers, int n, const double* curve, const double* factors,
                     const double* tune, double area_level, const double* tot_ratio, double* curve_out,
                     double* sigma_out) {
    double tk[3];
    for (int c = 0; c < 3; ++c) tk[c] = tune[0] * tune[1 + c];
    bool unit = true;
    for (int i = 0; i < 27; ++i)
        if (!(std::fabs(factors[i] - 1.0) < 1e-12)) unit = false;
    for (int c = 0; c < 3; ++c)
        if (tk[c] != 1.0) unit = false;
    if (unit) return false;
    double cx[9];
    for (int k = 0; k < 9; ++k) cx[k] = 32.0 * k;
    for (int c = 0; c < 3; ++c) {
        const nvn::Interp it(cx, factors + 9 * c, 9);
        for (int i = 0; i < n; ++i) {
            const double k = it(centers[i], false) * tk[c];
            curve_out[c * n + i] = static_cast<double>(M(static_cast<float>(curve[c * n + i]), static_cast<float>(k)));
        }
    }
    for (int c = 0; c < 3; ++c) {
        const nvn::Interp it(centers, curve_out + c * n, n);
        sigma_out[c] = it(area_level, false) * tot_ratio[c];
    }
    return true;
}

int build_profile_v5(const Frame& f, const Frame* pair, const Frame* more, int n_more, bool full_range,
                     int matrix, int mode, int levels, int area_n, const int* area_in, int threads,
                     const Cancel& cancel, Profile* out) {
    if (levels < 2 || levels > 8 || f.H < 1 || f.W < 1) return BAD_ARGUMENT;
    Profile& P = *out;
    P = Profile();
    const int H = f.H, W = f.W;
    P.sub = (f.Hc == H && f.Wc == W) ? 1 : 2;
    P.full_range = full_range;
    int area[4];
    if (area_n == 0) {
        // nv_native.find_flat_area
        int res[4] = {0, 0, 0, -1};
        bool found = false;
        if (std::min(H, W) >= 64) {
            const size_t n = static_cast<size_t>(H) * W;
            std::vector<float> rgb(n * 3);
            nvu::yuv_to_rgb(f.p[0], f.p[1], f.p[2], H, W, f.Hc, f.Wc, matrix, full_range, nvu::CLIP_FINITE,
                            rgb.data());
            nvc::Cancel cb;
            if (cancel) cb = cancel;
            const nvf::Result r = nvf::find_flat_area_genuine(rgb.data(), W, H, 3, 1, nullptr, threads, nullptr, cb);
            if (r.status == 5) return CANCELLED;
            if (r.status < 0) return BAD_ARGUMENT;
            if (r.status != 1 && std::min(r.w, r.h) >= 8) {
                res[0] = r.y;
                res[1] = r.x;
                res[2] = std::min(r.w, r.h);
                res[3] = r.status;
                found = true;
            }
        }
        if (!found) {
            int g[3];
            flat_area_gradient(f, full_range, matrix, g);
            res[0] = g[0];
            res[1] = g[1];
            res[2] = g[2];
            res[3] = 1;
        }
        area_n = 3;
        area[0] = res[0];
        area[1] = res[1];
        area[2] = area[3] = res[2];
        P.flat_status = res[3];
        // build_profile_v5(area=(y0, x0, S)): the square clamp leaves it unchanged
        const int Sz = std::min(std::min(std::max(area[2], 8), H), W);
        area[2] = area[3] = Sz;
        area[0] = std::min(std::max(area[0], 0), H - Sz);
        area[1] = std::min(std::max(area[1], 0), W - Sz);
    } else if (area_n == 4) {
        const int h = std::min(std::max(area_in[2], 8), H), w = std::min(std::max(area_in[3], 8), W);
        area[0] = std::min(std::max(area_in[0], 0), H - h);
        area[1] = std::min(std::max(area_in[1], 0), W - w);
        area[2] = h;
        area[3] = w;
        if (h == w) area_n = 3;
    } else if (area_n == 3) {
        const int Sz = std::min(std::min(std::max(area_in[2], 8), H), W);
        area[0] = std::min(std::max(area_in[0], 0), H - Sz);
        area[1] = std::min(std::max(area_in[1], 0), W - Sz);
        area[2] = area[3] = Sz;
    } else {
        return BAD_ARGUMENT;
    }
    P.area_n = area_n;
    for (int i = 0; i < 4; ++i) P.area[i] = area[i];
    P.mode = MODE_SPATIAL;
    for (int i = 0; i < CURVE_BINS; ++i) P.centers.push_back(256.0 / CURVE_BINS * (i + 0.5));
    const Plane planes[3] = {to_plane(f.p[0], H, W), to_plane(f.p[1], f.Hc, f.Wc), to_plane(f.p[2], f.Hc, f.Wc)};
    for (int c = 0; c < 3; ++c) {
        const int s = c == 0 ? 1 : P.sub;
        const int L = s == 1 ? levels : levels - 1;
        double tot;
        area_band_sigma(planes[c], area, L, s, &P.area_sigma[c], &tot);
        P.levels[c] = L;
        noise_shape(P.area_sigma[c], tot, 0.6, &P.band_ratio[c], &P.tot_ratio[c]);
    }
    std::vector<double> curves[3];
    bool have = false;                       // curves is not None
    bool has[3] = {false, false, false};
    if ((mode == MODE_TEMPORAL || mode == MODE_AUTO) && pair != nullptr) {
        std::vector<double> cv[3], ratios[3];
        double tots[3];
        bool hc[3], hr[3];
        double still;
        noise_curve_temporal(f, *pair, levels, cv, hc, ratios, tots, hr, &still);
        P.has_static = true;
        P.static_fraction = still;
        if (hc[0] && hc[1] && hc[2] && (mode == MODE_TEMPORAL || still >= 0.1)) {
            for (int c = 0; c < 3; ++c)
                if (hr[c]) {
                    P.band_ratio[c] = ratios[c];
                    P.tot_ratio[c] = tots[c];
                }
            P.mode = mode;
            if (mode == MODE_TEMPORAL) {
                for (int c = 0; c < 3; ++c) {
                    curves[c] = cv[c];
                    has[c] = hc[c];
                }
                have = true;
            }
        }
    }
    if (!have) {
        const double r2[3] = {P.band_ratio[0][1], P.band_ratio[1][1], P.band_ratio[2][1]};
        std::vector<std::vector<double>> per[3];
        // one curve per frame (independent; with threads > 1 the frames run concurrently, same results)
        const int nf = 1 + n_more;
        std::vector<std::array<std::vector<double>, 3>> cs(nf);
        std::vector<std::array<bool, 3>> hs(nf);
        auto one = [&](int k) {
            bool h[3];
            noise_curve_spatial(k == 0 ? f : more[k - 1], r2, full_range, matrix, cs[k].data(), h);
            for (int c = 0; c < 3; ++c) hs[k][c] = h[c];
        };
        if (threads > 1 && nf > 1) {
            std::vector<std::thread> th;
            for (int k = 1; k < nf; ++k) th.emplace_back(one, k);
            one(0);
            for (auto& t : th) t.join();
        } else {
            for (int k = 0; k < nf; ++k) one(k);
        }
        for (int k = 0; k < nf; ++k)
            for (int c = 0; c < 3; ++c)
                if (hs[k][c]) per[c].push_back(cs[k][c]);
        // _median_curves: per-bin np.median over the frames that have the plane's curve
        for (int c = 0; c < 3; ++c) {
            if (per[c].empty()) continue;
            has[c] = true;
            curves[c].assign(CURVE_BINS, 0.0);
            for (int i = 0; i < CURVE_BINS; ++i) {
                std::vector<double> v;
                for (const auto& cvv : per[c]) v.push_back(cvv[i]);
                curves[c][i] = median_f64(v.data(), v.size());
            }
        }
    }
    {
        const std::vector<uint8_t> dm = dead(planes[0]);
        size_t cnt = 0;
        for (uint8_t v : dm) cnt += v;
        P.dead_fraction = static_cast<double>(cnt) / static_cast<double>(dm.size());
    }
    if (P.dead_fraction > 0.5) {
        P.noise_free = true;
        for (int c = 0; c < 3; ++c) {
            curves[c].assign(CURVE_BINS, 0.0);
            has[c] = true;
        }
    }
    for (int c = 0; c < 3; ++c) {
        std::vector<double> cv = (has[c] && !curves[c].empty()) ? curves[c]
                                                                : std::vector<double>(CURVE_BINS, P.area_sigma[c][0]);
        if (c != 0)
            for (double& v : cv) v = v * CHROMA_GAIN[c];
        P.curve[c] = cv;
    }
    const int ah = area_n == 4 ? area[2] : area[2], aw = area_n == 4 ? area[3] : area[2];
    P.area_level = static_cast<double>(
        mean_view_f32(planes[0].d.data() + static_cast<size_t>(area[0]) * W + area[1], std::min(ah, H - area[0]),
                      std::min(aw, W - area[1]), W));
    for (int c = 0; c < 3; ++c) {
        const nvn::Interp it(P.centers.data(), P.curve[c].data(), CURVE_BINS);
        P.sigma[c] = it(P.area_level, false) * P.tot_ratio[c];
    }
    return OK;
}

float mean_f32(const float* a, size_t n) { return mean_all_f32(a, n); }

}  // namespace nvp5
