// nv_spatial6_c - a C ABI over nvs6::plane_filter for ctypes (work/omo/wp_spatial6/tools/e9_v6_spatial.py, the E9
// comparison).  Not part of libnvcore.
//
//   int nvs6_plane_filter(P, W, H, sigma, cfgf, cfgi, model, roi, out, popdir, popcfg, allow_wide, cfg_after_sigma)
//     sigma: the level maps concatenated (level k: (W >> k) x (H >> k), k < levels)
//     cfgf (31 floats): sigma[5] amount[5] c28[5] c3c[5] c78[5] c90[5] b8;  cfgi (8 ints): ca4[5] c8c levels set
//     model: >= 2005 floats;  roi: 4 ints (y0, x0, h, w) or NULL;  out: H x W
//     cfg_after_sigma: 5 floats (the sigma words of the cfg as the genuine leaves it) or NULL
//   returns nvs6::Status (0 OK, 1 NOT_IMPLEMENTED, 2 BAD_ARGUMENT)
#include "nv_spatial6.h"

#include <vector>

extern "C" int nvs6_plane_filter(const float* P, int W, int H, const float* sigma, const float* cfgf, const int* cfgi,
                                 const float* model, const int* roi, float* out, int popdir, int popcfg,
                                 int allow_wide, float* cfg_after_sigma) {
    nvs6::Cfg c;
    for (int k = 0; k < 5; ++k) {
        c.sigma[k] = cfgf[k];
        c.amount[k] = cfgf[5 + k];
        c.c28[k] = cfgf[10 + k];
        c.c3c[k] = cfgf[15 + k];
        c.c78[k] = cfgf[20 + k];
        c.c90[k] = cfgf[25 + k];
        c.ca4[k] = cfgi[k];
    }
    c.b8 = cfgf[30];
    c.c8c = cfgi[5];
    c.levels = cfgi[6];
    c.set = cfgi[7];
    if (c.levels < 1 || c.levels > 5 || W < 1 || H < 1) return nvs6::BAD_ARGUMENT;
    std::vector<const float*> sp;
    size_t off = 0;
    for (int k = 0; k < c.levels; ++k) sp.push_back(sigma + off), off += static_cast<size_t>(W >> k) * (H >> k);
    nvs6::Guard g;
    g.dir_popcount = popdir;
    g.cfg_popcount = popcfg;
    g.allow_wide = allow_wide != 0;
    nvs6::Cfg after;
    const int st = nvs6::plane_filter(P, W, H, sp, c, model, roi, out, g, cfg_after_sigma ? &after : nullptr);
    if (st == nvs6::OK && cfg_after_sigma)
        for (int k = 0; k < 5; ++k) cfg_after_sigma[k] = after.sigma[k];
    return st;
}
