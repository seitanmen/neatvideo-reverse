"""Checks of packaging/main.py's ffmpeg search and its "ffmpeg not found" dialog with the development
Python (offscreen; no window is shown). Usage: python3 packaging/test_launcher.py WORKDIR
Fake ffmpeg / ffprobe scripts are written under WORKDIR; nothing runs them."""
import importlib.util
import os
import sys

os.environ["QT_QPA_PLATFORM"] = "offscreen"
HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location("nvlauncher", os.path.join(HERE, "main.py"))
L = importlib.util.module_from_spec(spec)
spec.loader.exec_module(L)
RESULTS = []


def check(name, ok, detail=""):
    RESULTS.append(ok)
    print(f"[{'PASS' if ok else 'FAIL'}] {name} {detail}", flush=True)


def fake_tools(d):
    os.makedirs(d, exist_ok=True)
    for t in ("ffmpeg", "ffprobe"):
        p = os.path.join(d, t)
        with open(p, "w") as fh:
            fh.write("#!/bin/sh\nexit 1\n")
        os.chmod(p, 0o755)
    return os.path.abspath(d)


def main(work):
    env0 = dict(os.environ)
    a, b, c = (fake_tools(os.path.join(work, n)) for n in ("env_dir", "prefs_dir", "path_dir"))
    ini = os.path.join(work, "settings.ini")
    with open(ini, "w") as fh:
        fh.write(f"[prefs]\nffmpeg={b}/ffmpeg\n")
    os.environ.update(NV_UI_QT_SETTINGS=ini, NV_FFMPEG=a, PATH=f"{c}:/usr/bin:/bin")
    check("NV_FFMPEG (directory) first", L.find_ffmpeg_dir() == a, L.find_ffmpeg_dir())
    os.environ["NV_FFMPEG"] = os.path.join(a, "ffmpeg")
    check("NV_FFMPEG (binary) first", L.find_ffmpeg_dir() == a, L.find_ffmpeg_dir())
    del os.environ["NV_FFMPEG"]
    check("QSettings prefs/ffmpeg next", L.find_ffmpeg_dir() == b, L.find_ffmpeg_dir())
    os.remove(ini)
    L.FFMPEG_DIRS = ()
    check("PATH last", L.find_ffmpeg_dir() == c, L.find_ffmpeg_dir())
    os.environ["PATH"] = "/usr/bin:/bin"
    check("nothing found -> None", L.find_ffmpeg_dir() is None, L.find_ffmpeg_dir())

    os.environ.pop("NV_FFMPEG_DIR", None)
    L._setup_env()
    check("_setup_env records the miss", os.environ.get("NV_FFMPEG_DIR") == "")
    os.environ.pop("NV_FFMPEG_DIR")
    os.environ["PATH"] = f"/usr/bin:{c}:/bin"
    L._setup_env()
    check("_setup_env puts the directory first in PATH",
          os.environ["NV_FFMPEG_DIR"] == c and os.environ["PATH"].split(":")[:2] == [c, "/usr/bin"],
          os.environ["PATH"])

    # the dialog: shown over the main window when ffmpeg is missing; closed from a timer
    os.environ["NV_FFMPEG_DIR"] = ""
    os.environ["NV_UI_QT_SETTINGS"] = ini
    seen = []
    from PySide6.QtCore import QTimer
    from PySide6.QtWidgets import QApplication, QMessageBox

    def close_box():
        app = QApplication.instance()
        for w in app.topLevelWidgets():
            if isinstance(w, QMessageBox) and w.isVisible():
                seen.append((w.windowTitle(), w.text()))
                w.accept()
                QTimer.singleShot(0, app.quit)
                return
        QTimer.singleShot(50, close_box)

    _app = QApplication([sys.argv[0]])    # _ui uses it (a timer needs the application first)
    QTimer.singleShot(200, close_box)
    rc = L._ui([])
    check("ffmpeg-not-found dialog shown", rc == 0 and len(seen) == 1 and seen[0][1].startswith("ffmpeg and ffprobe were not found")
          and "brew install ffmpeg" in seen[0][1], seen[:1])
    # the PySide6 UI's own start (nv_ui_qt.main, ffmpeg_path.py): the same search and the same dialog (G2)
    from nv_ui_qt import ffmpeg_path as FP
    from nv_ui_qt import main as M
    a2 = fake_tools(os.path.join(work, "env_dir2"))
    with open(ini, "w") as fh:
        fh.write(f"[prefs]\nffmpeg={b}/ffmpeg\n")
    os.environ.update(NV_FFMPEG=a2, PATH=f"{c}:/usr/bin:/bin")
    check("ffmpeg_path: NV_FFMPEG first", FP.find_ffmpeg_dir() == a2 == L.find_ffmpeg_dir(), FP.find_ffmpeg_dir())
    del os.environ["NV_FFMPEG"]
    check("ffmpeg_path: prefs/ffmpeg next", FP.find_ffmpeg_dir() == b == L.find_ffmpeg_dir(), FP.find_ffmpeg_dir())
    FP.FFMPEG_DIRS = ()
    os.remove(ini)
    check("ffmpeg_path: PATH last", FP.find_ffmpeg_dir() == c, FP.find_ffmpeg_dir())
    # I3: a directory set in Preferences and cleared again is not found through the PATH front entry use_dir added
    os.environ["PATH"] = f"/usr/bin:{c}:/bin"
    for k in ("NV_FFMPEG_DIR", "NV_FFMPEG_PATH0", "NV_FFMPEG_PATH_SET"):
        os.environ.pop(k, None)
    FP.setup()
    FP.use_dir(FP.find_ffmpeg_dir(pref=b))
    back = FP.find_ffmpeg_dir(pref="")
    check("ffmpeg_path: the search ignores its own PATH front entry (setting cleared -> PATH again)",
          back == c and os.environ["PATH"].split(":")[0] == b, f"{back} {os.environ['PATH']}")
    os.environ["PATH"] = "/usr/bin:/bin"
    os.environ.pop("NV_FFMPEG_DIR", None)
    seen.clear()
    QTimer.singleShot(200, close_box)
    rc = M.main([sys.argv[0]])
    check("nv_ui_qt.main: the same ffmpeg-not-found dialog", rc == 0 and len(seen) == 1 and seen[0][1] == FP.NOT_FOUND_TEXT
          and os.environ.get("NV_FFMPEG_DIR") == "", seen[:1])
    os.environ.clear()
    os.environ.update(env0)
    n_fail = RESULTS.count(False)
    print(f"{len(RESULTS) - n_fail}/{len(RESULTS)} passed")
    return 1 if n_fail else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
