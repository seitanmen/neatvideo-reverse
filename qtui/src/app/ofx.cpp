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
#include <cstdlib>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unistd.h>

#include "depth.h"
#include "mainwindow.h"
#include "pynum.h"
#include "timeline.h"

namespace nvqt {

namespace {

// int() / float() of a Python argument (pynum.h: Unicode digits and spaces, underscores, no 0x prefix, no hex float,
// inf / nan, overflow to inf; N3c verification L-2).  argv arrives as UTF-8 (invalid bytes become U+FFFD here and
// lone surrogates in Python; both fail as '?')
bool arg_int(const QString& s, PyInt* v) { return py_int(s.toStdU32String(), v); }
bool arg_float(const QString& s, double* v) { return py_float(s.toStdU32String(), v); }

using u128 = unsigned __int128;
constexpr u128 U128_SAT = static_cast<u128>(1) << 126;
u128 sat_mul(u128 a, u128 b) { return a != 0 && b > U128_SAT / a ? U128_SAT : std::min(a * b, U128_SAT); }
u128 sat_add(u128 a, u128 b) { return std::min(a + b, U128_SAT); }   // a, b <= 2^126: no wrap

// ofx_bridge.frame_size(W, H, depth) with Python's unbounded ints (exact below 2^126, saturated there)
u128 frame_size_py(u128 W, u128 H, int depth) {
    return sat_mul(sat_add(sat_mul(W, H), sat_mul(2, sat_mul(W / 2, H / 2))), static_cast<u128>(depth / 8));
}

// The first stdin.buffer.read(fs) of ofx_bridge.read_exact, where Python allocates the whole result before reading
// (_bufferedreader_read_generic -> PyBytes_FromStringAndSize(NULL, fs) = malloc(33 + fs)) and raises for a frame size
// it cannot hold (N3c verification I-6): the exception it raises, or nullptr.  The allocation is tried here the same
// way (released at once, never touched), so the bound is this process's malloc as it is Python's.  That bound
// depends on the SDK the executable was linked against (LC_BUILD_VERSION, N3 fixes verification L1): about 2^46 bytes
// for this build (sdk 26), about 1.0507e14 for the anaconda python3.13 (sdk 11.1), so for 2^46 <= fs < ~1.0507e14
// the C++ UI reports MemoryError where the Python UI reads on and reports "no frames received on stdin".
const char* read_exception(u128 fs, const char** message) {
    constexpr u128 SSIZE_MAX_ = static_cast<u128>(INT64_MAX);
    constexpr u128 PYBYTES_HEADER = 33;                 // PyBytesObject_SIZE on 64-bit CPython 3.13
    if (fs > SSIZE_MAX_) {                              // read(n): n is converted to Py_ssize_t
        *message = "cannot fit 'int' into an index-sized integer";
        return "OverflowError";
    }
    if (fs > SSIZE_MAX_ - PYBYTES_HEADER) {             // PyBytes_FromStringAndSize's size check
        *message = "byte string is too large";
        return "OverflowError";
    }
    void* (*volatile alloc)(size_t) = std::malloc;      // volatile: the call is not elided
    void* p = alloc(static_cast<size_t>(fs + PYBYTES_HEADER));
    if (!p) {
        *message = "";
        return "MemoryError";
    }
    std::free(p);
    return nullptr;
}

// the traceback of `python3 -m nv_ui_qt.main --ofx ...` for an exception of the first frame read (paths relative to
// repro/, the runpy frames and 3.13's caret lines left out).  The line numbers and texts are those of repro/nv_ui_qt
// (marked there with a comment; qtui/tools/compare_ofx_args.py compares them with the real Python traceback)
void print_read_exception(const char* type, const char* message) {
    py_print_exception({{"nv_ui_qt/main.py", 157, "<module>", "sys.exit(main())"},
                        {"nv_ui_qt/main.py", 34, "main", "return _ofx_main(args[1:], argv)"},
                        {"nv_ui_qt/main.py", 73, "_ofx_main", "frames = src.read_all()"},
                        {"nv_ui_qt/ofx_bridge.py", 110, "read_all", "data = read_exact(sys.stdin.buffer, fs)"},
                        {"nv_ui_qt/ofx_bridge.py", 39, "read_exact", "chunk = stream.read(n - len(buf))"}},
                       type, message);
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

}  // namespace

Frame ofx_frame(const uint8_t* data, int w, int h, int depth, RestoreMap* restore) {
    if (depth != 8) return frame_from_planes(ofx_unpack(data, w, h, depth, restore));
    Frame f;
    f.H = h;
    f.W = w;
    f.Hc = h / 2;
    f.Wc = w / 2;
    const size_t ny = static_cast<size_t>(w) * h, nc = static_cast<size_t>(w / 2) * (h / 2);
    f.Y.assign(data, data + ny);
    f.Cb.assign(data + ny, data + ny + nc);
    f.Cr.assign(data + ny + nc, data + ny + 2 * nc);
    return f;
}

std::vector<uint8_t> ofx_process(const std::vector<Frame>& frames, int t, const PyObj& P, const PyObj* eff,
                                 bool full_range, const std::string& matrix, const PreviewEngine& engine, int depth,
                                 const RestoreMap* restore) {
    const PyObj prof = eff ? *eff : build_profile(frames, t, full_range, matrix, "auto", nullptr, nullptr);
    auto [idx, planes] = engine.window(t, static_cast<int>(P.get("radius").as_int()), std::nullopt);
    const int ci = static_cast<int>(std::find(idx.begin(), idx.end(), t) - idx.begin());
    Planes tmp, rv;
    temporal(planes, ci, prof, P, nullptr, &tmp, &rv);
    const Planes out = finish(planes[static_cast<size_t>(ci)], tmp, rv, prof, P);
    return ofx_pack(out, depth, restore);
}

int ofx_main(const QStringList& rest_in, const std::function<void(MainWindow*)>& on_shown) {
    const char* usage = "usage: --ofx W H [FPS] [MAX_FRAMES] [--depth 8|16|32]";
    QStringList rest = rest_in;
    PyInt depth_i, W_i, H_i, max_i;
    depth_i.mag = 8;
    double fps = 24.0;
    bool ok = true;
    const int di = static_cast<int>(rest.indexOf("--depth"));
    if (di >= 0) {
        ok = di + 1 < rest.size() && arg_int(rest[di + 1], &depth_i);
        rest.erase(rest.begin() + di, rest.begin() + std::min<qsizetype>(di + 2, rest.size()));
    }
    ok = ok && rest.size() >= 2 && arg_int(rest[0], &W_i) && arg_int(rest[1], &H_i);
    if (ok && rest.size() > 2) ok = arg_float(rest[2], &fps);
    if (ok && rest.size() > 3) ok = arg_int(rest[3], &max_i);
    const long long depth = depth_i.clamp_ll(), max_frames = max_i.clamp_ll();
    if (!ok || (depth != 8 && depth != 16 && depth != 32) || !W_i.positive() || !H_i.positive()) {
        std::fprintf(stderr, "%s\n", usage);
        return 1;
    }
    const int d = static_cast<int>(depth);
    // OfxFrameSource.read_all: the frames before the UI starts (the host writes them all upfront).  A frame size Python
    // cannot read raises before stdin is read (traceback, exit 1).  Sizes the UI cannot hold otherwise (W or H beyond
    // int; N3c verification L-1) cannot be a complete frame: stdin is read to its end and discarded, then "no frames
    // received on stdin" as Python reports for an input shorter than a frame
    const u128 fs128 = frame_size_py(W_i.mag, H_i.mag, d);
    const char* msg = nullptr;
    if (const char* exc = read_exception(fs128, &msg)) {
        print_read_exception(exc, msg);
        return 1;
    }
    const uint64_t fs = static_cast<uint64_t>(fs128);
    const long long W = W_i.clamp_ll(), H = H_i.clamp_ll();
    if (W > INT_MAX || H > INT_MAX) {
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
        RestoreMap rm;
        frames.push_back(ofx_frame(buf.data(), w, h, d, &rm));
        restore.push_back(std::move(rm));
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
            const RestoreMap* rs = t < static_cast<int>(restore.size()) && !restore[static_cast<size_t>(t)].empty()
                                       ? &restore[static_cast<size_t>(t)] : nullptr;
            const std::vector<uint8_t> bytes =
                ofx_process(win->frames, t, P, eff, win->full_range, win->matrix, *win->engine, d, rs);
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
