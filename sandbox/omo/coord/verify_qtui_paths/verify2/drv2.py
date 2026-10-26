"""Independent host driver for `python3 -m nv_ui_qt.main --ofx ...` (runpy, offscreen).
usage (cwd anywhere): ffmpeg ... | python3 drv2.py SCEN OUTDIR W H FPS [--depth D] > OUTDIR/stdout.bin 2> OUTDIR/stderr.log
Steps log (JSON on fd 3 file OUTDIR/steps.json): name, stdout size after the step, ref planes file.
"""
import json, os, sys, time
os.environ["QT_QPA_PLATFORM"] = "offscreen"
REPRO = "/Users/seitan/Desktop/work/reverse/repro"
sys.path.insert(0, REPRO)
os.chdir(REPRO)
T0 = time.time()
STEPS = []


def main():
    scen, outdir = sys.argv[1], sys.argv[2]
    rest = sys.argv[3:]
    os.makedirs(outdir, exist_ok=True)
    os.environ["NV_UI_QT_SETTINGS"] = os.path.join(outdir, "st.ini")
    import numpy as np
    from PySide6.QtCore import QTimer, Qt
    from PySide6.QtTest import QTest
    from PySide6.QtWidgets import QApplication
    from nv_ui_qt import backend
    from nv_ui_qt.main_window import MainWindow

    orig_show = MainWindow.show
    holder = []

    def show(win):
        orig_show(win)
        if not holder:
            holder.append(win)
            QTimer.singleShot(0, lambda: drive(win))
    MainWindow.show = show

    def fsz():
        return os.fstat(1).st_size

    def step(name, win, ref=True, **kw):
        d = dict(name=name, size=fsz(), t=round(time.time() - T0, 2), frame=win.timeline.frame(),
                 status=win.msg.text(), **kw)
        if ref:
            t = win.timeline.frame()
            P = win.state.backend(); eff = win.state.effective_profile()
            if eff is not None:
                idx, planes = win.engine.window(t, int(P["radius"]), None)
                out = backend.process_frame(planes, idx.index(t), eff, P)
                fn = os.path.join(outdir, f"ref_{len(STEPS)}.npz")
                np.savez(fn, *[np.asarray(p, np.float32) for p in out])
                d["ref"] = fn
        prev = win.filt_cache.get((win.timeline.frame(), win.gen, None))
        if prev is not None:
            fn = os.path.join(outdir, f"prev_{len(STEPS)}.npy")
            np.save(fn, prev); d["prev"] = fn
        STEPS.append(d)
        with open(os.path.join(outdir, "steps.json"), "w") as f:
            json.dump(STEPS, f, indent=1, default=str)

    def drive(win):
        app = QApplication.instance(); st = win.state
        idle = lambda: not win.runner.running and not win.runner.queued

        def wait(cond, timeout=300):
            t0 = time.time()
            while not cond():
                app.processEvents(); time.sleep(0.01)
                if time.time() - t0 > timeout:
                    return False
            app.processEvents(); return True

        def wait_prev():
            t = win.timeline.frame()
            return wait(lambda: idle() and (t, win.gen, None) in win.filt_cache)

        a = win.actions
        STEPS.append(dict(name="started", size=fsz(), frames=len(win.frames), shape=[list(p.shape) for p in win.frames[0]],
                          open=a["open"].isEnabled(), export=a["export"].isEnabled(), load=win.load_btn.isEnabled(),
                          generic=win.generic_btn.isEnabled(), frame=win.timeline.frame(), prof=st.profile.base is not None))
        a["open"].trigger(); a["export"].trigger()
        win.tabs.setCurrentIndex(1); app.processEvents()
        apply = lambda: QTest.mouseClick(win.b_apply, Qt.LeftButton)
        auto = lambda: QTest.mouseClick(win.auto_btn, Qt.LeftButton)
        slider = lambda v: win.filter_page.panel.controls["amt_Y"].slider.setValue(v)

        def do_apply(name, preview=True):
            apply(); wait(idle)
            if preview: wait_prev()
            step(name, win)

        def full_profile():
            auto(); wait(lambda: st.profile.base is not None)
            wait(lambda: st.profile.base.get("aft_applied") and idle())
            wait_prev()
            step("profile+aft", win, ref=False, aft=bool(st.profile.base.get("aft_applied")), flat=st.profile.base.get("flat_status"))

        if scen == "seq":            # apply, apply(same), change, close
            full_profile(); do_apply("apply1"); do_apply("apply2_same")
            slider(20); wait(lambda: idle() and any(k[1] == win.gen for k in win.filt_cache)); step("slider20", win, ref=False)
        elif scen == "race":         # Apply right after the profile appears, before AFT finished
            auto(); wait(lambda: st.profile.base is not None)
            pre = not st.profile.base.get("aft_applied")
            apply(); step("apply_pre_aft", win, pre_aft=pre, aft_busy=win.runner.busy("aft"))
            wait(lambda: st.profile.base.get("aft_applied") and idle()); wait_prev()
            step("aft_done", win, ref=False)
            do_apply("apply_post_aft"); do_apply("apply_post_aft_same")
        elif scen == "race2":        # Apply during AFT, AFT finishes, then only close -> close must write the post-AFT frame
            auto(); wait(lambda: st.profile.base is not None)
            pre = not st.profile.base.get("aft_applied")
            apply(); step("apply_pre_aft", win, pre_aft=pre, aft_busy=win.runner.busy("aft"))
            wait(lambda: st.profile.base.get("aft_applied") and idle()); wait_prev()
            step("aft_done", win, ref=False)
        elif scen == "noprof":       # Apply with no UI profile, then make a profile, Apply again
            apply(); wait(idle); step("apply_noprof", win, ref=False, prof=st.profile.base is not None)
            apply(); step("apply_noprof_same", win, ref=False)
            full_profile(); do_apply("apply_after_profile")
        elif scen == "frames":
            full_profile(); do_apply("apply_t0")
            t0 = win.timeline.frame(); t1 = t0 + 2
            win.timeline.set_frame(t1); app.processEvents(); do_apply("apply_t1")
            win.timeline.set_frame(t0); app.processEvents(); do_apply("apply_back_t0")
            do_apply("apply_back_t0_same")
            win.timeline.set_frame(t1); app.processEvents(); step("moved_t1", win, ref=False)
        elif scen == "dbg":          # preview stall after Apply + frame move? (standalone-equivalent; OFX emit not involved when ofx_apply=0)
            full_profile(); do_apply("apply_t0")
            t0 = win.timeline.frame(); t1 = t0 + 2
            for k in range(3):
                win.timeline.set_frame(t1 if k % 2 == 0 else t0); app.processEvents()
                tt = win.timeline.frame()
                ok = wait(lambda: idle() and (tt, win.gen, None) in win.filt_cache, 20)
                step(f"move{k}", win, ref=False, ok=ok, running=list(win.runner.running), queued=list(win.runner.queued),
                     keys=[list(map(str, kk)) for kk in win.filt_cache], gen=win.gen, timer=win.timer.isActive(), previewing=win.previewing())
                apply(); wait(idle)
                step(f"apply{k}", win, ref=False, gen=win.gen, timer=win.timer.isActive())
        elif scen == "revert":       # A, change B, back to A, apply -> same as last written => none
            full_profile(); do_apply("applyA")
            slider(20); wait(lambda: idle() and any(k[1] == win.gen for k in win.filt_cache))
            slider(70); wait_prev()
            do_apply("apply_reverted")
        elif scen == "tune":         # profile change after Apply (Noise Level tune) -> must emit
            full_profile(); do_apply("applyA")
            st.set_tune("all", 1.3); wait(idle); wait_prev(); step("tuned", win, ref=False)
            do_apply("apply_tuned"); do_apply("apply_tuned_same")
        elif scen == "cancel":
            full_profile(); do_apply("apply1")
            QTest.mouseClick(win.b_cancel, Qt.LeftButton); return
        elif scen == "cancel_none":
            full_profile(); QTest.mouseClick(win.b_cancel, Qt.LeftButton); return
        elif scen == "cancel_noprof":
            QTest.mouseClick(win.b_cancel, Qt.LeftButton); return
        elif scen == "cancel_changed":
            full_profile(); do_apply("apply1"); slider(30); wait(idle); step("slider30", win, ref=False)
            QTest.mouseClick(win.b_cancel, Qt.LeftButton); return
        elif scen == "close_only":
            pass
        # closing: reference of what close should write (computed before close)
        step("pre_close", win)
        win.close()

    sys.argv = ["nv_ui_qt/main.py", "--ofx"] + rest
    import runpy
    try:
        runpy.run_module("nv_ui_qt.main", run_name="__main__", alter_sys=True); rc = None
    except SystemExit as e:
        rc = e.code
    STEPS.append(dict(name="exit", rc=rc, size=fsz(), t=round(time.time() - T0, 2)))
    with open(os.path.join(outdir, "steps.json"), "w") as f:
        json.dump(STEPS, f, indent=1, default=str)
    return rc


if __name__ == "__main__":
    sys.exit(main())
