// nv_lstsq - numpy.polyfit / numpy.polyval as numpy 2.1.3 computes them on this machine (lstsq -> LAPACK
// dgelsd of the anaconda OpenBLAS 0.3.29, DYNAMIC_ARCH core neoversen1), for the small systems of
// nvfilter._fill_curve (m <= 8 points, degree 1 or 2).  dgelsd and the LAPACK / BLAS routines on its path
// are transcribed from the OpenBLAS 0.3.29 lapack-netlib sources with the operation order of the compiled
// library (gfortran FMA contractions and the NEOVERSEN1 BLAS kernels, read from the disassembly); checked
// bit for bit against the library (native/verify_prof5.py --lstsq, sandbox/omo/wp_qtui_a/prof5/).
#pragma once

namespace nvl5 {

// numpy.polyfit(x, y, deg) with the default rcond (len(x) * eps): coef[0..deg] (highest power first).
// Range: m >= deg + 1 (the over- / exactly determined systems; m <= 64).  Checked bit for bit against numpy for
// deg <= 6 (sandbox/omo/coord_n3a/VERDICT_n3a.md: deg 7 differs in the last bits, under-determined systems
// (m <= deg) return false where numpy gives the minimum-norm solution, x all equal gives NaN / false where numpy
// raises LinAlgError).  nvfilter._fill_curve uses deg <= 2 on 2..8 distinct integer x.  Returns false when dgelsd
// reports a failure (numpy raises LinAlgError) or the system is under-determined.
bool polyfit(const double* x, const double* y, int m, int deg, double* coef);

// numpy.polyval(p, x) for a scalar x: Horner without fused operations (y = y * x + p[i], y = 0 first).
double polyval(const double* p, int np, double x);

}  // namespace nvl5
