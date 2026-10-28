# N2a: libnvcore.dylib through repro/nv_native.py (ctypes) against the Python path, end to end, BIT FOR BIT.
#
#   make -C native lib && python3 native/verify_bridge.py [--jobs N] [--quick] [--tmp DIR] [--only GROUP,..]
#                                                        [--bench] [--export]
#
# The Python path is the reference: repro/nv_ui_qt/backend.py with the native core off (set_native(False):
# nvfilter.build_profile_v5 / nv_aft_v5.apply_aft / nvfilter.temporal_v5 + spatial_v5) and on
# (set_native(True): repro/nv_native.py).  Frames are read with denoise_video_v5.read_frames as the UI reads
# them (yuv420p uint8; test_v02 n = 306..320 BT.709, test.mp4 n = 298..308 BT.601) and cached in --tmp.
#
# Groups (each case compares every leaf: dict keys in order, Python types, floats / arrays by their bits):
#   curve    nv_curve.genuine_noise_curve vs nv_native.noise_curve (-j 1 / 3) and nvfilter.find_flat_area
#            vs nv_native.find_flat_area (area + status)
#   profile  backend.build_profile (modes auto / spatial / temporal; FindFlatArea search) off vs on
#   aft      the raw BuildNoiseModel blob (nv_profile.build_profile) and the AFT blob (nv_aft.auto_fine_tune)
#            vs nv_native.profile_blob / aft_blob (-j 1 / 3), and backend.run_aft's profile dict off vs on
#   frame    backend.process_frame (temporal + spatial_ui + mix) off vs on for test_v02 n = 310..316 with the
#            AFT profile of n = 312: defaults on all 7 frames, 7 parameter variants (noise level +/-,
#            threshold, pyramid, per-channel spatial + Temporal Noise Level, mix / radius 1 / ultra-low,
#            temporal 0, int parameters / bs 8) on 310 / 313 / 316, a Manual Fine-Tune profile; the
#            temporal stage (out, rv and the flows cache, empty and pre-filled); PreviewEngine with a crop
#            and the Assist Image; test.mp4 n = 300..304 (defaults, per-channel)
#   cancel   FindFlatArea (status 5, area, number of queries at k = 1, 120, 240, 241, 242, 247, never;
#            -j 1 / 3) and the AFT tile loop (number of queries, Cancelled) against the Python ports;
#            backend.run_in_child(task, run_aft) with a task cancelled during the AFT
#   fallback NV_NATIVE_LIB=<missing> in a child process (AVAILABLE False, backend uses the Python path);
#            NV_NATIVE=0; inputs the C path does not reproduce (np.float64 parameter, unknown similarity)
#   --export denoise_video_v5 export (nv_ui_qt.export, CRF 0, 4 frames) with NV_NATIVE=1 and 0: same bytes
#   --bench  timing table (1080p test_v02 n = 312, 1 thread)
import argparse
import json
import os
import struct
import subprocess
import sys
import time
from concurrent.futures import ProcessPoolExecutor

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
REPRO = os.path.join(ROOT, "repro")
if REPRO not in sys.path:
    sys.path.insert(0, REPRO)
sys.dont_write_bytecode = True
F32 = np.float32

VIDEOS = {"v02": (os.path.join(ROOT, "test_v02.mp4"), 306, 321, "bt709"),
          "t": (os.path.join(ROOT, "test.mp4"), 298, 309, "bt601")}
TMP = os.environ.get("NV_BRIDGE_TMP", os.path.join(HERE, "tmp_bridge"))   # main() sets it for the workers


# ------------------------------------------------------------------------------------- frames
def frames(video):
    """{n: (Y, Cb, Cr) uint8} of the cached range (denoise_video_v5.read_frames, as the UI loads them)."""
    path, lo, hi, _ = VIDEOS[video]
    cache = os.path.join(TMP, f"frames_{video}_{lo}_{hi}.npz")
    if not os.path.exists(cache):
        from denoise_video_v5 import probe, read_frames
        info = probe(path)
        d = {}
        for i, f in read_frames(path, info["W"], info["H"], lo, hi, info["fps"]):
            for c, p in zip("ybr", f):
                d[f"{c}{i}"] = np.array(p)
        np.savez(cache, **d)
    z = np.load(cache)
    return {n: tuple(z[f"{c}{n}"] for c in "ybr") for n in range(lo, hi)}


def frame_list(video):
    fr = frames(video)
    lo = min(fr)
    return [fr[n] for n in sorted(fr)], lo


# ------------------------------------------------------------------------------------- compare
def compare(a, b, path="$", out=None):
    """[words, diffs, first differences] of two Python objects: same types, dict keys in order, floats
    and arrays by their bits."""
    if out is None:
        out = [0, 0, []]

    def bad(msg):
        out[1] += 1
        if len(out[2]) < 5:
            out[2].append(f"{path}: {msg}")
    if type(a) is not type(b):
        out[0] += 1
        bad(f"type {type(a).__name__} != {type(b).__name__}")
    elif isinstance(a, dict):
        if list(a) != list(b):
            out[0] += 1
            bad(f"keys {list(a)} != {list(b)}")
        for k in a:
            if k in b:
                compare(a[k], b[k], f"{path}.{k}", out)
    elif isinstance(a, (list, tuple)):
        if len(a) != len(b):
            out[0] += 1
            bad(f"len {len(a)} != {len(b)}")
        for i, (x, y) in enumerate(zip(a, b)):
            compare(x, y, f"{path}[{i}]", out)
    elif isinstance(a, np.ndarray):
        out[0] += max(a.size, 1)
        if a.dtype != b.dtype or a.shape != b.shape:
            bad(f"array {a.dtype}{a.shape} != {b.dtype}{b.shape}")
        else:
            n = int(np.count_nonzero(np.frombuffer(np.ascontiguousarray(a).tobytes(), np.uint8).reshape(a.size, -1)
                                     != np.frombuffer(np.ascontiguousarray(b).tobytes(), np.uint8).reshape(b.size, -1))
                    ) if a.size else 0
            if n:
                out[1] += n - 1
                bad(f"{n} array words differ")
    elif isinstance(a, float):
        out[0] += 1
        if struct.pack("<d", a) != struct.pack("<d", b):
            bad(f"{a!r} != {b!r}")
    elif isinstance(a, np.generic):
        out[0] += 1
        if a.dtype != b.dtype or a.tobytes() != b.tobytes():
            bad(f"{a!r} != {b!r}")
    elif isinstance(a, (bytes, bytearray)):
        out[0] += max(len(a), 1)
        n = sum(x != y for x, y in zip(a, b)) + abs(len(a) - len(b))
        if n:
            out[1] += n - 1
            bad(f"{n} bytes differ")
    else:
        out[0] += 1
        if a != b:
            bad(f"{a!r} != {b!r}")
    return out


def result(group, label, cmp, t_py=None, t_nat=None, extra=""):
    w, d, first = cmp
    return dict(group=group, label=label, words=w, diffs=d, first=first, t_py=t_py, t_nat=t_nat, extra=extra)


def _setup():
    os.environ.pop("NV_NATIVE", None)
    from nv_ui_qt import backend
    import nvfilter as nv
    import nv_native
    assert nv_native.AVAILABLE, "libnvcore.dylib not loaded (make -C native lib)"
    return backend, nv, nv_native


def _clear(nv, nn):
    nv._FFA_CACHE.clear()
    nn._FFA_CACHE.clear()


def _timed(fn, *a, **k):
    t = time.perf_counter()
    r = fn(*a, **k)
    return r, time.perf_counter() - t


def _rgb(nv, planes, matrix):
    return np.clip(np.nan_to_num(nv.yuv_to_rgb(tuple(np.asarray(p, F32) for p in planes), matrix, False), nan=0.0,
                                 posinf=255.0, neginf=0.0), 0, 255).astype(F32)


# ------------------------------------------------------------------------------------- cases
def case_curve(video, n):
    backend, nv, nn = _setup()
    import nv_curve
    fr = frames(video)
    matrix = VIDEOS[video][3]
    rgb = _rgb(nv, fr[n], matrix)
    out = []
    ref, tp = _timed(nv_curve.genuine_noise_curve, rgb)
    ref = (list(map(float, ref[0])), list(map(float, ref[1])))
    for j in (1, 3):
        got, tn = _timed(nn.noise_curve, rgb, threads=j)
        out.append(result("curve", f"{video} n={n} noise_curve -j {j}", compare(ref, got), tp, tn))
    _clear(nv, nn)
    ref, tp = _timed(nv.find_flat_area, fr[n], False, matrix, with_status=True)
    for j in (1, 3):
        _clear(nv, nn)
        got, tn = _timed(nn.find_flat_area, fr[n], False, matrix, threads=j)
        out.append(result("curve", f"{video} n={n} find_flat_area -j {j} -> {list(got)}", compare(ref, got), tp, tn))
    return out


def case_profile(video, n, mode, area=None):
    backend, nv, nn = _setup()
    fl, lo = frame_list(video)
    matrix = VIDEOS[video][3]
    res = {}
    for on in (False, True):
        backend.set_native(on)
        _clear(nv, nn)
        res[on] = _timed(backend.build_profile, fl, n - lo, False, matrix, mode, area)
    lab = f"{video} n={n} {mode}" + (f" area {list(area)}" if area else "") + \
        f" -> area {res[True][0]['area']} status {res[True][0]['flat_status']}"
    return [result("profile", lab, compare(res[False][0], res[True][0]), res[False][1], res[True][1])]


def base_profile(video, n):
    """backend.build_profile (Python path, auto) of frame n, cached as JSON (Python floats)."""
    path = os.path.join(TMP, f"prof_{video}_{n}.json")
    if not os.path.exists(path):
        backend, nv, nn = _setup()
        backend.set_native(False)
        fl, lo = frame_list(video)
        prof = backend.build_profile(fl, n - lo, False, VIDEOS[video][3], "auto")
        with open(path + ".part", "w") as fh:
            json.dump(prof, fh)
        os.replace(path + ".part", path)
    with open(path) as fh:
        return json.load(fh)


def aft_profile(video, n):
    """backend.run_aft (Python path) of base_profile(video, n), cached as JSON (made by case_aft)."""
    with open(os.path.join(TMP, f"aft_{video}_{n}.json")) as fh:
        return json.load(fh)


def case_aft(video, n, mode=0, blobs=True):
    backend, nv, nn = _setup()
    fr = frames(video)
    matrix = VIDEOS[video][3]
    base = base_profile(video, n)
    planes = fr[n]
    out = []
    res = {}
    for on in (False, True):
        backend.set_native(on)
        res[on] = _timed(backend.run_aft, base, planes, False, matrix, mode)
    out.append(result("aft", f"{video} n={n} run_aft mode {mode} (area {base['area']})",
                      compare(res[False][0], res[True][0]), res[False][1], res[True][1]))
    if mode == 0:
        path = os.path.join(TMP, f"aft_{video}_{n}.json")
        with open(path + ".part", "w") as fh:
            json.dump(res[False][0], fh)
        os.replace(path + ".part", path)
    if blobs:
        import nv_profile as npr
        import nv_aft_v5  # noqa: F401  (sys.path of aft_port)
        import nv_aft
        rgb = np.clip(nv.yuv_to_rgb(tuple(np.asarray(p, F32) for p in planes), matrix, False), 0, 255).astype(F32)
        ay, ax, ah = base["area"][:3]
        aw = base["area"][3] if len(base["area"]) == 4 else ah
        (raw_py, _), tp = _timed(npr.build_profile, rgb, 1, (ax, ay, aw, ah))
        raw_py = bytes(raw_py)
        aft_py, tpa = _timed(nv_aft.auto_fine_tune, rgb, raw_py, mode=mode)
        for j in (1, 3):
            raw_n, tn = _timed(nn.profile_blob, rgb, (ax, ay, aw, ah), 1, j)
            out.append(result("aft", f"{video} n={n} BuildNoiseModel blob 0xc300 -j {j}", compare(raw_py, raw_n), tp, tn))
            aft_n, tna = _timed(nn.aft_blob, rgb, raw_py, mode, j)
            out.append(result("aft", f"{video} n={n} AFT blob mode {mode} -j {j}", compare(aft_py, aft_n), tpa, tna))
    return out


VARIANTS = {
    "default": {},
    "noise+": {"noise_level": 1.4, "threshold": 2.5},
    "noise-": {"noise_level": 0.6, "threshold": 6.0, "temporal": 0.8},
    "pyramid": {"similarity": "pyramid", "pyramid_scale": 2.0},
    "perchan": {"ui": {"bamt_Cr_medium": 60.0, "bamt_Cb_low": 30.0, "amt_Y": 50.0, "bnoise_Y_high": 130.0,
                       "t_noise": 60.0}},
    "mix_r1": {"mix": 0.25, "radius": 1, "spatial": 3.0, "band_amounts": {"ultra-low": 0.5}},
    "temporal0": {"temporal": 0.0},
    "int_bs8": {"threshold": 4, "noise_level": 1, "bs": 8, "search": 8},
}


def params(name):
    from nv_ui_qt.state import backend_params, default_params
    v = VARIANTS[name]
    ui = default_params()
    ui.update(v.get("ui", {}))
    P = backend_params(ui)
    for k, x in v.items():
        if k == "ui":
            continue
        if isinstance(x, dict):
            P[k] = dict(P[k], **x)
        else:
            P[k] = x
    return P


def _window(fr, c, R):
    ids = [i for i in range(c - R, c + R + 1) if i in fr]
    return [tuple(p.astype(F32) for p in fr[i]) for i in ids], ids.index(c)


def case_frame(video, n, variant, prof_kind="aft"):
    backend, nv, nn = _setup()
    fr = frames(video)
    pn = 312 if video == "v02" else 302
    prof = aft_profile(video, pn)
    if prof_kind == "tuned":
        from nv_ui_qt.state import effective_profile
        f = [[1.0, 1.2, 0.8, 1.1, 1.0, 0.9, 1.3, 1.0, 0.7], [1.0] * 9, [0.9] * 9]
        prof = effective_profile(prof, f, {"all": 1.1, "Cr": 0.8})
    P = params(variant)
    win, ci = _window(fr, n, 2)                 # backend.process_frame cuts the window to the radius
    res = {}
    for on in (False, True):
        backend.set_native(on)
        res[on] = _timed(backend.process_frame, win, ci, prof, P)
    lab = f"{video} n={n} process_frame {variant}" + ("" if prof_kind == "aft" else f" ({prof_kind} profile)")
    out = [result("frame", lab, compare(res[False][0], res[True][0]), res[False][1], res[True][1])]
    if variant in ("default", "pyramid", "int_bs8"):
        # the temporal stage alone, with the flows cache empty and pre-filled
        st = {}
        for on in (False, True):
            backend.set_native(on)
            flows = {}
            tmp, rv = backend.temporal(win, ci, prof, P, flows)
            st[on] = (tmp, rv, flows)
        out.append(result("frame", f"{video} n={n} temporal {variant}: out, rv, flows {{}}", compare(st[False], st[True])))
        pre = {}
        for on in (False, True):
            backend.set_native(on)
            flows = {k: v for k, v in st[False][2].items() if k < 0}     # half of the vectors given
            tmp, rv = backend.temporal(win, ci, prof, P, flows)
            pre[on] = (tmp, rv, flows)
        out.append(result("frame", f"{video} n={n} temporal {variant}: pre-filled flows", compare(pre[False], pre[True])))
    return out


def case_engine(video, n):
    backend, nv, nn = _setup()
    fr = frames(video)
    pn = 312 if video == "v02" else 302
    prof = aft_profile(video, pn)
    fl, lo = frame_list(video)
    P = params("perchan")
    matrix = VIDEOS[video][3]
    H, W = fl[0][0].shape
    crop = (H // 4 // 2 * 2, W // 4 // 2 * 2, H // 2 // 2 * 2, W // 2 // 2 * 2)
    out = []
    res = {}
    for on in (False, True):
        backend.set_native(on)
        eng = backend.PreviewEngine(fl, matrix, False)
        a = eng.process(n - lo, prof, 1, P, crop)
        b = eng.process(n - lo, prof, 1, dict(P, luma=0.4))       # spatial-only change: cached temporal
        m = eng.mask(n - lo, prof, 1, P)
        s = eng.assist(n - lo, prof, P)
        res[on] = (a, b, m, s)
    out.append(result("frame", f"{video} n={n} PreviewEngine crop {list(crop)} + full + mask + assist",
                      compare(res[False], res[True])))
    return out


def case_threads():
    """The UI calls the library from several threads at once (preview, profile, AFT workers): native results
    computed concurrently from 4 Python threads == the Python path computed one by one."""
    backend, nv, nn = _setup()
    import threading
    fr = frames("v02")
    prof = aft_profile("v02", 312)
    base = base_profile("v02", 312)
    fl, lo = frame_list("v02")
    jobs = [("frame", 311, "default"), ("frame", 314, "perchan"), ("aft", 312, None), ("profile", 315, None)]

    def run(kind, n, v):
        if kind == "frame":
            win, ci = _window(fr, n, 2)
            return backend.process_frame(win, ci, prof, params(v))
        if kind == "aft":
            return backend.run_aft(base, fr[n], False, "bt709")
        return backend.build_profile(fl, n - lo, False, "bt709", "auto")
    backend.set_native(False)
    _clear(nv, nn)
    ref = [run(*j) for j in jobs]
    backend.set_native(True)
    _clear(nv, nn)
    got = [None] * len(jobs)

    def worker(i):
        got[i] = run(*jobs[i])
    th = [threading.Thread(target=worker, args=(i,)) for i in range(len(jobs))]
    for t in th:
        t.start()
    for t in th:
        t.join()
    return [result("frame", f"concurrent native calls from 4 threads: {[j[0] for j in jobs]}", compare(ref, got))]


def case_cancel():
    backend, nv, nn = _setup()
    fr = frames("v02")
    planes = fr[312]
    out = []

    def counter(k):
        c = [0]

        def f():
            c[0] += 1
            return c[0] == k or (k > 0 and c[0] > k)
        return f, c
    for k in (1, 120, 240, 241, 242, 247, 0):
        f, c = counter(k)
        _clear(nv, nn)
        ref = nv.find_flat_area(planes, False, "bt709", with_status=True, cancel=f)
        nref = c[0]
        for j in (1, 3):
            g, d = counter(k)
            _clear(nv, nn)
            got = nn.find_flat_area(planes, False, "bt709", cancel=g, threads=j)
            out.append(result("cancel", f"find_flat_area cancel at query {k or 'never'} -j {j}: {list(got)}, "
                              f"{d[0]} queries", compare((ref, nref), (got, d[0]))))
    # AFT tile loop: the Python port's queries vs the C port's
    import nv_profile as npr
    import nv_aft_v5  # noqa: F401
    import nv_aft
    base = base_profile("v02", 312)
    rgb = np.clip(nv.yuv_to_rgb(tuple(np.asarray(p, F32) for p in planes), "bt709", False), 0, 255).astype(F32)
    ay, ax, ah = base["area"][:3]
    raw = bytes(npr.build_profile(rgb, 1, (ax, ay, ah, ah))[0])
    for k in (1, 100):
        f, c = counter(k)
        py = nv_aft.auto_fine_tune(rgb, raw, mode=0, cancel=f)
        for j in (1, 3):
            g, d = counter(k)
            try:
                nn.aft_blob(rgb, raw, 0, j, g)
                got = "no cancel"
            except nn.Cancelled:
                got = "Cancelled"
            exp_py = bytearray(raw)
            exp_py[0x2d] = 0
            ok = py == bytes(exp_py) and got == "Cancelled"
            out.append(result("cancel", f"AFT cancel at tile query {k} -j {j}: {got}, {d[0]} queries "
                              f"(Python {c[0]}; Python blob = raw with 0x2d = 0: {py == bytes(exp_py)})",
                              compare((c[0], True), (d[0], ok))))
    # never cancelled: number of queries = number of tiles
    for j in (1, 3):
        g, d = counter(0)
        nn.aft_blob(rgb, raw, 0, j, g)
        nt = (rgb.shape[1] // 32) * (rgb.shape[0] // 32)
        out.append(result("cancel", f"AFT without cancel -j {j}: {d[0]} queries (tiles {nt})", compare(nt, d[0])))

    # run_in_child(task, run_aft): the task is cancelled during the AFT (in this process)
    class Task:
        def __init__(self, after):
            self.n, self.after = 0, after

        @property
        def cancelled(self):
            self.n += 1
            return self.n > self.after
    backend.set_native(True)
    for after in (0, 2, 50):
        t = Task(after)
        t0 = time.perf_counter()
        try:
            backend.run_in_child(t, backend.run_aft, base, planes, False, "bt709")
            got = "returned"
        except backend.Cancelled:
            got = "Cancelled"
        dt = time.perf_counter() - t0
        out.append(result("cancel", f"run_in_child(task, run_aft) cancelled after {after} queries: {got} in {dt:.2f} s",
                          compare("Cancelled", got)))
    return out


def case_fallback():
    out = []
    code = ("import sys; sys.path.insert(0, %r); import numpy as np; from nv_ui_qt import backend; import nvfilter as nv;"
            "import nv_native as nn; z = np.load(%r);"
            "fr = [tuple(z[c + str(i)].astype(np.float32) for c in 'ybr') for i in range(300, 305)];"
            "prof = backend.build_profile([tuple(z[c + str(i)] for c in 'ybr') for i in range(300, 303)], 0, False, 'bt601', 'auto');"
            "P = nv.v5_params(); a = backend.process_frame(fr, 2, prof, P); b = nv.process_frame_v5(fr, 2, prof, P);"
            "print(nn.AVAILABLE, backend.native_available(), backend.native_enabled(),"
            " all(np.array_equal(x.view(np.uint32), y.view(np.uint32)) for x, y in zip(a, b)), nn.STATS)")
    frames("t")
    npz = os.path.join(TMP, "frames_t_298_309.npz")
    for label, env, exp in (("NV_NATIVE_LIB=<missing>", {"NV_NATIVE_LIB": os.path.join(TMP, "missing.dylib")},
                             "False False False True {'native': 0, 'fallback': 0}"),
                            ("NV_NATIVE=0", {"NV_NATIVE": "0"}, "True True False True {'native': 0, 'fallback': 0}"),
                            ("default (library found)", {}, "True True True True {'native': 2, 'fallback': 0}")):
        e = dict(os.environ)
        e.pop("NV_NATIVE", None)
        e.pop("NV_NATIVE_LIB", None)
        e.update(env)
        r = subprocess.run([sys.executable, "-c", code % (REPRO, npz)], env=e, capture_output=True, text=True, cwd=REPRO)
        got = r.stdout.strip().splitlines()[-1] if r.stdout.strip() else r.stderr.strip()[-300:]
        out.append(result("fallback", f"{label}: AVAILABLE, available, enabled, == process_frame_v5, STATS = {got}",
                          compare(exp, got)))
    # inputs outside the C path: the Python function runs (np.float64 promotes in numpy)
    backend, nv, nn = _setup()
    fr = frames("t")
    prof = aft_profile("t", 302)
    win, ci = _window(fr, 302, 2)
    for label, P in (("noise_level np.float64(1.1)", dict(nv.v5_params(), noise_level=np.float64(1.1))),
                     ("channel_noise Cb np.float32", dict(nv.v5_params(), channel_noise={"Y": 1.0, "Cb": np.float32(1.2),
                                                                                         "Cr": 1.0}))):
        s0 = dict(nn.STATS)
        backend.set_native(True)
        a = backend.process_frame(win, ci, prof, P)
        backend.set_native(False)
        b = backend.process_frame(win, ci, prof, P)
        fb = nn.STATS["fallback"] - s0["fallback"]
        out.append(result("fallback", f"{label}: Python path used ({fb} fallbacks), dtype {a[0].dtype}",
                          compare((b, 2), (a, fb))))
    # set_native writes NV_NATIVE: a process started afterwards (the export and its spawned frame workers)
    # follows the UI setting
    import multiprocessing
    for on in (False, True, False):
        backend.set_native(on)
        with multiprocessing.get_context("spawn").Pool(1) as pool:
            got = pool.apply(_child_native)
        out.append(result("fallback", f"set_native({on}) -> spawned child: native_enabled() = {got}", compare(on, got)))
    backend.set_native(True)
    P = dict(nv.v5_params(), similarity="bogus")
    backend.set_native(True)
    try:
        backend.process_frame(win, ci, prof, P)
        got = "no error"
    except ValueError as e:
        got = f"ValueError: {e}"
    backend.set_native(False)
    try:
        backend.process_frame(win, ci, prof, P)
        exp = "no error"
    except ValueError as e:
        exp = f"ValueError: {e}"
    out.append(result("fallback", f"similarity 'bogus': {got}", compare(exp, got)))
    return out


def _child_native():
    from nv_ui_qt import backend
    return backend.native_enabled()


def run_export(tmp):
    """nv_ui_qt.export with the native core on / off: 4 frames, CRF 0, the AFT profile and per-channel params."""
    out = []
    for video, src, start in (("t", os.path.join(ROOT, "test.mp4"), 300), ("v02", os.path.join(ROOT, "test_v02.mp4"), 310)):
        pn = 312 if video == "v02" else 302
        prof = aft_profile(video, pn)
        P = params("perchan")
        fp, pp = os.path.join(tmp, f"exp_{video}_prof.json"), os.path.join(tmp, f"exp_{video}_params.json")
        json.dump(prof, open(fp, "w"))
        json.dump(P, open(pp, "w"))
        files, times = {}, {}
        for on in ("1", "0"):
            dst = os.path.join(tmp, f"exp_{video}_native{on}.mp4")
            if os.path.exists(dst):
                os.remove(dst)
            e = dict(os.environ, NV_NATIVE=on)
            e.pop("NV_NATIVE_LIB", None)
            t0 = time.perf_counter()
            r = subprocess.run([sys.executable, "-m", "nv_ui_qt.export", src, dst, "--ui-profile", fp, "--params", pp,
                                "--start", str(start), "--frames", "4", "--crf", "0", "--jobs", "3", "--audio", "none"],
                               env=e, cwd=REPRO, capture_output=True, text=True)
            times[on] = time.perf_counter() - t0
            if r.returncode != 0:
                out.append(result("export", f"{video} NV_NATIVE={on} failed: {r.stderr[-400:]}", [1, 1, ["failed"]]))
                files[on] = b""
                continue
            files[on] = open(dst, "rb").read()
        dec = {}
        for on in ("1", "0"):
            dst = os.path.join(tmp, f"exp_{video}_native{on}.mp4")
            dec[on] = subprocess.run(["ffmpeg", "-v", "error", "-i", dst, "-f", "rawvideo", "-pix_fmt", "yuv420p", "-"],
                                     capture_output=True).stdout
        out.append(result("export", f"{video} n={start}..{start + 3} export file bytes NV_NATIVE=1 vs 0 "
                          f"({len(files['1'])} bytes; {times['1']:.1f} s vs {times['0']:.1f} s)",
                          compare(files["0"], files["1"])))
        out.append(result("export", f"{video} decoded yuv420p frames ({len(dec['1'])} bytes)", compare(dec["0"], dec["1"])))
    return out


def bench():
    """Best of 2 (Python AFT: 1 run), fresh FindFlatArea caches; 1080p test_v02 n = 312.  Rows: (name, Python
    function, native function, whether the native call takes threads)."""
    backend, nv, nn = _setup()
    fl, lo = frame_list("v02")
    fr = frames("v02")
    base = base_profile("v02", 312)
    prof = aft_profile("v02", 312)
    win, ci = _window(fr, 312, 2)
    planes = fr[312]

    def ui(fn):
        def run(on):
            backend.set_native(on)
            return fn()
        return run
    stages = [("find_flat_area (curve + search)", lambda on: (nn.find_flat_area if on else
                                                              lambda *a: nv.find_flat_area(*a, with_status=True))(
                                                                  planes, False, "bt709"), True),
              ("build_profile (auto: FindFlatArea + the rest in Python)",
               ui(lambda: backend.build_profile(fl, 312 - lo, False, "bt709", "auto")), True),
              ("run_aft (BuildNoiseModel 128x128 + AFT + combine)",
               ui(lambda: backend.run_aft(base, fr[312], False, "bt709")), True),
              ("process_frame (defaults, radius 2)", ui(lambda: backend.process_frame(win, ci, prof, params("default"))),
               False),
              ("process_frame (per-channel spatial)", ui(lambda: backend.process_frame(win, ci, prof, params("perchan"))),
               False)]

    def best(fn, on, reps):
        t = 1e9
        for _ in range(reps):
            _clear(nv, nn)
            t = min(t, _timed(fn, on)[1])
        return t
    rows = []
    for name, fn, threaded in stages:
        tp = best(fn, False, 1 if name.startswith("run_aft") else 2)
        nn.THREADS = 1
        tn = best(fn, True, 3)
        tm = None
        if threaded:
            nn.THREADS = 4
            tm = best(fn, True, 3)
        rows.append((name, tp, tn, tm))
    nn.THREADS = 1
    print("\n| stage (1080p test_v02 n=312) | Python [s] | native 1 thread [s] | speed-up | native 4 threads [s] |")
    print("|---|---|---|---|---|")
    for name, tp, tn, tm in rows:
        print(f"| {name} | {tp:.3f} | {tn:.3f} | {tp / tn:.1f}x | {'-' if tm is None else f'{tm:.3f}'} |")
    print(f"(load average {os.getloadavg()})")


def run_case(args):
    fn, a = args
    t = time.perf_counter()
    try:
        r = globals()[fn](*a)
    except Exception as e:  # noqa: BLE001
        import traceback
        r = [result(fn, f"{a} raised {type(e).__name__}: {e}", [1, 1, [traceback.format_exc()[-600:]]])]
    for x in r:
        x["case_time"] = time.perf_counter() - t
    return r


def main():
    global TMP
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", type=int, default=3)
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--tmp", default=TMP)
    ap.add_argument("--only", default="curve,profile,aft,frame,cancel,fallback")
    ap.add_argument("--bench", action="store_true")
    ap.add_argument("--export", action="store_true")
    A = ap.parse_args()
    TMP = os.path.abspath(A.tmp)
    os.environ["NV_BRIDGE_TMP"] = TMP
    os.makedirs(TMP, exist_ok=True)
    groups = set(A.only.split(",")) if A.only else set()
    frames("v02")
    frames("t")
    stages = []
    v02 = (310, 313, 316) if not A.quick else (312,)
    if "curve" in groups:
        stages.append([("case_curve", ("v02", n)) for n in v02] + [("case_curve", ("t", n)) for n in (300, 304)])
    if "profile" in groups:
        st = [("case_profile", ("v02", n, "auto")) for n in v02]
        st += [("case_profile", ("v02", 312, m)) for m in ("spatial", "temporal")]
        st += [("case_profile", ("t", n, "auto")) for n in (300, 304)] + [("case_profile", ("t", 302, "temporal"))]
        st += [("case_profile", ("v02", 312, "auto", (40, 60, 216, 512)))]
        stages.append(st)
    if groups & {"aft", "frame", "cancel", "fallback"} or A.export or A.bench:
        st = [("case_aft", ("v02", n)) for n in sorted(set(v02) | {312})]
        st += [("case_aft", ("t", 302)), ("case_aft", ("t", 300, 1, False))]
        if "aft" not in groups:     # only the profiles the other groups need (made once)
            st = [("case_aft", (v, n, 0, False)) for v, n in (("v02", 312), ("t", 302))
                  if not os.path.exists(os.path.join(TMP, f"aft_{v}_{n}.json"))]
        stages.append(st)
    if "frame" in groups:
        frames_d = range(310, 317) if not A.quick else (312,)
        st = [("case_frame", ("v02", n, "default")) for n in frames_d]
        st += [("case_frame", ("v02", n, v)) for n in v02 for v in VARIANTS if v != "default"]
        st += [("case_frame", ("v02", 313, "default", "tuned")), ("case_engine", ("v02", 312))]
        st += [("case_frame", ("t", n, v)) for n in (300, 302, 304) for v in ("default", "perchan")]
        st += [("case_engine", ("t", 302)), ("case_threads", ())]
        stages.append(st)
    if "cancel" in groups:
        stages.append([("case_cancel", ())])
    if "fallback" in groups:
        stages.append([("case_fallback", ())])
    rows = []
    t0 = time.perf_counter()
    with ProcessPoolExecutor(A.jobs) as ex:
        for st in stages:
            for r in ex.map(run_case, st):
                for x in r:
                    rows.append(x)
                    tp = "" if x["t_py"] is None else f"  py {x['t_py']:.2f}s nat {x['t_nat']:.3f}s"
                    print(f"[{x['group']}] {'OK ' if x['diffs'] == 0 else 'NG '} {x['label']}: "
                          f"{x['words'] - x['diffs']}/{x['words']} words{tp}", flush=True)
                    for f in x["first"] if x["diffs"] else ():
                        print("      ", f)
    if A.export:
        for x in run_export(TMP):
            rows.append(x)
            print(f"[{x['group']}] {'OK ' if x['diffs'] == 0 else 'NG '} {x['label']}: "
                  f"{x['words'] - x['diffs']}/{x['words']} words", flush=True)
            for f in x["first"] if x["diffs"] else ():
                print("      ", f)
    by = {}
    for x in rows:
        g = by.setdefault(x["group"], [0, 0, 0, 0])
        g[0] += 1
        g[1] += x["diffs"] == 0
        g[2] += x["words"]
        g[3] += x["diffs"]
    print("\nsummary (cases ok/total, words equal/total):")
    for g, (n, ok, w, d) in by.items():
        print(f"  {g:9s} {ok}/{n} cases, {w - d}/{w} words")
    bad = sum(n - ok for n, ok, _, _ in by.values())
    print(f"{'ALL MATCH' if not bad else f'{bad} CASES DIFFER'} ({time.perf_counter() - t0:.0f} s)")
    if A.bench:
        bench()
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
