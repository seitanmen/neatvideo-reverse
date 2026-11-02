"""Acceptance check of the AFT callers (2026-11-02): the production entries nv_aft_v5.apply_aft (Python) and
nv_native.apply_aft (the C ABI: nvc_aft_input16 -> nvc_profile_build_cancel -> nvc_aft -> nvc_aft_combine) on the
14 genuine .dnp of Phase C, with frame 5 of the clip given to Resolve (TIFF codes / 257, NOT quantized here: the
entries quantize) and the genuine's sample area.  "Two fields" (e9i, TFF): the frame's top-field rows stacked over
its bottom-field rows (what qtui fields.cpp run_aft_fields builds from the field RGBs), the area in the stacked image.

What is compared with the .dnp (the BuildNoiseModel profile and the AFT blob are captured inside the entry: the
module functions npr.build_profile / nv_aft.auto_fine_tune resp. nv_native.profile_blob / aft_blob are wrapped):
  model   the 6 plane models (+0x4c, 6 x 2005 words) of the entry's BuildNoiseModel vs the .dnp
  curve   the 27 AFT levels (ulp) and the flags +0x18 (u32), +0x1c, +0x1d, +0x1e of the entry's AFT blob vs the .dnp
The pre-AFT curve is the entry's own (a fresh BuildNoiseModel: the production behaviour); the genuine's was fresh,
except pa1_s4 (after Reset; Phase C: same result as fresh) and pa1_s5 (AFT again on s4, not reproduced by the callers).
Negative controls (C path): --neg runs the C entry without the 16-bit step, and the e9i with the fields stacked the
other way round (bottom over top; the area moved with its field so the model stays the same).
Nothing here runs or loads the genuine binaries.

  python3 verify_callers.py [--python] [--neg] [--only NAME ...]      writes ../out/verify_callers.{txt,json}
"""
import argparse
import ctypes
import json
import os
import re
import struct
import sys
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
WT = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
OUT = os.path.join(HERE, "..", "out")
PC = "/Users/seitan/Desktop/work/reverse/work/omo/wp_aft_runtime"
sys.path.insert(0, os.path.join(PC, "tools"))
import aftlib as A  # noqa: E402
import detail as D  # noqa: E402
import phase_c as P  # noqa: E402
# the worktree's repro and aft_port before the main checkout's (Phase C's tools put that one on sys.path)
sys.path.insert(0, os.path.join(WT, "reference", "genuine", "analysis", "noise", "aft_port", "src"))
sys.path.insert(0, os.path.join(WT, "reference", "python"))
for _m in [m for m in sys.modules if m.startswith("nv")]:
    del sys.modules[_m]
import nv_aft_v5  # noqa: E402
import nv_aft  # noqa: E402
import nv_native as nn  # noqa: E402
import nv_profile as npr  # noqa: E402
for _m in (nv_aft_v5, nv_aft, nn, npr):
    assert _m.__file__.startswith(WT), _m.__file__

F32 = np.float32
FRAME = 5
MODEL0, MODELN = 0x4C, 6 * 0x1F54
PA1_AREA = (160, 616, 128, 128)       # (y, x, h, w): Phase C locate_sample.py (frame 5, RMS 0.018)


def model_words(img):
    return np.frombuffer(bytes(img[MODEL0:MODEL0 + MODELN]), "<u4")


def e9p_area(rgb, gen):
    """The 128..48 square whose RGB means are closest to the .dnp's sample means (+0xbf28; E9 progressive .dnp carry
    no sample).  Returns (y, x, h, w), max |mean difference|."""
    m = np.array(struct.unpack_from("<3f", gen, 0xbf28))
    q = nv_aft_v5.to_internal16(rgb).astype(np.float64)
    H, W = q.shape[:2]
    cs = np.zeros((H + 1, W + 1, 3))
    cs[1:, 1:] = q.cumsum(0).cumsum(1)
    best = None
    for s in (320, 192, 128, 96, 64, 48):
        S = (cs[s:, s:] - cs[:-s, s:] - cs[s:, :-s] + cs[:-s, :-s]) / (s * s)
        d = np.abs(S - m).max(-1)
        y, x = np.unravel_index(np.argmin(d), d.shape)
        if best is None or d[y, x] < best[1]:
            best = ((int(y), int(x), s, s), float(d[y, x]))
    return best


def e9i_areas():
    """locate_i.log (Phase C): field, y, x of each e9i sample in its field."""
    res = {}
    name = None
    for ln in open(os.path.join(PC, "out", "logs", "locate_i.log")):
        if ln.startswith("== "):
            name = ln[3:].strip()
        m = re.match(r"MATCH frame (\d+) (top|bottom) y (\d+) x (\d+)", ln)
        if m and name:
            res[name] = (m.group(2), int(m.group(3)), int(m.group(4)))
    return res


def cases():
    out = []
    loc = e9i_areas()
    for name, (src, lay) in P.PROG.items():
        gen = A.image(P.dnp_path(name))
        rgb = A.to_rgb(A.read_codes(src, lay, FRAME), "d257")
        if name.startswith("pa1"):
            area, how = PA1_AREA, "Phase C locate_sample"
        else:
            area, d = e9p_area(rgb, gen)
            how = "sample means (+0xbf28) max diff %.1e" % d
        out.append(dict(name=name, gen=gen, rgb=rgb, area=area, how=how, rev=None))
    for name, (src, lay) in D.FIELDS.items():
        gen = A.image(P.dnp_path(name))
        p = A.load_dnp(P.dnp_path(name))
        s = p["samples"]
        fld, fy, fx = loc[name]
        rgb = A.to_rgb(A.read_codes(src, lay, FRAME), "d257")
        top, bot = rgb[0::2], rgb[1::2]
        hf = top.shape[0]
        stacked = np.ascontiguousarray(np.concatenate([top, bot], 0))      # TFF: first field (top) over the second
        rev = np.ascontiguousarray(np.concatenate([bot, top], 0))
        y = fy if fld == "top" else hf + fy
        yr = fy if fld == "bottom" else hf + fy
        out.append(dict(name=name, gen=gen, rgb=stacked, area=(y, fx, s["h"], s["W"]),
                        how="locate_i.log: %s field y %d x %d" % (fld, fy, fx),
                        rev=dict(rgb=rev, area=(yr, fx, s["h"], s["W"]))))
    return out


def prof_stub(area):
    c = [0.0, 32.0, 64.0, 96.0, 128.0, 160.0, 192.0, 224.0, 256.0]
    return {"area": [int(v) for v in area], "curve_centers": c, "curve": {n: [1.0] * 9 for n in ("Y", "Cb", "Cr")},
            "tot_ratio": {"Y": 1.0, "Cb": 1.0, "Cr": 1.0}, "area_level": 128.0,
            "sigma": {"Y": 1.0, "Cb": 1.0, "Cr": 1.0}}


def run_entry(path, rgb, area):
    """Call the production entry; returns (raw BuildNoiseModel blob, AFT blob, prof)."""
    cap = {}
    if path == "python":
        o_bp, o_aft = npr.build_profile, nv_aft.auto_fine_tune

        def bp(*a, **k):
            r = o_bp(*a, **k)
            cap["raw"] = bytes(r[0] if isinstance(r, tuple) else r)
            return r

        def aft(*a, **k):
            cap["aft"] = bytes(o_aft(*a, **k))
            return cap["aft"]
        npr.build_profile, nv_aft.auto_fine_tune = bp, aft
        try:
            prof = nv_aft_v5.apply_aft(prof_stub(area), rgb)
        finally:
            npr.build_profile, nv_aft.auto_fine_tune = o_bp, o_aft
    else:
        o_pb, o_ab = nn.profile_blob, nn.aft_blob

        def pb(*a, **k):
            cap["raw"] = o_pb(*a, **k)
            return cap["raw"]

        def ab(*a, **k):
            cap["aft"] = o_ab(*a, **k)
            return cap["aft"]
        nn.profile_blob, nn.aft_blob = pb, ab
        try:
            prof = nn.apply_aft(prof_stub(area), rgb)
        finally:
            nn.profile_blob, nn.aft_blob = o_pb, o_ab
        assert prof is not None, "the C entry returned None (fallback)"
    return cap["raw"], cap["aft"], prof


def summarize(gen, raw, aft):
    rows = A.compare(gen, aft)
    return dict(model_diff=int((model_words(raw) != model_words(gen)).sum()),
                max_ulp=max(r["ulp"] for r in rows), zero_ulp=sum(r["ulp"] == 0 for r in rows),
                flags=sum(r["flags_eq"] for r in rows), max_rel=max(abs(r["rel"]) for r in rows),
                measured=sum(1 for r in rows if r["gen_valid"]))


def fmt(tag, s):
    return "%-8s model %5d/12030  max ulp %8d  0-ulp %2d/27  flags %2d/27  max|rel| %.2e" % (
        tag, s["model_diff"], s["max_ulp"], s["zero_ulp"], s["flags"], s["max_rel"])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--python", action="store_true", help="also the Python entry (about 1 min a case)")
    ap.add_argument("--neg", action="store_true", help="negative controls on the C entry")
    ap.add_argument("--only", nargs="*")
    a = ap.parse_args()
    assert nn.AVAILABLE and nn.HAS_AFT_INPUT16, "libnvcore with nvc_aft_input16 not loaded (%s)" % nn.LIB_PATH
    os.makedirs(OUT, exist_ok=True)
    lines = ["lib %s" % nn.LIB_PATH]
    print(lines[0], flush=True)
    rep = {}
    for c in cases():
        if a.only and c["name"] not in a.only:
            continue
        r = rep[c["name"]] = dict(area_yxhw=c["area"], area_from=c["how"])
        lines.append("== %s  %dx%d  area (y, x, h, w) %s  [%s]" % (c["name"], c["rgb"].shape[1], c["rgb"].shape[0],
                                                                   c["area"], c["how"]))
        print(lines[-1], flush=True)
        t = time.time()
        raw_c, aft_c, prof_c = run_entry("c", c["rgb"], c["area"])
        r["c"] = summarize(c["gen"], raw_c, aft_c)
        lines.append("   " + fmt("C", r["c"]) + "  (%.1f s)" % (time.time() - t))
        print(lines[-1], flush=True)
        if a.python:
            t = time.time()
            raw_p, aft_p, prof_p = run_entry("python", c["rgb"], c["area"])
            r["python"] = summarize(c["gen"], raw_p, aft_p)
            r["python_eq_c"] = dict(raw=raw_p == raw_c, aft=aft_p == aft_c,
                                    prof=json.dumps(prof_p, sort_keys=True) == json.dumps(prof_c, sort_keys=True))
            lines.append("   " + fmt("Python", r["python"]) + "  (%.1f s)" % (time.time() - t))
            lines.append("   Python == C: BuildNoiseModel blob %s, AFT blob %s, prof dict %s" % (
                r["python_eq_c"]["raw"], r["python_eq_c"]["aft"], r["python_eq_c"]["prof"]))
            print("\n".join(lines[-2:]), flush=True)
        if a.neg:
            o = nn._lib.nvc_aft_input16
            nn._lib.nvc_aft_input16 = lambda i, n, out: ctypes.memmove(out, i, 4 * n)    # no 16-bit step
            try:
                raw_n, aft_n, _ = run_entry("c", c["rgb"], c["area"])
            finally:
                nn._lib.nvc_aft_input16 = o
            r["neg_float"] = summarize(c["gen"], raw_n, aft_n)
            lines.append("   " + fmt("-q16", r["neg_float"]))
            print(lines[-1], flush=True)
            if c["rev"] is not None:
                raw_n, aft_n, _ = run_entry("c", c["rev"]["rgb"], c["rev"]["area"])
                r["neg_reversed"] = summarize(c["gen"], raw_n, aft_n)
                lines.append("   " + fmt("rev", r["neg_reversed"]) + "  (bottom field over top, area %s)" % (
                    c["rev"]["area"],))
                print(lines[-1], flush=True)
    suffix = "" if not a.only else "_" + "_".join(a.only)
    with open(os.path.join(OUT, "verify_callers%s.txt" % suffix), "w") as f:
        f.write("\n".join(lines) + "\n")
    with open(os.path.join(OUT, "verify_callers%s.json" % suffix), "w") as f:
        json.dump(rep, f, indent=1, default=lambda o: o.tolist() if hasattr(o, "tolist") else str(o))


if __name__ == "__main__":
    main()
