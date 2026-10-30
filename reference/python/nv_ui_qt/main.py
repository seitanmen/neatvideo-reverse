"""Entry point.

Standalone mode (default):
  python3 -m nv_ui_qt.main [video]
  Loads a video file with ffmpeg, shows the UI, exports with denoise_video_v5.  ffmpeg / ffprobe are looked
  for as ffmpeg_path.py describes ("ffmpeg not found" dialog when there are none).

OFX host mode:
  python3 -m nv_ui_qt.main --ofx W H [FPS] [MAX_FRAMES] [--depth 8|16|32]
  Reads raw 4:2:0 frames from stdin until EOF (written by the DCC host; sample types in ofx_bridge.py,
  default 8 = uint8 yuv420p), shows the UI. The current frame, processed with the current settings
  (with an Auto Profile of it when no profile was made), is written to stdout in the same layout:
    - Apply writes it (the preview is refreshed as in standalone mode);
    - closing the window writes it once more only when nothing was written yet or the frame / settings /
      profile differ from the last written frame; an unchanged Apply writes nothing;
    - Cancel closes without writing (the host keeps the last frame written, if any).
  stdout carries whole frames only (k x frame_size bytes); the host keeps the last complete frame.
"""
import json
import sys


def main(argv=None):
    argv = sys.argv if argv is None else argv
    args = argv[1:]
    # ffmpeg / ffprobe: $NV_FFMPEG, prefs/ffmpeg, /opt/homebrew/bin, /usr/local/bin, /opt/local/bin, PATH
    # (ffmpeg_path.py; the same search as the MeatVideo.app launcher and the C++ UI)
    from nv_ui_qt import ffmpeg_path
    ffmpeg_path.setup()

    # --ofx W H [FPS] [MAX_FRAMES]  (no ffmpeg needed)
    if args and args[0] == "--ofx":
        # (the C++ UI's traceback copies this line's number and text: qtui/src/app/ofx.cpp print_read_exception)
        return _ofx_main(args[1:], argv)

    # standalone (default)
    from PySide6.QtWidgets import QApplication, QMessageBox
    from nv_ui_qt.main_window import MainWindow
    app = QApplication.instance() or QApplication(argv)
    app.setApplicationName("nv_ui_qt")
    if not ffmpeg_path.ffmpeg_dir():
        QMessageBox.critical(None, ffmpeg_path.NOT_FOUND_TITLE, ffmpeg_path.NOT_FOUND_TEXT)
        args = []                   # opening a video would fail in ffprobe
    win = MainWindow(args[0] if args else None)
    win.show()
    return app.exec()


def _ofx_main(rest, argv):
    """OFX host mode: read frames from stdin, launch the UI, emit the current frame on Apply / close."""
    usage = "usage: --ofx W H [FPS] [MAX_FRAMES] [--depth 8|16|32]"
    try:
        depth = 8
        if "--depth" in rest:
            i = rest.index("--depth")
            depth = int(rest[i + 1])
            rest = rest[:i] + rest[i + 2:]
        W, H = int(rest[0]), int(rest[1])
        fps = float(rest[2]) if len(rest) > 2 else 24.0
        max_frames = int(rest[3]) if len(rest) > 3 else 0
    except (IndexError, ValueError):
        depth = None
    if depth not in (8, 16, 32) or W <= 0 or H <= 0:
        print(usage, file=sys.stderr)
        return 1

    from PySide6.QtWidgets import QApplication, QMessageBox

    # read frames from stdin before starting the UI (the host writes them all upfront)
    from nv_ui_qt.ofx_bridge import OfxFrameSource
    src = OfxFrameSource(W, H, fps, max_frames, depth)
    # (the C++ UI's traceback copies this line's number and text: qtui/src/app/ofx.cpp print_read_exception)
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
    win.say("OFX host: Auto Profile (Prepare Noise Profile), adjust, then Apply to return the frame to the host")
    win.show()

    last = [None]                       # (frame, params, profile) of the last frame written to stdout

    def _emit(why):
        """Process the current frame and write it to stdout unless it equals the last written one.
        Returns "written", "same" or "error"."""
        t = win.timeline.frame()
        try:
            P = win.state.backend()
            prof = win.state.effective_profile()
            sig = (t, json.dumps(P, sort_keys=True, default=str), None if prof is None else win._profile_sig())
            if sig == last[0]:
                return "same"
            if prof is None:            # no profile made in the UI: Auto Profile of the current frame
                prof = backend.build_profile(frames, t, win.full_range, win.matrix, "auto")
            idx, planes = win.engine.window(t, int(P["radius"]), None)
            out = backend.process_frame(planes, idx.index(t), prof, P)
            src.emit_frame(out, t)
            last[0] = sig
            return "written"
        except Exception as e:
            print(f"OFX emit error ({why}): {e}", file=sys.stderr)
            return "error"

    def _apply():
        t = win.timeline.frame()
        win.say({"written": f"Apply: frame {t} processed and written to the host",
                 "same": f"Apply: frame {t} and the settings are unchanged since the last Apply (not written again)",
                 "error": "Apply: the frame could not be processed (see stderr)"}[_emit("Apply")])

    win.b_apply.clicked.connect(_apply)

    cancelled = [False]

    def _cancel():
        cancelled[0] = True

    win.b_cancel.clicked.disconnect()
    win.b_cancel.clicked.connect(_cancel)
    win.b_cancel.clicked.connect(win.close)

    def _on_close(event):
        if not cancelled[0]:
            _emit("close")
        MainWindow.closeEvent(win, event)
        if event.isAccepted():
            app.quit()      # the host waits for the exit; a stray top-level widget would keep the loop running

    win.closeEvent = _on_close
    return app.exec()


if __name__ == "__main__":
    # (the C++ UI's traceback copies this line's number and text: qtui/src/app/ofx.cpp print_read_exception)
    sys.exit(main())
