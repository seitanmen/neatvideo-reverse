# N3a: the UI numerics of libnvcore.dylib (nvcore_c_ui.h) against the Python UI code, BIT FOR BIT.
#
#   make -C native lib && python3 native/verify_bridge_ui.py [--jobs N] [--quick] [--tmp DIR]
#   (also a group of native/verify_bridge.py: --only ui)
#
# Reference: nvfilter.yuv_to_rgb (with the UI's clips), repro/nv_ui_qt/display.py (render_simple, freq_images,
# render_freq, minmax, noise_level, not_uniform) and repro/nv_ui_qt/quality.py (profile_numbers).
# Inputs: the frames native/verify_bridge.py caches (test_v02 n = 306..320 BT.709, test.mp4 n = 298..308
# BT.601, yuv420p uint8 as the UI reads them) as the viewer's RGB (np.clip(yuv_to_rgb(..), 0, 255) float32),
# crops of them of many sizes (odd, < 4 px, 16..512 px), synthetic RGB (noise, flat, clipped, gradients) and
# random planes for yuv_to_rgb (4:2:0 / 4:4:4 / odd chroma sizes, NaN / inf, full and limited range).
# Quality: profiles of backend.build_profile (Python path) and backend.run_aft on test frames with random
# factors / tune, AFT flags, older profiles without flags and Reset Fine-Tuning.
import argparse
import ctypes
import os
import sys
import time
import zlib
from concurrent.futures import ProcessPoolExecutor

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
REPRO = os.path.join(ROOT, "repro")
for p in (REPRO, HERE):
    if p not in sys.path:
        sys.path.insert(0, p)
sys.dont_write_bytecode = True
F32 = np.float32
P = ctypes.c_void_p
MATRIX = {"bt709": 0, "bt601": 1, "bt2020": 2}
SIMPLE = ["rgb", "R", "G", "B", "Y", "Cr", "Cb"]
KINDS = ["Y+", "Y", "Cr", "Cb"]
RK = {"Y": 0, "R": 1, "G": 2, "B": 3, "Cr": 4, "Cb": 5}
BRIGHT = (0, 1, 13, 37, 49, 50, 51, 62, 75, 88, 99, 100)
_LIB = None


def lib():
    global _LIB
    if _LIB is None:
        L = ctypes.CDLL(os.environ.get("NV_NATIVE_LIB", os.path.join(HERE, "libnvcore.dylib")))
        L.nvc_minmax.argtypes = [P, ctypes.c_size_t, P, P]
        L.nvc_render_simple.argtypes = [ctypes.c_int, P, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_double,
                                        ctypes.c_double, P]
        L.nvc_freq_images.argtypes = [P, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, P, P]
        L.nvc_render_freq.argtypes = [ctypes.c_int, P, ctypes.c_int, ctypes.c_int, ctypes.c_float, ctypes.c_float,
                                      ctypes.c_int, P]
        L.nvc_noise_level.argtypes = [P, ctypes.c_int, ctypes.c_int]
        L.nvc_noise_level.restype = ctypes.c_double
        L.nvc_not_uniform.argtypes = [P, ctypes.c_int, ctypes.c_int, P, P]
        L.nvc_quality_numbers.argtypes = [P, ctypes.c_int, ctypes.c_int, P, P, P, ctypes.c_int, ctypes.c_int, P, P, P]
        L.nvc_yuv_to_rgb.argtypes = [P, P, P] + [ctypes.c_int] * 7 + [P]
        L.nvc_mix.argtypes = [P, P] + [ctypes.c_int] * 4 + [ctypes.c_double]
        L.nvc_reference_floor.argtypes = [ctypes.c_char_p, ctypes.c_int, P]
        L.nvc_reference_floor.restype = ctypes.c_int
        L.nvc_temporal_mask.argtypes = [P, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_double, P]
        L.nvc_nlc_image.argtypes = [P, P, ctypes.c_int, ctypes.c_int, P]
        L.nvc_interp.argtypes = [P, P, ctypes.c_int, P, ctypes.c_int, P]
        L.nvc_mean_f32.argtypes = [P, ctypes.c_size_t]
        L.nvc_mean_f32.restype = ctypes.c_float
        _LIB = L
    return _LIB


def ptr(a):
    return a.ctypes.data_as(P)


def same(a, b):
    a, b = np.asarray(a), np.asarray(b)
    return a.dtype == b.dtype and a.shape == b.shape and a.tobytes() == b.tobytes()


def dbl_same(a, b):
    return np.float64(a).tobytes() == np.float64(b).tobytes()


# ------------------------------------------------------------------------------------- inputs
def video_rgbs(quick):
    import verify_bridge as vb
    import nvfilter as nv
    out = []
    for video in ("v02", "t"):
        fr = vb.frames(video)
        mat = vb.VIDEOS[video][3]
        ns = sorted(fr)
        for n in (ns[:2] if quick else ns[::3]):
            pl = tuple(np.asarray(p, F32) for p in fr[n])
            out.append((f"{video}:{n}", np.clip(nv.yuv_to_rgb(pl, mat, False), 0, 255).astype(F32)))
    return out


def synth_rgbs(rng):
    out = []
    for (h, w) in ((1, 1), (2, 3), (3, 5), (4, 4), (5, 7), (8, 8), (16, 17), (31, 33), (48, 64), (97, 131), (128, 128),
                   (180, 320), (257, 255)):
        base = rng.uniform(0, 255, (h, w, 3))
        out.append((f"noise{h}x{w}", base.astype(F32)))
        flat = np.full((h, w, 3), 120.0) + rng.normal(0, 2.0, (h, w, 3))
        out.append((f"flat{h}x{w}", np.clip(flat, 0, 255).astype(F32)))
        g = np.linspace(0, 255, w)[None, :, None] * np.ones((h, 1, 3)) + rng.normal(0, 1.5, (h, w, 3))
        g[: h // 2, :, 1] = 255.0                                     # clipped G in the upper half
        out.append((f"clip{h}x{w}", np.clip(g, 0, 255).astype(F32)))
        out.append((f"const{h}x{w}", np.full((h, w, 3), 37.0, F32)))
    return out


# ------------------------------------------------------------------------------------- display cases
def case_simple(label, rgb, bs):
    import nv_ui_qt.display as D
    L = lib()
    H, W = rgb.shape[:2]
    mn, mx = D.minmax(rgb)
    cmn, cmx = ctypes.c_double(), ctypes.c_double()
    L.nvc_minmax(ptr(rgb), rgb.size, ctypes.byref(cmn), ctypes.byref(cmx))
    ok, tot = int(dbl_same(mn, cmn.value) and dbl_same(mx, cmx.value)), 1
    bad = [] if ok else [f"{label} minmax"]
    for name_i, name in enumerate(SIMPLE):
        for b in bs:
            ref = D.render_simple(name, rgb, b, (mn, mx))
            out = np.empty((H, W, 3), np.uint8)
            L.nvc_render_simple(name_i, ptr(rgb), H, W, b, mn, mx, ptr(out))
            tot += 1
            if same(ref, out):
                ok += 1
            else:
                bad.append(f"{label} {name} b{b} ({int((ref != out).sum())} px)")
    return ok, tot, bad


def case_freq(label, rgb, bs, emphases=(True, False)):
    import nv_ui_qt.display as D
    L = lib()
    H, W = rgb.shape[:2]
    ok = tot = 0
    bad = []
    for ki, kind in enumerate(KINDS):
        for emph in (emphases if kind == "Y+" else (True,)):
            planes, levels = D.freq_images(rgb, kind, emph)
            out16 = np.empty((3, H, W), np.int16)
            lv = np.empty(6, F32)
            L.nvc_freq_images(ptr(rgb), H, W, ki, int(emph), ptr(out16), ptr(lv))
            tot += 1
            pl_ok = all(same(np.asarray(p, np.int16), out16[i]) for i, p in enumerate(planes))
            lv_ok = all(dbl_same(float(levels[i][j]), float(lv[2 * i + j])) for i in range(3) for j in range(2))
            if pl_ok and lv_ok:
                ok += 1
            else:
                bad.append(f"{label} freq {kind} emph={emph} planes={pl_ok} levels={lv_ok} "
                           f"{levels} vs {lv.tolist()}")
                continue
            names = (["Y+", "Cr+", "Cb+"] if kind == "Y+" else [f"{kind} High", f"{kind} Mid", f"{kind} Low"])
            for i, nm in enumerate(names):
                for b in bs:
                    ref = D.render_freq([nm], [planes[i]], [levels[i]], b)[0]
                    out = np.empty((H, W, 3), np.uint8)
                    p16 = np.ascontiguousarray(planes[i], np.int16)
                    L.nvc_render_freq(RK[D.pane_kind(nm)], ptr(p16), H, W, F32(levels[i][0]), F32(levels[i][1]), b,
                                      ptr(out))
                    tot += 1
                    if same(ref, out):
                        ok += 1
                    else:
                        bad.append(f"{label} render_freq {nm} b{b} ({int((ref != out).sum())} px)")
    return ok, tot, bad


def case_sel(label, rgb, rects):
    import nv_ui_qt.display as D
    L = lib()
    ok = tot = 0
    bad = []
    for (y, x, h, w) in rects:
        crop = np.ascontiguousarray(rgb[y:y + h, x:x + w], F32)
        ch, cw = crop.shape[:2]
        if ch == 0 or cw == 0:
            continue
        ref = D.noise_level(crop)
        got = L.nvc_noise_level(ptr(crop), ch, cw)
        tot += 1
        if dbl_same(ref, got):
            ok += 1
        else:
            bad.append(f"{label} noise_level {y},{x},{h},{w}: {ref!r} vs {got!r}")
        if min(ch, cw) < 2:
            continue                                   # band_levels of a 1-px plane: not a UI input (>= 16 px)
        rb, rc = D.not_uniform(crop)
        b3, c3 = np.zeros(3, np.int32), np.zeros(3, np.int32)
        L.nvc_not_uniform(ptr(crop), ch, cw, ptr(b3), ptr(c3))
        gb = [n for n, v in zip(("Y", "Cr", "Cb"), b3) if v]
        gc = [n for n, v in zip("RGB", c3) if v]
        tot += 1
        if gb == rb and gc == rc:
            ok += 1
        else:
            bad.append(f"{label} not_uniform {y},{x},{h},{w}: {rb, rc} vs {gb, gc}")
    return ok, tot, bad


def rects_for(rgb, rng, n):
    H, W = rgb.shape[:2]
    out = []
    sizes = [(16, 16), (16, 40), (31, 17), (32, 32), (48, 64), (64, 64), (100, 37), (127, 128), (128, 128),
             (200, 333), (256, 256), (512, 512), (512, 101)]
    for k in range(n):
        h, w = sizes[k % len(sizes)] if k < len(sizes) else (int(rng.integers(16, 513)), int(rng.integers(16, 513)))
        h, w = min(h, H), min(w, W)
        out.append((int(rng.integers(0, H - h + 1)), int(rng.integers(0, W - w + 1)), h, w))
    return out


# ------------------------------------------------------------------------------------- yuv_to_rgb
def case_yuv(seed):
    import nvfilter as nv
    L = lib()
    rng = np.random.default_rng(seed)
    ok = tot = 0
    bad = []
    for t in range(40):
        H, W = (int(v) for v in rng.integers(1, 48, 2))
        sub = t % 3
        if sub == 0:
            Hc, Wc = (H + 1) // 2, (W + 1) // 2
        elif sub == 1:
            Hc, Wc = H, W
        else:
            Hc, Wc = max(1, H // 2 + int(rng.integers(0, 2))), max(1, W // 2 + int(rng.integers(0, 2)))
        Y = rng.uniform(-20, 280, (H, W)).astype(F32)
        Cb = rng.uniform(-20, 280, (Hc, Wc)).astype(F32)
        Cr = rng.uniform(-20, 280, (Hc, Wc)).astype(F32)
        if t % 4 == 0:
            Y.flat[int(rng.integers(0, Y.size))] = np.nan
            Cb.flat[0] = np.inf
            Cr.flat[-1] = -np.inf
        for mat, mi in MATRIX.items():
            for fr in (False, True):
                ref = nv.yuv_to_rgb((Y, Cb, Cr), mat, fr)
                refs = [ref, np.clip(ref, 0, 255),
                        np.clip(np.nan_to_num(ref, nan=0.0, posinf=255.0, neginf=0.0), 0, 255)]
                for clip in range(3):
                    out = np.empty((H, W, 3), F32)
                    L.nvc_yuv_to_rgb(ptr(Y), ptr(Cb), ptr(Cr), H, W, Hc, Wc, mi, int(fr), clip, ptr(out))
                    tot += 1
                    if same(refs[clip].astype(F32), out):
                        ok += 1
                    else:
                        bad.append(f"yuv seed {seed} t{t} {mat} fr={fr} clip={clip}")
    return ok, tot, bad


# ------------------------------------------------------------------------------------- mix / floor
def case_mix(seed):
    """backend.finish's Mix: out = o + m * (s - o) (o float32 planes, s = np.asarray(window[ci], float32))."""
    L = lib()
    rng = np.random.default_rng(seed)
    ok = tot = 0
    bad = []
    for t in range(30):
        H, W = (int(v) for v in rng.integers(1, 40, 2))
        Hc, Wc = ((H + 1) // 2, (W + 1) // 2) if t % 2 else (H, W)
        o = [rng.uniform(-30, 290, sh).astype(F32) for sh in ((H, W), (Hc, Wc), (Hc, Wc))]
        src = [rng.integers(0, 256, sh).astype(np.uint8) for sh in ((H, W), (Hc, Wc), (Hc, Wc))]
        if t % 7 == 0:
            o[0].flat[0] = np.nan
        for m in (0.0, -0.5, 0.25, 0.75, 1.0, 1 / 3, float(rng.uniform(0, 1))):
            ref = [p for p in o] if not m > 0 else [p + m * (np.asarray(s, F32) - p) for p, s in zip(o, src)]
            got = [np.ascontiguousarray(p.copy()) for p in o]
            sf = [np.ascontiguousarray(np.asarray(s, F32)) for s in src]
            L.nvc_mix((P * 3)(*[ptr(g) for g in got]), (P * 3)(*[ptr(x) for x in sf]), H, W, Hc, Wc, m)
            tot += 1
            if all(same(np.asarray(r, F32), g) for r, g in zip(ref, got)):
                ok += 1
            else:
                bad.append(f"mix seed {seed} t{t} m={m}")
    return ok, tot, bad


def case_nlc(seed):
    """N3b: backend.temporal_mask / display.nlc_image / float(m.mean()) vs nvc_temporal_mask / nvc_nlc_image /
    nvc_mean_f32 on random residual variances (with 0, negative, NaN, inf) and the test frames' RGB."""
    import nv_ui_qt.backend as backend
    import nv_ui_qt.display as D
    L = lib()
    rng = np.random.default_rng(seed)
    ok = tot = 0
    bad = []
    rgbs = dict(video_rgbs(True)) if seed % 2 == 0 else {}
    for t in range(40):
        H, W = (int(v) for v in rng.integers(1, 50, 2))
        rv = rng.uniform(0.0, 1.5, (H, W)).astype(F32)
        if t % 5 == 1:
            rv.flat[rng.integers(0, H * W, 3)] = [0.0, np.nan, np.inf]
        if t % 5 == 2:
            rv -= F32(0.3)
        if t % 5 == 3:
            rv = (rv * F32(1e-3)).astype(F32)
        for n, a in ((5, 1.0), (1, 1.0), (5, 0.0), (3, 0.6), (2, float(rng.uniform(0.05, 1.0))), (5, -0.2)):
            ref = backend.temporal_mask(rv, n, a)
            got = np.zeros((H, W), F32)
            L.nvc_temporal_mask(ptr(rv), H, W, n, a, ptr(got))
            tot += 1
            if same(ref, got):
                ok += 1
            else:
                bad.append(f"temporal_mask seed {seed} t{t} n={n} a={a}")
            m_ref = float(ref.mean())
            m_got = L.nvc_mean_f32(ptr(got), got.size)
            tot += 1
            if np.float32(m_ref).tobytes() == np.float32(m_got).tobytes() and (m_ref != m_ref or float(np.float32(m_ref)) == m_ref):
                ok += 1
            else:
                bad.append(f"mask mean seed {seed} t{t}: {m_ref!r} vs {m_got!r}")
        rgb = rng.uniform(-10, 270, (H, W, 3)).astype(F32)
        if t % 4 == 0:
            rgb.flat[rng.integers(0, 3 * H * W, 2)] = [np.nan, np.inf]
        m = np.clip(rng.uniform(-0.1, 1.1, (H, W)), 0, 1).astype(F32)
        ref = D.nlc_image(rgb, m)
        got = np.zeros((H, W, 3), F32)
        L.nvc_nlc_image(ptr(rgb), ptr(m), H, W, ptr(got))
        tot += 1
        if same(ref, got):
            ok += 1
        else:
            bad.append(f"nlc_image seed {seed} t{t}")
    for label, rgb in list(rgbs.items())[:2]:
        rgb = np.ascontiguousarray(rgb)
        H, W = rgb.shape[:2]
        for k in range(3):
            m = rng.uniform(0, 1, (H, W)).astype(F32) if k else np.zeros((H, W), F32)
            ref = D.nlc_image(rgb, m)
            got = np.zeros((H, W, 3), F32)
            L.nvc_nlc_image(ptr(rgb), ptr(m), H, W, ptr(got))
            tot += 1
            if same(ref, got):
                ok += 1
            else:
                bad.append(f"nlc_image {label} k{k}")
    return ok, tot, bad


def case_interp(seed):
    """N3b: np.interp (state.auto_complete) vs nvc_interp: the 9 bins' subsets, random increasing xp, queries on
    / between / outside the knots, NaN."""
    L = lib()
    rng = np.random.default_rng(seed)
    ok = tot = 0
    bad = []
    CX = np.array([32.0 * k for k in range(9)])
    for t in range(300):
        if t % 3 == 0:
            n = int(rng.integers(1, 10))
            idx = np.sort(rng.choice(9, n, replace=False))
            xp = CX[idx]
            fp = rng.uniform(0.2, 3.0, n)
            xs = CX.copy()
        else:
            n = int(rng.integers(1, 12))
            xp = np.sort(rng.uniform(-1e3, 1e3, n))
            if len(np.unique(xp)) != n:
                continue
            fp = rng.uniform(-1e6, 1e6, n)
            if t % 7 == 0:
                fp[rng.integers(0, n)] = np.nan
            xs = np.concatenate([rng.uniform(-1.2e3, 1.2e3, 20), xp, xp + 1e-9, [np.nan, np.inf, -np.inf]])
        ref = np.array([float(np.interp(x, xp, fp)) for x in xs])
        got = np.zeros(len(xs))
        L.nvc_interp(ptr(np.ascontiguousarray(xp)), ptr(np.ascontiguousarray(fp)), n, ptr(np.ascontiguousarray(xs)),
                     len(xs), ptr(got))
        tot += len(xs)
        eq = ref.tobytes() == got.tobytes()
        if eq:
            ok += len(xs)
        else:
            d = int((ref.view(np.uint64) != got.view(np.uint64)).sum())
            ok += len(xs) - d
            bad.append(f"interp seed {seed} t{t}: {d} differ")
    return ok, tot, bad


def case_floor(_):
    """nv_profile.reference_floor() vs nvc_reference_floor (from data/ref_floor.npy and computed)."""
    import nv_profile as npr
    L = lib()
    ref = np.ascontiguousarray(npr.reference_floor(), F32)
    ok = tot = 0
    bad = []
    npy = os.path.join(ROOT, "data", "ref_floor.npy")
    for label, path in (("npy", npy.encode()), ("computed", None)):
        out = np.zeros(51, F32)
        r = L.nvc_reference_floor(path, 2, ptr(out))
        tot += 1
        if r == (1 if path else 2) and same(ref, out):
            ok += 1
        else:
            bad.append(f"reference floor {label}: return {r}, {int((ref.view(np.uint32) != out.view(np.uint32)).sum())} words differ")
    return ok, tot, bad


# ------------------------------------------------------------------------------------- quality
class PS:
    def __init__(self, base, factors, tune):
        self.base, self.factors, self.tune = base, factors, tune


def case_quality(video, n, variants):
    import copy
    import verify_bridge as vb
    import nvfilter as nv
    from nv_ui_qt import backend
    import nv_ui_qt.quality as Q
    L = lib()
    backend.set_native(False)
    fl, lo = vb.frame_list(video)
    mat = vb.VIDEOS[video][3]
    f = n - lo
    base = backend.build_profile(fl, f, False, mat, "auto")
    aft = backend.run_aft(base, fl[f], False, mat)
    rng = np.random.default_rng(n)
    rgb = np.clip(nv.yuv_to_rgb(tuple(np.asarray(p, F32) for p in fl[f]), mat, False), 0, 255).astype(F32)
    H, W = rgb.shape[:2]
    ok = tot = 0
    bad = []
    for v in range(variants):
        b = copy.deepcopy(aft if v % 3 else base)
        if v % 5 == 4 and "aft_bin_flags" in b:
            del b["aft_bin_flags"]                       # a profile saved before the flags
        if v % 7 == 6:
            b["aft_reset"] = True
        if v % 11 == 10:
            b["area"] = [int(b["area"][0]), int(b["area"][1]), 40, 97]
        factors = [[1.0] * 9 for _ in range(3)]
        tune = {}
        if v % 2:
            factors = [[float(x) for x in rng.choice([1.0, 0.2, 3.0, 0.5, 2.7, 3.0000001], 9)] for _ in range(3)]
        if v % 4 == 3:
            tune = {k: float(rng.uniform(0.2, 3.0)) for k in ("all", "Y", "Cb", "Cr") if rng.random() < 0.7}
        ps = PS(b, factors, tune)
        ref = Q.profile_numbers(ps, fl, False, mat)
        a = b["area"]
        area4 = np.array([a[0], a[1], a[2], a[3] if len(a) == 4 else a[2]], np.int32)
        fac = np.array([x for fs in factors for x in fs], np.float64)
        tn = np.array([float(tune.get(k, 1.0)) for k in ("all", "Y", "Cb", "Cr")], np.float64)
        st = b.get("aft_bin_flags")
        flags = None
        if st is not None:
            flags = np.array([bool(x) for key in ("marker", "clip", "gt3") for row in st.get(key, [[False] * 9] * 3)
                              for x in row], np.uint8)
        qp, cp = ctypes.c_int(), ctypes.c_int()
        r = L.nvc_quality_numbers(ptr(rgb), H, W, ptr(area4), ptr(fac), ptr(tn), int(bool(b.get("aft_applied"))),
                                  int(bool(b.get("aft_reset"))), None if flags is None else ptr(flags),
                                  ctypes.byref(qp), ctypes.byref(cp))
        got = (qp.value, cp.value) if r == 1 else (None, None)
        tot += 1
        if got == tuple(ref):
            ok += 1
        else:
            bad.append(f"quality {video}:{n} v{v}: {ref} vs {got}")
    return ok, tot, bad


# ------------------------------------------------------------------------------------- driver
def _run(job):
    kind, args = job
    t0 = time.time()
    if kind == "video":
        label, quick = args
        rgbs = dict(video_rgbs(quick))
        rgb = rgbs[label]
        rng = np.random.default_rng(zlib.crc32(label.encode()))
        crop_rects = rects_for(rgb, rng, 10 if quick else 24)
        ok, tot, bad = case_sel(label, rgb, crop_rects)
        bs = BRIGHT[::4] if quick else BRIGHT
        o, t, b = case_simple(label, rgb, bs)
        ok, tot, bad = ok + o, tot + t, bad + b
        o, t, b = case_freq(label, rgb, (50, 0, 100, 37, 73))
        ok, tot, bad = ok + o, tot + t, bad + b
        for (y, x, h, w) in crop_rects[:6]:
            c = np.ascontiguousarray(rgb[y:y + h, x:x + w])
            o, t, b = case_freq(f"{label}[{y},{x},{h},{w}]", c, (50, 21, 88))
            ok, tot, bad = ok + o, tot + t, bad + b
            o, t, b = case_simple(f"{label}[{y},{x},{h},{w}]", c, (50, 30, 90))
            ok, tot, bad = ok + o, tot + t, bad + b
    elif kind == "synth":
        rng = np.random.default_rng(args)
        ok = tot = 0
        bad = []
        for label, rgb in synth_rgbs(rng):
            for fn in (lambda: case_simple(label, rgb, (0, 25, 50, 51, 100)),
                       lambda: case_freq(label, rgb, (50, 10, 100)),
                       lambda: case_sel(label, rgb, [(0, 0, rgb.shape[0], rgb.shape[1])] +
                                        ([(1, 1, rgb.shape[0] - 2, rgb.shape[1] - 2)] if min(rgb.shape[:2]) > 4 else []))):
                o, t, b = fn()
                ok, tot, bad = ok + o, tot + t, bad + b
    elif kind == "yuv":
        ok, tot, bad = case_yuv(args)
    elif kind == "mix":
        ok, tot, bad = case_mix(args)
    elif kind == "floor":
        ok, tot, bad = case_floor(args)
    elif kind == "nlc":
        ok, tot, bad = case_nlc(args)
    elif kind == "interp":
        ok, tot, bad = case_interp(args)
    else:
        video, n, variants = args
        ok, tot, bad = case_quality(video, n, variants)
    return kind, str(args), ok, tot, bad, time.time() - t0


def jobs_for(quick):
    labels = [l for l, _ in video_rgbs(quick)]
    js = [("video", (l, quick)) for l in labels]
    js += [("synth", s) for s in ((1,) if quick else (1, 2))]
    js += [("yuv", s) for s in ((7,) if quick else (7, 8, 9))]
    js += [("mix", s) for s in ((3,) if quick else (3, 4))] + [("floor", 0)]
    js += [("nlc", s) for s in ((10,) if quick else (10, 11))] + [("interp", s) for s in ((5,) if quick else (5, 6))]
    js += [("quality", q) for q in ((("v02", 312, 12),) if quick else
                                    (("v02", 312, 30), ("v02", 309, 20), ("t", 300, 30), ("t", 305, 20)))]
    return js


def run(tmp=None, jobs=2, quick=False):
    """verify_bridge group 'ui': (ok, total)."""
    if tmp:
        os.environ["NV_BRIDGE_TMP"] = tmp
        import verify_bridge as vb
        vb.TMP = tmp
        os.makedirs(tmp, exist_ok=True)
    video_rgbs(True)                                   # fill the frame cache before the workers start
    js = jobs_for(quick)
    OK = TOT = 0
    with ProcessPoolExecutor(max_workers=max(1, jobs)) as ex:
        for kind, label, ok, tot, bad, dt in ex.map(_run, js):
            OK += ok
            TOT += tot
            print(f"  ui {kind:8s} {label:28s} {ok}/{tot}  {dt:.1f}s" + ("" if not bad else "  FAIL"), flush=True)
            for line in bad[:8]:
                print("     ", line)
    print(f"ui: {OK}/{TOT} bit-exact" + ("" if OK == TOT else "  MISMATCH"))
    return OK, TOT


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--tmp", default=None)
    a = ap.parse_args()
    ok, tot = run(a.tmp, a.jobs, a.quick)
    sys.exit(0 if ok == tot else 1)


if __name__ == "__main__":
    main()
