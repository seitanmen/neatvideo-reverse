"""v5 パイプラインに AutoFineTune を統合する (2026-10-24、2026-10-25 に厳密化)。

本家 FUN_009aae90 (σ マップ) と同じカラーマトリクス結合を使用:
  AFT は RGB 面 (face 0=R, 1=G, 2=B) の相対乗数を返す。
  Y/Cb/Cr の合成乗数は色行列の重みで RGB 面を結合する:
    c_cj = a_j²·m_cj / Σ_j(a_j²·m_cj)   (a_j = モデル q99、m_cj = BT.601 係数の二乗)
    f_c(lum) = √(Σ_j c_cj · f_j(lum))   (f_j = AFT の RGB 面乗数)
"""
import os
import struct
import sys

import numpy as np

F32 = np.float32

sys.dont_write_bytecode = True
_ROOT = os.path.dirname(os.path.abspath(__file__))
_AFT = os.path.join(_ROOT, "..", "neatvideo_ofx_614", "analysis", "noise", "aft_port", "src")
for _p in (_ROOT, _AFT):
    if _p not in sys.path:
        sys.path.insert(0, _p)

# BT.601 係数の二乗 (DAT_0147df80[mode 1]、行 = 出力成分 Y/Cb/Cr、列 = 入力面 R/G/B)
_COLOR_MATRIX_SQ = [
    [0.299 ** 2, 0.587 ** 2, 0.114 ** 2],   # Y
    [0.169 ** 2, 0.331 ** 2, 0.500 ** 2],   # Cb
    [0.500 ** 2, 0.419 ** 2, 0.081 ** 2],   # Cr
]

# モデル面のオフセット (nv_aft.py と同じ)
_FACE_OFFSETS = (0x4C, 0x1FA0, 0x3EF4)  # R, G, B


def _read_model_q99(raw_bytes, band=0):
    """生プロファイルから各 RGB 面 (R, G, B) のモデル q99 (語 5+band) を読む。"""
    f32 = np.frombuffer(raw_bytes, np.float32)
    return [float(f32[_FACE_OFFSETS[j] // 4 + 5 + band]) for j in range(3)]


def _color_weights(q99, band=0):
    """FUN_009aae90 の c_cj = a_j²·m_cj / Σ(a_j²·m_cj) を計算する。

    q99: モデル q99 値 (a_j) のリスト (R, G, B)
    戻り値: 3×3 の重み [成分 Y/Cb/Cr][面 R/G/B]
    """
    a_sq = [q99[j] ** 2 for j in range(3)]
    weights = []
    for ci in range(3):  # Y, Cb, Cr
        denom = sum(a_sq[j] * _COLOR_MATRIX_SQ[ci][j] for j in range(3))
        if denom <= 0:
            weights.append([1.0 / 3.0] * 3)
        else:
            weights.append([a_sq[j] * _COLOR_MATRIX_SQ[ci][j] / denom for j in range(3)])
    return weights


def apply_aft(prof, rgb, mode=0, area=None, matrix="bt709", band=0):
    """v5 の prof dict に AFT を適用する (厳密版: カラーマトリクス結合)。

    本家 FUN_009aae90 と同じ式で RGB 面 (R, G, B) の AFT 乗数を
    Y/Cb/Cr の合成乗数に変換してから v5 の曲線に乗算する:
      c_cj = a_j²·m_cj / Σ(a_j²·m_cj)     (a_j = モデル q99、m_cj = BT.601²)
      f_c(lum) = √(Σ_j c_cj · f_j(lum))   (f_j = AFT の RGB 面乗数曲線)

    prof: build_profile_v5() の戻り値 (dict)
    rgb:  (H, W, 3) float32 0..255
    mode: AFT モード (0=通常)
    area: (y0, x0, size) か長方形 (y0, x0, h, w) — None なら prof["area"] を使う
    band: 重みの計算に使う帯域 (0 = 最細帯域、既定)

    戻り値: AFT 適用後の prof (dict、curve が更新される)
    """
    import nv_profile as npr
    import nv_aft

    rgb = np.asarray(rgb, F32)
    a = prof["area"] if area is None else area
    ay, ax, ah = a[:3]
    aw = a[3] if len(a) == 4 else ah           # (y0, x0, size) or a rectangle (y0, x0, h, w)
    area4 = (ax, ay, aw, ah)                   # nv_profile.build_profile: (x, y, w, h)

    # 1. BuildNoiseModel の生プロファイルを生成 (RGB 面のモデル q99 を含む)
    result = npr.build_profile(rgb, 1, area4)
    raw = result[0] if isinstance(result, tuple) else result

    # 2. AFT 適用 → RGB 面 (R, G, B) の相対乗数曲線 3 本が得られる
    aft_bytes = nv_aft.auto_fine_tune(rgb, raw, mode=mode)
    aft_f32 = np.frombuffer(aft_bytes, np.float32)

    # 3. モデル q99 からカラーマトリクス重みを計算
    q99 = _read_model_q99(raw, band=band)
    cw = _color_weights(q99, band=band)
    # cw[0] = Y の重み (R, G, B)
    # cw[1] = Cb の重み
    # cw[2] = Cr の重み

    # 4. AFT の RGB 面乗数曲線を読む (9 ビン、各面)
    n_v5 = len(prof["curve_centers"])
    raw_centers = np.linspace(0, 256, 9)
    v5_centers = np.array(prof["curve_centers"])
    face_curves = []  # face_curves[j] = np.interp の結果 (face j = R/G/B)
    for j in range(3):  # R, G, B
        curve_off = 0xbf44 + 0x120 * j
        raw_vals = np.array([float(aft_f32[(curve_off + 0x20 * k) // 4]) for k in range(9)])
        interp = np.interp(v5_centers, raw_centers, raw_vals)
        face_curves.append(interp)

    # 5. カラーマトリクス結合: f_c = √(Σ_j c_cj · f_j)
    #    (本家 σ_c = √(Σ_j c_cj·T_j) と同じ式。T_j = AFT 乗数)
    combined = []
    for ci in range(3):  # Y, Cb, Cr
        weighted_sum = sum(cw[ci][j] * face_curves[j] for j in range(3))
        f_combined = np.sqrt(np.maximum(weighted_sum, F32(0)))
        combined.append(f_combined)

    # 6. v5 の曲線に乗算
    for ci, name in enumerate(("Y", "Cb", "Cr")):
        original = np.array(prof["curve"][name], F32)
        prof["curve"][name] = (original * combined[ci]).astype(F32).tolist()

    # 7. sigma 概算値も更新
    lvl = prof.get("area_level", 128.0)
    centers = prof["curve_centers"]
    prof["sigma"] = {n: float(np.interp(lvl, centers, prof["curve"][n]) * prof["tot_ratio"][n])
                     for n in ("Y", "Cb", "Cr")}

    # デバッグ情報を保存
    prof["aft_applied"] = True
    prof["aft_face_curves"] = [fc.tolist() for fc in face_curves]
    prof["aft_color_weights"] = cw
    prof["aft_band"] = band
    # 曲線ビンの旗 (Quality 表示が本家の曲線バイトを再現するため。フィルタ計算には使わない。
    # 2026-10-26: 本家 AFT は未測定ビンにだけ +0x1c マーカーを立てる (測定ビンは 0 のまま)。
    # 全ビンに立てると Quality/Completeness が高く出る。旗は RGB 面 (R, G, B) 順)
    _markers, _clips, _gt3s = [], [], []
    for _j in range(3):
        _mk, _cl, _gt = [], [], []
        for _k in range(9):
            _o = 0xbf44 + 0x120 * _j + 0x20 * _k
            _mk.append(struct.unpack_from("<I", aft_bytes, _o + 0x1c)[0] != 0)
            _cl.append(aft_bytes[_o + 0x1d] != 0)
            _gt.append(aft_bytes[_o + 0x1e] != 0)
        _markers.append(_mk)
        _clips.append(_cl)
        _gt3s.append(_gt)
    prof["aft_bin_flags"] = {"marker": _markers, "clip": _clips, "gt3": _gt3s}
    return prof


def assist_params(P, mode="profile_check"):
    """Profile Check / Noise Level Check 用のフィルタパラメータを返す。

    本家の Assist Image は空間フィルタのみを全帯域 100% で掛けた結果を表示する。
    これにより「空間フィルタが何を取り除くか」が可視化され、
    ユーザーは Noise Level が適切かを判断できる。

    P: 通常の v5_params() の戻り値
    戻り値: Assist 用に変更したパラメータ dict
    """
    import copy
    A = copy.deepcopy(P)
    if mode == "profile_check":
        # 空間フィルタのみ (時間フィルタはオフ)
        A["temporal"] = 0.0
        # 全帯域 100% (最大のノイズ除去)
        A["band_amounts"] = {b: 1.0 for b in A["band_amounts"]}
        # Noise Level はそのまま (プロファイルの推定値をそのまま使う)
        # Mix なし
        A["mix"] = 0.0
    elif mode == "noise_level_check":
        # 時間フィルタは有効のまま
        # 空間フィルタのみ全帯域 100%
        A["band_amounts"] = {b: 1.0 for b in A["band_amounts"]}
        A["mix"] = 0.0
    return A
