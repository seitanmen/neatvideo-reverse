"""Verifier's off-screen UI probe (run from repro/): memory cap with real bytes, status text for RGB float,
profile chroma-format mismatch, EXR export + cancel clean-up, export dialog extension."""
import glob, json, os, sys, time
os.environ["QT_QPA_PLATFORM"] = "offscreen"
OUT = os.path.dirname(os.path.abspath(__file__))
os.environ["NV_UI_QT_SETTINGS"] = os.path.join(OUT, "settings.ini")
if os.path.exists(os.environ["NV_UI_QT_SETTINGS"]): os.remove(os.environ["NV_UI_QT_SETTINGS"])
sys.path.insert(0, os.getcwd())
import numpy as np
def main():
    from PySide6.QtWidgets import QApplication
    global app
    app = QApplication([sys.argv[0]])
    from nv_ui_qt.main_window import MainWindow, settings
    from nv_ui_qt.state import dump_json
    import nv_depth as nd
    SYN = os.path.abspath(os.path.join(OUT, "..", "..", "syn"))
    global wait, res
    def wait(cond, timeout=600):
        t0 = time.time()
        while not cond():
            app.processEvents(); time.sleep(0.01)
            if time.time() - t0 > timeout: return False
        app.processEvents(); return True
    def res(name, ok, detail=""):
        print(("[PASS] " if ok else "[FAIL] ") + name, detail, flush=True)
    s = settings(); s.setValue("prefs/jobs", 2); s.setValue("prefs/depth", "16"); s.setValue("prefs/mem_gb", 0.004); s.setValue("prefs/auto_profile", True); s.sync()
    win = MainWindow(os.path.join(SYN, "y444p16.mkv")); win.show()
    msgs = []
    _say = win.say
    win.say = lambda t, *a, **k: (msgs.append(str(t)), _say(t, *a, **k))[1]
    idle = lambda: not win.runner.running and not win.runner.queued
    ok = wait(lambda: win.frames is not None and win.state.profile.base is not None and idle())
    f = win.fmt
    exp_cap = max(3, int(0.004 * 2 ** 30 // f.internal_bytes(288, 256)))
    res("16-bit 4:4:4 load", ok and f.name == "yuv444p16le", f"{f and f.name} prefs depth={win.prefs['depth']!r} mem={win.prefs['mem_gb']!r}")
    res("memory cap uses real bytes", len(win.frames) == exp_cap, f"frames {len(win.frames)} expected {exp_cap} (internal {f.internal_bytes(288, 256)} B/frame; 8-bit formula would give {max(3, int(0.004 * 2**30 // (288*256*3//2)))})")
    res("frame dtype/shape", win.frames[0][0].dtype == np.float32 and win.frames[0][1].shape == (256, 288), str([p.shape for p in win.frames[0]]))
    nbytes = sum(p.nbytes for p in win.frames[0]); res("internal_bytes == actual nbytes", nbytes == f.internal_bytes(288, 256), f"{nbytes}")
    res("status bar", "yuv444p16le (16-bit 4:4:4)" in win.depth_lbl.text() and "processing 16-bit" in win.depth_lbl.text(), win.depth_lbl.text())
    res("Performance Frame Bitdepth shows 16-bit", "16-bit (yuv444p16le)" in win.filter_page.panel.perf.text(), [l for l in win.filter_page.panel.perf.text().split("<tr>") if "Bitdepth" in l][:1])
    res("export dialog default .mkv for 16-bit", win._export_filter()[1] == ".mkv", str(win._export_filter()))
    prof16 = win.state.profile.to_json(); res("profile sub 1 at 4:4:4", win.state.effective_profile()["sub"] == 1)
    # 8-bit profile of the same video (4:2:0) -> load into the 16-bit 4:4:4 session
    dlg = win.preferences()
    from PySide6.QtWidgets import QComboBox
    combo = [c for c in dlg.findChildren(QComboBox) if c.findData("8") >= 0 and c.findData("source") >= 0][0]
    combo.setCurrentIndex(combo.findData("8")); dlg.accept()
    ok = wait(lambda: win.fmt is not None and win.fmt.legacy and win.state.profile.base is not None and idle())
    res("switch to 8-bit reloads as yuv420p uint8", ok and win.frames[0][0].dtype == np.uint8, f"{win.fmt.name} frames {len(win.frames)}")
    p8 = os.path.join(OUT, "prof8.nvprof.json"); dump_json(win.state.profile.to_json(), p8)
    dlg = win.preferences(); combo = [c for c in dlg.findChildren(QComboBox) if c.findData("8") >= 0 and c.findData("source") >= 0][0]
    combo.setCurrentIndex(combo.findData("16")); dlg.accept()
    ok = wait(lambda: win.fmt is not None and win.fmt.bits == 16 and win.state.profile.base is not None and idle())
    win.load_profile(p8); wait(idle)
    res("mismatched profile flagged in status", "chroma format differs" in win.prof_lbl.text(), win.prof_lbl.text())
    win.tabs.setCurrentIndex(1); win.invalidate(); win.schedule(); wait(idle, 120)
    res("UI alive after preview with mismatched profile", True, f"viewer.filt {None if win.viewer.filt is None else win.viewer.filt.shape}; msgs: {msgs[-3:]}")
    dst = os.path.join(OUT, "mismatch.mkv")
    win.process_video(dst, start=0, frames=2); wait(lambda: win.export is None, 600)
    res("export with mismatched profile refused, no video file", not getattr(win, "export_ok", True) and not os.path.exists(dst),
        f"export_ok={getattr(win, 'export_ok', None)} files={sorted(os.path.basename(p) for p in glob.glob(os.path.join(OUT, 'mismatch*')))} msgs={[m[-200:] for m in msgs[-2:]]}")
    # 32-bit float RGB session, EXR export + cancel
    s.setValue("prefs/mem_gb", 0.05); s.sync(); win.prefs["mem_gb"] = 0.05
    dlg = win.preferences(); combo = [c for c in dlg.findChildren(QComboBox) if c.findData("8") >= 0 and c.findData("source") >= 0][0]
    combo.setCurrentIndex(combo.findData("32")); dlg.accept()
    win.open_video(os.path.abspath(os.path.join(os.getcwd(), "..", "test.mp4")))
    ok = wait(lambda: win.src.endswith("test.mp4") and win.fmt is not None and win.fmt.is_float and win.state.profile.base is not None and idle())
    res("32-bit load of 8-bit yuv source", ok and win.fmt.name == "gbrpf32le", f"{win.depth_lbl.text()} frames {len(win.frames)}")
    res("export dialog for 32-bit offers EXR", "EXR" in win._export_filter()[0] and win._export_filter()[1] == ".mkv", win._export_filter()[0])
    os.makedirs(os.path.join(OUT, "exr"), exist_ok=True)
    keep = os.path.join(OUT, "exr", "unrelated_00001.exr"); open(keep, "w").write("x")
    dst = os.path.join(OUT, "exr", "cx_%05d.exr")
    win.process_video(dst)                       # full video -> cancel once frames appear
    seen = wait(lambda: len(glob.glob(os.path.join(OUT, "exr", "cx_*.exr"))) >= 3 or win.export is None, 600)
    n_before = len(glob.glob(os.path.join(OUT, "exr", "cx_*.exr")))
    win.cancel_export(); wait(lambda: win.export is None, 120); time.sleep(1.0); app.processEvents()
    left = sorted(os.listdir(os.path.join(OUT, "exr")))
    res("EXR export cancel removes its frames and json", n_before >= 3 and left == ["unrelated_00001.exr"], f"frames before cancel {n_before}; left {left[:6]}{'...' if len(left) > 6 else ''}")
    win.close(); app.processEvents()

if __name__ == '__main__':
    main()
