// nv_fields_test - the checks of nv_fields (INT-1), run by native/verify_fields.py.
//
//   nv_fields_test selftest [seed]
//       planes of uint8 / uint16 / float32 (random bit patterns: NaN payloads, -0, inf, denormals) for many W x H
//       (even, odd, 0, 1, 2, ...) and row strides, every field row against the frame row it must be (written out
//       here, not through nv_fields' own field_src_row), split -> join and join of the views back to the same bytes,
//       the bytes around the planes (stride gaps, a guard row) untouched, the 4:2:0 / 4:4:4 frames, bad shapes refused.
//   nv_fields_test raw FMT W H N in.raw top.raw bottom.raw joined.raw
//       N frames of FMT (u8_420 / u16_420 / f32_420 / u8_444 / u16_444 / f32_444; planes Y, Cb, Cr packed as
//       ffmpeg's rawvideo) from in.raw: the two fields of each frame (split_frame; the fields as packed frames of
//       their own) into top.raw / bottom.raw and the frame joined back from them into joined.raw.  The join of the
//       views (no copy) must also give the input bytes: the exit code is 1 otherwise.
// stdout: one summary line ("checks N failures M").
#include "nv_fields.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

using namespace nvfield;

namespace {

long checks = 0, failures = 0;

void check(bool ok, const char* what, int W, int H, int extra = -1) {
    ++checks;
    if (ok) return;
    if (++failures <= 20) std::fprintf(stderr, "FAIL %s (W %d H %d %d)\n", what, W, H, extra);
}

// the frame row of field row i (written out independently of nv_fields.cpp): top 2i, bottom 2i + 1, the bottom
// field's pad row of an odd H repeats frame row H - 2 (H == 1: row 0)
int expect_row(int H, int parity, int i) {
    if (parity == 0) return 2 * i;
    if (2 * i + 1 <= H - 1) return 2 * i + 1;
    return H == 1 ? 0 : H - 2;
}

template <class T>
T sentinel() {
    T s;
    std::memset(&s, 0xA5, sizeof(T));
    return s;
}

// a plane buffer: H rows of `stride` elements plus one guard row, everything outside the W x H plane = sentinel
template <class T>
struct Buf {
    std::vector<T> v;
    Plane<T> p;
    Buf(int W, int H, int gap) {
        const std::ptrdiff_t st = W + gap;
        v.assign(static_cast<std::size_t>(st) * (H + 1) + 1, sentinel<T>());
        p.data = v.data();
        p.W = W;
        p.H = H;
        p.stride = st;
    }
    Plane<const T> c() const { return Plane<const T>{p.data, p.W, p.H, p.stride}; }
    void fill(std::mt19937_64& rng) {
        for (int y = 0; y < p.H; ++y)
            for (int x = 0; x < p.W; ++x) {
                const uint64_t r = rng();
                std::memcpy(p.row(y) + x, &r, sizeof(T));
            }
    }
    bool same_plane(const Buf& o) const {     // the W x H elements, byte for byte
        for (int y = 0; y < p.H; ++y)
            if (p.W && std::memcmp(p.row(y), o.p.row(y), sizeof(T) * p.W) != 0) return false;
        return true;
    }
    bool outside_untouched() const {          // stride gaps and the guard row still hold the sentinel
        const T s = sentinel<T>();
        for (std::size_t i = 0; i < v.size(); ++i) {
            if (p.stride == 0) {
                if (std::memcmp(&v[i], &s, sizeof(T)) != 0) return false;
                continue;
            }
            const std::ptrdiff_t y = static_cast<std::ptrdiff_t>(i) / p.stride, x = static_cast<std::ptrdiff_t>(i) % p.stride;
            if (y < p.H && x < p.W) continue;
            if (std::memcmp(&v[i], &s, sizeof(T)) != 0) return false;
        }
        return true;
    }
};

template <class T>
bool row_is(const T* a, const T* b, int W) {
    return W == 0 || std::memcmp(a, b, sizeof(T) * W) == 0;
}

template <class T>
void plane_case(int W, int H, int gap, std::mt19937_64& rng) {
    Buf<T> src(W, H, gap);
    src.fill(rng);
    const int fh = (H + 1) / 2;
    // views
    FieldView<const T> v[2];
    for (int p = 0; p < 2; ++p) {
        check(field_view(src.c(), p, &v[p]), "field_view", W, H, p);
        check(v[p].H == fh && v[p].W == W, "view shape", W, H, p);
        check(v[p].real == (p == 0 ? (H + 1) / 2 : H / 2), "view real rows", W, H, p);
        for (int i = 0; i < fh; ++i)
            check(row_is(v[p].row(i), src.c().row(expect_row(H, p, i)), W), "view row", W, H, i);
    }
    // split (copy) into planes with another stride
    Buf<T> top(W, fh, gap + 1), bot(W, fh, gap + 2);
    check(split(src.c(), top.p, bot.p), "split", W, H);
    for (int i = 0; i < fh; ++i) {
        check(row_is(top.p.row(i), src.c().row(expect_row(H, 0, i)), W), "split top row", W, H, i);
        check(row_is(bot.p.row(i), src.c().row(expect_row(H, 1, i)), W), "split bottom row", W, H, i);
    }
    check(top.outside_untouched() && bot.outside_untouched(), "split outside", W, H);
    // join of the copies
    Buf<T> dst(W, H, gap);
    check(join(top.c(), bot.c(), dst.p), "join", W, H);
    check(dst.same_plane(src), "join == frame", W, H);
    check(dst.outside_untouched(), "join outside", W, H);
    // join of the views (no copy)
    Buf<T> dst2(W, H, gap + 3);
    check(join(v[0], v[1], dst2.p), "join views", W, H);
    check(dst2.same_plane(src) && dst2.outside_untouched(), "join views == frame", W, H);
    // join of views of dst itself (in place: nothing changes)
    FieldView<const T> w[2];
    field_view(dst.c(), 0, &w[0]);
    field_view(dst.c(), 1, &w[1]);
    check(join(w[0], w[1], dst.p) && dst.same_plane(src) && dst.outside_untouched(), "join in place", W, H);
    // bad shapes: refused, nothing written
    if (W > 0 && H > 0) {
        Buf<T> d3(W, H, gap);
        Buf<T> shortf(W, fh - 1, gap), wide(W + 1, fh, gap);
        check(!join(shortf.c(), bot.c(), d3.p) && !join(top.c(), wide.c(), d3.p), "join refuses", W, H);
        check(!split(src.c(), shortf.p, bot.p) && !split(src.c(), top.p, wide.p), "split refuses", W, H);
        check(d3.outside_untouched() && shortf.outside_untouched() && wide.outside_untouched(), "refused: untouched", W, H);
        const T s = sentinel<T>();
        bool clean = true;
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x) clean &= std::memcmp(d3.p.row(y) + x, &s, sizeof(T)) == 0;
        check(clean, "refused join: frame untouched", W, H);
        FieldView<const T> bad;
        check(!field_view(src.c(), 2, &bad) && !field_view(Plane<const T>{src.p.data, W, H, W - 1}, 0, &bad),
              "field_view refuses", W, H);
        // shared memory (nv_fields.h, join): the two fields packed into the halves of the frame buffer are refused
        // and leave it untouched (writing frame row 1 would overwrite top field row 1 before it is read)
        if (H >= 2) {
            Buf<T> pk(W, 2 * fh, gap);           // rows [0, fh) top, [fh, 2 fh) bottom; the frame is its first H rows
            for (int i = 0; i < fh; ++i) {
                std::memcpy(pk.p.row(i), top.p.row(i), sizeof(T) * W);
                std::memcpy(pk.p.row(fh + i), bot.p.row(i), sizeof(T) * W);
            }
            const std::vector<T> before = pk.v;
            const Plane<const T> ph{pk.p.data, W, fh, pk.p.stride}, pb{pk.p.data + fh * pk.p.stride, W, fh, pk.p.stride};
            const Plane<T> frame{pk.p.data, W, H, pk.p.stride};
            check(!join(ph, pb, frame) && std::memcmp(before.data(), pk.v.data(), sizeof(T) * before.size()) == 0,
                  "join refuses packed halves", W, H);
            // a field that reads a frame plane it does not belong to (top field = another view of dst's rows)
            Buf<T> d4(W, H, gap);
            const Plane<const T> wrong{d4.p.data + d4.p.stride, W, fh, 2 * d4.p.stride};   // (odd H: its last row is the guard row)
            check(H < 3 || !join(wrong, bot.c(), d4.p), "join refuses a shifted view of dst", W, H);
            // split into memory that overlaps the source, or top and bottom overlapping each other
            Buf<T> s2(W, H, gap);
            s2.fill(rng);
            const std::vector<T> s2b = s2.v;
            check(!split(s2.c(), Plane<T>{s2.p.data, W, fh, 2 * s2.p.stride}, bot.p) &&
                      !split(src.c(), top.p, Plane<T>{top.p.data, W, fh, top.p.stride}) &&
                      std::memcmp(s2b.data(), s2.v.data(), sizeof(T) * s2b.size()) == 0,
                  "split refuses overlap", W, H);
        }
    }
    // planes without elements may have no data (and any stride): nothing is computed from the null pointer
    if (W == 0) {
        const Plane<const T> np{nullptr, 0, H, 5};
        FieldView<const T> nv0, nv1;
        check(field_view(np, 0, &nv0) && field_view(np, 1, &nv1) && !nv0.data && !nv0.pad && !nv1.data && !nv1.pad &&
                  nv0.H == fh && nv1.real == H / 2 && nv0.stride == 0,
              "W 0 null view", W, H);
        const Plane<T> nt{nullptr, 0, fh, 3}, nb{nullptr, 0, fh, 0}, nd{nullptr, 0, H, 7};
        check(split(np, nt, nb) && join(Plane<const T>{nullptr, 0, fh, 3}, Plane<const T>{nullptr, 0, fh, 0}, nd) &&
                  join(nv0, nv1, nd),
              "W 0 null split / join", W, H);
    }
}

// a three-plane frame, chroma 4:2:0 (ceil(W/2) x ceil(H/2)) or 4:4:4
template <class T>
void frame_case(int W, int H, bool c420, int gap, std::mt19937_64& rng) {
    const int cw = c420 ? (W + 1) / 2 : W, ch = c420 ? (H + 1) / 2 : H;
    std::vector<Buf<T>> src, top, bot, dst, dst2;
    for (int k = 0; k < 3; ++k) {
        const int w = k ? cw : W, h = k ? ch : H, fh = (h + 1) / 2;
        src.emplace_back(w, h, gap);
        src.back().fill(rng);
        top.emplace_back(w, fh, gap + 1);
        bot.emplace_back(w, fh, gap);
        dst.emplace_back(w, h, gap + 2);
        dst2.emplace_back(w, h, gap);
    }
    Frame3<const T> s;
    Frame3<T> t, b, d, d2;
    for (int k = 0; k < 3; ++k) {
        s.p[k] = src[k].c();
        t.p[k] = top[k].p;
        b.p[k] = bot[k].p;
        d.p[k] = dst[k].p;
        d2.p[k] = dst2[k].p;
    }
    if (c420) {   // the field is a 4:2:0 picture itself
        int fw, fhh, fcw, fch;
        field_shape_420(W, H, &fw, &fhh, &fcw, &fch);
        check(fw == W && fhh == top[0].p.H && fcw == top[1].p.W && fch == top[1].p.H && fch == (fhh + 1) / 2,
              "field_shape_420", W, H);
    }
    check(split_frame(s, t, b), "split_frame", W, H);
    for (int k = 0; k < 3; ++k) {
        const int h = src[k].p.H;
        for (int i = 0; i < top[k].p.H; ++i) {
            check(row_is(top[k].p.row(i), src[k].c().row(expect_row(h, 0, i)), src[k].p.W), "frame top row", W, H, k);
            check(row_is(bot[k].p.row(i), src[k].c().row(expect_row(h, 1, i)), src[k].p.W), "frame bottom row", W, H, k);
        }
    }
    Frame3<const T> tc, bc;
    for (int k = 0; k < 3; ++k) {
        tc.p[k] = top[k].c();
        bc.p[k] = bot[k].c();
    }
    check(join_frame(tc, bc, d), "join_frame", W, H);
    FieldFrame3<const T> vt, vb;
    check(field_views(s, &vt, &vb) && join_frame(vt, vb, d2), "join_frame views", W, H);
    for (int k = 0; k < 3; ++k) {
        check(dst[k].same_plane(src[k]) && dst[k].outside_untouched(), "join_frame == frame", W, H, k);
        check(dst2[k].same_plane(src[k]) && dst2[k].outside_untouched(), "join_frame views == frame", W, H, k);
        for (int i = 0; i < vb.p[k].H; ++i)
            check(row_is(vb.p[k].row(i), src[k].c().row(expect_row(src[k].p.H, 1, i)), src[k].p.W), "frame view row",
                  W, H, k);
    }
    // join_frame through dst's own field views (in place) is allowed; a field plane inside another frame plane is not
    Frame3<const T> dc;
    for (int k = 0; k < 3; ++k) dc.p[k] = dst[k].c();
    FieldFrame3<const T> wt, wb;
    field_views(dc, &wt, &wb);
    check(join_frame(wt, wb, d), "join_frame in place", W, H);
    for (int k = 0; k < 3; ++k) check(dst[k].same_plane(src[k]) && dst[k].outside_untouched(), "join_frame in place == frame", W, H, k);
    if (W > 0 && H > 0) {
        FieldFrame3<const T> xt = wt;
        xt.p[1] = wt.p[0];                   // Cb's top field read from the luma plane being written
        xt.p[1].W = wt.p[1].W;
        xt.p[1].H = wt.p[1].H;
        xt.p[1].real = wt.p[1].real;
        check(!join_frame(xt, wb, d), "join_frame refuses a field in another frame plane", W, H);
    }
}

template <class T>
void all_cases(std::mt19937_64& rng) {
    const int Ws[] = {0, 1, 2, 3, 5, 8, 17, 64};
    for (int H = 0; H <= 33; ++H)
        for (int W : Ws)
            for (int gap : {0, 3}) {
                plane_case<T>(W, H, gap, rng);
                frame_case<T>(W, H, true, gap, rng);
                frame_case<T>(W, H, false, gap, rng);
            }
    const int big[][2] = {{1920, 1080}, {1920, 1081}, {720, 576}, {720, 486}, {720, 480}, {1, 1081}, {1081, 1}, {641, 361}};
    for (const auto& s : big) {
        plane_case<T>(s[0], s[1], 0, rng);
        plane_case<T>(s[0], s[1], 7, rng);
        frame_case<T>(s[0], s[1], true, 0, rng);
        frame_case<T>(s[0], s[1], false, 5, rng);
    }
}

int selftest(uint64_t seed) {
    std::mt19937_64 rng(seed);
    all_cases<uint8_t>(rng);
    all_cases<uint16_t>(rng);
    all_cases<float>(rng);
    // field_src_row against the written-out rule
    for (int H = 1; H <= 64; ++H)
        for (int p = 0; p < 2; ++p)
            for (int i = 0; i < field_rows(H); ++i) check(field_src_row(H, p, i) == expect_row(H, p, i), "field_src_row", 0, H, i);
    std::printf("checks %ld failures %ld\n", checks, failures);
    return failures ? 1 : 0;
}

template <class T>
int raw_run(bool c420, int W, int H, long N, std::FILE* in, std::FILE* ft, std::FILE* fb, std::FILE* fj) {
    const int cw = c420 ? (W + 1) / 2 : W, ch = c420 ? (H + 1) / 2 : H;
    const int ws[3] = {W, cw, cw}, hs[3] = {H, ch, ch};
    std::vector<T> src[3], top[3], bot[3], dst[3], dst2[3];
    Frame3<const T> s;
    Frame3<T> t, b, d, d2;
    Frame3<const T> tc, bc;
    for (int k = 0; k < 3; ++k) {
        const int fh = field_rows(hs[k]);
        src[k].resize(static_cast<std::size_t>(ws[k]) * hs[k]);
        dst[k].resize(src[k].size());
        dst2[k].resize(src[k].size());
        top[k].resize(static_cast<std::size_t>(ws[k]) * fh);
        bot[k].resize(top[k].size());
        s.p[k] = {src[k].data(), ws[k], hs[k], ws[k]};
        t.p[k] = {top[k].data(), ws[k], fh, ws[k]};
        b.p[k] = {bot[k].data(), ws[k], fh, ws[k]};
        d.p[k] = {dst[k].data(), ws[k], hs[k], ws[k]};
        d2.p[k] = {dst2[k].data(), ws[k], hs[k], ws[k]};
        tc.p[k] = {top[k].data(), ws[k], fh, ws[k]};
        bc.p[k] = {bot[k].data(), ws[k], fh, ws[k]};
    }
    for (long n = 0; n < N; ++n) {
        for (int k = 0; k < 3; ++k)
            if (std::fread(src[k].data(), sizeof(T), src[k].size(), in) != src[k].size()) {
                std::fprintf(stderr, "short input at frame %ld\n", n);
                return 2;
            }
        check(split_frame(s, t, b), "raw split_frame", W, H, static_cast<int>(n));
        check(join_frame(tc, bc, d), "raw join_frame", W, H, static_cast<int>(n));
        FieldFrame3<const T> vt, vb;
        check(field_views(s, &vt, &vb) && join_frame(vt, vb, d2), "raw join views", W, H, static_cast<int>(n));
        for (int k = 0; k < 3; ++k) {
            check(std::memcmp(dst2[k].data(), src[k].data(), sizeof(T) * src[k].size()) == 0, "raw views == frame", W, H, k);
            std::fwrite(top[k].data(), sizeof(T), top[k].size(), ft);
        }
        for (int k = 0; k < 3; ++k) std::fwrite(bot[k].data(), sizeof(T), bot[k].size(), fb);
        for (int k = 0; k < 3; ++k) std::fwrite(dst[k].data(), sizeof(T), dst[k].size(), fj);
    }
    std::printf("checks %ld failures %ld\n", checks, failures);
    return failures ? 1 : 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc >= 2 && std::strcmp(argv[1], "selftest") == 0)
        return selftest(argc >= 3 ? std::strtoull(argv[2], nullptr, 0) : 20261031ULL);
    if (argc == 10 && std::strcmp(argv[1], "raw") == 0) {
        const std::string fmt = argv[2];
        const int W = std::atoi(argv[3]), H = std::atoi(argv[4]);
        const long N = std::atol(argv[5]);
        std::FILE* in = std::fopen(argv[6], "rb");
        std::FILE* ft = std::fopen(argv[7], "wb");
        std::FILE* fb = std::fopen(argv[8], "wb");
        std::FILE* fj = std::fopen(argv[9], "wb");
        if (!in || !ft || !fb || !fj || W < 0 || H < 0 || N < 0) {
            std::fprintf(stderr, "cannot open the files or bad size\n");
            return 2;
        }
        const bool c420 = fmt.size() > 4 && fmt.compare(fmt.size() - 4, 4, "_420") == 0;
        int r = 2;
        if (fmt == "u8_420" || fmt == "u8_444") r = raw_run<uint8_t>(c420, W, H, N, in, ft, fb, fj);
        else if (fmt == "u16_420" || fmt == "u16_444") r = raw_run<uint16_t>(c420, W, H, N, in, ft, fb, fj);
        else if (fmt == "f32_420" || fmt == "f32_444") r = raw_run<float>(c420, W, H, N, in, ft, fb, fj);
        else std::fprintf(stderr, "unknown format %s\n", fmt.c_str());
        std::fclose(in);
        std::fclose(ft);
        std::fclose(fb);
        std::fclose(fj);
        return r;
    }
    std::fprintf(stderr,
                 "usage: %s selftest [seed]\n       %s raw u8_420|u16_420|f32_420|u8_444|u16_444|f32_444 W H N in top "
                 "bottom joined\n",
                 argv[0], argv[0]);
    return 2;
}
