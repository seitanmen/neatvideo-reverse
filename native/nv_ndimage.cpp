// nv_ndimage - see nv_ndimage.h.  References: scipy v1.15.3 scipy/ndimage/src/{ni_filters.c,
// ni_support.c, ni_interpolation.c, ni_splines.c}, numpy v2.1.3 numpy/_core/src/multiarray/
// compiled_base.c (arr_interp), numpy/_core/src/umath/{loops_utils.h.src, loops_minmax.dispatch.c.src,
// clip.cpp}; the operand order of the compiled loops was read from `otool -tv` of the installed
// _nd_image.cpython-313-darwin.so / _multiarray_umath.cpython-313-darwin.so.
#include "nv_ndimage.h"

#include <cmath>

namespace nvn {

#if !defined(__aarch64__)
float SQRT(float a) { return std::sqrt(a); }
double FMAD(double n, double m, double a) { return std::fma(n, m, a); }
#endif

namespace {
float (*volatile g_expf)(float) = ::expf;
double (*volatile g_floor)(double) = ::floor;

// gaussian_filter(sigma=1.0, truncate=4.0) weights: _gaussian_kernel1d(1.0, 0, 4)[::-1] as float64
// bit patterns (from Python; the kernel is exactly symmetric, so NI_Correlate1D takes the
// symmetric branch)
const uint64_t GW_BITS[9] = {0x3f218a9c4fd536c6ull, 0x3f722724cb7eb269ull, 0x3faba4b99d1799abull,
                             0x3fcef8eb9ad499baull, 0x3fd9884a307594fbull, 0x3fcef8eb9ad499baull,
                             0x3faba4b99d1799abull, 0x3f722724cb7eb269ull, 0x3f218a9c4fd536c6ull};
}  // namespace

float np_expf(float x) { return g_expf(x); }

long ext_index(long i, long n, Mode m) {
    if (i >= 0 && i < n) return i;
    if (m == NEAREST) return i < 0 ? 0 : n - 1;
    long p = 2 * n;
    long r = ((i % p) + p) % p;
    return r < n ? r : p - 1 - r;
}

// --------------------------------------------------------------------- gaussian_filter(a, 1.0)
// NI_Correlate1D, symmetric branch (disassembly 0x9c7c..0x9cc0):
//   d0 = iline[0] * fw[0];  for jj = -4..-1: d1 = iline[jj] + iline[-jj]; d0 = fmadd(d1, fw[jj], d0)
// The line buffer holds the float32 input as double; the output line is cast to float32.
static void gauss_axis(const Plane& in, Plane& out, int axis) {
    double fwv[9];
    for (int i = 0; i < 9; ++i) fwv[i] = dbits(GW_BITS[i]);
    const double* fw = fwv + 4;
    const int H = in.H, W = in.W;
    if (axis == 0) {
        std::vector<long> ix(H + 8);
        for (long i = -4; i < H + 4; ++i) ix[i + 4] = ext_index(i, H, REFLECT);
        for (int y = 0; y < H; ++y) {
            const float* r0 = in.row((int)ix[y + 4]);
            const float* rm[4];
            const float* rp[4];
            for (int k = 0; k < 4; ++k) {      // jj = -4 + k
                rm[k] = in.row((int)ix[y + 4 - 4 + k]);
                rp[k] = in.row((int)ix[y + 4 + 4 - k]);
            }
            float* o = out.row(y);
            for (int x = 0; x < W; ++x) {
                double acc = MD((double)r0[x], fw[0]);
                for (int k = 0; k < 4; ++k) {
                    double s = AD((double)rm[k][x], (double)rp[k][x]);
                    acc = FMAD(s, fw[-4 + k], acc);
                }
                o[x] = (float)acc;
            }
        }
    } else {
        std::vector<double> buf(W + 8);
        std::vector<long> ix(W + 8);
        for (long i = -4; i < W + 4; ++i) ix[i + 4] = ext_index(i, W, REFLECT);
        for (int y = 0; y < H; ++y) {
            const float* r = in.row(y);
            for (int i = 0; i < W + 8; ++i) buf[i] = (double)r[ix[i]];
            const double* il = buf.data() + 4;
            float* o = out.row(y);
            for (int x = 0; x < W; ++x) {
                double acc = MD(il[x], fw[0]);
                for (int jj = -4; jj < 0; ++jj) {
                    double s = AD(il[x + jj], il[x - jj]);
                    acc = FMAD(s, fw[jj], acc);
                }
                o[x] = (float)acc;
            }
        }
    }
}

Plane gauss1(const Plane& a) {
    Plane t(a.H, a.W), o(a.H, a.W);
    if (a.size() == 0) return o;
    gauss_axis(a, t, 0);
    gauss_axis(t, o, 1);
    return o;
}

// ------------------------------------------------------------------- uniform_filter(a, size, mode)
// NI_UniformFilter1D (disassembly 0xa928..0xa988): tmp = 0.0; tmp += iline[0..size-1];
// out[0] = tmp / size; then tmp = tmp + (iline[ll-1+size] - iline[ll-1]); out[ll] = tmp / size.
static void uniform_axis(const Plane& in, Plane& out, int axis, int size, Mode mode) {
    const int size1 = size / 2, size2 = size - size1 - 1;
    const double fs = (double)size;
    const int H = in.H, W = in.W;
    if (axis == 0) {
        std::vector<long> ix(H + size);
        for (long i = -size1; i < H + size2; ++i) ix[i + size1] = ext_index(i, H, mode);
        std::vector<double> tmp(W, 0.0);
        for (int k = 0; k < size; ++k) {
            const float* r = in.row((int)ix[k]);
            for (int x = 0; x < W; ++x) tmp[x] = AD(tmp[x], (double)r[x]);
        }
        float* o = out.row(0);
        for (int x = 0; x < W; ++x) o[x] = (float)DD(tmp[x], fs);
        for (int ll = 1; ll < H; ++ll) {
            const float* r2 = in.row((int)ix[ll - 1 + size]);
            const float* r1 = in.row((int)ix[ll - 1]);
            o = out.row(ll);
            for (int x = 0; x < W; ++x) {
                tmp[x] = AD(tmp[x], SD((double)r2[x], (double)r1[x]));
                o[x] = (float)DD(tmp[x], fs);
            }
        }
    } else {
        std::vector<double> buf(W + size);
        std::vector<long> ix(W + size);
        for (long i = -size1; i < W + size2; ++i) ix[i + size1] = ext_index(i, W, mode);
        for (int y = 0; y < H; ++y) {
            const float* r = in.row(y);
            for (int i = 0; i < W + size - 1; ++i) buf[i] = (double)r[ix[i]];
            float* o = out.row(y);
            double tmp = 0.0;
            for (int k = 0; k < size; ++k) tmp = AD(tmp, buf[k]);
            o[0] = (float)DD(tmp, fs);
            for (int ll = 1; ll < W; ++ll) {
                tmp = AD(tmp, SD(buf[ll - 1 + size], buf[ll - 1]));
                o[ll] = (float)DD(tmp, fs);
            }
        }
    }
}

Plane uniform(const Plane& a, int size, Mode mode) {
    if (size <= 1) return a;            // uniform_filter skips axes with size <= 1
    Plane t(a.H, a.W), o(a.H, a.W);
    if (a.size() == 0) return o;
    uniform_axis(a, t, 0, size, mode);
    uniform_axis(t, o, 1, size, mode);
    return o;
}

// ------------------------------------------------------- map_coordinates(order=1, mode="nearest")
// NI_GeometricTransform: cc = coord + 0 (nprepad); start = (intp)floor(cc); at the borders the
// two taps are clamped (map_coordinate NEAREST); weights x = cc - floor(cc), w0 = 1 - x,
// w1 = 1 - w0 (get_spline_interpolation_weights); t = 0; for the taps (0,0),(0,1),(1,0),(1,1):
// coeff = value; coeff *= wy; coeff *= wx; t += coeff (disassembly 0xfed8.., 0x1011c).
Plane map_coords1(const Plane& img, const Plane& cy, const Plane& cx) {
    Plane out(cy.H, cy.W);
    const long H = img.H, W = img.W;
    const size_t n = cy.size();
    for (size_t i = 0; i < n; ++i) {
        double ccy = AD((double)cy.d[i], 0.0);
        double ccx = AD((double)cx.d[i], 0.0);
        double fy = g_floor(ccy), fx = g_floor(ccx);
        long sy = (long)fy, sx = (long)fx;
        long y0 = sy, y1 = sy + 1, x0 = sx, x1 = sx + 1;
        if (sy < 0 || sy + 1 >= H) {
            y0 = y0 < 0 ? 0 : (y0 > H - 1 ? H - 1 : y0);
            y1 = y1 < 0 ? 0 : (y1 > H - 1 ? H - 1 : y1);
        }
        if (sx < 0 || sx + 1 >= W) {
            x0 = x0 < 0 ? 0 : (x0 > W - 1 ? W - 1 : x0);
            x1 = x1 < 0 ? 0 : (x1 > W - 1 ? W - 1 : x1);
        }
        double ry = SD(ccy, fy), rx = SD(ccx, fx);
        double wy0 = SD(1.0, ry), wy1 = SD(1.0, wy0);
        double wx0 = SD(1.0, rx), wx1 = SD(1.0, wx0);
        double t = 0.0;
        t = AD(t, MD(MD((double)img.d[y0 * W + x0], wy0), wx0));
        t = AD(t, MD(MD((double)img.d[y0 * W + x1], wy0), wx1));
        t = AD(t, MD(MD((double)img.d[y1 * W + x0], wy1), wx0));
        t = AD(t, MD(MD((double)img.d[y1 * W + x1], wy1), wx1));
        out.d[i] = (float)t;
    }
    return out;
}

// --------------------------------------------------------------- median_filter(a, 3, "nearest")
// NI_RankFilter: the 3x3 footprint in row-major order (borders clamped), as doubles, then
// NI_Select(buffer, 0, 8, 4) (Hoare partition quickselect, recursive).
static double ni_select(double* buffer, long min, long max, long rank) {
    for (;;) {
        if (min == max) return buffer[min];
        double x = buffer[min];
        long ii = min - 1, jj = max + 1;
        for (;;) {
            do jj--; while (buffer[jj] > x);
            do ii++; while (buffer[ii] < x);
            if (ii < jj) {
                double t = buffer[ii];
                buffer[ii] = buffer[jj];
                buffer[jj] = t;
            } else {
                break;
            }
        }
        ii = jj - min + 1;
        if (rank < ii) {
            max = jj;
        } else {
            min = jj + 1;
            rank = rank - ii;
        }
    }
}

Plane median3(const Plane& a) {
    Plane out(a.H, a.W);
    double buf[9];
    for (int y = 0; y < a.H; ++y) {
        for (int x = 0; x < a.W; ++x) {
            int k = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                int yy = (int)ext_index(y + dy, a.H, NEAREST);
                for (int dx = -1; dx <= 1; ++dx) {
                    int xx = (int)ext_index(x + dx, a.W, NEAREST);
                    buf[k++] = (double)a.at(yy, xx);
                }
            }
            out.at(y, x) = (float)ni_select(buf, 0, 8, 4);
        }
    }
    return out;
}

// --------------------------------------------------------------------------------- numpy.interp
Interp::Interp(const double* xp_, const double* fp_, int n) : xp(xp_, xp_ + n), fp(fp_, fp_ + n) {
    for (int i = 0; i + 1 < n; ++i) slopes.push_back(DD(SD(fp[i + 1], fp[i]), SD(xp[i + 1], xp[i])));
}

double Interp::operator()(double x, bool) const {
    const long n = (long)xp.size();
    const double lval = fp[0], rval = fp[n - 1];
    if (n == 1) return (x < xp[0]) ? lval : ((x > xp[0]) ? rval : fp[0]);
    if (isnand_(x)) return x;
    long j;
    if (x > xp[n - 1]) j = n;
    else if (x < xp[0]) j = -1;
    else {
        // binary_search_with_guess: for a strictly increasing xp the result is the last j with
        // xp[j] <= x whatever the guess
        long lo = 0, hi = n;
        while (lo < hi) {
            long mid = lo + ((hi - lo) >> 1);
            if (x >= xp[mid]) lo = mid + 1; else hi = mid;
        }
        j = lo - 1;
    }
    if (j == -1) return lval;
    if (j == n) return rval;
    if (j == n - 1) return fp[j];
    if (xp[j] == x) return fp[j];
    const double slope = slopes[j];
    double r = FMAD(slope, SD(x, xp[j]), fp[j]);
    if (isnand_(r)) {
        r = FMAD(slope, SD(x, xp[j + 1]), fp[j + 1]);
        if (isnand_(r) && fp[j] == fp[j + 1]) r = fp[j];
    }
    return r;
}

Plane curve_sigma(const Plane& lum, const double* centers, const double* curve, int n) {
    Interp it(centers, curve, n);
    Plane out(lum.H, lum.W);
    for (size_t i = 0; i < lum.size(); ++i) out.d[i] = (float)it((double)lum.d[i], true);
    return out;
}

// ------------------------------------------------------------------------------ _b3 / atrous
// _b3: p = pad(a, 2*step, "symmetric"); tap(k)[i] = a[sym(i + (k-2)*step)];
// out = ((1/16 * (t0 + t4)) + (4/16 * (t1 + t3))) + (6/16 * t2)   (numpy float32, left to right)
Plane b3(const Plane& a, int step, int axis) {
    const float w0 = bits(0x3d800000u), w1 = bits(0x3e800000u), w2 = bits(0x3ec00000u);  // 1/16 4/16 6/16
    Plane o(a.H, a.W);
    const int H = a.H, W = a.W;
    if (a.size() == 0) return o;
    if (axis == 0) {
        for (int y = 0; y < H; ++y) {
            const float* t[5];
            for (int k = 0; k < 5; ++k) t[k] = a.row((int)ext_index((long)y + (long)(k - 2) * step, H, REFLECT));
            float* r = o.row(y);
            for (int x = 0; x < W; ++x)
                r[x] = A(A(M(w0, A(t[0][x], t[4][x])), M(w1, A(t[1][x], t[3][x]))), M(w2, t[2][x]));
        }
    } else {
        std::vector<int> ix[5];
        for (int k = 0; k < 5; ++k) {
            ix[k].resize(W);
            for (int x = 0; x < W; ++x) ix[k][x] = (int)ext_index((long)x + (long)(k - 2) * step, W, REFLECT);
        }
        for (int y = 0; y < H; ++y) {
            const float* s = a.row(y);
            float* r = o.row(y);
            for (int x = 0; x < W; ++x)
                r[x] = A(A(M(w0, A(s[ix[0][x]], s[ix[4][x]])), M(w1, A(s[ix[1][x]], s[ix[3][x]]))),
                         M(w2, s[ix[2][x]]));
        }
    }
    return o;
}

std::vector<Plane> atrous(const Plane& img, int levels) {
    std::vector<Plane> bands;
    Plane c = img;
    for (int j = 0; j < levels; ++j) {
        Plane s = b3(b3(c, 1 << j, 0), 1 << j, 1);
        Plane b(c.H, c.W);
        for (size_t i = 0; i < c.size(); ++i) b.d[i] = S(c.d[i], s.d[i]);
        bands.push_back(std::move(b));
        c = std::move(s);
    }
    bands.push_back(std::move(c));
    return bands;
}

// --------------------------------------------------------------------------- _down2 / _fit
// a[:H, :W].reshape(H/2, 2, W/2, 2).mean((1, 3)): add.reduce with the identity +0 over axis 1 of
// the pairwise sums of axis 3 (n < 8: -0 + x0 + x1), then / 4; when the output is one column
// wide numpy's iterator runs the four values in sequence instead (both observed bit for bit)
Plane down2(const Plane& a) {
    const int H2 = a.H / 2, W2 = a.W / 2;
    Plane o(H2, W2);
    const float z = 0.0f, four = bits(0x40800000u);
    for (int y = 0; y < H2; ++y) {
        const float* r0 = a.row(2 * y);
        const float* r1 = a.row(2 * y + 1);
        float* r = o.row(y);
        if (W2 == 1) {
            // output width 1: numpy runs the four values in sequence, starting from the first value
            // itself (a signalling NaN enters unquieted), then adds the +0 identity: (((a + b) + c)
            // + d) + 0 (observed with NaN payloads and -0 on every shape up to 40x40 and large ones)
            r[0] = D(A(A(A(A(r0[0], r0[1]), r1[0]), r1[1]), z), four);
            continue;
        }
        for (int x = 0; x < W2; ++x) {
            // numpy's pairwise sum for n < 8 starts from -0.0, which the compiler folds away
            // (-0.0 + x -> x): the first value enters unchanged (a signalling NaN stays signalling)
            float p0 = A(r0[2 * x], r0[2 * x + 1]);
            float p1 = A(r1[2 * x], r1[2 * x + 1]);
            r[x] = D(A(p1, A(p0, z)), four);   // out = +0; out = pw + out (operands as compiled)
        }
    }
    return o;
}

Plane fit(const Plane& a, int H, int W) {
    Plane o(H, W);
    for (int y = 0; y < H; ++y) {
        int yy = y < a.H ? y : a.H - 1;
        for (int x = 0; x < W; ++x) {
            int xx = x < a.W ? x : a.W - 1;
            o.at(y, x) = a.at(yy, xx);
        }
    }
    return o;
}

Plane to_grid(const Plane& a, int H, int W) {
    if (a.H == H && a.W == W) return a;
    return fit(down2(a), H, W);
}

}  // namespace nvn
