// automatch - N4e: the Profile folder, the saved-profile list (the genuine ProfilePopupButton, "Show saved noise
// profiles") and the match model of the reproduction's own Auto Match (the score: native/nvcore_c_match.h).
//
// Genuine (work/omo/wp_automatch/DECODE.md 3): settings key ProfileDir, default <Documents>/Neat Video v6 OpenFX/Profiles,
// $NV6OFX_PATH/Profiles when that variable names a directory; the list is recursive, folders first, case-insensitive
// name order, and empty it shows "Neat Video could not find any profiles in the assigned Profile folder: ...".
// Repro: settings key prefs/profile_dir ("" or missing = the default), default <Documents>/Meat Video/Profiles,
// $MEATVIDEO_PATH/Profiles when that variable names a directory; files *.json except *.nvpreset.json / *.ui_params.json.
//
// The match model is the repro definition (the genuine Auto Match is an empty slot): the words of the AFT's profile
// model (native/nvcore_c_match.h) of the profile's area, kept in the profile file as "match_model".
#pragma once

#include <QSettings>
#include <QString>

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "pyobj.h"
#include "state.h"

namespace nvqt {

extern const char* PROFILE_DIR_KEY;            // "prefs/profile_dir"
QString default_profile_dir();                 // $MEATVIDEO_PATH/Profiles or <Documents>/Meat Video/Profiles
QString profile_dir(QSettings& s);             // the setting, else the default
QString no_profiles_text(const QString& dir);  // the genuine text with the folder (native separators)

struct ProfileEntry {
    QString name;                              // shown name: the file name without .json (and .nvprof)
    QString path;                              // absolute path
    bool dir = false;
    std::vector<ProfileEntry> children;        // a folder's entries (same order rules)
};
// the folder's entries: folders first, then files, each in case-insensitive name order (QDir Name | DirsFirst |
// IgnoreCase), folders recursively; a missing folder gives none.  Each folder (canonical path) is walked once, so
// symbolic-link loops end and a folder reached again through another link is not listed again
std::vector<ProfileEntry> list_profiles(const QString& dir);
std::vector<QString> profile_files(const std::vector<ProfileEntry>& entries);   // the files in menu order

constexpr int MATCH_FACE = 492, MATCH_STATS = 51, MATCH_WORDS = 3 * MATCH_FACE + 3 * MATCH_STATS;
// "match_model" of a profile file: the words (faces R, G, B then stats R, G, B, as nvc_match_model writes them)
// with the source frame size, the area (x, y, w, h), the chroma subsampling (base "sub"), matrix, range and depth
PyObj match_model_json(const std::vector<float>& words, int W, int H, const int area_xywh[4], int sub,
                       const std::string& matrix, bool full_range, const std::string& depth);
void check_match_model(const PyObj& m);                    // throws std::runtime_error when malformed
std::vector<float> match_model_words(const PyObj& m);      // MATCH_WORDS floats (m checked)

// ---------------------------------------------------------------- the repro Auto Match
struct Frame;
// one profile file of the folder: status "scored", "unreadable", "no match model" or "chroma format differs";
// detail = the reason / the differences that do not exclude it (matrix, range, depth); score = nvc_match_score
struct AutoMatchEntry {
    QString path, name;
    std::string status, detail;
    double score = 0.0;
    int accept = 0;
};
struct AutoMatchResult {
    QString dir;
    int frame = 0;                                 // the frame scored against
    // 0 ok, 1 too small, 3 model error, 4 all clipped (nvc_match_score), 5 no reference (FindFlatArea found no square)
    int status = 0;
    int tiles = 0, used = 0, ref_accept = 0;
    int ref_area[4] = {0, 0, 0, 0};                // the reference square (x, y, w, h) and its FindFlatArea status
    int ref_status = -1;
    std::vector<AutoMatchEntry> entries;           // every listed file, in menu order
    std::vector<ProfileState> states;              // per entry (meaningful for "scored")
    int best = -1;                                 // best_match(entries)
};
// the entry to load: the highest score among the scored entries, the first in menu order on equal scores; -1 when
// none is scored or the best score is not > 0 (no tile of the frame fits any profile)
int best_match(const std::vector<AutoMatchEntry>& entries);
// reads every listed file of dir, keeps the profiles with a match model of the frame's chroma format (sub) and scores
// them against frame with the frame's own model (reference_model) as the reference (one nvc_match_score call;
// throws Cancelled)
AutoMatchResult run_auto_match(const QString& dir, const Frame& frame, int frame_index, bool full_range,
                               const std::string& matrix, const std::string& depth, const std::function<bool()>& cancel);
// the frame's chroma subsampling as base "sub" counts it (4:4:4 = 1, else 2)
int frame_sub(const Frame& f);
// the profile's chroma format fits sub: match_model "sub" and base "sub" (when present) both equal it (Auto Match and
// the Match row); *have = the profile's value shown in messages (base "sub", else the match model's)
bool sub_fits(const ProfileState& ps, int sub, int* have = nullptr);

}  // namespace nvqt
