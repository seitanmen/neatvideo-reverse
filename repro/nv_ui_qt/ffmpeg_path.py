"""Where ffmpeg / ffprobe are (they are not bundled): the same search in the PySide6 UI (main.py), the MeatVideo.app
launcher (packaging/main.py) and the C++ UI (qtui/src/core/video.cpp find_ffmpeg_dir):

  1. $NV_FFMPEG (the ffmpeg binary or its directory)
  2. the setting prefs/ffmpeg (Preferences > ffmpeg (directory or binary); a binary or its directory)
  3. /opt/homebrew/bin, /usr/local/bin, /opt/local/bin
  4. $PATH

The first directory holding both executables is used.  repro calls "ffmpeg" / "ffprobe" by name, so the directory
found goes to the front of $PATH (a Finder launch has only /usr/bin:/bin:...) and is recorded in $NV_FFMPEG_DIR
("" = not found), which the spawned children (export, workers) inherit.  The search reads $PATH as it was before
this front entry ($NV_FFMPEG_PATH0 while $PATH is still the value set here, $NV_FFMPEG_PATH_SET), as the C++ UI,
which never changes $PATH, does (N3 fixes verification I3: otherwise a directory set once and cleared again would
still be found through $PATH).
"""
import os

FFMPEG_DIRS = ("/opt/homebrew/bin", "/usr/local/bin", "/opt/local/bin")
NOT_FOUND_TITLE = "ffmpeg not found"
NOT_FOUND_TEXT = (
    "ffmpeg and ffprobe were not found.\n\nMeatVideo reads and writes video with them; they are not "
    "bundled. Install them (e.g. `brew install ffmpeg`), or name the ffmpeg binary in the environment "
    "variable NV_FFMPEG or as ffmpeg=<path> under [prefs] in ~/.config/NeatRepro/nv_ui_qt.ini, then "
    "start the app again.\n\nLooked in: " + ", ".join(FFMPEG_DIRS + ("PATH",)))
TOOLTIP = ("ffmpeg / ffprobe: a binary or the directory holding both (prefs/ffmpeg); empty = $NV_FFMPEG, "
           "/opt/homebrew/bin, /usr/local/bin, /opt/local/bin, PATH")


def missing_text(path):
    """The Preferences warning for a path without both tools (the C++ UI shows the same text)."""
    return (f"No ffmpeg and ffprobe at {path}.\n\nName the ffmpeg binary or the directory that holds ffmpeg and "
            "ffprobe, or leave the field empty: ffmpeg is then looked for in $NV_FFMPEG, /opt/homebrew/bin, "
            "/usr/local/bin, /opt/local/bin and PATH.")


def has_tools(d):
    return all(os.path.isfile(os.path.join(d, t)) and os.access(os.path.join(d, t), os.X_OK)
               for t in ("ffmpeg", "ffprobe"))


def tool_dir(v):
    """A binary or directory setting -> the directory to look in (~ expanded)."""
    v = os.path.expanduser(v)
    return os.path.dirname(v) if os.path.isfile(v) else v


def tools_in(v):
    """True when the setting v (a binary or a directory) names a directory holding both tools."""
    return bool(v) and has_tools(tool_dir(v))


def settings_ffmpeg():
    """prefs/ffmpeg of the UI's settings file ($NV_UI_QT_SETTINGS, else the user's nv_ui_qt.ini), or None."""
    path = os.environ.get("NV_UI_QT_SETTINGS")
    try:
        from PySide6.QtCore import QSettings
        s = (QSettings(path, QSettings.IniFormat) if path else
             QSettings(QSettings.IniFormat, QSettings.UserScope, "NeatRepro", "nv_ui_qt"))
        v = s.value("prefs/ffmpeg")
        return str(v) if v else None
    except Exception:       # noqa: BLE001 - no settings: the other places are searched
        return None


def find_ffmpeg_dir(pref=None, dirs=None, use_settings=True):
    """The directory holding ffmpeg and ffprobe, or None.  pref = the prefs/ffmpeg value to use (None: read the
    settings file when use_settings); dirs = the fixed places (None: FFMPEG_DIRS)."""
    if pref is None and use_settings:
        pref = settings_ffmpeg()
    cands = [tool_dir(v) for v in (os.environ.get("NV_FFMPEG"), pref) if v]
    cands += list(FFMPEG_DIRS if dirs is None else dirs)
    cands += [d for d in search_path().split(os.pathsep) if d]
    for d in cands:
        if has_tools(d):
            return os.path.abspath(d)
    return None


def search_path():
    """$PATH without the front entry use_dir put there (the $PATH before it, unless $PATH was changed since)."""
    path = os.environ.get("PATH", "")
    if "NV_FFMPEG_PATH0" in os.environ and os.environ.get("NV_FFMPEG_PATH_SET") == path:
        return os.environ["NV_FFMPEG_PATH0"]
    return path


def use_dir(d):
    """Make d (or nothing, for None / "") the ffmpeg of this process and its children."""
    os.environ["NV_FFMPEG_DIR"] = d or ""
    if d:
        base = search_path()
        path = [p for p in base.split(os.pathsep) if p and p != d]
        os.environ["PATH"] = os.pathsep.join([d] + path)
        os.environ["NV_FFMPEG_PATH0"] = base
        os.environ["NV_FFMPEG_PATH_SET"] = os.environ["PATH"]


def setup(dirs=None):
    """At start: search once per process tree (a child inherits $NV_FFMPEG_DIR and the $PATH of its parent)."""
    if "NV_FFMPEG_DIR" not in os.environ:
        use_dir(find_ffmpeg_dir(dirs=dirs))


def ffmpeg_dir():
    """The ffmpeg directory of this process: $NV_FFMPEG_DIR when set, else the search without the setting (as the
    C++ UI's ffmpeg_dir() before main sets it); "" = not found."""
    if "NV_FFMPEG_DIR" in os.environ:
        return os.environ["NV_FFMPEG_DIR"]
    return find_ffmpeg_dir(use_settings=False) or ""
