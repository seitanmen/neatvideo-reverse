"""Verifier: OFX mode frame counts. Wrapper (own) monkeypatches MainWindow.show to run a scripted action.
   cd repro && python3 ../sandbox/omo/coord/genuine_ui/verify/v_ofx.py"""
import os, subprocess, sys
HERE = os.path.dirname(os.path.abspath(__file__))
REPRO = os.path.abspath(os.path.join(HERE, *[".."] * 5, "repro"))
W, H = 640, 360
FS = W * H + 2 * (W // 2) * (H // 2)

def child():
    os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    sys.path.insert(0, os.getcwd())
    from PySide6.QtCore import QTimer
    from nv_ui_qt import main as M
    from nv_ui_qt.main_window import MainWindow
    _show = MainWindow.show
    act = os.environ["V_ACT"]
    def show(self):
        _show(self)
        def go():
            if act == "apply":
                self.b_apply.click(); self.close()
            elif act == "apply_apply_cancel":
                self.b_apply.click(); self.b_apply.click(); self.b_cancel.click()
            elif act == "cancel":
                self.b_cancel.click()
            elif act == "apply_change_close":
                self.b_apply.click(); self.state.set_param("amt_Y", 55.0); self.close()
            elif act == "close":
                self.close()
            elif act == "menu_apply_cancel":
                self.actions["apply"].trigger(); self.actions["cancel"].trigger()
        QTimer.singleShot(400, go)
    MainWindow.show = show
    return M.main([sys.argv[0]] + sys.argv[2:])

def main():
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", os.path.join(REPRO, "..", "test.mp4"), "-vf",
                          "select=between(n\\,500\\,505)", "-fps_mode", "passthrough", "-f", "rawvideo",
                          "-pix_fmt", "yuv420p", "-"], capture_output=True, check=True).stdout
    ok = True
    for act, want in (("apply", 1), ("apply_apply_cancel", 1), ("cancel", 0), ("apply_change_close", 2),
                      ("close", 1), ("menu_apply_cancel", 1)):
        env = dict(os.environ, QT_QPA_PLATFORM="offscreen", V_ACT=act, NV_UI_QT_SETTINGS=os.path.join(HERE, "ofx.ini"))
        r = subprocess.run([sys.executable, os.path.abspath(__file__), "--child", "--ofx", str(W), str(H), "30"],
                           input=raw, capture_output=True, cwd=REPRO, env=env, timeout=900)
        n = len(r.stdout) / FS
        lines = r.stderr.decode(errors="replace").splitlines()
        other = [l for l in lines if "propagateSizeHints" not in l and not l.startswith("qt.qpa.fonts")]
        with open(os.path.join(HERE, f"ofx_stderr_{act}.txt"), "w") as fh:
            fh.write(r.stderr.decode(errors="replace"))
        good = r.returncode == 0 and n == want and not other
        ok &= good
        print(f"[{"PASS" if good else "FAIL"}] {act}: exit {r.returncode}, {n:g} frames (expected {want}), stderr tracebacks {r.stderr.count(b"Traceback")}, stderr lines {len(lines)} (other than Qt offscreen notices: {len(other)})"
              + ("" if good else " stderr: " + r.stderr.decode()[-400:]), flush=True)
    return 0 if ok else 1

if __name__ == "__main__":
    sys.exit(child() if len(sys.argv) > 1 and sys.argv[1] == "--child" else main())
