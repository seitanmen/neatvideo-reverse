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

namespace {

// backend.temporal (qtui nvcore.cpp temporal) of a window used as it is: tmp, rv; shift (INT-4 fields): the half-line
// offsets of the window frames (nvc_temporal_shift), nullptr = nvc_temporal
void temporal_stage(const std::vector<const Planes*>& window, int ci, const RenderState& s, const double* shift,
                    Planes* tmp_out, Planes* rv_out) {
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
    if (shift) {
        if (nvc_temporal_shift(nf, ci, H, W, H, W, planes.data(), tb.pi, tb.pd, tb.qi, tb.qd, 0, &offs0, nullptr,
                               fop.data(), nw.data(), tp, rp, shift) != 0)
            throw Error("nvc_temporal_shift failed");
    } else if (nvc_temporal(nf, ci, H, W, H, W, planes.data(), tb.pi, tb.pd, tb.qi, tb.qd, 0, &offs0, nullptr, fop.data(),
                            nw.data(), tp, rp) != 0) {
        throw Error("nvc_temporal failed");
    }
    *tmp_out = std::move(tmp);
    *rv_out = std::move(rv);
}

// backend.finish (qtui nvcore.cpp finish): spatial_ui + Mix with src
Planes finish_stage(const Planes& src, const Planes& tmp, const Planes& rv, const RenderState& s) {
    const int H = tmp.H, W = tmp.W;
    Planes out = tmp;
    if (s.spatial) {
        if (s.uniform)
            spatial_call(tmp, rv, s.spatial_all, 7, &out);
        else
            for (int c = 0; c < 3; ++c) spatial_call(tmp, rv, s.spatial_ch[c], 1 << c, &out);
    }
    if (s.mix > 0) {
        float* op[3] = {out.p(0), out.p(1), out.p(2)};
        const float* srcp[3] = {src.p(0), src.p(1), src.p(2)};
        nvc_mix(op, srcp, H, W, H, W, s.mix);
    }
    return out;
}

// the field `parity` of 4:4:4 planes (qtui fields.cpp field_planes: rows parity, parity + 2, ..; the bottom field of
// an odd height ends with its last row repeated)
Planes field_of(const Planes& p, int parity) {
    const int h = (p.H + 1) / 2, W = p.W;
    Planes o = alloc(h, W);
    for (int i = 0; i < h; ++i) {
        int r = 2 * i + parity;
        if (r >= p.H) r = p.H >= 2 ? p.H - 2 : 0;
        for (int c = 0; c < 3; ++c)
            std::memcpy(o.p(c) + static_cast<size_t>(i) * W, p.p(c) + static_cast<size_t>(r) * W, sizeof(float) * W);
    }
    return o;
}

}  // namespace

Planes process_frame(const std::vector<const Planes*>& window, int ci, const RenderState& s) {
    Planes tmp, rv;
    temporal_stage(window, ci, s, nullptr, &tmp, &rv);
    return finish_stage(*window[static_cast<size_t>(ci)], tmp, rv, s);
}

Planes process_fields(const std::vector<const Planes*>& window, int ci, const RenderState& s) {
    // qtui fields.cpp process_fields: field 2k + j of window frame k has parity j ^ first
    if (s.field_first != 0 && s.field_first != 1) throw Error("not an interlaced state");
    std::vector<Planes> fields;
    std::vector<int> parity;
    for (const Planes* f : window)
        for (int j = 0; j < 2; ++j) {
            fields.push_back(field_of(*f, j ^ s.field_first));
            parity.push_back(j ^ s.field_first);
        }
    const int R = s.field_radius;
    Planes out[2];
    for (int j = 0; j < 2; ++j) {
        const int c0 = 2 * ci + j, q = parity[static_cast<size_t>(c0)];
        const int lo = std::max(0, c0 - R), hi = std::min(static_cast<int>(fields.size()), c0 + R + 1);
        std::vector<const Planes*> sub;
        std::vector<double> shift;
        int c = 0;
        for (int k = lo; k < hi; ++k) {
            if (s.field_mode == 2 && parity[static_cast<size_t>(k)] != q) continue;   // same parity
            if (k == c0) c = static_cast<int>(sub.size());
            sub.push_back(&fields[static_cast<size_t>(k)]);
            const double d = 0.5 * (q - parity[static_cast<size_t>(k)]);
            shift.push_back(d);
            shift.push_back(d);
        }
        Planes tmp, rv;
        temporal_stage(sub, c, s, s.field_mode == 1 ? shift.data() : nullptr, &tmp, &rv);
        out[q] = finish_stage(fields[static_cast<size_t>(c0)], tmp, rv, s);
    }
    // join: frame row y from field y & 1, row y / 2
    const Planes& f = *window[static_cast<size_t>(ci)];
    Planes o = alloc(f.H, f.W);
    for (int y = 0; y < f.H; ++y)
        for (int c = 0; c < 3; ++c)
            std::memcpy(o.p(c) + static_cast<size_t>(y) * f.W, out[y & 1].p(c) + static_cast<size_t>(y / 2) * f.W,
                        sizeof(float) * f.W);
    return o;
}

}  // namespace mvofx
