"""Processes the interlaced E9 inputs with the reproduction's field processing (INT-3, the C++ UI of the int2 worktree,
built on a scratch copy by tools/cpp_int3/CMakeLists.txt).

    python3 tools/run_ours_fields.py --build BUILD_DIR CLIP [CLIP ...] [--first 0] [--kind two] [--profile-frame 0]
                                     [--variants same_frames:same:4,same_fields:same:2,shift_frames:shift:4,plain_frames:plain:4]
                                     [--jobs 4]
    CLIP = <src>/<layout>_<level>, e.g. sintel/tff_y3

For each clip (input clips/<src>/noisy_<layout>_<level>.y4m, 8-bit 4:2:0 y4m whose header says It / Ib):
  1. BUILD/e9_profile_fields: the UI's profile of an interlaced clip with the Video Scan Type set explicitly
     (--first; the y4m's field flag is not used): Auto Profile (Current Frame) on frame --profile-frame, Profiled with
     --kind (Two fields by default), Auto Fine-Tune on the profile's field image; the default parameters with
     P["interlace"] = {first, mode, radius in fields} per variant (MainWindow::proc_params)
     -> runs/ours_field/<src>/<clip>/ (ui_profile.json, nvprof.json, profile_info.json, params_<variant>.json)
  2. BUILD/qtui_export (export_cli.cpp: the UI's ExportJob; the interlace entry of the parameters selects the field
     path) per variant, --crf 0 (lossless 8-bit x264; interlaced coding flags as the export sets them), --audio none
     -> the first variant (the default, same parity / radius in frames) at runs/ours_field/<src>/<clip>/out.mp4,
        the others at runs/ours_field_<variant>/<src>/<clip>/out.mp4 (eval_all.py picks up every runs/ours_field*)
Settings: NV_UI_QT_SETTINGS=runs/settings_e9.ini; ~/.config/NeatRepro/nv_ui_qt.ini is hashed before and after.
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
    ap.add_argument("--build", required=True)
    ap.add_argument("--first", type=int, default=0)
    ap.add_argument("--kind", default="two")
    ap.add_argument("--profile-frame", default="0")
    ap.add_argument("--variants", default="same_frames:same:4,same_fields:same:2,shift_frames:shift:4,plain_frames:plain:4")
    ap.add_argument("--jobs", type=int, default=4)
    a = ap.parse_args()
    variants = [v.split(":") for v in a.variants.split(",")]
    ini_before = C.sha256(USER_INI) if os.path.exists(USER_INI) else None
    settings = os.path.join(ROOT, "runs", "settings_e9.ini")
    open(settings, "a").close()
    env = dict(os.environ, NV_UI_QT_SETTINGS=settings, QT_QPA_PLATFORM="offscreen")
    bins = {b: os.path.join(a.build, b) for b in ("e9_profile_fields", "qtui_export")}
    for clip in a.clips:
        src, lv = clip.split("/")
        layout, level = lv.split("_")
        inp = os.path.join(ROOT, "clips", src, f"noisy_{layout}_{level}.y4m")
        pdir = os.path.join(ROOT, "runs", "ours_field", src, lv)
        os.makedirs(pdir, exist_ok=True)
        tp = run([bins["e9_profile_fields"], inp, pdir, a.profile_frame, str(a.first), a.kind] + [":".join(v) for v in variants],
                 os.path.join(pdir, "profile.log"), env)
        info = json.load(open(os.path.join(pdir, "profile_info.json")))
        for i, (name, mode, radius) in enumerate(variants):
            out = pdir if i == 0 else os.path.join(ROOT, "runs", f"ours_field_{name}", src, lv)
            os.makedirs(out, exist_ok=True)
            dst = os.path.join(out, "out.mp4")
            if os.path.exists(dst):
                os.remove(dst)
            trace = os.path.join(out, "trace.jsonl")
            if os.path.exists(trace):
                os.remove(trace)
            params = os.path.join(pdir, f"params_{name}.json")
            cmd = [bins["qtui_export"], inp, dst, "--ui-profile", os.path.join(pdir, "ui_profile.json"), "--params", params,
                   "--jobs", str(a.jobs), "--crf", "0", "--audio", "none"]
            te = run(cmd, os.path.join(out, "export.log"), dict(env, NV_EXPORT_TRACE=trace))
            rec = {"clip": clip, "variant": name, "interlace": {"first": a.first, "mode": mode, "radius_fields": int(radius)},
                   "input": os.path.relpath(inp, ROOT), "input_sha256": C.sha256(inp), "profile": info,
                   "profile_dir": os.path.relpath(pdir, ROOT), "export_cmd": cmd,
                   "seconds": {"profile": round(tp, 1), "export": round(te, 1)},
                   "output": os.path.relpath(dst, ROOT), "output_sha256": C.sha256(dst),
                   "binaries": {b: C.sha256(p) for b, p in bins.items()}}
            C.write_json(os.path.join(out, "run.json"), rec)
            print(f"{clip} {name}: profile {info['profiled_with']} field {info['field']} area {info['area_field_rows']} "
                  f"mode {info['mode']} flat {info['flat_status']}; export {te:.0f} s", flush=True)
    ini_after = C.sha256(USER_INI) if os.path.exists(USER_INI) else None
    if ini_before != ini_after:
        raise SystemExit(f"{USER_INI} changed: {ini_before} -> {ini_after}")
    print(f"user settings unchanged: {ini_after}")


if __name__ == "__main__":
    main()
