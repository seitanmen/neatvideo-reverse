// depth - see depth.h.
#include "depth.h"

#include <cstring>
#include <regex>
#include <stdexcept>

#include "nvcore_c_depth.h"

namespace nvqt {

namespace {

int chroma_code(const std::string& c) { return c == "422" ? 422 : c == "444" ? 444 : 420; }

// the C ABI's restore arrays of a RestoreMap (pointers into it)
struct RestoreArgs {
    const long* idx[3] = {nullptr, nullptr, nullptr};
    const float* vals[3] = {nullptr, nullptr, nullptr};
    long n[3] = {0, 0, 0};
    bool any = false;
    explicit RestoreArgs(const RestoreMap* r) {
        if (!r) return;
        for (const auto& [k, rs] : *r)
            if (k >= 0 && k < 3 && !rs.idx.empty()) {
                idx[k] = rs.idx.data();
                vals[k] = rs.vals.data();
                n[k] = static_cast<long>(rs.idx.size());
                any = true;
            }
    }
};

}  // namespace

// ------------------------------------------------------------------ PixFmt
void PixFmt::plane_shapes(int W, int H, int rows[3], int cols[3]) const {
    rows[0] = H;
    cols[0] = W;
    if (chroma == "420") {
        rows[1] = rows[2] = (H + 1) / 2;
        cols[1] = cols[2] = (W + 1) / 2;
    } else if (chroma == "422") {
        rows[1] = rows[2] = H;
        cols[1] = cols[2] = (W + 1) / 2;
    } else {
        rows[1] = rows[2] = H;
        cols[1] = cols[2] = W;
    }
}

long long PixFmt::frame_bytes(int W, int H) const {
    int r[3], c[3];
    plane_shapes(W, H, r, c);
    long long n = 0;
    for (int i = 0; i < 3; ++i) n += static_cast<long long>(r[i]) * c[i];
    return n * itemsize;
}

long long PixFmt::internal_bytes(int W, int H) const {
    if (legacy()) return frame_bytes(W, H);
    const long long c = chroma == "420" ? static_cast<long long>((H + 1) / 2) * ((W + 1) / 2) : static_cast<long long>(H) * W;
    return 4 * (static_cast<long long>(H) * W + 2 * c);
}

PixFmt pixfmt(const std::string& name) {
    std::smatch m;
    if (std::regex_match(name, m, std::regex("yuv(420|422|444)p(?:(10|12|16)le)?"))) {
        const int bits = m[2].matched ? std::stoi(m[2].str()) : 8;
        if (bits == 8 && m[1].str() != "420")
            throw std::runtime_error("unsupported processing format " + name + " (8 bits is always yuv420p)");
        return PixFmt{name, "yuv", bits, false, m[1].str(), bits == 8 ? 1 : 2};
    }
    if (std::regex_match(name, m, std::regex("gbrp(10|12|16)le"))) return PixFmt{name, "rgb", std::stoi(m[1].str()), false, "444", 2};
    if (name == "gbrpf32le") return PixFmt{name, "rgb", 32, true, "444", 4};
    throw std::runtime_error("unsupported processing format " + name);
}

PixFmt choose_format(const SrcFmt& src, const std::string& depth) {
    std::string d = depth;
    if (d == "source") {
        const int b = src.bits;
        d = (src.is_float || b > 16) ? "32" : b <= 8 ? "8" : b <= 10 ? "10" : b <= 12 ? "12" : "16";
    }
    if (d == "8") return pixfmt("yuv420p");
    if (d == "32") return pixfmt("gbrpf32le");
    if (d == "10" || d == "12" || d == "16") return pixfmt(src.family == "rgb" ? "gbrp" + d + "le" : "yuv" + src.chroma + "p" + d + "le");
    throw std::runtime_error("unknown depth '" + depth + "' (8, 10, 12, 16, 32 or source)");
}

std::string describe(const PixFmt& f) {
    const std::string kind = std::to_string(f.bits) + "-bit" + (f.is_float ? " float" : "");
    const std::string ch = f.family == "rgb" ? "RGB" : std::string(1, f.chroma[0]) + ":" + f.chroma[1] + ":" + f.chroma[2];
    return f.name + " \xc2\xb7 " + kind + " " + ch;
}

bool full_range_internal(bool stream_full_range, const PixFmt& f) { return f.family == "rgb" ? true : stream_full_range; }

// ------------------------------------------------------------------ nv_depth conversions
Planes to_internal(const uint8_t* raw, const PixFmt& fmt, int W, int H, const std::string& matrix, RestoreMap* info) {
    int rows[3], cols[3];
    fmt.plane_shapes(W, H, rows, cols);
    Planes p;
    p.H = H;
    p.W = W;
    p.Hc = rows[1];
    p.Wc = fmt.chroma == "422" && !fmt.legacy() ? W : cols[1];
    p.Y.resize(static_cast<size_t>(H) * W);
    p.Cb.resize(static_cast<size_t>(p.Hc) * p.Wc);
    p.Cr.resize(p.Cb.size());
    std::vector<uint8_t> buf;
    const uint8_t* src = raw;
    if (fmt.is_float) {
        // denoise_video_v5.read_frames: nv_depth.sanitize of the raw float planes (in a copy), then to_internal
        buf.assign(raw, raw + fmt.frame_bytes(W, H));
        size_t o = 0;
        for (int k = 0; k < 3; ++k) {
            const long n = static_cast<long>(rows[k]) * cols[k];
            std::vector<long> idx(static_cast<size_t>(n));
            std::vector<float> vals(static_cast<size_t>(n));
            const long c = nvc_depth_sanitize(reinterpret_cast<float*>(buf.data() + o), n, idx.data(), vals.data());
            if (c && info) {
                idx.resize(static_cast<size_t>(c));
                vals.resize(static_cast<size_t>(c));
                (*info)[k] = Restore{std::move(idx), std::move(vals)};
            }
            o += static_cast<size_t>(n) * 4;
        }
        src = buf.data();
    }
    if (nvc_depth_to_internal(src, fmt.family == "rgb" ? 1 : 0, fmt.bits, fmt.is_float ? 1 : 0, chroma_code(fmt.chroma), W,
                              H, matrix_code(matrix), p.Y.data(), p.Cb.data(), p.Cr.data()) != 0)
        throw std::runtime_error("unsupported processing format " + fmt.name);
    return p;
}

std::vector<uint8_t> from_internal(const Planes& p, const PixFmt& fmt, const std::string& matrix, const RestoreMap* restore) {
    std::vector<uint8_t> out(static_cast<size_t>(fmt.frame_bytes(p.W, p.H)));
    const RestoreArgs ra(restore);
    if (nvc_depth_from_internal(p.Y.data(), p.Cb.data(), p.Cr.data(), fmt.family == "rgb" ? 1 : 0, fmt.bits, fmt.is_float ? 1 : 0,
                                chroma_code(fmt.chroma), p.W, p.H, matrix_code(matrix), ra.any ? ra.idx : nullptr,
                                ra.any ? ra.vals : nullptr, ra.any ? ra.n : nullptr, out.data()) != 0)
        throw std::runtime_error("unsupported processing format " + fmt.name);
    return out;
}

// ------------------------------------------------------------------ OFX bridge
long long ofx_frame_size(int W, int H, int depth) {
    const long long n = static_cast<long long>(W) * H + 2LL * (W / 2) * (H / 2);
    return n * (depth == 32 ? 4 : depth == 16 ? 2 : 1);
}

Planes ofx_unpack(const uint8_t* data, int W, int H, int depth, RestoreMap* restore) {
    Planes p;
    p.H = H;
    p.W = W;
    p.Hc = H / 2;
    p.Wc = W / 2;
    p.Y.resize(static_cast<size_t>(W) * H);
    p.Cb.resize(static_cast<size_t>(p.Wc) * p.Hc);
    p.Cr.resize(p.Cb.size());
    std::vector<long> idx[3];
    std::vector<float> vals[3];
    long* ip[3];
    float* vp[3];
    long rn[3] = {0, 0, 0};
    for (int k = 0; k < 3; ++k) {
        const size_t n = k == 0 ? p.Y.size() : p.Cb.size();
        idx[k].resize(depth == 32 ? n : 0);
        vals[k].resize(depth == 32 ? n : 0);
        ip[k] = idx[k].data();
        vp[k] = vals[k].data();
    }
    if (nvc_ofx_unpack(data, W, H, depth, p.Y.data(), p.Cb.data(), p.Cr.data(), depth == 32 ? ip : nullptr,
                       depth == 32 ? vp : nullptr, rn) != 0)
        throw std::runtime_error("--depth " + std::to_string(depth) + ": 16 or 32 (8 holds the uint8 planes)");
    if (restore)
        for (int k = 0; k < 3; ++k)
            if (rn[k]) {
                idx[k].resize(static_cast<size_t>(rn[k]));
                vals[k].resize(static_cast<size_t>(rn[k]));
                (*restore)[k] = Restore{std::move(idx[k]), std::move(vals[k])};
            }
    return p;
}

std::vector<uint8_t> ofx_pack(const Planes& p, int depth, const RestoreMap* restore) {
    std::vector<uint8_t> out(static_cast<size_t>(ofx_frame_size(p.W, p.H, depth)));
    const RestoreArgs ra(restore);
    if (nvc_ofx_pack(p.Y.data(), p.Cb.data(), p.Cr.data(), p.W, p.H, depth, ra.any ? ra.idx : nullptr,
                     ra.any ? ra.vals : nullptr, ra.any ? ra.n : nullptr, out.data()) != 0)
        throw std::runtime_error("--depth " + std::to_string(depth) + ": 8, 16 or 32");
    return out;
}

}  // namespace nvqt
