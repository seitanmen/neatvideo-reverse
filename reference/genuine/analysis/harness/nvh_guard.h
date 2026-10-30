// Safety guard for calling into the genuine bundle from a CLI harness.
// The bundle's assertion handler (FUN_009d95a4) shows a modal system alert via
// CFUserNotificationDisplayAlert, and some assertion sites loop on it forever.
//
// Use: nvh_guard_preload() BEFORE dlopen of the bundle, nvh_guard_check() right after it.
//  - preload loads AppKit, replaces -[NSAlert runModal] and registers a dyld add-image
//    callback. dyld runs that callback once the bundle is mapped and bound, before its static
//    initialisers; for an OFX bundle (MH_BUNDLE exporting _OfxGetPlugin, any build) it rebinds the
//    bundle's pointers to _CFUserNotificationDisplayAlert to a stub that prints the message and
//    exits with code 3, and verifies that the bundle's __stubs entry loads the rebound slot.
//  - check (after dlopen) exits with 5 if the callback did not run or the slot no longer holds
//    the stub, and with 7 if the loaded bundle is not the analysed build (offsets would be wrong).
// `nvh order` reports whether the callback ran before the bundle's initialisers.
#include <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <objc/runtime.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// LC_UUID of the arm64 slice analysed in Ghidra (NeatVideo6.arm64.ofx, Agents.md §2)
static const uint8_t NVH_UUID[16] = {0xA9, 0xDF, 0x38, 0x75, 0x4B, 0x77, 0x31, 0xEB,
                                     0xB8, 0xC6, 0x54, 0x3A, 0x30, 0xF2, 0x1C, 0x9F};
static int nvh_guard_ok;                 // 1 once the rebinding was installed and verified
static int nvh_uuid_ok;                  // the guarded bundle is the analysed build (offsets valid)
static const struct mach_header_64 *nvh_mh;
static intptr_t nvh_slide;
static unsigned char *nvh_data_snap;    // copy of the bundle's __DATA taken in the callback
static uintptr_t nvh_data_lo, nvh_data_hi;

static SInt32 nvh_alert_stub(CFTimeInterval t, CFOptionFlags f, CFURLRef i, CFURLRef s, CFURLRef l,
                             CFStringRef head, CFStringRef msg, CFStringRef b1, CFStringRef b2,
                             CFStringRef b3, CFOptionFlags *resp) {
  char h[256] = "", m[2048] = "";
  (void)t; (void)f; (void)i; (void)s; (void)l; (void)b1; (void)b2; (void)b3; (void)resp;
  if (head) CFStringGetCString(head, h, sizeof h, kCFStringEncodingUTF8);
  if (msg) CFStringGetCString(msg, m, sizeof m, kCFStringEncodingUTF8);
  fprintf(stderr, "[nvh] genuine alert suppressed: %s\n%s\n", h, m);
  _exit(3);
}

static long nvh_runmodal_stub(id self, SEL cmd) {
  (void)self; (void)cmd;
  fprintf(stderr, "[nvh] NSAlert runModal suppressed\n");
  _exit(4);
}

static int nvh_has_uuid(const struct mach_header_64 *mh) {
  if (mh->magic != MH_MAGIC_64) return 0;
  const struct load_command *lc = (const void *)(mh + 1);
  for (uint32_t c = 0; c < mh->ncmds; c++, lc = (const void *)((const char *)lc + lc->cmdsize))
    if (lc->cmd == LC_UUID) return !memcmp(((const struct uuid_command *)lc)->uuid, NVH_UUID, 16);
  return 0;
}

static int nvh_tables(const struct mach_header_64 *mh, intptr_t slide, const struct nlist_64 **syms,
                      const char **strs, const uint32_t **ind) {
  const struct load_command *lc = (const void *)(mh + 1);
  const struct symtab_command *st = NULL;
  const struct dysymtab_command *dy = NULL;
  const struct segment_command_64 *le = NULL;
  for (uint32_t c = 0; c < mh->ncmds; c++, lc = (const void *)((const char *)lc + lc->cmdsize)) {
    if (lc->cmd == LC_SYMTAB) st = (const void *)lc;
    else if (lc->cmd == LC_DYSYMTAB) dy = (const void *)lc;
    else if (lc->cmd == LC_SEGMENT_64 && !strcmp(((const struct segment_command_64 *)lc)->segname, "__LINKEDIT"))
      le = (const void *)lc;
  }
  if (!st || !dy || !le) return 0;
  uintptr_t lb = (uintptr_t)slide + le->vmaddr - le->fileoff;
  *syms = (const void *)(lb + st->symoff);
  *strs = (const char *)(lb + st->stroff);
  *ind = (const uint32_t *)(lb + dy->indirectsymoff);
  return 1;
}

// Rebind every lazy / non-lazy pointer of `sym` in the image to `repl`; returns the count.
static int nvh_rebind(const struct mach_header_64 *mh, intptr_t slide, const char *sym, void *repl) {
  const struct nlist_64 *syms; const char *strs; const uint32_t *ind;
  if (!nvh_tables(mh, slide, &syms, &strs, &ind)) return -1;
  int n = 0;
  const struct load_command *lc = (const void *)(mh + 1);
  for (uint32_t c = 0; c < mh->ncmds; c++, lc = (const void *)((const char *)lc + lc->cmdsize)) {
    if (lc->cmd != LC_SEGMENT_64) continue;
    const struct segment_command_64 *sg = (const void *)lc;
    const struct section_64 *sc = (const void *)(sg + 1);
    for (uint32_t k = 0; k < sg->nsects; k++, sc++) {
      uint32_t ty = sc->flags & SECTION_TYPE;
      if (ty != S_LAZY_SYMBOL_POINTERS && ty != S_NON_LAZY_SYMBOL_POINTERS) continue;
      void **ptr = (void **)((uintptr_t)slide + sc->addr);
      for (uint64_t j = 0; j < sc->size / 8; j++) {
        uint32_t si = ind[sc->reserved1 + j];
        if (si & (INDIRECT_SYMBOL_ABS | INDIRECT_SYMBOL_LOCAL)) continue;
        if (strcmp(strs + syms[si].n_un.n_strx, sym)) continue;
        uintptr_t pg = (uintptr_t)&ptr[j] & ~(uintptr_t)(getpagesize() - 1);
        if (mprotect((void *)pg, getpagesize(), PROT_READ | PROT_WRITE)) return -2;
        ptr[j] = repl;
        n++;
      }
    }
  }
  return n;
}

// The __stubs entry of `sym`: decode "adrp x16, P; ldr x16, [x16, #o]; br x16" -> loaded slot.
static void **nvh_stub_slot(const struct mach_header_64 *mh, intptr_t slide, const char *sym) {
  const struct nlist_64 *syms; const char *strs; const uint32_t *ind;
  if (!nvh_tables(mh, slide, &syms, &strs, &ind)) return NULL;
  const struct load_command *lc = (const void *)(mh + 1);
  for (uint32_t c = 0; c < mh->ncmds; c++, lc = (const void *)((const char *)lc + lc->cmdsize)) {
    if (lc->cmd != LC_SEGMENT_64) continue;
    const struct segment_command_64 *sg = (const void *)lc;
    const struct section_64 *sc = (const void *)(sg + 1);
    for (uint32_t k = 0; k < sg->nsects; k++, sc++) {
      if ((sc->flags & SECTION_TYPE) != S_SYMBOL_STUBS || sc->reserved2 == 0) continue;
      for (uint64_t j = 0; j < sc->size / sc->reserved2; j++) {
        uint32_t si = ind[sc->reserved1 + j];
        if (si & (INDIRECT_SYMBOL_ABS | INDIRECT_SYMBOL_LOCAL)) continue;
        if (strcmp(strs + syms[si].n_un.n_strx, sym)) continue;
        uintptr_t pc = (uintptr_t)slide + sc->addr + j * sc->reserved2;
        uint32_t i0 = ((uint32_t *)pc)[0], i1 = ((uint32_t *)pc)[1], i2 = ((uint32_t *)pc)[2];
        if ((i0 & 0x9f00001f) != 0x90000010 || (i1 & 0xffc003ff) != 0xf9400210 || i2 != 0xd61f0200) return NULL;
        int64_t imm = ((int64_t)((i0 >> 5) & 0x7ffff) << 2 | ((i0 >> 29) & 3)) << 12;
        if (imm & ((int64_t)1 << 32)) imm -= (int64_t)1 << 33;
        uintptr_t page = (pc & ~(uintptr_t)0xfff) + imm;
        return (void **)(page + ((i1 >> 10) & 0xfff) * 8);
      }
    }
  }
  return NULL;
}

// An OFX plug-in bundle: MH_BUNDLE whose symbol table exports _OfxGetPlugin. Decided from the
// image's own load commands only (no dyld / objc calls inside the add-image callback).
static int nvh_is_ofx_bundle(const struct mach_header_64 *mh, intptr_t slide) {
  if (mh->magic != MH_MAGIC_64 || mh->filetype != MH_BUNDLE) return 0;
  const struct nlist_64 *syms; const char *strs; const uint32_t *ind;
  if (!nvh_tables(mh, slide, &syms, &strs, &ind)) return 0;
  const struct load_command *lc = (const void *)(mh + 1);
  for (uint32_t c = 0; c < mh->ncmds; c++, lc = (const void *)((const char *)lc + lc->cmdsize)) {
    if (lc->cmd != LC_SYMTAB) continue;
    const struct symtab_command *st = (const void *)lc;
    for (uint32_t i = 0; i < st->nsyms; i++)
      if ((syms[i].n_type & N_EXT) && (syms[i].n_type & N_TYPE) == N_SECT &&
          !strcmp(strs + syms[i].n_un.n_strx, "_OfxGetPlugin"))
        return 1;
  }
  return 0;
}

static void nvh_on_add_image(const struct mach_header *mh32, intptr_t slide) {
  const struct mach_header_64 *mh = (const struct mach_header_64 *)mh32;
  if (nvh_mh || !nvh_is_ofx_bundle(mh, slide)) return;   // every OFX bundle gets the alert guard ...
  nvh_mh = mh;
  nvh_slide = slide;
  nvh_uuid_ok = nvh_has_uuid(mh);           // ... only the analysed build may be called by offset
  int n = nvh_rebind(mh, slide, "_CFUserNotificationDisplayAlert", (void *)nvh_alert_stub);
  void **stub = nvh_stub_slot(mh, slide, "_CFUserNotificationDisplayAlert");
  if (n < 1 || !stub || *stub != (void *)nvh_alert_stub) {
    fprintf(stderr, "[nvh] guard: cannot verify alert rebinding (n=%d stub=%p) - aborting before the "
            "bundle's initialisers\n", n, (void *)stub);
    _exit(5);
  }
  // snapshot __DATA (incl. zero-fill): bytes that differ after dlopen were written by the
  // bundle's initialisers, i.e. after this callback (`nvh order`)
  const struct load_command *lc = (const void *)(mh + 1);
  for (uint32_t c = 0; c < mh->ncmds; c++, lc = (const void *)((const char *)lc + lc->cmdsize))
    if (lc->cmd == LC_SEGMENT_64 && !strcmp(((const struct segment_command_64 *)lc)->segname, "__DATA")) {
      const struct segment_command_64 *sg = (const void *)lc;
      nvh_data_lo = (uintptr_t)slide + sg->vmaddr;
      nvh_data_hi = nvh_data_lo + sg->vmsize;
      nvh_data_snap = malloc(sg->vmsize);
      if (nvh_data_snap) memcpy(nvh_data_snap, (void *)nvh_data_lo, sg->vmsize);
    }
  nvh_guard_ok = 1;
}

static void nvh_guard_preload(void) {
  if (!dlopen("/System/Library/Frameworks/AppKit.framework/AppKit", RTLD_NOW | RTLD_GLOBAL)) {
    fprintf(stderr, "[nvh] guard: AppKit not loadable: %s\n", dlerror());
    _exit(5);
  }
  Class c = objc_getClass("NSAlert");
  Method m = c ? class_getInstanceMethod(c, sel_registerName("runModal")) : NULL;
  if (!m) {
    fprintf(stderr, "[nvh] guard: NSAlert runModal not found\n");
    _exit(5);
  }
  method_setImplementation(m, (IMP)nvh_runmodal_stub);
  _dyld_register_func_for_add_image(nvh_on_add_image);
}

// After dlopen: returns the bundle base (vmaddr 0 image -> header address) or exits.
static uintptr_t nvh_guard_check(void) {
  if (!nvh_guard_ok || !nvh_mh) {
    fprintf(stderr, "[nvh] guard: bundle callback did not run - refusing\n");
    _exit(5);
  }
  if (!nvh_uuid_ok) {
    fprintf(stderr, "[nvh] guard: the loaded bundle is not the analysed build (LC_UUID) - offsets invalid, refusing\n");
    _exit(7);
  }
  void **stub = nvh_stub_slot(nvh_mh, nvh_slide, "_CFUserNotificationDisplayAlert");
  if (!stub || *stub != (void *)nvh_alert_stub) {
    fprintf(stderr, "[nvh] guard: alert slot changed after load - refusing\n");
    _exit(5);
  }
  return (uintptr_t)nvh_mh;
}
