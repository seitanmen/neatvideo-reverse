#!/usr/bin/env python3
"""nv_curve.py — Neat Video 6 (arm64) の EstimateNoiseLevelCurve (FUN_009b6c8c) の外側の移植。

画素単位のノイズ推定 (FUN_00998d98) は別モジュール `nv_pixnoise.pixnoise(luma, weight) ->
(noise, weight, bright)` (nv_lines.py の線カーネル・箱フィルタを使う) に委ね (`_default_pixnoise` が
本モジュールの規約 (bright, noise, weight) に並べ替える)、ここでは次を本家と同じ演算順で再現する。

  (a) タイル分割     FUN_00ac1194(sched, W, H, 96, 96, 12, 0, 0x30, 0, 0, W, H, 0) →
                     FUN_00abfc50 (行優先の 96px 格子) → FUN_00ac03c0 (余白 12px、最小 48px)
  (b) タイル処理     FUN_009b76b0: 16x16 ブロックの IsClip (R または B のクリップ率 > 0.33 で重み 0)、
                     輝度 = 0.2989 R + 0.5866 G + 0.1145 B (FUN_0097bb88 → FUN_009f11fc、float32)、
                     画素ノイズ → コア領域 (余白を除く) で重み >= 0.5 の画素を 8 ビンに集計
                     (bin = (long)((float)8 * (1/256) * bright)、和は double)
  (c) 曲線           FUN_009b6c8c: 画素数 < (W*H)>>10 のビンは無効 (-1)、有効ビン (> 0) を
                     最小二乗の多項式 (FUN_009d43f4: 正規方程式を Cramer の公式 FUN_009d41b0 /
                     FUN_009d4708 で解く) で補間・外挿、[1,2,1]/4 平滑、x_i = 32 (i + 0.5)
  (d) 補間器         FUN_00aeda08 / FUN_00aedb18: 節点間は線形、外側は両端 2 点の直線で外挿

float の演算は本家と同じ順序で行い、ビット一致を狙う: 輝度と融合積和は nv_pixnoise の 1 か所の実装
(luma_601 / fma32) を使う。IsClip は最小・最大 (FUN_009efdd4: SIMD 部は NaN が伝播、端数部は NaN を
無視) と fcvtzs の飽和まで再現し、ビン番号は frintn (最近接丸め)、double の累積は画素順 (np.add.at)、
多項式は正規方程式 + Cramer の公式を同じ順で評価する (融合積和なし、pow は libSystem の pow)。
ビット一致の基準は 1 スレッドで動く本家 (ハーネス nvh は DAT_014a67c0 = 1)。ホストが本家を複数スレッドで
走らせると、FUN_009b6c8c はスレッドごとの double の部分和 (sums[thread][bin]) をビンごとにスレッド順に足すので、
画素の和を 1 本にタイル順で足す本移植とは ys の末尾のビットが異なり得る (ハーネスでは確かめられない)。
差が出得るのは、ビンの double の和が丸めを含む入力だけである。実写フレームと通常の合成画像では、タイルの順や
画素の順を逆にしても ys のビットは変わらなかった (観測、verify_curve.py の順序の集計)。変わったのは、巨大な値
(1e13〜1e20) の画素が少数だけ通常の値に混ざる入力に限られた (画像全体を 1e18 倍しても変わらない)。従って
通常の入力では、複数スレッドの本家とも一致するとみられる (推測)。
入力はチャンネルが最後の (H, W, C >= 3) の配列 (チャンネル 0..2 が R, G, B) で H, W >= 48、nbins >= 1。
満たさなければ genuine_noise_curve は ValueError (本家は 48px 未満でスケジューラがアサーションになる。
ハーネスでは 500x20 で安全装置が作動した)。
照合 (2026-10-16。`nvh curve` / `nvh interp` は %.17g で表示するので double をそのまま比べ、許容誤差なし):
  analysis/noise/verify_curve.py (nv_pixnoise + nv_lines を組み込んだ genuine_noise_curve vs `nvh curve`):
  test_v02 n=0/312/576・test.mp4 n=300・合成 1920x1080/640x360/100x70/97x61 × {勾配+ガウス, 黒帯,
  白飛び, 平坦 128}・8 ビットの範囲外 3 種 (x1e18、1e20 / 1e25 の高輝度ブロック: NaN を含む曲線) の
  23 ケースで 8 点の ys が 184/184 ビット一致。ys に NaN を含むケースは自動でタイル別にも照合する (`nvh pixnoise`
  の 3 面を NaN のペイロードまで、タイル別 (sums, counts)、非有限・巨大な値のブロックの IsClip)。
  各ケースで、タイルの順・タイル内の画素の順を逆にして足すと移植の ys が変わるか (加算順に敏感か) も数える。
  敏感なケースで ys がビット一致することは、移植の加算順 (スケジューラ順のタイルごと、タイル内は行優先) と
  矛盾しない。ただし同じ ys を出す誤った順もある。本家との比較で排除できたのは、タイルの逆順・画素の逆順・
  列優先・タイルごとの部分和・ビンごとのペアワイズ和・先頭 2 タイルの入れ替え・順番待ちなし (2026-10-16 の
  再検証)。順番待ちのタイルを足す順 (現タイルより先・逆順) は本家と区別できず、verify_curve_shell の
  殻どうしの比較で確かめる。--sensitivity は、加算順の故意の誤りが本家との比較で検出されることを確かめる。
  --extra (レビューだけが確かめていた入力: 実写フレームを並べた 3840x2160 / 4096x2160 / 7680x4320、実写の
  切り出し 48x48〜1366x768、クリップしない RGB と整数の RGB、画素数がしきい値ちょうど / 1 少ないビン、外挿・内側の
  当てはめが 0 以下になる分岐、有限で巨大な ys) の 20 ケースと --fuzz 200 (48〜260px の乱数画像、非有限・巨大・負・
  微小・量子化した値、高速経路と参照経路) もビット一致。
  --diagnose: 全タイルで `nvh pixnoise` の 3 面とビット一致、
  16x16 ブロックの IsClip は R・B のどちらかが 0 でない全ブロック + 0.33±0.05 + 無作為抽出を `nvh clip`
  と照合して不一致 0、タイル別 (sums, counts) も同一。
  analysis/noise/verify_curve_shell.py: タイル配置 9 サイズ、IsClip 103/103 と非有限・巨大な面 39/39、
  補間器 2232/2232 問い合わせ (2〜8 点の表、範囲外・節点・極端な値)、本家 pixnoise を差し込んだ殻の
  16 曲線がビット一致。
速度 (2026-10-16、M2 Max、1 スレッド): genuine_noise_curve は既定で高速経路 (batch=True:
tile_histogram_batch) を使い 1080p 1 フレーム約 0.7 秒 (本家 0.44 秒、参照実装 11.5 秒)、4096x2160 約 2.8 秒
(参照実装 48.6 秒)。同じ大きさのタイルを最大 MAX_BATCH 枚まとめて nv_pixnoise.pixnoise_batch (nv_lines の
*_bm 経路) に通し、16x16 の IsClip は is_clip_blocks で一括。加算は本家の順 (タイル順・行優先) にタイルごとの
np.add.at で行う。大きさ別のバッチで先に済んだタイルは、採用画素 (ビン番号と float32 のノイズ) だけを順番が
来るまで保持し、足したらすぐ捨てる。ビン番号は nbins が収まる最小の符号なし整数型 (8 ビンなら uint8) で持つ。
バッチは先頭タイルの順に処理するので、順番待ちはおよそ 1 バッチ分まで。
入力の面は切り出しごとに float32 にする (フレーム全体の複製を作らない)。
tracemalloc の最大は 1920x1080 / 4096x2160 / 7680x4320 で 251 / 363 / 889 → 224 / 249 / 239 MiB
(この変更の前 → 後、verify_curve_speed.py --mem)。時間は、交互計測で 1080p が約 2% 遅くなり、4096x2160 では
差が測れない (数値は exp_results.txt)。
batch=False は従来の参照実装 (tile_histogram、タイルごと) のまま。
  analysis/noise/verify_curve_speed.py --orig: 19 画像 (test_v02 n=0/312/576、test.mp4 n=300、合成 1920x1080 /
  640x360 / 97x61 / 48x48 / 101x67 / 333x211 / 49x50 / 4096x2160 / 1001x777、黒帯・白飛び、平坦、範囲外 3 種) で
  (xs, ys, sums, counts) が参照実装とも高速化前のモジュール (analysis/noise/frozen_20261016/、別プロセス) とも
  ビット一致。verify_curve_shell.py は両方の殻を、和の丸めが加算順で変わる代用 pixnoise で比べる
  (4 通りのバッチの大きさ、逆順・ランダム順のタイル列、nbins 256 / 257 / 300 で一致。加算順の故意の誤り 2 種を検出)。
numpy と標準ライブラリ (ctypes で libSystem の pow) のみに依存する。
"""
import ctypes
import math
import os
import sys

import numpy as np

_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)
import nv_pixnoise as _npx  # noqa: E402  (FUN_00998d98, the float helpers and the luma)

_F32 = np.float32
_F64 = np.float64

TILE = 96                 # DAT_01244570
MARGIN = 12               # DAT_01244578
MIN_TILE = 48             # FUN_00ac1194 の第 8 引数 0x30 (余白込みのタイルの最小辺)
BLOCK = 16                # DAT_01244580 (IsClip のブロック)
CLIP_THR = _F32(0.33)     # DAT_01244588 (0x3ea8f5c3)
NBINS = 8                 # DAT_012445a0
LUMA_RGB = _npx.LUMA_601  # 0.2989 0.5866 0.1145 (0x3e99096c 0x3f162b6b 0x3dea7efa)
_FLT_MAX = np.finfo(_F32).max
_INT64_MIN, _INT64_MAX = -(1 << 63), (1 << 63) - 1

try:                      # the plugin calls libSystem's pow; math.pow raises on overflow / domain errors
    _libm = ctypes.CDLL("libSystem.B.dylib")
    _libm.pow.restype = ctypes.c_double
    _libm.pow.argtypes = [ctypes.c_double, ctypes.c_double]
    _pow = _libm.pow
except OSError:           # not macOS: numpy's float64 power (the C library pow, no exceptions)
    def _pow(x, y):
        with np.errstate(all="ignore"):
            return float(np.power(_F64(x), _F64(y)))


# ----------------------------------------------------------------------------------------
# (a) タイル分割
# ----------------------------------------------------------------------------------------
def _tile_desc(x, y, w, h, W, H, margin, min_size):
    """FUN_00ac03c0: コア (x, y, w, h) に余白を付けたタイル記述子 (11 整数)。
    余白は画像の縁で切り詰め、辺が min_size 未満なら手前側 → 奥側の順に余白を広げる。"""
    lx = x if x < margin else margin
    rx = margin if margin + w + x <= W else (W - x) - w
    ty = y if y < margin else margin
    by = margin if margin + h + y <= H else (H - y) - h
    tw = lx + w + rx
    th = ty + h + by
    if tw < min_size:
        lx2 = (min_size - tw) + lx
        lx = x if lx2 > x else lx2
        tw = lx + w + rx
        if tw < min_size:
            rx2 = (min_size - tw) + rx
            rx = rx2 if rx2 + w + x <= W else (W - x) - w
            tw = rx + lx + w
    if th < min_size:
        ty2 = (min_size - th) + ty
        ty = y if ty2 > y else ty2
        th = ty + h + by
        if th < min_size:
            by2 = (min_size - th) + by
            by = by2 if by2 + h + y <= H else (H - y) - h
            th = by + ty + h
    return (x - lx, y - ty, tw, th, x, y, w, h, lx, ty, 0)


def tile_layout(W, H, tile=TILE, margin=MARGIN, min_size=MIN_TILE):
    """FUN_00abfc50 の順 (行優先) のタイル記述子のリスト。各要素は
    (x, y, w, h, core_x_img, core_y_img, core_w, core_h, core_x, core_y, 0):
    x, y, w, h は余白込みのタイル (画像座標)、core_* は余白を除いた領域 (画像座標と
    タイル内オフセット)。`nvh tiles W H` と同じ。"""
    W, H = int(W), int(H)
    if W < min_size or H < min_size:
        # FUN_00abf6a8 はどちらかの辺が 0x30 未満だとアサーション (ダイアログ) で止まる。ハーネスでも
        # 500x20 で安全装置が作動した (終了コード 3)。黙って不正なタイルを作らず、ここで拒否する
        raise ValueError(f"image {W}x{H} is smaller than {min_size} px: the genuine scheduler asserts")
    ncols = (W + tile - 1) // tile if tile else 0
    nrows = (H + tile - 1) // tile if tile else 0
    out = []
    y = 0
    for _ in range(nrows):
        h = min(tile, H - y)
        x = 0
        for _ in range(ncols):
            w = min(tile, W - x)
            out.append(_tile_desc(x, y, w, h, W, H, margin, min_size))
            x += w
        y += h
    return out


# ----------------------------------------------------------------------------------------
# (b) タイル処理
# ----------------------------------------------------------------------------------------
def _fcvtzs(v):
    """fcvtzs (float -> int64, 切り捨て) の飽和: NaN → 0、2^63 以上 (+inf を含む) → INT64_MAX、
    -2^63 未満 → INT64_MIN。v は float32 の配列 (int64 の配列を返す)。"""
    v = np.asarray(v, _F32).astype(_F64)
    out = np.zeros(v.shape, np.int64)
    ok = (v < 2.0 ** 63) & (v >= -(2.0 ** 63))          # NaN は False
    out[ok] = np.trunc(v[ok]).astype(np.int64)
    out[v >= 2.0 ** 63] = _INT64_MAX
    out[v < -(2.0 ** 63)] = _INT64_MIN
    return out


def _minmax_genuine(v, simd=True):
    """FUN_009efdd4 (float 配列の最小・最大): 最小は FLT_MAX、最大は -FLT_MAX から始め、SIMD 経路
    (DAT_014a67a8 != 0) は先頭 n & ~7 要素を fmin / fmax (NaN が伝播する) で、残りをスカラー経路
    (fcmp; csel gt / mi: NaN は無視、すでに NaN なら NaN のまま) で処理する。"""
    n = v.size
    m = (n & ~7) if simd else 0
    mn, mx = _F32(_FLT_MAX), _F32(-_FLT_MAX)
    if m:
        bmn, bmx = v[:m].min(), v[:m].max()   # numpy's min / max propagate a NaN like fmin / fmax
        if np.isnan(bmn) or np.isnan(bmx):
            mn = mx = _F32(np.nan)
        else:
            mn, mx = min(mn, bmn), max(mx, bmx)
    if m < n:
        tail = v[m:]
        tail = tail[~np.isnan(tail)]      # the scalar compares skip NaN elements
        if tail.size:                     # (a NaN minimum / maximum stays NaN)
            if not np.isnan(mx):
                mx = max(mx, tail.max())
            if not np.isnan(mn):
                mn = min(mn, tail.min())
    return _F32(mn), _F32(mx)


def _fcvtzs1(x):
    """_fcvtzs of one float32 value (Python int)."""
    x = float(x)
    if x != x:
        return 0
    if x >= 2.0 ** 63:
        return _INT64_MAX
    if x < -(2.0 ** 63):
        return _INT64_MIN
    return int(x)


def is_clip_genuine(plane, simd=True):
    """FUN_009c6f88 (IsClip、float 面): 面の値域を 1/8 刻み (nb = (int)(range*8+1)、上限 0x4000)
    のヒストグラムにし、最頻ビン (同数なら先頭) の割合を、そのビンが過半数か、分布の端
    (片側が他方の 10 倍以上) のときだけ返す。ビン番号は (int)rint(k*(v-mn)) で、k = (float)(nb-1)/range。
    0x9c70bc の frintn (最近接・偶数丸め) を再現する (nvfilter.is_clip は切り捨てで、値域の狭い面で
    本家と食い違う)。値域が 1/8 未満 (nb < 2) なら 1.0。割合は float32 の (float)peak/(float)n。
    非有限・巨大な値も本家どおり: 最小・最大は FUN_009efdd4 (_minmax_genuine)、nb とビン番号は
    fcvtzs の飽和 (NaN → 0、+inf → INT64_MAX) で、NaN / inf を含む 16x16 ブロックは 1.0、値域 * 8 が
    float32 で inf になる面は nb = 0x4000。空の面は値域 -inf → 1.0。ビン番号が [0, nb) を外れる
    ことは有限の値では起きない (本家は範囲外に書き込む。ここでは端に寄せる)。"""
    v = np.ascontiguousarray(plane, _F32).ravel()
    n = v.size
    mn, mx = _minmax_genuine(v, simd)
    with np.errstate(over="ignore", invalid="ignore"):
        rng = _F32(mx - mn)
        nb = _fcvtzs1(_F32(_F32(rng * _F32(8.0)) + _F32(1.0)))
        if nb < 2:
            return _F32(1.0)
        nb = min(nb, 0x4000)
        k = _F32(_F32(nb - 1) / rng)
        r = np.rint(k * (v - mn))                     # fsub, fmul, frintn
    idx = r.astype(np.int64) if np.isfinite(r).all() else _fcvtzs(r)    # fcvtzs
    h = np.bincount(np.clip(idx, 0, nb - 1), minlength=nb)
    m = int(np.argmax(h))
    peak, below, above = int(h[m]), int(h[:m].sum()), int(h[m + 1:].sum())
    if n < 2 * peak or below * 10 < above or above * 10 < below:
        return _F32(_F32(peak) / _F32(n))
    return _F32(0.0)


def clip_weight(R, G, B, block=BLOCK, thr=CLIP_THR, is_clip=None):
    """FUN_009b76b0 のクリップ判定: タイルを 16x16 ブロックに分け (行・列の最後のブロックは
    タイルの端に寄せる)、各ブロックの R と B の IsClip 率のどちらかが 0.33 を超えれば (G は見ない)
    そのブロックの重みを 0 にする。重みの初期値は 1.0 (DAT_00c962e0)。
    判定は 0x9b7b7c-0x9b7b84 の順序付き比較 fcmp R, thr / fccmp B, thr, #0, le / b.le (b.le で
    ブロックを残す): 0 にするのは (R > thr) または (B > thr) のときだけで、NaN は「超えない」側
    (順序なしの比較は le が成り立ち残す)。is_clip の既定は is_clip_genuine (本家と同値)。"""
    if is_clip is None:
        is_clip = is_clip_genuine
    R = np.asarray(R, _F32)
    B = np.asarray(B, _F32)
    h, w = R.shape
    weight = np.ones((h, w), _F32)
    nrow = (h + block - 1) // block
    ncol = (w + block - 1) // block
    for r in range(nrow):
        by = r * block if block <= h - r * block else h - block
        for c in range(ncol):
            bx = c * block if block <= w - c * block else w - block
            # タイルが 16px 未満 (画像が 48px 未満のときだけ) は本家の範囲外読みになる。ここでは 0 に留める
            by0, bx0 = max(by, 0), max(bx, 0)
            cr = _F32(is_clip(R[by0:by0 + block, bx0:bx0 + block]))
            cb = _F32(is_clip(B[by0:by0 + block, bx0:bx0 + block]))
            if cr > thr or cb > thr:               # fcmp / fccmp ... b.le keeps the block (NaN too)
                weight[by0:by0 + block, bx0:bx0 + block] = 0
    return weight


def luma_bt601(R, G, B, simd=True):
    """FUN_0097bb88 → FUN_009f11fc: 輝度 = R*0.2989 + G*0.5866 + B*0.1145 (float32)。実装は
    nv_pixnoise.luma_601 の 1 か所だけ (SIMD 経路は先頭 n & ~3 要素を fma(B, c, fma(G, b, a*R))、
    残りを (R*a + G*b) + B*c。simd=False は非 SIMD 経路)。この名前は互換のために残す。"""
    return _npx.luma_601(R, G, B, simd)


def bin_index(bright, nbins=NBINS):
    """bin = (long)((float)nbins * 0.00390625f * bright) を [0, nbins-1] に制限 (fcvtzs: NaN → 0)。"""
    scale = _F32(_F32(nbins) * _F32(0.00390625))
    v = np.trunc(scale * np.asarray(bright, _F32))
    v = np.where(np.isnan(v), 0, v)
    return np.clip(v, 0, nbins - 1).astype(np.int64)


def accumulate(bright, noise, weight, desc, sums, counts, nbins=NBINS):
    """タイルのコア領域 (desc の core_x, core_y, core_w, core_h) の画素を行優先の順に
    sums[bin] += (double)noise、counts[bin] += 1 で集計する (重み >= 0.5 の画素のみ。
    fcmp ... b.mi なので NaN の重みは含める)。sums は float64、counts は int64 (累積・更新)。"""
    cx, cy, cw, ch = desc[8], desc[9], desc[6], desc[7]
    sl = (slice(cy, cy + ch), slice(cx, cx + cw))
    w = np.asarray(weight, _F32)[sl].ravel()
    keep = ~(w < _F32(0.5))
    b = np.asarray(bright, _F32)[sl].ravel()[keep]
    n = np.asarray(noise, _F32)[sl].ravel()[keep]
    if b.size == 0:
        return
    idx = bin_index(b, nbins)
    np.add.at(sums, idx, n.astype(_F64))    # 画素順に逐次加算 (ufunc.at は添字順)
    counts += np.bincount(idx, minlength=nbins)[:nbins]


# ----------------------------------------------------------------------------------------
# (c) 多項式あてはめ (FUN_009d43f4 / FUN_009d41b0 / FUN_009d4708 / FUN_009d4890)
# ----------------------------------------------------------------------------------------
def _fdiv(a, b):
    """IEEE の double 除算 (0 除算で例外を出さない)。"""
    with np.errstate(divide="ignore", invalid="ignore"):
        return float(_F64(a) / _F64(b))


def _det(m, n):
    """FUN_009d4708: 第 1 列に沿った余因子展開。det = Σ_i sign_i * (m[i][0] * det(minor_i)) を
    i の順に加算 (sign は +1, -1, ...)。"""
    if n == 1:
        return m[0][0]
    if n < 1:
        return 0.0
    det = 0.0
    sign = 1.0
    for i in range(n):
        minor = [row[1:n] for k, row in enumerate(m[:n]) if k != i]
        t = m[i][0] * _det(minor, n - 1)
        t = sign * t
        det = det + t
        sign = -sign
    return det


def _cramer(a, b, n):
    """FUN_009d41b0: coeffs[k] = det(A の k 列を b に置換) / det(A)。"""
    d = _det(a, n)
    out = []
    for k in range(n):
        mk = [list(row) for row in a]
        for r in range(n):
            mk[r][k] = b[r]
        out.append(_fdiv(_det(mk, n), d))
    return out


def polyfit_genuine(xs, ys, npts, ncoef):
    """FUN_009d43f4(xs, ys, npts, coeffs, ncoef): 正規方程式 A c = b、
    b[i] = Σ_p ys[p] * pow(xs[p], i)、A[i][j] = Σ_p pow(xs[p], i + j) (p の順に double で累積、
    融合積和なし。pow は libSystem の pow = _pow で、溢れても例外にせず inf / NaN を返す)。"""
    xs = [float(v) for v in xs[:npts]]
    ys = [float(v) for v in ys[:npts]]
    if ncoef < 1:
        return []
    b = [0.0] * ncoef
    a = [[0.0] * ncoef for _ in range(ncoef)]
    for i in range(ncoef):
        s = 0.0
        for p in range(npts):
            s = s + ys[p] * _pow(xs[p], float(i))
        b[i] = s
        for j in range(ncoef):
            s = 0.0
            for p in range(npts):
                s = s + _pow(xs[p], float(i + j))
            a[i][j] = s
    return _cramer(a, b, ncoef)


def polyval_genuine(x, coeffs, ncoef):
    """FUN_009d4890: Σ_i pow(x, i) * coeffs[i] を i の順に double で累積 (pow は _pow)。"""
    x = float(x)
    s = 0.0
    for i in range(ncoef):
        s = s + _pow(x, float(i)) * float(coeffs[i])
    return s


# ----------------------------------------------------------------------------------------
# (c) 曲線 (FUN_009b6c8c の集計後)
# ----------------------------------------------------------------------------------------
def curve_means(sums, counts, W, H, nbins=NBINS):
    """ビンの平均: counts >= (W*H) >> 10 なら sums / (double)counts、それ以外は -1.0。"""
    thr = (int(W) * int(H)) >> 10
    m = np.empty(nbins, _F64)
    for i in range(nbins):
        c = int(counts[i])
        m[i] = _fdiv(float(sums[i]), float(c)) if c >= thr else -1.0
    return m


def fill_curve(means, nbins=NBINS):
    """無効ビンの補間・外挿 (0x9b7140-0x9b7368)。有効 = 平均 > 0 (fcmp ... b.ls: NaN も有効)。
    - 有効 0 個: 全ビン 1.0、1 個: 全ビンその値
    - 2 個以上で欠けがある: 全有効点の多項式 (3 点以上で 2 次、2 点で 1 次) で内側の穴を埋め、
      先頭側は first < nbins/3 なら先頭 min(3, n) 点の直線、そうでなければ先頭 1 点の定数で
      下向きに外挿 (値 <= 0 なら隣をコピー)、末尾側は last >= 2*nbins/3 なら末尾 min(3, n) 点の
      直線、そうでなければ末尾 1 点の定数で上向きに外挿 (同様)。"""
    m = np.array(means, _F64).copy()
    xs, ys = [], []
    first, last = -1, -1
    for i in range(nbins):
        v = float(m[i])
        if not (v <= 0.0):
            if first < 0:
                first = i
            xs.append(float(i))
            ys.append(v)
            last = i
    npts = len(xs)
    if npts < 2:
        m[:] = ys[0] if npts == 1 else 1.0
        return m
    if npts == nbins:
        return m
    ncoef = 3 if npts >= 3 else 2
    c = polyfit_genuine(xs, ys, npts, ncoef)
    for i in range(first + 1, last):
        if not (m[i] > 0.0):
            m[i] = polyval_genuine(float(i), c, ncoef)
    if first > 0:
        third = nbins // 3
        if first < third:
            nfit, nc = (3 if npts > 2 else 2), 2
        else:
            nfit, nc = 1, 1
        c = polyfit_genuine(xs[:nfit], ys[:nfit], nfit, nc)
        for i in range(first - 1, -1, -1):
            v = polyval_genuine(float(i), c, nc)
            m[i] = v
            if v <= 0.0:
                m[i] = m[i + 1]
    if last + 1 < nbins:
        two3 = (2 * nbins) // 3
        if last >= two3:
            nfit, nc = (3 if npts > 2 else 2), 2
        else:
            nfit, nc = 1, 1
        c = polyfit_genuine(xs[npts - nfit:], ys[npts - nfit:], nfit, nc)
        for i in range(last + 1, nbins):
            v = polyval_genuine(float(i), c, nc)
            m[i] = v
            if v <= 0.0:
                m[i] = m[i - 1]
    return m


def smooth_curve(m, nbins=NBINS):
    """[1,2,1]/4: out[i] = (m[i] + (m[i-1] + m[i+1]) * 0.5) * 0.5、両端はコピー。"""
    m = np.asarray(m, _F64)
    out = np.empty(nbins, _F64)
    out[0] = m[0]
    out[nbins - 1] = m[nbins - 1]
    for i in range(1, nbins - 1):
        out[i] = (m[i] + (m[i - 1] + m[i + 1]) * 0.5) * 0.5
    return out


def curve_xs(nbins=NBINS):
    """x_i = (double)(float32(256 / nbins) * (float32(i) + 0.5f))。"""
    step = _F32(_F32(256.0) / _F32(nbins))
    return np.array([float(step * (_F32(i) + _F32(0.5))) for i in range(nbins)], _F64)


def curve_table(sums, counts, W, H, nbins=NBINS):
    """集計 (sums: double, counts: 画素数) → 補間器に渡る (xs, ys) (各 nbins 個の double)。"""
    ys = smooth_curve(fill_curve(curve_means(sums, counts, W, H, nbins), nbins), nbins)
    return curve_xs(nbins), ys


# ----------------------------------------------------------------------------------------
# (d) 補間器 (FUN_00aeda08 / FUN_00aedb18)
# ----------------------------------------------------------------------------------------
class CurveInterp:
    """BaseInterpolator: 節点 (xs, ys)、xs[0] 未満は先頭 2 点、xs[n-1] 以上は末尾 2 点の
    1 次多項式 (FUN_00aeda08 が FUN_009d43f4 で 2 点 2 係数を求める) で外挿、間は線形補間。
    評価は FUN_00aedb18 の命令どおり (double、融合積和なし):
      q < xs[0] (fcmp / b.le: 順序なしは素通り) → 下の直線、xs[n-1] <= q (b.ls) → 上の直線、
      それ以外は j = 0 から区間を探す: xs[j] > q か順序なし (b.hi) なら進め、そうでなければ
      xs[j+1] > q (b.gt) で区間 k = j + 1 に決まる。進めるたびに lo = xs[j+1] <= q (b.ls) を見る。
      t = (q - xs[k-1]) / (xs[k] - xs[k-1])、値 = ys[k-1] * (1 - t) + t * ys[k]。
    n < 2 は本家が配列の外を読む (FUN_00aeda08 が xs[1] を読む) ので ValueError。区間が見つからない
    まま j = n - 2 に達すると本家は k = n で配列の外 (xs[n], ys[n]) を読む。これは q か節点に NaN が
    あるときだけ起こり、q が NaN なら結果は外の値によらず NaN になる。ここでは NaN を返す。"""

    def __init__(self, xs, ys):
        self.xs = np.asarray(xs, _F64).copy()
        self.ys = np.asarray(ys, _F64).copy()
        n = self.n = len(self.xs)
        if n < 2 or len(self.ys) != n:
            raise ValueError(f"the genuine interpolator needs n >= 2 nodes and as many values (got {n}, {len(self.ys)})")
        self.lo = polyfit_genuine(self.xs[:2], self.ys[:2], 2, 2)
        self.hi = polyfit_genuine(self.xs[n - 2:], self.ys[n - 2:], 2, 2)

    def __call__(self, x):
        x = float(x)
        xs, ys, n = self.xs, self.ys, self.n
        if x < xs[0]:
            return polyval_genuine(x, self.lo, 2)
        if xs[n - 1] <= x:
            return polyval_genuine(x, self.hi, 2)
        j = 0
        check = xs[0] <= x                       # not (b.hi): ordered and xs[0] <= q
        while True:
            if check and xs[j + 1] > x:          # b.gt
                k = j + 1
                break
            if j == n - 2:                       # the genuine continues with k = n (past the arrays)
                return math.nan
            j += 1
            check = xs[j] <= x                   # b.ls
        x0 = float(xs[k - 1])
        t = (x - x0) / (float(xs[k]) - x0)
        return float(ys[k - 1]) * (1.0 - t) + t * float(ys[k])


def curve_eval(table, x):
    """table = (xs, ys) または CurveInterp。x はスカラーか配列。"""
    if not isinstance(table, CurveInterp):
        table = CurveInterp(*table)
    if np.ndim(x) == 0:
        return table(x)
    flat = np.asarray(x, _F64).ravel()
    return np.array([table(v) for v in flat], _F64).reshape(np.shape(x))


# ----------------------------------------------------------------------------------------
# 駆動部
# ----------------------------------------------------------------------------------------
def _default_pixnoise():
    """nv_pixnoise.pixnoise (FUN_00998d98 の移植) を本モジュールの呼び出し規約
    pixnoise(luma, weight) -> (bright, noise, weight) に合わせる (nv_pixnoise は
    (noise, weight, bright) の順で返す)。nv_pixnoise は冒頭で 1 度だけ import 済み
    (sys.path にはこのディレクトリが無いときだけ 1 度足す)。"""
    def _pix(luma, weight):
        noise, weight, bright = _npx.pixnoise(luma, weight)
        return bright, noise, weight
    return _pix


def _cancelled(cancel, n=1):
    """n cancel queries as the worker FUN_009b76b0 makes them (one before each tile it takes from the
    scheduler, one more that finds the scheduler empty); stops at the first that reports a cancel."""
    if cancel is not None:
        for _ in range(n):
            if cancel():
                return True
    return False


def tile_histogram(rgb, pixnoise=None, nbins=NBINS, is_clip=None, tiles=None, cancel=None):
    """全タイルを処理して (sums, counts) を返す (1 スレッドの本家と同じ順)。参照実装 (タイルごと)。
    rgb は (H, W, 3 以上)。tiles を省略すると tile_layout (48px 未満は ValueError)。
    cancel: 取り消しの問い合わせ (引数なしで真なら取り消し)。本家のワーカーと同じく各タイルの前と最後に 1 回ずつ
    問い合わせ、真なら None を返す (本家の EstimateNoiseLevelCurve の失敗)。"""
    if pixnoise is None:
        pixnoise = _default_pixnoise()
    rgb = _check_rgb(rgb, 0)                          # the size limit is tile_layout's (custom tiles allowed)
    H, W = rgb.shape[:2]
    R, G, B = (np.ascontiguousarray(rgb[..., c], _F32) for c in range(3))
    sums = np.zeros(nbins, _F64)
    counts = np.zeros(nbins, np.int64)
    for d in (tiles if tiles is not None else tile_layout(W, H)):
        if _cancelled(cancel):
            return None
        x, y, w, h = d[:4]
        tR, tG, tB = R[y:y + h, x:x + w], G[y:y + h, x:x + w], B[y:y + h, x:x + w]
        weight = clip_weight(tR, tG, tB, is_clip=is_clip)
        bright, noise, weight = pixnoise(_npx.luma_601(tR, tG, tB), weight)
        accumulate(bright, noise, weight, d, sums, counts, nbins)
    if _cancelled(cancel):
        return None
    return sums, counts


# ----------------------------------------------------------------------------------------
# 高速経路 (2026-10-16): 同じ大きさのタイルをまとめて処理する。演算と加算順は参照実装と同じ
# ----------------------------------------------------------------------------------------
def _min_peak(n, thr):
    """Smallest peak p with (float)p / (float)n > thr (n + 1 if none): below it IsClip is <= thr."""
    for p in range(n + 1):
        if _F32(_F32(p) / _F32(n)) > _F32(thr):
            return p
    return n + 1


def is_clip_blocks(V, simd=True, thr=None):
    """is_clip_genuine を V (N, n) の各行 (1 面の画素を行優先に並べたもの) に一括で適用する
    (値はビット一致)。最小・最大は FUN_009efdd4 と同じ規則 (先頭 n & ~7 要素は NaN が伝播、
    端数部は NaN を無視、開始値 ±FLT_MAX)、ビン番号は frintn と fcvtzs の飽和、最頻ビンは
    各行を並べ替えて連長の最大 (同数なら値の小さい方 = argmax の先頭) で求める。
    thr を与えると判定 (値 > thr) だけを保証する: 最頻ビンが (float)p/(float)n > thr となる p 個に
    届かない行は値を求めず 0.0 にする (本当の値も thr 以下なので判定は同じ)。"""
    V = np.ascontiguousarray(V, _F32)
    N, n = V.shape
    out = np.ones(N, _F32)
    if N == 0:
        return out
    m = (n & ~7) if simd else 0
    mn = np.full(N, _FLT_MAX, _F32)
    mx = np.full(N, -_FLT_MAX, _F32)
    if m:
        bmn, bmx = V[:, :m].min(1), V[:, :m].max(1)
        nanb = np.isnan(bmn) | np.isnan(bmx)
        mn = np.where(bmn < mn, bmn, mn)             # Python min(FLT_MAX, bmn) of the reference
        mx = np.where(bmx > mx, bmx, mx)
        mn[nanb] = np.nan
        mx[nanb] = np.nan
    if m < n:                                        # scalar tail: NaN elements skipped
        tail = V[:, m:]
        tn = np.isnan(tail)
        has = ~tn.all(1)
        tmax = np.where(tn, -np.inf, tail).max(1).astype(_F32)
        tmin = np.where(tn, np.inf, tail).min(1).astype(_F32)
        mx = np.where(has & ~np.isnan(mx) & (tmax > mx), tmax, mx)
        mn = np.where(has & ~np.isnan(mn) & (tmin < mn), tmin, mn)
    with np.errstate(over="ignore", invalid="ignore"):
        rng = (mx - mn).astype(_F32)
        nbf = (rng * _F32(8.0)).astype(_F32) + _F32(1.0)
    nb = _fcvtzs(nbf)
    act = np.nonzero(nb >= 2)[0]
    if act.size == 0:
        return out
    nba = np.minimum(nb[act], 0x4000)
    with np.errstate(over="ignore", invalid="ignore", divide="ignore"):
        k = (nba - 1).astype(_F32) / rng[act]
        r = np.rint(k[:, None] * (V[act] - mn[act, None]))
    cap = (nba - 1).astype(np.int16)[:, None]
    if np.isfinite(r).all():
        # finite: 0 <= r <= nb (every element lies in [mn, mx]), so the int16 cast is the fcvtzs value
        idx = r.astype(np.int16)
        np.minimum(idx, cap, out=idx)
    else:                                            # NaN / inf bins: saturating fcvtzs, then clamp
        idx = np.clip(_fcvtzs(r), 0, cap).astype(np.int16)
    s = np.sort(idx, axis=1)
    if thr is not None:
        p = _min_peak(n, thr)
        if p > n:
            return np.where(out > _F32(thr), out, _F32(0.0)).astype(_F32)
        cand = (s[:, p - 1:] == s[:, :n - p + 1]).any(1) if p > 1 else np.ones(act.size, bool)
        out[act[~cand]] = _F32(0.0)
        act, s = act[cand], s[cand]
        if act.size == 0:
            return out
    new = np.empty(s.shape, bool)
    new[:, 0] = True
    np.not_equal(s[:, 1:], s[:, :-1], out=new[:, 1:])
    pos = np.arange(n, dtype=np.int32)
    start = np.maximum.accumulate(np.where(new, pos, 0), axis=1)
    run = pos - start + 1                            # length of the run up to each position
    am = run.argmax(1)                               # end of the first longest run = lowest mode
    rows = np.arange(act.size)
    peak = run[rows, am].astype(np.int64)
    below = start[rows, am].astype(np.int64)
    above = n - below - peak
    keep = (n < 2 * peak) | (below * 10 < above) | (above * 10 < below)
    out[act] = np.where(keep, peak.astype(_F32) / _F32(n), _F32(0.0))
    return out


def _block_origins(size, block):
    return [i * block if block <= size - i * block else size - block for i in range((size + block - 1) // block)]


def clip_weight_batch(R, B, block=BLOCK, thr=CLIP_THR, G=None):
    """clip_weight (is_clip_genuine) of T tiles of one size at once: R, B (T, h, w) -> (T, h, w)
    weights, identical to [clip_weight(R[t], G[t], B[t]) for t].  G (T, h, w) is optional and never
    read: FUN_009b76b0 tests the R and B blocks only (it is only passed on to clip_weight)."""
    R = np.asarray(R, _F32)
    B = np.asarray(B, _F32)
    T, h, w = R.shape
    if h < block or w < block:                       # tiles under 16 px (custom tile lists only): per tile
        return np.stack([clip_weight(R[t], None if G is None else G[t], B[t], block, thr) for t in range(T)]) \
            if T else np.ones((0, h, w), _F32)
    rows, cols = _block_origins(h, block), _block_origins(w, block)
    sw = np.lib.stride_tricks.sliding_window_view
    ri, ci = np.array(rows)[:, None], np.array(cols)[None, :]
    zero = None
    for P in (R, B):
        blk = sw(P, (block, block), axis=(1, 2))[:, ri, ci]               # (T, nr, nc, block, block)
        z = is_clip_blocks(blk.reshape(-1, block * block), thr=thr) > thr   # NaN never zeroes
        zero = z if zero is None else (zero | z)
    zero = zero.reshape(T, len(rows), len(cols))
    weight = np.ones((T, h, w), _F32)
    for i, by in enumerate(rows):
        for j, bx in enumerate(cols):
            zt = zero[:, i, j]
            if zt.any():
                weight[zt, by:by + block, bx:bx + block] = 0
    return weight


MAX_BATCH = 96    # tiles per batch (1080p: the 180 tiles of 120x120 in two batches of 90; 0.70 s and a
#                   traced peak of 179 MB, against 0.695 s / 302 MB in one batch of 180, 2026-10-16)


def _check_rgb(rgb, min_size=MIN_TILE):
    """The input contract of genuine_noise_curve: a channel-last (H, W, C >= 3) array (R, G, B are channels
    0..2, further channels are ignored) with H, W >= min_size (the genuine asserts below 48 px)."""
    rgb = np.asarray(rgb)
    # a channel-first (C, H, W) frame passes the shape test with H = C: name the likely cause
    hint = (" (channel-first? pass np.moveaxis(rgb, 0, -1))"
            if rgb.ndim == 3 and rgb.shape[0] <= 4 < rgb.shape[2] else "")
    if rgb.ndim != 3 or rgb.shape[2] < 3:
        raise ValueError(f"rgb must be a channel-last (H, W, C >= 3) array (R, G, B in channels 0..2), "
                         f"got shape {rgb.shape}")
    H, W = rgb.shape[:2]
    if W < min_size or H < min_size:
        raise ValueError(f"image {W}x{H} (shape {rgb.shape}) is smaller than {min_size} px: the genuine scheduler "
                         f"asserts{hint}")
    return rgb


def _add_kept(sums, counts, bins, noise, nbins):
    """sums[bin] += (double)noise and counts[bin] += 1 for the kept pixels of one tile, in their order
    (np.add.at adds sequentially in index order, as accumulate)."""
    if bins.size:
        np.add.at(sums, bins, noise.astype(_F64))
        counts += np.bincount(bins, minlength=nbins)[:nbins]


def _tile_batches(tiles, max_batch=MAX_BATCH):
    """Batches of tile indices of one size (w, h), at most max_batch each, in tile order."""
    groups = {}
    for ti, d in enumerate(tiles):
        groups.setdefault((d[2], d[3]), []).append(ti)
    out = []
    for idx in groups.values():
        nb = max(1, -(-len(idx) // max_batch))
        step = -(-len(idx) // nb)
        out += [idx[i:i + step] for i in range(0, len(idx), step)]
    return out


def tile_histogram_batch(rgb, pixnoise=None, nbins=NBINS, is_clip=None, tiles=None, max_batch=MAX_BATCH,
                         cancel=None):
    """tile_histogram の高速版 (結果はビット一致)。同じ大きさのタイルを max_batch 枚までまとめて
    クリップ判定 (clip_weight_batch)・輝度 (luma_601_batch)・画素ノイズ (nv_pixnoise.pixnoise_batch)
    に通し、加算は本家と同じ順 (タイルはスケジューラの順、タイル内は行優先、double) にタイルごとの
    np.add.at で行う (ufunc.at は添字の順に逐次加算するので accumulate と同じ加算列)。バッチは大きさ別
    なのでタイル順とは限らない (先頭タイルの順に処理する): 前のタイルがまだのタイルは採用画素
    (ビン番号とノイズ float32) だけを順番が来るまで保持し、加えたら捨てる (1 フレーム分の採用画素を連結しない)。
    保持するビン番号は nbins - 1 が収まる最小の符号なし整数型 (8 ビンなら uint8)。入力の面は切り出しごとに
    float32 にする (面全体の複製を作らない。値は全体を変換した場合と同じ)。
    pixnoise / is_clip を渡すとその部分だけタイルごとに呼ぶ (照合用)。
    cancel は tile_histogram と同じ (問い合わせはバッチの前にそのタイル数だけ、最後に 1 回。回数は本家と同じ)。"""
    rgb = _check_rgb(rgb, 0)                          # the size limit is tile_layout's (custom tiles allowed)
    H, W = rgb.shape[:2]
    planes = [rgb[..., c] for c in range(3)]          # views; each batch copies its crops
    tiles = tiles if tiles is not None else tile_layout(W, H)
    sums = np.zeros(nbins, _F64)
    counts = np.zeros(nbins, np.int64)
    pending = {}      # tile -> (bins, noise) of its kept core pixels while an earlier tile is still missing
    nxt = 0           # the next tile in scheduler order to be added
    bin_t = np.min_scalar_type(max(int(nbins) - 1, 0))     # compact waiting bins (uint8 up to 256 bins)
    sw = np.lib.stride_tricks.sliding_window_view
    # batches in the order of their first tile: the waiting tiles stay within about one batch even when a
    # size first appears late in a row (4096x2160: at most 75 waiting tiles instead of 840 in group order)
    for idx in sorted(_tile_batches(tiles, max_batch), key=min):
        if _cancelled(cancel, len(idx)):
            return None
        w, h = tiles[idx[0]][2], tiles[idx[0]][3]
        ys = np.array([tiles[t][1] for t in idx])
        xs = np.array([tiles[t][0] for t in idx])
        cR, cG, cB = (np.asarray(sw(P, (h, w))[ys, xs], _F32) for P in planes)     # (T, h, w) copies
        with np.errstate(all="ignore"):
            if is_clip is None:
                cw = clip_weight_batch(cR, cB, G=cG)
            else:
                cw = np.stack([clip_weight(cR[t], cG[t], cB[t], is_clip=is_clip) for t in range(len(idx))])
            luma = _npx.luma_601_batch(cR, cG, cB)
            if pixnoise is None:
                noise, weight, bright = _npx.pixnoise_batch(luma, cw)
            else:
                res = [pixnoise(luma[t], cw[t]) for t in range(len(idx))]
                bright, noise, weight = (np.stack([np.asarray(r[j], _F32) for r in res]) for j in range(3))
            for t, ti in enumerate(idx):
                d = tiles[ti]
                core = (t, slice(d[9], d[9] + d[7]), slice(d[8], d[8] + d[6]))
                keep = ~(weight[core] < _F32(0.5)).ravel()                 # NaN weights are counted
                kb = bin_index(bright[core].ravel()[keep], nbins)
                kn = noise[core].ravel()[keep]
                if ti != nxt:
                    pending[ti] = (kb.astype(bin_t), kn)
                    continue
                _add_kept(sums, counts, kb, kn, nbins)
                nxt += 1
                while nxt in pending:                                      # tiles of earlier batches
                    _add_kept(sums, counts, *pending.pop(nxt), nbins)
                    nxt += 1
    if pending or nxt != len(tiles):
        raise RuntimeError(f"tile list not covered in order ({nxt} of {len(tiles)} tiles added)")
    if _cancelled(cancel):
        return None
    return sums, counts


def genuine_noise_curve(rgb, pixnoise=None, return_hist=False, nbins=NBINS, is_clip=None, batch=True,
                        cancel=None):
    """rgb (H, W, 3) float32 0..255 → (xs, ys) (本家の `nvh curve` と同じ 8 点の表)。
    pixnoise(luma, weight) -> (bright, noise, weight) を省略すると nv_pixnoise の移植を使う。
    batch=True (既定) は高速経路 (tile_histogram_batch)、batch=False は参照実装 (tile_histogram:
    タイルごとに nv_pixnoise.pixnoise と nv_lines.linefilt)。(xs, ys, sums, counts) は両者でビット一致。
    入力はチャンネルが最後の (H, W, C >= 3) の配列 (チャンネル 0..2 が R, G, B、4 番目以降は読まない)、
    H, W >= 48 (本家は 48px 未満でアサーション)、nbins >= 1 (本家は 8)。満たさなければ ValueError。
    float32 以外は float32 に変換して使う。
    cancel: 取り消しの問い合わせ (tile_histogram)。取り消されたら None を返す (本家はこのとき偽を返し、
    FindFlatArea は状態 5 になる)。"""
    rgb = _check_rgb(rgb)
    if int(nbins) < 1:
        raise ValueError(f"nbins must be >= 1 (the genuine uses {NBINS}), got {nbins}")
    H, W = rgb.shape[:2]
    hist = (tile_histogram_batch if batch else tile_histogram)(rgb, pixnoise, nbins, is_clip, cancel=cancel)
    if hist is None:
        return None
    sums, counts = hist
    xs, ys = curve_table(sums, counts, W, H, nbins)
    if return_hist:
        return xs, ys, sums, counts
    return xs, ys


def noise_at(table):
    """(xs, ys) → 輝度 → ノイズの関数 (FindFlatArea が EstimateNoiseLevelCurve から取る形)。"""
    return CurveInterp(*table)
