// nv_lzma_test - batch driver for the byte-exact verification against repro/nv_lzma.py.
//
// Input file:  uint64 LE count N, then N records {uint8 kind; uint64 count; payload}
//              kind 0: count raw bytes (the source of compress / blob)
//              kind 1: count float32 values (a plane; the source is plane_int16 of it)
// Output file: N records {[kind 1: uint64 n16; n16 bytes of plane_int16]
//                         int32 rc; props 5; uint64 slen; slen stream bytes   (compress, unlimited output)
//                         int64 bsize; bsize bytes when bsize > 0}            (blob, -1 on overflow)
#include "nv_lzma.h"

#include <cstdint>
#include <cstdio>
#include <vector>

static bool rd(std::FILE* f, void* p, size_t n) { return n == 0 || std::fread(p, 1, n, f) == n; }
static void wr(std::FILE* f, const void* p, size_t n) {
    if (n) std::fwrite(p, 1, n, f);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s <records.in> <results.out>\n", argv[0]);
        return 2;
    }
    std::FILE* in = std::fopen(argv[1], "rb");
    if (!in) { std::perror("open in"); return 1; }
    std::FILE* out = std::fopen(argv[2], "wb");
    if (!out) { std::perror("open out"); return 1; }
    uint64_t n = 0;
    if (!rd(in, &n, 8)) { std::fprintf(stderr, "short header\n"); return 1; }
    for (uint64_t i = 0; i < n; ++i) {
        uint8_t kind = 0;
        uint64_t count = 0;
        if (!rd(in, &kind, 1) || !rd(in, &count, 8)) { std::fprintf(stderr, "short record %llu\n", (unsigned long long)i); return 1; }
        std::vector<uint8_t> src;
        if (kind == 1) {
            std::vector<float> plane(static_cast<size_t>(count));
            if (!rd(in, plane.data(), plane.size() * 4)) { std::fprintf(stderr, "short plane\n"); return 1; }
            src = nvlz::plane_int16(plane.data(), plane.size());
            const uint64_t n16 = src.size();
            wr(out, &n16, 8);
            wr(out, src.data(), src.size());
        } else {
            src.resize(static_cast<size_t>(count));
            if (!rd(in, src.data(), src.size())) { std::fprintf(stderr, "short bytes\n"); return 1; }
        }
        uint8_t props[5] = {0, 0, 0, 0, 0};
        std::vector<uint8_t> stream;
        const int32_t rc = nvlz::compress(src.data(), src.size(), props, &stream);
        const uint64_t slen = stream.size();
        wr(out, &rc, 4);
        wr(out, props, 5);
        wr(out, &slen, 8);
        wr(out, stream.data(), stream.size());
        std::vector<uint8_t> b;
        const int64_t bsize = nvlz::blob(src.data(), src.size(), &b) ? static_cast<int64_t>(b.size()) : -1;
        wr(out, &bsize, 8);
        wr(out, b.data(), b.size());
    }
    std::fclose(in);
    if (std::fclose(out) != 0) { std::perror("close out"); return 1; }
    return 0;
}
