"""AutoFineTune (FUN_009a775c) の移植 (wp_aft, 2026-10-17)。

解読の根拠は ../notes/AutoFineTune.md (命令単位)。本家の AFT 全体は入口に保護判定があり
ハーネスで実行できないため、本移植は「正しいトークン表なら本体が走る (加算 0)」仮定の
本体のみ。本家との数値照合は未実施 (REPORT.md)。

再利用部品 (いずれも本家とビット一致が確認済み):
  nv_band.quantile   = FUN_00997bcc (分位点)      nv_band.mean_exact = FUN_009ef3fc (平均)
  nv_curve.is_clip_genuine = FUN_009c6f88 (IsClip) nv_curve._pow      = libSystem の _pow
  nv_curve._cramer   = FUN_009d41b0 (クラメル)     nv_curve.polyval_genuine = FUN_009d4890
  nv_model.model_full = FUN_00995250 (1 面モデル。w5=levels, w6=set — harness nvh.c で確認)
  nv_profile.curve_bin = AFT の輝度ビン計算 (fcvtzs 飽和)

使い方:
  auto_fine_tune(rgb (H, W, 3) float32, profile_bytes (0xc300), mode=0, image2=None, cancel=None)
      -> bytes (0xc300)。書き換えるのは 3 つの曲線領域 (0xbf44/0xc064/0xc184) とバイト 0x2d のみ
  auto_fine_tune_fast(...)  同じ結果 (自己一致試験は verify/verify_self.py)
  auto_fine_tune_mode2(...) モード 2: (bytes, (R, G, B の平均比)) — FUN_009b5408 の戻り値
"""
import os
import sys

import numpy as np

sys.dont_write_bytecode = True
_HERE = os.path.dirname(os.path.abspath(__file__))
_BASE = os.environ.get("NV_AFT_BASE") or os.path.abspath(os.path.join(_HERE, "..", "..", "..", "..", "..", "reference", "python"))
for _p in (_BASE,):
    if _p not in sys.path:
        sys.path.insert(0, _p)
import nv_band as nb            # noqa: E402
import nv_curve as ncv          # noqa: E402
import nv_model as nm           # noqa: E402
import nv_profile as npr        # noqa: E402

F32 = np.float32
PROF = 0xc300
MODEL = 0x1f54
FACE = (0x4c, 0x1fa0, 0x3ef4)                 # プロファイル面 R, G, B (BuildNoiseModel と同じ並び)
STATS = (0xbc44, 0xbd10, 0xbddc)              # tile_dev (FUN_009b2f34) の許容 51 語
CURVE = (0xbf44, 0xc064, 0xc184)              # 曲線 3ch × 9 ビン × 0x20 B
TEN4 = F32(10000.0)
S_MULT = (F32(4.0), F32(3.0), F32(4.0))       # DAT_012477d0[mode 0/1/2] (実測)
S_LVL = (F32(3.0), F32(2.0), F32(3.0))        # DAT_012477dc[mode 0/1/2] (実測)
NDIR = 8                                      # DAT_01244098 (実測)
Q66 = F32(0.66)                               # 0x3f28f5c3
COEF1 = F32(1.0)                              # 段係数 5 語の値: memset_pattern16 の模様 0xc962e0 = 0000803f x4 (実測)
CLIP = F32(0.33)
HALF = F32(0.5)
THREE = F32(3.0)
NEG_C = F32(-0.25)                            # FUN_009d49b4 の c


# --------------------------------------------------------------------- 基本演算
def _penalty(x, s):
    """FUN_009d49b4(x, s, c=-0.25f) (fn_0x9d49b4.s 確定)。

    s == 0.0f (順序付き。NaN は下へ): t = x*(-0.25f) を返し、t > 0.0f (gt、NaN→偽) なら 1.0f。
    main: v = ((0.5f/s)/c)*x + ((c+(-1.0f))*0.5f)/c (= -8x/s + 2.5)。
    v < 0 (b.mi、NaN→不採択) → 0.0f。v > 1.0f → 1.0f。それ以外 (v <= 1 または NaN) → v。
    """
    x = F32(x)
    s = F32(s)
    if s == F32(0.0):
        t = F32(x * NEG_C)
        return F32(1.0) if t > F32(0.0) else F32(0.0)
    a = F32(F32(F32(F32(0.5) / s) / NEG_C) * x)
    b = F32(F32(F32(NEG_C + F32(-1.0)) * F32(0.5)) / NEG_C)
    v = F32(a + b)
    if v < F32(0.0):
        return F32(0.0)
    if v > F32(1.0):
        return F32(1.0)
    return v


def _dev(x, y):
    """FUN_009b6338 内のずれ: |x| <= |y| (b.le、unordered も成立扱い) ? |y|/|x| : |x|/|y| − 1.0f。"""
    ax, ay = F32(abs(x)), F32(abs(y))
    with np.errstate(all="ignore"):
        r = F32(ax / ay) if ax > ay else F32(ay / ax)
        return F32(r + F32(-1.0))


def _ratio_dev(a, b):
    """a == 0.0f (順序付き) または b == 0.0f → 10000.0f、それ以外は _dev(a, b)。"""
    if a == F32(0.0) or b == F32(0.0):
        return TEN4
    return _dev(a, b)


def _cl(v, lo, hi):
    """FUN_009b6338 系の格納 clamp: t = (v <= hi) ? v : hi、(lo <= v) ? t : lo (NaN → lo)。"""
    t = v if v <= hi else hi
    return t if lo <= v else lo


# ---------------------------------------------------------------- 類似度 FUN_009b6338
def similarity(t, p, stats, s_lvl, s_mult):
    """FUN_009b6338 (s0=s_lvl, s1=s_mult, x2=tile_model, x3=profile_model, x4=stats51)。

    t/p: FUN_00995250 の 4096 語 (語 0..54 = mean[5], q99[5], q50[5], cv[5,8])。
    stats: 51 語 ([0]=帯域比、[1+i]/[0x11+i]=方向 L0/L1、[34+8l+m]=CV、[0x31+l]=分位点)。
    積の順序は asm のとおり。
    """
    t = np.asarray(t, F32)
    p = np.asarray(p, F32)
    stats = np.asarray(stats, F32)
    s_mult = F32(s_mult)
    with np.errstate(all="ignore"):
        prod = F32(_penalty(F32(abs(F32(t[5] / p[5]))), s_lvl)
                   * _penalty(F32(abs(F32(t[6] / p[6]))), s_lvl))
        prod = F32(prod * _penalty(_ratio_dev(F32(t[6] / t[5]), F32(p[6] / p[5])),
                                   F32(stats[0] * s_mult)))
        for i in range(NDIR):
            a0 = F32(t[240 + i] / p[240 + i])          # 方向 A 応答 L0 (byte 0x3c0+i*4)
            b0 = F32(t[490] / p[490])                  # 残差 R L0 (byte 0x7a8 — 語 490。2026-10-17 訂正:
            a1 = F32(t[256 + i] / p[256 + i])          # 旧 474 は 0x768 = 組 4 の方向語で未初期化)
            b1 = F32(t[491] / p[491])                  # L1 (byte 0x7ac — 語 491)
            prod = F32(prod * _penalty(_ratio_dev(a0, b0), F32(stats[1 + i] * s_mult)))
            prod = F32(prod * _penalty(_ratio_dev(a1, b1), F32(stats[0x11 + i] * s_mult)))
        for l in (0, 1):
            base = 16 + 8 * l                          # cv[l][1..7] (語 15+8l は使わない)
            sb = 34 + 8 * l
            for m in range(7):
                wt, wp = F32(t[base + m]), F32(p[base + m])
                # 次語の検査: 通常は b.mi で終了、許容 0 の直後は b.pl で続行 (0x9b65fc・0x9b668c)。
                # どちらも「wt < 0 (順序付き) なら終了、NaN は続行」で同じ (2026-10-18 訂正)
                if wt < F32(0.0):
                    break
                if wp < F32(0.0):                      # b.mi (NaN は続行)
                    break
                tol = F32(stats[sb + m])
                if tol != F32(0.0):                    # b.ne / b.eq (NaN は罰則を掛ける側)
                    prod = F32(prod * _penalty(_ratio_dev(wt, wp), F32(tol * s_mult)))
            qa = F32(t[l] / t[5 + l])                  # mean[l]/q99[l]
            qb = F32(p[l] / p[5 + l])
            prod = F32(prod * _penalty(_ratio_dev(qa, qb), F32(stats[0x31 + l] * s_mult)))
        return F32(prod)


# ------------------------------------------------------- 当てはめ FUN_009d3e8c
def polyfit_aft(xs, ys, npts, ncoef):
    """FUN_009d3e8c(xs float*, ys float*, npts, out double*, ncoef) (asm 確定)。

    xs は float32 (fcvt で double へ)。b[i] = Σ_p term(xs[p], i)*(double)ys[p]、
    A[i][j] = Σ_p term(xs[p], i+j)。term(x, e) = _pow((double)x, (double)(int)e)。
    i == 0 / i+j == 0 のループだけ「x == 0.0f (fcmp eq) なら 1.0」の fcsel がある。
    累積は p 昇順・double・融合なし。求解は FUN_009d41b0 = nv_curve._cramer。
    ncoef < 1 は本家が未初期化行列でクラメルを呼ぶ到達不能経路 (ここでは ValueError)。
    """
    if ncoef < 1:
        raise ValueError("FUN_009d3e8c: ncoef < 1 reaches the genuine only with uninitialized matrices")
    xs = [F32(v) for v in xs[:npts]]
    ys = [F32(v) for v in ys[:npts]]
    b = [0.0] * ncoef
    a = [[0.0] * ncoef for _ in range(ncoef)]
    for i in range(ncoef):
        s = 0.0
        if npts >= 1:
            if i == 0:
                for k in range(npts):
                    x = xs[k]
                    term = 1.0 if x == F32(0.0) else ncv._pow(float(x), 0.0)
                    s = s + term * float(ys[k])
            else:
                for k in range(npts):
                    s = s + ncv._pow(float(xs[k]), float(i)) * float(ys[k])
        b[i] = s
        for j in range(ncoef):
            s2 = 0.0
            e = i + j
            for k in range(npts):
                x = xs[k]
                if e == 0:
                    term = 1.0 if x == F32(0.0) else ncv._pow(float(x), 0.0)
                    s2 = s2 + term
                else:
                    s2 = s2 + ncv._pow(float(x), float(e))
            a[i][j] = s2
    return ncv._cramer(a, b, ncoef)


def polyval_aft(x, coeffs, ncoef):
    """FUN_009d4890 = nv_curve.polyval_genuine (Σ pow(x, i)*c[i] を i 昇順 double)。"""
    return ncv.polyval_genuine(float(x), coeffs, ncoef)


# ------------------------------------------------------ モード 2 の順序 FUN_009d39ec
def shuffle_order(n):
    """FUN_009d39ec(out, n): mask = 2^bit_length(n)-1、s=0x7b の LCG で Fisher-Yates 型交換。

    s の更新: t = s + (s>>16); u = t + (t<<8) + (s>>24); s' = (u<<16) | (s>>16) (32 ビット)。
    j = (s & mask) % n (sdiv/msub、非負)。n == 0 は何もしない。
    """
    if n == 0:
        return []
    mask = (1 << n.bit_length()) - 1
    out = list(range(n))
    s = 0x7b
    for i in range(n):
        j = (s & mask) % n
        out[i], out[j] = out[j], out[i]
        t = (s + (s >> 16)) & 0xffffffff
        u = (t + ((t << 8) & 0xffffffff)) & 0xffffffff
        u = (u + (s >> 24)) & 0xffffffff
        s = (((u << 16) & 0xffffffff) | (s >> 16)) & 0xffffffff
    return out


# ------------------------------------------------------------------ プロファイル视图
class Prof(object):
    """0xc300 バイトのプロファイル (書き換え可能)。byte/uint32/float32/int32 の共用视图。"""

    __slots__ = ("buf", "b", "u", "f", "i")

    def __init__(self, buf):
        if len(buf) != PROF:
            raise ValueError(f"profile must be {PROF} bytes (got {len(buf)})")
        self.buf = buf
        a = np.frombuffer(buf, np.uint8)
        self.b = a
        self.u = a.view(np.uint32)
        self.f = a.view(F32)
        self.i = a.view(np.int32)

    def curve_base(self, ch):
        """ch の曲線領域: ビン k の語 index = base + 8k (level=+0, coefs=+1..5, count=int at +6)。"""
        return CURVE[ch] >> 2


# ------------------------------------------------------ 曲線化 FUN_009b4e08
def curve_post(bins, prof, ch, mode, quantiles=None):
    """FUN_009b4e08 (位相 1〜3)。bins: 9 個の float32 リスト。prof を書き換える。

    thr = 4 (mode 0) / 8 (mode != 0)。位相 2 はフラグ付きのビン 0..8 を書く
    (0x9b5238 `cmp x9,#0x100` は本体の後。2026-10-18 訂正: 旧版はビン 8 を書かなかった)。
    段係数 5 語は 1.0f (memset_pattern16 の模様 0xc962e0。0x9b5108 と 0x9b52d8)。
    位相 3 は ratio_n > 0 のときだけ走り、本体は「ビン 0 が非フラグ」または
    「直前のビンが非フラグ」のビンに実行される (asm 0x9b5294-0x9b5308 の走査)。
    quantiles: 計算済みの分位点 (9 個。高速版から渡す。None ならここで計算)。
    """
    f, i, bb = prof.f, prof.i, prof.b
    base0 = prof.curve_base(ch)
    thr = 4 if mode == 0 else 8
    flags = [len(bins[k]) >= thr for k in range(9)]
    if quantiles is None:
        quantiles = [nb.quantile(np.asarray(bins[k], F32), Q66, 0, 1) if flags[k] else None
                     for k in range(9)]
    q = quantiles
    ratio_sum = F32(0.0)
    ratio_n = 0
    with np.errstate(all="ignore"):
        for k in range(9):                                    # 位相 2: ビン 0..8
            if not flags[k]:
                continue
            wb = base0 + 8 * k
            o = CURVE[ch] + 0x20 * k
            if i[wb + 6] > 0:                                 # 古い count (書く前に読む)
                ratio_sum = F32(ratio_sum + F32(q[k] / f[wb]))
                ratio_n += 1
            f[wb] = q[k]
            f[wb + 1:wb + 6] = COEF1                          # 段係数 5 つ = 1.0f (0x9b521c/0x9b5220)
            i[wb + 6] = 1
            bb[o + 0x1c] = 0
            bb[o + 0x1d] = 0
            bb[o + 0x1e] = 1 if THREE < q[k] else 0
        if ratio_n == 0:
            return
        avg = F32(ratio_sum / F32(ratio_n))

        def body(k):
            wb = base0 + 8 * k
            o = CURVE[ch] + 0x20 * k
            if i[wb + 6] > 0 or bb[o + 0x1c] != 0:
                f[wb] = F32(avg * f[wb])
                f[wb + 1:wb + 6] = COEF1                      # memset_pattern16 (0x9b52d8)
                i[wb + 6] = 0
                bb[o + 0x1c] = 1
                bb[o + 0x1e] = 1 if THREE < f[wb] else 0

        # 走査 (decompile 0x9b529c-0x9b5308): x20 = &local_b4 = flag[1] のスロット —
        # [x20, x22] = flag[x22+1]。本体は非フラグのビンそれぞれに実行される
        # (ビン 0 は入口の bVar1、ビン 1..8 はスキップループ)。lVar7 == 8 で終了。
        # フラグ付き (新規分位点あり) のビンは書き換えられない。
        if not flags[0]:
            body(0)
        k = 0
        while k != 8:
            k += 1
            if not flags[k]:
                body(k)


# ------------------------------------------------------ 補間 FUN_009bdf30
def curve_fill(lo, hi, prof, ch, flag=0):
    """FUN_009bdf30(lo, hi, _, curve, flag)。結び目 = count != 0 かつ +0x1d/+0x1e/+0x1c == 0 のビン。

    knot 1 個 → clamp(ys[0]) を全ビンへ。knot >= 2 → 1 本の多項式 (ncoef = min(3, nknot)) で
    first+1..last-1 を埋め、下側 (first > 0) は first < 3 ? 2 : 1 個、上側 (last < 8) は
    last > 5 ? 2 : 1 個の結び点で外挿。書込み条件は flag == 0 なら (count==0 or 旗) and +0x1c==0、
    flag != 0 なら (count==0 or 旗)。評価は FUN_009d4890 (double) → float32 → clamp。
    knot 0 個 → 何もしない。
    """
    lo = F32(lo)
    hi = F32(hi)
    f, i, bb = prof.f, prof.i, prof.b
    base0 = prof.curve_base(ch)

    def knot_ok(k):
        wb = base0 + 8 * k
        o = CURVE[ch] + 0x20 * k
        return i[wb + 6] != 0 and bb[o + 0x1d] == 0 and bb[o + 0x1e] == 0 and bb[o + 0x1c] == 0

    xs, ys = [], []
    first, last = -1, -1
    for k in range(9):
        if knot_ok(k):
            if first < 0:
                first = k
            xs.append(F32(k))
            ys.append(f[base0 + 8 * k])
            last = k
    n = len(xs)
    if n == 0:
        return

    def fillable(k):
        wb = base0 + 8 * k
        o = CURVE[ch] + 0x20 * k
        if flag & 1:
            return i[wb + 6] == 0 or bb[o + 0x1d] != 0 or bb[o + 0x1e] != 0 or bb[o + 0x1c] != 0
        c = i[wb + 6] == 0 or bb[o + 0x1d] != 0 or bb[o + 0x1e] != 0 or bb[o + 0x1c] != 0
        return c and bb[o + 0x1c] == 0

    def write(k, v):
        wb = base0 + 8 * k
        o = CURVE[ch] + 0x20 * k
        f[wb] = v
        f[wb + 1:wb + 6] = COEF1                              # memset_pattern16 (9bdf30 の 8 か所、模様 0xc962e0)
        i[wb + 6] = 0
        bb[o + 0x1c] = 1
        bb[o + 0x1e] = 1 if THREE < v else 0

    with np.errstate(all="ignore"):
        if n == 1:
            v = _cl(ys[0], lo, hi)
            for k in range(9):
                if fillable(k):
                    write(k, v)
            return
        ncoef = 3 if n > 2 else 2
        c = polyfit_aft(xs, ys, n, ncoef)
        for k in range(first + 1, last):
            if fillable(k):
                v = _cl(F32(polyval_aft(k, c, ncoef)), lo, hi)
                write(k, v)
        if first > 0:
            n2 = 2 if first < 3 else 1
            c2 = polyfit_aft(xs, ys, n2, n2)
            for k in range(0, first):
                if fillable(k):
                    v = _cl(F32(polyval_aft(k, c2, n2)), lo, hi)
                    write(k, v)
        if last < 8:
            n2 = 2 if last > 5 else 1
            c3 = polyfit_aft(xs[n - n2:], ys[n - n2:], n2, n2)
            for k in range(last + 1, 9):
                if fillable(k):
                    v = _cl(F32(polyval_aft(k, c3, n2)), lo, hi)
                    write(k, v)


# ------------------------------------------------ モード 2 の後処理 FUN_009b5408
def mode2_post(bins, prof, ch):
    """FUN_009b5408 → float: level != 0 かつ count >= 9 のビンで q66/level を足し、
    n < 2 なら -1.0f、それ以外は total/(float)n。"""
    f = prof.f
    base0 = prof.curve_base(ch)
    total = F32(0.0)
    n = 0
    with np.errstate(all="ignore"):
        for k in range(9):
            wb = base0 + 8 * k
            if f[wb] == F32(0.0) or len(bins[k]) < 9:
                continue
            qk = nb.quantile(np.asarray(bins[k], F32), Q66, 0, 1)
            total = F32(total + F32(qk / f[wb]))
            n += 1
        if n < 2:
            return F32(-1.0)
        return F32(total / F32(n))


# ------------------------------------------------------------------- worker 本体
def _tiles(rgb, mode):
    """親のタイル並び (FUN_009a775c)。行優先 (y 外側)。モード 2 は FUN_009d39ec の順でスロットへ。"""
    H, W = rgb.shape[:2]
    nx, ny = W // 32, H // 32                     # 切り捨て (端は処理しない)
    ntiles = nx * ny
    if ntiles == 0:
        return []
    if mode == 2:
        order = shuffle_order(ntiles)
        slots = [None] * ntiles
        i = 0
        for ty in range(ny):
            for tx in range(nx):
                slots[order[i]] = (32 * ty, 32 * tx)
                i += 1
        return slots
    return [(32 * ty, 32 * tx) for ty in range(ny) for tx in range(nx)]


def _run_tiles(rgb, prof, mode, image2, cancel, fast, diag):
    """FUN_009b56f4 (worker) の 1 スレッド動作。bins (3ch × 9) を返す。

    W/H < 48 → False (worker は何もせず 0 — 親の後処理も走らない)。
    cancel が真を返したら None (後処理を省く — 親の戻り値の扱いは推定、notes §8)。
    """
    H, W = rgb.shape[:2]
    if W < 48 or H < 48:
        return False
    slots = _tiles(rgb, mode)
    faces = [np.asarray(prof.f[o >> 2:(o + MODEL) >> 2], F32) for o in FACE]
    stats = [np.asarray(prof.f[(o >> 2):(o >> 2) + 51], F32) for o in STATS]
    fw0 = [F32(abs(faces[c][0])) for c in range(3)]
    s_lvl = S_LVL[mode]
    s_mult = S_MULT[mode]
    bins = [[[] for _ in range(9)] for _ in range(3)]
    ntiles = len(slots)
    if diag is not None:
        diag["ntiles"] = ntiles
    for s in range(ntiles):
        if cancel is not None and cancel():
            return None
        y, x = slots[s]
        t = rgb[y:y + 32, x:x + 32]
        R = np.ascontiguousarray(t[..., 0], F32)
        G = np.ascontiguousarray(t[..., 1], F32)
        B = np.ascontiguousarray(t[..., 2], F32)
        with np.errstate(all="ignore"):
            cR = ncv.is_clip_genuine(R)
            cG = ncv.is_clip_genuine(G)
            cB = ncv.is_clip_genuine(B)
            if cR > CLIP or cG > CLIP or cB > CLIP:            # b.gt (NaN は素通り)
                if diag is not None:
                    diag["clip_skip"] = diag.get("clip_skip", 0) + 1
                continue
            if fast:
                m = nb.mean_bm(np.concatenate((R.reshape(-1, 1), G.reshape(-1, 1),
                                               B.reshape(-1, 1)), axis=1))
                mR, mG, mB = m[0], m[1], m[2]
            else:
                mR = nb.mean_exact(R)
                mG = nb.mean_exact(G)
                mB = nb.mean_exact(B)
            bR = npr.curve_bin(mR)
            bG = npr.curve_bin(mG)
            bB = npr.curve_bin(mB)
            if (mode == 2 and len(bins[0][bR]) >= 32 and len(bins[1][bG]) >= 32
                    and len(bins[2][bB]) >= 32):
                continue
            mGm = nm.model_full(G, 2, None, 2).view(F32)   # model_full は U32 ビット列を返す
            simG = similarity(mGm, faces[1], stats[1], s_lvl, s_mult)
            if simG < HALF:                                     # b.mi (NaN は素通り)
                if diag is not None:
                    diag["simG_skip"] = diag.get("simG_skip", 0) + 1
                continue
            mRm = nm.model_full(R, 2, None, 2).view(F32)
            simR = similarity(mRm, faces[0], stats[0], s_lvl, s_mult)
            prod = F32(simG * simR)
            if prod < HALF:
                if diag is not None:
                    diag["simR_skip"] = diag.get("simR_skip", 0) + 1
                continue
            mBm = nm.model_full(B, 2, None, 2).view(F32)
            simB = similarity(mBm, faces[2], stats[2], s_lvl, s_mult)
            prod = F32(F32(simG * simR) * simB)
            if prod < HALF:
                if diag is not None:
                    diag["simB_skip"] = diag.get("simB_skip", 0) + 1
                continue
            if image2 is not None:
                t2 = image2[y:y + 32, x:x + 32]
                dm = []
                ok = True
                for c in range(3):
                    d = F32(np.asarray(t2[..., c], F32) - (R if c == 0 else G if c == 1 else B))
                    dm.append(nm.model_full(np.ascontiguousarray(d), 0, None, 2).view(F32))
                for c, w in ((0, 5), (1, 5), (2, 5), (0, 6), (1, 6), (2, 6)):
                    mm = (mRm, mGm, mBm)[c]
                    a = F32(abs(dm[c][w]))                       # |diff|
                    b_ = F32(abs(mm[w]))                         # |tile|
                    if a < b_ or a > F32(b_ + b_):               # b.mi / b.gt (NaN は棄却しない)
                        ok = False
                        break
                if not ok:
                    if diag is not None:
                        diag["diff_skip"] = diag.get("diff_skip", 0) + 1
                    continue
            if prod > HALF:                                      # 厳密 (b.le は NaN を拾う)
                bins[0][bR].append(F32(F32(abs(mRm[0])) / fw0[0]))
                bins[1][bG].append(F32(F32(abs(mGm[0])) / fw0[1]))
                bins[2][bB].append(F32(F32(abs(mBm[0])) / fw0[2]))
                if diag is not None:
                    diag["accepted"] = diag.get("accepted", 0) + 1
    return bins


# ------------------------------------------------------------------- 親 FUN_009a775c
def _finish(prof, bins, mode):
    """親の後処理 (成功時のみ): 9b4e08 ×3 → 9bdf30 ×3 (mode < 2)、9b5408 ×3 (mode 2)。"""
    if mode < 2:
        for c in range(3):
            curve_post(bins[c], prof, c, mode)
        for c in range(3):
            curve_fill(F32(0.2), F32(3.0), prof, c, 0)
        return None
    return [mode2_post(bins[c], prof, c) for c in range(3)]


def auto_fine_tune(rgb, profile, mode=0, image2=None, cancel=None, diag=None):
    """FUN_009a775c 本体 (保護判定は通過と仮定 — notes/AutoFineTune.md §8)。

    rgb: (H, W, 3) float32 0..255。profile: 0xc300 バイト。戻り値: bytes (0xc300)。
    書き換えるのは曲線 3 領域とバイト 0x2d のみ。W/H < 48 は worker が何もせず 0 を
    返す経路 (曲線はそのまま、0x2d = 0 のみ)。cancel が途中で真を返したら曲線は
    書かない (親の後処理の扱いは推定)。
    """
    rgb = np.asarray(rgb, F32)
    buf = bytearray(profile)
    prof = Prof(buf)
    bins = _run_tiles(rgb, prof, mode, image2, cancel, False, diag)
    outs = None
    if bins not in (False, None):
        outs = _finish(prof, bins, mode)
    prof.b[0x2d] = 0
    if mode == 2 and outs is not None:
        return bytes(buf), tuple(F32(v) for v in outs)
    return bytes(buf)


def auto_fine_tune_fast(rgb, profile, mode=0, image2=None, cancel=None, diag=None):
    """高速版: 3 面の平均を 1 度に (nb.mean_bm は列ごとに本家とビット一致の検証済み)。
    結果は auto_fine_tune と同じ (verify/verify_self.py でバイト一致を確認)。"""
    rgb = np.asarray(rgb, F32)
    buf = bytearray(profile)
    prof = Prof(buf)
    bins = _run_tiles(rgb, prof, mode, image2, cancel, True, diag)
    outs = None
    if bins not in (False, None):
        outs = _finish(prof, bins, mode)
    prof.b[0x2d] = 0
    if mode == 2 and outs is not None:
        return bytes(buf), tuple(F32(v) for v in outs)
    return bytes(buf)


def auto_fine_tune_mode2(rgb, profile, image2=None, cancel=None, diag=None):
    """モード 2 (FUN_009b5408 の戻り値 3 個を返す)。"""
    return auto_fine_tune(rgb, profile, 2, image2, cancel, diag)
