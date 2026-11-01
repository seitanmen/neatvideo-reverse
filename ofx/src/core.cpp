// core - see core.h.
#include "core.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "nvcore_c.h"
#include "nvcore_c_depth.h"
#include "nvcore_c_ui.h"

namespace mvofx {

namespace {

// gbrpf32le in nvcore_c_depth.h's terms: family 1 (rgb), 32 bits, float, 4:4:4
constexpr int FAM = 1, BITS = 32, IS_FLOAT = 1, CHROMA = 444;

Planes alloc(int H, int W) {
    Planes p;
    p.H = H;
    p.W = W;
    const size_t n = static_cast<size_t>(H) * W;
    p.Y.resize(n);
    p.Cb.resize(n);
    p.Cr.resize(n);
    return p;
}

void spatial_call(const Planes& tmp, const Planes& rv, const V5Blocks& b, int mask, Planes* out) {
    const float* sp[3] = {tmp.p(0), tmp.p(1), tmp.p(2)};
    const float* rvp[3] = {rv.p(0), rv.p(1), rv.p(2)};
    float* op[3] = {out->p(0), out->p(1), out->p(2)};
    if (nvc_spatial(tmp.H, tmp.W, tmp.H, tmp.W, sp, rvp, b.pi, b.pd, b.qi, b.qd, mask, op) != 0)
        throw Error("nvc_spatial failed");
}

}  // namespace

std::vector<float> rgba_to_gbr(const RgbaView& v) {
    const int W = v.W, H = v.H;
    const size_t n = static_cast<size_t>(W) * H;
    // the raw frame in ffmpeg's plane order G, B, R (nv_depth.split of gbrpf32le), top-down
    std::vector<float> raw(3 * n);
    float *G = raw.data(), *B = G + n, *R = B + n;
    for (int i = 0; i < H; ++i)
        for (int j = 0; j < W; ++j) {
            const float* px = v.px(i, j);
            const size_t k = static_cast<size_t>(i) * W + j;
            R[k] = px[0];
            G[k] = px[1];
            B[k] = px[2];
        }
    return raw;
}

void rgba_to_internal(const RgbaView& v, int matrix, Planes* out, Restore* rs) {
    const int W = v.W, H = v.H;
    const size_t n = static_cast<size_t>(W) * H;
    std::vector<float> raw = rgba_to_gbr(v);
    // nv_depth.sanitize of each plane (to_internal sanitizes float input; read_frames keeps the restore info)
    for (int k = 0; k < 3; ++k) {
        float* p = raw.data() + k * n;
        const bool bad = std::any_of(p, p + n, [](float x) { return !std::isfinite(x); });
        if (rs) {
            rs->idx[k].clear();
            rs->vals[k].clear();
        }
        if (!bad) continue;
        std::vector<long> idx(n);
        std::vector<float> vals(n);
        const long c = nvc_depth_sanitize(p, static_cast<long>(n), idx.data(), vals.data());
        if (rs) {
            idx.resize(static_cast<size_t>(c));
            vals.resize(static_cast<size_t>(c));
            rs->idx[k] = std::move(idx);
            rs->vals[k] = std::move(vals);
        }
    }
    *out = alloc(H, W);
    if (nvc_depth_to_internal(reinterpret_cast<const uint8_t*>(raw.data()), FAM, BITS, IS_FLOAT, CHROMA, W, H, matrix,
                              out->Y.data(), out->Cb.data(), out->Cr.data()) != 0)
        throw Error("nvc_depth_to_internal failed");
}

std::vector<float> internal_to_rgb(const Planes& p, int matrix, const Restore* rs) {
    const size_t n = static_cast<size_t>(p.W) * p.H;
    std::vector<float> raw(3 * n);    // G, B, R
    const long* ridx[3] = {nullptr, nullptr, nullptr};
    const float* rvals[3] = {nullptr, nullptr, nullptr};
    long rn[3] = {0, 0, 0};
    if (rs)
        for (int k = 0; k < 3; ++k)
            if (!rs->idx[k].empty()) {
                ridx[k] = rs->idx[k].data();
                rvals[k] = rs->vals[k].data();
                rn[k] = static_cast<long>(rs->idx[k].size());
            }
    if (nvc_depth_from_internal(p.Y.data(), p.Cb.data(), p.Cr.data(), FAM, BITS, IS_FLOAT, CHROMA, p.W, p.H, matrix,
                                ridx, rvals, rn, reinterpret_cast<uint8_t*>(raw.data())) != 0)
        throw Error("nvc_depth_from_internal failed");
    std::vector<float> rgb(3 * n);
    const float *G = raw.data(), *B = G + n, *R = B + n;
    for (size_t k = 0; k < n; ++k) {
        rgb[3 * k] = R[k];
        rgb[3 * k + 1] = G[k];
        rgb[3 * k + 2] = B[k];
    }
    return rgb;
}

Planes process_frame(const std::vector<const Planes*>& window, int ci, const RenderState& s) {
    const int nf = static_cast<int>(window.size());
    if (nf < 1 || ci < 0 || ci >= nf) throw Error("bad window");
    const Planes& f0 = *window[0];
    const V5Blocks& tb = s.temporal;
    const int H = f0.H, W = f0.W, bs = tb.qi[1];
    for (const Planes* w : window)
        if (w->H != H || w->W != W) throw Error("window frames of different sizes");
    if (bs < 1 || std::min(H, W) < bs) throw Error("the frame is smaller than the motion block");
    // backend.temporal (qtui nvcore.cpp temporal; the window is already t-R .. t+R)
    std::vector<const float*> planes;
    for (const Planes* w : window)
        for (int c = 0; c < 3; ++c) planes.push_back(w->p(c));
    const int nby = H / bs, nbx = W / bs;
    std::vector<std::vector<float>> fout(2 * static_cast<size_t>(nf), std::vector<float>(static_cast<size_t>(nby) * nbx));
    std::vector<float*> fop;
    for (auto& v : fout) fop.push_back(v.data());
    std::vector<uint8_t> nw(static_cast<size_t>(nf), 0);
    Planes tmp = alloc(H, W), rv = alloc(H, W);
    float* tp[3] = {tmp.p(0), tmp.p(1), tmp.p(2)};
    float* rp[3] = {rv.p(0), rv.p(1), rv.p(2)};
    const int offs0 = 0;                     // nv_native passes [0] for no given flows (nin = 0)
    if (nvc_temporal(nf, ci, H, W, H, W, planes.data(), tb.pi, tb.pd, tb.qi, tb.qd, 0, &offs0, nullptr, fop.data(),
                     nw.data(), tp, rp) != 0)
        throw Error("nvc_temporal failed");
    // backend.finish (qtui nvcore.cpp finish): spatial_ui + Mix with original
    Planes out = tmp;
    if (s.spatial) {
        if (s.uniform)
            spatial_call(tmp, rv, s.spatial_all, 7, &out);
        else
            for (int c = 0; c < 3; ++c) spatial_call(tmp, rv, s.spatial_ch[c], 1 << c, &out);
    }
    if (s.mix > 0) {
        const Planes& src = *window[static_cast<size_t>(ci)];
        float* op[3] = {out.p(0), out.p(1), out.p(2)};
        const float* srcp[3] = {src.p(0), src.p(1), src.p(2)};
        nvc_mix(op, srcp, H, W, H, W, s.mix);
    }
    return out;
}

}  // namespace mvofx
