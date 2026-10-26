// nv_lzma - thin wrapper over the LZMA SDK 9.20 C sources (native/lzma/, public domain) that reproduces
// repro/nv_lzma.py byte for byte: the genuine's FUN_009b41c4 / FUN_009d61f4 LZMA step of BuildNoiseModel.
//
//   plane_int16   FUN_009f0554: float32 -> int16 of rint(x * 16) (ties to even), saturated, NaN -> 0
//                 (row major, no row padding, little endian: n * 2 bytes)
//   dict_size_for FUN_009d61f4: (n + 0xfff) & ~0xfff, or 128 MiB when n + 0xfff >= 2^27
//   compress      LzmaCompress(level 5, dict_size_for(n), lc 3, lp 0, pb 2, fb 32, 1 thread) with an output
//                 buffer large enough for any stream -> props (5 bytes) + stream (nv_lzma.compress)
//   blob          FUN_009d61f4 into a buffer of n + 0x90 bytes (destLen = n + 0x80): the 16-byte header
//                 [n u32][stream len u32][props 5][3 bytes, 0 here (heap garbage in the genuine)] + stream,
//                 or false when LzmaCompress fails (SZ_ERROR_OUTPUT_EOF; nv_lzma.blob returns None)
//   blob_size     16 + stream length, or -1 (nv_lzma.blob_size)
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace nvlz {

uint32_t dict_size_for(size_t n);

std::vector<uint8_t> plane_int16(const float* plane, size_t n);

// Returns the SDK result code (0 = SZ_OK).  props receives 5 bytes, stream the encoded stream.
int compress(const uint8_t* src, size_t n, uint8_t props[5], std::vector<uint8_t>* stream);

// Returns false (and clears out) when the stream does not fit destLen = n + 0x80.
bool blob(const uint8_t* src, size_t n, std::vector<uint8_t>* out);

long long blob_size(const uint8_t* src, size_t n);

}  // namespace nvlz
