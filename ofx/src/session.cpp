// session - see session.h.
#include "session.h"

#include <dlfcn.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "modal_wait.h"
#include "pyobj.h"

extern char** environ;

namespace mvofx {

using nvqt::PyObj;

namespace {

// the Mach-O image this code is in (Contents/MacOS/MeatVideo.ofx of the bundle)
std::string own_image() {
    Dl_info info;
    if (dladdr(reinterpret_cast<const void*>(&own_image), &info) == 0 || !info.dli_fname) return "";
    return info.dli_fname;
}

// variables the helper must not inherit from the host: the dynamic loader's and Qt's plug-in / import paths
bool dropped(const char* kv) {
    static const char* const PREFIXES[] = {"DYLD_", "QT_PLUGIN_PATH=", "QT_QPA_PLATFORM_PLUGIN_PATH=", "QML2_IMPORT_PATH=",
                                           "QML_IMPORT_PATH=", "QT_CONF_PATH="};
    for (const char* p : PREFIXES)
        if (std::strncmp(kv, p, std::strlen(p)) == 0) return true;
    return false;
}

bool write_all(int fd, const void* data, size_t n) {
    const char* p = static_cast<const char*>(data);
    while (n > 0) {
        const ssize_t w = ::write(fd, p, n > (1u << 20) ? (1u << 20) : n);
        if (w < 0 && errno == EINTR) continue;
        if (w <= 0) return false;
        p += w;
        n -= static_cast<size_t>(w);
    }
    return true;
}

std::string header(const SessionRequest& rq) {
    PyObj h = PyObj::dict();
    h.set("protocol", PyObj::str("meatvideo-ofx-session"));
    h.set("version", PyObj::integer(1));
    h.set("page", PyObj::str(rq.page));
    h.set("width", PyObj::integer(rq.W));
    h.set("height", PyObj::integer(rq.H));
    h.set("fps", PyObj::real(rq.fps));
    h.set("matrix", PyObj::str(rq.matrix));
    h.set("format", PyObj::str("gbrpf32le"));
    h.set("times", PyObj::reals(rq.times));
    h.set("current", PyObj::integer(rq.current));
    h.set("clip_params", PyObj::str(rq.clip_params));
    if (rq.field_order >= 0) h.set("field_order", PyObj::str(rq.field_order == 0 ? "tff" : "bff"));
    return h.dump() + "\n";
}

// the helper's reply: "APPLY <n>\n" + n bytes, or "CANCEL\n"
SessionResult parse_reply(const std::string& out) {
    SessionResult r;
    if (out == "CANCEL\n") {
        r.kind = SessionResult::CANCEL;
        return r;
    }
    const size_t nl = out.find('\n');
    if (out.compare(0, 6, "APPLY ") == 0 && nl != std::string::npos && nl > 6 && nl - 6 <= 12) {
        unsigned long long n = 0;
        bool ok = true;
        for (size_t i = 6; i < nl; ++i) {
            if (out[i] < '0' || out[i] > '9') ok = false;
            n = n * 10 + static_cast<unsigned>(out[i] - '0');
        }
        if (ok && out.size() - nl - 1 == n) {
            r.kind = SessionResult::APPLY;
            r.clip_params = out.substr(nl + 1);
            return r;
        }
    }
    r.error = "the window returned an unexpected reply (" + std::to_string(out.size()) + " bytes)";
    return r;
}

}  // namespace

std::string helper_path() {
    if (const char* e = std::getenv("MEATVIDEO_OFX_HELPER"))
        if (*e) return e;
    std::string img = own_image();       // .../MeatVideo.ofx.bundle/Contents/MacOS/MeatVideo.ofx
    for (int k = 0; k < 2; ++k) {
        const size_t s = img.rfind('/');
        if (s == std::string::npos) return "";
        img.resize(s);
    }
    return img + "/Helpers/MeatVideoQt.app/Contents/MacOS/MeatVideoQt";
}

namespace {

// "CANCEL\n", or "<WORD> <n>[ ..]\n" + n bytes (APPLY / DEFAULTS) received in full
bool reply_complete(const std::string& r) {
    if (r == "CANCEL\n") return true;
    const size_t nl = r.find('\n');
    if (nl == std::string::npos || nl > 64) return false;
    const size_t sp = r.find(' ');
    if (sp == std::string::npos || sp > nl) return false;
    unsigned long long n = 0;
    size_t i = sp + 1;
    for (; i < nl && r[i] >= '0' && r[i] <= '9' && i - sp <= 12; ++i) n = n * 10 + static_cast<unsigned>(r[i] - '0');
    return i > sp + 1 && r.size() - nl - 1 >= n;
}

// the helper run with `arg`: `head` and the frames written to its stdin, its stdout read until it exits; "" = it
// exited 0 (the reply in *reply), else why not
std::string run_helper(const char* arg, const std::string& head, const std::vector<std::vector<float>>& frames,
                       std::string* reply) {
    const std::string exe = helper_path();
    if (exe.empty()) return "the window helper (MeatVideoQt) was not found next to the plug-in";
    int in[2] = {-1, -1}, out[2] = {-1, -1};
    if (::pipe(in) != 0 || ::pipe(out) != 0) {
        for (int fd : {in[0], in[1], out[0], out[1]})
            if (fd >= 0) ::close(fd);
        return std::string("pipe: ") + std::strerror(errno);
    }
    // close-on-exec at once, and at 3 or above: a process the host starts meanwhile must not keep the pipes (macOS
    // has no pipe2; the reply is also read only up to its own length, so a stray holder of the write end cannot make
    // the read wait), and a pipe end the kernel put on a closed 0 / 1 / 2 of the host must still be dup2'ed onto the
    // child's 0 / 1 (dup2 of an fd onto itself would keep its close-on-exec and the child would start without it)
    for (int* fd : {&in[0], &in[1], &out[0], &out[1]}) {
        const int moved = ::fcntl(*fd, F_DUPFD_CLOEXEC, 3);
        if (moved < 0) {
            const std::string e = std::strerror(errno);
            for (int f : {in[0], in[1], out[0], out[1]})
                if (f >= 0) ::close(f);
            return "fcntl: " + e;
        }
        ::close(*fd);
        *fd = moved;
    }
    posix_spawn_file_actions_t fa;
    posix_spawnattr_t at;
    posix_spawn_file_actions_init(&fa);
    posix_spawnattr_init(&at);
    posix_spawn_file_actions_adddup2(&fa, in[0], 0);
    posix_spawn_file_actions_adddup2(&fa, out[1], 1);
    if (::fcntl(2, F_GETFD) != -1) posix_spawn_file_actions_addinherit_np(&fa, 2);   // the host's stderr, if open
    sigset_t none, all;
    sigemptyset(&none);
    sigfillset(&all);
    posix_spawnattr_setsigmask(&at, &none);
    posix_spawnattr_setsigdefault(&at, &all);
    posix_spawnattr_setflags(&at, POSIX_SPAWN_CLOEXEC_DEFAULT | POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF);
    std::vector<char*> env;
    for (char** e = environ; e && *e; ++e)
        if (!dropped(*e)) env.push_back(*e);
    env.push_back(nullptr);
    std::string arg0 = exe, arg1 = arg;
    char* argv[] = {arg0.data(), arg1.data(), nullptr};
    pid_t pid = -1;
    const int sp = posix_spawn(&pid, exe.c_str(), &fa, &at, argv, env.data());
    posix_spawn_file_actions_destroy(&fa);
    posix_spawnattr_destroy(&at);
    ::close(in[0]);
    ::close(out[1]);
    if (sp != 0) {
        ::close(in[1]);
        ::close(out[0]);
        return "cannot start the window helper " + exe + ": " + std::strerror(sp);
    }
    // the request; a helper that exits early makes the writes fail (EPIPE, no SIGPIPE on this descriptor)
    fcntl(in[1], F_SETNOSIGPIPE, 1);
    // on the main thread of a running Cocoa host the waits below serve its events (modal_wait.h); else they block.
    // The request is written by blocking writes in large pieces - on a worker thread while the events are served
    bool sent = true;
    ModalWait mw(pid);
    mw.run([&] {
        sent = write_all(in[1], head.data(), head.size());
        for (const std::vector<float>& f : frames)
            if (sent) sent = write_all(in[1], f.data(), f.size() * sizeof(float));
        ::close(in[1]);
    });
    // the reply: the window writes it when it closes; read until EOF or until it is complete
    char buf[65536];
    while (!reply_complete(*reply)) {
        mw.wait_fd(out[0], POLLIN);                     // (active: until readable; the read then does not block)
        const ssize_t r = ::read(out[0], buf, sizeof buf);
        if (r < 0 && errno == EINTR) continue;
        if (r <= 0) break;
        reply->append(buf, static_cast<size_t>(r));
    }
    ::close(out[0]);
    const int status = mw.wait_child(pid);
    if (WIFSIGNALED(status)) return "the window helper was terminated by signal " + std::to_string(WTERMSIG(status));
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return "the window helper exited with status " + std::to_string(WIFEXITED(status) ? WEXITSTATUS(status) : -1) +
               (sent ? "" : " before it read the request");
    return "";
}

}  // namespace

SessionResult run_session(const SessionRequest& rq) {
    std::string reply;
    const std::string err = run_helper("--ofx-session", header(rq), rq.frames, &reply);
    if (!err.empty()) {
        SessionResult res;
        res.error = err;
        return res;
    }
    return parse_reply(reply);
}

DefaultsResult request_defaults() {
    // "DEFAULTS <n> <profile> <preset>\n" + n bytes (qtui/src/app/ofx_session.cpp ofx_default_main)
    DefaultsResult d;
    std::string out;
    d.error = run_helper("--ofx-default", "", {}, &out);
    if (!d.error.empty()) return d;
    const size_t nl = out.find('\n');
    char p[16] = {0}, q[16] = {0};
    unsigned long long n = 0;
    if (nl == std::string::npos || nl > 64 ||
        std::sscanf(out.substr(0, nl).c_str(), "DEFAULTS %llu %15s %15s", &n, p, q) != 3 || out.size() - nl - 1 != n) {
        d.error = "the window helper returned an unexpected reply for the defaults (" + std::to_string(out.size()) +
                  " bytes)";
        return d;
    }
    auto st = [](const char* v) {
        return std::strcmp(v, "ok") == 0 ? DefaultsResult::OK : std::strcmp(v, "error") == 0 ? DefaultsResult::ERROR
                                                                                            : DefaultsResult::NONE;
    };
    d.profile = st(p);
    d.preset = st(q);
    d.clip_params = out.substr(nl + 1);
    return d;
}

}  // namespace mvofx
