# nv_spatial.py — 本家空間フィルタ (Filter 0x9ac2b0 CPU 経路) の参照移植 (wp_spatial)
#
# 対象: FUN_0098edb0 の高速経路 (方向フィルタなし)。数学的構造は notes/spatial_analysis.md
# (逆アセンブルで確定した分)。σ 基底は v5 のノイズモデル成分で代用 (下記「既知の代用」)。
#  - 分解:  L_{k+1} = 1/2↓(box3x3(L_k));  B_k = L_k - up(L_{k+1})   (up = (3,1)/4 x2)
#  - 白色化: W = B_k / (sigma_k + eps)        (eps = 0x38d1b717 = 9.4433e-4)
#  - 縮小:  R = sqrt(box3x3(W^2));  X = W * clamp((2t - R)/t, 0, 1);  t = 1.1 * t_scale
#           (縮小核は FUN_00990f74: clamp ランプ FUN_009ee93c 相当、しきい値の 1.1 は
#            FUN_0099afb4 と同じ定数)
#  - 再構成: ACC = up(ACC) - A_k * X * sigma_k;  OUT = 入力 + ACC   (A_k = Amount × バンドゲイン)
# 方向フィルタ経路 (FUN_00990f74 の model 分岐、組 1..4) と周期パターン検出 (FUN_00ad06ec) は
# 未実装 (静的解析のみ)。段別の輝度テーブル (+0xbf40) と色行列も v5 の curve_sigma で代用。
# 単位は 0..255 (本家の内部スケール)。
import numpy as np

EPS_WHITEN = 9.4433e-4  # 0x38d1b717 (float32)


def _f32(a):
    return np.ascontiguousarray(a, np.float32)


def box3(a):
    """FUN_009ed014: 3x3 箱平均 (1/9)。端は行/列の複製。"""
    h, w = a.shape
    ar = np.empty((h, w + 2), np.float32)
    ar[:, 0], ar[:, -1], ar[:, 1:-1] = a[:, 0], a[:, -1], a
    hs = ar[:, :-2] + ar[:, 1:-1] + ar[:, 2:]
    br = np.empty((h + 2, w), np.float32)
    br[0], br[-1], br[1:-1] = hs[0], hs[-1], hs
    return (br[:-2] + br[1:-1] + br[2:]) * np.float32(1.0 / 9.0)


def halve(a):
    """FUN_009f15b4: 隣接 2x2 平均の 1/2 縮小 (床除算)。"""
    h2, w2 = a.shape[0] >> 1, a.shape[1] >> 1
    if h2 == 0 or w2 == 0:
        return a.copy()
    r0, r1 = a[0:2 * h2:2], a[1:2 * h2:2]
    return (r0[:, 0:2 * w2:2] + r1[:, 0:2 * w2:2] + r0[:, 1:2 * w2:2] + r1[:, 1:2 * w2:2]) * np.float32(0.25)


def _shift(a, axis, back):
    """back=True で a[i-1]、False で a[i+1] (端は複製)。"""
    idx = list(range(a.shape[axis]))
    if back:
        idx = [idx[0]] + idx[:-1]
    else:
        idx = idx[1:] + [idx[-1]]
    return np.take(a, idx, axis=axis)


def _up1(a, n_out, axis):
    """FUN_00993df0 の 1 軸 (3,1)/4 x2 拡大: out[2i]=(3a[i]+a[i-1])/4, out[2i+1]=(3a[i]+a[i+1])/4。
    n_out が奇数 (2n+1) のとき最後の 1 行/列は末端の複製。"""
    n = a.shape[axis]
    e = 3.0 * a
    lo = (e + _shift(a, axis, True)) * np.float32(0.25)
    hi = (e + _shift(a, axis, False)) * np.float32(0.25)
    if axis == 0:
        out = np.empty((2 * n, lo.shape[1]), np.float32)
        out[0::2], out[1::2] = lo, hi
    else:
        out = np.empty((lo.shape[0], 2 * n), np.float32)
        out[:, 0::2], out[:, 1::2] = lo, hi
    if n_out > 2 * n:
        out = np.concatenate([out, out[-1:] if axis == 0 else out[:, -1:]], axis=axis)
    return out


def up2(a, shape):
    """FUN_00993df0 相当: 水平→垂直の順の (3,1)/4 x2 拡大。"""
    return _up1(_up1(a, shape[1], 1), shape[0], 0)


def local_rms(a):
    """FUN_009950f4: sqrt(3x3 箱平均(x^2))。"""
    return np.sqrt(np.maximum(box3(a * a), 0.0)).astype(np.float32)


def _pyramid(plane, levels):
    """FUN_0098edb0 の分解。bands[k] = L_k - up(L_{k+1}) (k=0 が最細)、resid = 最粗段。"""
    bands, cur = [], _f32(plane)
    for _ in range(levels):
        coarse = halve(box3(cur))
        bands.append((cur - up2(coarse, cur.shape)).astype(np.float32))
        cur = coarse
    return bands, cur


def _halve_n(a, n):
    for _ in range(n):
        a = halve(a)
    return a


def spatial_genuine(planes, prof, rv, P):
    """v5 の spatial_v5 と同じ署名・同じノイズ成分 (curve × band_ratio × Noise Level × sqrt(rv))
    を使う本家構造の空間段 (参照版)。σ を段解像度へ 1/2 縮小して使い、縮小は白色化 + 局所 RMS
    のしきい値ランプ。t_scale は P["genuine_t"] (既定 1.0 → t = 1.1)。"""
    import nvfilter as nv  # base/ へのコピー (COMMON §2)
    Y = np.asarray(planes[0], np.float32)
    lum = nv.gaussian_filter(Y, 1.0)
    t = np.float32(1.1 * float(P.get("genuine_t", 1.0)))
    out = []
    for c, (name, p) in enumerate(zip(("Y", "Cb", "Cr"), planes)):
        L = prof["levels"][name]
        names = nv.V5_BANDS[len(nv.V5_BANDS) - L:] if c else nv.V5_BANDS[:L]
        A = float(P["luma"] if c == 0 else P["chroma"])
        s1 = (nv.curve_sigma(prof, name, nv._to_grid(lum, p.shape)) * P["noise_level"]
              * P["channel_noise"][name] * np.sqrt(rv[c]))
        plane = _f32(p)
        bands, _resid = _pyramid(plane, L)
        acc = None
        for k in range(L - 1, -1, -1):
            s = _halve_n(s1 * prof["band_ratio"][name][k] * P["band_noise"][names[k]], k)
            W = bands[k] / (s + np.float32(EPS_WHITEN))
            X = (W * np.clip((2.0 * t - local_rms(W)) / t, 0.0, 1.0)).astype(np.float32)
            acc = up2(acc, bands[k].shape) if acc is not None else np.zeros(bands[k].shape, np.float32)
            amt = min(max(A * P["band_amounts"][names[k]], 0.0), 1.0)
            acc = acc - np.float32(amt) * X * s
        out.append(plane + acc)
    return out
