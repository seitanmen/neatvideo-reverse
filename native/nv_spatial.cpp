// nv_spatial - see nv_spatial.h.  Line-by-line port of nvfilter.spatial_v5 (each numpy operation
// is one float32 operation, left to right as Python evaluates it; Python-float operands are cast
// to float32 first, as numpy does with weak scalars).
#include "nv_spatial.h"

namespace nvs {

using namespace nvn;

std::vector<Plane> spatial_v5(const Plane planes[3], const V5Profile& prof, const Plane rv[3],
                              const V5Params& P) {
    const float one = bits(0x3f800000u);       // 1.0
    const float eps6 = bits(0x358637bdu);      // 1e-6
    // lum = gaussian_filter(Y, 1.0)
    const Plane lum = gauss1(planes[0]);
    std::vector<Plane> out;
    for (int c = 0; c < 3; ++c) {
        const Plane& p = planes[c];
        const int L = prof.levels[c];
        // names = V5_BANDS[5-L:] if c else V5_BANDS[:L]  -> band index into V5_BANDS
        int name_off = c ? NBANDS - L : 0;
        const double A_ = c == 0 ? P.luma : P.chroma;
        // s1 = curve_sigma(prof, name, _to_grid(lum, p.shape)) * noise_level * channel_noise[name] * sqrt(rv[c])
        Plane s1 = curve_sigma(to_grid(lum, p.H, p.W), prof.centers, prof.curve[c], prof.ncurve);
        const float nl = f32(P.noise_level), cn = f32(P.channel_noise[c]);
        for (size_t i = 0; i < s1.size(); ++i)
            s1.d[i] = M(M(M(s1.d[i], nl), cn), SQRT(rv[c].d[i]));
        std::vector<Plane> b = atrous(p, L);
        Plane rec = b[L];
        for (int j = 0; j < L; ++j) {
            const int nm = name_off + j;
            const float br = f32(prof.band_ratio[c][j]), bn = f32(P.band_noise[nm]);
            const float sp = f32(P.spatial);
            // amt = min(max(A * band_amounts[name], 0.0), 1.0)   (Python floats)
            const float amt = f32(py_min(py_max(A_ * P.band_amounts[nm], 0.0), 1.0));
            // e = uniform_filter(b[j] * b[j], 7)
            Plane sq(p.H, p.W);
            const Plane& bj = b[j];
            for (size_t i = 0; i < sq.size(); ++i) sq.d[i] = M(bj.d[i], bj.d[i]);
            const Plane e = uniform(sq, 7, REFLECT);
            for (size_t i = 0; i < rec.size(); ++i) {
                // s = s1 * band_ratio * band_noise
                float s = M(M(s1.d[i], br), bn);
                // g = np.maximum(e - spatial * s * s, 0.0) / (e + 1e-6)
                float g = D(np_max(S(e.d[i], M(M(sp, s), s)), 0.0f), A(e.d[i], eps6));
                // rec = rec + b[j] * (1.0 - amt * (1.0 - g))
                rec.d[i] = A(rec.d[i], M(bj.d[i], S(one, M(amt, S(one, g)))));
            }
        }
        out.push_back(std::move(rec));
    }
    return out;
}

void mix_v5(std::vector<Plane>& out, const Plane src[3], double m) {
    if (!(m > 0)) return;
    const float mf = f32(m);
    for (int c = 0; c < 3; ++c)
        for (size_t i = 0; i < out[c].size(); ++i)
            out[c].d[i] = A(out[c].d[i], M(mf, S(src[c].d[i], out[c].d[i])));
}

}  // namespace nvs
