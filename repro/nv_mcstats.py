"""FUN_0098718c: the MotionCompensator statistics BuildNoiseModel stores at profile + 0xbea8 (32 floats), ported
from the disassembly (sandbox/dirfilt; integer arithmetic except the final divisions).

    mc_stats(R, G, B) -> float32[32]   (out[0:16] 16 x 16 blocks, out[16:32] 32 x 32 blocks, per pyramid level)

1. 16-bit grey of level 0 (FUN_009f854c): rint(((G + (G + R)) + B) * 4), saturated (NEON and tail agree);
   levels l = 1.. by 2 x 2 means (FUN_00986d90 / FUN_009fb114: arithmetic >> 2 on the first (W_l & ~7) columns,
   logical >> 2 of the uint32 sum on the rest); a level exists while the previous one had W, H >= 32 (at most 16).
2. per level the u8 high-pass (FUN_00984d60 "Pyramid.LFHorHalf" + FUN_00984f7c "LFVerHalfDiffToChar"):
   T = (sum of 17 horizontally mirrored values * floor(2^32 / 17)) >> 32, then the same vertically on T (mirrored),
   U = clamp((LP - X + 0x800) >> 4, 0, 255).
3. R = U rotated by 180 degrees (FUN_0097f4a4 / f5e0 / f6cc).  2000 samples of the PRNG (seed 0x7495f1a5) give a block
   of U at (a, b) and a block of R at (c, d); SAD (FUN_009f862c 16 x 16 / FUN_009f9290 32 x 32) at the centre and the
   minimum over the valid 5 x 5 (16) / 3 x 3 (32) neighbourhood; with >= 4 valid positions acc += centre - min, n += 1,
   stop at n = 1000.  out = acc / n (scvtf, fdiv); with fewer samples, too small a level (< 21 / < 37) or no level:
   the previous level's value * 0.5 (out32[0] = out16[0] * 1.4 when the 32 stage of level 0 fails)."""
import numpy as np

F32 = np.float32
M = 0xffffffff
R17 = (1 << 32) // 17
SEED = 0x7495f1a5


def grey16(R, G, B):
    with np.errstate(invalid="ignore", over="ignore"):
        v = (((G.astype(F32) + (G.astype(F32) + R.astype(F32))).astype(F32) + B.astype(F32)).astype(F32) * F32(4.0))
        v = v.astype(F32)
        r = np.rint(v.astype(np.float64))
    r = np.where(np.isnan(r), 0, r)
    return np.clip(r, -32768, 32767).astype(np.int64)


def down2(X):
    H, W = X.shape
    h, w = H >> 1, W >> 1
    s = X[0:2 * h:2, 0:2 * w:2] + X[1:2 * h:2, 0:2 * w:2] + X[0:2 * h:2, 1:2 * w:2] + X[1:2 * h:2, 1:2 * w:2]
    out = np.empty((h, w), np.int64)
    m = w & ~7
    a = s[:, :m] >> 2
    out[:, :m] = np.clip(a, -32768, 32767)
    t = ((s[:, m:] & M) >> 2) & 0xffff
    out[:, m:] = np.where(t >= 0x8000, t - 0x10000, t)
    return out


def _box17(X, axis):
    """sum of the 17 neighbours along axis with mirrored padding of 8, times floor(2^32/17), >> 32 (arithmetic)."""
    P = np.pad(X, [(8, 8) if a == axis else (0, 0) for a in range(2)], mode="reflect")
    c = np.cumsum(P, axis=axis)
    c = np.concatenate([np.zeros_like(np.take(c, [0], axis)), c], axis=axis)
    n = X.shape[axis]
    s = np.take(c, np.arange(17, 17 + n), axis) - np.take(c, np.arange(0, n), axis)
    return (s * R17) >> 32


def highpass(X):
    T = _box17(X, 1)
    T = ((T & 0xffff) ^ 0x8000) - 0x8000                         # stored as int16
    LP = _box17(T, 0)
    v = LP - X + 0x800
    return np.clip(v >> 4, 0, 255).astype(np.int64)


def _sad(A, ax, ay, Bm, bx, by, n):
    return int(np.abs(A[ay:ay + n, ax:ax + n] - Bm[by:by + n, bx:bx + n]).sum())


def _rng(s, wm, hm):
    a = s % wm
    w9 = (s + (s >> 16)) & M
    w9 = (w9 + (w9 << 8)) & M
    w9 = (w9 + (s >> 24)) & M
    w10 = ((w9 << 16) | (s >> 16)) & M
    b = w10 % hm
    w19 = (w10 + (w9 & 0xffff)) & M
    w19 = (w19 + (w19 << 8)) & M
    w19 = (w19 + ((w9 >> 8) & 0xff)) & M
    w9b = ((w9 & 0xffff) | (w19 << 16)) & M
    c = w9b % wm
    w24 = (w9b + (w19 & 0xffff)) & M
    w24 = (w24 + (w24 << 8)) & M
    w24 = (w24 + ((w19 >> 8) & 0xff)) & M
    x9 = ((w19 & 0xffff) | (w24 << 16)) & M
    d = x9 % hm
    w8 = (w19 + w24) & M
    w8 = (w8 + (w8 << 8)) & M
    w8 = (w8 + ((w24 >> 8) & 0xff)) & M
    return a, b, c, d, ((w24 & 0xffff) | (w8 << 16)) & M


def _stage(U, Rot, n, rad, minside):
    H, W = U.shape
    if W < minside or H < minside:
        return None
    wm, hm = W - (n - 1), H - (n - 1)
    lim_x, lim_y = W - n, H - n
    s = SEED
    acc = cnt = 0
    for _ in range(2000):
        a, b, c, d, s = _rng(s, wm, hm)
        centre = _sad(U, a, b, Rot, c, d, n)
        best, valid = centre, 0
        for dy in range(-rad, rad + 1):
            yy = d + dy
            if yy < 0 or yy > lim_y:
                continue
            for dx in range(-rad, rad + 1):
                xx = c + dx
                if xx < 0 or xx > lim_x:
                    continue
                v = _sad(U, a, b, Rot, xx, yy, n)
                if v < best:
                    best = v
                valid += 1
        if valid > 3:
            acc += centre - best
            cnt += 1
            if cnt >= 1000:
                break
    if cnt <= 999:
        return None
    return F32(F32(acc) / F32(cnt))


def mc_stats(R, G, B):
    R, G, B = (np.asarray(p, F32) for p in (R, G, B))
    H, W = R.shape
    dims = []
    if W >= 16 and H >= 16:
        w, h = W, H
        for l in range(16):
            dims.append((w, h))
            if l > 14 or h < 32:
                break
            if not w > 31:
                break
            w, h = w >> 1, h >> 1
    out = np.zeros(32, F32)
    v16 = v32 = F32(0.0)
    X = grey16(R, G, B) if dims else None
    for l, (w, h) in enumerate(dims):
        if l:
            X = down2(X)
        U = highpass(X)
        Rot = U[::-1, ::-1]
        r16 = _stage(U, Rot, 16, 2, 21)
        if r16 is None:
            v16 = F32(v16 * F32(0.5))
            v32 = F32(v32 * F32(0.5))
        else:
            v16 = r16
            r32 = _stage(U, Rot, 32, 1, 37)
            if r32 is not None:
                v32 = r32
            elif l:
                v32 = F32(v32 * F32(0.5))
            else:
                v32 = F32(v16 * np.array([0x3fb33333], np.uint32).view(F32)[0])
        out[l], out[16 + l] = v16, v32
    for l in range(len(dims), 16):
        v16 = F32(v16 * F32(0.5))
        v32 = F32(v32 * F32(0.5))
        out[l], out[16 + l] = v16, v32
    return out
