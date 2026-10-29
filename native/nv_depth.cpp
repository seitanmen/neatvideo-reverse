// nv_depth - see nv_depth.h.
#include "nv_depth.h"

#include <cmath>
#include <cstring>
#include <vector>

#include "nv_portable.h"

namespace nvd {

namespace {

// nvfilter._KRB: (kr, kb) of bt709 / bt601 / bt2020
void krb(int matrix, double* kr, double* kb) {
    if (matrix == 1) {
        *kr = 0.299;
        *kb = 0.114;
    } else if (matrix == 2) {
        *kr = 0.2627;
        *kb = 0.0593;
    } else {
        *kr = 0.2126;
        *kb = 0.0722;
    }
}

bool valid(const Fmt& f) {
    if (f.family == 0) {
        if (f.is_float) return false;
        if (f.bits == 8) return f.chroma == 420;
        return (f.bits == 10 || f.bits == 12 || f.bits == 16) && (f.chroma == 420 || f.chroma == 422 || f.chroma == 444);
    }
    if (f.family == 1) return f.chroma == 444 && (f.is_float ? f.bits == 32 : (f.bits == 10 || f.bits == 12 || f.bits == 16));
    return false;
}

inline uint16_t rd16(const uint8_t* p) {
    uint16_t u;
    std::memcpy(&u, p, 2);   // little-endian ('<u2') on the supported hosts
    return u;
}
inline void wr16(uint8_t* p, uint16_t u) { std::memcpy(p, &u, 2); }

// np.clip(x, 0, top) (NaN kept) then .astype(unsigned): NaN -> 0 (the arm64 cast numpy uses)
template <class T>
inline uint32_t clip_cast(T x, T top) {
    if (nvp::isnan_(x)) return 0;
    if (x < T(0)) x = T(0);
    if (x > top) x = top;
    return static_cast<uint32_t>(x);
}

// np.rint (round half to even, NaN / inf kept)
inline float rint_(float x) { return std::nearbyint(x); }
inline double rint_(double x) { return std::nearbyint(x); }

}  // namespace

void Fmt::raw_shape(int W, int H, int k, int* rows, int* cols) const {
    if (k == 0 || chroma == 444) {
        *rows = H;
        *cols = W;
    } else if (chroma == 420) {
        *rows = (H + 1) / 2;
        *cols = (W + 1) / 2;
    } else {
        *rows = H;
        *cols = (W + 1) / 2;
    }
}

void Fmt::internal_shape(int W, int H, int k, int* rows, int* cols) const {
    raw_shape(W, H, k, rows, cols);
    if (k > 0 && chroma == 422) *cols = W;
}

long sanitize(float* p, long n, long* idx, float* vals) {
    long bad = 0;
    bool have = false;
    float top = 0.0f, low = 0.0f;
    for (long i = 0; i < n; ++i) {
        const float v = p[i];
        if (!std::isfinite(v)) {
            ++bad;
            continue;
        }
        if (!have) {
            top = low = v;
            have = true;
        } else {
            top = nvp::fmax(top, v);    // fin.max() / fin.min(): +0 above -0 (NEON FMAX / FMIN)
            low = nvp::fmin(low, v);
        }
    }
    if (bad == 0) return 0;
    long k = 0;
    for (long i = 0; i < n; ++i) {
        const float v = p[i];
        if (std::isfinite(v)) continue;
        if (idx) idx[k] = i;
        if (vals) vals[k] = v;
        ++k;
        p[i] = nvp::isnan_(v) ? 0.0f : (v > 0 ? top : low);   // fin.size == 0: top = low = 0
    }
    return k;
}

bool to_internal(const uint8_t* raw, const Fmt& f, int W, int H, int matrix, float* Y, float* Cb, float* Cr) {
    if (!valid(f)) return false;
    float* out[3] = {Y, Cb, Cr};
    int rows[3], cols[3];
    size_t off[3], o = 0;
    for (int k = 0; k < 3; ++k) {
        f.raw_shape(W, H, k, &rows[k], &cols[k]);
        off[k] = o;
        o += static_cast<size_t>(rows[k]) * cols[k] * f.itemsize();
    }
    if (f.legacy()) {
        for (int k = 0; k < 3; ++k) {
            const size_t n = static_cast<size_t>(rows[k]) * cols[k];
            for (size_t i = 0; i < n; ++i) out[k][i] = static_cast<float>(raw[off[k] + i]);
        }
        return true;
    }
    if (f.family == 0) {
        // np.asarray(p, np.float32) * np.float32(1.0 / 2 ** (b - 8))
        const float s = static_cast<float>(1.0 / std::ldexp(1.0, f.bits - 8));
        std::vector<float> tmp;
        for (int k = 0; k < 3; ++k) {
            const size_t n = static_cast<size_t>(rows[k]) * cols[k];
            const bool wide = k > 0 && f.chroma == 422;
            if (wide) tmp.resize(n);
            float* dst = wide ? tmp.data() : out[k];
            for (size_t i = 0; i < n; ++i) dst[i] = nvp::mul(static_cast<float>(rd16(raw + off[k] + 2 * i)), s);
            if (!wide) continue;
            // nv_depth._widen: even columns = the samples, odd = (c + next) * 0.5 (the last column repeated)
            const int Wc = cols[k];
            for (int y = 0; y < H; ++y) {
                const float* row = tmp.data() + static_cast<size_t>(y) * Wc;
                float* w = out[k] + static_cast<size_t>(y) * W;
                for (int x = 0; x < (W + 1) / 2; ++x) w[2 * x] = row[x];
                for (int x = 0; x < W / 2; ++x) {
                    const float nxt = x + 1 < Wc ? row[x + 1] : row[Wc - 1];
                    w[2 * x + 1] = nvp::mul(nvp::add(row[x], nxt), 0.5f);
                }
            }
        }
        return true;
    }
    // RGB (planes G, B, R): float64 r, g, b = plane * s, then the _KRB matrix to full-range YCbCr 4:4:4
    const double s = f.is_float ? 255.0 : 1.0 / std::ldexp(1.0, f.bits - 8);
    double kr, kb;
    krb(matrix, &kr, &kb);
    const double kg = (1 - kr) - kb, db = 2 * (1 - kb), dr = 2 * (1 - kr);
    const size_t n = static_cast<size_t>(H) * W;
    auto px = [&](int k, size_t i) -> double {
        if (f.is_float) {
            float v;
            std::memcpy(&v, raw + off[k] + 4 * i, 4);
            return static_cast<double>(v);
        }
        return static_cast<double>(rd16(raw + off[k] + 2 * i));
    };
    for (size_t i = 0; i < n; ++i) {
        const double g = nvp::mul(px(0, i), s), b = nvp::mul(px(1, i), s), r = nvp::mul(px(2, i), s);
        const double y = nvp::add(nvp::add(nvp::mul(kr, r), nvp::mul(kg, g)), nvp::mul(kb, b));
        const double cb = nvp::add(nvp::div(nvp::sub(b, y), db), 128.0);
        const double cr = nvp::add(nvp::div(nvp::sub(r, y), dr), 128.0);
        Y[i] = static_cast<float>(y);
        Cb[i] = static_cast<float>(cb);
        Cr[i] = static_cast<float>(cr);
    }
    return true;
}

bool from_internal(const float* Y, const float* Cb, const float* Cr, const Fmt& f, int W, int H, int matrix,
                   const long* const* ridx, const float* const* rvals, const long* rn, uint8_t* out) {
    if (!valid(f)) return false;
    const float* in[3] = {Y, Cb, Cr};
    if (f.legacy()) {
        size_t o = 0;
        for (int k = 0; k < 3; ++k) {
            int r, c;
            f.raw_shape(W, H, k, &r, &c);
            const size_t n = static_cast<size_t>(r) * c;
            for (size_t i = 0; i < n; ++i) out[o + i] = static_cast<uint8_t>(clip_cast(rint_(in[k][i]), 255.0f));
            o += n;
        }
        return true;
    }
    if (f.family == 0) {
        // np.clip(np.rint(np.asarray(p, np.float32) * np.float32(2 ** (b - 8))), 0, 2 ** b - 1).astype('<u2');
        // 4:2:2: the even columns of the widened chroma
        const float s = static_cast<float>(std::ldexp(1.0, f.bits - 8));
        const float top = static_cast<float>((1 << f.bits) - 1);
        size_t o = 0;
        for (int k = 0; k < 3; ++k) {
            int r, c, ir, ic;
            f.raw_shape(W, H, k, &r, &c);
            f.internal_shape(W, H, k, &ir, &ic);
            const int step = (k > 0 && f.chroma == 422) ? 2 : 1;
            for (int y = 0; y < r; ++y)
                for (int x = 0; x < c; ++x) {
                    const float v = in[k][static_cast<size_t>(y) * ic + static_cast<size_t>(x) * step];
                    wr16(out + o, static_cast<uint16_t>(clip_cast(rint_(nvp::mul(v, s)), top)));
                    o += 2;
                }
        }
        return true;
    }
    double kr, kb;
    krb(matrix, &kr, &kb);
    const double dr = 2 * (1 - kr), db = 2 * (1 - kb), kg = (1 - kr) - kb;
    const size_t n = static_cast<size_t>(H) * W;
    if (f.is_float) {
        for (size_t i = 0; i < n; ++i) {
            const double y = Y[i], cb = Cb[i], cr = Cr[i];
            const double r = nvp::add(y, nvp::mul(dr, nvp::sub(cr, 128.0)));
            const double b = nvp::add(y, nvp::mul(db, nvp::sub(cb, 128.0)));
            const double g = nvp::div(nvp::sub(nvp::sub(y, nvp::mul(kr, r)), nvp::mul(kb, b)), kg);
            const float v[3] = {static_cast<float>(nvp::div(g, 255.0)), static_cast<float>(nvp::div(b, 255.0)),
                                static_cast<float>(nvp::div(r, 255.0))};
            for (int k = 0; k < 3; ++k) std::memcpy(out + (static_cast<size_t>(k) * n + i) * 4, &v[k], 4);
        }
        if (ridx && rvals && rn)
            for (int k = 0; k < 3; ++k)
                if (ridx[k] && rvals[k])
                    for (long j = 0; j < rn[k]; ++j)
                        std::memcpy(out + (static_cast<size_t>(k) * n + static_cast<size_t>(ridx[k][j])) * 4, &rvals[k][j], 4);
        return true;
    }
    const double s = std::ldexp(1.0, f.bits - 8), top = static_cast<double>((1 << f.bits) - 1);
    for (size_t i = 0; i < n; ++i) {
        const double y = Y[i], cb = Cb[i], cr = Cr[i];
        const double r = nvp::add(y, nvp::mul(dr, nvp::sub(cr, 128.0)));
        const double b = nvp::add(y, nvp::mul(db, nvp::sub(cb, 128.0)));
        const double g = nvp::div(nvp::sub(nvp::sub(y, nvp::mul(kr, r)), nvp::mul(kb, b)), kg);
        const double v[3] = {g, b, r};
        for (int k = 0; k < 3; ++k)
            wr16(out + (static_cast<size_t>(k) * n + i) * 2, static_cast<uint16_t>(clip_cast(rint_(nvp::mul(v[k], s)), top)));
    }
    return true;
}

bool ofx_unpack(const uint8_t* data, int W, int H, int depth, float* Y, float* Cb, float* Cr, long* const* ridx,
                float* const* rvals, long* rn) {
    if (depth != 16 && depth != 32) return false;
    const size_t n[3] = {static_cast<size_t>(W) * H, static_cast<size_t>(W / 2) * (H / 2), static_cast<size_t>(W / 2) * (H / 2)};
    float* out[3] = {Y, Cb, Cr};
    size_t o = 0;
    for (int k = 0; k < 3; ++k) {
        if (depth == 16) {
            // p.astype(np.float32) * np.float32(1 / 256)
            const float s = static_cast<float>(1.0 / 256);
            for (size_t i = 0; i < n[k]; ++i) out[k][i] = nvp::mul(static_cast<float>(rd16(data + o + 2 * i)), s);
            o += 2 * n[k];
        } else {
            // nv_depth.sanitize, then * np.float32(255)
            std::memcpy(out[k], data + o, 4 * n[k]);
            o += 4 * n[k];
            const long c = sanitize(out[k], static_cast<long>(n[k]), ridx ? ridx[k] : nullptr, rvals ? rvals[k] : nullptr);
            if (rn) rn[k] = c;
            for (size_t i = 0; i < n[k]; ++i) out[k][i] = nvp::mul(out[k][i], 255.0f);
        }
    }
    return true;
}

bool ofx_pack(const float* Y, const float* Cb, const float* Cr, int W, int H, int depth, const long* const* ridx,
              const float* const* rvals, const long* rn, uint8_t* out) {
    if (depth != 8 && depth != 16 && depth != 32) return false;
    const size_t n[3] = {static_cast<size_t>(W) * H, static_cast<size_t>(W / 2) * (H / 2), static_cast<size_t>(W / 2) * (H / 2)};
    const float* in[3] = {Y, Cb, Cr};
    size_t o = 0;
    for (int k = 0; k < 3; ++k) {
        for (size_t i = 0; i < n[k]; ++i) {
            const float v = in[k][i];
            if (depth == 8) {
                out[o++] = static_cast<uint8_t>(clip_cast(rint_(v), 255.0f));      // np.clip(np.rint(p), 0, 255)
            } else if (depth == 16) {
                wr16(out + o, static_cast<uint16_t>(clip_cast(rint_(nvp::mul(v, 256.0f)), 65535.0f)));
                o += 2;
            } else {
                const float q = nvp::div(v, 255.0f);                                // p / np.float32(255)
                std::memcpy(out + o, &q, 4);
                o += 4;
            }
        }
        if (depth == 32 && ridx && rvals && rn && ridx[k] && rvals[k]) {
            uint8_t* base = out + o - 4 * n[k];
            for (long j = 0; j < rn[k]; ++j) std::memcpy(base + 4 * static_cast<size_t>(ridx[k][j]), &rvals[k][j], 4);
        }
    }
    return true;
}

}  // namespace nvd
