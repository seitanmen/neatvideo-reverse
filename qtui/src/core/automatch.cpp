// automatch - see automatch.h.
#include "automatch.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include <cstdio>
#include <cstring>
#include <set>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "nvcore.h"

namespace nvqt {

const char* PROFILE_DIR_KEY = "prefs/profile_dir";

QString default_profile_dir() {
    // genuine FUN_0015c860 / FUN_0015ec38: $NV6OFX_PATH (a directory) + "/Profiles", else Documents + product + "/Profiles"
    const QByteArray env = qgetenv("MEATVIDEO_PATH");
    if (!env.isEmpty()) {
        const QString d = QString::fromLocal8Bit(env);
        if (QFileInfo(d).isDir()) return QDir::cleanPath(d + "/Profiles");
    }
    return QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Meat Video/Profiles");
}

QString profile_dir(QSettings& s) {
    const QString v = s.value(PROFILE_DIR_KEY).toString();
    return v.isEmpty() ? default_profile_dir() : v;
}

QString no_profiles_text(const QString& dir) {
    // the genuine text (FUN_000d8ec8) with the product name of the reproduction
    return QString("Meat Video could not find any profiles in the assigned Profile folder:\n %1\n\nPlease either put "
                   "some profiles to that folder or adjust its location using the Preferences dialog:\n  menu Tools > "
                   "Preferences > Folders > Profile folder")
        .arg(QDir::toNativeSeparators(dir));
}

namespace {

bool listed(const QString& name) {
    const QString n = name.toLower();
    return n.endsWith(".json") && !n.endsWith(".nvpreset.json") && !n.endsWith(".ui_params.json");
}

QString shown(const QString& file) {
    QString n = file.left(file.size() - 5);                  // ".json"
    if (n.toLower().endsWith(".nvprof")) n.chop(7);
    return n;
}

// the entries of dir; visited = the canonical paths of the folders already walked (symbolic-link loops, and a folder
// reached a second time through another link, are listed once)
std::vector<ProfileEntry> list_in(const QString& dir, std::set<QString>* visited) {
    std::vector<ProfileEntry> out;
    QDir d(dir);
    // FUN_00026954: setFilter(Files | AllDirs | NoDotAndDotDot), setSorting(Name | DirsFirst | IgnoreCase)
    d.setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);
    d.setSorting(QDir::Name | QDir::DirsFirst | QDir::IgnoreCase);
    for (const QFileInfo& fi : d.entryInfoList()) {
        ProfileEntry e;
        e.path = fi.absoluteFilePath();
        if (fi.isDir()) {
            const QString canon = fi.canonicalFilePath();
            if (canon.isEmpty() || !visited->insert(canon).second) continue;
            e.dir = true;
            e.name = fi.fileName();
            e.children = list_in(e.path, visited);
        } else {
            if (!listed(fi.fileName())) continue;
            e.name = shown(fi.fileName());
        }
        out.push_back(std::move(e));
    }
    return out;
}

}  // namespace

std::vector<ProfileEntry> list_profiles(const QString& dir) {
    QDir d(dir);
    if (dir.isEmpty() || !d.exists()) return {};
    std::set<QString> visited = {QFileInfo(dir).canonicalFilePath()};
    return list_in(dir, &visited);
}

std::vector<QString> profile_files(const std::vector<ProfileEntry>& entries) {
    std::vector<QString> out;
    for (const ProfileEntry& e : entries) {
        if (!e.dir) {
            out.push_back(e.path);
            continue;
        }
        const std::vector<QString> sub = profile_files(e.children);
        out.insert(out.end(), sub.begin(), sub.end());
    }
    return out;
}

namespace {

std::string hex_words(const float* w, int n) {
    std::string s;
    s.reserve(static_cast<size_t>(n) * 8);
    char b[9];
    for (int i = 0; i < n; ++i) {
        uint32_t u;
        std::memcpy(&u, w + i, 4);
        std::snprintf(b, sizeof(b), "%08x", u);
        s += b;
    }
    return s;
}

void unhex(const std::string& s, float* w, int n) {
    for (int i = 0; i < n; ++i) {
        uint32_t u = 0;
        for (int j = 0; j < 8; ++j) {
            const char c = s[static_cast<size_t>(8 * i + j)];
            const uint32_t v = c >= '0' && c <= '9' ? static_cast<uint32_t>(c - '0') : static_cast<uint32_t>(c - 'a' + 10);
            u = (u << 4) | v;
        }
        std::memcpy(w + i, &u, 4);
    }
}

bool hex_ok(const PyObj& v, int n) {
    if (v.type() != PyObj::STR || v.as_str().size() != static_cast<size_t>(n) * 8) return false;
    for (char c : v.as_str())
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

}  // namespace

PyObj match_model_json(const std::vector<float>& words, int W, int H, const int area[4], int sub,
                       const std::string& matrix, bool full_range, const std::string& depth) {
    PyObj m = PyObj::dict();
    m.set("format", PyObj::str("nv_repro_match_model"));
    m.set("version", PyObj::integer(1));
    m.set("definition", PyObj::str("repro (N4e), not a genuine structure: the AFT's profile model words (FUN_009b6338 "
                                   "inputs) of BuildNoiseModel on the profile's area, colour mode 1"));
    PyObj f = PyObj::list(), s = PyObj::list();
    for (int c = 0; c < 3; ++c) {
        f.append(PyObj::str(hex_words(words.data() + c * MATCH_FACE, MATCH_FACE)));
        s.append(PyObj::str(hex_words(words.data() + 3 * MATCH_FACE + c * MATCH_STATS, MATCH_STATS)));
    }
    m.set("faces", f);
    m.set("stats", s);
    m.set("frame", PyObj::ints({W, H}));
    m.set("area_xywh", PyObj::ints({area[0], area[1], area[2], area[3]}));
    m.set("sub", PyObj::integer(sub));
    m.set("matrix", PyObj::str(matrix));
    m.set("full_range", PyObj::boolean(full_range));
    m.set("depth", PyObj::str(depth));
    return m;
}

void check_match_model(const PyObj& m) {
    auto bad = [](const std::string& what) { throw std::runtime_error("profile match_model " + what); };
    if (!m.is_dict() || m.get("format").type() != PyObj::STR || m.get("format").as_str() != "nv_repro_match_model")
        bad("is not an nv_repro_match_model");
    if (m.get("version").type() != PyObj::INT || m.get("version").as_int() != 1) bad("version must be 1");
    const PyObj& f = m.get("faces");
    const PyObj& s = m.get("stats");
    if (!f.is_list() || f.size() != 3 || !s.is_list() || s.size() != 3) bad("faces / stats must be 3 hex strings each");
    for (size_t c = 0; c < 3; ++c)
        if (!hex_ok(f[c], MATCH_FACE) || !hex_ok(s[c], MATCH_STATS))
            bad("faces / stats must be lower-case hex of 492 / 51 words");
    if (m.get("sub").type() != PyObj::INT || (m.get("sub").as_int() != 1 && m.get("sub").as_int() != 2))
        bad("sub must be 1 or 2");
}

std::vector<float> match_model_words(const PyObj& m) {
    std::vector<float> w(MATCH_WORDS);
    for (size_t c = 0; c < 3; ++c) {
        unhex(m.get("faces")[c].as_str(), w.data() + c * MATCH_FACE, MATCH_FACE);
        unhex(m.get("stats")[c].as_str(), w.data() + 3 * MATCH_FACE + c * MATCH_STATS, MATCH_STATS);
    }
    return w;
}

int frame_sub(const Frame& f) { return (f.Hc == f.H && f.Wc == f.W) ? 1 : 2; }

bool sub_fits(const ProfileState& ps, int sub, int* have) {
    const int msub = static_cast<int>(ps.match.get("sub").as_int());
    const int bsub = ps.base.has("sub") ? static_cast<int>(ps.base.get("sub").as_int()) : msub;
    if (have) *have = bsub;
    return msub == sub && bsub == sub;
}

int best_match(const std::vector<AutoMatchEntry>& entries) {
    int best = -1;
    for (size_t i = 0; i < entries.size(); ++i) {
        const AutoMatchEntry& e = entries[i];
        if (e.status != "scored" || !(e.score > 0.0)) continue;
        if (best < 0 || e.score > entries[static_cast<size_t>(best)].score) best = static_cast<int>(i);
    }
    return best;
}

AutoMatchResult run_auto_match(const QString& dir, const Frame& frame, int frame_index, bool full_range,
                               const std::string& matrix, const std::string& depth, const std::function<bool()>& cancel) {
    AutoMatchResult r;
    r.dir = dir;
    r.frame = frame_index;
    const int sub = frame_sub(frame);
    std::vector<std::vector<float>> models;
    std::vector<size_t> which;
    for (const QString& path : profile_files(list_profiles(dir))) {
        if (cancel && cancel()) throw Cancelled();
        AutoMatchEntry e;
        e.path = path;
        e.name = QFileInfo(path).fileName();
        ProfileState ps;
        try {
            std::ifstream f(path.toStdString(), std::ios::binary);
            if (!f) throw std::runtime_error("cannot read the file");
            std::stringstream ss;
            ss << f.rdbuf();
            ps = ProfileState::from_json(PyObj::parse(ss.str()));
        } catch (const std::exception& ex) {
            e.status = "unreadable";
            e.detail = ex.what();
        }
        if (e.status.empty() && ps.match.is_none()) {
            e.status = "no match model";
            e.detail = "saved before N4e, or by the Python UI / Batch Profiler: rebuild the profile and save it again";
        }
        if (e.status.empty()) {
            int bsub = 0;
            if (!sub_fits(ps, sub, &bsub)) {
                e.status = "chroma format differs";
                e.detail = "profile sub " + std::to_string(bsub) + ", video sub " + std::to_string(sub) +
                           " (export would refuse it)";
            }
        }
        if (e.status.empty()) {
            e.status = "scored";
            std::string d;
            const PyObj& m = ps.match;
            if (m.get("matrix").type() == PyObj::STR && m.get("matrix").as_str() != matrix)
                d += "matrix " + m.get("matrix").as_str() + " (video " + matrix + ")";
            if (m.get("full_range").type() == PyObj::BOOL && m.get("full_range").truthy() != full_range)
                d += std::string(d.empty() ? "" : ", ") + (m.get("full_range").truthy() ? "full" : "limited") + " range";
            if (m.get("depth").type() == PyObj::STR && m.get("depth").as_str() != depth)
                d += std::string(d.empty() ? "" : ", ") + "depth " + m.get("depth").as_str() + " (now " + depth + ")";
            e.detail = d;
            models.push_back(match_model_words(m));
            which.push_back(r.entries.size());
        }
        r.entries.push_back(std::move(e));
        r.states.push_back(std::move(ps));
    }
    if (!models.empty()) {
        const RefModel ref = reference_model(frame, full_range, matrix, cancel);
        r.ref_status = ref.status;
        for (int i = 0; i < 4; ++i) r.ref_area[i] = ref.area[i];
        if (ref.words.empty()) {
            r.status = 5;
            return r;
        }
        const MatchScores ms = match_scores(frame, full_range, matrix, ref.words, models, cancel);
        r.status = ms.status;
        r.tiles = ms.tiles;
        r.used = ms.used;
        r.ref_accept = ms.ref_accept;
        if (ms.status == 0)
            for (size_t k = 0; k < which.size(); ++k) {
                r.entries[which[k]].score = ms.scores[k];
                r.entries[which[k]].accept = ms.accept[k];
            }
    }
    r.best = r.status == 0 ? best_match(r.entries) : -1;
    return r;
}

}  // namespace nvqt
