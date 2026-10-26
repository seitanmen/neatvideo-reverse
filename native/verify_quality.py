# Bit-exact verification of the native Quality port against the Python implementation.
#
#   make -C native && python3 native/verify_quality.py
#
# Generates structured + random profile blobs, computes Quality/Completeness with the
# Python ports (repro/nv_ui_qt/quality.py + quality_draft.py + repro/nv_flat.py), runs the
# C++ binary over the same blobs, and compares the results BIT FOR BIT.
import os
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
sys.path.insert(0, ROOT)
sys.path.insert(0, os.path.join(ROOT, "repro"))
sys.path.insert(0, os.path.join(ROOT, "sandbox", "omo", "coord", "verify_quality", "static"))
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
            if rng.random() < 0.3:
                b[o + 0x1C] = rng.integers(0, 3)         # marker (sometimes > 1)
            if rng.random() < 0.1:
                b[o + 0x1D] = 1
            if rng.random() < 0.1:
                b[o + 0x1E] = 1
            if rng.random() < 0.1:
                b[o + 0x20] = rng.integers(0, 255)       # poisons the neighbour's next-byte branch
            if rng.random() < 0.08:
                struct.pack_into("<f", b, o + 4, 0.0)    # zero scale word
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


def main():
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 4000
    rng = np.random.default_rng(0xC0FFEE)
    blobs = [make_blob(rng, i) for i in range(n)]
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
    print(f"{n - bad}/{n} bit-exact" + ("" if bad == 0 else f"  ({bad} MISMATCHES)"))
    for p in (inp, outp):
        os.remove(p)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())


def debug_blob(i, n=4000):
    """Regenerate blob i and dump both sides' components."""
    rng = np.random.default_rng(0xC0FFEE)
    blobs = [make_blob(rng, k) for k in range(n)]
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
