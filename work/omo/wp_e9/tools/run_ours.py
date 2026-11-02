"""Processes the E9 inputs with the reproduction's official C++ build (the C++ UI's code: qtui/src at the checkout,
built privately into work/omo/wp_e9/build by tools/cpp/CMakeLists.txt).

    python3 tools/run_ours.py CLIP [CLIP ...] [--processing frame|field] [--profile-frame 0|noisy] [--jobs 4]
                              [--field-cmd TEMPLATE] [--tag NAME] [--root WP_E9] [--build DIR] [--nvcore LIB]
    CLIP = <src>/<layout>_<level>, e.g. park_joy/p_y3, old_town_cross/tff_y3

For each clip (input clips/<src>/noisy_<layout>_<level>.y4m):
  1. build/e9_profile: the UI's default profile (Auto Profile (Current Frame) on --profile-frame, default 0 = the
     current frame after loading, then Auto Fine-Tune; the defaults of Preferences) -> ui_profile.json,
     ui_params.json (default parameters), nvprof.json, profile_info.json (profile frame, area, flat_status)
  2. build/qtui_export (qtui/tests/export_cli.cpp = the UI's ExportJob) with those files, --crf 0 (8-bit x264
     qp 0 = lossless; the UI's default CRF 16 would add coding loss to the comparison), --audio none, default depth 8
     -> runs/ours_<processing>[_<tag>]/<src>/<layout>_<level>/out.mp4 (+ export.log, trace.jsonl, run.json)
--processing field: the field processing of INT-2 is not merged yet. The hook runs --field-cmd (a shell template
with {src} {dst} {profile} {params} {jobs}) when given, else stops with a message; the outputs go to
runs/ours_field[_<tag>]/ so that the frame results stay.
--root / --build / --nvcore (added 2026-11-02 for the frame-5 rerun, eval/E9_rerun_frame5.md): the wp_e9 folder that
holds clips/ and runs/ (default: the one of this script), the build of tools/cpp (default ROOT/build) and the
libnvcore.dylib it links (recorded in run.json; default the checkout's native/libnvcore.dylib).
Settings: a private QSettings file (NV_UI_QT_SETTINGS=runs/settings_e9.ini); ~/.config/NeatRepro/nv_ui_qt.ini is
hashed before and after and must not change.
"""
import argparse
import json
import os
import shlex
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import e9_common as C  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
BUILD = os.path.join(ROOT, "build")
USER_INI = os.path.expanduser("~/.config/NeatRepro/nv_ui_qt.ini")


def run(cmd, log, env):
    t = time.time()
    with open(log, "w") as fh:
        fh.write("$ " + " ".join(shlex.quote(c) for c in cmd) + "\n")
        fh.flush()
        r = subprocess.run(cmd, stdout=fh, stderr=subprocess.STDOUT, env=env, cwd=ROOT)
    if r.returncode != 0:
        raise SystemExit(f"failed ({r.returncode}): {' '.join(cmd)}; see {log}")
    return time.time() - t


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("clips", nargs="+")
    ap.add_argument("--processing", choices=("frame", "field"), default="frame")
    ap.add_argument("--profile-frame", default="0")
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--field-cmd", default=None)
    ap.add_argument("--tag", default="")
    ap.add_argument("--root", default=None)
    ap.add_argument("--build", default=None)
    ap.add_argument("--nvcore", default=None)
    a = ap.parse_args()
    global ROOT, BUILD
    if a.root:
        ROOT = os.path.abspath(a.root)
    BUILD = os.path.abspath(a.build) if a.build else os.path.join(ROOT, "build")
    nvcore = os.path.abspath(a.nvcore) if a.nvcore else os.path.join(ROOT, "..", "..", "..", "native", "libnvcore.dylib")
    ini_before = C.sha256(USER_INI) if os.path.exists(USER_INI) else None
    settings = os.path.join(ROOT, "runs", "settings_e9.ini")
    os.makedirs(os.path.dirname(settings), exist_ok=True)
    open(settings, "a").close()
    env = dict(os.environ, NV_UI_QT_SETTINGS=settings, QT_QPA_PLATFORM="offscreen")
    base = os.path.join(ROOT, "runs", f"ours_{a.processing}" + (f"_{a.tag}" if a.tag else ""))
    for clip in a.clips:
        srcname, lv = clip.split("/")
        layout, level = lv.split("_")
        src = os.path.join(ROOT, "clips", srcname, f"noisy_{layout}_{level}.y4m")
        if not os.path.exists(src):
            raise SystemExit(f"missing {src}")
        out = os.path.join(base, srcname, lv)
        os.makedirs(out, exist_ok=True)
        dst = os.path.join(out, "out.mp4")
        if os.path.exists(dst):
            os.remove(dst)
        tp = run([os.path.join(BUILD, "e9_profile"), src, out, a.profile_frame], os.path.join(out, "profile.log"), env)
        prof, params = os.path.join(out, "ui_profile.json"), os.path.join(out, "ui_params.json")
        if a.processing == "frame":
            trace = os.path.join(out, "trace.jsonl")
            if os.path.exists(trace):
                os.remove(trace)
            cmd = [os.path.join(BUILD, "qtui_export"), src, dst, "--ui-profile", prof, "--params", params,
                   "--jobs", str(a.jobs), "--crf", "0", "--audio", "none"]
            te = run(cmd, os.path.join(out, "export.log"), dict(env, NV_EXPORT_TRACE=trace))
        else:
            # INT-2 hook: the field processing entry (e.g. a qtui_export option of INT-2) goes here
            if not a.field_cmd:
                raise SystemExit("--processing field: the reproduction's field processing (INT-2) is not merged; "
                                 "pass --field-cmd 'TEMPLATE' ({src} {dst} {profile} {params} {jobs}) once it is")
            cmd = ["/bin/sh", "-c", a.field_cmd.format(src=shlex.quote(src), dst=shlex.quote(dst),
                                                       profile=shlex.quote(prof), params=shlex.quote(params),
                                                       jobs=a.jobs)]
            te = run(cmd, os.path.join(out, "export.log"), env)
        info = json.load(open(os.path.join(out, "profile_info.json")))
        rec = {"clip": clip, "input": os.path.relpath(src, ROOT), "input_sha256": C.sha256(src),
               "processing": a.processing, "profile": info, "export_cmd": cmd, "jobs": a.jobs,
               "seconds": {"profile": round(tp, 1), "export": round(te, 1)},
               "output": os.path.relpath(dst, ROOT), "output_sha256": C.sha256(dst),
               "binaries": {b: C.sha256(os.path.join(BUILD, b)) for b in ("e9_profile", "qtui_export")},
               "libnvcore": nvcore, "libnvcore_sha256": C.sha256(nvcore)}
        C.write_json(os.path.join(out, "run.json"), rec)
        print(f"{clip}: profile frame {info['profile_frame']} area {info['area']} flat_status {info['flat_status']}, "
              f"profile {tp:.0f} s, export {te:.0f} s", flush=True)
    ini_after = C.sha256(USER_INI) if os.path.exists(USER_INI) else None
    if ini_before != ini_after:
        raise SystemExit(f"{USER_INI} changed: {ini_before} -> {ini_after}")
    print(f"user settings unchanged: {ini_after}")


if __name__ == "__main__":
    main()
