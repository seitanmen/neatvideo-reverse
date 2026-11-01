// nv_fieldproc - see nv_fieldproc.h.  The fields move by nv_fields (bytes only); the arithmetic is that of
// nv_temporal / nv_spatial.
#include "nv_fieldproc.h"

#include <algorithm>
#include <map>
#include <stdexcept>

#include "nv_fields.h"

namespace nvfp {

namespace {

nvfield::Plane<const float> cview(const Plane& p) { return {p.d.data(), p.W, p.H, p.W}; }
nvfield::Plane<float> view(Plane& p) { return {p.d.data(), p.W, p.H, p.W}; }

}  // namespace

void split_fields(const Frame& f, Frame* top, Frame* bottom) {
    for (int c = 0; c < 3; ++c) {
        const Plane& p = f.p[c];
        const int h = nvfield::field_rows(p.H);
        top->p[c] = Plane(h, p.W);
        bottom->p[c] = Plane(h, p.W);
        if (!nvfield::split(cview(p), view(top->p[c]), view(bottom->p[c])))
            throw std::invalid_argument("split_fields: bad plane");
    }
}

Frame join_fields(const Frame& top, const Frame& bottom, int H, int Hc) {
    Frame f;
    for (int c = 0; c < 3; ++c) {
        f.p[c] = Plane(c ? Hc : H, top.p[c].W);
        if (!nvfield::join(cview(top.p[c]), cview(bottom.p[c]), view(f.p[c])))
            throw std::invalid_argument("join_fields: bad field");
    }
    return f;
}

std::vector<Plane> process_field_v5(const std::vector<Frame>& fields, const std::vector<int>& parity, int ci, int mode,
                                    const V5Profile& prof, const V5Params& P) {
    const int R = P.radius;
    const int lo = std::max(0, ci - R), hi = std::min((int)fields.size(), ci + R + 1);
    std::vector<Frame> sub;
    std::vector<double> shift;
    int c = 0;
    for (int k = lo; k < hi; ++k) {
        if (mode == SAME && parity[k] != parity[ci]) continue;
        if (k == ci) c = (int)sub.size();
        sub.push_back(fields[k]);
        const double d = 0.5 * (parity[ci] - parity[k]);
        shift.push_back(d);
        shift.push_back(d);
    }
    std::map<int, nvt::Flow> flows;
    std::vector<Plane> tmp, rv;
    nvt::temporal_v5(sub, c, prof, P, &flows, tmp, rv, mode == SHIFT ? shift.data() : nullptr);
    Plane tp[3] = {tmp[0], tmp[1], tmp[2]}, rvp[3] = {rv[0], rv[1], rv[2]};
    std::vector<Plane> out = nvs::spatial_v5(tp, prof, rvp, P);
    nvs::mix_v5(out, fields[ci].p, P.mix);
    return out;
}

std::vector<Plane> process_frame_fields(const std::vector<Frame>& window, int ci, int first, int mode,
                                        const V5Profile& prof, const V5Params& P) {
    if (first != nvfield::TOP && first != nvfield::BOTTOM) throw std::invalid_argument("process_frame_fields: first");
    if (mode != PLAIN && mode != SHIFT && mode != SAME) throw std::invalid_argument("process_frame_fields: mode");
    std::vector<Frame> fields(2 * window.size());
    std::vector<int> parity(fields.size());
    for (size_t k = 0; k < window.size(); ++k) {
        Frame t, b;
        split_fields(window[k], &t, &b);
        fields[2 * k] = first == nvfield::TOP ? t : b;
        fields[2 * k + 1] = first == nvfield::TOP ? b : t;
        parity[2 * k] = first;
        parity[2 * k + 1] = 1 - first;
    }
    Frame o[2];
    for (int j = 0; j < 2; ++j) {
        std::vector<Plane> r = process_field_v5(fields, parity, 2 * ci + j, mode, prof, P);
        for (int c = 0; c < 3; ++c) o[j].p[c] = std::move(r[c]);
    }
    const Frame& top = first == nvfield::TOP ? o[0] : o[1];
    const Frame& bottom = first == nvfield::TOP ? o[1] : o[0];
    Frame f = join_fields(top, bottom, window[ci].p[0].H, window[ci].p[1].H);
    return {f.p[0], f.p[1], f.p[2]};
}

}  // namespace nvfp
