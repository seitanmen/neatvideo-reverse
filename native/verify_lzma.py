# Byte-exact verification of the native LZMA wrapper (native/nv_lzma.{h,cpp} over the LZMA SDK 9.20 C sources in
# native/lzma/) against the Python port repro/nv_lzma.py, and against the genuine's saved LZMA blobs.
#
#   make -C native -f Makefile.lzma && python3 native/verify_lzma.py [--quick] [--jobs J] [--no-genuine]
#
# (1) Python: synthetic and real inputs -> nv_lzma.plane_int16 (float32 planes), nv_lzma.compress (props + stream)
#     and nv_lzma.blob / blob_size (16-byte header + stream, or None = -1 when the stream does not fit
#     destLen = n + 0x80) against nv_lzma_test, byte for byte.  Includes planes with NaN / +-inf / huge values /
#     rint ties / the int16 saturation edges, dictionary-size boundaries (4 KiB multiples +-1), and inputs whose
#     stream length is searched to lie exactly around the output limit (n + 0x7e .. n + 0x82).
# (2) Genuine (no genuine run here; only blobs already saved by earlier `nvh profile` runs): every saved
#     *.lzma0/1/2 under neatvideo_ofx_614/analysis/noise and sandbox/ is decoded with Python's lzma module (an
#     independent decoder), recompressed by nv_lzma_test and compared byte for byte (bytes 13..15 of the header are
#     heap garbage in the genuine and are skipped).  For the saved crops whose float32 frame and area are known,
#     plane_int16 of the crop is also compared with the genuine's decoded plane.
# The pass criterion is byte equality only.  Temporary files: sandbox/omo/wp_native_lzma/tmp/.
import argparse
import glob
import hashlib
import lzma
import os
import struct
import subprocess
import sys
import time
from concurrent.futures import ProcessPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
REPRO = os.path.join(ROOT, "repro")
TMP = os.path.join(ROOT, "sandbox", "omo", "wp_native_lzma", "tmp")
EXE = os.environ.get("NV_LZMA_EXE", os.path.join(HERE, "nv_lzma_test"))   # override: sensitivity runs
sys.path.insert(0, REPRO)
sys.dont_write_bytecode = True

import numpy as np  # noqa: E402
import nv_lzma as L  # noqa: E402

F32 = np.float32


# ---------------------------------------------------------------- the C++ driver
def run_native(records, tag):
    """records: list of (kind, payload) with kind 0 = bytes, 1 = float32 array.  Returns per record
    (i16 or None, rc, props, stream, bsize, blob)."""
    os.makedirs(TMP, exist_ok=True)
    fin = os.path.join(TMP, f"{tag}_in.bin")
    fout = os.path.join(TMP, f"{tag}_out.bin")
    with open(fin, "wb") as f:
        f.write(struct.pack("<Q", len(records)))
        for kind, p in records:
            if kind == 1:
                a = np.ascontiguousarray(p, F32).ravel()
                f.write(struct.pack("<BQ", 1, a.size))
                f.write(a.astype("<f4").tobytes())
            else:
                f.write(struct.pack("<BQ", 0, len(p)))
                f.write(p)
    t = time.time()
    subprocess.run([EXE, fin, fout], check=True)
    dt = time.time() - t
    buf = open(fout, "rb").read()
    out, o = [], 0
    for kind, _ in records:
        i16 = None
        if kind == 1:
            (n16,) = struct.unpack_from("<Q", buf, o)
            o += 8
            i16 = buf[o:o + n16]
            o += n16
        rc, = struct.unpack_from("<i", buf, o)
        props = buf[o + 4:o + 9]
        (sl,) = struct.unpack_from("<Q", buf, o + 9)
        o += 17
        stream = buf[o:o + sl]
        o += sl
        (bs,) = struct.unpack_from("<q", buf, o)
        o += 8
        blob = buf[o:o + bs] if bs > 0 else None
        o += max(bs, 0)
        out.append((i16, rc, props, stream, bs, blob))
    assert o == len(buf), "trailing bytes in the driver output"
    for fn in (fin, fout):
        os.unlink(fn)
    return out, dt


# ---------------------------------------------------------------- the Python side
def py_expected(job):
    kind, p = job
    t = time.time()
    i16 = None
    if kind == 1:
        i16 = L.plane_int16(np.asarray(p, F32))
        src = i16
    else:
        src = bytes(p)
    b = L.blob(src)
    if b is None:
        props, stream = L.compress(src)
    else:
        props, stream = b[8:13], b[16:]
        # blob() is compress() plus the limit check, so its stream is compress()'s stream
    # blob_size() itself (= blob() again) only on small inputs; on the others its definition from blob()
    bs = L.blob_size(src) if len(src) <= 64 else (-1 if b is None else len(b))
    return i16, props, stream, bs, b, len(src), time.time() - t


# ---------------------------------------------------------------- inputs
def frame(path, n):
    """RGB float32 0..255 of frame n (rgb48le / 257, as quality_draft._frame), cached in TMP."""
    os.makedirs(TMP, exist_ok=True)
    cp = os.path.join(TMP, f"{os.path.splitext(os.path.basename(path))[0]}_{n}.npy")
    if os.path.exists(cp):
        return np.load(cp)
    probe = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
                            "stream=width,height", "-of", "csv=p=0", path], check=True, capture_output=True,
                           text=True).stdout.strip().split(",")
    W, H = int(probe[0]), int(probe[1])
    raw = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-i", path, "-vf", f"select=eq(n\\,{n})",
                          "-frames:v", "1", "-f", "rawvideo", "-pix_fmt", "rgb48le", "-"],
                         check=True, capture_output=True).stdout
    a = (np.frombuffer(raw, np.uint16).reshape(H, W, 3).astype(F32) / F32(257.0)).astype(F32)
    np.save(cp, a)
    return a


def edge_planes(rng):
    out = []
    f = F32
    vals = np.array([0.0, -0.0, np.nan, np.inf, -np.inf, 1e30, -1e30, 3.4e38, -3.4e38, 1e-45, -1e-45, 1e-38,
                     2047.9375, 2047.96875, 2048.0, 2048.03125, -2048.0, -2048.03125, -2048.0625, -2047.96875,
                     0.03125, -0.03125, 0.09375, -0.09375, 3.03125, -3.03125, 127.96875, 255.0, 255.96875,
                     0.0625, 1.0 / 3.0, -1.0 / 3.0, 2.5 / 16, -2.5 / 16, 4095.99, -4095.99, 65535.0],
                    np.float64).astype(f)
    out.append(("edge values list", vals))
    out.append(("edge values tiled 37x41", np.resize(vals, (37, 41))))
    nb = np.array([0x7fc00001, 0xffc00000, 0x7f800001, 0xff800001, 0x00000001, 0x80000001, 0x007fffff,
                   0x807fffff, 0x7f7fffff, 0xff7fffff, 0x45000000, 0xc5000000, 0x44fffff0, 0xc5000001,
                   0x3d000000, 0xbd000000], np.uint32).view(f)
    out.append(("edge bit patterns", np.resize(nb, (16, 16))))
    # every tie (k + 0.5) / 16 over a range: rint to even
    k = np.arange(-40000, 40000, 37, dtype=np.float64)[:2000]
    out.append(("rint ties", ((k + 0.5) / 16).astype(f).reshape(-1, 40)[:50]))
    out.append(("rint near ties", np.nextafter(((k + 0.5) / 16).astype(f), f(np.inf)).reshape(-1, 40)[:50]))
    # saturation sweep around +-2048
    s = np.linspace(-2049, 2049, 64 * 64).astype(f)
    out.append(("saturation sweep 64x64", s.reshape(64, 64)))
    a = (128 + 8 * rng.standard_normal((90, 100))).astype(f)
    a[rng.random(a.shape) < 0.05] = np.nan
    a[rng.random(a.shape) < 0.02] = np.inf
    a[rng.random(a.shape) < 0.02] = -np.inf
    a[rng.random(a.shape) < 0.02] = 1e20
    out.append(("noise with nan/inf/huge 90x100", a))
    out.append(("all nan 33x31", np.full((33, 31), np.nan, f)))
    out.append(("all +inf 40x40", np.full((40, 40), np.inf, f)))
    out.append(("all -0 40x40", np.full((40, 40), -0.0, f)))
    out.append(("denormals 48x48", (rng.standard_normal((48, 48)) * 1e-40).astype(f)))
    return out


def random_planes(rng, count, quick):
    """Planes of realistic and adversarial content; most small (Python is ~10 us per byte)."""
    out = []
    for i in range(count):
        r = rng.random()
        if r < 0.80:
            h, w = int(rng.integers(1, 97)), int(rng.integers(1, 97))
        elif r < 0.95:
            h, w = int(rng.integers(97, 161)), int(rng.integers(97, 161))
        else:
            h, w = (int(rng.integers(160, 257)), int(rng.integers(160, 257))) if not quick else (128, 128)
        kind = i % 9
        if kind == 0:     # realistic noisy flat area
            a = rng.uniform(5, 250) + rng.uniform(0.1, 40) * rng.standard_normal((h, w))
        elif kind == 1:   # integer-valued (rgb24 hosts)
            a = np.round(rng.uniform(5, 250) + rng.uniform(0.3, 10) * rng.standard_normal((h, w)))
        elif kind == 2:   # full int16 range and beyond (incompressible -> overflow for larger planes)
            a = rng.uniform(-2100, 2100, (h, w))
        elif kind == 3:   # gradient + noise
            yy, xx = np.mgrid[0:h, 0:w]
            a = rng.uniform(-2, 2) * xx + rng.uniform(-2, 2) * yy + rng.uniform(0, 100) + \
                rng.uniform(0, 3) * rng.standard_normal((h, w))
        elif kind == 4:   # constant
            a = np.full((h, w), rng.uniform(-3000, 3000))
        elif kind == 5:   # sparse spikes on a constant
            a = np.full((h, w), float(rng.integers(0, 256)))
            m = rng.random((h, w)) < rng.uniform(0.001, 0.1)
            a[m] = rng.uniform(-5000, 5000, m.sum())
        elif kind == 6:   # values on the 1/16 grid and ties
            a = (rng.integers(-40000, 40000, (h, w)) + rng.choice([0.0, 0.5, 0.25], (h, w))) / 16
        elif kind == 7:   # random walk (smooth)
            a = np.cumsum(rng.normal(0, 0.5, (h, w)), axis=1) + 128
        else:             # with NaN / inf
            a = 128 + 5 * rng.standard_normal((h, w))
            a[rng.random((h, w)) < 0.03] = np.nan
            a[rng.random((h, w)) < 0.01] = np.inf * rng.choice([-1, 1])
        with np.errstate(invalid="ignore", over="ignore"):
            out.append((f"rand {i} k{kind} {h}x{w}", np.asarray(a, np.float64).astype(F32)))
    return out


def byte_inputs(rng, quick):
    out = [("0 bytes", b"")]
    for n in list(range(1, 33)) + [47, 63, 64, 65, 127, 128, 129, 255, 256, 257]:
        out.append((f"random {n} bytes", rng.integers(0, 256, n, dtype=np.uint8).tobytes()))
        if n <= 8:
            out.append((f"zeros {n}", bytes(n)))
    for n in (4095, 4096, 4097, 8191, 8192, 8193, 12287, 12288, 12289, 16385, 20480, 24577, 32769, 49153):
        out.append((f"dict edge lowent {n}", rng.integers(0, 4, n, dtype=np.uint8).tobytes()))
    out += [("zeros 40000", bytes(40000)), ("ff 305", b"\xff" * 305), ("ab 307", b"ab" * 153 + b"a"),
            ("text", b"the quick brown fox jumps over the lazy dog; " * 300)]
    for per in (3, 7, 64, 200, 1000):
        base = rng.integers(0, 256, per, dtype=np.uint8).tobytes()
        out.append((f"period {per}", (base * (20000 // per + 1))[:20000]))
    blk = rng.integers(0, 256, 6000, dtype=np.uint8)
    a = np.concatenate([blk, rng.integers(0, 256, 9000, dtype=np.uint8), blk, blk[::-1], blk])
    m = rng.random(a.size) < 0.01
    a[m] = rng.integers(0, 256, m.sum())
    out.append(("far repeats", a.astype(np.uint8).tobytes()))
    parts = [bytes([k & 0xff, (k * 7) & 0xff]) + b"XYZW"[k % 4:k % 4 + 1] * (2 + k % 5) for k in range(2000)]
    out.append(("rep mix", b"".join(parts)))
    out.append(("runs", b"".join(bytes([v]) * int(n) for v, n in
                                 zip(rng.integers(0, 256, 300), rng.integers(1, 600, 300)))))
    for n in (2000, 9000, 20000) if quick else (2000, 9000, 20000, 40000):
        out.append((f"random bytes {n} (overflow)", rng.integers(0, 256, n, dtype=np.uint8).tobytes()))
    return out


def limit_search(rng, want):
    """Inputs whose stream length is n + 0x7e .. n + 0x82 (around the FUN_009d61f4 limit n + 0x80), found with the
    C++ driver on a family 'r random bytes + t low-entropy bytes'; the Python check is then run on them."""
    found = {}
    rounds = 0
    while rounds < 12 and any(found.get(d, 0) < want for d in range(0x7e, 0x83)):
        rounds += 1
        cands = []
        for _ in range(400):
            r = int(rng.integers(8500, 13000))     # random bytes cost about n / 80 + 12 over n
            t = int(rng.integers(0, 400))
            alph = int(rng.integers(64, 257))
            b = rng.integers(0, 256, r, dtype=np.uint8).tobytes() + \
                rng.integers(0, alph, t, dtype=np.uint8).tobytes()
            cands.append(b)
        res, _ = run_native([(0, b) for b in cands], "search")
        for b, (_, rc, _, stream, _, _) in zip(cands, res):
            d = len(stream) - len(b)
            if rc == 0 and 0x7e <= d <= 0x82 and found.get(d, 0) < want:
                found[d] = found.get(d, 0) + 1
                yield (f"limit search n{len(b)} stream n+{d:#x}", b)


def real_planes(rng, quick):
    out = []
    specs = [(os.path.join(ROOT, "test_v02.mp4"), n) for n in ((312,) if quick else (0, 312, 576))] + \
            [(os.path.join(ROOT, "test.mp4"), n) for n in ((300,) if quick else (0, 300))]
    sizes = ((48, 48), (128, 128)) if quick else ((32, 32), (48, 48), (64, 64), (97, 71), (128, 128), (200, 150))
    for path, n in specs:
        fr = frame(path, n)
        H, W = fr.shape[:2]
        for (w, h) in sizes:
            x, y = int(rng.integers(0, W - w + 1)), int(rng.integers(0, H - h + 1))
            chans = (0, 1, 2) if (w, h) == (128, 128) else (int(rng.integers(0, 3)),)
            for c in chans:
                out.append((f"{os.path.basename(path)}:{n} {w}x{h}@{x},{y} ch{c}",
                            np.ascontiguousarray(fr[y:y + h, x:x + w, c])))
    return out


# ---------------------------------------------------------------- (1) against Python
def check_python(a):
    rng = np.random.default_rng(20261026)
    cases = [(nm, 1, p) for nm, p in edge_planes(rng)]
    cases += [(nm, 1, p) for nm, p in random_planes(rng, 120 if a.quick else 520, a.quick)]
    cases += [(nm, 0, p) for nm, p in byte_inputs(rng, a.quick)]
    cases += [(nm, 0, p) for nm, p in limit_search(rng, 1 if a.quick else 3)]
    cases += [(nm, 1, p) for nm, p in real_planes(rng, a.quick)]
    if not a.quick:
        cases.append(("white 512x512", 1, (128 + 5 * rng.standard_normal((512, 512))).astype(F32)))
    nbytes = sum(p.size * 2 if k == 1 else len(p) for _, k, p in cases)
    print(f"[python] {len(cases)} inputs ({nbytes} source bytes); planes {sum(k == 1 for _, k, _ in cases)}, "
          f"byte strings {sum(k == 0 for _, k, _ in cases)}", flush=True)
    native, dt_c = run_native([(k, p) for _, k, p in cases], "py")
    t = time.time()
    jobs = [(k, p) for _, k, p in cases]
    order = sorted(range(len(jobs)), key=lambda i: -(jobs[i][1].size * 2 if jobs[i][0] == 1 else len(jobs[i][1])))
    exp = [None] * len(jobs)
    with ProcessPoolExecutor(min(a.jobs, 4)) as ex:
        for i, r in zip(order, ex.map(py_expected, [jobs[i] for i in order], chunksize=1)):
            exp[i] = r
    dt_py = time.time() - t
    bad = 0
    n_over = n_fit = 0
    known = []
    limit = []
    py_cpu = 0.0
    for (nm, kind, _), (i16, rc, props, stream, bs, blob), e in zip(cases, native, exp):
        ei16, eprops, estream, ebs, eblob, n, dtp = e
        py_cpu += dtp
        errs = []
        if kind == 1 and i16 != ei16:
            d = next(j for j in range(min(len(i16), len(ei16))) if i16[j] != ei16[j]) if len(i16) == len(ei16) else -1
            errs.append(f"plane_int16 differs (first byte {d})")
        if rc != 0:
            errs.append(f"compress rc {rc}")
        if props != eprops:
            errs.append(f"props {props.hex()} != {eprops.hex()}")
        if stream != estream:
            errs.append(f"stream {len(stream)} != {len(estream)} bytes")
        if bs != ebs or blob != eblob:
            errs.append(f"blob {bs} != {ebs}" if bs != ebs else "blob bytes differ")
        if eblob is None:
            n_over += 1
        else:
            n_fit += 1
        if nm.startswith("limit search"):
            limit.append((len(estream) - n, eblob is not None))
        if errs:
            # n == 0: dict_size_for(0) = 0, which the SDK's LzmaEncProps_Normalize turns into 1 << 24 (props
            # 5d 00 00 00 01) while nv_lzma.props_bytes writes 4 KiB (5d 00 10 00 00); the stream is the same.
            if n == 0 and stream == estream and props.hex() == "5d00000001" and eprops.hex() == "5d00100000":
                known.append((nm, errs))
                continue
            bad += 1
            print(f"  MISMATCH {nm}: {'; '.join(errs)}", flush=True)
        elif a.verbose:
            print(f"  ok {nm}: {n} -> {len(stream)} blob {bs}")
    print(f"[python] byte equal: {len(cases) - bad - len(known)}/{len(cases) - len(known)} "
          f"(blob fits {n_fit}, overflow -1 {n_over}); C++ {dt_c:.2f} s, Python {dt_py:.1f} s wall "
          f"({py_cpu / max(nbytes, 1) * 1e6:.1f} us/byte over {min(a.jobs, 4)} workers)", flush=True)
    print(f"[python] around the output limit n + 0x80 (stream length - n, fits): "
          f"{sorted(limit)}", flush=True)
    for nm, errs in known:
        print(f"  known divergence (empty input, not a genuine case; REPORT.md): {nm}: {'; '.join(errs)}")
    return bad, len(cases) - len(known)


# ---------------------------------------------------------------- (2) against the genuine's saved blobs
def genuine_blobs():
    pats = [os.path.join(ROOT, "neatvideo_ofx_614", "analysis", "noise", "**", "*.lzma[012]"),
            os.path.join(ROOT, "sandbox", "**", "*.lzma[012]")]
    files = sorted({f for p in pats for f in glob.glob(p, recursive=True)
                    if not f.startswith(TMP)})
    seen, out = {}, []
    for f in files:
        b = open(f, "rb").read()
        k = hashlib.sha256(b[:13] + b[16:]).hexdigest()
        if k in seen:
            continue
        seen[k] = f
        out.append((f, b))
    return files, out


def decode(b):
    n, sl = struct.unpack_from("<II", b, 0)
    assert len(b) == 16 + sl, "blob length != 16 + stream length"
    alone = b[8:13] + struct.pack("<Q", n) + b[16:]
    src = lzma.decompress(alone, format=lzma.FORMAT_ALONE)
    assert len(src) == n
    return src


def known_crops():
    """Saved float32 frames with the area given to `nvh profile` (from the run logs / scripts)."""
    out = []
    f100 = os.path.join(ROOT, "sandbox", "omo", "coord", "verify_lzma", "nvh", "f100.rgb.f32")
    for S in (32, 45, 46):
        out.append((f100, 700, 200, S, S, os.path.join(os.path.dirname(f100), f"p{S}.bin")))
    f312 = os.path.join(ROOT, "sandbox", "omo", "wp_lzma", "data", "boundary", "frame312.rgb.f32")
    for S in (32, 45, 46, 64, 512):
        out.append((f312, 830, 300, S, S, os.path.join(os.path.dirname(f312), f"prof_r{S}.bin")))
    return [c for c in out if os.path.exists(c[0]) and os.path.exists(c[5] + ".lzma0")]


def check_genuine(a):
    files, blobs = genuine_blobs()
    if not blobs:
        print("[genuine] no saved blobs found; skipped")
        return 0, 0
    srcs = [decode(b) for _, b in blobs]
    print(f"[genuine] {len(files)} saved blob files, {len(blobs)} distinct "
          f"({sum(len(s) for s in srcs)} plane bytes, {min(len(s) for s in srcs)}..{max(len(s) for s in srcs)})",
          flush=True)
    native, dt = run_native([(0, s) for s in srcs], "gen")
    bad = 0
    for (f, b), (_, rc, props, stream, bs, blob) in zip(blobs, native):
        ok = blob is not None and len(blob) == len(b) and blob[:13] == b[:13] and blob[16:] == b[16:]
        if not ok:
            bad += 1
            print(f"  MISMATCH {os.path.relpath(f, ROOT)}: native {bs} genuine {len(b)}", flush=True)
    print(f"[genuine] blob byte equal (bytes 13..15 skipped): {len(blobs) - bad}/{len(blobs)}; C++ {dt:.2f} s",
          flush=True)
    # end to end: plane_int16 of the known crops == the genuine's decoded plane, and the blob
    crops = known_crops()
    recs, want = [], []
    for fr, x, y, w, h, prof in crops:
        img = np.fromfile(fr, F32).reshape(3, 1080, 1920)
        for c in range(3):
            recs.append((1, np.ascontiguousarray(img[c, y:y + h, x:x + w])))
            want.append((f"{os.path.relpath(prof, ROOT)}.lzma{c}", open(f"{prof}.lzma{c}", "rb").read()))
    bad2 = 0
    if recs:
        nat, _ = run_native(recs, "gencrop")
        for (nm, b), (i16, rc, props, stream, bs, blob) in zip(want, nat):
            ok = i16 == decode(b) and blob is not None and blob[:13] == b[:13] and blob[16:] == b[16:]
            if not ok:
                bad2 += 1
                print(f"  MISMATCH crop {nm}", flush=True)
        print(f"[genuine] plane_int16 + blob of known crops: {len(recs) - bad2}/{len(recs)} "
              f"({len(crops)} areas x 3 planes)", flush=True)
    return bad + bad2, len(blobs) + len(recs)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--no-genuine", action="store_true")
    ap.add_argument("--verbose", action="store_true")
    a = ap.parse_args()
    if not os.path.exists(EXE):
        sys.exit(f"{EXE} missing: make -C native -f Makefile.lzma")
    t = time.time()
    bad, n = check_python(a)
    if not a.no_genuine:
        b2, n2 = check_genuine(a)
        bad += b2
        n += n2
    print(f"TOTAL: {n - bad}/{n} byte equal, {bad} mismatches ({time.time() - t:.1f} s)")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
