"""v5 パイプラインに AutoFineTune を統合する (2026-10-24)。

nv_aft (静的移植) を使い、v5 の prof["curve"] に AFT の輝度別係数を反映する。
nv_profile.build_profile (生 0xc300 バイト) を経由するため低速 (~15 秒) だが、
本家と同じメカニズムで曲線を調整する。

使い方:
  from nv_aft_v5 import apply_aft
  prof = build_profile_v5(planes, ...)   # 通常の v5 プロファイル
  prof = apply_aft(prof, rgb)            # AFT 適用後の prof (curve が更新される)
"""
import os
import sys

import numpy as np

F32 = np.float32

sys.dont_write_bytecode = True
_ROOT = os.path.dirname(os.path.abspath(__file__))
_AFT = os.path.join(_ROOT, "..", "neatvideo_ofx_614", "analysis", "noise", "aft_port", "src")
for _p in (_ROOT, _AFT):
    if _p not in sys.path:
        sys.path.insert(0, _p)


def apply_aft(prof, rgb, mode=0, area=None, matrix="bt709"):
    """v5 の prof dict に AFT を適用する。

    prof: build_profile_v5() の戻り値 (dict)
    rgb:  (H, W, 3) float32 0..255 (プロファイル用フレームの RGB)
    mode: AFT モード (0=通常, 1=第2画像使用, 2=モード2)
    area: (y0, x0, size) — None なら prof["area"] を使う

    戻り値: AFT 適用後の prof (dict、curve が更新される)
    """
    import nv_profile as npr
    import nv_aft

    rgb = np.asarray(rgb, F32)
    if area is None:
        ay, ax, asz = prof["area"][:3]
    else:
        ay, ax, asz = area[:3]
    area4 = (ax, ay, asz, asz)

    # 1. BuildNoiseModel の生プロファイルを生成
    result = npr.build_profile(rgb, 1, area4)  # mode 1 = YCrCb
    raw = result[0] if isinstance(result, tuple) else result

    # 2. AFT 適用
    aft_bytes = nv_aft.auto_fine_tune(rgb, raw, mode=mode)

    # 3. AFT 調整後の曲線を読み取り、v5 の prof dict に反映
    #    AFT は相対スケーリング係数を返す (生プロファイルの曲線は 1.0 基準)。
    #    v5 の曲線は絶対 σ 値なので、AFT の係数を「乗算」する (上書きではない)。
    aft_f32 = np.frombuffer(aft_bytes, np.float32)
    n_v5 = len(prof["curve_centers"])
    raw_centers = np.linspace(0, 256, 9)
    v5_centers = np.array(prof["curve_centers"])
    for c, name in enumerate(("Y", "Cb", "Cr")):
        curve_off = 0xbf44 + 0x120 * c
        raw_vals = [float(aft_f32[(curve_off + 0x20 * k) // 4]) for k in range(9)]
        # 9 点 → v5 の 8 中心で線形補間 = AFT の相対スケーリング係数
        aft_factor = np.interp(v5_centers, raw_centers, raw_vals)
        # v5 の絶対 curve に AFT の相対係数を乗算
        original = np.array(prof["curve"][name], F32)
        prof["curve"][name] = (original * aft_factor).astype(F32).tolist()

    # sigma 概算値も更新
    lvl = prof.get("area_level", 128.0)
    centers = prof["curve_centers"]
    prof["sigma"] = {n: float(np.interp(lvl, centers, prof["curve"][n]) * prof["tot_ratio"][n])
                     for n in ("Y", "Cb", "Cr")}
    prof["aft_applied"] = True
    return prof
