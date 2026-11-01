// nv_fields - see nv_fields.h.
#include "nv_fields.h"

#include <cstring>

namespace nvfield {

int field_src_row(int H, int parity, int i) {
    if (parity == TOP || 2 * i + 1 < H) return 2 * i + parity;
#if defined(NVF_MUT_PADROW)          // negative control (verify_fields.py --sensitivity): the pad row repeats the
    return H - 1;                    // frame's last row (the other field)
#else
    return H >= 3 ? H - 2 : 0;
#endif
}

namespace {

// The implementations are templates with internal linkage; the public functions are plain overloads (nv_fields.h),
// so the library exports strong symbols only.

template <class T>
bool plane_ok(const Plane<T>& p) {
    if (p.W < 0 || p.H < 0 || p.stride < p.W) return false;
    return p.data != nullptr || p.W == 0 || p.H == 0;
}

template <class T>
bool view_ok(const FieldView<T>& v) {
    if (v.W < 0 || v.H < 0 || v.real < 0 || v.real > v.H || v.stride < v.W) return false;
    if (v.W == 0) return true;
    return (v.real == 0 || v.data != nullptr) && (v.real == v.H || v.pad != nullptr);
}

// The bytes [a, b) a plane's rows 0 .. rows - 1 occupy (first row to the end of the last row); empty without elements
struct Span {
    std::uintptr_t a = 0, b = 0;
};
template <class T>
Span span(const T* data, int rows, std::ptrdiff_t stride, int W) {
    if (!data || rows <= 0 || W <= 0) return {};
    const std::uintptr_t a = reinterpret_cast<std::uintptr_t>(data);
    return {a, a + static_cast<std::uintptr_t>((rows - 1) * stride + W) * sizeof(T)};
}
bool overlap(const Span& x, const Span& y) { return x.a < x.b && y.a < y.b && x.a < y.b && y.a < x.b; }

// field `parity` (rows: data, stride) read into frame plane dst: no shared memory, or the field_view of dst itself
template <class T>
bool join_source_ok(const T* data, std::ptrdiff_t stride, int parity, const Plane<T>& dst) {
    const Span d = span<T>(dst.data, dst.H, dst.stride, dst.W);
    const Span f = span(data, field_real_rows(dst.H, parity), stride, dst.W);
#if defined(NVF_MUT_NOALIAS)         // negative control: any shared memory is accepted
    return true;
#endif
    if (!overlap(d, f)) return true;
    return reinterpret_cast<std::uintptr_t>(data) ==
               reinterpret_cast<std::uintptr_t>(dst.data) + static_cast<std::uintptr_t>(parity * dst.stride) * sizeof(T) &&
           stride == 2 * dst.stride;
}

// row copy of n elements (memmove; nothing for the in-place rows of a join through dst's own field views)
template <class T>
inline void copy_row(T* dst, const T* src, int n) {
    if (n > 0 && dst != src) std::memmove(dst, src, static_cast<std::size_t>(n) * sizeof(T));
}

// the frame rows of dst from two row sources (field row i of parity p -> frame row 2i + p)
template <class S, class T>
void join_rows(const S& top, const S& bottom, const Plane<T>& dst) {
    if (dst.W == 0) return;
    for (int y = 0; y < dst.H; ++y) {
        const S& f = (y & 1) ? bottom : top;
#if defined(NVF_MUT_NARROW)          // negative control: the last column is lost
        copy_row(dst.row(y), f.row(y >> 1), dst.W - 1);
#else
        copy_row(dst.row(y), f.row(y >> 1), dst.W);
#endif
    }
#if defined(NVF_MUT_PADWRITE)        // negative control: the bottom field's pad row is written past the frame
    if (dst.H & 1) copy_row(dst.data + dst.H * dst.stride, bottom.row(dst.H >> 1), dst.W);
#endif
}

template <class T>
bool field_view_t(const Plane<T>& p, int parity, FieldView<T>* v) {
    if (!plane_ok(p) || (parity != TOP && parity != BOTTOM) || !v) return false;
    FieldView<T> r;
    r.W = p.W;
    r.H = field_rows(p.H);
    r.real = field_real_rows(p.H, parity);
    if (p.data != nullptr && p.W > 0) {      // a plane without elements: no pointers (nothing to point into)
        r.stride = 2 * p.stride;
        r.data = r.real > 0 ? p.data + parity * p.stride : nullptr;
        r.pad = r.real < r.H ? p.row(field_src_row(p.H, parity, r.real)) : nullptr;
    }
    *v = r;
    return true;
}

template <class T>
bool split_t(const Plane<const T>& src, const Plane<T>& top, const Plane<T>& bottom) {
    if (!plane_ok(src) || !plane_ok(top) || !plane_ok(bottom)) return false;
    const int fh = field_rows(src.H);
    if (top.W != src.W || bottom.W != src.W || top.H != fh || bottom.H != fh) return false;
    const Span s = span(src.data, src.H, src.stride, src.W), t = span<T>(top.data, fh, top.stride, top.W),
               b = span<T>(bottom.data, fh, bottom.stride, bottom.W);
    if (overlap(s, t) || overlap(s, b) || overlap(t, b)) return false;
    if (src.W == 0) return true;
    FieldView<const T> v[2];
    field_view_t(src, TOP, &v[0]);
    field_view_t(src, BOTTOM, &v[1]);
    const Plane<T>* out[2] = {&top, &bottom};
    for (int f = 0; f < 2; ++f)
        for (int i = 0; i < fh; ++i) copy_row(out[f]->row(i), v[f].row(i), src.W);
    return true;
}

template <class T>
bool join_t(const Plane<const T>& top, const Plane<const T>& bottom, const Plane<T>& dst) {
    if (!plane_ok(top) || !plane_ok(bottom) || !plane_ok(dst)) return false;
    const int fh = field_rows(dst.H);
    if (top.W != dst.W || bottom.W != dst.W || top.H < fh || bottom.H < fh) return false;
    if (!join_source_ok(top.data, top.stride, TOP, dst) || !join_source_ok(bottom.data, bottom.stride, BOTTOM, dst))
        return false;
    join_rows(top, bottom, dst);
    return true;
}

template <class T>
bool join_t(const FieldView<const T>& top, const FieldView<const T>& bottom, const Plane<T>& dst) {
    if (!view_ok(top) || !view_ok(bottom) || !plane_ok(dst)) return false;
    if (top.W != dst.W || bottom.W != dst.W || top.real < field_real_rows(dst.H, TOP) ||
        bottom.real < field_real_rows(dst.H, BOTTOM))
        return false;
    if (!join_source_ok(top.data, top.stride, TOP, dst) || !join_source_ok(bottom.data, bottom.stride, BOTTOM, dst))
        return false;
    join_rows(top, bottom, dst);
    return true;
}

template <class T>
bool field_views_t(const Frame3<T>& f, FieldFrame3<T>* top, FieldFrame3<T>* bottom) {
    if (!top || !bottom) return false;
    FieldFrame3<T> t, b;
    for (int k = 0; k < 3; ++k) {
#if defined(NVF_MUT_CHROMA)          // negative control: the chroma rows split as one field (progressive 4:2:0 siting)
        const int bp = k == 0 ? BOTTOM : TOP;
#else
        const int bp = BOTTOM;
#endif
        if (!field_view_t(f.p[k], TOP, &t.p[k]) || !field_view_t(f.p[k], bp, &b.p[k])) return false;
    }
    *top = t;
    *bottom = b;
    return true;
}

template <class T>
bool split_frame_t(const Frame3<const T>& src, const Frame3<T>& top, const Frame3<T>& bottom) {
    Span rd[3], wr[6];
    for (int k = 0; k < 3; ++k) {            // check every plane first: nothing is written on a mismatch
        const int fh = field_rows(src.p[k].H);
        if (!plane_ok(src.p[k]) || !plane_ok(top.p[k]) || !plane_ok(bottom.p[k]) || top.p[k].W != src.p[k].W ||
            bottom.p[k].W != src.p[k].W || top.p[k].H != fh || bottom.p[k].H != fh)
            return false;
        rd[k] = span(src.p[k].data, src.p[k].H, src.p[k].stride, src.p[k].W);
        wr[2 * k] = span<T>(top.p[k].data, fh, top.p[k].stride, top.p[k].W);
        wr[2 * k + 1] = span<T>(bottom.p[k].data, fh, bottom.p[k].stride, bottom.p[k].W);
    }
    for (int i = 0; i < 6; ++i) {
        for (int k = 0; k < 3; ++k)
            if (overlap(wr[i], rd[k])) return false;
        for (int j = i + 1; j < 6; ++j)
            if (overlap(wr[i], wr[j])) return false;
    }
    FieldFrame3<const T> vt, vb;
    field_views_t(src, &vt, &vb);
    for (int k = 0; k < 3; ++k) {
        if (src.p[k].W == 0) continue;
        for (int i = 0; i < top.p[k].H; ++i) {
            copy_row(top.p[k].row(i), vt.p[k].row(i), src.p[k].W);
            copy_row(bottom.p[k].row(i), vb.p[k].row(i), src.p[k].W);
        }
    }
    return true;
}

// join_frame: field plane k (both parities) may share memory with frame plane k only as its field_view, and with no
// other frame plane
template <class S, class T>
bool join_frame_sources_ok(const S& top, const S& bottom, const Frame3<T>& dst) {
    for (int k = 0; k < 3; ++k)
        for (int j = 0; j < 3; ++j) {
            if (j == k) {
                if (!join_source_ok(top.p[k].data, top.p[k].stride, TOP, dst.p[j]) ||
                    !join_source_ok(bottom.p[k].data, bottom.p[k].stride, BOTTOM, dst.p[j]))
                    return false;
                continue;
            }
            const Span d = span<T>(dst.p[j].data, dst.p[j].H, dst.p[j].stride, dst.p[j].W);
            const int W = dst.p[k].W;
            if (overlap(d, span(top.p[k].data, field_real_rows(dst.p[k].H, TOP), top.p[k].stride, W)) ||
                overlap(d, span(bottom.p[k].data, field_real_rows(dst.p[k].H, BOTTOM), bottom.p[k].stride, W)))
                return false;
        }
    return true;
}

template <class T>
bool join_frame_t(const Frame3<const T>& top, const Frame3<const T>& bottom, const Frame3<T>& dst) {
    for (int k = 0; k < 3; ++k) {
        const int fh = field_rows(dst.p[k].H);
        if (!plane_ok(top.p[k]) || !plane_ok(bottom.p[k]) || !plane_ok(dst.p[k]) || top.p[k].W != dst.p[k].W ||
            bottom.p[k].W != dst.p[k].W || top.p[k].H < fh || bottom.p[k].H < fh)
            return false;
    }
    if (!join_frame_sources_ok(top, bottom, dst)) return false;
    for (int k = 0; k < 3; ++k) join_rows(top.p[k], bottom.p[k], dst.p[k]);
    return true;
}

template <class T>
bool join_frame_t(const FieldFrame3<const T>& top, const FieldFrame3<const T>& bottom, const Frame3<T>& dst) {
    for (int k = 0; k < 3; ++k)
        if (!view_ok(top.p[k]) || !view_ok(bottom.p[k]) || !plane_ok(dst.p[k]) || top.p[k].W != dst.p[k].W ||
            bottom.p[k].W != dst.p[k].W || top.p[k].real < field_real_rows(dst.p[k].H, TOP) ||
            bottom.p[k].real < field_real_rows(dst.p[k].H, BOTTOM))
            return false;
    if (!join_frame_sources_ok(top, bottom, dst)) return false;
    for (int k = 0; k < 3; ++k) join_rows(top.p[k], bottom.p[k], dst.p[k]);
    return true;
}

}  // namespace

#define NVF_DEFINE(T)                                                                                           \
    bool field_view(const Plane<T>& p, int parity, FieldView<T>* v) { return field_view_t(p, parity, v); }     \
    bool field_view(const Plane<const T>& p, int parity, FieldView<const T>* v) {                              \
        return field_view_t(p, parity, v);                                                                      \
    }                                                                                                           \
    bool split(const Plane<const T>& src, const Plane<T>& top, const Plane<T>& bottom) {                       \
        return split_t(src, top, bottom);                                                                       \
    }                                                                                                           \
    bool join(const Plane<const T>& top, const Plane<const T>& bottom, const Plane<T>& dst) {                  \
        return join_t(top, bottom, dst);                                                                        \
    }                                                                                                           \
    bool join(const FieldView<const T>& top, const FieldView<const T>& bottom, const Plane<T>& dst) {          \
        return join_t(top, bottom, dst);                                                                        \
    }                                                                                                           \
    bool field_views(const Frame3<T>& f, FieldFrame3<T>* top, FieldFrame3<T>* bottom) {                        \
        return field_views_t(f, top, bottom);                                                                   \
    }                                                                                                           \
    bool field_views(const Frame3<const T>& f, FieldFrame3<const T>* top, FieldFrame3<const T>* bottom) {      \
        return field_views_t(f, top, bottom);                                                                   \
    }                                                                                                           \
    bool split_frame(const Frame3<const T>& src, const Frame3<T>& top, const Frame3<T>& bottom) {              \
        return split_frame_t(src, top, bottom);                                                                 \
    }                                                                                                           \
    bool join_frame(const Frame3<const T>& top, const Frame3<const T>& bottom, const Frame3<T>& dst) {         \
        return join_frame_t(top, bottom, dst);                                                                  \
    }                                                                                                           \
    bool join_frame(const FieldFrame3<const T>& top, const FieldFrame3<const T>& bottom, const Frame3<T>& dst) { \
        return join_frame_t(top, bottom, dst);                                                                  \
    }

NVF_DEFINE(uint8_t)
NVF_DEFINE(uint16_t)
NVF_DEFINE(float)

#undef NVF_DEFINE

}  // namespace nvfield
