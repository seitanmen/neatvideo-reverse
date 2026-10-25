"""Entry point.

Standalone mode (default):
  python3 -m nv_ui_qt.main [video]
  Loads a video file with ffmpeg, shows the UI, exports with denoise_video_v5.

OFX host mode:
  python3 -m nv_ui_qt.main --ofx W H [FPS] [MAX_FRAMES]
  Reads raw yuv420p frames from stdin (written by the DCC host), shows the UI.
  Pressing Apply processes the current frame and writes it to stdout.
  The host reads stdout to get the filtered frame back.
"""
import sys


def main(argv=None):
    argv = sys.argv if argv is None else argv
    args = argv[1:]

    # --ofx W H [FPS] [MAX_FRAMES]
    if args and args[0] == "--ofx":
        return _ofx_main(args[1:])

    # standalone (default)
    from PySide6.QtWidgets import QApplication
    from nv_ui_qt.main_window import MainWindow
    app = QApplication(argv)
    app.setApplicationName("nv_ui_qt")
    win = MainWindow(args[0] if args else None)
    win.show()
    return app.exec()


def _ofx_main(rest):
    """OFX host mode: read frames from stdin, launch the UI, emit on Apply."""
    if not rest:
        print("usage: --ofx W H [FPS] [MAX_FRAMES]", file=sys.stderr)
        return 1
    W, H = int(rest[0]), int(rest[1])
    fps = float(rest[2]) if len(rest) > 2 else 24.0
    max_frames = int(rest[3]) if len(rest) > 3 else 0

    from PySide6.QtWidgets import QApplication, QMessageBox

    # read frames from stdin before starting the UI (the host writes them all upfront)
    from nv_ui_qt.ofx_bridge import OfxFrameSource
    src = OfxFrameSource(W, H, fps, max_frames)
    frames = src.read_all()
    if not frames:
        print("no frames received on stdin", file=sys.stderr)
        return 1
    info = src.info()

    app = QApplication(argv)
    app.setApplicationName("nv_ui_qt (OFX)")

    from nv_ui_qt.main_window import MainWindow
    win = MainWindow()               # no video path
    # inject the host-provided frames into the UI
    win.src = "<OFX host>"
    win.info = info
    win.frames = frames
    win.matrix = "bt709"
    win.engine = __import__("nv_ui_qt.backend", fromlist=["PreviewEngine"]).PreviewEngine(
        frames, win.matrix, info["full_range"])
    win.timeline.set_count(len(frames), fps)
    pf = min(len(frames) // 3, max(len(frames) - 3, 0))
    win.timeline.set_frame(pf)
    win.video_lbl.setText(f"OFX host: {W}x{H} · {len(frames)} frames · {fps} fps")
    # disable standalone-only controls
    win._set_ofx_mode(True)
    win.show()

    # when the window is closed, emit all processed frames (or just the current one)
    def _on_close(event):
        # emit the current frame to stdout
        t = win.timeline.frame()
        try:
            P = win.params()
            idx = [i for i in range(t - P["radius"], t + P["radius"] + 1) if 0 <= i < len(frames)]
            ci = idx.index(t) if t in idx else len(idx) // 2
            out = win.process_frame(t, P)
            src.emit_frame(out)
        except Exception as e:
            print(f"OFX emit error: {e}", file=sys.stderr)
        event.accept()

    win.closeEvent = _on_close
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
