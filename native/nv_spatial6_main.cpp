// nv_spatial6_test - command-line driver of nv_spatial6 for work/omo/wp_spatial6/tools (the same argument forms as the
// harness `nvh`, so a leaf can be run on the port and on the genuine with one command line):
//   elem NAME n out s0 s1 s2 s3 in1 [in2 [in3]]          (scalars as float32 hex bits)
//   argmax n count stride maps.f32 out                   -> out.idx, out.max.f32
//   gather n stride idx.u8 maps.f32 out.f32
//   sel2 n flag stride a b i3 i4 best src wt out full    -> out.out.f32 / .best.f32 / .b.f32 / .i4
//   strip3 W H plane.f32 box|sq mode sbits out.f32
//   down2 W H plane.f32 out.f32
//   plane W H P.f32 sigma.f32 model.f32 cfg.txt out.f32 [roi y x h w] [rec DIR] [popdir P] [popcfg P]
//     popdir / popcfg: the popcounts of nvs6::Guard (default 16)
//     sigma.f32 = the level maps of FUN_009aae90 concatenated (level k: (W >> k) x (H >> k));
//     cfg.txt = lines "key v..." (float values as hex bits): sigma amount c28 c3c c78 c90 (5 each), ca4 (5 ints),
//     c8c b8 levels set.  Exit 0 = OK, 3 = NOT_IMPLEMENTED, 2 = bad argument.
#include "nv_spatial6.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::vector<float> loadf(const char* p, size_t n) {
    std::vector<float> v(n + 4, 0.0f);
    FILE* f = std::fopen(p, "rb");
    if (!f || std::fread(v.data(), 4, n, f) != n) {
        std::fprintf(stderr, "read %s failed\n", p);
        std::exit(1);
    }
    std::fclose(f);
    return v;
}
std::vector<uint8_t> loadb(const char* p, size_t n) {
    std::vector<uint8_t> v(n + 4, 0);
    FILE* f = std::fopen(p, "rb");
    if (!f || std::fread(v.data(), 1, n, f) != n) {
        std::fprintf(stderr, "read %s failed\n", p);
        std::exit(1);
    }
    std::fclose(f);
    return v;
}
void save(const std::string& p, const void* d, size_t bytes) {
    FILE* f = std::fopen(p.c_str(), "wb");
    if (!f) {
        std::perror(p.c_str());
        std::exit(1);
    }
    std::fwrite(d, 1, bytes, f);
    std::fclose(f);
}
float hexf(const char* s) {
    uint32_t u = static_cast<uint32_t>(std::strtoul(s, nullptr, 16));
    float f;
    std::memcpy(&f, &u, 4);
    return f;
}

int elem(int argc, char** argv) {
    struct E { const char* nm; int ns, ni; };
    static const E T[] = {{"recip", 1, 1}, {"scale", 1, 1}, {"affine", 2, 1}, {"sqr", 0, 1}, {"sqrtabs", 0, 1},
                          {"sqrtk", 1, 1}, {"mul", 0, 2}, {"add", 0, 2}, {"sub", 0, 2}, {"blend", 2, 2},
                          {"rstep", 1, 2}, {"rinv", 1, 2}, {"clampmul", 4, 2}, {"acc", 1, 3}, {"robust", 1, 2},
                          {"ratio", 1, 2}, {"bladd", 2, 2}, {"ramp4", 4, 1}, {"wmul4", 4, 2}, {"pairw", 0, 3}, {"fabs", 0, 1}};
    if (argc < 9) return 2;
    const std::string nm = argv[2];
    int t = -1;
    for (int i = 0; i < static_cast<int>(sizeof T / sizeof T[0]); ++i)
        if (nm == T[i].nm) t = i;
    const size_t n = std::strtoul(argv[3], nullptr, 10);
    if (t < 0 || n < 1 || argc < 9 + T[t].ni) return 2;
    float s[4];
    for (int i = 0; i < 4; ++i) s[i] = hexf(argv[5 + i]);
    std::vector<float> in[3];
    for (int i = 0; i < T[t].ni; ++i) in[i] = loadf(argv[9 + i], n);
    std::vector<float> o(n + 4);
    float* out = o.data();
    const float* a = in[0].data();
    const float* b = T[t].ni > 1 ? in[1].data() : nullptr;
    const float* c = T[t].ni > 2 ? in[2].data() : nullptr;
    using namespace nvs6;
    if (nm == "recip") recip(s[0], a, out, n);
    else if (nm == "scale") scale(s[0], a, out, n);
    else if (nm == "affine") affine(s[0], s[1], a, out, n);
    else if (nm == "sqr") sqr(a, out, n);
    else if (nm == "sqrtabs") sqrtabs(a, out, n);
    else if (nm == "fabs") nvs6::fabs(a, out, n);
    else if (nm == "sqrtk") sqrtk(s[0], a, out, n);
    else if (nm == "mul") mul(a, b, out, n);
    else if (nm == "add") add(a, b, out, n);
    else if (nm == "sub") sub(a, b, out, n);
    else if (nm == "blend") blend(s[0], s[1], a, b, out, n);
    else if (nm == "rstep") rstep(s[0], a, b, out, n);
    else if (nm == "rinv") rinv(s[0], a, b, out, n);
    else if (nm == "clampmul") clampmul(s[0], s[1], s[2], s[3], a, b, out, n);
    else if (nm == "acc") acc(s[0], a, b, c, out, n);
    else if (nm == "robust") robust(s[0], a, b, out, n);
    else if (nm == "ratio") ratio(s[0], a, b, out, n);
    else if (nm == "bladd") bladd(s[0], s[1], a, b, out, n);
    else if (nm == "ramp4") ramp(s[0], s[1], s[2], s[3], a, out, n);
    else if (nm == "wmul4") wmul(s[0], s[1], s[2], s[3], a, b, out, n);
    else if (nm == "pairw") pairw(a, b, c, out, n);
    save(argv[4], out, n * 4);
    return 0;
}

int plane(int argc, char** argv) {
    if (argc < 9) return 2;
    const int W = std::atoi(argv[2]), H = std::atoi(argv[3]);
    nvs6::Cfg cfg;
    std::ifstream cf(argv[7]);
    std::string line;
    while (std::getline(cf, line)) {
        std::istringstream is(line);
        std::string key;
        is >> key;
        std::vector<std::string> v;
        for (std::string t; is >> t;) v.push_back(t);
        auto fv = [&](float* dst, int m) { for (int i = 0; i < m && i < static_cast<int>(v.size()); ++i) dst[i] = hexf(v[i].c_str()); };
        if (key == "sigma") fv(cfg.sigma, 5);
        else if (key == "amount") fv(cfg.amount, 5);
        else if (key == "c28") fv(cfg.c28, 5);
        else if (key == "c3c") fv(cfg.c3c, 5);
        else if (key == "c78") fv(cfg.c78, 5);
        else if (key == "c90") fv(cfg.c90, 5);
        else if (key == "ca4") for (int i = 0; i < 5 && i < static_cast<int>(v.size()); ++i) cfg.ca4[i] = std::atoi(v[i].c_str());
        else if (key == "c8c") cfg.c8c = std::atoi(v.at(0).c_str());
        else if (key == "b8") cfg.b8 = hexf(v.at(0).c_str());
        else if (key == "levels") cfg.levels = std::atoi(v.at(0).c_str());
        else if (key == "set") cfg.set = std::atoi(v.at(0).c_str());
    }
    size_t tot = 0;
    for (int k = 0; k < cfg.levels; ++k) tot += static_cast<size_t>(W >> k) * (H >> k);
    const std::vector<float> P = loadf(argv[4], static_cast<size_t>(W) * H);
    const std::vector<float> sg = loadf(argv[5], tot);
    const std::vector<float> model = loadf(argv[6], 2005);
    std::vector<const float*> sp;
    size_t off = 0;
    for (int k = 0; k < cfg.levels; ++k) sp.push_back(sg.data() + off), off += static_cast<size_t>(W >> k) * (H >> k);
    int roi[4];
    bool have_roi = false;
    nvs6::Guard g;
    std::unique_ptr<nvs6::Recorder> rec;
    for (int i = 9; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "roi" && i + 4 < argc) {
            for (int j = 0; j < 4; ++j) roi[j] = std::atoi(argv[i + 1 + j]);
            have_roi = true;
            i += 4;
        } else if (a == "rec" && i + 1 < argc) {
            rec = std::make_unique<nvs6::Recorder>(argv[++i]);
        } else if (a == "popdir" && i + 1 < argc) {
            g.dir_popcount = std::atoi(argv[++i]);
        } else if (a == "popcfg" && i + 1 < argc) {
            g.cfg_popcount = std::atoi(argv[++i]);
        } else {
            return 2;
        }
    }
    nvs6::set_recorder(rec.get());
    std::vector<float> out(static_cast<size_t>(W) * H);
    const int st = nvs6::plane_filter(P.data(), W, H, sp, cfg, model.data(), have_roi ? roi : nullptr, out.data(), g);
    nvs6::set_recorder(nullptr);
    if (st == nvs6::NOT_IMPLEMENTED) return 3;
    if (st != nvs6::OK) return 2;
    save(argv[8], out.data(), out.size() * 4);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) return 2;
    const std::string cmd = argv[1];
    if (cmd == "elem") return elem(argc, argv);
    if (cmd == "plane") return plane(argc, argv);
    if (cmd == "argmax" && argc > 6) {
        const size_t n = std::strtoul(argv[2], nullptr, 10), stride = std::strtoul(argv[4], nullptr, 10);
        const int count = std::atoi(argv[3]);
        const std::vector<float> maps = loadf(argv[5], count ? count * stride : 1);
        std::vector<uint8_t> idx(n);
        std::vector<float> mx(n);
        nvs6::argmax(maps.data(), idx.data(), mx.data(), n, stride, count);
        save(std::string(argv[6]) + ".max.f32", mx.data(), n * 4);
        save(std::string(argv[6]) + ".idx", idx.data(), n);
        return 0;
    }
    if (cmd == "gather" && argc > 6) {
        const size_t n = std::strtoul(argv[2], nullptr, 10), stride = std::strtoul(argv[3], nullptr, 10);
        const std::vector<uint8_t> idx = loadb(argv[4], n);
        const std::vector<float> maps = loadf(argv[5], 256 * stride);
        std::vector<float> out(n);
        nvs6::gather(idx.data(), maps.data(), out.data(), stride, n);
        save(argv[6], out.data(), n * 4);
        return 0;
    }
    if (cmd == "sel2" && argc > 12) {
        const size_t n = std::strtoul(argv[2], nullptr, 10), stride = std::strtoul(argv[4], nullptr, 10);
        const int flag = std::atoi(argv[3]) & 1;
        std::vector<float> a = loadf(argv[5], n), b = loadf(argv[6], n), best = loadf(argv[9], n);
        const std::vector<uint8_t> i3 = loadb(argv[7], n);
        std::vector<uint8_t> i4 = loadb(argv[8], n);
        const std::vector<float> src = loadf(argv[10], 256 * stride), wt = loadf(argv[11], 256 * stride);
        std::vector<float> out(n);
        nvs6::sel2(a.data(), b.data(), i3.data(), i4.data(), out.data(), best.data(), src.data(), wt.data(), flag, stride, n);
        const std::string p = argv[12];
        save(p + ".out.f32", out.data(), n * 4);
        save(p + ".best.f32", best.data(), n * 4);
        save(p + ".b.f32", b.data(), n * 4);
        save(p + ".i4", i4.data(), n);
        return 0;
    }
    if (cmd == "strip3" && argc > 8) {
        const int W = std::atoi(argv[2]), H = std::atoi(argv[3]), mode = std::atoi(argv[6]);
        if (W < 4 || H < 2) return 2;
        const std::vector<float> x = loadf(argv[4], static_cast<size_t>(W) * H);
        std::vector<float> o(static_cast<size_t>(W) * H);
        if (!std::strcmp(argv[5], "sq")) nvs6::strip_rms3(x.data(), W, H, mode, hexf(argv[7]), o.data());
        else nvs6::strip_box3(x.data(), W, H, mode, o.data());
        save(argv[8], o.data(), o.size() * 4);
        return 0;
    }
    if (cmd == "down2" && argc > 5) {
        const int W = std::atoi(argv[2]), H = std::atoi(argv[3]);
        const std::vector<float> x = loadf(argv[4], static_cast<size_t>(W) * H);
        std::vector<float> o(static_cast<size_t>(W >> 1) * (H >> 1));
        nvs6::down2(x.data(), W, H, o.data());
        save(argv[5], o.data(), o.size() * 4);
        return 0;
    }
    return 2;
}
