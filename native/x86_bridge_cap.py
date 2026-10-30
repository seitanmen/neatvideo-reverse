# G9: records the libnvcore.dylib calls (the nvc_* functions of nvcore_c*.h) of an arm64 Python process, so
# native/x86_bridge_replay.py can run the same calls on another build of the library (libnvcore_x86.dylib under
# Rosetta 2, libnvcore_p.dylib) and compare every buffer after the call byte for byte.
#
# Loaded by native/x86cap/sitecustomize.py when $NV_X86_CAP names the capture directory (native/verify_x86_bridge.sh
# puts native/x86cap on PYTHONPATH, so the spawned worker processes of verify_bridge.py record as well).  It wraps
# ctypes.CDLL: the functions of a library named libnvcore*.dylib are proxies that, around the real call,
#   - snapshot every buffer the call may read or write (sizes from the C ABI below, SPEC) before the call,
#   - wrap a cancel callback so the answers it gives are kept (the replay answers the same sequence),
#   - hash every buffer after the call (and keep it when small) and keep the return value.
# The calls themselves are unchanged (same arguments, same library), so verify_bridge.py checks as usual.
#
# Output: $NV_X86_CAP/cap_<pid>.pkl (a stream of pickled dicts, plain Python types only, so the replay needs no
# numpy) and $NV_X86_CAP/cas/<sha256> for the buffers larger than INLINE bytes (content addressed: the frames
# that every window holds are stored once).
import ctypes
import hashlib
import os
import pickle
import struct
import sys
import threading
import time

CAP = os.environ.get("NV_X86_CAP")
INLINE = 1 << 20          # before-call buffers up to this size go into the record, larger ones into cas/
KEEP_AFTER = 1 << 12      # after-call buffers up to this size are kept (else only the hash)

F32, F64, I32, I64, U8, I16 = 4, 8, 4, 8, 1, 2


def _rd(addr, n):
    return ctypes.string_at(addr, n) if addr and n > 0 else b""


def _i32(addr, k):
    return struct.unpack("<i", _rd(addr + 4 * k, 4))[0]


def _i64(addr, k):
    return struct.unpack("<q", _rd(addr + 8 * k, 8))[0]


def _ptrs_at(addr, n):
    return list(struct.unpack(f"<{n}Q", _rd(addr, 8 * n))) if addr and n > 0 else [0] * max(n, 0)


def B(nbytes, esize, role="in"):
    """A buffer argument: nbytes bytes of esize-byte words, role in / out / io."""
    return ("buf", max(int(nbytes), 0), esize, role)


def A(sizes, esize, role="in"):
    """A pointer-array argument: entry k points to sizes[k] bytes."""
    return ("arr", [max(int(s), 0) for s in sizes], esize, role)


def _yuv_sizes(H, W, Hc, Wc, n):
    return [(H * W if k % 3 == 0 else Hc * Wc) * F32 for k in range(n)]


def _depth_valid(fam, bits, isf, chroma):
    if fam == 0 and bits == 8 and not isf:
        return chroma == 420
    if fam == 0 and bits in (10, 12, 16) and not isf:
        return chroma in (420, 422, 444)
    if fam == 1 and bits in (10, 12, 16) and not isf:
        return chroma == 444
    return fam == 1 and bits == 32 and isf and chroma == 444


def _depth_raw(fam, bits, isf, chroma, W, H):
    """(raw frame bytes, [raw plane samples], internal chroma samples) of a format nv_depth.pixfmt accepts."""
    c = ((H + 1) // 2) * ((W + 1) // 2) if chroma == 420 else H * ((W + 1) // 2) if chroma == 422 else H * W
    bps = 1 if bits == 8 else 4 if bits == 32 else 2
    ic = c if chroma == 420 else H * W
    return (H * W + 2 * c) * bps, [H * W, c, c], ic


def _restore(a, iidx, ivals, irn):
    """ridx / rvals pointer arrays of 3 with rn[k] entries each (NULL = none)."""
    rn = [_i64(a[irn], k) for k in range(3)] if a[irn] else [0, 0, 0]
    out = {irn: B(24, I64)}
    if a[iidx]:
        out[iidx] = A([n * I64 for n in rn], I64)
    if a[ivals]:
        out[ivals] = A([n * F32 for n in rn], F32)
    return out


def _sp_temporal(a):
    nf, ci, H, W, Hc, Wc = a[:6]
    nin = a[11]
    bs = _i32(a[9], 1) if a[9] else 0
    nb = (H // bs) * (W // bs) * F32 if bs >= 1 else 0
    return {6: A(_yuv_sizes(H, W, Hc, Wc, 3 * nf), F32), 7: B(16, I32), 8: B(82 * 8, F64), 9: B(16, I32),
            10: B(21 * 8, F64), 12: B(nin * 4, I32), 13: A([nb] * (2 * nin), F32), 14: A([nb] * (2 * nf), F32, "out"),
            15: B(nf, U8, "out"), 16: A(_yuv_sizes(H, W, Hc, Wc, 3), F32, "out"),
            17: A(_yuv_sizes(H, W, Hc, Wc, 3), F32, "out")}


def _sp_spatial(a):
    H, W, Hc, Wc = a[:4]
    s = _yuv_sizes(H, W, Hc, Wc, 3)
    return {4: A(s, F32), 5: A(s, F32), 6: B(16, I32), 7: B(82 * 8, F64), 8: B(16, I32), 9: B(21 * 8, F64),
            11: A(s, F32, "out")}


def _sp_to_internal(a):
    fam, bits, isf, chroma, W, H = a[1:7]
    if not _depth_valid(fam, bits, isf, chroma) or W < 1 or H < 1:
        return {0: B(0, U8), 8: B(0, F32, "out"), 9: B(0, F32, "out"), 10: B(0, F32, "out")}
    nb, _, ic = _depth_raw(fam, bits, isf, chroma, W, H)
    return {0: B(nb, U8), 8: B(H * W * F32, F32, "out"), 9: B(ic * F32, F32, "out"), 10: B(ic * F32, F32, "out")}


def _sp_from_internal(a):
    fam, bits, isf, chroma, W, H = a[3:9]
    if not _depth_valid(fam, bits, isf, chroma) or W < 1 or H < 1:
        return {0: B(0, F32), 1: B(0, F32), 2: B(0, F32), 13: B(0, U8, "out")}
    nb, _, ic = _depth_raw(fam, bits, isf, chroma, W, H)
    out = {0: B(H * W * F32, F32), 1: B(ic * F32, F32), 2: B(ic * F32, F32), 13: B(nb, U8, "out")}
    if a[12]:
        out.update(_restore(a, 10, 11, 12))
    return out


def _ofx_bytes(W, H, depth):
    return (W * H + 2 * (W // 2) * (H // 2)) * {8: 1, 16: 2, 32: 4}.get(depth, 0)


def _sp_ofx_unpack(a):
    W, H, depth = a[1:4]
    if depth not in (16, 32) or W < 1 or H < 1:
        return {0: B(0, U8), 4: B(0, F32, "out"), 5: B(0, F32, "out"), 6: B(0, F32, "out")}
    n = [W * H, (W // 2) * (H // 2), (W // 2) * (H // 2)]
    out = {0: B(_ofx_bytes(W, H, depth), U8), 4: B(n[0] * F32, F32, "out"), 5: B(n[1] * F32, F32, "out"),
           6: B(n[2] * F32, F32, "out")}
    if a[7]:
        out[7] = A([k * I64 for k in n], I64, "out")
    if a[8]:
        out[8] = A([k * F32 for k in n], F32, "out")
    if a[9]:
        out[9] = B(24, I64, "out")
    return out


def _sp_ofx_pack(a):
    W, H, depth = a[3:6]
    if depth not in (8, 16, 32) or W < 1 or H < 1:
        return {0: B(0, F32), 1: B(0, F32), 2: B(0, F32), 9: B(0, U8, "out")}
    c = (W // 2) * (H // 2)
    out = {0: B(W * H * F32, F32), 1: B(c * F32, F32), 2: B(c * F32, F32), 9: B(_ofx_bytes(W, H, depth), U8, "out")}
    if a[8]:
        out.update(_restore(a, 6, 7, 8))
    return out


def _sp_build_v5(a):
    H, W, Hc, Wc = a[1:5]
    s = _yuv_sizes(H, W, Hc, Wc, 3)
    return {0: A(s, F32), 5: A(s, F32), 7: A(_yuv_sizes(H, W, Hc, Wc, 3 * a[6]), F32), 13: B(max(a[12], 0) * 4, I32),
            16: B(16 * 4, I32, "out"), 17: B(72 * 8, F64, "out")}


def _rgb(H, W):
    return H * W * 3 * F32


# fn -> spec(args) = {argument index: B(..) | A(..)}; the other arguments are scalars, cancel callbacks or strings
SPEC = {
    "nvc_abi_version": lambda a: {},
    "nvc_noise_curve": lambda a: {0: B(_rgb(a[2], a[1]), F32), 5: B(64, F64, "out"), 6: B(64, F64, "out")},
    "nvc_flat_area": lambda a: {0: B(_rgb(a[2], a[1]), F32), 6: B(20, I32, "out")},
    "nvc_profile_build": lambda a: {0: B(_rgb(a[2], a[1]), F32), 8: B(51 * F32, F32), 11: B(0xc300, U8, "out")},
    "nvc_aft": lambda a: {0: B(_rgb(a[2], a[1]), F32), 3: B(0xc300, U8, "io"), 7: B(12, F32, "out")},
    "nvc_aft_combine": lambda a: {0: B(0xc300, U8), 1: B(0xc300, U8), 2: B(a[3] * F64, F64), 4: B(3 * a[3] * F32, F32),
                                  6: B(24, F64), 8: B(72, F64, "out"), 9: B(3 * a[3] * F64, F64, "out"),
                                  10: B(3 * a[3] * F32, F32, "out"), 11: B(24, F64, "out"), 12: B(81, U8, "out")},
    "nvc_temporal": _sp_temporal,
    "nvc_spatial": _sp_spatial,
    "nvc_depth_sanitize": lambda a: {0: B(a[1] * F32, F32, "io"), 2: B(a[1] * I64, I64, "out"),
                                     3: B(a[1] * F32, F32, "out")},
    "nvc_depth_to_internal": _sp_to_internal,
    "nvc_depth_from_internal": _sp_from_internal,
    "nvc_ofx_unpack": _sp_ofx_unpack,
    "nvc_ofx_pack": _sp_ofx_pack,
    "nvc_build_profile_v5": _sp_build_v5,
    "nvc_wn_band_std": lambda a: {1: B(min(max(a[0], 0), 8) * F64, F64, "out")},
    "nvc_hh1_sigma": lambda a: {0: B(a[1] * a[2] * F32, F32), 3: B(8, F64, "out")},
    "nvc_noisiest_candidates": lambda a: {1: B(32 * I32, I32, "out")},
    "nvc_effective_curve": lambda a: {0: B(a[1] * F64, F64), 2: B(3 * a[1] * F64, F64), 3: B(27 * F64, F64),
                                      4: B(4 * F64, F64), 6: B(3 * F64, F64), 7: B(3 * a[1] * F64, F64, "out"),
                                      8: B(3 * F64, F64, "out")},
    "nvc_yuv_to_rgb": lambda a: {0: B(a[3] * a[4] * F32, F32), 1: B(a[5] * a[6] * F32, F32),
                                 2: B(a[5] * a[6] * F32, F32), 10: B(_rgb(a[3], a[4]), F32, "out")},
    "nvc_minmax": lambda a: {0: B(a[1] * F32, F32), 2: B(8, F64, "out"), 3: B(8, F64, "out")},
    "nvc_render_simple": lambda a: {1: B(_rgb(a[2], a[3]), F32), 7: B(a[2] * a[3] * 3, U8, "out")},
    "nvc_freq_images": lambda a: {0: B(_rgb(a[1], a[2]), F32), 5: B(3 * a[1] * a[2] * I16, I16, "out"),
                                  6: B(6 * F32, F32, "out")},
    "nvc_render_freq": lambda a: {1: B(a[2] * a[3] * I16, I16), 7: B(a[2] * a[3] * 3, U8, "out")},
    "nvc_noise_level": lambda a: {0: B(_rgb(a[1], a[2]), F32)},
    "nvc_not_uniform": lambda a: {0: B(_rgb(a[1], a[2]), F32), 3: B(12, I32, "out"), 4: B(12, I32, "out")},
    "nvc_quality_numbers": lambda a: {0: B(_rgb(a[1], a[2]), F32), 3: B(16, I32), 4: B(27 * F64, F64),
                                      5: B(4 * F64, F64), 8: B(81, U8), 9: B(4, I32, "out"), 10: B(4, I32, "out")},
    "nvc_mix": lambda a: {0: A(_yuv_sizes(a[2], a[3], a[4], a[5], 3), F32, "io"),
                          1: A(_yuv_sizes(a[2], a[3], a[4], a[5], 3), F32)},
    "nvc_reference_floor": lambda a: {2: B(51 * F32, F32, "out")},
    "nvc_temporal_mask": lambda a: {0: B(a[1] * a[2] * F32, F32), 5: B(a[1] * a[2] * F32, F32, "out")},
    "nvc_nlc_image": lambda a: {0: B(_rgb(a[2], a[3]), F32), 1: B(a[2] * a[3] * F32, F32),
                                4: B(_rgb(a[2], a[3]), F32, "out")},
    "nvc_interp": lambda a: {0: B(a[2] * F64, F64), 1: B(a[2] * F64, F64), 3: B(a[4] * F64, F64),
                             5: B(a[4] * F64, F64, "out")},
    "nvc_mean_f32": lambda a: {0: B(a[1] * F32, F32)},
}

_lock = threading.Lock()
_state = {"f": None, "seq": 0}


def _out():
    if _state["f"] is None:
        os.makedirs(os.path.join(CAP, "cas"), exist_ok=True)
        _state["f"] = open(os.path.join(CAP, f"cap_{os.getpid()}.pkl"), "ab")
    return _state["f"]


def _blob(b):
    if len(b) <= INLINE:
        return ("i", b)
    h = hashlib.sha256(b).hexdigest()
    p = os.path.join(CAP, "cas", h)
    if not os.path.exists(p):
        t = f"{p}.{os.getpid()}.{threading.get_ident()}.tmp"
        with open(t, "wb") as f:
            f.write(b)
        os.replace(t, p)
    return ("h", h, len(b))


def _addr(x):
    if x is None:
        return 0
    if isinstance(x, int):
        return x
    if isinstance(x, ctypes.c_void_p):
        return x.value or 0
    if isinstance(x, ctypes.Array):
        return ctypes.addressof(x)
    if type(x).__name__ == "CArgObject":        # ctypes.byref(obj)
        return ctypes.addressof(x._obj)
    if hasattr(x, "value") and isinstance(x.value, int):
        return x.value
    raise TypeError(f"x86_bridge_cap: pointer argument of type {type(x).__name__}")


def _tname(t):
    if t is None:
        return None
    if isinstance(t, type) and issubclass(t, ctypes._CFuncPtr):
        return "CFUNC_int"
    return t.__name__


def _export_process():
    """True in the processes of an export (verify_bridge.py --export runs `python -m nv_ui_qt.export`, whose
    frame workers run backend._work in spawned processes: their __mp_main__ is that same module; a direct
    denoise_video_v5.py run likewise)."""
    for name in ("__main__", "__mp_main__"):
        f = getattr(sys.modules.get(name), "__file__", None) or ""
        if f.endswith(os.path.join("nv_ui_qt", "export.py")) or os.path.basename(f) == "denoise_video_v5.py":
            return True
    return False


def _label():
    """(module, function) of the outermost frame of a native/verify_* script, for grouping the results; else
    ("export", function) in an export process (_export_process); else ("?", "?")."""
    f = sys._getframe(2)
    best, inner = ("?", "?"), None
    while f is not None:
        fn = os.path.basename(f.f_code.co_filename)
        if inner is None:
            inner = f.f_code.co_name
        if fn.startswith("verify_") and f.f_code.co_name not in ("run_case", "main", "<module>", "run", "<lambda>",
                                                                  "<listcomp>", "<genexpr>"):
            best = (fn[:-3], f.f_code.co_name)
        f = f.f_back
    if best[0] == "?" and _export_process():
        return ("export", inner or "?")
    return best


class _Proxy:
    def __init__(self, name, real):
        self.__dict__["_name"] = name
        self.__dict__["_real"] = real

    def __getattr__(self, k):
        return getattr(self._real, k)

    def __setattr__(self, k, v):
        setattr(self._real, k, v)

    def __call__(self, *args):
        name, real = self._name, self._real
        sp = SPEC.get(name)
        if sp is None:
            with _lock:
                pickle.dump({"fn": name, "unspec": True}, _out(), protocol=4)
                _out().flush()
            return real(*args)
        argtypes = list(real.argtypes) if real.argtypes is not None else None
        scal = []
        for i, x in enumerate(args):
            t = argtypes[i] if argtypes else None
            if isinstance(x, (int, float)) or x is None:
                scal.append(x if not isinstance(x, bool) else int(x))
            elif isinstance(x, ctypes._SimpleCData) and t is not ctypes.c_void_p and not isinstance(x, ctypes.c_void_p):
                scal.append(x.value)
            elif hasattr(x, "__float__") and not isinstance(x, (ctypes._CFuncPtr, ctypes.Array, bytes)) \
                    and t in (ctypes.c_float, ctypes.c_double):
                scal.append(float(x))
            elif hasattr(x, "__index__") and t in (ctypes.c_int, ctypes.c_long, ctypes.c_size_t):
                scal.append(int(x))
            else:
                scal.append(x)
        desc = sp([_addr(x) if not isinstance(x, (int, float, bytes, ctypes._CFuncPtr)) and x is not None else
                   (x if isinstance(x, int) else 0) for x in scal])
        regions, rkey = [], {}

        def region(addr, nbytes, esize, role):
            if addr == 0:
                return ("null",)
            if nbytes == 0:
                return ("z",)
            k = (addr, nbytes)
            if k not in rkey:
                rkey[k] = len(regions)
                regions.append({"addr": addr, "n": nbytes, "esize": esize, "role": role,
                                "before": _rd(addr, nbytes)})
            elif role != "in":
                regions[rkey[k]]["role"] = role
            return ("r", rkey[k])

        enc, call, answers = [], list(args), {}
        for i, x in enumerate(scal):
            t = argtypes[i] if argtypes else None
            if i in desc:
                d = desc[i]
                a = _addr(x)
                if d[0] == "buf":
                    enc.append(region(a, d[1], d[2], d[3]))
                elif a == 0:
                    enc.append(("null",))
                else:
                    ps = _ptrs_at(a, len(d[1]))
                    enc.append(("a", [region(p, s, d[2], d[3]) for p, s in zip(ps, d[1])]))
            elif isinstance(x, ctypes._CFuncPtr) or (isinstance(t, type) and issubclass(t, ctypes._CFuncPtr)):
                if x is None or not bool(x):
                    enc.append(("cb", None))
                else:
                    ans = answers.setdefault(i, [])

                    def cb(orig=x, ans=ans):
                        r = orig()
                        ans.append(int(r))
                        return r
                    call[i] = type(x)(cb)
                    enc.append(("cb", i))
            elif isinstance(x, bytes):
                enc.append(("s", x))
            elif isinstance(x, (int, float)) or x is None:
                enc.append(("v", x))
            else:
                raise TypeError(f"x86_bridge_cap: {name} argument {i} of type {type(x).__name__}")
        spans = sorted((r["addr"], r["addr"] + r["n"]) for r in regions)
        overlap = any(spans[k + 1][0] < spans[k][1] for k in range(len(spans) - 1))
        t0 = time.perf_counter()
        ret = real(*call)
        dt = time.perf_counter() - t0
        rec = {"fn": name, "argtypes": [_tname(t) for t in argtypes] if argtypes is not None else None,
               "restype": _tname(real.restype) if real.restype is not int else "c_int",
               "args": [("cb", answers[e[1]]) if e[0] == "cb" and e[1] is not None else e for e in enc],
               "ret": ret if isinstance(ret, (int, float)) or ret is None else repr(ret), "t": dt,
               "label": _label(), "overlap": overlap, "regions": []}
        for r in regions:
            after = _rd(r["addr"], r["n"])
            rec["regions"].append({"n": r["n"], "esize": r["esize"], "role": r["role"], "before": _blob(r["before"]),
                                   "after_sha": hashlib.sha256(after).hexdigest(),
                                   "after": after if r["n"] <= KEEP_AFTER else None,
                                   "changed": after != r["before"]})
        with _lock:
            rec["pid"], rec["seq"] = os.getpid(), _state["seq"]
            _state["seq"] += 1
            pickle.dump(rec, _out(), protocol=4)
            _out().flush()
        return ret


_RealCDLL = ctypes.CDLL


class CapCDLL(_RealCDLL):
    def __getitem__(self, name):
        f = _RealCDLL.__getitem__(self, name)
        if isinstance(name, str) and name.startswith("nvc_") and "libnvcore" in os.path.basename(self._name or ""):
            return _Proxy(name, f)
        return f


def install():
    if CAP:
        ctypes.CDLL = CapCDLL
