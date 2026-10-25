#!/usr/bin/env python3
"""区間 A/B (test_v02, n=296/560, 32f) で、v5 の時間段の出力に本家移植の空間段 (nv_spatial.filter_frame) を掛けて本家出力と比べる。

    python3 verify/run_ab_frame.py data 296 [--jobs 3] [--frames 8..23]

- 時間段は v5 (temporal_v5、既定値)。空間段だけを差し替える。
- プロファイル: v5 auto と同じ領域 (本家移植の FindFlatArea、nvfilter.find_flat_area) で、本家とビット一致の BuildNoiseModel
  (repro/nv_profile.build_profile、色モード 1、LZMA なし) を n=16 の RGB から作る。AutoFineTune は通さない (輝度別の表は 1)。
- RGB はホスト想定の変換 (nvfilter.yuv_to_rgb、BT.709 制限範囲、色差は 2×2 複製) を 0..255 に切ったもの。出力は逆変換して
  色差を 2×2 平均で 4:2:0 に戻し、8 ビットに丸めて比べる。
- 設定は nv_spatial.default_settings (推定値) と、その変種 (候補の "settings" で上書き。キーはバイトオフセットの 10 進/16 進文字列)。
評価は run_ab.py と同じ (出力 k=8..23 の Y/U/V PSNR、Ymov、帯域別誤差、フレーム差 RMS)。repro/out/ には書かない。
"""
import argparse
import json
import os
import sys
import time
from concurrent.futures import ProcessPoolExecutor

import numpy as np

sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "base"))
sys.path.insert(0, os.path.join(HERE, "..", "src"))
sys.path.insert(0, HERE)
import nvfilter as nv          # noqa: E402
import nv_spatial as sp        # noqa: E402
import run_ab as RA            # noqa: E402

W, H, N = RA.W, RA.H, RA.N
K = RA.K
KR, KB = nv._KRB["bt709"]
_G = {}


def to_rgb(planes):
    rgb = np.clip(np.nan_to_num(nv.yuv_to_rgb(planes, "bt709", False), nan=0.0, posinf=255.0, neginf=0.0), 0, 255)
    return np.ascontiguousarray(rgb.astype(np.float32).transpose(2, 0, 1))


def to_yuv(rgb):
    r, g, b = (rgb[c].astype(np.float64) for c in range(3))
    y = KR * r + (1 - KR - KB) * g + KB * b
    cb = (b - y) / (2 * (1 - KB))
    cr = (r - y) / (2 * (1 - KR))
    Y = y * 219 / 255 + 16
    Cb = cb * 224 / 255 + 128
    Cr = cr * 224 / 255 + 128

    def sub(p):
        return 0.25 * (p[0::2, 0::2] + p[1::2, 0::2] + p[0::2, 1::2] + p[1::2, 1::2])
    return [np.clip(np.rint(p), 0, 255).astype(np.float32) for p in (Y, sub(Cb), sub(Cr))]


def _init(path, prof_gen, settings):
    _G["in"] = RA.load(path)
    _G["prof"] = prof_gen
    _G["settings"] = settings


def _frame(job):
    k, prof, P, flows, settings = job
    win = _G["in"][k - 2:k + 3]
    t0 = time.time()
    tmp, rv = nv.temporal_v5(win, 2, prof, P, dict(flows))
    rgb = to_rgb(tmp)
    out = sp.filter_frame(rgb, _G["prof"], settings)
    return k, to_yuv(out), time.time() - t0


def settings_from(cand):
    s = sp.default_settings()
    for kk, v in cand.get("settings", {}).items():
        s[int(kk, 0)] = tuple(v) if isinstance(v, list) else v
    return s


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dir")
    ap.add_argument("seg")
    ap.add_argument("--candidates", default=None)
    ap.add_argument("--jobs", type=int, default=3)
    A = ap.parse_args()
    src = f"{A.dir}/test_v02_{A.seg}.yuv"
    fr, gen = RA.load(src), RA.load(f"{A.dir}/test_v02_reduce_{A.seg}.yuv")
    gen = {k: gen[k] for k in K}
    from scipy.ndimage import uniform_filter
    moving = {k: (np.abs(uniform_filter(fr[k][0] - fr[k - 1][0], 15)) > 1.0)
              | (np.abs(uniform_filter(fr[k + 1][0] - fr[k][0], 15)) > 1.0) for k in K}
    prof = nv.build_profile_v5(fr[16], pair=fr[17], mode="auto", more=(fr[17], fr[18]))
    y0, x0, size = prof["area"]
    import nv_profile as npf
    t0 = time.time()
    rgb16 = to_rgb(fr[16]).transpose(1, 2, 0)
    pg, _miss = npf.build_profile(np.ascontiguousarray(rgb16), 1, (int(x0), int(y0), int(size), int(size)), lzma=False)
    print(f"area y0={y0} x0={x0} size={size} flat_status={prof.get('flat_status')} genuine profile {time.time() - t0:.1f} s", flush=True)
    cands = json.load(open(A.candidates)) if A.candidates else [{"name": "genuine-port-default"}]
    P = dict(nv.v5_params())
    with ProcessPoolExecutor(A.jobs, initializer=_init, initargs=(src, pg, None)) as ex:
        flows = {k: {} for k in K}
        for k, o, f in ex.map(_flow_job, [(src, k, o) for k in K for o in (-2, -1, 1, 2)]):
            flows[k][o] = f
        for cand in cands:
            st = settings_from(cand)
            res = list(ex.map(_frame, [(k, prof, P, flows[k], st) for k in K]))
            outs = {k: o for k, o, _ in res}
            secs = [s for _, _, s in res]
            r = RA.metrics(outs, gen, moving)
            print(f"{cand['name']:24s} Y={r['Y']:.3f} Ymov={r['Ymov']:.3f} U={r['U']:.3f} V={r['V']:.3f} "
                  f"band_err={r['band_err']} fd={r['fd_out']:.3f}/{r['fd_gen']:.3f} sec/frame={np.mean(secs):.0f}", flush=True)


def _flow_job(job):
    src, k, o = job
    fr = _G["in"]
    return k, o, nv.estimate_flow_v5(fr[k + o][0], fr[k][0])


if __name__ == "__main__":
    main()
