#!/usr/bin/env python3
"""nv_motion.py — Neat Video v6 MotionCompensator の移植 (WP motion, 2026-10-17).

解読の根拠は ../notes/decode_motion.md (Ghidra アドレスつき)。各関数の docstring に
**O** (逆コンパイル・バイナリからの観測) と **E** (推定) を区別して記す。
部品の SAD (FUN_009f862c/009f9290) と FUN_0098bf18 (refine32) は nvh sad / nvh b32 と
ビット照合済み (../verify/verify_motion.py)。他は自己整合 (../src/selftest_motion.py)。

構成 (本家の関数 ↔ 本ファイル):
  FUN_0097f020/0097ef54  余白の複製   -> _pad
  FUN_00986d90           2x2 平均縮小 -> _down2
  FUN_009f862c           16x16 8bit SAD -> _sad_blocks / sad16
  FUN_0098b9a4           パターン探索   -> pattern_search
  FUN_0098bf18           32x32 + サブピクセル重心 -> refine32
  FUN_0098adbc           候補検証と受理 -> analyze_level
  FUN_009896fc           レベル間伝播   -> propagate
  FUN_0098a79c/98a94c/98ab5c 平滑化    -> smooth_level0
  FUN_0098a368           格子初期化     -> _grid
  FUN_0098c90c           ゲイン/オフセット回帰 -> gain_offset
  探索全体 (CompensateImageSet 系)     -> estimate_motion

ベクトルの向きの約束: estimate_motion(ref, cur) は「cur のブロック p に対応する ref の
位置が p + v」であるような v を返す (nvfilter.estimate_flow_v5 と同じ
neighbour(p + v) ~ centre(p))。**E** (本家 98a94c の SAD 呼び出しの地址計算からは
向きは確定できない; v5 との差し替え比較に必要な向きで定義した)。
"""
from __future__ import annotations

import numpy as np
from numpy.lib.stride_tricks import sliding_window_view

# ---------------------------------------------------------------------------
# 定数 (O: バイナリから読んだ値)
# ---------------------------------------------------------------------------

# DAT_014877f8..01487807: int8[16] = 8 近傍 (dx, dy) (O)
NEIGHBORS8 = np.array([(-1, -1), (0, -1), (1, -1),
                       (-1, 0),           (1, 0),
                       (-1, 1),  (0, 1),  (1, 1)], np.int32)
SAD_NONE = 0x7fffffff          # 本家の「測定なし」の値 (O)
MARGIN = 8                     # 面の余白サンプル数 (FUN_0097eebc margin=8) (O)
BLOCK = 16                     # ブロック格子の間隔 (FUN_0098a368) (O)
# DAT_01244888: 回帰のフォールバック {1.0, 0.0, 3138.0, ...} (O; 4 語目は大きい値)
REGRESSION_FALLBACK = (1.0, 0.0, 3138.0, 3138.0)
# FUN_0098bf18 のサブピクセル重心の定数 (O)
SUBPEL_SLOPE = 0.66
SUBPEL_CUT = 1.5151515         # = 1/0.66 (O)


# ---------------------------------------------------------------------------
# 面とピラミッド
# ---------------------------------------------------------------------------

def _pad(a: np.ndarray, m: int = MARGIN) -> np.ndarray:
    """FUN_0097f020/0097ef54: 上下左右と四隅を端の値で複製した余白 (O)。"""
    return np.pad(a, m, mode="edge")


def _down2(a: np.ndarray) -> np.ndarray:
    """FUN_00986d90: 2x2 平均の縮小 (O)。本家は int16 面に対し short 演算。
    移植は 8bit 面を想定し rint で丸める (E: 探索面の要素型は未確定、decode_motion.md §6.1)。"""
    h, w = a.shape[0] // 2 * 2, a.shape[1] // 2 * 2
    q = a[:h, :w].reshape(h // 2, 2, w // 2, 2).mean((1, 3), dtype=np.float32)
    return np.clip(np.rint(q), 0, 255).astype(np.uint8)


def _grid(w: int, h: int, bs: int = BLOCK):
    """FUN_0098a368: x0 = min(bs*col, W-bs) の格子 (O)。余白込みの面に対して使う。"""
    if w < bs or h < bs:
        return None, None, None
    nx, ny = (w - bs) // bs + 1, (h - bs) // bs + 1
    xs = np.minimum(bs * np.arange(nx), w - bs)
    ys = np.minimum(bs * np.arange(ny), h - bs)
    return xs.astype(np.int32), ys.astype(np.int32), (nx, ny)


# ---------------------------------------------------------------------------
# SAD (FUN_009f862c = 16x16 8bit SAD / FUN_009f9290 = 32x32)
# ---------------------------------------------------------------------------

def sad16(a: np.ndarray, b: np.ndarray) -> int:
    """FUN_009f862c: 8bit ブロック SAD = Σ|a-b| (O)。a, b は同じ形状の 2 次元断片。"""
    return int(np.abs(a.astype(np.int32) - b.astype(np.int32)).sum())


def _sad_all(win_cur, win_ref, ys, xs, dys, dxs, bs):
    """全ブロック一括 SAD。ys/xs はブロック位置・dys/dxs は ref 側変位 (同じ形状の
    メッシュ、余白込み座標)。win_* は sliding_window_view(面, (bs, bs))。
    ref 窓が [0, H+2M-bs] を出る範囲は SAD_NONE (FUN_0098b9a4 の
    画像座標 [-8, W+8-bs] 検査と同一、O)。"""
    ry, rx = ys + dys, xs + dxs
    h, w = win_ref.shape[:2]
    ok = (ry >= 0) & (rx >= 0) & (ry <= h - 1) & (rx <= w - 1)
    ry = np.clip(ry, 0, h - 1)
    rx = np.clip(rx, 0, w - 1)
    d = np.abs(win_cur[ys, xs].astype(np.int32)
               - win_ref[ry, rx].astype(np.int32))
    s = d.sum((-2, -1), dtype=np.int64)
    return np.where(ok, s, SAD_NONE)


def _windows(padded: np.ndarray, bs: int, step: int = BLOCK):
    """余白込み面から (窓 view, ブロック x/y 位置) を作る。位置は余白込み座標で
    FUN_0098a368 と同じ min(bs*col, W-bs) 格子 (O)。"""
    h, w = padded.shape
    ny = (h - 2 * MARGIN - bs) // step + 1
    nx = (w - 2 * MARGIN - bs) // step + 1
    ys = (MARGIN + step * np.arange(ny)).astype(np.int32)
    xs = (MARGIN + step * np.arange(nx)).astype(np.int32)
    return sliding_window_view(padded, (bs, bs)), xs, ys


# ---------------------------------------------------------------------------
# パターン探索 (FUN_0098b9a4)
# ---------------------------------------------------------------------------

def pattern_search(win_cur, win_ref, y0, x0, vy, vx, bs=BLOCK, rounds=(2, 1)):
    """FUN_0098b9a4: 現在位置の 8 近傍を評価し最良へ 1 回ずつ動く (O)。
    rounds の各要素 r はオフセット表を r 倍する (本家は ×2 (粗) → ×1 (細)、O)。
    y0/x0 は中心ブロック位置 (余白込み)、vy/vx は初期変位 (サンプル単位、float 可)。
    返り値: (best_y, best_x, sad) — 位置は「ref 側窓位置」(中心位置+変位)。"""
    by = y0 + np.rint(vy).astype(np.int32)
    bx = x0 + np.rint(vx).astype(np.int32)
    sad = _sad_all(win_cur, win_ref, y0, x0, by - y0, bx - x0, bs)
    for r in rounds:
        cand_y = np.empty((len(NEIGHBORS8),) + by.shape, np.int32)
        cand_x = np.empty_like(cand_y)
        cand_s = np.empty((len(NEIGHBORS8),) + sad.shape, np.int64)
        for i, (dx, dy) in enumerate(NEIGHBORS8):
            cand_y[i] = by + r * dy
            cand_x[i] = bx + r * dx
            cand_s[i] = _sad_all(win_cur, win_ref, y0, x0, cand_y[i] - y0, cand_x[i] - x0, bs)
        for i in range(len(NEIGHBORS8)):                    # 位置ごとの min (スキャン順は同順)
            better = cand_s[i] < sad
            sad = np.where(better, cand_s[i], sad)
            by = np.where(better, cand_y[i], by)
            bx = np.where(better, cand_x[i], bx)
    return by, bx, sad


# ---------------------------------------------------------------------------
# 32x32 探索とサブピクセル重心 (FUN_0098bf18)
# ---------------------------------------------------------------------------

def refine32(win32_cur, win32_ref, y0, x0, dys, dxs, thr):
    """FUN_0098bf18 (O、逆アセンブル 0x98bf18〜0x98c774 を命令単位で読んだ):
    32x32 SAD の 8 近傍 + 基準位置の最小 best に対する SAD 重み付き重心。
    演算はすべて float32 の単独演算 (FMA なし、double なし):
      inv  = 1.0f / (thr + 0.01f)                      (fadd → fdiv、0x98c3f4〜0x98c408)
      d_i  = inv * (float)(SAD_i − best)                (ucvtf は 64 ビット符号なし → f32、fmul)
      w_i  = (d_i * −0.66f) + 1.0f   (d_i < 1.5151515f のときのみ、fmul → fadd。下限の clamp なし)
      den  = (w_base + 0.0f) から w_0..w_7 を順に fadd、num = (0,0) に (float)(dx,dy)·w_i を順に fadd
      out  = (num / den) + (float)位置                (fdiv → fadd、0x98c708〜0x98c728)
    範囲外の近傍は SAD = 0x7fffffff のまま式に入る (inv が小さければ打切りで除外)。
    基準位置が範囲外なら (位置そのまま, SAD_NONE) (O、0x98c3a0)。
    win32_* は 32x32 窓 (余白込み座標)。返り値: (fdy, fdx, best_sad, base_sad)。
    fdy/fdx は float64 の変位で、dys + (out − 画像座標) (out は f32 の絶対位置。差は double で厳密)。"""
    f32 = np.float32
    bs = 32
    h, w = win32_ref.shape[:2]
    pos_y, pos_x = y0 + dys, x0 + dxs
    inb = (pos_y >= 0) & (pos_x >= 0) & (pos_y <= h - 1) & (pos_x <= w - 1)
    base = _sad_all(win32_cur, win32_ref, y0, x0, dys, dxs, bs).astype(np.int64)
    sads = np.empty((8,) + base.shape, np.int64)
    for i, (dx, dy) in enumerate(NEIGHBORS8):
        sads[i] = _sad_all(win32_cur, win32_ref, y0, x0, dys + dy, dxs + dx, bs)
    best = np.minimum(base, sads.min(0))
    ok = inb & (best != SAD_NONE)
    with np.errstate(invalid="ignore", divide="ignore", over="ignore"):
        inv = f32(1.0) / (f32(thr) + f32(0.01))
        slope = f32(-SUBPEL_SLOPE)
        cut = f32(SUBPEL_CUT)
        one = f32(1.0)
        d = inv * (base - best).astype(f32)                     # ucvtf → f32、fmul
        den = np.zeros(base.shape, f32)
        num_y = np.zeros(base.shape, f32)
        num_x = np.zeros(base.shape, f32)
        for i, (dx, dy) in enumerate(NEIGHBORS8):
            d = inv * (sads[i] - best).astype(f32)
            take = d < cut
            wgt = (d * slope + one).astype(f32)                 # fmul → fadd (別々に丸め)
            num_x = np.where(take, num_x + f32(dx) * wgt, num_x)
            num_y = np.where(take, num_y + f32(dy) * wgt, num_y)
            den = np.where(take, den + wgt, den)
        # 画像座標 = 余白込み窓座標 − MARGIN
        img_y = (pos_y - MARGIN).astype(f32)
        img_x = (pos_x - MARGIN).astype(f32)
        out_y = (num_y / den) + img_y
        out_x = (num_x / den) + img_x
    fdy = np.where(ok, dys + (out_y.astype(np.float64) - (pos_y - MARGIN)), dys).astype(np.float64)
    fdx = np.where(ok, dxs + (out_x.astype(np.float64) - (pos_x - MARGIN)), dxs).astype(np.float64)
    return fdy, fdx, np.where(inb, best, SAD_NONE), np.where(inb, base, SAD_NONE)


# ---------------------------------------------------------------------------
# 候補の検証と受理 (FUN_0098adbc)
# ---------------------------------------------------------------------------

def analyze_level(cur_p, ref_p, vx, vy, cands=None, weight=1.0,
                  refine32_enable=True, refine32_every=16, thr_scale=1.1):
    """FUN_0098adbc (O):
    1) 記録のベクトル位置から丸め位置を作りパターン探索 (×2, ×1 の 2 ラウンド)。
    2) 親レベルの 4 候補を max(|dx|,|dy|)>=4 のものだけ SAD 検証 (境界内)。
    3) 最良候補の位置からもう一度パターン探索。
    4) 受理: 2*weight + SAD_sub < SAD_first なら置き換え (O)。
    5) (条件付き) 32x32 枝: 本家は 16 個に 1 個のブロックで FUN_0098bf18 を呼び、
       SAD は >>2 で 16x16 換算に戻し +0x40=1 を立てる (O)。入り口に難読化ゲートが
       ありハーネス条件では実行されない (推定; decode_motion.md §6.3)。
       refine32_enable=False でオフにできる。
    vx/vy/cands は (ny, nx) の float 配列。返り値: (vx, vy, sad, slack, r32flag)。"""
    bs = BLOCK
    win_cur, xs, ys = _windows(cur_p, bs)
    win_ref, _, _ = _windows(ref_p, bs)
    y0 = np.repeat(ys[:, None], xs.shape[0], 1)
    x0 = np.repeat(xs[None, :], ys.shape[0], 0)
    by, bx, sad = pattern_search(win_cur, win_ref, y0, x0, vy, vx, bs)
    sad_first = sad.copy()
    if cands is not None:
        best_c = sad.copy()
        by_c, bx_c = by.copy(), bx.copy()
        for cx, cy in cands:               # 4 候補 (O)
            far = np.maximum(np.abs(cx - (bx - x0)), np.abs(cy - (by - y0))) >= 4
            cyi, cxi = np.rint(cy).astype(np.int32), np.rint(cx).astype(np.int32)
            s = _sad_all(win_cur, win_ref, y0, x0, cyi, cxi, bs)
            take = far & (s < best_c)
            best_c = np.where(take, s, best_c)
            by_c, bx_c = np.where(take, np.clip(cyi, -MARGIN, win_ref.shape[0] - 1), by_c), \
                         np.where(take, np.clip(cxi, -MARGIN, win_ref.shape[1] - 1), bx_c)
            # 注: _sad_all が範囲外を SAD_NONE にするため take は自動的に有効位置のみ
        by2, bx2, sad2 = pattern_search(win_cur, win_ref, y0, x0,
                                        by_c - y0, bx_c - x0, bs)
        accept = (2 * weight + sad2) < sad_first            # (O) 2w + SAD_sub < SAD_first
        sad = np.where(accept, sad2, sad_first)
        by = np.where(accept, by2, by)
        bx = np.where(accept, bx2, bx)
    slack = np.zeros(sad.shape, np.float32)
    r32 = np.zeros(sad.shape, bool)
    if refine32_enable:
        sel = (np.arange(sad.size).reshape(sad.shape) % refine32_every) == 0
        if sel.any():
            win32_cur, _, _ = _windows(cur_p, 32)
            win32_ref, _, _ = _windows(ref_p, 32)
            thr = weight * thr_scale                    # 呼び出し元の weight*1.1 (O)
            # 32x32 窓の錨はブロック位置 -8 (FUN_0098a94c と同じ、O)
            fdy, fdx, best32, _ = refine32(win32_cur, win32_ref, y0 - 8, x0 - 8,
                                           by - y0, bx - x0, thr)
            ok = (best32 != SAD_NONE) & sel
            sad = np.where(ok, (best32 / 4.0), sad)     # SAD >>2 (O)
            vy_f = np.where(ok, fdy, by - y0).astype(np.float32)
            vx_f = np.where(ok, fdx, bx - x0).astype(np.float32)
            r32 = ok
            slack = np.where(ok, thr, 0.0).astype(np.float32)
            return vx_f, vy_f, sad, slack, r32
    return (bx - x0).astype(np.float32), (by - y0).astype(np.float32), sad, slack, r32


# ---------------------------------------------------------------------------
# レベル間伝播 (FUN_009896fc) と平滑化 (FUN_0098a79c/98a94c/98ab5c)
# ---------------------------------------------------------------------------

def propagate(vx, vy, nx, ny):
    """FUN_009896fc (O): 細かいレベルの各ブロックに親レベルの 2x2 近傍から
    4 候補 = 2*親ベクトル を置き、現在ベクトルを (9,3,3,1)/16 の平均 (2x 済み) にする。
    親の対応 (E): 子 (r,c) ← 親 (r>>1, c>>1) を最近傍、(+1 行/列) を遠傍 (クランプ)。"""
    py, px = vy.shape
    pr = np.clip(np.arange(ny) >> 1, 0, py - 1)
    pc = np.clip(np.arange(nx) >> 1, 0, px - 1)
    pr1, pc1 = np.clip(pr + 1, 0, py - 1), np.clip(pc + 1, 0, px - 1)
    a = (vy[pr][:, pc], vx[pr][:, pc])          # (r>>1,     c>>1)     重み 9
    b = (vy[pr][:, pc1], vx[pr][:, pc1])        # (r>>1,     c>>1+1)  重み 3
    c = (vy[pr1][:, pc], vx[pr1][:, pc])        # (r>>1+1,   c>>1)     重み 3
    d = (vy[pr1][:, pc1], vx[pr1][:, pc1])      # (r>>1+1,   c>>1+1)   重み 1
    cands = [(2 * q[1], 2 * q[0]) for q in (a, b, c, d)]    # 4 候補 = 2*親 (O)
    mvy = (9 * a[0] + 3 * b[0] + 3 * c[0] + d[0]) / 16.0    # (9,3,3,1)/16 (O)
    mvx = (9 * a[1] + 3 * b[1] + 3 * c[1] + d[1]) / 16.0
    return cands, mvy.copy(), mvx.copy()


def neighbor_mean(vx, vy, sad):
    """FUN_0098a79c (O): 5x5 近傍のうち |dvx|<=2 かつ |dvy|<=2 かつ SAD 有効
    (SAD_NONE は float として NaN 相当 = 除外、O) の平均。4 方向とも有効数 >3 が必要。
    失敗時は None (呼び出し元は現在ベクトルを維持)。"""
    ny, nx = vx.shape
    acc_x = np.zeros((ny, nx), np.float64)
    acc_y = np.zeros_like(acc_x)
    cnt = np.zeros((ny, nx), np.int32)
    l = np.zeros((ny, nx), np.int32); r = np.zeros((ny, nx), np.int32)
    u = np.zeros((ny, nx), np.int32); dn = np.zeros((ny, nx), np.int32)
    for ddy in range(-2, 3):
        for ddx in range(-2, 3):
            if ddy == 0 and ddx == 0:
                continue
            sy, sx = slice(max(0, ddy), ny + min(0, ddy)), slice(max(0, ddx), nx + min(0, ddx))
            ty, tx = slice(max(0, -ddy), ny + min(0, -ddy)), slice(max(0, -ddx), nx + min(0, -ddx))
            ok = (sad[sy, sx] != SAD_NONE) & \
                 (np.abs(vx[sy, sx] - vx[ty, tx]) <= 2.0) & \
                 (np.abs(vy[sy, sx] - vy[ty, tx]) <= 2.0)
            acc_x[ty, tx] += np.where(ok, vx[sy, sx], 0.0)
            acc_y[ty, tx] += np.where(ok, vy[sy, sx], 0.0)
            cnt[ty, tx] += ok
            if ddx < 0:
                l[ty, tx] += ok
            elif ddx > 0:
                r[ty, tx] += ok
            if ddy < 0:
                u[ty, tx] += ok
            elif ddy > 0:
                dn[ty, tx] += ok
    good = (cnt > 0) & (l > 3) & (r > 3) & (u > 3) & (dn > 3)
    with np.errstate(invalid="ignore", divide="ignore"):
        mx = np.where(good, acc_x / np.maximum(cnt, 1), np.nan)
        my = np.where(good, acc_y / np.maximum(cnt, 1), np.nan)
    return my, mx, good


def smooth_level0(cur_p, ref_p, vx, vy, sad, slack, scales=(1.0, 2.0)):
    """FUN_0098ab5c (O): scale 1.0 で平滑化 → +0x30/+0x34 を +0x08/+0x0c へコピー →
    scale 2.0 でもう一度。受理 (FUN_0098a94c、O):
      SAD32(平滑化位置) <= slack*scale + (SAD<<2)。
    slack は 32x32 枝が立てた値 (+0x38)、SAD<<2 は 32x32 換算に戻す (O)。"""
    bs = 32
    win_cur, _, _ = _windows(cur_p, bs)          # 32x32 窓 (SAD 取得用)
    win_ref, _, _ = _windows(ref_p, bs)
    xs16, ys16, _ = _grid(cur_p.shape[1] - 2 * MARGIN, cur_p.shape[0] - 2 * MARGIN, BLOCK)
    y0 = np.repeat(ys16[:, None], xs16.shape[0], 1)   # 全ブロックの 16 格子
    x0 = np.repeat(xs16[None, :], ys16.shape[0], 0)
    for sc in scales:
        my, mx, good = neighbor_mean(vx, vy, sad)
        if not good.any():
            continue
        cy = np.rint(np.where(good, my, vy)).astype(np.int32)
        cx = np.rint(np.where(good, mx, vx)).astype(np.int32)
        s32 = _sad_all(win_cur, win_ref, y0 - 8, x0 - 8, cy, cx, bs)   # 錨は -8 (O)
        accept = good & (s32 <= slack.astype(np.float64) * sc + (sad.astype(np.float64) * 4.0))
        vy = np.where(accept, np.where(good, my, vy), vy).astype(np.float32)
        vx = np.where(accept, np.where(good, mx, vx), vx).astype(np.float32)
    return vx, vy


# ---------------------------------------------------------------------------
# ゲイン/オフセット回帰 (FUN_0098c90c)
# ---------------------------------------------------------------------------

def gain_offset(ref, cur, vx, vy, win=32, min_valid_pct=30.0):
    """FUN_0098c90c の回帰部 (O): ブロックごとに補償位置の win×win 窓の平均を
    ref 側 (x) と cur 側 (y) で取り、y ≈ a*x + b を最小二乗で当てる。
      a = (nΣxy - ΣyΣx)/(nΣyy - Σy²) (下限 0.66)、b = (Σx - aΣy)/n (下限 -64) (O)
      有効率 > 30% かつ分母 ≠ 0 のときのみ。それ以外は DAT_01244888 = (1, 0, 3138, …) (O)。
    変位はサブピクセル可 (4 隅の整数窓平均の双一次ブレンド、E)。
    返り値: dict(a, b, valid_pct, res_before, res_after)。"""
    ref = np.asarray(ref, np.float32)
    cur = np.asarray(cur, np.float32)
    wr = sliding_window_view(_pad(ref), (win, win)).mean((-2, -1))
    wc = sliding_window_view(_pad(cur), (win, win)).mean((-2, -1))
    ny, nx = vx.shape
    iy, ix = MARGIN - 8 + BLOCK * np.arange(ny), MARGIN - 8 + BLOCK * np.arange(nx)
    fy = iy[:, None] + vy
    fx = ix[None, :] + vx
    y0f, x0f = np.floor(fy).astype(np.int32), np.floor(fx).astype(np.int32)
    ty, tx = fy - y0f, fx - x0f
    h, w = wr.shape
    y0c, x0c = np.clip(y0f, 0, h - 1), np.clip(x0f, 0, w - 1)
    y1c, x1c = np.minimum(y0c + 1, h - 1), np.minimum(x0c + 1, w - 1)
    m_ref = (wr[y0c, x0c] * (1 - ty) * (1 - tx) + wr[y1c, x0c] * ty * (1 - tx)
             + wr[y0c, x1c] * (1 - ty) * tx + wr[y1c, x1c] * ty * tx)
    m_cur = wc[iy[:, None], ix[None, :]]
    valid = (m_ref > -1e30) & np.isfinite(m_ref) & np.isfinite(m_cur)
    n = int(valid.sum())
    pct = 100.0 * n / max(vx.size, 1)
    out = dict(a=REGRESSION_FALLBACK[0], b=REGRESSION_FALLBACK[1], valid_pct=pct,
               res_before=REGRESSION_FALLBACK[2], res_after=REGRESSION_FALLBACK[3])
    if n == 0 or pct <= min_valid_pct:
        return out
    x, y = m_ref[valid].astype(np.float64), m_cur[valid].astype(np.float64)
    sxx, sxy, syy = float((x * x).sum()), float((x * y).sum()), float((y * y).sum())
    sx, sy = float(x.sum()), float(y.sum())
    den = n * syy - sy * sy
    if den == 0:
        return out
    a = (n * sxy - sy * sx) / den
    b = (sx - a * sy) / n
    a = max(a, 0.66)
    b = max(b, -64.0)
    out.update(a=a, b=b,
               res_before=float(np.abs(y - x).mean()),
               res_after=float(np.abs((b + a * y) - x).mean()))
    return out


# ---------------------------------------------------------------------------
# 全体 (CompensateImageSet 系のドライバ)
# ---------------------------------------------------------------------------

def estimate_motion(ref, cur, levels=3, bs=BLOCK, weight=1.0,
                    refine32_enable=True, refine32_every=16, smooth=True):
    """3 レベルのピラミッド BM (E: レベル数は実行時設定、本家では未確定)。
    粗いレベルで探索 → propagate で候補 → 細かいレベルで検証 → レベル 0 で平滑化。
    返り値: (vy, vx, sad) — すべて (ny, nx)。v は「cur のブロック p ↔ ref の p+v」(E)。
    v5 の estimate_flow_v5 と同じ呼び出し規約。"""
    ref = np.clip(np.rint(np.asarray(ref, np.float32)), 0, 255).astype(np.uint8)
    cur = np.clip(np.rint(np.asarray(cur, np.float32)), 0, 255).astype(np.uint8)
    rc, cc = [_pad(ref)], [_pad(cur)]
    for _ in range(levels - 1):
        rc.insert(0, _pad(_down2(rc[0][MARGIN:-MARGIN, MARGIN:-MARGIN])))
        cc.insert(0, _pad(_down2(cc[0][MARGIN:-MARGIN, MARGIN:-MARGIN])))
    vx = vy = None
    for lvl in range(levels):
        cur_p, ref_p = cc[lvl], rc[lvl]
        g = _grid(cur_p.shape[1] - 2 * MARGIN, cur_p.shape[0] - 2 * MARGIN, bs)
        if g[0] is None:
            raise ValueError("frame too small for the 16-sample block grid")
        xs, ys, (nx, ny) = g
        if lvl == 0:
            cands, vy0, vx0 = None, np.zeros((ny, nx), np.float32), np.zeros((ny, nx), np.float32)
        else:
            cands, vy0, vx0 = propagate(vx, vy, nx, ny)
        vx, vy, sad, slack, _ = analyze_level(
            cur_p, ref_p, vx0, vy0, cands, weight=weight,
            refine32_enable=refine32_enable, refine32_every=refine32_every)
    if smooth:
        vx, vy = smooth_level0(cc[-1], rc[-1], vx, vy, sad, slack)
    return vy, vx, sad.astype(np.float64)


def flow_for_v5(ref, cur, bs=16, search=16, **kw):
    """nvfilter.estimate_flow_v5 の代替 (差し替え用アダプタ)。
    temporal_v5 は (vy, vx) の block grid を期待する。**E**: 本家のゲイン適用を
    除き、ベクトルだけを差し替える。"""
    vy, vx, _ = estimate_motion(ref, cur, **kw)
    return vy, vx
