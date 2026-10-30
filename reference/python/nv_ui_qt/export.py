"""Full-video export for the Qt UI: denoise_video_v5.main with the UI's profile and parameters.

    python3 -m nv_ui_qt.export SRC DST --ui-profile PROF.json --params P.json [denoise_video_v5 options]

Everything else (reading, audio, colour tags, encoding, frame parallelism) is denoise_video_v5.py
unchanged. Two of its module attributes are replaced in this process only: build_profile_v5
returns the UI profile (so AFT and Manual Fine-Tune edits are kept), and the frame worker is
backend._work (per-channel spatial settings; identical to process_frame_v5 when the channels share
their band settings). The bit depth (--depth, Preferences > Bit depth) is passed through; a profile
whose chroma sub-sampling differs from the processing format (e.g. made at 8 bits = 4:2:0 and
exported at 10 bits from a 4:4:4 source) is refused.
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

    def ui_profile(planes, full_range=False, *a, **k):
        # the profile's chroma sub-sampling must match the processing format (--depth): its chroma
        # band counts (levels) follow it, e.g. a profile made on 4:2:0 cannot run on 4:4:4 planes
        sub = 1 if planes[1].shape == planes[0].shape else 2
        if prof.get("sub") != sub:
            sys.exit(f"error: the UI profile was made on {'4:4:4' if prof.get('sub') == 1 else '4:2:0'} planes "
                     f"but this export processes {'4:4:4' if sub == 1 else '4:2:0'} planes (--depth); "
                     "rebuild the profile with the same bit-depth setting")
        if bool(prof.get("full_range")) != bool(full_range):
            print("warning: the UI profile and this export differ in range (limited / full: RGB formats are "
                  "processed as full range); the noise curve's brightness axis may be off", flush=True)
        return prof

    nv.build_profile_v5 = ui_profile
    dv._init, dv._work = backend._init, backend._work
    sys.argv = ["denoise_video_v5.py"] + rest
    dv.main()


if __name__ == "__main__":
    main(sys.argv[1:])
