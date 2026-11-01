// params - see params.h.
#include "params.h"

#include <xlocale.h>

#include <cmath>
#include <cstdint>

#include "pyobj.h"

namespace mvofx {

using nvqt::PyObj;

namespace {

const char* const NAMES[3] = {"Y", "Cb", "Cr"};
const char* const V5_BANDS[5] = {"high", "medium", "low", "very-low", "ultra-low"};   // nvfilter.V5_BANDS

// a key the render needs (PyObj::get gives None for a missing one; a host-kept string is checked instead)
const PyObj& need(const PyObj& d, const std::string& k, const std::string& where) {
    if (!d.is_dict() || !d.has(k)) throw Error("ClipParams: " + where + " lacks \"" + k + "\"");
    return d.get(k);
}

// a finite number (int or float; the UI writes no other)
double num(const PyObj& d, const std::string& k, const std::string& where) {
    const PyObj& v = need(d, k, where);
    if (!v.is_num() || !std::isfinite(v.as_double()))
        throw Error("ClipParams: " + where + "[\"" + k + "\"] is not a finite number");
    return v.as_double();
}

// an integer in [lo, hi] (a JSON int, as the UI writes them; 2.5, "16" and 16.0 are refused)
int integer(const PyObj& d, const std::string& k, const std::string& where, int lo, int hi) {
    const PyObj& v = need(d, k, where);
    if (v.type() != PyObj::INT || v.as_int() < lo || v.as_int() > hi)
        throw Error("ClipParams: " + where + "[\"" + k + "\"] must be an integer in [" + std::to_string(lo) + ", " +
                    std::to_string(hi) + "]");
    return static_cast<int>(v.as_int());
}

// a finite number in [lo, hi]
double ranged(const PyObj& d, const std::string& k, const std::string& where, double lo, double hi) {
    const double x = num(d, k, where);
    if (x < lo || x > hi)
        throw Error("ClipParams: " + where + "[\"" + k + "\"] = " + nvqt::py_float_repr(x) + " is outside [" +
                    nvqt::py_float_repr(lo) + ", " + nvqt::py_float_repr(hi) + "]");
    return x;
}

// qtui/src/core/nvcore.cpp v5_blocks (nv_native._v5_blocks), the keys checked
V5Blocks v5_blocks(const PyObj& prof, const PyObj& P) {
    V5Blocks b;
    const std::string sim = P.has("similarity") ? P.get("similarity").as_str() : "v5";
    if (sim != "v5" && sim != "pyramid") throw Error("ClipParams: unknown similarity " + sim);
    const PyObj& cc = need(prof, "curve_centers", "profile");
    const int n = static_cast<int>(cc.size());
    if (!cc.is_list() || n < 1 || n > 16) throw Error("ClipParams: the profile curve has " + std::to_string(n) + " points");
    b.pi[0] = n;
    for (int i = 0; i < n; ++i) {
        const PyObj& v = cc[static_cast<size_t>(i)];
        if (!v.is_num() || !std::isfinite(v.as_double())) throw Error("ClipParams: profile curve_centers not finite numbers");
        b.pd[i] = v.as_double();
    }
    for (int c = 0; c < 3; ++c) {
        const int L = integer(need(prof, "levels", "profile"), NAMES[c], "profile levels", 0, 5);
        b.pi[1 + c] = L;
        const PyObj& cv = need(need(prof, "curve", "profile"), NAMES[c], "profile curve");
        if (!cv.is_list() || static_cast<int>(cv.size()) != n) throw Error("ClipParams: profile curve size");
        for (int i = 0; i < n; ++i) {
            const PyObj& v = cv[static_cast<size_t>(i)];
            if (!v.is_num() || !std::isfinite(v.as_double())) throw Error("ClipParams: profile curve not finite numbers");
            b.pd[16 + 16 * c + i] = v.as_double();
        }
        const PyObj& br = need(need(prof, "band_ratio", "profile"), NAMES[c], "profile band_ratio");
        if (!br.is_list() || static_cast<int>(br.size()) < L) throw Error("ClipParams: profile band_ratio size");
        for (int j = 0; j < L && j < 5; ++j) {
            const PyObj& v = br[static_cast<size_t>(j)];
            if (!v.is_num() || !std::isfinite(v.as_double())) throw Error("ClipParams: profile band_ratio not finite numbers");
            b.pd[64 + 5 * c + j] = v.as_double();
        }
        b.pd[79 + c] = num(need(prof, "tot_ratio", "profile"), NAMES[c], "profile tot_ratio");
    }
    // the ranges the UI makes (Temporal Radius 0..5; bs / search V5_DEFAULTS 16) with room around them
    b.qi[0] = integer(P, "radius", "P", 0, 16);
    b.qi[1] = integer(P, "bs", "P", 4, 256);
    b.qi[2] = integer(P, "search", "P", 0, 256);
    b.qi[3] = sim == "pyramid" ? 1 : 0;
    const char* SC[8] = {"temporal", "threshold", "spatial", "luma", "chroma", "noise_level", "pyramid_scale", "mix"};
    // non-negative finite values (the UI's ranges are inside: amounts 0..1, threshold 0.25..8, noise levels 0..4)
    for (int i = 0; i < 8; ++i)
        b.qd[i] = (std::string(SC[i]) == "pyramid_scale" && !P.has("pyramid_scale")) ? 1.5 : ranged(P, SC[i], "P", 0, 1e6);
    for (int j = 0; j < 5; ++j) {
        b.qd[8 + j] = ranged(need(P, "band_amounts", "P"), V5_BANDS[j], "P band_amounts", 0, 1e6);
        b.qd[13 + j] = ranged(need(P, "band_noise", "P"), V5_BANDS[j], "P band_noise", 0, 1e6);
    }
    for (int c = 0; c < 3; ++c) b.qd[18 + c] = ranged(need(P, "channel_noise", "P"), NAMES[c], "P channel_noise", 0, 1e6);
    return b;
}

// backend._uniform(P) (qtui nvcore.cpp uniform)
bool uniform(const PyObj& P) {
    const PyObj& ch = P.get("ui_channels");
    if (ch.is_none()) return true;
    const PyObj &y = need(ch, "Y", "P ui_channels"), &cb = need(ch, "Cb", "P ui_channels"),
                &cr = need(ch, "Cr", "P ui_channels");
    return y.get("bands") == cb.get("bands") && cb.get("bands") == cr.get("bands") && y.get("noise") == cb.get("noise") &&
           cb.get("noise") == cr.get("noise") && cb.get("amount") == cr.get("amount");
}

// backend._channel_params(P, c) (qtui nvcore.cpp channel_params)
PyObj channel_params(const PyObj& P, int c) {
    const PyObj& ch = need(need(P, "ui_channels", "P"), NAMES[c], "P ui_channels");
    PyObj Q = P;
    Q.set("band_amounts", need(ch, "bands", "P ui_channels"));
    Q.set("band_noise", need(ch, "noise", "P ui_channels"));
    Q.set(c == 0 ? "luma" : "chroma", need(ch, "amount", "P ui_channels"));
    return Q;
}

// JSON numbers read in the "C" locale whatever the host set (PyObj::parse uses strtod); per thread
PyObj parse_c_locale(const std::string& s) {
    static const locale_t c_loc = newlocale(LC_ALL_MASK, "C", nullptr);
    const locale_t old = uselocale(c_loc);
    try {
        PyObj o = PyObj::parse(s);
        uselocale(old);
        return o;
    } catch (const std::exception& e) {
        uselocale(old);
        throw Error(std::string("ClipParams is not valid JSON (") + e.what() + ")");
    }
}

}  // namespace

RenderState parse_clip_params(const std::string& s) {
    RenderState rs;
    if (s.empty()) return rs;
    const PyObj d = parse_c_locale(s);
    if (!d.is_dict() || d.get("format").type() != PyObj::STR || d.get("format").as_str() != "meatvideo_ofx_clip_params")
        throw Error("ClipParams: not a Meat Video ClipParams (format meatvideo_ofx_clip_params)");
    // version 1: progressive; 2 (INT-4): interlaced, "scan" tff / bff and render.P["interlace"]
    const int64_t ver = d.get("version").type() == PyObj::INT ? d.get("version").as_int() : -1;
    if (ver != 1 && ver != 2)
        throw Error("ClipParams: version " + d.get("version").dump() + " is not supported (1, 2)");
    const PyObj& scan = d.get("scan");
    if (ver == 1 && !scan.is_none()) throw Error("ClipParams: version 1 has no \"scan\" (interlaced settings are version 2)");
    if (ver == 2 && !(scan.type() == PyObj::STR && (scan.as_str() == "tff" || scan.as_str() == "bff")))
        throw Error("ClipParams: version 2 needs \"scan\": \"tff\" or \"bff\"");
    const PyObj& fs = d.get("frame_size");
    // (informative only: read when it is a pair of ints of a frame size)
    if (fs.is_list() && fs.size() == 2 && fs[0].type() == PyObj::INT && fs[1].type() == PyObj::INT &&
        fs[0].as_int() >= 0 && fs[0].as_int() <= 1000000 && fs[1].as_int() >= 0 && fs[1].as_int() <= 1000000) {
        rs.W = static_cast<int>(fs[0].as_int());
        rs.H = static_cast<int>(fs[1].as_int());
    }
    const PyObj& r = d.get("render");
    if (r.is_none()) return rs;
    const std::string mat = need(r, "matrix", "render").as_str();
    if (mat != "bt709" && mat != "bt601" && mat != "bt2020" && mat != "auto")
        throw Error("ClipParams: unknown matrix " + mat);
    if (!need(r, "full_range", "render").truthy())
        throw Error("ClipParams: limited-range state (the host's RGB frames are processed as full-range YCbCr)");
    const PyObj& P = need(r, "P", "render");
    const PyObj& prof = need(r, "profile", "render");
    rs.matrix_name = mat;
    rs.matrix = mat == "auto" ? -1 : mat == "bt601" ? 1 : mat == "bt2020" ? 2 : 0;
    // backend.temporal: Q = P with noise_level x t_noise (t_noise 1: P)
    const double tn = P.has("t_noise") ? ranged(P, "t_noise", "P", 0, 1e6) : 1.0;
    PyObj Q = P;
    if (tn != 1.0) Q.set("noise_level", PyObj::real(num(P, "noise_level", "P") * tn));
    rs.temporal = v5_blocks(prof, Q);
    rs.radius = rs.temporal.qi[0];
    // backend.finish: spatial_ui (+ mix)
    rs.spatial = !P.has("ui_spatial") || P.get("ui_spatial").truthy();
    rs.uniform = uniform(P);
    if (rs.uniform)
        rs.spatial_all = v5_blocks(prof, P);
    else
        for (int c = 0; c < 3; ++c) rs.spatial_ch[c] = v5_blocks(prof, channel_params(P, c));
    rs.mix = ranged(P, "mix", "P", 0, 1);
    // INT-4: the field processing of an interlaced clip (version 2 only, its first field = "scan")
    const PyObj& il = P.get("interlace");
    if (il.is_none() != (ver == 1)) throw Error("ClipParams: render.P[\"interlace\"] goes with version 2 only");
    if (!il.is_none()) {
        if (!il.is_dict()) throw Error("ClipParams: P[\"interlace\"] is not a dict");
        rs.field_first = integer(il, "first", "P interlace", 0, 1);
        const std::string m = need(il, "mode", "P interlace").type() == PyObj::STR ? il.get("mode").as_str() : "";
        rs.field_mode = m == "plain" ? 0 : m == "shift" ? 1 : m == "same" ? 2 : -1;
        if (rs.field_mode < 0) throw Error("ClipParams: unknown field mode \"" + m + "\" (same, shift, plain)");
        rs.field_radius = integer(il, "radius", "P interlace", 0, 32);
        if (rs.field_first != (scan.as_str() == "tff" ? 0 : 1))
            throw Error("ClipParams: render.P[\"interlace\"][\"first\"] disagrees with \"scan\"");
    }
    rs.has_profile = true;
    return rs;
}

void params_hash(const std::string& s, int out[3]) {
    uint64_t h = 14695981039346656037ull;
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ull;
    }
    out[0] = static_cast<int>(h & 0x7fffffffu);
    out[1] = static_cast<int>((h >> 33) & 0x7fffffffu);
    out[2] = static_cast<int>(s.size() & 0x7fffffffu);
}

}  // namespace mvofx
