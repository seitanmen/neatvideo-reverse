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
import struct
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
    """Patch nvfilter in this process: variant='nvm' swaps the flow, 'pyrgen' swaps pyramid_weight
    for the genuine structure (normalize by the model q99 per level instead of s1*ratios)."""
    if _STATE.get("installed"):
        return
    import nvfilter as nv
    if variant == "nvm":
        import nv_motion as nvm

        def _flow(ref, cur, bs=16, search=16):
            return nvm.flow_for_v5(ref, cur, bs, search)

        nv.estimate_flow_v5 = _flow
    elif variant == "sim3":
        # the genuine's mode-2 similarity: one weight from the 3-plane normalized difference
        _DF, _WP, _TG = nv.dense_flow, nv.warp_plane, nv._to_grid
        _UF, _GF, _CS = nv.uniform_filter, nv.gaussian_filter, nv.curve_sigma
        _NBVAR = nv.NEIGHBOUR_VAR

        def _tv(window, ci, prof, P, flows=None):
            cur = [np.asarray(p, np.float32) for p in window[ci]]
            Y = cur[0]
            a = float(P["temporal"])
            thr = max(P["threshold"], 1e-3)
            den = {}
            for c, name in enumerate(("Y", "Cb", "Cr")):
                plane = cur[c] if c == 0 else _TG(cur[c], Y.shape)
                st = (_CS(prof, name, _GF(plane, 1.0)) * prof["tot_ratio"][name]
                      * P["noise_level"] * P["channel_noise"][name])
                den[c] = 2.0 * st * st + 1e-6
            acc = [p.copy() for p in cur]
            sw = [np.ones(p.shape, np.float32) for p in cur]
            sf = [np.zeros(p.shape, np.float32) for p in cur]
            for k, nb in enumerate(window):
                if k == ci or a <= 0:
                    continue
                if flows is not None and (k - ci) in flows:
                    vy, vx = flows[k - ci]
                else:
                    vy, vx = nv.estimate_flow_v5(nb[0], Y, P["bs"], P["search"])
                    if flows is not None:
                        flows[k - ci] = (vy, vx)
                FY, FX = _DF(vy, vx, Y.shape, P["bs"])
                warped = [nb[0]]
                wmaps = []
                D = _UF((warped[0] - Y) ** 2, 5) / den[0]
                for c in (1, 2):
                    p = nb[c]
                    if p.shape == Y.shape:
                        wp = _WP(p, FY, FX)
                        curs = cur[c]
                        dc = _UF((wp - curs) ** 2, 5) / den[c]
                    else:
                        wp = _WP(p, _TG(FY, p.shape) / 2, _TG(FX, p.shape) / 2)
                        curs = cur[c]
                        dc = _UF((wp - curs) ** 2, 5) / _TG(den[c], p.shape)
                    warped.append(wp)
                    D = D + (_TG(dc, Y.shape) if dc.shape != Y.shape else dc)
                w = np.exp(-np.maximum(D / 3.0 - 1.0, 0.0) / thr)
                for c, p in enumerate(nb):
                    wp = warped[c] if c == 0 or warped[c].shape == p.shape else \
                        _WP(p, _TG(FY, p.shape) / 2, _TG(FX, p.shape) / 2)
                    wc = w if c == 0 or w.shape == p.shape else _TG(w, p.shape)
                    acc[c] += wc * wp
                    sw[c] += wc
                    sf[c] += _NBVAR * wc * wc
            out, rv = [], []
            for c, p in enumerate(cur):
                out.append(p + a * (acc[c] / sw[c] - p))
                rv.append(((1 - a) + a / sw[c]) ** 2 + a * a * sf[c] / (sw[c] * sw[c]))
            return out, rv

        nv.temporal_v5 = _tv
    elif variant == "sim3b":
        # mode 2 decoded (FUN_0097bbb0): the difference = (dY + dCb + dCr) / 3 with NO per-channel
        # normalization; the similarity normalizes by the COMBINED noise variance of the mean
        _DF, _WP, _TG = nv.dense_flow, nv.warp_plane, nv._to_grid
        _UF, _GF, _CS = nv.uniform_filter, nv.gaussian_filter, nv.curve_sigma

        def _tv(window, ci, prof, P, flows=None):
            cur = [np.asarray(p, np.float32) for p in window[ci]]
            Y = cur[0]
            a = float(P["temporal"])
            thr = max(P["threshold"], 1e-3)
            # the combined noise variance of the 3-plane mean: (sY^2 + sCb^2 + sCr^2) / 9
            var = None
            for c, name in enumerate(("Y", "Cb", "Cr")):
                plane = cur[c] if c == 0 else _TG(cur[c], Y.shape)
                st = (_CS(prof, name, _GF(plane, 1.0)) * prof["tot_ratio"][name]
                      * P["noise_level"] * P["channel_noise"][name])
                v = st * st
                var = v if var is None else var + (_TG(v, Y.shape) if v.shape != Y.shape else v)
            den = var / 9.0 * 2.0 + 1e-6
            acc = [p.copy() for p in cur]
            sw = [np.ones(p.shape, np.float32) for p in cur]
            sf = [np.zeros(p.shape, np.float32) for p in cur]
            for k, nb in enumerate(window):
                if k == ci or a <= 0:
                    continue
                if flows is not None and (k - ci) in flows:
                    vy, vx = flows[k - ci]
                else:
                    vy, vx = nv.estimate_flow_v5(nb[0], Y, P["bs"], P["search"])
                    if flows is not None:
                        flows[k - ci] = (vy, vx)
                FY, FX = _DF(vy, vx, Y.shape, P["bs"])
                warped = [_WP(nb[0], FY, FX)]
                diffs = [warped[0] - Y]
                for c in (1, 2):
                    p = nb[c]
                    if p.shape == Y.shape:
                        wp = _WP(p, FY, FX)
                    else:
                        wp = _WP(p, _TG(FY, p.shape) / 2, _TG(FX, p.shape) / 2)
                    warped.append(wp)
                    dwp = wp - cur[c]                    # at the channel's own resolution
                    diffs.append(_TG(dwp, Y.shape) if dwp.shape != Y.shape else dwp)
                d = (diffs[0] + diffs[1] + diffs[2]) * np.float32(1.0 / 3.0)
                w = np.exp(-np.maximum(_UF(d * d, 5) / den - 1.0, 0.0) / thr)
                for c, p in enumerate(nb):
                    wp = warped[c] if warped[c].shape == p.shape else \
                        _WP(p, _TG(FY, p.shape) / 2, _TG(FX, p.shape) / 2)
                    wc = w if w.shape == p.shape else _TG(w, p.shape)
                    acc[c] += wc * wp
                    sw[c] += wc
                    sf[c] += nv.NEIGHBOUR_VAR * wc * wc
            out, rv = [], []
            for c, p in enumerate(cur):
                out.append(p + a * (acc[c] / sw[c] - p))
                rv.append(((1 - a) + a / sw[c]) ** 2 + a * a * sf[c] / (sw[c] * sw[c]))
            return out, rv

        nv.temporal_v5 = _tv
    elif variant == "sim3c":
        # per-channel similarity: each plane is averaged with its own weight from its own warped
        # difference normalized by its own sigma (the Cr flicker no longer drags Y and vice versa)
        _DF, _WP, _TG = nv.dense_flow, nv.warp_plane, nv._to_grid
        _UF, _GF, _CS = nv.uniform_filter, nv.gaussian_filter, nv.curve_sigma

        def _tv(window, ci, prof, P, flows=None):
            cur = [np.asarray(p, np.float32) for p in window[ci]]
            Y = cur[0]
            a = float(P["temporal"])
            thr = max(P["threshold"], 1e-3)
            den = {}
            for c, name in enumerate(("Y", "Cb", "Cr")):
                plane = cur[c]
                st = (_CS(prof, name, _GF(plane, 1.0)) * prof["tot_ratio"][name]
                      * P["noise_level"] * P["channel_noise"][name])
                den[c] = 2.0 * st * st + 1e-6
            acc = [p.copy() for p in cur]
            sw = [np.ones(p.shape, np.float32) for p in cur]
            sf = [np.zeros(p.shape, np.float32) for p in cur]
            for k, nb in enumerate(window):
                if k == ci or a <= 0:
                    continue
                if flows is not None and (k - ci) in flows:
                    vy, vx = flows[k - ci]
                else:
                    vy, vx = nv.estimate_flow_v5(nb[0], Y, P["bs"], P["search"])
                    if flows is not None:
                        flows[k - ci] = (vy, vx)
                FY, FX = _DF(vy, vx, Y.shape, P["bs"])
                for c in range(3):
                    p = nb[c]
                    if p.shape == Y.shape:
                        wp = _WP(p, FY, FX) if c == 0 else _WP(p, FY, FX)
                        wc_shape = None
                    else:
                        wp = _WP(p, _TG(FY, p.shape) / 2, _TG(FX, p.shape) / 2)
                    d = wp - cur[c]
                    dmap = _TG(den[c], p.shape) if den[c].shape != p.shape else den[c]
                    w = np.exp(-np.maximum(_UF(d * d, 5) / dmap - 1.0, 0.0) / thr)
                    acc[c] += w * wp
                    sw[c] += w
                    sf[c] += nv.NEIGHBOUR_VAR * w * w
            out, rv = [], []
            for c, p in enumerate(cur):
                out.append(p + a * (acc[c] / sw[c] - p))
                rv.append(((1 - a) + a / sw[c]) ** 2 + a * a * sf[c] / (sw[c] * sw[c]))
            return out, rv

        nv.temporal_v5 = _tv
    elif variant == "nocrt":
        # diagnostic: keep the temporal stage for Y/Cb but pass Cr through untouched
        _orig = nv.temporal_v5

        def _tv(window, ci, prof, P, flows=None):
            out, rv = _orig(window, ci, prof, P, flows)
            out = list(out)
            rv = list(rv)
            out[2] = np.asarray(window[ci][2], np.float32)
            rv[2] = np.float32(1.0)
            return out, rv

        nv.temporal_v5 = _tv
    elif variant == "pyrgen2":
        # the genuine structure per NOTES.md §14-§18: threshold t = 1.3*|model word 3| (constant
        # across levels, negative word 3 = the level is skipped), normalization X = band/(sigma_k+1e-4)
        # with sigma_k = the model word 5+k, R = sqrt(3x3 box mean of X^2), coarse-to-fine 4 levels,
        # w_k = clamp(2 - R/t, 0, 1) * w_{k-1}
        import struct as _st
        sk_path = os.environ["NV6_SIGMA_K"]
        sk = json.load(open(sk_path))            # [word3, ?, ?, ?, ?, q99_0..q99_4] — computed in main
        word3 = sk[0]
        q99 = sk[5:10]
        t = 1.3 * abs(word3)

        def _pw(d, s1, ratios, scale=1.0):
            b = nv.atrous(d, 4)                   # b[0] = the finest ... b[3] = the coarsest
            w = np.ones(d.shape, np.float32)
            for k in (3, 2, 1, 0):                # coarse -> fine
                sig = q99[k] if k < len(q99) else q99[-1]
                if sig <= 0:
                    sig = abs(q99[k - 1]) * 0.5 if k > 0 and q99[k - 1] > 0 else 1.0
                x = b[k] / (sig + 1e-4)
                r = np.sqrt(nv.uniform_filter(x * x, 3))   # the 3x3 box
                if t > 0:
                    w = w * np.clip((2.0 * t - r) / t, 0.0, 1.0)
            return w.astype(np.float32)

        nv.pyramid_weight = _pw
    elif variant == "pyrgen":
        sk = json.load(open(os.environ["NV6_SIGMA_K"]))

        def _pw(d, s1, ratios, scale=1.0):
            b = nv.atrous(d, len(nv._PYR_C))
            w = np.ones(d.shape, np.float32)
            for j, c in enumerate(nv._PYR_C):
                v = sk[j] if j < len(sk) else sk[-1]
                if v <= 0:                     # the "level too small" marker: reuse the previous level
                    v = abs(sk[j - 1]) * 0.5 if j > 0 and sk[j - 1] > 0 else 1.0
                x = b[j] / (v + 1e-3)
                r = np.sqrt(nv.uniform_filter(x * x, 2 ** (j + 1) + 1))
                t = max(scale, 1e-3) * c * nv._PYR_K
                w *= np.clip((2.0 * t - r) / t, 0.0, 1.0)
            return w

        nv.pyramid_weight = _pw
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
    ap.add_argument("--variant", default="nvm", choices=("nvm", "v5", "pyrgen", "pyrgen2", "nocrt", "sim3", "sim3b", "sim3c"),
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
        try:
            P[k] = json.loads(v)
        except json.JSONDecodeError:
            P[k] = v
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
    if a.variant in ("pyrgen", "pyrgen2"):
        # U4: the genuine per-level sigma = the model q99 (words 5+k) of the profile region's Y plane
        import nv_model
        raw = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-i", src,
                              "-vf", "select=eq(n\\,%d)" % int(info["profile_frame"]), "-frames:v", "1",
                              "-f", "rawvideo", "-pix_fmt", "yuv420p", "-"], capture_output=True, check=True).stdout
        W_, H_ = 1920, 1080
        y_ = np.frombuffer(raw[:W_*H_], np.uint8).reshape(H_, W_).astype(np.float32)
        ar = prof["area"]
        reg = y_[ar[0]:ar[0]+(ar[2] if len(ar) == 3 else ar[2]),
                 ar[1]:ar[1]+(ar[2] if len(ar) == 3 else ar[3])] if len(ar) == 3 else \
              y_[ar[0]:ar[0]+ar[2], ar[1]:ar[1]+ar[3]]
        m = nv_model.model_full(reg, set_=-1, levels=5)
        sk = [struct.unpack("<f", struct.pack("<I", int(m[k]) & 0xFFFFFFFF))[0] for k in range(10)]
        skf = os.path.join(out, "sigma_k.json")
        json.dump(sk, open(skf, "w"))
        os.environ["NV6_SIGMA_K"] = skf
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
