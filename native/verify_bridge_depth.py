# N3c: the pixel-format conversions of libnvcore.dylib (nvcore_c_depth.h) against the Python functions, BIT FOR BIT.
#
#   make -C native lib && python3 native/verify_bridge_depth.py [--jobs N] [--quick]
#   (also a group of native/verify_bridge.py: --only depth)
#
# Reference: repro/nv_depth.py (split, sanitize, to_internal, from_internal) and repro/nv_ui_qt/ofx_bridge.py
# (unpack_yuv420p, pack_yuv420p).  Every output word is compared: float32 planes as their u32 bits, raw frames byte
# for byte, the sanitize restore info (indices and the original values' bits).
# Inputs: every processing format of nv_depth.pixfmt (yuv420p legacy, yuv{420,422,444}p{10,12,16}le,
# gbrp{10,12,16}le, gbrpf32le) x sizes from 1 x 1 to 161 x 91 (odd, one row, one column) x matrices bt709 / bt601 /
# bt2020 / an unknown name (-> bt709) x value sets: uniform random, the boundary values 0 and 2^b - 1, for float
# random -0.5..1.5 with NaN (several payloads) / +-inf / -0 / subnormals / 1e30 / 3e38 and planes with no finite
# value; internal planes for from_internal with values out of range, NaN / +-inf and 1e30, with and without the
# restore info of a sanitized frame; the OFX frames of --depth 8 / 16 / 32 (unpack, pack, and the round trip).
import argparse
import ctypes
import os
import sys
import time
from concurrent.futures import ProcessPoolExecutor

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
REPRO = os.path.join(ROOT, "repro")
for p in (REPRO, HERE):
    if p not in sys.path:
        sys.path.insert(0, p)
sys.dont_write_bytecode = True
P = ctypes.c_void_p
MATRIX = {"bt709": 0, "bt601": 1, "bt2020": 2, "xyz": 0}
FORMATS = ["yuv420p"] + [f"yuv{c}p{b}le" for b in (10, 12, 16) for c in (420, 422, 444)] + \
          [f"gbrp{b}le" for b in (10, 12, 16)] + ["gbrpf32le"]
SIZES = [(1, 1), (1, 7), (7, 1), (2, 3), (3, 2), (5, 4), (33, 17), (64, 48), (161, 91)]
_LIB = None


def lib():
    global _LIB
    if _LIB is None:
        L = ctypes.CDLL(os.environ.get("NV_NATIVE_LIB", os.path.join(HERE, "libnvcore.dylib")))
        L.nvc_depth_sanitize.argtypes = [P, ctypes.c_long, P, P]
        L.nvc_depth_sanitize.restype = ctypes.c_long
        L.nvc_depth_to_internal.argtypes = [P] + [ctypes.c_int] * 7 + [P, P, P]
        L.nvc_depth_from_internal.argtypes = [P, P, P] + [ctypes.c_int] * 7 + [P, P, P, P]
        L.nvc_ofx_unpack.argtypes = [P, ctypes.c_int, ctypes.c_int, ctypes.c_int, P, P, P, P, P, P]
        L.nvc_ofx_pack.argtypes = [P, P, P, ctypes.c_int, ctypes.c_int, ctypes.c_int, P, P, P, P]
        _LIB = L
    return _LIB


def ptr(a):
    return a.ctypes.data_as(P)


def fcode(fmt):
    return (0 if fmt.family == "yuv" else 1, fmt.bits, int(fmt.is_float), int(fmt.chroma))


def internal_shapes(fmt, W, H):
    out = []
    for k, (h, w) in enumerate(fmt.plane_shapes(W, H)):
        out.append((h, W) if k > 0 and fmt.chroma == "422" else (h, w))
    return out


def words(a):
    a = np.asarray(a)
    return a.size if a.dtype != np.uint8 else a.size


def cmp_arr(label, a, b, bad):
    """(equal words, words) of two arrays compared as their bytes / bits."""
    a, b = np.ascontiguousarray(a), np.ascontiguousarray(b)
    if a.dtype != b.dtype or a.shape != b.shape:
        bad.append(f"{label}: dtype/shape {a.dtype}{a.shape} vs {b.dtype}{b.shape}")
        return 0, max(a.size, b.size, 1)
    if a.dtype == np.float32:
        x, y = a.view(np.uint32), b.view(np.uint32)
    elif a.dtype == np.float64:
        x, y = a.view(np.uint64), b.view(np.uint64)
    else:
        x, y = a, b
    eq = int(np.count_nonzero(x == y))
    if eq != a.size:
        i = int(np.flatnonzero(x != y)[0])
        bad.append(f"{label}: {a.size - eq}/{a.size} differ, first at {i}: {a.ravel()[i]!r} vs {b.ravel()[i]!r}")
    return eq, a.size


# ------------------------------------------------------------------ inputs
def special_floats(rng, n):
    v = rng.uniform(-0.5, 1.5, n).astype(np.float32)
    if n == 0:
        return v
    sp = np.array([np.nan, -np.nan, np.inf, -np.inf, -0.0, 0.0, 1e-40, -1e-42, 1e30, -1e30, 3e38, -3e38,
                   np.float32(1.0), np.float32(0.0)], np.float32)
    sp = np.concatenate([sp, np.array([0x7fc12345, 0xffd00001, 0x7f800001, 0x7fbfffff], np.uint32).view(np.float32)])
    k = max(1, n // 7)
    pos = rng.integers(0, n, k)
    v[pos] = sp[rng.integers(0, sp.size, k)]
    return v


def raw_frame(fmt, W, H, rng, kind):
    """A raw frame of fmt (bytes) for the value set `kind`."""
    planes = []
    for h, w in fmt.plane_shapes(W, H):
        n = h * w
        if fmt.is_float:
            if kind == "rand":
                v = rng.uniform(-0.25, 1.25, n).astype(np.float32)
            elif kind == "special":
                v = special_floats(rng, n)
            elif kind == "allbad":
                v = np.array([np.nan, np.inf, -np.inf], np.float32)[rng.integers(0, 3, n)]
            else:   # "edges": 0, 1, -0, the largest finite
                v = np.array([0.0, 1.0, -0.0, 3.4028235e38, -3.4028235e38], np.float32)[rng.integers(0, 5, n)]
            planes.append(v.astype("<f4"))
        else:
            top = 2 ** fmt.bits - 1
            if kind == "rand":
                v = rng.integers(0, top + 1, n)
            elif kind == "edges":
                v = np.array([0, top, 1, top - 1, top // 2], np.int64)[rng.integers(0, 5, n)]
            elif kind == "ramp":
                v = (np.arange(n) * 37) % (top + 1)
            else:
                v = rng.integers(0, top + 1, n)
            planes.append(v.astype(fmt.dtype))
    return b"".join(p.tobytes() for p in planes)


def internal_planes(fmt, W, H, rng, kind):
    out = []
    for h, w in internal_shapes(fmt, W, H):
        n = h * w
        if kind == "rand":
            v = rng.uniform(-3.0, 258.0, n).astype(np.float32)
        elif kind == "special":
            v = rng.uniform(0.0, 255.0, n).astype(np.float32)
            sp = np.array([np.nan, -np.nan, np.inf, -np.inf, -0.0, 1e30, -1e30, 255.75, 254.5, 0.5, 1.5, -0.5,
                           1e-40, 3e38], np.float32)
            k = max(1, n // 5)
            v[rng.integers(0, n, k)] = sp[rng.integers(0, sp.size, k)]
        else:   # "half": halves (the rint ties)
            v = (rng.integers(-4, 520, n) / 2.0).astype(np.float32)
        out.append(v.reshape(h, w))
    return out


# ------------------------------------------------------------------ cases
def case_formats(seed, quick):
    """to_internal (after split + sanitize) and from_internal for every format / size / matrix / value set."""
    import nv_depth as nd
    L = lib()
    rng = np.random.default_rng(seed)
    ok = tot = 0
    bad = []
    sizes = SIZES[::2] if quick else SIZES
    for name in FORMATS:
        fmt = nd.pixfmt(name)
        fc = fcode(fmt)
        kinds = ("rand", "special", "allbad", "edges") if fmt.is_float else ("rand", "edges", "ramp")
        for (W, H) in sizes:
            for matrix in (("bt709", "bt601", "bt2020", "xyz") if fmt.family == "rgb" else ("bt709", "bt601")):
                for kind in kinds:
                    lab = f"{name} {W}x{H} {matrix} {kind}"
                    buf = raw_frame(fmt, W, H, rng, kind)
                    # Python: denoise_video_v5.read_frames (split, sanitize for float, to_internal)
                    raw = nd.split(buf, fmt, W, H)
                    info = None
                    if fmt.is_float:
                        raw, info = nd.sanitize(raw)
                    exp = [np.asarray(p) for p in nd.to_internal(raw, fmt, matrix)]
                    # C: sanitize each raw float plane in place, then to_internal
                    src = np.frombuffer(buf, np.uint8).copy()
                    shapes = fmt.plane_shapes(W, H)
                    cinfo = {}
                    if fmt.is_float:
                        a = src.view(np.float32)
                        o = 0
                        for k, (h, w) in enumerate(shapes):
                            n = h * w
                            pl = a[o:o + n]                     # a view: sanitized in place
                            idx = np.zeros(n, np.int64)
                            vals = np.zeros(n, np.float32)
                            c = L.nvc_depth_sanitize(ptr(pl), n, ptr(idx), ptr(vals))
                            if c:
                                cinfo[k] = (idx[:c].copy(), vals[:c].copy())
                            o += n
                        # restore info
                        pi = info or {}
                        if sorted(pi) != sorted(cinfo):
                            bad.append(f"{lab}: restore planes {sorted(pi)} vs {sorted(cinfo)}")
                            tot += 1
                        else:
                            for k in pi:
                                e, t = cmp_arr(lab + f" ridx{k}", np.asarray(pi[k][0], np.int64), cinfo[k][0], bad)
                                ok, tot = ok + e, tot + t
                                e, t = cmp_arr(lab + f" rvals{k}", np.asarray(pi[k][1], np.float32), cinfo[k][1], bad)
                                ok, tot = ok + e, tot + t
                    ish = internal_shapes(fmt, W, H)
                    got = [np.zeros(s, np.float32) for s in ish]
                    r = L.nvc_depth_to_internal(ptr(src), *fc, W, H, MATRIX[matrix], ptr(got[0]), ptr(got[1]), ptr(got[2]))
                    if r != 0:
                        bad.append(f"{lab}: to_internal returned {r}")
                        tot += 1
                        continue
                    for k in range(3):
                        e, t = cmp_arr(f"{lab} to_internal p{k}", np.asarray(exp[k], np.float32) if not fmt.legacy
                                       else np.asarray(exp[k], np.float32), got[k], bad)
                        ok, tot = ok + e, tot + t
                    # from_internal of the internal planes (the round trip) and of other internal values,
                    # float formats with the restore info of this frame
                    for ikind in ("trip", "rand", "special", "half"):
                        planes = [np.asarray(p, np.float32) for p in exp] if ikind == "trip" else \
                            internal_planes(fmt, W, H, rng, ikind)
                        for use_restore in ((False, True) if fmt.is_float else (False,)):
                            rst = (info if use_restore else None)
                            e_bytes = nd.from_internal(tuple(planes) if not fmt.legacy else
                                                       tuple(planes), fmt, matrix, rst)
                            out = np.zeros(fmt.frame_bytes(W, H), np.uint8)
                            keep = []
                            if rst:
                                ridx = (P * 3)()
                                rvals = (P * 3)()
                                rn = np.zeros(3, np.int64)
                                for k, (idx, vals) in rst.items():
                                    ia, va = np.ascontiguousarray(idx, np.int64), np.ascontiguousarray(vals, np.float32)
                                    keep += [ia, va]
                                    ridx[k], rvals[k], rn[k] = ptr(ia), ptr(va), len(ia)
                                args = (ctypes.cast(ridx, P), ctypes.cast(rvals, P), ptr(rn))
                            else:
                                args = (None, None, None)
                            cp = [np.ascontiguousarray(p, np.float32) for p in planes]
                            r = L.nvc_depth_from_internal(ptr(cp[0]), ptr(cp[1]), ptr(cp[2]), *fc, W, H, MATRIX[matrix],
                                                          *args, ptr(out))
                            if r != 0:
                                bad.append(f"{lab}: from_internal returned {r}")
                                tot += 1
                                continue
                            e, t = cmp_arr(f"{lab} from_internal {ikind}{' restore' if rst else ''}",
                                           np.frombuffer(e_bytes, np.uint8), out, bad)
                            ok, tot = ok + e, tot + t
                            if ikind == "trip" and not fmt.is_float and e_bytes != buf:
                                bad.append(f"{lab}: the Python round trip itself is not exact")
    return ok, tot, bad


def case_ofx(seed, quick):
    """ofx_bridge.unpack_yuv420p (16 / 32) and pack_yuv420p (8 / 16 / 32), restore and the round trip."""
    from nv_ui_qt import ofx_bridge as ob
    L = lib()
    rng = np.random.default_rng(seed)
    ok = tot = 0
    bad = []
    sizes = [(1, 1), (2, 2), (3, 3), (5, 4), (64, 36), (161, 91)] if not quick else [(2, 2), (5, 4), (64, 36)]
    for (W, H) in sizes:
        n = [W * H, (W // 2) * (H // 2), (W // 2) * (H // 2)]
        for depth in (8, 16, 32):
            for kind in ("rand", "special", "edges"):
                lab = f"ofx {W}x{H} d{depth} {kind}"
                tot_n = sum(n)
                if depth == 8:
                    data = rng.integers(0, 256, tot_n).astype(np.uint8).tobytes()
                elif depth == 16:
                    v = rng.integers(0, 65536, tot_n) if kind != "edges" else \
                        np.array([0, 65535, 256, 65280, 128], np.int64)[rng.integers(0, 5, tot_n)]
                    data = v.astype("<u2").tobytes()
                else:
                    v = rng.uniform(-0.1, 1.1, tot_n).astype(np.float32) if kind == "rand" else \
                        special_floats(rng, tot_n) if kind == "special" else \
                        np.array([16 / 255, 235 / 255, 128 / 255, 0.0, 1.0], np.float32)[rng.integers(0, 5, tot_n)]
                    data = v.astype("<f4").tobytes()
                restore = []
                exp = ob.unpack_yuv420p(data, W, H, depth, restore)
                rst = restore[0] if restore else None
                if depth == 8:
                    planes = [np.asarray(p, np.float32) for p in exp]
                else:
                    got = [np.zeros(k, np.float32) for k in n]
                    ridx = (P * 3)()
                    rvals = (P * 3)()
                    rn = np.zeros(3, np.int64)
                    bufs = [(np.zeros(k, np.int64), np.zeros(k, np.float32)) for k in n]
                    for k in range(3):
                        ridx[k], rvals[k] = ptr(bufs[k][0]), ptr(bufs[k][1])
                    src = np.frombuffer(data, np.uint8)
                    r = L.nvc_ofx_unpack(ptr(src), W, H, depth, ptr(got[0]), ptr(got[1]), ptr(got[2]),
                                         ctypes.cast(ridx, P), ctypes.cast(rvals, P), ptr(rn))
                    if r != 0:
                        bad.append(f"{lab}: unpack returned {r}")
                        tot += 1
                        continue
                    for k in range(3):
                        e, t = cmp_arr(f"{lab} unpack p{k}", np.asarray(exp[k], np.float32).ravel(), got[k], bad)
                        ok, tot = ok + e, tot + t
                    if depth == 32:
                        cr = {k: (bufs[k][0][:rn[k]], bufs[k][1][:rn[k]]) for k in range(3) if rn[k]}
                        if sorted(cr) != sorted(rst or {}):
                            bad.append(f"{lab}: restore planes {sorted(rst or {})} vs {sorted(cr)}")
                            tot += 1
                        else:
                            for k in cr:
                                e, t = cmp_arr(f"{lab} ridx{k}", np.asarray(rst[k][0], np.int64), cr[k][0], bad)
                                ok, tot = ok + e, tot + t
                                e, t = cmp_arr(f"{lab} rvals{k}", np.asarray(rst[k][1], np.float32), cr[k][1], bad)
                                ok, tot = ok + e, tot + t
                    planes = [np.asarray(p, np.float32) for p in exp]
                # pack: the unpacked planes (round trip), random processed planes, specials; with the restore info
                for pkind in ("trip", "rand", "special"):
                    if pkind == "trip":
                        pl = planes
                    else:
                        pl = []
                        for k in range(3):
                            v = rng.uniform(-5, 262, n[k]).astype(np.float32)
                            if pkind == "special" and n[k]:
                                sp = np.array([np.nan, -np.nan, np.inf, -np.inf, -0.0, 1e30, 255.5, 254.5, 0.5, -0.5,
                                               127.998046875], np.float32)
                                m = max(1, n[k] // 4)
                                v[rng.integers(0, n[k], m)] = sp[rng.integers(0, sp.size, m)]
                            pl.append(v.reshape(planes[k].shape))
                    e_bytes = ob.pack_yuv420p(pl, depth, rst)
                    out = np.zeros(ob.frame_size(W, H, depth), np.uint8)
                    cp = [np.ascontiguousarray(p, np.float32) for p in pl]
                    keep = []
                    if rst:
                        ridx = (P * 3)()
                        rvals = (P * 3)()
                        rn = np.zeros(3, np.int64)
                        for k, (idx, vals) in rst.items():
                            ia, va = np.ascontiguousarray(idx, np.int64), np.ascontiguousarray(vals, np.float32)
                            keep += [ia, va]
                            ridx[k], rvals[k], rn[k] = ptr(ia), ptr(va), len(ia)
                        args = (ctypes.cast(ridx, P), ctypes.cast(rvals, P), ptr(rn))
                    else:
                        args = (None, None, None)
                    r = L.nvc_ofx_pack(ptr(cp[0]), ptr(cp[1]), ptr(cp[2]), W, H, depth, *args, ptr(out))
                    if r != 0:
                        bad.append(f"{lab}: pack returned {r}")
                        tot += 1
                        continue
                    e, t = cmp_arr(f"{lab} pack {pkind}", np.frombuffer(e_bytes, np.uint8), out, bad)
                    ok, tot = ok + e, tot + t
                    if pkind == "trip" and depth != 32 and e_bytes != data:
                        bad.append(f"{lab}: the Python round trip itself is not exact")
    return ok, tot, bad


def case_bad_format(_seed, _quick):
    """Formats nv_depth.pixfmt refuses give -1 (8-bit 4:2:2, yuv float, rgb 4:2:0, OFX depth 12)."""
    L = lib()
    ok = tot = 0
    bad = []
    z = np.zeros(64, np.float32)
    raw = np.zeros(1024, np.uint8)
    for fc in ((0, 8, 0, 422), (0, 32, 1, 420), (1, 10, 0, 420), (0, 14, 0, 420), (2, 10, 0, 444)):
        tot += 2
        r1 = L.nvc_depth_to_internal(ptr(raw), *fc, 4, 4, 0, ptr(z), ptr(z), ptr(z))
        r2 = L.nvc_depth_from_internal(ptr(z), ptr(z), ptr(z), *fc, 4, 4, 0, None, None, None, ptr(raw))
        ok += (r1 == -1) + (r2 == -1)
        if r1 != -1 or r2 != -1:
            bad.append(f"format {fc}: {r1}, {r2} (expected -1)")
    for d in (8, 12):
        tot += 1
        r = L.nvc_ofx_unpack(ptr(raw), 4, 4, d, ptr(z), ptr(z), ptr(z), None, None, None)
        ok += r == -1
    tot += 1
    ok += L.nvc_ofx_pack(ptr(z), ptr(z), ptr(z), 4, 4, 12, None, None, None, ptr(raw)) == -1
    return ok, tot, bad


CASES = {"formats": case_formats, "ofx": case_ofx, "badfmt": case_bad_format}


def _run(job):
    import warnings
    warnings.simplefilter("ignore", RuntimeWarning)      # the NaN / inf inputs (numpy's cast warnings)
    kind, seed, quick = job
    t0 = time.perf_counter()
    ok, tot, bad = CASES[kind](seed, quick)
    return kind, f"seed {seed}", ok, tot, bad, time.perf_counter() - t0


def jobs_for(quick):
    seeds = (1,) if quick else (1, 2, 3)
    return [("formats", s, quick) for s in seeds] + [("ofx", s + 10, quick) for s in seeds] + [("badfmt", 0, quick)]


def run(tmp=None, jobs=2, quick=False):
    """verify_bridge group 'depth': (ok, total) words."""
    OK = TOT = 0
    with ProcessPoolExecutor(max_workers=max(1, jobs)) as ex:
        for kind, label, ok, tot, bad, dt in ex.map(_run, jobs_for(quick)):
            OK += ok
            TOT += tot
            print(f"  depth {kind:8s} {label:10s} {ok}/{tot}  {dt:.1f}s" + ("" if not bad else "  FAIL"), flush=True)
            for line in bad[:8]:
                print("     ", line)
    print(f"depth: {OK}/{TOT} bit-exact" + ("" if OK == TOT else "  MISMATCH"))
    return OK, TOT


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--tmp", default=None)
    A = ap.parse_args()
    ok, tot = run(A.tmp, A.jobs, A.quick)
    return 0 if ok == tot else 1


if __name__ == "__main__":
    sys.exit(main())
