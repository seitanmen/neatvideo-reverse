/* W2a: forced include (-include) for the openlibm sources: the public names get the nv_ol_ prefix so that
   they cannot bind in place of the system libm; every other symbol is made local by `ld -r`
   (Makefile.libm_alt).  The sources themselves are not edited. */
#define pow nv_ol_pow
#define tanf nv_ol_tanf
#define atan2 nv_ol_atan2
#define atan nv_ol_atan
#define sincos nv_ol_sincos
#define sin nv_ol_sin
#define cos nv_ol_cos
#define expf nv_ol_expf
