"""Adversarial state checks of Noise Level Check in the offscreen UI (writes nothing outside nlc_verify)."""
import os, sys, time, traceback
os.environ["QT_QPA_PLATFORM"] = "offscreen"
sys.path.insert(0, os.getcwd())
import numpy as np
from PySide6.QtWidgets import QApplication
excs = []
def hook(t, v, tb):
    excs.append("".join(traceback.format_exception(t, v, tb))); sys.__excepthook__(t, v, tb)
sys.excepthook = hook
def main():
    global app
    app = QApplication([sys.argv[0]])
    from nv_ui_qt.main_window import MainWindow, settings
    from nv_ui_qt import backend, display as D
    s = settings(); s.setValue("prefs/mem_gb", 0.04); s.setValue("ui/advanced", True); s.sync()
    video = sys.argv[1]
    win = MainWindow(video); win.show()
    st, vw, fpn = win.state, win.viewer, win.filter_page.panel
    idle = lambda: not win.runner.running and not win.runner.queued
    stale = {"frame": 0, "setting": 0, "samples": 0}
    def probe_inv():
        if vw.showing_nlc():
            stale["samples"] += 1
            sh = win._nlc_shown
            if sh is None or sh[0] != win.timeline.frame(): stale["frame"] += 1
            elif sh != win._nlc_key(): stale["setting"] += 1
    def wait(c, to=300):
        t0 = time.time()
        while not c():
            app.processEvents(); probe_inv(); time.sleep(0.005)
            if time.time() - t0 > to: return False
        for _ in range(5): app.processEvents(); probe_inv()
        return True
    res = []
    def check(name, ok, info=""):
        res.append((name, bool(ok))); print(("[PASS] " if ok else "[FAIL] ") + name, info, flush=True)
    rot = (win.info["rotation"] // 90) % 4 if win.info else 0
    def expected():
        t, P = win.timeline.frame(), st.backend()
        m = backend.PreviewEngine(win.frames, win.matrix, win.full_range).mask(t, st.effective_profile(), 0, P)
        return np.ascontiguousarray(np.rot90(D.nlc_image(win.frame_rgb(t), m), rot))
    def settled():
        return wait(lambda: idle() and not win.nlc_timer.isActive() and vw.showing_nlc() and win._nlc_shown == win._nlc_key())
    def synced(on):
        return (win.nlc == on and fpn.nl_check.button.isChecked() == on and win.fsc_nlc.isChecked() == on
                and win.actions["nlc"].isChecked() == on and win.fsc_btn.isChecked() == on
                and win.fsc_btn.text() == ("Noise Level Check" if on else "Filter Settings Check"))

    check("profile", wait(lambda: st.profile.base is not None and idle()), f"{len(win.frames)} frames")
    # 1. Begin from the Prepare page via toolbar menu entry
    win.tabs.setCurrentIndex(0)
    win.fsc_nlc.trigger()
    check("begin from Prepare -> Adjust page, Normal", synced(True) and win.page() == 1 and vw.mode() == "Normal")
    check("mask = independent expected", settled() and np.array_equal(vw._f["nlc"], expected()))
    # 2. rapid frame changes
    t0 = win.timeline.frame()
    for k in range(1, 7):
        win.timeline.set_frame(t0 + k, emit=True); app.processEvents(); probe_inv()
    ok = settled()
    check("rapid frames: final frame shown, equals expected", ok and win._nlc_shown[0] == t0 + 6 and np.array_equal(vw._f["nlc"], expected()))
    check("never showed another frame's mask", stale["frame"] == 0, str(stale))
    # 3. rapid settings changes
    for v in (-10, -30, -50, -60):
        st.new_edit(); fpn.controls["t_noise"].slider.setValue(v); app.processEvents(); probe_inv()
    ok = settled()
    check("rapid settings: final equals expected (t_noise 40)", ok and st.params["t_noise"] == 40.0 and np.array_equal(vw._f["nlc"], expected()))
    print("stale counts (setting stale = old-settings image shown while recomputing):", stale)
    # 4. frame change right after setting change
    st.set_param("t_noise", 70.0); win.timeline.set_frame(t0 + 2, emit=True)
    check("setting+frame change -> correct", settled() and np.array_equal(vw._f["nlc"], expected()))
    # 5. radius 0, Amount 0 (via state), enable off: mask all sepia (m=0)
    for k, v in (("t_radius", 0), ("t_amount", 0.0)):
        st.set_param(k, v); ok = settled()
        m0 = np.ascontiguousarray(np.rot90(D.nlc_image(win.frame_rgb(win.timeline.frame()), np.zeros(win.frames[0][0].shape, np.float32)), rot))
        check(f"{k}={v}: m = 0 image", ok and np.array_equal(vw._f["nlc"], m0))
        st.undo.undo(); settled()
    # 6. view mode change during NLC, hold
    vw.set_mode("YCrCb"); app.processEvents(); wait(idle)
    src_ycc = vw._source()
    vw.set_mode("Normal")
    vw.hold = True; held = vw._source(); vw.hold = False
    check("mode change / hold during NLC: no exception", not excs, f"src YCrCb={src_ycc} hold={held}")
    # 7. Beginner mode toggle during NLC
    win.actions["beginner"].trigger(); app.processEvents()
    beg = synced(True)
    win.actions["advanced"].trigger(); app.processEvents()
    check("beginner/advanced toggle keeps state", beg and synced(True) and not excs)
    # 8. Begin then Finish immediately (pending job) -> no mask appears later, preview resumes
    win.set_noise_level_check(False); st.set_param("t_noise", 55.0)
    win.set_noise_level_check(True); win.set_noise_level_check(False)
    wait(lambda: idle() and win.filt_cache.get((win.timeline.frame(), win.gen, win.crop())) is not None, 300)
    check("quick Begin/Finish: no mask after, filtered preview for current settings", not vw.showing_nlc() and vw._source() != "nlc"
          and synced(False) and (win.timeline.frame(), win.gen, win.crop()) in win.filt_cache)
    st.undo.undo()
    # 9. settings changed during NLC -> preview after Finish uses new settings
    win.set_noise_level_check(True); settled()
    st.set_param("t_noise", 30.0); settled()
    win.set_noise_level_check(False)
    key = (win.timeline.frame(), win.gen, win.crop())
    ok = wait(lambda: idle() and key in win.filt_cache)
    P = st.backend(); t = win.timeline.frame()
    eng = backend.PreviewEngine(win.frames, win.matrix, win.full_range)
    ref = eng.process(t, st.effective_profile(), 0, P, win.crop())[0]
    check("after Finish: filtered preview == fresh process at t_noise 30", ok and np.array_equal(win.filt_cache[key], ref))
    st.undo.undo()
    # 10. exclusivity with Filter Tuning Assist and Profile Check
    win.set_noise_level_check(True); win.set_assist(True)
    a1 = synced(False) and vw.assist
    win.actions["nlc"].trigger()
    a2 = synced(True) and not vw.assist
    check("exclusive with Filter Tuning Assist", a1 and a2)
    win.set_noise_level_check(False)
    win.tabs.setCurrentIndex(0); win.set_profile_check(True); pc = win.pcheck
    fpn.nl_check.button.click()     # panel (hidden page) Begin
    check("Profile Check ends when NLC begins (page change)", pc and not win.pcheck and synced(True) and win.page() == 1)
    # 11. toolbar Finish
    fin = [a for a in win.fsc_btn.menu().actions() if a.text().replace("&", "") == "Finish"]
    fin[0].trigger(); app.processEvents()
    check("toolbar menu Finish ends NLC", synced(False) and fpn.tabs.isTabEnabled(1))
    # 12. reload video during NLC
    win.set_noise_level_check(True); settled()
    win.open_video(video)
    ok = wait(lambda: idle() and st.profile.base is not None, 300)
    check("reload video ends NLC, panel restored", synced(False) and not vw.showing_nlc() and fpn.tabs.isTabEnabled(2)
          and fpn.controls["t_amount"].isEnabled(), f"loaded {ok}")
    # 13. reset profile during NLC, then trigger without profile
    win.tabs.setCurrentIndex(1); win.set_noise_level_check(True); settled()
    win.reset_profile(); app.processEvents()
    r1 = synced(False)
    win.set_noise_level_check(True); fpn.nl_check.button.click(); win.fsc_nlc.trigger(); app.processEvents()
    check("no profile: ends and cannot start", r1 and synced(False) and not vw.showing_nlc())
    wait(idle)
    check("no exceptions in slots", not excs, "\n".join(excs)[:3000])
    print(f"{sum(o for _, o in res)}/{len(res)}", "stale", stale)
    win.close()

if __name__ == '__main__':
    main()
