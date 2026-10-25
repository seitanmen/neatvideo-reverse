"""Entry point.

Standalone mode (default):
  python3 -m nv_ui_qt.main [video]
  Loads a video file with ffmpeg, shows the UI, exports with denoise_video_v5.

OFX host mode:
  python3 -m nv_ui_qt.main --ofx W H [FPS] [MAX_FRAMES] [--depth 8|16|32]
  Reads raw 4:2:0 frames from stdin (written by the DCC host; sample types in ofx_bridge.py,
  default 8 = uint8 yuv420p), shows the UI. Closing the window processes the current frame
  (with an Auto Profile of it when no profile was made) and writes it to stdout in the same layout.
  The host reads stdout to get the filtered frame back.
"""
import sys


def main(argv=None):
    argv = sys.argv if argv is None else argv
    args = argv[1:]

    # --ofx W H [FPS] [MAX_FRAMES]
    if args and args[0] == "--ofx":
        return _ofx_main(args[1:], argv)

    # standalone (default)
    from PySide6.QtWidgets import QApplication
    from nv_ui_qt.main_window import MainWindow
    app = QApplication(argv)
    app.setApplicationName("nv_ui_qt")
    win = MainWindow(args[0] if args else None)
    win.show()
    return app.exec()


def _ofx_main(rest, argv):
    """OFX host mode: read frames from stdin, launch the UI, emit the current frame on close."""
    depth = 8
    if "--depth" in rest:
        i = rest.index("--depth")
        depth = int(rest[i + 1])
        rest = rest[:i] + rest[i + 2:]
    if len(rest) < 2:
        print("usage: --ofx W H [FPS] [MAX_FRAMES] [--depth 8|16|32]", file=sys.stderr)
        return 1
    W, H = int(rest[0]), int(rest[1])
    fps = float(rest[2]) if len(rest) > 2 else 24.0
    max_frames = int(rest[3]) if len(rest) > 3 else 0

    from PySide6.QtWidgets import QApplication, QMessageBox

    # read frames from stdin before starting the UI (the host writes them all upfront)
    from nv_ui_qt.ofx_bridge import OfxFrameSource
    src = OfxFrameSource(W, H, fps, max_frames, depth)
    frames = src.read_all()
    if not frames:
        print("no frames received on stdin", file=sys.stderr)
        return 1
    info = src.info()

    app = QApplication(argv)
    app.setApplicationName("nv_ui_qt (OFX)")

    from nv_ui_qt import backend
    from nv_ui_qt.main_window import MainWindow
    win = MainWindow()               # no video path
    # inject the host-provided frames into the UI
    win.src = "<OFX host>"
    win.info = info
    win.frames = frames
    win.matrix = "bt709"
    win.full_range = info["full_range"]
    win.engine = backend.PreviewEngine(frames, win.matrix, info["full_range"])
    win.timeline.set_count(len(frames), fps)
    pf = min(len(frames) // 3, max(len(frames) - 3, 0))
    win.timeline.set_frame(pf)
    win.video_lbl.setText(f"OFX host: {W}x{H} · {len(frames)} frames · {fps} fps")
    win.depth_lbl.setText(f"source OFX host ({depth}-bit 4:2:0) · processing {depth}-bit")
    win.viewer_bar.set_frame_info(W, H, depth)
    win._update_performance()
    # disable standalone-only controls
    win._set_ofx_mode(True)
    win.show()

    # when the window is closed, emit all processed frames (or just the current one)
    def _on_close(event):
        # emit the current frame to stdout
        t = win.timeline.frame()
        try:
            P = win.state.backend()
            prof = win.state.effective_profile()
            if prof is None:            # no profile made in the UI: Auto Profile of the current frame
                prof = backend.build_profile(frames, t, win.full_range, win.matrix, "auto")
            idx, planes = win.engine.window(t, int(P["radius"]), None)
            out = backend.process_frame(planes, idx.index(t), prof, P)
            src.emit_frame(out, t)
        except Exception as e:
            print(f"OFX emit error: {e}", file=sys.stderr)
        MainWindow.closeEvent(win, event)
        if event.isAccepted():
            app.quit()      # the host waits for the exit; a stray top-level widget would keep the loop running

    win.closeEvent = _on_close
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
