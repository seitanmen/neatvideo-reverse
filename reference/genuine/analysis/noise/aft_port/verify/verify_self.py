"""自己一致・構造試験 (wp_aft)。COMMON §7: 誤り注入で「空振りしない」ことの確認を含む。

使い方: cd work/omo/wp_aft/verify && PYTHONDONTWRITEBYTECODE=1 python3 verify_self.py [--quick]
環境変数 NV_AFT_SRC=<dir> で nv_aft.py の場所を差し替える (誤り注入の試験用)。

定数・ゲート閾値・段係数の期待値は逆アセンブル (/tmp/nv_expand/disasm_full.txt) と arm64 スライスの
__TEXT (fileoff = vmaddr) の静的読みに由来する (2026-10-18、notes/AutoFineTune.md):
  S_LVL = DAT_012477dc (3,2,3)、S_MULT = DAT_012477d0 (4,3,4)、NDIR = DAT_01244098 = 8、
  Q66 = 0x3f28f5c3 (0x9b4e7c)、IsClip 閾値 0x3ea8f5c3、ゲート 0.5 (worker 0x9b5d70 他)、
  段係数 = memset_pattern16 の模様 0xc962e0 = 0000803f x4 (1.0f)、位相 2 はビン 0..8 (0x9b5238)。
"""
import json
import os
import sys

import numpy as np

sys.dont_write_bytecode = True
_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.environ.get("NV_AFT_SRC") or os.path.join(os.path.dirname(_HERE), "src"))
sys.path.insert(0, os.environ.get("NV_AFT_BASE") or os.path.abspath(os.path.join(_HERE, "..", "..", "..", "..", "..", "reference", "python")))
import nv_aft as na            # noqa: E402
import nv_band as nb           # noqa: E402
import nv_curve as ncv         # noqa: E402
import nv_profile as npr       # noqa: E402

F32 = np.float32
FAILED = []


def check(name, cond, detail=""):
    print(("PASS" if cond else "FAIL"), name, detail if not cond else "")
    if not cond:
        FAILED.append(name)


def frame(h, w, rng, kind):
    if kind == "noise":
        return rng.uniform(0, 255, (h, w, 3)).astype(F32)
    if kind == "flat":
        v = rng.uniform(40, 200, 3).astype(F32)
        return np.tile(v, (h, w, 1)).astype(F32) + rng.normal(0, 2.0, (h, w, 3)).astype(F32)
    if kind == "clip":
        f = rng.uniform(0, 255, (h, w, 3)).astype(F32)
        return np.clip(f, None, 3.0).astype(F32)     # 大半が 0..3 → 分布の端
    if kind == "grad":
        y, x = np.mgrid[0:h, 0:w]
        g = np.stack([(20 * x / w).astype(F32), (40 * y / h).astype(F32),
                      np.full((h, w), 120.0, F32)], -1)
        return (g + rng.normal(0, 3.0, (h, w, 3))).astype(F32)
    raise ValueError(kind)


def bits32(v):
    return np.asarray(v, F32).view(np.uint32)


# --------------------------------------------------------------------- 1. 部品
def t_penalty():
    # s==0 分岐: x*(-0.25) > 0 (= x < 0) → 1
    check("pen s=0 x<0", na._penalty(F32(-1.0), F32(0.0)) == F32(1.0))
    check("pen s=0 x>0", na._penalty(F32(1.0), F32(0.0)) == F32(0.0))
    check("pen s=0 x=0", na._penalty(F32(0.0), F32(0.0)) == F32(0.0))
    # main: v = -2x/s + 2.5 (c=-0.25): x=0 → v=2.5 → 1。丁度 v=1 は x=0.75s、v=0 は x=1.25s
    check("pen x=0 s=3", na._penalty(F32(0.0), F32(3.0)) == F32(1.0))
    check("pen x=0.75s", na._penalty(F32(3.0), F32(4.0)) == F32(1.0))       # v=1.0 (丁度)
    check("pen x=1.25s", na._penalty(F32(5.0), F32(4.0)) == F32(0.0))       # v=0.0 (丁度)
    check("pen x=s", na._penalty(F32(4.0), F32(4.0)) == F32(0.5))          # v=1.5 → 1? いいえ 2.5-2=0.5
    check("pen x=5s/8", na._penalty(F32(2.5), F32(4.0)) == F32(1.0))      # v=1.25 → 1
    check("pen NaN x", np.isnan(na._penalty(F32(np.nan), F32(3.0))))        # NaN は本値を返す (asm)
    check("pen NaN s", np.isnan(na._penalty(F32(1.0), F32(np.nan))))


def t_dev():
    check("dev same", na._dev(F32(1.0), F32(1.0)) == F32(0.0))
    check("dev 2:1", na._dev(F32(2.0), F32(1.0)) == F32(1.0))
    check("dev unordered", np.isnan(na._dev(F32(np.nan), F32(1.0))))       # |a|<=|b| 成立扱い → 1/NaN
    check("ratio_dev zero", na._ratio_dev(F32(0.0), F32(5.0)) == F32(10000.0))
    check("ratio_dev NaN", np.isnan(na._ratio_dev(F32(np.nan), F32(5.0))))


def t_cl():
    check("cl mid", na._cl(F32(1.0), F32(0.2), F32(3.0)) == F32(1.0))
    check("cl hi", na._cl(F32(9.0), F32(0.2), F32(3.0)) == F32(3.0))
    check("cl lo", na._cl(F32(0.01), F32(0.2), F32(3.0)) == F32(0.2))
    check("cl NaN", na._cl(F32(np.nan), F32(0.2), F32(3.0)) == F32(0.2))


def t_shuffle():
    # n=0..3: 0 は即 return。LCG の手計算: s0=0x7b, mask(n=3)=7 → j=(123&7)%3=3%3=0 → swap(0,0)
    check("shuffle n=0", na.shuffle_order(0) == [])
    check("shuffle n=1", na.shuffle_order(1) == [0])
    o2 = na.shuffle_order(2)
    check("shuffle n=2 perm", sorted(o2) == [0, 1])
    o3 = na.shuffle_order(3)
    check("shuffle n=3 perm", sorted(o3) == [0, 1, 2])
    # 手計算 (mask=3, s0=0x7b): i=0 j=(0x7b&3)%3=0; i=1 j=(0x7b7b0000&3)%3=0 → swap(1,0);
    # i=2 j=(0xf6f67b7b&3)%3=0 → swap(2,0) → [2,0,1]
    check("shuffle n=3 trace", o3 == [2, 0, 1], str(o3))
    o10 = na.shuffle_order(10)
    check("shuffle n=10 perm", sorted(o10) == list(range(10)))


def t_polyfit():
    # 整数 xs では nv_curve.polyfit_genuine (FUN_009d43f4, double 入力) と同じ結果になる
    for npts, ncoef in ((2, 2), (3, 2), (3, 3), (4, 3), (9, 3)):
        xs = np.arange(npts, dtype=F32)
        ys = (1.0 + 0.37 * xs + 0.011 * xs * xs).astype(F32)
        a = na.polyfit_aft(xs, ys, npts, ncoef)
        b = ncv.polyfit_genuine([float(v) for v in xs], [float(v) for v in ys], npts, ncoef)
        ok = all(x == y for x, y in zip(a, b))
        check(f"polyfit {npts}/{ncoef} == genuine", ok, f"{a} vs {b}")
    # 線形データで y = x が復元される (1,0,0)
    xs = np.array([0, 1, 2, 3], F32)
    ys = np.array([0, 1, 2, 3], F32)
    c = na.polyfit_aft(xs, ys, 4, 3)
    vals = [ncv.polyval_genuine(float(k), c, 3) for k in range(4)]
    check("polyfit linear", all(abs(v - k) < 1e-9 for v, k in zip(vals, range(4))))


# ---------------------------------------------------------------- 2. 曲線部品
def _curve_state(levels, counts=None, b1c=None, b1d=None, b1e=None, coef=1.0):
    """テスト用の 1ch 曲線領域を持つ最小プロファイル (他は 0)。"""
    buf = bytearray(0xc300)
    prof = na.Prof(buf)
    base0 = prof.curve_base(0)
    for k in range(9):
        wb = base0 + 8 * k
        o = na.CURVE[0] + 0x20 * k
        prof.f[wb] = F32(levels[k])
        for j in range(1, 6):
            prof.f[wb + j] = F32(coef)
        prof.i[wb + 6] = 0 if counts is None else counts[k]
        if b1c:
            prof.b[o + 0x1c] = b1c[k]
        if b1d:
            prof.b[o + 0x1d] = b1d[k]
        if b1e:
            prof.b[o + 0x1e] = b1e[k]
    return prof


def t_curve_fill():
    # knot 1 個 (bin4 の level=2.0): 全ビン clamp(2.0) = 2.0、+0x1c=1
    prof = _curve_state([1.0, 1.0, 1.0, 1.0, 2.0, 1.0, 1.0, 1.0, 1.0], counts=[0, 0, 0, 0, 1, 0, 0, 0, 0])
    na.curve_fill(F32(0.2), F32(3.0), prof, 0, 0)
    lv = [float(prof.f[prof.curve_base(0) + 8 * k]) for k in range(9)]
    check("fill singleton", lv == [2.0] * 9, str(lv))
    o4 = na.CURVE[0] + 0x20 * 4
    check("fill singleton knot untouched", prof.b[o4 + 0x1c] == 0)
    check("fill singleton flag", prof.b[na.CURVE[0] + 0x1c] == 1)

    # knot 2 個 (bin2=1.0, bin6=2.0): 中間は直線、下は 2 点外挿、上は last<=5 → 1 点
    prof = _curve_state([1.0] * 9, counts=[0, 0, 1, 0, 0, 0, 1, 0, 0])
    prof.f[prof.curve_base(0) + 8 * 2] = F32(1.0)
    prof.f[prof.curve_base(0) + 8 * 6] = F32(2.0)
    na.curve_fill(F32(0.2), F32(3.0), prof, 0, 0)
    lv = [float(prof.f[prof.curve_base(0) + 8 * k]) for k in range(9)]
    # (2,1) と (6,2) を通る直線 y = 0.5 + x/4 の外挿・内挿
    exp = [0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0, 2.25, 2.5]
    check("fill 2 knots", all(abs(a - b) < 1e-6 for a, b in zip(lv, exp)), str(lv))

    # knot 9 個 → 何も書かない (全ビンが結び目)
    prof = _curve_state([1.0 + k for k in range(9)], counts=[1] * 9)
    na.curve_fill(F32(0.2), F32(3.0), prof, 0, 0)
    lv = [float(prof.f[prof.curve_base(0) + 8 * k]) for k in range(9)]
    check("fill all knots", lv == [1.0 + k for k in range(9)])

    # knot 0 個 → 何もしない
    prof = _curve_state([5.0] * 9)
    na.curve_fill(F32(0.2), F32(3.0), prof, 0, 0)
    check("fill no knots", float(prof.f[prof.curve_base(0)]) == 5.0)

    # +0x1e のビン (level>3) は結び目にならない
    prof = _curve_state([1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0], counts=[1] * 9,
                        b1e=[0, 0, 0, 0, 0, 0, 0, 0, 1])
    na.curve_fill(F32(0.2), F32(3.0), prof, 0, 0)
    # bin8 は非結び目 → 上側外挿 (last=7 > 5 → 2 点: bin6,7 の直線)
    lv = [float(prof.f[prof.curve_base(0) + 8 * k]) for k in range(9)]
    check("fill flagged bin8", abs(lv[8] - 1.0) < 1e-6, str(lv))

    # 下側 1 点 (first=5 ≥ 3): bin0..4 は bin5 の値
    prof = _curve_state([0, 0, 0, 0, 0, 1.5, 0, 0, 0], counts=[0, 0, 0, 0, 0, 1, 0, 0, 0], coef=7.0)
    prof.f[prof.curve_base(0) + 8 * 5] = F32(1.5)
    na.curve_fill(F32(0.2), F32(3.0), prof, 0, 0)
    lv = [float(prof.f[prof.curve_base(0) + 8 * k]) for k in range(9)]
    check("fill coef = 1.0f", all(prof.f[prof.curve_base(0) + 8 * k + j] == F32(1.0)
                                  for k in (0, 1, 2, 3, 4, 6, 7, 8) for j in range(1, 6)))
    check("fill prefix 1pt", all(abs(lv[k] - 1.5) < 1e-6 for k in range(5)) and abs(lv[8] - 1.5) < 1e-6,
          str(lv))


def t_curve_post():
    # ビン 4 にだけ 4 標本 (mode 0): 位相 2 が level=q66、位相 3 は他ビンを avg*level で更新しない
    # (他ビンは count=0 かつ +0x1c=0 → 条件不成立 → そのまま → 9bdf30 が補間する)
    prof = _curve_state([1.0] * 9, counts=[0, 0, 0, 0, 0, 0, 0, 0, 0])
    bins = [[] for _ in range(9)]
    vals = [F32(v) for v in (0.8, 0.9, 1.0, 1.3)]
    bins[4] = vals
    na.curve_post(bins, prof, 0, 0)
    base0 = prof.curve_base(0)
    q = nb.quantile(np.asarray(vals, F32), F32(0.66), 0, 1)
    check("post level = q66", prof.f[base0 + 32] == F32(q), f"{prof.f[base0 + 32]} vs {q}")
    check("post count = 1", prof.i[base0 + 32 + 6] == 1)
    check("post others untouched", float(prof.f[base0]) == 1.0 and float(prof.f[base0 + 64]) == 1.0)

    # ratio の集計: bin3 (古い count=1, level=1.0) と bin5 (count=0) に標本 → ratio_sum に bin3 だけ
    prof = _curve_state([1.0] * 9, counts=[0, 0, 0, 1, 0, 0, 0, 0, 0])
    prof.f[base0 + 24] = F32(2.0)   # bin3 の古い level
    bins = [[] for _ in range(9)]
    bins[3] = [F32(1.0), F32(2.0), F32(3.0), F32(4.0)]     # q66 → ~3.0
    bins[5] = [F32(0.5), F32(0.6), F32(0.7), F32(0.8)]
    bins[0] = [F32(1.0)]                                    # 閾値未満 → フラグなし
    na.curve_post(bins, prof, 0, 0)
    q3 = nb.quantile(np.asarray(bins[3], F32), F32(0.66), 0, 1)
    avg = F32(F32(q3 / F32(2.0)) / F32(1))
    # 位相 3: body は bin0 (非フラグ) と bin4 (bin3 非フラグ? — bin3 はフラグ付き → bin4 は「直前」
    # bin3 がフラグ → スキップ). 非フラグは bin0,1,2,4,6,7,8 → body(0), body(1),... bin0 のみ count=0,
    # +0x1c=0 → 条件不成立で実質何も変わらない (全ビン count=0/flag0)。
    check("post phase2 bin3 level", prof.f[base0 + 24] == F32(q3))
    check("post phase2 bin5 level",
          prof.f[base0 + 40] == nb.quantile(np.asarray(bins[5], F32), F32(0.66), 0, 1))

    # ビン 8: フラグ付きなら位相 2 が書く (0x9b5238 の cmp x9,#0x100 は本体の後。2026-10-18 訂正)
    prof = _curve_state([1.0] * 9, counts=[0] * 9, coef=7.0)
    bins = [[] for _ in range(9)]
    bins[8] = [F32(2.0), F32(2.1), F32(2.2), F32(2.3)]
    na.curve_post(bins, prof, 0, 0)
    q8 = nb.quantile(np.asarray(bins[8], F32), F32(0.66), 0, 1)
    check("post bin8 written by phase 2", prof.i[base0 + 64 + 6] == 1 and prof.f[base0 + 64] == F32(q8),
          f"{prof.i[base0 + 70]} {prof.f[base0 + 64]} vs {q8}")
    check("post bin8 coef = 1.0f", all(prof.f[base0 + 64 + j] == F32(1.0) for j in range(1, 6)))
    check("post bin7 untouched", prof.i[base0 + 56 + 6] == 0 and float(prof.f[base0 + 56 + 1]) == 7.0)

    # mode 1 は閾値 8
    prof = _curve_state([1.0] * 9, counts=[0] * 9)
    bins = [[] for _ in range(9)]
    bins[2] = [F32(1.0)] * 7                                # 7 < 8 → フラグなし
    na.curve_post(bins, prof, 0, 1)
    check("post mode1 threshold", prof.i[base0 + 16 + 6] == 0)
    bins[2] = [F32(1.0)] * 8
    na.curve_post(bins, prof, 0, 1)
    check("post mode1 threshold 8", prof.i[base0 + 16 + 6] == 1)

    # 位相 3 の拡大縮小 (訂正後の走査): 非フラグで古い count>0 のビンは avg × level になる。
    # bin5 がフラグ (標本 4、古い count=1、level=1.0) → ratio = q5/1.0、avg = q5。
    # bin2 は非フラグ (標本 2) で古い count=1、level=2.0 → level = q5 × 2.0 に置き換わる
    prof = _curve_state([1.0] * 9, counts=[0, 0, 1, 0, 0, 1, 0, 0, 0])
    prof.f[base0 + 16] = F32(2.0)                            # bin2 の古い level
    bins = [[] for _ in range(9)]
    bins[2] = [F32(1.0), F32(1.1)]                           # 2 < 4 → 非フラグ
    bins[5] = [F32(1.2), F32(1.3), F32(1.4), F32(1.5)]       # フラグ
    na.curve_post(bins, prof, 0, 0)
    q5 = nb.quantile(np.asarray(bins[5], F32), F32(0.66), 0, 1)
    # 位相 2 が bin5 の level を q5 にしてから、位相 3 が bin2 を avg=q5 × 2.0 に
    check("post phase3 rescale bin2", prof.f[base0 + 16] == F32(F32(q5) * F32(2.0)),
          f"{prof.f[base0 + 16]} vs {F32(F32(q5) * F32(2.0))}")
    check("post phase3 flag bin2", prof.b[na.CURVE[0] + 0x20 * 2 + 0x1c] == 1)
    check("post phase3 flagged bin5 untouched by phase3", prof.i[base0 + 40 + 6] == 1)

    # 段係数 5 語 = 1.0f (memset_pattern16 の模様 0xc962e0)。番兵 7.0 から書き換わること
    prof = _curve_state([1.0] * 9, counts=[0, 0, 1, 0, 0, 1, 0, 0, 0], coef=7.0)
    prof.f[base0 + 16] = F32(2.0)
    na.curve_post(bins, prof, 0, 0)
    check("post phase2 coef = 1.0f (bin5)", all(prof.f[base0 + 40 + j] == F32(1.0) for j in range(1, 6)),
          str([float(prof.f[base0 + 40 + j]) for j in range(1, 6)]))
    check("post phase3 coef = 1.0f (bin2)", all(prof.f[base0 + 16 + j] == F32(1.0) for j in range(1, 6)),
          str([float(prof.f[base0 + 16 + j]) for j in range(1, 6)]))
    check("post untouched bin keeps coef", float(prof.f[base0 + 8 * 7 + 1]) == 7.0)


def t_constants():
    """逆アセンブル・静的データ由来の定数 (期待値は手で書いた値。モジュールの値と比べる)。"""
    check("const S_LVL", tuple(float(v) for v in na.S_LVL) == (3.0, 2.0, 3.0), str(na.S_LVL))
    check("const S_MULT", tuple(float(v) for v in na.S_MULT) == (4.0, 3.0, 4.0), str(na.S_MULT))
    check("const NDIR", na.NDIR == 8)
    check("const Q66 bits", int(bits32(na.Q66)) == 0x3f28f5c3, hex(int(bits32(na.Q66))))
    check("const CLIP bits", int(bits32(na.CLIP)) == 0x3ea8f5c3, hex(int(bits32(na.CLIP))))
    check("const gate 0.5", int(bits32(na.HALF)) == 0x3f000000)
    check("const COEF1 1.0f", int(bits32(na.COEF1)) == 0x3f800000)
    # 本家ファイル (読むだけ。__TEXT は fileoff = vmaddr) と照らす。無ければ省略
    binp = os.path.join(os.path.dirname(_HERE), "..", "..", "..", "NeatVideo6.arm64.ofx")
    if os.path.exists(binp):
        with open(binp, "rb") as fh:
            def rd(a, n):
                fh.seek(a)
                return fh.read(n)
            check("file DAT_012477dc == S_LVL", np.frombuffer(rd(0x12477dc, 12), "<f4").tolist()
                  == [float(v) for v in na.S_LVL])
            check("file DAT_012477d0 == S_MULT", np.frombuffer(rd(0x12477d0, 12), "<f4").tolist()
                  == [float(v) for v in na.S_MULT])
            check("file DAT_01244098 == NDIR", int(np.frombuffer(rd(0x1244098, 8), "<i8")[0]) == na.NDIR)
            check("file pattern 0xc962e0 == COEF1",
                  np.frombuffer(rd(0xc962e0, 16), "<f4").tolist() == [float(na.COEF1)] * 4)


def t_cv_nan():
    """CV 鎖: 許容 0 の直後の次語検査は b.pl (NaN で続行)。負なら終了。"""
    t = np.ones(4096, F32)
    p = np.ones(4096, F32)
    st = np.ones(51, F32)
    st[34] = 0.0                     # m=0 の許容 0
    st[35] = 0.0                     # m=1 の許容 0 (NaN 語に罰則を掛けない)
    t[18] = 10.0                     # m=2: dev = 9 → 罰則 0 (許容 1 × s_mult 4)
    t1 = t.copy()
    t1[17] = np.nan                  # m=1 の t 語が NaN → 続行 → m=2 の罰則が掛かる
    v = na.similarity(t1, p, st, F32(3.0), F32(4.0))
    check("cv chain NaN after tol0 continues", v == F32(0.0), str(v))
    t2 = t.copy()
    t2[17] = -1.0                    # 負 → 終了 (m=2 の罰則なし)
    v = na.similarity(t2, p, st, F32(3.0), F32(4.0))
    check("cv chain negative after tol0 breaks", v == F32(1.0), str(v))
    t3 = t.copy()
    st3 = st.copy()
    st3[34] = 1.0
    st3[35] = 0.0
    t3[16] = np.nan                  # 通常経路 (m=0、b.mi) でも NaN は続行 (m=0 は許容 1 → 罰則 NaN)
    v = na.similarity(t3, p, st3, F32(3.0), F32(4.0))
    check("cv chain NaN normal path continues", np.isnan(v), str(v))


def t_gate():
    """worker のゲート (G → G·R → G·R·B、閾値 0.5、最後だけ厳密 b.le) と S_LVL/S_MULT の受け渡し。
    類似度を差し替え、面の目印 (語 2000 = チャンネル番号) で G/R/B を区別する。"""
    rng = np.random.default_rng(7)
    rgb = frame(64, 64, rng, "flat")
    buf = bytearray(0xc300)
    prof = na.Prof(buf)
    for c in range(3):
        prof.f[(na.FACE[c] >> 2)] = F32(1.0)
        prof.f[(na.FACE[c] >> 2) + 2000] = F32(c)
    orig = na.similarity
    try:
        for name, vals, exp_acc, exp_calls in (
                ("G=0.5 passes, final 0.5 rejected", (1.0, 0.5, 1.0), 0, {0: 4, 1: 4, 2: 4}),
                ("G=0.49 rejected", (1.0, 0.49, 1.0), 0, {1: 4}),
                ("R gate 0.54 passes, final 0.513 accepted", (0.6, 0.9, 0.95), 4, {0: 4, 1: 4, 2: 4}),
                ("R gate 0.45 rejected", (0.5, 0.9, 1.0), 0, {0: 4, 1: 4}),
                ("B gate 0.48 rejected", (0.6, 0.9, 0.9), 0, {0: 4, 1: 4, 2: 4})):
            calls = {}

            def fake(t, p, stats, s_lvl, s_mult, _v=vals):
                c = int(p[2000])
                calls[c] = calls.get(c, 0) + 1
                return F32(_v[c])
            na.similarity = fake
            d = {}
            na._run_tiles(rgb, prof, 0, None, None, True, d)
            check(f"gate {name}", d.get("accepted", 0) == exp_acc and calls == exp_calls,
                  f"{d} {calls}")
        for mode, exp in ((0, (3.0, 4.0)), (1, (2.0, 3.0))):
            seen = set()

            def fake2(t, p, stats, s_lvl, s_mult):
                seen.add((float(s_lvl), float(s_mult)))
                return F32(1.0)
            na.similarity = fake2
            na._run_tiles(rgb, prof, mode, None, None, True, {})
            check(f"gate s_lvl/s_mult mode{mode}", seen == {exp}, str(seen))
    finally:
        na.similarity = orig


def t_mode2_post():
    prof = _curve_state([1.0] * 9, counts=[0] * 9)
    bins = [[] for _ in range(9)]
    for k in range(3):
        bins[k] = [F32(1.0 + 0.01 * i) for i in range(9)]
    v = na.mode2_post(bins, prof, 0)
    qs = [nb.quantile(np.asarray(bins[k], F32), F32(0.66), 0, 1) for k in range(3)]
    exp = F32(F32(F32(qs[0] / 1.0) + F32(qs[1] / 1.0)) + F32(qs[2] / 1.0)) / F32(3.0)
    check("mode2 avg", v == F32(exp), f"{v} vs {exp}")
    # n < 2 → -1
    prof2 = _curve_state([1.0] * 9)
    bins2 = [[] for _ in range(9)]
    bins2[4] = [F32(1.0)] * 9
    check("mode2 too few", na.mode2_post(bins2, prof2, 0) == F32(-1.0))


# ---------------------------------------------------------------- 3. 全体
def t_full(quick):
    rng = np.random.default_rng(20261017)
    cases = []
    for kind in ("noise", "flat", "grad"):
        cases.append((96, 128, kind))                        # 3x4 タイル
    cases.append((160, 192, "noise"))                         # 5x6
    if not quick:
        cases.append((128, 128, "clip"))                       # IsClip で全タイル棄却が期待
        cases.append((47, 64, "flat"))                        # W < 48 → worker 何もしない
        cases.append((64, 63, "flat"))                        # H < 48
    for h, w, kind in cases:
        rgb = frame(h, w, rng, kind)
        base = npr.build_profile(rgb, 1, (0, 0, min(w, 128), min(h, 128)), with_floor=True,
                                  lzma=False)[0]
        # build_profile は領域 128 上限 → 128x128 に切り詰めた画像で作る (AFT は全画面で走らせる)
        prof_in = bytearray(base)
        for mode in (0, 1):
            d1 = {}
            out1 = na.auto_fine_tune(rgb, prof_in, mode, diag=d1)
            d2 = {}
            out2 = na.auto_fine_tune_fast(rgb, prof_in, mode, diag=d2)
            same = out1 == out2
            check(f"self {kind} {w}x{h} mode{mode} ref==fast", same)
            # 曲線の範囲とフラグの構造検査
            prof = na.Prof(bytearray(out1))
            for c in range(3):
                b0 = prof.curve_base(c)
                lv = np.array([prof.f[b0 + 8 * k] for k in range(9)], F32)
                ck = np.array([prof.i[b0 + 8 * k + 6] for k in range(9)])
                fl = np.array([prof.b[na.CURVE[c] + 0x20 * k + 0x1c] for k in range(9)])
                check(f"curve range {kind} m{mode} c{c}", bool((lv >= F32(0.2)).all() and (lv <= F32(3.0)).all()))
                if mode == 0:
                    # 全ビンが何らかの形で確定 (level!=0) — 補間か測定か
                    check(f"curve filled {kind} m{mode} c{c}", bool((lv != F32(0.0)).all()) or d1.get("accepted", 0) == 0)
            if kind == "clip":
                # クリップ面は tile がほぼ全棄却 → accepted は 0 に近い (保証はしない)
                pass
            if kind == "noise" and mode == 0 and d1.get("accepted", 0) > 0:
                # 誤り注入: 統計 1 語 (stats[0]) を変える → 類似度が変わり曲線が変わるはず
                mut = bytearray(prof_in)
                m = na.Prof(mut)
                m.f[na.STATS[0] >> 2] = F32(float(m.f[na.STATS[0] >> 2]) * 0.5 + 0.1)
                out3 = na.auto_fine_tune(rgb, mut, mode)
                check(f"injection stats {kind} {w}x{h} m{mode}", out3 != out1)
            if mode == 0 and kind != "clip" and h >= 48 and w >= 48:
                # 自己受入回帰: プロファイル領域内のタイルは自己類似で受入されるべき
                # (U32/F32 の型混同と R 語 474→490 の誤りを検出した — 受入 0 なら赤。
                #  clip 素材は IsClip で全棄却、48px 未満は worker が走らないので対象外)
                check(f"self-accept {kind} {w}x{h}", d1.get("accepted", 0) > 0,
                      json.dumps(d1))
        # mode 2: 参照 == 高速 + 戻り値
        o1 = na.auto_fine_tune_mode2(rgb, prof_in)
        o2 = na.auto_fine_tune_fast(rgb, prof_in, 2)
        # W/H < 48 では worker が何もせず bytes を返す (タプルではない) — 型も揃って比較
        if isinstance(o2, tuple):
            same = isinstance(o1, tuple) and o1[0] == o2[0] and o1[1] == o2[1]
        else:
            same = o1 == o2
        check(f"self mode2 {kind} {w}x{h} ref==fast", same)
        # image2 == 同一画像 → 差分 0 → |0| < |tile.w5| で棄却されるはず (d = 0 → |dm|=0 < |m|)
        d = {}
        o3 = na.auto_fine_tune(rgb, prof_in, 0, image2=rgb, diag=d)
        check(f"image2 zero-diff rejects {kind} {w}x{h}", d.get("accepted", 0) == 0 or d.get("diff_skip", 0) > 0)


def t_readonly():
    """COMMON: 試験中に読み取り専用領域を書いていないこと (ここでは src/ 以外の成果物がないこと)。"""
    p = os.path.join(os.path.dirname(_HERE), "logs", os.environ.get("NV_AFT_RO", "readonly_before.sha256"))
    if not os.path.exists(p):
        return
    import hashlib
    ok = True
    root = os.path.abspath(os.path.join(os.path.dirname(_HERE), "..", "..", ".."))
    with open(p) as fh:
        for line in fh:
            h, rel = line.split()
            fp = os.path.join(root, rel)
            if not os.path.exists(fp):
                continue
            with open(fp, "rb") as f2:
                if hashlib.sha256(f2.read()).hexdigest() != h:
                    ok = False
                    print("MUTATED", rel)
    check("readonly files unchanged", ok)


def main():
    quick = "--quick" in sys.argv
    t_penalty()
    t_dev()
    t_cl()
    t_shuffle()
    t_polyfit()
    t_curve_fill()
    t_curve_post()
    t_constants()
    t_cv_nan()
    t_gate()
    t_mode2_post()
    t_full(quick)
    t_readonly()
    print()
    if FAILED:
        print("FAILED:", len(FAILED))
        for f in FAILED:
            print("  -", f)
        sys.exit(1)
    print("ALL PASS")


if __name__ == "__main__":
    main()
