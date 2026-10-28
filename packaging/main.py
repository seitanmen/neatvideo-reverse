"""MeatVideo.app launcher (PyInstaller entry, N2b).

    MeatVideo [video]                     the Qt UI (python3 -m nv_ui_qt.main [video])
    MeatVideo --ofx W H ...               OFX host mode (nv_ui_qt.main, unchanged)
    MeatVideo -m nv_ui_qt.export ARGS     the export process the UI starts (worker.ExportProcess runs
                                               sys.executable -m nv_ui_qt.export; frozen, sys.executable is
                                               this launcher, which cannot take -m)
    MeatVideo --selftest VIDEO OUTDIR     nv_ui_qt.selftest with the bundled Python (offscreen by default);
                                               prints nv_native.STATS at the end
    MeatVideo --info                      bundle diagnostics (library, ref_floor, versions, ffmpeg) as JSON

Done here so that repro/ is not changed (every process, including the spawned multiprocessing children,
runs this file first):
  - multiprocessing.freeze_support() (the AFT child of backend.run_in_child and the frame workers of
    denoise_video_v5 are spawned: frozen, they re-run this executable);
  - ffmpeg / ffprobe (not bundled, GPL): the first directory holding both of $NV_FFMPEG (the binary or its
    directory), the QSettings key prefs/ffmpeg if it was set by hand, /opt/homebrew/bin, /usr/local/bin,
    /opt/local/bin, then $PATH goes to the front of $PATH (a Finder launch has only /usr/bin:/bin:...).
    repro calls "ffmpeg" / "ffprobe" by name and the children inherit the environment;
  - data/ref_floor.npy: nv_profile.reference_floor() looks for <dir of repro>/../data, which frozen is outside
    the bundle; its default argument is pointed at sys._MEIPASS/data/ref_floor.npy (else ~7 s recomputation).
"""
import os
import sys

FFMPEG_DIRS = ("/opt/homebrew/bin", "/usr/local/bin", "/opt/local/bin")
FROZEN = getattr(sys, "frozen", False)
BASE = getattr(sys, "_MEIPASS", None) or os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
REF_FLOOR = os.path.join(BASE, "data", "ref_floor.npy")
if not FROZEN:      # python3 packaging/main.py ... (checks of the launcher without a build)
    for _p in (os.path.join(BASE, "neatvideo_ofx_614", "analysis", "noise", "aft_port", "src"),
               os.path.join(BASE, "repro")):
        if _p not in sys.path:
            sys.path.insert(0, _p)


def _has_tools(d):
    return all(os.path.isfile(os.path.join(d, t)) and os.access(os.path.join(d, t), os.X_OK)
               for t in ("ffmpeg", "ffprobe"))


def _settings_ffmpeg():
    """prefs/ffmpeg of the UI's QSettings file (read only; the UI has no control for it)."""
    path = os.environ.get("NV_UI_QT_SETTINGS")
    try:
        from PySide6.QtCore import QSettings
        s = (QSettings(path, QSettings.IniFormat) if path else
             QSettings(QSettings.IniFormat, QSettings.UserScope, "NeatRepro", "nv_ui_qt"))
        v = s.value("prefs/ffmpeg")
        return str(v) if v else None
    except Exception:
        return None


def find_ffmpeg_dir():
    cands = []
    for v in (os.environ.get("NV_FFMPEG"), _settings_ffmpeg()):
        if v:
            v = os.path.expanduser(v)
            cands.append(os.path.dirname(v) if os.path.isfile(v) else v)
    cands += FFMPEG_DIRS
    cands += [d for d in os.environ.get("PATH", "").split(os.pathsep) if d]
    for d in cands:
        if _has_tools(d):
            return os.path.abspath(d)
    return None


def _setup_env():
    if "NV_FFMPEG_DIR" not in os.environ:        # once per process tree: children inherit the result
        d = find_ffmpeg_dir()
        os.environ["NV_FFMPEG_DIR"] = d or ""
        if d:
            path = [p for p in os.environ.get("PATH", "").split(os.pathsep) if p and p != d]
            os.environ["PATH"] = os.pathsep.join([d] + path)


def _setup_ref_floor():
    if os.path.isfile(REF_FLOOR):
        import nv_profile
        nv_profile.reference_floor.__defaults__ = (REF_FLOOR,)


def _info():
    import json
    import time
    import numpy
    import scipy
    import PySide6
    import nv_curve
    import nv_native
    import nv_profile
    from nv_ui_qt import backend
    t = time.perf_counter()
    floor = nv_profile.reference_floor()
    dt = time.perf_counter() - t
    ref_ok = None
    if os.path.isfile(REF_FLOOR):
        ref_ok = bool(numpy.array_equal(numpy.load(REF_FLOOR).astype(numpy.float32).view(numpy.uint32),
                                        floor.view(numpy.uint32)))
    # a spawned child (as backend.run_in_child / the export's frame workers): it re-runs this launcher
    import multiprocessing
    with multiprocessing.get_context("spawn").Pool(1) as pool:
        t = time.perf_counter()
        child_defaults = pool.apply(getattr, (nv_profile.reference_floor, "__defaults__"))
        child_start = time.perf_counter() - t
        t = time.perf_counter()
        child_floor = pool.apply(nv_profile.reference_floor)
        child_floor_s = time.perf_counter() - t
        child_ffmpeg = pool.apply(os.getenv, ("NV_FFMPEG_DIR",))
    child = {"start_seconds": round(child_start, 3), "ref_floor_path": child_defaults[0],
             "ref_floor_seconds": round(child_floor_s, 4),
             "ref_floor_equals": bool(numpy.array_equal(child_floor.view(numpy.uint32), floor.view(numpy.uint32))),
             "ffmpeg_dir": child_ffmpeg or None}
    print(json.dumps({
        "frozen": FROZEN, "meipass": getattr(sys, "_MEIPASS", None), "executable": sys.executable,
        "python": sys.version.split()[0], "numpy": numpy.__version__, "scipy": scipy.__version__,
        "pyside6": PySide6.__version__,
        "native_available": nv_native.AVAILABLE, "native_lib": nv_native.LIB_PATH,
        "native_enabled": backend.native_enabled(), "native_threads": nv_native.THREADS,
        "ref_floor_path": nv_profile.reference_floor.__defaults__[0], "ref_floor_file_exists":
            os.path.isfile(nv_profile.reference_floor.__defaults__[0]),
        "ref_floor_seconds": round(dt, 4), "ref_floor_equals_file": ref_ok,
        "libsystem_pow": type(nv_curve._pow).__name__,
        "ffmpeg_dir": os.environ.get("NV_FFMPEG_DIR") or None,
        "repro_file": nv_profile.__file__, "child": child,
    }, indent=1))
    return 0


def _selftest(video, outdir):
    import time
    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    import nv_native
    from nv_ui_qt import backend, main_window, selftest
    done = main_window.MainWindow._export_done

    def _export_done(self, ok, text):           # the UI shows the export's error only in the status bar
        if not ok:
            print(f"[export failed] {text}", flush=True)
        return done(self, ok, text)
    main_window.MainWindow._export_done = _export_done
    t = time.perf_counter()
    rc = selftest.main(video, outdir)
    print(f"elapsed {time.perf_counter() - t:.0f} s; native_enabled {backend.native_enabled()}; "
          f"STATS {nv_native.STATS}", flush=True)
    return rc


def _ui(args):
    from PySide6.QtWidgets import QApplication, QMessageBox
    from nv_ui_qt.main_window import MainWindow
    app = QApplication.instance() or QApplication([sys.argv[0]] + args)
    app.setApplicationName("nv_ui_qt")
    if not os.environ.get("NV_FFMPEG_DIR"):
        QMessageBox.critical(
            None, "ffmpeg not found",
            "ffmpeg and ffprobe were not found.\n\nMeatVideo reads and writes video with them; they are not "
            "bundled. Install them (e.g. `brew install ffmpeg`), or name the ffmpeg binary in the environment "
            "variable NV_FFMPEG or as ffmpeg=<path> under [prefs] in ~/.config/NeatRepro/nv_ui_qt.ini, then "
            "start the app again.\n\nLooked in: " + ", ".join(FFMPEG_DIRS + ("PATH",)))
        args = []                   # opening a video would fail in ffprobe
    win = MainWindow(args[0] if args else None)
    win.show()
    if os.environ.get("NV_LAUNCH_TIMING"):      # startup time: launch to the first event-loop turn
        from PySide6.QtCore import QTimer

        def _report():
            import time
            print(f"startup {time.time() - float(os.environ['NV_LAUNCH_TIMING']):.3f} s", flush=True)
            app.quit()
        QTimer.singleShot(0, _report)
    return app.exec()


def main(argv):
    args = argv[1:]
    if args[:2] == ["-m", "nv_ui_qt.export"]:
        from nv_ui_qt import export
        export.main(args[2:])
        return 0
    if args[:1] == ["--selftest"]:
        if len(args) != 3:
            print("usage: --selftest VIDEO OUTDIR", file=sys.stderr)
            return 2
        # absolute: the export process runs in another working directory (main_window.REPRO)
        return _selftest(os.path.abspath(args[1]), os.path.abspath(args[2]))
    if args[:1] == ["--info"]:
        return _info()
    if args[:1] == ["--ofx"]:
        from nv_ui_qt import main as ui_main
        return ui_main.main(argv)
    # Finder may pass -psn_0_xxxx (old macOS); drop it
    args = [os.path.abspath(a) for a in args if not a.startswith("-psn_")]
    return _ui(args)


if __name__ == "__main__":
    _setup_env()
    _setup_ref_floor()                   # before freeze_support: the AFT child builds a profile too
    import multiprocessing
    multiprocessing.freeze_support()     # a spawned child runs its task here and exits
    sys.exit(main(sys.argv))
