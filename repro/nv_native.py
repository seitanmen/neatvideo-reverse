"""ctypes bridge to libnvcore.dylib (N2a): the N1 C++ ports (native/, bit-exact with the Python code)
behind the same contracts as the Python functions the UI backend calls.

    find_flat_area(planes, ...)      == nvfilter.find_flat_area(planes, ..., with_status=True)  (curve "genuine")
    build_profile_v5(planes, ...)    == nvfilter.build_profile_v5(planes, ...)
    apply_aft(prof, rgb, ...)        == nv_aft_v5.apply_aft(prof, rgb, ...)
    temporal_v5(window, ci, ...)     == nvfilter.temporal_v5(window, ci, ...)
    spatial_v5(planes, prof, rv, P)  == nvfilter.spatial_v5(planes, prof, rv, P)
    noise_curve(rgb)                 == nv_curve.genuine_noise_curve(rgb)

Division of the work: the C side runs build_profile_v5 whole (nv_prof5: FindFlatArea with its noise
curve, the band sigmas, the spatial / temporal curves; nvcore_c_prof5.h), the BuildNoiseModel profile
(nv_profile), AutoFineTune and its colour combination (nv_aft), and the v5 temporal / spatial kernels
(nv_temporal / nv_spatial); Python keeps the RGB conversion of the AFT (nvfilter.yuv_to_rgb, numpy), the dict
building, the caches and the Mix step. native/verify_bridge.py compares both paths bit for bit.
build_profile_v5 runs in Python for inputs the C port does not reproduce (planes that are not finite float32,
a luma side under 2 px, other chroma shapes, levels other than 5, unknown mode / matrix names, other area
lengths).

AVAILABLE is False when the library cannot be loaded (or has another ABI version); callers then use the
Python functions. temporal_v5 / spatial_v5 / apply_aft return None for inputs outside what the C ports
reproduce (numpy scalars of non-weak type in the profile or parameters - numpy would then promote to
float64 -, an unknown similarity, planes smaller than the motion block, an area outside the frame, an
AFT mode other than 0 / 1, ...); the caller then runs the Python function, so the result is the same.
Cancel: find_flat_area / build_profile_v5 / apply_aft take a cancel query (no arguments, true =
cancelled) that the C code asks where the Python code asks it (FindFlatArea, the AFT tile loop) and,
in apply_aft, also inside the BuildNoiseModel profile (profile_blob: the plane models, their levels,
direction sets and directions, the tile models; native only, G7). A cancelled apply_aft / profile_blob
raises Cancelled.

Library search: $NV_NATIVE_LIB (when set, only that path), else sys._MEIPASS (PyInstaller), <app>/Contents/Frameworks, then
native/libnvcore.dylib next to repro/. Threads of the profile / AFT calls: $NV_NATIVE_THREADS
(default min(4, cpu count)); their results do not depend on it (native/verify_*.py -j 1 / -j 3).
"""
import ctypes
import hashlib
import os
import sys
import threading

import numpy as np

import nvfilter as nv

F32 = np.float32
ABI_VERSION = 3
PROF_BYTES = 0xc300
_PROF_I, _PROF_D, _PAR_I, _PAR_D = 4, 82, 4, 21
_MAXC = 16
_HERE = os.path.dirname(os.path.abspath(__file__))


class Cancelled(Exception):
    """A cancel query answered true inside the native code."""


def _candidates():
    env = os.environ.get("NV_NATIVE_LIB")
    if env is not None:             # an explicit path only (a missing file: AVAILABLE = False)
        yield env
        return
    mei = getattr(sys, "_MEIPASS", None)
    if mei:
        yield os.path.join(mei, "libnvcore.dylib")
    exe = os.path.dirname(os.path.abspath(sys.executable))
    yield os.path.join(exe, "..", "Frameworks", "libnvcore.dylib")
    yield os.path.join(_HERE, "..", "native", "libnvcore.dylib")


def _load():
    for p in _candidates():
        p = os.path.normpath(p)
        if not os.path.isfile(p):
            continue
        try:
            lib = ctypes.CDLL(p)
            if lib.nvc_abi_version() != ABI_VERSION:
                continue
        except (OSError, AttributeError):
            continue
        return lib, p
    return None, None


_lib, LIB_PATH = _load()
# calls that ran in C / returned None for the Python fallback (diagnostics; native/verify_bridge.py)
STATS = {"native": 0, "fallback": 0}


def _count(res):
    STATS["native" if res is not None else "fallback"] += 1
    return res
AVAILABLE = _lib is not None
THREADS = max(1, int(os.environ.get("NV_NATIVE_THREADS", min(4, os.cpu_count() or 1))))

_P = ctypes.c_void_p
_I = ctypes.c_int
_D = ctypes.c_double
CANCEL_FN = ctypes.CFUNCTYPE(ctypes.c_int)
if AVAILABLE:
    _lib.nvc_noise_curve.argtypes = [_P, _I, _I, _I, CANCEL_FN, _P, _P]
    _lib.nvc_noise_curve.restype = _I
    _lib.nvc_flat_area.argtypes = [_P, _I, _I, _I, _I, CANCEL_FN, _P]
    _lib.nvc_flat_area.restype = None
    _lib.nvc_profile_build.argtypes = [_P, _I, _I, _I, _I, _I, _I, _I, _P, _I, _I, _P]
    _lib.nvc_profile_build.restype = _I
    _lib.nvc_profile_build_cancel.argtypes = [_P, _I, _I, _I, _I, _I, _I, _I, _P, _I, _I, CANCEL_FN, _P]
    _lib.nvc_profile_build_cancel.restype = _I
    _lib.nvc_build_profile_v5.argtypes = [_P, _I, _I, _I, _I, _P, _I, _P, _I, _I, _I, _I, _I, _P, _I, CANCEL_FN,
                                          _P, _P]
    _lib.nvc_build_profile_v5.restype = _I
    _lib.nvc_aft.argtypes = [_P, _I, _I, _P, _I, _I, CANCEL_FN, _P]
    _lib.nvc_aft.restype = _I
    _lib.nvc_aft_combine.argtypes = [_P, _P, _P, _I, _P, _D, _P, _I, _P, _P, _P, _P, _P]
    _lib.nvc_aft_combine.restype = None
    _lib.nvc_temporal.argtypes = [_I, _I, _I, _I, _I, _I, _P, _P, _P, _P, _P, _I, _P, _P, _P, _P, _P, _P]
    _lib.nvc_temporal.restype = _I
    _lib.nvc_spatial.argtypes = [_I, _I, _I, _I, _P, _P, _P, _P, _P, _P, _I, _P]
    _lib.nvc_spatial.restype = _I


# ------------------------------------------------------------------------------------- helpers
def _ptr(a):
    return a.ctypes.data


def _ptrs(arrays):
    return (ctypes.c_void_p * len(arrays))(*[_ptr(a) for a in arrays])


def _cancel_fn(cancel):
    """CFUNCTYPE wrapper of a Python cancel query (None -> NULL). Keep a reference during the call."""
    if cancel is None:
        return CANCEL_FN()

    def f():
        try:
            return 1 if cancel() else 0
        except Exception:       # noqa: BLE001 - an error in the query must not cross the C code
            return 1
    return CANCEL_FN(f)


def _c32(a):
    return np.ascontiguousarray(a, F32)


def _weak(v):
    """A Python int / float: numpy casts it to the array's float32 (what the C kernels do). numpy scalars
    (float64 would promote the arrays) and other types are not reproduced."""
    return type(v) in (int, float)


def _v5_blocks(prof, P):
    """(prof_i, prof_d, par_i, par_d) of nvcore_c.h, or None when the Python kernels would not compute in
    float32 with weak scalars (or the profile does not fit the C structs)."""
    try:
        sim = {"v5": 0, "pyramid": 1}.get(P.get("similarity", "v5"))
        if sim is None:
            return None
        names = ("Y", "Cb", "Cr")
        n = len(prof["curve_centers"])
        if not 1 <= n <= _MAXC or any(len(prof["curve"][c]) != n for c in names):
            return None
        levels = [prof["levels"][c] for c in names]
        if any(type(L) is not int or not 1 <= L <= 5 for L in levels):
            return None
        br = [list(prof["band_ratio"][c]) for c in names]
        tot = [prof["tot_ratio"][c] for c in names]
        if any(len(b) < L for b, L in zip(br, levels)) or not all(_weak(v) for b, L in zip(br, levels)
                                                                  for v in b[:L]) or not all(map(_weak, tot)):
            return None
        ints = [P["radius"], P["bs"], P["search"]]
        if any(type(v) is not int for v in ints) or P["bs"] < 1:
            return None
        sc = [P["temporal"], P["threshold"], P["spatial"], P["luma"], P["chroma"], P["noise_level"],
              P.get("pyramid_scale", 1.5), P["mix"]]
        ba = [P["band_amounts"][b] for b in nv.V5_BANDS]
        bn = [P["band_noise"][b] for b in nv.V5_BANDS]
        cn = [P["channel_noise"][c] for c in names]
        if not all(map(_weak, sc + ba + bn + cn)):
            return None
        pd = np.zeros(_PROF_D, np.float64)
        pd[:n] = np.asarray(prof["curve_centers"], np.float64)
        for c, nm in enumerate(names):
            pd[16 + 16 * c:16 + 16 * c + n] = np.asarray(prof["curve"][nm], np.float64)
            pd[64 + 5 * c:64 + 5 * c + levels[c]] = [float(v) for v in br[c][:levels[c]]]
        pd[79:82] = [float(v) for v in tot]
        pi = np.array([n] + levels, np.int32)
        qi = np.array(ints + [sim], np.int32)
        qd = np.array([float(v) for v in sc + ba + bn + cn], np.float64)
        return pi, pd, qi, qd
    except (KeyError, TypeError, ValueError):
        return None


def _shapes(planes):
    """(H, W, Hc, Wc) of a (Y, Cb, Cr) tuple of 2-D planes with Cb and Cr of one shape, else None."""
    if len(planes) != 3 or any(np.ndim(p) != 2 for p in planes):
        return None
    (H, W), s1, s2 = (np.shape(p) for p in planes)
    if s1 != s2 or min(H, W, *s1) < 1:
        return None
    return H, W, s1[0], s1[1]


# ------------------------------------------------------------------------------- noise estimation
def noise_curve(rgb, threads=None, cancel=None):
    """nv_curve.genuine_noise_curve(rgb): (xs, ys) lists of 8 floats, or None (W or H < 48, cancelled)."""
    rgb = _c32(rgb)
    H, W = rgb.shape[:2]
    xs, ys = np.zeros(8), np.zeros(8)
    cb = _cancel_fn(cancel)
    r = _lib.nvc_noise_curve(_ptr(rgb), W, H, threads or THREADS, cb, _ptr(xs), _ptr(ys))
    if r < 0:
        raise RuntimeError("nvc_noise_curve failed")
    return (xs.tolist(), ys.tolist()) if r == 1 else None


def flat_area_rgb(rgb, mode=1, threads=None, cancel=None):
    """nv_flat.find_flat_area_genuine(rgb, None, mode, cancel, curve=<the genuine curve of rgb>):
    (x0, y0, w, h, status)."""
    rgb = _c32(rgb)
    H, W = rgb.shape[:2]
    out = np.zeros(5, np.int32)
    cb = _cancel_fn(cancel)
    _lib.nvc_flat_area(_ptr(rgb), W, H, int(mode), threads or THREADS, cb, _ptr(out))
    if out[4] < 0:
        raise RuntimeError("nvc_flat_area failed")
    return tuple(int(v) for v in out)


_FFA_CACHE = {}
_FFA_LOCK = threading.Lock()


def find_flat_area(planes, full_range=False, matrix="bt709", mode=1, cancel=None, threads=None):
    """nvfilter.find_flat_area(planes, full_range, matrix, mode, with_status=True, curve="genuine", cancel)
    -> (y0, x0, size, status): the same steps, the search in C. Own cache (same key and size as nvfilter's)."""
    planes = tuple(np.asarray(p, F32) for p in planes)
    key = _ffa_key(planes, full_range, matrix, mode)
    with _FFA_LOCK:
        res = _FFA_CACHE.get(key)
    if res is None:
        H, W = planes[0].shape
        if min(H, W) >= 64:
            rgb = np.clip(np.nan_to_num(nv.yuv_to_rgb(planes, matrix, full_range), nan=0.0, posinf=255.0,
                                        neginf=0.0), 0, 255)
            x0, y0, w, hh, status = flat_area_rgb(rgb.astype(F32), mode, threads, cancel)
            if status == 5:
                return int(y0), int(x0), int(min(w, hh)), 5
            if status != 1 and min(w, hh) >= 8:
                res = (int(y0), int(x0), int(min(w, hh)), int(status))
        if res is None:
            res = (*nv.find_flat_area_gradient(planes, full_range, matrix=matrix), 1)
        with _FFA_LOCK:
            if key not in _FFA_CACHE:
                while len(_FFA_CACHE) >= 16:
                    _FFA_CACHE.pop(next(iter(_FFA_CACHE)))
                _FFA_CACHE[key] = res
    return res


_P5_MATRIX = {"bt709": 0, "bt601": 1, "bt2020": 2}
_P5_MODES = ("auto", "spatial", "temporal")
_P5_NAMES = ("Y", "Cb", "Cr")
_P5_I, _P5_D = 16, 72


def _ffa_key(planes, full_range, matrix, mode=1):
    """The FindFlatArea cache key of find_flat_area (nvfilter's key)."""
    h = hashlib.blake2b(digest_size=16)
    for p in planes:
        h.update(str(p.shape).encode())
        h.update(np.ascontiguousarray(p).view(np.uint8))
    return h.hexdigest(), bool(full_range), matrix, int(mode), "genuine"


def _p5_frame(fr, shapes):
    """The three planes of a frame as given (C-contiguous float32), or None when nv_prof5 does not reproduce the
    Python result for them: not float32 2-D arrays of the shapes `shapes`, or values that are not finite
    (nvcore_c_prof5.h; Python propagates NaN where the port does not)."""
    try:
        if len(fr) != 3:
            return None
    except TypeError:
        return None
    out = []
    for p, s in zip(fr, shapes):
        if not isinstance(p, np.ndarray) or p.dtype != F32 or p.shape != s:
            return None
        p = np.ascontiguousarray(p)
        if not np.isfinite(p).all():
            return None
        out.append(p)
    return out


def _p5_dict(oi, od):
    """The prof dict of nvfilter.build_profile_v5 from out_i / out_d (nvcore_c_prof5.h): key order and types."""
    an, lev, n = int(oi[0]), [int(v) for v in oi[7:10]], int(oi[14])
    prof = {"version": 5, "sub": int(oi[12]), "full_range": bool(oi[13]), "area": [int(v) for v in oi[1:1 + an]],
            "mode": _P5_MODES[int(oi[5])] + ("+noise-free" if oi[6] else ""),
            "levels": {c: lev[i] for i, c in enumerate(_P5_NAMES)},
            "band_ratio": {c: [float(v) for v in od[5 * i:5 * i + lev[i]]] for i, c in enumerate(_P5_NAMES)},
            "tot_ratio": {c: float(od[15 + i]) for i, c in enumerate(_P5_NAMES)},
            "area_sigma": {c: [float(v) for v in od[18 + 5 * i:18 + 5 * i + lev[i]]] for i, c in enumerate(_P5_NAMES)},
            "curve": {c: [float(v) for v in od[33 + 8 * i:33 + 8 * i + n]] for i, c in enumerate(_P5_NAMES)},
            "curve_centers": [float(v) for v in od[57:57 + n]],
            "flat_status": None if oi[10] == -1 else int(oi[10])}
    if oi[11]:
        prof["static_fraction"] = float(od[65])
    prof["dead_fraction"] = float(od[66])
    prof["area_level"] = float(od[67])
    prof["sigma"] = {c: float(od[68 + i]) for i, c in enumerate(_P5_NAMES)}
    return prof


def _p5_args(planes, full_range, area, pair, mode, levels, more, matrix):
    """The arguments of nvc_build_profile_v5, or None (the Python function then runs)."""
    if type(levels) is not int or levels != 5 or mode not in _P5_MODES or matrix not in _P5_MATRIX:
        return None
    shp = _shapes(planes) if isinstance(planes, (tuple, list)) else None
    if shp is None:
        return None
    H, W, Hc, Wc = shp
    if min(H, W) < 2:                   # Python raises ValueError there (np.gradient / an empty chroma crop)
        return None
    if (Hc, Wc) != (H, W) and (Hc, Wc) != ((H + 1) // 2, (W + 1) // 2):
        return None
    shapes = ((H, W), (Hc, Wc), (Hc, Wc))
    pl = _p5_frame(planes, shapes)
    pp = _p5_frame(pair, shapes) if pair is not None else []
    try:
        mm = [_p5_frame(f, shapes) for f in more]
    except TypeError:
        return None
    if pl is None or pp is None or any(m is None for m in mm):
        return None
    if area is None:
        ar = []
    else:
        try:
            ar = [int(v) for v in area]
        except (TypeError, ValueError):
            return None
        if len(ar) not in (3, 4) or any(not -2 ** 31 <= v < 2 ** 31 for v in ar):
            return None
    return pl, pp or None, [p for m in mm for p in m], len(mm), shp, ar


def _p5_call(args, full_range, mode, matrix, cancel, threads):
    """nvc_build_profile_v5 -> the prof dict, or None on an error status (the caller runs the Python path);
    status 5 raises ProfileCancelled."""
    pl, pp, mm, n_more, (H, W, Hc, Wc), ar = args
    ara = np.array(ar or [0], np.int32)
    oi, od = np.zeros(_P5_I, np.int32), np.zeros(_P5_D, np.float64)
    cb = _cancel_fn(cancel)
    keep = (pl, pp, mm, ara)                # noqa: F841 - the buffers live through the call
    r = _lib.nvc_build_profile_v5(_ptrs(pl), H, W, Hc, Wc, _ptrs(pp) if pp else None, n_more,
                                  _ptrs(mm) if mm else None, int(bool(full_range)), _P5_MATRIX[matrix],
                                  _P5_MODES.index(mode), 5, len(ar), _ptr(ara), threads or THREADS, cb,
                                  _ptr(oi), _ptr(od))
    if r == 5:
        raise nv.ProfileCancelled("profiling cancelled (FindFlatArea status 5)")
    return _p5_dict(oi, od) if r == 0 else None


def build_profile_v5(planes, full_range=False, area=None, pair=None, mode="auto", levels=5, more=(),
                     matrix="bt709", cancel=None, threads=None):
    """nvfilter.build_profile_v5 in C (nvc_build_profile_v5, G8), with the FindFlatArea cache of find_flat_area
    (a cached search passes its area to the C call as a manual square, whose clamp leaves an area of
    find_flat_area unchanged, and sets flat_status after; a new search is stored). Inputs the C port does not
    reproduce (_p5_args) take the earlier path: the search in C (find_flat_area), the rest nvfilter.build_profile_v5."""
    try:
        more = tuple(more)                  # iterated twice below (the checks, then a call)
    except TypeError:
        pass
    args = _p5_args(planes, full_range, area, pair, mode, levels, more, matrix)
    prof = None
    if args is not None:
        prof = _p5_build(args, full_range, area, mode, matrix, cancel, threads)
    if prof is not None:
        return prof
    if area is not None:
        return nv.build_profile_v5(planes, full_range, area, pair, mode, levels, more, matrix)
    y0, x0, S, status = find_flat_area(planes, full_range, matrix, cancel=cancel, threads=threads)
    if status == 5:
        raise nv.ProfileCancelled("profiling cancelled (FindFlatArea status 5)")
    prof = nv.build_profile_v5(planes, full_range, (y0, x0, S), pair, mode, levels, more, matrix)
    prof["flat_status"] = status
    return prof


def _p5_build(args, full_range, area, mode, matrix, cancel, threads):
    """build_profile_v5 through nvc_build_profile_v5 and the FindFlatArea cache, or None (error status)."""
    if area is not None:
        return _p5_call(args, full_range, mode, matrix, cancel, threads)
    key = _ffa_key(args[0], full_range, matrix)
    with _FFA_LOCK:
        res = _FFA_CACHE.get(key)
    if res is not None:
        prof = _p5_call(args[:5] + ([res[0], res[1], res[2]],), full_range, mode, matrix, cancel, threads)
        if prof is not None:
            prof["flat_status"] = res[3]
        return prof
    prof = _p5_call(args, full_range, mode, matrix, cancel, threads)
    if prof is None:
        return None
    with _FFA_LOCK:
        if key not in _FFA_CACHE:
            while len(_FFA_CACHE) >= 16:
                _FFA_CACHE.pop(next(iter(_FFA_CACHE)))
            _FFA_CACHE[key] = (*prof["area"], prof["flat_status"])
    return prof


# ------------------------------------------------------------------------------------- AutoFineTune
def _floor():
    import nv_profile as npr
    return np.ascontiguousarray(npr.reference_floor(), F32)


def profile_blob(rgb, area4, mode=1, threads=None, cancel=None):
    """nv_profile.build_profile(rgb, mode, (x, y, w, h))[0] as bytes, or None (status not OK). cancel: asked
    inside the model (nvc_profile_build_cancel); a true answer raises Cancelled."""
    rgb = _c32(rgb)
    H, W = rgb.shape[:2]
    x, y, w, h = (int(v) for v in area4)
    out = np.zeros(PROF_BYTES, np.uint8)
    fl = _floor()
    cb = _cancel_fn(cancel)
    st = _lib.nvc_profile_build_cancel(_ptr(rgb), W, H, int(mode), x, y, w, h, _ptr(fl), 1, threads or THREADS, cb,
                                       _ptr(out))
    if st == 3:
        raise Cancelled("cancelled")
    return out.tobytes() if st == 0 else None


def aft_blob(rgb, raw, mode=0, threads=None, cancel=None):
    """nv_aft.auto_fine_tune(rgb, raw, mode) (mode 0 / 1) -> bytes, or None (model error). Cancelled raises."""
    rgb = _c32(rgb)
    H, W = rgb.shape[:2]
    buf = np.frombuffer(bytearray(raw), np.uint8)
    out3 = np.zeros(3, F32)
    cb = _cancel_fn(cancel)
    st = _lib.nvc_aft(_ptr(rgb), W, H, _ptr(buf), int(mode), threads or THREADS, cb, _ptr(out3))
    if st == 2:
        raise Cancelled("cancelled")
    return buf.tobytes() if st in (0, 1) else None


def _apply_aft(prof, rgb, mode=0, area=None, matrix="bt709", band=0, cancel=None, threads=None):
    """See apply_aft."""
    if mode not in (0, 1) or type(band) is not int or not 0 <= band < 5:
        return None
    rgb = _c32(rgb)
    if rgb.ndim != 3 or rgb.shape[2] != 3:
        return None
    a = prof["area"] if area is None else area
    ay, ax, ah = a[:3]
    aw = a[3] if len(a) == 4 else ah
    if any(type(v) not in (int, np.int64, np.int32) for v in (ay, ax, ah, aw)):
        return None
    if cancel is not None and cancel():
        raise Cancelled("cancelled")
    raw = profile_blob(rgb, (ax, ay, aw, ah), 1, threads, cancel)
    if raw is None:
        return None
    if cancel is not None and cancel():
        raise Cancelled("cancelled")
    aft = aft_blob(rgb, raw, mode, threads, cancel)
    if aft is None:
        return None
    names = ("Y", "Cb", "Cr")
    centers = np.ascontiguousarray(np.array(prof["curve_centers"]), np.float64)
    n = len(centers)
    curves = np.ascontiguousarray(np.stack([np.array(prof["curve"][c], F32) for c in names]))
    tot = np.array([prof["tot_ratio"][c] for c in names], np.float64)
    lvl = float(prof.get("area_level", 128.0))
    cw, face, cv = np.zeros(9), np.zeros(3 * n), np.zeros(3 * n, F32)
    sig, flags = np.zeros(3), np.zeros(81, np.uint8)
    rawb, aftb = np.frombuffer(raw, np.uint8), np.frombuffer(aft, np.uint8)
    _lib.nvc_aft_combine(_ptr(rawb), _ptr(aftb), _ptr(centers), n, _ptr(curves), lvl, _ptr(tot), band,
                         _ptr(cw), _ptr(face), _ptr(cv), _ptr(sig), _ptr(flags))
    for ci, c in enumerate(names):
        prof["curve"][c] = cv[ci * n:(ci + 1) * n].tolist()
    prof["sigma"] = {c: float(sig[ci]) for ci, c in enumerate(names)}
    prof["aft_applied"] = True
    prof["aft_face_curves"] = [face[j * n:(j + 1) * n].tolist() for j in range(3)]
    prof["aft_color_weights"] = [cw[3 * ci:3 * ci + 3].tolist() for ci in range(3)]
    prof["aft_band"] = band
    fl = flags.reshape(3, 3, 9).astype(bool)
    prof["aft_bin_flags"] = {"marker": fl[0].tolist(), "clip": fl[1].tolist(), "gt3": fl[2].tolist()}
    return prof


# ------------------------------------------------------------------------------- v5 kernels
def _temporal_v5(window, ci, prof, P, flows=None):
    """See temporal_v5."""
    blocks = _v5_blocks(prof, P)
    if blocks is None or not 0 <= ci < len(window):
        return None
    shp = [_shapes(f) for f in window]
    if shp[0] is None or any(s != shp[0] for s in shp):
        return None
    H, W, Hc, Wc = shp[0]
    bs = P["bs"]
    if min(H, W) < bs:
        return None
    nby, nbx = H // bs, W // bs
    nf = len(window)
    planes = [_c32(p) for f in window for p in f]
    offs, fin = [], []
    if flows is not None:
        for k in range(nf):
            if k != ci and (k - ci) in flows:
                vy, vx = flows[k - ci]
                vy, vx = np.asarray(vy), np.asarray(vx)
                if vy.dtype != F32 or vx.dtype != F32 or vy.shape != (nby, nbx) or vx.shape != (nby, nbx):
                    return None
                offs.append(k - ci)
                fin += [_c32(vy), _c32(vx)]
    fout = [np.empty((nby, nbx), F32) for _ in range(2 * nf)]
    new = np.zeros(nf, np.uint8)
    out = [np.empty((H, W), F32), np.empty((Hc, Wc), F32), np.empty((Hc, Wc), F32)]
    rv = [np.empty((H, W), F32), np.empty((Hc, Wc), F32), np.empty((Hc, Wc), F32)]
    pi, pd, qi, qd = blocks
    offa = np.array(offs or [0], np.int32)
    keep = (planes, fin, fout, out, rv)          # noqa: F841 - the buffers live through the call
    r = _lib.nvc_temporal(nf, ci, H, W, Hc, Wc, _ptrs(planes), _ptr(pi), _ptr(pd), _ptr(qi), _ptr(qd), len(offs),
                          _ptr(offa), _ptrs(fin) if fin else None, _ptrs(fout), _ptr(new), _ptrs(out), _ptrs(rv))
    if r != 0:
        return None
    if flows is not None:
        for k in range(nf):
            if new[k]:
                flows[k - ci] = (fout[2 * k], fout[2 * k + 1])
    return out, rv


def _spatial_v5(planes, prof, rv, P, only=None):
    """See spatial_v5."""
    blocks = _v5_blocks(prof, P)
    shp = _shapes(planes)
    if blocks is None or shp is None or len(rv) != 3:
        return None
    H, W, Hc, Wc = shp
    rvs = [np.asarray(r) for r in rv]
    if any(r.dtype != F32 or r.shape != np.shape(p) for r, p in zip(rvs, planes)):
        return None
    mask = 7 if only is None else 1 << only
    pl = [_c32(p) for p in planes]
    rvc = [np.ascontiguousarray(r) for r in rvs]
    out = [np.empty((H, W), F32), np.empty((Hc, Wc), F32), np.empty((Hc, Wc), F32)]
    pi, pd, qi, qd = blocks
    r = _lib.nvc_spatial(H, W, Hc, Wc, _ptrs(pl), _ptrs(rvc), _ptr(pi), _ptr(pd), _ptr(qi), _ptr(qd), mask, _ptrs(out))
    if r != 0:
        return None
    return out if only is None else [o if c == only else None for c, o in enumerate(out)]


def temporal_v5(window, ci, prof, P, flows=None):
    """nvfilter.temporal_v5(window, ci, prof, P, flows) -> (out, rv), or None (not reproduced: the caller
    runs nvfilter.temporal_v5). New block vectors go into `flows` as the Python function stores them."""
    return _count(_temporal_v5(window, ci, prof, P, flows))


def spatial_v5(planes, prof, rv, P, only=None):
    """nvfilter.spatial_v5(planes, prof, rv, P) -> 3 planes, or None (not reproduced). only = c computes
    plane c alone (== spatial_v5(planes[:c + 1], prof, rv[:c + 1], P)[c]; the list holds None elsewhere)."""
    return _count(_spatial_v5(planes, prof, rv, P, only))


def apply_aft(prof, rgb, mode=0, area=None, matrix="bt709", band=0, cancel=None, threads=None):
    """nv_aft_v5.apply_aft(prof, rgb, mode, area, matrix, band) through the C ports: prof is updated in
    place and returned. None (prof untouched) where the C path does not apply - the caller runs
    nv_aft_v5.apply_aft. Cancelled when `cancel` answers true."""
    return _count(_apply_aft(prof, rgb, mode, area, matrix, band, cancel, threads))
