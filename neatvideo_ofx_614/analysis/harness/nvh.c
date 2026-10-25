// Harness that calls internal functions of the genuine Neat Video 6 OFX bundle (arm64)
// on float planes read from disk. Offsets are Ghidra addresses (image vmaddr 0) of the build
// with LC_UUID A9DF3875-4B77-31EB-B8C6-543A30F21C9F (checked by nvh_guard.h).
// Build: clang -O1 -arch arm64 -framework CoreFoundation -o nvh nvh.c   (see Makefile)
//   nvh selftest                    -> calls the genuine assertion handler; must exit with code 3
//   nvh order                       -> did the guard run before the bundle's initialisers?
//   nvh info                        -> read-only constants (thread count, SIMD flag, tables) and
//                                      the AutoFineTune entry condition (not bypassed)
//   nvh levels W H plane.f32        -> FUN_009980e8 (5-level band noise measure, FindFlatArea)
//   nvh model  W H plane.f32 [set]  -> FUN_00995250 (BuildNoiseModel per-plane model, 5 levels)
//                                      both print the 0x4000-byte struct as "index %.9g %08x" lines
//                                      (value, then its bit pattern)
//   nvh profile W H rgb.f32 mode out.bin [x y w h]
//                                   -> Auto Profile as in FUN_0014e4a4: FindFlatArea (or the given
//                                      area), crop FUN_009c7318, BuildNoiseModel FUN_009af718,
//                                      AutoFineTune FUN_009a775c; dumps the profile object (0xc300 B)
//                                      and the three LZMA blobs of FUN_009b41c4 to out.bin.lzma0/1/2
//   nvh curve  W H rgb.f32          -> FUN_009b6c8c EstimateNoiseLevelCurve: prints the 8-bin
//                                      (brightness, noise) table of the BaseInterpolator
//   nvh clip   W H rgb.f32          -> FUN_009c6648 IsClip fractions of R, G, B
//   nvh interp n x_1..x_n y_1..y_n q_1..q_m -> BaseInterpolator (FUN_00aed9f0, slot 3 FUN_00aeda08 set,
//                                      FUN_00aedb18 eval): prints "q value" (%.17g) for every query q
//   nvh tiles  W H [tile margin]    -> tile descriptors of the scheduler as EstimateNoiseLevelCurve
//                                      builds it (FUN_00ac0ff0 / FUN_00ac1194 / slot 3 FUN_00ac10c0)
//   nvh refmodel out.bin            -> white-noise reference model DAT_01561db0 (FUN_009b4428), 0x1f54 B
//   nvh kernel W H angle            -> FUN_00990448 line-kernel tables (index array and line triples)
//   nvh linefilt W H plane.f32 angle r mode [angle2 r2] -> FUN_009944c0 along the FUN_00990448 kernel
//                                      (r = radius, mode 0/1), optional second pass; dumps the plane
//   nvh pixnoise W H luma.f32 out   -> FUN_00998d98 per-pixel noise: writes out.noise/.weight/.bright
//                                      (.f32, weight starts at 1.0; W, H <= 512 like the 96+24 tiles)
//   nvh flat   W H rgb.f32 [mode] [sel] -> FUN_009b1a5c FindFlatArea on planar R,G,B (0..255 floats);
//                                      sel 0 = size table (Auto Profile path), 1 = 18%-width square;
//                                      mode = profile colour mode (0 RGB, 1 BT.601 YCbCr, 2 mean)
//                                      env NVH_CANCEL_AT=k: the cancel query (core vtable slot 0) reports a
//                                      cancel from its k-th call on (status 5); the count goes to stderr
//   nvh cand   W H luma.f32 rx ry step [nb per_bin] -> FUN_009999cc FindFlatArea candidates on a reduced
//                                      luma plane: prints "i x y mean score" (floats as %08x bits, then %.9g)
//   nvh reduce W H rgb.f32 s out       -> FUN_009b7e58 s x s box reduction of a float image and FUN_0097bb88
//                                      luma: writes out.r/.g/.b/.luma (.f32, (W/s) x (H/s))
//   nvh xform  W H rgb.f32 mode out    -> profile colour transform DAT_0147df50[mode] in place on the three
//                                      planes as FindFlatArea calls it: writes out.p0/.p1/.p2 (.f32)
//   nvh flatness W H plane.f32         -> FUN_009980e8 then FUN_009b096c(model, luma) for luma = 1 and 0
//   nvh quantile n v.f32 qbits flag div -> FUN_00997bcc(q, v, &out, flag, div, n) with q given as the hex bit
//                                      pattern of a float32: prints "quantile result out" as %08x, then %.9g
//                                      (out keeps the marker 7fbadbad where the genuine does not write it)
// rgb.f32 = R, G, B planes (W*H float32 each, 0..255) concatenated.
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "nvh_guard.h"

static uintptr_t base;
#define FN(off) ((void *)(base + (off)))

static float *load(const char *p, long n) {
  float *b = malloc(n * 4); FILE *f = fopen(p, "rb");   // (checked below)
  if (!f || fread(b, 4, n, f) != (size_t)n) { fprintf(stderr, "read %s failed\n", p); exit(1); }
  fclose(f); return b;
}

static long not_cancelled(void *self) { (void)self; return 0; }
// vtable slot 0 (the cancel query): with NVH_CANCEL_AT=k the k-th and later queries report a cancel
// (the host's own callback, as when the user aborts a profile; nothing of the bundle is changed)
static long cancel_at, cancel_calls;
static long cancel_query(void *self) {
  (void)self;
  long n = __atomic_add_fetch(&cancel_calls, 1, __ATOMIC_SEQ_CST);
  return cancel_at > 0 && n >= cancel_at;
}
static void *fake_vtbl[16];

// Image object (0x108 bytes): ctor FUN_001c1114; planes +0xb0/+0xb8/+0xc0, alpha +0xc8,
// W +0xd0, H +0xd8, valid byte +0xe0, bytes/sample +0xe8, type +0xf0 (2 = float32), grey flag +0xf4.
static void *make_image(long W, long H, float *rgb) {
  unsigned char *img = calloc(1, 0x200);
  ((void (*)(void *))FN(0x1c1114))(img);
  *(float **)(img + 0xb0) = rgb;
  *(float **)(img + 0xb8) = rgb + W * H;
  *(float **)(img + 0xc0) = rgb + 2 * W * H;
  *(long *)(img + 0xd0) = W;
  *(long *)(img + 0xd8) = H;
  *(unsigned char *)(img + 0xe0) = 1;
  *(long *)(img + 0xe8) = 4;
  *(int *)(img + 0xf0) = 2;
  return img;
}

// Filter-core object as used by FindFlatArea/BuildNoiseModel: vtable slot 0 = cancel query,
// +8 progress tree, +0x70 current progress node, +0x3f90 -> profile object (+0x48 colour mode).
static unsigned char *make_core(unsigned int mode) {
  unsigned char *core = calloc(1, 0x30000);   // AutoFineTune uses fields up to +0x25bc8
  unsigned char *prof = calloc(1, 0x10000);
  for (int i = 0; i < 16; i++) fake_vtbl[i] = (void *)not_cancelled;
  fake_vtbl[0] = (void *)cancel_query;
  cancel_at = getenv("NVH_CANCEL_AT") ? atol(getenv("NVH_CANCEL_AT")) : 0;
  if (cancel_at > 0) fprintf(stderr, "[nvh] NVH_CANCEL_AT=%ld: cancel from query %ld on\n", cancel_at, cancel_at);
  *(void ***)core = fake_vtbl;
  *(float *)(core + 8) = 1.0f;   // progress root: total fraction
  *(int *)(core + 12) = 1;       // progress root: state (1 = sequential children)
  *(unsigned char **)(core + 0x3f90) = prof;
  *(unsigned int *)(prof + 0x48) = mode;
  return core;
}

static int profile(long W, long H, float *rgb, long mode, const char *out, long *area) {
  void *img = make_image(W, H, rgb);
  unsigned char *core = make_core((unsigned int)mode);
  unsigned long x = 0, y = 0; long w = 0, h = 0; int st = -1;
  if (area) { x = area[0]; y = area[1]; w = area[2]; h = area[3]; }
  else st = ((int (*)(void *, void *, unsigned, unsigned long *, unsigned long *, long *, long *))FN(0x9b1a5c))(
      core, img, 0, &x, &y, &w, &h);
  unsigned char *crop = calloc(1, 0x200);
  ((void (*)(void *))FN(0x1c1114))(crop);
  ((void (*)(void *, long, long, long, long, void *, long, long))FN(0x9c7318))(img, x, y, w, h, crop, 2, 1);
  int bm = ((int (*)(void *, void *, int))FN(0x9af718))(core, crop, 1);
  int ft = ((int (*)(void *, void *, int, unsigned, long, void *, void *, void *, long, void *, int))FN(0x9a775c))(
      core, img, 0, 0, 0, NULL, NULL, NULL, 0, NULL, 1);
  unsigned char *prof = *(unsigned char **)(core + 0x3f90);
  FILE *f = fopen(out, "wb");
  if (!f) { perror(out); exit(1); }
  fwrite(prof, 1, 0xc300, f);
  fclose(f);
  // the three LZMA blobs FUN_009b41c4 allocated (profile +0x00/+0x08/+0x10, lengths +0x18/+0x1c/+0x20, read only):
  // written to out.bin.lzma0/1/2 when the build succeeded (+0xc2a4 = 1)
  if (prof[0xc2a4]) {
    for (int i = 0; i < 3; i++) {
      const unsigned char *blob = *(unsigned char **)(prof + 8 * i);
      int n = *(int *)(prof + 0x18 + 4 * i);
      char name[1024];
      snprintf(name, sizeof name, "%s.lzma%d", out, i);
      FILE *g = fopen(name, "wb");
      if (!g) { perror(name); exit(1); }
      if (blob && n > 0) fwrite(blob, 1, (size_t)n, g);
      fclose(g);
    }
  }
  printf("flat_status %d area x=%lu y=%lu w=%ld h=%ld build %d finetune %d\n", st, x, y, w, h, bm, ft);
  return 0;
}

static int flat(long W, long H, float *rgb, long mode, unsigned sel) {
  void *img = make_image(W, H, rgb);
  unsigned char *core = make_core((unsigned int)mode);
  unsigned long x = 0, y = 0; long w = 0, h = 0;
  int r = ((int (*)(void *, void *, unsigned, unsigned long *, unsigned long *, long *, long *))FN(0x9b1a5c))(
      core, img, sel, &x, &y, &w, &h);
  printf("status %d area x=%lu y=%lu w=%ld h=%ld\n", r, x, y, w, h);
  fprintf(stderr, "[nvh] cancel queries %ld (NVH_CANCEL_AT=%ld)\n", cancel_calls, cancel_at);
  return 0;
}

static unsigned bits32(float v) { unsigned u; memcpy(&u, &v, 4); return u; }

static void dump_f32(const char *prefix, const char *suffix, const float *a, long n) {
  char name[1024];
  snprintf(name, sizeof name, "%s%s", prefix, suffix);
  FILE *f = fopen(name, "wb");
  if (!f) { perror(name); exit(1); }
  fwrite(a, 4, n, f);
  fclose(f);
}

// FUN_009999cc(ctx, luma, W, H, rx, ry, step, nb, per_bin, xs, ys, means, scores): the candidate
// search of FindFlatArea (ctx is not read; buffers come from the bundle's allocator FUN_009d7200).
static int cand(long W, long H, float *luma, long rx, long ry, long step, long nb, long per) {
  long total = nb * per, ctx = 0;
  if (total < 1 || total > 4096) { fprintf(stderr, "cand: nb * per_bin must be 1..4096\n"); return 1; }
  long *xs = calloc(total, 8), *ys = calloc(total, 8);
  float *means = calloc(total, 4), *scores = calloc(total, 4);
  ((void (*)(long *, float *, long, long, long, long, long, long, long, long *, long *, float *, float *))
       FN(0x9999cc))(&ctx, luma, W, H, rx, ry, step, nb, per, xs, ys, means, scores);
  for (long i = 0; i < total; i++)
    printf("%ld %ld %ld %08x %08x %.9g %.9g\n", i, xs[i], ys[i], bits32(means[i]), bits32(scores[i]),
           means[i], scores[i]);
  return 0;
}

// FUN_009b7e58(img, R, G, B, s) then FUN_0097bb88(R, G, B, luma, n), as FindFlatArea runs them.
static int reduce(long W, long H, float *rgb, long s, const char *out) {
  if (s < 1) { fprintf(stderr, "reduce: s must be >= 1\n"); return 1; }
  long w = W / s, h = H / s;
  void *img = make_image(W, H, rgb);
  float *r = calloc(w * h + 1, 4), *g = calloc(w * h + 1, 4), *b = calloc(w * h + 1, 4), *l = calloc(w * h + 1, 4);
  ((void (*)(void *, float *, float *, float *, long))FN(0x9b7e58))(img, r, g, b, s);
  ((void (*)(float *, float *, float *, float *, long))FN(0x97bb88))(r, g, b, l, w * h);
  dump_f32(out, ".r.f32", r, w * h);
  dump_f32(out, ".g.f32", g, w * h);
  dump_f32(out, ".b.f32", b, w * h);
  dump_f32(out, ".luma.f32", l, w * h);
  printf("reduce %ld x %ld\n", w, h);
  return 0;
}

// DAT_0147df50[mode](p0, p1, p2, p0, p1, p2, n): in place, as FindFlatArea calls it on the crop planes.
static int xform(long W, long H, float *rgb, long mode, const char *out) {
  if (mode < 0 || mode > 3) { fprintf(stderr, "xform: mode 0..3\n"); return 1; }
  void (*fn)(float *, float *, float *, float *, float *, float *, long) =
      *(void **)(base + 0x147df50 + 8 * mode);   // rebased pointer table, read only
  float *p0 = rgb, *p1 = rgb + W * H, *p2 = rgb + 2 * W * H;
  fn(p0, p1, p2, p0, p1, p2, W * H);
  dump_f32(out, ".p0.f32", p0, W * H);
  dump_f32(out, ".p1.f32", p1, W * H);
  dump_f32(out, ".p2.f32", p2, W * H);
  printf("xform mode %ld fn %lx\n", mode, (long)((uintptr_t)fn - base));
  return 0;
}

static void info(void) {   // read-only: nothing of the bundle is called
  printf("threads DAT_014a67c0=%ld simd DAT_014a67a8=%d bins DAT_012445a0=%ld clip-penalty DAT_0127a7a0=%g\n",
         *(long *)(base + 0x14a67c0), *(unsigned char *)(base + 0x14a67a8), *(long *)(base + 0x12445a0),
         *(double *)(base + 0x127a7a0));
  const long tabs[3] = {0x1246d30, 0x1246da8, 0x1246e20};
  for (int t = 0; t < 3; t++) {
    unsigned char *p = (unsigned char *)(base + tabs[t]);
    printf("FindFlatArea table %lx: <= %ld px, sizes", tabs[t], *(long *)(p + 8));
    for (long i = 0; i < *(long *)(p + 0x10); i++) printf(" %ld", *(long *)(p + 0x18 + 8 * i));
    printf(" factors");
    for (long i = 0; i < *(long *)(p + 0x10); i++) printf(" %g", *(float *)(p + 0x58 + 4 * i));
    printf("\n");
  }
  printf("quantile small-sample factors UNK_01244678:");
  for (int i = 0; i < 12; i++) printf(" %g", *(float *)(base + 0x1244678 + 4 * i));
  printf("\nAutoFineTune d8 DAT_012477d0 %g %g %g, d9 DAT_012477dc %g %g %g\n", *(float *)(base + 0x12477d0),
         *(float *)(base + 0x12477d4), *(float *)(base + 0x12477d8), *(float *)(base + 0x12477dc),
         *(float *)(base + 0x12477e0), *(float *)(base + 0x12477e4));
  uint32_t w28 = *(uint32_t *)(base + 0x1488496);
  uintptr_t holder = *(uintptr_t *)(base + 0x1488ce8) + 0x1a07eb80;
  uintptr_t tbl = *(uintptr_t *)holder;
  long x20 = tbl ? -(long)*(unsigned char *)(tbl + (w28 & 0xffff)) : (long)w28;
  printf("AutoFineTune entry: token table image+%lx = %lx, condition %ld (0 = body runs; not bypassed)\n",
         (long)(holder - base), (long)tbl, (x20 + ((w28 >> 16) & 0xff)) * 2 + (w28 >> 24));
}

static void probe(void) {   // read-only: dumps the full protection state (no calls into the bundle)
  printf("== protection state probe (base = %p) ==\n", (void *)base);

  // -- token table pointer cells (__common, zero-init; filled by the host when licensed) --
  const struct { uintptr_t addr; const char *name; } cells[] = {
    {0x1566B80, "AFT entry"},
    {0x15636B8, "AFT body"},
    {0x1566BC0, "shrink w19 (FUN_00990f74 0x99106c)"},
    {0x1565418, "shrink w23 (FUN_00990f74 0x9915c8)"},
    {0x1566B30, "kernel shallow (FUN_00990448 0x9906b0)"},
    {0x15666C8, "AFT 0x9a7d9c"},
    {0x1569FB0, "watch-list (AFT writes the word address here)"},
  };
  printf("-- token table pointer cells (__common) --\n");
  for (int i = 0; i < (int)(sizeof cells / sizeof cells[0]); i++) {
    uintptr_t v = *(uintptr_t *)(base + cells[i].addr);
    printf("  0x%06lX %-42s = 0x%016lx%s\n", cells[i].addr, cells[i].name, v,
           v == 0 ? "  (NULL: not filled)" : "");
  }

  // -- token table contents (only readable when the pointer is non-NULL) --
  printf("-- token table contents --\n");
  uintptr_t aft_entry_root = *(uintptr_t *)(base + 0x1488ce8);
  uintptr_t aft_entry_cell = aft_entry_root + 0x1a07eb80;
  uintptr_t aft_tbl = *(uintptr_t *)aft_entry_cell;
  if (aft_tbl) {
    uint32_t w = *(uint32_t *)(base + 0x1488496);
    unsigned char tok = *(unsigned char *)(aft_tbl + (w & 0xffff));
    printf("  AFT entry: *(0x1488CE8)+0x1A07EB80 = 0x%lx, table[0x%04x] = %d (need 97 for body)\n",
           aft_tbl, w & 0xffff, tok);
  } else {
    printf("  AFT entry: table NULL (cell 0x%lx = 0)\n", aft_entry_cell);
  }
  uintptr_t shrink_tbl = *(uintptr_t *)(base + 0x1566bc0);
  if (shrink_tbl) {
    printf("  shrink w19: table[0x3997] = %d\n", *(unsigned char *)(shrink_tbl + 0x3997));
  } else {
    printf("  shrink w19: table NULL\n");
  }
  uintptr_t shrink_tbl2 = *(uintptr_t *)(base + 0x1565418);
  if (shrink_tbl2) {
    printf("  shrink w23: table[0x5cc8] = %d\n", *(unsigned char *)(shrink_tbl2 + 0x5cc8));
  } else {
    printf("  shrink w23: table NULL\n");
  }
  uintptr_t kern_tbl = *(uintptr_t *)(base + 0x1566b30);
  if (kern_tbl) {
    printf("  kernel shallow: table[0xc3a1] = %d\n", *(unsigned char *)(kern_tbl + 0xc3a1));
  } else {
    printf("  kernel shallow: table NULL\n");
  }

  // -- state words (__data; contain designed constants or runtime-populated values) --
  printf("-- state words (__data) --\n");
  uint32_t w19 = *(uint32_t *)(base + 0x1489e68);
  printf("  0x1489E68 (FUN_0099afb4 w19) = 0x%08x (popcount %d → subtrahend %d)\n",
         w19, __builtin_popcount(w19), __builtin_popcount(w19) - 16);
  printf("  0x1489E64                     = 0x%08x\n", *(uint32_t *)(base + 0x1489e64));
  uint32_t guar = *(uint32_t *)(base + 0x1561dec);
  printf("  0x1561DEC (guarantee word)   = 0x%08x (& 0x3fffff = 0x%08x %s)\n",
         guar, guar & 0x3fffff, (guar & 0x3fffff) == 1 ? "== 1: gate OK" : "!= 1: gate FAIL");
  uint16_t g16 = *(uint16_t *)(base + 0x1561df0);
  printf("  0x1561DF0 (16bit gate)       = 0x%04x (%s)\n", g16, g16 == 1 ? "== 1: gate OK" : "!= 1: gate FAIL");
  printf("  0x1488496 (AFT constant)     = 0x%08x\n", *(uint32_t *)(base + 0x1488496));
  printf("  0x148849C (AFT body const)   = 0x%08x\n", *(uint32_t *)(base + 0x148849c));

  // -- runtime state (__common / __data; written by the protection code at runtime) --
  printf("-- runtime state --\n");
  printf("  0x1561D58 (kernel shift)     = 0x%016lx (%lu = lsl amount)\n",
         *(unsigned long *)(base + 0x1561d58), *(unsigned long *)(base + 0x1561d58) & 63);
  printf("  0x147E0E0 (ixform counter)   = 0x%08x (byte24 = %d)\n",
         *(uint32_t *)(base + 0x147e0e0), (*(uint32_t *)(base + 0x147e0e0)) >> 24);
  printf("  0x1561C1C..C40 (hash cells)  =");
  for (int i = 0; i < 8; i++) printf(" %08x", *(uint32_t *)(base + 0x1561c1c + 4 * i));
  printf("\n");

  // -- AutoFineTune gate conditions (mirrors nvh info but includes body gate) --
  uint32_t w28 = *(uint32_t *)(base + 0x1488496);
  long cond_entry = aft_tbl ? ((-(long)*(unsigned char *)(aft_tbl + (w28 & 0xffff)) + ((w28 >> 16) & 0xff)) * 2 + (w28 >> 24)) : (long)w28;
  printf("-- AutoFineTune gates --\n");
  printf("  entry gate condition = %ld (%s)\n", cond_entry,
         cond_entry == 0 ? "0: BODY RUNS" : "!= 0: body SKIPPED");
  // body gate: word 0x148849c, table cell at *(image+0x15636B8)
  uintptr_t body_cell = *(uintptr_t *)(base + 0x15636b8);
  if (body_cell) {
    uint32_t wb = *(uint32_t *)(base + 0x148849c);
    long x20b = -(long)*(unsigned char *)(body_cell + (wb & 0xffff));
    long cond_body = (x20b + ((wb >> 16) & 0xff)) * 2 + (wb >> 24);
    printf("  body gate condition  = %ld (%s)\n", cond_body,
           cond_body == 0 ? "0: body check PASSES" : "!= 0: body check FAILS");
  } else {
    printf("  body gate: table NULL (cell 0x15636B8 = 0) → body check FAILS\n");
  }
  printf("-- end probe --\n");
}

int main(int argc, char **argv) {
  const char *lib = getenv("NV_BUNDLE") ? getenv("NV_BUNDLE") :
    "/Users/seitan/Desktop/work/reverse/neatvideo_ofx_614/NeatVideo6.ofx.bundle/Contents/MacOS/NeatVideo6.ofx";
  nvh_guard_preload();                      // must precede dlopen (initialisers run inside it)
  if (!dlopen(lib, RTLD_NOW | RTLD_LOCAL)) { fprintf(stderr, "dlopen: %s\n", dlerror()); return 1; }
  base = nvh_guard_check();
  fprintf(stderr, "[nvh] guard installed before the bundle's initialisers\n");
  if (argc > 1 && !strcmp(argv[1], "selftest")) {   // must end with exit(3) and no dialog
    ((void (*)(const char *, long, long, long, long, long))FN(0x9d95a4))("nvh selftest", 0, 0, 0, -1, 0x1234);
    fprintf(stderr, "[nvh] selftest: handler returned without the guard firing\n");
    return 6;
  }
  if (argc > 1 && !strcmp(argv[1], "order")) {   // which __DATA sections changed after the guard?
    const struct load_command *lc = (const void *)((const struct mach_header_64 *)base + 1);
    for (uint32_t c = 0; c < ((const struct mach_header_64 *)base)->ncmds; c++, lc = (const void *)((const char *)lc + lc->cmdsize)) {
      if (lc->cmd != LC_SEGMENT_64 || strcmp(((const struct segment_command_64 *)lc)->segname, "__DATA")) continue;
      const struct segment_command_64 *sg = (const void *)lc;
      const struct section_64 *sc = (const void *)(sg + 1);
      for (uint32_t k = 0; k < sg->nsects; k++, sc++) {
        long diff = 0;
        for (uintptr_t a = base + sc->addr; nvh_data_snap && a < base + sc->addr + sc->size; a++)
          if (a >= nvh_data_lo && a < nvh_data_hi && *(unsigned char *)a != nvh_data_snap[a - nvh_data_lo]) diff++;
        if (diff) printf("%-16s %8ld bytes changed after the add-image callback\n", sc->sectname, diff);
      }
    }
    printf("(changes in __data/__bss/__common come from the bundle's own static initialisers; __objc_* from the\n"
           " objc runtime's image registration, which dyld runs before them)\n");
    return 0;
  }
  if (argc > 1 && !strcmp(argv[1], "info")) { info(); return 0; }
  if (argc > 1 && !strcmp(argv[1], "probe")) { probe(); return 0; }
  if (argc > 2 && !strcmp(argv[1], "interp")) {   // read-only maths of the curve interpolator
    long n = atol(argv[2]);
    if (n < 1 || n > 64 || argc < 3 + 2 * n) { fprintf(stderr, "interp n x.. y.. q..\n"); return 1; }
    double xs[64], ys[64];
    for (long i = 0; i < n; i++) { xs[i] = atof(argv[3 + i]); ys[i] = atof(argv[3 + n + i]); }
    unsigned char it[0x80] __attribute__((aligned(16)));
    memset(it, 0, sizeof it);
    ((void (*)(void *))FN(0xaed9f0))(it);
    ((void (*)(void *, double *, double *, unsigned long))FN(0xaeda08))(it, xs, ys, (unsigned long)n);
    for (int a = 3 + 2 * (int)n; a < argc; a++) {
      double q = atof(argv[a]);
      printf("%.17g %.17g\n", q, ((double (*)(double, void *))FN(0xaedb18))(q, it));
    }
    return 0;
  }
  if (argc > 2 && !strcmp(argv[1], "aftshuffle")) {   // FUN_009d39ec: AutoFineTune mode-2 tile order (leaf: no calls,
    // one __TEXT constant load at 0xc95950, writes only out[0..n))
    long n = atol(argv[2]);
    if (n < 0 || n > 4096) { fprintf(stderr, "aftshuffle: 0 <= N <= 4096\n"); return 1; }
    long *out = calloc(n ? n : 1, 8);
    ((void (*)(long *, long))FN(0x9d39ec))(out, n);
    for (long i = 0; i < n; i++) printf("%ld %ld\n", i, out[i]);
    free(out);
    return 0;
  }
  if (argc > 3 && !strcmp(argv[1], "tiles")) {
    long W = atol(argv[2]), H = atol(argv[3]);
    long tile = argc > 4 ? atol(argv[4]) : 96, margin = argc > 5 ? atol(argv[5]) : 12;
    unsigned char *sch = calloc(1, 0x8000);
    ((void (*)(void *))FN(0xac0ff0))(sch);
    ((void (*)(void *, long, long, long, long, long, int, long, long, long, long, long, long))FN(0xac1194))(
        sch, W, H, tile, tile, margin, 0, 0x30, 0, 0, W, H, 0);
    ((void (*)(void *))FN(0xac0d94))(sch);
    void **vt = *(void ***)sch;
    for (int i = 0;; i++) {
      long *d = ((long *(*)(void *, void *))vt[3])(sch, NULL);
      if (!d) break;
      printf("tile %d x=%ld y=%ld w=%ld h=%ld f4=%ld f5=%ld core_w=%ld core_h=%ld core_x=%ld core_y=%ld f10=%ld\n",
             i, d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10]);
    }
    ((void (*)(void *))vt[1])(sch);
    return 0;
  }
  if (argc > 2 && !strcmp(argv[1], "refmodel")) {
    unsigned char *core = make_core(1);
    ((void (*)(void *))FN(0x9b4428))(core);
    float *m = *(float **)(base + 0x1561db0);
    if (!m) { fprintf(stderr, "reference model not built\n"); return 1; }
    FILE *f = fopen(argv[2], "wb");
    if (!f) { perror(argv[2]); return 1; }
    fwrite(m, 1, 0x1f54, f);
    fclose(f);
    printf("refmodel mean %g %g %g %g %g  set3 level0 dir0..3 %g %g %g %g\n", m[0], m[1], m[2], m[3], m[4],
           m[0x500 / 4], m[0x500 / 4 + 1], m[0x500 / 4 + 2], m[0x500 / 4 + 3]);
    return 0;
  }
  if (argc > 4 && !strcmp(argv[1], "kernel")) {
    long W = atol(argv[2]), H = atol(argv[3]);
    if (W > 512 || H > 512) { fprintf(stderr, "kernel tables hold <= 512 entries\n"); return 1; }
    unsigned char *k = calloc(1, 0x4000);
    ((void (*)(void *, long, long, float))FN(0x990448))(k, W, H, (float)atof(argv[4]));
    long n = *(long *)k, lines = *(long *)(k + 0x3808);
    printf("n %ld lines %ld horiz %d vert %d flag %d\n", n, lines, k[0x3824], k[0x3825], *(int *)(k + 0x3828));
    printf("idx");
    for (long i = 0; i < n && i < 1024; i++) printf(" %d", *(int *)(k + 8 + 4 * i));
    printf("\n");
    for (long i = 0; i < lines && i < 1100; i++)
      printf("line %ld off %d start %d count %d\n", i, *(int *)(k + 0x808 + 12 * i), *(int *)(k + 0x80c + 12 * i),
             *(int *)(k + 0x810 + 12 * i));
    return 0;
  }
  if (argc > 7 && !strcmp(argv[1], "cand")) {
    long W = atol(argv[2]), H = atol(argv[3]);
    return cand(W, H, load(argv[4], W * H), atol(argv[5]), atol(argv[6]), atol(argv[7]),
                argc > 8 ? atol(argv[8]) : 16, argc > 9 ? atol(argv[9]) : 4);
  }
  if (argc > 6 && !strcmp(argv[1], "reduce")) {
    long W = atol(argv[2]), H = atol(argv[3]);
    return reduce(W, H, load(argv[4], 3 * W * H), atol(argv[5]), argv[6]);
  }
  if (argc > 6 && !strcmp(argv[1], "xform")) {
    long W = atol(argv[2]), H = atol(argv[3]);
    return xform(W, H, load(argv[4], 3 * W * H), atol(argv[5]), argv[6]);
  }
  if (argc > 4 && !strcmp(argv[1], "flatness")) {   // FUN_009980e8 then FUN_009b096c
    long W = atol(argv[2]), H = atol(argv[3]), ctx = 0;
    float *out = calloc(1, 0x4000);
    ((void (*)(long *, float *, long, long, float *))FN(0x9980e8))(&ctx, load(argv[4], W * H), W, H, out);
    for (int luma = 1; luma >= 0; luma--) {
      float f = ((float (*)(float *, int))FN(0x9b096c))(out, luma);
      printf("flatness luma=%d %08x %.9g\n", luma, bits32(f), f);
    }
    return 0;
  }
  if (argc > 6 && !strcmp(argv[1], "quantile")) {   // FUN_00997bcc: the quantile search of FindFlatArea
    long n = atol(argv[2]), div = atol(argv[6]);
    if (n < 0 || div < 0) { fprintf(stderr, "quantile: n >= 0 and div >= 0 (n / div indexes a 12-entry table)\n"); return 1; }
    float *v = load(argv[3], n);
    uint32_t qb = (uint32_t)strtoul(argv[4], NULL, 16), mark = 0x7fbadbadu;
    float q, g;
    memcpy(&q, &qb, 4);
    memcpy(&g, &mark, 4);
    float r = ((float (*)(float, const float *, float *, unsigned char, long, unsigned long))FN(0x997bcc))(
        q, v, &g, (unsigned char)atol(argv[5]), div, (unsigned long)n);
    printf("quantile %08x %08x %.9g %.9g\n", bits32(r), bits32(g), r, g);
    return 0;
  }
  // ---- 2026-10-17: leaf helpers of the temporal filter and the motion search (sandbox/omo wp_temporal / wp_motion).
  // Each was checked on the arm64 slice to call nothing outside itself (b32: only 0x9f9290 / 0x9f93ac / 0x9f949c and
  // __stack_chk_fail) and to read no global except the NEON dispatch byte 0x14a67a8 (also read by 0x9ee04c, which
  // `curve` already runs) and __stack_chk_guard. Argument roles follow the disassembly, not the worker drafts.
  if (argc > 5 && !strcmp(argv[1], "ramp")) {   // ramp t in.f32 n out.f32: FUN_009ee93c(t, 2t, 1, 0, in, out, n)
    float t = strtof(argv[2], NULL);
    long n = atol(argv[4]);
    if (n < 1 || n > (1L << 22)) { fprintf(stderr, "ramp: 1 <= n <= 4M\n"); return 1; }
    float *in = load(argv[3], n), *out = calloc(n + 4, 4);
    ((void (*)(float, float, float, float, const float *, float *, long))FN(0x9ee93c))(t, 2.0f * t, 1.0f, 0.0f, in, out, n);
    dump_f32(argv[5], "", out, n);
    printf("ramp %ld\n", n);
    return 0;
  }
  if (argc > 6 && !strcmp(argv[1], "wmul")) {   // wmul t r.f32 prev.f32 n out.f32: out = ramp(r) * prev (FUN_009eebf8)
    float t = strtof(argv[2], NULL);
    long n = atol(argv[5]);
    if (n < 1 || n > (1L << 22)) { fprintf(stderr, "wmul: 1 <= n <= 4M\n"); return 1; }
    float *r = load(argv[3], n), *prev = load(argv[4], n), *out = calloc(n + 4, 4);
    ((void (*)(float, float, float, float, const float *, const float *, float *, long))FN(0x9eebf8))(
        t, 2.0f * t, 1.0f, 0.0f, r, prev, out, n);
    dump_f32(argv[6], "", out, n);
    printf("wmul %ld\n", n);
    return 0;
  }
  if (argc > 6 && !strcmp(argv[1], "pairw")) {   // pairw a.f32 b.f32 c.f32 n out.f32: out = c * (max(a, b) - b) (FUN_009efb1c)
    long n = atol(argv[5]);
    if (n < 1 || n > (1L << 22)) { fprintf(stderr, "pairw: 1 <= n <= 4M\n"); return 1; }
    float *a = load(argv[2], n), *b = load(argv[3], n), *c = load(argv[4], n), *out = calloc(n + 4, 4);
    ((void (*)(const float *, const float *, const float *, float *, long))FN(0x9efb1c))(a, b, c, out, n);
    dump_f32(argv[6], "", out, n);
    printf("pairw %ld\n", n);
    return 0;
  }
  if (argc > 5 && !strcmp(argv[1], "down2")) {   // down2 W H plane.f32 out.f32: FUN_009f15b4(src_row_pair, dst_row, W) per row
    long W = atol(argv[2]), H = atol(argv[3]);
    if (W < 2 || H < 2 || W > 4096 || H > 4096) { fprintf(stderr, "down2: 2 <= W, H <= 4096\n"); return 1; }
    long w2 = W / 2, h2 = H / 2;
    float *src = load(argv[4], W * H), *dst = calloc(w2 * h2 + 4, 4);
    for (long y = 0; y < h2; y++)
      ((void (*)(const float *, float *, long))FN(0x9f15b4))(src + 2 * y * W, dst + y * w2, W);
    dump_f32(argv[5], "", dst, w2 * h2);
    printf("down2 %ld %ld\n", w2, h2);
    return 0;
  }
  if ((argc > 9 && !strcmp(argv[1], "sad")) || (argc > 8 && !strcmp(argv[1], "b32"))) {
    // sad W H a.f32 b.f32 ax ay bx by [elem]: FUN_009f862c (16x16) and FUN_009f9290 (32x32) on planes padded by 8
    // (edge copies), elem 1 = bytes, 2 = int16 (the kernels read raw bytes either way).
    // b32 W H ref.f32 cur.f32 cx cy thr: FUN_0098bf18(thr, -, desc(ref), cur block at (cx, cy), 32, cx, cy, offs, ...)
    // with the descriptor from FUN_0097eebc(desc, W, H, 8, 1, 16) / FUN_0097eef8 and the 8-neighbour table of
    // DAT_014877f8 as {int byte offset, short dx, short dy} entries (the layout read by 0x98bf18).
    int b32 = !strcmp(argv[1], "b32");
    long W = atol(argv[2]), H = atol(argv[3]);
    if (W < 32 || H < 32 || W > 512 || H > 512) { fprintf(stderr, "%s: 32 <= W, H <= 512\n", argv[1]); return 1; }
    int elem = (!b32 && argc > 10) ? atoi(argv[10]) : 1;
    if (elem != 1 && elem != 2) { fprintf(stderr, "sad: elem is 1 or 2\n"); return 1; }
    float *fa = load(argv[4], W * H), *fb = load(argv[5], W * H);
    long desc[16] = {0};
    ((void (*)(long *, long, long, long, long, long))FN(0x97eebc))(desc, W, H, 8, elem, 16);
    long stride = desc[6], rows = H + 16;
    if (stride < (W + 16) * elem || desc[1] < stride * rows) { fprintf(stderr, "b32: unexpected descriptor\n"); return 1; }
    unsigned char *mem[2];
    float *src[2] = {fa, fb};
    for (int k = 0; k < 2; k++) {
      mem[k] = calloc(1, desc[1] + 64);
      ((void (*)(long *, void *))FN(0x97eef8))(desc, mem[k]);
      unsigned char *p = (unsigned char *)desc[2];   // the (0, 0) sample
      if (p < mem[k] + 8 * stride + 8 * elem || p + (H + 7) * stride + (W + 8) * elem > mem[k] + desc[1]) { fprintf(stderr, "b32: layout\n"); return 1; }
      for (long y = -8; y < H + 8; y++)
        for (long x = -8; x < W + 8; x++) {
          long sy = y < 0 ? 0 : y >= H ? H - 1 : y, sx = x < 0 ? 0 : x >= W ? W - 1 : x;
          int v = (int)(src[k][sy * W + sx] + 0.5f);
          if (elem == 1) p[y * stride + x] = (unsigned char)v;
          else { short s = (short)v; memcpy(p + y * stride + 2 * x, &s, 2); }
        }
      src[k] = (float *)p;   // reuse as the origin pointer
    }
    unsigned char *pa = (unsigned char *)src[0], *pb = (unsigned char *)src[1];
    if (!b32) {
      long ax = atol(argv[6]), ay = atol(argv[7]), bx = atol(argv[8]), by = atol(argv[9]);
      long xs[2] = {ax, bx}, ys[2] = {ay, by};
      for (int k = 0; k < 2; k++)
        if (xs[k] < -8 || ys[k] < -8 || xs[k] * elem + 32 > (W + 8) * elem || ys[k] + 32 > H + 8) {
          fprintf(stderr, "sad: a 32x32 byte window at (%ld, %ld) leaves the padded plane\n", xs[k], ys[k]); return 1; }
      unsigned char *qa = pa + ay * stride + ax * elem, *qb = pb + by * stride + bx * elem;
      unsigned long s16 = ((unsigned long (*)(const void *, const void *, long))FN(0x9f862c))(qa, qb, stride);
      unsigned long s32 = ((unsigned long (*)(const void *, const void *, long))FN(0x9f9290))(qa, qb, stride);
      printf("sad16=%lu sad32=%lu\n", s16, s32);
      return 0;
    }
    long cx = atol(argv[6]), cy = atol(argv[7]);
    float thr = strtof(argv[8], NULL);
    if (cx < 1 || cy < 1 || cx + 33 > W || cy + 33 > H) { fprintf(stderr, "b32: 1 <= cx, cy and cx, cy + 33 <= W, H\n"); return 1; }
    ((void (*)(long *, void *))FN(0x97eef8))(desc, mem[0]);   // the descriptor describes ref
    static const signed char nb[16] = {-1, -1, 0, -1, 1, -1, -1, 0, 1, 0, -1, 1, 0, 1, 1, 1};
    struct { int off; short dx, dy; } offs[8];
    for (int i = 0; i < 8; i++) { offs[i].dx = nb[2 * i]; offs[i].dy = nb[2 * i + 1]; offs[i].off = (int)(offs[i].dy * stride + offs[i].dx); }
    float ox = -1, oy = -1;
    unsigned long osad = 0;
    unsigned long ret = ((unsigned long (*)(float, void *, long *, const void *, long, long, long, void *, float *, float *, unsigned long *))
        FN(0x98bf18))(thr, NULL, desc, pb + cy * stride + cx, 32, cx, cy, offs, &ox, &oy, &osad);
    printf("b32 ret=%lu out_x=%.9g out_y=%.9g sad=%lu bits=%08x %08x\n", ret, ox, oy, osad, bits32(ox), bits32(oy));
    return 0;
  }
  // ---- 2026-10-21 (sandbox/omo/wp_spatial, align_plan.md): leaf helpers of the spatial filter FUN_0098edb0 / FUN_009aae90 /
  // FUN_009ae918 and of the directional path of FUN_00990f74. Each was checked on the arm64 slice to call nothing outside itself
  // and to read no global except the NEON dispatch byte 0x14a67a8 (strip3: the single zero float at __TEXT 0x12504a0 instead;
  // sigfuse / ixform / sel2: no global at all). No guard, hash or token word is read or written by any of them.
  if (argc > 4 && !strcmp(argv[1], "elem")) {
    // elem NAME n out.f32 s0 s1 s2 s3 in1.f32 [in2.f32 [in3.f32]]: one element-wise helper on n values (scalars as float32 hex bits)
    static const struct { const char *nm; unsigned long off; int ns, ni, u8out; } T[] = {
      {"recip", 0x9ed5f8, 1, 1, 0}, {"scale", 0x9ee304, 1, 1, 0}, {"affine", 0x9ee3fc, 2, 1, 0}, {"fabs", 0x9ee50c, 0, 1, 0},
      {"sqr", 0x9ed2f0, 0, 1, 0}, {"sqrtabs", 0x9ed3e0, 0, 1, 0}, {"sqrtk", 0x9ed4e0, 1, 1, 0}, {"u8", 0x9f02b4, 0, 1, 1},
      {"mul", 0x9edc38, 0, 2, 0}, {"add", 0x9ed844, 0, 2, 0}, {"sub", 0x9ed710, 0, 2, 0}, {"mulk", 0x9edd6c, 1, 2, 0},
      {"blend", 0x9ee1a4, 2, 2, 0}, {"rstep", 0x9ef260, 1, 2, 0}, {"rinv", 0x9eef5c, 1, 2, 0}, {"clampmul", 0x9ee7bc, 4, 2, 0},
      {"acc", 0x9edeb8, 1, 3, 0}};
    int t = -1;
    for (int i = 0; i < (int)(sizeof T / sizeof T[0]); i++) if (!strcmp(argv[2], T[i].nm)) t = i;
    long n = atol(argv[3]);
    if (t < 0 || n < 1 || n > (1L << 22) || argc < 9 + T[t].ni) { fprintf(stderr, "elem: NAME n out s0 s1 s2 s3 in1 [in2 [in3]], 1 <= n <= 4M\n"); return 1; }
    float s[4];
    for (int i = 0; i < 4; i++) { unsigned u = (unsigned)strtoul(argv[5 + i], NULL, 16); memcpy(&s[i], &u, 4); }
    float *in[3] = {0, 0, 0};
    for (int i = 0; i < T[t].ni; i++) in[i] = load(argv[9 + i], n);
    float *out = calloc(n + 4, 4);
    void *f = FN(T[t].off);
    switch (T[t].ns * 10 + T[t].ni) {
      case 1: ((void (*)(const float *, void *, long))f)(in[0], out, n); break;
      case 11: ((void (*)(float, const float *, float *, long))f)(s[0], in[0], out, n); break;
      case 21: ((void (*)(float, float, const float *, float *, long))f)(s[0], s[1], in[0], out, n); break;
      case 2: ((void (*)(const float *, const float *, float *, long))f)(in[0], in[1], out, n); break;
      case 12: ((void (*)(float, const float *, const float *, float *, long))f)(s[0], in[0], in[1], out, n); break;
      case 22: ((void (*)(float, float, const float *, const float *, float *, long))f)(s[0], s[1], in[0], in[1], out, n); break;
      case 42: ((void (*)(float, float, float, float, const float *, const float *, float *, long))f)(
          s[0], s[1], s[2], s[3], in[0], in[1], out, n); break;
      case 13: ((void (*)(float, const float *, const float *, const float *, float *, long))f)(s[0], in[0], in[1], in[2], out, n); break;
      default: fprintf(stderr, "elem: bad table entry\n"); return 1;
    }
    FILE *fo = fopen(argv[4], "wb");
    if (!fo) { perror(argv[4]); return 1; }
    fwrite(out, T[t].u8out ? 1 : 4, n, fo);
    fclose(fo);
    printf("elem %s %ld\n", argv[2], n);
    return 0;
  }
  if (argc > 8 && !strcmp(argv[1], "strip3")) {
    // strip3 W H plane.f32 box|sq mode sbits out.f32: the 4-column strip loop of FUN_0098edb0 (0x98f378, box = FUN_009ed014(src+c, dst+c,
    // W, H, mode, first, last)) and of FUN_009950f4 (0x995174, sq = FUN_009ed164(s, src+c, dst+c, W, H, mode, first, last)):
    // c = min(x, W - 4), first = (c == 0), last = (W - 4 <= x), x = c + 4 while x < W
    long W = atol(argv[2]), H = atol(argv[3]);
    int sq = !strcmp(argv[5], "sq"), mode = atoi(argv[6]);
    if (W < 4 || H < 2 || W > 512 || H > 512 || (!sq && strcmp(argv[5], "box")) || (mode != 0 && mode != 1)) {
      fprintf(stderr, "strip3: 4 <= W <= 512, 2 <= H <= 512, box|sq, mode 0|1\n"); return 1;
    }
    unsigned u = (unsigned)strtoul(argv[7], NULL, 16); float sc; memcpy(&sc, &u, 4);
    float *src = load(argv[4], W * H), *dst = calloc(W * H + 4, 4);
    for (long x = 0;;) {
      long c = x < W - 4 ? x : W - 4;
      if (sq) ((void (*)(float, const float *, float *, long, long, long, long, long))FN(0x9ed164))(
          sc, src + c, dst + c, W, H, mode, c == 0, W - 4 <= x);
      else ((void (*)(const float *, float *, long, long, long, long, long))FN(0x9ed014))(
          src + c, dst + c, W, H, mode, c == 0, W - 4 <= x);
      x = c + 4;
      if (x >= W) break;
    }
    dump_f32(argv[8], "", dst, W * H);
    printf("strip3 %ld %ld\n", W, H);
    return 0;
  }
  if (argc > 8 && !strcmp(argv[1], "sigfuse")) {
    // sigfuse n r.f32 g.f32 b.f32 tab.f32 coef.f32 out: FUN_009f1d8c(R, G, B, T_R, T_G, T_B, o0, o1, o2, coef[9], n) (the NEON
    // sigma map of FUN_009aae90; it handles 4 values per step up to n rounded up, so every buffer is padded by 4 zeros);
    // tab.f32 = 3 x 257 floats, coef.f32 = 9 floats; writes out.0/.1/.2 (.f32, n values each)
    long n = atol(argv[2]);
    if (n < 1 || n > (1L << 22)) { fprintf(stderr, "sigfuse: 1 <= n <= 4M\n"); return 1; }
    float *pl[3], *o[3];
    for (int i = 0; i < 3; i++) { pl[i] = calloc(n + 4, 4); float *t = load(argv[3 + i], n); memcpy(pl[i], t, n * 4); o[i] = calloc(n + 4, 4); }
    float *tab = load(argv[6], 3 * 257), *coef = load(argv[7], 9);
    ((void (*)(const float *, const float *, const float *, const float *, const float *, const float *, float *, float *, float *,
               const float *, long))FN(0x9f1d8c))(pl[0], pl[1], pl[2], tab, tab + 257, tab + 514, o[0], o[1], o[2], coef, n);
    dump_f32(argv[8], ".0.f32", o[0], n); dump_f32(argv[8], ".1.f32", o[1], n); dump_f32(argv[8], ".2.f32", o[2], n);
    printf("sigfuse %ld\n", n);
    return 0;
  }
  if (argc > 6 && !strcmp(argv[1], "ixform")) {
    // ixform n y.f32 a.f32 b.f32 out: FUN_009f1d0c(Y, a, b, o0, o1, o2, n), the NEON part of the inverse YCrCb transform 0x97b76c
    // (DAT_0147df68[1]); it writes n & ~3 values, which go to out.0/.1/.2 (.f32)
    long n = atol(argv[2]);
    if (n < 4 || n > (1L << 22)) { fprintf(stderr, "ixform: 4 <= n <= 4M\n"); return 1; }
    float *y = load(argv[3], n), *a = load(argv[4], n), *b = load(argv[5], n);
    float *o0 = calloc(n + 4, 4), *o1 = calloc(n + 4, 4), *o2 = calloc(n + 4, 4);
    ((void (*)(const float *, const float *, const float *, float *, float *, float *, long))FN(0x9f1d0c))(y, a, b, o0, o1, o2, n);
    dump_f32(argv[6], ".0.f32", o0, n & ~3L); dump_f32(argv[6], ".1.f32", o1, n & ~3L); dump_f32(argv[6], ".2.f32", o2, n & ~3L);
    printf("ixform %ld\n", n & ~3L);
    return 0;
  }
  if (argc > 12 && !strcmp(argv[1], "sel2")) {
    // sel2 n flag stride a.f32 b.f32 i3.u8 i4.u8 best.f32 src.f32 wt.f32 out: FUN_009f19cc, the NEON part of FUN_0098eba8, with the
    // arguments FUN_0098eba8 passes (a, b, i3, i4, out, best, src, wt, flag, stride, n); it handles n & ~3 values. src and wt
    // hold 256 * stride floats (index i + stride * byte), n <= stride. Writes out.out/.best/.b (.f32) and out.i4 (bytes)
    // 2026-10-21 (wp_spatial): a 13th argument "full" calls FUN_0098eba8 itself (same arguments; it calls only the leaf
    // FUN_009f19cc and reads only the NEON byte 0x14a67a8), which adds the scalar tail for the last n & 3 values; all n are written
    long n = atol(argv[2]), stride = atol(argv[4]);
    int flag = atoi(argv[3]) & 1, full = argc > 13 && !strcmp(argv[13], "full");
    if (n < (full ? 1 : 4) || stride < n || stride > (1L << 16)) { fprintf(stderr, "sel2: 4 (full: 1) <= n <= stride <= 65536\n"); return 1; }
    float *a = load(argv[5], n), *b = load(argv[6], n), *best = load(argv[9], n);
    float *src = load(argv[10], 256 * stride), *wt = load(argv[11], 256 * stride), *out = calloc(n + 4, 4);
    unsigned char *i3 = calloc(n + 4, 1), *i4 = calloc(n + 4, 1);
    FILE *f3 = fopen(argv[7], "rb"), *f4 = fopen(argv[8], "rb");
    if (!f3 || !f4 || fread(i3, 1, n, f3) != (size_t)n || fread(i4, 1, n, f4) != (size_t)n) { fprintf(stderr, "sel2: read u8 failed\n"); return 1; }
    fclose(f3); fclose(f4);
    ((void (*)(const float *, float *, const unsigned char *, unsigned char *, float *, float *, const float *, const float *,
               unsigned char, long, long))FN(full ? 0x98eba8 : 0x9f19cc))(a, b, i3, i4, out, best, src, wt, (unsigned char)flag, stride, n);
    long m = full ? n : n & ~3L;
    dump_f32(argv[12], ".out.f32", out, m); dump_f32(argv[12], ".best.f32", best, m); dump_f32(argv[12], ".b.f32", b, m);
    char nm[1024]; snprintf(nm, sizeof nm, "%s.i4", argv[12]);
    FILE *fo = fopen(nm, "wb"); if (!fo) { perror(nm); return 1; } fwrite(i4, 1, m, fo); fclose(fo);
    printf("sel2 %ld\n", m);
    return 0;
  }
  if (argc < 5) {
    fprintf(stderr, "usage: nvh selftest|order|info | levels|model W H plane.f32 [set] | "
            "curve|clip W H rgb.f32 | flat W H rgb.f32 [mode] [sel] | profile W H rgb.f32 mode out.bin [x y w h] | "
            "tiles W H [tile margin] | refmodel out.bin | kernel W H angle | linefilt W H plane.f32 angle r mode [angle2 r2] | "
            "pixnoise W H luma.f32 out | cand W H luma.f32 rx ry step [nb per_bin] | reduce W H rgb.f32 s out | "
            "xform W H rgb.f32 mode out | flatness W H plane.f32 | quantile n v.f32 qbits flag div | "
            "ramp t in.f32 n out | wmul t r.f32 prev.f32 n out | pairw a.f32 b.f32 c.f32 n out | down2 W H plane.f32 out | "
            "sad W H a.f32 b.f32 ax ay bx by [elem] | b32 W H ref.f32 cur.f32 cx cy thr | aftshuffle N | interp n x.. y.. q.. | "
            "elem NAME n out s0 s1 s2 s3 in1 [in2 [in3]] | strip3 W H plane.f32 box|sq mode sbits out | "
            "sigfuse n r g b tab coef out | ixform n y a b out | sel2 n flag stride a b i3 i4 best src wt out [full]\n");
    return 1;
  }
  long W = atol(argv[2]), H = atol(argv[3]);
  if (!strcmp(argv[1], "clip")) {   // FUN_009c6648 IsClip fractions of the whole image
    void *img = make_image(W, H, load(argv[4], 3 * W * H));
    float r = -1, g = -1, b = -1;
    ((void (*)(void *, float *, float *, float *, long))FN(0x9c6648))(img, &r, &g, &b, 1);
    printf("clip %.6f %.6f %.6f\n", r, g, b);
    return 0;
  }
  if (!strcmp(argv[1], "curve")) {
    void *img = make_image(W, H, load(argv[4], 3 * W * H));
    unsigned char *core = make_core(1);
    unsigned char interp[0x80] __attribute__((aligned(16)));
    memset(interp, 0, sizeof interp);
    ((void (*)(void *))FN(0xaed9f0))(interp);
    // progress node as FindFlatArea prepares it before EstimateNoiseLevelCurve (0x9b1d60)
    ((void (*)(void *))FN(0xa22470))(core + 8);
    *(void **)(core + 0x70) = ((void *(*)(float, void *, long))FN(0xa22934))(0.8f, core + 8, W * H);
    long ok = ((long (*)(void *, void *, void *))FN(0x9b6c8c))(core, img, interp);
    int n = *(int *)(interp + 8);
    double *xs = *(double **)(interp + 0x10), *ys = *(double **)(interp + 0x18);
    printf("ok %ld n %d\n", ok & 1, n);
    for (int i = 0; i < n && xs && ys; i++) printf("%.17g %.17g\n", xs[i], ys[i]);
    return 0;
  }
  if (!strcmp(argv[1], "linefilt")) {   // linefilt W H plane.f32 angle r mode [angle2 r2]
    if (W > 512 || H > 512) { fprintf(stderr, "kernel tables hold <= 512 entries\n"); return 1; }
    if (argc < 8) { fprintf(stderr, "linefilt needs angle r mode\n"); return 1; }
    float *src = load(argv[4], W * H), *dst = calloc(W * H, 4);
    unsigned char *k = calloc(1, 0x4000);
    ((void (*)(void *, long, long, float))FN(0x990448))(k, W, H, (float)atof(argv[5]));
    ((void (*)(void *, float *, float *, long, unsigned))FN(0x9944c0))(k, src, dst, atol(argv[6]), (unsigned)atol(argv[7]));
    if (argc > 9) {
      float *dst2 = calloc(W * H, 4);
      unsigned char *k2 = calloc(1, 0x4000);
      ((void (*)(void *, long, long, float))FN(0x990448))(k2, W, H, (float)atof(argv[8]));
      ((void (*)(void *, float *, float *, long, unsigned))FN(0x9944c0))(k2, dst, dst2, atol(argv[9]), (unsigned)atol(argv[7]));
      dst = dst2;
    }
    for (long i = 0; i < W * H; i++) printf("%ld %.9g\n", i, dst[i]);
    return 0;
  }
  if (!strcmp(argv[1], "pixnoise")) {   // pixnoise W H luma.f32 out_prefix
    if (W > 512 || H > 512) { fprintf(stderr, "FUN_00998d98 works on tiles <= 512 px\n"); return 1; }
    if (argc < 6) { fprintf(stderr, "pixnoise needs out_prefix\n"); return 1; }
    unsigned char *core = make_core(1);
    ((void (*)(void *))FN(0x9b4428))(core);
    void *ref = *(void **)(base + 0x1561db0);
    float *luma = load(argv[4], W * H);
    float *wt = malloc(W * H * 4), *bright = calloc(W * H, 4), *noise = calloc(W * H, 4);
    for (long i = 0; i < W * H; i++) wt[i] = 1.0f;
    long ctx = 0;
    unsigned char pool[0x50] __attribute__((aligned(16)));
    memset(pool, 0, sizeof pool);
    ((void (*)(void *))FN(0x9d7bc4))(pool);
    ((void (*)(long *, float *, float *, float *, float *, long, long, void *, void *))FN(0x998d98))(
        &ctx, luma, wt, bright, noise, W, H, ref, pool);
    char name[1024];
    const char *suf[3] = {".noise.f32", ".weight.f32", ".bright.f32"};
    float *arr[3] = {noise, wt, bright};
    for (int j = 0; j < 3; j++) {
      snprintf(name, sizeof name, "%s%s", argv[5], suf[j]);
      FILE *f = fopen(name, "wb");
      if (!f) { perror(name); return 1; }
      fwrite(arr[j], 4, W * H, f);
      fclose(f);
    }
    printf("pixnoise done\n");
    return 0;
  }
  if (!strcmp(argv[1], "profile")) {
    long area[4], *ap = NULL;
    if (argc > 10) { for (int i = 0; i < 4; i++) area[i] = atol(argv[7 + i]); ap = area; }
    return profile(W, H, load(argv[4], 3 * W * H), atol(argv[5]), argv[6], ap);
  }
  if (!strcmp(argv[1], "flat"))
    return flat(W, H, load(argv[4], 3 * W * H), argc > 5 ? atol(argv[5]) : 1, argc > 6 ? (unsigned)atol(argv[6]) : 0);
  float *pl = load(argv[4], W * H);
  float *out = calloc(1, 0x4000);
  if (!strcmp(argv[1], "levels")) {
    long ctx = 0;
    ((void (*)(long *, float *, long, long, float *))FN(0x9980e8))(&ctx, pl, W, H, out);
  } else if (!strcmp(argv[1], "model")) {
    long ctx = 0; long set = argc > 5 ? atol(argv[5]) : -1;
    unsigned char pool[0x50] __attribute__((aligned(16)));
    memset(pool, 0, sizeof pool);
    ((void (*)(void *))FN(0x9d7bc4))(pool);
    ((void (*)(long *, float *, long, long, float *, long, long, void *))FN(0x995250))(
        &ctx, pl, W, H, out, 5, set, pool);
  } else { fprintf(stderr, "unknown mode\n"); return 1; }
  // dump the whole struct: index, value (%.9g round-trips float32), bit pattern (-0, NaN payloads)
  for (int i = 0; i < 0x4000 / 4; i++) printf("%d %.9g %08x\n", i, out[i], bits32(out[i]));
  return 0;
}
