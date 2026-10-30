// nvcore - see nvcore.h.
#include "nvcore.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>
#include <utility>

#include "nvcore_c.h"
#include "nvcore_c_prof5.h"
#include "nvcore_c_ui.h"
#include "state.h"

namespace nvqt {

namespace {

const char* NAMES[3] = {"Y", "Cb", "Cr"};

// The C ABI takes a plain int(void) cancel function, called from the calling thread or (threads > 1) from the
// native worker threads: each call takes one of a pool of trampolines bound to its query for its duration.
constexpr int NSLOT = 16;
std::mutex slot_mu;
const CancelFn* slot_fn[NSLOT] = {nullptr};

template <int I>
int trampoline() {
    const CancelFn* f = slot_fn[I];
    try {
        return (f && *f && (*f)()) ? 1 : 0;
    } catch (...) {
        return 1;
    }
}

template <int... I>
constexpr std::array<int (*)(), sizeof...(I)> make_tramps(std::integer_sequence<int, I...>) {
    return {&trampoline<I>...};
}
const auto TRAMPS = make_tramps(std::make_integer_sequence<int, NSLOT>{});

struct CancelScope {
    int slot = -1;
    explicit CancelScope(const CancelFn* c) {
        if (!c || !*c) return;
        std::lock_guard<std::mutex> lk(slot_mu);
        for (int i = 0; i < NSLOT; ++i)
            if (!slot_fn[i]) {
                slot_fn[i] = c;
                slot = i;
                return;
            }
        throw std::runtime_error("too many concurrent native calls with a cancel query");
    }
    ~CancelScope() {
        if (slot < 0) return;
        std::lock_guard<std::mutex> lk(slot_mu);
        slot_fn[slot] = nullptr;
    }
    int (*fn() const)() { return slot < 0 ? nullptr : TRAMPS[static_cast<size_t>(slot)]; }
};

PyObj reals(const double* v, int n) { return PyObj::reals(std::vector<double>(v, v + n)); }

PyObj ycbcr_dict(const PyObj& Y, const PyObj& Cb, const PyObj& Cr) {
    PyObj d = PyObj::dict();
    d.set("Y", Y);
    d.set("Cb", Cb);
    d.set("Cr", Cr);
    return d;
}

}  // namespace

Planes to_planes(const Frame& f) {
    Planes p;
    p.H = f.H;
    p.W = f.W;
    p.Hc = f.Hc;
    p.Wc = f.Wc;
    if (f.is_float()) {
        p.Y = f.fY;
        p.Cb = f.fCb;
        p.Cr = f.fCr;
        return p;
    }
    p.Y.assign(f.Y.begin(), f.Y.end());
    p.Cb.assign(f.Cb.begin(), f.Cb.end());
    p.Cr.assign(f.Cr.begin(), f.Cr.end());
    return p;
}

Frame frame_from_planes(Planes&& p) {
    Frame f;
    f.H = p.H;
    f.W = p.W;
    f.Hc = p.Hc;
    f.Wc = p.Wc;
    f.fY = std::move(p.Y);
    f.fCb = std::move(p.Cb);
    f.fCr = std::move(p.Cr);
    return f;
}

int matrix_code(const std::string& m) { return m == "bt601" ? 1 : m == "bt2020" ? 2 : 0; }

std::vector<float> to_rgb(const Planes& p, const std::string& matrix, bool full_range, int clip) {
    std::vector<float> out(static_cast<size_t>(p.H) * p.W * 3);
    nvc_yuv_to_rgb(p.Y.data(), p.Cb.data(), p.Cr.data(), p.H, p.W, p.Hc, p.Wc, matrix_code(matrix), full_range ? 1 : 0,
                   clip, out.data());
    return out;
}

int native_threads() {
    if (const char* e = std::getenv("NV_NATIVE_THREADS")) {
        const int v = std::atoi(e);
        if (v >= 1) return v;
    }
    const unsigned hc = std::thread::hardware_concurrency();
    return std::max(1, std::min(4, hc ? static_cast<int>(hc) : 1));
}

// ---------------------------------------------------------------- profile
PyObj build_profile(const std::vector<Frame>& frames, int f, bool full_range, const std::string& matrix,
                    const std::string& mode, const std::vector<int>* area, const CancelFn& cancel) {
    if (frames.empty()) throw std::runtime_error("no frames");
    f = std::max(0, std::min(f, static_cast<int>(frames.size()) - 1));
    const Planes pl = to_planes(frames[static_cast<size_t>(f)]);
    // the search of a frame below 64 px is nvfilter.find_flat_area_gradient, whose np.gradient needs 2 samples a side
    if (!area && (pl.H < 2 || pl.W < 2))
        throw std::runtime_error("Shape of array too small to calculate a numerical gradient, at least (edge_order + 1) "
                                 "elements are required.");
    std::vector<Planes> more;
    for (int i = f + 1; i < std::min(f + 3, static_cast<int>(frames.size())); ++i)
        more.push_back(to_planes(frames[static_cast<size_t>(i)]));
    const float* planes[3] = {pl.Y.data(), pl.Cb.data(), pl.Cr.data()};
    std::vector<const float*> mp;
    for (const auto& m : more)
        for (int c = 0; c < 3; ++c) mp.push_back(m.p(c));
    const int mcode = mode == "spatial" ? 1 : mode == "temporal" ? 2 : 0;
    int area_n = 0, a[4] = {0, 0, 0, 0};
    if (area) {
        area_n = static_cast<int>(area->size());
        for (int i = 0; i < area_n && i < 4; ++i) a[i] = (*area)[static_cast<size_t>(i)];
    }
    int oi[NVC_P5_I] = {0};
    double od[NVC_P5_D] = {0};
    int st;
    {
        CancelScope cs(&cancel);
        st = nvc_build_profile_v5(planes, pl.H, pl.W, pl.Hc, pl.Wc, more.empty() ? nullptr : mp.data(),
                                  static_cast<int>(more.size()), mp.empty() ? nullptr : mp.data(), full_range ? 1 : 0,
                                  matrix_code(matrix), mcode, 5, area_n, area ? a : nullptr, native_threads(),
                                  cs.fn(), oi, od);
    }
    if (st == 5) throw ProfileCancelled();
    if (st != 0) throw std::runtime_error("nvc_build_profile_v5 failed (" + std::to_string(st) + ")");
    // the dict in nvfilter.build_profile_v5's insertion order (nvcore_c_prof5.h)
    PyObj p = PyObj::dict();
    p.set("version", PyObj::integer(5));
    p.set("sub", PyObj::integer(oi[12]));
    p.set("full_range", PyObj::boolean(oi[13] != 0));
    std::vector<int> ar(oi + 1, oi + 1 + oi[0]);
    p.set("area", PyObj::ints(ar));
    static const char* MODES[3] = {"auto", "spatial", "temporal"};
    p.set("mode", PyObj::str(std::string(MODES[oi[5]]) + (oi[6] ? "+noise-free" : "")));
    p.set("levels", ycbcr_dict(PyObj::integer(oi[7]), PyObj::integer(oi[8]), PyObj::integer(oi[9])));
    p.set("band_ratio", ycbcr_dict(reals(od + 0, oi[7]), reals(od + 5, oi[8]), reals(od + 10, oi[9])));
    p.set("tot_ratio", ycbcr_dict(PyObj::real(od[15]), PyObj::real(od[16]), PyObj::real(od[17])));
    p.set("area_sigma", ycbcr_dict(reals(od + 18, oi[7]), reals(od + 23, oi[8]), reals(od + 28, oi[9])));
    const int n = oi[14];
    p.set("curve", ycbcr_dict(reals(od + 33, n), reals(od + 41, n), reals(od + 49, n)));
    p.set("curve_centers", reals(od + 57, n));
    p.set("flat_status", oi[10] < 0 ? PyObj::none() : PyObj::integer(oi[10]));
    if (oi[11]) p.set("static_fraction", PyObj::real(od[65]));
    p.set("dead_fraction", PyObj::real(od[66]));
    p.set("area_level", PyObj::real(od[67]));
    p.set("sigma", ycbcr_dict(PyObj::real(od[68]), PyObj::real(od[69]), PyObj::real(od[70])));
    p.set("ui_frame", PyObj::integer(f));
    return p;
}

int noisiest_frame(const std::vector<Frame>& frames, const CancelFn& cancel) {
    const int n = static_cast<int>(frames.size());
    if (n == 0) return 0;
    int cand[64];
    const int nc = nvc_noisiest_candidates(n, cand);
    int best = cand[0];
    double best_s = -1.0;
    for (int k = 0; k < nc; ++k) {
        if (cancel && cancel()) throw ProfileCancelled();
        const Frame& fr = frames[static_cast<size_t>(cand[k])];
        std::vector<float> Y(static_cast<size_t>(fr.H) * fr.W);
        for (size_t i = 0; i < Y.size(); ++i) Y[i] = fr.at(0, i);
        double s = 0.0;
        if (nvc_hh1_sigma(Y.data(), fr.H, fr.W, &s) != 1) continue;
        if (s > best_s) {
            best = cand[k];
            best_s = s;
        }
    }
    return best;
}

int aft_frame(const PyObj& base, size_t nframes) {
    const int f = base.has("ui_frame") ? static_cast<int>(base.get("ui_frame").as_int()) : 0;
    return std::max(0, std::min(f, static_cast<int>(nframes) - 1));
}

const std::vector<float>& reference_floor() {
    static std::vector<float> fl;
    static std::once_flag once;
    std::call_once(once, [] {
        fl.assign(51, 0.0f);
        std::vector<std::string> cands;
        if (const char* e = std::getenv("NV_REF_FLOOR")) cands.push_back(e);
        if (QCoreApplication::instance()) {
            const QString d = QCoreApplication::applicationDirPath();
            cands.push_back(QDir(d).filePath("../Resources/data/ref_floor.npy").toStdString());
            cands.push_back(QDir(d).filePath("data/ref_floor.npy").toStdString());
        }
        cands.push_back(std::string(NV_SOURCE_ROOT) + "/data/ref_floor.npy");
        for (const auto& c : cands)
            if (QFileInfo::exists(QString::fromStdString(c)) && nvc_reference_floor(c.c_str(), 1, fl.data()) == 1)
                return;
        nvc_reference_floor(nullptr, native_threads(), fl.data());
    });
    return fl;
}

PyObj run_aft(const PyObj& base_in, const Frame& frame, bool full_range, const std::string& matrix,
              const CancelFn& cancel) {
    // backend._run_aft
    PyObj prof = base_in;
    PyObj pre = prof.get("ui_pre_aft");
    prof.pop("ui_pre_aft");
    if (!pre.is_none()) {
        prof.set("curve", pre.get("curve"));
        prof.set("sigma", pre.get("sigma"));
    }
    PyObj keep = PyObj::dict();
    keep.set("curve", prof.get("curve"));
    keep.set("sigma", prof.get("sigma"));
    const Planes pl = to_planes(frame);
    const std::vector<float> rgb = to_rgb(pl, matrix, full_range, 1);
    const int H = pl.H, W = pl.W;
    // nv_native._apply_aft (mode 0, band 0)
    const PyObj& a = prof.get("area");
    const int ay = static_cast<int>(a[0].as_int()), ax = static_cast<int>(a[1].as_int()),
              ah = static_cast<int>(a[2].as_int());
    const int aw = a.size() == 4 ? static_cast<int>(a[3].as_int()) : ah;
    if (cancel && cancel()) throw Cancelled();
    std::vector<uint8_t> raw(NVC_PROF_BYTES, 0);
    const int threads = native_threads();
    int pst;
    {
        CancelScope cs(&cancel);    // asked inside the BuildNoiseModel (nv_native.profile_blob, G7)
        pst = nvc_profile_build_cancel(rgb.data(), W, H, 1, ax, ay, aw, ah, reference_floor().data(), 1, threads,
                                       cs.fn(), raw.data());
    }
    if (pst == 3) throw Cancelled();
    if (pst != 0)
        throw std::runtime_error("Auto Fine-Tune: the native BuildNoiseModel does not take this area (the Python UI "
                                 "would run the Python AFT)");
    if (cancel && cancel()) throw Cancelled();
    std::vector<uint8_t> aft = raw;
    float out3[3];
    int st;
    {
        CancelScope cs(&cancel);
        st = nvc_aft(rgb.data(), W, H, aft.data(), 0, threads, cs.fn(), out3);
    }
    if (st == 2) throw Cancelled();
    if (st != 0 && st != 1) throw std::runtime_error("Auto Fine-Tune: model error in the native AFT");
    const PyObj& cc = prof.get("curve_centers");
    const int n = static_cast<int>(cc.size());
    std::vector<double> centers = cc.as_doubles();
    std::vector<float> curves(3 * static_cast<size_t>(n));
    double tot[3];
    for (int c = 0; c < 3; ++c) {
        const PyObj& cv = prof.get("curve").get(NAMES[c]);
        for (int i = 0; i < n; ++i) curves[static_cast<size_t>(c * n + i)] = static_cast<float>(cv[static_cast<size_t>(i)].as_double());
        tot[c] = prof.get("tot_ratio").get(NAMES[c]).as_double();
    }
    const double lvl = prof.has("area_level") ? prof.get("area_level").as_double() : 128.0;
    std::vector<double> cw(9), face(3 * static_cast<size_t>(n)), sig(3);
    std::vector<float> cv(3 * static_cast<size_t>(n));
    std::vector<uint8_t> flags(81);
    nvc_aft_combine(raw.data(), aft.data(), centers.data(), n, curves.data(), lvl, tot, 0, cw.data(), face.data(),
                    cv.data(), sig.data(), flags.data());
    for (int c = 0; c < 3; ++c) {
        std::vector<double> v(cv.begin() + c * n, cv.begin() + (c + 1) * n);
        prof.at("curve").set(NAMES[c], PyObj::reals(v));
    }
    prof.set("sigma", ycbcr_dict(PyObj::real(sig[0]), PyObj::real(sig[1]), PyObj::real(sig[2])));
    prof.set("aft_applied", PyObj::boolean(true));
    PyObj fc = PyObj::list();
    for (int j = 0; j < 3; ++j) fc.append(reals(face.data() + j * n, n));
    prof.set("aft_face_curves", fc);
    PyObj cws = PyObj::list();
    for (int c = 0; c < 3; ++c) cws.append(reals(cw.data() + 3 * c, 3));
    prof.set("aft_color_weights", cws);
    prof.set("aft_band", PyObj::integer(0));
    PyObj bf = PyObj::dict();
    static const char* FK[3] = {"marker", "clip", "gt3"};
    for (int k = 0; k < 3; ++k) {
        PyObj rows = PyObj::list();
        for (int c = 0; c < 3; ++c) {
            PyObj row = PyObj::list();
            for (int b = 0; b < 9; ++b) row.append(PyObj::boolean(flags[static_cast<size_t>(27 * k + 9 * c + b)] != 0));
            rows.append(row);
        }
        bf.set(FK[k], rows);
    }
    prof.set("aft_bin_flags", bf);
    prof.pop("aft_reset");
    prof.set("ui_pre_aft", keep);
    return prof;
}

std::vector<double> wn_band_std(int levels) {
    std::vector<double> out(static_cast<size_t>(std::max(levels, 0)));
    if (levels >= 1) nvc_wn_band_std(levels, out.data());
    return out;
}

// ---------------------------------------------------------------- v5 kernels
namespace {

struct Blocks {
    int pi[NVC_PROF_I] = {0};
    double pd[NVC_PROF_D] = {0};
    int qi[NVC_PAR_I] = {0};
    double qd[NVC_PAR_D] = {0};
};

// nv_native._v5_blocks
Blocks v5_blocks(const PyObj& prof, const PyObj& P) {
    Blocks b;
    const std::string sim = P.has("similarity") ? P.get("similarity").as_str() : "v5";
    if (sim != "v5" && sim != "pyramid") throw std::runtime_error("unknown similarity " + sim);
    const PyObj& cc = prof.get("curve_centers");
    const int n = static_cast<int>(cc.size());
    if (n < 1 || n > 16) throw std::runtime_error("the profile curve has " + std::to_string(n) + " points");
    b.pi[0] = n;
    for (int i = 0; i < n; ++i) b.pd[i] = cc[static_cast<size_t>(i)].as_double();
    for (int c = 0; c < 3; ++c) {
        const int L = static_cast<int>(prof.get("levels").get(NAMES[c]).as_int());
        b.pi[1 + c] = L;
        const PyObj& cv = prof.get("curve").get(NAMES[c]);
        for (int i = 0; i < n; ++i) b.pd[16 + 16 * c + i] = cv[static_cast<size_t>(i)].as_double();
        const PyObj& br = prof.get("band_ratio").get(NAMES[c]);
        for (int j = 0; j < L && j < 5; ++j) b.pd[64 + 5 * c + j] = br[static_cast<size_t>(j)].as_double();
        b.pd[79 + c] = prof.get("tot_ratio").get(NAMES[c]).as_double();
    }
    b.qi[0] = static_cast<int>(P.get("radius").as_int());
    b.qi[1] = static_cast<int>(P.get("bs").as_int());
    b.qi[2] = static_cast<int>(P.get("search").as_int());
    b.qi[3] = sim == "pyramid" ? 1 : 0;
    const char* SC[8] = {"temporal", "threshold", "spatial", "luma", "chroma", "noise_level", "pyramid_scale", "mix"};
    for (int i = 0; i < 8; ++i)
        b.qd[i] = (std::string(SC[i]) == "pyramid_scale" && !P.has("pyramid_scale")) ? 1.5 : P.get(SC[i]).as_double();
    for (int j = 0; j < 5; ++j) {
        b.qd[8 + j] = P.get("band_amounts").get(V5_BANDS[static_cast<size_t>(j)]).as_double();
        b.qd[13 + j] = P.get("band_noise").get(V5_BANDS[static_cast<size_t>(j)]).as_double();
    }
    for (int c = 0; c < 3; ++c) b.qd[18 + c] = P.get("channel_noise").get(NAMES[c]).as_double();
    return b;
}

Planes alloc_like(const Planes& p) {
    Planes o;
    o.H = p.H;
    o.W = p.W;
    o.Hc = p.Hc;
    o.Wc = p.Wc;
    o.Y.resize(p.Y.size());
    o.Cb.resize(p.Cb.size());
    o.Cr.resize(p.Cr.size());
    return o;
}

// backend._uniform(P)
bool uniform(const PyObj& P) {
    const PyObj& ch = P.get("ui_channels");
    if (ch.is_none()) return true;
    const PyObj &y = ch.get("Y"), &cb = ch.get("Cb"), &cr = ch.get("Cr");
    return y.get("bands") == cb.get("bands") && cb.get("bands") == cr.get("bands") && y.get("noise") == cb.get("noise") &&
           cb.get("noise") == cr.get("noise") && cb.get("amount") == cr.get("amount");
}

// backend._channel_params(P, c)
PyObj channel_params(const PyObj& P, int c) {
    const PyObj& ch = P.get("ui_channels").get(NAMES[c]);
    PyObj Q = P;
    Q.set("band_amounts", ch.get("bands"));
    Q.set("band_noise", ch.get("noise"));
    Q.set(c == 0 ? "luma" : "chroma", ch.get("amount"));
    return Q;
}

void spatial_call(const Planes& tmp, const Planes& rv, const PyObj& prof, const PyObj& P, int mask, Planes* out) {
    const Blocks b = v5_blocks(prof, P);
    const float* pl[3] = {tmp.Y.data(), tmp.Cb.data(), tmp.Cr.data()};
    const float* rp[3] = {rv.Y.data(), rv.Cb.data(), rv.Cr.data()};
    float* op[3] = {out->Y.data(), out->Cb.data(), out->Cr.data()};
    if (nvc_spatial(tmp.H, tmp.W, tmp.Hc, tmp.Wc, pl, rp, b.pi, b.pd, b.qi, b.qd, mask, op) != 0)
        throw std::runtime_error("nvc_spatial failed");
}

}  // namespace

void temporal(const std::vector<Planes>& window_in, int ci_in, const PyObj& prof, const PyObj& P, Flows* flows,
              Planes* out, Planes* rv) {
    const int R = static_cast<int>(P.get("radius").as_int());
    const int lo = std::max(0, ci_in - R), hi = std::min(static_cast<int>(window_in.size()), ci_in + R + 1);
    const double tn = P.has("t_noise") ? P.get("t_noise").as_double() : 1.0;
    PyObj Q = P;
    if (tn != 1.0) Q.set("noise_level", PyObj::real(P.get("noise_level").as_double() * tn));
    const int nf = hi - lo, ci = ci_in - lo;
    const Blocks b = v5_blocks(prof, Q);
    const Planes& f0 = window_in[static_cast<size_t>(lo)];
    const int H = f0.H, W = f0.W, Hc = f0.Hc, Wc = f0.Wc, bs = b.qi[1];
    if (bs >= 4 && std::min(H, W) < bs) {
        // below the motion block nv_native.temporal_v5 returns None and the Python UI runs nvfilter.temporal_v5 itself:
        // the first neighbour's estimate_flow_v5 raises (np.pad of the empty quarter-size pyramid level, else the max
        // of the empty block vectors in _refine); with no neighbour to filter (one frame, Amount 0) the centre passes,
        // out = p + a (acc / sw - p) and rv = ((1 - a) + a / sw)^2 + a^2 sf / sw^2 with acc = p, sw = 1, sf = 0, in
        // float32 as numpy evaluates them (the Python float a as a weak scalar: float32(a), float32(1 - a))
        const double a = Q.get("temporal").as_double();
        if (nf > 1 && a > 0) {
            if (H / 4 == 0) throw std::runtime_error("can't extend empty axis 0 using modes other than 'constant' or 'empty'");
            if (W / 4 == 0) throw std::runtime_error("can't extend empty axis 1 using modes other than 'constant' or 'empty'");
            throw std::runtime_error("zero-size array to reduction operation maximum which has no identity");
        }
        const Planes& cur = window_in[static_cast<size_t>(ci_in)];
        *out = alloc_like(cur);
        *rv = alloc_like(cur);
        const float fa = static_cast<float>(a), f1a = static_cast<float>(1.0 - a), faa = static_cast<float>(a * a);
        const float one = 1.0f, zero = 0.0f;
        for (int c = 0; c < 3; ++c) {
            const float* p = cur.p(c);
            float* o = out->p(c);
            float* r = rv->p(c);
            const size_t n = (c == 0 ? cur.Y : c == 1 ? cur.Cb : cur.Cr).size();
            for (size_t i = 0; i < n; ++i) {
                o[i] = p[i] + fa * (p[i] / one - p[i]);
                const float q = f1a + fa / one;
                r[i] = q * q + faa * zero / (one * one);
            }
        }
        return;
    }
    if (bs < 1 || std::min(H, W) < bs) throw std::runtime_error("the frame is smaller than the motion block");
    const int nby = H / bs, nbx = W / bs;
    std::vector<const float*> planes;
    for (int k = lo; k < hi; ++k)
        for (int c = 0; c < 3; ++c) planes.push_back(window_in[static_cast<size_t>(k)].p(c));
    std::vector<int> offs;
    std::vector<const float*> fin;
    if (flows)
        for (int k = 0; k < nf; ++k) {
            auto it = flows->find(k - ci);
            if (k != ci && it != flows->end() && it->second.nby == nby && it->second.nbx == nbx) {
                offs.push_back(k - ci);
                fin.push_back(it->second.vy.data());
                fin.push_back(it->second.vx.data());
            }
        }
    std::vector<std::vector<float>> fout(2 * static_cast<size_t>(nf), std::vector<float>(static_cast<size_t>(nby) * nbx));
    std::vector<float*> fop;
    for (auto& v : fout) fop.push_back(v.data());
    std::vector<uint8_t> nw(static_cast<size_t>(nf), 0);
    *out = alloc_like(f0);
    *rv = alloc_like(f0);
    float* op[3] = {out->Y.data(), out->Cb.data(), out->Cr.data()};
    float* rp[3] = {rv->Y.data(), rv->Cb.data(), rv->Cr.data()};
    if (offs.empty()) offs.push_back(0);   // nv_native passes [0] for no given flows (nin = 0)
    const int nin = static_cast<int>(fin.size() / 2);
    if (nvc_temporal(nf, ci, H, W, Hc, Wc, planes.data(), b.pi, b.pd, b.qi, b.qd, nin, offs.data(),
                     fin.empty() ? nullptr : fin.data(), fop.data(), nw.data(), op, rp) != 0)
        throw std::runtime_error("nvc_temporal failed");
    if (flows)
        for (int k = 0; k < nf; ++k)
            if (nw[static_cast<size_t>(k)]) {
                Flow fl;
                fl.nby = nby;
                fl.nbx = nbx;
                fl.vy = fout[2 * static_cast<size_t>(k)];
                fl.vx = fout[2 * static_cast<size_t>(k) + 1];
                (*flows)[k - ci] = std::move(fl);
            }
}

Planes finish(const Planes& src, const Planes& tmp, const Planes& rv, const PyObj& prof, const PyObj& P) {
    Planes out = tmp;                                  // ui_spatial off: the temporal result as is
    if (!P.has("ui_spatial") || P.get("ui_spatial").truthy()) {
        if (uniform(P)) {
            spatial_call(tmp, rv, prof, P, 7, &out);
        } else {
            for (int c = 0; c < 3; ++c) spatial_call(tmp, rv, prof, channel_params(P, c), 1 << c, &out);
        }
    }
    const double m = P.get("mix").as_double();
    if (m > 0) {
        float* op[3] = {out.Y.data(), out.Cb.data(), out.Cr.data()};
        const float* sp[3] = {src.Y.data(), src.Cb.data(), src.Cr.data()};
        nvc_mix(op, sp, out.H, out.W, out.Hc, out.Wc, m);
    }
    return out;
}

// ---------------------------------------------------------------- display
std::pair<double, double> minmax(const std::vector<float>& rgb) {
    double mn = 0, mx = 0;
    nvc_minmax(rgb.data(), rgb.size(), &mn, &mx);
    return {mn, mx};
}

std::vector<uint8_t> render_simple(int name, const std::vector<float>& rgb, int H, int W, int b, double mn, double mx) {
    std::vector<uint8_t> out(static_cast<size_t>(H) * W * 3);
    nvc_render_simple(name, rgb.data(), H, W, b, mn, mx, out.data());
    return out;
}

FreqImages freq_images(const std::vector<float>& rgb, int H, int W, int kind, bool emphasis) {
    FreqImages fi;
    fi.H = H;
    fi.W = W;
    fi.planes.resize(3 * static_cast<size_t>(H) * W);
    nvc_freq_images(rgb.data(), H, W, kind, emphasis ? 1 : 0, fi.planes.data(), fi.levels);
    return fi;
}

std::vector<uint8_t> render_freq(int range_kind, const FreqImages& fi, int plane, int b) {
    std::vector<uint8_t> out(static_cast<size_t>(fi.H) * fi.W * 3);
    nvc_render_freq(range_kind, fi.planes.data() + static_cast<size_t>(plane) * fi.H * fi.W, fi.H, fi.W,
                    fi.levels[2 * plane], fi.levels[2 * plane + 1], b, out.data());
    return out;
}

std::vector<float> crop_rgb(const std::vector<float>& rgb, int H, int W, int y0, int x0, int h, int w, int* ch, int* cw) {
    auto cl = [](int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; };
    const int ya = cl(y0, 0, H), yb = cl(y0 + h, 0, H), xa = cl(x0, 0, W), xb = cl(x0 + w, 0, W);
    *ch = std::max(0, yb - ya);
    *cw = std::max(0, xb - xa);
    std::vector<float> out(static_cast<size_t>(*ch) * *cw * 3);
    for (int y = 0; y < *ch; ++y)
        std::memcpy(out.data() + static_cast<size_t>(y) * *cw * 3, rgb.data() + (static_cast<size_t>(ya + y) * W + xa) * 3,
                    sizeof(float) * 3 * static_cast<size_t>(*cw));
    return out;
}

double noise_level(const std::vector<float>& crop, int h, int w) { return nvc_noise_level(crop.data(), h, w); }

std::vector<float> nlc_image(const std::vector<float>& rgb, const std::vector<float>& m, int H, int W) {
    std::vector<float> out(static_cast<size_t>(H) * W * 3);
    nvc_nlc_image(rgb.data(), m.data(), H, W, out.data());
    return out;
}

double mean_f32(const std::vector<float>& m) { return static_cast<double>(nvc_mean_f32(m.data(), m.size())); }

void not_uniform(const std::vector<float>& crop, int h, int w, std::vector<std::string>* bad, std::vector<std::string>* clip) {
    int b3[3] = {0, 0, 0}, c3[3] = {0, 0, 0};
    nvc_not_uniform(crop.data(), h, w, b3, c3);
    static const char* YN[3] = {"Y", "Cr", "Cb"};
    static const char* RN[3] = {"R", "G", "B"};
    bad->clear();
    clip->clear();
    for (int i = 0; i < 3; ++i) {
        if (b3[i]) bad->push_back(YN[i]);
        if (c3[i]) clip->push_back(RN[i]);
    }
}

std::optional<std::pair<int, int>> profile_numbers(const ProfileState& ps, const std::vector<Frame>& frames,
                                                   bool full_range, const std::string& matrix) {
    if (!ps.has_profile() || frames.empty()) return std::nullopt;
    const PyObj& base = ps.base;
    const PyObj& a = base.get("area");
    int area4[4] = {static_cast<int>(a[0].as_int()), static_cast<int>(a[1].as_int()), static_cast<int>(a[2].as_int()),
                    static_cast<int>(a.size() == 3 ? a[2].as_int() : a[3].as_int())};
    const int f = aft_frame(base, frames.size());
    const Planes pl = to_planes(frames[static_cast<size_t>(f)]);
    const std::vector<float> rgb = to_rgb(pl, matrix, full_range, 1);
    double fac[27];
    for (int c = 0; c < 3; ++c)
        for (int k = 0; k < 9; ++k) fac[9 * c + k] = ps.factors[c][k];
    const double tune[4] = {ps.tune_value("all"), ps.tune_value("Y"), ps.tune_value("Cb"), ps.tune_value("Cr")};
    const bool aft = base.get("aft_applied").truthy(), reset = base.get("aft_reset").truthy();
    std::vector<uint8_t> flags;
    const PyObj& st = base.get("aft_bin_flags");
    if (!st.is_none()) {
        static const char* FK[3] = {"marker", "clip", "gt3"};
        for (int k = 0; k < 3; ++k) {
            const PyObj& rows = st.get(FK[k]);
            for (int c = 0; c < 3; ++c)
                for (int b = 0; b < 9; ++b)
                    flags.push_back(!rows.is_none() && rows[static_cast<size_t>(c)][static_cast<size_t>(b)].truthy());
        }
    }
    int qp = 0, cp = 0;
    const int r = nvc_quality_numbers(rgb.data(), pl.H, pl.W, area4, fac, tune, aft, reset,
                                      flags.empty() ? nullptr : flags.data(), &qp, &cp);
    if (r != 1) return std::nullopt;
    return std::make_pair(qp, cp);
}

}  // namespace nvqt
