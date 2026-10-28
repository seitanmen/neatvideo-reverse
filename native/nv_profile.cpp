// nv_profile - see nv_profile.h.  Every routine names the Python function it reproduces.
#include "nv_profile.h"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "nv_band.h"
#include "nv_curve.h"
#include "nv_flat.h"
#include "nv_lzma.h"
#include "nv_model.h"

namespace nvp {

namespace {

// ------------------------------------------------------------------------------ float helpers
inline uint32_t fbits(float x) {
    uint32_t u;
    std::memcpy(&u, &x, 4);
    return u;
}
inline float bitsf(uint32_t u) {
    float x;
    std::memcpy(&x, &u, 4);
    return x;
}

// Constants from the Python side's float32 bit patterns (np.float32(v).view(uint32)).
const float K_0_33 = bitsf(0x3ea8f5c3u);     // F32(0.33)
const float K_1_4 = bitsf(0x3fb33333u);      // 0x3fb33333 (nv_mcstats)
const float K_TEN4 = bitsf(0x461c4000u);     // 10000.0
const float K_M1 = bitsf(0xbf800000u);       // -1.0
const float K_16 = bitsf(0x41800000u);       // 16.0
const float K_1_32 = bitsf(0x3d000000u);     // 0.03125
const float K_25 = bitsf(0x41c80000u);       // 25.0
const float K_1 = bitsf(0x3f800000u);        // 1.0
const float K_HALF = bitsf(0x3f000000u);     // 0.5
const uint32_t DEFAULT_NAN = 0x7fc00000u;

inline bool is_snan(float x) { return std::isnan(x) && !(fbits(x) & 0x00400000u); }
inline float quiet(float x) { return bitsf(fbits(x) | 0x00400000u); }

// AArch64 FPProcessNaNs of a hardware op(x, y) (FPCR.DN = 0): a signalling NaN first (quieted),
// then the first quiet NaN.  Returns false when neither operand is a NaN.
inline bool nan2(float x, float y, float* r) {
    const bool nx = std::isnan(x), ny = std::isnan(y);
    if (!nx && !ny) return false;
    if (is_snan(x)) *r = quiet(x);
    else if (is_snan(y)) *r = quiet(y);
    else *r = nx ? x : y;
    return true;
}
// A NaN produced from non-NaN operands (inf - inf, 0 / 0, 0 * inf): the AArch64 default NaN.
inline float dflt(float r) { return std::isnan(r) ? bitsf(DEFAULT_NAN) : r; }

// numpy 2.1.3 float32 scalar arithmetic on arm64 (probed): a + b and a * b run as fadd / fmul (b, a);
// a / b as fdiv (a, b).
inline float np_add(float a, float b) {
    float r;
    if (nan2(b, a, &r)) return r;
    return dflt(a + b);
}
inline float np_mul(float a, float b) {
    float r;
    if (nan2(b, a, &r)) return r;
    return dflt(a * b);
}
inline float np_div(float a, float b) {
    float r;
    if (nan2(a, b, &r)) return r;
    return dflt(a / b);
}
inline float np_sqrt(float x) {
    if (std::isnan(x)) return quiet(x);
    return dflt(std::sqrt(x));
}
inline float fabs_bits(float x) { return bitsf(fbits(x) & 0x7fffffffu); }   // abs() keeps an sNaN signalling

// ------------------------------------------------------------------------------ threads
void parallel_for(int n, int threads, const std::function<void(int)>& fn) {
    if (threads <= 1 || n <= 1) {
        for (int i = 0; i < n; ++i) fn(i);
        return;
    }
    std::atomic<int> next(0);
    std::vector<std::thread> pool;
    const int nt = threads < n ? threads : n;
    for (int t = 0; t < nt; ++t)
        pool.emplace_back([&]() {
            for (int i; (i = next.fetch_add(1)) < n;) fn(i);
        });
    for (auto& th : pool) th.join();
}

// ------------------------------------------------------------------------------ tile_dev parts
// nv_profile._dev: |x| <= |y| (also unordered) ? |y| / |x| : |x| / |y|, plus -1.
inline float dev(float x, float y) {
    const float ax = fabs_bits(x), ay = fabs_bits(y);
    const float r = ax > ay ? np_div(ax, ay) : np_div(ay, ax);
    return np_add(r, K_M1);
}

// nv_profile._ratio_dev with b evaluated lazily (b_num / b_den).
inline float ratio_dev(float a, float b_num, float b_den) {
    if (a == 0.0f) return K_TEN4;
    const float b = np_div(b_num, b_den);
    if (b == 0.0f) return K_TEN4;
    return dev(a, b);
}

inline void acc(float* out, int j, float d) { out[j] = np_add(out[j], np_mul(d, d)); }

constexpr int NDIR2 = 8;
constexpr int SIZECLASS[5] = {16, 32, 64, 128, 256};

// ------------------------------------------------------------------------------ mc_stats parts
constexpr uint32_t MC_SEED = 0x7495f1a5u;
constexpr int64_t R17 = (int64_t(1) << 32) / 17;

using IPlane = std::vector<int64_t>;

// nv_mcstats.grey16
IPlane grey16(const float* R, const float* G, const float* B, size_t n) {
    IPlane out(n);
    for (size_t i = 0; i < n; ++i) {
        const float v = ((G[i] + (G[i] + R[i])) + B[i]) * 4.0f;   // a NaN of any payload ends as 0
        double r = std::isnan(v) ? 0.0 : std::rint(static_cast<double>(v));
        if (r < -32768.0) r = -32768.0;
        if (r > 32767.0) r = 32767.0;
        out[i] = static_cast<int64_t>(r);
    }
    return out;
}

// nv_mcstats.down2
IPlane down2(const IPlane& X, int W, int H) {
    const int h = H >> 1, w = W >> 1;
    IPlane out(static_cast<size_t>(h) * w);
    const int m = w & ~7;
    for (int i = 0; i < h; ++i)
        for (int j = 0; j < w; ++j) {
            const int64_t s = X[size_t(2 * i) * W + 2 * j] + X[size_t(2 * i + 1) * W + 2 * j] +
                              X[size_t(2 * i) * W + 2 * j + 1] + X[size_t(2 * i + 1) * W + 2 * j + 1];
            int64_t v;
            if (j < m) {
                v = s >> 2;
                if (v < -32768) v = -32768;
                if (v > 32767) v = 32767;
            } else {
                const uint32_t t = (static_cast<uint32_t>(s) >> 2) & 0xffffu;
                v = t >= 0x8000u ? int64_t(t) - 0x10000 : int64_t(t);
            }
            out[size_t(i) * w + j] = v;
        }
    return out;
}

inline int reflect(int i, int n) {   // np.pad mode="reflect" (one reflection: n > 8 here)
    if (i < 0) return -i;
    if (i >= n) return 2 * (n - 1) - i;
    return i;
}

// nv_mcstats._box17 along axis 1 (horizontal) or 0 (vertical).
IPlane box17(const IPlane& X, int W, int H, int axis) {
    IPlane out(X.size());
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            int64_t s = 0;
            for (int k = -8; k <= 8; ++k)
                s += axis == 1 ? X[size_t(y) * W + reflect(x + k, W)] : X[size_t(reflect(y + k, H)) * W + x];
            out[size_t(y) * W + x] = (s * R17) >> 32;
        }
    return out;
}

// nv_mcstats.highpass
IPlane highpass(const IPlane& X, int W, int H) {
    IPlane T = box17(X, W, H, 1);
    for (auto& t : T) t = int64_t(((uint64_t(t) & 0xffffu) ^ 0x8000u)) - 0x8000;   // stored as int16
    IPlane LP = box17(T, W, H, 0);
    IPlane U(X.size());
    for (size_t i = 0; i < X.size(); ++i) {
        int64_t v = (LP[i] - X[i] + 0x800) >> 4;
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        U[i] = v;
    }
    return U;
}

// nv_mcstats._rng: (a, b, c, d) and the next state.
inline void rng(uint32_t& s, uint32_t wm, uint32_t hm, int* a, int* b, int* c, int* d) {
    *a = static_cast<int>(s % wm);
    uint32_t w9 = s + (s >> 16);
    w9 = w9 + (w9 << 8);
    w9 = w9 + (s >> 24);
    const uint32_t w10 = (w9 << 16) | (s >> 16);
    *b = static_cast<int>(w10 % hm);
    uint32_t w19 = w10 + (w9 & 0xffffu);
    w19 = w19 + (w19 << 8);
    w19 = w19 + ((w9 >> 8) & 0xffu);
    const uint32_t w9b = (w9 & 0xffffu) | (w19 << 16);
    *c = static_cast<int>(w9b % wm);
    uint32_t w24 = w9b + (w19 & 0xffffu);
    w24 = w24 + (w24 << 8);
    w24 = w24 + ((w19 >> 8) & 0xffu);
    const uint32_t x9 = (w19 & 0xffffu) | (w24 << 16);
    *d = static_cast<int>(x9 % hm);
    uint32_t w8 = w19 + w24;
    w8 = w8 + (w8 << 8);
    w8 = w8 + ((w24 >> 8) & 0xffu);
    s = (w24 & 0xffffu) | (w8 << 16);
}

// nv_mcstats._sad of U at (ax, ay) against Rot (U turned by 180 degrees) at (bx, by), n x n.
inline int64_t sad(const IPlane& U, int W, int H, int ax, int ay, int bx, int by, int n) {
    int64_t s = 0;
    for (int y = 0; y < n; ++y) {
        const int64_t* u = &U[size_t(ay + y) * W + ax];
        const int64_t* r = &U[size_t(H - 1 - (by + y)) * W + (W - 1 - bx)];   // Rot[by + y][bx + x]
        for (int x = 0; x < n; ++x) {
            const int64_t d = u[x] - r[-x];
            s += d < 0 ? -d : d;
        }
    }
    return s;
}

// nv_mcstats._stage; false for None.
bool stage(const IPlane& U, int W, int H, int n, int rad, int minside, float* result) {
    if (W < minside || H < minside) return false;
    const uint32_t wm = uint32_t(W - (n - 1)), hm = uint32_t(H - (n - 1));
    const int lim_x = W - n, lim_y = H - n;
    uint32_t s = MC_SEED;
    int64_t accu = 0, cnt = 0;
    for (int it = 0; it < 2000; ++it) {
        int a, b, c, d;
        rng(s, wm, hm, &a, &b, &c, &d);
        const int64_t centre = sad(U, W, H, a, b, c, d, n);
        int64_t best = centre;
        int valid = 0;
        for (int dy = -rad; dy <= rad; ++dy) {
            const int yy = d + dy;
            if (yy < 0 || yy > lim_y) continue;
            for (int dx = -rad; dx <= rad; ++dx) {
                const int xx = c + dx;
                if (xx < 0 || xx > lim_x) continue;
                const int64_t v = sad(U, W, H, a, b, xx, yy, n);
                if (v < best) best = v;
                ++valid;
            }
        }
        if (valid > 3) {
            accu += centre - best;
            ++cnt;
            if (cnt >= 1000) break;
        }
    }
    if (cnt <= 999) return false;
    *result = static_cast<float>(accu) / static_cast<float>(cnt);   // F32(acc) / F32(cnt): exact ints
    return true;
}

}  // namespace

// ------------------------------------------------------------------------------ reference floor
bool load_reference_floor(const char* path, float out[DEV_WORDS]) {
    std::FILE* f = std::fopen(path, "rb");
    if (!f) return false;
    unsigned char head[10];
    bool ok = std::fread(head, 1, 10, f) == 10 && std::memcmp(head, "\x93NUMPY", 6) == 0 && head[6] == 1;
    std::string hdr;
    if (ok) {
        const size_t hl = size_t(head[8]) | (size_t(head[9]) << 8);
        hdr.resize(hl);
        ok = std::fread(&hdr[0], 1, hl, f) == hl;
    }
    // the header numpy writes for np.save(float32[51]): little-endian f4, C order, shape (51,)
    ok = ok && hdr.find("'descr': '<f4'") != std::string::npos &&
         hdr.find("'fortran_order': False") != std::string::npos &&
         hdr.find("'shape': (51,)") != std::string::npos;
    ok = ok && std::fread(out, 4, DEV_WORDS, f) == size_t(DEV_WORDS);
    std::fclose(f);
    return ok;
}

void reference_noise(size_t n, uint32_t seed, float* out) {
    // The loop body 0x9c3bd0..0x9c3cc4 of FUN_009c3bc4, statement for statement as nv_refnoise._gen_src
    // translates the instruction text (only w8 carries over between iterations).
    uint32_t w8 = seed, w9 = 0, w10 = 0;
    float s0 = 0.0f, s1 = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        w9 = w8 >> 16;
        w9 = w9 + w8;
        w9 = w9 + (w9 << 8);
        w9 = w9 + (w8 >> 24);
        w10 = (w9 << 16) | (w8 >> 16);
        w8 = (w8 >> 16) & 31u;
        s0 = static_cast<float>(w8);
        w8 = w10 + (w9 & 0xffffu);
        w8 = w8 + (w8 << 8);
        w10 = w9 >> 8;
        w8 = w8 + (w10 & 0xffu);
        w10 = w9 & 31u;
        w9 = (w9 & 0x0000ffffu) | ((w8 << 16) & 0xffff0000u);
        s1 = static_cast<float>(w10);
        s0 = s0 + s1;
        w9 = w9 + (w8 & 0xffffu);
        w9 = w9 + (w9 << 8);
        w10 = w8 >> 8;
        w9 = w9 + (w10 & 0xffu);
        w10 = w8 & 31u;
        w8 = (w8 & 0x0000ffffu) | ((w9 << 16) & 0xffff0000u);
        s1 = static_cast<float>(w10);
        s0 = s0 + s1;
        w8 = w8 + (w9 & 0xffffu);
        w8 = w8 + (w8 << 8);
        w10 = w9 >> 8;
        w8 = w8 + (w10 & 0xffu);
        w10 = w9 & 31u;
        w9 = (w9 & 0x0000ffffu) | ((w8 << 16) & 0xffff0000u);
        s1 = static_cast<float>(w10);
        s0 = s0 + s1;
        w9 = w9 + (w8 & 0xffffu);
        w9 = w9 + (w9 << 8);
        w10 = w8 >> 8;
        w9 = w9 + (w10 & 0xffu);
        w10 = w8 & 31u;
        w8 = (w8 & 0x0000ffffu) | ((w9 << 16) & 0xffff0000u);
        s1 = static_cast<float>(w10);
        s0 = s0 + s1;
        w8 = w8 + (w9 & 0xffffu);
        w8 = w8 + (w8 << 8);
        w10 = w9 >> 8;
        w10 = w8 + (w10 & 0xffu);
        w8 = w9 & 31u;
        w9 = (w9 & 0x0000ffffu) | ((w10 << 16) & 0xffff0000u);
        s1 = static_cast<float>(w8);
        s0 = s0 + s1;
        w8 = w9 + (w10 & 0xffffu);
        w8 = w8 + (w8 << 8);
        w9 = w10 >> 8;
        w8 = w8 + (w9 & 0xffu);
        w9 = w10 & 31u;
        s1 = static_cast<float>(w9);
        s0 = s0 + s1;
        w9 = w8 + w10;
        w9 = w9 + (w9 << 8);
        w10 = w8 >> 8;
        w9 = w9 + (w10 & 0xffu);
        w10 = w8 & 31u;
        w8 = (w8 & 0x0000ffffu) | ((w9 << 16) & 0xffff0000u);
        s1 = static_cast<float>(w10);
        s0 = s0 + s1;
        out[i] = s0;   // small integers: exact in float32
    }
}

int compute_reference_floor(float out[DEV_WORDS], int threads) {
    std::vector<float> x(65536);
    reference_noise(x.size(), 0x12345678u, x.data());
    std::vector<uint32_t> m(nvm::STRUCT_WORDS);
    const int st = nvm::model_full(x.data(), 256, 256, -1, m.data());
    if (st != nvm::OK) return st;
    return tile_dev(x.data(), 256, 256, m.data(), nullptr, out, threads);
}

// ------------------------------------------------------------------------------ tile_dev
int tile_dev(const float* plane, int W, int H, const uint32_t* model, const float* floor,
             float out[DEV_WORDS], int threads) {
    if (W < 32 || H < 32) return NOT_IMPLEMENTED;
    const int nx = ((W - 32) >> 4) + 1;
    const int ny = ((H - 32) >> 4) + 1;
    const int ntiles = nx * ny;
    std::vector<uint32_t> tw(size_t(ntiles) * nvm::STRUCT_WORDS);
    std::vector<int> tst(ntiles, 0);
    // tile index tx * ny + ty: the Python loop order (tx outer, ty inner)
    parallel_for(ntiles, threads, [&](int k) {
        const int tx = k / ny, ty = k % ny;
        std::vector<float> tile(32 * 32);
        for (int r = 0; r < 32; ++r)
            std::memcpy(&tile[size_t(r) * 32], plane + size_t(16 * ty + r) * W + 16 * tx, 32 * sizeof(float));
        tst[k] = nvm::model_full(tile.data(), 32, 32, 2, &tw[size_t(k) * nvm::STRUCT_WORDS], 2);
    });
    for (int k = 0; k < ntiles; ++k)
        if (tst[k] != nvm::OK) return tst[k];
    float pm[MODEL_WORDS];
    std::memcpy(pm, model, sizeof(pm));
    for (int j = 0; j < DEV_WORDS; ++j) out[j] = 0.0f;
    for (int k = 0; k < ntiles; ++k) {
        float t[0x7b0 / 4];   // the words read: up to 0x7ac
        std::memcpy(t, &tw[size_t(k) * nvm::STRUCT_WORDS], sizeof(t));
        acc(out, 0, ratio_dev(np_div(t[6], t[5]), pm[6], pm[5]));
        for (int i = 0; i < NDIR2; ++i)
            for (int lv = 0; lv < 2; ++lv) {
                const int j = lv == 0 ? 1 + i : 0x11 + i;
                const int oA = (0x3c0 + 0x40 * lv) / 4 + i;
                const int oR = (0x7a8 + 4 * lv) / 4;
                const float a = np_div(t[oA], pm[oA]);
                acc(out, j, ratio_dev(a, t[oR], pm[oR]));
            }
        for (int kk = 0; kk < 2; ++kk) {
            for (int j = 0; j < 8; ++j) {
                const float x = t[15 + 8 * kk + j], y = pm[15 + 8 * kk + j];
                if (x < 0.0f || y < 0.0f) break;            // fcmp #0 / b.mi (a NaN goes on)
                const float d = (x == 0.0f || y == 0.0f) ? K_TEN4 : dev(x, y);
                acc(out, 0x21 + 8 * kk + j, d);
            }
            const float a = np_div(t[kk], t[5 + kk]);
            acc(out, 0x31 + kk, ratio_dev(a, pm[kk], pm[5 + kk]));
        }
    }
    const float n = static_cast<float>(ny * nx);
    int idx[DEV_WORDS];
    int ni = 0;
    idx[ni++] = 0;
    for (int j = 1; j < 1 + NDIR2; ++j) idx[ni++] = j;
    for (int j = 0x11; j < 0x11 + NDIR2; ++j) idx[ni++] = j;
    for (int j = 0x21; j < 0x33; ++j) idx[ni++] = j;
    for (int q = 0; q < ni; ++q) out[idx[q]] = np_sqrt(np_div(out[idx[q]], n));
    if (floor)
        for (int q = 0; q < ni; ++q)
            if (!(out[idx[q]] > floor[idx[q]])) out[idx[q]] = floor[idx[q]];   // a NaN takes the floor
    return OK;
}

// ------------------------------------------------------------------------------ mc_stats
void mc_stats(const float* R, const float* G, const float* B, int W, int H, float out[MC_WORDS]) {
    std::vector<std::pair<int, int>> dims;
    if (W >= 16 && H >= 16) {
        int w = W, h = H;
        for (int l = 0; l < 16; ++l) {
            dims.emplace_back(w, h);
            if (l > 14 || h < 32) break;
            if (!(w > 31)) break;
            w >>= 1;
            h >>= 1;
        }
    }
    for (int i = 0; i < MC_WORDS; ++i) out[i] = 0.0f;
    float v16 = 0.0f, v32 = 0.0f;
    IPlane X;
    int xw = W, xh = H;
    if (!dims.empty()) X = grey16(R, G, B, size_t(W) * H);
    for (size_t l = 0; l < dims.size(); ++l) {
        if (l) {
            X = down2(X, xw, xh);
            xw >>= 1;
            xh >>= 1;
        }
        const IPlane U = highpass(X, xw, xh);
        float r16, r32;
        if (!stage(U, xw, xh, 16, 2, 21, &r16)) {
            v16 = v16 * K_HALF;
            v32 = v32 * K_HALF;
        } else {
            v16 = r16;
            if (stage(U, xw, xh, 32, 1, 37, &r32)) v32 = r32;
            else if (l) v32 = v32 * K_HALF;
            else v32 = v16 * K_1_4;
        }
        out[l] = v16;
        out[16 + l] = v32;
    }
    for (int l = static_cast<int>(dims.size()); l < 16; ++l) {
        v16 = v16 * K_HALF;
        v32 = v32 * K_HALF;
        out[l] = v16;
        out[16 + l] = v32;
    }
}

// ------------------------------------------------------------------------------ small parts
int size_class(int m) {
    if (m < SIZECLASS[0]) return 0;
    for (int c = 1; c < 5; ++c)
        if (m < SIZECLASS[c]) {
            const int lo = SIZECLASS[c - 1], hi = SIZECLASS[c];
            return static_cast<int>((static_cast<float>(m - lo) / static_cast<float>(hi - lo)) +
                                    static_cast<float>(c));
        }
    return 5;
}

int curve_bin(float mean) {
    const float v = np_mul(np_add(mean, K_16), K_1_32);
    if (!(v > 0.0f)) return 0;
    return (!std::isfinite(v) || v >= 8.0f) ? 8 : static_cast<int>(v);
}

// ------------------------------------------------------------------------------ build_profile
int build_profile(const float* rgb, int W, int H, int stride_c, int mode, int x, int y, int w, int h,
                  const float* floor, uint8_t* out, bool lzma, int threads) {
    if (mode < 0 || mode > 3 || stride_c < 3 || x < 0 || y < 0 || w < 1 || h < 1 || x + w > W || y + h > H)
        return BAD_ARGUMENT;
    const size_t n = size_t(w) * h;
    std::vector<std::vector<float>> planes(6, std::vector<float>(n));
    for (int r = 0; r < h; ++r)
        for (int c = 0; c < w; ++c) {
            const float* px = rgb + (size_t(y + r) * W + (x + c)) * stride_c;
            for (int ch = 0; ch < 3; ++ch) planes[ch][size_t(r) * w + c] = planes[3 + ch][size_t(r) * w + c] = px[ch];
        }
    nvf::transform(planes[3].data(), planes[4].data(), planes[5].data(), n, mode);

    std::memset(out, 0, PROF_BYTES);
    auto put_u32 = [&](size_t off, uint32_t v) { std::memcpy(out + off, &v, 4); };
    auto put_f32 = [&](size_t off, float v) { std::memcpy(out + off, &v, 4); };
    put_u32(0x48, static_cast<uint32_t>(mode));

    std::vector<std::vector<uint32_t>> models(6, std::vector<uint32_t>(nvm::STRUCT_WORDS));
    int mst[6];
    parallel_for(6, threads, [&](int i) { mst[i] = nvm::model_full(planes[i].data(), w, h, -1, models[i].data()); });
    for (int i = 0; i < 6; ++i)
        if (mst[i] != nvm::OK) return mst[i];
    for (int i = 0; i < 6; ++i) std::memcpy(out + 0x4c + size_t(i) * 0x1f54, models[i].data(), 0x1f54);

    const size_t dev_off[3] = {0xbc44, 0xbd10, 0xbddc};
    for (int i = 0; i < 3; ++i) {
        float d[DEV_WORDS];
        const int st = tile_dev(planes[i].data(), w, h, models[i].data(), floor, d, threads);
        if (st != OK) return st;
        std::memcpy(out + dev_off[i], d, sizeof(d));
    }
    float mc[MC_WORDS];
    mc_stats(planes[0].data(), planes[1].data(), planes[2].data(), w, h, mc);
    std::memcpy(out + 0xbea8, mc, sizeof(mc));
    float means[3];
    bool flags[3];
    for (int c = 0; c < 3; ++c) {
        means[c] = nvb::mean_exact(planes[c].data(), n);
        put_f32(0xbf28 + 4 * c, means[c]);
        flags[c] = nvc::is_clip_genuine(planes[c].data(), n) > K_0_33;
        out[0xbf3c + c] = flags[c] ? 1 : 0;
    }
    out[0xbf34] = 1;
    put_u32(0xbf38, static_cast<uint32_t>(size_class(w < h ? w : h)));
    // curve: 3 channels x 9 bins x 0x20 bytes from 0xbf44 (mode word 0xbf40 stays 0)
    for (int c = 0; c < 3; ++c) {
        for (int k = 0; k < 9; ++k)
            for (int q = 0; q < 6; ++q) put_f32(0xbf44 + 0x120 * c + 0x20 * k + 4 * q, K_1);
        const size_t e = 0xbf44 + 0x120 * c + 0x20 * curve_bin(means[c]);
        put_u32(e + 0x18, 1);
        out[e + 0x1d] = flags[c] ? 1 : 0;
        out[e + 0x1e] = 0;                                  // 3.0 < level 1.0
    }
    if (lzma) {
        // FUN_009b41c4 (nv_profile.lzma_part): R, G, B in turn; a failure clears 0x18..0x2b and 0xc2a4
        std::vector<uint8_t> blobs[3];
        bool ok[3];
        parallel_for(3, threads, [&](int c) {
            const std::vector<uint8_t> src = nvlz::plane_int16(planes[c].data(), n);
            ok[c] = nvlz::blob(src.data(), src.size(), &blobs[c]);
        });
        if (ok[0] && ok[1] && ok[2]) {
            for (int c = 0; c < 3; ++c) put_u32(0x18 + 4 * c, static_cast<uint32_t>(blobs[c].size()));
            put_u32(0x24, static_cast<uint32_t>(w));
            put_u32(0x28, static_cast<uint32_t>(h));
            out[0xc2a4] = 1;
        }
    } else {                                                // the LZMA sizes left 0
        put_u32(0x24, static_cast<uint32_t>(w));
        put_u32(0x28, static_cast<uint32_t>(h));
        out[0xc2a4] = 1;
    }
    put_u32(0xc2c0, static_cast<uint32_t>(w));
    put_u32(0xc2c4, static_cast<uint32_t>(h));
    put_f32(0xc2c8, K_25);
    for (int q = 1; q < 10; ++q) put_f32(0xc2c8 + 4 * q, K_1);
    return OK;
}

}  // namespace nvp
