#!/usr/bin/env python3
"""Task A: the 27 curve bins right after BuildNoiseModel (before AutoFineTune) and their g / Completeness / Quality.

    python3 pre_aft.py [CAPTURE_DIR]      (default: the main checkout's work/omo/wp_genuine_capture)

Profiles are built with reference/python/nv_profile.build_profile (bit-exact with the genuine BuildNoiseModel in the
harness, AGENTS.md section 6.5; nothing of the genuine is run here):
  1. the 128 x 128 noise sample stored in pa1_s1_autoprofile.dnp (int16 rint(x*16) / 16, decoded by tools/dnp.py),
     colour mode 1; its six model faces are compared with the faces in the genuine file
  2. test_v02.mp4 frame 312, the area FindFlatArea picks (x 832, y 0, 128 x 128; AGENTS.md section 6.5)
  3. the same frame, full frame (1920 x 1080): only the words Quality reads (faces 3..5 words 0..54 by
     nv_band.model_levels, as quality._curve_blob builds them; the full build is too slow in Python), the curve as
     nv_profile writes it
For each: the bin table (valid +0x18, marker +0x1c, clip +0x1d, >3.0 +0x1e) and the corrected g (quality.genuine_g),
Completeness and Quality (quality.quality_from_blob).
"""
import os
import struct
import subprocess
import sys
import tempfile

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "reference", "python"))
sys.dont_write_bytecode = True
import nv_profile as npf  # noqa: E402
from nv_ui_qt import quality as Q  # noqa: E402

F32 = np.float32
CAP = sys.argv[1] if len(sys.argv) > 1 else "/Users/seitan/Desktop/work/reverse/work/omo/wp_genuine_capture"
MODEL = 0x1F54


def table(blob):
    rows = []
    for c in range(3):
        for k in range(9):
            o = 0xBF44 + 0x120 * c + 0x20 * k
            lvl = struct.unpack_from("<f", blob, o)[0]
            sc = struct.unpack_from("<5f", blob, o + 4)
            valid = struct.unpack_from("<I", blob, o + 0x18)[0]
            if (lvl, sc, valid, bytes(blob[o + 0x1C:o + 0x20])) != (1.0, (1.0,) * 5, 0, b"\0\0\0\0"):
                rows.append(f"    ch{c} bin{k}: level {lvl} scales {sc} valid {valid} "
                            f"marker {blob[o + 0x1c]} clip {blob[o + 0x1d]} gt3 {blob[o + 0x1e]} b1f {blob[o + 0x1f]}")
    return rows


def report(label, blob):
    g = Q.genuine_g(blob)
    q = Q.quality_from_blob(blob)
    print(f"{label}")
    print("    all other bins: level 1.0, scales 1.0, valid 0, marker/clip/gt3/+0x1f 0")
    print("\n".join(table(blob)))
    print(f"    g = {float(g)!r} (x81 = {float(g) * 81:.4f}), Completeness {int(np.floor(float(g) * 100 + 0.5))}%, "
          f"Quality {int(np.floor(float(q) * 100 + 0.5))}% (q = {float(q)!r}); W,H = {struct.unpack_from('<ii', blob, 0xc2c0)}, "
          f"clips {list(blob[0xbf3c:0xbf3f])}")


def frame(path, n):
    vf = f"select=eq(n\\,{n})"
    raw = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-i", path, "-vf", vf, "-frames:v", "1",
                          "-f", "rawvideo", "-pix_fmt", "rgb48le", "-"], check=True, capture_output=True).stdout
    return (np.frombuffer(raw, np.uint16).reshape(1080, 1920, 3).astype(F32) / F32(257.0)).astype(F32)


def main():
    with tempfile.TemporaryDirectory() as tmp:
        dnp = os.path.join(CAP, "dnp", "pa1", "pa1_s1_autoprofile.dnp")
        subprocess.run([sys.executable, "-I", os.path.join(CAP, "tools", "dnp.py"), dnp, "--samples", tmp,
                        "--raw", os.path.join(tmp, "img.bin")], check=True, capture_output=True)
        img = open(os.path.join(tmp, "img.bin"), "rb").read()
        pl = [np.load(os.path.join(tmp, f"sample_{c}.npy")) for c in "RGB"]
    rgb = np.stack([p.astype(F32) / F32(16.0) for p in pl], axis=-1).astype(F32)
    h, w = rgb.shape[:2]
    blob, _ = npf.build_profile(rgb, 1, (0, 0, w, h), with_floor=False, lzma=False)
    blob = bytes(blob)
    same = sum(blob[0x4C + i * MODEL:0x4C + (i + 1) * MODEL] == img[0x4C + i * MODEL:0x4C + (i + 1) * MODEL]
               for i in range(6))
    print(f"pa1_s1 sample {w}x{h}: model faces equal to the genuine file's: {same}/6; "
          f"means bf28 {np.frombuffer(blob[0xbf28:0xbf34], F32).tolist()} (file "
          f"{np.frombuffer(img[0xbf28:0xbf34], F32).tolist()})")
    report("1. pa1_s1 sample, fresh build", blob)
    print(f"   (the genuine file after AFT: Quality / Completeness "
          f"{int(np.floor(float(Q.quality_from_blob(img[:0xc300])) * 100 + 0.5))}% / "
          f"{int(np.floor(float(Q.genuine_g(img[:0xc300])) * 100 + 0.5))}%)")
    rgb = frame(os.path.join(ROOT, "test_v02.mp4"), 312)
    blob, _ = npf.build_profile(rgb, 1, (832, 0, 128, 128), with_floor=False, lzma=False)
    report("2. test_v02 n=312, x832 y0 128x128, fresh build", bytes(blob))
    report("3. test_v02 n=312, full frame, fresh build (Quality words only)", fast_blob(rgb))


def fast_blob(rgb):
    import nv_band as nb
    import nv_curve as ncv
    import nv_flat as nf
    R, G, B = (np.ascontiguousarray(rgb[..., c]) for c in range(3))
    h, w = R.shape
    buf = bytearray(0xC300)
    w32 = np.frombuffer(buf, np.uint32)
    f32 = np.frombuffer(buf, F32)
    w32[0x48 // 4] = 1
    for i, p in enumerate(nf.transform(R, G, B, 1)):
        off = (0x4C + (3 + i) * MODEL) // 4
        w32[off:off + 55] = np.asarray(nb.model_levels(np.ascontiguousarray(p, F32))).view(np.uint32)[:55]
    buf[0xBF34] = 1
    w32[0xBF38 // 4] = npf.size_class(min(w, h))
    flags = [bool(ncv.is_clip_genuine(p) > F32(0.33)) for p in (R, G, B)]
    buf[0xBF3C:0xBF3F] = bytes(flags)
    for c, p in enumerate((R, G, B)):              # nv_profile.build_profile's curve
        for k in range(9):
            e = 0xBF44 + 0x120 * c + 0x20 * k
            f32[e // 4:e // 4 + 6] = F32(1.0)
        e = 0xBF44 + 0x120 * c + 0x20 * npf.curve_bin(nb.mean_exact(p))
        w32[(e + 0x18) // 4] = 1
        buf[e + 0x1D] = flags[c]
    buf[0xC2A4] = 1
    struct.pack_into("<ii", buf, 0xC2C0, w, h)
    return bytes(buf)


if __name__ == "__main__":
    main()
