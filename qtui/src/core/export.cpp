// export - see export.h (port of repro/nv_ui_qt/export.py + repro/denoise_video_v5.py main() and the IO
// conversions of repro/nv_depth.py).  Child processes (ffmpeg / ffprobe) are spawned with posix_spawn and
// plain pipes so that the raw-video reads and writes block exactly as the Python subprocess pipes do; the
// argument lists are the Python ones word for word (NV_EXPORT_TRACE=<file> appends every spawned command
// as a JSON list, qtui/tools/compare_export.py compares them with the Python side).
#include "export.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QThread>

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <regex>
#include <set>
#include <sstream>
#include <thread>

#include "nvcore.h"

extern char** environ;

namespace nvqt {

namespace {

// ------------------------------------------------------------------ Python formatting helpers
std::string fmt6(double v) {
    char b[64];
    std::snprintf(b, sizeof(b), "%.6f", v);
    return b;
}
std::string fmt0(double v) {
    char b[64];
    std::snprintf(b, sizeof(b), "%.0f", v);
    return b;
}
std::string fmt1(double v) {
    char b[64];
    std::snprintf(b, sizeof(b), "%.1f", v);
    return b;
}
std::string lower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
std::string ext_of(const std::string& path) {    // os.path.splitext(path)[1].lower()
    const std::string b = QFileInfo(QString::fromStdString(path)).fileName().toStdString();
    const auto p = b.rfind('.');
    if (p == std::string::npos || p == 0) return "";
    return lower(b.substr(p));
}
std::string stem_of(const std::string& path) {   // os.path.splitext(path)[0]
    const std::string e = ext_of(path);
    return path.substr(0, path.size() - e.size());
}
// repr() of a Python str (single quotes, as the tags dict prints; no escapes needed for ffprobe names)
std::string pyrepr(const std::string& s) { return "'" + s + "'"; }

// ------------------------------------------------------------------ child processes
thread_local const std::atomic<bool>* tl_cancel = nullptr;   // the export's cancel flag (child registry key)
std::mutex reg_mu;
std::map<const std::atomic<bool>*, std::set<pid_t>> reg;

void reg_add(pid_t p) {
    if (!tl_cancel) return;
    std::lock_guard<std::mutex> lk(reg_mu);
    reg[tl_cancel].insert(p);
}
void reg_del(pid_t p) {
    if (!tl_cancel) return;
    std::lock_guard<std::mutex> lk(reg_mu);
    reg[tl_cancel].erase(p);
}
void reg_kill(const std::atomic<bool>* c) {
    std::lock_guard<std::mutex> lk(reg_mu);
    for (pid_t p : reg[c]) ::kill(p, SIGKILL);
}

void trace(const std::vector<std::string>& args) {
    const char* t = std::getenv("NV_EXPORT_TRACE");
    if (!t || !*t) return;
    static std::mutex mu;
    std::lock_guard<std::mutex> lk(mu);
    PyObj l = PyObj::list();
    for (const auto& a : args) l.append(PyObj::str(a));
    std::ofstream f(t, std::ios::app);
    f << l.dump() << "\n";
}

struct Child {
    pid_t pid = -1;
    int in = -1, out = -1, err = -1;      // the parent's ends
};

// posix_spawnp of args (args[0] = the program); stdin: a pipe (pipe_in) or /dev/null; stdout / stderr: pipes or
// inherited.  Throws when the process cannot be started.
Child spawn(const std::vector<std::string>& args, bool pipe_in, bool pipe_out, bool pipe_err) {
    trace(args);
    int pin[2] = {-1, -1}, pout[2] = {-1, -1}, perr[2] = {-1, -1};
    // close_fds=True of subprocess (N3b verification L5): the pipe ends are close-on-exec (no other child inherits
    // them) and the child gets only 0 / 1 / 2 (POSIX_SPAWN_CLOEXEC_DEFAULT; the dup2 targets and the inherited
    // stdout / stderr are kept by the file actions)
    auto mkpipe = [](int p[2]) {
        if (::pipe(p)) throw std::runtime_error("pipe");
        ::fcntl(p[0], F_SETFD, FD_CLOEXEC);
        ::fcntl(p[1], F_SETFD, FD_CLOEXEC);
    };
    if (pipe_in) mkpipe(pin);
    if (pipe_out) mkpipe(pout);
    if (pipe_err) mkpipe(perr);
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);
    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);
    posix_spawnattr_setflags(&attr, POSIX_SPAWN_CLOEXEC_DEFAULT);
    if (pipe_in) {
        posix_spawn_file_actions_adddup2(&fa, pin[0], 0);
        posix_spawn_file_actions_addclose(&fa, pin[0]);
        posix_spawn_file_actions_addclose(&fa, pin[1]);
    } else {
        posix_spawn_file_actions_addopen(&fa, 0, "/dev/null", O_RDONLY, 0);
    }
    if (pipe_out) {
        posix_spawn_file_actions_adddup2(&fa, pout[1], 1);
        posix_spawn_file_actions_addclose(&fa, pout[0]);
        posix_spawn_file_actions_addclose(&fa, pout[1]);
    } else {
        posix_spawn_file_actions_addinherit_np(&fa, 1);
    }
    if (pipe_err) {
        posix_spawn_file_actions_adddup2(&fa, perr[1], 2);
        posix_spawn_file_actions_addclose(&fa, perr[0]);
        posix_spawn_file_actions_addclose(&fa, perr[1]);
    } else {
        posix_spawn_file_actions_addinherit_np(&fa, 2);
    }
    std::vector<char*> av;
    for (const auto& a : args) av.push_back(const_cast<char*>(a.c_str()));
    av.push_back(nullptr);
    pid_t pid = -1;
    const int rc = posix_spawnp(&pid, av[0], &fa, &attr, av.data(), environ);
    posix_spawn_file_actions_destroy(&fa);
    posix_spawnattr_destroy(&attr);
    if (pipe_in) ::close(pin[0]);
    if (pipe_out) ::close(pout[1]);
    if (pipe_err) ::close(perr[1]);
    if (rc != 0) {
        if (pipe_in) ::close(pin[1]);
        if (pipe_out) ::close(pout[0]);
        if (pipe_err) ::close(perr[0]);
        throw std::runtime_error("cannot start " + args[0] + ": " + std::strerror(rc));
    }
    Child c;
    c.pid = pid;
    c.in = pipe_in ? pin[1] : -1;
    c.out = pipe_out ? pout[0] : -1;
    c.err = pipe_err ? perr[0] : -1;
    reg_add(pid);
    return c;
}

// waitpid -> the Python returncode (exit status, or -signal)
int wait_child(Child& c) {
    if (c.pid < 0) return -1;
    int st = 0;
    while (::waitpid(c.pid, &st, 0) < 0 && errno == EINTR) {
    }
    reg_del(c.pid);
    c.pid = -1;
    if (WIFEXITED(st)) return WEXITSTATUS(st);
    if (WIFSIGNALED(st)) return -WTERMSIG(st);
    return -1;
}

void kill_child(Child& c) {
    if (c.pid >= 0) ::kill(c.pid, SIGKILL);
}

// subprocess.run(args, capture_output=True): stdout / stderr text and the return code
int run_capture(const std::vector<std::string>& args, std::string* out, std::string* err) {
    Child c = spawn(args, false, true, true);
    std::string so, se;
    struct pollfd pf[2] = {{c.out, POLLIN, 0}, {c.err, POLLIN, 0}};
    int open = 2;
    char buf[65536];
    while (open > 0) {
        if (::poll(pf, 2, -1) < 0) {
            if (errno == EINTR) continue;
            break;
        }
        for (int i = 0; i < 2; ++i) {
            if (pf[i].fd < 0 || !(pf[i].revents & (POLLIN | POLLHUP | POLLERR))) continue;
            const ssize_t n = ::read(pf[i].fd, buf, sizeof(buf));
            if (n > 0) {
                (i == 0 ? so : se).append(buf, static_cast<size_t>(n));
            } else {
                ::close(pf[i].fd);
                pf[i].fd = -1;
                --open;
            }
        }
    }
    const int code = wait_child(c);
    if (out) *out = so;
    if (err) *err = se;
    return code;
}

bool write_all(int fd, const uint8_t* d, size_t n) {
    while (n > 0) {
        const ssize_t w = ::write(fd, d, n);
        if (w < 0) {
            if (errno == EINTR) continue;
            return false;                    // EPIPE: the encoder stopped (BrokenPipeError)
        }
        d += w;
        n -= static_cast<size_t>(w);
    }
    return true;
}

// a frame-by-frame reader of ffmpeg's raw-video output (read_frames' Popen)
struct RawReader {
    Child c;
    long long fs = 0;
    std::vector<uint8_t> buf;
    bool read_frame() {
        buf.resize(static_cast<size_t>(fs));
        size_t got = 0;
        while (got < buf.size()) {
            const ssize_t n = ::read(c.out, buf.data() + got, buf.size() - got);
            if (n < 0) {
                if (errno == EINTR) continue;
                break;
            }
            if (n == 0) break;
            got += static_cast<size_t>(n);
        }
        return got == buf.size();
    }
    void close() {
        if (c.out >= 0) ::close(c.out);
        c.out = -1;
        if (c.pid >= 0) {
            int st = 0;
            if (::waitpid(c.pid, &st, WNOHANG) == 0) kill_child(c);   // p.poll() is None -> p.kill()
            wait_child(c);
        }
    }
};

// ------------------------------------------------------------------ Python's probe helpers
double num(const PyObj& v, double def, bool* ok = nullptr) {
    if (ok) *ok = true;
    if (v.type() == PyObj::STR) {
        try {
            size_t used = 0;
            const double d = std::stod(v.as_str(), &used);
            return d;
        } catch (...) {
            if (ok) *ok = false;
            return def;
        }
    }
    if (v.is_num()) return v.as_double();
    if (ok) *ok = false;
    return def;
}

const std::set<std::string> TS_DISCONT = {"mpegts", "mpegtsraw", "mpeg", "hls"};
const std::set<std::string> MP4_AUDIO = {"aac", "alac", "mp3", "ac3", "eac3", "pcm_s16le", "pcm_s24le", "pcm_s32le",
                                         "pcm_f32le"};
const std::set<std::string> MOV_AUDIO = {"aac", "alac", "mp3", "ac3", "eac3", "pcm_s16le", "pcm_s24le", "pcm_s32le",
                                         "pcm_f32le", "mp2", "pcm_s16be", "pcm_s24be", "pcm_s32be", "pcm_f32be", "pcm_u8"};
const std::map<int, std::string> FLAT_STATUS = {{0, "ok"},
                                                {1, "no square found (fallback)"},
                                                {2, "small square (< 128 px)"},
                                                {3, "not flat (details in the square)"},
                                                {4, "clipped pixels in the square"},
                                                {5, "cancelled (no profile built)"}};

std::vector<std::string> split_commas(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == ',') {
            out.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    out.push_back(cur);
    return out;
}

double fps_of(const std::string& rate) {   // _fps: float(num) / float(den)
    const auto s = rate.find('/');
    const double a = std::stod(rate.substr(0, s));
    const double b = s == std::string::npos ? 1.0 : std::stod(rate.substr(s + 1));
    return a / b;
}

// ------------------------------------------------------------------ ffmpeg command pieces
std::vector<std::string> vcodec(const PixFmt& fmt, const std::string& codec, int crf) {   // _vcodec
    if (codec == "x264") return {"libx264", "-crf", std::to_string(crf)};
    if (codec == "x265") return {"libx265", "-crf", std::to_string(crf), "-x265-params", "log-level=error"};
    if (codec == "ffv1") {
        if (fmt.is_float) return {"ffv1", "-level", "4", "-strict", "experimental"};
        return {"ffv1", "-level", "3"};
    }
    if (codec == "prores") return {"prores_ks", "-profile:v", fmt.chroma == "444" ? "4" : "3"};
    return {"exr"};
}

// denoise_video_v5.trial_encode: nullopt or ffmpeg's first error line
std::optional<std::string> trial_encode(const std::string& dst, const PixFmt& fmt, const std::string& codec, int crf) {
    const std::string ext = ext_of(dst);
    std::vector<std::string> vc = (fmt.legacy() && codec == "x264") ? std::vector<std::string>{"libx264", "-crf", std::to_string(crf)}
                                                                  : vcodec(fmt, codec, crf);
    QTemporaryDir td;
    const std::string out = QDir(td.path()).filePath(QString::fromStdString(codec == "exr" ? "trial_%05d.exr" : "trial" + ext)).toStdString();
    std::vector<std::string> args = {tool_path("ffmpeg"), "-v", "error", "-nostdin", "-f", "lavfi", "-i",
                                     "color=c=gray:s=64x64:r=24", "-frames:v", "1", "-c:v"};
    args.insert(args.end(), vc.begin(), vc.end());
    args.insert(args.end(), {"-pix_fmt", fmt.name});
    if (codec == "exr") args.insert(args.end(), {"-f", "image2"});
    args.push_back(out);
    std::string so, se;
    const int code = run_capture(args, &so, &se);
    if (code == 0) return std::nullopt;
    std::istringstream ss(se);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.find_first_not_of(" \t\r") != std::string::npos) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
            return line;
        }
    }
    return "ffmpeg exit " + std::to_string(code);
}

using Plan = std::vector<std::pair<int, std::string>>;

// denoise_video_v5.audio_plan
Plan audio_plan(const std::string& src, const std::string& dst, const ProbeInfo& info, const std::string& mode,
                const std::function<void(const std::string&)>& print) {
    Plan plan;
    if (mode == "none" || info.audio.empty()) return plan;
    std::string ext = ext_of(dst);
    if (ext.empty()) ext = ".mp4";
    const std::set<std::string>* safe = (ext == ".mp4" || ext == ".m4v") ? &MP4_AUDIO : ext == ".mov" ? &MOV_AUDIO : nullptr;
    QTemporaryDir td;
    for (size_t k = 0; k < info.audio.size(); ++k) {
        const auto& a = info.audio[k];
        std::vector<std::string> tries = mode == "copy" ? std::vector<std::string>{"copy", "aac"} : std::vector<std::string>{"aac"};
        if (safe && !safe->count(a.codec)) tries = {"aac"};
        bool okk = false;
        for (const auto& c : tries) {
            std::vector<std::string> args = {tool_path("ffmpeg"), "-v", "error", "-nostdin", "-i", src, "-map",
                                             "0:a:" + std::to_string(k), "-c:a", c};
            if (c == "aac") args.insert(args.end(), {"-b:a", "256k"});
            args.insert(args.end(), {"-t", "0.5", "-y",
                                     QDir(td.path()).filePath(QString::fromStdString("trial" + std::to_string(k) + ext)).toStdString()});
            std::string so, se;
            if (run_capture(args, &so, &se) == 0) {
                plan.emplace_back(static_cast<int>(k), c);
                okk = true;
                break;
            }
        }
        if (!okk)
            print("warning: audio stream " + std::to_string(k) + " (" + a.codec + ") can be neither copied nor re-encoded - dropped");
    }
    return plan;
}

// denoise_video_v5.encoder: the ffmpeg command (plan may be emptied when the range holds no audio)
std::vector<std::string> encoder_args(const std::string& dst, const ProbeInfo& info, int crf, Plan& plan, const std::string& src,
                                      double start, std::optional<double> dur, const PixFmt& fmt, const std::string& codec,
                                      const std::function<void(const std::string&)>& print) {
    const bool legacy = fmt.legacy() && codec == "x264";
    std::vector<std::pair<std::string, std::string>> keys = {{"color_space", "-colorspace"},
                                                             {"color_primaries", "-color_primaries"},
                                                             {"color_transfer", "-color_trc"},
                                                             {"color_range", "-color_range"}};
    if (!legacy && fmt.family == "rgb") keys = {keys[1], keys[2]};
    std::vector<std::string> tags;
    for (const auto& [key, flag] : keys) {
        auto it = info.v.tags.find(key);
        if (it != info.v.tags.end() && !it->second.empty() && it->second != "unknown") {
            tags.push_back(flag);
            tags.push_back(it->second);
        }
    }
    std::vector<std::string> rot;
    if (info.v.rotation) rot = {"-display_rotation", std::to_string(info.v.rotation), "-noautorotate"};
    std::vector<std::string> crop;
    const bool mp4ish = codec == "x264" || codec == "x265" || codec == "prores";
    if (legacy || (mp4ish && fmt.chroma == "420")) {
        if (info.v.W % 2 || info.v.H % 2) {
            crop = {"-vf", "crop=trunc(iw/2)*2:trunc(ih/2)*2"};
            print("note: odd frame size - the last row/column is dropped on encode");
        }
    } else if (mp4ish && fmt.chroma == "422" && info.v.W % 2) {
        crop = {"-vf", "crop=trunc(iw/2)*2:ih"};
        print("note: odd frame width - the last column is dropped on encode");
    }
    std::vector<std::string> a_in, a_out;
    if (!plan.empty()) {
        const double cut = info.v0 + start;
        const bool ranged = start > 0 || dur.has_value();
        const double ss = ranged ? std::max(cut - info.fs, 0.0) : 0.0;
        double base = info.fs + ss;
        std::vector<double> starts;
        for (const auto& [k, c] : plan)
            if (info.audio[static_cast<size_t>(k)].start) starts.push_back(*info.audio[static_cast<size_t>(k)].start);
        if (info.discont && !starts.empty()) base = std::max(base, *std::min_element(starts.begin(), starts.end()));
        const double off = base - cut;
        if (dur && *dur - std::max(off, 0.0) <= 0) {
            plan.clear();
        } else {
            if (ranged) a_in.insert(a_in.end(), {"-ss", fmt6(ss)});
            if (std::fabs(off) > 1e-4) a_in.insert(a_in.end(), {"-itsoffset", fmt6(off)});
            if (dur) a_in.insert(a_in.end(), {"-t", fmt6(*dur)});
            a_in.insert(a_in.end(), {"-i", src});
            bool any_copy = false;
            for (size_t j = 0; j < plan.size(); ++j) {
                const auto& [k, c] = plan[j];
                a_out.insert(a_out.end(), {"-map", "1:a:" + std::to_string(k), "-c:a:" + std::to_string(j), c});
                if (c == "aac") a_out.insert(a_out.end(), {"-b:a:" + std::to_string(j), "256k"});
                any_copy = any_copy || c == "copy";
            }
            if (ranged && any_copy) a_out.insert(a_out.end(), {"-copypriorss:a", "0"});
        }
    }
    const std::string size = std::to_string(info.v.W) + "x" + std::to_string(info.v.H);
    std::vector<std::string> cmd;
    if (legacy) {
        cmd = {tool_path("ffmpeg"), "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", "yuv420p", "-s", size, "-r", info.v.fps};
        cmd.insert(cmd.end(), tags.begin(), tags.end());
        cmd.insert(cmd.end(), rot.begin(), rot.end());
        cmd.insert(cmd.end(), {"-i", "-"});
        cmd.insert(cmd.end(), a_in.begin(), a_in.end());
        cmd.insert(cmd.end(), crop.begin(), crop.end());
        cmd.insert(cmd.end(), {"-map", "0:v:0", "-c:v", "libx264", "-crf", std::to_string(crf), "-pix_fmt", "yuv420p"});
        cmd.insert(cmd.end(), tags.begin(), tags.end());
        cmd.insert(cmd.end(), a_out.begin(), a_out.end());
        cmd.insert(cmd.end(), {"-movflags", "+faststart", dst});
        return cmd;
    }
    const std::vector<std::string> vc = vcodec(fmt, codec, crf);
    cmd = {tool_path("ffmpeg"), "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", fmt.name, "-s", size, "-r", info.v.fps};
    cmd.insert(cmd.end(), tags.begin(), tags.end());
    cmd.insert(cmd.end(), rot.begin(), rot.end());
    cmd.insert(cmd.end(), {"-i", "-"});
    cmd.insert(cmd.end(), a_in.begin(), a_in.end());
    cmd.insert(cmd.end(), crop.begin(), crop.end());
    cmd.insert(cmd.end(), {"-map", "0:v:0", "-c:v"});
    cmd.insert(cmd.end(), vc.begin(), vc.end());
    cmd.insert(cmd.end(), {"-pix_fmt", fmt.name});
    cmd.insert(cmd.end(), tags.begin(), tags.end());
    cmd.insert(cmd.end(), a_out.begin(), a_out.end());
    const std::string ext = ext_of(dst);
    if (codec == "exr")
        cmd.insert(cmd.end(), {"-f", "image2"});
    else if (ext == ".mp4" || ext == ".mov" || ext == ".m4v")
        cmd.insert(cmd.end(), {"-movflags", "+faststart"});
    cmd.push_back(dst);
    return cmd;
}

// read_frames' ffmpeg command (fmt: the raw pix_fmt; hi < 0 = to the end; vf: the extra filter or "")
std::vector<std::string> reader_args(const std::string& path, int lo, int hi, const std::string& fps, const std::string& pix_fmt,
                                     const std::string& vf) {
    std::vector<std::string> chain;
    if (!fps.empty()) {
        chain.push_back("setpts=PTS-STARTPTS");
        chain.push_back("fps=" + fps);
    }
    if (lo > 0 || hi >= 0)
        chain.push_back(hi >= 0 ? "select=between(n\\," + std::to_string(lo) + "\\," + std::to_string(hi - 1) + ")"
                                : "select=gte(n\\," + std::to_string(lo) + ")");
    if (!vf.empty()) chain.push_back(vf);
    std::vector<std::string> args = {tool_path("ffmpeg"), "-v", "error", "-nostdin", "-noautorotate", "-i", path};
    if (!chain.empty()) {
        std::string joined;
        for (size_t i = 0; i < chain.size(); ++i) joined += (i ? "," : "") + chain[i];
        args.insert(args.end(), {"-vf", joined});
    }
    if (hi >= 0) args.insert(args.end(), {"-frames:v", std::to_string(hi - lo)});
    args.insert(args.end(), {"-fps_mode", "passthrough", "-f", "rawvideo", "-pix_fmt", pix_fmt, "-"});
    return args;
}

// nv_depth.read_filter
std::string read_filter(const ProbeInfo& info, const PixFmt& fmt, const std::string& matrix) {
    if (fmt.family != "rgb" || source_format(info.v.pix_fmt).family == "rgb") return "";
    return "scale=in_color_matrix=" + matrix + ":in_range=" + (info.v.full_range ? "pc" : "tv");
}

// nvfilter.v5_params(**kw)
PyObj v5_params(const PyObj& kw) {
    PyObj P = PyObj::dict();
    P.set("radius", PyObj::integer(2));
    P.set("temporal", PyObj::real(1.0));
    P.set("threshold", PyObj::real(4.0));
    P.set("spatial", PyObj::real(2.0));
    P.set("luma", PyObj::real(0.7));
    P.set("chroma", PyObj::real(1.0));
    PyObj ba = PyObj::dict(), bn = PyObj::dict();
    for (const char* b : {"high", "medium", "low", "very-low", "ultra-low"}) {
        ba.set(b, PyObj::real(std::string(b) == "ultra-low" ? 0.0 : 1.0));
        bn.set(b, PyObj::real(1.0));
    }
    P.set("band_amounts", ba);
    P.set("band_noise", bn);
    P.set("noise_level", PyObj::real(1.0));
    PyObj cn = PyObj::dict();
    for (const char* c : {"Y", "Cb", "Cr"}) cn.set(c, PyObj::real(1.0));
    P.set("channel_noise", cn);
    P.set("similarity", PyObj::str("v5"));
    P.set("pyramid_scale", PyObj::real(1.5));
    P.set("bs", PyObj::integer(16));
    P.set("search", PyObj::integer(16));
    P.set("mix", PyObj::real(0.0));
    for (const auto& k : kw.keys()) {
        const PyObj& v = kw.get(k);
        if (v.is_none()) continue;
        if (v.is_dict() && P.has(k) && P.get(k).is_dict()) {
            PyObj& d = P.at(k);
            for (const auto& kk : v.keys()) d.set(kk, v.get(kk));
        } else {
            P.set(k, v);
        }
    }
    return P;
}

// ------------------------------------------------------------------ the frame worker pool
struct Result {
    int idx = 0;
    Planes out;
};

class Pool {
public:
    explicit Pool(int n) {
        for (int i = 0; i < std::max(1, n); ++i) threads_.emplace_back([this] { loop(); });
    }
    ~Pool() {
        {
            std::lock_guard<std::mutex> lk(mu_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& t : threads_) t.join();
    }
    std::future<Result> submit(std::function<Result()> fn) {
        auto task = std::make_shared<std::packaged_task<Result()>>(std::move(fn));
        std::future<Result> f = task->get_future();
        {
            std::lock_guard<std::mutex> lk(mu_);
            q_.push_back([task] { (*task)(); });
        }
        cv_.notify_one();
        return f;
    }

private:
    void loop() {
        for (;;) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lk(mu_);
                cv_.wait(lk, [this] { return stop_ || !q_.empty(); });
                if (stop_ && q_.empty()) return;
                job = std::move(q_.front());
                q_.pop_front();
            }
            job();
        }
    }
    std::mutex mu_;
    std::condition_variable cv_;
    std::deque<std::function<void()>> q_;
    std::vector<std::thread> threads_;
    bool stop_ = false;
};

struct ExportCancelled {};

}  // namespace

// ------------------------------------------------------------------ probe
ProbeInfo probe_full(const std::string& path) {
    ProbeInfo info;
    // the first ffprobe is video.cpp's probe() (the same command; traced here for the comparison)
    trace({tool_path("ffprobe"), "-v", "error", "-select_streams", "v:0", "-show_entries",
           "stream=width,height,r_frame_rate,nb_frames,pix_fmt,bits_per_raw_sample,color_space,color_primaries,"
           "color_transfer,color_range,start_time:stream_side_data=rotation:format=duration,start_time",
           "-of", "json", path});
    info.v = probe(path);
    // the video stream's start_time (fallback of v0): probe() does not keep it, read it again from the same JSON
    std::string so, se;
    run_capture({tool_path("ffprobe"), "-v", "error", "-show_entries", "format=start_time,format_name:stream=codec_type,codec_name,start_time",
                 "-of", "json", path},
                &so, &se);
    PyObj allj;
    try {
        allj = PyObj::parse(so.empty() ? "{}" : so);
    } catch (...) {
        allj = PyObj::dict();
    }
    const PyObj fmt = allj.get("format");
    std::string first_err;
    run_capture({tool_path("ffmpeg"), "-nostdin", "-hide_banner", "-copyts", "-noautorotate", "-i", path, "-map", "0:v:0", "-frames:v",
                 "1", "-vf", "showinfo", "-f", "null", "-"},
                &so, &first_err);
    info.fs = num(fmt.get("start_time"), 0.0);
    std::smatch m;
    bool have_v0 = false;
    if (std::regex_search(first_err, m, std::regex("pts_time:([-0-9.e+]+)"))) {
        bool ok = false;
        const double v = num(PyObj::str(m[1].str()), 0.0, &ok);
        if (ok) {
            info.v0 = v;
            have_v0 = true;
        }
    }
    if (!have_v0) {
        // s.get("start_time") of the video stream: the first video stream of the second probe
        const PyObj& streams = allj.get("streams");
        bool found = false;
        for (size_t i = 0; i < streams.size() && !found; ++i)
            if (streams[i].get("codec_type").type() == PyObj::STR && streams[i].get("codec_type").as_str() == "video") {
                bool ok = false;
                const double v = num(streams[i].get("start_time"), info.fs, &ok);
                info.v0 = ok ? v : info.fs;
                found = true;
            }
        if (!found) info.v0 = info.fs;
    }
    const PyObj& streams = allj.get("streams");
    for (size_t i = 0; i < streams.size(); ++i) {
        const PyObj& a = streams[i];
        if (a.get("codec_type").type() != PyObj::STR || a.get("codec_type").as_str() != "audio") continue;
        ProbeInfo::Audio au;
        au.codec = a.get("codec_name").type() == PyObj::STR ? a.get("codec_name").as_str() : "unknown";
        bool ok = false;
        const double st = num(a.get("start_time"), 0.0, &ok);
        if (ok) au.start = st;
        info.audio.push_back(au);
    }
    const std::string fn = fmt.get("format_name").type() == PyObj::STR ? fmt.get("format_name").as_str() : "";
    for (const auto& part : split_commas(fn))
        if (TS_DISCONT.count(part)) info.discont = true;
    return info;
}

// ------------------------------------------------------------------ paths / codecs
bool is_sequence(const std::string& dst) {
    const std::string b = QFileInfo(QString::fromStdString(dst)).fileName().toStdString();
    return std::regex_search(b, std::regex("%0?\\d*d"));
}

std::string sidecar_stem(const std::string& dst) {
    std::string stem = stem_of(dst);
    if (is_sequence(dst)) {
        const QFileInfo fi(QString::fromStdString(stem));
        std::string b = std::regex_replace(fi.fileName().toStdString(), std::regex("%0?\\d*d"), "");
        while (!b.empty() && (b.back() == '.' || b.back() == '_' || b.back() == '-')) b.pop_back();
        if (b.empty()) b = "frames";
        const std::string d = fi.path().toStdString();
        stem = (d.empty() || d == ".") && stem.find('/') == std::string::npos ? b : d + "/" + b;
    }
    return stem;
}

std::string pick_codec(const PixFmt& fmt, const std::string& dst, const std::string& codec) {
    const std::string ext = ext_of(dst);
    const bool exr = ext == ".exr" && is_sequence(dst);
    if (codec == "auto") {
        if (fmt.legacy()) return "x264";
        if (ext == ".mkv") return "ffv1";
        if (fmt.family == "rgb") {
            if (fmt.is_float && exr) return "exr";
            throw std::runtime_error(fmt.name + " output needs .mkv (FFV1)" +
                                     (fmt.is_float ? " or an EXR sequence pattern such as out_%05d.exr" : ""));
        }
        if (fmt.bits == 16) throw std::runtime_error("16-bit output needs .mkv (FFV1, lossless)");
        const bool mp4 = ext == ".mp4" || ext == ".mov" || ext == ".m4v";
        if (mp4 && fmt.bits == 10) return "x264";
        if (ext == ".mp4" || ext == ".mov") return "x265";
        if (ext == ".m4v") throw std::runtime_error("12-bit output: .m4v (iPod muxer) cannot hold HEVC; use .mp4 / .mov (x265) or .mkv (FFV1)");
        throw std::runtime_error(std::to_string(fmt.bits) + "-bit output: use .mp4 / .mov / .m4v (x264 10-bit, x265 12-bit) or .mkv (FFV1)");
    }
    bool ok;
    if (codec == "x264") ok = fmt.family == "yuv" && (fmt.bits == 8 || fmt.bits == 10);
    else if (codec == "x265") ok = fmt.family == "yuv" && (fmt.bits == 8 || fmt.bits == 10 || fmt.bits == 12);
    else if (codec == "ffv1") ok = true;
    else if (codec == "prores") ok = fmt.name == "yuv422p10le" || fmt.name == "yuv444p10le";
    else if (codec == "exr") ok = fmt.name == "gbrpf32le";
    else throw std::runtime_error("unknown codec " + codec);
    if (!ok) {
        const std::map<std::string, std::string> need = {
            {"x264", "8 or 10-bit yuv (--depth 8 or 10 with a yuv source)"},
            {"x265", "8, 10 or 12-bit yuv"},
            {"prores", "10-bit yuv 4:2:2 or 4:4:4 (--depth 10 with a 4:2:2 / 4:4:4 source)"},
            {"exr", "32-bit float (--depth 32)"}};
        throw std::runtime_error("--codec " + codec + " cannot write " + fmt.name + ": it needs " + need.at(codec));
    }
    if (codec == "exr" && !exr) throw std::runtime_error("--codec exr needs an image-sequence pattern such as out_%05d.exr");
    return codec;
}

void remove_output(const std::string& dst) {
    std::vector<std::string> paths = {dst};
    if (is_sequence(dst)) {
        const QFileInfo fi(QDir::cleanPath(QFileInfo(QString::fromStdString(dst)).absoluteFilePath()));
        const std::string d = fi.path().toStdString(), b = fi.fileName().toStdString();
        // re.escape(b) with %0?(\d*)d -> \d{n,}
        std::string rx;
        std::string pat = b;
        std::smatch m;
        std::string rest = pat;
        std::string out;
        std::regex r("%0?(\\d*)d");
        auto esc = [](const std::string& s) {
            std::string e;
            for (char c : s) {
                if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') e += c;
                else e += std::string("\\") + c;
            }
            return e;
        };
        while (std::regex_search(rest, m, r)) {
            out += esc(m.prefix().str());
            out += "\\d{" + (m[1].str().empty() ? std::string("1") : m[1].str()) + ",}";
            rest = m.suffix().str();
        }
        out += esc(rest);
        paths.clear();
        QDir dir(QString::fromStdString(d));
        if (dir.exists()) {
            const std::regex frx(out + "$");
            for (const QString& n : dir.entryList(QDir::Files | QDir::NoDotAndDotDot))
                if (std::regex_match(n.toStdString(), frx)) paths.push_back(dir.filePath(n).toStdString());
        }
    }
    for (const auto& q : paths) ::unlink(q.c_str());
}

// ------------------------------------------------------------------ run_export
int run_export(const ExportOptions& o, const std::function<void(const std::string&)>& print,
               const std::atomic<bool>* cancel) {
    signal(SIGPIPE, SIG_IGN);
    tl_cancel = cancel;
    auto cancelled = [&] { return cancel && cancel->load(); };
    const auto t0 = std::chrono::steady_clock::now();
    auto secs = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(); };
    try {
        // export.py main: the UI profile
        const PyObj& prof = o.prof;
        // denoise_video_v5.main
        QDir().mkpath(QFileInfo(QString::fromStdString(o.dst)).absolutePath());
        const ProbeInfo info = probe_full(o.src);
        const int W = info.v.W, H = info.v.H, N = info.v.n;
        const PixFmt fmt = choose_format(source_format(info.v.pix_fmt), o.depth);
        std::string codec;
        try {
            codec = pick_codec(fmt, o.dst, o.codec);
        } catch (const std::runtime_error& e) {
            print(std::string("error: ") + e.what());
            return 1;
        }
        if (auto err = trial_encode(o.dst, fmt, codec, o.crf)) {
            const std::string e = ext_of(o.dst);
            print("error: " + codec + " / " + fmt.name + " cannot be written to " + (e.empty() ? o.dst : e) +
                  " (trial encode: " + *err + "); choose another --codec or container (.mkv takes them all)");
            return 1;
        }
        const std::string matrix = matrix_from_tags(info.v.tags, H);
        const bool full_range = full_range_internal(info.v.full_range, fmt);
        const std::string vf = fmt.legacy() ? "" : read_filter(info, fmt, matrix);
        const std::string pix = fmt.legacy() ? "yuv420p" : fmt.name;
        const long long fs = fmt.legacy() ? static_cast<long long>(W) * H + 2LL * ((W + 1) / 2) * ((H + 1) / 2)
                                          : fmt.frame_bytes(W, H);
        const PyObj P = v5_params(o.params);
        const int R = static_cast<int>(P.get("radius").as_int());
        const int lo = std::max(0, o.start.value_or(0));
        const int hi = o.frames ? lo + *o.frames : -1;
        const int total = o.frames ? *o.frames : std::max(N - lo, 0);
        {
            std::string tg = "{";
            const char* keys[4] = {"color_space", "color_primaries", "color_transfer", "color_range"};
            for (int i = 0; i < 4; ++i) {
                auto it = info.v.tags.find(keys[i]);
                tg += std::string(i ? ", " : "") + pyrepr(keys[i]) + ": " + (it == info.v.tags.end() ? "None" : pyrepr(it->second));
            }
            tg += "}";
            print(std::to_string(W) + "x" + std::to_string(H) + " " + info.v.fps + " frames~" + (N ? std::to_string(N) : "?") +
                  " rotation=" + std::to_string(info.v.rotation) + " out=[" + std::to_string(lo) + "," +
                  (hi >= 0 ? std::to_string(hi) : "end") + ") tags=" + tg);
        }
        print("source " + describe_source(info.v.pix_fmt) + " -> processing " + describe(fmt) + ", codec " + codec);

        // the profile frames (read as the Python script does, for the same error paths)
        int pf = hi >= 0 ? (lo + hi) / 2 : N / 3;
        pf = N > 0 ? std::max(std::min(pf, N - 3), 0) : std::max(pf, 0);
        auto count_frames = [&](int a, int b) {
            RawReader rd;
            rd.c = spawn(reader_args(o.src, a, b, info.v.fps, pix, vf), false, true, false);
            rd.fs = fs;
            int n = 0;
            while (n < b - a && rd.read_frame()) ++n;
            rd.close();
            return n;
        };
        int npr = count_frames(pf, pf + 3);
        if (npr == 0 && pf > 0) {
            print("note: profile frame " + std::to_string(pf) + " not decodable, using the first frames");
            pf = 0;
            npr = count_frames(0, 3);
        }
        if (npr == 0) {
            print("no decodable video frames");
            return 1;
        }
        // export.ui_profile: the chroma sub-sampling of the processing format vs the profile's
        const int sub = fmt.chroma == "420" ? 2 : 1;
        const int psub = prof.has("sub") ? static_cast<int>(prof.get("sub").as_int()) : -1;
        if (psub != sub) {
            print(std::string("error: the UI profile was made on ") + (psub == 1 ? "4:4:4" : "4:2:0") +
                  " planes but this export processes " + (sub == 1 ? "4:4:4" : "4:2:0") +
                  " planes (--depth); rebuild the profile with the same bit-depth setting");
            return 1;
        }
        if (prof.get("full_range").truthy() != full_range)
            print("warning: the UI profile and this export differ in range (limited / full: RGB formats are "
                  "processed as full range); the noise curve's brightness axis may be off");
        {
            const PyObj& st = prof.get("flat_status");
            std::string status = "manual";
            if (!st.is_none()) {
                auto it = FLAT_STATUS.find(static_cast<int>(st.as_int()));
                status = it == FLAT_STATUS.end() ? std::to_string(st.as_int()) : it->second;
            }
            std::string sg = "{";
            const PyObj& s = prof.get("sigma");
            for (size_t i = 0; i < s.keys().size(); ++i) {
                const double v = s.get(s.keys()[i]).as_double();
                sg += std::string(i ? ", " : "") + pyrepr(s.keys()[i]) + ": " + py_float_str(std::nearbyint(v * 1000.0) / 1000.0);
            }
            sg += "}";
            print("profile frame " + std::to_string(pf) + ": mode=" + prof.get("mode").as_str() + " area=" + prof.get("area").dump() +
                  " (" + status + ") sigma=" + sg);
            if (!st.is_none() && (st.as_int() == 1 || st.as_int() == 3 || st.as_int() == 4))
                print("warning: profiling square: " + FLAT_STATUS.at(static_cast<int>(st.as_int())) + " - consider --area Y,X,SIZE");
        }

        if (codec == "exr" && !info.audio.empty() && o.audio != "none") print("note: an EXR sequence holds no audio - audio dropped");
        Plan plan = audio_plan(o.src, o.dst, info, codec == "exr" ? "none" : o.audio, print);
        const double fr_s = fps_of(info.v.fps);
        const std::optional<double> dur = hi >= 0 ? std::optional<double>((hi - lo) / fr_s) : std::nullopt;
        const std::vector<std::string> enc_args = encoder_args(o.dst, info, o.crf, plan, o.src, lo / fr_s, dur, fmt, codec, print);
        if (cancelled()) return 1;
        Child enc = spawn(enc_args, true, false, false);
        if (!plan.empty()) {
            std::string a;
            for (size_t j = 0; j < plan.size(); ++j)
                a += (j ? ", " : "") + info.audio[static_cast<size_t>(plan[j].first)].codec + " -> " + plan[j].second;
            print("audio: " + a);
        }

        // the frame windows
        struct Entry {
            int idx;
            std::shared_ptr<Planes> p;
        };
        std::deque<Entry> dq;
        std::deque<std::future<Result>> pending;
        std::map<int, RestoreMap> nonfin;
        int done = 0;
        bool broken = false;
        Pool pool(o.jobs);
        const int jobs = std::max(1, o.jobs);
        auto write = [&](bool block) {
            while (!pending.empty() &&
                   (block || pending.front().wait_for(std::chrono::seconds(0)) == std::future_status::ready ||
                    static_cast<int>(pending.size()) > 2 * jobs)) {
                Result r = pending.front().get();
                pending.pop_front();
                auto rs = nonfin.find(r.idx);
                const std::vector<uint8_t> bytes = from_internal(r.out, fmt, matrix, rs == nonfin.end() ? nullptr : &rs->second);
                if (rs != nonfin.end()) nonfin.erase(rs);
                if (!write_all(enc.in, bytes.data(), bytes.size())) {
                    pending.clear();                                  // the encoder stopped (its error is above)
                    throw std::runtime_error("BrokenPipe");
                }
                for (auto it = nonfin.begin(); it != nonfin.end();)
                    it = it->first < r.idx ? nonfin.erase(it) : std::next(it);
                ++done;
                if (done % 24 == 0)
                    print("frame " + std::to_string(done) + "/" + std::to_string(std::max(total, done)) + " " + fmt0(secs()) + "s");
            }
        };
        auto submit = [&](int c) {
            std::vector<int> idx;
            std::vector<std::shared_ptr<Planes>> win;
            for (const auto& e : dq)
                if (std::abs(e.idx - c) <= R) {
                    idx.push_back(e.idx);
                    win.push_back(e.p);
                }
            const int ci = static_cast<int>(std::find(idx.begin(), idx.end(), c) - idx.begin());
            const PyObj* profp = &prof;
            const PyObj* Pp = &P;
            pending.push_back(pool.submit([c, win, ci, profp, Pp] {
                std::vector<Planes> w;
                w.reserve(win.size());
                for (const auto& p : win) w.push_back(*p);        // _work: astype(np.float32) copies
                Planes tmp, rv;
                temporal(w, ci, *profp, *Pp, nullptr, &tmp, &rv);
                Result r;
                r.idx = c;
                r.out = finish(w[static_cast<size_t>(ci)], tmp, rv, *profp, *Pp);
                return r;
            }));
        };
        auto wanted = [&](int c) { return c >= lo && (hi < 0 || c < hi); };
        int last = lo - 1;
        RawReader rd;
        try {
            rd.c = spawn(reader_args(o.src, std::max(0, lo - R), hi >= 0 ? hi + R : -1, info.v.fps, pix, vf), false, true, false);
            rd.fs = fs;
            int i = std::max(0, lo - R);
            while (true) {
                if (cancelled()) throw ExportCancelled();
                if (hi >= 0 && i >= hi + R) break;
                if (!rd.read_frame()) break;
                auto p = std::make_shared<Planes>();
                RestoreMap rm;
                *p = to_internal(rd.buf.data(), fmt, W, H, matrix, fmt.is_float ? &rm : nullptr);
                if (!rm.empty()) nonfin[i] = std::move(rm);
                dq.push_back({i, p});
                while (static_cast<int>(dq.size()) > 2 * R + 1) dq.pop_front();
                last = i;
                if (wanted(i - R)) submit(i - R);
                write(false);
                ++i;
            }
            rd.close();
            for (int c = std::max(lo, last - R + 1); c <= last; ++c)
                if (wanted(c)) submit(c);
            write(true);
            ::close(enc.in);
            enc.in = -1;
        } catch (const ExportCancelled&) {
            rd.close();
            kill_child(enc);
            if (enc.in >= 0) ::close(enc.in);
            enc.in = -1;
            for (auto& f : pending) f.wait();
            wait_child(enc);
            return 1;
        } catch (const std::runtime_error& e) {
            if (std::string(e.what()) != "BrokenPipe") throw;
            broken = true;
            rd.close();
            if (enc.in >= 0) ::close(enc.in);
            enc.in = -1;
        }
        for (auto& f : pending) f.wait();
        const int rc = wait_child(enc);
        print("frame " + std::to_string(done) + "/" + std::to_string(done) + " " + fmt0(secs()) + "s");
        if (done == 0 && !broken) {
            print("no frames in the requested range (start=" + std::to_string(lo) + ")");
            return 1;
        }
        if (broken || rc != 0) {
            remove_output(o.dst);
            print("error: the encoder failed (ffmpeg exit " + std::to_string(rc) + ", message above); the incomplete " + o.dst +
                  " was removed");
            return 1;
        }
        int n_out = 0;
        if (codec != "exr") {
            std::string so, se;
            run_capture({tool_path("ffprobe"), "-v", "error", "-select_streams", "a", "-show_entries", "stream=index", "-of",
                         "csv=p=0", o.dst},
                        &so, &se);
            std::istringstream ss(so);
            std::string tok;
            while (ss >> tok) ++n_out;
        }
        if (!plan.empty() && n_out == 0) {
            print("warning: the requested range contains no audio - video only");
            plan.clear();
        }
        const double seconds = std::nearbyint(secs() * 10.0) / 10.0;
        PyObj meta = PyObj::dict();
        meta.set("src", PyObj::str(o.src));
        meta.set("frames", PyObj::integer(done));
        meta.set("range", PyObj::ints({lo, lo + done}));
        meta.set("seconds", PyObj::real(seconds));
        if (plan.empty()) {
            meta.set("audio", PyObj::none());
        } else {
            PyObj al = PyObj::list();
            for (const auto& [k, c] : plan) {
                PyObj d = PyObj::dict();
                d.set("stream", PyObj::integer(k));
                d.set("codec", PyObj::str(info.audio[static_cast<size_t>(k)].codec));
                d.set("out", PyObj::str(c));
                al.append(d);
            }
            meta.set("audio", al);
        }
        meta.set("source_pix_fmt", info.v.pix_fmt.empty() ? PyObj::none() : PyObj::str(info.v.pix_fmt));
        meta.set("pix_fmt", PyObj::str(fmt.name));
        meta.set("depth", PyObj::integer(fmt.bits));
        meta.set("codec", PyObj::str(codec));
        meta.set("params", P);
        meta.set("profile", prof);
        {
            std::ofstream f(sidecar_stem(o.dst) + ".json", std::ios::binary);
            f << meta.dump(1);
        }
        print("wrote " + o.dst + " (" + std::to_string(done) + " frames, " + fmt1(seconds) + "s)");
        return 0;
    } catch (const std::exception& e) {
        print(std::string("error: ") + e.what());
        return 1;
    }
}

// ------------------------------------------------------------------ ExportJob
// ------------------------------------------------------------------ the UI's reader at a processing format
std::string read_filter(const VideoInfo& info, const PixFmt& fmt, const std::string& matrix) {
    if (fmt.family != "rgb" || source_format(info.pix_fmt).family == "rgb") return "";
    return "scale=in_color_matrix=" + matrix + ":in_range=" + (info.full_range ? "pc" : "tv");
}

void read_frames_fmt(const std::string& path, int W, int H, int lo, int hi, const std::string& fps, const PixFmt& fmt,
                     const std::string& matrix, const std::string& vf, const std::function<bool(int, Planes&&)>& on_frame) {
    // denoise_video_v5.read_frames(path, W, H, lo, hi, fps, fmt, matrix, vf): Popen(stdout=PIPE, stdin=DEVNULL)
    if (hi >= 0 && hi <= lo) return;
    RawReader rd;
    rd.c = spawn(reader_args(path, lo, hi, fps, fmt.name, vf), false, true, false);
    rd.fs = fmt.frame_bytes(W, H);
    int i = lo;
    try {
        while (rd.read_frame()) {
            Planes p = to_internal(rd.buf.data(), fmt, W, H, matrix, nullptr);
            if (!on_frame(i, std::move(p))) break;
            ++i;
        }
    } catch (...) {
        rd.close();
        throw;
    }
    rd.close();
}

ExportJob::ExportJob(ExportOptions o, QObject* parent) : QObject(parent), o_(std::move(o)) {
    connect(this, &ExportJob::lineSignal, this, [this](QString line) {
        log << line;
        static const QRegularExpression rx("^frame (\\d+)/(\\d+)");
        const auto m = rx.match(line);
        if (m.hasMatch())
            emit progress(100.0 * m.captured(1).toInt() / std::max(m.captured(2).toInt(), 1), line);
        else if (!line.trimmed().isEmpty())
            emit progress(-1.0, line);
    }, Qt::QueuedConnection);
    connect(this, &ExportJob::doneSignal, this, [this](int code) {
        running_ = false;
        const bool ok = code == 0 && !cancel_;
        QString text;
        if (ok) {
            text = log.isEmpty() ? QString() : log.last();
        } else {
            const int n = std::min<int>(6, log.size());
            text = QStringList(log.mid(log.size() - n)).join("\n");
        }
        emit finished(ok, text);
    }, Qt::QueuedConnection);
}

ExportJob::~ExportJob() {
    if (thread_) {
        cancel_ = true;
        reg_kill(&cancel_);
        thread_->wait();
        delete thread_;
    }
    std::lock_guard<std::mutex> lk(reg_mu);
    reg.erase(&cancel_);
}

void ExportJob::start() {
    if (thread_) return;
    running_ = true;
    thread_ = QThread::create([this] {
        const int code = run_export(o_, [this](const std::string& line) { emit lineSignal(QString::fromStdString(line)); }, &cancel_);
        emit doneSignal(code);
    });
    thread_->start();
}

void ExportJob::kill() {
    cancel_ = true;
    reg_kill(&cancel_);
}

}  // namespace nvqt
