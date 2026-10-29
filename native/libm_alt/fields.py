# W2a: the impact stage of native/verify_libm_alt.py.  The replay stage recorded, per native call and alt,
# the differing 4-byte words of every output file (diff_alt<a>_<arg>.npz: idx, default word, alt word).
# Here the outputs are parsed with the driver formats (the headers of native/nv_*_main.cpp) so that every
# differing word gets a field name, and the fields are compared as values (ULP, absolute difference):
#   curve  (nv_curve_test curve / interp / clip / tile)     the 8-point tables, interpolations, IsClip, tiles
#   flat   (nv_flat_test flat / cancel)                      FindFlatArea area / status, curve, candidates, trace
#   v5     (nv_v5_test, the temporal / spatial suites)      the planes by case kind; 8 / 10 / 16-bit codes
#   aft    (nv_aft_test aft / sim / finish / combine)        profile fields, Quality / Completeness %, bins
#   model / profile / band / lzma / quality                 word ranges (no parser needed when nothing differs)
import glob
import json
import os
import struct
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
NATIVE = os.path.dirname(HERE)
ROOT = os.path.dirname(NATIVE)
SUITES = ["quality", "band", "curve", "lzma", "flat", "flat_cancel", "model", "temporal", "spatial", "profile", "aft"]
ALT_NAME = {0: "system libm via libm_alt (plumbing)", 1: "openlibm 0.8.8", 2: "musl 1.2.6", 3: "naive (double)"}
PROF = 0xc300
MODEL_WORDS = 0x1f54 // 4


# ------------------------------------------------------------------------------------- reading
class R:
    def __init__(self, b, off=0):
        self.b, self.o = b, off

    def take(self, fmt, n=1):
        dt = np.dtype(fmt)
        a = np.frombuffer(self.b, dt, n, self.o)
        self.o += dt.itemsize * n
        return a

    def one(self, fmt):
        return self.take(fmt, 1)[0]


class Fields:
    """(label, dtype, byte offset, count, case) of one output file."""

    def __init__(self):
        self.f = []

    def add(self, label, dtype, off, n, case=None):
        if n > 0:
            self.f.append((label, np.dtype(dtype), off, int(n), case))

    def at(self, r, label, dtype, n, case=None):
        self.add(label, dtype, r.o, n, case)
        r.take(dtype, n)


# ------------------------------------------------------------------------------- curve driver
def p_curve(sub, inb, outb):
    F = Fields()
    if sub == "curve":
        n = len(outb) // 257
        for c in range(n):
            o = 257 * c
            F.add("curve.ok", "u1", o, 1, c)
            F.add("curve.xs", "<f8", o + 1, 8, c)
            F.add("curve.ys", "<f8", o + 65, 8, c)
            F.add("curve.sums", "<f8", o + 129, 8, c)
            F.add("curve.counts", "<i8", o + 193, 8, c)
    elif sub == "interp":
        r, ro = R(inb), R(outb)
        for c in range(int(r.one("<u8"))):
            k = int(r.one("<i4"))
            r.take("<f8", 2 * k)
            q = int(r.one("<i4"))
            r.take("<f8", q)
            F.at(ro, "interp.values", "<f8", q, c)
    elif sub == "clip":
        F.add("isclip", "<f4", 0, len(outb) // 4)
    elif sub == "tile":
        r, ro = R(inb), R(outb)
        for c in range(int(r.one("<u8"))):
            W, H = int(r.one("<i4")), int(r.one("<i4"))
            r.take("<f4", 2 * W * H)
            for nm in ("noise", "weight", "bright"):
                F.at(ro, f"tile.{nm}", "<f4", W * H, c)
    return F


# -------------------------------------------------------------------------------- flat driver
def p_flat(sub, inb, outb):
    F = Fields()
    r = R(outb)
    c = 0
    if sub == "cancel":
        while r.o < len(outb):
            F.at(r, "flat.area", "<i4", 4, c)
            F.at(r, "flat.status", "<i4", 1, c)
            F.at(r, "flat.queries", "<i4", 1, c)
            c += 1
        return F
    while r.o < len(outb):
        F.at(r, "flat.area", "<i4", 4, c)
        F.at(r, "flat.status", "<i4", 1, c)
        F.at(r, "flat.s_rw_rh_i0_run_ok", "<i4", 6, c)
        F.at(r, "flat.curve.xs", "<f8", 8, c)
        F.at(r, "flat.curve.ys", "<f8", 8, c)
        rn = int(r.one("<i8"))
        F.at(r, "flat.reduced_planes", "<f4", 4 * rn, c)
        ns = int(r.one("<i4"))
        for _ in range(ns):
            F.at(r, "flat.size.meta", "<i4", 3, c)
            F.at(r, "flat.cand.xy", "<i8", 128, c)
            F.at(r, "flat.cand.means", "<f4", 64, c)
            F.at(r, "flat.cand.scores", "<f4", 64, c)
            for _ in range(int(r.one("<i4"))):
                F.at(r, "flat.placed.box", "<i4", 4, c)
                F.at(r, "flat.placed.isclip", "<f4", 3, c)
        nt = int(r.one("<i4"))
        for _ in range(nt):
            F.at(r, "flat.trace.ints", "<i4", 8, c)
            for nm in ("mean", "nz", "fl0", "fl1", "fl2", "E", "R", "score"):
                F.at(r, f"flat.trace.{nm}", "<f4", 1, c)
            nw = int(r.one("<i4"))
            F.at(r, "flat.trace.words", "<u4", nw, c)
        c += 1
    return F


# ---------------------------------------------------------------------------------- v5 driver
V5_KIND = {1: "spatial_v5", 4: "spatial_v5+mix", 2: "temporal_v5", 3: "process_frame_v5", 10: "gauss1", 11: "uniform",
           12: "map_coords1", 13: "median3", 14: "curve_sigma", 15: "atrous", 16: "down2", 17: "estimate_flow_v5",
           18: "pyramid_weight", 19: "np.exp", 20: "to_grid", 21: "dense_flow", 22: "warp_plane", 23: "search_block",
           24: "refine"}


def v5_cases(inb):
    """kind of every case of a CASES.bin"""
    r = R(inb)
    kinds = []
    for _ in range(int(r.one("<u4"))):
        kind = int(r.one("<u4"))
        n = int(r.one("<i4"))
        r.take("<f8", n + 3 * n)
        r.take("<i4", 3)
        r.take("<f8", 15 + 3)
        r.take("<i4", 4)
        r.take("<f8", 8 + 5 + 5 + 3)
        r.take("<i4", int(r.one("<i4")))
        r.take("<f8", int(r.one("<i4")))
        for _ in range(int(r.one("<i4"))):
            H, W = int(r.one("<i4")), int(r.one("<i4"))
            r.take("<f4", H * W)
        kinds.append(kind)
    return kinds


def v5_plane_name(kind, j):
    if kind in (1, 4):
        return ("out.Y", "out.Cb", "out.Cr")[j] if j < 3 else f"plane{j}"
    if kind == 2:
        return ("out.Y", "out.Cb", "out.Cr", "rv.Y", "rv.Cb", "rv.Cr")[j] if j < 6 else ("flow.vy", "flow.vx")[(j - 6) % 2]
    if kind == 3:
        return ("out.Y", "out.Cb", "out.Cr")[j] if j < 3 else ("flow.vy", "flow.vx")[(j - 3) % 2]
    return f"plane{j}"


def p_v5(sub, inb, outb):
    F = Fields()
    kinds = v5_cases(inb)
    r = R(outb)
    for c, kind in enumerate(kinds):
        for j in range(int(r.one("<u4"))):
            H, W = int(r.one("<i4")), int(r.one("<i4"))
            F.at(r, f"{V5_KIND.get(kind, kind)}.{v5_plane_name(kind, j)}", "<f4", H * W, c)
    return F


# ------------------------------------------------------------------------------- profile blob
def model_family(w):
    if w < 5: return "levels.mean"
    if w < 10: return "levels.q99"
    if w < 15: return "levels.q50"
    if w < 55: return "levels.cv"
    if w < 80: return "dc"
    if w < 480: return "A"
    if w < 505: return "R"
    if w < 905: return "B"
    if w < 1305: return "C"
    if w < 1345: return "ad319c.score"
    if w < 1985: return "ad319c.offsets"
    if w < 2005: return "ad1ae0.q"
    return "unused"


def profile_word_labels():
    """label and dtype of every 4-byte word of the 0xc300-byte profile (nv_profile.cpp)."""
    lab, typ = [None] * (PROF // 4), ["<u4"] * (PROF // 4)

    def put(off, n, name, t="<u4"):
        for k in range(n):
            lab[off // 4 + k] = name
            typ[off // 4 + k] = t
    put(0, 6, "ptr")
    put(0x18, 5, "lzma.len_w_h")
    put(0x2c, 7, "hdr.misc")
    put(0x48, 1, "mode", "<i4")
    for p in range(6):
        for w in range(MODEL_WORDS):
            fam = model_family(w)
            put(0x4c + p * 0x1f54 + 4 * w, 1, f"model[{p}].{fam}", "<u4" if fam == "ad319c.offsets" else "<f4")
    for c, off in enumerate((0xbc44, 0xbd10, 0xbddc)):
        put(off, 51, f"tile_dev[{c}]", "<f4")
    put(0xbea8, (0xbf28 - 0xbea8) // 4, "mcstats", "<f4")
    put(0xbf28, 3, "means", "<f4")
    put(0xbf34, 1, "flags.0xbf34")
    put(0xbf38, 1, "size_class", "<i4")
    put(0xbf3c, 1, "clip_flags")
    put(0xbf40, 1, "curve.mode")
    for c in range(3):
        for k in range(9):
            e = 0xbf44 + 0x120 * c + 0x20 * k
            put(e, 1, f"curve[{c}].level", "<f4")
            put(e + 4, 5, f"curve[{c}].scale", "<f4")
            put(e + 0x18, 1, f"curve[{c}].valid")
            put(e + 0x1c, 1, f"curve[{c}].flags(marker,clip,gt3)")
    put(0xc2a4, 1, "lzma_ok")
    put(0xc2a8, 6, "tail.misc")
    put(0xc2c0, 2, "area.w_h", "<i4")
    put(0xc2c8, 10, "tail.consts", "<f4")
    for i in range(PROF // 4):
        if lab[i] is None:
            lab[i] = "other"
    return lab, typ


PROF_LAB, PROF_TYP = profile_word_labels()


def add_profile(F, r, case, prefix):
    base = r.o
    i = 0
    while i < PROF // 4:
        j = i
        while j < PROF // 4 and PROF_LAB[j] == PROF_LAB[i] and PROF_TYP[j] == PROF_TYP[i]:
            j += 1
        F.add(f"{prefix}{PROF_LAB[i]}", PROF_TYP[i], base + 4 * i, j - i, case)
        i = j
    r.take("u1", PROF)


# --------------------------------------------------------------------------------- aft driver
def p_aft(sub, inb, outb):
    F = Fields()
    r = R(outb)
    if sub == "aft":
        c = 0
        while r.o < len(outb):
            F.at(r, "aft.status", "<u4", 1, c)
            add_profile(F, r, c, "aft.prof.")
            F.at(r, "aft.out3", "<f4", 3, c)
            F.at(r, "aft.diag", "<i4", 7, c)
            for b in range(27):
                n = int(r.one("<u4"))
                F.at(r, "aft.bin_values", "<f4", n, c)
            c += 1
    elif sub == "sim":
        F.add("aft.sim", "<f4", 0, len(outb) // 4)
    elif sub == "finish":
        c = 0
        while r.o < len(outb):
            add_profile(F, r, c, "finish.prof.")
            F.at(r, "finish.out3", "<f4", 3, c)
            c += 1
    elif sub == "shuffle":
        F.add("shuffle.order", "<i4", 0, len(outb) // 4)
    elif sub == "combine":
        ri = R(inb)
        for c in range(int(ri.one("<u8"))):
            ri.take("u1", 2 * PROF)
            n = int(ri.one("<i4"))
            ri.take("<f8", n)
            ri.take("<f4", 3 * n)
            ri.take("<f8", 4)
            ri.take("<i4", 1)
            F.at(r, "combine.cw", "<f8", 9, c)
            F.at(r, "combine.face", "<f8", 3 * n, c)
            F.at(r, "combine.curve", "<f4", 3 * n, c)
            F.at(r, "combine.sigma", "<f8", 3, c)
            F.at(r, "combine.marker", "u1", 27, c)
            F.at(r, "combine.clip", "u1", 27, c)
            F.at(r, "combine.gt3", "u1", 27, c)
    return F


def p_model(sub, inb, outb):
    F = Fields()
    if sub == "bm":
        F.add("bm", "<u4", 0, len(outb) // 4)
        return F
    n = len(outb) // (4 * 4097)
    for c in range(n):
        o = c * 4 * 4097
        F.add("model.status", "<u4", o, 1, c)
        w = 0
        while w < 4096:
            fam = model_family(w)
            j = w
            while j < 4096 and model_family(j) == fam:
                j += 1
            F.add(f"model.{fam}", "<u4" if fam in ("ad319c.offsets", "unused") else "<f4", o + 4 + 4 * w, j - w, c)
            w = j
    return F


def p_profile(sub, inb, outb):
    F = Fields()
    r = R(outb)
    if sub in ("floor", "noise"):
        F.add(sub, "<f4", 0, len(outb) // 4)
        return F
    c = 0
    while r.o + 4 + PROF <= len(outb):
        F.at(r, "profile.status", "<u4", 1, c)
        add_profile(F, r, c, "profile.")
        c += 1
    return F


PARSERS = {"nv_curve_test": p_curve, "nv_flat_test": p_flat, "nv_v5_test": p_v5, "nv_aft_test": p_aft,
           "nv_model_test": p_model, "nv_profile_test": p_profile}


# -------------------------------------------------------------------------------- comparisons
def ulp_f32(a, b):
    ia = a.view(np.int32).astype(np.int64)
    ib = b.view(np.int32).astype(np.int64)
    ka = np.where(ia < 0, np.int64(-(2 ** 31)) - ia, ia)
    kb = np.where(ib < 0, np.int64(-(2 ** 31)) - ib, ib)
    return np.abs(ka - kb)


def ulp_f64(a, b):
    """ULP distance of float64 pairs (IEEE order, -0 == +0); pairs of opposite signs -> inf"""
    ia = a.view(np.int64)
    ib = b.view(np.int64)
    same = (ia < 0) == (ib < 0)
    with np.errstate(over="ignore"):
        d = np.abs(np.where(same, ia - ib, 0)).astype(np.float64)
    zero = (a == 0) & (b == 0)
    return np.where(same, d, np.where(zero, 0.0, np.inf))


def compare_field(dt, da, db):
    """counts and magnitudes of one field (two arrays of the same dtype)."""
    if dt.kind == "f":
        bits = da.view(np.uint32 if dt.itemsize == 4 else np.uint64) != db.view(np.uint32 if dt.itemsize == 4 else np.uint64)
    else:
        bits = da != db
    n = int(bits.sum())
    res = {"n": int(da.size), "diff": n}
    if n and dt.kind == "f":
        a, b = da[bits], db[bits]
        nan = np.isnan(a) != np.isnan(b)
        fin = np.isfinite(a) & np.isfinite(b)
        u = ulp_f32(a, b) if dt.itemsize == 4 else ulp_f64(a, b)
        res["nan_mismatch"] = int(nan.sum())
        res["max_ulp"] = float(u[~nan].max()) if (~nan).any() else 0.0
        res["ulp_hist"] = {k: int(v) for k, v in zip(("1", "2", "3-16", ">16"),
                                                        (np.sum(u == 1), np.sum(u == 2), np.sum((u > 2) & (u <= 16)),
                                                         np.sum(u > 16)))}
        if fin.any():
            d = np.abs(a[fin].astype(np.float64) - b[fin].astype(np.float64))
            res["max_abs"] = float(d.max())
            den = np.abs(a[fin].astype(np.float64))
            with np.errstate(divide="ignore", invalid="ignore"):
                rel = np.where(den > 0, d / den, 0.0)
            res["max_rel"] = float(rel.max())
    elif n:
        res["examples"] = [[int(x), int(y)] for x, y in zip(da[bits][:4], db[bits][:4])]
    return res


def merge(acc, res, case):
    acc["n"] = acc.get("n", 0) + res["n"]
    acc["diff"] = acc.get("diff", 0) + res["diff"]
    if res["diff"]:
        acc.setdefault("cases", set()).add(case)
        for k in ("max_ulp", "max_abs", "max_rel"):
            if k in res:
                acc[k] = max(acc.get(k, 0.0), res[k])
        for k in ("nan_mismatch",):
            if k in res:
                acc[k] = acc.get(k, 0) + res[k]
        if "ulp_hist" in res:
            h = acc.setdefault("ulp_hist", {})
            for k, v in res["ulp_hist"].items():
                h[k] = h.get(k, 0) + v
        if "examples" in res:
            acc.setdefault("examples", []).extend(res["examples"][:4 - len(acc.get("examples", []))])


def reconstruct(cd, alt, arg, defb):
    p = os.path.join(cd, f"alt{alt}_{arg}")
    if os.path.exists(p):
        with open(p, "rb") as f:
            return f.read()
    z = np.load(os.path.join(cd, f"diff_alt{alt}_{arg}.npz"))
    if bool(z["truncated"]):
        return None
    n = len(defb) + (-len(defb)) % 4
    w = np.frombuffer(defb + b"\0" * (n - len(defb)), "<u4").copy()
    w[z["idx"].astype(np.int64)] = z["b"]
    return w.tobytes()[:len(defb)]


# ------------------------------------------------------------------------- final-output impact
def v5_codes(F, defb, altb, acc):
    """8 / 10 / 16-bit codes of the output planes as the export writes them (nv_depth: rint(x * k), clip)."""
    for label, dt, off, n, case in F.f:
        if ".out." not in label:
            continue
        a = np.frombuffer(defb, dt, n, off)
        b = np.frombuffer(altb, dt, n, off)
        if np.array_equal(a.view(np.uint32), b.view(np.uint32)):
            for bits, k in ((8, 1.0), (10, 4.0), (16, 256.0)):
                e = acc.setdefault(f"{label}@{bits}bit", {"pixels": 0, "diff": 0, "max_code": 0})
                e["pixels"] += n
            continue
        fin = np.isfinite(a) & np.isfinite(b)      # adversarial planes hold NaN / inf; frames do not
        for bits, k in ((8, 1.0), (10, 4.0), (16, 256.0)):
            top = 2 ** bits - 1
            ca = np.clip(np.rint(a[fin].astype(np.float32) * np.float32(k)), 0, top)
            cb = np.clip(np.rint(b[fin].astype(np.float32) * np.float32(k)), 0, top)
            d = np.abs(ca.astype(np.int64) - cb.astype(np.int64))
            e = acc.setdefault(f"{label}@{bits}bit", {"pixels": 0, "diff": 0, "max_code": 0})
            e["pixels"] += n
            e["diff"] += int((d > 0).sum())
            e["max_code"] = max(e["max_code"], int(d.max()) if d.size else 0)
            e["nonfinite_pixels"] = e.get("nonfinite_pixels", 0) + int((~fin).sum())


def quality_numbers(blob):
    sys.path.insert(0, os.path.join(ROOT, "repro"))
    from nv_ui_qt import quality as Q
    q = Q.quality_from_blob(blob)
    g = Q.genuine_g(blob)
    qp = None if q is None else int(np.floor(float(q) * 100.0 + 0.5))
    return qp, int(np.floor(float(g) * 100.0 + 0.5)), (None if q is None else float(q)), float(g)


def profile_blobs(F, b):
    """the 0xc300-byte blobs of an aft / finish / profile output, by case"""
    out = {}
    starts = {}
    for label, dt, off, n, case in F.f:
        if label.endswith("ptr") and n == 6:
            starts[case] = off
    for case, off in starts.items():
        out[case] = b[off:off + PROF]
    return out


# ----------------------------------------------------------------------------------- analyze
def analyze(out):
    res = {"suites": {}, "impact": {}}
    for s in SUITES:
        d = os.path.join(out, "capture", s)
        rec = os.path.join(d, "replay.jsonl")
        if not os.path.exists(rec):
            continue
        with open(rec) as f:
            rows = [json.loads(x) for x in f if x.strip()]
        S = res["suites"].setdefault(s, {})
        for r in rows:
            a = str(r["alt"])
            A = S.setdefault(a, {"calls": 0, "calls_diff": 0, "words": 0, "diff_words": 0, "rc_mismatch": 0,
                                 "stdout_mismatch": 0, "fields": {}, "by_sub": {}})
            A["calls"] += 1
            w = sum(o["words"] for o in r["outputs"])
            dw = sum(o["diff_words"] for o in r["outputs"])
            A["words"] += w
            A["diff_words"] += dw
            A["rc_mismatch"] += r["rc_alt"] != r["rc_def"]
            A["stdout_mismatch"] += r.get("stdout_equal") is False
            A["calls_diff"] += bool(dw or r["rc_alt"] != r["rc_def"] or r.get("stdout_equal") is False)
            sub = r["argv0"] if r["bin"] in ("nv_curve_test", "nv_flat_test", "nv_aft_test") else \
                ("bm" if r["argv0"] == "bm" else r["argv0"] if r["argv0"] in ("floor", "noise") else "main")
            B = A["by_sub"].setdefault(f"{r['bin']} {sub}", [0, 0, 0, 0])
            B[0] += 1
            B[1] += bool(dw)
            B[2] += w
            B[3] += dw
            if not dw:
                continue
            cd = os.path.join(d, "calls", r["call"])
            with open(os.path.join(cd, "meta.json")) as f:
                meta = json.load(f)
            inb = b""
            if meta["inputs"]:
                with open(os.path.join(cd, f"in_{meta['inputs'][0]}"), "rb") as f:
                    inb = f.read()
            for o in r["outputs"]:
                if not o["diff_words"]:
                    continue
                with open(os.path.join(cd, f"def_{o['arg']}"), "rb") as f:
                    defb = f.read()
                altb = reconstruct(cd, r["alt"], o["arg"], defb)
                parser = PARSERS.get(r["bin"])
                if altb is None or parser is None or len(altb) != len(defb):
                    A["fields"].setdefault("(unparsed)", {"n": 0, "diff": 0})["diff"] += o["diff_words"]
                    continue
                F = parser(sub if r["bin"] != "nv_v5_test" else "main", inb, defb)
                for label, dt, off, n, case in F.f:
                    da = np.frombuffer(defb, dt, n, off)
                    db = np.frombuffer(altb, dt, n, off)
                    merge(A["fields"].setdefault(label, {}), compare_field(dt, da, db), f"{r['call']}:{case}")
                # final outputs
                I = res["impact"].setdefault(s, {}).setdefault(a, {})
                if r["bin"] == "nv_flat_test":
                    for label, dt, off, n, case in F.f:
                        if label in ("flat.area", "flat.status"):
                            e = I.setdefault(label, {"cases": 0, "changed": 0, "examples": []})
                            da, db = np.frombuffer(defb, dt, n, off), np.frombuffer(altb, dt, n, off)
                            e["changed"] += int(not np.array_equal(da, db))
                            if not np.array_equal(da, db) and len(e["examples"]) < 5:
                                e["examples"].append([da.tolist(), db.tolist()])
                if r["bin"] == "nv_v5_test":
                    v5_codes(F, defb, altb, I.setdefault("codes", {}))
                if r["bin"] in ("nv_aft_test", "nv_profile_test"):
                    bd, ba = profile_blobs(F, defb), profile_blobs(F, altb)
                    e = I.setdefault("quality_pct", {"blobs": 0, "q_changed": 0, "c_changed": 0, "q_value_changed": 0,
                                                     "examples": []})
                    for case in bd:
                        if bd[case][0xbf34] == 0:
                            continue
                        nd, na = quality_numbers(bd[case]), quality_numbers(ba[case])
                        e["blobs"] += 1
                        e["q_changed"] += nd[0] != na[0]
                        e["c_changed"] += nd[1] != na[1]
                        e["q_value_changed"] += nd[2] != na[2] or nd[3] != na[3]
                        if (nd != na) and len(e["examples"]) < 6:
                            e["examples"].append({"case": f"{r['call']}:{case}", "default": nd, "alt": na})
        # unchanged-flat counts: cases with no diff count too
        for a, A in S.items():
            for label, v in A["fields"].items():
                if "cases" in v:
                    v["cases"] = len(v["cases"])
    # the flat cases that did not differ at all still count as "area unchanged"
    for s in ("flat", "flat_cancel"):
        n_cases = count_flat_cases(out, s)
        for a, I in res["impact"].get(s, {}).items():
            for k in ("flat.area", "flat.status"):
                if k in I:
                    I[k]["cases"] = n_cases
    res["flat_cases"] = {s: count_flat_cases(out, s) for s in ("flat", "flat_cancel")}
    return res


def count_flat_cases(out, s):
    d = os.path.join(out, "capture", s, "calls")
    n = 0
    for c in os.listdir(d) if os.path.isdir(d) else ():
        with open(os.path.join(d, c, "meta.json")) as f:
            meta = json.load(f)
        if meta["bin"] != "nv_flat_test":
            continue
        for i in meta["outputs"]:
            with open(os.path.join(d, c, f"def_{i}"), "rb") as f:
                b = f.read()
            F = p_flat(meta["argv"][0], b"", b)
            n += sum(1 for x in F.f if x[0] == "flat.status")
    return n


def print_summary(res):
    for s, S in res["suites"].items():
        for a, A in sorted(S.items()):
            print(f"[impact] {s} alt{a}: {A['calls_diff']}/{A['calls']} calls, {A['diff_words']}/{A['words']} words"
                  + (f", rc {A['rc_mismatch']}" if A["rc_mismatch"] else "")
                  + (f", stdout {A['stdout_mismatch']}" if A["stdout_mismatch"] else ""))
            for label, v in sorted(A["fields"].items()):
                if v.get("diff"):
                    print(f"           {label}: {v['diff']}/{v['n']} values in {v.get('cases', '?')} cases, "
                          f"max {v.get('max_ulp', '-')} ulp, abs {v.get('max_abs', '-')}, rel {v.get('max_rel', '-')}")
    for s, I in res["impact"].items():
        for a, v in sorted(I.items()):
            print(f"[final] {s} alt{a}: {json.dumps(v)[:600]}")


def write_report(out):
    with open(os.path.join(out, "impact.json")) as f:
        res = json.load(f)
    ulp = {}
    p = os.path.join(out, "ulp", "ulp.json")
    if os.path.exists(p):
        with open(p) as f:
            ulp = json.load(f)
    bridge = {}
    for a in range(4):
        p = os.path.join(out, "bridge", f"alt{a}.log")
        if os.path.exists(p):
            with open(p) as f:
                lines = f.read().splitlines()
            i = next((k for k, x in enumerate(lines) if x.startswith("summary")), None)
            bridge[a] = lines[i + 1:] if i is not None else lines[-8:]
    frames = {}
    p = os.path.join(out, "frames_e2e.json")
    if os.path.exists(p):
        with open(p) as f:
            frames = json.load(f)
    L = ["# W2a libm sensitivity: summary (generated by native/verify_libm_alt.py --stages report)", ""]
    L.append("## Suites (replay of every native call; words = 4-byte words of the output files)")
    L.append("")
    L.append("| suite | alt | calls differing / total | words differing / total | fields differing (values, max ULP) |")
    L.append("|---|---|---|---|---|")
    for s in SUITES:
        for a, A in sorted(res["suites"].get(s, {}).items()):
            fl = "; ".join(f"{k} {v['diff']}/{v['n']} ({v.get('max_ulp', '-')})" for k, v in sorted(A["fields"].items())
                           if v.get("diff")) or "none"
            L.append(f"| {s} | {a} | {A['calls_diff']}/{A['calls']} | {A['diff_words']}/{A['words']} | {fl} |")
    L.append("")
    L.append("## verify_bridge with libm_alt/bin/libnvcore.dylib")
    for a, lines in bridge.items():
        L.append(f"- alt {a}: " + " / ".join(x.strip() for x in lines if x.strip()))
    L.append("")
    L.append("## Final outputs")
    L.append("```")
    L.append(json.dumps(res["impact"], indent=1)[:20000])
    L.append("```")
    if frames:
        L.append("## End-to-end frames (frames_e2e.py)")
        L.append("```")
        L.append(json.dumps(frames, indent=1)[:20000])
        L.append("```")
    if ulp:
        L.append("## Function level (libm_ulp)")
        L.append("| func | alt | args | differ | max ULP | max rel | hist |")
        L.append("|---|---|---|---|---|---|---|")
        for x in ulp.get("all", []):
            if "hist" in x:
                L.append(f"| {x['func']} | {x['alt']} | {x['n']} | {x['bitdiff']} | {x['max_ulp']} | {x['max_rel']:.3g} | "
                         + ", ".join(f"{k}:{v}" for k, v in x["hist"].items() if v) + " |")
    with open(os.path.join(out, "summary.md"), "w") as f:
        f.write("\n".join(L) + "\n")
    with open(os.path.join(out, "summary.json"), "w") as f:
        json.dump({"impact": res, "ulp": ulp, "bridge": bridge, "frames": frames}, f, indent=1)
    print(f"[report] {os.path.join(out, 'summary.md')}")
