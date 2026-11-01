# fields_ref - the Python reference of the field pipeline (INT-2, native/nv_fieldproc.h): repro's v5 functions
# (reference/python/nvfilter.py, not modified) applied to the field sequence of interlaced frames.  The C++ port
# (nvfp::process_frame_fields, nvt::temporal_v5 with shifts, nvc_temporal_shift) is checked against it bit for bit
# by native/verify_fieldproc.py.
#
# Fields (as native/nv_fields.h): top = frame rows 0, 2, ..., bottom = rows 1, 3, ...; both ceil(H/2) rows, the
# bottom field of an odd H ends with a pad row repeating its own last row (frame row H - 2, row 0 when H == 1);
# every plane (4:2:0 chroma too) is split by its own row parity.  Field 2k + j of frame k has parity j ^ first.
#
# temporal_shift is nvfilter.temporal_v5 with one addition: shift[k] = (dy luma, dy chroma), the known vertical
# offset of window frame k against the centre (rows of the luma / chroma planes).  Frames with dy = 0 take exactly
# the operations of temporal_v5 (checked against it by verify_fieldproc.py).
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.normpath(os.path.join(HERE, "..")), "reference", "python"))
sys.dont_write_bytecode = True

import nvfilter as nv  # noqa: E402
from nvfilter import NEIGHBOUR_VAR, curve_sigma, dense_flow, estimate_flow_v5, gaussian_filter  # noqa: E402
from nvfilter import pyramid_weight, uniform_filter, warp_plane, _to_grid  # noqa: E402

F32 = np.float32
TOP, BOTTOM = 0, 1
PLAIN, SHIFT, SAME = 0, 1, 2
MODES = {"plain": PLAIN, "shift": SHIFT, "same": SAME}


def split_plane(p, parity):
    """The field `parity` of plane p (nv_fields split)."""
    p = np.asarray(p, F32)
    H = p.shape[0]
    f = p[parity::2]
    if f.shape[0] < (H + 1) // 2:
        f = np.concatenate([f, p[max(H - 2, 0)][None]], 0)
    return np.ascontiguousarray(f)


def join_plane(top, bottom, H):
    out = np.empty((H, top.shape[1]), F32)
    out[0::2] = top[:(H + 1) // 2]
    out[1::2] = bottom[:H // 2]
    return out


def temporal_shift(window, ci, prof, P, flows=None, shift=None):
    """nvfilter.temporal_v5 + the known offsets `shift` (see the top of this file)."""
    sim = P.get("similarity", "v5")
    if sim not in ("v5", "pyramid"):
        raise ValueError(f"unknown similarity {sim!r} (expected 'v5' or 'pyramid')")
    cur = [np.asarray(p, np.float32) for p in window[ci]]
    Y = cur[0]
    a = float(P["temporal"])
    st = (curve_sigma(prof, "Y", gaussian_filter(Y, 1.0)) * prof["tot_ratio"]["Y"]
          * P["noise_level"] * P["channel_noise"]["Y"])
    den = 2.0 * st * st + 1e-6
    acc = [p.copy() for p in cur]
    sw = [np.ones(p.shape, np.float32) for p in cur]
    sf = [np.zeros(p.shape, np.float32) for p in cur]
    for k, nb in enumerate(window):
        if k == ci or a <= 0:
            continue
        dyl, dyc = (float(shift[k][0]), float(shift[k][1])) if shift is not None else (0.0, 0.0)
        if flows is not None and (k - ci) in flows:
            vy, vx = flows[k - ci]
        else:
            ref = nb[0]
            if dyl != 0.0:      # the motion left after the known offset
                ref = warp_plane(nb[0], np.full(Y.shape, dyl, F32), np.zeros(Y.shape, F32))
            vy, vx = estimate_flow_v5(ref, Y, P["bs"], P["search"])
            if flows is not None:
                flows[k - ci] = (vy, vx)
        FY, FX = dense_flow(vy, vx, Y.shape, P["bs"])
        wy = warp_plane(nb[0], FY + dyl if dyl != 0.0 else FY, FX)
        if sim == "pyramid":
            s1 = st / max(prof["tot_ratio"]["Y"], 1e-6)          # finest-band sigma map
            w = pyramid_weight(Y - wy, s1, prof["band_ratio"]["Y"], P.get("pyramid_scale", 1.5))
        else:
            w = np.exp(-np.maximum(uniform_filter((wy - Y) ** 2, 5) / den - 1.0, 0.0) / max(P["threshold"], 1e-3))
        for c, p in enumerate(nb):
            if c == 0:
                wp, wc = wy, w
            elif p.shape == Y.shape:
                wp, wc = warp_plane(p, FY + dyc if dyc != 0.0 else FY, FX), w
            else:
                fyc = _to_grid(FY, p.shape) / 2
                if dyc != 0.0:
                    fyc = fyc + dyc
                wp = warp_plane(p, fyc, _to_grid(FX, p.shape) / 2)
                wc = _to_grid(w, p.shape)
            acc[c] += wc * wp
            sw[c] += wc
            sf[c] += NEIGHBOUR_VAR * wc * wc
    out, rv = [], []
    for c, p in enumerate(cur):
        out.append(p + a * (acc[c] / sw[c] - p))
        rv.append(((1 - a) + a / sw[c]) ** 2 + a * a * sf[c] / (sw[c] * sw[c]))
    return out, rv


def process_field(fields, parity, ci, mode, prof, P):
    """One field of a field sequence, as nvfilter.process_frame_v5 does a frame (nvfp::process_field_v5)."""
    R = int(P["radius"])
    lo, hi = max(0, ci - R), min(len(fields), ci + R + 1)
    idx = [k for k in range(lo, hi) if mode != SAME or parity[k] == parity[ci]]
    sub = [fields[k] for k in idx]
    shift = [(0.5 * (parity[ci] - parity[k]),) * 2 for k in idx] if mode == SHIFT else None
    tmp, rv = temporal_shift(sub, idx.index(ci), prof, P, {}, shift)
    out = nv.spatial_v5(tmp, prof, rv, P)
    m = float(P["mix"])
    if m > 0:
        out = [o + m * (np.asarray(s, np.float32) - o) for o, s in zip(out, fields[ci])]
    return out


def field_sequence(window, first):
    fields, parity = [], []
    for f in window:
        for q in (first, 1 - first):
            fields.append(tuple(split_plane(p, q) for p in f))
            parity.append(q)
    return fields, parity


def process_frame_fields(window, ci, first, mode, prof, P):
    """The output frame ci of `window` processed field by field (nvfp::process_frame_fields)."""
    if first not in (TOP, BOTTOM) or mode not in (PLAIN, SHIFT, SAME):
        raise ValueError("first / mode")
    fields, parity = field_sequence(window, first)
    o = [process_field(fields, parity, 2 * ci + j, mode, prof, P) for j in (0, 1)]
    top, bot = (o[0], o[1]) if first == TOP else (o[1], o[0])
    return [join_plane(t, b, np.asarray(p).shape[0]) for t, b, p in zip(top, bot, window[ci])]
