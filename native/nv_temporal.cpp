// nv_temporal - see nv_temporal.h.  Line-by-line port of nvfilter.py's v5 motion / temporal code.
// numpy reductions reproduced (checked bit for bit, sandbox/omo/wp_native_temporal/dev/probe_sum.py):
//   d.reshape(nby, blk, nbx, blk).sum((1, 3))  = out = +0; out = pw(row r) + out for each block row
//                                                 (pw = numpy's pairwise sum of the blk values of the
//                                                 row); for nbx == 1: pw(all blk*blk values) + 0
//   (d * d).sum((2, 3)) in _refine              = pw(all blk*blk values, row-major) + 0
// ("pw + out" is the operand order of the compiled reduce, observed with NaN payloads)
#include "nv_temporal.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <stdexcept>

namespace nvt {

using namespace nvn;

namespace {

const float ZERO = 0.0f;
const float NEG0 = bits(0x80000000u);
const float ONE = bits(0x3f800000u);
const float TWO = bits(0x40000000u);
const float HALF = bits(0x3f000000u);
const float MHALF = bits(0xbf000000u);
const float EPS6 = bits(0x358637bdu);      // 1e-6
const float EPS3 = bits(0x3a83126fu);      // 1e-3
const float NVAR = bits(0x3f23d70au);      // NEIGHBOUR_VAR 0.64
const double PYR_K = dbits(0x3ff448fc35b4dc86ull);   // 1.1 * 0.9 * 1.2806249
const double PYR_C[3] = {dbits(0x3ff0f5c28f5c28f6ull), 1.0, 1.0};   // (1.06, 1.0, 1.0)

// numpy FLOAT_pairwise_sum (loops_utils.h.src) over x[0], x[stride], ...
float pairwise(const float* a, long n, long stride) {
    if (n < 8) {
        // res = -0.0; res += x[i]: the compiled loop drops the -0.0 + x[0] (LLVM folds it), so the
        // first value enters unchanged (observed with signalling NaN payloads)
        if (n <= 0) return NEG0;
        float res = a[0];
        for (long i = 1; i < n; ++i) res = A(res, a[i * stride]);
        return res;
    } else if (n <= 128) {
        float r[8];
        for (int j = 0; j < 8; ++j) r[j] = a[j * stride];
        long i;
        for (i = 8; i < n - (n % 8); i += 8)
            for (int j = 0; j < 8; ++j) r[j] = A(r[j], a[(i + j) * stride]);
        // ((r0 + r1) + (r2 + r3)) + ((r4 + r5) + (r6 + r7)); the compiled code adds each pair as
        // (right, left) (NaN priority observed with two payloads, dev/probe_nan.py)
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

inline int clampi(long i, long n) { return (int)(i < 0 ? 0 : (i > n - 1 ? n - 1 : i)); }

// np.argmin over 9 values: first NaN, else first minimum
inline int argmin9(const float* v) {
    int k = 0;
    float best = v[0];
    if (isnanf_(best)) return 0;
    for (int i = 1; i < 9; ++i) {
        if (isnanf_(v[i])) return i;
        if (v[i] < best) { best = v[i]; k = i; }
    }
    return k;
}

}  // namespace

namespace detail {
// d.reshape(nby, blk, nbx, blk).sum((1, 3)) of a contiguous (nby*blk) x (nbx*blk) array: out = +0,
// then out = pw(block row r) + out for each of the blk rows (pw = numpy's pairwise sum of the blk
// values); with nbx == 1 the blocks are contiguous and numpy sums all blk*blk values at once:
// pw(block) + 0 (both observed bit for bit, dev/probe_sum2.py; operand order from NaN payloads)
void block_sum13(const float* d, int nby, int nbx, int blk, Plane& s) {
    const size_t Wp = (size_t)nbx * blk;
    for (int by = 0; by < nby; ++by) {
        float* srow = s.row(by);
        for (int bx = 0; bx < nbx; ++bx) {
            const float* b0 = d + (size_t)by * blk * Wp + (size_t)bx * blk;
            if (nbx == 1) {
                srow[bx] = A(pairwise(b0, (long)blk * blk, 1), ZERO);
            } else {
                float acc = ZERO;
                for (int r = 0; r < blk; ++r) acc = A(pairwise(b0 + (size_t)r * Wp, blk, 1), acc);
                srow[bx] = acc;
            }
        }
    }
}
}  // namespace detail

namespace {

// _search(cur, ref, rng, nby, nbx, blk)
void search(const Plane& cur, const Plane& ref, int rng, int nby, int nbx, int blk,
            std::vector<int>& vy, std::vector<int>& vx) {
    const int Hp = nby * blk, Wp = nbx * blk;
    const size_t nb = (size_t)nby * nbx;
    std::vector<float> best(nb, std::numeric_limits<float>::infinity());
    vy.assign(nb, 0);
    vx.assign(nb, 0);
    const double lam = 0.02 * blk * blk;
    std::vector<float> d((size_t)Hp * Wp);
    Plane s(nby, nbx);
    for (int dy = -rng; dy <= rng; ++dy) {
        for (int dx = -rng; dx <= rng; ++dx) {
            for (int y = 0; y < Hp; ++y) {
                const float* c = cur.row(y);
                const float* r = ref.row(clampi((long)y + dy, ref.H));
                float* dr = d.data() + (size_t)y * Wp;
                for (int x = 0; x < Wp; ++x) dr[x] = std::fabs(S(c[x], r[clampi((long)x + dx, ref.W)]));
            }
            detail::block_sum13(d.data(), nby, nbx, blk, s);
            Plane u = uniform(s, 3, NEAREST);
            const float pen = f32(lam * (double)(std::abs(dy) + std::abs(dx)));
            for (size_t i = 0; i < nb; ++i) {
                float v = A(u.d[i], pen);
                if (v < best[i]) { best[i] = v; vy[i] = dy; vx[i] = dx; }
            }
        }
    }
}

// _refine(cur, ref, vy, vx, blk) -> new (vy, vx) and the 3x3 cost cube cost[(i*3+j)][block]
void refine(const Plane& cur, const Plane& ref, std::vector<int>& vy, std::vector<int>& vx, int nby,
            int nbx, int blk, std::vector<float>* cost_out) {
    const size_t nb = (size_t)nby * nbx;
    std::vector<float> cost(9 * nb);
    std::vector<float> dd((size_t)blk * blk);
    for (int by = 0; by < nby; ++by) {
        for (int bx = 0; bx < nbx; ++bx) {
            const size_t bi = (size_t)by * nbx + bx;
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    const long oy = vy[bi] + i - 1, ox = vx[bi] + j - 1;
                    for (int r = 0; r < blk; ++r) {
                        const int y = by * blk + r;
                        const float* c = cur.row(y);
                        const float* rr = ref.row(clampi(y + oy, ref.H));
                        for (int q = 0; q < blk; ++q) {
                            const int x = bx * blk + q;
                            float dv = S(c[x], rr[clampi(x + ox, ref.W)]);
                            dd[(size_t)r * blk + q] = M(dv, dv);
                        }
                    }
                    cost[(size_t)(i * 3 + j) * nb + bi] = A(pairwise(dd.data(), (long)blk * blk, 1), ZERO);
                }
            }
        }
    }
    for (size_t bi = 0; bi < nb; ++bi) {
        float v[9];
        for (int k = 0; k < 9; ++k) v[k] = cost[(size_t)k * nb + bi];
        int k = argmin9(v);
        vy[bi] += k / 3 - 1;
        vx[bi] += k % 3 - 1;
    }
    if (cost_out) cost_out->swap(cost);
}

}  // namespace

void detail::refine_test(const Plane& cur, const Plane& ref, Plane& vy, Plane& vx, int blk, std::vector<Plane>& cost) {
    const int nby = vy.H, nbx = vy.W;
    std::vector<int> iy(vy.size()), ix(vx.size());
    for (size_t i = 0; i < iy.size(); ++i) { iy[i] = (int)vy.d[i]; ix[i] = (int)vx.d[i]; }
    std::vector<float> c;
    refine(cur, ref, iy, ix, nby, nbx, blk, &c);
    cost.assign(9, Plane(nby, nbx));
    for (int k = 0; k < 9; ++k)
        for (size_t i = 0; i < (size_t)nby * nbx; ++i) cost[k].d[i] = c[(size_t)k * nby * nbx + i];
    for (size_t i = 0; i < iy.size(); ++i) { vy.d[i] = (float)iy[i]; vx.d[i] = (float)ix[i]; }
}

// ------------------------------------------------------------------------------ estimate_flow_v5
Flow estimate_flow_v5(const Plane& ref, const Plane& cur, int bs, int search_) {
    const int nby = cur.H / bs, nbx = cur.W / bs;
    // Python fails here too (_refine: max of an empty array -> ValueError)
    if (nby <= 0 || nbx <= 0) throw std::invalid_argument("estimate_flow_v5: frame smaller than one block");
    const size_t nb = (size_t)nby * nbx;
    Plane a0 = gauss1(cur), b0 = gauss1(ref);
    Plane a1 = down2(a0), b1 = down2(b0);
    Plane a2 = down2(a1), b2 = down2(b1);
    std::vector<int> vy, vx;
    // max(1, -(-search // 4)) with Python's floor division
    const long q = -(long)search_, fq = q / 4 - ((q % 4 != 0 && q < 0) ? 1 : 0);
    const int rng = (int)std::max(1L, -fq);
    search(a2, b2, rng, nby, nbx, bs / 4, vy, vx);
    for (size_t i = 0; i < nb; ++i) { vy[i] *= 2; vx[i] *= 2; }
    refine(a1, b1, vy, vx, nby, nbx, bs / 2, nullptr);
    for (size_t i = 0; i < nb; ++i) { vy[i] *= 2; vx[i] *= 2; }
    refine(a0, b0, vy, vx, nby, nbx, bs, nullptr);
    std::vector<float> c;
    refine(a0, b0, vy, vx, nby, nbx, bs, &c);
    Plane fy(nby, nbx), fx(nby, nbx);
    const float lo6 = EPS6;
    for (size_t i = 0; i < nb; ++i) {
        float v[9];
        for (int k = 0; k < 9; ++k) v[k] = c[(size_t)k * nb + i];
        const bool centred = argmin9(v) == 4;
        // v index: (i*3 + j) for c[i, j]
        const float c01 = v[1], c10 = v[3], c11 = v[4], c12 = v[5], c21 = v[7];
        float fyv = (float)vy[i], fxv = (float)vx[i];
        const float dny = A(S(c01, M(TWO, c11)), c21);
        const float dnx = A(S(c10, M(TWO, c11)), c12);
        const float ty = np_clip(D(M(HALF, S(c01, c21)), np_max(dny, lo6)), MHALF, HALF);
        const float tx = np_clip(D(M(HALF, S(c10, c12)), np_max(dnx, lo6)), MHALF, HALF);
        fy.d[i] = A(fyv, (centred && dny > 0) ? ty : ZERO);
        fx.d[i] = A(fxv, (centred && dnx > 0) ? tx : ZERO);
    }
    Flow f;
    f.vy = median3(fy);
    f.vx = median3(fx);
    return f;
}

// ----------------------------------------------------------------------- dense_flow / warp_plane
void dense_flow(const Flow& f, int H, int W, int bs, Plane& FY, Plane& FX) {
    const float half = f32((bs - 1) / 2.0), fbs = f32((double)bs);
    std::vector<float> u(H), v(W);
    for (int i = 0; i < H; ++i) u[i] = D(S((float)i, half), fbs);
    for (int j = 0; j < W; ++j) v[j] = D(S((float)j, half), fbs);
    Plane uu(H, W), vv(H, W);
    for (int i = 0; i < H; ++i)
        for (int j = 0; j < W; ++j) { uu.at(i, j) = u[i]; vv.at(i, j) = v[j]; }
    FY = map_coords1(f.vy, uu, vv);
    FX = map_coords1(f.vx, uu, vv);
}

Plane warp_plane(const Plane& img, const Plane& FY, const Plane& FX) {
    const int H = img.H, W = img.W;
    Plane cy(H, W), cx(H, W);
    const float hm = f32((double)(H - 1)), wm = f32((double)(W - 1));
    for (int i = 0; i < H; ++i)
        for (int j = 0; j < W; ++j) {
            cy.at(i, j) = np_clip(A((float)i, FY.at(i, j)), ZERO, hm);
            cx.at(i, j) = np_clip(A((float)j, FX.at(i, j)), ZERO, wm);
        }
    return map_coords1(img, cy, cx);
}

// ---------------------------------------------------------------------------- pyramid_weight
Plane pyramid_weight(const Plane& d, const Plane& s1, const double* ratios, double scale) {
    std::vector<Plane> b = atrous(d, 3);
    Plane w(d.H, d.W, ONE);
    for (int j = 0; j < 3; ++j) {
        const float rj = f32(ratios[j]);
        Plane xx(d.H, d.W);
        for (size_t i = 0; i < xx.size(); ++i) {
            float x = D(b[j].d[i], A(M(s1.d[i], rj), EPS3));
            xx.d[i] = M(x, x);
        }
        Plane u = uniform(xx, (1 << (j + 1)) + 1, REFLECT);
        const double t = py_max(scale, 1e-3) * PYR_C[j] * PYR_K;
        const float t2 = f32(2.0 * t), tf = f32(t);
        for (size_t i = 0; i < w.size(); ++i) {
            float r = SQRT(u.d[i]);
            w.d[i] = M(w.d[i], np_clip(D(S(t2, r), tf), ZERO, ONE));
        }
    }
    return w;
}

// ------------------------------------------------------------------------------- temporal_v5
void temporal_v5(const std::vector<Frame>& window, int ci, const V5Profile& prof, const V5Params& P,
                 std::map<int, Flow>* flows, std::vector<Plane>& out, std::vector<Plane>& rv) {
    const Frame& curf = window[ci];
    const Plane& Y = curf.p[0];
    const double a = P.temporal;
    const Plane gY = gauss1(Y);
    Plane st = curve_sigma(gY, prof.centers, prof.curve[0], prof.ncurve);
    {
        const float tr = f32(prof.tot_ratio[0]), nl = f32(P.noise_level), cn = f32(P.channel_noise[0]);
        for (size_t i = 0; i < st.size(); ++i) st.d[i] = M(M(M(st.d[i], tr), nl), cn);
    }
    Plane den(Y.H, Y.W);
    for (size_t i = 0; i < den.size(); ++i) den.d[i] = A(M(M(TWO, st.d[i]), st.d[i]), EPS6);
    std::vector<Plane> acc, sw, sf;
    for (int c = 0; c < 3; ++c) {
        acc.push_back(curf.p[c]);
        sw.emplace_back(curf.p[c].H, curf.p[c].W, ONE);
        sf.emplace_back(curf.p[c].H, curf.p[c].W, ZERO);
    }
    for (int k = 0; k < (int)window.size(); ++k) {
        if (k == ci || a <= 0) continue;
        const Frame& nb = window[k];
        Flow fl;
        auto it = flows ? flows->find(k - ci) : std::map<int, Flow>::iterator();
        if (flows && it != flows->end()) {
            fl = it->second;
        } else {
            fl = estimate_flow_v5(nb.p[0], Y, P.bs, P.search);
            if (flows) (*flows)[k - ci] = fl;
        }
        Plane FY, FX;
        dense_flow(fl, Y.H, Y.W, P.bs, FY, FX);
        Plane wy = warp_plane(nb.p[0], FY, FX);
        Plane w(Y.H, Y.W);
        if (P.similarity == 1) {
            const float trd = f32(py_max(prof.tot_ratio[0], 1e-6));
            Plane s1(Y.H, Y.W), dd(Y.H, Y.W);
            for (size_t i = 0; i < s1.size(); ++i) {
                s1.d[i] = D(st.d[i], trd);
                dd.d[i] = S(Y.d[i], wy.d[i]);
            }
            w = pyramid_weight(dd, s1, prof.band_ratio[0], P.pyramid_scale);
        } else {
            Plane q(Y.H, Y.W);
            for (size_t i = 0; i < q.size(); ++i) {
                float t = S(wy.d[i], Y.d[i]);
                q.d[i] = M(t, t);
            }
            Plane u = uniform(q, 5, REFLECT);
            const float thr = f32(py_max(P.threshold, 1e-3));
            for (size_t i = 0; i < w.size(); ++i) {
                float e = np_max(S(D(u.d[i], den.d[i]), ONE), ZERO);
                w.d[i] = np_expf(D(-e, thr));
            }
        }
        for (int c = 0; c < 3; ++c) {
            const Plane& p = nb.p[c];
            Plane wp, wcs;
            const Plane* wc = &w;
            if (c == 0) {
                wp = wy;
            } else if (p.same_shape(Y)) {
                wp = warp_plane(p, FY, FX);
            } else {
                Plane fyc = to_grid(FY, p.H, p.W), fxc = to_grid(FX, p.H, p.W);
                for (size_t i = 0; i < fyc.size(); ++i) { fyc.d[i] = D(fyc.d[i], TWO); fxc.d[i] = D(fxc.d[i], TWO); }
                wp = warp_plane(p, fyc, fxc);
                wcs = to_grid(w, p.H, p.W);
                wc = &wcs;
            }
            for (size_t i = 0; i < wp.size(); ++i) {
                const float wv = wc->d[i];
                acc[c].d[i] = A(acc[c].d[i], M(wv, wp.d[i]));
                sw[c].d[i] = A(sw[c].d[i], wv);
                sf[c].d[i] = A(sf[c].d[i], M(M(NVAR, wv), wv));
            }
        }
    }
    out.clear();
    rv.clear();
    const float af = f32(a), a1 = f32(1 - a), aa = f32(a * a);
    for (int c = 0; c < 3; ++c) {
        const Plane& p = curf.p[c];
        Plane o(p.H, p.W), r(p.H, p.W);
        for (size_t i = 0; i < p.size(); ++i) {
            o.d[i] = A(p.d[i], M(af, S(D(acc[c].d[i], sw[c].d[i]), p.d[i])));
            float t = A(a1, D(af, sw[c].d[i]));
            r.d[i] = A(M(t, t), D(M(aa, sf[c].d[i]), M(sw[c].d[i], sw[c].d[i])));
        }
        out.push_back(std::move(o));
        rv.push_back(std::move(r));
    }
}

// --------------------------------------------------------------------------- process_frame_v5
std::vector<Plane> process_frame_v5(const std::vector<Frame>& window, int ci, const V5Profile& prof,
                                    const V5Params& P, std::map<int, Flow>* flows) {
    const int R = P.radius;
    const int lo = std::max(0, ci - R), hi = std::min((int)window.size(), ci + R + 1);
    std::vector<Frame> sub(window.begin() + lo, window.begin() + hi);
    std::vector<Plane> tmp, rv;
    temporal_v5(sub, ci - lo, prof, P, flows, tmp, rv);
    Plane tp[3] = {tmp[0], tmp[1], tmp[2]}, rvp[3] = {rv[0], rv[1], rv[2]};
    std::vector<Plane> out = nvs::spatial_v5(tp, prof, rvp, P);
    nvs::mix_v5(out, window[ci].p, P.mix);
    return out;
}

}  // namespace nvt
