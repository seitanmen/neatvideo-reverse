"""Host-side stand-in driver for OFX mode: runs nv_ui_qt.main.main(["--ofx", ...]) unchanged, with
the UI driven by a QTimer (off-screen). Frames come from stdin (piped by ffmpeg); the processed
frames go to stdout exactly as in a real run. Log lines (JSON) go to stderr.

    ffmpeg ... -f rawvideo - | python3 ofx_driver.py SCENARIO OUTDIR W H FPS [--depth D]

SCENARIO: apply_close     Auto Profile -> AFT -> Apply -> close (no change in between)
          apply_change    Auto Profile -> AFT -> Apply -> Apply -> slider change -> close
          close_only      close at once (no profile made in the UI)
          cancel          Auto Profile -> AFT -> Cancel button
Each emitted frame's planes (before packing) are saved as OUTDIR/emit_<k>.npz together with the
preview image of the same frame/settings when it is available.
"""
import json
import os
import sys
import time

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
REPRO = "/Users/seitan/Desktop/work/reverse/repro"
sys.path.insert(0, REPRO)
os.chdir(REPRO)


def log(**kw):
    kw["t"] = round(time.time() - T0, 2)
    print("DRV " + json.dumps(kw, default=str), file=sys.stderr, flush=True)


T0 = time.time()


def main():
    scen, outdir = sys.argv[1], sys.argv[2]
    rest = sys.argv[3:]
    os.makedirs(outdir, exist_ok=True)
    os.environ["NV_UI_QT_SETTINGS"] = os.path.join(outdir, "st.ini")
    import numpy as np
    from PySide6.QtCore import QTimer, Qt
    from PySide6.QtTest import QTest
    from nv_ui_qt import backend, ofx_bridge
    from nv_ui_qt.main_window import MainWindow

    emits = []
    RGB = []
    orig_emit = ofx_bridge.OfxFrameSource.emit_frame

    def emit_frame(self, planes, index=None):
        k = len(emits)
        win = W[0]
        t = win.timeline.frame()
        prev = win.filt_cache.get((t, win.gen, None))
        rgb = win.engine.to_rgb([np.clip(np.asarray(p, np.float32), 0, 255) for p in planes])
        same_prev = None if prev is None else bool(np.array_equal(prev, rgb))
        np.savez(os.path.join(outdir, f"emit_{k}.npz"), *[np.asarray(p) for p in planes])
        emits.append(index)
        RGB.append(rgb)
        log(ev="emit", k=k, index=index, frame=t, preview_cached=prev is not None, equals_preview=same_prev,
            amt_Y=win.state.params.get("amt_Y"), aft=bool((win.state.profile.base or {}).get("aft_applied")))
        return orig_emit(self, planes, index)

    ofx_bridge.OfxFrameSource.emit_frame = emit_frame
    W = []
    orig_show = MainWindow.show

    def show(win):
        orig_show(win)
        W.append(win)
        QTimer.singleShot(0, lambda: drive(win))

    MainWindow.show = show

    def drive(win):
        from PySide6.QtWidgets import QApplication
        app = QApplication.instance()
        st = win.state
        idle = lambda: not win.runner.running and not win.runner.queued

        def wait(cond, timeout=600):
            t0 = time.time()
            while not cond():
                app.processEvents()
                time.sleep(0.01)
                if time.time() - t0 > timeout:
                    return False
            app.processEvents()
            return True

        a = win.actions
        log(ev="started", frames=len(win.frames), shape=[list(p.shape) for p in win.frames[0]],
            dtype=str(win.frames[0][0].dtype), info=win.info, src=win.src,
            open_enabled=a["open"].isEnabled(), export_enabled=a["export"].isEnabled(),
            load_profile_enabled=win.load_btn.isEnabled(), generic_enabled=win.generic_btn.isEnabled(),
            apply_tip=win.b_apply.toolTip(), status=win.msg.text(), prof=st.profile.base is not None,
            frame=win.timeline.frame())
        # trying the disabled actions does nothing (no dialog, no state change)
        a["open"].trigger()
        a["export"].trigger()
        log(ev="disabled_triggered", export=win.export is None, src=win.src)
        if scen == "close_only":
            win.close()
            return
        win.tabs.setCurrentIndex(1)
        app.processEvents()
        t1 = time.time()
        QTest.mouseClick(win.auto_btn, Qt.LeftButton)
        ok = wait(lambda: st.profile.base is not None, 300)
        b = st.profile.base or {}
        log(ev="profile", ok=ok, flat_status=b.get("flat_status"), area=b.get("area"), frame=b.get("ui_frame"),
            secs=round(time.time() - t1, 2))
        t1 = time.time()
        ok = wait(lambda: st.profile.base.get("aft_applied") and idle(), 300)
        log(ev="aft", ok=ok, aft_applied=st.profile.base.get("aft_applied"), secs=round(time.time() - t1, 2),
            status=win.msg.text())
        ok = wait(lambda: idle() and win.filt_cache, 300)
        log(ev="preview", ok=ok, keys=list(win.filt_cache))
        if scen == "cancel":
            QTest.mouseClick(win.b_cancel, Qt.LeftButton)
            return
        n0 = len(emits)
        QTest.mouseClick(win.b_apply, Qt.LeftButton)
        wait(lambda: idle(), 300)
        log(ev="after_apply", emitted=len(emits) - n0, status=win.msg.text())
        t = win.timeline.frame()
        ok = wait(lambda: idle() and (t, win.gen, None) in win.filt_cache, 300)
        log(ev="apply_preview", ok=ok, equals_last_emit=bool(RGB) and ok and
            bool(np.array_equal(win.filt_cache[(t, win.gen, None)], RGB[-1])))
        if scen == "apply_change":
            n0 = len(emits)
            QTest.mouseClick(win.b_apply, Qt.LeftButton)
            wait(lambda: idle(), 300)
            log(ev="after_apply2", emitted=len(emits) - n0, status=win.msg.text())
            win.filter_page.panel.controls["amt_Y"].slider.setValue(20)
            ok = wait(lambda: idle() and any(k[1] == win.gen for k in win.filt_cache), 300)
            log(ev="slider", ok=ok, amt_Y=st.params["amt_Y"])
        n0 = len(emits)
        win.close()
        log(ev="after_close", emitted=len(emits) - n0)

    if os.environ.get("DRV_RUNPY"):     # same as python3 -m nv_ui_qt.main (spawned AFT child imports it as __mp_main__)
        import runpy
        sys.argv = ["nv_ui_qt/main.py", "--ofx"] + rest
        try:
            runpy.run_module("nv_ui_qt.main", run_name="__main__", alter_sys=True)
            rc = None
        except SystemExit as e:
            rc = e.code
    else:
        from nv_ui_qt import main as m
        rc = m.main(["nv_ui_qt.main", "--ofx"] + rest)
    log(ev="exit", rc=rc, emits=emits)
    return rc


if __name__ == "__main__":
    sys.exit(main())
