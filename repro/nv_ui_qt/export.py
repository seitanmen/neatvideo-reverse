"""Full-video export for the Qt UI: denoise_video_v5.main with the UI's profile and parameters.

    python3 -m nv_ui_qt.export SRC DST --ui-profile PROF.json --params P.json [denoise_video_v5 options]

Everything else (reading, audio, colour tags, encoding, frame parallelism) is denoise_video_v5.py
unchanged. Two of its module attributes are replaced in this process only: build_profile_v5
returns the UI profile (so AFT and Manual Fine-Tune edits are kept), and the frame worker is
backend._work (per-channel spatial settings; identical to process_frame_v5 when the channels share
their band settings).
"""
import json
import os
import signal
import sys
import threading
import time

from nv_ui_qt import backend  # noqa: F401  (sets sys.path to repro/)
import denoise_video_v5 as dv
import nvfilter as nv


def _own_group():
    """Run in a process group of our own (the UI kills the group: this process, the frame workers,
    multiprocessing's resource tracker and ffmpeg) and kill it if the UI dies (parent becomes init)."""
    try:
        os.setpgid(0, 0)
    except OSError:
        pass
    parent = os.getppid()

    def watch():
        while True:
            if os.getppid() != parent:
                os.killpg(os.getpgid(0), signal.SIGKILL)
            time.sleep(1.0)

    threading.Thread(target=watch, daemon=True).start()


def main(argv):
    _own_group()
    if "--ui-profile" not in argv:
        sys.exit("--ui-profile PROF.json is required")
    i = argv.index("--ui-profile")
    path = argv[i + 1]
    rest = argv[:i] + argv[i + 2:]
    with open(path) as fh:
        prof = json.load(fh)

    def ui_profile(*a, **k):
        return prof

    nv.build_profile_v5 = ui_profile
    dv._init, dv._work = backend._init, backend._work
    sys.argv = ["denoise_video_v5.py"] + rest
    dv.main()


if __name__ == "__main__":
    main(sys.argv[1:])
