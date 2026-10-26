"""Test 1: standalone mode, driven off-screen (task items 1.1-1.6).

    cd repro && QT_QPA_PLATFORM=offscreen python3 <this> ../test.mp4 <outdir>
Writes only into outdir (settings INI, a 5-frame export + its JSON sidecars).
"""
import json
import os
import subprocess
import sys
import time

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
REPRO = "/Users/seitan/Desktop/work/reverse/repro"
sys.path.insert(0, REPRO)
RESULTS = []


def check(name, ok, detail=""):
    RESULTS.append((name, bool(ok)))
    print(f"[{'PASS' if ok else 'FAIL'}] {name} {detail}", flush=True)


def wait(app, cond, timeout=600.0):
    t0 = time.time()
    while not cond():
        app.processEvents()
        time.sleep(0.01)
        if time.time() - t0 > timeout:
            return False
    app.processEvents()
    return True


def main(video, outdir):
    os.makedirs(outdir, exist_ok=True)
    ini = os.path.join(outdir, "st1.ini")
    os.environ["NV_UI_QT_SETTINGS"] = ini
    if os.path.exists(ini):
        os.remove(ini)
    import numpy as np
    from PySide6.QtCore import Qt
    from PySide6.QtTest import QTest
    from PySide6.QtWidgets import QApplication
    app = QApplication([sys.argv[0]])
    from nv_ui_qt import backend
    from nv_ui_qt.main_window import MainWindow, settings
    s = settings()
    s.setValue("prefs/jobs", 3)
    s.setValue("prefs/crf", 0)
    s.sync()
    t0 = time.time()
    win = MainWindow(video)
    win.show()
    st = win.state
    idle = lambda: not win.runner.running and not win.runner.queued

    # 1.1 load
    ok = wait(app, lambda: win.frames is not None, 300)
    check("1.1 video loaded", ok and len(win.frames) > 0 and win.info["W"] > 0,
          f"frames {len(win.frames or [])}, W {win.info and win.info['W']}, H {win.info and win.info['H']}, "
          f"{time.time() - t0:.1f}s")
    # 1.2 Auto Profile (started automatically after load: prefs auto_profile = True)
    t1 = time.time()
    ok = wait(app, lambda: st.profile.base is not None, 300)
    b = st.profile.base
    check("1.2 Auto Profile builds a profile with flat_status", ok and b is not None and "flat_status" in b,
          f"flat_status {b and b.get('flat_status')}, area {b and b.get('area')}, frame {b and b.get('ui_frame')}, "
          f"{time.time() - t1:.1f}s")
    # 1.3 AFT runs automatically after the profile
    t1 = time.time()
    ok = wait(app, lambda: st.profile.base.get("aft_applied") and idle(), 300)
    check("1.3 AFT runs automatically after Auto Profile", ok and st.profile.base.get("aft_applied") is True,
          f"aft_applied {st.profile.base.get('aft_applied')}, {time.time() - t1:.1f}s; status '{win.msg.text()}'")
    # explicit Auto Profile button click -> profile rebuilt -> AFT again
    n_undo = st.undo.index()
    QTest.mouseClick(win.auto_btn, Qt.LeftButton)
    ok = wait(app, lambda: st.undo.index() >= n_undo + 2 and st.profile.base.get("aft_applied") and idle(), 300)
    check("1.2/1.3 Auto Profile button -> profile -> AFT", ok,
          f"undo entries {n_undo} -> {st.undo.index()}, frame {st.profile.base.get('ui_frame')}, "
          f"aft_applied {st.profile.base.get('aft_applied')}")

    # 1.4 slider -> preview recomputed
    win.tabs.setCurrentIndex(1)
    ok = wait(app, lambda: idle() and win.filt_cache and win.viewer.filt is not None, 300)
    check("1.4a filtered preview shown on Adjust and Preview", ok)
    gen0, keys0 = win.gen, set(win.filt_cache)
    filt0 = win.viewer.filt.copy()
    win.filter_page.panel.controls["amt_Y"].slider.setValue(20)
    ok = wait(app, lambda: idle() and win.gen > gen0 and any(k[1] == win.gen for k in win.filt_cache), 300)
    diff = None if not ok else float(np.abs(win.viewer.filt.astype(int) - filt0.astype(int)).mean())
    check("1.4 slider change recomputes the preview", ok and st.params["amt_Y"] == 20.0 and diff and diff > 0,
          f"gen {gen0} -> {win.gen}, amt_Y {st.params['amt_Y']}, mean |filtered change| {diff}")

    # 1.5 Apply: invalidates and recomputes the preview with the current settings
    gen1 = win.gen
    QTest.mouseClick(win.b_apply, Qt.LeftButton)
    ok = wait(app, lambda: idle() and win.gen > gen1 and any(k[1] == win.gen for k in win.filt_cache), 300)
    t = win.timeline.frame()
    ref_rgb, _ = win.engine.process(t, st.effective_profile(), win.prof_token, st.backend(), None)
    same = ok and np.array_equal(win.filt_cache[(t, win.gen, None)], ref_rgb)
    check("1.5 Apply recomputes the preview with the current settings", same,
          f"gen {gen1} -> {win.gen}; preview == engine.process(current params, amt_Y 20): {same}; "
          f"status '{win.msg.text()}'")

    # 1.6 Effect > Export Full Video (action triggered; save dialog replaced by a fixed path)
    dst = os.path.join(outdir, "export5.mp4")
    for p in (dst, dst[:-4] + ".json", dst[:-4] + ".ui_params.json", dst[:-4] + ".ui_profile.json"):
        if os.path.exists(p):
            os.remove(p)
    win._ask_save = lambda *a: dst
    act = win.actions["export"]
    menu_ok = any(a is act for m in win.menuBar().actions() if m.text().replace("&", "") == "Effect"
                  for a in m.menu().actions())
    # the action exports the whole clip; limit to 5 frames by wrapping process_video
    orig = win.process_video
    win.process_video = lambda *a, **k: orig(dst, 0, 5)
    act.triggered.disconnect()
    act.triggered.connect(lambda: win.process_video())
    t1 = time.time()
    act.trigger()
    ok = wait(app, lambda: win.export is None and hasattr(win, "export_ok"), 600)
    n = subprocess.run(["ffprobe", "-v", "error", "-count_frames", "-select_streams", "v:0", "-show_entries",
                        "stream=nb_read_frames,width,height,pix_fmt,codec_name", "-of", "json", dst],
                       capture_output=True, text=True)
    info = json.loads(n.stdout or "{}").get("streams", [{}])[0]
    check("1.6 Effect > Export Full Video writes an mp4 (5 frames)",
          ok and win.export_ok and info.get("nb_read_frames") == "5",
          f"in Effect menu {menu_ok}; {info}; {os.path.getsize(dst) if os.path.exists(dst) else 0} bytes, "
          f"{time.time() - t1:.1f}s; status '{win.msg.text()}'")
    # export frame 0 == backend.process_frame (CRF 0 = lossless)
    import nvfilter as nv
    from denoise_video_v5 import read_frames
    exp = [f for _, f in read_frames(dst, win.info["W"], win.info["H"], 0, 5, win.info["fps"])]
    P = st.backend()
    R = int(P["radius"])
    prof = st.effective_profile()
    eq = []
    for t in (0, 4):
        idx, planes = win.engine.window(t, R, None)
        ref = backend.process_frame(planes, idx.index(t), prof, P)
        eq.append(all(np.array_equal(np.clip(np.rint(r), 0, 255).astype(np.uint8), np.asarray(e))
                      for r, e in zip(ref, exp[t])))
    check("1.6b exported frames 0 and 4 == backend.process_frame (CRF 0)", all(eq), f"{eq}")
    win.close()
    app.processEvents()
    npass = sum(ok for _, ok in RESULTS)
    print(f"{npass}/{len(RESULTS)} PASS", flush=True)
    return 0 if npass == len(RESULTS) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2]))
