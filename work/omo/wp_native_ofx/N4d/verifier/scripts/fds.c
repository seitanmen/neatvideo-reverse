#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
int main(int argc, char** argv) {
    FILE* f = fopen(getenv("FDS_OUT"), "a");
    fprintf(f, "%s:", argv[1]);
    for (int fd = 0; fd < 1024; ++fd) if (fcntl(fd, F_GETFD) != -1) fprintf(f, " %d", fd);
    sigset_t m; sigprocmask(SIG_BLOCK, NULL, &m);
    int blocked = 0; for (int s = 1; s < 32; ++s) if (sigismember(&m, s)) blocked++;
    struct sigaction sa; int ign = 0; for (int s = 1; s < 32; ++s) if (sigaction(s, NULL, &sa) == 0 && sa.sa_handler == SIG_IGN) ign++;
    fprintf(f, " | blocked %d ignored %d\n", blocked, ign);
    fclose(f);
    if (strcmp(argv[1], "--ofx-default") == 0) { printf("DEFAULTS 0 none none\n"); return 0; }
    char buf[65536]; while (read(0, buf, sizeof buf) > 0) {}
    printf("CANCEL\n"); return 0;
}
