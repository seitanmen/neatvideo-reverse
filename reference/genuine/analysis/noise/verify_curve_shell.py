#!/usr/bin/env python3
"""repro/nv_curve.py (EstimateNoiseLevelCurve の外側の移植) を本家 (harness/nvh) と照合する。

1. tile_layout  vs `nvh tiles W H`      : 9 サイズで記述子 11 項目と順序が完全一致
2. is_clip_genuine vs `nvh clip`         : 16x16 の切り出し (飽和率 0.33 付近・値域の狭い面・丸めの同点)
3. clip_weight の 16x16 ブロックの 0 埋め (規則どおりか、IsClip 値は nvh clip で確認)
4. accumulate の逐次加算 (np.add.at) が画素順の double 加算と一致
5. curve_table の単体テスト (手作りの sums/counts: 全有効・内側の穴・先頭/末尾の両分岐・1 個・0 個)
   と多項式あてはめ・行列式・補間器の独立な参照 (numpy) との比較
6. genuine_noise_curve vs `nvh curve` (高速経路の殻 batch=True と参照の殻 batch=False の両方):
   画素ノイズは本家の `nvh pixnoise` をタイルごとに呼んで
   差し込む (FUN_00998d98 は重みを読まず乗算で更新するだけなので、1 の重みで呼んだ結果に
   クリップのマスクを掛ければ本家の場内処理と同値)。合成画像 (タイル境界・奇数サイズ・クリップ・
   外挿の各分岐) と、--clip で実写フレーム (ホストと同じ RGB 変換)。
   nvh curve は %.17g で表示する (double が正確に読める) ので、8 点の ys のビット一致を求める
   (NaN は NaN とだけ一致。許容誤差なし)。
7. CurveInterp / curve_eval vs `nvh interp` (BaseInterpolator FUN_00aed9f0 / FUN_00aeda08 /
   FUN_00aedb18、%.17g): 2〜8 点の単調・非単調・重複のある表、節点・区間内・範囲の下と上・極端な値
   (±1e308、±inf、NaN、-0、非正規化数、pow が溢れる 1e200 級の節点) の問い合わせでビット一致
8. IsClip の非有限・巨大な値: NaN / ±inf の画素 (SIMD 部と端数部)、値域 * 8 が溢れる面、全 NaN の面
   を `nvh clip` と照合 (nvh clip は %.6f 表示なので、移植の値も %.6f の文字列で比べる)
9. 高速経路 (2026-10-16): 2 と 8 の面を is_clip_blocks (一括版) でも照合、clip_weight_batch == clip_weight、
   殻の一致 (tile_histogram_batch == tile_histogram、決定的な代用 pixnoise で sums をビット比較。本家は不要)。
   代用 pixnoise のノイズは 2^-40..2^40 倍に散らしてあり、ビンの double の和が丸めを含むので加算順で変わる
   (逆順のタイル列で参照の殻の和が変わる画像の数を示し、殻に入れた加算順の誤り 2 種 = タイルの逆順・タイル内の
   画素の逆順が、この比較で検出されることも確かめる)。バッチの大きさ 96 / 1 / 5 / 10000 (小さいほど先に処理した
   タイルが順番待ちになる)、逆順・ランダム順のタイル列、nbins 256 / 257 / 300 (順番待ちのビン番号が uint8 を超える。
   実際の pixnoise でも) で同じビット。genuine_noise_curve の入力検査 (2 次元・3 チャンネル未満・48px 未満・
   チャンネルが先頭の配列・nbins 0 は ValueError)

usage: verify_curve_shell.py [--clip PATH --frames 312,...] [--no-genuine]"""
import argparse
import os
import sys
import tempfile

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "..", "reference", "python"))
sys.path.insert(0, HERE)
import nv_curve as nc  # noqa: E402

F32 = np.float32
RESULTS = []


def check(name, ok, detail=""):
    RESULTS.append((name, bool(ok)))
    print(f"[{'OK ' if ok else 'NG '}] {name}" + (f"  {detail}" if detail else ""), flush=True)
    return bool(ok)


# ----------------------------------------------------------------------------------------
# genuine wrappers
# ----------------------------------------------------------------------------------------
def _nvh():
    from nvh_common import require_guard, run
    require_guard()
    return run


def genuine_tiles(run, W, H):
    """`nvh tiles W H` (no input file; nvh_common.run would insert one as the tile size)."""
    import re
    import subprocess
    from nvh_common import NVH
    r = subprocess.run([NVH, "tiles", str(W), str(H)], capture_output=True, text=True, timeout=120)
    if r.returncode:
        raise RuntimeError(f"nvh tiles exit {r.returncode}: {r.stderr[-300:]}")
    out = r.stdout
    tiles = []
    for line in out.splitlines():
        if not line.startswith("tile "):
            continue
        kv = dict(re.findall(r"(\w+)=(-?\d+)", line))
        tiles.append(tuple(int(kv[k]) for k in ("x", "y", "w", "h", "f4", "f5", "core_w", "core_h",
                                                "core_x", "core_y", "f10")))
    return tiles


def genuine_clip(run, rgb):
    H, W = rgb.shape[:2]
    out = run(["clip", W, H], [np.moveaxis(np.asarray(rgb, F32), -1, 0)])
    return tuple(float(v) for v in out.split()[1:4])


def genuine_clip_str(run, planes):
    """`nvh clip` on up to three (h, w) planes packed as R, G, B -> the IsClip values as printed (%.6f)."""
    h, w = planes[0].shape
    stack = list(planes) + [np.zeros((h, w), F32)] * (3 - len(planes))
    out = run(["clip", w, h], [np.stack(stack)])
    return out.split()[1:1 + len(planes)]


def genuine_interp(xs, ys, qs):
    """`nvh interp n x.. y.. q..` (no input file) -> the values (%.17g) for the queries."""
    import subprocess
    from nvh_common import NVH, require_guard
    require_guard()
    args = [NVH, "interp", str(len(xs))] + [repr(float(v)) for v in list(xs) + list(ys) + list(qs)]
    r = subprocess.run(args, capture_output=True, text=True, timeout=120)
    if r.returncode:
        raise RuntimeError(f"nvh interp exit {r.returncode}: {r.stderr[-300:]}")
    rows = [l.split() for l in r.stdout.splitlines() if l.strip()]
    if len(rows) != len(qs):
        raise RuntimeError(f"nvh interp returned {len(rows)} rows for {len(qs)} queries")
    return np.array([float(q) for q, _ in rows]), np.array([float(v) for _, v in rows])


def same_bits(a, b):
    """Elementwise: identical doubles (sign of zero included) or both NaN."""
    a = np.asarray(a, np.float64)
    b = np.asarray(b, np.float64)
    return (a.view(np.uint64) == b.view(np.uint64)) | (np.isnan(a) & np.isnan(b))


def genuine_curve(run, rgb):
    H, W = rgb.shape[:2]
    out = run(["curve", W, H], [np.moveaxis(np.asarray(rgb, F32), -1, 0)])
    lines = out.strip().splitlines()
    ok, n = int(lines[0].split()[1]), int(lines[0].split()[3])
    pts = [tuple(float(v) for v in l.split()) for l in lines[1:1 + n]]
    return ok, np.array([p[0] for p in pts]), np.array([p[1] for p in pts])


class GenuinePixnoise:
    """pixnoise(luma, weight) を本家の FUN_00998d98 (`nvh pixnoise`、重み 1 で開始) で代替する。
    FUN_00998d98 は入力の重みを読まず FUN_009edc38 で乗算するだけなので weight * 出力重みが本家と同値。"""

    def __init__(self, run):
        self.run = run
        self.dir = tempfile.mkdtemp(prefix="nvcurve_")
        self.calls = 0

    def __call__(self, luma, weight):
        h, w = luma.shape
        pre = os.path.join(self.dir, f"t{self.calls}")
        self.calls += 1
        self.run(["pixnoise", w, h, pre], [np.ascontiguousarray(luma, F32)])
        out = []
        for s in ("bright", "noise", "weight"):
            out.append(np.fromfile(f"{pre}.{s}.f32", F32).reshape(h, w))
            os.unlink(f"{pre}.{s}.f32")
        bright, noise, wt = out
        return bright, noise, (wt * np.asarray(weight, F32)).astype(F32)


# ----------------------------------------------------------------------------------------
# 1. tiles
# ----------------------------------------------------------------------------------------
def test_tiles(run):
    sizes = [(1920, 1080), (1280, 720), (640, 360), (100, 70), (97, 61), (96, 96), (200, 96), (50, 300),
             (4096, 2160)]
    allok = True
    for W, H in sizes:
        g = genuine_tiles(run, W, H)
        p = nc.tile_layout(W, H)
        same = len(g) == len(p) and all(a == b for a, b in zip(g, p))
        allok &= check(f"tiles {W}x{H} ({len(g)} tiles)", same)
    return allok


# ----------------------------------------------------------------------------------------
# 2. IsClip on 16x16 crops
# ----------------------------------------------------------------------------------------
def clip_cases(rng):
    cases = []
    for k in (0, 1, 20, 83, 84, 85, 86, 100, 128, 129, 200, 255, 256):
        for _ in range(2):
            v = np.clip(rng.normal(120, 30, 256), 0, 254.5).astype(F32)
            pos = rng.choice(256, k, replace=False)
            v[pos] = 255.0
            cases.append((f"sat255 k={k}", v.reshape(16, 16)))
            v2 = np.clip(rng.normal(120, 30, 256), 0.5, 255).astype(F32)
            v2[pos] = 0.0
            cases.append((f"sat0 k={k}", v2.reshape(16, 16)))
    cases.append(("constant", np.full((16, 16), 77.0, F32)))
    cases.append(("two values", np.where(rng.random((16, 16)) < 0.4, 10.0, 11.0).astype(F32)))
    cases.append(("gradient", np.linspace(0, 255, 256, dtype=F32).reshape(16, 16)))
    for s in (0.03, 0.05, 0.08, 0.12, 0.2, 0.3, 1.0, 3.0, 10.0):
        for _ in range(4):
            cases.append((f"noise s={s}", (128 + rng.normal(0, s, (16, 16))).astype(F32)))
    for _ in range(6):
        cases.append(("tie grid", (100 + rng.integers(0, 4, (16, 16)) * 0.0625).astype(F32)))
        cases.append(("eighth grid", (100 + rng.integers(0, 9, (16, 16)) * 0.125).astype(F32)))
    return cases


def test_isclip(run):
    rng = np.random.default_rng(3)
    try:
        from nvfilter import is_clip as is_clip_v5
    except Exception:
        is_clip_v5 = None
    bad = bad_v5 = bad_blk = 0
    cases = clip_cases(rng)
    for name, v in cases:
        rgb = np.stack([v, np.flipud(v), np.fliplr(v)], -1)
        g = genuine_clip(run, rgb)
        p = tuple(float(nc.is_clip_genuine(rgb[..., c])) for c in range(3))
        if not all(f"{a:.6f}" == f"{b:.6f}" for a, b in zip(g, p)):      # nvh clip prints %.6f
            bad += 1
            print(f"    DIFF {name}: genuine {g} port {p}")
        q = nc.is_clip_blocks(np.stack([rgb[..., c].ravel() for c in range(3)]))      # batch path
        if not (np.array_equal(q.view(np.uint32), np.array(p, F32).view(np.uint32))
                and all(f"{a:.6f}" == f"{float(b):.6f}" for a, b in zip(g, q))):
            bad_blk += 1
            print(f"    DIFF is_clip_blocks {name}: genuine {g} blocks {q.tolist()}")
        if is_clip_v5 is not None:
            q = tuple(float(F32(is_clip_v5(rgb[..., c]))) for c in range(3))
            if not all(abs(a - b) <= 1e-6 for a, b in zip(g, q)):
                bad_v5 += 1
    ok = check(f"is_clip_genuine vs nvh clip ({len(cases) - bad}/{len(cases)} identical)", bad == 0)
    ok &= check(f"is_clip_blocks (batch path) vs nvh clip and is_clip_genuine ({len(cases) - bad_blk}/{len(cases)} "
                f"identical)", bad_blk == 0)
    if is_clip_v5 is not None:
        print(f"    (info) nvfilter.is_clip (切り捨て) は {bad_v5}/{len(cases)} 件で本家と不一致"
              " (本家は frintn で最近接丸め)")
    return ok


def isclip_extreme_planes(rng):
    """Non-finite and huge planes: [(name, plane)], sizes with and without a scalar tail (n % 8)."""
    out = []

    def g(h, w):
        return rng.normal(100, 10, (h, w)).astype(F32)

    for (h, w) in ((16, 16), (7, 13), (3, 5), (2, 3)):
        n = h * w
        body = n & ~7
        for name, idx, val in (("NaN", 0, np.nan), ("NaN", n - 1, np.nan), ("+inf", n // 2, np.inf),
                               ("-inf", n - 1, -np.inf), ("+inf", 0, np.inf)):
            p = g(h, w).ravel()
            p[idx] = val
            where = "SIMD part" if idx < body else "scalar tail"
            out.append((f"{h}x{w} {name} at {idx} ({where})", p.reshape(h, w)))
        out.append((f"{h}x{w} all NaN", np.full((h, w), np.nan, F32)))
        p = g(h, w).ravel()
        p[body:] = np.nan
        out.append((f"{h}x{w} NaN on the whole scalar tail", p.reshape(h, w)))
    for hi in (1e38, 5e37, 4e37, 3e38):
        for (h, w) in ((16, 16), (7, 13)):
            n = h * w
            p = np.concatenate([np.zeros(n * 100 // 256), rng.uniform(0, hi, n - n * 100 // 256)]).astype(F32)
            out.append((f"{h}x{w} zeros + uniform(0, {hi:g})", p.reshape(h, w)))
    p = np.where(rng.random((16, 16)) < 0.5, F32(3e38), F32(-3e38)).astype(F32)
    out.append(("16x16 +-3e38 (range overflows)", p))
    p = g(16, 16)
    p[3, 3] = F32(2e38)
    out.append(("16x16 one 2e38 pixel", p))
    p = (rng.normal(0, 1e-40, (16, 16))).astype(F32)
    out.append(("16x16 subnormal noise", p))
    return out


def test_isclip_extreme(run):
    rng = np.random.default_rng(17)
    cases = isclip_extreme_planes(rng)
    bysize = {}
    for name, p in cases:
        bysize.setdefault(p.shape, []).append((name, p))
    bad = bad_blk = 0
    for shape, lst in bysize.items():
        for i in range(0, len(lst), 3):
            grp = lst[i:i + 3]
            gs = genuine_clip_str(run, [p for _, p in grp])
            with np.errstate(all="ignore"):
                blk = nc.is_clip_blocks(np.stack([p.ravel() for _, p in grp]))     # batch path, same planes
            for (name, p), gstr, q in zip(grp, gs, blk):
                try:
                    with np.errstate(all="ignore"):
                        pv = nc.is_clip_genuine(p)
                    pstr = f"{float(pv):.6f}"
                except Exception as e:  # noqa: BLE001
                    pv, pstr = None, f"{type(e).__name__}: {e}"
                if pstr != gstr:
                    bad += 1
                    print(f"    DIFF {name}: nvh clip {gstr} port {pstr}")
                if f"{float(q):.6f}" != gstr or pv is None or F32(pv).view(np.uint32) != q.view(np.uint32):
                    bad_blk += 1
                    print(f"    DIFF is_clip_blocks {name}: nvh clip {gstr} blocks {float(q):.6f}")
    ok = check(f"is_clip_genuine on non-finite / huge planes vs nvh clip ({len(cases) - bad}/{len(cases)} identical "
               f"as %.6f, no exceptions)", bad == 0)
    return ok & check(f"is_clip_blocks (batch path) on the same planes ({len(cases) - bad_blk}/{len(cases)} identical "
                      f"to nvh clip and bit-identical to is_clip_genuine)", bad_blk == 0)


def test_clip_weight_rule():
    """0x9b7b7c-0x9b7b84: fcmp R, thr / fccmp B, thr, #0, le / b.le keeps the block -> zeroed only when
    R > thr or B > thr (ordered); an unordered (NaN) fraction never zeroes on its own.  The genuine
    IsClip never returns NaN, so this checks the rule with stub IsClip values."""
    R = np.zeros((16, 16), F32)
    B = np.ones((16, 16), F32)
    ok = True
    for r, b, zero in ((np.nan, 0.5, True), (np.nan, np.nan, False), (np.nan, 0.2, False), (0.5, np.nan, True),
                       (0.2, np.nan, False), (0.33, 0.33, False), (0.3300001, 0.0, True), (0.0, 0.3300001, True)):
        w = nc.clip_weight(R, R, B, is_clip=lambda pl, r=r, b=b: F32(r) if pl[0, 0] == 0 else F32(b))
        ok &= bool((w.max() == 0) == zero)
    return check("clip_weight zeroes iff R > 0.33 or B > 0.33 (NaN keeps the block)", ok)


# ----------------------------------------------------------------------------------------
# 3. clip-block zeroing
# ----------------------------------------------------------------------------------------
def test_clip_weight(run):
    rng = np.random.default_rng(5)
    h, w = 108, 120     # a full tile with 16px blocks: 7 rows (last shifted to 92), 8 cols (last at 104)
    base = 60 + rng.normal(0, 4, (h, w, 3))
    base[16:32, 32:48, 0] = 255.0            # R saturated block (aligned)
    base[96:112, 104:120, 2] = 0.0           # B clipped block at the shifted last block (rows 92..108)
    base[48:64, 64:80, 1] = 255.0            # G saturated only -> must NOT be zeroed
    base[80:88, 16:32, 0] = 255.0            # half a block of R (128 px = 50% -> clipped)
    base[0:16, 80:96, 0] = 255.0
    base[0:16, 80:96, 0][rng.random((16, 16)) < 0.6] = 70.0   # ~40% saturated -> clipped (> 0.33)
    base[32:48, 0:16, 0] = 255.0
    base[32:48, 0:16, 0][rng.random((16, 16)) < 0.8] = 70.0   # ~20% saturated -> kept
    rgb = np.clip(base, 0, 255).astype(F32)
    R, G, B = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    weight = nc.clip_weight(R, G, B)
    # independent evaluation of the rule with the genuine IsClip of each crop
    expect = np.ones((h, w), F32)
    rows = [min(r * 16, h - 16) for r in range((h + 15) // 16)]
    cols = [min(c * 16, w - 16) for c in range((w + 15) // 16)]
    ng = 0
    for by in rows:
        for bx in cols:
            crop = rgb[by:by + 16, bx:bx + 16]
            gr, gg, gb = genuine_clip(run, crop)
            pr, pb = float(nc.is_clip_genuine(crop[..., 0])), float(nc.is_clip_genuine(crop[..., 2]))
            if abs(gr - pr) > 1e-6 or abs(gb - pb) > 1e-6:
                ng += 1
            if gr > 0.33 or gb > 0.33:
                expect[by:by + 16, bx:bx + 16] = 0
    zero_blocks = int((expect == 0).sum() // 256)
    ok = check(f"clip_weight mask == rule from genuine IsClip ({zero_blocks} blocks zeroed, "
               f"{len(rows) * len(cols)} crops, IsClip diffs {ng})", np.array_equal(weight, expect) and ng == 0)
    wb = nc.clip_weight_batch(R[None], B[None])[0]
    ok &= check("clip_weight_batch (batch path) == clip_weight on the same tile", np.array_equal(wb, weight))
    # the G-only block must stay
    ok &= check("clip_weight ignores G (block at 48:64,64:80 kept)", float(weight[48:64, 64:80].min()) == 1.0)
    ok &= check("clip_weight zeroes the shifted last block (92:108,104:120)", float(weight[92:108, 104:120].max()) == 0.0)
    return ok


# ----------------------------------------------------------------------------------------
# 4. accumulate order
# ----------------------------------------------------------------------------------------
def test_accumulate():
    rng = np.random.default_rng(9)
    h, w = 108, 120
    desc = (84, 0, 120, 108, 96, 0, 96, 96, 12, 0, 0)
    bright = rng.uniform(-5, 260, (h, w)).astype(F32)
    noise = rng.uniform(0, 10, (h, w)).astype(F32)
    weight = rng.uniform(0, 1, (h, w)).astype(F32)
    weight[3, 15] = 0.5
    sums = np.zeros(8)
    counts = np.zeros(8, np.int64)
    nc.accumulate(bright, noise, weight, desc, sums, counts)
    rs, rc = [0.0] * 8, [0] * 8
    scale = F32(8) * F32(0.00390625)
    for r in range(96):
        for c in range(96):
            y, x = r, 12 + c
            if weight[y, x] < 0.5:
                continue
            v = F32(scale * bright[y, x])
            b = min(max(int(v), 0), 7)
            rs[b] = rs[b] + float(noise[y, x])
            rc[b] += 1
    return check("accumulate == sequential per-pixel double sums (core only, weight >= 0.5)",
                 sums.tolist() == rs and counts.tolist() == rc)


# ----------------------------------------------------------------------------------------
# 5. curve_table unit tests (independent references)
# ----------------------------------------------------------------------------------------
def _ref_fit(xs, ys, deg):
    return np.polyfit(np.asarray(xs, float), np.asarray(ys, float), deg)[::-1]   # lowest order first


def _ref_eval(c, x):
    return float(sum(ci * x ** i for i, ci in enumerate(c)))


def _smooth(m):
    out = np.array(m, float)
    for i in range(1, len(m) - 1):
        out[i] = (m[i] + (m[i - 1] + m[i + 1]) * 0.5) * 0.5
    return out


def test_curve_units():
    W, H = 640, 480            # threshold (W*H)>>10 = 300
    thr = (W * H) >> 10
    ok = True
    xs = nc.curve_xs()
    ok &= check("curve xs = 32*(i+0.5)", np.array_equal(xs, 32.0 * (np.arange(8) + 0.5)))

    def table(means, counts):
        counts = np.asarray(counts, np.int64)
        sums = np.asarray(means, float) * counts
        return nc.curve_table(sums, counts, W, H)[1]

    # all valid
    m = [2.0, 2.2, 2.5, 2.7, 2.6, 2.3, 2.0, 1.6]
    ys = table(m, [thr] * 8)
    ok &= check("all valid -> [1,2,1]/4 smoothing only", np.allclose(ys, _smooth(m), rtol=0, atol=1e-12))
    # interior gap, first/last valid (quadratic through the 7 valid points)
    cnt = [thr] * 8
    cnt[3] = thr - 1
    ys = table(m, cnt)
    exp = list(m)
    exp[3] = _ref_eval(_ref_fit([0, 1, 2, 4, 5, 6, 7], [m[i] for i in (0, 1, 2, 4, 5, 6, 7)], 2), 3.0)
    ok &= check("interior gap -> quadratic LS of the valid bins", np.allclose(ys, _smooth(exp), rtol=1e-9, atol=0))
    # two valid points -> ncoef 2 (line) for the interior
    cnt = [0] * 8
    cnt[2] = cnt[7] = thr
    ys = table(m, cnt)
    line = _ref_fit([2, 7], [m[2], m[7]], 1)
    exp = [m[2]] * 2 + [_ref_eval(line, float(i)) for i in range(2, 8)]
    ok &= check("two valid (2,7) -> linear interior, constant below (first >= nbins/3)",
                np.allclose(ys, _smooth(exp), rtol=1e-9, atol=0))
    # first = 1 (< nbins//3 = 2): line through the first 3 valid points extrapolates bin 0
    cnt = [thr] * 8
    cnt[0] = 0
    ys = table(m, cnt)
    exp = list(m)
    exp[0] = _ref_eval(_ref_fit([1, 2, 3], m[1:4], 1), 0.0)
    ok &= check("first=1 -> line of first 3 points (ncoef 2)", np.allclose(ys, _smooth(exp), rtol=1e-9, atol=0))
    # first = 2 (>= 2): constant from the first valid point
    cnt = [thr] * 8
    cnt[0] = cnt[1] = 0
    ys = table(m, cnt)
    exp = list(m)
    exp[0] = exp[1] = m[2]
    ok &= check("first=2 -> constant of first point (ncoef 1)", np.allclose(ys, _smooth(exp), rtol=0, atol=1e-12))
    # last = 5 (>= 2*nbins//3 = 5): line through the last 3 valid points
    cnt = [thr] * 8
    cnt[6] = cnt[7] = 0
    ys = table(m, cnt)
    exp = list(m)
    line = _ref_fit([3, 4, 5], m[3:6], 1)
    exp[6], exp[7] = _ref_eval(line, 6.0), _ref_eval(line, 7.0)
    ok &= check("last=5 -> line of last 3 points (ncoef 2)", np.allclose(ys, _smooth(exp), rtol=1e-9, atol=0))
    # last = 4 (< 5): constant from the last valid point
    cnt = [thr] * 8
    cnt[5] = cnt[6] = cnt[7] = 0
    ys = table(m, cnt)
    exp = list(m)
    exp[5] = exp[6] = exp[7] = m[4]
    ok &= check("last=4 -> constant of last point (ncoef 1)", np.allclose(ys, _smooth(exp), rtol=0, atol=1e-12))
    # extrapolated value <= 0 -> copy the neighbour (steep line going negative at bin 7)
    m2 = [3.0, 2.5, 2.0, 1.5, 1.0, 0.5, 2.0, 2.0]
    cnt = [thr] * 8
    cnt[6] = cnt[7] = 0
    ys = table(m2, cnt)
    line = _ref_fit([3, 4, 5], m2[3:6], 1)
    exp = list(m2)
    exp[6] = _ref_eval(line, 6.0)          # 0.0 -> <= 0 -> copy m2[5]
    exp[6] = exp[6] if exp[6] > 0 else exp[5]
    exp[7] = _ref_eval(line, 7.0)          # -0.5 -> copy exp[6]
    exp[7] = exp[7] if exp[7] > 0 else exp[6]
    ok &= check("fit <= 0 above -> neighbour copied", np.allclose(ys, _smooth(exp), rtol=1e-9, atol=1e-12)
                and abs(exp[7] - 0.5) < 1e-9)
    m3 = [2.0, 2.0, 0.2, 0.7, 1.2, 1.7, 2.2, 2.7]
    cnt = [thr] * 8
    cnt[0] = cnt[1] = 0
    ys = table(m3, cnt)
    line = _ref_fit([2, 3, 4], m3[2:5], 1)   # slope 0.5 -> y(1) = -0.3 -> copy
    exp = list(m3)
    exp[1] = _ref_eval(line, 1.0)
    exp[1] = exp[1] if exp[1] > 0 else exp[2]
    exp[0] = _ref_eval(line, 0.0)
    exp[0] = exp[0] if exp[0] > 0 else exp[1]
    ok &= check("fit <= 0 below -> neighbour copied", np.allclose(ys, _smooth(exp), rtol=1e-9, atol=1e-12)
                and abs(exp[0] - 0.2) < 1e-9)
    # one valid / none valid / zero mean counts as invalid
    cnt = [0] * 8
    cnt[4] = thr
    ok &= check("one valid -> constant", np.allclose(table(m, cnt), [m[4]] * 8, rtol=0, atol=0))
    ok &= check("none valid -> 1.0", np.allclose(table(m, [0] * 8), [1.0] * 8, rtol=0, atol=0))
    ok &= check("mean 0 is invalid", np.allclose(table([0.0] * 8, [thr] * 8), [1.0] * 8, rtol=0, atol=0))
    # threshold boundary: count == thr valid, thr-1 invalid
    cnt = [thr] * 8
    cnt[7] = thr - 1
    ys = table(m, cnt)
    exp = list(m)
    exp[7] = _ref_eval(_ref_fit([4, 5, 6], m[4:7], 1), 7.0)
    ok &= check("count == (W*H)>>10 valid, one less invalid", np.allclose(ys, _smooth(exp), rtol=1e-9, atol=0))

    # polynomial pieces vs numpy
    rng = np.random.default_rng(2)
    worst = 0.0
    for ncoef in (1, 2, 3):
        for _ in range(50):
            n = rng.integers(ncoef, 9)
            px = sorted(rng.choice(8, n, replace=False).astype(float))
            py = rng.uniform(0.5, 5, n)
            c = nc.polyfit_genuine(px, py, n, ncoef)
            r = _ref_fit(px, py, ncoef - 1)
            worst = max(worst, float(np.max(np.abs(np.array(c) - r) / np.maximum(np.abs(r), 1e-9))))
    ok &= check(f"polyfit_genuine vs np.polyfit (worst rel {worst:.1e})", worst < 1e-8)
    worst = 0.0
    for n in (1, 2, 3, 4):
        for _ in range(30):
            a = rng.normal(0, 3, (n, n)).tolist()
            d = nc._det(a, n)
            r = float(np.linalg.det(np.array(a)))
            worst = max(worst, abs(d - r) / max(abs(r), 1e-9))
    ok &= check(f"_det vs np.linalg.det (worst rel {worst:.1e})", worst < 1e-9)
    # interpolator
    t = (xs, np.array(m))
    ci = nc.CurveInterp(*t)
    # inner nodes are hit exactly (t = 0); the last node already uses the upper line (x >= xs[n-1])
    at_nodes = all(nc.curve_eval(ci, x) == y for x, y in zip(xs[:-1], m[:-1])) and abs(nc.curve_eval(ci, xs[-1]) - m[-1]) < 1e-9
    mids = all(abs(nc.curve_eval(ci, (xs[i] + xs[i + 1]) / 2) - (m[i] + m[i + 1]) / 2) < 1e-12 for i in range(7))
    lo = _ref_fit(xs[:2], m[:2], 1)
    hi = _ref_fit(xs[-2:], m[-2:], 1)
    ext = (abs(nc.curve_eval(t, 0.0) - _ref_eval(lo, 0.0)) < 1e-9 and abs(nc.curve_eval(t, 255.0) - _ref_eval(hi, 255.0)) < 1e-9
           and abs(nc.curve_eval(t, 15.0) - _ref_eval(lo, 15.0)) < 1e-9)
    ok &= check("curve_eval: nodes, midpoints, linear extrapolation of the end pairs", at_nodes and mids and ext)
    arr = nc.curve_eval(t, np.array([[0.0, 16.0], [100.0, 300.0]]))
    ok &= check("curve_eval on arrays", arr.shape == (2, 2) and arr[0, 1] == m[0])
    return ok


# ----------------------------------------------------------------------------------------
# 7. interpolator vs nvh interp
# ----------------------------------------------------------------------------------------
def interp_tables(rng):
    """[(name, xs, ys)] for `nvh interp`: monotone, non-monotone, duplicates, the curve's own grid,
    extreme magnitudes (no NaN nodes: the genuine then reads past its arrays)."""
    out = []
    for n in range(2, 9):
        for k in range(3):
            xs = np.sort(rng.uniform(-50, 300, n))
            out.append((f"monotone n={n} #{k}", xs, rng.uniform(0.1, 30, n)))
            out.append((f"non-monotone n={n} #{k}", rng.permutation(xs) if n > 2 else xs[::-1].copy(),
                        rng.normal(0, 20, n)))
    out.append(("curve grid 16..240", 32.0 * (np.arange(8) + 0.5),
                np.array([2.383856, 2.559666, 2.748498, 2.857779, 2.822830, 2.553341, 2.049236, 1.496170])))
    out.append(("duplicate inner knot", np.array([0.0, 1.0, 1.0, 2.0, 3.0]), np.array([1.0, 2.0, 5.0, 3.0, 4.0])))
    out.append(("duplicate end knots", np.array([0.0, 0.0, 1.0, 2.0, 2.0]), np.array([1.0, 2.0, 5.0, 3.0, 4.0])))
    out.append(("huge knots 1e200", np.array([1e200, 2e200, 5e200]), np.array([1.0, 2.0, -3.0])))
    out.append(("tiny knots", np.array([1e-300, 2e-300, 5e-300, 1e-299]), np.array([1e300, -1e300, 3.0, 5e-324])))
    out.append(("huge values", np.array([0.0, 1.0, 2.0]), np.array([1e308, -1e308, 1.7e308])))
    out.append(("NaN / inf values", np.array([0.0, 1.0, 2.0, 3.0]), np.array([np.nan, 1.0, np.inf, -np.inf])))
    out.append(("negative zero knot", np.array([-0.0, 1.0]), np.array([-0.0, 1.0])))
    return out


def interp_queries(rng, xs):
    lo, hi = float(np.min(xs)), float(np.max(xs))
    span = hi - lo if np.isfinite(hi - lo) and hi > lo else 1.0
    q = list(xs) + list(rng.uniform(lo, hi, 12)) + [lo - span * 0.37, lo - 1e-9 * span, hi + 1e-9 * span,
                                                     hi + span * 2.1, (lo + hi) / 2]
    q += [0.0, -0.0, 1e308, -1e308, np.inf, -np.inf, np.nan, 5e-324, -5e-324, 1e-300, 255.0, 16.0, 240.0]
    q += [float(np.nextafter(v, np.inf)) for v in xs] + [float(np.nextafter(v, -np.inf)) for v in xs]
    return np.array(q, np.float64)


def test_interp():
    rng = np.random.default_rng(31)
    total = bad = 0
    for name, xs, ys in interp_tables(rng):
        qs = interp_queries(rng, xs)
        gq, gv = genuine_interp(xs, ys, qs)
        if not np.all(same_bits(gq, qs)):
            raise RuntimeError(f"nvh interp did not read the queries back exactly ({name})")
        with np.errstate(all="ignore"):
            pv = nc.curve_eval((xs, ys), qs)
        eq = same_bits(gv, pv)
        total += len(qs)
        bad += int((~eq).sum())
        for i in np.nonzero(~eq)[0][:3]:
            print(f"    DIFF {name} q={qs[i]!r}: nvh interp {gv[i]!r} port {pv[i]!r}")
    return check(f"CurveInterp / curve_eval vs nvh interp: {total - bad}/{total} queries bit-identical "
                 f"({len(interp_tables(np.random.default_rng(31)))} tables, n = 2..8)", bad == 0)


# ----------------------------------------------------------------------------------------
# 6. end-to-end vs nvh curve
# ----------------------------------------------------------------------------------------
def synthetic_images():
    rng = np.random.default_rng(21)
    out = []

    def grad(W, H, seed=1, flip=False, s=5):
        r = np.random.default_rng(seed)
        _, xx = np.mgrid[0:H, 0:W]
        base = (40 + 150 * (xx / 256)).astype(F32)
        if flip:
            base = base[:, ::-1]
        return np.clip(np.stack([base] * 3, -1) + r.normal(0, s, (H, W, 3)), 0, 255).astype(F32)

    out.append(("gradient 256x256 (3x3 tiles)", grad(256, 256)))
    out.append(("gradient 256x256 flipped", grad(256, 256, flip=True)))
    out.append(("gradient 108x108 (2x2, 48px tiles)", grad(108, 108)))
    out.append(("gradient 256x96 (one row)", grad(256, 96)))
    out.append(("gradient 97x61 (odd, n%4 luma tail)", grad(97, 61)))
    out.append(("gradient 200x96", grad(200, 96)))
    H, W = 240, 320
    _, xx = np.mgrid[0:H, 0:W]
    base = (40 + 150 * (xx / W)).astype(F32)
    img = np.stack([base] * 3, -1) + rng.normal(0, 5, (H, W, 3))
    img[40:120, 60:200] = 300
    img[200:, :] = -10
    img[:, 280:] = [255, 128, 128]
    out.append(("clipped regions 320x240", np.clip(img, 0, 255).astype(F32)))
    mid = np.clip(np.stack([np.full((H, W), 100.0) + 40 * (xx / W)] * 3, -1) + rng.normal(0, 4, (H, W, 3)), 0, 255)
    out.append(("mid bins only (constant extrapolation both sides)", mid.astype(F32)))
    pl = np.full((H, W), 50.0)
    pl[:, 110:220] = 180.0
    pl[:, 220:] = 215.0
    out.append(("plateaus bins 1,5,6 (gap + line extrapolation)",
                np.clip(np.stack([pl] * 3, -1) + rng.normal(0, 3, (H, W, 3)), 0, 255).astype(F32)))
    pl2 = np.full((H, W), 80.0)
    pl2[:, 160:] = 240.0
    out.append(("plateaus bins 2,7 (two points)",
                np.clip(np.stack([pl2] * 3, -1) + rng.normal(0, 3, (H, W, 3)), 0, 255).astype(F32)))
    out.append(("single bin 4", np.clip(np.stack([np.full((H, W), 150.0)] * 3, -1) + rng.normal(0, 6, (H, W, 3)), 0, 255).astype(F32)))
    col = np.stack([base * 0.9, base, base * 0.7], -1) + rng.normal(0, 1, (H, W, 3)) * [4, 2, 6]
    out.append(("coloured noise RGB", np.clip(col, 0, 255).astype(F32)))
    out.append(("constant, no noise (all blocks clipped)", np.full((H, W, 3), 77.0, F32)))
    out.append(("all white", np.full((H, W, 3), 255.0, F32)))
    return out


def compare_curve(run, pix, name, rgb):
    """nvh curve vs the shell fed with `pix`, through both shells: batch=True (tile_histogram_batch, the
    default since 2026-10-16) and batch=False (tile_histogram, the reference)."""
    ok, gx, gy = genuine_curve(run, rgb)
    same_all = True
    for batch in (True, False):
        with np.errstate(all="ignore"):
            px, py, sums, counts = nc.genuine_noise_curve(rgb, pixnoise=pix, return_hist=True, batch=batch)
        n_eq = int(same_bits(gy, py).sum()) if len(gy) == len(py) else 0
        with np.errstate(all="ignore"):
            d = float(np.nanmax(np.abs(gy - py))) if len(gy) == len(py) and np.any(np.isfinite(gy - py)) else 0.0
        same = ok == 1 and np.array_equal(gx, px) and n_eq == len(gy) == len(py)
        check(f"curve {name} [{'batch' if batch else 'reference'} shell]: {n_eq}/{len(gy)} ys bit-identical (%.17g), "
              f"max|diff| {d:.3g}, counts {counts.tolist()}", same)
        if not same:
            print("    genuine", [repr(float(v)) for v in gy])
            print("    port   ", [repr(float(v)) for v in py])
        same_all &= same
    return same_all


def _stub_pixnoise(luma, weight):
    """A deterministic stand-in for FUN_00998d98 (a function of the tile alone) for the shell test.
    noise = |horizontal difference| * 2^e, e in -40..40 from a hash of each pixel's luma bits: the double bin
    sums of values from ~1e-18 to 3e14 round, so they depend on the order of the additions (the order is
    tested, not only which pixels are added; an exact sum would match in any order).  NaN noise only on
    pixels of luma >= 224 (bin 7 of 8) and 3e38 only below 32 (bin 0), so the other bins stay finite;
    weight 0.25 where the difference exceeds 25, NaN here and there (NaN weights are counted)."""
    luma = np.ascontiguousarray(luma, F32)
    diff = np.abs(luma - np.roll(luma, 1, 1)).astype(F32)
    e = ((luma.view(np.uint32) * np.uint32(2654435761)) >> np.uint32(25)) % np.uint32(81)
    noise = np.ldexp(diff, e.astype(np.int32) - 40).astype(F32)          # exact: a power of two
    at = np.zeros(luma.shape, bool)
    at[::37, ::29] = True
    noise[at & (luma >= 224)] = np.nan
    at[:] = False
    at[5::41, 3::31] = True
    noise[at & (luma < 32)] = F32(3e38)
    w = (np.asarray(weight, F32) * np.where(diff > 25, F32(0.25), F32(1.0))).astype(F32)
    w[7::43, ::53] = np.nan
    return luma, noise, w


def _same_hist(a, b):
    return np.array_equal(a[0].view(np.uint64), b[0].view(np.uint64)) and np.array_equal(a[1], b[1])


def _waiting_tiles(tiles, max_batch):
    """The tiles the batch shell holds back as documented (size batches in the order of their first tile,
    tiles added in list order): those computed while an earlier tile is still missing."""
    nxt, held, out = 0, set(), set()
    for idx in sorted(nc._tile_batches(tiles, max_batch), key=min):
        for ti in idx:
            if ti != nxt:
                held.add(ti)
                out.add(ti)
                continue
            nxt += 1
            while nxt in held:
                held.discard(nxt)
                nxt += 1
    return out


def _kept_bins(rgb, d, pixnoise, nbins):
    """Bin numbers of the kept core pixels of tile d (the reference shell's steps for one tile)."""
    x, y, w, h = d[:4]
    R, G, B = (np.ascontiguousarray(rgb[y:y + h, x:x + w, c], F32) for c in range(3))
    with np.errstate(all="ignore"):
        bright, _, weight = (pixnoise or nc._default_pixnoise())(nc.luma_bt601(R, G, B), nc.clip_weight(R, G, B))
    core = (slice(d[9], d[9] + d[7]), slice(d[8], d[8] + d[6]))
    return nc.bin_index(bright[core][~(weight[core] < F32(0.5))], nbins)


def test_shell_batch(clip=None, frames="312"):
    """tile_histogram_batch == tile_histogram bit for bit with the same stub pixnoise (clip weights,
    luma, the core selection and the accumulation order of the batch shell), no harness.  The stub's
    sums depend on the addition order; two injected order faults in the batch shell must be detected."""
    from verify_curve import OrderFault
    imgs = synthetic_images()
    if clip:
        import nvfilter as nv
        from denoise_video_v5 import probe, read_frames
        info = probe(clip)
        want = sorted(int(v) for v in frames.split(","))
        mat = nv.matrix_from_tags(info["tags"], info["H"])
        for i, pl in read_frames(clip, info["W"], info["H"], want[0], want[-1] + 1, fps=info["fps"]):
            if i in want:
                pl = tuple(np.asarray(p, F32) for p in pl)
                imgs.append((f"{os.path.basename(clip)} n={i}", np.clip(nv.yuv_to_rgb(pl, mat, info["full_range"]), 0, 255).astype(F32)))
    # Tiles wait only when a later size batch holds earlier tiles: none do on the small synthetic images. With
    # 288 rows the first and the last tile row (both 108 high) share a size batch, so the last row's tiles wait
    # for the middle row; on 1920x1080, 21 tiles wait (18 with max_batch 5). The 320x288 image catches waiting
    # tiles flushed in reverse, the 1080p one waiting tiles flushed before the current tile.
    pl = dict(imgs)["plateaus bins 2,7 (two points)"]                      # 320x240
    tall = np.ascontiguousarray(np.concatenate([pl, pl[:48]], 0))         # 320x288
    yy, xx = np.mgrid[0:1080, 0:1920]
    g = (20 + 220 * (xx / 1920) * (0.6 + 0.4 * yy / 1080)).astype(F32)
    hd = np.clip(np.stack([g * 0.95, g, g * 0.9], -1) + np.random.default_rng(3).normal(0, 4, (1080, 1920, 3)), 0, 255)
    main = imgs + [("plateaus 320x288 (last tile row waits)", tall), ("gradient 1920x1080 (21 tiles wait)", hd.astype(F32))]
    bad = waiting = 0
    waits = {}
    sizes = (nc.MAX_BATCH, 1, 5, 10000)      # 96 / 5 / 10000: tiles of later rows can wait; 1 never waits
    refs = []
    for name, rgb in main:
        tl = nc.tile_layout(rgb.shape[1], rgb.shape[0])
        with np.errstate(all="ignore"):
            a = nc.tile_histogram(rgb, pixnoise=_stub_pixnoise)
            refs.append(a)
            for mb in sizes:
                w = len(_waiting_tiles(tl, mb))
                waiting += w
                waits[name] = waits.get(name, 0) + w
                b = nc.tile_histogram_batch(rgb, pixnoise=_stub_pixnoise, max_batch=mb)
                if not _same_hist(a, b):
                    bad += 1
                    print(f"    DIFF shell {name} max_batch {mb}: reference {a[0].tolist()} {a[1].tolist()} "
                          f"batch {b[0].tolist()} {b[1].tolist()}")
    ok = check(f"batch shell == reference shell with a stub pixnoise (sums bitwise, counts), max_batch "
               f"{'/'.join(map(str, sizes))}: {len(main) * len(sizes) - bad}/{len(main) * len(sizes)} "
               f"({waiting} waiting tiles over all runs; 320x288 {waits[main[-2][0]]}, 1920x1080 {waits[main[-1][0]]})",
               bad == 0 and waits[main[-2][0]] > 0 and waits[main[-1][0]] > 0)
    # the stub makes the comparison order-sensitive: the reference shell on the reversed tile list gives other
    # sums, and a wrong order inside the batch shell (OrderFault) is caught by the comparison above
    n_rev = det_t = det_p = 0
    for (name, rgb), a in zip(main, refs):
        tl = nc.tile_layout(rgb.shape[1], rgb.shape[0])
        with np.errstate(all="ignore"):
            n_rev += not _same_hist(a, nc.tile_histogram(rgb, pixnoise=_stub_pixnoise, tiles=tl[::-1]))
            for kind in ("tiles", "pixels"):
                with OrderFault(kind):
                    f = nc.tile_histogram_batch(rgb, pixnoise=_stub_pixnoise, max_batch=5)
                if kind == "tiles":
                    det_t += not _same_hist(a, f)
                else:
                    det_p += not _same_hist(a, f)
    ok &= check(f"stub sums are order-sensitive: reversed tile list changes the reference shell's sums on {n_rev}/"
                f"{len(main)} images; injected faults in the batch shell detected: tiles added in reverse order "
                f"{det_t}/{len(main)}, each tile's pixels reversed {det_p}/{len(main)}",
                n_rev > 0 and det_t > 0 and det_p > 0)
    # the scheduler order is the order of the tile list: a reversed / shuffled list through both shells
    rgb = imgs[0][1]
    tl = nc.tile_layout(rgb.shape[1], rgb.shape[0])
    perm = np.random.default_rng(5).permutation(len(tl))
    bad = moved = 0
    for order in (tl[::-1], [tl[i] for i in perm]):
        with np.errstate(all="ignore"):
            a = nc.tile_histogram(rgb, pixnoise=_stub_pixnoise, tiles=order)
            b = nc.tile_histogram_batch(rgb, pixnoise=_stub_pixnoise, tiles=order, max_batch=3)
        bad += not _same_hist(a, b)
        moved += not _same_hist(a, refs[0])
    ok &= check(f"batch shell == reference shell for a reversed and a shuffled tile list ({imgs[0][0]}; both sums "
                f"differ from the scheduler order's: {moved}/2)", bad == 0 and moved == 2)
    # bins of waiting tiles beyond uint8 (nbins >= 257; the genuine uses 8), stub and the real pixnoise, on the
    # 320x288 image whose last tile row waits; luma 240 of the plateau image lands in bin 281 of 300 (bins >= 256
    # need luma >= 218.5)
    crop = np.ascontiguousarray(tall[:, 100:260])                         # 160x288 for the real pixnoise
    cases = [(f"stub {name}", rgb, _stub_pixnoise) for name, rgb in imgs + [("plateaus 320x288", tall)]]
    cases.append((f"pixnoise {crop.shape[1]}x{crop.shape[0]}", crop, None))
    bad = n = held = held_hi = 0
    for nbins in (256, 257, 300):
        for name, rgb, pix in cases:
            tl = nc.tile_layout(rgb.shape[1], rgb.shape[0])
            with np.errstate(all="ignore"):
                a = nc.tile_histogram(rgb, pixnoise=pix, nbins=nbins)
                for mb in ((nc.MAX_BATCH, 1, 5) if pix is not None else (nc.MAX_BATCH, 1)):
                    n += 1
                    if not _same_hist(a, nc.tile_histogram_batch(rgb, pixnoise=pix, nbins=nbins, max_batch=mb)):
                        bad += 1
                        print(f"    DIFF nbins {nbins} {name} max_batch {mb}")
                    if nbins == 300:
                        w = _waiting_tiles(tl, mb)
                        held += len(w)
                        held_hi += sum(int((_kept_bins(rgb, tl[t], pix, nbins) >= 256).sum()) for t in w)
    ok &= check(f"nbins 256 / 257 / 300: batch shell == reference shell ({n - bad}/{n}; stub on {len(imgs) + 1} images, "
                f"the real pixnoise on {crop.shape[1]}x{crop.shape[0]}; at nbins 300 {held} waiting tiles with "
                f"{held_hi} kept pixels in bins >= 256)", bad == 0 and held_hi > 0)
    # input checks of genuine_noise_curve (ValueError, no genuine run: the genuine asserts below 48 px)
    msgs = []
    for shape, nbins in (((100, 100), 8), ((100, 100, 2), 8), ((47, 100, 3), 8), ((100, 47, 3), 8), ((0, 0, 3), 8),
                         ((3, 120, 160), 8), ((48, 48, 3), 0)):
        try:
            nc.genuine_noise_curve(np.zeros(shape, F32), nbins=nbins)
            msgs.append(f"{shape} nbins {nbins}: returned")
        except ValueError as e:
            msgs.append(f"{shape} nbins {nbins}: ValueError")
            if shape in ((47, 100, 3), (3, 120, 160)):
                print(f"    e.g. {e}")
            if shape == (3, 120, 160) and "channel-first" not in str(e):
                msgs[-1] += " without the channel-first hint"
        except Exception as e:  # noqa: BLE001
            msgs.append(f"{shape} nbins {nbins}: {type(e).__name__}")
    return ok & check("genuine_noise_curve rejects bad input with ValueError: " + ", ".join(msgs),
                      all(m.endswith("ValueError") for m in msgs))


def test_end_to_end(run):
    pix = GenuinePixnoise(run)
    ok = True
    for name, rgb in synthetic_images():
        ok &= compare_curve(run, pix, name, rgb)
    return ok


def test_clip_frames(run, path, frames):
    import nvfilter as nv
    from denoise_video_v5 import probe, read_frames
    pix = GenuinePixnoise(run)
    info = probe(path)
    want = sorted(int(v) for v in frames.split(","))
    mat = nv.matrix_from_tags(info["tags"], info["H"])
    ok = True
    for i, pl in read_frames(path, info["W"], info["H"], want[0], want[-1] + 1, fps=info["fps"]):
        if i in want:
            pl = tuple(np.asarray(p, F32) for p in pl)
            rgb = np.clip(nv.yuv_to_rgb(pl, mat, info["full_range"]), 0, 255).astype(F32)
            ok &= compare_curve(run, pix, f"{os.path.basename(path)} n={i} [{mat}]", rgb)
            _, gy = genuine_curve(run, rgb)[1:]
            print("    genuine curve", np.round(gy, 4).tolist())
    return ok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--clip")
    ap.add_argument("--frames", default="312")
    ap.add_argument("--no-genuine", action="store_true", help="unit tests only (no harness)")
    A = ap.parse_args()
    test_accumulate()
    test_curve_units()
    test_clip_weight_rule()
    test_shell_batch(A.clip, A.frames)
    if not A.no_genuine:
        run = _nvh()
        test_tiles(run)
        test_isclip(run)
        test_isclip_extreme(run)
        test_clip_weight(run)
        test_interp()
        test_end_to_end(run)
        if A.clip:
            test_clip_frames(run, A.clip, A.frames)
    n_ok = sum(1 for _, ok in RESULTS if ok)
    print(f"\n{n_ok}/{len(RESULTS)} checks passed")
    sys.exit(0 if n_ok == len(RESULTS) else 1)


if __name__ == "__main__":
    main()
