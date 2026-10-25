// LZMA oracle: LzmaCompress as FUN_009d61f4 calls it (level 5, dict = len rounded up to 4 KiB (max 128 MiB),
// lc 3, lp 0, pb 2, fb 32, 1 thread); writes [srcLen u32][destLen u32][props 5][3 zero bytes][stream].
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LzmaLib.h"
int main(int argc, char **argv) {
  if (argc != 3) { fprintf(stderr, "lzo in out\n"); return 2; }
  FILE *f = fopen(argv[1], "rb"); fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
  unsigned char *src = malloc(n + 1); fread(src, 1, n, f); fclose(f);
  size_t cap = n * 2 + 0x90, dl = cap - 16, ps = 5;
  unsigned char *dst = calloc(1, cap);
  unsigned dict = (unsigned)((n + 0xfff) & 0xfffff000); if ((n + 0xfff) >> 27) dict = 0x8000000;
  int r = LzmaCompress(dst + 16, &dl, src, n, dst + 8, &ps, 5, dict, 3, 0, 2, 32, 1);
  if (r) { fprintf(stderr, "LzmaCompress %d\n", r); return 1; }
  unsigned a = (unsigned)n, b = (unsigned)dl; memcpy(dst, &a, 4); memcpy(dst + 4, &b, 4);
  f = fopen(argv[2], "wb"); fwrite(dst, 1, dl + 16, f); fclose(f);
  printf("%zu\n", dl + 16);
  return 0;
}
