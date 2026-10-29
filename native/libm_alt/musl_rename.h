/* W2a: forced include (-include) for the musl sources: the public names get the nv_mu_ prefix so that
   they cannot bind in place of the system libm; every other symbol is made local by `ld -r`
   (Makefile.libm_alt).  The sources themselves are not edited. */
#define pow nv_mu_pow
#define tanf nv_mu_tanf
#define atan2 nv_mu_atan2
#define atan nv_mu_atan
#define sincos nv_mu_sincos
#define sin nv_mu_sin
#define cos nv_mu_cos
#define expf nv_mu_expf
