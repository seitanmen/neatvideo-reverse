// ofx_session - see ofx_session.h.
#include "ofx_session.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QPushButton>
#include <QTabBar>
#include <QTimer>
#include <QToolButton>
#include <QUndoStack>

#include <fcntl.h>
#include <unistd.h>
#include <xlocale.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>

#include "filter_panel.h"
#include "mainwindow.h"
#include "profile_panel.h"
#include "rows.h"
#include "timeline.h"

namespace nvqt {

namespace {

// JSON numbers in the "C" locale whatever QApplication set (PyObj::parse uses strtod)
PyObj parse_c(const std::string& s) {
    static const locale_t c_loc = newlocale(LC_ALL_MASK, "C", nullptr);
    const locale_t old = uselocale(c_loc);
    try {
        PyObj o = PyObj::parse(s);
        uselocale(old);
        return o;
    } catch (...) {
        uselocale(old);
        throw;
    }
}

// stdin with the bytes read past the header line kept for the frames
struct In {
    std::vector<uint8_t> buf;
    size_t pos = 0;
    bool eof = false;
    bool fill() {
        if (eof) return false;
        if (pos > 0) {
            buf.erase(buf.begin(), buf.begin() + static_cast<std::ptrdiff_t>(pos));
            pos = 0;
        }
        const size_t old = buf.size();
        buf.resize(old + (1u << 20));
        ssize_t r;
        do {
            r = ::read(0, buf.data() + old, 1u << 20);
        } while (r < 0 && errno == EINTR);
        buf.resize(old + static_cast<size_t>(r > 0 ? r : 0));
        if (r <= 0) eof = true;
        return r > 0;
    }
    // the header: bytes up to "\n" (at most `limit`)
    bool line(std::string* out, size_t limit) {
        for (;;) {
            const auto b = buf.begin() + static_cast<std::ptrdiff_t>(pos);
            const auto nl = std::find(b, buf.end(), static_cast<uint8_t>('\n'));
            if (nl != buf.end()) {
                out->assign(b, nl);
                pos = static_cast<size_t>(nl - buf.begin()) + 1;
                return true;
            }
            if (buf.size() - pos > limit || !fill()) return false;
        }
    }
    bool exact(std::vector<uint8_t>* out, size_t n) {
        out->clear();
        while (buf.size() - pos < n && !eof) fill();
        if (buf.size() - pos < n) return false;
        out->assign(buf.begin() + static_cast<std::ptrdiff_t>(pos), buf.begin() + static_cast<std::ptrdiff_t>(pos + n));
        pos += n;
        return true;
    }
};

bool write_all(int fd, const std::string& s) {
    const char* p = s.data();
    size_t n = s.size();
    while (n > 0) {
        const ssize_t w = ::write(fd, p, n);
        if (w < 0 && errno == EINTR) continue;
        if (w <= 0) return false;
        p += w;
        n -= static_cast<size_t>(w);
    }
    return true;
}

// fd 1 (the reply was dup'ed away) onto stderr, so stray output cannot get into the reply; the host may have started
// the helper without a stderr (its fd 2 closed): then fd 1 and 2 go to /dev/null
bool stray_output_to_stderr() {
    if (::fcntl(2, F_GETFD) != -1) return ::dup2(2, 1) >= 0;
    const int n = ::open("/dev/null", O_WRONLY);
    if (n < 0) return false;
    const bool ok = ::dup2(n, 1) >= 0 && ::dup2(n, 2) >= 0;
    if (n > 2) ::close(n);
    return ok;
}

int fail(const std::string& why) {
    std::fprintf(stderr, "ofx-session: %s\n", why.c_str());
    return 1;
}

}  // namespace

std::string clip_params_text(const PyObj& params, const ProfileState& ps, const std::string& matrix, bool full_range,
                             int W, int H, const std::optional<Interlace>& il) {
    PyObj d = PyObj::dict();
    d.set("format", PyObj::str("meatvideo_ofx_clip_params"));
    d.set("version", PyObj::integer(il ? 2 : 1));    // INT-4: version 2 = interlaced (an older plug-in refuses it)
    if (W > 0 && H > 0) {
        PyObj fs = PyObj::list();
        fs.append(PyObj::integer(W));
        fs.append(PyObj::integer(H));
        d.set("frame_size", fs);
    } else {
        d.set("frame_size", PyObj::none());
    }
    d.set("preset", preset_json(params));
    const bool has = ps.has_profile();
    d.set("profile", has ? ps.to_json() : PyObj::none());
    if (has) {
        PyObj r = PyObj::dict();
        r.set("matrix", PyObj::str(matrix));
        r.set("full_range", PyObj::boolean(full_range));
        PyObj P = backend_params(params);
        if (il) P.set("interlace", interlace_json(*il));   // the render's field processing (fields.h)
        r.set("P", P);
        r.set("profile", ps.effective());
        d.set("render", r);
    } else {
        d.set("render", PyObj::none());
    }
    if (il) d.set("scan", PyObj::str(il->first == 0 ? "tff" : "bff"));   // the window's Video Scan Type
    return d.dump();
}

std::string clip_params_json(MainWindow* win) {
    return clip_params_text(win->state->params, win->state->profile, win->matrix, win->full_range,
                            win->info ? win->info->W : 0, win->info ? win->info->H : 0, win->interlace());
}

Defaults read_defaults() {
    // the values Assign As Default / Change Default… store in the UI's settings (the whole profile / preset as JSON);
    // a profile counts as loadable when ProfileState::from_json takes it and its effective profile can be made
    Defaults d;
    auto st = settings();
    const QVariant vp = st->value("default_profile");
    if (vp.isValid() && !vp.toString().isEmpty()) {
        try {
            ProfileState ps = ProfileState::from_json(parse_c(vp.toString().toStdString()));
            if (!ps.has_profile()) throw std::runtime_error("no profile");
            (void)ps.effective();
            d.profile = std::move(ps);
        } catch (const std::exception& e) {
            d.profile_error = e.what();
        }
    }
    const QVariant vq = st->value("default_preset");
    if (vq.isValid() && !vq.toString().isEmpty()) {
        try {
            d.preset = params_from_json(parse_c(vq.toString().toStdString()));
        } catch (const std::exception& e) {
            d.preset_error = e.what();
        }
    }
    return d;
}

QString Defaults::message() const {
    QStringList m;
    if (!profile_error.empty()) m << DEFAULT_PROFILE_ERROR;
    if (!preset_error.empty()) m << DEFAULT_PRESET_ERROR;
    return m.join(' ');
}

int ofx_default_main() {
    // "DEFAULTS <n> <profile> <preset>\n" + n bytes: the ClipParams of a new instance (empty without either default);
    // <profile> / <preset> = none (not assigned), ok, error (assigned but not loadable: left out)
    const int reply_fd = ::dup(1);
    if (reply_fd < 0 || !stray_output_to_stderr()) return fail("cannot set up stdout");
    const Defaults d = read_defaults();
    auto status = [](bool set, const std::string& err) { return !err.empty() ? "error" : set ? "ok" : "none"; };
    std::string text;
    if (d.profile || d.preset)
        // the matrix of the host's frames is not known yet: "auto" = the window's rule for untagged frames, by height
        text = clip_params_text(d.preset ? *d.preset : default_params(), d.profile ? *d.profile : ProfileState(), "auto",
                                true, 0, 0);
    const std::string head = "DEFAULTS " + std::to_string(text.size()) + " " +
                             status(d.profile.has_value(), d.profile_error) + " " +
                             status(d.preset.has_value(), d.preset_error) + "\n";
    if (!d.profile_error.empty()) std::fprintf(stderr, "ofx-default: default profile: %s\n", d.profile_error.c_str());
    if (!d.preset_error.empty()) std::fprintf(stderr, "ofx-default: default preset: %s\n", d.preset_error.c_str());
    const bool ok = write_all(reply_fd, head + text);
    ::close(reply_fd);
    return ok ? 0 : 1;
}

void apply_clip_params(MainWindow* win, const std::string& text) {
    const PyObj d = parse_c(text);
    if (!d.is_dict() || d.get("format").type() != PyObj::STR || d.get("format").as_str() != "meatvideo_ofx_clip_params")
        throw std::runtime_error("not a Meat Video ClipParams (format meatvideo_ofx_clip_params)");
    const int64_t ver = d.get("version").type() == PyObj::INT ? d.get("version").as_int() : -1;
    if (ver != 1 && ver != 2)
        throw std::runtime_error("ClipParams version " + d.get("version").dump() + " is not supported (1, 2)");
    const PyObj P = params_from_json(d.get("preset"));
    std::optional<ProfileState> ps;
    if (!d.get("profile").is_none()) ps = ProfileState::from_json(d.get("profile"));
    // INT-4: the Video Scan Type the instance was applied with, before the profile it belongs to: version 2 = interlaced
    // ("scan" tff / bff); version 1 = progressive - except the ClipParams a new instance got from the Default Profile /
    // Preset (frame_size null: no frames seen yet), which keeps the scan the window started with (the host's order)
    const PyObj& sc = d.get("scan");
    if (ver == 2) {
        const std::string scan = sc.type() == PyObj::STR ? sc.as_str() : std::string();
        if (scan != "tff" && scan != "bff")
            throw std::runtime_error("ClipParams version 2 needs \"scan\": \"tff\" or \"bff\"");
        win->set_scan(scan == "tff" ? 0 : 1, false);
    } else if (!sc.is_none()) {
        throw std::runtime_error("ClipParams version 1 has no \"scan\" (interlaced settings are version 2)");
    } else if (!d.get("frame_size").is_none()) {
        win->set_scan(-1, false);
    }
    win->state->set_all_params(P, "OFX settings");
    if (ps) {
        win->prof_clean = ps->to_json().dump(-1, true);
        win->state->set_profile(*ps, "OFX profile");
    }
    win->state->undo->clear();            // the restored state is where the window starts
}

int ofx_session_main(const QStringList& /*rest*/, const std::function<void(MainWindow*, const OfxSession&)>& on_shown) {
    // the reply goes to the original stdout; everything else written to fd 1 lands on stderr
    const int reply_fd = ::dup(1);
    if (reply_fd < 0 || !stray_output_to_stderr()) return fail("cannot set up stdout");
    ::fcntl(reply_fd, F_SETFD, FD_CLOEXEC);

    In in;
    std::string head;
    if (!in.line(&head, 256u << 20)) return fail("no request header on stdin");
    auto s = std::make_shared<OfxSession>();
    PyObj h;
    try {
        h = parse_c(head);
    } catch (const std::exception& e) {
        return fail(std::string("malformed request header: ") + e.what());
    }
    auto str = [&](const char* k) { return h.get(k).type() == PyObj::STR ? h.get(k).as_str() : std::string(); };
    if (str("protocol") != "meatvideo-ofx-session" || h.get("version").type() != PyObj::INT || h.get("version").as_int() != 1)
        return fail("not a meatvideo-ofx-session version 1 request");
    s->page = str("page");
    s->matrix = str("matrix");
    s->clip_params = str("clip_params");
    // INT-4: the host's field order (kOfxImageClipPropFieldOrder of the Source clip, sent only when fielded)
    const std::string fo = str("field_order");
    s->field_order = fo == "tff" ? 0 : fo == "bff" ? 1 : -1;
    const PyObj& times = h.get("times");
    if ((s->page != "prepare" && s->page != "adjust") || (s->matrix != "bt709" && s->matrix != "bt601" && s->matrix != "bt2020") ||
        str("format") != "gbrpf32le" || h.get("width").type() != PyObj::INT || h.get("height").type() != PyObj::INT ||
        !h.get("fps").is_num() || !times.is_list() || times.size() == 0 || h.get("current").type() != PyObj::INT)
        return fail("malformed request header (page, matrix, format, width, height, fps, times, current)");
    const int64_t W = h.get("width").as_int(), H = h.get("height").as_int(), cur = h.get("current").as_int();
    if (W < 1 || H < 1 || W > 65536 || H > 65536 || cur < 0 || cur >= static_cast<int64_t>(times.size()))
        return fail("request header out of range (width, height, current)");
    s->W = static_cast<int>(W);
    s->H = static_cast<int>(H);
    s->current = static_cast<int>(cur);
    s->fps = h.get("fps").as_double();
    for (const PyObj& t : times.items()) s->times.push_back(t.as_double());

    // the frames, as a gbrpf32le video read at --depth 32 (denoise_video_v5.read_frames)
    const PixFmt fmt = pixfmt("gbrpf32le");
    const size_t fb = static_cast<size_t>(fmt.frame_bytes(s->W, s->H));
    std::vector<Frame> frames;
    std::vector<uint8_t> raw;
    for (size_t k = 0; k < s->times.size(); ++k) {
        if (!in.exact(&raw, fb))
            return fail("the request ended after " + std::to_string(k) + " of " + std::to_string(s->times.size()) +
                        " frames");
        RestoreMap rm;
        frames.push_back(frame_from_planes(to_internal(raw.data(), fmt, s->W, s->H, s->matrix, &rm)));
        s->restore.push_back(std::move(rm));
    }
    raw.clear();
    raw.shrink_to_fit();

    QApplication::setApplicationName("nv_ui_qt (OFX)");
    auto* win = new MainWindow();          // no video path
    win->set_ofx_frames(std::move(frames), s->W, s->H, s->fps, 32, s->matrix, true);
    win->timeline->set_frame(s->current, true);
    // INT-4: the window's Video Scan Type is the user's (the genuine's own menu): the button works here; a new
    // instance (or ClipParams without "scan") starts from the host's field order
    win->ofx_scan = true;
    win->scan_detected = s->field_order;
    win->set_scan(s->field_order, false);
    if (!s->clip_params.empty()) {
        try {
            apply_clip_params(win, s->clip_params);
        } catch (const std::exception& e) {
            win->say(QString("The instance's settings could not be read (%1); starting from the defaults").arg(e.what()));
        }
    } else {
        // a new instance: the Default Profile (the Default Preset is set by MainWindow's constructor)
        const Defaults d = read_defaults();
        if (d.profile) {
            win->prof_clean = d.profile->to_json().dump(-1, true);
            win->state->set_profile(*d.profile, "Default profile");
            win->state->undo->clear();
        }
        if (!d.message().isEmpty()) win->say(d.message());
    }
    // Reset Profile / Reset Filter Settings return to the Default Profile / Preset ("Default Profile is used when a new
    // instance of the effect is created or the current profile is reset"); the standalone UI keeps its own behaviour
    auto reset_profile = [win] {
        const Defaults d = read_defaults();
        win->state->set_profile(d.profile ? *d.profile : ProfileState(), "Reset Profile");
        win->say(!d.profile_error.empty() ? DEFAULT_PROFILE_ERROR
                 : d.profile           ? QString("Reset Profile: the Default Profile")
                                       : QString("Reset Profile: no Default Profile assigned"));
    };
    auto reset_filter = [win] {
        const Defaults d = read_defaults();
        win->reset_filter();               // the Default Preset, or the factory settings without a loadable one
        if (!d.preset_error.empty()) win->say(DEFAULT_PRESET_ERROR);
    };
    for (QObject* o : std::initializer_list<QObject*>{win->actions.at("p_reset"), win->profile_panel->icons->reset})
        QObject::disconnect(o, nullptr, win, nullptr);
    for (QObject* o : std::initializer_list<QObject*>{win->actions.at("f_reset"), win->filter_panel->icons->reset})
        QObject::disconnect(o, nullptr, win, nullptr);
    QObject::connect(win->actions.at("p_reset"), &QAction::triggered, win, reset_profile);
    QObject::connect(win->profile_panel->icons->reset, &QToolButton::clicked, win, reset_profile);
    QObject::connect(win->actions.at("f_reset"), &QAction::triggered, win, reset_filter);
    QObject::connect(win->filter_panel->icons->reset, &QToolButton::clicked, win, reset_filter);
    win->tabs->setCurrentIndex(s->page == "adjust" ? 1 : 0);
    win->setWindowTitle(s->page == "adjust" ? "Meat Video - Adjust Filter Settings" : "Meat Video - Prepare Noise Profile");
    win->b_apply->setToolTip("Apply: return the noise profile and the filter settings to the host's effect instance");
    win->b_cancel->setToolTip("Cancel: close the window and leave the effect instance unchanged");

    // the reply, once: Apply -> the new ClipParams, Cancel / closing the window -> CANCEL
    auto replied = std::make_shared<bool>(false);
    auto reply = [reply_fd, replied](const std::string& text) {
        if (*replied) return;
        *replied = true;
        if (!write_all(reply_fd, text)) std::fprintf(stderr, "ofx-session: the host did not take the reply\n");
        ::close(reply_fd);
    };
    QObject::disconnect(win->b_apply, &QPushButton::clicked, nullptr, nullptr);
    QObject::connect(win->b_apply, &QPushButton::clicked, win, [win, reply] {
        const std::string cp = clip_params_json(win);
        reply("APPLY " + std::to_string(cp.size()) + "\n" + cp);
        win->close();
    });
    win->close_hook = [reply](QCloseEvent*) { reply("CANCEL\n"); };
    win->after_close_hook = [](QCloseEvent* e) {
        if (e->isAccepted()) QApplication::quit();   // the host waits for the exit
    };
    win->show();
    win->raise();                          // started by the host's process, not by the Finder: bring it to the front
    win->activateWindow();
    std::fprintf(stderr, "ofx-session: pid %d window shown (%s, %d frames, current %d)\n", static_cast<int>(::getpid()),
                 s->page.c_str(), static_cast<int>(s->times.size()), s->current);
    std::fflush(stderr);
    if (on_shown) QTimer::singleShot(0, win, [win, on_shown, s] { on_shown(win, *s); });
    QApplication::exec();
    delete win;
    reply("CANCEL\n");                     // the event loop ended without a close (never expected)
    return 0;
}

}  // namespace nvqt
