// minihost - a small OFX host for testing MeatVideo.ofx.bundle without Resolve (N4a / N4b).
//
//   minihost BUNDLE --check
//       dlopen the bundle's binary, check the three exports and the OfxPlugin struct, run Load / Describe /
//       DescribeInContext (Filter) / CreateInstance / DestroyInstance / Unload and check what the plug-in declared.
//   minihost BUNDLE --check-floor
//       the reference floor compiled into the plug-in (ofx/src/ref_floor.inc) against nvc_reference_floor(NULL, ..)
//       of the bundle's libnvcore.dylib (computed, no file).
// minihost does not link libnvcore: the plug-in loads the copy in its bundle (Contents/Frameworks).
//   minihost BUNDLE --frames F.f32 --size W H N [--first T0] [--origin X Y] [--layout pad|neg] [--field-order O]
//            [--missing t1,t2,..] [--depth float|short|byte] [--reenter]
//            [--set name=value[,name=value..]] [--render t1,t2,..] [--range A,B] [--set ..] [--render ..] .. [--out OUT.f32]
//       the host (this program) reads N RGBA float32 frames (top-down rows, W x H x 4 each) from F.f32 into memory,
//       then - with the file-access monitor armed (fsguard, when injected) - loads the plug-in and renders: each
//       --set changes parameter values (InstanceChanged), each --render renders those times (GetRegionOfDefinition,
//       GetFramesNeeded, Render).  The Source clip covers times T0 .. T0 + N - 1; images are handed over in OFX
//       layout (bottom row first) with padded (pad) or negative (neg) row bytes.  The outputs (top-down RGBA
//       float32) go to OUT.f32 after the monitor is disarmed, in render order.  --missing: clipGetImage fails
//       (kOfxStatFailed, "no image at that time") at those times; --range A,B (a step): the Source clip's frame range
//       becomes A .. B (images outside it fail too); --depth: the pixel depth the images claim (the data stays float).
//       --reenter: inside every Source clipGetImage of a render the host calls GetFramesNeeded and InstanceChanged
//       (Rebuild Profile) on the same instance, from the same thread (a plug-in holding a lock across clipGetImage
//       would deadlock here; the profile being built must then not be kept).
//
// The genuine's bundles are refused (this host must never load them).
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ofxImageEffect.h"
#include "ofxMessage.h"

namespace {

// ---------------------------------------------------------------- properties
struct Prop {
    char t = 0;      // 'i', 'd', 's', 'p'
    std::vector<int> i;
    std::vector<double> d;
    std::vector<std::string> s;
    std::vector<void*> p;
};

struct PropSet {
    std::map<std::string, Prop> m;
    virtual ~PropSet() = default;
};

PropSet* PS(OfxPropertySetHandle h) { return reinterpret_cast<PropSet*>(h); }
OfxPropertySetHandle H(PropSet* p) { return reinterpret_cast<OfxPropertySetHandle>(p); }

template <class T>
std::vector<T>& vec(Prop& p);
template <>
std::vector<int>& vec<int>(Prop& p) { return p.i; }
template <>
std::vector<double>& vec<double>(Prop& p) { return p.d; }
template <>
std::vector<std::string>& vec<std::string>(Prop& p) { return p.s; }
template <>
std::vector<void*>& vec<void*>(Prop& p) { return p.p; }

template <class T>
OfxStatus pset(OfxPropertySetHandle h, const char* name, int idx, char t, const T& v) {
    if (!h || idx < 0) return kOfxStatErrBadHandle;
    Prop& p = PS(h)->m[name];
    if (p.t && p.t != t) return kOfxStatErrValue;
    p.t = t;
    auto& a = vec<T>(p);
    if (static_cast<int>(a.size()) <= idx) a.resize(static_cast<size_t>(idx) + 1);
    a[static_cast<size_t>(idx)] = v;
    return kOfxStatOK;
}

template <class T>
OfxStatus pget(OfxPropertySetHandle h, const char* name, int idx, char t, T* v) {
    if (!h) return kOfxStatErrBadHandle;
    auto it = PS(h)->m.find(name);
    if (it == PS(h)->m.end()) return kOfxStatErrUnknown;
    if (it->second.t != t) return kOfxStatErrValue;
    auto& a = vec<T>(it->second);
    if (idx < 0 || idx >= static_cast<int>(a.size())) return kOfxStatErrBadIndex;
    *v = a[static_cast<size_t>(idx)];
    return kOfxStatOK;
}

OfxStatus propSetPointer(OfxPropertySetHandle h, const char* n, int i, void* v) { return pset<void*>(h, n, i, 'p', v); }
OfxStatus propSetString(OfxPropertySetHandle h, const char* n, int i, const char* v) {
    return pset<std::string>(h, n, i, 's', v ? v : "");
}
OfxStatus propSetDouble(OfxPropertySetHandle h, const char* n, int i, double v) { return pset<double>(h, n, i, 'd', v); }
OfxStatus propSetInt(OfxPropertySetHandle h, const char* n, int i, int v) { return pset<int>(h, n, i, 'i', v); }
OfxStatus propSetPointerN(OfxPropertySetHandle h, const char* n, int c, void* const* v) {
    for (int i = 0; i < c; ++i)
        if (OfxStatus s = propSetPointer(h, n, i, v[i])) return s;
    return kOfxStatOK;
}
OfxStatus propSetStringN(OfxPropertySetHandle h, const char* n, int c, const char* const* v) {
    for (int i = 0; i < c; ++i)
        if (OfxStatus s = propSetString(h, n, i, v[i])) return s;
    return kOfxStatOK;
}
OfxStatus propSetDoubleN(OfxPropertySetHandle h, const char* n, int c, const double* v) {
    if (h) PS(h)->m[n].d.clear();
    for (int i = 0; i < c; ++i)
        if (OfxStatus s = propSetDouble(h, n, i, v[i])) return s;
    return kOfxStatOK;
}
OfxStatus propSetIntN(OfxPropertySetHandle h, const char* n, int c, const int* v) {
    for (int i = 0; i < c; ++i)
        if (OfxStatus s = propSetInt(h, n, i, v[i])) return s;
    return kOfxStatOK;
}
OfxStatus propGetPointer(OfxPropertySetHandle h, const char* n, int i, void** v) { return pget<void*>(h, n, i, 'p', v); }
OfxStatus propGetString(OfxPropertySetHandle h, const char* n, int i, char** v) {
    if (!h) return kOfxStatErrBadHandle;
    auto it = PS(h)->m.find(n);
    if (it == PS(h)->m.end()) return kOfxStatErrUnknown;
    if (it->second.t != 's') return kOfxStatErrValue;
    if (i < 0 || i >= static_cast<int>(it->second.s.size())) return kOfxStatErrBadIndex;
    *v = const_cast<char*>(it->second.s[static_cast<size_t>(i)].c_str());
    return kOfxStatOK;
}
OfxStatus propGetDouble(OfxPropertySetHandle h, const char* n, int i, double* v) { return pget<double>(h, n, i, 'd', v); }
OfxStatus propGetInt(OfxPropertySetHandle h, const char* n, int i, int* v) { return pget<int>(h, n, i, 'i', v); }
OfxStatus propGetPointerN(OfxPropertySetHandle h, const char* n, int c, void** v) {
    for (int i = 0; i < c; ++i)
        if (OfxStatus s = propGetPointer(h, n, i, v + i)) return s;
    return kOfxStatOK;
}
OfxStatus propGetStringN(OfxPropertySetHandle h, const char* n, int c, char** v) {
    for (int i = 0; i < c; ++i)
        if (OfxStatus s = propGetString(h, n, i, v + i)) return s;
    return kOfxStatOK;
}
OfxStatus propGetDoubleN(OfxPropertySetHandle h, const char* n, int c, double* v) {
    for (int i = 0; i < c; ++i)
        if (OfxStatus s = propGetDouble(h, n, i, v + i)) return s;
    return kOfxStatOK;
}
OfxStatus propGetIntN(OfxPropertySetHandle h, const char* n, int c, int* v) {
    for (int i = 0; i < c; ++i)
        if (OfxStatus s = propGetInt(h, n, i, v + i)) return s;
    return kOfxStatOK;
}
OfxStatus propReset(OfxPropertySetHandle h, const char* n) {
    if (!h) return kOfxStatErrBadHandle;
    PS(h)->m.erase(n);
    return kOfxStatOK;
}
OfxStatus propGetDimension(OfxPropertySetHandle h, const char* n, int* c) {
    if (!h) return kOfxStatErrBadHandle;
    auto it = PS(h)->m.find(n);
    if (it == PS(h)->m.end()) return kOfxStatErrUnknown;
    const Prop& p = it->second;
    *c = static_cast<int>(p.t == 'i' ? p.i.size() : p.t == 'd' ? p.d.size() : p.t == 's' ? p.s.size() : p.p.size());
    return kOfxStatOK;
}

const OfxPropertySuiteV1 PROP_SUITE = {
    propSetPointer, propSetString, propSetDouble, propSetInt, propSetPointerN, propSetStringN, propSetDoubleN,
    propSetIntN, propGetPointer, propGetString, propGetDouble, propGetInt, propGetPointerN, propGetStringN,
    propGetDoubleN, propGetIntN, propReset, propGetDimension};

int geti(PropSet* p, const char* n, int i = 0, int def = -999) {
    int v = def;
    propGetInt(H(p), n, i, &v);
    return v;
}
std::string gets(PropSet* p, const char* n, int i = 0) {
    char* v = nullptr;
    return propGetString(H(p), n, i, &v) == kOfxStatOK && v ? v : "<unset>";
}

// ---------------------------------------------------------------- parameters
struct Param {
    std::string name, type;
    PropSet props;
    double value = 0;
};
struct ParamSet {
    std::vector<std::unique_ptr<Param>> v;
    PropSet props;
    Param* find(const std::string& n) {
        for (auto& p : v)
            if (p->name == n) return p.get();
        return nullptr;
    }
};
ParamSet* PSET(OfxParamSetHandle h) { return reinterpret_cast<ParamSet*>(h); }
Param* PAR(OfxParamHandle h) { return reinterpret_cast<Param*>(h); }

OfxStatus paramDefine(OfxParamSetHandle ps, const char* type, const char* name, OfxPropertySetHandle* props) {
    if (!ps) return kOfxStatErrBadHandle;
    if (PSET(ps)->find(name)) return kOfxStatErrExists;
    auto p = std::make_unique<Param>();
    p->name = name;
    p->type = type;
    propSetString(H(&p->props), kOfxPropType, 0, kOfxTypeParameter);
    propSetString(H(&p->props), kOfxParamPropType, 0, type);
    propSetString(H(&p->props), kOfxPropName, 0, name);
    if (props) *props = H(&p->props);
    PSET(ps)->v.push_back(std::move(p));
    return kOfxStatOK;
}
OfxStatus paramGetHandle(OfxParamSetHandle ps, const char* name, OfxParamHandle* h, OfxPropertySetHandle* props) {
    if (!ps) return kOfxStatErrBadHandle;
    Param* p = PSET(ps)->find(name);
    if (!p) return kOfxStatErrUnknown;
    *h = reinterpret_cast<OfxParamHandle>(p);
    if (props) *props = H(&p->props);
    return kOfxStatOK;
}
OfxStatus paramSetGetPropertySet(OfxParamSetHandle ps, OfxPropertySetHandle* props) {
    *props = H(&PSET(ps)->props);
    return kOfxStatOK;
}
OfxStatus paramGetPropertySet(OfxParamHandle h, OfxPropertySetHandle* props) {
    *props = H(&PAR(h)->props);
    return kOfxStatOK;
}
bool double_type(const Param* p) { return p->type == kOfxParamTypeDouble; }
OfxStatus get_value(OfxParamHandle h, va_list ap) {
    Param* p = PAR(h);
    if (!p) return kOfxStatErrBadHandle;
    if (double_type(p)) {
        *va_arg(ap, double*) = p->value;
    } else if (p->type == kOfxParamTypeInteger || p->type == kOfxParamTypeBoolean || p->type == kOfxParamTypeChoice) {
        *va_arg(ap, int*) = static_cast<int>(p->value);
    } else {
        return kOfxStatErrValue;
    }
    return kOfxStatOK;
}
std::map<std::string, int> g_param_reads;     // parameter name -> reads (paramGetValue / AtTime)
OfxStatus paramGetValue(OfxParamHandle h, ...) {
    va_list ap;
    va_start(ap, h);
    const OfxStatus s = get_value(h, ap);
    va_end(ap);
    if (h) ++g_param_reads[PAR(h)->name];
    return s;
}
OfxStatus paramGetValueAtTime2(OfxParamHandle h, OfxTime t, ...) {
    va_list ap;
    va_start(ap, t);
    const OfxStatus s = get_value(h, ap);
    va_end(ap);
    if (h) ++g_param_reads[PAR(h)->name];
    return s;
}
OfxStatus paramUnsupported1(OfxParamHandle, OfxTime, ...) { return kOfxStatErrUnsupported; }
OfxStatus paramGetIntegral(OfxParamHandle, OfxTime, OfxTime, ...) { return kOfxStatErrUnsupported; }
OfxStatus paramSetValue(OfxParamHandle, ...) { return kOfxStatErrUnsupported; }
OfxStatus paramGetNumKeys(OfxParamHandle, unsigned int* n) {
    *n = 0;
    return kOfxStatOK;
}
OfxStatus paramGetKeyTime(OfxParamHandle, unsigned int, OfxTime*) { return kOfxStatErrBadIndex; }
OfxStatus paramGetKeyIndex(OfxParamHandle, OfxTime, int, int*) { return kOfxStatFailed; }
OfxStatus paramDeleteKey(OfxParamHandle, OfxTime) { return kOfxStatErrUnsupported; }
OfxStatus paramDeleteAllKeys(OfxParamHandle) { return kOfxStatOK; }
OfxStatus paramCopy(OfxParamHandle, OfxParamHandle, OfxTime, const OfxRangeD*) { return kOfxStatErrUnsupported; }
OfxStatus paramEditBegin(OfxParamSetHandle, const char*) { return kOfxStatOK; }
OfxStatus paramEditEnd(OfxParamSetHandle) { return kOfxStatOK; }

const OfxParameterSuiteV1 PARAM_SUITE = {
    paramDefine, paramGetHandle, paramSetGetPropertySet, paramGetPropertySet, paramGetValue, paramGetValueAtTime2,
    paramUnsupported1, paramGetIntegral, paramSetValue, paramUnsupported1, paramGetNumKeys, paramGetKeyTime,
    paramGetKeyIndex, paramDeleteKey, paramDeleteAllKeys, paramCopy, paramEditBegin, paramEditEnd};

// ---------------------------------------------------------------- clips, images, effects
struct Host;
struct Clip {
    std::string name;
    PropSet props;
    Host* host = nullptr;
};
struct Image : PropSet {
    std::vector<unsigned char> buf;
    unsigned char* base = nullptr;   // bottom row
    std::ptrdiff_t rb = 0;
    bool output = false;
    double time = 0;
};
struct Effect {
    PropSet props;
    ParamSet params;
    std::vector<std::unique_ptr<Clip>> clips;
    Clip* clip(const std::string& n) {
        for (auto& c : clips)
            if (c->name == n) return c.get();
        return nullptr;
    }
};
Effect* FX(OfxImageEffectHandle h) { return reinterpret_cast<Effect*>(h); }
OfxImageEffectHandle FXH(Effect* e) { return reinterpret_cast<OfxImageEffectHandle>(e); }

struct Host {
    int W = 0, H = 0, N = 0;
    double first = 0;
    int ox = 0, oy = 0;              // origin of the pixel bounds / RoD
    bool neg = false;                // negative row bytes
    std::set<double> missing;        // times without an image
    double lo = 0, hi = -1;          // current frame range of the Source clip (default first .. first + N - 1)
    std::string depth = kOfxBitDepthFloat;
    bool reenter = false, rendering = false;
    int reentries = 0;
    std::function<void(double)> reenter_fn;   // set by run()
    std::vector<float> frames;       // N x H x W x 4, top-down
    std::vector<double> fetched;     // Source times handed over during a render
    std::vector<double> refused;     // Source times asked for without an image
    std::vector<std::unique_ptr<Image>> live;
    Image* output = nullptr;
    int released = 0, handed = 0;
    std::string persistent;
};
Host g;

Image* make_image(double t, bool output) {
    auto im = std::make_unique<Image>();
    im->output = output;
    im->time = t;
    const std::ptrdiff_t row = static_cast<std::ptrdiff_t>(g.W) * 16, pad = 48;
    const std::ptrdiff_t stride = row + pad;
    im->buf.assign(static_cast<size_t>(stride * g.H + 64), 0);
    unsigned char* mem = im->buf.data() + 16;
    if (!g.neg) {
        im->base = mem;              // memory row r = bottom-up row r
        im->rb = stride;
    } else {
        im->base = mem + stride * (g.H - 1);    // memory row r = top-down row r; base = the bottom row
        im->rb = -stride;
    }
    if (!output) {
        const int k = static_cast<int>(t - g.first);
        for (int ib = 0; ib < g.H; ++ib) {
            const int i = g.H - 1 - ib;
            std::memcpy(im->base + ib * im->rb, g.frames.data() + (static_cast<size_t>(k) * g.H + i) * g.W * 4,
                        static_cast<size_t>(row));
        }
        // poison the padding so that a plug-in reading past a row shows up as a difference
        for (int ib = 0; ib < g.H; ++ib) std::memset(im->base + ib * im->rb + row, 0x7f, static_cast<size_t>(pad));
    }
    PropSet* p = im.get();
    propSetPointer(H(p), kOfxImagePropData, 0, im->base);
    const int b[4] = {g.ox, g.oy, g.ox + g.W, g.oy + g.H};
    propSetIntN(H(p), kOfxImagePropBounds, 4, b);
    propSetIntN(H(p), kOfxImagePropRegionOfDefinition, 4, b);
    propSetInt(H(p), kOfxImagePropRowBytes, 0, static_cast<int>(im->rb));
    propSetString(H(p), kOfxImageEffectPropPixelDepth, 0, g.depth.c_str());
    propSetString(H(p), kOfxImageEffectPropComponents, 0, kOfxImageComponentRGBA);
    propSetString(H(p), kOfxImageEffectPropPreMultiplication, 0, kOfxImageOpaque);
    propSetDouble(H(p), kOfxImageEffectPropRenderScale, 0, 1.0);
    propSetDouble(H(p), kOfxImageEffectPropRenderScale, 1, 1.0);
    propSetDouble(H(p), kOfxImagePropPixelAspectRatio, 0, 1.0);
    propSetString(H(p), kOfxImagePropField, 0, kOfxImageFieldNone);
    propSetString(H(p), kOfxImagePropUniqueIdentifier, 0, ((output ? "out" : "src") + std::to_string(t)).c_str());
    Image* r = im.get();
    g.live.push_back(std::move(im));
    return r;
}

OfxStatus getPropertySet(OfxImageEffectHandle e, OfxPropertySetHandle* p) {
    if (!e) return kOfxStatErrBadHandle;
    *p = H(&FX(e)->props);
    return kOfxStatOK;
}
OfxStatus getParamSet(OfxImageEffectHandle e, OfxParamSetHandle* p) {
    if (!e) return kOfxStatErrBadHandle;
    *p = reinterpret_cast<OfxParamSetHandle>(&FX(e)->params);
    return kOfxStatOK;
}
OfxStatus clipDefine(OfxImageEffectHandle e, const char* name, OfxPropertySetHandle* p) {
    if (!e) return kOfxStatErrBadHandle;
    auto c = std::make_unique<Clip>();
    c->name = name;
    propSetString(H(&c->props), kOfxPropType, 0, kOfxTypeClip);
    propSetString(H(&c->props), kOfxPropName, 0, name);
    *p = H(&c->props);
    FX(e)->clips.push_back(std::move(c));
    return kOfxStatOK;
}
OfxStatus clipGetHandle(OfxImageEffectHandle e, const char* name, OfxImageClipHandle* c, OfxPropertySetHandle* p) {
    if (!e) return kOfxStatErrBadHandle;
    Clip* cl = FX(e)->clip(name);
    if (!cl) return kOfxStatErrUnknown;
    *c = reinterpret_cast<OfxImageClipHandle>(cl);
    if (p) *p = H(&cl->props);
    return kOfxStatOK;
}
OfxStatus clipGetPropertySet(OfxImageClipHandle c, OfxPropertySetHandle* p) {
    if (!c) return kOfxStatErrBadHandle;
    *p = H(&reinterpret_cast<Clip*>(c)->props);
    return kOfxStatOK;
}
OfxStatus clipGetImage(OfxImageClipHandle c, OfxTime t, const OfxRectD*, OfxPropertySetHandle* img) {
    Clip* cl = reinterpret_cast<Clip*>(c);
    if (!cl) return kOfxStatErrBadHandle;
    const bool out = cl->name == kOfxImageEffectOutputClipName;
    if (!out) {
        const double k = t - g.first;
        if (k < 0 || k >= g.N || k != std::floor(k) || t < g.lo || t > g.hi || g.missing.count(t)) {
            g.refused.push_back(t);
            return kOfxStatFailed;
        }
        g.fetched.push_back(t);
        if (g.reenter && g.rendering && g.reenter_fn) g.reenter_fn(t);
    }
    Image* im = make_image(t, out);
    if (out) g.output = im;
    ++g.handed;
    *img = H(im);
    return kOfxStatOK;
}
OfxStatus clipReleaseImage(OfxPropertySetHandle img) {
    for (auto it = g.live.begin(); it != g.live.end(); ++it)
        if (H(it->get()) == img) {
            ++g.released;
            if (!(*it)->output) g.live.erase(it);    // the output buffer stays with the host until it is read
            return kOfxStatOK;
        }
    return kOfxStatErrBadHandle;
}
OfxStatus clipGetRegionOfDefinition(OfxImageClipHandle, OfxTime, OfxRectD* r) {
    r->x1 = g.ox;
    r->y1 = g.oy;
    r->x2 = g.ox + g.W;
    r->y2 = g.oy + g.H;
    return kOfxStatOK;
}
int g_abort_calls = 0;
int abort_fn(OfxImageEffectHandle) { ++g_abort_calls; const char* e = std::getenv("MH_ABORT_AT"); return e && std::atoi(e) == g_abort_calls ? 1 : 0; }
OfxStatus memAlloc(OfxImageEffectHandle, size_t, OfxImageMemoryHandle*) { return kOfxStatErrUnsupported; }
OfxStatus memFree(OfxImageMemoryHandle) { return kOfxStatErrUnsupported; }
OfxStatus memLock(OfxImageMemoryHandle, void**) { return kOfxStatErrUnsupported; }
OfxStatus memUnlock(OfxImageMemoryHandle) { return kOfxStatErrUnsupported; }

const OfxImageEffectSuiteV1 FX_SUITE = {getPropertySet, getParamSet,  clipDefine, clipGetHandle, clipGetPropertySet,
                                        clipGetImage,   clipReleaseImage, clipGetRegionOfDefinition, abort_fn, memAlloc,
                                        memFree,        memLock,      memUnlock};

OfxStatus msg(void*, const char* type, const char*, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[2048];
    std::vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    std::fprintf(stderr, "[message %s] %s\n", type, buf);
    return kOfxStatOK;
}
OfxStatus set_persistent(void*, const char* type, const char*, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[2048];
    std::vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    g.persistent = std::string(type) + ": " + buf;
    return kOfxStatOK;
}
OfxStatus clear_persistent(void*) {
    g.persistent.clear();
    return kOfxStatOK;
}
const OfxMessageSuiteV2 MSG_SUITE = {msg, set_persistent, clear_persistent};

std::vector<std::string> g_suites;
const void* fetchSuite(OfxPropertySetHandle, const char* name, int version) {
    g_suites.push_back(std::string(name) + " v" + std::to_string(version));
    if (!std::strcmp(name, kOfxPropertySuite) && version == 1) return &PROP_SUITE;
    if (!std::strcmp(name, kOfxParameterSuite) && version == 1) return &PARAM_SUITE;
    if (!std::strcmp(name, kOfxImageEffectSuite) && version == 1) return &FX_SUITE;
    if (!std::strcmp(name, kOfxMessageSuite) && version == 2) return &MSG_SUITE;
    return nullptr;
}

PropSet g_host_props;
OfxHost g_host = {nullptr, fetchSuite};

// ---------------------------------------------------------------- the file-access monitor (fsguard.c)
using ArmFn = void (*)(int);
using ReportFn = int (*)(FILE*);
ArmFn g_arm = nullptr;
ReportFn g_report = nullptr;
void arm(int on) {
    if (g_arm) g_arm(on);
}

// ---------------------------------------------------------------- plug-in
struct Loaded {
    void* dl = nullptr;
    int (*num)() = nullptr;
    OfxPlugin* (*get)(int) = nullptr;
    int (*unload_ok)() = nullptr;
    OfxPlugin* plug = nullptr;
};

int g_fail = 0;
void expect(bool ok, const std::string& what) {
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
    if (!ok) ++g_fail;
}

std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool load(const std::string& bundle, Loaded* L) {
    if (lower(bundle).find("neatvideo") != std::string::npos) {
        std::fprintf(stderr, "minihost: refusing to load %s (only the Meat Video bundle is loaded here)\n", bundle.c_str());
        std::exit(2);
    }
    const std::string bin = bundle + "/Contents/MacOS/MeatVideo.ofx";
    L->dl = dlopen(bin.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!L->dl) {
        std::fprintf(stderr, "dlopen %s: %s\n", bin.c_str(), dlerror());
        return false;
    }
    L->num = reinterpret_cast<int (*)()>(dlsym(L->dl, "OfxGetNumberOfPlugins"));
    L->get = reinterpret_cast<OfxPlugin* (*)(int)>(dlsym(L->dl, "OfxGetPlugin"));
    L->unload_ok = reinterpret_cast<int (*)()>(dlsym(L->dl, "FoundryOfxAllowPluginUnload"));
    if (!L->num || !L->get) return false;
    L->plug = L->get(0);
    return L->plug != nullptr;
}

OfxStatus act(Loaded* L, const char* action, Effect* e, PropSet* in = nullptr, PropSet* out = nullptr) {
    return L->plug->mainEntry(action, e ? FXH(e) : nullptr, in ? H(in) : nullptr, out ? H(out) : nullptr);
}

std::string clip_preferences(Loaded* L, Effect* inst);

// descriptor -> instance (Filter context), clips with the instance properties of the test clip
std::unique_ptr<Effect> instantiate(Effect* desc, const std::string& field_order) {
    auto inst = std::make_unique<Effect>();
    inst->props = desc->props;
    propSetString(H(&inst->props), kOfxPropType, 0, kOfxTypeImageEffectInstance);
    propSetString(H(&inst->props), kOfxImageEffectPropContext, 0, kOfxImageEffectContextFilter);
    propSetPointer(H(&inst->props), kOfxPropInstanceData, 0, nullptr);
    const double fr[2] = {g.first, g.first + g.N - 1};
    propSetDoubleN(H(&inst->props), kOfxImageEffectPropFrameRange, 2, fr);
    propSetDouble(H(&inst->props), kOfxImageEffectPropFrameRate, 0, 24.0);
    propSetDouble(H(&inst->props), kOfxImageEffectInstancePropEffectDuration, 0, g.N);
    for (auto& p : desc->params.v) {
        auto q = std::make_unique<Param>(*p);
        double dv = 0;
        int iv = 0;
        if (q->type == kOfxParamTypeDouble && propGetDouble(H(&q->props), kOfxParamPropDefault, 0, &dv) == kOfxStatOK)
            q->value = dv;
        else if (propGetInt(H(&q->props), kOfxParamPropDefault, 0, &iv) == kOfxStatOK)
            q->value = iv;
        inst->params.v.push_back(std::move(q));
    }
    for (auto& c : desc->clips) {
        auto d = std::make_unique<Clip>(*c);
        propSetString(H(&d->props), kOfxPropType, 0, kOfxTypeClip);
        propSetString(H(&d->props), kOfxImageEffectPropPixelDepth, 0, kOfxBitDepthFloat);
        propSetString(H(&d->props), kOfxImageEffectPropComponents, 0, kOfxImageComponentRGBA);
        propSetString(H(&d->props), kOfxImageClipPropUnmappedPixelDepth, 0, kOfxBitDepthFloat);
        propSetString(H(&d->props), kOfxImageClipPropUnmappedComponents, 0, kOfxImageComponentRGBA);
        propSetString(H(&d->props), kOfxImageEffectPropPreMultiplication, 0, kOfxImageOpaque);
        propSetDouble(H(&d->props), kOfxImagePropPixelAspectRatio, 0, 1.0);
        propSetDouble(H(&d->props), kOfxImageEffectPropFrameRate, 0, 24.0);
        propSetDoubleN(H(&d->props), kOfxImageEffectPropFrameRange, 2, fr);
        propSetDoubleN(H(&d->props), kOfxImageEffectPropUnmappedFrameRange, 2, fr);
        propSetString(H(&d->props), kOfxImageClipPropFieldOrder, 0, field_order.c_str());
        propSetInt(H(&d->props), kOfxImageClipPropConnected, 0, 1);
        propSetInt(H(&d->props), kOfxImageEffectPropTemporalClipAccess, 0, geti(&c->props, kOfxImageEffectPropTemporalClipAccess, 0, 0));
        inst->clips.push_back(std::move(d));
    }
    return inst;
}

// Load + Describe + DescribeInContext(Filter): the descriptor
std::unique_ptr<Effect> describe(Loaded* L) {
    L->plug->setHost(&g_host);
    expect(act(L, kOfxActionLoad, nullptr) == kOfxStatOK, "kOfxActionLoad");
    auto desc = std::make_unique<Effect>();
    propSetString(H(&desc->props), kOfxPropType, 0, kOfxTypeImageEffect);
    expect(act(L, kOfxActionDescribe, desc.get()) == kOfxStatOK, "kOfxActionDescribe");
    PropSet in;
    propSetString(H(&in), kOfxImageEffectPropContext, 0, kOfxImageEffectContextFilter);
    expect(act(L, kOfxImageEffectActionDescribeInContext, desc.get(), &in) == kOfxStatOK,
           "kOfxImageEffectActionDescribeInContext (Filter)");
    return desc;
}

int check(const std::string& bundle) {
    Loaded L;
    expect(load(bundle, &L), "dlopen + OfxGetPlugin(0)");
    if (!L.plug) return 1;
    expect(L.num() == 1, "OfxGetNumberOfPlugins() == 1");
    expect(L.get(1) == nullptr && L.get(-1) == nullptr, "OfxGetPlugin(1) / (-1) == NULL");
    expect(L.unload_ok && L.unload_ok() == 0, "FoundryOfxAllowPluginUnload() == 0 (keep loaded, as the genuine)");
    const OfxPlugin* p = L.plug;
    expect(p->pluginApi && !std::strcmp(p->pluginApi, kOfxImageEffectPluginApi), "pluginApi == \"OfxImageEffect\"");
    expect(p->apiVersion == 1, "apiVersion == 1");
    expect(p->pluginIdentifier && !std::strcmp(p->pluginIdentifier, "com.meatvideo.denoise"),
           "pluginIdentifier == com.meatvideo.denoise");
    std::printf("     pluginVersion %u.%u\n", p->pluginVersionMajor, p->pluginVersionMinor);
    expect(p->setHost && p->mainEntry, "setHost / mainEntry set");
    auto desc = describe(&L);
    PropSet* d = &desc->props;
    expect(gets(d, kOfxPropLabel) == "Meat Video Denoise", "label = " + gets(d, kOfxPropLabel));
    expect(gets(d, kOfxImageEffectPropSupportedContexts, 0) == kOfxImageEffectContextFilter,
           "context 0 = " + gets(d, kOfxImageEffectPropSupportedContexts, 0));
    int nd = 0;
    propGetDimension(H(d), kOfxImageEffectPropSupportedPixelDepths, &nd);
    expect(nd == 1 && gets(d, kOfxImageEffectPropSupportedPixelDepths) == kOfxBitDepthFloat,
           "pixel depths = [" + gets(d, kOfxImageEffectPropSupportedPixelDepths) + "] only");
    expect(geti(d, kOfxImageEffectPropSupportsTiles) == 0, "SupportsTiles = 0");
    expect(geti(d, kOfxImageEffectPropSupportsMultiResolution) == 0, "SupportsMultiResolution = 0");
    expect(geti(d, kOfxImageEffectPropTemporalClipAccess) == 1, "TemporalClipAccess = 1");
    expect(geti(d, kOfxImageEffectPluginPropFieldRenderTwiceAlways) == 0, "FieldRenderTwiceAlways = 0");
    expect(gets(d, kOfxImageEffectPluginRenderThreadSafety) == kOfxImageEffectRenderInstanceSafe,
           "RenderThreadSafety = " + gets(d, kOfxImageEffectPluginRenderThreadSafety));
    expect(desc->clips.size() == 2 && desc->clip("Source") && desc->clip("Output"), "clips Source + Output");
    for (auto& c : desc->clips) {
        int nc = 0;
        propGetDimension(H(&c->props), kOfxImageEffectPropSupportedComponents, &nc);
        expect(nc == 1 && gets(&c->props, kOfxImageEffectPropSupportedComponents) == kOfxImageComponentRGBA,
               c->name + " components = [RGBA] only");
        expect(gets(&c->props, kOfxImageClipPropFieldExtraction) == kOfxImageFieldBoth,
               c->name + " field extraction = " + gets(&c->props, kOfxImageClipPropFieldExtraction));
    }
    std::printf("     parameters (%zu):\n", desc->params.v.size());
    for (auto& q : desc->params.v) {
        std::string def = "-";
        double dv;
        int iv;
        if (propGetDouble(H(&q->props), kOfxParamPropDefault, 0, &dv) == kOfxStatOK) def = std::to_string(dv);
        else if (propGetInt(H(&q->props), kOfxParamPropDefault, 0, &iv) == kOfxStatOK) def = std::to_string(iv);
        std::printf("       %-16s %-24s default %s  (%s)\n", q->name.c_str(), q->type.c_str(), def.c_str(),
                    gets(&q->props, kOfxPropLabel).c_str());
    }
    g.W = g.H = 64;
    g.N = 1;
    auto inst = instantiate(desc.get(), kOfxImageFieldNone);
    expect(act(&L, kOfxActionCreateInstance, inst.get()) == kOfxStatOK, "kOfxActionCreateInstance");
    void* data = nullptr;
    propGetPointer(H(&inst->props), kOfxPropInstanceData, 0, &data);
    expect(data != nullptr, "instance data set");
    {
        const std::string cp = clip_preferences(&L, inst.get());
        expect(cp.find("status 0") == 0 && cp.find("OfxImageClipPropComponents_Source=OfxImageComponentRGBA") != std::string::npos &&
                   cp.find("OfxImageClipPropDepth_Source=OfxBitDepthFloat") != std::string::npos &&
                   cp.find("OfxImageClipPropComponents_Output=OfxImageComponentRGBA") != std::string::npos &&
                   cp.find("OfxImageClipPropDepth_Output=OfxBitDepthFloat") != std::string::npos,
               "GetClipPreferences: " + cp);
    }
    expect(act(&L, kOfxActionDestroyInstance, inst.get()) == kOfxStatOK, "kOfxActionDestroyInstance");
    expect(act(&L, kOfxActionUnload, nullptr) == kOfxStatOK, "kOfxActionUnload");
    std::printf("     suites fetched:");
    for (auto& s : g_suites) std::printf(" [%s]", s.c_str());
    std::printf("\n%s (%d failure%s)\n", g_fail ? "CHECK FAILED" : "CHECK OK", g_fail, g_fail == 1 ? "" : "s");
    return g_fail ? 1 : 0;
}

int check_floor(const std::string& bundle) {
    static const uint32_t bits[51] = {
#include "/Users/seitan/Desktop/work/reverse/.claude/worktrees/n4-ofx/ofx/src/ref_floor.inc"
    };
    if (lower(bundle).find("neatvideo") != std::string::npos) return 2;
    void* lib = dlopen((bundle + "/Contents/Frameworks/libnvcore.dylib").c_str(), RTLD_NOW | RTLD_LOCAL);
    auto fn = lib ? reinterpret_cast<int (*)(const char*, int, float*)>(dlsym(lib, "nvc_reference_floor")) : nullptr;
    if (!fn) {
        std::fprintf(stderr, "cannot load nvc_reference_floor from the bundle\n");
        return 1;
    }
    float fl[51];
    const int r = fn(nullptr, 4, fl);
    int bad = 0;
    for (int i = 0; i < 51; ++i) {
        uint32_t b;
        std::memcpy(&b, &fl[i], 4);
        if (b != bits[i]) ++bad;
    }
    std::printf("reference floor: nvc_reference_floor(NULL) returned %d, %d/51 words differ from ref_floor.inc\n", r, bad);
    return bad == 0 && r == 2 ? 0 : 1;
}

// GetClipPreferences: the components / depths the plug-in asks for, as text
std::string clip_preferences(Loaded* L, Effect* inst) {
    PropSet out;
    const OfxStatus st = act(L, kOfxImageEffectActionGetClipPreferences, inst, nullptr, &out);
    std::string r = "status " + std::to_string(st);
    for (auto& kv : out.m) r += " " + kv.first + "=" + (kv.second.s.empty() ? "?" : kv.second.s[0]);
    return r;
}

void set_range(Effect* inst, double a, double b) {
    g.lo = a;
    g.hi = b;
    const double fr[2] = {a, b};
    propSetDoubleN(H(&inst->props), kOfxImageEffectPropFrameRange, 2, fr);
    for (auto& c : inst->clips) propSetDoubleN(H(&c->props), kOfxImageEffectPropFrameRange, 2, fr);
}

std::vector<std::string> split(const std::string& s, char c) {
    std::vector<std::string> out;
    std::string cur;
    for (char ch : s) {
        if (ch == c) {
            out.push_back(cur);
            cur.clear();
        } else {
            cur += ch;
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

int run(const std::string& bundle, int argc, char** argv) {
    std::string frames_path, out_path, field_order = kOfxImageFieldNone;
    struct Step {
        char kind;
        std::string arg;
    };
    std::vector<Step> steps;
    for (int i = 2; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "missing value after %s\n", a.c_str());
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "--frames") frames_path = next();
        else if (a == "--size") {
            g.W = std::atoi(next().c_str());
            g.H = std::atoi(next().c_str());
            g.N = std::atoi(next().c_str());
        } else if (a == "--first") g.first = std::atof(next().c_str());
        else if (a == "--origin") {
            g.ox = std::atoi(next().c_str());
            g.oy = std::atoi(next().c_str());
        } else if (a == "--layout") g.neg = next() == "neg";
        else if (a == "--field-order") field_order = next();
        else if (a == "--missing")
            for (const std::string& m : split(next(), ',')) g.missing.insert(std::atof(m.c_str()));
        else if (a == "--depth") {
            const std::string d = next();
            g.depth = d == "byte" ? kOfxBitDepthByte : d == "short" ? kOfxBitDepthShort : kOfxBitDepthFloat;
        } else if (a == "--range") steps.push_back({'g', next()});
        else if (a == "--reenter") g.reenter = true;
        else if (a == "--set") steps.push_back({'s', next()});
        else if (a == "--render") steps.push_back({'r', next()});
        else if (a == "--out") out_path = next();
        else {
            std::fprintf(stderr, "unknown argument %s\n", a.c_str());
            return 2;
        }
    }
    // host side: the test frames into memory (before the monitor is armed)
    const size_t fsz = static_cast<size_t>(g.W) * g.H * 4;
    g.frames.resize(fsz * static_cast<size_t>(g.N));
    {
        std::ifstream f(frames_path, std::ios::binary);
        if (!f.read(reinterpret_cast<char*>(g.frames.data()), static_cast<std::streamsize>(g.frames.size() * 4))) {
            std::fprintf(stderr, "cannot read %zu frames of %dx%d from %s\n", static_cast<size_t>(g.N), g.W, g.H,
                         frames_path.c_str());
            return 2;
        }
    }
    std::vector<float> outputs;
    std::vector<std::string> log;

    arm(1);   // from here to the end of the plug-in session: no file access is expected but the host's dlopen
    Loaded L;
    if (!load(bundle, &L)) {
        arm(0);
        std::fprintf(stderr, "cannot load the plug-in\n");
        return 1;
    }
    auto desc = describe(&L);
    auto inst = instantiate(desc.get(), field_order);
    if (act(&L, kOfxActionCreateInstance, inst.get()) != kOfxStatOK) {
        arm(0);
        std::fprintf(stderr, "CreateInstance failed\n");
        return 1;
    }
    g.lo = g.first;
    g.hi = g.first + g.N - 1;
    g.reenter_fn = [&](double t) {
        ++g.reentries;
        PropSet fin, fout, cin;
        propSetDouble(H(&fin), kOfxPropTime, 0, t);
        act(&L, kOfxImageEffectActionGetFramesNeeded, inst.get(), &fin, &fout);
        propSetString(H(&cin), kOfxPropType, 0, kOfxTypeParameter);
        propSetString(H(&cin), kOfxPropName, 0, "rebuild_profile");
        propSetString(H(&cin), kOfxPropChangeReason, 0, kOfxChangeUserEdited);
        propSetDouble(H(&cin), kOfxPropTime, 0, t);
        act(&L, kOfxActionInstanceChanged, inst.get(), &cin);
    };
    char line[4096];
    log.push_back("clip preferences: " + clip_preferences(&L, inst.get()));
    for (const Step& st : steps) {
        if (st.kind == 'g') {
            const std::vector<std::string> ab = split(st.arg, ',');
            set_range(inst.get(), std::atof(ab.at(0).c_str()), std::atof(ab.at(1).c_str()));
            log.push_back("range " + st.arg);
            continue;
        }
        if (st.kind == 's') {
            for (const std::string& kv : split(st.arg, ',')) {
                const size_t eq = kv.find('=');
                Param* p = eq == std::string::npos ? nullptr : inst->params.find(kv.substr(0, eq));
                if (!p) {
                    arm(0);
                    std::fprintf(stderr, "unknown parameter in %s\n", kv.c_str());
                    return 2;
                }
                p->value = std::atof(kv.c_str() + eq + 1);
                PropSet in;
                propSetString(H(&in), kOfxPropType, 0, kOfxTypeParameter);
                propSetString(H(&in), kOfxPropName, 0, p->name.c_str());
                propSetString(H(&in), kOfxPropChangeReason, 0, kOfxChangeUserEdited);
                propSetDouble(H(&in), kOfxPropTime, 0, g.first);
                propSetDouble(H(&in), kOfxImageEffectPropRenderScale, 0, 1.0);
                propSetDouble(H(&in), kOfxImageEffectPropRenderScale, 1, 1.0);
                const OfxStatus s = act(&L, kOfxActionInstanceChanged, inst.get(), &in);
                std::snprintf(line, sizeof line, "set %s = %s -> InstanceChanged %d", p->name.c_str(),
                              kv.substr(eq + 1).c_str(), s);
                log.push_back(line);
            }
            continue;
        }
        for (const std::string& ts : split(st.arg, ',')) {
            const double t = std::atof(ts.c_str());
            PropSet in, out;
            propSetDouble(H(&in), kOfxPropTime, 0, t);
            propSetDouble(H(&in), kOfxImageEffectPropRenderScale, 0, 1.0);
            propSetDouble(H(&in), kOfxImageEffectPropRenderScale, 1, 1.0);
            const OfxStatus rs = act(&L, kOfxImageEffectActionGetRegionOfDefinition, inst.get(), &in, &out);
            double rod[4] = {0, 0, 0, 0};
            propGetDoubleN(H(&out), kOfxImageEffectPropRegionOfDefinition, 4, rod);
            PropSet fin, fout;
            propSetDouble(H(&fin), kOfxPropTime, 0, t);
            const OfxStatus fs = act(&L, kOfxImageEffectActionGetFramesNeeded, inst.get(), &fin, &fout);
            int nr = 0;
            propGetDimension(H(&fout), "OfxImageClipPropFrameRange_Source", &nr);
            std::vector<double> ranges(static_cast<size_t>(nr));
            if (nr) propGetDoubleN(H(&fout), "OfxImageClipPropFrameRange_Source", nr, ranges.data());
            PropSet rin;
            propSetDouble(H(&rin), kOfxPropTime, 0, t);
            propSetString(H(&rin), kOfxImageEffectPropFieldToRender, 0, kOfxImageFieldNone);
            const int sh = std::getenv("MH_SHRINK") ? std::atoi(std::getenv("MH_SHRINK")) : 0;
            const int win[4] = {static_cast<int>(rod[0]) + sh, static_cast<int>(rod[1]) + 2 * sh, static_cast<int>(rod[2]) - 3 * sh,
                                static_cast<int>(rod[3]) - sh};
            propSetIntN(H(&rin), kOfxImageEffectPropRenderWindow, 4, win);
            propSetDouble(H(&rin), kOfxImageEffectPropRenderScale, 0, 1.0);
            propSetDouble(H(&rin), kOfxImageEffectPropRenderScale, 1, 1.0);
            propSetInt(H(&rin), kOfxImageEffectPropSequentialRenderStatus, 0, 0);
            propSetInt(H(&rin), kOfxImageEffectPropInteractiveRenderStatus, 0, 0);
            g.fetched.clear();
            g.refused.clear();
            g.output = nullptr;
            const int handed0 = g.handed, released0 = g.released;
            g.rendering = true;
            const OfxStatus st2 = act(&L, kOfxImageEffectActionRender, inst.get(), &rin);
            g.rendering = false;
            // every Source frame fetched must lie in a declared range
            bool declared = true;
            std::vector<double> asked = g.fetched;
            asked.insert(asked.end(), g.refused.begin(), g.refused.end());
            for (double f : asked) {
                bool in_r = false;
                for (int k = 0; k + 1 < nr; k += 2) in_r = in_r || (f >= ranges[static_cast<size_t>(k)] && f <= ranges[static_cast<size_t>(k) + 1]);
                declared = declared && in_r;
            }
            std::string fr, rr, nr2;
            for (double f : g.fetched) fr += (fr.empty() ? "" : " ") + std::to_string(static_cast<long>(f));
            for (double f : g.refused) nr2 += (nr2.empty() ? "" : " ") + std::to_string(static_cast<long>(f));
            for (int k = 0; k < nr; ++k) rr += (rr.empty() ? "" : " ") + std::to_string(static_cast<long>(ranges[static_cast<size_t>(k)]));
            std::snprintf(line, sizeof line,
                          "render t=%g status=%d rod=%d,%d,%d,%d (%d) frames_needed=[%s] (%d) fetched=[%s] no_image=[%s] "
                          "declared=%d images=%d released=%d message=\"%s\"",
                          t, st2, win[0], win[1], win[2], win[3], rs, rr.c_str(), fs, fr.c_str(), nr2.c_str(),
                          declared ? 1 : 0,
                          g.handed - handed0, g.released - released0, g.persistent.c_str());
            log.push_back(line);
            // the output image back to top-down RGBA
            const size_t o = outputs.size();
            outputs.resize(o + fsz, std::nanf(""));
            if (st2 == kOfxStatOK && g.output)
                for (int i = 0; i < g.H; ++i)
                    std::memcpy(outputs.data() + o + static_cast<size_t>(i) * g.W * 4,
                                g.output->base + static_cast<std::ptrdiff_t>(g.H - 1 - i) * g.output->rb,
                                static_cast<size_t>(g.W) * 16);
            for (auto it = g.live.begin(); it != g.live.end();)
                it = (*it)->output ? g.live.erase(it) : it + 1;
        }
    }
    act(&L, kOfxActionDestroyInstance, inst.get());
    act(&L, kOfxActionUnload, nullptr);
    arm(0);

    for (const std::string& s : log) std::printf("%s\n", s.c_str());
    std::printf("suites fetched:");
    for (auto& s : g_suites) std::printf(" [%s]", s.c_str());
    std::printf("\nleaked source images: %zu\n", g.live.size());
    if (g.reenter) std::printf("re-entrant calls from clipGetImage: %d\n", g.reentries);
    if (g_report) g_report(stdout);
    if (!out_path.empty()) {
        std::ofstream f(out_path, std::ios::binary);
        f.write(reinterpret_cast<const char*>(outputs.data()), static_cast<std::streamsize>(outputs.size() * 4));
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    g_arm = reinterpret_cast<ArmFn>(dlsym(RTLD_DEFAULT, "fsguard_arm"));
    g_report = reinterpret_cast<ReportFn>(dlsym(RTLD_DEFAULT, "fsguard_report"));

    if (argc >= 2 && std::string(argv[1]) == "--fsguard-selftest") {
        // positive control of the monitor: four accesses from this image must be recorded
        if (!g_arm || !g_report) {
            std::printf("fsguard not injected\n");
            return 1;
        }
        arm(1);
        if (FILE* f = std::fopen("/dev/null", "r")) std::fclose(f);
        std::ifstream s("/dev/null");
        const int fd = ::open("/dev/null", O_RDONLY);
        struct stat sb;
        ::stat("/dev/null", &sb);
        void* d = dlopen("/usr/lib/libSystem.B.dylib", RTLD_NOW);
        arm(0);
        if (d) dlclose(d);
        if (fd >= 0) ::close(fd);
        return g_report(stdout) >= 5 ? 0 : 1;
    }
    if (argc < 3) {
        std::fprintf(stderr, "usage: minihost --fsguard-selftest | BUNDLE --check | BUNDLE --check-floor | BUNDLE --frames F --size W H N ...\n");
        return 2;
    }
    const std::string bundle = argv[1];
    if (std::string(argv[2]) == "--check") return check(bundle);
    if (std::string(argv[2]) == "--check-floor") return check_floor(bundle);
    return run(bundle, argc, argv);
}
