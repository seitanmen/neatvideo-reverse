// nv_quality_test - batch driver for the bit-exact verification against the Python port.
//
// Input file:  uint64 LE count N, then N x 0xc300-byte profile blobs.
// Output file: N records of {uint8 valid; float32 quality; float32 completeness}.

#include "nv_quality.h"

#include <cstdint>
#include <cstdio>
#include <vector>
#include <cstring>

int main(int argc, char** argv) {
    if (argc == 3 && std::strcmp(argv[1], "--debug") == 0) {
        std::FILE* f = std::fopen(argv[2], "rb");
        if (!f) { std::perror("open"); return 1; }
        std::vector<uint8_t> blob(static_cast<size_t>(nvq::PROF_SIZE), 0u);
        if (std::fread(blob.data(), 1, blob.size(), f) != blob.size()) return 1;
        std::fclose(f);
        float q = 0.0f, c = 0.0f;
        bool ok = nvq::quality_numbers(blob.data(), blob.size(), &q, &c);
        std::printf("valid=%d q=%a c=%a mode=%d W=%d H=%d cls=%d clips=%d,%d,%d\n", ok ? 1 : 0, q, c,
                    *reinterpret_cast<int32_t*>(&blob[nvq::OFF_MODE]),
                    *reinterpret_cast<int32_t*>(&blob[nvq::OFF_W]),
                    *reinterpret_cast<int32_t*>(&blob[nvq::OFF_H]),
                    *reinterpret_cast<int32_t*>(&blob[nvq::OFF_SIZECLS]),
                    blob[nvq::OFF_CLIPS], blob[nvq::OFF_CLIPS + 1], blob[nvq::OFF_CLIPS + 2]);
        for (int i = 0; i < 3; ++i) {
            float w[55];
            const int off = nvq::OFF_FACE3 + i * nvq::MODEL;
            for (int k = 0; k < 55; ++k) std::memcpy(&w[k], &blob[off + 4 * k], 4);
            std::printf("face%d flatness=%a\n", i, nvq::flatness(w, i == 0));
        }
        nvq::QualityParts part;
        if (nvq::quality_parts(blob.data(), blob.size(), &part)) {
            std::printf("u = %a %a %a\nP = %a\ns11 = %a\nS = %a\ng = %a (%08x)\nq = %a (%08x)\n",
                        part.u[0], part.u[1], part.u[2], part.P, part.s11, part.S,
                        part.g, *reinterpret_cast<uint32_t*>(&part.g),
                        part.q, *reinterpret_cast<uint32_t*>(&part.q));
        }
        return 0;
    }
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s [--debug <blob>] <blobs.in> <results.out>\n", argv[0]);
        return 2;
    }
    std::FILE* in = std::fopen(argv[1], "rb");
    if (!in) { std::perror("open in"); return 1; }
    uint64_t n = 0;
    if (std::fread(&n, 8, 1, in) != 1) { std::fprintf(stderr, "short header\n"); return 1; }
    std::vector<uint8_t> blob(static_cast<size_t>(nvq::PROF_SIZE), 0u);
    std::FILE* out = std::fopen(argv[2], "wb");
    if (!out) { std::perror("open out"); return 1; }
    for (uint64_t i = 0; i < n; ++i) {
        if (std::fread(blob.data(), 1, blob.size(), in) != blob.size()) {
            std::fprintf(stderr, "short blob %llu\n", (unsigned long long)i);
            return 1;
        }
        float q = 0.0f, c = 0.0f;
        const bool ok = nvq::quality_numbers(blob.data(), blob.size(), &q, &c);
        uint8_t v = ok ? 1 : 0;
        std::fwrite(&v, 1, 1, out);
        std::fwrite(&q, 4, 1, out);
        std::fwrite(&c, 4, 1, out);
    }
    std::fclose(in);
    std::fclose(out);
    return 0;
}
