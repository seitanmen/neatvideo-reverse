# nv_spatial.py — 本家空間フィルタ (Filter 0x9ac2b0 CPU 経路) の移植 (wp_spatial)
#
# 2026-10-21 (spatial-align、notes/align_plan.md): 本家の入力 (プロファイル・設定) から本家と同じ式で計算する部品と
# 1 面の駆動部 FUN_0098edb0 (高速経路と、組 1・2 の方向経路 0x991aac。組 3・4 と一部の任意機能は NotImplementedError) を追加。
#  - 分解:   blur = FUN_009ed014 の 4 列帯 (端は複製)、L_{k+1} = FUN_009f15b4、B_k = FUN_00990da8 (repro/nv_pixnoise.band_from)
#  - 白色化: inv = FUN_009ed5f8(1e-4, σ_k) (FRECPE+FRECPS / 端数 fdiv)、W = FUN_009edc38(B_k, inv)
#  - 縮小:   R = FUN_009ed164 の 4 列帯 (0 詰め、√)、t = (cfg[k] + 1e-6)·(|model[5+k]|·1.1)、X = FUN_009eebf8(t, 2t, 1, 0, R, W)、
#            dst = FUN_009ee304(−amount_k, X) (または適応 FUN_009ee7bc)
#  - 再構成: ACC = FUN_009edeb8(1, dst, σ_k, ACC)、k > 0 で ACC = FUN_00993df0(ACC)、OUT = FUN_009ed844(入力, ACC)
#  - 段の切り出し FUN_0099c694、cfg FUN_0099ead0、σ マップ FUN_009aae90 + 輝度別テーブル FUN_0099e6f4/FUN_0099e264、逆色変換
# v5 互換の入口 spatial_genuine (v5 の σ・Amount で動かす実験用) も同じ部品を使う。
# 単位は 0..255 (本家の内部スケール)。
import numpy as np

EPS_WHITEN = 9.999999747378752e-05  # 0x38d1b717 (float32). Was 9.4433e-4 (wrong) in the measured runs


# --- 本家の葉 (FUN_009ee93c / FUN_009eebf8 / FUN_009f15b4) -------------------------------
# sandbox/omo/wp_temporal/src/nv_temporal.py (nvh ramp/wmul/down2 とビット一致済み) からのコピー。
# NaN の選び方は ARM の FPProcessNaNs (第 1 sNaN → 第 2 sNaN → 第 1 qNaN → 第 2 qNaN、静化)。
# 変更点は _nan_fix の先頭の早期 return だけ (結果に NaN がなければ元の処理も res をそのまま返す)。

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
    if not np.isnan(res).any():
        return res
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




def _f32(a):
    return np.ascontiguousarray(a, np.float32)


# --- 2026-10-21 (spatial-align、notes/align_plan.md): 本家の葉と FUN_0098edb0 の駆動部 ------------------------------
# 葉の式は逆アセンブル (tmp/disasm_full.txt) から。NEON 判定のバイト 0x14a67a8 は 1 (ハーネスと同じ) として、要素ごとの関数は
# 先頭 n & ~3 個が NEON の式、残りがスカラーの式 (どの関数も、NEON 経路の後の端数は 4 個未満なので第 2 のベクタ経路には入らない)。
# n は「1 回の呼び出しの要素数」で、切り出しのある段では行の幅になる (呼び出しごとに境界が変わる)。

import os as _os
import sys as _sys

_BASE = _os.path.join(_os.path.dirname(_os.path.abspath(__file__)), "..", "base")
if _BASE not in _sys.path:
    _sys.path.insert(0, _BASE)
import nv_pixnoise as _npx  # noqa: E402  (repro/nv_pixnoise.py のコピー: up2・band_from・FRECPE/FRECPS)

C_NINTH = np.uint32(0x3DE38E39).view(np.float32)   # FUN_009ed014 / FUN_009ed164 の 1/9
EPS_WHITEN_F = np.uint32(0x38D1B717).view(np.float32)
C_1E6 = np.uint32(0x358637BD).view(np.float32)       # 0x9915a8: 1e-6
C_1P1 = np.uint32(0x3F8CCCCD).view(np.float32)       # 0x99167c: 1.1
C_255INV = np.uint32(0x3B808081).view(np.float32)    # 0x9ab860: 1/255
NEG_1E6 = np.float32(-1e6)


def _sqrt(x):
    """fsqrt (負は既定の NaN、NaN は静化して伝播)。"""
    x = np.asarray(x, np.float32)
    with np.errstate(invalid="ignore"):
        r = np.sqrt(x).astype(np.float32)
    return _nan_fix(r, (x,))


def _fabs(x):
    return (_bits(x) & np.uint32(0x7FFFFFFF)).view(np.float32)


def _split(n, neon=True):
    return (n & ~3) if neon else 0


def _frecpe(x):
    return _npx.frecpe32_fast(np.ascontiguousarray(x, np.float32))


def _frecps(a, b):
    return _npx._frecps32_fast(np.asarray(a, np.float32), np.asarray(b, np.float32))


def leaf_recip(eps, x, neon=True):
    """FUN_009ed5f8(eps, x, out, n): NEON d = eps + x、e = FRECPE(d)、out = e·FRECPS(d, e)。端数 1/(x + eps) (fdiv)。"""
    x = _f32(x).reshape(-1)
    eps = _F32(eps)
    n4 = _split(x.size, neon)
    out = np.empty_like(x)
    if n4:
        d = _add(eps, x[:n4])
        e = _frecpe(d)
        out[:n4] = _mul(e, _frecps(d, e))
    if n4 < x.size:
        out[n4:] = _div(_F32(1.0), _add(x[n4:], eps))
    return out


def leaf_mul(a, b, neon=True):
    """FUN_009edc38(a, b, out, n): a·b (NEON・端数とも第 1 オペランド a)。"""
    return _mul(_f32(a).reshape(-1), _f32(b).reshape(-1))


def leaf_add(a, b, neon=True):
    """FUN_009ed844: a + b。"""
    return _add(_f32(a).reshape(-1), _f32(b).reshape(-1))


def leaf_sub(a, b, neon=True):
    """FUN_009ed710: a − b。"""
    return _sub(_f32(a).reshape(-1), _f32(b).reshape(-1))


def leaf_scale(s, x, neon=True):
    """FUN_009ee304(s, x, out, n): NEON s·x、端数 x·s。"""
    x = _f32(x).reshape(-1)
    s = _F32(s)
    n4 = _split(x.size, neon)
    out = np.empty_like(x)
    out[:n4] = _mul(s, x[:n4])
    out[n4:] = _mul(x[n4:], s)
    return out


def leaf_affine(a, b, x, neon=True):
    """FUN_009ee3fc(a, b, x, out, n): NEON FMLA(b, x, a)、端数 (x·a) + b。"""
    x = _f32(x).reshape(-1)
    a, b = _F32(a), _F32(b)
    n4 = _split(x.size, neon)
    out = np.empty_like(x)
    out[:n4] = _fma(b, x[:n4], a)
    out[n4:] = _add(_mul(x[n4:], a), b)
    return out


def leaf_mulk(k, a, b, neon=True):
    """FUN_009edd6c(k, a, b, out, n): NEON k·(a·b)、端数 (a·b)·k。"""
    a, b = _f32(a).reshape(-1), _f32(b).reshape(-1)
    k = _F32(k)
    n4 = _split(a.size, neon)
    out = np.empty_like(a)
    out[:n4] = _mul(k, _mul(a[:n4], b[:n4]))
    out[n4:] = _mul(_mul(a[n4:], b[n4:]), k)
    return out


def leaf_acc(s, a, b, c, neon=True):
    """FUN_009edeb8(s, a, b, c, out, n): NEON FMLA(c, s, a·b)、端数 c + ((a·b)·s)。"""
    a, b, c = _f32(a).reshape(-1), _f32(b).reshape(-1), _f32(c).reshape(-1)
    s = _F32(s)
    n4 = _split(a.size, neon)
    out = np.empty_like(a)
    if n4:
        out[:n4] = _fma(c[:n4], s, _mul(a[:n4], b[:n4]))
    out[n4:] = _add(c[n4:], _mul(_mul(a[n4:], b[n4:]), s))
    return out


def leaf_blend(a, b, x, y, neon=True):
    """FUN_009ee1a4(a, b, x, y, out, n): NEON FMLA(a·x, b, y)、端数 (x·a) + (y·b)。"""
    x, y = _f32(x).reshape(-1), _f32(y).reshape(-1)
    a, b = _F32(a), _F32(b)
    n4 = _split(x.size, neon)
    out = np.empty_like(x)
    if n4:
        out[:n4] = _fma(_mul(a, x[:n4]), b, y[:n4])
    out[n4:] = _add(_mul(x[n4:], a), _mul(y[n4:], b))
    return out


def leaf_clampmul(a, b, lo, hi, x, y, neon=True):
    """FUN_009ee7bc(a, b, lo, hi, x, y, out, n): NEON FMIN(FMLA(b, x, a), hi) → FMAX(·, lo) → ·y、
    端数 v = x·a + b (2 回丸め)、v > hi ? hi : v、v > lo ? v : lo、y·v。"""
    x, y = _f32(x).reshape(-1), _f32(y).reshape(-1)
    a, b, lo, hi = _F32(a), _F32(b), _F32(lo), _F32(hi)
    n4 = _split(x.size, neon)
    out = np.empty_like(x)
    if n4:
        v = _vfmax(_vfmin(_fma(b, x[:n4], a), hi), lo)
        out[:n4] = _mul(v, y[:n4])
    if n4 < x.size:
        v = _add(_mul(x[n4:], a), b)
        v = _gt_sel(v, hi, hi, v)
        v = _gt_sel(v, lo, v, lo)
        out[n4:] = _mul(y[n4:], v)
    return out


def leaf_u8(x, neon=True):
    """FUN_009f02b4(x, out, n): 先頭 n & ~15 個は FCVTNU → UQXTN → UQXTN、残りは frintn → fcvtzs → [0, 255]。
    どちらも「最近接偶数丸め、NaN と負は 0、255 で飽和」で同じ値になる。"""
    x = _f32(x).reshape(-1)
    with np.errstate(invalid="ignore"):
        r = np.rint(x.astype(np.float64))
        r = np.where(np.isnan(r), 0.0, np.clip(r, 0.0, 255.0))
    return r.astype(np.uint8)


def leaf_rstep(s, x0, x1, neon=True):
    """FUN_009ef260(s, x0, x1, out, n): s' = 1/s。NEON v = s'·|x1 − x0|、v > 1 なら x0·(FRECPE(v²)·FRECPS(v², e)) そうでなければ x0。
    端数 v = s'·(x1 − x0)、|v| > 1 でなければ (NaN を含む) x0、そうでなければ x0 / (v·v)。"""
    x0, x1 = _f32(x0).reshape(-1), _f32(x1).reshape(-1)
    si = _div(_F32(1.0), _F32(s))
    n4 = _split(x0.size, neon)
    out = np.empty_like(x0)
    if n4:
        d = _fabs(_sub(x1[:n4], x0[:n4]))                    # FABD: |x1 − x0| (NaN は fsub と同じ選び方)
        v = _mul(si, d)
        m = _fabs(v) > 1.0
        q = _mul(v, v)
        e = _frecpe(q)
        r = _mul(e, _frecps(q, e))
        out[:n4] = _mul(x0[:n4], np.where(m, r, _F32(1.0)).astype(np.float32))
    if n4 < x0.size:
        v = _mul(si, _sub(x1[n4:], x0[n4:]))
        keep = ~(_fabs(v) > 1.0)          # fcmp; b.le は順序なし (NaN) でも成立 → x0 をそのまま (sNaN も静化しない)
        out[n4:] = np.where(keep, x0[n4:], _div(x0[n4:], _mul(v, v))).astype(np.float32)
    return out


def leaf_rinv(s, x0, x1, neon=True):
    """FUN_009eef5c(s, x0, x1, out, n): s' = 1/s、v = |s'·x1|。NEON v > 1 なら x0·(FRECPE(v)·FRECPS(v, e)) そうでなければ x0。
    端数 v > 1 なら x0 / v、そうでなければ x0。"""
    x0, x1 = _f32(x0).reshape(-1), _f32(x1).reshape(-1)
    si = _div(_F32(1.0), _F32(s))
    n4 = _split(x0.size, neon)
    out = np.empty_like(x0)
    if n4:
        v = _fabs(_mul(si, x1[:n4]))
        e = _frecpe(v)
        r = _mul(e, _frecps(v, e))
        out[:n4] = _mul(x0[:n4], np.where(v > 1.0, r, _F32(1.0)).astype(np.float32))
    if n4 < x0.size:
        v = _fabs(_mul(si, x1[n4:]))
        out[n4:] = np.where(v > 1.0, _div(x0[n4:], v), x0[n4:]).astype(np.float32)
    return out


def leaf_sqr(x, neon=True):
    """FUN_009ed2f0: x·x。"""
    x = _f32(x).reshape(-1)
    return _mul(x, x)


def leaf_sqrtabs(x, neon=True):
    """FUN_009ed3e0: fsqrt(fabs(x))。"""
    return _sqrt(_fabs(_f32(x).reshape(-1)))


def leaf_sqrtk(k, x, neon=True):
    """FUN_009ed4e0(k, x, out, n): NEON k·√|x|、端数 √|x|·k (NaN の選び方だけが違う)。"""
    x = _f32(x).reshape(-1)
    k = _F32(k)
    r = _sqrt(_fabs(x))
    n4 = _split(x.size, neon)
    out = np.empty_like(x)
    out[:n4] = _mul(k, r[:n4])
    out[n4:] = _mul(r[n4:], k)
    return out


def strip_box3(x, mode=0):
    """FUN_009ed014 を 4 列帯で回したもの (0x98f378〜0x98f3b4。W ≥ 4、H ≥ 2)。帯の重なりは同じ値を書き直すだけなので、
    面全体で同じ式になる: 横 h = (x + 左) + 右、縦 上端 (h0 + m·h0) + h1、中 (h_r + h_{r−1}) + h_{r+1}、下端 m·h_l + (h_l + h_{l−1})
    (m = 1: mode 0 (端は複製)、m = 0: mode 1 (0 詰め。0 は __TEXT 0x12504a0 の 1 語))、最後に × 1/9 (0x3de38e39)。"""
    x = _f32(x)
    H, W = x.shape
    if W < 4 or H < 2:                       # 呼び出し元の分離型の経路: FUN_0099ba54 (行) → FUN_0099bf40 (列)、半径 1
        _, nl = _models_mod()
        return nl.box_cols(nl.box_rows(x, 1, mode), 1, mode)
    z = np.zeros((H, 1), np.float32)
    if mode == 0:
        L = np.concatenate([x[:, :1], x[:, :-1]], 1)
        R = np.concatenate([x[:, 1:], x[:, -1:]], 1)
    else:
        L = np.concatenate([z, x[:, :-1]], 1)
        R = np.concatenate([x[:, 1:], z], 1)
    h = _add(_add(x, L), R)
    out = np.empty_like(h)
    e0 = h[0] if mode == 0 else np.zeros(W, np.float32)
    el = h[H - 1] if mode == 0 else np.zeros(W, np.float32)
    out[0] = _add(_add(h[0], e0), h[1])
    if H > 2:
        out[1:H - 1] = _add(_add(h[1:H - 1], h[0:H - 2]), h[2:H])
    out[H - 1] = _add(el, _add(h[H - 1], h[H - 2]))
    return _mul(out, C_NINTH)


def strip_rms3(x, mode=1, s=1.0):
    """FUN_009ed164(s, …) を 4 列帯で回したもの (FUN_009950f4 の NEON 経路、0x995174〜)。k = (s·(1/9))·s、
    横 h = (x² + 左²) + 右²、縦は strip_box3 と同じ順、上端は (h0 + m·h0) + h1 に k (sum·k)、中・下端は k·sum、fsqrt。
    FUN_00990f74 は mode 1 (0 詰め)・s = 1 で呼ぶ (0x99157c〜0x9915a4)。"""
    x = _f32(x)
    H, W = x.shape
    if W < 4 or H < 2:                       # FUN_009950f4 の端数経路: x² (FUN_009ed2f0) → 行 → 列 (半径 1) → FUN_009ed4e0(s)
        _, nl = _models_mod()
        q = nl.box_cols(nl.box_rows(leaf_sqr(x).reshape(H, W), 1, mode), 1, mode)
        return leaf_sqrtk(_F32(s), q.reshape(-1)).reshape(H, W)
    s = _F32(s)
    k = _mul(_mul(s, C_NINTH), s)
    z = np.zeros((H, 1), np.float32)
    if mode == 0:
        L = np.concatenate([x[:, :1], x[:, :-1]], 1)
        R = np.concatenate([x[:, 1:], x[:, -1:]], 1)
    else:
        L = np.concatenate([z, x[:, :-1]], 1)
        R = np.concatenate([x[:, 1:], z], 1)
    h = _add(_add(_mul(x, x), _mul(L, L)), _mul(R, R))
    out = np.empty_like(h)
    e0 = h[0] if mode == 0 else np.zeros(W, np.float32)
    el = h[H - 1] if mode == 0 else np.zeros(W, np.float32)
    out[0] = _mul(_add(_add(h[0], e0), h[1]), k)
    if H > 2:
        out[1:H - 1] = _mul(k, _add(_add(h[1:H - 1], h[0:H - 2]), h[2:H]))
    out[H - 1] = _mul(k, _add(el, _add(h[H - 1], h[H - 2])))
    return _sqrt(out)


def halve(a):
    """FUN_009f15b4 の行ループ (呼び出し元 0x98f2bc〜0x98f2dc・0x990d70〜0x990d90): 出力行 y = down2_row(行 2y, 行 2y+1)。
    先頭 W&~7 列は (a+b)+(c+d)、残りの列は ((a+b)+c)+d、×0.25。奇数の最終列・最終行は捨てる。
    H < 2 なら本家は 1 行も書かず (出力バッファの前の内容が残る)、ここでは None を返す。W < 2 なら 0 列の行を書く。"""
    a = np.ascontiguousarray(a, np.float32)
    h2, w2 = a.shape[0] >> 1, a.shape[1] >> 1
    if h2 == 0:
        return None
    if w2 == 0:
        return np.empty((h2, 0), np.float32)
    W = a.shape[1]
    w8 = W & ~7
    top, bot = a[0:2 * h2:2], a[1:2 * h2:2]
    q = np.float32(0.25)
    out = np.empty((h2, w2), np.float32)
    if w8:
        out[:, :w8 >> 1] = _mul(_add(_add(top[:, 0:w8:2], top[:, 1:w8:2]),
                                     _add(bot[:, 0:w8:2], bot[:, 1:w8:2])), q)
    if w2 > (w8 >> 1):
        e = 2 * w2
        out[:, w8 >> 1:] = _mul(_add(_add(_add(top[:, w8:e:2], top[:, w8 + 1:e:2]),
                                          bot[:, w8:e:2]), bot[:, w8 + 1:e:2]), q)
    return out


def level_sizes(W, H, levels):
    """FUN_0099c3a4: 段 k の (W >> k, H >> k) と奇数フラグ (repro/nv_pixnoise.level_sizes と同じ)。"""
    return _npx.level_sizes(W, H, levels + 1)


def up2(lo, oddW, oddH):
    """FUN_00993df0(lo, dst, Wc, Hc, oddW, oddH) (repro/nv_pixnoise.up2 のコピー経由。`nvh pixnoise` で照合済みの部品)。"""
    return _npx.up2(lo, oddW, oddH)


def band_from(lo, src, oddW, oddH):
    """FUN_00990da8(lo, src, dst, Wc, Hc, oddW, oddH) (repro/nv_pixnoise.band_from)。"""
    return _npx.band_from(lo, src, oddW, oddH)


# --- 段の切り出し (FUN_0099c694) -------------------------------------------------------------------------------------

SET_MARGIN = (2, 4, 4, 8, 32)          # DAT_012438d0[組] (__TEXT、静的に読み出し)


def level_rects(roi, sizes, margin, levels=5):
    """FUN_0099c694(rect, sizes, roi, margin): roi = (y0, x0, h, w) (段 0 の座標。ROI の構造体は [0] = y、[1] = x、[2] = w、[3] = h)。
    段 k: x0' = max((x0 >> k) − m, 0)、x1' = min(((x0 + w − 1) >> k) + m, W_k − 1) (y も同じ)。切り出しなし (False) は
    4 辺とも面の端に当たったとき (左は x0>>k − m < 1、右は W_k − 1 ≤ x1>>k + m)。戻り値は段ごとの (y0, x0, h, w, 切り出しあり)。
    入口と末尾のハッシュ・トークン表の処理は値に効かないとみて読んでいない (align_plan §3)。"""
    y, x, h, w = roi
    ylast, xlast = y + h - 1, x + w - 1
    out = []
    for k in range(levels):
        Wk, Hk = sizes[k][0], sizes[k][1]
        n = 0
        x0 = (x >> k) - margin
        n += x0 < 1
        x0 = max(x0, 0)
        x1 = (xlast >> k) + margin
        if Wk - 1 <= x1:
            x1 = Wk - 1
            n += 1
        y0 = (y >> k) - margin
        n += y0 < 1
        y0 = max(y0, 0)
        y1 = (ylast >> k) + margin
        if Hk - 1 <= y1:
            y1 = Hk - 1
            n += 1
        out.append((y0, x0, y1 - y0 + 1, x1 - x0 + 1, n < 4))
    return out


# --- 縮小核 FUN_00990f74 の高速経路と 1 面の駆動部 FUN_0098edb0 --------------------------------------------------

SET_TABLE = {   # 組の表 S = 0x1243980 + 0x260·組 (BuildNoiseModel と同じ表。analysis/noise/model_notes/layout.md)
    0: dict(ndir=0, rA=0, rB=0, rC=0, flag=0, f984=1.0),
    1: dict(ndir=4, rA=4, rB=0, rC=0, flag=0, f984=1.0),
    2: dict(ndir=8, rA=4, rB=0, rC=0, flag=0, f984=1.0),
    3: dict(ndir=16, rA=4, rB=6, rC=0, flag=1, f984=0.5),
    4: dict(ndir=16, rA=4, rB=6, rC=12, flag=1, f984=0.5),
}


def _neg(x):
    return (_bits(x) ^ np.uint32(0x80000000)).view(np.float32)


def ramp_ab(a, b, c, d, x, neon=True):
    """FUN_009ee93c(a, b, c, d, x, out, n) の一般形 (係数は ramp_coeffs、クランプは d ≤ c かどうかで上下が入れ替わる)。"""
    return _ramp_core(x, a, b, c, d, neon).reshape(np.shape(x))


def wmul_ab(a, b, c, d, r, prev, neon=True):
    """FUN_009eebf8(a, b, c, d, r, prev, out, n): ramp(r)·prev (NEON は ramp·prev、端数は prev·ramp)。"""
    r_in = r
    r = np.ascontiguousarray(r, np.float32).reshape(-1)
    p = np.ascontiguousarray(prev, np.float32).reshape(-1)
    w = _ramp_core(r, a, b, c, d, neon)
    n4 = _split(r.size, neon)
    out = np.empty(r.size, np.float32)
    out[:n4] = _mul(w[:n4], p[:n4])
    out[n4:] = _mul(p[n4:], w[n4:])
    return out.reshape(np.shape(r_in))


C_1P05 = np.uint32(0x3F866666).view(np.float32)     # 0x992034 / 0x992770: 1.05


def _models_mod():
    import nv_model as _nm   # base/ (repro/nv_model.py のコピー): 組の表・線カーネル・FUN_009ef0d0・FUN_009ee5fc・箱
    import nv_lines as _nl
    return _nm, _nl


def shrink(Wb, inv, cfg, model, k, Bb, Ib, A18=None, neon=True, guard_w=True, force_fast=False):
    """FUN_00990f74(core, W, dst, P4 = inv, w, h, k, cfg, model, A18, sc1, B, I, pat) の、var 出力 (sc1) を除いた本体。
    Wb: 白色化した帯域 (h, w)。Bb (float)・Ib (uint8)・A18 (float、組 3・4 の第 10 引数) は 98edb0 が段ごとに渡す作業面 (h·w)。
    戻り値 (dst, Bb, Ib, A18)。経路:
      高速経路 (0x99155c〜0x9916a4): 方向数 0、モデル語 A (0x140 + 0x140·組 + 0x40·k) = −1e6、または 2·半径 A ≥ w か h。
        X = FUN_009eebf8(t, 2t, 1, 0, R, W)、f984 > 0 なら B = 1 (0x9916a8〜0x9916dc)。
      方向経路 (0x991aac〜0x993d18): _directional を参照 (組 1〜3。組 4 (C 族・自己相似パターン) は NotImplementedError)。
      乗数 (0x991748〜0x9917c0): −amount·(1 − x20) (x20 = 0 と仮定) か適応 FUN_009ee7bc。
      cfg+0x90 ≠ 0 (0x99185c〜0x992608): 残差 W − X の高域を戻す (_detail)。
    s13 = cfg[k] + 1e-6、t = s13·(|model[5+k]|·1.1)。"""
    h, w = Wb.shape
    n = h * w
    sid = int(cfg["set"])
    st = SET_TABLE[sid]
    if guard_w and w >= 0x201:
        raise NotImplementedError("FUN_00990f74 with W > 512 enters 0x9915c0.. (not read)")
    R = strip_rms3(Wb, mode=1, s=1.0)
    s13 = _add(_F32(cfg["sigma"][k]), C_1E6)
    t = _F32(_mul(s13, _mul(_fabs(_F32(model[5 + k])), C_1P1)))
    a_word = model[80 + 80 * sid + 16 * k] if st["ndir"] else NEG_1E6
    directional = (not force_fast) and st["ndir"] and a_word != NEG_1E6 and 2 * st["rA"] < h and 2 * st["rA"] < w
    Wf = Wb.reshape(-1)
    if not directional:
        X = wmul_genuine(t, R.reshape(-1), Wf, neon=neon)
        if Bb is not None and st["f984"] > 0:
            Bb = np.ones(n, np.float32)
    else:
        X, Bb, Ib, A18 = _directional(Wb, R, t, s13, cfg, model, k, Bb, Ib, A18, neon)
    amt = _F32(cfg["amount"][k])
    b8 = _F32(cfg["b8"])
    if b8 > 0 and amt != _F32(1.0):
        a = _mul(b8, _sub(_F32(1.0), amt))
        dst = leaf_clampmul(a, _F32(-1.0), _F32(-1.0), _neg(amt), _f32(inv).reshape(-1), X, neon=neon)
    else:
        dst = leaf_scale(_mul(_F32(1.0), _neg(amt)), X, neon=neon)
    c90 = _F32(cfg["c90"][k])
    if c90 != 0:
        dst = _detail(Wf, X, R.reshape(-1), dst, c90, cfg, k, h, w, neon)
    return dst.reshape(h, w), Bb, Ib, A18


def _box_n(x, h, w, nb):
    """0x991894〜0x99231c の箱の回数: 1 回 + (nb ≥ 1 で 1 回) + (nb ≥ 2 で 2 回)。いずれも 0 詰めの 3×3 (mode 1)。"""
    y = strip_box3(x.reshape(h, w), 1)
    if nb >= 1:
        y = strip_box3(y, 1)
    if nb >= 2:
        y = strip_box3(strip_box3(y, 1), 1)
    return y.reshape(-1)


def _detail(Wf, X, Rf, dst, c90, cfg, k, h, w, neon):
    """cfg+0x90 ≠ 0 のときの後段 (0x99185c〜0x992608):
    D = W − X、Bx = 箱_n(D)、G = D − Bx、Q = 箱_n(|D|)、r = Q/(|Bx| + 0.001) (FUN_009ee04c)、G = FUN_009eebf8(1, 2, 0, 1, r, G)、
    cfg+0x8c (バイト) ≠ 0 なら q = G/(R + 1e-4)、G = FUN_009eef5c(1.33/(c90 + 1), G, q)、dst = FUN_009ed978(c90, 0, dst, G)。
    箱の回数 nb = cfg+0xa4+4k (int)。"""
    nm, _ = _models_mod()
    nb = int(cfg["ca4"][k])
    D = leaf_sub(Wf, X, neon)
    Bx = _box_n(D, h, w, nb)
    G = leaf_sub(D, Bx, neon)
    Ab = _fabs(Bx)
    Q = _box_n(_fabs(D), h, w, nb)
    r = _npx.ratio_eps(Q, Ab, np.uint32(0x3A83126F).view(np.float32))
    G = wmul_ab(_F32(1.0), _F32(2.0), _F32(0.0), _F32(1.0), r, G, neon)
    if int(cfg["c8c"]):
        q = _npx.ratio_eps(G, Rf, EPS_WHITEN_F)
        sc = _div(np.uint32(0x3FAA3D71).view(np.float32), _add(c90, _F32(1.0)))
        G = leaf_rinv(sc, G, q, neon)
    return nm.blend_add(c90, _F32(0.0), dst, G)


def _directional(Wb, R, t, s13, cfg, model, k, Bb, Ib, A18, neon):
    """方向経路 (0x991aac〜0x993d18、組 1〜3)。記号: rA = 4、K_d = 線カーネル (角度 S+8+4d)、dfilt = 半径 r で K_d、
    2 段目の添字 (S+0x348+8d) があれば続けて K_idx2 を半径 rad2 (S+0x3c8+8d)。
        Wc = FUN_009eef5c(4.5t, W, R)
        各方向 d: f = 線(K_d, Wc, rA)
          フラグ S+0 = 0 (組 1・2): g = FUN_009ef260(t, Wc, f)、F_d = dfilt(g, rA)、E_d = FUN_009950f4(1/|A_{k,d}|, F_d)
          フラグ 1 (組 3): Rr = 局所 RMS(Wc − f)、g = FUN_009ef0d0(0.75t, Wc, Rr)、F_d = dfilt(g, rA)、
                           E_d = (1/|A|)·√|3×3z(3×3z(F_d²))| (平滑半径 S+0x450 = 1)、F_d += dfilt(g − F_d, rA)
        (idx, E*) = FUN_009ee5fc(E)、w = ramp(1.05·s13, 2·, 1, 0; E*)、S = FUN_0098eba8(w, B, idx, I, …, flag = k ≠ 0)
        m = ramp(t2·s0, 2·, 0, 1; E*) (t2 = 1.05·s13、s0 = 1、B 族が有効なら c78/4 + 1)、cfg+0x3c > 0 で m を平滑化して掛ける
        Xd = Wc − m·S、D = Wc − S、(B 族かフラグ) Mb = Wc + ramp(1.75·t2, 2·, 0, −1; E*)·S
        Rx = √|3×3z(3×3z(Xd²))|、cfg+0x78 > 0 で Rx·(1 − c78·m)、wR = ramp(s13·(|R_k|·1.1), 2·, 1, 0; Rx)、
        cfg+0x28 > 0 で P = FUN_009efb1c(…)、X = wR·Xd
        (B 族かフラグ) Y = A18 + wR·Mb、A18 = FUN_009ef0d0(1.75·t2, S, E*)
        B 族 (組 3・4、半径 rB = 6、語 0x7e4 + 0x140·組 + 0x40·k ≠ −1e6、2rB < w, h): _family_b
        dc (組 2〜4、半径 4、語 0xdc + 0x14·組 + 4k ≠ −1e6、2r < w, h): q = FUN_009ef0d0(1.25t, D, R)、
          Qd = √|箱 r (q²)| (組 3・4 は箱を 2 回)、X = FUN_009eebf8(s13·|dc|, 1.5·s13·|dc|, 1, 0, Qd, X)
        P があれば X += P"""
    nm, nl = _models_mod()
    h, w = Wb.shape
    n = h * w
    sid = int(cfg["set"])
    S_ = nm.SETS[sid]
    st = SET_TABLE[sid]
    if sid >= 4:
        raise NotImplementedError("directional path of set 4 (family C, self-similar pattern FUN_00ad06ec..) not ported")
    flag = bool(S_["flag"])
    c28, c3c, c78 = _F32(cfg["c28"][k]), _F32(cfg["c3c"][k]), _F32(cfg["c78"][k])
    Wf, Rf = Wb.reshape(-1), R.reshape(-1)
    Wc = leaf_rinv(_mul(t, _F32(4.5)), Wf, Rf, neon)                       # 0x991ac0〜0x991adc
    K = nm.kernels(w, h, S_["angles"])
    nd = st["ndir"]
    rA = st["rA"]
    E = np.empty((nd, n), np.float32)
    Fd = np.empty((nd, n), np.float32)
    for d in range(nd):
        A_d = _F32(model[80 + 80 * sid + 16 * k + d])
        f = nl.linefilt(K[d], Wc.reshape(h, w), rA, 1).reshape(-1)
        if not flag:
            g = leaf_rstep(t, Wc, f, neon)                                  # 0x991c34
            F = nm.dfilt(K, d, g.reshape(h, w), rA, S_)
            Fd[d] = F.reshape(-1)
            E[d] = strip_rms3(F, mode=1, s=_div(_F32(1.0), _fabs(A_d))).reshape(-1)   # 0x991f28〜0x991f5c
        else:
            Rr = strip_rms3(leaf_sub(Wc, f, neon).reshape(h, w), mode=1, s=_F32(1.0)).reshape(-1)   # 0x991bcc〜0x991c04
            g = nm.robust_weight(_mul(t, _F32(0.75)), Wc, Rr)                 # 0x991c1c
            F = nm.dfilt(K, d, g.reshape(h, w), rA, S_).reshape(-1)
            q = strip_box3(strip_box3(leaf_sqr(F, neon).reshape(h, w), 1), 1).reshape(-1)   # 0x991cc4〜0x991e6c (平滑半径 1)
            E[d] = leaf_sqrtk(_div(_F32(1.0), _fabs(A_d)), q, neon)          # 0x991e88〜0x991ea0
            D2 = leaf_sub(g, F, neon)                                       # 0x991eb4
            F = leaf_add(F, nm.dfilt(K, d, D2.reshape(h, w), rA, S_).reshape(-1), neon)   # 0x991b70
            Fd[d] = F
    idx, best = nm.argmax_dirs(E)                                           # 0x992000 FUN_009ee5fc
    t1 = _mul(s13, C_1P05)
    if st["f984"] > 0:
        wv = ramp_ab(t1, _add(t1, t1), _F32(1.0), _F32(0.0), best, neon).reshape(-1)
        if Bb is None:
            Bb = np.zeros(n, np.float32)
        if Ib is None:
            Ib = np.zeros(n, np.uint8)
        S, best, Bn, In = leaf_sel2(wv, Bb, idx, Ib, best, Fd.reshape(-1), E.reshape(-1), int(k != 0), n, neon)
        Bb, Ib = Bn, In
    else:
        S = Fd[idx.astype(np.int64), np.arange(n)].astype(np.float32)      # FUN_0098ed34
    rB = st["rB"]
    bword = _F32(model[505 + 80 * sid + 16 * k]) if rB else NEG_1E6
    bvalid = bool(rB) and bword != NEG_1E6 and 2 * rB < w and 2 * rB < h  # 0x99145c〜0x99148c
    s0 = _F32(1.0)
    if bvalid:                                                              # 0x9926e4〜0x992700
        s0 = _add(_mul(c78, _F32(0.25)), _F32(1.0))
    t2 = _mul(s13, C_1P05)
    tm = _mul(t2, s0)
    m = ramp_ab(tm, _add(tm, tm), _F32(0.0), _F32(1.0), best, neon).reshape(-1)
    if c3c > 0:                                                             # 0x9927a4〜0x992838
        sm = nm.box2(m.reshape(h, w), 8, 1).reshape(-1)                     # 半径 S+0x1d8 = 8
        a7 = _mul(_mul(np.uint32(0x3DA3D70A).view(np.float32), c3c), _F32(0.5))   # S+0x1e0 = 0.08
        m = wmul_ab(a7, _add(a7, a7), _F32(0.0), _F32(1.0), sm, m, neon)
    Xd = leaf_acc(_F32(-1.0), m, S, Wc, neon)                               # 0x992854
    Mb = None
    if bvalid or flag:                                                      # 0x992868〜0x9928a4
        a9 = _mul(t2, _F32(1.75))
        Mb = ramp_ab(a9, _add(a9, a9), _F32(0.0), _F32(-1.0), best, neon).reshape(-1)
        Mb = leaf_acc(_F32(1.0), Mb, S, Wc, neon)
    D = leaf_sub(Wc, S, neon)                                               # 0x9928b8
    q = strip_box3(strip_box3(leaf_sqr(Xd, neon).reshape(h, w), 1), 1)      # 0x9928cc〜0x992a38
    Rx = leaf_sqrtabs(q.reshape(-1), neon)
    if c78 > 0:                                                             # 0x992a78〜0x992ab8
        Rx = leaf_mul(Rx, leaf_affine(_neg(c78), _F32(1.0), m, neon), neon)
    Rw = _F32(model[480 + 5 * sid + k])
    t4 = _mul(s13, _mul(_fabs(Rw), C_1P1))
    wR = ramp_ab(t4, _add(t4, t4), _F32(1.0), _F32(0.0), Rx, neon).reshape(-1)   # 0x992b50
    Pc = None
    if c28 > 0:                                                             # 0x992b60〜0x992c10
        sw = nm.box2(wR.reshape(h, w), 5, 1).reshape(-1)
        m1 = leaf_affine(_F32(-1.0), _F32(1.0), m, neon)                    # 1 − m
        s1 = _mul(c28, np.uint32(0x3DCCCCCD).view(np.float32))
        a8 = _sub(_F32(1.0), s1)
        b8_ = _add(_mul(s1, _F32(-0.5)), _F32(1.0))
        sw = wmul_ab(a8, b8_, _F32(0.0), _F32(1.0), sw, m1, neon)
        Pc = _pairw(sw, wR, Xd, neon)
    X = leaf_mul(wR, Xd, neon)                                              # 0x992c24
    if bvalid or flag:                                                      # 0x992c38〜0x992c88
        A18 = np.zeros(n, np.float32) if A18 is None else _f32(A18).reshape(-1)
        Y = leaf_acc(_F32(1.0), wR, Mb, A18, neon)
        A18 = nm.robust_weight(_mul(t2, _F32(1.75)), S, best)
    if bvalid:
        X = _family_b(X, Y, best, t2, s13, c3c, cfg, model, k, K, S_, st, h, w, neon)
    rdc = (0, 0, 4, 4, 4)[sid]
    dcw = _F32(model[55 + 5 * sid + k])
    if rdc >= 1 and dcw != NEG_1E6 and 2 * rdc < w and 2 * rdc < h:        # 0x993a10〜0x993bbc
        qd = nm.robust_weight(_mul(t, _F32(1.25)), D, Rf)
        qd = nm.box2(leaf_sqr(qd, neon).reshape(h, w), rdc, 1)
        if S_["dc_twice"]:                                                  # DAT_01243920[組]
            qd = nm.box2(qd, rdc, 1)
        Qd = leaf_sqrtabs(qd.reshape(-1), neon)
        a6 = _mul(s13, _fabs(dcw))
        X = wmul_ab(a6, _mul(a6, _F32(1.5)), _F32(1.0), _F32(0.0), Qd, X, neon)
    if Pc is not None:                                                      # 0x993bc0〜0x993bdc
        X = leaf_add(X, Pc, neon)
    return X, Bb, Ib, A18


def _family_b(X, Y, bestA, t2, s13, c3c, cfg, model, k, K, S_, st, h, w, neon):
    """B 族 (0x992cac〜0x993284、組 3・4):
        k = 0 なら Y = FUN_009ee1a4(0.5, 0.5, Y, 3×3z(Y))
        各方向 d: F = dfilt(Y, rB)、E_d = (1/|B_{k,d}|)·√|箱2(箱2(F²))| (箱の半径 S+0x470 = 2)、
          F = dfilt(X, rB)、F += dfilt(X − F, rB/2)
        (idx, E*_B) = FUN_009ee5fc、S_B = F_idx (FUN_0098ed34)、
        mB = ramp(t2·s9, 2·, 0, 1; E*_B) (s9 = 1: C 族なし)、cfg+0x3c > 0 で平滑化 (半径 S+0x478 = 12、係数 S+0x480)、
        X = X − mB·S_B (FUN_009edeb8(−1, mB, S_B, X))"""
    nm, nl = _models_mod()
    sid = int(cfg["set"])
    n = h * w
    rB = st["rB"]
    rBh = rB >> 1
    nd = st["ndir"]
    if k == 0:                                                              # 0x992cc0〜0x992db8
        Y = leaf_blend(_F32(0.5), _F32(0.5), Y, strip_box3(Y.reshape(h, w), 1).reshape(-1), neon)
    E = np.empty((nd, n), np.float32)
    Fd = np.empty((nd, n), np.float32)
    for d in range(nd):
        F = nm.dfilt(K, d, Y.reshape(h, w), rB, S_)
        q = nm.box2(nm.box2(leaf_sqr(F.reshape(-1), neon).reshape(h, w), 2, 1), 2, 1)
        Bw = _F32(model[505 + 80 * sid + 16 * k + d])
        E[d] = leaf_sqrtk(_div(_F32(1.0), _fabs(Bw)), q.reshape(-1), neon)
        F = nm.dfilt(K, d, X.reshape(h, w), rB, S_).reshape(-1)
        F = leaf_add(F, nm.dfilt(K, d, leaf_sub(X, F, neon).reshape(h, w), rBh, S_).reshape(-1), neon)
        Fd[d] = F
    idx, bestB = nm.argmax_dirs(E)
    SB = Fd[idx.astype(np.int64), np.arange(n)].astype(np.float32)
    tm = _mul(t2, _F32(1.0))
    mB = ramp_ab(tm, _add(tm, tm), _F32(0.0), _F32(1.0), bestB, neon).reshape(-1)
    if c3c > 0:
        sm = nm.box2(mB.reshape(h, w), 12, 1).reshape(-1)
        a7 = _mul(_mul(np.uint32(0x3D591687).view(np.float32), c3c), _F32(0.5))
        mB = wmul_ab(a7, _add(a7, a7), _F32(0.0), _F32(1.0), sm, mB, neon)
    return leaf_acc(_F32(-1.0), mB, SB, X, neon)


def _pairw(a, b, c, neon=True):
    """FUN_009efb1c(a, b, c, out, n): c·(max(a, b) − b) (NEON は FMAX、端数は a > b ? a : b)。
    sandbox/omo/wp_temporal/src/nv_temporal.pairw_genuine のコピー (nvh pairw とビット一致済み)。"""
    a, b, c = (np.ascontiguousarray(x, np.float32).reshape(-1) for x in (a, b, c))
    n4 = _split(a.size, neon)
    m = np.empty(a.size, np.float32)
    m[:n4] = _vfmax(a[:n4], b[:n4])
    m[n4:] = _gt_sel(a[n4:], b[n4:], a[n4:], b[n4:])
    return _mul(c, _sub(m, b))


def _expand(compact, rect, full_shape, dtype):
    """FUN_00994338(src, dst, W, H, rect, elem): 切り出しの面を全面の位置へ戻し、外側を 0 で埋める。"""
    y0, x0, hh, ww = rect[:4]
    out = np.zeros(full_shape, dtype)
    out[y0:y0 + hh, x0:x0 + ww] = np.asarray(compact, dtype).reshape(hh, ww)
    return out


def _up_nearest_u8(I, oddW, oddH):
    """0x98fe98〜0x98ffe4: 1 バイトずつ 2×2 に複製し、奇数の幅は各行の最後の値、奇数の高さは最後の行を繰り返す。"""
    I = np.asarray(I, np.uint8)
    out = np.repeat(np.repeat(I, 2, 0), 2, 1)
    if oddW:
        out = np.concatenate([out, out[:, -1:]], 1)
    if oddH:
        out = np.concatenate([out, out[-1:]], 0)
    return out


def plane_filter(P, sigma, cfg, model, roi=None, neon=True, allow_fast_only=False, guard_w=True):
    """FUN_0098edb0 の 1 面 (var 出力 (第 10 引数) なし)。
    P: 面 (H, W) float32、sigma: 段ごとの σ 面のリスト (段 k の大きさ。FUN_009aae90 の出力)、cfg: build_cfg の 1 面、
    model: プロファイルの変換面のモデル (0x1f54/4 語の float32)、roi: (y0, x0, h, w) か None (全面 = 切り出しなし)。
    作業面 B (第 12 引数、[x29−0xb8]) と I (第 13 引数、[x29−0xd0]、バイト) は f984 = 組の表 S+4 > 0 のとき (組 0〜4 すべて) 使う:
    B は 1.0、I は 0 で始め、段ごとに B ← s10 − s10·B (FUN_009ee3fc(−s10, s10))、縮小核の後に B は FUN_00993df0、I は最近傍で拡大。
    allow_fast_only: 方向経路の条件が成り立つ段も高速経路で計算する (本家とは異なる。部分経路の確認用)。"""
    P = _f32(P)
    H, W = P.shape
    L = int(cfg["levels"])
    amt = [_F32(a) for a in cfg["amount"]]
    c90 = [_F32(a) for a in cfg["c90"]]
    if not (_F32(cfg["b8"]) > 0) and all(amt[k] == 0 and c90[k] == 0 for k in range(L)):
        return P.copy()                                        # 0x98ef04: FUN_009f14e4 でコピー
    sid = int(cfg["set"])
    s10 = _F32(SET_TABLE[sid]["f984"])
    use_b = s10 > 0
    use_a18 = bool(SET_TABLE[sid]["flag"])                    # [sp+0xec] = 組の表 S+0 (0x98ef9c)
    sizes = level_sizes(W, H, L)
    # 分解 (0x98f184〜0x98f368)
    bands, cur = [], P
    for k in range(L):
        wk, hk = sizes[k][0], sizes[k][1]
        blur = strip_box3(cur, mode=0)       # W < 4 か H < 2 なら分離型 (0x98f24c〜0x98f280、strip_box3 の中で切り替え)
        coarse = halve(blur)
        if coarse is None:
            raise NotImplementedError("level with H < 2: FUN_009f15b4 writes nothing (align_plan 5)")
        bands.append(band_from(coarse, cur, sizes[k][2], sizes[k][3]))
        cur = coarse
    rects = (level_rects(roi, sizes, SET_MARGIN[sid], L) if roi is not None
             else [(0, 0, sizes[k][1], sizes[k][0], False) for k in range(L)])
    # 再構成 (0x98f774〜0x9902c4)
    k = L - 1
    shp = (sizes[k][1], sizes[k][0])
    acc = np.zeros(shp, np.float32)                               # 0x98f468: FUN_009f13bc で 0 埋め
    Bf = np.ones(shp, np.float32) if use_b else None             # 0x98f4c0: 1.0
    If = np.zeros(shp, np.uint8) if use_b else None              # 0x98f4e8: 0
    Af = np.zeros(shp, np.float32) if use_a18 else None          # 0x98f498: 0 ([x29−0xc0])
    for k in range(L - 1, -1, -1):
        y0, x0, hh, ww, tiled = rects[k]
        sg = _f32(sigma[k])
        if tiled:
            inv = np.stack([leaf_recip(EPS_WHITEN_F, sg[y0 + r, x0:x0 + ww], neon) for r in range(hh)])
            Wb = np.stack([leaf_mul(bands[k][y0 + r, x0:x0 + ww], inv[r], neon) for r in range(hh)])
            if use_b:
                Bc = np.concatenate([leaf_affine(_neg(s10), s10, Bf[y0 + r, x0:x0 + ww], neon) for r in range(hh)])
                Ic = If[y0:y0 + hh, x0:x0 + ww].reshape(-1).copy()
            Ac = Af[y0:y0 + hh, x0:x0 + ww].reshape(-1).copy() if use_a18 else None   # 0x98f8e0〜0x98f950
        else:
            inv = leaf_recip(EPS_WHITEN_F, sg.reshape(-1), neon).reshape(sg.shape)
            Wb = leaf_mul(bands[k].reshape(-1), inv.reshape(-1), neon).reshape(sg.shape)
            if use_b:
                Bc = leaf_affine(_neg(s10), s10, Bf.reshape(-1), neon)
                Ic = If.reshape(-1).copy()
            Ac = Af.reshape(-1).copy() if use_a18 else None
        if not use_b:
            Bc = Ic = None
        try:
            dst, Bc, Ic, Ac = shrink(Wb, inv, cfg, model, k, Bc, Ic, Ac, neon=neon, guard_w=guard_w)
        except NotImplementedError:
            if not allow_fast_only:
                raise
            dst, Bc, Ic, Ac = shrink(Wb, inv, cfg, model, k, Bc, Ic, Ac, neon=neon, guard_w=guard_w, force_fast=True)
        if tiled:
            for r in range(hh):
                acc[y0 + r, x0:x0 + ww] = leaf_acc(_F32(1.0), dst[r], sg[y0 + r, x0:x0 + ww], acc[y0 + r, x0:x0 + ww], neon)
        else:
            acc = leaf_acc(_F32(1.0), dst.reshape(-1), sg.reshape(-1), acc.reshape(-1), neon).reshape(acc.shape)
        if k:
            oW, oH = sizes[k - 1][2], sizes[k - 1][3]
            acc = up2(acc, oW, oH)
            if use_a18:                                             # 0x98fd50〜0x98fdd0
                Ak = _expand(Ac, rects[k], sg.shape, np.float32) if tiled else _f32(Ac).reshape(sg.shape)
                Af = up2(Ak, oW, oH)
            if use_b:
                full = sg.shape
                Bk = _expand(Bc, rects[k], full, np.float32) if tiled else Bc.reshape(full)
                Ik = _expand(Ic, rects[k], full, np.uint8) if tiled else Ic.reshape(full)
                Bf = up2(Bk, oW, oH)
                If = _up_nearest_u8(Ik, oW, oH)
    return leaf_add(P.reshape(-1), acc.reshape(-1), neon).reshape(H, W)


# --- cfg (FUN_0099ead0) と設定構造体 -----------------------------------------------------------------------------

CFG_TEMPLATE = dict(   # 0x12445a8 (0xd0 B、__TEXT、静的に読み出し)
    sigma=(1.06, 1.0, 1.0, 1.0, 1.0), amount=(1.0,) * 5, c28=(1.0,) * 5, c3c=(1.0,) * 5, c50=(1.0, 1.0, 1.0, 0.0, 0.0),
    c64=(1.0, 1.0, 1.0), c70=(0.0,) * 7, c8c=1, c90=(1.0, 1.0, 1.0, 0.0, 0.0), ca4=(1,) * 5, b8=0.0, levels=4, set=2)
SET_SELECT = {   # 0x1246c48 [品質 s+0x104][色モード] → (面 0, 1, 2) の組 (静的に読み出し)
    (0, 0): (2, 2, 2), (1, 0): (3, 3, 3), (2, 0): (4, 4, 4),
    (0, 1): (2, 1, 1), (1, 1): (3, 2, 2), (2, 1): (4, 2, 2),
    (0, 2): (2, 1, 1), (1, 2): (3, 2, 2), (2, 2): (4, 2, 2),
}


def default_settings():
    """設定構造体 (0xb0c B) のうち空間段が読む語。**既定値は未特定** (コンストラクタを見つけていない)。ここでは
    「空間フィルタ有効・ノイズ量と Amount はすべて 1・品質 Normal・適応と混合は無効」を仮定する。オフセットはバイト。"""
    return {0x00: 1, 0x04: 1.0, 0x08: (1.0,) * 5, 0x1c: (1.0,) * 5, 0x30: (1.0,) * 3, 0x3c: (1.0,) * 3,
            0x48: 0, 0x4c: 1.0, 0x50: 1.0, 0x54: (1.0,) * 3, 0x60: 0, 0x64: (0.0,) * 3,
            0x90: 0, 0x91: 1, 0x94: (1.0,) * 3, 0xa0: (0.0,) * 3, 0x154: (1, 1, 1), 0x104: 0, 0xcd: 0, 0xd0: 0, 0xec: 0.0, 0x128: 0.0,
            0x10c: 0, 0x110: 1.0, 0x114: (1.0,) * 3}


def levels_from_settings(s, flag=0):
    """FUN_009ae918 / FUN_0099ead0 の段数: flag & 1 なら 5。そうでなければ s[0x2c] > 0 → 5、s[0x28] > 0 → 4、
    それ以外は 3 (0x99f5xx のループは結果が 2 にしかならない)。"""
    if flag & 1:
        return 5
    a = [_F32(v) for v in s[0x1c]]
    if a[4] > 0:
        return 5
    if a[3] > 0:
        return 4
    return 3


def build_cfg(s, colour_mode=1, flag=0):
    """FUN_0099ead0(ctx, flag) が書く 3 面の cfg のうち、空間段の高速経路が読む語 (演算順は逆アセンブル 0x99ec14〜0x99ed60)。
    cfg[k] = cfg0[k]·(s4·(s[8+4k]·s[0x30+4c]))、amount[k] = amount0[k]·((s[0x3c+4c]·s[0x1c+4k])·(float)s0)
    (s0 = 0 の枝は amount に 0 を掛けるだけで cfg[k] は変えない)、b8 = s[0xcd] ? √(2·(a·q²) + 1)/(2·(q·a) + 1) : 0
    (a = s[0xec]·(float)s[0xd0]、q = s[0x128])、set = SET_SELECT[品質, 色モード][c]、levels = levels_from_settings。
    方向経路が読む語 (+0x28〜+0x8c、+0x90〜+0xb4) は未移植。"""
    T = CFG_TEMPLATE
    s0 = _F32(int(s[0x00]) & 0xFF)
    if int(s[0xcd]):
        a = _mul(_F32(s[0xec]), _F32(int(s[0xd0])))
        q = _F32(s[0x128])
        aq = _mul(a, _mul(q, q))
        num = _sqrt(_add(_add(aq, aq), _F32(1.0)))
        t = _mul(q, a)
        b8 = _F32(_div(num, _add(_add(t, t), _F32(1.0))))
    else:
        b8 = _F32(0.0)
    qual = int(s[0x104])
    # 0x99f5xx: 表 0x1246c48 + (long)(int)s[0x104]·0x18 + (u32)色モード·0x48 から 3 語を読む (範囲の確認はない)。
    # 品質 0〜2・色モード 0〜2 の外では本家は表の外 (別の行かその先のデータ) を読むので、ここでは拒否する
    if not (0 <= qual <= 2 and 0 <= int(colour_mode) <= 2):
        raise ValueError("quality %d / colour mode %d: the genuine reads outside the 0x1246c48 table" % (qual, int(colour_mode)))
    sets = SET_SELECT[(qual, int(colour_mode))]
    L = levels_from_settings(s, flag)
    out = []
    for c in range(3):
        sig, am = [], []
        for k in range(5):
            if int(s[0x00]):
                sig.append(_F32(_mul(_F32(T["sigma"][k]), _mul(_F32(s[0x04]), _mul(_F32(s[0x08][k]), _F32(s[0x30][c]))))))
            else:
                sig.append(_F32(T["sigma"][k]))
            am.append(_F32(_mul(_F32(T["amount"][k]), _mul(_mul(_F32(s[0x3c][c]), _F32(s[0x1c][k])), s0))))
        f90 = _F32(int(s[0x00]) * int(s[0x90]) & 0xFF)
        g = _F32(s[0x94][c])
        # cfg+0x90+4k (k < 3) (0x99f384〜、演算順は逆アセンブルで確認)
        c90 = tuple(_F32(_mul(_F32(T["c90"][k]), _mul(_mul(_F32(s[0xa0][k]), f90), g))) if k < 3 else _F32(T["c90"][k])
                    for k in range(5))   # 0x99f384〜: c90·((s[0xa0+4k]·f90)·s[0x94+4c])
        # 0x99f0d0〜0x99f290: f48 = (float)(u8)(s0·s[0x48])、f60 = (float)(u8)(s0·s[0x60])。段 3・4 は雛形のまま
        f48 = _F32(int(s[0x00]) * int(s[0x48]) & 0xFF)
        f60 = _F32(int(s[0x00]) * int(s[0x60]) & 0xFF)
        c28 = tuple(_F32(_mul(_F32(T["c28"][j]), _mul(_F32(s[0x54][j]), _mul(_F32(s[0x4c]), f48)))) if j < 3
                    else _F32(T["c28"][j]) for j in range(5))
        c3c = tuple(_F32(_mul(_F32(T["c3c"][j]), _mul(_mul(_F32(s[0x50]), f48), _F32(s[0x54][j])))) if j < 3
                    else _F32(T["c3c"][j]) for j in range(5))
        c78 = tuple(_F32(_mul(_F32(s[0x64][j]), f60)) if j < 3 else _F32(0.0) for j in range(5))
        ca4 = tuple(int(s[0x154][j]) if j < 3 else 1 for j in range(5))     # 0x99f3e8..: s[0x154+4j] (段 3・4 は雛形の 1)
        out.append(dict(sigma=tuple(sig), amount=tuple(am), c90=c90, c28=c28, c3c=c3c, c78=c78, ca4=ca4,
                        c8c=int(s[0x91]) & 0xFF, b8=b8, levels=L, set=sets[c]))
    return out


# --- σ マップ (FUN_009aae90) と輝度別テーブル (FUN_0099e6f4 / FUN_0099e264) -------------------------------------------

MATRIX = {   # DAT_0147df80[色モード] (0x97b980 / 0x97b9b0 / 0x97ba18 が書く 9 語。m[c][j])
    0: np.array([[1, 0, 0], [0, 1, 0], [0, 0, 1]], np.float32),
    1: np.array([0x3DB6E2EB, 0x3EB020C5, 0x3C56A162, 0x3E800000, 0x3E333333, 0x3BD844D0,
                 0x3CE8A71E, 0x3DE075F7, 0x3E800000], np.uint32).view(np.float32).reshape(3, 3),
    2: np.array([0x3DE353F8, 0x3DE353F8, 0x3DE353F8, 0x3E800000, 0x3D800000, 0x3D800000,
                 0x3D800000, 0x3D800000, 0x3E800000], np.uint32).view(np.float32).reshape(3, 3),
}


def lum_tables(profile_f32, gains=(1.0, 1.0, 1.0)):
    """FUN_0099e6f4 + FUN_0099e264(ctx, 0): 面 j・段 k の 257 値 T = (g_j·(c_b·f_{b,k} + frac·(c_{b+1}·f_{b+1,k} − c_b·f_{b,k})))²。
    曲線は profile+0xbf44 + 0x120·j + 0x20·b (語 0 = c、語 1+k = f)、b = min((long)(i·0.03125), 7)、frac = i·0.03125 − b。
    gains は ctx+0x114d0.. (FUN_0099e264 の第 2 引数が 0 なら 1.0)。**演算順は逆コンパイルから** (FUN_0099e6f4 の逆アセンブルは
    入口のハッシュ書き込みを避けて未確認。FMA の有無は要確認)。戻り値 (3, 5, 257) float32。"""
    p = np.asarray(profile_f32, np.float32)
    out = np.empty((3, 5, 257), np.float32)
    i = np.arange(257, dtype=np.float32)
    x = _mul(i, _F32(0.03125))
    b = np.minimum(x.astype(np.int64), 7)
    frac = _sub(x, b.astype(np.float32))
    for j in range(3):
        base = (0xBF44 + 0x120 * j) // 4
        rec = p[base:base + 72].reshape(9, 8)
        for k in range(5):
            v0 = _mul(rec[b, 0], rec[b, 1 + k])
            v1 = _mul(rec[b + 1, 0], rec[b + 1, 1 + k])
            T = _add(v0, _mul(frac, _sub(v1, v0)))
            g = _mul(_F32(gains[j]), T)
            out[j, k] = _mul(g, g)
    return out


def sigma_coefs(model_rgb_q99, mode, k, scale=1.0):
    """FUN_009aae90 の c_cj (0x9ab484〜0x9ab674): a_j² = (面 j のモデル語 5+k)²、inv_c = 1/((a_R²·m_c0 + a_G²·m_c1) + a_B²·m_c2)、
    inv_c' = inv_c·scale (scale は 17−popcount 型の係数で、正規時 1 と仮定)、c_cj = (a_j²·m_cj)·inv_c'。戻り値 (3, 3)。"""
    m = MATRIX[mode]
    a2 = [_mul(_F32(v), _F32(v)) for v in model_rgb_q99]
    C = np.empty((3, 3), np.float32)
    for c in range(3):
        p = [_mul(a2[j], m[c, j]) for j in range(3)]
        inv = _mul(_div(_F32(1.0), _add(_add(p[0], p[1]), p[2])), _F32(scale))
        for j in range(3):
            C[c, j] = _mul(p[j], inv)
    return C


def sigma_level(Rk1, Gk1, Bk1, tabs_k, C, neon=True):
    """段 k の σ (段 k+1 の大きさ): NEON かつ n ≥ 5 は FUN_009f1d8c (添字 = FCVTNU の下位 8 ビット、
    σ_c = √(FMLA(FMLA(c_c0·T_R, T_G, c_c1), T_B, c_c2)))、それ以外は FUN_009f02b4 (飽和) と ((c0·T_R + c1·T_G) + c2·T_B)、FUN_009ed3e0。"""
    R, G, B = (_f32(a).reshape(-1) for a in (Rk1, Gk1, Bk1))
    n = R.size
    tR, tG, tB = tabs_k
    if neon and n >= 5:
        def idx(x):
            with np.errstate(invalid="ignore"):
                r = np.rint(x.astype(np.float64))
                r = np.where(np.isnan(r) | (r < 0), 0.0, np.minimum(r, 4294967295.0))
            return (r.astype(np.uint64) & np.uint64(0xFF)).astype(np.int64)
        TR, TG, TB = tR[idx(R)], tG[idx(G)], tB[idx(B)]
        out = []
        for c in range(3):
            v = _mul(C[c, 0], TR)
            v = _fma(v, TG, C[c, 1])
            v = _fma(v, TB, C[c, 2])
            out.append(_sqrt(v))
        return out
    TR, TG, TB = tR[leaf_u8(R, neon)], tG[leaf_u8(G, neon)], tB[leaf_u8(B, neon)]
    out = []
    for c in range(3):
        v = _add(_add(_mul(C[c, 0], TR), _mul(C[c, 1], TG)), _mul(C[c, 2], TB))
        out.append(leaf_sqrtabs(v, neon))
    return out


def sigma_maps(R, G, B, models_rgb, mode, L, tabs, extra=None, neon=True):
    """FUN_009aae90(ctx, R, G, B, w, h, extra, σ0, σ1, σ2, 0, L): 段 k = 0..L−1 の σ_c (段 k の大きさ) のリスト 3 本。
    models_rgb: プロファイルの面 0..2 (R, G, B) のモデル (語 5+k を使う)。extra は (1/255)·extra を掛ける面 (FUN_009edd6c) か None。"""
    H, W = _f32(R).shape
    sizes = level_sizes(W, H, L)
    cur = [_f32(R), _f32(G), _f32(B)]
    ex = _f32(extra) if extra is not None else None
    out = [[], [], []]
    for k in range(L):
        nxt = [halve(p) for p in cur]
        if any(p is None for p in nxt):
            raise NotImplementedError("FUN_00990d2c with H < 2 writes nothing (align_plan 5)")
        if ex is not None:
            ex = halve(ex)
        C = sigma_coefs([models_rgb[j][5 + k] for j in range(3)], mode, k)
        sg = sigma_level(nxt[0], nxt[1], nxt[2], (tabs[0, k], tabs[1, k], tabs[2, k]), C, neon)
        hk1, wk1 = nxt[0].shape
        for c in range(3):
            s = sg[c]
            if ex is not None:
                s = leaf_mulk(C_255INV, s, ex.reshape(-1), neon)
            out[c].append(up2(s.reshape(hk1, wk1), sizes[k][2], sizes[k][3]))
        cur = nxt
    return out


def sigfuse_port(rgb, tabs, C):
    """FUN_009f1d8c(R, G, B, T_R, T_G, T_B, o0, o1, o2, coef, n) そのもの (n を 4 の倍数に切り上げて処理する本家の振る舞いは、
    出力の先頭 n 個に影響しない)。tabs は 3 本の 257 語、C は (3, 3)。"""
    return sigma_level(rgb[0], rgb[1], rgb[2], tabs, C, neon=True) if np.asarray(rgb[0]).size >= 5 else \
        _sigfuse_small(rgb, tabs, C)


def _sigfuse_small(rgb, tabs, C):
    R, G, B = (_f32(a).reshape(-1) for a in rgb)
    def idx(x):
        with np.errstate(invalid="ignore"):
            r = np.rint(x.astype(np.float64))
            r = np.where(np.isnan(r) | (r < 0), 0.0, np.minimum(r, 4294967295.0))
        return (r.astype(np.uint64) & np.uint64(0xFF)).astype(np.int64)
    TR, TG, TB = tabs[0][idx(R)], tabs[1][idx(G)], tabs[2][idx(B)]
    out = []
    for c in range(3):
        v = _fma(_fma(_mul(C[c, 0], TR), TG, C[c, 1]), TB, C[c, 2])
        out.append(_sqrt(v))
    return out


C_IX = np.array([0xBEB0F27C, 0xBF36E979, 0x3FB37B4A, 0x3FE2B021], np.uint32).view(np.float32)   # 0x9f1d1c〜0x9f1d48


def ixform_neon(Y, a, b):
    """FUN_009f1d0c(Y, a, b, o0, o1, o2, n) (n は 4 の倍数): o0 = FMLA(Y, 1.4022, a)、o1 = FMLA(FMLA(Y, −0.3446, b), −0.7145, a)、
    o2 = FMLA(Y, 1.7710, b)。"""
    Y, a, b = (_f32(x).reshape(-1) for x in (Y, a, b))
    o1 = _fma(_fma(Y, C_IX[0], b), C_IX[1], a)
    o0 = _fma(Y, C_IX[2], a)
    o2 = _fma(Y, C_IX[3], b)
    return [o0, o1, o2]


def ixform(Y, a, b, mode=1, neon=True):
    """DAT_0147df68[色モード]。モード 0 はコピー、モード 1 は 0x97b76c (先頭 n & ~3 個は ixform_neon、残りは
    R = Y + a·1.4022、G = (Y + b·(−0.3446)) + a·(−0.7145)、B = Y + b·1.7710 の非融合)。モード 2 (0x97b924) は未移植。
    0x97b7a0〜0x97b81c のトークン表による出力ポインタの加算は 0 と仮定 (align_plan §3)。"""
    Y, a, b = (_f32(x).reshape(-1) for x in (Y, a, b))
    if mode == 0:
        return [Y.copy(), a.copy(), b.copy()]
    if mode != 1:
        raise NotImplementedError("inverse transform of colour mode 2 (0x97b924) not ported")
    n4 = _split(Y.size, neon)
    out = [np.empty_like(Y) for _ in range(3)]
    if n4:
        o = ixform_neon(Y[:n4], a[:n4], b[:n4])
        for c in range(3):
            out[c][:n4] = o[c]
    if n4 < Y.size:
        y, aa, bb = Y[n4:], a[n4:], b[n4:]
        out[0][n4:] = _add(y, _mul(aa, C_IX[2]))
        out[1][n4:] = _add(_add(y, _mul(bb, C_IX[0])), _mul(aa, C_IX[1]))
        out[2][n4:] = _add(y, _mul(bb, C_IX[3]))
    return out


# --- v5 互換の入口 (本家の入力ではなく v5 の σ・Amount で動かす実験用。区間 A/B の測定はこれで行った) ----------------------

def spatial_genuine(planes, prof, rv, P):
    """v5 の spatial_v5 と同じ署名。σ は v5 の成分 (curve × band_ratio × Noise Level × √rv) を段解像度へ縮小した代用、
    Amount も v5 の band_amounts × luma/chroma (本家の cfg ではない)。分解・白色化・局所 RMS・ランプ・累積は本家の葉
    (2026-10-21 版: 4 列帯の箱、FUN_00990da8 / FUN_00993df0、FRECPE の逆数、0 詰めの局所 RMS、FMLA の累積)。
    しきい値倍率は P["genuine_t"] (既定 1.0 → t = 1.1·1e-6 込みの本家式に v5 の σ を入れたもの)。"""
    import nvfilter as nv  # base/ へのコピー
    Y = np.asarray(planes[0], np.float32)
    lum = nv.gaussian_filter(Y, 1.0)
    tscale = float(P.get("genuine_t", 1.0))
    out = []
    for c, (name, p) in enumerate(zip(("Y", "Cb", "Cr"), planes)):
        L = prof["levels"][name]
        names = nv.V5_BANDS[len(nv.V5_BANDS) - L:] if c else nv.V5_BANDS[:L]
        A = float(P["luma"] if c == 0 else P["chroma"])
        s1 = (nv.curve_sigma(prof, name, nv._to_grid(lum, p.shape)) * P["noise_level"]
              * P["channel_noise"][name] * np.sqrt(rv[c])).astype(np.float32)
        plane = _f32(p)
        H, W = plane.shape
        sizes = level_sizes(W, H, L)
        sig, cur = [], s1
        for k in range(L):
            sig.append(_f32(cur * F32k(prof["band_ratio"][name][k] * P["band_noise"][names[k]])))
            cur = halve(cur) if k < L - 1 else cur
        model = np.full(0x1f54 // 4, NEG_1E6, np.float32)
        model[5:5 + L] = 1.0
        cfg = dict(sigma=(tscale,) * 5, amount=tuple(min(max(A * P["band_amounts"][names[k]], 0.0), 1.0) for k in range(L))
                   + (0.0,) * (5 - L), c90=(0.0,) * 5, c28=(0.0,) * 5, c3c=(0.0,) * 5, c78=(0.0,) * 5, ca4=(1,) * 5, c8c=1, b8=0.0, levels=L, set=0)
        out.append(plane_filter(plane, sig, cfg, model, guard_w=False))   # 全面 (チャンクに分けない) なので W > 512 の防御を外す
    return out


def F32k(v):
    return np.float32(v)


def box3(a):
    """FUN_0098edb0 の分解の 3×3 箱 (W ≥ 4、H ≥ 2 の NEON 経路) = strip_box3(a, 0)。"""
    return strip_box3(a, 0)


def leaf_sel2(a, b, i3, i4, best, src, wt, flag, stride, neon=True):
    """FUN_0098eba8(a, b, i3, i4, out, best, src, wt, flag, stride, n) (先頭 n & ~3 個は FUN_009f19cc)。
    f = wt[i4·stride + i]·v、v = NEON は FMLA(1, b, a)、端数は (a·b) + 1。f > best (順序付き) なら best = f・out = src[i4·stride + i]、
    そうでなければ out = src[i3·stride + i]。flag なら b ← a、i4 ← i3 も書く。戻り値 (out, best, b, i4) (入力は書き換えない)。"""
    a, b, best = (_f32(x).reshape(-1).copy() for x in (a, b, best))
    i3 = np.asarray(i3, np.uint8).reshape(-1)
    i4 = np.asarray(i4, np.uint8).reshape(-1).copy()
    src, wt = _f32(src).reshape(-1), _f32(wt).reshape(-1)
    n = a.size
    n4 = _split(n, neon)
    i = np.arange(n)
    k4 = i4.astype(np.int64) * stride + i
    k3 = i3.astype(np.int64) * stride + i
    v = np.empty(n, np.float32)
    if n4:
        v[:n4] = _fma(_F32(1.0), b[:n4], a[:n4])
    v[n4:] = _add(_mul(a[n4:], b[n4:]), _F32(1.0))
    f = _mul(wt[k4], v)
    m = f > best
    out = np.where(m, src[k4], src[k3]).astype(np.float32)
    best = np.where(m, f, best).astype(np.float32)
    if flag & 1:
        b = a.copy()
        i4 = i3.copy()
    return out, best, b, i4


# --- チャンク (FUN_009ae918) とフレーム (Filter 0x9ac2b0 → FUN_009adc8c) の駆動部 (2026-10-21) ---------------------------

CHUNK_CORE = {0: (96,) * 5, 1: (96,) * 5, 2: (96, 96, 96, 96, 128), 3: (96, 96, 96, 128, 128),   # 0x1243740[組][L−1]
              4: (96, 128, 192, 192, 192)}
CHUNK_MARGIN = {0: (1, 2, 4, 8, 16), 1: (2, 4, 8, 16, 32), 2: (4, 8, 16, 32, 64), 3: (6, 12, 24, 48, 96),   # 0x1243808[組][L−1]
                4: (32, 64, 128, 128, 128)}


def profile_parts(prof):
    """プロファイル 0xc300 B (repro/nv_profile.build_profile) から、色モード (+0x48) と 6 面のモデル (+0x4c + 0x1f54·i)。"""
    b = np.asarray(prof, np.uint8)
    pf = b.view(np.float32)
    mode = int(b[0x48:0x4c].view(np.uint32)[0])
    models = [pf[(0x4c + i * 0x1f54) // 4:(0x4c + (i + 1) * 0x1f54) // 4] for i in range(6)]
    return mode, models, pf


def chunk_geometry(cfgs):
    """FUN_0099ead0 の末尾 (0x99f62c〜): 核 ctx[0x7f0] = max_c 0x1243740[組_c][L−1]、余白 ctx[0x7f1] = max_c 0x1243808[組_c][L−1]。"""
    L = int(cfgs[0]["levels"])
    core = max(CHUNK_CORE[int(c["set"])][L - 1] for c in cfgs)
    margin = max(CHUNK_MARGIN[int(c["set"])][L - 1] for c in cfgs)
    return core, margin


def inv_transform(p0, p1, p2, mode, neon=True):
    """DAT_0147df68[色モード] (0x9af204〜0x9af22c で出力の 3 面にその場で掛ける)。モード 2 は 0x97b924:
    R = Y + (a·4)/3、B = Y + (b·4)/3、G = Y − ((a + b)·4)/3 (すべて非融合、NEON 経路なし)。"""
    if mode == 2:
        Y, a, b = (_f32(x).reshape(-1) for x in (p0, p1, p2))
        f4, f3 = _F32(4.0), _F32(3.0)
        R = _add(Y, _div(_mul(a, f4), f3))
        B = _add(Y, _div(_mul(b, f4), f3))
        G = _sub(Y, _div(_mul(_add(a, b), f4), f3))
        return [R, G, B]
    if mode == 3:
        mode = 0
    return ixform(p0, p1, p2, mode, neon)


def filter_chunk(rgb, prof, settings, roi=None, orig=None, extra=None, grey=False, neon=True, flag=0):
    """FUN_009ae918 の 1 チャンク: rgb (3, h, w) float32 (0..255)、prof は 0xc300 B のプロファイル、settings は設定構造体の語
    (default_settings の形)、roi = (y0, x0, h, w) (チャンク内の核。None なら全面 = 切り出しなし)。orig は混合の元 (Filter の
    第 3 引数側の画像。None なら rgb)、extra は σ マップに (1/255)·extra を掛ける面 (FUN_009adc8c の第 5 引数。None = なし)。
    grey (画像の +0xf4) なら面 1・2 は素通し (0x9aec60〜)。戻り値 (3, h, w) の RGB。
    流れ: σ = FUN_009aae90 → 色変換 DAT_0147df50 → 面ごと FUN_0098edb0 → 混合 (設定 +0x10c / +0x110..+0x11c) → 逆変換 DAT_0147df68。"""
    import nv_flat as _nf          # base/ (repro/nv_flat.py のコピー): 色変換 DAT_0147df50 (`nvh xform` と一致)
    R, G, B = (_f32(x) for x in rgb)
    h, w = R.shape
    mode, models, pf = profile_parts(prof)
    cfgs = build_cfg(settings, colour_mode=mode, flag=flag)
    L = int(cfgs[0]["levels"])
    tabs = lum_tables(pf)
    sig = sigma_maps(R, G, B, models[:3], mode, L, tabs, extra=extra, neon=neon)
    P = [np.ascontiguousarray(p, np.float32) for p in _nf.transform(R, G, B, mode)]
    out = []
    for c in range(3):
        if grey and c:
            out.append(P[c].copy())
            continue
        out.append(plane_filter(P[c], sig[c], cfgs[c], models[3 + c], roi=roi, neon=neon))
    s = settings
    fs = [_F32(s[0x110])] + [_F32(v) for v in s[0x114]]
    if int(s[0x10c]) and any(v != _F32(1.0) for v in fs):
        O = orig if orig is not None else rgb
        Q = [np.ascontiguousarray(p, np.float32) for p in _nf.transform(*(_f32(x) for x in O), mode)]
        for c in range(3):
            f = _mul(fs[1 + c], fs[0])                                   # s[0x114+4c]·s[0x110]
            if f != _F32(1.0):
                out[c] = leaf_blend(_sub(_F32(1.0), f), f, Q[c].reshape(-1), out[c].reshape(-1), neon).reshape(h, w)
    res = inv_transform(out[0], out[1], out[2], mode, neon)
    return np.stack([np.asarray(r, np.float32).reshape(h, w) for r in res])


def filter_frame(rgb, prof, settings, orig=None, extra=None, grey=False, neon=True, demo_limit=False, flag=0):
    """Filter (0x9ac2b0) の CPU 経路を 1 スレッドで: 48px 未満の辺があれば処理しない (コピー)。チャンクはスケジューラ
    (FUN_00ac14d4 → FUN_00abf6a8) の格子 = repro/nv_curve.tile_layout(W, H, 核, 余白, 48) (`nvh tiles` と 204 配置で一致)。
    各チャンクの ROI = (余白 ty, 余白 lx, 核 h, 核 w) (FUN_009adc8c がタスク記述子の [9], [8], [7], [6] から作る)、
    出力は核だけを書き戻す (FUN_009af2f8)。demo_limit: FUN_00ac14d4 が大域 0x14de194.. (ファイルの値) で幅を 1856・高さを 1024 に
    中央で切り詰める処理を再現する (本家出力 test_v02_reduce は端の列・行も処理しているので既定は False。align_plan §6)。"""
    import nv_curve as _nc
    rgb = np.asarray(rgb, np.float32)
    _, Hh, Ww = rgb.shape
    out = rgb.copy()
    if Ww < 48 or Hh < 48:
        return out
    mode, _, _ = profile_parts(prof)
    core, margin = chunk_geometry(build_cfg(settings, colour_mode=mode, flag=flag))
    x0r, y0r, Wr, Hr = 0, 0, Ww, Hh
    if demo_limit:
        if Wr > 0x5C * 20 + 16:
            d = Wr - 0x135 * 6
            x0r += ((d - 1) if d - 2 < 0 else (d - 2)) >> 1
            Wr = 0x3A0 * 2
        lim = 0x2C * 23 + 12
        if Hr > lim:
            d = Hr - lim
            y0r += ((d + 1) if d < 0 else d) >> 1
            Hr = lim
    for t in _nc.tile_layout(Wr, Hr, core, margin, 48):
        tx, ty, tw, th, _, _, cw, ch, lx, ly, _ = t
        tx += x0r
        ty += y0r
        sl = (slice(None), slice(ty, ty + th), slice(tx, tx + tw))
        o = filter_chunk(rgb[sl], prof, settings, roi=(ly, lx, ch, cw),
                         orig=None if orig is None else np.asarray(orig, np.float32)[sl],
                         extra=None if extra is None else np.asarray(extra, np.float32)[ty:ty + th, tx:tx + tw],
                         grey=grey, neon=neon, flag=flag)
        out[:, ty + ly:ty + ly + ch, tx + lx:tx + lx + cw] = o[:, ly:ly + ch, lx:lx + cw]
    return out
