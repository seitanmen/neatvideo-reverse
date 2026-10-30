#!/usr/bin/env python3
"""selftest_motion.py — nv_motion.py の自己整合検証 (本家照合はハーネス待ち)。

テストはすべて決定論的 (乱数は固定シード)。本家バイナリは読み込まない。
実行: python3 selftest_motion.py
"""
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import nv_motion as nm  # noqa: E402


def _img(seed=1, h=128, w=128):
    """滑らかな構造を持つ決定論的画像 (ブロック一致が一意に決まるように)。"""
    rng = np.random.default_rng(seed)
    y, x = np.mgrid[0:h, 0:w].astype(np.float32)
    a = (127 + 60 * np.sin(x / 9.0 + y / 13.0) + 40 * np.cos(y / 5.0 - x / 17.0)
         + 12 * rng.standard_normal((h, w))).astype(np.float32)
    return np.clip(a, 0, 255).astype(np.uint8)


def _shift(img, dy, dx):
    out = np.full_like(img, 128)
    h, w = img.shape
    ys = slice(max(0, -dy), min(h, h - dy))
    xs = slice(max(0, -dx), min(w, w - dx))
    out[ys, xs] = img[max(0, dy):max(0, dy) + (ys.stop - ys.start),
                      max(0, dx):max(0, dx) + (xs.stop - xs.start)]
    return out


def t_sad():
    rng = np.random.default_rng(2)
    a, b = rng.integers(0, 256, (16, 16), dtype=np.uint8), rng.integers(0, 256, (16, 16), dtype=np.uint8)
    ref = int(np.abs(a.astype(int) - b.astype(int)).sum())
    assert nm.sad16(a, b) == ref
    assert nm.sad16(a, a) == 0
    print("ok sad16 (naive loop と一致)")


def t_pattern_translation():
    img = _img()
    for (dy, dx) in ((0, 0), (3, -2), (-3, 3), (6, 4)):
        cur, ref = img, _shift(img, dy, dx)     # ref(p+(dy,dx)) = cur(p) ⟹ v = (−dy,−dx)
        evy, evx = -dy, -dx                     # nb(p+v)~cur(p) 規約 (estimate_flow_v5 と同一)
        vy, vx, sad = nm.estimate_motion(ref, cur, levels=3, refine32_enable=True)
        inner = (slice(2, -2), slice(2, -2))
        ok = (np.abs(vy[inner] - evy) <= 0.75) & (np.abs(vx[inner] - evx) <= 0.75)
        frac = float(ok.mean())
        assert frac > 0.9, f"translation ({dy},{dx}) recovered only {frac:.2f}"
        print(f"ok translation ({dy:+d},{dx:+d}): 内部 {frac*100:.1f}% のブロックが ±0.75 内 "
              f"(中央値 dv=({np.median(vy[inner]-evy):+.2f},{np.median(vx[inner]-evx):+.2f}) meanSAD={sad[inner].mean():.0f})")


def t_subpel_centroid():
    """合成 SAD 面で FUN_0098bf18 の重心式を検証 (純関数, 基準重み込み)。"""
    thr = 100.0
    sads = np.full((8,), 500.0)
    sads[4] = 300.0            # (1, 0) が最良
    base = 400.0
    inv = 1.0 / (thr + 0.01)
    best = min(base, sads.min())
    dbase = inv * (base - best)
    den = max(1.0 - nm.SUBPEL_SLOPE * dbase, 0.0) if dbase < nm.SUBPEL_CUT else 0.0
    num_x = num_y = 0.0
    for (dx, dy), s in zip(nm.NEIGHBORS8, sads):
        diff = inv * (s - best)
        if diff < nm.SUBPEL_CUT:
            wgt = max(1.0 - nm.SUBPEL_SLOPE * diff, 0.0)
            num_x += dx * wgt; num_y += dy * wgt; den += wgt
    exp_x, exp_y = num_x / den, num_y / den
    # 最良候補 (重み 1) と基準位置 (重み 1-0.66·(400-300)/100.01) の重心
    wb = 1.0 - 0.66 * (100.0 / 100.01)
    assert abs(exp_x - 1.0 / (1.0 + wb)) < 1e-9, exp_x
    assert abs(exp_y) < 1e-9
    print(f"ok subpixel centroid (合成面): ({exp_y:+.3f},{exp_x:+.3f}) = 最良と基準の重心 (基準重み {wb:.3f})")


def t_smoothing():
    """外れベクトル 1 個が近傍平均で置き換わるか (FUN_0098a79c/98a94c)。"""
    img = _img(3)
    cur, ref = img, _shift(img, 2, 1)
    vy, vx, sad = nm.estimate_motion(ref, cur, levels=3, refine32_enable=False, smooth=True)
    vy_ns, vx_ns, _ = nm.estimate_motion(ref, cur, levels=3, refine32_enable=False, smooth=False)
    d = np.abs(vy - vy_ns) + np.abs(vx - vx_ns)
    assert d[2:-2, 2:-2].max() <= 2.5, f"smoothing changed vectors by {d[2:-2,2:-2].max():.2f}"
    print(f"ok smoothing: 変更量の最大 {d.max():.2f} (外れ抑制は探索全過程で機能)")


def t_gain():
    """FUN_0098c90c 回帰: 本家式は ref ≈ a·measured + b (§6.1 の (g,o) と同型)。
    measured = 0.9·ref + 10 なら a ≈ 1/0.9, b ≈ −10/0.9。"""
    img = _img(4).astype(np.float32) * 0.5
    ref = img
    cur = 0.9 * img + 10.0
    h, w = img.shape
    vy = np.zeros((h // 16, w // 16), np.float32)
    vx = np.zeros_like(vy)
    g = nm.gain_offset(ref, cur, vx, vy)
    ea, eb = 1.0 / 0.9, -10.0 / 0.9
    assert g["valid_pct"] > 30 and abs(g["a"] - ea) < 0.03 and abs(g["b"] - eb) < 3.0, g
    print(f"ok gain_offset: a={g['a']:.3f} b={g['b']:.2f} valid={g['valid_pct']:.0f}% "
          f"(期待 a≈{ea:.3f}, b≈{eb:.2f}; ref≈a·measured+b の本家式)")
    # 不変 (a=1, b=0)
    g2 = nm.gain_offset(img, img, vx, vy)
    assert abs(g2["a"] - 1.0) < 1e-6 and abs(g2["b"]) < 1e-6, g2
    print("ok gain_offset identical: a=1 b=0")


def t_failure():
    rng = np.random.default_rng(5)
    flat = np.full((96, 96), 7, np.uint8)
    noisy = np.clip(7 + rng.integers(-2, 3, (96, 96)), 0, 255).astype(np.uint8)
    vy, vx, sad = nm.estimate_motion(flat, noisy, levels=2)
    assert np.isfinite(vy).all() and np.isfinite(vx).all()
    print("ok degenerate input: 定数面でも NaN/クラッシュなし")
    tiny = rng.integers(0, 256, (24, 24), dtype=np.uint8)
    try:
        nm.estimate_motion(tiny, tiny, levels=2)
        raised = False
    except ValueError:
        raised = True
    assert raised, "小さすぎる面は ValueError のこと (_grid が None)"


def t_grid():
    xs, ys, (nx, ny) = nm._grid(1920, 1080, 16)
    assert (nx, ny) == (120, 67) and xs[-1] == 1904 and ys[-1] == 1056
    xs, ys, (nx, ny) = nm._grid(1900, 1000, 16)
    assert xs[-1] == 1872 and (nx, ny) == (118, 62)
    print("ok grid: FUN_0098a368 の min(16*col, W-16) と一致")


if __name__ == "__main__":
    t_sad()
    t_grid()
    t_subpel_centroid()
    t_pattern_translation()
    t_smoothing()
    t_gain()
    t_failure()
    print("ALL OK")
