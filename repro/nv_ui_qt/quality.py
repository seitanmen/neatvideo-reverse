"""Overall Profile Quality - the genuine measure (RS 0xa4acf8 / Demo 0x9bb3e0), decoded 2026-10-26.

    q = s11 * P * S/5 * 0.5^clips,  displayed as qRound(q * 100) + "%"

  s11 = 0.5*g + 0.5, where g is the luminance-curve completeness (RS 0xa4b03c / Demo 0x9bb748).
        The lead decoded g through the obfuscation blocks at its entry and exit: they only do
        bookkeeping on global words and never touch the float accumulator.  RS and Demo agree on
        the numeric path instruction for instruction (different registers / obfuscation layout).
  P   = sample uniformity: flatness of the profile's conversion faces Y/Cr/Cb (colour mode 1),
        chroma penalised at half.  u = clamp(2.5 - f, 0, 1); P = ((u0+.5)(u1+.5)(u2+.5)+.25)/3.625.
  S   = size of the sample area: min side < 16 -> 0, >= 256 -> 5, else linear on the table
        16/32/64/128/256 (segment k adds k + (m - T[k-1]) / (T[k] - T[k-1])).  With W or H == 0
        the profile's integer size class is used instead (S = that value, not divided).
  clips = profile bytes 0xbf3c..0xbf3e (R/G/B IsClip > 0.33): q halved once per set byte.

g sweeps the 27 curve bins (3 channels x 9) in (bin, channel) order, float32 accumulation:
    byte +0x1e != 0 (level > 3.0 flag)   -> -1/27
    elif byte +0x1d != 0 (clip flag)     -> -1/27
    elif scale word (+0x4) == 0          -> -1/81
    elif marker word (+0x1c) != 0 (AFT)  -> +1/27
    elif next bin's byte 0 != 0          -> +1/81   (ch2 bin8 reads 0xc2a4 = LZMA success flag)
    else                                 -> +0
and returns max(sum, 0).  A fresh profile marks nothing, so g = 1/81 (only the LZMA flag path);
after AFT every bin carries the marker, so g = float32 27 * (1/27) accumulated = 0.999999762.

The repro maps its own state onto the genuine curve bytes (assumption, see QUALITY.md 5: manual
Fine-Tune writes level words without markers - not verifiable without running the genuine):
    bin level word = factor[bin] * tune_all * tune_channel   (all 1.0 when untouched)
    markers: all set when base["aft_applied"], else none
    fresh filled bin (+0x18 word = 1): byte +0x1d = clip flag of its plane
Static analysis and verification: sandbox/omo/coord/verify_quality/static/QUALITY.md;
quality_draft.genuine_g there is the same decode - keep the two in sync.
"""
import copy
import struct

import numpy as np

F32 = np.float32
U32 = np.uint32
MODEL = 0x1f54
SIZE_T = (16, 32, 64, 128, 256)                    # RS 0x162c580 / Demo 0x1488128
LUMA_FLAGS = (1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0)  # RS 0x1622348 / Demo 0x147dfc8 (mode*3 + i)
NEG27 = F32(np.frombuffer(struct.pack("<I", 0xBD17B426), F32)[0])   # -1/27
POS27 = F32(np.frombuffer(struct.pack("<I", 0x3D17B426), F32)[0])   # +1/27
NEG81 = F32(np.frombuffer(struct.pack("<I", 0xBC4A4588), F32)[0])   # -1/81
POS81 = F32(np.frombuffer(struct.pack("<I", 0x3C4A4588), F32)[0])   # +1/81
HALF = F32(0.5)


def genuine_g(blob):
    """RS 0xa4b03c / Demo 0x9bb748 on the 0xc300-byte profile blob (see module docstring)."""
    b = bytes(blob)
    f32 = np.frombuffer(b, F32)
    s = F32(0.0)
    for k in range(9):
        for c in range(3):
            ob = 0xbf44 + 0x120 * c + 0x20 * k
            if b[ob + 0x1e] != 0:
                s = F32(s + NEG27)
            elif b[ob + 0x1d] != 0:
                s = F32(s + NEG27)
            elif f32[(ob // 4) + 1] == F32(0.0):                     # word at +0x4 (first scale)
                s = F32(s + NEG81)
            elif struct.unpack_from("<I", b, ob + 0x1c)[0] != 0:
                s = F32(s + POS27)
            elif b[ob + 0x20] != 0:
                s = F32(s + POS81)
    return F32(s) if s > F32(0.0) else F32(0.0)                      # fmax s0, 0.0


def _size_term(W, H, fallback):
    if W == 0 or H == 0:
        return F32(fallback)
    m = min(W, H)
    if m < SIZE_T[0]:
        return F32(0.0)
    if m >= SIZE_T[4]:
        return F32(5.0)
    k = next(k for k in range(1, 5) if m < SIZE_T[k])
    lo, hi = SIZE_T[k - 1], SIZE_T[k]
    return F32(F32(F32(m - lo) / F32(hi - lo)) + F32(k))


def _penalty(x, s=F32(2.0), c=F32(-0.25)):
    """FUN_009d49b4 = clamp(2.5 - x, 0, 1) for (s, c) = (2.0, -0.25)."""
    x, s, c = F32(x), F32(s), F32(c)
    if s == F32(0.0):
        return F32(1.0) if F32(x * c) > F32(0.0) else F32(0.0)
    a = F32(F32(F32(F32(0.5) / s) / c) * x)
    b = F32(F32(F32(c + F32(-1.0)) * F32(0.5)) / c)
    v = F32(a + b)
    return F32(0.0) if v < F32(0.0) else (F32(1.0) if v > F32(1.0) else v)


def quality_from_blob(blob):
    """The genuine q (float32) from a 0xc300-byte blob; None when p[0xbf34] == 0 (no profile)."""
    import nv_flat as nf
    b = bytes(blob)
    if b[0xbf34] == 0:
        return None
    mode = struct.unpack_from("<i", b, 0x48)[0]
    w = np.frombuffer(b, U32)
    u = []
    for i in range(3):
        off = (0x4c + (3 + i) * MODEL) // 4
        luma = LUMA_FLAGS[mode * 3 + i]
        f = nf.flatness(w[off:off + 55], bool(luma))
        v = _penalty(f)
        if luma == 0:
            v = F32(F32(v * HALF) + HALF)
        u.append(v)
    P = F32(F32(F32(F32(u[0] + HALF) * F32(u[1] + HALF)) * F32(u[2] + HALF)) + F32(0.25))
    P = F32(P / F32(3.625))
    W, H = struct.unpack_from("<ii", b, 0xc2c0)
    S = _size_term(W, H, struct.unpack_from("<i", b, 0xbf38)[0])
    q = F32(F32(F32(F32(genuine_g(b)) * HALF) + HALF) * P)
    q = F32(q * F32(S / F32(5.0)))
    for c in range(3):
        if b[0xbf3c + c] != 0:
            q = F32(q * HALF)
    return q


def _curve_blob(ps, planes, full_range, matrix):
    """The bytes Quality reads, from the repro profile state (module docstring: the mapping)."""
    import nv_band as nb
    import nv_curve as ncv
    import nv_flat as nf
    import nv_profile as npv
    import nvfilter as nv
    base = ps.base
    area = base["area"]
    y0, x0 = int(area[0]), int(area[1])
    h, w = (int(area[2]), int(area[2])) if len(area) == 3 else (int(area[2]), int(area[3]))
    f = max(0, min(int(base.get("ui_frame", 0)), len(planes) - 1))
    pl = tuple(np.asarray(p, np.float32) for p in planes[f])
    rgb = np.clip(nv.yuv_to_rgb(pl, matrix, full_range), 0, 255).astype(np.float32)
    crop = rgb[y0:y0 + h, x0:x0 + w]
    R, G, B = (np.ascontiguousarray(crop[..., c]) for c in range(3))
    T = [np.ascontiguousarray(p, np.float32) for p in nf.transform(R, G, B, 1)]
    buf = bytearray(0xc300)
    w32 = np.frombuffer(buf, np.uint32)
    f32 = np.frombuffer(buf, F32)
    w32[0x48 // 4] = 1                                   # colour mode 1 (the genuine default)
    for i, p in enumerate(T):                            # faces 3..5, words 0..54
        m = np.asarray(nb.model_levels(p)).view(U32)[:55]
        off = (0x4c + (3 + i) * MODEL) // 4
        w32[off:off + 55] = m
    buf[0xbf34] = 1
    w32[0xbf38 // 4] = npv.size_class(min(w, h))
    clips = [bool(ncv.is_clip_genuine(p) > F32(0.33)) for p in (R, G, B)]
    for c, cl in enumerate(clips):
        buf[0xbf3c + c] = cl
    struct.pack_into("<ii", buf, 0xc2c0, w, h)
    buf[0xc2a4] = 1                                      # LZMA part written (always true in repro)
    means = [nb.mean_exact(p) for p in (R, G, B)]
    filled = [npv.curve_bin(m) for m in means]
    aft = bool(base.get("aft_applied"))
    tune = ps.tune or {}
    chans = ("Y", "Cr", "Cb")                            # repro channel order (the genuine: R, G, B)
    tk = [float(tune.get("all", 1.0)) * float(tune.get(c, 1.0)) for c in chans]
    for c in range(3):
        for k in range(9):
            e = 0xbf44 + 0x120 * c + 0x20 * k
            lvl = F32(F32(ps.factors[c][k]) * F32(tk[c]))    # 1.0 exactly when untouched
            f32[e // 4:e // 4 + 6] = F32(1.0)
            f32[e // 4] = lvl                                # word 0 = the bin's level
            if aft:
                buf[e + 0x1c] = 1                            # AFT marks every bin (+1/27)
        e = 0xbf44 + 0x120 * c + 0x20 * filled[c]
        if not aft:
            w32[(e + 0x18) // 4] = 1                         # the fresh profile's valid word
            buf[e + 0x1d] = clips[c]                         # and the clip flag in +0x1d
    return buf


def profile_quality(ps, frames, full_range, matrix):
    """Overall Profile Quality in percent (int, qRound(q*100) as the panels compute it), or None.
    ps: ProfileState; frames: the in-memory frame list (None -> not computable)."""
    if ps is None or ps.base is None or frames is None:
        return None
    q = quality_from_blob(_curve_blob(ps, frames, full_range, matrix))
    if q is None:
        return None
    return int(np.floor(float(q) * 100.0 + 0.5))
