// ofx - see ofx.h.
#include "ofx.h"

#include <QApplication>
#include <QCloseEvent>
#include <QPushButton>
#include <QTimer>

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unistd.h>

#include "depth.h"
#include "mainwindow.h"
#include "timeline.h"

namespace nvqt {

namespace {

// int() / float() of a Python argument (surrounding whitespace allowed, the whole text a number); an integer beyond
// the 64-bit range (Python's int has none) saturates to +-LLONG_MAX
bool py_int(const QString& s, long long* v) {
    const std::string t = s.trimmed().toStdString();
    if (t.empty()) return false;
    try {
        size_t used = 0;
        *v = std::stoll(t, &used, 10);
        return used == t.size();
    } catch (const std::out_of_range&) {
        const size_t i = (t[0] == '+' || t[0] == '-') ? 1 : 0;
        if (i == t.size() || t.find_first_not_of("0123456789", i) != std::string::npos) return false;
        *v = t[0] == '-' ? -LLONG_MAX : LLONG_MAX;
        return true;
    } catch (...) {
        return false;
    }
}
bool py_float(const QString& s, double* v) {
    const std::string t = s.trimmed().toStdString();
    if (t.empty()) return false;
    try {
        size_t used = 0;
        *v = std::stod(t, &used);
        return used == t.size();
    } catch (...) {
        return false;
    }
}

// ofx_bridge.read_exact: exactly n bytes of stdin, false on EOF.  The buffer grows with what arrives (1 MiB reads),
// so a frame size larger than the input never allocates more than the input (as Python's reads)
bool read_exact(std::vector<uint8_t>& buf, uint64_t n) {
    constexpr uint64_t CHUNK = 1u << 20;
    buf.clear();
    while (buf.size() < n) {
        const size_t old = buf.size();
        const size_t want = static_cast<size_t>(std::min<uint64_t>(n - old, CHUNK));
        buf.resize(old + want);
        ssize_t r;
        do {
            r = ::read(0, buf.data() + old, want);
        } while (r < 0 && errno == EINTR);
        if (r <= 0) {
            buf.resize(old);
            return false;
        }
        buf.resize(old + static_cast<size_t>(r));
    }
    return true;
}

// ofx_bridge.frame_size(W, H, depth) without overflow: false when it does not fit in 64 bits
bool frame_size64(long long W, long long H, int depth, uint64_t* fs) {
    const uint64_t w = static_cast<uint64_t>(W), h = static_cast<uint64_t>(H);
    uint64_t y, c, n;
    if (__builtin_mul_overflow(w, h, &y) || __builtin_mul_overflow(w / 2, h / 2, &c) || __builtin_mul_overflow(c, 2u, &c) ||
        __builtin_add_overflow(y, c, &n) || __builtin_mul_overflow(n, static_cast<uint64_t>(depth / 8), fs))
        return false;
    return true;
}

}  // namespace

int ofx_main(const QStringList& rest_in, const std::function<void(MainWindow*)>& on_shown) {
    const char* usage = "usage: --ofx W H [FPS] [MAX_FRAMES] [--depth 8|16|32]";
    QStringList rest = rest_in;
    long long depth = 8, W = 0, H = 0, max_frames = 0;
    double fps = 24.0;
    bool ok = true;
    const int di = static_cast<int>(rest.indexOf("--depth"));
    if (di >= 0) {
        ok = di + 1 < rest.size() && py_int(rest[di + 1], &depth);
        rest.erase(rest.begin() + di, rest.begin() + std::min<qsizetype>(di + 2, rest.size()));
    }
    ok = ok && rest.size() >= 2 && py_int(rest[0], &W) && py_int(rest[1], &H);
    if (ok && rest.size() > 2) ok = py_float(rest[2], &fps);
    if (ok && rest.size() > 3) ok = py_int(rest[3], &max_frames);
    if (!ok || (depth != 8 && depth != 16 && depth != 32) || W <= 0 || H <= 0) {
        std::fprintf(stderr, "%s\n", usage);
        return 1;
    }
    // OfxFrameSource.read_all: the frames before the UI starts (the host writes them all upfront).  Sizes the UI cannot
    // hold (W or H beyond int, a frame size beyond 64 bits; N3c verification L-1) cannot be a complete frame: stdin is
    // read to its end and discarded, then "no frames received on stdin" as Python reports for an input shorter than a frame
    const int d = static_cast<int>(depth);
    uint64_t fs = 0;
    if (W > INT_MAX || H > INT_MAX || !frame_size64(W, H, d, &fs)) {
        std::vector<uint8_t> sink(1u << 20);
        ssize_t r;
        while ((r = ::read(0, sink.data(), sink.size())) > 0 || (r < 0 && errno == EINTR)) {
        }
        std::fprintf(stderr, "no frames received on stdin\n");
        return 1;
    }
    const int w = static_cast<int>(W), h = static_cast<int>(H);
    std::vector<Frame> frames;
    std::vector<RestoreMap> restore;       // per frame: the non-finite samples to write back (depth 32)
    std::vector<uint8_t> buf;
    while (read_exact(buf, fs)) {
        if (d == 8) {
            Frame f;
            f.H = h;
            f.W = w;
            f.Hc = h / 2;
            f.Wc = w / 2;
            const size_t ny = static_cast<size_t>(w) * h, nc = static_cast<size_t>(w / 2) * (h / 2);
            f.Y.assign(buf.begin(), buf.begin() + static_cast<long>(ny));
            f.Cb.assign(buf.begin() + static_cast<long>(ny), buf.begin() + static_cast<long>(ny + nc));
            f.Cr.assign(buf.begin() + static_cast<long>(ny + nc), buf.end());
            frames.push_back(std::move(f));
            restore.emplace_back();
        } else {
            RestoreMap rm;
            frames.push_back(frame_from_planes(ofx_unpack(buf.data(), w, h, d, &rm)));
            restore.push_back(std::move(rm));
        }
        if (max_frames && static_cast<long long>(frames.size()) >= max_frames) break;
    }
    if (frames.empty()) {
        std::fprintf(stderr, "no frames received on stdin\n");
        return 1;
    }
    QApplication::setApplicationName("nv_ui_qt (OFX)");
    auto* win = new MainWindow();          // no video path
    win->set_ofx_frames(std::move(frames), w, h, fps, d);
    win->show();

    // (frame, params, profile) of the last frame written to stdout
    auto last = std::make_shared<std::optional<std::string>>();
    auto emit_frame = [win, last, restore, d](const char* why) -> std::string {
        // process the current frame and write it to stdout unless it equals the last written one
        const int t = win->timeline->frame();
        try {
            const PyObj P = win->state->backend();
            const PyObj* eff = win->state->effective();
            const std::string sig = std::to_string(t) + "|" + P.dump(-1, true) + "|" + (eff ? win->profile_sig() : "None");
            if (*last && **last == sig) return "same";
            PyObj prof = eff ? *eff
                             : build_profile(win->frames, t, win->full_range, win->matrix, "auto", nullptr, nullptr);
            auto [idx, planes] = win->engine->window(t, static_cast<int>(P.get("radius").as_int()), std::nullopt);
            const int ci = static_cast<int>(std::find(idx.begin(), idx.end(), t) - idx.begin());
            Planes tmp, rv;
            temporal(planes, ci, prof, P, nullptr, &tmp, &rv);
            const Planes out = finish(planes[static_cast<size_t>(ci)], tmp, rv, prof, P);
            const RestoreMap* rs = t < static_cast<int>(restore.size()) && !restore[static_cast<size_t>(t)].empty()
                                       ? &restore[static_cast<size_t>(t)] : nullptr;
            const std::vector<uint8_t> bytes = ofx_pack(out, d, rs);
            std::fwrite(bytes.data(), 1, bytes.size(), stdout);
            std::fflush(stdout);
            *last = sig;
            return "written";
        } catch (const std::exception& e) {
            std::fprintf(stderr, "OFX emit error (%s): %s\n", why, e.what());
            return "error";
        }
    };
    QObject::connect(win->b_apply, &QPushButton::clicked, win, [win, emit_frame] {
        const int t = win->timeline->frame();
        const std::string r = emit_frame("Apply");
        win->say(r == "written" ? QString("Apply: frame %1 processed and written to the host").arg(t)
                 : r == "same"  ? QString("Apply: frame %1 and the settings are unchanged since the last Apply (not written again)").arg(t)
                                : QString("Apply: the frame could not be processed (see stderr)"));
    });
    auto cancelled = std::make_shared<bool>(false);
    QObject::disconnect(win->b_cancel, &QPushButton::clicked, nullptr, nullptr);
    QObject::connect(win->b_cancel, &QPushButton::clicked, win, [cancelled] { *cancelled = true; });
    QObject::connect(win->b_cancel, &QPushButton::clicked, win, &QWidget::close);
    win->close_hook = [cancelled, emit_frame](QCloseEvent*) {
        if (!*cancelled) emit_frame("close");
    };
    win->after_close_hook = [](QCloseEvent* e) {
        if (e->isAccepted()) QApplication::quit();   // the host waits for the exit
    };
    if (on_shown) QTimer::singleShot(0, win, [win, on_shown] { on_shown(win); });
    const int rc = QApplication::exec();
    delete win;
    return rc;
}

}  // namespace nvqt
