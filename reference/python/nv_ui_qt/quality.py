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

g sweeps the 27 curve bins (3 channels x 9) in (bin, channel) order, float32 accumulation.
The genuine is called with x0 = p + 0xbf40 and reads x1 = x0 + 0x20k + 0x120c at +0x22/+0x21/+0x4/+0x1c/+0x20;
the bins themselves start at +0xbf44 (the .dnp writer/reader), so relative to the bin o = 0xbf44 + 0x120c + 0x20k:
    byte o+0x1e != 0 (level > 3.0 flag)           -> -1/27
    elif byte o+0x1d != 0 (clip flag)             -> -1/27
    elif level word o+0x00 == 0                   -> -1/81
    elif u32 o+0x18 != 0 (valid: measured)        -> +1/27
    elif byte o+0x1c != 0 (marker: filled in)     -> +1/81   (ch2 bin8 reads 0xc2a0; 0xc2a4 is never read)
    else                                          -> +0
and returns max(sum, 0).  Corrected 2026-11-02 (work/omo/wp_genuine_capture/REPORT.md section 5,
work/omo/wp_quality_fix/REPORT.md): the decode of 2026-10-26 read the same addresses but named the
fields 4 bytes off (+0x4 "scale", +0x1c "AFT marker", +0x20 "next bin / 0xc2a4 LZMA flag").
Checked against 13 genuine screens (Quality and Completeness, decoded .dnp files):
native/testdata/genuine_quality.json.

Completeness = g * 100, shown as qRound(g * 100) + "%" next to Quality
(RS 0xe2e20 / 0xe437c for the panels, 0x118ef8 / 0x119038 for Profile Viewer).

The bin states (observed in the genuine .dnp files, wp_genuine_capture/REPORT.md section 2, and the
harness-verified BuildNoiseModel port nv_profile):
    fresh build (BuildNoiseModel, before AFT): every bin level 1.0, valid 0, marker 0, except the bin of
        each channel's mean: valid 1, clip = IsClip > 0.33  -> g = (3 - 2 * clipped) / 27, at least 0
        (11% without clipping).  Not reachable in the genuine RS UI (Build / Auto Profile include AFT).
    after AFT: bins measured from the tiles valid 1, bins interpolated / extrapolated marker 1.
    Reset Fine-Tuning: every bin level 1.0, valid 0, marker 0  -> g = 0.
    manual edit of a bin: its level, valid 0, marker 1.
The repro maps its own state onto these bytes:
    bin level word = factor[bin] * tune_all * tune_channel   (1.0 exactly when untouched)
    after AFT: prof["aft_bin_flags"] (nv_aft_v5.apply_aft, the AFT port's own bytes): "marker" is the
        u32 at +0x1c != 0 (the marker, or a clip / >3.0 flag), so valid = not marker; profiles saved
        before the flags existed count every bin as filled in (marker, g = 1/3)
    fresh: the bin of each channel's mean valid 1 with its clip flag
    Reset (base["aft_reset"], cleared by the next AFT): all clear
    pinned (manually edited) bins: valid 0, marker 1, >3.0 flag from the level (the flag rule is the AFT
        port's write(); the genuine edit above 3.0 was not observed)
Known gap of valid = not marker: a channel whose AFT found no usable bin (every measured bin above 3.0,
or the fresh build's bin clipped and no bin measured) keeps unfilled bins with valid 0 and marker 0;
they count +1/27 here instead of 0.
Static analysis and verification: work/omo/coord/verify_quality/static/QUALITY.md;
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
            elif f32[ob // 4] == F32(0.0):                            # level word
                s = F32(s + NEG81)
            elif struct.unpack_from("<I", b, ob + 0x18)[0] != 0:       # valid (measured)
                s = F32(s + POS27)
            elif b[ob + 0x1c] != 0:                                    # marker (filled in)
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
    means = [nb.mean_exact(p) for p in (R, G, B)]
    filled = [npv.curve_bin(m) for m in means]
    aft = bool(base.get("aft_applied"))
    reset = bool(base.get("aft_reset"))
    if reset:                                         # Reset Fine-Tuning: every bin clear (g = 0)
        stored = {"marker": [[False] * 9] * 3, "clip": [[False] * 9] * 3, "gt3": [[False] * 9] * 3}
    elif aft:
        stored = base.get("aft_bin_flags")            # per-bin flags from the AFT run
        if stored is None:
            stored = {"marker": [[True] * 9] * 3}     # profiles saved before the flags existed
    else:
        stored = None
    buf[0xc2a4] = 1                                   # not read by Quality (kept: the build's LZMA flag)
    tune = ps.tune or {}
    pinned = getattr(ps, "pinned", None) or [set(), set(), set()]
    chans = ("Y", "Cr", "Cb")                            # repro channel order (the genuine: R, G, B)
    tk = [float(tune.get("all", 1.0)) * float(tune.get(c, 1.0)) for c in chans]
    for c in range(3):
        for k in range(9):
            e = 0xbf44 + 0x120 * c + 0x20 * k
            lvl = F32(F32(ps.factors[c][k]) * F32(tk[c]))    # 1.0 exactly when untouched
            f32[e // 4:e // 4 + 6] = F32(1.0)
            f32[e // 4] = lvl                                # word 0 = the bin's level
            if stored is not None:
                # the AFT run's own flags (after Reset: all clear): "marker" = the u32 at +0x1c of the
                # port (marker, clip or >3.0), so a bin without it was measured (valid)
                marked = bool(stored["marker"][c][k])
                if aft and not reset and not marked:
                    w32[(e + 0x18) // 4] = 1
                if marked:
                    buf[e + 0x1c] = 1
                if stored.get("clip", [[False] * 9] * 3)[c][k]:
                    buf[e + 0x1d] = 1
                if stored.get("gt3", [[False] * 9] * 3)[c][k]:
                    buf[e + 0x1e] = 1
            elif lvl > F32(3.0):
                buf[e + 0x1e] = 1                            # level above 3.0 before AFT
        if stored is None:
            # the fresh profile (BuildNoiseModel): the bin of the channel mean is valid, with its clip flag
            e = 0xbf44 + 0x120 * c + 0x20 * filled[c]
            w32[(e + 0x18) // 4] = 1
            buf[e + 0x1d] = clips[c]
        for k in sorted(int(k) for k in pinned[c] if 0 <= int(k) < 9):
            # a manual edit: the level as set, valid 0, marker 1 (observed); >3.0 from the level (inferred)
            e = 0xbf44 + 0x120 * c + 0x20 * k
            w32[(e + 0x18) // 4] = 0
            buf[e + 0x1c] = 1
            buf[e + 0x1e] = 1 if f32[e // 4] > F32(3.0) else 0
    return buf


def profile_numbers(ps, frames, full_range, matrix):
    """(Quality %, Completeness %) - the two genuine percents (qRound(q*100), qRound(g*100)),
    or (None, None) without a profile or without the frames in memory."""
    if ps is None or ps.base is None or frames is None:
        return None, None
    blob = _curve_blob(ps, frames, full_range, matrix)
    q = quality_from_blob(blob)
    if q is None:
        return None, None
    return (int(np.floor(float(q) * 100.0 + 0.5)),
            int(np.floor(float(genuine_g(blob)) * 100.0 + 0.5)))


def profile_quality(ps, frames, full_range, matrix):
    """Overall Profile Quality in percent (int, qRound(q*100) as the panels compute it), or None.
    ps: ProfileState; frames: the in-memory frame list (None -> not computable)."""
    qp, _ = profile_numbers(ps, frames, full_range, matrix)
    return qp


def profile_completeness(ps, frames, full_range, matrix):
    """Completeness in percent (int, qRound(g*100) as the panels compute it), or None."""
    _, cp = profile_numbers(ps, frames, full_range, matrix)
    return cp
