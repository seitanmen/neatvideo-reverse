// nv_fields - the field split / join of interlaced frames (INT-1, .claude/tasks/interlace.md): a frame plane of H rows
// is two fields, top = the frame rows 0, 2, 4, ... and bottom = the rows 1, 3, 5, ...  Both fields are ceil(H/2) rows
// high.  Only bytes move (memcpy / memmove per row), so every element type round-trips bit for bit (NaN payloads,
// -0, denormals included); native/verify_fields.py checks it.
//
// Odd H: the top field has ceil(H/2) frame rows, the bottom field floor(H/2) frame rows and one pad row at the end
// that repeats its own last row (frame row H - 2: edge replication inside the field, one instant of time), or frame
// row 0 when H == 1 (the bottom field has no row of its own).  join() writes back only the frame rows; the pad row is
// never written into the frame.
//
// Chroma: every plane is split by its own row parity.  For 4:2:0 that is the interlaced chroma layout (chroma row c
// belongs to field c & 1 and covers the luma rows of that field, MPEG-2 / H.264 field pictures; ffmpeg's decoders
// return interlaced 4:2:0 frames this way and its tinterlace / separatefields filters split every plane by row
// parity).  A field of a W x H 4:2:0 frame (chroma ceil(W/2) x ceil(H/2)) is again a 4:2:0 picture: luma W x ceil(H/2),
// chroma ceil(W/2) x ceil(ceil(H/2)/2).  4:4:4 and the widened 4:2:2 planes (nv_depth: H chroma rows) split the same
// way.  The genuine's own chroma handling of fields is not decoded (docs/INTERLACE_PROGRESSIVE.md).
//
// The views (field_view) do not copy: data + parity * stride, stride * 2 and, for the pad row, a pointer to the frame
// row it repeats (so with an odd H the bottom field's last row is not at data + i * stride: a kernel that takes only a
// pointer and a stride needs field_real_rows rows, or a copy by split).
#pragma once

#include <cstddef>
#include <cstdint>

namespace nvfield {

enum { TOP = 0, BOTTOM = 1 };

// The height of both fields of an H-row plane: ceil(H/2) (0 for H <= 0)
inline int field_rows(int H) { return H <= 0 ? 0 : (H + 1) / 2; }
// The frame rows of field `parity` (rows parity, parity + 2, ... < H): ceil(H/2) top, floor(H/2) bottom
inline int field_real_rows(int H, int parity) { return H <= 0 ? 0 : (H + 1 - parity) / 2; }
// The frame row field row i (0 <= i < field_rows(H)) of field `parity` reads: 2i + parity, or for the pad row of the
// bottom field (odd H) H - 2, or 0 when H == 1
int field_src_row(int H, int parity, int i);

// A plane: W x H elements, row y at data + y * stride (stride in elements, >= W)
template <class T>
struct Plane {
    T* data = nullptr;
    int W = 0, H = 0;
    std::ptrdiff_t stride = 0;
    T* row(int y) const { return data + y * stride; }
};

// A field of a plane without a copy: W x H (= field_rows of the frame), row i < real at data + i * stride (stride =
// 2 * the frame's stride), the pad row (i == real < H: bottom field, odd frame height) at pad
template <class T>
struct FieldView {
    T* data = nullptr;
    int W = 0, H = 0, real = 0;
    std::ptrdiff_t stride = 0;
    T* pad = nullptr;
    T* row(int i) const { return i < real ? data + i * stride : pad; }
};

// The three planes of a frame (Y, Cb, Cr; chroma 4:2:0 ceil(W/2) x ceil(H/2), or 4:4:4 W x H), each split by its own
// row parity (see the top of this file)
template <class T>
struct Frame3 {
    Plane<T> p[3];
};
template <class T>
struct FieldFrame3 {
    FieldView<T> p[3];
};

// The functions below exist for T = uint8_t, uint16_t and float (plain overloads, not templates: strong symbols).
// All return false and write nothing on a bad argument.  A plane with W == 0 or H == 0 may have data == nullptr.
//
// field_view: the field `parity` of plane p.  False (v untouched) for a bad plane (W < 0, H < 0, stride < W, no data
// with elements) or parity other than TOP / BOTTOM.  A view of a plane without elements has data = pad = nullptr and
// stride 0.
//
// split: the two fields of src copied into top and bottom (each W == src.W, H == field_rows(src.H), stride >= W).
// False on a shape mismatch, or when the memory of src, top and bottom overlaps (rows are compared as byte ranges
// [first row, last row + W): planes interleaved inside each other's stride gaps are refused too).
//
// join: the frame dst (dst.H rows) from its two fields (each W == dst.W, at least the frame rows of its parity:
// H >= field_rows(dst.H) for planes, real >= field_real_rows(dst.H, parity) for views; the bottom field's pad row and
// any rows past the frame are not read).  A field may share memory with dst only as the field_view of dst itself
// (data == dst.data + parity * dst.stride and stride == 2 * dst.stride: every row is then already in place); any other
// overlap (for example the two fields packed into the two halves of dst) is refused, because writing the frame rows in
// order would overwrite field rows before they are read.
//
// field_views / split_frame / join_frame: the same per plane of a Frame3.  split_frame refuses any overlap between
// the planes read and written, join_frame any overlap of a field plane with a frame plane other than the identity
// above (field k of frame plane k).
#define NVF_DECLARE(T)                                                                                          \
    bool field_view(const Plane<T>& p, int parity, FieldView<T>* v);                                           \
    bool field_view(const Plane<const T>& p, int parity, FieldView<const T>* v);                               \
    bool split(const Plane<const T>& src, const Plane<T>& top, const Plane<T>& bottom);                        \
    bool join(const Plane<const T>& top, const Plane<const T>& bottom, const Plane<T>& dst);                   \
    bool join(const FieldView<const T>& top, const FieldView<const T>& bottom, const Plane<T>& dst);           \
    bool field_views(const Frame3<T>& f, FieldFrame3<T>* top, FieldFrame3<T>* bottom);                         \
    bool field_views(const Frame3<const T>& f, FieldFrame3<const T>* top, FieldFrame3<const T>* bottom);       \
    bool split_frame(const Frame3<const T>& src, const Frame3<T>& top, const Frame3<T>& bottom);               \
    bool join_frame(const Frame3<const T>& top, const Frame3<const T>& bottom, const Frame3<T>& dst);          \
    bool join_frame(const FieldFrame3<const T>& top, const FieldFrame3<const T>& bottom, const Frame3<T>& dst);
NVF_DECLARE(uint8_t)
NVF_DECLARE(uint16_t)
NVF_DECLARE(float)
#undef NVF_DECLARE

// The plane shapes of one field of a 4:2:0 frame W x H: luma W x field_rows(H), chroma ceil(W/2) x
// field_rows(ceil(H/2)) (== ceil(field_rows(H) / 2): the field is a 4:2:0 picture itself)
inline void field_shape_420(int W, int H, int* fw, int* fh, int* fcw, int* fch) {
    *fw = W;
    *fh = field_rows(H);
    *fcw = (W + 1) / 2;
    *fch = field_rows((H + 1) / 2);
}

}  // namespace nvfield
