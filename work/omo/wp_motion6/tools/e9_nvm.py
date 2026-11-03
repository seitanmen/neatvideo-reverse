#!/usr/bin/env python3
"""E9 run with the temporal flow estimation replaced by the nv_motion port (the genuine's block matching:
pattern search + candidate acceptance + 32x32 + smoothing, work/omo/wp_motion/src/nv_motion.py).

    python3 e9_nvm.py CLIP [--start S --frames M] [--jobs 4] [--tag T]
    CLIP = <src>/p_y3 | <src>/p_y6

Same pipeline as wp_spatial6/tools/e9_v6.py (the frame-5 profile, --crf 0), but the worker initializer
patches nvfilter.estimate_flow_v5 with nv_motion.flow_for_v5 instead of the spatial stage.
Output: wp_e9/runs/mtn_<tag>/<clip>/out.mp4 + run.json.
"""
import argparse
import hashlib
import json
import os
import subprocess
import sys
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
E9 = os.path.join(ROOT, "work", "omo", "wp_e9")
USER_INI = os.path.expanduser("~/.config/NeatRepro/nv_ui_qt.ini")
sys.path.insert(0, os.path.join(ROOT, "reference", "python"))
sys.path.insert(1, os.path.join(ROOT, "work", "omo", "wp_motion", "src"))
sys.path.insert(2, os.path.join(ROOT, "work", "omo", "wp_spatial6", "tools"))

_STATE = {}


def install(variant):
    """Patch nvfilter.estimate_flow_v5 with the nv_motion port in this process (variant='nvm')."""
    if _STATE.get("installed") or variant != "nvm":
        return
    import nvfilter as nv
    import nv_motion as nvm

    def _flow(ref, cur, bs=16, search=16):
        return nvm.flow_for_v5(ref, cur, bs, search)

    nv.estimate_flow_v5 = _flow
    _STATE["installed"] = True


def worker_init(prof, P):
    install(os.environ.get("NV6_MTN_VARIANT", "nvm"))
    if os.environ.get("NV6_E9_VARIANT", "v5") != "v5":
        import e9_v6_spatial as ev
        ev.install()
    from nv_ui_qt import backend
    backend._init(prof, P)


def sha256(p):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("clip")
    ap.add_argument("--start", type=int, default=0)
    ap.add_argument("--frames", type=int, default=None)
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--tag", default="nvm")
    ap.add_argument("--set", action="append", default=[],
                    help="override a ui_params value, e.g. --set threshold=8")
    ap.add_argument("--variant", default="nvm", choices=("nvm", "v5"),
                    help="nvm = the nv_motion flow, v5 = the built-in flow (for threshold sweeps)")
    ap.add_argument("--profile-file", default=None,
                    help="use this profile JSON instead of runs/ours_frame_f5 ui_profile.json")
    ap.add_argument("--chroma-noise", type=float, default=None, help="channel_noise Cb/Cr override")
    ap.add_argument("--noise-level", type=float, default=None, help="noise_level override")
    ap.add_argument("--spatial-variant", default=None,
                    choices=("v6set0", "v6set0m", "v6dir"), help="also patch the spatial stage (e9_v6_spatial)")
    a = ap.parse_args()
    ini0 = sha256(USER_INI) if os.path.exists(USER_INI) else None
    src_name, lv = a.clip.split("/")
    src = os.path.join(E9, "clips", src_name, "noisy_%s.y4m" % lv)
    ref = os.path.join(E9, "runs", "ours_frame_f5", src_name, lv)
    prof_p, params_p = os.path.join(ref, "ui_profile.json"), os.path.join(ref, "ui_params.json")
    out = os.path.join(E9, "runs", "mtn_" + a.tag, src_name, lv)
    os.makedirs(out, exist_ok=True)
    dst = os.path.join(out, "out.mp4")
    if os.path.exists(dst):
        os.remove(dst)
    P = json.load(open(params_p))
    for kv in a.set:
        k, v = kv.split("=", 1)
        P[k] = json.loads(v)
    if a.chroma_noise is not None:
        P["channel_noise"] = {**P.get("channel_noise", {}), "Cb": a.chroma_noise, "Cr": a.chroma_noise}
    if a.noise_level is not None:
        P["noise_level"] = a.noise_level
    if a.set or a.chroma_noise is not None or a.noise_level is not None:
        params_p = os.path.join(out, "ui_params.json")
        json.dump(P, open(params_p, "w"), indent=1)
    if a.profile_file:
        prof_p = a.profile_file
    prof = json.load(open(prof_p))
    info = json.load(open(os.path.join(ref, "profile_info.json")))
    rec = {"clip": a.clip, "variant": a.variant, "overrides": dict(kv.split("=", 1) for kv in a.set),
           "input": src, "input_sha256": sha256(src),
           "profile": prof_p, "params": params_p, "profile_info": info, "start": a.start,
           "frames": a.frames, "jobs": a.jobs}
    import e9_v6_spatial as ev  # for worker_init compatibility (no spatial patch: NV6_E9_VARIANT=v5)
    os.environ["NV6_E9_VARIANT"] = a.spatial_variant or "v5"
    if a.spatial_variant in ("v6dir", "v6set0m"):
        # e9_v6_spatial.install は models.npz を要求する
        planes = None
        import e9_v6_spatial as ev2
        import nvfilter as nv2
        sys.path.insert(0, HERE)
        raw = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-i", src,
                              "-vf", "select=eq(n\\,%d)" % int(info["profile_frame"]), "-frames:v", "1",
                              "-f", "rawvideo", "-pix_fmt", "yuv420p", "-"], capture_output=True, check=True).stdout
        W_, H_ = 1920, 1080
        y_ = np.frombuffer(raw[:W_*H_], np.uint8).reshape(H_, W_).astype(np.float32)
        u_ = np.frombuffer(raw[W_*H_:W_*H_*5//4], np.uint8).reshape(H_//2, W_//2).astype(np.float32)
        v_ = np.frombuffer(raw[W_*H_*5//4:], np.uint8).reshape(H_//2, W_//2).astype(np.float32)
        Pv = nv2.v5_params(**P)
        m = ev2.dir_models(prof, Pv, (y_, u_, v_))
        np.savez(os.path.join(out, "models.npz"), **{k: v["model"] for k, v in m.items()})
        os.environ["NV6_E9_MODELS"] = os.path.join(out, "models.npz")
    os.environ["NV6_MTN_VARIANT"] = a.variant
    from nv_ui_qt import backend  # noqa: F401
    import denoise_video_v5 as dv
    import nvfilter as nv

    def ui_profile(planes, full_range=False, *x, **k):
        return prof

    nv.build_profile_v5 = ui_profile
    dv._init, dv._work = worker_init, backend._work
    argv = ["denoise_video_v5.py", src, dst, "--params", params_p, "--jobs", str(a.jobs), "--crf", "0",
            "--audio", "none", "--start", str(a.start)]
    if a.frames:
        argv += ["--frames", str(a.frames)]
    sys.argv = argv
    t0 = time.time()
    dv.main()
    rec["seconds"] = round(time.time() - t0, 1)
    rec["output"] = dst
    rec["output_sha256"] = sha256(dst)
    json.dump(rec, open(os.path.join(out, "run.json"), "w"), indent=1, default=str)
    if ini0 != sha256(USER_INI) if os.path.exists(USER_INI) else None:
        raise SystemExit("%s changed" % USER_INI)
    print("%s nvm: %.0f s -> %s" % (a.clip, rec["seconds"], dst), flush=True)


if __name__ == "__main__":
    main()
