#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#define INTERPOSE(rep, orig) __attribute__((used)) static struct { const void* r; const void* o; } ip_##orig __attribute__((section("__DATA,__interpose"))) = {(const void*)&rep, (const void*)&orig}
static void lg(const char* s) { int fd = open("/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/verify_n4d/r3/fd/dup.log", O_WRONLY|O_APPEND|O_CREAT, 0644); write(fd, s, strlen(s)); close(fd); }
static const char* kind(int fd) { struct stat st; if (fstat(fd, &st)) return "closed"; return S_ISFIFO(st.st_mode) ? "fifo" : S_ISCHR(st.st_mode) ? "chr" : S_ISSOCK(st.st_mode) ? "sock" : S_ISREG(st.st_mode) ? "reg" : "other"; }
static int my_dup(int o) { char b[200]; char k2[16]; strcpy(k2, kind(2)); int r = dup(o); snprintf(b, sizeof b, "dup(%d)=%d  [fd2 before: %s]\n", o, r, k2); lg(b); return r; }
INTERPOSE(my_dup, dup);
static int my_dup2(int a, int c) { char b[200]; int r = dup2(a, c); snprintf(b, sizeof b, "dup2(%d[%s],%d)=%d\n", a, kind(a), c, r); lg(b); return r; }
INTERPOSE(my_dup2, dup2);
