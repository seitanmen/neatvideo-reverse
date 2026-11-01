#include <dlfcn.h>
#include <stdio.h>
int main(int argc, char** argv) {
    void (*arm)(int) = dlsym(RTLD_DEFAULT, "fsguard_arm");
    int (*rep)(FILE*) = dlsym(RTLD_DEFAULT, "fsguard_report");
    void* L = dlopen(argv[1], RTLD_NOW);
    int (*fl)(const char*, int, float*) = dlsym(L, "nvc_reference_floor");
    float out[51];
    arm(1);
    int r = fl(argv[2], 1, out);
    arm(0);
    printf("nvc_reference_floor(path) = %d\n", r);
    return rep(stdout) >= 1 ? 0 : 1;
}
