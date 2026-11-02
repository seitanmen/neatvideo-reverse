# Bit-exact verification of the native Quality port against the Python implementation.
#
#   make -C native && python3 native/verify_quality.py
#
# Generates structured + random profile blobs (after the 13 decoded genuine profiles of
# native/testdata/genuine_quality.json, whose screen values are checked too), computes Quality/Completeness with the
# Python ports (reference/python/nv_ui_qt/quality.py + quality_draft.py + repro/nv_flat.py), runs the
# C++ binary over the same blobs, and compares the results BIT FOR BIT.
import os
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, ROOT)
sys.path.insert(0, os.path.join(ROOT, "reference", "python"))
sys.path.insert(0, os.path.join(ROOT, "work", "omo", "coord", "verify_quality", "static"))
sys.dont_write_bytecode = True

import numpy as np  # noqa: E402
F32 = np.float32
U32 = np.uint32

PROF = 0xC300
MODEL = 0x1F54


def make_face(rng, style):
    """55 words of a model face (mean/q99/q50/cv), with realistic structure."""
    w = np.zeros(55, U32)
    f = rng.uniform(0, 10, 55).astype(F32)
    q99 = (np.cumprod(rng.uniform(0.4, 0.9, 5).astype(F32)) * F32(rng.uniform(0.5, 8))).astype(F32)
    q50 = q99 * rng.uniform(0.3, 0.95, 5).astype(F32)
    mean = q99 * rng.uniform(1.0, 3.0, 5).astype(F32)
    f[0:5], f[5:10], f[10:15] = mean, q99, q50
    cv = rng.uniform(-2, 6, 40).astype(F32)
    if style == "nan":
        cv[rng.random(40) < 0.1] = np.nan
        q99[rng.random(5) < 0.2] = np.nan
        mean[:] = np.nan
        f[0:15] = np.nan
    if style == "negcv":                      # chains that stop early
        m = rng.random(40) < 0.3
        cv[m] = -np.abs(cv[m])
    if style == "tiny":                       # q99 below the 1e-5 threshold at some level
        q99[2] = F32(0.0)
    f[15:55] = cv
    f[5:10], f[10:15] = q99, q50
    return f.view(U32)


def make_blob(rng, i):
    b = bytearray(PROF)
    u32 = np.frombuffer(b, U32)
    style = ("plain", "nan", "negcv", "tiny")[i % 4]
    b[0xBF34] = 0 if (i % 17 == 3) else 1                # some invalid profiles
    u32[0x48 // 4] = i % 4                               # colour modes 0..3
    for face in range(3, 6):
        off = (0x4C + face * MODEL) // 4
        u32[off:off + 55] = make_face(rng, style)
    # curve bins
    for c in range(3):
        for k in range(9):
            o = 0xBF44 + 0x120 * c + 0x20 * k
            lvl = F32(1.0)
            r = rng.random()
            if r < 0.15:
                lvl = F32(rng.uniform(0.2, 5.0))
            elif r < 0.2:
                lvl = F32(0.0)
            struct.pack_into("<6f", b, o, lvl, 1.0, 1.0, 1.0, 1.0, 1.0)
            r = rng.random()
            if r < 0.3:
                struct.pack_into("<I", b, o + 0x18, 1)   # valid (measured)
            elif r < 0.35:                               # valid as a u32: only an upper byte set
                b[o + 0x18 + int(rng.integers(1, 4))] = int(rng.integers(1, 256))
            if rng.random() < 0.3:
                b[o + 0x1C] = rng.integers(0, 3)         # marker (sometimes > 1)
            if rng.random() < 0.05:
                b[o + 0x1F] = int(rng.integers(1, 256))  # the unused byte (0x6d in genuine files)
            if rng.random() < 0.1:
                b[o + 0x1D] = 1
            if rng.random() < 0.1:
                b[o + 0x1E] = 1
            if rng.random() < 0.08:
                struct.pack_into("<f", b, o + 4, 0.0)    # zero scale word (not read)
            if rng.random() < 0.03:
                struct.pack_into("<f", b, o, float("nan") if rng.random() < 0.5 else -0.0)
    b[0xC2A4] = rng.integers(0, 2)
    struct.pack_into("<ii", b, 0xC2C0,
                     int(rng.integers(0, 600)), int(rng.integers(0, 600)))
    struct.pack_into("<i", b, 0xBF38, int(rng.integers(0, 7)))
    for c in range(3):
        b[0xBF3C + c] = rng.integers(0, 2)
    if i % 11 == 5:                                      # byte-soup blob (NaN/inf everywhere)
        soup = rng.integers(0, 256, PROF, dtype=np.uint8)
        soup[0xBF34] = 1
        b[:] = soup.tobytes()
    return bytes(b)


def expected(blob):
    import nv_flat as nf
    import quality_draft as qd
    from nv_ui_qt import quality as uiq
    g_py = qd.genuine_g(blob)
    g_ui = float(uiq.genuine_g(blob))
    assert struct.pack("<f", g_py) == struct.pack("<f", g_ui), "the two Python g's disagree"
    if blob[0xBF34] == 0:
        return None, None
    mode = struct.unpack_from("<i", blob, 0x48)[0]
    if not (0 <= mode < 4):
        mode = 1                                   # the genuine table has modes 0..3; blob soup may differ
    w = np.frombuffer(blob, U32)
    u = []
    for i in range(3):
        off = (0x4C + (3 + i) * MODEL) // 4
        luma = qd.LUMA_FLAGS[mode * 3 + i]
        f = nf.flatness(w[off:off + 55], bool(luma))
        v = qd.penalty(f)
        if luma == 0:
            v = F32(F32(v * F32(0.5)) + F32(0.5))
        u.append(v)
    P = F32(F32(F32(F32(u[0] + F32(0.5)) * F32(u[1] + F32(0.5))) * F32(u[2] + F32(0.5))) + F32(0.25))
    P = F32(P / F32(3.625))
    W, H = struct.unpack_from("<ii", blob, 0xC2C0)
    S = qd.size_term(W, H, struct.unpack_from("<i", blob, 0xBF38)[0])
    q = F32(F32(F32(F32(g_py) * F32(0.5)) + F32(0.5)) * P)
    q = F32(q * F32(S / F32(5.0)))
    for c in range(3):
        if blob[0xBF3C + c]:
            q = F32(q * F32(0.5))
    return float(q), float(g_py)


GENUINE = os.path.join(HERE, "testdata", "genuine_quality.json")


def genuine_blobs():
    """The 13 decoded genuine profiles (native/testdata/genuine_quality.json) with their screen values."""
    import json
    doc = json.load(open(GENUINE))
    out = []
    for c in doc["cases"]:
        b = bytearray(doc["blob_size"])
        for off, hx in c["ranges"]:
            d = bytes.fromhex(hx)
            b[off:off + len(d)] = d
        out.append((c["name"], c["quality"], c["completeness"], bytes(b)))
    return out


def pct(x):
    """quality.profile_numbers' qRound(x * 100) (double) and the panels' float qRound(x * 100.0f)."""
    return int(np.floor(float(x) * 100.0 + 0.5)), int(np.floor(float(F32(F32(x) * F32(100.0))) + 0.5))


# The repro state each genuine case maps to (quality._curve_blob): the AFT run's flags as nv_aft_v5.apply_aft stores
# them (marker = the u32 at +0x1c != 0) taken from the decoded file, Reset, or the AFT state of s5 plus the manually
# edited bins (pinned) of s6 / s7.
STATES = {"pa1_s3_resetft": ("reset", None, {}), "pa1_s6_manual_R4_up": ("aft", "pa1_s5_aft_again", {0: {4}}),
          "pa1_s7_manual_G2_down": ("aft", "pa1_s5_aft_again", {0: {4}, 1: {2}})}


def bins(blob):
    """(valid, marker, clip, gt3) of the 27 bins, [c][k]."""
    out = []
    for c in range(3):
        row = []
        for k in range(9):
            o = 0xBF44 + 0x120 * c + 0x20 * k
            row.append((struct.unpack_from("<I", blob, o + 0x18)[0] != 0, blob[o + 0x1C] != 0, blob[o + 0x1D] != 0,
                        blob[o + 0x1E] != 0))
        out.append(row)
    return out


def mapping_checks(gen):
    """quality._curve_blob writes the genuine's bin bytes (valid / marker / clip / >3.0) for the repro state of each
    genuine case, so its g gives the genuine's Completeness.  The frame is synthetic (only the curve is compared)."""
    from nv_ui_qt import quality as uiq
    blobs = {name: b for name, _, _, b in gen}
    y = np.full((64, 64), 120.0, F32) + np.random.default_rng(1).normal(0, 3, (64, 64)).astype(F32)
    planes = [(y, np.full((32, 32), 128.0, F32), np.full((32, 32), 128.0, F32))]
    bad = 0
    for name, _, wc, blob in gen:
        kind, src, pins = STATES.get(name, ("aft", name, {}))
        base = {"area": [0, 0, 64], "aft_applied": True}
        if kind == "reset":
            base["aft_reset"] = True
        else:
            st = bins(blobs[src])
            base["aft_bin_flags"] = {
                "marker": [[bool(st[c][k][1] or st[c][k][2] or st[c][k][3]) for k in range(9)] for c in range(3)],
                "clip": [[bool(st[c][k][2]) for k in range(9)] for c in range(3)],
                "gt3": [[bool(st[c][k][3]) for k in range(9)] for c in range(3)]}

        class PS:
            pass
        ps = PS()
        ps.base, ps.factors, ps.tune = base, [[1.0] * 9 for _ in range(3)], {}
        ps.pinned = [set(pins.get(c, ())) for c in range(3)]
        mine = bytes(uiq._curve_blob(ps, planes, False, "bt709"))
        same = bins(mine) == bins(blob)
        cp = pct(uiq.genuine_g(mine))[0]
        ok = same and cp == wc
        bad += not ok
        print(f"  mapping {name:28s} bins {'equal' if same else 'DIFFER'}  completeness {cp:3d}% (screen {wc:3d}%)  "
              f"{'OK' if ok else 'MISMATCH'}")
    print(f"bin mapping: {len(gen) - bad}/{len(gen)} match")
    return bad


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 4000
    rng = np.random.default_rng(0xC0FFEE)
    gen = genuine_blobs()
    blobs = [b for _, _, _, b in gen] + [make_blob(rng, i) for i in range(n)]
    inp = os.path.join(HERE, "tmp_blobs.bin")
    outp = os.path.join(HERE, "tmp_results.bin")
    exe = os.path.join(HERE, "nv_quality_test")
    with open(inp, "wb") as f:
        f.write(struct.pack("<Q", len(blobs)))
        for b in blobs:
            f.write(b)
    subprocess.run([exe, inp, outp], check=True)
    res = open(outp, "rb").read()
    bad = 0
    for i, blob in enumerate(blobs):
        v, q, c = struct.unpack_from("<Bff", res, 9 * i)
        eq, eg = expected(blob)
        if v:
            got_q, got_c = struct.pack("<f", q), struct.pack("<f", c)
            want_q = struct.pack("<f", eq) if eq is not None else b"\x00" * 4
            want_c = struct.pack("<f", eg)
            if got_q != want_q or got_c != want_c:
                bad += 1
                if bad <= 5:
                    print(f"MISMATCH blob {i}: q {got_q.hex()} vs {want_q.hex()}, "
                          f"c {got_c.hex()} vs {want_c.hex()}")
        else:
            if blob[0xBF34] != 0:
                bad += 1
                print(f"MISMATCH blob {i}: C++ says invalid, Python computes")
    # the genuine screens: Python and C++ give the Quality / Completeness the genuine UI showed
    from nv_ui_qt import quality as uiq
    gbad = 0
    for i, (name, wq, wc, blob) in enumerate(gen):
        v, q, c = struct.unpack_from("<Bff", res, 9 * i)
        pq, pg = uiq.quality_from_blob(blob), uiq.genuine_g(blob)
        got = {pct(pq), pct(q)}, {pct(pg), pct(c)}
        ok = v == 1 and got == ({(wq, wq)}, {(wc, wc)})
        gbad += not ok
        print(f"  genuine {name:28s} screen {wq:3d}% / {wc:3d}%  python {pct(pq)[0]:3d}% / {pct(pg)[0]:3d}%  "
              f"c++ {pct(q)[0]:3d}% / {pct(c)[0]:3d}%  {'OK' if ok else 'MISMATCH'}")
    print(f"genuine screens: {len(gen) - gbad}/{len(gen)} match")
    gbad += mapping_checks(gen)
    n = len(blobs)
    print(f"{n - bad}/{n} bit-exact" + ("" if bad == 0 else f"  ({bad} MISMATCHES)"))
    bad += gbad
    for p in (inp, outp):
        os.remove(p)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())


def debug_blob(i, n=4000):
    """Regenerate blob i (the numbering of main: the genuine cases first) and dump both sides' components."""
    rng = np.random.default_rng(0xC0FFEE)
    blobs = [b for _, _, _, b in genuine_blobs()] + [make_blob(rng, k) for k in range(n)]
    blob = blobs[i]
    p = os.path.join(HERE, "tmp_debug_blob.bin")
    open(p, "wb").write(blob)
    print(subprocess.run([os.path.join(HERE, "nv_quality_test"), "--debug", p],
                         capture_output=True, text=True).stdout)
    mode = struct.unpack_from("<i", blob, 0x48)[0]
    if not (0 <= mode < 4):
        mode = 1
    w = np.frombuffer(blob, U32)
    import nv_flat as nf
    import quality_draft as qd
    for face in range(3):
        off = (0x4C + (3 + face) * MODEL) // 4
        luma = bool(qd.LUMA_FLAGS[mode * 3 + face])
        f = nf.flatness(w[off:off + 55], luma)
        print(f"py face{face} flatness={float(f).hex()} luma={luma}")
