"""UI off-screen with non-finite float input: load at 32 bits, profile, preview finite, export restores."""
import os, sys, time, subprocess
os.environ["QT_QPA_PLATFORM"] = "offscreen"
OUT = os.path.dirname(os.path.abspath(__file__))
os.environ["NV_UI_QT_SETTINGS"] = os.path.join(OUT, "settings.ini")
sys.path.insert(0, os.getcwd())
import numpy as np
NAN = os.path.abspath(os.path.join(OUT, "..", "nan"))
def main():
    if os.path.exists(os.environ["NV_UI_QT_SETTINGS"]): os.remove(os.environ["NV_UI_QT_SETTINGS"])
    from PySide6.QtWidgets import QApplication
    app = QApplication([sys.argv[0]])
    from nv_ui_qt.main_window import MainWindow, settings
    s = settings(); s.setValue("prefs/jobs", 2); s.setValue("prefs/depth", "32"); s.setValue("prefs/crf", 0); s.sync()
    def wait(c, t=600):
        t0 = time.time()
        while not c():
            app.processEvents(); time.sleep(0.01)
            if time.time() - t0 > t: return False
        app.processEvents(); return True
    def res(n, ok, d=""): print(("[PASS] " if ok else "[FAIL] ") + n, d, flush=True)
    for name in ("scatter", "whole_plane"):
        src = os.path.join(NAN, name + "_in.mkv")
        win = MainWindow(src); win.show()
        idle = lambda: not win.runner.running and not win.runner.queued
        ok = wait(lambda: win.frames is not None and win.state.profile.base is not None and idle())
        fin = all(np.isfinite(p).all() for f in win.frames for p in f)
        prof = win.state.effective_profile()
        sig = prof and prof["sigma"]
        res(f"{name}: load 32-bit + profile, frames finite in memory", ok and fin and all(np.isfinite(v) for v in sig.values()), f"fmt {win.fmt.name} sigma {sig}")
        win.timeline.set_frame(3); win.tabs.setCurrentIndex(1); win.invalidate(); win.schedule()
        ok = wait(lambda: idle() and win.viewer.filt is not None, 300)
        res(f"{name}: preview of frame 3 computed", ok and win.viewer.filt is not None,
            f"filt {None if win.viewer.filt is None else (win.viewer.filt.dtype, win.viewer.filt.shape)}")
        dst = os.path.join(OUT, f"{name}_ui.mkv")
        win.process_video(dst, start=2, frames=3); ok = wait(lambda: win.export is None, 900)
        W, H = 256, 192
        dec = lambda p: np.frombuffer(subprocess.run(["ffmpeg", "-v", "error", "-i", p, "-f", "rawvideo", "-pix_fmt", "gbrpf32le", "-"], capture_output=True).stdout, "<f4").reshape(-1, 3, H, W)
        o, i = dec(dst), dec(src)[2:5]
        nf = ~np.isfinite(i)
        res(f"{name}: UI export restores non-finite, rest finite", ok and getattr(win, "export_ok", False) and o.shape == i.shape
            and np.array_equal(i.view("<u4")[nf], o.view("<u4")[nf]) and np.isfinite(o[~nf]).all(),
            f"in_nf {int(nf.sum())} out_nf {int((~np.isfinite(o)).sum())}")
        win.close(); app.processEvents()
if __name__ == "__main__":
    main()
