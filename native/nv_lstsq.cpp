// nv_lstsq - see nv_lstsq.h.
//
// The path of numpy.linalg.lstsq (umath_linalg: column-major copy of A, B in a max(m, n) x nrhs buffer,
// optimal lwork from a workspace query) into dgelsd for m >= n, n <= SMLSIZ (25), nrhs = 1:
//   dgelsd: dlange / dlascl (range), [m >= mnthr: dgeqrf (-> dgeqr2) + dormqr (-> dorm2r) + dlaset 'L'],
//           dgebrd (-> dgebd2), dormbr Q (-> dorm2r), dlalsd (n <= smlsiz: dlanst, dlascl, dlaset, dlasdq ->
//           dbdsqr, dlascl per row, dgemm, dlacpy, dlascl), dormbr P (-> dorml2), unscaling.
// FMA contractions of the gfortran build (fmadd / fnmsub in the disassembly of libopenblasp-r0.3.29.dylib):
//   dlartg   d = sqrt(fma(f, f, g*g))                      dlapy2  w * sqrt(fma(z/w, z/w, 1))
//   dlas2    sqrt(fma(q, q, round(as*as))), sqrt(fma(r, r, 1))
//   dlasv2   sqrt(fma(T, T, MM)), sqrt(fma(L, L, MM)), sqrt(fma(T, T, 4)), (fma(SRT, M, CRT)) / A
//   dlasr    A(j+1) = fma(c, temp, -round(s*A(j))), A(j) = fma(s, temp, round(c*A(j)))
//   dbdsqr   c*x +- s*y = fma(c, x, +-round(s*y)) in the shifted QR sweeps
// NEOVERSEN1 kernels (the DYNAMIC_ARCH choice on this M2 Max): dnrm2 (scaled sum of squares, fma for
// |x| <= scale), dgemv_t (8-lane fmla blocks for incx = 1, then sequential fma; y = fma(alpha, dot, y)),
// dgemv_n (y_i = fma(alpha*x_j, a_ij, y_i)), dger (axpy with da = y_j*alpha, fma), drot (fma(s, y, c*x),
// fma(-s, x, c*y)), dscal (x*alpha), dgemm (sequential fma over k; checked by the tests).
#include "nv_lstsq.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace nvl5 {
namespace {

inline double bitsd(uint64_t u) { double d; std::memcpy(&d, &u, 8); return d; }

const double EPS_E = bitsd(0x3ca0000000000000ULL);   // dlamch('E') = 2^-53
const double EPS_P = bitsd(0x3cb0000000000000ULL);   // dlamch('P') = 2^-52
const double SFMIN = bitsd(0x0010000000000000ULL);   // dlamch('S') = 2^-1022
const double HUGEV = bitsd(0x7fefffffffffffffULL);   // dlamch('O')
const double RTMIN = bitsd(0x2000000000000000ULL);   // dlartg sqrt(safmin)
const double RTMAX = bitsd(0x5fd6a09e667f3bcdULL);   // dlartg sqrt(safmax / 2)
const double SAFMAX_LA = bitsd(0x7fd0000000000000ULL);  // la_constants dsafmax = 2^1022

inline double sign(double a, double b) { return std::copysign(std::fabs(a), b); }
inline double fmaxnm(double a, double b) { return std::fmax(a, b); }
inline double fminnm(double a, double b) { return std::fmin(a, b); }

// column-major access, 1-based (Fortran)
#define AT(p, ld, i, j) (p)[((i) - 1) + static_cast<long>((j) - 1) * (ld)]

// ------------------------------------------------------------------------------------------- BLAS
double dnrm2(int n, const double* x, int incx) {
    if (n < 1) return 0.0;
    if (n == 1) return std::fabs(x[0]);
    if (incx < 0) x += static_cast<long>(-incx) * (1 - n);   // OpenBLAS interface
    double scale = 0.0, ssq = 1.0;
    for (int i = 0; i < n; ++i) {
        const double v = x[static_cast<long>(i) * incx];
        if (v == 0.0) continue;
        const double a = std::fabs(v);
        if (scale >= a) {
            const double t = a / scale;
            ssq = std::fma(t, t, ssq);
        } else {
            double t = scale / a;
            t = t * t;
            ssq = 1.0 + ssq * t;
            scale = a;
        }
    }
    return scale * std::sqrt(ssq);
}

void dscal(int n, double alpha, double* x, int incx) {
    if (n < 1 || incx < 1 || alpha == 1.0) return;
    for (int i = 0; i < n; ++i) x[static_cast<long>(i) * incx] *= alpha;
}

// y = alpha * A^T x (beta = 0); A m x n, lda
void dgemv_t(int m, int n, double alpha, const double* a, int lda, const double* x, int incx, double* y) {
    if (m == 0 || n == 0) return;
    for (int j = 0; j < n; ++j) y[j] = 0.0;      // dscal_k with beta = 0 (zero path)
    if (alpha == 0.0) return;
    for (int j = 0; j < n; ++j) {
        const double* col = a + static_cast<long>(j) * lda;
        double acc;
        if (incx == 1) {
            double lane[8] = {0, 0, 0, 0, 0, 0, 0, 0};
            const int m8 = m & ~7;
            for (int i = 0; i < m8; ++i) lane[i & 7] = std::fma(col[i], x[i], lane[i & 7]);
            acc = (lane[0] + lane[1] + (lane[2] + lane[3])) + ((lane[4] + lane[5]) + (lane[6] + lane[7]));
            for (int i = m8; i < m; ++i) acc = std::fma(col[i], x[i], acc);
        } else {
            acc = 0.0;
            for (int i = 0; i < m; ++i) acc = std::fma(col[i], x[static_cast<long>(i) * incx], acc);
        }
        y[j] = std::fma(alpha, acc, y[j]);
    }
}

// y = alpha * A x (beta = 0)
void dgemv_n(int m, int n, double alpha, const double* a, int lda, const double* x, int incx, double* y) {
    if (m == 0 || n == 0) return;
    for (int i = 0; i < m; ++i) y[i] = 0.0;
    if (alpha == 0.0) return;
    for (int j = 0; j < n; ++j) {
        const double t = alpha * x[static_cast<long>(j) * incx];
        const double* col = a + static_cast<long>(j) * lda;
        for (int i = 0; i < m; ++i) y[i] = std::fma(t, col[i], y[i]);
    }
}

// A += alpha x y^T
void dger(int m, int n, double alpha, const double* x, int incx, const double* y, int incy, double* a, int lda) {
    if (m == 0 || n == 0 || alpha == 0.0) return;
    for (int j = 0; j < n; ++j) {
        const double da = y[static_cast<long>(j) * incy] * alpha;
        if (da == 0.0) continue;
        double* col = a + static_cast<long>(j) * lda;
        for (int i = 0; i < m; ++i) col[i] = std::fma(x[static_cast<long>(i) * incx], da, col[i]);
    }
}

void drot(int n, double* x, int incx, double* y, int incy, double c, double s) {
    if (n <= 0) return;
    const double ns = 0.0 - s;
    for (int i = 0; i < n; ++i) {
        double& xi = x[static_cast<long>(i) * incx];
        double& yi = y[static_cast<long>(i) * incy];
        const double xv = xi, yv = yi;
        xi = std::fma(s, yv, c * xv);
        yi = std::fma(ns, xv, c * yv);
    }
}

void dswap(int n, double* x, int incx, double* y, int incy) {
    for (int i = 0; i < n; ++i) {
        const double t = x[static_cast<long>(i) * incx];
        x[static_cast<long>(i) * incx] = y[static_cast<long>(i) * incy];
        y[static_cast<long>(i) * incy] = t;
    }
}

// C = alpha A^T B (beta = 0); A k x m (lda), B k x n (ldb), C m x n (ldc)
void dgemm_tn(int m, int n, int k, double alpha, const double* a, int lda, const double* b, int ldb, double* c,
              int ldc) {
    for (int j = 0; j < n; ++j)
        for (int i = 0; i < m; ++i) {
            double acc = 0.0;
            for (int l = 0; l < k; ++l)
                acc = std::fma(a[l + static_cast<long>(i) * lda], b[l + static_cast<long>(j) * ldb], acc);
            c[i + static_cast<long>(j) * ldc] = std::fma(alpha, acc, 0.0);
        }
}

// ------------------------------------------------------------------------------------------- LAPACK
double dlapy2(double x, double y) {
    const bool xn = std::isnan(x), yn = std::isnan(y);
    double r = 0.0;
    if (xn) r = x;
    if (yn) r = y;
    if (!(xn || yn)) {
        const double xa = std::fabs(x), ya = std::fabs(y);
        const double w = fmaxnm(xa, ya), z = fminnm(xa, ya);
        if (z == 0.0 || w > HUGEV) {
            r = w;
        } else {
            const double q = z / w;
            r = w * std::sqrt(std::fma(q, q, 1.0));
        }
    }
    return r;
}

void dlartg(double f, double g, double* c, double* s, double* r) {
    const double f1 = std::fabs(f), g1 = std::fabs(g);
    if (g == 0.0) {
        *c = 1.0;
        *s = 0.0;
        *r = f;
    } else if (f == 0.0) {
        *c = 0.0;
        *s = std::copysign(1.0, g);
        *r = g1;
    } else if (f1 > RTMIN && f1 < RTMAX && g1 > RTMIN && g1 < RTMAX) {
        const double d = std::sqrt(std::fma(f, f, g * g));
        *c = f1 / d;
        *r = std::copysign(d, f);
        *s = g / *r;
    } else {
        const double u = fminnm(fmaxnm(fmaxnm(g1, SFMIN), f1), SAFMAX_LA);
        const double fs = f / u, gs = g / u;
        const double d = std::sqrt(std::fma(fs, fs, gs * gs));
        *c = std::fabs(fs) / d;
        double rr = std::copysign(d, f);
        *s = gs / rr;
        *r = rr * u;
    }
}

void dlas2(double f, double g, double h, double* ssmin, double* ssmax) {
    const double fa = std::fabs(f), ga = std::fabs(g), ha = std::fabs(h);
    const double fhmn = fminnm(fa, ha), fhmx = fmaxnm(fa, ha);
    if (fhmn == 0.0) {
        *ssmin = 0.0;
        if (fhmx == 0.0) {
            *ssmax = ga;
        } else {
            const double mx = fmaxnm(ga, fhmx), mn = fminnm(ga, fhmx);
            const double q = mn / mx;
            *ssmax = std::sqrt(std::fma(q, q, 1.0)) * mx;
        }
    } else if (ga < fhmx) {
        const double as = fhmn / fhmx + 1.0;
        const double at = (fhmx - fhmn) / fhmx;
        const double q = ga / fhmx;
        const double c = 2.0 / (std::sqrt(std::fma(q, q, as * as)) + std::sqrt(std::fma(q, q, at * at)));
        *ssmin = fhmn * c;
        *ssmax = fhmx / c;
    } else {
        const double au = fhmx / ga;
        if (au == 0.0) {
            *ssmin = (fhmn * fhmx) / ga;
            *ssmax = ga;
        } else {
            const double as = fhmn / fhmx + 1.0;
            const double at = (fhmx - fhmn) / fhmx;
            const double p = as * au, q = at * au;
            const double c = 1.0 / (std::sqrt(std::fma(p, p, 1.0)) + std::sqrt(std::fma(q, q, 1.0)));
            double mn = (fhmn * c) * au;
            mn = mn + mn;
            *ssmin = mn;
            *ssmax = ga / (c + c);
        }
    }
}

void dlasv2(double f, double g, double h, double* ssmin, double* ssmax, double* snr, double* csr, double* snl,
            double* csl) {
    double ft = f, fa = std::fabs(ft), ht = h, ha = std::fabs(h);
    int pmax = 1;
    const bool swap = ha > fa;
    if (swap) {
        pmax = 3;
        std::swap(ft, ht);
        std::swap(fa, ha);
    }
    const double gt = g, ga = std::fabs(gt);
    double clt, crt, slt, srt, smin, smax;
    if (ga == 0.0) {
        smin = ha;
        smax = fa;
        clt = 1.0;
        crt = 1.0;
        slt = 0.0;
        srt = 0.0;
    } else {
        bool gasmal = true;
        if (ga > fa) {
            pmax = 2;
            if ((fa / ga) < EPS_E) {
                gasmal = false;
                smax = ga;
                smin = ha > 1.0 ? fa / (ga / ha) : (fa / ga) * ha;
                clt = 1.0;
                slt = ht / gt;
                srt = 1.0;
                crt = ft / gt;
            }
        }
        if (gasmal) {
            const double d = fa - ha;
            const double l = (d == fa) ? 1.0 : d / fa;
            const double m = gt / ft;
            double t = 2.0 - l;
            const double mm = m * m;
            const double s = std::sqrt(std::fma(t, t, mm));
            const double r = (l == 0.0) ? std::fabs(m) : std::sqrt(std::fma(l, l, mm));
            const double a = (s + r) * 0.5;
            smin = ha / a;
            smax = fa * a;
            if (mm == 0.0) {
                if (l == 0.0)
                    t = sign(2.0, ft) * sign(1.0, gt);
                else
                    t = gt / sign(d, ft) + m / t;
            } else {
                t = (m / (s + t) + m / (r + l)) * (1.0 + a);
            }
            const double ll = std::sqrt(std::fma(t, t, 4.0));
            crt = 2.0 / ll;
            srt = t / ll;
            clt = std::fma(srt, m, crt) / a;
            slt = ((ht / ft) * srt) / a;
        }
    }
    if (swap) {
        *csl = srt;
        *snl = crt;
        *csr = slt;
        *snr = clt;
    } else {
        *csl = clt;
        *snl = slt;
        *csr = crt;
        *snr = srt;
    }
    double tsign = 1.0;
    if (pmax == 1) tsign = sign(1.0, *csr) * sign(1.0, *csl) * sign(1.0, f);
    if (pmax == 2) tsign = sign(1.0, *snr) * sign(1.0, *csl) * sign(1.0, g);
    if (pmax == 3) tsign = sign(1.0, *snr) * sign(1.0, *snl) * sign(1.0, h);
    *ssmax = sign(smax, tsign);
    *ssmin = sign(smin, tsign * sign(1.0, f) * sign(1.0, h));
}

// dlasr('L', 'V', direct, m, n, c, s, A, lda)
void dlasr_lv(bool forward, int m, int n, const double* c, const double* s, double* a, int lda) {
    if (m <= 0 || n <= 0) return;
    auto rot = [&](int j) {
        const double ct = c[j - 1], st = s[j - 1];
        if (ct != 1.0 || st != 0.0) {
            for (int i = 1; i <= n; ++i) {
                const double temp = AT(a, lda, j + 1, i);
                const double aj = AT(a, lda, j, i);
                AT(a, lda, j + 1, i) = std::fma(ct, temp, -(st * aj));
                AT(a, lda, j, i) = std::fma(st, temp, ct * aj);
            }
        }
    };
    if (forward)
        for (int j = 1; j <= m - 1; ++j) rot(j);
    else
        for (int j = m - 1; j >= 1; --j) rot(j);
}

// dlascl('G', 0, 0, cfrom, cto, m, n, A, lda)
void dlascl(double cfrom, double cto, int m, int n, double* a, int lda) {
    const double smlnum = SFMIN, bignum = 1.0 / smlnum;
    double cfromc = cfrom, ctoc = cto;
    bool done = false;
    while (!done) {
        const double cfrom1 = cfromc * smlnum;
        double mul, cto1;
        if (cfrom1 == cfromc) {
            mul = ctoc / cfromc;
            done = true;
            cto1 = ctoc;
        } else {
            cto1 = ctoc / bignum;
            if (cto1 == ctoc) {
                mul = ctoc;
                done = true;
                cfromc = 1.0;
            } else if (std::fabs(cfrom1) > std::fabs(ctoc) && ctoc != 0.0) {
                mul = smlnum;
                done = false;
                cfromc = cfrom1;
            } else if (std::fabs(cto1) > std::fabs(cfromc)) {
                mul = bignum;
                done = false;
                ctoc = cto1;
            } else {
                mul = ctoc / cfromc;
                done = true;
                if (mul == 1.0) return;
            }
        }
        for (int j = 1; j <= n; ++j)
            for (int i = 1; i <= m; ++i) AT(a, lda, i, j) = AT(a, lda, i, j) * mul;
    }
}

void dlarfg(int n, double* alpha, double* x, int incx, double* tau) {
    if (n <= 1) {
        *tau = 0.0;
        return;
    }
    double xnorm = dnrm2(n - 1, x, incx);
    if (xnorm == 0.0) {
        *tau = 0.0;
        return;
    }
    double beta = -sign(dlapy2(*alpha, xnorm), *alpha);
    const double safmin = SFMIN / EPS_E;
    int knt = 0;
    if (std::fabs(beta) < safmin) {
        const double rsafmn = 1.0 / safmin;
        do {
            ++knt;
            dscal(n - 1, rsafmn, x, incx);
            beta = beta * rsafmn;
            *alpha = *alpha * rsafmn;
        } while (std::fabs(beta) < safmin && knt < 20);
        xnorm = dnrm2(n - 1, x, incx);
        beta = -sign(dlapy2(*alpha, xnorm), *alpha);
    }
    *tau = (beta - *alpha) / beta;
    dscal(n - 1, 1.0 / (*alpha - beta), x, incx);
    for (int j = 1; j <= knt; ++j) beta = beta * safmin;
    *alpha = beta;
}

int iladlc(int m, int n, const double* a, int lda) {
    if (n == 0) return n;
    if (AT(a, lda, 1, n) != 0.0 || AT(a, lda, m, n) != 0.0) return n;
    for (int c = n; c >= 1; --c)
        for (int i = 1; i <= m; ++i)
            if (AT(a, lda, i, c) != 0.0) return c;
    return 0;
}

int iladlr(int m, int n, const double* a, int lda) {
    if (m == 0) return m;
    if (AT(a, lda, m, 1) != 0.0 || AT(a, lda, m, n) != 0.0) return m;
    int r = 0;
    for (int j = 1; j <= n; ++j) {
        int i = m;
        while (AT(a, lda, (i > 1 ? i : 1), j) == 0.0 && i >= 1) --i;
        if (i > r) r = i;
    }
    return r;
}

void dlarf(bool left, int m, int n, const double* v, int incv, double tau, double* c, int ldc, double* work) {
    int lastv = 0, lastc = 0;
    if (tau != 0.0) {
        lastv = left ? m : n;
        long i = incv > 0 ? 1 + static_cast<long>(lastv - 1) * incv : 1;
        while (lastv > 0 && v[i - 1] == 0.0) {
            --lastv;
            i -= incv;
        }
        lastc = left ? iladlc(lastv, n, c, ldc) : iladlr(m, lastv, c, ldc);
    }
    if (lastv <= 0) return;
    if (left) {
        dgemv_t(lastv, lastc, 1.0, c, ldc, v, incv, work);
        dger(lastv, lastc, -tau, v, incv, work, 1, c, ldc);
    } else {
        dgemv_n(lastc, lastv, 1.0, c, ldc, v, incv, work);
        dger(lastc, lastv, -tau, work, 1, v, incv, c, ldc);
    }
}

void dgeqr2(int m, int n, double* a, int lda, double* tau, double* work) {
    const int k = m < n ? m : n;
    for (int i = 1; i <= k; ++i) {
        dlarfg(m - i + 1, &AT(a, lda, i, i), &AT(a, lda, (i + 1 < m ? i + 1 : m), i), 1, &tau[i - 1]);
        if (i < n) {
            const double aii = AT(a, lda, i, i);
            AT(a, lda, i, i) = 1.0;
            dlarf(true, m - i + 1, n - i, &AT(a, lda, i, i), 1, tau[i - 1], &AT(a, lda, i, i + 1), lda, work);
            AT(a, lda, i, i) = aii;
        }
    }
}

// dorm2r('L', 'T', m, n, k, ...)
void dorm2r_lt(int m, int n, int k, double* a, int lda, const double* tau, double* c, int ldc, double* work) {
    if (m == 0 || n == 0 || k == 0) return;
    for (int i = 1; i <= k; ++i) {
        const double aii = AT(a, lda, i, i);
        AT(a, lda, i, i) = 1.0;
        dlarf(true, m - i + 1, n, &AT(a, lda, i, i), 1, tau[i - 1], &AT(c, ldc, i, 1), ldc, work);
        AT(a, lda, i, i) = aii;
    }
}

// dorml2('L', 'T', m, n, k, ...): i = k .. 1
void dorml2_lt(int m, int n, int k, double* a, int lda, const double* tau, double* c, int ldc, double* work) {
    if (m == 0 || n == 0 || k == 0) return;
    for (int i = k; i >= 1; --i) {
        const double aii = AT(a, lda, i, i);
        AT(a, lda, i, i) = 1.0;
        dlarf(true, m - i + 1, n, &AT(a, lda, i, i), lda, tau[i - 1], &AT(c, ldc, i, 1), ldc, work);
        AT(a, lda, i, i) = aii;
    }
}

// dgebd2 for m >= n
void dgebd2(int m, int n, double* a, int lda, double* d, double* e, double* tauq, double* taup, double* work) {
    for (int i = 1; i <= n; ++i) {
        dlarfg(m - i + 1, &AT(a, lda, i, i), &AT(a, lda, (i + 1 < m ? i + 1 : m), i), 1, &tauq[i - 1]);
        d[i - 1] = AT(a, lda, i, i);
        AT(a, lda, i, i) = 1.0;
        if (i < n) dlarf(true, m - i + 1, n - i, &AT(a, lda, i, i), 1, tauq[i - 1], &AT(a, lda, i, i + 1), lda, work);
        AT(a, lda, i, i) = d[i - 1];
        if (i < n) {
            dlarfg(n - i, &AT(a, lda, i, i + 1), &AT(a, lda, i, (i + 2 < n ? i + 2 : n)), lda, &taup[i - 1]);
            e[i - 1] = AT(a, lda, i, i + 1);
            AT(a, lda, i, i + 1) = 1.0;
            dlarf(false, m - i, n - i, &AT(a, lda, i, i + 1), lda, taup[i - 1], &AT(a, lda, i + 1, i + 1), lda,
                  work);
            AT(a, lda, i, i + 1) = e[i - 1];
        } else {
            taup[i - 1] = 0.0;
        }
    }
}

// dbdsqr('U', n, ncvt, nru = 0, ncc, d, e, vt, ldvt, -, -, c, ldc, work); returns info
int dbdsqr_u(int n, int ncvt, int ncc, double* d, double* e, double* vt, int ldvt, double* c, int ldc,
             double* work) {
    if (n == 0) return 0;
    const double HNDRTH = 0.01, TEN = 10.0, HNDRD = 100.0, MEIGTH = -0.125;
    const int MAXITR = 6;
    int info = 0;
    if (n > 1) {
        const int nm1 = n - 1, nm12 = nm1 + nm1, nm13 = nm12 + nm1;
        int idir = 0;
        const double eps = EPS_E, unfl = SFMIN;
        const double tolmul = std::fmax(TEN, std::fmin(HNDRD, std::pow(eps, MEIGTH)));
        const double tol = tolmul * eps;
        double smax = 0.0;
        for (int i = 1; i <= n; ++i) smax = std::fmax(smax, std::fabs(d[i - 1]));
        for (int i = 1; i <= n - 1; ++i) smax = std::fmax(smax, std::fabs(e[i - 1]));
        double smin = 0.0, thresh;
        if (tol >= 0.0) {
            double sminoa = std::fabs(d[0]);
            if (sminoa != 0.0) {
                double mu = sminoa;
                for (int i = 2; i <= n; ++i) {
                    mu = std::fabs(d[i - 1]) * (mu / (mu + std::fabs(e[i - 2])));
                    sminoa = std::fmin(sminoa, mu);
                    if (sminoa == 0.0) break;
                }
            }
            sminoa = sminoa / std::sqrt(static_cast<double>(n));
            const double dn = static_cast<double>(n);
            thresh = std::fmax(tol * sminoa, static_cast<double>(MAXITR) * (dn * (dn * unfl)));
        } else {
            const double dn = static_cast<double>(n);
            thresh = std::fmax(std::fabs(tol) * smax, static_cast<double>(MAXITR) * (dn * (dn * unfl)));
        }
        const int maxitdivn = MAXITR * n;
        int iterdivn = 0, iter = -1, oldll = -1, oldm = -1, m = n;
        int ll = 0;
        double* D = d - 1;   // 1-based
        double* E = e - 1;
        double* W = work - 1;
        while (true) {   // label 60
            if (m <= 1) break;
            if (iter >= n) {
                iter -= n;
                ++iterdivn;
                if (iterdivn >= maxitdivn) {
                    info = 0;
                    for (int i = 1; i <= n - 1; ++i)
                        if (E[i] != 0.0) ++info;
                    return info;
                }
            }
            if (tol < 0.0 && std::fabs(D[m]) <= thresh) D[m] = 0.0;
            smax = std::fabs(D[m]);
            bool split = false;
            for (int lll = 1; lll <= m - 1; ++lll) {
                ll = m - lll;
                const double abss = std::fabs(D[ll]), abse = std::fabs(E[ll]);
                if (tol < 0.0 && abss <= thresh) D[ll] = 0.0;
                if (abse <= thresh) {
                    split = true;
                    break;
                }
                smax = std::fmax(std::fmax(smax, abss), abse);
            }
            if (split) {
                E[ll] = 0.0;
                if (ll == m - 1) {
                    m = m - 1;
                    continue;
                }
            } else {
                ll = 0;
            }
            ll = ll + 1;   // label 90
            if (ll == m - 1) {
                double sigmn, sigmx, sinr, cosr, sinl, cosl;
                dlasv2(D[m - 1], E[m - 1], D[m], &sigmn, &sigmx, &sinr, &cosr, &sinl, &cosl);
                D[m - 1] = sigmx;
                E[m - 1] = 0.0;
                D[m] = sigmn;
                if (ncvt > 0) drot(ncvt, &AT(vt, ldvt, m - 1, 1), ldvt, &AT(vt, ldvt, m, 1), ldvt, cosr, sinr);
                if (ncc > 0) drot(ncc, &AT(c, ldc, m - 1, 1), ldc, &AT(c, ldc, m, 1), ldc, cosl, sinl);
                m = m - 2;
                continue;
            }
            if (ll > oldm || m < oldll) idir = (std::fabs(D[ll]) >= std::fabs(D[m])) ? 1 : 2;
            bool restart = false;
            if (idir == 1) {
                if (std::fabs(E[m - 1]) <= std::fabs(tol) * std::fabs(D[m]) ||
                    (tol < 0.0 && std::fabs(E[m - 1]) <= thresh)) {
                    E[m - 1] = 0.0;
                    continue;
                }
                if (tol >= 0.0) {
                    double mu = std::fabs(D[ll]);
                    smin = mu;
                    for (int lll = ll; lll <= m - 1; ++lll) {
                        if (std::fabs(E[lll]) <= tol * mu) {
                            E[lll] = 0.0;
                            restart = true;
                            break;
                        }
                        mu = std::fabs(D[lll + 1]) * (mu / (mu + std::fabs(E[lll])));
                        smin = std::fmin(smin, mu);
                    }
                }
            } else {
                if (std::fabs(E[ll]) <= std::fabs(tol) * std::fabs(D[ll]) ||
                    (tol < 0.0 && std::fabs(E[ll]) <= thresh)) {
                    E[ll] = 0.0;
                    continue;
                }
                if (tol >= 0.0) {
                    double mu = std::fabs(D[m]);
                    smin = mu;
                    for (int lll = m - 1; lll >= ll; --lll) {
                        if (std::fabs(E[lll]) <= tol * mu) {
                            E[lll] = 0.0;
                            restart = true;
                            break;
                        }
                        mu = std::fabs(D[lll]) * (mu / (mu + std::fabs(E[lll])));
                        smin = std::fmin(smin, mu);
                    }
                }
            }
            if (restart) continue;
            oldll = ll;
            oldm = m;
            double shift, r;
            if (tol >= 0.0 && static_cast<double>(n) * tol * (smin / smax) <= std::fmax(eps, HNDRTH * tol)) {
                shift = 0.0;
            } else {
                double sll;
                if (idir == 1) {
                    sll = std::fabs(D[ll]);
                    dlas2(D[m - 1], E[m - 1], D[m], &shift, &r);
                } else {
                    sll = std::fabs(D[m]);
                    dlas2(D[ll], E[ll], D[ll + 1], &shift, &r);
                }
                if (sll > 0.0) {
                    const double q = shift / sll;
                    if (q * q < eps) shift = 0.0;
                }
            }
            iter = iter + m - ll;
            if (shift == 0.0) {
                if (idir == 1) {
                    double cs = 1.0, oldcs = 1.0, sn = 0.0, oldsn = 0.0;
                    for (int i = ll; i <= m - 1; ++i) {
                        dlartg(D[i] * cs, E[i], &cs, &sn, &r);
                        if (i > ll) E[i - 1] = oldsn * r;
                        dlartg(oldcs * r, D[i + 1] * sn, &oldcs, &oldsn, &D[i]);
                        W[i - ll + 1] = cs;
                        W[i - ll + 1 + nm1] = sn;
                        W[i - ll + 1 + nm12] = oldcs;
                        W[i - ll + 1 + nm13] = oldsn;
                    }
                    const double h = D[m] * cs;
                    D[m] = h * oldcs;
                    E[m - 1] = h * oldsn;
                    if (ncvt > 0) dlasr_lv(true, m - ll + 1, ncvt, &W[1], &W[n], &AT(vt, ldvt, ll, 1), ldvt);
                    if (ncc > 0) dlasr_lv(true, m - ll + 1, ncc, &W[nm12 + 1], &W[nm13 + 1], &AT(c, ldc, ll, 1), ldc);
                    if (std::fabs(E[m - 1]) <= thresh) E[m - 1] = 0.0;
                } else {
                    double cs = 1.0, oldcs = 1.0, sn = 0.0, oldsn = 0.0;
                    for (int i = m; i >= ll + 1; --i) {
                        dlartg(D[i] * cs, E[i - 1], &cs, &sn, &r);
                        if (i < m) E[i] = oldsn * r;
                        dlartg(oldcs * r, D[i - 1] * sn, &oldcs, &oldsn, &D[i]);
                        W[i - ll] = cs;
                        W[i - ll + nm1] = -sn;
                        W[i - ll + nm12] = oldcs;
                        W[i - ll + nm13] = -oldsn;
                    }
                    const double h = D[ll] * cs;
                    D[ll] = h * oldcs;
                    E[ll] = h * oldsn;
                    if (ncvt > 0)
                        dlasr_lv(false, m - ll + 1, ncvt, &W[nm12 + 1], &W[nm13 + 1], &AT(vt, ldvt, ll, 1), ldvt);
                    if (ncc > 0) dlasr_lv(false, m - ll + 1, ncc, &W[1], &W[n], &AT(c, ldc, ll, 1), ldc);
                    if (std::fabs(E[ll]) <= thresh) E[ll] = 0.0;
                }
            } else {
                if (idir == 1) {
                    double f = (std::fabs(D[ll]) - shift) * (sign(1.0, D[ll]) + shift / D[ll]);
                    double g = E[ll];
                    double cosr, sinr, cosl, sinl;
                    for (int i = ll; i <= m - 1; ++i) {
                        dlartg(f, g, &cosr, &sinr, &r);
                        if (i > ll) E[i - 1] = r;
                        f = std::fma(cosr, D[i], sinr * E[i]);
                        E[i] = std::fma(cosr, E[i], -(sinr * D[i]));
                        g = sinr * D[i + 1];
                        D[i + 1] = cosr * D[i + 1];
                        dlartg(f, g, &cosl, &sinl, &r);
                        D[i] = r;
                        f = std::fma(cosl, E[i], sinl * D[i + 1]);
                        D[i + 1] = std::fma(cosl, D[i + 1], -(sinl * E[i]));
                        if (i < m - 1) {
                            g = sinl * E[i + 1];
                            E[i + 1] = cosl * E[i + 1];
                        }
                        W[i - ll + 1] = cosr;
                        W[i - ll + 1 + nm1] = sinr;
                        W[i - ll + 1 + nm12] = cosl;
                        W[i - ll + 1 + nm13] = sinl;
                    }
                    E[m - 1] = f;
                    if (ncvt > 0) dlasr_lv(true, m - ll + 1, ncvt, &W[1], &W[n], &AT(vt, ldvt, ll, 1), ldvt);
                    if (ncc > 0) dlasr_lv(true, m - ll + 1, ncc, &W[nm12 + 1], &W[nm13 + 1], &AT(c, ldc, ll, 1), ldc);
                    if (std::fabs(E[m - 1]) <= thresh) E[m - 1] = 0.0;
                } else {
                    double f = (std::fabs(D[m]) - shift) * (sign(1.0, D[m]) + shift / D[m]);
                    double g = E[m - 1];
                    double cosr, sinr, cosl, sinl;
                    for (int i = m; i >= ll + 1; --i) {
                        dlartg(f, g, &cosr, &sinr, &r);
                        if (i < m) E[i] = r;
                        f = std::fma(cosr, D[i], sinr * E[i - 1]);
                        E[i - 1] = std::fma(cosr, E[i - 1], -(sinr * D[i]));
                        g = sinr * D[i - 1];
                        D[i - 1] = cosr * D[i - 1];
                        dlartg(f, g, &cosl, &sinl, &r);
                        D[i] = r;
                        f = std::fma(cosl, E[i - 1], sinl * D[i - 1]);
                        D[i - 1] = std::fma(cosl, D[i - 1], -(sinl * E[i - 1]));
                        if (i > ll + 1) {
                            g = sinl * E[i - 2];
                            E[i - 2] = cosl * E[i - 2];
                        }
                        W[i - ll] = cosr;
                        W[i - ll + nm1] = -sinr;
                        W[i - ll + nm12] = cosl;
                        W[i - ll + nm13] = -sinl;
                    }
                    E[ll] = f;
                    if (std::fabs(E[ll]) <= thresh) E[ll] = 0.0;
                    if (ncvt > 0)
                        dlasr_lv(false, m - ll + 1, ncvt, &W[nm12 + 1], &W[nm13 + 1], &AT(vt, ldvt, ll, 1), ldvt);
                    if (ncc > 0) dlasr_lv(false, m - ll + 1, ncc, &W[1], &W[n], &AT(c, ldc, ll, 1), ldc);
                }
            }
        }
    }
    // label 160: make the singular values positive, sort decreasing
    for (int i = 1; i <= n; ++i) {
        if (d[i - 1] < 0.0) {
            d[i - 1] = -d[i - 1];
            if (ncvt > 0) dscal(ncvt, -1.0, &AT(vt, ldvt, i, 1), ldvt);
        }
    }
    for (int i = 1; i <= n - 1; ++i) {
        int isub = 1;
        double smin = d[0];
        for (int j = 2; j <= n + 1 - i; ++j) {
            if (d[j - 1] <= smin) {
                isub = j;
                smin = d[j - 1];
            }
        }
        if (isub != n + 1 - i) {
            d[isub - 1] = d[n - i];
            d[n - i] = smin;
            if (ncvt > 0) dswap(ncvt, &AT(vt, ldvt, isub, 1), ldvt, &AT(vt, ldvt, n + 1 - i, 1), ldvt);
            if (ncc > 0) dswap(ncc, &AT(c, ldc, isub, 1), ldc, &AT(c, ldc, n + 1 - i, 1), ldc);
        }
    }
    return info;
}

// dlasdq('U', sqre = 0, n, ncvt, nru = 0, ncc, ...)
int dlasdq_u(int n, int ncvt, int ncc, double* d, double* e, double* vt, int ldvt, double* c, int ldc,
             double* work) {
    if (n == 0) return 0;
    const int info = dbdsqr_u(n, ncvt, ncc, d, e, vt, ldvt, c, ldc, work);
    for (int i = 1; i <= n; ++i) {
        int isub = i;
        double smin = d[i - 1];
        for (int j = i + 1; j <= n; ++j)
            if (d[j - 1] < smin) {
                isub = j;
                smin = d[j - 1];
            }
        if (isub != i) {
            d[isub - 1] = d[i - 1];
            d[i - 1] = smin;
            if (ncvt > 0) dswap(ncvt, &AT(vt, ldvt, isub, 1), ldvt, &AT(vt, ldvt, i, 1), ldvt);
            if (ncc > 0) dswap(ncc, &AT(c, ldc, isub, 1), ldc, &AT(c, ldc, i, 1), ldc);
        }
    }
    return info;
}

int idamax(int n, const double* x) {
    int k = 1;
    double mx = std::fabs(x[0]);
    for (int i = 2; i <= n; ++i)
        if (std::fabs(x[i - 1]) > mx) {
            mx = std::fabs(x[i - 1]);
            k = i;
        }
    return k;
}

// dlalsd('U', smlsiz, n <= smlsiz, nrhs, d, e, b, ldb, rcond); returns info
int dlalsd_small(int n, int nrhs, double* d, double* e, double* b, int ldb, double rcond, int* rank) {
    const double eps = EPS_E;
    const double rcnd = (rcond <= 0.0 || rcond >= 1.0) ? eps : rcond;
    *rank = 0;
    if (n == 0) return 0;
    if (n == 1) {
        if (d[0] == 0.0) {
            for (int j = 1; j <= nrhs; ++j) AT(b, ldb, 1, j) = 0.0;
        } else {
            *rank = 1;
            dlascl(d[0], 1.0, 1, nrhs, b, ldb);
            d[0] = std::fabs(d[0]);
        }
        return 0;
    }
    // dlanst('M')
    double orgnrm = std::fabs(d[n - 1]);
    for (int i = 1; i <= n - 1; ++i) {
        double s = std::fabs(d[i - 1]);
        if (orgnrm < s || std::isnan(s)) orgnrm = s;
        s = std::fabs(e[i - 1]);
        if (orgnrm < s || std::isnan(s)) orgnrm = s;
    }
    if (orgnrm == 0.0) {
        for (int j = 1; j <= nrhs; ++j)
            for (int i = 1; i <= n; ++i) AT(b, ldb, i, j) = 0.0;
        return 0;
    }
    dlascl(orgnrm, 1.0, n, 1, d, n);
    dlascl(orgnrm, 1.0, n - 1, 1, e, n - 1);
    std::vector<double> work(static_cast<size_t>(n) * n + static_cast<size_t>(n) * nrhs + 4 * n + 8, 0.0);
    double* vt = work.data();
    double* nw = work.data() + static_cast<size_t>(n) * n;
    for (int j = 1; j <= n; ++j)
        for (int i = 1; i <= n; ++i) AT(vt, n, i, j) = (i == j) ? 1.0 : 0.0;
    const int info = dlasdq_u(n, n, nrhs, d, e, vt, n, b, ldb, nw);
    if (info != 0) return info;
    const double tol = rcnd * std::fabs(d[idamax(n, d) - 1]);
    for (int i = 1; i <= n; ++i) {
        if (d[i - 1] <= tol) {
            for (int j = 1; j <= nrhs; ++j) AT(b, ldb, i, j) = 0.0;
        } else {
            dlascl(d[i - 1], 1.0, 1, nrhs, &AT(b, ldb, i, 1), ldb);
            ++*rank;
        }
    }
    dgemm_tn(n, nrhs, n, 1.0, vt, n, b, ldb, nw, n);
    for (int j = 1; j <= nrhs; ++j)
        for (int i = 1; i <= n; ++i) AT(b, ldb, i, j) = AT(nw, n, i, j);
    dlascl(1.0, orgnrm, n, 1, d, n);
    // dlasrt('D') of d: not needed for the solution
    dlascl(orgnrm, 1.0, n, nrhs, b, ldb);
    return 0;
}

// dgelsd for m >= n, n <= 25, nrhs = 1 (A m x n column-major lda = m, b of length m); x = b[0..n-1]
int dgelsd(int m, int n, double* a, double* b, double rcond, int* rank) {
    const int lda = m, ldb = m;
    const int minmn = n;
    const int mnthr = static_cast<int>(static_cast<float>(minmn) * 1.6f);
    const double eps = EPS_P, sfmin = SFMIN;
    const double smlnum = sfmin / eps, bignum = 1.0 / smlnum;
    // dlange('M')
    double anrm = 0.0;
    for (int j = 1; j <= n; ++j)
        for (int i = 1; i <= m; ++i) {
            const double t = std::fabs(AT(a, lda, i, j));
            if (anrm < t || std::isnan(t)) anrm = t;
        }
    int iascl = 0;
    if (anrm > 0.0 && anrm < smlnum) {
        dlascl(anrm, smlnum, m, n, a, lda);
        iascl = 1;
    } else if (anrm > bignum) {
        dlascl(anrm, bignum, m, n, a, lda);
        iascl = 2;
    } else if (anrm == 0.0) {
        for (int i = 0; i < m; ++i) b[i] = 0.0;
        *rank = 0;
        return 0;
    }
    double bnrm = 0.0;
    for (int i = 1; i <= m; ++i) {
        const double t = std::fabs(b[i - 1]);
        if (bnrm < t || std::isnan(t)) bnrm = t;
    }
    int ibscl = 0;
    if (bnrm > 0.0 && bnrm < smlnum) {
        dlascl(bnrm, smlnum, m, 1, b, ldb);
        ibscl = 1;
    } else if (bnrm > bignum) {
        dlascl(bnrm, bignum, m, 1, b, ldb);
        ibscl = 2;
    }
    std::vector<double> work(static_cast<size_t>(8 * (m + n) + 64), 0.0);
    int mm = m;
    if (m >= mnthr) {
        mm = n;
        std::vector<double> tau(n);
        dgeqr2(m, n, a, lda, tau.data(), work.data());
        dorm2r_lt(m, 1, n, a, lda, tau.data(), b, ldb, work.data());
        // dlaset('L', n - 1, n - 1, 0, 0, A(2, 1))
        for (int j = 1; j <= n - 1; ++j)
            for (int i = j + 1; i <= n; ++i) AT(a, lda, i, j) = 0.0;
    }
    std::vector<double> d(n), e(n), tauq(n), taup(n);
    dgebd2(mm, n, a, lda, d.data(), e.data(), tauq.data(), taup.data(), work.data());
    dorm2r_lt(mm, 1, n, a, lda, tauq.data(), b, ldb, work.data());       // dormbr Q, L, T (nq >= k)
    const int info = dlalsd_small(n, 1, d.data(), e.data(), b, ldb, rcond, rank);
    if (info != 0) return info;
    if (n > 1) dorml2_lt(n - 1, 1, n - 1, &AT(a, lda, 1, 2), lda, taup.data(), &b[1], ldb, work.data());  // dormbr P
    if (iascl == 1)
        dlascl(anrm, smlnum, n, 1, b, ldb);
    else if (iascl == 2)
        dlascl(anrm, bignum, n, 1, b, ldb);
    if (ibscl == 1)
        dlascl(smlnum, bnrm, n, 1, b, ldb);
    else if (ibscl == 2)
        dlascl(bignum, bnrm, n, 1, b, ldb);
    return 0;
}

}  // namespace

bool polyfit(const double* x, const double* y, int m, int deg, double* coef) {
    const int order = deg + 1;
    if (m < 1 || deg < 0 || order > 8 || m > 64) return false;
    // np.vander(x, order): column j = x ** (order - 1 - j) by multiply.accumulate
    std::vector<double> v(static_cast<size_t>(m) * order);
    for (int i = 0; i < m; ++i) {
        double* row = &v[static_cast<size_t>(i) * order];
        row[order - 1] = 1.0;
        if (order > 1) {
            double p = x[i];
            row[order - 2] = p;
            for (int j = order - 3; j >= 0; --j) {
                p = p * x[i];
                row[j] = p;
            }
        }
    }
    double scale[8];
    for (int j = 0; j < order; ++j) {
        double s = v[j] * v[j];          // add.reduce over axis 0: row 0, then += row i
        for (int i = 1; i < m; ++i) s = s + v[static_cast<size_t>(i) * order + j] * v[static_cast<size_t>(i) * order + j];
        scale[j] = std::sqrt(s);
    }
    if (m < order) return false;         // not on the _fill_curve path (dgelsd m < n branch not ported)
    std::vector<double> a(static_cast<size_t>(m) * order), b(m);
    for (int j = 0; j < order; ++j)
        for (int i = 0; i < m; ++i) a[i + static_cast<size_t>(j) * m] = v[static_cast<size_t>(i) * order + j] / scale[j];
    for (int i = 0; i < m; ++i) b[i] = y[i];
    const double rcond = static_cast<double>(m) * 2.220446049250313e-16;
    int rank = 0;
    if (dgelsd(m, order, a.data(), b.data(), rcond, &rank) != 0) return false;
    for (int j = 0; j < order; ++j) coef[j] = b[j] / scale[j];
    return true;
}

double polyval(const double* p, int np, double x) {
    double y = 0.0;
    for (int i = 0; i < np; ++i) y = y * x + p[i];
    return y;
}

}  // namespace nvl5
