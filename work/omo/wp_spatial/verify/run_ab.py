#!/usr/bin/env python3
"""区間 A/B (test_v02, n=296/560, 32f) で v5 と空間段差し替えを比較する (wp_spatial)。

手順は blackbox/calibrate_v5.py (読み取り専用) の複写: 評価は出力 k=8..23 を本家出力
(ネイティブ YUV, 8bit 丸め) と比較し、Y/U/V PSNR・Ymov・帯域別誤差・フレーム差 RMS を出す。
候補 (verify/candidates_ab.json): {"name","mode","params","genuine"}。genuine=true は
process_frame_v5 内の spatial_v5 を nv_spatial.spatial_genuine に差し替える。
"""
import argparse
import json
import os
import sys
from concurrent.futures import ProcessPoolExecutor

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "base"))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src"))   # 2026-10-21: nv_spatial は src/ だけ
import nvfilter as nv      # noqa: E402
import nv_spatial as nvs   # noqa: E402

_orig_spatial = nv.spatial_v5


def _spatial_maybe_genuine(planes, prof, rv, P):
    P = dict(P)
    if P.pop("_genuine", False):
        return nvs.spatial_genuine(planes, prof, rv, P)
    return _orig_spatial(planes, prof, rv, P)


nv.spatial_v5 = _spatial_maybe_genuine

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
    ap.add_argument("--candidates", required=True)
    ap.add_argument("--jobs", type=int, default=6)
    A = ap.parse_args()
    src = f"{A.dir}/test_v02_{A.seg}.yuv"
    fr, gen = load(src), load(f"{A.dir}/test_v02_reduce_{A.seg}.yuv")
    gen = {k: gen[k] for k in K}
    from scipy.ndimage import uniform_filter
    moving = {k: (np.abs(uniform_filter(fr[k][0] - fr[k - 1][0], 15)) > 1.0)
              | (np.abs(uniform_filter(fr[k + 1][0] - fr[k][0], 15)) > 1.0) for k in K}
    profs = {m: nv.build_profile_v5(fr[16], pair=fr[17], mode=m, more=(fr[17], fr[18]))
             for m in ("spatial", "temporal", "auto")}
    print("input  vs genuine: " + json.dumps(metrics({k: fr[k] for k in K}, gen, moving)), flush=True)
    print(f"moving-pixel fraction: {np.mean([m.mean() for m in moving.values()]):.3f}", flush=True)
    for m in profs:
        print(f"profile[{m}] mode={profs[m]['mode']} flat={profs[m].get('flat_status')} "
              f"sigma={json.dumps({n: round(v, 3) for n, v in profs[m]['sigma'].items()})}", flush=True)
    with ProcessPoolExecutor(A.jobs, initializer=_init, initargs=(src,)) as ex:
        flows = {k: {} for k in K}
        for k, o, f in ex.map(_flow, [(k, o) for k in K for o in (-2, -1, 1, 2)]):
            flows[k][o] = f

        def evaluate(mode, cand):
            P = dict(nv.v5_params(**cand.get("params", {})))
            if cand.get("genuine"):
                P["_genuine"] = True
            outs = dict(ex.map(_frame, [(k, profs[mode], P, flows[k]) for k in K]))
            return metrics(outs, gen, moving)

        for cand in json.load(open(A.candidates)):
            r = evaluate(cand.get("mode", "auto"), cand)
            print(f"{cand['name']:20s} [{cand.get('mode','auto')}] "
                  f"Y={r['Y']:.3f} Ymov={r['Ymov']:.3f} U={r['U']:.3f} V={r['V']:.3f} "
                  f"band_err={r['band_err']} fd={r['fd_out']:.3f}/{r['fd_gen']:.3f}", flush=True)


if __name__ == "__main__":
    main()
