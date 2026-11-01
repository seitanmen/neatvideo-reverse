// plugin - the OFX entry points of MeatVideo.ofx (N4a / N4b / N4d): one image effect "com.meatvideo.denoise" on the
// plain OpenFX C API (ofx/vendor/openfx).  Exported: OfxGetNumberOfPlugins, OfxGetPlugin, FoundryOfxAllowPluginUnload
// (the genuine's three; ofx/exports.txt).
//
// What it tells the host (Describe / DescribeInContext): contexts Filter and General, clips Source and Output in RGBA
// float32 only, no tiles, no multi-resolution, temporal clip access, one render at a time per instance, fielded
// imagery fetched as full frames and refused at render (interlaced material is not supported,
// docs/INTERLACE_PROGRESSIVE.md).  The parameters are the genuine's (params.h): the buttons Prepare Noise Profile /
// Adjust Filter Settings open the window (a separate process, session.h) on frames fetched from the host around the
// current time, and its Apply stores the settings and the noise profile in the secret ClipParams; the render reads
// ClipParams only (no profile is built while rendering) and passes the source through while it holds no profile.
// Every pixel comes from the host's images (clipGetImage).
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <string>
#include <vector>

#include "core.h"
#include "ofxImageEffect.h"
#include "ofxMessage.h"
#include "params.h"
#include "session.h"

#define MV_EXPORT extern "C" __attribute__((visibility("default")))

namespace {

using namespace mvofx;

const char* const PLUGIN_ID = "com.meatvideo.denoise";
const char* const SOURCE = kOfxImageEffectSimpleSourceClipName;
const char* const OUTPUT = kOfxImageEffectOutputClipName;
// frames the window gets: the current one and up to WINDOW_HALF on each side (the genuine's 11 thumbnails; they also
// cover the largest Temporal Radius, 5)
constexpr int WINDOW_HALF = 5;
const char* const NO_PROFILE =
    "Meat Video: no noise profile yet; the source is passed through. Press Prepare Noise Profile to make one.";

OfxHost* g_host = nullptr;
const OfxPropertySuiteV1* g_prop = nullptr;
const OfxParameterSuiteV1* g_param = nullptr;
const OfxImageEffectSuiteV1* g_fx = nullptr;
const OfxMessageSuiteV2* g_msg2 = nullptr;     // optional (persistent messages)

struct Instance {
    OfxImageEffectHandle fx = nullptr;
    OfxImageClipHandle src = nullptr, dst = nullptr;
    OfxParamSetHandle ps = nullptr;
    OfxParamHandle prepare = nullptr, adjust = nullptr, clip = nullptr, hash[3] = {nullptr, nullptr, nullptr};
    // mu guards the members below; it is never held across a host call, a parse or the window
    std::mutex mu;
    std::string text;                              // the ClipParams text `state` was parsed from
    std::shared_ptr<const RenderState> state;
    // a new instance's ClipParams from the Default Profile / Preset, asked for at CreateInstance (where parameters
    // cannot be set) and used as ClipParams until it is stored there: at the first InstanceChanged or
    // SyncPrivateData (commit_defaults)
    std::string pending;
    bool committing = false;                       // commit_defaults running
    bool store_warned = false;                     // the "could not be stored" warning is shown
    std::string note;                              // why the defaults could not be loaded ("" = nothing to say)
};

struct Failure : Error {
    OfxStatus st;
    Failure(OfxStatus s, const std::string& m) : Error(m), st(s) {}
};

// a frame time as text (%g: 12, 12.5)
std::string tstr(double t) {
    char b[32];
    std::snprintf(b, sizeof b, "%g", t);
    return b;
}

void check(OfxStatus s, const char* what) {
    if (s != kOfxStatOK) throw Failure(s == kOfxStatErrMemory ? kOfxStatErrMemory : kOfxStatFailed, what);
}

Instance* instance_of(OfxImageEffectHandle fx) {
    OfxPropertySetHandle props;
    check(g_fx->getPropertySet(fx, &props), "getPropertySet");
    void* p = nullptr;
    check(g_prop->propGetPointer(props, kOfxPropInstanceData, 0, &p), "kOfxPropInstanceData");
    if (!p) throw Failure(kOfxStatFailed, "no instance data");
    return static_cast<Instance*>(p);
}

void persistent(OfxImageEffectHandle fx, const char* type, const std::string& text) {
    if (g_msg2) g_msg2->setPersistentMessage(fx, type, "com.meatvideo.denoise.status", "%s", text.c_str());
}

void clear_persistent(OfxImageEffectHandle fx) {
    if (g_msg2) g_msg2->clearPersistentMessage(fx);
}

// ---------------------------------------------------------------- describe
OfxStatus describe(OfxImageEffectHandle fx) {
    OfxPropertySetHandle p;
    check(g_fx->getPropertySet(fx, &p), "getPropertySet");
    g_prop->propSetString(p, kOfxPropLabel, 0, "Meat Video Denoise");
    g_prop->propSetString(p, kOfxPropShortLabel, 0, "Meat Video");
    g_prop->propSetString(p, kOfxPropLongLabel, 0, "Meat Video Denoise");
    g_prop->propSetString(p, kOfxImageEffectPluginPropGrouping, 0, "Meat Video");
    g_prop->propSetString(p, kOfxPropPluginDescription, 0,
                          "Meat Video noise reduction (temporal + spatial, v5 core). Prepare Noise Profile and Adjust "
                          "Filter Settings open the Meat Video window on the frames around the current one; its Apply "
                          "stores the noise profile and the settings in this instance. Takes progressive RGBA float32 "
                          "frames of at least 64 x 64 pixels only (interlaced material, field rendering and other "
                          "depths are refused).");
    g_prop->propSetString(p, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
    g_prop->propSetString(p, kOfxImageEffectPropSupportedContexts, 1, kOfxImageEffectContextGeneral);
    g_prop->propSetString(p, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthFloat);
    g_prop->propSetInt(p, kOfxImageEffectPluginPropSingleInstance, 0, 0);
    g_prop->propSetInt(p, kOfxImageEffectPluginPropHostFrameThreading, 0, 0);
    g_prop->propSetInt(p, kOfxImageEffectPropSupportsMultiResolution, 0, 0);
    g_prop->propSetInt(p, kOfxImageEffectPropSupportsTiles, 0, 0);
    g_prop->propSetInt(p, kOfxImageEffectPropTemporalClipAccess, 0, 1);
    g_prop->propSetInt(p, kOfxImageEffectPropSupportsMultipleClipDepths, 0, 0);
    g_prop->propSetInt(p, kOfxImageEffectPropSupportsMultipleClipPARs, 0, 0);
    g_prop->propSetInt(p, kOfxImageEffectPluginPropFieldRenderTwiceAlways, 0, 0);
    g_prop->propSetString(p, kOfxImageEffectPluginRenderThreadSafety, 0, kOfxImageEffectRenderInstanceSafe);
    return kOfxStatOK;
}

void define_clip(OfxImageEffectHandle fx, const char* name, bool temporal) {
    OfxPropertySetHandle c;
    check(g_fx->clipDefine(fx, name, &c), "clipDefine");
    g_prop->propSetString(c, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);
    g_prop->propSetInt(c, kOfxImageEffectPropSupportsTiles, 0, 0);
    g_prop->propSetInt(c, kOfxImageEffectPropTemporalClipAccess, 0, temporal ? 1 : 0);
    g_prop->propSetString(c, kOfxImageClipPropFieldExtraction, 0, kOfxImageFieldBoth);
}

// the genuine's page "Controls" (prof_0xa22c.c): two buttons, ClipParams and ParamsHash1..3, in this order
OfxStatus describe_in_context(OfxImageEffectHandle fx) {
    define_clip(fx, SOURCE, true);
    define_clip(fx, OUTPUT, false);
    OfxParamSetHandle ps;
    check(g_fx->getParamSet(fx, &ps), "getParamSet");
    OfxPropertySetHandle page, p;
    check(g_param->paramDefine(ps, kOfxParamTypePage, PARAM_PAGE, &page), "paramDefine page");
    int child = 0;
    auto button = [&](const char* name, const char* label, const char* hint, bool enabled) {
        check(g_param->paramDefine(ps, kOfxParamTypePushButton, name, &p), "paramDefine button");
        g_prop->propSetString(p, kOfxPropLabel, 0, label);
        g_prop->propSetString(p, kOfxParamPropHint, 0, hint);
        g_prop->propSetString(p, kOfxParamPropScriptName, 0, name);
        if (!enabled) g_prop->propSetInt(p, kOfxParamPropEnabled, 0, 0);
        g_prop->propSetString(page, kOfxParamPropPageChild, child++, name);
    };
    button(PARAM_PREPARE, "Prepare Noise Profile", "Prepare a noise profile to enable the filters in this instance of "
           "the effect", true);
    button(PARAM_ADJUST, "Adjust Filter Settings", "Adjust filter settings to customize the processing applied by this "
           "instance of the effect", false);
    check(g_param->paramDefine(ps, kOfxParamTypeCustom, PARAM_CLIP, &p), "paramDefine ClipParams");
    g_prop->propSetInt(p, kOfxParamPropSecret, 0, 1);
    g_prop->propSetString(p, kOfxParamPropScriptName, 0, PARAM_CLIP);
    g_prop->propSetString(p, kOfxParamPropDefault, 0, "");
    g_prop->propSetString(p, kOfxParamPropCacheInvalidation, 0, kOfxParamInvalidateAll);
    g_prop->propSetInt(p, kOfxParamPropAnimates, 0, 0);
    g_prop->propSetString(page, kOfxParamPropPageChild, child++, PARAM_CLIP);
    for (const char* h : PARAM_HASH) {
        check(g_param->paramDefine(ps, kOfxParamTypeInteger, h, &p), "paramDefine ParamsHash");
        g_prop->propSetString(p, kOfxParamPropScriptName, 0, h);
        g_prop->propSetInt(p, kOfxParamPropSecret, 0, 1);
        g_prop->propSetInt(p, kOfxParamPropDefault, 0, 0);
        g_prop->propSetString(p, kOfxParamPropCacheInvalidation, 0, kOfxParamInvalidateAll);
        g_prop->propSetInt(p, kOfxParamPropAnimates, 0, 0);
        g_prop->propSetString(page, kOfxParamPropPageChild, child++, h);
    }
    return kOfxStatOK;
}

// ---------------------------------------------------------------- ClipParams
// the ClipParams text (a copy: the host's pointer is only valid until the next call)
std::string clip_text(Instance* in, std::optional<double> t) {
    char* v = nullptr;
    if (t)
        check(g_param->paramGetValueAtTime(in->clip, *t, &v), "ClipParams");
    else
        check(g_param->paramGetValue(in->clip, &v), "ClipParams");
    return v ? std::string(v) : std::string();
}

// ClipParams as the instance uses it: the parameter, or while it is empty the defaults not stored yet
std::string clip_effective(Instance* in, std::optional<double> t) {
    std::string text = clip_text(in, t);
    if (text.empty()) {
        std::lock_guard<std::mutex> lk(in->mu);
        text = in->pending;
    }
    return text;
}

std::string note_of(Instance* in) {
    std::lock_guard<std::mutex> lk(in->mu);
    return in->note;
}

// the parsed ClipParams, parsed once per text
std::shared_ptr<const RenderState> clip_state(Instance* in, std::optional<double> t) {
    const std::string text = clip_effective(in, t);
    {
        std::lock_guard<std::mutex> lk(in->mu);
        if (in->state && in->text == text) return in->state;
    }
    auto st = std::make_shared<const RenderState>(parse_clip_params(text));
    std::lock_guard<std::mutex> lk(in->mu);
    in->text = text;
    in->state = st;
    return st;
}

// Adjust Filter Settings is enabled while ClipParams holds a noise profile (the genuine defines it disabled)
void update_adjust(Instance* in) {
    bool has = false;
    try {
        has = clip_state(in, std::nullopt)->has_profile;
    } catch (const Failure&) {
        throw;
    } catch (const std::exception& e) {
        persistent(in->fx, kOfxMessageError, std::string("Meat Video: ") + e.what());
    }
    OfxPropertySetHandle p;
    check(g_param->paramGetPropertySet(in->adjust, &p), "paramGetPropertySet");
    check(g_prop->propSetInt(p, kOfxParamPropEnabled, 0, has ? 1 : 0), "OfxParamPropEnabled");
}

// store ClipParams and ParamsHash1..3 (InstanceChanged / SyncPrivateData only)
void store_clip_params(Instance* in, const std::string& text) {
    int hv[3];
    params_hash(text, hv);
    // the host's undo block is closed on every path once it was opened
    struct Edit {
        OfxParamSetHandle ps;
        bool open = false;
        ~Edit() {
            if (open) g_param->paramEditEnd(ps);
        }
    } edit{in->ps};
    check(g_param->paramEditBegin(in->ps, "Meat Video settings"), "paramEditBegin");
    edit.open = true;
    check(g_param->paramSetValue(in->clip, text.c_str()), "paramSetValue ClipParams");
    for (int k = 0; k < 3; ++k) check(g_param->paramSetValue(in->hash[k], hv[k]), "paramSetValue ParamsHash");
    edit.open = false;
    check(g_param->paramEditEnd(in->ps), "paramEditEnd");
}

// A new instance (ClipParams empty): the defaults from the helper (MeatVideoQt --ofx-default, no window), kept as
// `pending`.  Not assigned: nothing (the source passes through).  Not loadable: left out, with the genuine's message.
void load_defaults(Instance* in) {
    const DefaultsResult d = request_defaults();
    std::string note, pending;
    if (!d.error.empty()) {
        note = "Meat Video: the Default Profile and Default Preset could not be read (" + d.error + ").";
    } else {
        if (d.profile == DefaultsResult::ERROR) note = "Selected default profile cannot be loaded.";
        if (d.preset == DefaultsResult::ERROR) note += std::string(note.empty() ? "" : " ") + "Selected default preset cannot be loaded.";
        if (!d.clip_params.empty()) {
            try {
                parse_clip_params(d.clip_params);
                pending = d.clip_params;
            } catch (const std::exception& e) {
                note += std::string(note.empty() ? "" : " ") + "Meat Video: the defaults cannot be used (" + e.what() + ").";
            }
        }
    }
    {
        std::lock_guard<std::mutex> lk(in->mu);
        in->pending = pending;
        in->note = note;
    }
    if (!note.empty()) persistent(in->fx, kOfxMessageWarning, note);
}

// the defaults into ClipParams, once, while it is still empty (an existing instance's ClipParams is never replaced)
// A failed store keeps them pending (still used, stored at a later InstanceChanged / SyncPrivateData).  `committing`
// stops the InstanceChanged a host may send from inside paramSetValue from storing them twice.
void commit_defaults(Instance* in) {
    std::string text;
    {
        std::lock_guard<std::mutex> lk(in->mu);
        if (in->committing || in->pending.empty()) return;
        text = in->pending;
        in->committing = true;
    }
    struct Done {
        Instance* in;
        ~Done() {
            std::lock_guard<std::mutex> lk(in->mu);
            in->committing = false;
        }
    } done{in};
    try {
        // empty: store; the same text: an earlier store set ClipParams but not ParamsHash1..3, store again; another
        // value: the host set one meanwhile, the defaults are dropped
        const std::string cur = clip_text(in, std::nullopt);
        if (cur.empty() || cur == text) store_clip_params(in, text);
        bool warned;
        std::string note;
        {
            std::lock_guard<std::mutex> lk(in->mu);
            if (in->pending == text) in->pending.clear();
            warned = in->store_warned;
            in->store_warned = false;
            note = in->note;
        }
        if (warned) {                      // the warning of a failed store is over
            if (note.empty())
                clear_persistent(in->fx);
            else
                persistent(in->fx, kOfxMessageWarning, note);
        }
    } catch (const std::exception& e) {
        {
            std::lock_guard<std::mutex> lk(in->mu);
            in->store_warned = true;
        }
        persistent(in->fx, kOfxMessageWarning,
                   std::string("Meat Video: the defaults could not be stored in the instance yet (") + e.what() +
                       "); they are used and stored later");
    }
}

// ---------------------------------------------------------------- instance
OfxStatus create_instance(OfxImageEffectHandle fx) {
    auto inst = std::make_unique<Instance>();
    inst->fx = fx;
    check(g_fx->clipGetHandle(fx, SOURCE, &inst->src, nullptr), "clipGetHandle Source");
    check(g_fx->clipGetHandle(fx, OUTPUT, &inst->dst, nullptr), "clipGetHandle Output");
    check(g_fx->getParamSet(fx, &inst->ps), "getParamSet");
    check(g_param->paramGetHandle(inst->ps, PARAM_PREPARE, &inst->prepare, nullptr), "paramGetHandle");
    check(g_param->paramGetHandle(inst->ps, PARAM_ADJUST, &inst->adjust, nullptr), "paramGetHandle");
    check(g_param->paramGetHandle(inst->ps, PARAM_CLIP, &inst->clip, nullptr), "paramGetHandle");
    for (int k = 0; k < 3; ++k) check(g_param->paramGetHandle(inst->ps, PARAM_HASH[k], &inst->hash[k], nullptr), "paramGetHandle");
    OfxPropertySetHandle props;
    check(g_fx->getPropertySet(fx, &props), "getPropertySet");
    check(g_prop->propSetPointer(props, kOfxPropInstanceData, 0, inst.get()), "kOfxPropInstanceData");
    Instance* in = inst.release();
    if (clip_text(in, std::nullopt).empty()) load_defaults(in);   // a new instance (not a saved project's)
    update_adjust(in);
    return kOfxStatOK;
}

OfxStatus destroy_instance(OfxImageEffectHandle fx) {
    OfxPropertySetHandle props;
    check(g_fx->getPropertySet(fx, &props), "getPropertySet");
    void* p = nullptr;
    g_prop->propGetPointer(props, kOfxPropInstanceData, 0, &p);
    delete static_cast<Instance*>(p);
    g_prop->propSetPointer(props, kOfxPropInstanceData, 0, nullptr);
    return kOfxStatOK;
}

// ---------------------------------------------------------------- clip helpers
struct Range {
    double first = 0, last = 0;
};

Range clip_range(Instance* in) {
    OfxPropertySetHandle cp;
    check(g_fx->clipGetPropertySet(in->src, &cp), "clipGetPropertySet");
    double r[2] = {0, 0};
    if (g_prop->propGetDoubleN(cp, kOfxImageEffectPropFrameRange, 2, r) != kOfxStatOK) {
        OfxPropertySetHandle ep;
        check(g_fx->getPropertySet(in->fx, &ep), "getPropertySet");
        check(g_prop->propGetDoubleN(ep, kOfxImageEffectPropFrameRange, 2, r), "kOfxImageEffectPropFrameRange");
    }
    return {r[0], r[1]};
}

bool fielded(const char* f) {
    return f && (std::strcmp(f, kOfxImageFieldLower) == 0 || std::strcmp(f, kOfxImageFieldUpper) == 0);
}

// One fetched image (released on destruction)
struct Image {
    OfxPropertySetHandle h = nullptr;
    unsigned char* data = nullptr;
    int bounds[4] = {0, 0, 0, 0};
    int row_bytes = 0;
    ~Image() {
        if (h) g_fx->clipReleaseImage(h);
    }
    int W() const { return bounds[2] - bounds[0]; }
    int H() const { return bounds[3] - bounds[1]; }
    RgbaView view() const { return {data, row_bytes, W(), H()}; }
};

// clipGetImage at time t (region: the default = the RoI declared, or the given one); RGBA float32 checked.
// nullptr when the host gives no image.
std::unique_ptr<Image> fetch(OfxImageClipHandle clip, double t, bool required, const OfxRectD* region = nullptr) {
    auto im = std::make_unique<Image>();
    if (g_fx->clipGetImage(clip, t, region, &im->h) != kOfxStatOK || !im->h) {
        im->h = nullptr;
        if (required) throw Failure(kOfxStatFailed, "the host gave no image at frame " + tstr(t));
        return nullptr;
    }
    char* depth = nullptr;
    char* comps = nullptr;
    check(g_prop->propGetString(im->h, kOfxImageEffectPropPixelDepth, 0, &depth), "kOfxImageEffectPropPixelDepth");
    check(g_prop->propGetString(im->h, kOfxImageEffectPropComponents, 0, &comps), "kOfxImageEffectPropComponents");
    if (!depth || std::strcmp(depth, kOfxBitDepthFloat) != 0 || !comps || std::strcmp(comps, kOfxImageComponentRGBA) != 0)
        throw Failure(kOfxStatFailed, std::string("unsupported image format (") + (comps ? comps : "?") + ", " +
                                          (depth ? depth : "?") + "): RGBA float32 only");
    char* field = nullptr;
    if (g_prop->propGetString(im->h, kOfxImagePropField, 0, &field) == kOfxStatOK && fielded(field))
        throw Failure(kOfxStatFailed, "interlaced (fielded) images are not supported: progressive only");
    void* data = nullptr;
    check(g_prop->propGetPointer(im->h, kOfxImagePropData, 0, &data), "kOfxImagePropData");
    check(g_prop->propGetIntN(im->h, kOfxImagePropBounds, 4, im->bounds), "kOfxImagePropBounds");
    check(g_prop->propGetInt(im->h, kOfxImagePropRowBytes, 0, &im->row_bytes), "kOfxImagePropRowBytes");
    im->data = static_cast<unsigned char*>(data);
    if (!im->data || im->W() <= 0 || im->H() <= 0) throw Failure(kOfxStatFailed, "empty image");
    return im;
}

bool aborted(Instance* in) { return g_fx->abort(in->fx) != 0; }

// the frame at time t converted to internal planes, the image released afterwards; nullopt when the host gives no
// image there (clipGetImage kOfxStatFailed: "no image at that time, the plug-in should continue")
std::optional<Planes> fetch_planes(Instance* in, double t, int matrix, int W, int H) {
    std::unique_ptr<Image> im = fetch(in->src, t, false);
    if (!im) return std::nullopt;
    if (im->W() != W || im->H() != H)
        throw Failure(kOfxStatFailed, "frame " + tstr(t) + " has another size than the rendered frame");
    Planes p;
    rgba_to_internal(im->view(), matrix, &p, nullptr);
    return p;
}

// the matrix of untagged host frames (nv.matrix_from_tags(None, H)): BT.709 from 720 lines up, else BT.601
const char* auto_matrix(int H) { return H >= 720 ? "bt709" : "bt601"; }

// ---------------------------------------------------------------- the window (buttons)
// Prepare Noise Profile / Adjust Filter Settings: the frames around time t from the host, the window on them, and on
// its Apply the new ClipParams (+ ParamsHash1..3)
std::atomic<bool> g_window_open{false};      // one window at a time, whichever instance and thread asks

void open_window(Instance* in, const char* page, double t) {
    // a button pressed again while a window is open (the host delivered it during the wait, or from another thread)
    if (g_window_open.exchange(true)) {
        persistent(in->fx, kOfxMessageWarning, "Meat Video: a Meat Video window is already open; close it first.");
        return;
    }
    struct Closed {
        ~Closed() { g_window_open = false; }
    } closed;
    const Range cr = clip_range(in);
    SessionRequest rq;
    rq.page = page;
    rq.clip_params = clip_effective(in, std::nullopt);
    // the current frame, then outwards on each side up to WINDOW_HALF frames, ending at the clip end or at the first
    // frame the host gives no image for (as the render's temporal window)
    std::map<double, std::vector<float>> got;
    auto get = [&](double tk) {
        OfxRectD rod;
        check(g_fx->clipGetRegionOfDefinition(in->src, tk, &rod), "clipGetRegionOfDefinition");
        std::unique_ptr<Image> im = fetch(in->src, tk, tk == t, &rod);
        if (!im) return false;
        if (got.empty()) {
            rq.W = im->W();
            rq.H = im->H();
        } else if (im->W() != rq.W || im->H() != rq.H) {
            throw Failure(kOfxStatFailed, "frame " + tstr(tk) + " has another size than frame " + tstr(t));
        }
        got.emplace(tk, rgba_to_gbr(im->view()));
        return true;
    };
    get(t);
    for (int k = 1; k <= WINDOW_HALF && t + k <= cr.last && get(t + k); ++k) {
    }
    for (int k = 1; k <= WINDOW_HALF && t - k >= cr.first && get(t - k); ++k) {
    }
    for (auto& [tk, f] : got) {
        if (tk == t) rq.current = static_cast<int>(rq.times.size());
        rq.times.push_back(tk);
        rq.frames.push_back(std::move(f));
    }
    got.clear();
    if (std::min(rq.W, rq.H) < 64) throw Failure(kOfxStatFailed, "frames below 64 x 64 pixels are not supported");
    rq.matrix = auto_matrix(rq.H);
    OfxPropertySetHandle cp;
    check(g_fx->clipGetPropertySet(in->src, &cp), "clipGetPropertySet");
    if (g_prop->propGetDouble(cp, kOfxImageEffectPropFrameRate, 0, &rq.fps) != kOfxStatOK || !(rq.fps > 0)) rq.fps = 24.0;

    const SessionResult r = run_session(rq);
    rq.frames.clear();
    if (r.kind == SessionResult::CANCEL) return;
    if (r.kind == SessionResult::FAILED) throw Failure(kOfxStatFailed, r.error + "; the settings were not changed");
    // Apply: the new state, checked before it is stored
    try {
        parse_clip_params(r.clip_params);
    } catch (const std::exception& e) {
        throw Failure(kOfxStatFailed, std::string("the window returned settings the render cannot use (") + e.what() +
                                          "); the settings were not changed");
    }
    store_clip_params(in, r.clip_params);
    {
        std::lock_guard<std::mutex> lk(in->mu);
        in->pending.clear();
        in->note.clear();
    }
    update_adjust(in);
    clear_persistent(in->fx);
}

OfxStatus instance_changed(OfxImageEffectHandle fx, OfxPropertySetHandle in_args) {
    char* type = nullptr;
    char* name = nullptr;
    char* reason = nullptr;
    commit_defaults(instance_of(fx));
    if (g_prop->propGetString(in_args, kOfxPropType, 0, &type) != kOfxStatOK || !type ||
        std::strcmp(type, kOfxTypeParameter) != 0)
        return kOfxStatReplyDefault;
    if (g_prop->propGetString(in_args, kOfxPropName, 0, &name) != kOfxStatOK || !name) return kOfxStatReplyDefault;
    const std::string n = name;
    g_prop->propGetString(in_args, kOfxPropChangeReason, 0, &reason);
    const bool user = reason && std::strcmp(reason, kOfxChangeUserEdited) == 0;
    Instance* in = instance_of(fx);
    if ((n == PARAM_PREPARE || n == PARAM_ADJUST) && user) {
        double t = 0;
        check(g_prop->propGetDouble(in_args, kOfxPropTime, 0, &t), "kOfxPropTime");
        if (n == PARAM_ADJUST && !clip_state(in, std::nullopt)->has_profile) {
            update_adjust(in);
            return kOfxStatOK;                   // no profile: the button is disabled (a host that ignores that)
        }
        open_window(in, n == PARAM_PREPARE ? "prepare" : "adjust", t);
        return kOfxStatOK;
    }
    if (n == PARAM_CLIP) {
        update_adjust(in);                       // set by the window, an undo, a paste or a loaded project
        return kOfxStatOK;
    }
    return kOfxStatReplyDefault;
}

// ---------------------------------------------------------------- actions
OfxStatus region_of_definition(OfxImageEffectHandle fx, OfxPropertySetHandle in_args, OfxPropertySetHandle out_args) {
    Instance* in = instance_of(fx);
    double t = 0;
    check(g_prop->propGetDouble(in_args, kOfxPropTime, 0, &t), "kOfxPropTime");
    OfxRectD r;
    check(g_fx->clipGetRegionOfDefinition(in->src, t, &r), "clipGetRegionOfDefinition");
    const double v[4] = {r.x1, r.y1, r.x2, r.y2};
    check(g_prop->propSetDoubleN(out_args, kOfxImageEffectPropRegionOfDefinition, 4, v), "RoD");
    return kOfxStatOK;
}

OfxStatus regions_of_interest(OfxImageEffectHandle fx, OfxPropertySetHandle in_args, OfxPropertySetHandle out_args) {
    // the filters need the whole source frame whatever the region rendered
    Instance* in = instance_of(fx);
    double t = 0;
    check(g_prop->propGetDouble(in_args, kOfxPropTime, 0, &t), "kOfxPropTime");
    OfxRectD r;
    check(g_fx->clipGetRegionOfDefinition(in->src, t, &r), "clipGetRegionOfDefinition");
    const double v[4] = {r.x1, r.y1, r.x2, r.y2};
    check(g_prop->propSetDoubleN(out_args, "OfxImageClipPropRoI_Source", 4, v), "RoI");
    return kOfxStatOK;
}

OfxStatus frames_needed(OfxImageEffectHandle fx, OfxPropertySetHandle in_args, OfxPropertySetHandle out_args) {
    // the temporal window t - R .. t + R inside the clip (R of ClipParams; 0 without a profile)
    Instance* in = instance_of(fx);
    double t = 0;
    check(g_prop->propGetDouble(in_args, kOfxPropTime, 0, &t), "kOfxPropTime");
    const std::shared_ptr<const RenderState> s = clip_state(in, t);
    const int R = s->has_profile ? s->radius : 0;
    const Range cr = clip_range(in);
    double v[2] = {std::fmax(t - R, cr.first), std::fmin(t + R, cr.last)};
    if (v[1] < v[0]) v[0] = v[1] = t;
    check(g_prop->propSetDoubleN(out_args, "OfxImageClipPropFrameRange_Source", 2, v), "frame range");
    return kOfxStatOK;
}

// write the render window of the destination: pix(top-down row i, column j) for the source pixels, 0 outside them
template <class Pix>
void write_window(const Image& dst, const Image& src, const int win[4], Pix pix) {
    const int W = src.W(), H = src.H(), sx1 = src.bounds[0], sy1 = src.bounds[1];
    for (int y = std::max(win[1], dst.bounds[1]); y < std::min(win[3], dst.bounds[3]); ++y) {
        float* drow = reinterpret_cast<float*>(dst.data + static_cast<std::ptrdiff_t>(y - dst.bounds[1]) * dst.row_bytes);
        for (int x = std::max(win[0], dst.bounds[0]); x < std::min(win[2], dst.bounds[2]); ++x) {
            float* d = drow + 4 * (x - dst.bounds[0]);
            const int j = x - sx1, ib = y - sy1;            // column, bottom-up row in the source
            if (j < 0 || j >= W || ib < 0 || ib >= H) {
                d[0] = d[1] = d[2] = d[3] = 0.0f;
                continue;
            }
            pix(H - 1 - ib, j, d);                           // top-down row of the planes
        }
    }
}

// Render.  Failures (unsupported input, missing frames, errors) are kOfxStatFailed with a persistent message; an abort
// by the host is kOfxStatFailed without one (the host asked for no result, and the previous message is kept).
OfxStatus render(OfxImageEffectHandle fx, OfxPropertySetHandle in_args) {
    Instance* in = instance_of(fx);
    double t = 0;
    check(g_prop->propGetDouble(in_args, kOfxPropTime, 0, &t), "kOfxPropTime");
    char* field = nullptr;
    if (g_prop->propGetString(in_args, kOfxImageEffectPropFieldToRender, 0, &field) == kOfxStatOK && fielded(field))
        throw Failure(kOfxStatFailed, "field rendering is not supported: progressive material only");
    OfxPropertySetHandle cp;
    check(g_fx->clipGetPropertySet(in->src, &cp), "clipGetPropertySet");
    char* order = nullptr;
    if (g_prop->propGetString(cp, kOfxImageClipPropFieldOrder, 0, &order) == kOfxStatOK && fielded(order))
        throw Failure(kOfxStatFailed, "interlaced source clip (field order " + std::string(order) +
                                          ") is not supported: progressive material only");
    int win[4] = {0, 0, 0, 0};
    check(g_prop->propGetIntN(in_args, kOfxImageEffectPropRenderWindow, 4, win), "kOfxImageEffectPropRenderWindow");

    const std::shared_ptr<const RenderState> s = clip_state(in, t);
    const Range cr = clip_range(in);
    std::unique_ptr<Image> dst = fetch(in->dst, t, true);
    std::unique_ptr<Image> src = fetch(in->src, t, true);
    if (!s->has_profile) {
        // no noise profile: the source as it is (alpha included)
        const RgbaView sv = src->view();
        write_window(*dst, *src, win, [&](int i, int j, float* d) { std::memcpy(d, sv.px(i, j), 16); });
        const std::string note = note_of(in);
        persistent(fx, kOfxMessageWarning, note.empty() ? std::string(NO_PROFILE) : note + " " + NO_PROFILE);
        return kOfxStatOK;
    }
    const int W = src->W(), H = src->H();
    if (std::min(W, H) < 64) throw Failure(kOfxStatFailed, "frames below 64 x 64 pixels are not supported");
    const int matrix = s->matrix < 0 ? (H >= 720 ? 0 : 1) : s->matrix;   // "auto": auto_matrix(H)
    Restore rs;
    Planes cur;
    rgba_to_internal(src->view(), matrix, &cur, &rs);
    if (aborted(in)) return kOfxStatFailed;

    // the temporal window t - R .. t + R inside the clip (backend PreviewEngine.window / the export's window); on each
    // side it ends at the first neighbour the host does not give, as at the clip end
    std::map<double, Planes> got;
    auto planes_at = [&](double tk) -> const Planes* {
        std::optional<Planes> p = fetch_planes(in, tk, matrix, W, H);
        return p ? &got.emplace(tk, std::move(*p)).first->second : nullptr;
    };
    const int R = s->radius;
    std::vector<const Planes*> left, right;
    for (int k = 1; k <= R && t - k >= cr.first; ++k) {
        const Planes* p = planes_at(t - k);
        if (aborted(in)) return kOfxStatFailed;
        if (!p) break;
        left.push_back(p);
    }
    for (int k = 1; k <= R && t + k <= cr.last; ++k) {
        const Planes* p = planes_at(t + k);
        if (aborted(in)) return kOfxStatFailed;
        if (!p) break;
        right.push_back(p);
    }
    std::vector<const Planes*> window(left.rbegin(), left.rend());
    const int ci = static_cast<int>(window.size());
    window.push_back(&cur);
    window.insert(window.end(), right.begin(), right.end());
    const Planes out = process_frame(window, ci, *s);
    window.clear();
    got.clear();
    const std::vector<float> rgb = internal_to_rgb(out, matrix, &rs);

    // the render window: rows of the source frame (OFX y = bounds.y1 + bottom-up row), alpha from the source
    const RgbaView sv = src->view();
    write_window(*dst, *src, win, [&](int i, int j, float* d) {
        const float* p = rgb.data() + 3 * (static_cast<size_t>(i) * W + j);
        d[0] = p[0];
        d[1] = p[1];
        d[2] = p[2];
        d[3] = sv.px(i, j)[3];
    });
    const std::string note = note_of(in);
    if (note.empty())
        clear_persistent(fx);
    else
        persistent(fx, kOfxMessageWarning, note);
    return kOfxStatOK;
}

OfxStatus clip_preferences(OfxPropertySetHandle out_args) {
    // what the plug-in takes and gives: RGBA float32 on both clips (the host converts other depths if it can)
    for (const char* c : {SOURCE, OUTPUT}) {
        check(g_prop->propSetString(out_args, (std::string("OfxImageClipPropComponents_") + c).c_str(), 0,
                                    kOfxImageComponentRGBA),
              "clip preferences");
        check(g_prop->propSetString(out_args, (std::string("OfxImageClipPropDepth_") + c).c_str(), 0, kOfxBitDepthFloat),
              "clip preferences");
    }
    return kOfxStatOK;
}

OfxStatus on_load() {
    if (!g_host) return kOfxStatErrMissingHostFeature;
    g_prop = static_cast<const OfxPropertySuiteV1*>(g_host->fetchSuite(g_host->host, kOfxPropertySuite, 1));
    g_param = static_cast<const OfxParameterSuiteV1*>(g_host->fetchSuite(g_host->host, kOfxParameterSuite, 1));
    g_fx = static_cast<const OfxImageEffectSuiteV1*>(g_host->fetchSuite(g_host->host, kOfxImageEffectSuite, 1));
    g_msg2 = static_cast<const OfxMessageSuiteV2*>(g_host->fetchSuite(g_host->host, kOfxMessageSuite, 2));
    if (!g_prop || !g_param || !g_fx) return kOfxStatErrMissingHostFeature;
    return kOfxStatOK;
}

OfxStatus main_entry(const char* action, const void* handle, OfxPropertySetHandle in_args, OfxPropertySetHandle out_args) {
    OfxImageEffectHandle fx = static_cast<OfxImageEffectHandle>(const_cast<void*>(handle));
    try {
        if (std::strcmp(action, kOfxActionLoad) == 0) return on_load();
        if (std::strcmp(action, kOfxActionUnload) == 0) return kOfxStatOK;
        if (std::strcmp(action, kOfxActionDescribe) == 0) return describe(fx);
        if (std::strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) return describe_in_context(fx);
        if (std::strcmp(action, kOfxActionCreateInstance) == 0) return create_instance(fx);
        if (std::strcmp(action, kOfxActionDestroyInstance) == 0) return destroy_instance(fx);
        if (std::strcmp(action, kOfxActionInstanceChanged) == 0) return instance_changed(fx, in_args);
        if (std::strcmp(action, kOfxActionSyncPrivateData) == 0) {
            commit_defaults(instance_of(fx));        // before the host saves / copies the instance
            return kOfxStatOK;
        }
        if (std::strcmp(action, kOfxImageEffectActionGetRegionOfDefinition) == 0)
            return region_of_definition(fx, in_args, out_args);
        if (std::strcmp(action, kOfxImageEffectActionGetRegionsOfInterest) == 0)
            return regions_of_interest(fx, in_args, out_args);
        if (std::strcmp(action, kOfxImageEffectActionGetFramesNeeded) == 0) return frames_needed(fx, in_args, out_args);
        if (std::strcmp(action, kOfxImageEffectActionGetClipPreferences) == 0) return clip_preferences(out_args);
        if (std::strcmp(action, kOfxImageEffectActionRender) == 0) return render(fx, in_args);
        return kOfxStatReplyDefault;
    } catch (const Failure& e) {
        persistent(fx, kOfxMessageError, std::string("Meat Video: ") + e.what());
        return e.st;
    } catch (const std::bad_alloc&) {
        return kOfxStatErrMemory;
    } catch (const std::exception& e) {
        persistent(fx, kOfxMessageError, std::string("Meat Video: ") + e.what());
        return kOfxStatFailed;
    } catch (...) {
        return kOfxStatFailed;
    }
}

void set_host(OfxHost* h) { g_host = h; }

OfxPlugin g_plugin = {kOfxImageEffectPluginApi, 1, PLUGIN_ID, 1, 0, set_host, main_entry};

}  // namespace

MV_EXPORT int OfxGetNumberOfPlugins(void) { return 1; }

MV_EXPORT OfxPlugin* OfxGetPlugin(int nth) { return nth == 0 ? &g_plugin : nullptr; }

// the genuine returns 0 (static disassembly of its export: mov w0, #0; ret): the host keeps the plug-in loaded
MV_EXPORT int FoundryOfxAllowPluginUnload(void) { return 0; }
