// fields - see fields.h.  Only rows move here; the arithmetic is libnvcore's (nvcore.h temporal / finish).
#include "fields.h"

#include <algorithm>
#include <stdexcept>

namespace nvqt {

namespace {

// the frame row field row i of field `parity` reads (nv_fields field_src_row)
int src_row(int H, int parity, int i) {
    const int r = 2 * i + parity;
    if (r < H) return r;
    return H >= 2 ? H - 2 : 0;
}

template <class T>
std::vector<T> split_rows(const std::vector<T>& p, int H, int W, int parity) {
    const int h = (H + 1) / 2;
    std::vector<T> o(static_cast<size_t>(h) * W);
    for (int i = 0; i < h; ++i)
        std::copy_n(p.begin() + static_cast<std::ptrdiff_t>(src_row(H, parity, i)) * W, W,
                    o.begin() + static_cast<std::ptrdiff_t>(i) * W);
    return o;
}

std::vector<float> join_rows(const std::vector<float>& t, const std::vector<float>& b, int H, int W) {
    std::vector<float> o(static_cast<size_t>(H) * W);
    for (int y = 0; y < H; ++y) {
        const std::vector<float>& f = (y & 1) ? b : t;
        std::copy_n(f.begin() + static_cast<std::ptrdiff_t>(y / 2) * W, W, o.begin() + static_cast<std::ptrdiff_t>(y) * W);
    }
    return o;
}

}  // namespace

const char* field_mode_name(int mode) { return mode == FIELD_PLAIN ? "plain" : mode == FIELD_SHIFT ? "shift" : "same"; }

int field_mode_of(const std::string& name) {
    return name == "plain" ? FIELD_PLAIN : name == "shift" ? FIELD_SHIFT : name == "same" ? FIELD_SAME : -1;
}

std::optional<Interlace> interlace_of(const PyObj& P) {
    if (!P.is_dict() || !P.has("interlace") || P.get("interlace").is_none()) return std::nullopt;
    const PyObj& d = P.get("interlace");
    Interlace il;
    il.first = static_cast<int>(d.get("first").as_int());
    il.mode = field_mode_of(d.get("mode").as_str());
    il.radius = static_cast<int>(d.get("radius").as_int());
    if ((il.first != 0 && il.first != 1) || il.mode < 0 || il.radius < 0)
        throw std::runtime_error("bad \"interlace\" parameters: " + d.dump());
    return il;
}

PyObj interlace_json(const Interlace& il) {
    PyObj d = PyObj::dict();
    d.set("first", PyObj::integer(il.first));
    d.set("mode", PyObj::str(field_mode_name(il.mode)));
    d.set("radius", PyObj::integer(il.radius));
    return d;
}

Planes field_planes(const Planes& p, int parity) {
    Planes o;
    o.H = (p.H + 1) / 2;
    o.W = p.W;
    o.Hc = (p.Hc + 1) / 2;
    o.Wc = p.Wc;
    o.Y = split_rows(p.Y, p.H, p.W, parity);
    o.Cb = split_rows(p.Cb, p.Hc, p.Wc, parity);
    o.Cr = split_rows(p.Cr, p.Hc, p.Wc, parity);
    return o;
}

Frame field_frame(const Frame& f, int parity) {
    Frame o;
    o.H = (f.H + 1) / 2;
    o.W = f.W;
    o.Hc = (f.Hc + 1) / 2;
    o.Wc = f.Wc;
    if (f.is_float()) {
        o.fY = split_rows(f.fY, f.H, f.W, parity);
        o.fCb = split_rows(f.fCb, f.Hc, f.Wc, parity);
        o.fCr = split_rows(f.fCr, f.Hc, f.Wc, parity);
    } else {
        o.Y = split_rows(f.Y, f.H, f.W, parity);
        o.Cb = split_rows(f.Cb, f.Hc, f.Wc, parity);
        o.Cr = split_rows(f.Cr, f.Hc, f.Wc, parity);
    }
    return o;
}

Planes join_planes(const Planes& top, const Planes& bottom, int H, int Hc) {
    Planes o;
    o.H = H;
    o.W = top.W;
    o.Hc = Hc;
    o.Wc = top.Wc;
    o.Y = join_rows(top.Y, bottom.Y, H, top.W);
    o.Cb = join_rows(top.Cb, bottom.Cb, Hc, top.Wc);
    o.Cr = join_rows(top.Cr, bottom.Cr, Hc, top.Wc);
    return o;
}

FieldStage field_temporal(const std::vector<Planes>& window, int ci, const Interlace& il, const PyObj& prof,
                          const PyObj& P) {
    // the field sequence of the window: field 2k + j of frame k has parity j ^ first
    std::vector<Planes> fields;
    std::vector<int> parity;
    for (const Planes& f : window)
        for (int j = 0; j < 2; ++j) {
            const int q = j ^ il.first;
            fields.push_back(field_planes(f, q));
            parity.push_back(q);
        }
    // the window radius of nvcore temporal (it slices by P["radius"]): the selected fields are all inside
    PyObj Q = P;
    Q.set("radius", PyObj::integer(il.radius));
    FieldStage st;
    st.H = window[static_cast<size_t>(ci)].H;
    st.Hc = window[static_cast<size_t>(ci)].Hc;
    for (int j = 0; j < 2; ++j) {
        const int s = 2 * ci + j, q = parity[static_cast<size_t>(s)];
        const int lo = std::max(0, s - il.radius), hi = std::min(static_cast<int>(fields.size()), s + il.radius + 1);
        std::vector<Planes> sub;
        std::vector<double> shift;
        int c = 0;
        for (int k = lo; k < hi; ++k) {
            if (il.mode == FIELD_SAME && parity[static_cast<size_t>(k)] != q) continue;
            if (k == s) c = static_cast<int>(sub.size());
            sub.push_back(fields[static_cast<size_t>(k)]);
            const double d = 0.5 * (q - parity[static_cast<size_t>(k)]);
            shift.push_back(d);
            shift.push_back(d);
        }
        temporal(sub, c, prof, Q, nullptr, &st.tmp[q], &st.rv[q], il.mode == FIELD_SHIFT ? shift.data() : nullptr);
        st.src[q] = fields[static_cast<size_t>(s)];
        st.n[q] = static_cast<int>(sub.size());
    }
    return st;
}

Planes field_finish(const FieldStage& st, const PyObj& prof, const PyObj& P) {
    Planes o[2];
    for (int q = 0; q < 2; ++q) o[q] = finish(st.src[q], st.tmp[q], st.rv[q], prof, P);
    return join_planes(o[0], o[1], st.H, st.Hc);
}

Planes process_fields(const std::vector<Planes>& window, int ci, const Interlace& il, const PyObj& prof,
                      const PyObj& P) {
    return field_finish(field_temporal(window, ci, il, prof, P), prof, P);
}

Planes assist_fields(const Planes& frame, const PyObj& prof, const PyObj& P) {
    Planes o[2];
    for (int q = 0; q < 2; ++q) {
        const Planes fp = field_planes(frame, q);
        Planes ones = fp;
        for (int c = 0; c < 3; ++c) std::fill(ones.v(c).begin(), ones.v(c).end(), 1.0f);
        o[q] = finish(fp, fp, ones, prof, P);
    }
    return join_planes(o[0], o[1], frame.H, frame.Hc);
}

// ---------------------------------------------------------------- profiles
std::string profile_fields(const PyObj& prof) {
    if (!prof.is_dict() || prof.get("fields").type() != PyObj::STR) return "frame";
    const std::string k = prof.get("fields").as_str();
    return k == "two" || k == "one" ? k : "frame";
}

int profile_parity(const PyObj& prof) {
    if (profile_fields(prof) == "frame") return -1;
    const PyObj& f = prof.get("field");
    return f.type() == PyObj::STR && f.as_str() == "bottom" ? 1 : 0;
}

Frame profile_image(const PyObj& prof, const Frame& f) {
    const int q = profile_parity(prof);
    return q < 0 ? f : field_frame(f, q);
}

PyObj build_profile_fields(const std::vector<Frame>& frames, int f, bool full_range, const std::string& matrix,
                           const std::string& mode, const std::vector<int>* area, const std::string& kind, int parity,
                           const CancelFn& cancel) {
    if (frames.empty()) throw std::runtime_error("no frames");
    if (kind != "two" && kind != "one") throw std::runtime_error("unknown field profile kind " + kind);
    if (parity != 0 && parity != 1) throw std::runtime_error("bad field parity");
    const int n = static_cast<int>(frames.size());
    f = std::max(0, std::min(f, n - 1));
    auto fld = [&](int k, int q) { return to_planes(field_frame(frames[static_cast<size_t>(k)], q)); };
    const Planes pl = fld(f, parity);
    if (!area && (pl.H < 2 || pl.W < 2))
        throw std::runtime_error("Shape of array too small to calculate a numerical gradient, at least (edge_order + 1) "
                                 "elements are required.");
    std::vector<Planes> more;
    std::optional<Planes> pair;
    if (kind == "one") {
        for (int k = f + 1; k < std::min(f + 3, n); ++k) more.push_back(fld(k, parity));
        if (!more.empty()) pair = more[0];
    } else {
        more.push_back(fld(f, 1 - parity));                // the next field in time (the other parity)
        if (f + 1 < n) {
            more.push_back(fld(f + 1, parity));            // and the one after it (frame f + 1, the same parity)
            pair = more[1];
        }
    }
    std::vector<int> fa;
    if (area) {                                            // frame rows -> field rows
        fa = *area;                                        // (y0, x0, h, w) or a square (y0, x0, size)
        if (!fa.empty()) fa[0] /= 2;
        if (fa.size() == 4) fa[2] = std::max(fa[2] / 2, 8);   // a square keeps its side (clamped by the build)
    }
    PyObj p = build_profile_planes(pl, pair ? &*pair : nullptr, more, f, full_range, matrix, mode, area ? &fa : nullptr,
                                   cancel);
    p.set("fields", PyObj::str(kind));
    p.set("field", PyObj::str(parity == 0 ? "top" : "bottom"));
    return p;
}

}  // namespace nvqt
