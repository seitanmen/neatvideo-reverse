// nvcore_c - see nvcore_c.h.  The C ABI of libnvcore.dylib: argument unpacking and the calls of the N1
// routines, nothing else (no arithmetic of its own).  C++ exceptions do not cross the ABI: every entry
// point catches them and returns an error code that repro/nv_native.py turns into the Python fallback.
#include "nvcore_c.h"

#include <cstring>
#include <map>
#include <vector>

#include "nv_aft.h"
#include "nv_curve.h"
#include "nv_flat.h"
#include "nv_ndimage.h"
#include "nv_profile.h"
#include "nv_spatial.h"
#include "nv_temporal.h"

namespace {

using nvn::Plane;

nvc::Cancel make_cancel(nvc_cancel_fn fn) {
    if (!fn) return nvc::Cancel();
    return [fn]() { return fn() != 0; };
}

Plane to_plane(const float* p, int H, int W) {
    Plane pl(H, W);
    std::memcpy(pl.d.data(), p, sizeof(float) * pl.size());
    return pl;
}

void from_plane(const Plane& pl, float* out) { std::memcpy(out, pl.d.data(), sizeof(float) * pl.size()); }

nvs::V5Profile unpack_profile(const int* pi, const double* pd) {
    nvs::V5Profile p;
    p.ncurve = pi[0];
    for (int c = 0; c < 3; ++c) p.levels[c] = pi[1 + c];
    for (int i = 0; i < 16; ++i) p.centers[i] = pd[i];
    for (int c = 0; c < 3; ++c)
        for (int i = 0; i < 16; ++i) p.curve[c][i] = pd[16 + 16 * c + i];
    for (int c = 0; c < 3; ++c)
        for (int j = 0; j < nvs::NBANDS; ++j) p.band_ratio[c][j] = pd[64 + 5 * c + j];
    for (int c = 0; c < 3; ++c) p.tot_ratio[c] = pd[79 + c];
    return p;
}

nvs::V5Params unpack_params(const int* pi, const double* pd) {
    nvs::V5Params P;
    P.radius = pi[0];
    P.bs = pi[1];
    P.search = pi[2];
    P.similarity = pi[3];
    P.temporal = pd[0];
    P.threshold = pd[1];
    P.spatial = pd[2];
    P.luma = pd[3];
    P.chroma = pd[4];
    P.noise_level = pd[5];
    P.pyramid_scale = pd[6];
    P.mix = pd[7];
    for (int j = 0; j < nvs::NBANDS; ++j) P.band_amounts[j] = pd[8 + j];
    for (int j = 0; j < nvs::NBANDS; ++j) P.band_noise[j] = pd[13 + j];
    for (int c = 0; c < 3; ++c) P.channel_noise[c] = pd[18 + c];
    return P;
}

bool bad_profile(const nvs::V5Profile& p) {
    if (p.ncurve < 1 || p.ncurve > 16) return true;
    for (int c = 0; c < 3; ++c)
        if (p.levels[c] < 1 || p.levels[c] > nvs::NBANDS) return true;
    return false;
}

}  // namespace

extern "C" {

int nvc_abi_version(void) { return NVC_ABI_VERSION; }

int nvc_noise_curve(const float* rgb, int W, int H, int threads, nvc_cancel_fn cancel, double* xs, double* ys) {
    try {
        nvc::CurveResult r;
        if (!nvc::genuine_noise_curve(rgb, W, H, 3, &r, threads, make_cancel(cancel))) return 0;
        for (int i = 0; i < nvc::NBINS; ++i) {
            xs[i] = r.xs[i];
            ys[i] = r.ys[i];
        }
        return 1;
    } catch (...) {
        return -1;
    }
}

void nvc_flat_area(const float* rgb, int W, int H, int mode, int threads, nvc_cancel_fn cancel, int* out5) {
    try {
        const nvf::Result r = nvf::find_flat_area_genuine(rgb, W, H, 3, mode, nullptr, threads, nullptr,
                                                          make_cancel(cancel));
        out5[0] = r.x;
        out5[1] = r.y;
        out5[2] = r.w;
        out5[3] = r.h;
        out5[4] = r.status;
    } catch (...) {
        out5[0] = out5[1] = out5[2] = out5[3] = 0;
        out5[4] = -2;
    }
}

int nvc_profile_build(const float* rgb, int W, int H, int mode, int x, int y, int w, int h, const float* floor,
                      int lzma, int threads, uint8_t* out) {
    try {
        return nvp::build_profile(rgb, W, H, 3, mode, x, y, w, h, floor, out, lzma != 0, threads);
    } catch (...) {
        return -1;
    }
}

int nvc_profile_build_cancel(const float* rgb, int W, int H, int mode, int x, int y, int w, int h,
                             const float* floor, int lzma, int threads, nvc_cancel_fn cancel, uint8_t* out) {
    try {
        nvp::Cancel cb;
        if (cancel) cb = [cancel]() { return cancel() != 0; };
        return nvp::build_profile(rgb, W, H, 3, mode, x, y, w, h, floor, out, lzma != 0, threads, cb);
    } catch (...) {
        return -1;
    }
}

int nvc_aft(const float* rgb, int W, int H, uint8_t* prof, int mode, int threads, nvc_cancel_fn cancel,
            float* out3) {
    try {
        nva::Cancel cb;
        if (cancel) cb = [cancel]() { return cancel() != 0; };
        return nva::auto_fine_tune(rgb, W, H, prof, mode, nullptr, 0, threads, out3, nullptr, nullptr, cb);
    } catch (...) {
        return -1;
    }
}

void nvc_aft_combine(const uint8_t* raw, const uint8_t* aft, const double* centers, int n, const float* curves,
                     double area_level, const double* tot_ratio, int band, double* cw, double* face,
                     float* curve_out, double* sigma, uint8_t* flags) {
    const float* cv[3] = {curves, curves + n, curves + 2 * n};
    nva::Combine r;
    nva::combine(raw, aft, centers, n, cv, area_level, tot_ratio, band, &r);
    for (int c = 0; c < 3; ++c) {
        for (int j = 0; j < 3; ++j) cw[3 * c + j] = r.cw[c][j];
        for (int i = 0; i < n; ++i) {
            face[c * n + i] = r.face[c][i];
            curve_out[c * n + i] = r.curve[c][i];
        }
        sigma[c] = r.sigma[c];
        for (int k = 0; k < 9; ++k) {
            flags[9 * c + k] = r.marker[c][k];
            flags[27 + 9 * c + k] = r.clip[c][k];
            flags[54 + 9 * c + k] = r.gt3[c][k];
        }
    }
}

int nvc_temporal(int nf, int ci, int H, int W, int Hc, int Wc, const float* const* planes, const int* prof_i,
                 const double* prof_d, const int* par_i, const double* par_d, int nin, const int* off_in,
                 const float* const* flow_in, float* const* flow_out, uint8_t* new_flow, float* const* out,
                 float* const* rv) {
    try {
        const nvs::V5Profile prof = unpack_profile(prof_i, prof_d);
        const nvs::V5Params P = unpack_params(par_i, par_d);
        if (bad_profile(prof) || nf < 1 || ci < 0 || ci >= nf || P.bs < 1) return 2;
        std::vector<nvt::Frame> win(nf);
        for (int f = 0; f < nf; ++f)
            for (int c = 0; c < 3; ++c)
                win[f].p[c] = c ? to_plane(planes[3 * f + c], Hc, Wc) : to_plane(planes[3 * f], H, W);
        const int nby = H / P.bs, nbx = W / P.bs;
        std::map<int, nvt::Flow> flows;
        for (int i = 0; i < nin; ++i)
            flows[off_in[i]] = nvt::Flow{to_plane(flow_in[2 * i], nby, nbx), to_plane(flow_in[2 * i + 1], nby, nbx)};
        std::vector<Plane> o, r;
        nvt::temporal_v5(win, ci, prof, P, &flows, o, r);
        for (int c = 0; c < 3; ++c) {
            from_plane(o[c], out[c]);
            from_plane(r[c], rv[c]);
        }
        for (int k = 0; k < nf; ++k) {
            new_flow[k] = 0;
            if (k == ci) continue;
            bool given = false;
            for (int i = 0; i < nin; ++i) given = given || off_in[i] == k - ci;
            auto it = flows.find(k - ci);
            if (given || it == flows.end()) continue;
            from_plane(it->second.vy, flow_out[2 * k]);
            from_plane(it->second.vx, flow_out[2 * k + 1]);
            new_flow[k] = 1;
        }
        return 0;
    } catch (...) {
        return -1;
    }
}

int nvc_spatial(int H, int W, int Hc, int Wc, const float* const* planes, const float* const* rv,
                const int* prof_i, const double* prof_d, const int* par_i, const double* par_d, int mask,
                float* const* out) {
    try {
        const nvs::V5Profile prof = unpack_profile(prof_i, prof_d);
        const nvs::V5Params P = unpack_params(par_i, par_d);
        if (bad_profile(prof)) return 2;
        const Plane Y = to_plane(planes[0], H, W);
        const Plane lum = nvn::gauss1(Y);
        for (int c = 0; c < 3; ++c) {
            if (!(mask & (1 << c))) continue;
            const int h = c ? Hc : H, w = c ? Wc : W;
            const Plane p = c ? to_plane(planes[c], h, w) : Y;
            from_plane(nvs::spatial_v5_plane(lum, p, c, prof, to_plane(rv[c], h, w), P), out[c]);
        }
        return 0;
    } catch (...) {
        return -1;
    }
}

}  // extern "C"
