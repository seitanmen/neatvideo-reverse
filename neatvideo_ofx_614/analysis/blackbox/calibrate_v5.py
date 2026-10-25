#!/usr/bin/env python3
"""Calibrate / evaluate nvfilter v5 against the genuine Neat output on native YUV segments.

Segments are 32 raw yuv420p frames extracted with
  ffmpeg -i test_v02.mp4        -vf "select='between(n\\,S\\,S+31)'" -vsync 0 -f rawvideo -pix_fmt yuv420p test_v02_S.yuv
  ffmpeg -i test_v02_reduce.mov -vf (same)                                                       test_v02_reduce_S.yuv
Output frames k = 8..23 of the segment are compared with the genuine frames (8-bit rounded).

usage:
  calibrate_v5.py DIR SEG --search               coordinate-descent fit (prints every trial)
  calibrate_v5.py DIR SEG --params '{"luma":0.7}' [--mode temporal]   evaluate one setting
  calibrate_v5.py DIR SEG --candidates C.json    evaluate [{"name", "mode", "params"[, "area"]}, ...]
                                                 "area": [y0, x0, size] profiling square (default: find_flat_area),
                                                 or {"SEG": [y0, x0, size], ...} per segment
"""
import argparse
import json
import os
import sys
from concurrent.futures import ProcessPoolExecutor

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../../repro"))
import nvfilter as nv  # noqa: E402

W, H, N = 1920, 1080, 32
K = list(range(8, 24))
_G = {}


def load(path):
    raw = np.fromfile(path, np.uint8).reshape(N, W * H * 3 // 2)
    return [(raw[k, :W * H].reshape(H, W).astype(np.float32),
             raw[k, W * H:W * H * 5 // 4].reshape(H // 2, W // 2).astype(np.float32),
             raw[k, W * H * 5 // 4:].reshape(H // 2, W // 2).astype(np.float32)) for k in range(N)]


def _init(path):
    _G["in"] = load(path)


def _flow(job):
    k, o = job
    fr = _G["in"]
    return k, o, nv.estimate_flow_v5(fr[k + o][0], fr[k][0])


def _frame(job):
    k, prof, P, flows = job
    out = nv.process_frame_v5(_G["in"][k - 2:k + 3], 2, prof, P, flows=dict(flows))
    return k, [np.clip(np.rint(o), 0, 255).astype(np.float32) for o in out]


def psnr(a, b):
    return 10 * np.log10(255.0 ** 2 / max(float(((a - b) ** 2).mean()), 1e-12))


def metrics(outs, gen, moving=None):
    """Y/U/V PSNR vs genuine, Y band rms error (L1, L2, L3, rest), frame-diff rms out/genuine;
    with `moving` ({k: bool mask}) also the Y PSNR on moving pixels only ("Ymov")."""
    r = {}
    for c, n in enumerate("YUV"):
        r[n] = float(np.mean([psnr(outs[k][c], gen[k][c]) for k in K]))
    if moving is not None:
        r["Ymov"] = float(np.mean([psnr(outs[k][0][moving[k]], gen[k][0][moving[k]]) for k in K]))
    eb = np.zeros(4)
    for k in K[::4]:
        bo, bg = nv.atrous(outs[k][0], 3), nv.atrous(gen[k][0], 3)
        for j in range(4):
            eb[j] += float((((bo[j] - bg[j])[60:-60, 60:-60]) ** 2).mean())
    r["band_err"] = [round(float(np.sqrt(v / len(K[::4]))), 3) for v in eb]
    r["fd_out"] = float(np.sqrt(np.mean([((outs[k + 1][0] - outs[k][0]) ** 2).mean() for k in K[:-1]])))
    r["fd_gen"] = float(np.sqrt(np.mean([((gen[k + 1][0] - gen[k][0]) ** 2).mean() for k in K[:-1]])))
    return r


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dir")
    ap.add_argument("seg")
    ap.add_argument("--search", action="store_true")
    ap.add_argument("--params", default="{}")
    ap.add_argument("--mode", default="temporal", choices=["spatial", "temporal", "auto"])
    ap.add_argument("--candidates", help="JSON list of {name, mode, params} to evaluate")
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2) - 1))
    A = ap.parse_args()
    src = f"{A.dir}/test_v02_{A.seg}.yuv"
    fr, gen = load(src), load(f"{A.dir}/test_v02_reduce_{A.seg}.yuv")
    gen = {k: gen[k] for k in K}
    from scipy.ndimage import uniform_filter
    # moving pixels: 15x15-mean temporal difference of the input above 1.0 to either neighbour
    moving = {k: (np.abs(uniform_filter(fr[k][0] - fr[k - 1][0], 15)) > 1.0)
              | (np.abs(uniform_filter(fr[k + 1][0] - fr[k][0], 15)) > 1.0) for k in K}
    profs = {m: nv.build_profile_v5(fr[16], pair=fr[17], mode=m, more=(fr[17], fr[18]))
             for m in ("spatial", "temporal", "auto")}
    with ProcessPoolExecutor(A.jobs, initializer=_init, initargs=(src,)) as ex:
        flows = {k: {} for k in K}
        for k, o, f in ex.map(_flow, [(k, o) for k in K for o in (-2, -1, 1, 2)]):
            flows[k][o] = f

        def profile_for(mode, area=None):
            if isinstance(area, dict):
                area = area.get(A.seg)
            if area is None:
                return profs[mode]
            key = (mode, tuple(area))
            if key not in profs:
                profs[key] = nv.build_profile_v5(fr[16], area=tuple(area), pair=fr[17], mode=mode, more=(fr[17], fr[18]))
                print(f"profile[{mode} area={list(area)}] mode={profs[key]['mode']} sigma="
                      f"{json.dumps({n: round(v, 3) for n, v in profs[key]['sigma'].items()})}", flush=True)
            return profs[key]

        def evaluate(mode, P, area=None):
            prof = profile_for(mode, area)
            outs = dict(ex.map(_frame, [(k, prof, P, flows[k]) for k in K]))
            return metrics(outs, gen, moving)

        print("input  vs genuine: " + json.dumps(metrics({k: fr[k] for k in K}, gen, moving)), flush=True)
        print(f"moving-pixel fraction: {np.mean([m.mean() for m in moving.values()]):.3f}", flush=True)
        for m in ("spatial", "temporal", "auto"):
            print(f"profile[{m}] mode={profs[m]['mode']} sigma={json.dumps({n: round(v, 3) for n, v in profs[m]['sigma'].items()})}",
                  flush=True)
        if A.candidates:
            for cand in json.load(open(A.candidates)):
                r = evaluate(cand["mode"], nv.v5_params(**cand["params"]), cand.get("area"))
                print(f"{cand['name']:28s} [{cand['mode']}] Y={r['Y']:.3f} Ymov={r['Ymov']:.3f} U={r['U']:.3f} V={r['V']:.3f} "
                      f"band_err={r['band_err']} fd={r['fd_out']:.3f}/{r['fd_gen']:.3f}", flush=True)
            return
        if not A.search:
            P = nv.v5_params(**json.loads(A.params))
            print(f"[{A.mode}] {json.dumps(json.loads(A.params))} -> {json.dumps(evaluate(A.mode, P))}", flush=True)
            return

        def trial(mode, kw):
            r = evaluate(mode, nv.v5_params(**kw))
            print(f"[{mode}] {json.dumps(kw)} -> Y={r['Y']:.3f} U={r['U']:.3f} V={r['V']:.3f} "
                  f"band_err={r['band_err']} fd={r['fd_out']:.3f}/{r['fd_gen']:.3f}", flush=True)
            return r

        best = {}
        for mode in ("spatial", "temporal"):
            kw = {}
            # 1. temporal threshold (luma metric)
            res = {t: trial(mode, {**kw, "threshold": t}) for t in (0.5, 1.0, 2.0, 4.0, 8.0)}
            kw["threshold"] = max(res, key=lambda t: res[t]["Y"])
            # 2. luminance amount / spatial strength
            res = {(a, s): trial(mode, {**kw, "luma": a, "spatial": s})
                   for a in (0.4, 0.7, 1.0) for s in (0.5, 1.0, 2.0)}
            kw["luma"], kw["spatial"] = max(res, key=lambda t: res[t]["Y"])
            # 3. chroma amount / chroma noise level (chroma metric)
            res = {(a, n): trial(mode, {**kw, "chroma": a, "channel_noise": {"Cb": n, "Cr": n}})
                   for a in (0.5, 1.0) for n in (0.5, 1.0, 2.0, 4.0)}
            a, n = max(res, key=lambda t: res[t]["U"] + res[t]["V"])
            kw["chroma"], kw["channel_noise"] = a, {"Cb": n, "Cr": n}
            # 4. re-check the threshold with the final spatial settings
            res = {t: trial(mode, {**kw, "threshold": t}) for t in (kw["threshold"] / 2, kw["threshold"] * 2)}
            res[kw["threshold"]] = trial(mode, kw)
            kw["threshold"] = max(res, key=lambda t: res[t]["Y"])
            best[mode] = (kw, res[kw["threshold"]])
            print(f"BEST[{mode}] {json.dumps(kw)} -> {json.dumps(res[kw['threshold']])}", flush=True)


if __name__ == "__main__":
    main()
