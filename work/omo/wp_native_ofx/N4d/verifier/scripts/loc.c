#include <locale.h>
#include <stdio.h>
__attribute__((constructor)) static void setloc(void) { const char* r = setlocale(LC_ALL, "de_DE.UTF-8"); char b[32]; snprintf(b, sizeof b, "%.3f", 1.5); fprintf(stderr, "LOC %s %s\n", r ? r : "(null)", b); }
