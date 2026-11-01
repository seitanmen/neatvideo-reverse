/* fsguard - a file / process access monitor for the OFX test host (N4b).  Injected with
 * DYLD_INSERT_LIBRARIES=libfsguard.dylib, it interposes (dyld __interpose) the libc entry points through which a
 * program reaches files, pipes and other processes, for every image of the process (the host, MeatVideo.ofx,
 * libnvcore.dylib, libc++, ...).  While armed (fsguard_arm(1), called by minihost around the plug-in session) each
 * call is recorded; fsguard_report() prints them.  Calls are always passed on unchanged.
 *
 * Interposed: open, openat, fopen, freopen, fdopen, opendir, stat, lstat, fstatat, access, faccessat, mkdir, unlink,
 * rename, creat, popen, system, fork, execve, execv, execvp, posix_spawn, posix_spawnp, dlopen (RTLD_NOLOAD recorded as
 * dlopen_noload), and read /
 * pread on file descriptor 0 (stdin).  The positive control is minihost --fsguard-selftest (calls
 * from another image: dyld does not interpose a library's calls to itself). */
#include <dirent.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define INTERPOSE(rep, orig)                                                                                     \
    __attribute__((used)) static struct {                                                                       \
        const void* r;                                                                                          \
        const void* o;                                                                                          \
    } interpose_##orig __attribute__((section("__DATA,__interpose"))) = {(const void*)(unsigned long)&rep,       \
                                                                          (const void*)(unsigned long)&orig}

#define MAXREC 4096
static volatile int armed = 0;
static char rec[MAXREC][320];
static int nrec = 0, ntotal = 0;

static void note(const char* fn, const char* arg) {
    if (!armed) return;
    armed = 0;                      /* no recursion through snprintf */
    if (nrec < MAXREC) snprintf(rec[nrec++], sizeof rec[0], "%s(%s)", fn, arg ? arg : "");
    ++ntotal;
    armed = 1;
}

__attribute__((visibility("default"))) void fsguard_arm(int on) { armed = on; }

__attribute__((visibility("default"))) int fsguard_report(FILE* f) {
    fprintf(f, "fsguard: %d file/process call(s) while armed%s\n", ntotal, ntotal > nrec ? " (first ones listed)" : "");
    for (int i = 0; i < nrec; ++i) fprintf(f, "fsguard:   %s\n", rec[i]);
    return ntotal;
}

__attribute__((visibility("default"))) void fsguard_reset(void) { nrec = ntotal = 0; }

/* FSGUARD_ALWAYS=1: armed from load to exit, the report on stderr at exit (a program that does not call fsguard_arm,
 * e.g. the window helper) */
static void report_at_exit(void) {
    armed = 0;
    fsguard_report(stderr);
}
__attribute__((constructor)) static void always(void) {
    const char* e = getenv("FSGUARD_ALWAYS");
    if (e && *e == '1') {
        atexit(report_at_exit);
        armed = 1;
    }
}

static int my_open(const char* p, int fl, ...) {
    int mode = 0;
    if (fl & O_CREAT) {
        va_list ap;
        va_start(ap, fl);
        mode = va_arg(ap, int);
        va_end(ap);
    }
    note("open", p);
    return open(p, fl, mode);
}
INTERPOSE(my_open, open);

static int my_openat(int d, const char* p, int fl, ...) {
    int mode = 0;
    if (fl & O_CREAT) {
        va_list ap;
        va_start(ap, fl);
        mode = va_arg(ap, int);
        va_end(ap);
    }
    note("openat", p);
    return openat(d, p, fl, mode);
}
INTERPOSE(my_openat, openat);

static FILE* my_fopen(const char* p, const char* m) {
    note("fopen", p);
    return fopen(p, m);
}
INTERPOSE(my_fopen, fopen);

static FILE* my_freopen(const char* p, const char* m, FILE* s) {
    note("freopen", p);
    return freopen(p, m, s);
}
INTERPOSE(my_freopen, freopen);

static FILE* my_fdopen(int d, const char* m) {
    char b[32];
    snprintf(b, sizeof b, "fd %d", d);
    note("fdopen", b);
    return fdopen(d, m);
}
INTERPOSE(my_fdopen, fdopen);

static DIR* my_opendir(const char* p) {
    note("opendir", p);
    return opendir(p);
}
INTERPOSE(my_opendir, opendir);

static int my_stat(const char* p, struct stat* s) {
    note("stat", p);
    return stat(p, s);
}
INTERPOSE(my_stat, stat);

static int my_lstat(const char* p, struct stat* s) {
    note("lstat", p);
    return lstat(p, s);
}
INTERPOSE(my_lstat, lstat);

static int my_fstatat(int d, const char* p, struct stat* s, int fl) {
    note("fstatat", p);
    return fstatat(d, p, s, fl);
}
INTERPOSE(my_fstatat, fstatat);

static int my_access(const char* p, int m) {
    note("access", p);
    return access(p, m);
}
INTERPOSE(my_access, access);

static int my_faccessat(int d, const char* p, int m, int fl) {
    note("faccessat", p);
    return faccessat(d, p, m, fl);
}
INTERPOSE(my_faccessat, faccessat);

static int my_mkdir(const char* p, mode_t m) {
    note("mkdir", p);
    return mkdir(p, m);
}
INTERPOSE(my_mkdir, mkdir);

static int my_unlink(const char* p) {
    note("unlink", p);
    return unlink(p);
}
INTERPOSE(my_unlink, unlink);

static int my_rename(const char* a, const char* b) {
    note("rename", a);
    return rename(a, b);
}
INTERPOSE(my_rename, rename);

static int my_creat(const char* p, mode_t m) {
    note("creat", p);
    return creat(p, m);
}
INTERPOSE(my_creat, creat);

static FILE* my_popen(const char* c, const char* m) {
    note("popen", c);
    return popen(c, m);
}
INTERPOSE(my_popen, popen);

static int my_system(const char* c) {
    note("system", c);
    return system(c);
}
INTERPOSE(my_system, system);

static pid_t my_fork(void) {
    note("fork", "");
    return fork();
}
INTERPOSE(my_fork, fork);

static int my_execve(const char* p, char* const a[], char* const e[]) {
    note("execve", p);
    return execve(p, a, e);
}
INTERPOSE(my_execve, execve);

static int my_execv(const char* p, char* const a[]) {
    note("execv", p);
    return execv(p, a);
}
INTERPOSE(my_execv, execv);

static int my_execvp(const char* p, char* const a[]) {
    note("execvp", p);
    return execvp(p, a);
}
INTERPOSE(my_execvp, execvp);

static int my_posix_spawn(pid_t* pid, const char* p, const posix_spawn_file_actions_t* fa, const posix_spawnattr_t* at,
                          char* const a[], char* const e[]) {
    char what[300];                 /* the path and the first argument (which mode of the helper) */
    snprintf(what, sizeof what, "%s%s%s", p ? p : "", a && a[0] && a[1] ? " " : "", a && a[0] && a[1] ? a[1] : "");
    note("posix_spawn", what);
    return posix_spawn(pid, p, fa, at, a, e);
}
INTERPOSE(my_posix_spawn, posix_spawn);

static int my_posix_spawnp(pid_t* pid, const char* p, const posix_spawn_file_actions_t* fa, const posix_spawnattr_t* at,
                           char* const a[], char* const e[]) {
    note("posix_spawnp", p);
    return posix_spawnp(pid, p, fa, at, a, e);
}
INTERPOSE(my_posix_spawnp, posix_spawnp);

static void* my_dlopen(const char* p, int m) {
    /* RTLD_NOLOAD only looks the image up among those already loaded (no file is opened): recorded apart */
    note((m & RTLD_NOLOAD) ? "dlopen_noload" : "dlopen", p);
    if (armed && getenv("FSGUARD_BACKTRACE")) {          /* who asked (diagnosis only) */
        void* bt[32];
        backtrace_symbols_fd(bt, backtrace(bt, 32), 2);
    }
    return dlopen(p, m);
}
INTERPOSE(my_dlopen, dlopen);

static ssize_t my_read(int d, void* b, size_t n) {
    /* a run of reads of stdin is listed once (counted each time) */
    if (d == 0 && armed && nrec > 0 && strcmp(rec[nrec - 1], "read(stdin)") == 0) ++ntotal;
    else if (d == 0) note("read", "stdin");
    return read(d, b, n);
}
INTERPOSE(my_read, read);

static ssize_t my_pread(int d, void* b, size_t n, off_t o) {
    if (d == 0) note("pread", "stdin");
    return pread(d, b, n, o);
}
INTERPOSE(my_pread, pread);
