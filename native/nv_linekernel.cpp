// nv_linekernel.cpp - see nv_linekernel.h (moved unchanged from nv_curve.cpp / nv_model.cpp).
#include "nv_linekernel.h"
#include "nv_libm.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace nvl {

namespace {

inline float bits(uint32_t b) {
    float f;
    std::memcpy(&f, &b, 4);
    return f;
}

// float32 constants, copied from the Python side's bit patterns
const float F_0_01 = bits(0x3c23d70au);       // np.float32(0.01)
const float F_PI = bits(0x40490fdbu);         // F32(3.1415927)
const float F_HALF_PI = bits(0x3fc90fdbu);    // F32(1.5707964)
const float F_NEG_HALF_PI = bits(0xbfc90fdbu);
const float F_0_26 = bits(0x3e851eb8u);
const double QUARTER_PI_D = 0x1.921fb54442d18p-1;  // DAT_012436d0

// libSystem's tanf through a volatile pointer: the compiler must not constant-fold or rewrite it
float (*volatile g_tanf)(float) = NV_LIBM(tanf, tanf);

}  // namespace

// nv_lines.LineKernel.__init__ (FUN_00990448)
LineKernel make_kernel(int W, int H, float a) {
    if (!(1 <= W && W <= 512 && 1 <= H && H <= 512)) throw std::invalid_argument("LineKernel side out of 1..512");
    LineKernel k;
    k.W = W;
    k.H = H;
    k.horiz = std::fabs(a) < F_0_01;
    if (k.horiz) return k;
    k.vert = std::fabs(a + F_NEG_HALF_PI) < F_0_01;
    if (k.vert) return k;
    const float folded = a > F_HALF_PI ? (F_PI - a) : a;
    int n, limit, stride;
    float t;
    if (static_cast<double>(folded) <= QUARTER_PI_D) {
        k.flag = 1;
        n = W;
        limit = H;
        stride = W;
        t = g_tanf(folded);
    } else {
        k.flag = 0;
        n = H;
        limit = W;
        stride = 1;
        t = g_tanf(F_HALF_PI - folded);
    }
    std::vector<int64_t> off(n);
    for (int i = 0; i < n; ++i) {
        const float v = t * static_cast<float>(i);
        off[i] = static_cast<int64_t>(std::rint(v + F_0_26));  // fmul, fadd, frintn, fcvtzs
    }
    for (int i = 1; i < n; ++i)
        if (off[i] < off[i - 1]) throw std::invalid_argument("non-monotone line offsets");
    const int64_t maxoff = off[n - 1];
    for (int64_t base = -maxoff; base < limit; ++base) {
        const int64_t start = std::lower_bound(off.begin(), off.end(), -base) - off.begin();
        const int64_t end = (std::lower_bound(off.begin(), off.end(), limit - base) - off.begin()) - 1;
        k.loff.push_back(static_cast<int32_t>(base * stride));
        k.lstart.push_back(static_cast<int32_t>(start));
        k.lcount.push_back(static_cast<int32_t>(end - start + 1));
    }
    k.idx.resize(n);
    for (int i = 0; i < n; ++i) {
        if (k.flag) {
            const int64_t col = a <= F_HALF_PI ? i : (W - 1 - i);
            k.idx[i] = static_cast<int32_t>(col + static_cast<int64_t>(W) * off[i]);
        } else {
            const int64_t row = a <= F_HALF_PI ? i : (H - 1 - i);
            k.idx[i] = static_cast<int32_t>(row * W + off[i]);
        }
    }
    return k;
}

// nv_lines.LinePlan
LinePlan make_plan(const LineKernel& K) {
    LinePlan p;
    const int HW = K.W * K.H;
    p.L = static_cast<int>(K.loff.size());
    for (int c : K.lcount) p.m = std::max(p.m, c);
    p.G.assign(static_cast<size_t>(p.m) * p.L, HW);
    p.inv.assign(HW, -1);
    for (int l = 0; l < p.L; ++l)
        for (int j = 0; j < K.lcount[l]; ++j) {
            const int pix = K.loff[l] + K.idx[K.lstart[l] + j];
            if (pix < 0 || pix >= HW || p.inv[pix] != -1) throw std::logic_error("lines do not partition the plane");
            p.G[static_cast<size_t>(j) * p.L + l] = pix;
            p.inv[pix] = j * p.L + l;
        }
    for (int v : p.inv)
        if (v < 0) throw std::logic_error("lines do not cover the plane");
    return p;
}

}  // namespace nvl
