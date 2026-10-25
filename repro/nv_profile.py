"""BuildNoiseModel (FUN_009af718) as `nvh profile W H rgb mode out x y w h` runs it (area given, so FindFlatArea is
not called): the 0xc300-byte profile object (reference implementation; ported in sandbox/dirfilt, 2026-10-16).

    build_profile(rgb (H, W, 3) float32, mode, area) -> (uint8[0xc300], dict of the parts that are not ported)
    lzma_part(crop (h, w, 3) float32) -> FUN_009b41c4's words 0x18..0x2b, flag 0xc2a4 and the three LZMA blobs

Parts (byte offsets of the profile; "観測" = checked against `nvh profile`, see verify/verify_profile.py):
  0x048            colour mode
  0x04c + i*0x1f54 FUN_00995250 (set -1, 5 levels) of the crop planes R, G, B and the DAT_0147df50[mode] transform
  0xbc44/bd10/bddc FUN_009b2f34: RMS over the 32 x 32 tiles (step 16) of the deviations between the tile model
                   (FUN_00995250, 2 levels, set 2) and the plane's model, R, G, B; floored with the deviations of the
                   white-noise reference (FUN_009b4428: FUN_009c3bc4 noise 256 x 256, seed 0x12345678)
  0xbea8..0xbf24   FUN_0098718c (MotionCompensator statistics, 16 + 16 levels): nv_mcstats
  0xbf28..0xbf30   FUN_009c4a4c channel means (FUN_009ef3fc) R, G, B
  0xbf34 = 1, 0xbf38 size class of min(W, h) (DAT_01488128.. = 16, 32, 64, 128, 256)
  0xbf3c..0xbf3e   IsClip (FUN_009c6648) fraction > 0.33 per channel
  0xbf40..         luminance curve: FUN_009b3c84 init + FUN_009b3e78 (bin of each channel mean set to 1.0, valid)
  0x000..0x017     FUN_009b41c4: heap pointers of the LZMA-compressed planes - not reproducible (0 when it fails)
  0x018..0x020     16 + LZMA stream length of the int16 R, G, B planes (nv_lzma: LZMA SDK 9.20 LzmaCompress
                   level 5, lc 3, lp 0, pb 2, fb 32; checked against the genuine's blobs, lzma_oracle/verify_lzma.py)
  0x024, 0x028     W, h and 0xc2a4 = 1 when all three fit their buffers (len + 0x80 stream bytes); otherwise the
                   cleanup FUN_009ba4cc zeroes 0x00..0x2b and 0xc2a4
  0xc2c0 (W, h);  0xc2c8..0xc2ec constants DAT_012453b8..;  0xc2f0 = 0
"""
import os
import struct
import sys

import numpy as np

sys.dont_write_bytecode = True
_HERE = os.path.dirname(os.path.abspath(__file__))
for _p in (_HERE,):
    if _p not in sys.path:
        sys.path.insert(0, _p)
import nv_band as nb  # noqa: E402
import nv_curve as ncv  # noqa: E402
import nv_flat as nf  # noqa: E402
import nv_model as nm  # noqa: E402
import nv_refnoise as rn  # noqa: E402
import nv_mcstats as mc  # noqa: E402

F32 = np.float32
U32 = np.uint32
PROF = 0xc300
MODEL = 0x1f54
TEN4 = F32(10000.0)
NDIR2 = 8                                   # DAT_01244098 (set 2 directions)
SIZECLASS = (16, 32, 64, 128, 256)          # DAT_01488128..48
C2C8 = [F32(25.0)] + [F32(1.0)] * 9         # DAT_012453b8..d8 (0xc2c8..0xc2ec)


def _w(m, off):
    return F32(m[off // 4])


def _dev(x, y):
    """|x| <= |y| (b.le, also when unordered) ? |y| / |x| : |x| / |y|, minus 1 (fadd -1)."""
    ax, ay = F32(abs(x)), F32(abs(y))
    with np.errstate(all="ignore"):
        if ax > ay:
            r = F32(ax / ay)
        else:
            r = F32(ay / ax)
        return F32(r + F32(-1.0))


def _ratio_dev(a, b_fn):
    """a == 0 -> 10000, else b = b_fn(); b == 0 -> 10000, else _dev(a, b)."""
    if a == F32(0.0):
        return TEN4
    b = b_fn()
    if b == F32(0.0):
        return TEN4
    return _dev(a, b)


def _acc(out, j, d):
    with np.errstate(all="ignore"):
        out[j] = F32(out[j] + F32(d * d))


def tile_dev(plane, model, floor=None):
    """FUN_009b2f34(prof, plane, W, H, model, out, flag, ...) -> float32[51] (0xcc bytes)."""
    plane = np.ascontiguousarray(plane, F32)
    H, W = plane.shape
    if W < 32 or H < 32:
        raise NotImplementedError("FUN_009b2f34 reads 32 x 32 tiles beyond a plane narrower than 32 px")
    pm = np.asarray(model, U32).view(F32)
    nx = ((W - 32) >> 4) + 1
    ny = ((H - 32) >> 4) + 1
    out = np.zeros(51, F32)
    with np.errstate(all="ignore"):
        for tx in range(nx):
            for ty in range(ny):
                t = nm.model_full(plane[16 * ty:16 * ty + 32, 16 * tx:16 * tx + 32], 2, levels=2).view(F32)
                _acc(out, 0, _ratio_dev(F32(t[6] / t[5]), lambda: F32(pm[6] / pm[5])))
                for i in range(NDIR2):
                    for lv, j in ((0, 1 + i), (1, 0x11 + i)):
                        oA = (0x3c0 + 0x40 * lv) // 4 + i
                        oR = (0x7a8 + 4 * lv) // 4
                        a = F32(t[oA] / pm[oA])
                        _acc(out, j, _ratio_dev(a, lambda: F32(t[oR] / pm[oR])))
                for k in range(2):
                    for j in range(8):
                        x, y = t[15 + 8 * k + j], pm[15 + 8 * k + j]
                        if x < 0 or y < 0:                   # fcmp #0 / b.mi (a NaN goes on)
                            break
                        d = TEN4 if (x == 0 or y == 0) else _dev(x, y)
                        _acc(out, 0x21 + 8 * k + j, d)
                    a = F32(t[k] / t[5 + k])
                    _acc(out, 0x31 + k, _ratio_dev(a, lambda: F32(pm[k] / pm[5 + k])))
        n = F32((ny) * (nx))
        idx = [0] + list(range(1, 1 + NDIR2)) + list(range(0x11, 0x11 + NDIR2)) + list(range(0x21, 0x33))
        for j in idx:
            out[j] = F32(np.sqrt(F32(out[j] / n)))
        if floor is not None:
            for j in idx:
                if not out[j] > floor[j]:          # fcsel gt / fcmgt + bsl: a NaN takes the floor
                    out[j] = floor[j]
    return out


_FLOOR = None


def reference_floor(cache=os.path.join(os.path.dirname(_HERE), "data", "ref_floor.npy")):
    """DAT_01561d88: FUN_009b2f34 of the reference noise against its own model (flag 0); cached in data/ (it is
    recomputed when the cache is missing)."""
    global _FLOOR
    if _FLOOR is None and cache and os.path.exists(cache):
        _FLOOR = np.load(cache).astype(F32)
    if _FLOOR is None:
        x = rn.noise(65536).reshape(256, 256)
        m = nm.model_full(x)
        _FLOOR = tile_dev(x, m, None)
    return _FLOOR


def size_class(m):
    if m < SIZECLASS[0]:
        return 0
    for c in range(1, 5):
        if m < SIZECLASS[c]:
            lo, hi = SIZECLASS[c - 1], SIZECLASS[c]
            return int(F32(F32(F32(m - lo) / F32(hi - lo)) + F32(c)))
    return 5


def curve_bin(mean):
    v = F32(F32(mean + F32(16.0)) * F32(0.03125))
    # fcvtzs (0x9b3fbc: saturating, NaN -> 0), then > 7 -> 8 and a negative value -> 0 (+inf / huge -> 8)
    if not v > 0:
        return 0
    return 8 if (not np.isfinite(v) or v >= F32(8.0)) else int(v)


def lzma_part(crop):
    """FUN_009b41c4 (called by BuildNoiseModel after the curve): the crop is copied to the 16-bit image type
    (FUN_009c7318 type 1 -> FUN_009f0554) and each of R, G, B (W * h * 2 bytes, no row padding) is compressed by
    FUN_009d61f4 in turn; a failure (-1) stops the chain and the cleanup clears 0x00..0x2b and 0xc2a4."""
    import nv_lzma
    crop = np.asarray(crop, F32)
    h, w = crop.shape[:2]
    words = bytearray(0x2c - 0x18)
    blobs = []
    for c in range(3):
        b = nv_lzma.blob(nv_lzma.plane_int16(crop[..., c]))
        if b is None:
            return dict(ok=False, words_18_2c=bytes(0x2c - 0x18), flag_c2a4=0, blobs=[])
        blobs.append(b)
        struct.pack_into("<i", words, 4 * c, len(b))
    struct.pack_into("<ii", words, 0x24 - 0x18, w, h)
    return dict(ok=True, words_18_2c=bytes(words), flag_c2a4=1, blobs=blobs)


def build_profile(rgb, mode, area, with_floor=True, lzma=True):
    rgb = np.asarray(rgb, F32)
    x, y, w, h = area
    crop = rgb[y:y + h, x:x + w]
    R, G, B = (np.ascontiguousarray(crop[..., c]) for c in range(3))
    T = nf.transform(R, G, B, mode)
    planes = [R, G, B] + [np.ascontiguousarray(p, F32) for p in T]
    buf = np.zeros(PROF, np.uint8)
    w32 = buf.view(U32)
    f32 = buf.view(F32)
    w32[0x48 // 4] = mode
    models = []
    for i, p in enumerate(planes):
        m = nm.model_full(p)[:MODEL // 4]
        models.append(m)
        w32[(0x4c + i * MODEL) // 4:(0x4c + (i + 1) * MODEL) // 4] = m
    floor = reference_floor() if with_floor else None
    for i, off in enumerate((0xbc44, 0xbd10, 0xbddc)):
        f32[off // 4:off // 4 + 51] = tile_dev(planes[i], models[i], floor)
    f32[0xbea8 // 4:0xbea8 // 4 + 32] = mc.mc_stats(R, G, B)
    means = [nb.mean_exact(p) for p in planes[:3]]
    f32[0xbf28 // 4:0xbf28 // 4 + 3] = means
    clips = [ncv.is_clip_genuine(p) for p in planes[:3]]
    flags = [bool(c > F32(0.33)) for c in clips]
    buf[0xbf3c:0xbf3f] = flags
    buf[0xbf34] = 1
    w32[0xbf38 // 4] = size_class(min(w, h))
    # curve: 3 channels x 9 bins x 0x20 bytes from 0xbf44 (mode word 0xbf40 stays 0 in the harness)
    for c in range(3):
        for k in range(9):
            e = 0xbf44 + 0x120 * c + 0x20 * k
            f32[e // 4:e // 4 + 6] = F32(1.0)
        e = 0xbf44 + 0x120 * c + 0x20 * curve_bin(means[c])
        f32[e // 4:e // 4 + 6] = F32(1.0)
        w32[(e + 0x18) // 4] = 1
        buf[e + 0x1d] = flags[c]
        buf[e + 0x1e] = 0                                   # 3.0 < level 1.0
    if lzma:
        part = lzma_part(crop)
        buf[0x18:0x2c] = np.frombuffer(part["words_18_2c"], np.uint8)
        buf[0xc2a4] = part["flag_c2a4"]
        missing = {"heap pointers 0x00..0x17": (0x00, 0x18)} if part["ok"] else {}
    else:                                   # the LZMA sizes left 0 (the build assumed to succeed)
        struct.pack_into("<ii", buf, 0x24, w, h)
        buf[0xc2a4] = 1
        missing = {"heap pointers 0x00..0x17": (0x00, 0x18), "LZMA sizes 0x18..0x23": (0x18, 0x24)}
    struct.pack_into("<ii", buf, 0xc2c0, w, h)
    f32[0xc2c8 // 4:0xc2c8 // 4 + 10] = C2C8
    return buf, missing
