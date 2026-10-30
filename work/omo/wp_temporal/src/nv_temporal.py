"""時間フィルタ (Neat Video の FilterTemporal) の移植。

構成:
  * ``Settings``            … 設定構造体 (param_1[0x7a0]) の写し
  * ``ramp_genuine`` 等     … 本家の葉 4 関数 (nvh ramp/wmul/pairw/down2 とビット一致させる対象)
  * ``band_weights``        … 0x99a76c (類似度) + 0x99afb4 (段の重み) の参照実装
  * ``temporal_combine``    … 9a84f8 の蓄積部 (Σw, Σw·x, 残留ノイズ) の参照実装

確かめた事実 (逆アセンブル + nvh による葉の照合、REPORT_20261018.md):
  * 葉 4 関数は命令列どおりに移植し、nvh ramp/wmul/pairw/down2 と uint32 ビット単位で一致
    (verify/verify_leaves.py、logs/verify_leaves_full.log。NaN のペイロードと sNaN の静化まで)
      ramp (FUN_009ee93c(a=t, b=2t, c=1, d=0)): b − a ≤ b·1e-5 (順序付きの比較で。順序なしでは hi が成立して補正しないが、slope が NaN になり出力は同じ) なら
            a ← a·0.99999 − 1e-5、b ← b·1.00001 + 1e-5 (t ≤ 0・±inf で効く。NaN では hi が成立して補正しない)。slope = (d − c)/(b − a)、
            ic = c − a·slope。先頭 n&~3 個は FMLA (1 回丸め) → FMIN.4s(·,1) → FMAX.4s(·,0) (NaN は伝播)、
            残りは fmul・fadd (非融合) → v > 1 ? 1 : v → v > 0 ? v : 0 (NaN は 0)
      wmul (FUN_009eebf8): ramp(r)·prev (NEON 部は fmul ramp·prev、端数部は fmul prev·ramp)
      pairw (FUN_009efb1c): c·(max(a, b) − b)。NEON 部の max は FMAX.4s (NaN 伝播)、端数部は a > b ? a : b
      down2 (FUN_009f15b4、1 回で 2 行 → 1 行): 先頭 W&~7 列は (a+b)+(c+d)、残りは ((a+b)+c)+d、×0.25
  * 段の閾値 t_k = C_k · 1.1 · s_k · 1.2806249 · 0.9 · (17 − popcount) · |σ_k| + T²
    - C_k = DAT_012445a8[k] = (1.06, 1.0, 1.0, 1.0)
    - σ_k = σ 構造体 (= インスタンス +0x17330、プロファイル複写 +0x114e8 の変換面 3 (モード 1 では Y)
      のモデル面ブロック) の語 5+k = 帯域の 99% 値 (統括の独立検証による訂正)
    - s_k は最粗段のみ ×1.3、(17 − popcount) は正常時 1 と仮定 (仮定。回避・追跡はしない)
  * X = 差分 / (σ² + 1e-4)   (1e-4 = 0x38d1b717)
  * R = sqrt(3x3 平均(X²))   (FUN_009950f4。呼び出し形が未確定のため未照合)
  * w = ramp(t, R)。粗い段の重みに FUN_009eebf8 (wmul) で乗算しながら細かい段へ降りる (k=3 → 0)
  * 帯域分割 FUN_00990da8 と拡大 FUN_00993df0 は base/nv_pixnoise の移植 (§6.5 で本家とビット一致) を使う。
    FUN_0099a76c 内から 0x99aa50 (9f15b4)・0x99aa9c (990da8)・0x99ad20/0x99adc8/0x99ae70 (993df0) で呼ばれる
    ことは全逆アセンブルの bl 参照で確認 (99a76c の中身は難読化を含むので読んでいない)

推定 (未照合。REPORT.md に列挙):
  * 段のサイズ (W>>k, H>>k) と奇数フラグ、段 3 を最粗の低域とすること、拡大に渡す奇数フラグ
  * 3x3 平均 (FUN_009950f4) は端 0 詰め・1/9 と仮定 (呼び出し形が未確定)
  * 出力の正規化 (9abfd4) は「中心 + Σw·(近傍 − 中心) / Σw」と仮定
  * σ² ピラミッド (9aae90) は外部から与える (本モジュールでは作らない)

本モジュールは numpy の float32 で扱う。照合は verify/verify_leaves.py (本家 nvh とのビット比較)。
"""

from __future__ import annotations

import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "base"))
import nv_pixnoise as _px  # noqa: E402  (base/ のコピー。repro/ は読み込まない)

# --- 本家から読み出した定数 (観測) ------------------------------------------
C_TABLE = (1.06, 1.0, 1.0, 1.0)          # DAT_012445a8 (先頭 4 語)
CONST_1_1 = 1.1                          # 0x3f8ccccd
CONST_SQRT_1_64 = 1.2806249              # 0x3fa3eb84
CONST_0_9 = 0.9                          # 0x3f666666
EPS_SIG2 = 1e-4                          # 0x38d1b717 (FUN_009ee04c)
CENTRE_NOISE_FACTOR = 0.64               # §6.4: 近傍の分散係数
COARSEST_BOOST = 1.3                     # 最粗段のみ s_k に掛かる
N_BANDS = 4                              # 99a76c のループ (k=3 → 0)
T2_MODE1 = 64.0                          # 設定 +0x168 == 1 のときの T²
THRESHOLD_SCALE = 0.16                   # T² = (Threshold * 0.16)²


# --- 本家の葉 4 関数 (nvh ramp/wmul/pairw/down2 とビット一致) ----------------
#
# 逆アセンブル (/tmp/nv_expand/disasm_full.txt) で読んだ命令列をそのまま numpy で再現する。
# NEON 判定のバイト 0x14a67a8 が非 0 (ハーネスで観測した経路) を neon=True とする。
# NaN の伝播は ARM の FPProcessNaNs の規則 (第 1 オペランドの sNaN → 第 2 の sNaN → 第 1 の qNaN →
# 第 2 の qNaN、結果は静化)、入力に NaN のない無効演算は既定の NaN 0x7fc00000 (FPCR.DN=0)。

_QBIT = np.uint32(0x00400000)
_DEFAULT_NAN = np.uint32(0x7FC00000)
_F32 = np.float32


def _bits(a):
    return np.asarray(a, dtype=np.float32).view(np.uint32)


def _isnan_bits(u):
    return (u & np.uint32(0x7FFFFFFF)) > np.uint32(0x7F800000)


def _issnan_bits(u):
    return _isnan_bits(u) & ((u & _QBIT) == 0)


def _nan_fix(res, ops):
    """res (f32 配列) の NaN を ARM の規則で選び直す。ops は優先順のオペランド列。"""
    res = np.array(res, dtype=np.float32, copy=True)
    shape = res.shape
    rb = res.view(np.uint32).reshape(-1)
    obs = [np.broadcast_to(_bits(np.asarray(o, dtype=np.float32)), shape).reshape(-1) for o in ops]
    is_nan_res = _isnan_bits(rb)
    chosen = np.zeros(rb.shape, dtype=bool)
    out = np.where(is_nan_res, _DEFAULT_NAN, rb).astype(np.uint32)
    for pred in (_issnan_bits, lambda u: _isnan_bits(u) & ~_issnan_bits(u)):
        for ob in obs:
            m = pred(ob) & ~chosen
            out = np.where(m, ob | _QBIT, out)
            chosen |= m
    return out.view(np.float32).reshape(shape)


def _add(a, b):
    with np.errstate(all="ignore"):
        r = np.asarray(_F32(a), dtype=np.float32) + np.asarray(_F32(b), dtype=np.float32)
    return _nan_fix(r, (a, b))


def _sub(a, b):
    with np.errstate(all="ignore"):
        r = np.asarray(a, dtype=np.float32) - np.asarray(b, dtype=np.float32)
    return _nan_fix(r, (a, b))


def _mul(a, b):
    with np.errstate(all="ignore"):
        r = np.asarray(a, dtype=np.float32) * np.asarray(b, dtype=np.float32)
    return _nan_fix(r, (a, b))


def _div(a, b):
    with np.errstate(all="ignore"):
        r = np.asarray(a, dtype=np.float32) / np.asarray(b, dtype=np.float32)
    return _nan_fix(r, (a, b))


def _fma(addend, op1, op2):
    """FMLA: addend + op1·op2 を 1 回だけ丸める (f64 で積は厳密、和は TwoSum + 奇数丸め)。

    NaN は FPProcessNaNs3(addend, op1, op2)。addend が qNaN で op1·op2 が ∞·0 なら既定の NaN。
    """
    c = np.asarray(addend, dtype=np.float32)
    x = np.asarray(op1, dtype=np.float32)
    y = np.asarray(op2, dtype=np.float32)
    c, x, y = np.broadcast_arrays(c, x, y)
    with np.errstate(all="ignore"):
        p = x.astype(np.float64) * y.astype(np.float64)      # 24×24 ビット: f64 で厳密
        c64 = c.astype(np.float64)
        s = p + c64
        bb = s - p
        err = (p - (s - bb)) + (c64 - bb)                      # TwoSum (s が有限のとき厳密)
        fin = np.isfinite(s) & np.isfinite(err)
        sb = s.view(np.uint64)
        even = (sb & np.uint64(1)) == 0
        adj = fin & (err != 0) & even
        toward = np.where(err > 0, np.inf, -np.inf)
        s = np.where(adj, np.nextafter(s, toward), s)          # 奇数丸め (53 ≥ 24+2 なので二重丸めなし)
        r = s.astype(np.float32)
    r = _nan_fix(r, (c, x, y))
    inv = (np.isinf(x) & (y == 0)) | ((x == 0) & np.isinf(y))
    qn_add = _isnan_bits(_bits(c)) & ~_issnan_bits(_bits(c))
    m = inv & qn_add & ~_issnan_bits(_bits(x)) & ~_issnan_bits(_bits(y))
    if np.any(m):
        rb = r.view(np.uint32).copy()
        rb[m] = _DEFAULT_NAN
        r = rb.view(np.float32)
    return r


def _vfmin(a, b):
    """FMIN.4s: NaN を伝播、−0 < +0。"""
    a = np.asarray(a, dtype=np.float32)
    b = np.broadcast_to(np.asarray(b, dtype=np.float32), a.shape)
    ab, bb = _bits(a), _bits(b)
    sa, sb = (ab >> 31) == 1, (bb >> 31) == 1
    r = np.where(a < b, a, b)
    r = np.where((a == b) & (a == 0), np.where(sa | sb, _F32(-0.0), _F32(0.0)), r)
    r = np.where(_isnan_bits(ab) | _isnan_bits(bb), _F32(np.nan), r).astype(np.float32)
    return _nan_fix(r, (a, b))


def _vfmax(a, b):
    """FMAX.4s: NaN を伝播、+0 > −0。"""
    a = np.asarray(a, dtype=np.float32)
    b = np.broadcast_to(np.asarray(b, dtype=np.float32), a.shape)
    ab, bb = _bits(a), _bits(b)
    sa, sb = (ab >> 31) == 1, (bb >> 31) == 1
    r = np.where(a > b, a, b)
    r = np.where((a == b) & (a == 0), np.where(sa & sb, _F32(-0.0), _F32(0.0)), r)
    r = np.where(_isnan_bits(ab) | _isnan_bits(bb), _F32(np.nan), r).astype(np.float32)
    return _nan_fix(r, (a, b))


def _gt_sel(v, k, if_gt, if_not):
    """fcmp v, k → fcsel (gt)。順序なし (NaN) は gt 不成立。"""
    return np.where(np.asarray(v) > np.asarray(k), if_gt, if_not).astype(np.float32)


def ramp_coeffs(a, b, c=1.0, d=0.0):
    """FUN_009ee93c / FUN_009eebf8 の先頭 (0x9ee93c〜0x9ee9a0) の係数計算。

    b − a ≤ b·1e-5 (順序付きの比較で。順序なしでは hi が成立して補正しないが、slope が NaN になり出力は同じ) なら a ← a·0.99999 − 1e-5、b ← b·1.00001 + 1e-5
    (定数 0x3727c5ac / 0x3f7fff58 / 0xb727c5ac / 0x3f800054)。slope = (d − c)/(b − a)、ic = c − a·slope。
    """
    a, b, c, d = _F32(a), _F32(b), _F32(c), _F32(d)
    e5 = np.uint32(0x3727C5AC).view(np.float32)
    m5 = np.uint32(0xB727C5AC).view(np.float32)
    lo_k = np.uint32(0x3F7FFF58).view(np.float32)
    hi_k = np.uint32(0x3F800054).view(np.float32)
    s4 = _sub(b, a)
    s6 = _mul(b, e5)
    s7 = _add(_mul(a, lo_k), m5)
    s5 = _add(_mul(b, hi_k), e5)
    keep = bool(s4 > s6)                                         # fcsel hi: 命令では順序なしで成立 (keep)。ここは偽になるが slope が NaN になるので出力は同じ (検証者が確認)
    b2 = b if keep else _F32(s5)
    a2 = a if keep else _F32(s7)
    slope = _F32(_div(_sub(d, c), _sub(b2, a2)))
    ic = _F32(_sub(c, _mul(a2, slope)))
    return slope, ic


def _ramp_core(x, a, b, c, d, neon):
    """ramp の本体。戻り値は f32 配列 (クランプ済み)。"""
    x = np.ascontiguousarray(x, dtype=np.float32).reshape(-1)
    slope, ic = ramp_coeffs(a, b, c, d)
    c, d = _F32(c), _F32(d)
    le = bool(d <= c)                                            # fcmp s3, s2; b.le
    lo, hi = (d, c) if le else (c, d)                            # fmin 側 = hi、fmax 側 = lo
    n = x.size
    n4 = (n & ~3) if neon else 0
    out = np.empty(n, dtype=np.float32)
    if n4:
        v = _fma(ic, x[:n4], slope)                              # fmla v17(=ic), v16(=x), v4(=slope)
        out[:n4] = _vfmax(_vfmin(v, hi), lo)
    if n4 < n:
        v = _add(ic, _mul(slope, x[n4:]))                        # fmul s0(slope)·x、fadd s1(ic)+
        v = _gt_sel(v, hi, hi, v)                                # v > hi ? hi : v
        out[n4:] = _gt_sel(v, lo, v, lo)                         # v > lo ? v : lo
    return out


def ramp_genuine(t, x, neon=True):
    """FUN_009ee93c(t, 2t, 1, 0, in, out, n) (nvh ramp と同じ引数。2t は f32 の積)。"""
    t = _F32(t)
    return _ramp_core(x, t, _F32(_mul(_F32(2.0), t)), 1.0, 0.0, neon).reshape(np.shape(x))


def wmul_genuine(t, r, prev, neon=True):
    """FUN_009eebf8(t, 2t, 1, 0, r, prev, out, n): ramp(r)·prev。

    NEON 部は fmul ramp·prev (第 1 オペランド = ramp)、端数部は fmul prev·ramp (第 1 = prev)。
    """
    t = _F32(t)
    r_in = r
    r = np.ascontiguousarray(r, dtype=np.float32).reshape(-1)
    p = np.ascontiguousarray(prev, dtype=np.float32).reshape(-1)
    w = _ramp_core(r, t, _F32(_mul(_F32(2.0), t)), 1.0, 0.0, neon)
    n4 = (r.size & ~3) if neon else 0
    out = np.empty(r.size, dtype=np.float32)
    out[:n4] = _mul(w[:n4], p[:n4])
    out[n4:] = _mul(p[n4:], w[n4:])
    return out.reshape(np.shape(r_in))


def pairw_genuine(a, b, c, neon=True):
    """FUN_009efb1c(a, b, c, out, n): c·(max(a, b) − b)。

    NEON 部 (先頭 n&~3): FMAX.4s (NaN 伝播) → fsub → fmul c·(…)。
    端数部: a > b ? a : b (順序なしは b) → fsub → fmul c·(…)。
    """
    shape = np.shape(a)
    a = np.ascontiguousarray(a, dtype=np.float32).reshape(-1)
    b = np.ascontiguousarray(b, dtype=np.float32).reshape(-1)
    c = np.ascontiguousarray(c, dtype=np.float32).reshape(-1)
    n = a.size
    n4 = (n & ~3) if neon else 0
    m = np.empty(n, dtype=np.float32)
    m[:n4] = _vfmax(a[:n4], b[:n4])
    m[n4:] = _gt_sel(a[n4:], b[n4:], a[n4:], b[n4:])
    return _mul(c, _sub(m, b)).reshape(shape)


def down2_row(top, bot, neon=True):
    """FUN_009f15b4(src, dst, W) の 1 回分: 上の行 top と下の行 bot (長さ W) → W//2 個。

    NEON 部 (先頭 W&~7 列): faddp で a+b・c+d を作り (a+b)+(c+d)、×0.25。
    残り (W&7 列。NEON なしなら全列): ((a+b)+c)+d、×0.25 (0x9f1708〜0x9f1720、ld2 経路も同じ順)。
    """
    top = np.ascontiguousarray(top, dtype=np.float32).reshape(-1)
    bot = np.ascontiguousarray(bot, dtype=np.float32).reshape(-1)
    W = top.size
    w8 = (W & ~7) if neon else 0
    q = _F32(0.25)
    out = []
    if w8:
        a, b_ = top[0:w8:2], top[1:w8:2]
        c, d = bot[0:w8:2], bot[1:w8:2]
        out.append(_mul(_add(_add(a, b_), _add(c, d)), q))
    nr = (W - w8) >> 1
    if nr:
        idx = w8 + 2 * np.arange(nr)
        a, b_, c, d = top[idx], top[idx + 1], bot[idx], bot[idx + 1]
        out.append(_mul(_add(_add(_add(a, b_), c), d), q))
    if not out:
        return np.empty(0, dtype=np.float32)
    return np.concatenate(out).astype(np.float32)


def down2_genuine(plane, neon=True):
    """nvh down2 と同じ行ループ: 出力行 y = down2_row(行 2y, 行 2y+1)。出力 (H//2, W//2)。"""
    plane = np.asarray(plane, dtype=np.float32)
    H, W = plane.shape
    h2, w2 = H // 2, W // 2
    out = np.empty((h2, w2), dtype=np.float32)
    for y in range(h2):
        out[y] = down2_row(plane[2 * y], plane[2 * y + 1], neon)
    return out


# --- 段のサイズと帯域分解 ----------------------------------------------------

def band_sizes(w, h):
    """段 k の (W_k, H_k, W_k&1, H_k&1) = (W>>k, H>>k, …)。

    FUN_0099c3a4 は難読化様のコードを含むので読まない・呼ばない。同じ関数の結果を
    base/nv_pixnoise.level_sizes が W>>k として使い、EstimateNoiseLevelCurve が本家とビット一致している
    (§6.5) ことからの間接的な裏付け。時間側 (FUN_0099a76c) が渡す引数は未確認 (推定)。
    """
    return [(int(w) >> k, int(h) >> k, (int(w) >> k) & 1, (int(h) >> k) & 1) for k in range(N_BANDS)]


def _up2(a, oddW, oddH):
    """FUN_00993df0 ((3,1)/4 の 2 倍拡大)。base/nv_pixnoise.up2 (本家とビット一致の移植) を使う。"""
    return _px.up2(a, oddW, oddH)


def band_pyramid(plane):
    """差分面 → 各段の面 (細→粗)。段 0..2 は FUN_00990da8 (src − up2(lo))、段 3 は最粗の低域。

    縮小は FUN_009f15b4 (down2_genuine、nvh down2 とビット一致)。段 3 を低域とするのは推定
    (FUN_0099a76c は難読化を含むので中を読んでいない)。
    """
    bands = []
    cur = np.ascontiguousarray(plane, dtype=np.float32)
    for k in range(N_BANDS - 1):
        H, W = cur.shape
        lo = down2_genuine(cur)
        bands.append(_px.band_from(lo, cur, W & 1, H & 1))
        cur = lo
    bands.append(cur)
    return bands


# --- 段の重み (0x99afb4) -----------------------------------------------------

def _box3_mean_sq(x):
    """3x3 箱平均 (端は 0 詰め) の平方根 = R (FUN_009950f4 を 1.0 で初期化。推定)。"""
    p = np.zeros((x.shape[0] + 2, x.shape[1] + 2), dtype=np.float32)
    p[1:-1, 1:-1] = x * x
    acc = np.zeros_like(x, dtype=np.float32)
    for dy in range(3):
        for dx in range(3):
            acc += p[dy:dy + x.shape[0], dx:dx + x.shape[1]]
    return np.sqrt(acc * np.float32(1.0 / 9.0)).astype(np.float32)


def band_t(settings, k, sigma_k):
    """t_k (99afb4 の命令列どおり)。

    sigma_k = σ 構造体 (+0x14+4k) の絶対値 = プロファイルのモデル面 (変換面 3) の
    語 5+k = 帯域 k の 99% 値 (統括の独立検証による訂定)。
    """
    s_k = settings.band_scale * (COARSEST_BOOST if k == N_BANDS - 1 else 1.0)
    t = np.float32(C_TABLE[k])
    t = np.float32(t * np.float32(CONST_1_1))
    t = np.float32(t * np.float32(s_k))
    t = np.float32(t * np.float32(CONST_SQRT_1_64))
    t = np.float32(t * np.float32(CONST_0_9))
    t = np.float32(t * np.float32(abs(float(sigma_k))))
    return t


def band_weight(diff_band, sig2_band, t, prev_w=None):
    """1 段分の重み。X = diff/(σ²+1e-4)、R = sqrt(3x3 mean X²)、w = ramp(t, R)。"""
    t = np.float32(t)
    x = (diff_band.astype(np.float32) /
         (sig2_band.astype(np.float32) + np.float32(EPS_SIG2))).astype(np.float32)
    r = _box3_mean_sq(x)
    w = ramp_genuine(t, r)
    if prev_w is not None:
        w = wmul_genuine(t, r, prev_w)
    return w.astype(np.float32)


# --- 蓄積と出力 (9a84f8 / 9abfd4) --------------------------------------------

class Settings:
    """設定構造体 param_1[0x7a0] の写し。σ_k (モデル語 5+k) は別途引数で渡す。"""

    def __init__(self, threshold=25.0, mode=0, similarity_mode=0,
                 band_scale=1.0, weight_scale=1.0):
        # +0x160 Threshold / +0x168 mode / +0x130 類似度モード / +0x0e0 段係数 s_k
        self.threshold = float(threshold)
        self.mode = int(mode)
        self.similarity_mode = int(similarity_mode)
        self.band_scale = float(band_scale)      # settings+0xe0 (s_k)
        self.weight_scale = float(weight_scale)  # settings+0x12c (1.0 以外なら重みに掛かる)

    def t2(self):
        """9a84f8 の観測どおりの T²。"""
        if self.mode == 1:
            return T2_MODE1
        return (self.threshold * THRESHOLD_SCALE) ** 2


def band_weights(diff, sig2_pyramid, settings, sigma_k):
    """0x99a76c の移植 (参照形)。

    diff          : 差分面 (float32, 合成済み)
    sig2_pyramid  : 段ごとの σ² 面 (細→粗の順、要素数 N_BANDS)
    sigma_k       : σ 構造体の 4 値 (k=0..3) = プロファイルの帯域 99% 値
    戻り値         : 最細段の重みマップ (float32)
    """
    bands = band_pyramid(diff)                       # 細→粗
    t2 = np.float32(settings.t2())
    w = None
    for k in range(N_BANDS - 1, -1, -1):             # 粗 → 細 (99a76c の順)
        t = band_t(settings, k, sigma_k[k])
        if k == N_BANDS - 1:
            t = np.float32(t + t2)                   # 最粗段のみ T² を加算
        if w is not None:
            # FUN_00993df0 で前段の重みを細かい段のサイズへ拡大 (関数は xref で確認、引数は推定)
            H, W = bands[k].shape
            w = _up2(w, W & 1, H & 1)
        w = band_weight(bands[k], sig2_pyramid[k], t, w)
    return w


def temporal_combine(center, neighbours, weights, f=CENTRE_NOISE_FACTOR):
    """Σw, Σw·x, Σw² の蓄積と出力。

    center      : 中心フレームの面 (float32)
    neighbours  : [(g, o, 面), ...] 近傍フレームと 1 フレーム分の (ゲイン, オフセット)
    weights     : 近傍ごとの重みマップのリスト
    戻り値       : (出力面, 残留ノイズ面 Σf·w²/(Σw)²)
    """
    acc_w = np.zeros_like(center, dtype=np.float32)
    acc_x = np.zeros_like(center, dtype=np.float32)
    acc_w2 = np.zeros_like(center, dtype=np.float32)
    for (gain, offset, plane), w in zip(neighbours, weights):
        p = (plane.astype(np.float32) * np.float32(gain) + np.float32(offset)).astype(np.float32)
        acc_w = (acc_w + w).astype(np.float32)
        acc_x = (acc_x + (w * p).astype(np.float32)).astype(np.float32)
        acc_w2 = (acc_w2 + (w * w).astype(np.float32)).astype(np.float32)
    out = np.where(acc_w > 0, (acc_x / np.maximum(acc_w, np.float32(1e-30))).astype(np.float32),
                   center.astype(np.float32)).astype(np.float32)
    denom = (acc_w + np.float32(1.0)).astype(np.float32)
    res = (np.float32(f) * acc_w2 / (denom * denom)).astype(np.float32) + np.float32(f) / (denom * denom)
    return out, res


def similarity_weights(center, neighbours, sig2_pyramid, settings, sigma_k):
    """近傍ごとの重みをまとめて作る (差分の合成は呼び出し側のモード表に従う)。"""
    weights = []
    for gain, offset, plane in neighbours:
        d = ((plane.astype(np.float32) * np.float32(gain) + np.float32(offset)) -
             center.astype(np.float32)).astype(np.float32)
        weights.append(band_weights(d, sig2_pyramid, settings, sigma_k))
    return weights
