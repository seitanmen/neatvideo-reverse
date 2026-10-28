"""Bridge between the UI and the v5 backend (nvfilter.py, nv_aft_v5.py); no Qt here.

process_frame() runs nv.temporal_v5 and nv.spatial_v5 in the same order as nv.process_frame_v5.
The only addition is per-channel spatial settings (the genuine panel has separate Luminance / Cb /
Cr band sliders, v5 shares one band set): when all channels use the same band settings and
Cb = Cr, it calls nv.spatial_v5 once with exactly the parameters process_frame_v5 would get;
otherwise it calls spatial_v5 once per channel with that channel's settings and keeps that
channel's output. _init / _work are the worker functions of the full-video export (export.py).

Temporal Noise Level (P["t_noise"], UI Temporal > Temporal Noise > Noise Level): temporal() calls
nv.temporal_v5 with noise_level x t_noise for the temporal stage only (the spatial stage keeps
noise_level); at t_noise = 1 (or without the key) P is passed unchanged.

Native core (N2a, Preferences > "Use the native core"): with libnvcore.dylib (repro/nv_native.py) the
temporal / spatial kernels, the FindFlatArea search of build_profile and Auto Fine-Tune run in C++
(bit-exact with the Python functions, native/verify_bridge.py); inputs the C ports do not reproduce fall
back to the Python functions. set_native() switches it; NV_NATIVE=0 in the environment disables it, and
set_native writes NV_NATIVE so that the export process and its frame workers follow the UI setting. With
the native core, run_in_child(task, run_aft, ...) runs the AFT in this process (no child process), and
task.cancelled is asked in the AFT tile loop.

Noise Level Check (repro definition of the genuine's temporal filter mask; its formula is not decoded):
temporal_mask() maps the luma residual noise variance rv of the temporal stage to
m = clamp((1/rv - 1) / (1/rv_min - 1), 0, 1), rv_min = the variance when every neighbour of the window
has weight 1; PreviewEngine.mask() uses the same cached temporal result as the preview.
"""
import collections
import copy
import os
import threading

import numpy as np

import nvfilter as nv

try:
    import nv_native
except Exception:       # noqa: BLE001 - a broken library or bridge must not stop the UI
    nv_native = None

CHANNEL_NAMES = ("Y", "Cb", "Cr")

# ---------------- native core switch ----------------
_NATIVE_FORCED_OFF = os.environ.get("NV_NATIVE", "1") == "0"
_NATIVE = {"on": not _NATIVE_FORCED_OFF}


def native_available():
    """libnvcore.dylib was loaded (repro/nv_native.py)."""
    return nv_native is not None and nv_native.AVAILABLE


def native_enabled():
    return _NATIVE["on"] and native_available()


def set_native(on):
    """Use the native core (when available) for the processing of this process and of the processes it
    starts afterwards (NV_NATIVE in the environment). NV_NATIVE=0 at start-up keeps it off."""
    _NATIVE["on"] = bool(on) and not _NATIVE_FORCED_OFF
    os.environ["NV_NATIVE"] = "1" if _NATIVE["on"] else "0"


def _uniform(P):
    ch = P.get("ui_channels")
    if ch is None:
        return True
    y, cb, cr = (ch[c] for c in CHANNEL_NAMES)
    return (y["bands"] == cb["bands"] == cr["bands"] and y["noise"] == cb["noise"] == cr["noise"]
            and cb["amount"] == cr["amount"])


def _channel_params(P, c):
    ch = P["ui_channels"][CHANNEL_NAMES[c]]
    Q = dict(P)
    Q["band_amounts"] = dict(ch["bands"])
    Q["band_noise"] = dict(ch["noise"])
    Q["luma" if c == 0 else "chroma"] = ch["amount"]
    return Q


def _spatial(planes, prof, rv, P, only=None):
    """nv.spatial_v5 (only = c: plane c of spatial_v5(planes[:c + 1], ...)), native when enabled."""
    if native_enabled():
        out = nv_native.spatial_v5(planes, prof, rv, P, only)
        if out is not None:
            return out if only is None else out[only]
    if only is None:
        return nv.spatial_v5(planes, prof, rv, P)
    return nv.spatial_v5(list(planes[:only + 1]), prof, list(rv[:only + 1]), P)[only]


def spatial_ui(planes, prof, rv, P):
    if not P.get("ui_spatial", True):
        return [np.asarray(p, np.float32) for p in planes]
    if _uniform(P):
        return _spatial(planes, prof, rv, P)
    # spatial_v5 handles planes[0..c]; only plane c of each call is kept
    return [_spatial(planes, prof, rv, _channel_params(P, c), c) for c in range(len(planes))]


def temporal(window, ci, prof, P, flows=None):
    R = int(P["radius"])
    lo, hi = max(0, ci - R), min(len(window), ci + R + 1)
    tn = float(P.get("t_noise", 1.0))
    Q = P if tn == 1.0 else dict(P, noise_level=P["noise_level"] * tn)
    if native_enabled():
        res = nv_native.temporal_v5(window[lo:hi], ci - lo, prof, Q, flows)
        if res is not None:
            return res
    return nv.temporal_v5(window[lo:hi], ci - lo, prof, Q, flows)


def temporal_mask(rv, n, a):
    """Noise Level Check mask from the luma residual variance rv of temporal_v5 on n frames with
    Amount a: 0 where the temporal filter averages nothing, 1 where every neighbour has weight 1
    (rv = rv_min = ((1 - a) + a/n)^2 + a^2 0.64 (n - 1) / n^2). a <= 0 or n = 1: 0 everywhere."""
    rv = np.asarray(rv, np.float32)
    if a <= 0 or n <= 1:
        return np.zeros(rv.shape, np.float32)
    rv_min = ((1.0 - a) + a / n) ** 2 + a * a * nv.NEIGHBOUR_VAR * (n - 1) / (n * n)
    return np.clip((1.0 / rv - 1.0) / (1.0 / rv_min - 1.0), 0.0, 1.0).astype(np.float32)


def finish(window, ci, tmp, rv, prof, P):
    out = spatial_ui(tmp, prof, rv, P)
    m = float(P["mix"])
    if m > 0:
        out = [o + m * (np.asarray(s, np.float32) - o) for o, s in zip(out, window[ci])]
    return out


def process_frame(window, ci, prof, P, flows=None):
    """Same contract as nv.process_frame_v5 (float32 planes, not clipped)."""
    tmp, rv = temporal(window, ci, prof, P, flows)
    return finish(window, ci, tmp, rv, prof, P)


# ---------------- full-video export workers (pickled by reference) ----------------
_W = {}


def _init(prof, P):
    _W["prof"], _W["P"] = prof, P


def _work(job):
    idx, window, ci = job
    win = [tuple(p.astype(np.float32) for p in f) for f in window]
    return idx, process_frame(win, ci, _W["prof"], _W["P"])


# ---------------- preview engine ----------------
_TEMPORAL_KEYS = ("radius", "temporal", "threshold", "noise_level", "similarity", "pyramid_scale", "bs", "search",
                  "t_noise")


def temporal_sig(P):
    """The parameters the temporal stage depends on (cache key; t_noise defaults to 1)."""
    return tuple(P.get(k, 1.0) if k == "t_noise" else P[k] for k in _TEMPORAL_KEYS), P["channel_noise"]["Y"]


class PreviewEngine:
    """Processes one frame (optionally a crop) for the preview. Caches motion vectors per centre
    frame and the temporal result per temporal-parameter set, so spatial-only changes skip the
    temporal stage (the cached arrays are the ones process_frame would compute)."""

    def __init__(self, frames, matrix, full_range):
        self.frames, self.matrix, self.full_range = frames, matrix, full_range
        self._flows = collections.OrderedDict()
        self._temporal = collections.OrderedDict()
        self._lock = threading.Lock()

    @staticmethod
    def _crop_planes(planes, crop):
        if crop is None:
            return tuple(np.asarray(p, np.float32) for p in planes)
        y0, x0, h, w = crop
        out = [np.asarray(planes[0][y0:y0 + h, x0:x0 + w], np.float32)]
        for p in planes[1:]:
            s = 2 if p.shape != planes[0].shape else 1
            out.append(np.asarray(p[y0 // s:(y0 + h) // s, x0 // s:(x0 + w) // s], np.float32))
        return tuple(out)

    def window(self, t, R, crop):
        idx = [i for i in range(t - R, t + R + 1) if 0 <= i < len(self.frames)]
        return idx, [self._crop_planes(self.frames[i], crop) for i in idx]

    @staticmethod
    def _put(cache, key, val, n):
        cache[key] = val
        cache.move_to_end(key)
        while len(cache) > n:
            cache.popitem(last=False)

    def _temporal_stage(self, t, prof, prof_token, P, crop):
        """(window, ci, (tmp, rv)) of frame t; the temporal result is cached per temporal_sig."""
        R = int(P["radius"])
        idx, win = self.window(t, R, crop)
        ci = idx.index(t)
        tkey = (t, crop, prof_token) + temporal_sig(P)
        with self._lock:
            hit = self._temporal.get(tkey)
            fkey = (t, crop, P["bs"], P["search"])
            flows = self._flows.setdefault(fkey, {})
            self._flows.move_to_end(fkey)
            while len(self._flows) > 16:
                self._flows.popitem(last=False)
        if hit is None:
            hit = temporal(win, ci, prof, P, flows)
            with self._lock:
                self._put(self._temporal, tkey, hit, 6)
        return win, ci, hit

    def process(self, t, prof, prof_token, P, crop=None):
        win, ci, (tmp, rv) = self._temporal_stage(t, prof, prof_token, P, crop)
        out = finish(win, ci, [p.copy() for p in tmp], rv, prof, P)
        return self.to_rgb([np.clip(o, 0, 255) for o in out]), self.to_rgb(win[ci])

    def mask(self, t, prof, prof_token, P):
        """Noise Level Check mask (H x W float32 0..1, coded orientation) of the whole frame t from the
        luma residual variance of the cached temporal stage."""
        win, ci, (tmp, rv) = self._temporal_stage(t, prof, prof_token, P, None)
        return temporal_mask(rv[0], len(win), float(P["temporal"]))

    def assist(self, t, prof, P):
        """Assist Image of Profile Check (repro definition; the genuine's processing is not decoded):
        frame t through the spatial stage only (spatial_ui with the residual variance 1 that
        temporal_v5 gives without temporal filtering), as RGB of the whole frame."""
        planes = self._crop_planes(self.frames[t], None)
        out = spatial_ui(planes, prof, [np.ones(p.shape, np.float32) for p in planes], P)
        return self.to_rgb([np.clip(o, 0, 255) for o in out])

    def to_rgb(self, planes):
        return np.clip(nv.yuv_to_rgb(planes, self.matrix, self.full_range), 0, 255)


# ---------------- profiling helpers ----------------
def profile_window(frames, f):
    """Profiling frame f plus up to two following frames (pair / more), as nv_ui.py does."""
    f = max(0, min(f, len(frames) - 1))
    planes = tuple(np.asarray(p, np.float32) for p in frames[f])
    more = [tuple(np.asarray(p, np.float32) for p in frames[i]) for i in range(f + 1, min(f + 3, len(frames)))]
    return f, planes, more


def build_profile(frames, f, full_range, matrix, mode, area=None, cancel=None):
    f, planes, more = profile_window(frames, f)
    build = nv_native.build_profile_v5 if native_enabled() else nv.build_profile_v5
    prof = build(planes, full_range, area=area, pair=more[0] if more else None, mode=mode,
                 more=more, matrix=matrix, cancel=cancel)
    prof["ui_frame"] = f
    return prof


def noisiest_frame(frames, samples=32, cancel=None):
    """Most Noisy Frame (repro heuristic; the genuine criterion is not decoded): the sampled frame
    with the largest robust noise estimate median(|HH1|)/0.6745 of the luma (Haar diagonal band)."""
    n = len(frames)
    if n == 0:
        return 0
    last = max(n - 3, 0)                      # keep two following frames for the temporal profile
    cand = sorted(set(np.linspace(0, last, min(samples, last + 1)).round().astype(int).tolist()))
    best, best_s = cand[0], -1.0
    for i in cand:
        if cancel is not None and cancel():
            raise nv.ProfileCancelled("cancelled")
        Y = np.asarray(frames[i][0], np.float32)
        H, W = Y.shape[0] // 2 * 2, Y.shape[1] // 2 * 2
        a, b = Y[0:H:2, 0:W:2], Y[0:H:2, 1:W:2]
        c, d = Y[1:H:2, 0:W:2], Y[1:H:2, 1:W:2]
        s = float(np.median(np.abs(a - b - c + d) / 2.0)) / 0.6745
        if s > best_s:
            best, best_s = i, s
    return best


def hh1_sigma(Y):
    """Robust luma noise sigma median(|HH1|)/0.6745 (the noisiest_frame estimator) of a plane; used
    for the Noise Level labels of the Prepare page (repro display, not a genuine measure)."""
    Y = np.asarray(Y, np.float32)
    H, W = Y.shape[0] // 2 * 2, Y.shape[1] // 2 * 2
    if H < 2 or W < 2:
        return None
    a, b = Y[0:H:2, 0:W:2], Y[0:H:2, 1:W:2]
    c, d = Y[1:H:2, 0:W:2], Y[1:H:2, 1:W:2]
    return float(np.median(np.abs(a - b - c + d) / 2.0)) / 0.6745


def aft_frame(base, frames):
    """Index of the profile frame (clamped: a loaded profile may come from another clip)."""
    return max(0, min(int(base.get("ui_frame", 0)), len(frames) - 1))


def run_aft(base, planes, full_range, matrix, mode=0):
    """Auto Fine-Tune through nv_aft_v5.apply_aft (static AFT port; its factors were not compared
    with the genuine, Agents.md §6.5) on the profile frame `planes`. apply_aft multiplies the curve
    it is given, so it always starts from the curve before any earlier AFT (kept in
    'ui_pre_aft'); pressing Auto Fine-Tune again gives the same result. Returns a new profile.
    With the native core enabled, nv_native.apply_aft (the same result) is used."""
    return _run_aft(base, planes, full_range, matrix, mode)


def _run_aft(base, planes, full_range, matrix, mode=0, cancel=None, native=None):
    """run_aft; native: None = native_enabled(), True = the C path or None when it does not apply (no
    Python fallback), False = nv_aft_v5.apply_aft. cancel: asked by the C path (nv_native.Cancelled)."""
    import nv_aft_v5
    strict = native is True
    if native is None:
        native = native_enabled()
    prof = copy.deepcopy(base)
    pre = prof.pop("ui_pre_aft", None)
    if pre is not None:
        prof["curve"], prof["sigma"] = copy.deepcopy(pre["curve"]), dict(pre["sigma"])
    keep = {"curve": copy.deepcopy(prof["curve"]), "sigma": dict(prof["sigma"])}
    rgb = np.clip(nv.yuv_to_rgb(tuple(np.asarray(p, np.float32) for p in planes), matrix, full_range),
                  0, 255).astype(np.float32)
    res = nv_native.apply_aft(prof, rgb, mode=mode, cancel=cancel) if native else None
    if res is None and strict:
        return None
    prof = res if res is not None else nv_aft_v5.apply_aft(prof, rgb, mode=mode)
    prof.pop("aft_reset", None)     # a run after Reset Fine-Tuning rewrites the curve flags
    prof["ui_pre_aft"] = keep
    return prof


class Cancelled(Exception):
    pass


def run_in_child(task, fn, *args):
    """fn(*args) in a spawned child process that is terminated when task.cancelled is set (AFT
    runs pure-Python code for seconds to a minute and cannot be interrupted in a thread).
    fn = run_aft with the native core enabled: runs here (about a second) and asks task.cancelled in
    the AFT tile loop; inputs the C path does not reproduce go to the child process as before."""
    if fn is run_aft and native_enabled():
        if task.cancelled:
            raise Cancelled("cancelled")
        try:
            res = _run_aft(*args, cancel=lambda: task.cancelled, native=True)
        except nv_native.Cancelled:
            raise Cancelled("cancelled") from None
        if task.cancelled:
            raise Cancelled("cancelled")
        if res is not None:
            return res
    import multiprocessing
    ctx = multiprocessing.get_context("spawn")
    pool = ctx.Pool(1)
    try:
        r = pool.apply_async(fn, args)
        while not r.ready():
            if task.cancelled:
                pool.terminate()
                raise Cancelled("cancelled")
            r.wait(0.1)
        return r.get()
    finally:
        pool.terminate()
        pool.join()
