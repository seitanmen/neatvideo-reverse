#include <dlfcn.h>
#include <stdio.h>
static void* my_dlopen(const char* p, int m) {
    void* r = dlopen(p, m);
    fprintf(stderr, "[dlopen] %s mode=0x%x NOLOAD=%d -> %p\n", p ? p : "(null)", m, (m & RTLD_NOLOAD) != 0, r);
    return r;
}
__attribute__((used)) static struct { const void* r; const void* o; } ip __attribute__((section("__DATA,__interpose"))) = {(const void*)&my_dlopen, (const void*)&dlopen};
