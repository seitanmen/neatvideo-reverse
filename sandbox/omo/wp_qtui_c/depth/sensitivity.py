"""Sensitivity of verify_bridge_depth: perturb the Python reference (not the C code) in small ways and check that the
comparison reports differences.  python3 sandbox/omo/wp_qtui_c/depth/sensitivity.py"""
import os, sys, warnings
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))
sys.path[:0] = [os.path.join(ROOT, "native"), os.path.join(ROOT, "repro")]
warnings.simplefilter("ignore", RuntimeWarning)
import verify_bridge_depth as V
import nv_depth as nd
from nv_ui_qt import ofx_bridge as ob

orig_ti, orig_fi, orig_san, orig_pack = nd.to_internal, nd.from_internal, nd.sanitize, ob.pack_yuv420p

def ti_kg(raw, fmt, matrix="bt709"):          # kg = 1 - (kr + kb) instead of (1 - kr) - kb
    if fmt.family != "rgb":
        return orig_ti(raw, fmt, matrix)
    G, B, R = raw
    s = 255.0 if fmt.is_float else 1.0 / 2 ** (fmt.bits - 8)
    r, g, b = (np.asarray(p, np.float64) * s for p in (R, G, B))
    kr, kb = nd._KRB.get(matrix, nd._KRB["bt709"])
    y = kr * r + (1 - (kr + kb)) * g + kb * b
    cb = (b - y) / (2 * (1 - kb)) + 128.0
    cr = (r - y) / (2 * (1 - kr)) + 128.0
    return tuple(np.ascontiguousarray(v, np.float32) for v in (y, cb, cr))

def ti_widen(raw, fmt, matrix="bt709"):       # odd columns (c + nxt) / 2 in float64 of the scaled values + 1 ulp
    out = orig_ti(raw, fmt, matrix)
    if fmt.family == "yuv" and fmt.chroma == "422" and not fmt.legacy:
        out = list(out)
        out[1] = out[1].copy(); out[1][:, 1::2] = np.nextafter(out[1][:, 1::2], np.float32(np.inf))
    return tuple(out)

def fi_noclip_nan(planes, fmt, matrix="bt709", restore=None):   # NaN -> 255 instead of 0 in the legacy cast
    if fmt.legacy:
        return b"".join(np.clip(np.rint(np.nan_to_num(o, nan=255.0)), 0, 255).astype(np.uint8).tobytes() for o in planes)
    return orig_fi(planes, fmt, matrix, restore)

def san_signed_zero(raw):                      # max with -0 preferred
    out, info = orig_san(raw)
    return out, info

def pack_trunc(planes, depth=8, restore=None):  # truncation instead of rint at depth 16
    if depth != 16:
        return orig_pack(planes, depth, restore)
    return b"".join(np.clip(np.trunc(np.asarray(p, np.float32) * 256), 0, 65535).astype("<u2").tobytes() for p in planes)

for name, attr, fn, case in (("rgb kg order", "to_internal", ti_kg, "formats"),
                             ("4:2:2 widen +1 ulp", "to_internal", ti_widen, "formats"),
                             ("legacy NaN -> 255", "from_internal", fi_noclip_nan, "formats"),
                             ("OFX 16 trunc", "pack", pack_trunc, "ofx")):
    if attr == "pack":
        ob.pack_yuv420p = fn
    else:
        setattr(nd, attr, fn)
    ok, tot, bad = V.CASES[case](1, True)
    nd.to_internal, nd.from_internal, ob.pack_yuv420p = orig_ti, orig_fi, orig_pack
    print(f"{name:22s}: {tot - ok} of {tot} words differ -> {'DETECTED' if ok < tot else 'NOT DETECTED'}")
