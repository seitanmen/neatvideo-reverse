#include <fcntl.h>
#include <spawn.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#define INTERPOSE(rep, orig) __attribute__((used)) static struct { const void* r; const void* o; } ip_##orig __attribute__((section("__DATA,__interpose"))) = {(const void*)&rep, (const void*)&orig}
static int my_spawn(pid_t* pid, const char* p, const posix_spawn_file_actions_t* fa, const posix_spawnattr_t* at, char* const a[], char* const e[]) {
    char b[1024]; int n = snprintf(b, sizeof b, "SPAWNFDS %s %s:", p, a[1] ? a[1] : "");
    for (int fd = 3; fd < 512; ++fd) { struct stat st; if (fstat(fd, &st) == 0 && S_ISFIFO(st.st_mode)) n += snprintf(b + n, sizeof b - n, " %d%s", fd, (fcntl(fd, F_GETFD) & FD_CLOEXEC) ? "(cloexec)" : "(INHERITABLE)"); }
    b[n++] = '\n'; write(2, b, n);
    return posix_spawn(pid, p, fa, at, a, e);
}
INTERPOSE(my_spawn, posix_spawn);
