// video - see video.h.
#include "video.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStringList>

#include <cmath>
#include <mutex>
#include <regex>
#include <stdexcept>

#include "pyobj.h"

namespace nvqt {

namespace {

std::mutex dir_mu;
bool dir_known = false;
std::string dir_value;

bool has_tools(const QString& d) {      // ffmpeg_path.has_tools: executable files (not directories)
    for (const char* t : {"ffmpeg", "ffprobe"}) {
        const QFileInfo fi(QDir(d).filePath(t));
        if (!fi.isFile() || !fi.isExecutable()) return false;
    }
    return true;
}

// ffmpeg_path.tool_dir: a binary or directory setting -> the directory to look in
QString tool_dir(const QString& v) {
    const QString e = QDir::fromNativeSeparators(v.startsWith("~") ? QDir::homePath() + v.mid(1) : v);
    return QFileInfo(e).isFile() ? QFileInfo(e).absolutePath() : e;
}

QString tool(const char* name) {
    const std::string d = ffmpeg_dir();
    return d.empty() ? QString(name) : QDir(QString::fromStdString(d)).filePath(name);
}

QByteArray run(const QString& prog, const QStringList& args, int timeout_ms = 120000) {
    QProcess p;
    p.setStandardInputFile(QProcess::nullDevice());
    p.start(prog, args);
    if (!p.waitForStarted(10000)) throw std::runtime_error("cannot start " + prog.toStdString());
    p.waitForFinished(timeout_ms);
    return p.readAllStandardOutput();
}

double num(const PyObj& v, double def) {
    if (v.type() == PyObj::STR) {
        try {
            size_t used = 0;
            const double d = std::stod(v.as_str(), &used);
            return used ? d : def;
        } catch (...) {
            return def;
        }
    }
    if (v.is_num()) return v.as_double();
    return def;
}

}  // namespace

std::string find_ffmpeg_dir(const std::string& pref) {
    QStringList cands;
    const QByteArray env = qgetenv("NV_FFMPEG");
    for (const QString& v : {QString::fromLocal8Bit(env), QString::fromStdString(pref)})
        if (!v.isEmpty()) cands << tool_dir(v);
    cands << "/opt/homebrew/bin" << "/usr/local/bin" << "/opt/local/bin";
    const QString path = QProcessEnvironment::systemEnvironment().value("PATH");
    for (const QString& p : path.split(':', Qt::SkipEmptyParts)) cands << p;
    for (const QString& c : cands)
        if (has_tools(c)) return QDir(c).absolutePath().toStdString();
    return "";
}

bool ffmpeg_tools_in(const std::string& pref) { return !pref.empty() && has_tools(tool_dir(QString::fromStdString(pref))); }

std::string ffmpeg_missing_text(const std::string& path) {
    return "No ffmpeg and ffprobe at " + path +
           ".\n\nName the ffmpeg binary or the directory that holds ffmpeg and ffprobe, or leave the field empty: "
           "ffmpeg is then looked for in $NV_FFMPEG, /opt/homebrew/bin, /usr/local/bin, /opt/local/bin and PATH.";
}

const char* FFMPEG_TOOLTIP = "ffmpeg / ffprobe: a binary or the directory holding both (prefs/ffmpeg); empty = "
                             "$NV_FFMPEG, /opt/homebrew/bin, /usr/local/bin, /opt/local/bin, PATH";

std::string ffmpeg_dir() {
    std::lock_guard<std::mutex> lk(dir_mu);
    if (!dir_known) {
        dir_value = find_ffmpeg_dir();
        dir_known = true;
    }
    return dir_value;
}

void set_ffmpeg_dir(const std::string& d) {
    std::lock_guard<std::mutex> lk(dir_mu);
    dir_value = d;
    dir_known = true;
}

std::string tool_path(const char* name) { return tool(name).toStdString(); }

double VideoInfo::fps_value() const {
    const auto s = fps.find('/');
    try {
        const double a = std::stod(fps.substr(0, s));
        const double b = s == std::string::npos ? 1.0 : std::stod(fps.substr(s + 1));
        if (b == 0) return 24.0;
        return a / b;
    } catch (...) {
        return 24.0;
    }
}

VideoInfo probe(const std::string& path) {
    const QByteArray out = run(tool("ffprobe"),
                               {"-v", "error", "-select_streams", "v:0", "-show_entries",
                                "stream=width,height,r_frame_rate,nb_frames,pix_fmt,bits_per_raw_sample,color_space,"
                                "color_primaries,color_transfer,color_range,start_time:stream_side_data=rotation:"
                                "format=duration,start_time",
                                "-of", "json", QString::fromStdString(path)});
    PyObj j;
    try {
        j = PyObj::parse(out.toStdString());
    } catch (const std::exception&) {
        throw std::runtime_error("ffprobe could not read " + path);
    }
    const PyObj& streams = j.get("streams");
    if (!streams.is_list() || streams.size() == 0) throw std::runtime_error("no video stream in " + path);
    const PyObj& s = streams[0];
    VideoInfo info;
    info.W = static_cast<int>(s.get("width").as_int());
    info.H = static_cast<int>(s.get("height").as_int());
    info.fps = s.get("r_frame_rate").type() == PyObj::STR ? s.get("r_frame_rate").as_str() : "24/1";
    const auto sl = info.fps.find('/');
    const double fnum = std::stod(info.fps.substr(0, sl)), fden = sl == std::string::npos ? 1.0 : std::stod(info.fps.substr(sl + 1));
    int n = 0;
    try {
        n = s.get("nb_frames").type() == PyObj::STR ? std::stoi(s.get("nb_frames").as_str())
                                                    : static_cast<int>(s.get("nb_frames").as_int());
    } catch (...) {
        n = 0;
    }
    if (n <= 0) {
        const double d = num(j.get("format").get("duration"), 0.0);
        n = fden != 0 ? static_cast<int>(std::nearbyint(d * fnum / fden)) : 0;   // int(round(...)): half to even
    }
    info.n = std::max(n, 0);
    const PyObj& sd = s.get("side_data_list");
    for (size_t i = 0; i < sd.size(); ++i)
        if (sd[i].has("rotation")) {
            int r = static_cast<int>(std::nearbyint(num(sd[i].get("rotation"), 0.0))) % 360;
            if (r < 0) r += 360;
            info.rotation = r;
        }
    for (const char* k : {"color_space", "color_primaries", "color_transfer", "color_range"})
        if (s.get(k).type() == PyObj::STR) info.tags[k] = s.get(k).as_str();
    info.pix_fmt = s.get("pix_fmt").type() == PyObj::STR ? s.get("pix_fmt").as_str() : "";
    info.bits_raw = static_cast<int>(num(s.get("bits_per_raw_sample"), 0.0));
    info.full_range = info.tags.count("color_range") && info.tags["color_range"] == "pc";
    return info;
}

void read_frames(const std::string& path, int W, int H, int lo, int hi, const std::string& fps,
                 const std::function<bool(int, Frame&&)>& on_frame) {
    if (hi >= 0 && hi <= lo) return;
    QStringList chain;
    if (!fps.empty()) chain << "setpts=PTS-STARTPTS" << QString::fromStdString("fps=" + fps);
    if (lo > 0 || hi >= 0)
        chain << (hi >= 0 ? QString("select=between(n\\,%1\\,%2)").arg(lo).arg(hi - 1) : QString("select=gte(n\\,%1)").arg(lo));
    QStringList args{"-v", "error", "-nostdin", "-noautorotate", "-i", QString::fromStdString(path)};
    if (!chain.isEmpty()) args << "-vf" << chain.join(",");
    if (hi >= 0) args << "-frames:v" << QString::number(hi - lo);
    args << "-fps_mode" << "passthrough" << "-f" << "rawvideo" << "-pix_fmt" << "yuv420p" << "-";
    const int cw = (W + 1) / 2, ch = (H + 1) / 2;
    const qint64 fs = static_cast<qint64>(W) * H + 2LL * cw * ch;
    QProcess p;
    p.setStandardInputFile(QProcess::nullDevice());
    p.setProcessChannelMode(QProcess::SeparateChannels);
    p.setStandardErrorFile(QProcess::nullDevice());
    p.start(tool("ffmpeg"), args);
    if (!p.waitForStarted(10000)) throw std::runtime_error("cannot start ffmpeg");
    QByteArray buf;
    int i = lo;
    bool more = true;
    while (more) {
        while (buf.size() < fs) {
            if (p.bytesAvailable() == 0 && !p.waitForReadyRead(60000)) break;
            buf += p.read(fs - buf.size());
        }
        if (buf.size() < fs) break;
        Frame f;
        f.H = H;
        f.W = W;
        f.Hc = ch;
        f.Wc = cw;
        const auto* d = reinterpret_cast<const uint8_t*>(buf.constData());
        f.Y.assign(d, d + static_cast<size_t>(W) * H);
        f.Cb.assign(d + static_cast<size_t>(W) * H, d + static_cast<size_t>(W) * H + static_cast<size_t>(cw) * ch);
        f.Cr.assign(d + static_cast<size_t>(W) * H + static_cast<size_t>(cw) * ch, d + fs);
        buf.clear();
        more = on_frame(i, std::move(f));
        ++i;
    }
    if (p.state() != QProcess::NotRunning) {
        p.kill();
        p.waitForFinished(5000);
    }
}

std::string matrix_from_tags(const std::map<std::string, std::string>& tags, int height) {
    static const std::map<std::string, std::string> TAG = {{"bt709", "bt709"}, {"smpte170m", "bt601"},
                                                           {"bt470bg", "bt601"}, {"bt2020nc", "bt2020"},
                                                           {"bt2020c", "bt2020"}};
    auto it = tags.find("color_space");
    if (it != tags.end()) {
        auto t = TAG.find(it->second);
        if (t != TAG.end()) return t->second;
    }
    return height >= 720 ? "bt709" : "bt601";
}

// ---------------------------------------------------------------- nv_depth.source_format
SrcFmt source_format(const std::string& name_in) {
    static const std::map<std::string, SrcFmt> FIXED = {
        {"nv12", {"yuv", 8, false, "420"}}, {"nv21", {"yuv", 8, false, "420"}}, {"nv16", {"yuv", 8, false, "422"}},
        {"nv24", {"yuv", 8, false, "444"}}, {"nv42", {"yuv", 8, false, "444"}}, {"nv20", {"yuv", 10, false, "422"}},
        {"p010", {"yuv", 10, false, "420"}}, {"p012", {"yuv", 12, false, "420"}}, {"p016", {"yuv", 16, false, "420"}},
        {"p210", {"yuv", 10, false, "422"}}, {"p212", {"yuv", 12, false, "422"}}, {"p216", {"yuv", 16, false, "422"}},
        {"p410", {"yuv", 10, false, "444"}}, {"p412", {"yuv", 12, false, "444"}}, {"p416", {"yuv", 16, false, "444"}},
        {"yuyv422", {"yuv", 8, false, "422"}}, {"uyvy422", {"yuv", 8, false, "422"}}, {"yvyu422", {"yuv", 8, false, "422"}},
        {"uyyvyy411", {"yuv", 8, false, "444"}}, {"y210", {"yuv", 10, false, "422"}}, {"y212", {"yuv", 12, false, "422"}},
        {"y216", {"yuv", 16, false, "422"}}, {"v210", {"yuv", 10, false, "422"}}, {"y410", {"yuv", 10, false, "444"}},
        {"y412", {"yuv", 12, false, "444"}}, {"xv30", {"yuv", 10, false, "444"}}, {"v30x", {"yuv", 10, false, "444"}},
        {"xv36", {"yuv", 12, false, "444"}}, {"xv48", {"yuv", 16, false, "444"}}, {"ayuv64", {"yuv", 16, false, "444"}},
        {"ayuv", {"yuv", 8, false, "444"}}, {"vuya", {"yuv", 8, false, "444"}}, {"vuyx", {"yuv", 8, false, "444"}},
        {"uyva", {"yuv", 8, false, "444"}}, {"v408", {"yuv", 8, false, "444"}}, {"ya8", {"yuv", 8, false, "420"}},
        {"ya16", {"yuv", 16, false, "420"}}, {"monow", {"yuv", 8, false, "420"}}, {"monob", {"yuv", 8, false, "420"}},
        {"pal8", {"rgb", 8, false, "444"}}, {"rgb4_byte", {"rgb", 8, false, "444"}}, {"bgr4_byte", {"rgb", 8, false, "444"}},
        {"x2rgb10", {"rgb", 10, false, "444"}}, {"x2bgr10", {"rgb", 10, false, "444"}}, {"xyz12", {"rgb", 12, false, "444"}}};
    static const std::map<std::string, std::string> CHROMA = {{"420", "420"}, {"422", "422"}, {"444", "444"},
                                                              {"440", "444"}, {"411", "444"}, {"410", "444"}};
    auto chroma_of = [&](const std::string& k) {
        auto it = CHROMA.find(k);
        return it == CHROMA.end() ? std::string("444") : it->second;
    };
    std::string n = name_in;
    for (auto& c : n) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    n = std::regex_replace(n, std::regex("(le|be)$"), "");
    n = std::regex_replace(n, std::regex("msb$"), "");
    auto fx = FIXED.find(n);
    if (fx != FIXED.end()) return fx->second;
    std::smatch m;
    if (std::regex_search(n, m, std::regex("f(16|32)$")) &&
        std::regex_search(n, std::regex("^(gbra?p|rgba?|bgra?|gray|ya|yuva?4\\d\\dp)"))) {
        const bool yuv = n.rfind("gray", 0) == 0 || n.rfind("ya", 0) == 0 || n.rfind("yuv", 0) == 0;
        std::smatch c;
        const bool has_ch = std::regex_search(n, c, std::regex("^yuva?(4\\d\\d)"));
        return {yuv ? "yuv" : "rgb", std::stoi(m[1].str()), true,
                has_ch ? chroma_of(c[1].str()) : (yuv ? "420" : "444")};
    }
    if (std::regex_match(n, m, std::regex("yuvj?a?(4\\d\\d)p(\\d+)?")))
        return {"yuv", m[2].matched ? std::stoi(m[2].str()) : 8, false, chroma_of(m[1].str())};
    if (std::regex_match(n, m, std::regex("gray(\\d+)?")))
        return {"yuv", m[1].matched ? std::stoi(m[1].str()) : 8, false, "420"};
    if (std::regex_match(n, m, std::regex("gbra?p(\\d+)?")))
        return {"rgb", m[1].matched ? std::stoi(m[1].str()) : 8, false, "444"};
    if (std::regex_match(n, m, std::regex("(?:rgba?|bgra?|argb|abgr|0rgb|rgb0|0bgr|bgr0)(\\d+)?"))) {
        const std::string g = m[1].matched ? m[1].str() : "";
        const int bits = g == "48" || g == "64" ? 16 : g == "96" || g == "128" ? 32 : 8;
        return {"rgb", bits, false, "444"};
    }
    if (std::regex_match(n, m, std::regex("bayer_\\w+?(16)?")))
        return {"rgb", m[1].matched ? 16 : 8, false, "444"};
    return {"yuv", 8, false, "420"};
}

std::string describe_source(const std::string& name) {
    const SrcFmt s = source_format(name);
    std::string kind = std::to_string(s.bits) + "-bit" + (s.is_float ? " float" : "");
    std::string ch = s.family == "rgb" ? "RGB" : std::string(1, s.chroma[0]) + ":" + s.chroma[1] + ":" + s.chroma[2];
    return (name.empty() ? "unknown" : name) + " (" + kind + " " + ch + ")";
}

}  // namespace nvqt
