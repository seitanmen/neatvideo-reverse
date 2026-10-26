// nv_lzma - see nv_lzma.h.  The encoder itself is the unmodified LZMA SDK 9.20 (native/lzma/, SOURCE.md).
#include "nv_lzma.h"

#include <cmath>
#include <cstring>
#include <stdexcept>

#include "lzma/LzmaLib.h"

namespace nvlz {

uint32_t dict_size_for(size_t n) {
    const uint64_t m = static_cast<uint64_t>(n) + 0xfffu;
    uint32_t d = static_cast<uint32_t>(m & 0xfffff000u);
    if (m >> 0x1b) d = 0x8000000u;
    return d;
}

std::vector<uint8_t> plane_int16(const float* plane, size_t n) {
    std::vector<uint8_t> out(n * 2);
    for (size_t i = 0; i < n; ++i) {
        const float x = plane[i] * 16.0f;      // float32 product (overflow -> inf, as numpy)
        float r;
        if (std::isnan(x)) r = 0.0f;
        else if (std::isinf(x)) r = x > 0.0f ? 32767.0f : -32768.0f;
        else r = std::rint(x);                 // round half to even (default rounding mode), like np.rint
        if (r < -32768.0f) r = -32768.0f;
        if (r > 32767.0f) r = 32767.0f;
        const int16_t v = static_cast<int16_t>(static_cast<int>(r));
        const uint16_t u = static_cast<uint16_t>(v);
        out[2 * i] = static_cast<uint8_t>(u & 0xffu);
        out[2 * i + 1] = static_cast<uint8_t>(u >> 8);
    }
    return out;
}

static int call(uint8_t* dest, size_t* dest_len, const uint8_t* src, size_t n, uint8_t* props) {
    size_t ps = 5;
    // n == 0: an empty vector may give a null data pointer; pass a valid one (srcLen 0, never read).
    static const uint8_t dummy = 0;
    return LzmaCompress(dest, dest_len, n ? src : &dummy, n, props, &ps, 5, dict_size_for(n), 3, 0, 2, 32, 1);
}

int compress(const uint8_t* src, size_t n, uint8_t props[5], std::vector<uint8_t>* stream) {
    // An LZMA stream never exceeds the input by more than a few percent; 2n + 0x90 as the C oracle (lzo.c).
    std::vector<uint8_t> buf(n * 2 + 0x90);
    size_t dl = buf.size();
    const int r = call(buf.data(), &dl, src, n, props);
    if (r) {
        stream->clear();
        return r;
    }
    buf.resize(dl);
    *stream = std::move(buf);
    return 0;
}

bool blob(const uint8_t* src, size_t n, std::vector<uint8_t>* out) {
    std::vector<uint8_t> buf(n + 0x90, 0u);    // the caller's capacity W * h * 2 + 0x90
    size_t dl = n + 0x80;                      // FUN_009d61f4: destLen = length + 0x80
    const int r = call(buf.data() + 16, &dl, src, n, buf.data() + 8);
    if (r) {
        out->clear();
        return false;
    }
    const uint32_t a = static_cast<uint32_t>(n), b = static_cast<uint32_t>(dl);
    for (int i = 0; i < 4; ++i) {
        buf[i] = static_cast<uint8_t>(a >> (8 * i));
        buf[4 + i] = static_cast<uint8_t>(b >> (8 * i));
    }
    buf.resize(16 + dl);
    *out = std::move(buf);
    return true;
}

long long blob_size(const uint8_t* src, size_t n) {
    std::vector<uint8_t> b;
    return blob(src, n, &b) ? static_cast<long long>(b.size()) : -1;
}

}  // namespace nvlz
