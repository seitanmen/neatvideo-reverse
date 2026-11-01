#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#define INTERPOSE(rep, orig) __attribute__((used)) static struct { const void* r; const void* o; } ip_##orig __attribute__((section("__DATA,__interpose"))) = {(const void*)&rep, (const void*)&orig}
static void lg(const char* f, const char* p) { if (p && strncmp(p, "/System/", 8) && strncmp(p, "/usr/lib", 8) && strncmp(p, "/dev/", 5)) { char b[1200]; int n = snprintf(b, sizeof b, "LOGOPEN %s %s\n", f, p); write(2, b, n); } }
static int my_open(const char* p, int fl, ...) { int m = 0; if (fl & O_CREAT) { va_list a; va_start(a, fl); m = va_arg(a, int); va_end(a);} lg("open", p); return open(p, fl, m); }
INTERPOSE(my_open, open);
static int my_stat(const char* p, struct stat* s) { lg("stat", p); return stat(p, s); }
INTERPOSE(my_stat, stat);
static int my_access(const char* p, int m) { lg("access", p); return access(p, m); }
INTERPOSE(my_access, access);
static FILE* my_fopen(const char* p, const char* m) { lg("fopen", p); return fopen(p, m); }
INTERPOSE(my_fopen, fopen);
