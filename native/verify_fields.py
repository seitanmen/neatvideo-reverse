# Verification of nv_fields (INT-1: the field split / join of interlaced frames), byte for byte.
#
#   make -C native nv_fields_test && python3 native/verify_fields.py [--video CLIP[:N] ...] [--make-clips DIR]
#                                                                     [--sensitivity] [--tmp DIR]
#
# Always: nv_fields_test selftest (the C++ checks), then random frames of every format (uint8 / uint16 / float32
# with random bit patterns, 4:2:0 and 4:4:4, odd and even sizes down to 1 x 1) through `nv_fields_test raw`: each
# field against numpy's plane[0::2] / plane[1::2] (+ the pad row of an odd height: plane[H-2], plane[0] for H == 1),
# and the joined frame against the input.
# --video: the same for the decoded frames of real clips (yuv420p, yuv420p10le, yuv444p, gbrpf32le), plus for
# yuv420p the fields of ffmpeg's separatefields filter (in the order of the first frame's top_field_first, or of
# the stream's tt / bb when the frame is not flagged; the stream's tb / bt do not give the order: ffmpeg 8.0.1 writes
# tb for top-field-first ffv1 / prores) and ffprobe's field_order.  When the chroma height is odd (H = 2 mod 4) the
# bottom field's last chroma row is the pad row (its own last row), where separatefields reads past the plane: that
# row is left out of the separatefields comparison and counted.
# --make-clips DIR: interlaced clips from test_v02.mp4 / test.mp4 (tinterlace, lossless x264 with interlaced coding),
# then --video on them, plus each field against the source frame it was made from.
# --sensitivity: rebuilds nv_fields_test with each -DNVF_MUT_* (into --tmp) and expects every one to be detected (the
# selftest fails or the numpy comparison mismatches; NVF_MUT_NOALIAS, which accepts shared memory, only by the selftest).
import argparse
import json
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
EXE = os.path.join(HERE, "nv_fields_test")
sys.dont_write_bytecode = True

import numpy as np  # noqa: E402

FMTS = {"u8_420": (np.uint8, True), "u16_420": (np.uint16, True), "f32_420": (np.float32, True),
        "u8_444": (np.uint8, False), "u16_444": (np.uint16, False), "f32_444": (np.float32, False)}
PIX = {"yuv420p": "u8_420", "yuv420p10le": "u16_420", "yuv444p": "u8_444", "gbrpf32le": "f32_444"}
MUTANTS = ("NVF_MUT_NARROW", "NVF_MUT_PADWRITE", "NVF_MUT_PADROW", "NVF_MUT_CHROMA", "NVF_MUT_NOALIAS")


def shapes(fmt, W, H):
    c420 = FMTS[fmt][1]
    cw, ch = ((W + 1) // 2, (H + 1) // 2) if c420 else (W, H)
    return [(H, W), (ch, cw), (ch, cw)]


def field(p, parity):
    """the field of plane p (numpy slicing; the pad row of an odd height written out as the rule)"""
    H = p.shape[0]
    f = p[parity::2]
    if f.shape[0] < (H + 1) // 2:
        f = np.concatenate([f, p[[H - 2 if H >= 3 else 0]]])
    return f


def frames_of(buf, fmt, W, H):
    dt = FMTS[fmt][0]
    a = np.frombuffer(buf, dt)
    out, o = [], 0
    while o < a.size:
        fr = []
        for (h, w) in shapes(fmt, W, H):
            fr.append(a[o:o + h * w].reshape(h, w))
            o += h * w
        out.append(fr)
    return out


def field_frames_of(buf, fmt, W, H):
    fs = [((h + 1) // 2, w) for (h, w) in shapes(fmt, W, H)]
    dt = FMTS[fmt][0]
    a = np.frombuffer(buf, dt)
    out, o = [], 0
    while o < a.size:
        fr = []
        for (h, w) in fs:
            fr.append(a[o:o + h * w].reshape(h, w))
            o += h * w
        out.append(fr)
    return out


def same(a, b):
    return a.shape == b.shape and a.tobytes() == b.tobytes()


def run_raw(exe, fmt, W, H, raw, tmp, tag):
    """nv_fields_test raw on the frames in raw: (top, bottom, joined) bytes, the C++ summary, the exit code"""
    n = len(raw) // (sum(h * w for (h, w) in shapes(fmt, W, H)) * np.dtype(FMTS[fmt][0]).itemsize)
    p = [os.path.join(tmp, f"{tag}.{k}") for k in ("in", "top", "bot", "join")]
    with open(p[0], "wb") as fh:
        fh.write(raw)
    r = subprocess.run([exe, "raw", fmt, str(W), str(H), str(n)] + p, capture_output=True, text=True)
    outs = []
    for q in p[1:]:
        with open(q, "rb") as fh:
            outs.append(fh.read())
    for q in p:
        os.remove(q)
    return outs, r.stdout.strip() + r.stderr.strip(), r.returncode, n


def compare(fmt, W, H, raw, outs):
    """(frames, fields) that match, the frame count, the bad items"""
    src = frames_of(raw, fmt, W, H)
    top, bot = field_frames_of(outs[0], fmt, W, H), field_frames_of(outs[1], fmt, W, H)
    joined = frames_of(outs[2], fmt, W, H)
    bad = []
    if not (len(top) == len(bot) == len(joined) == len(src)):
        return ["frame counts differ"]
    for i, fr in enumerate(src):
        for k in range(3):
            if not same(top[i][k], field(fr[k], 0)):
                bad.append(f"frame {i} plane {k} top")
            if not same(bot[i][k], field(fr[k], 1)):
                bad.append(f"frame {i} plane {k} bottom")
            if not same(joined[i][k], fr[k]):
                bad.append(f"frame {i} plane {k} joined")
    return bad


def random_raw(rng, fmt, W, H, n):
    dt = FMTS[fmt][0]
    size = n * sum(h * w for (h, w) in shapes(fmt, W, H))
    if dt == np.float32:     # random bit patterns, with NaN payloads, +-0, +-inf and denormals mixed in
        u = rng.integers(0, 2 ** 32, size, dtype=np.uint64).astype(np.uint32)
        sp = np.array([0x7fc00000, 0xffc00000, 0x7fc12345, 0x7f800001, 0xff812345, 0x80000000, 0, 0x7f800000,
                       0xff800000, 0x00000001, 0x807fffff], np.uint32)
        m = rng.random(size) < 0.2
        u[m] = sp[rng.integers(0, sp.size, int(m.sum()))]
        return u.tobytes()
    return rng.integers(0, np.iinfo(dt).max + 1, size, dtype=np.uint64).astype(dt).tobytes()


def synthetic(exe, tmp):
    rng = np.random.default_rng(20261031)
    sizes = [(1, 1), (2, 1), (1, 2), (3, 2), (2, 3), (5, 3), (4, 4), (16, 9), (17, 33), (64, 1), (1, 64), (7, 128),
             (640, 361), (641, 360), (1920, 1080), (720, 486)]
    ok = total = 0
    for fmt in FMTS:
        for (W, H) in sizes:
            n = 1 if W * H > 200000 else 3
            raw = random_raw(rng, fmt, W, H, n)
            outs, msg, code, nn = run_raw(exe, fmt, W, H, raw, tmp, "syn")
            bad = compare(fmt, W, H, raw, outs)
            total += 1
            if code == 0 and not bad:
                ok += 1
            else:
                print(f"  MISMATCH {fmt} {W}x{H}: code {code} {msg} {bad[:4]}")
    print(f"synthetic frames (numpy fields + round trip): {ok}/{total} cases")
    return ok == total


def probe(path):
    r = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
                        "stream=width,height,field_order,pix_fmt", "-of", "json", path],
                       capture_output=True, text=True, check=True)
    s = json.loads(r.stdout)["streams"][0]
    r = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-read_intervals", "%+#1", "-show_entries",
                        "frame=interlaced_frame,top_field_first", "-of", "json", path],
                       capture_output=True, text=True, check=True)
    fr = json.loads(r.stdout).get("frames", [])
    f0 = fr[0] if fr else {}
    return (s["width"], s["height"], s.get("field_order", ""), f0.get("interlaced_frame", -1), f0.get("top_field_first", -1),
            s.get("pix_fmt", ""))


def first_top_of(fo, fi, tff):
    """True / False: the top / bottom field is the first in time; None: not known (qtui video.cpp scan_of)"""
    if fi == 1:
        return tff == 1
    return {"tt": True, "bb": False}.get(fo)


def decode(path, pix, n, extra_vf=None):
    vf = ["-vf", extra_vf] if extra_vf else []
    r = subprocess.run(["ffmpeg", "-v", "error", "-nostdin", "-i", path, "-frames:v", str(n)] + vf +
                       ["-f", "rawvideo", "-pix_fmt", pix, "-"], capture_output=True, check=True)
    return r.stdout


def video(exe, path, n, tmp):
    W, H, fo, fi, tff, spix = probe(path)
    first_top = first_top_of(fo, fi, tff)
    print(f"{os.path.basename(path)}: {W}x{H}, ffprobe field_order = {fo or '(none)'}, first frame interlaced_frame "
          f"{fi} top_field_first {tff} -> first field {'top' if first_top else 'bottom' if first_top is False else '?'}")
    ok = True
    for pix, fmt in PIX.items():
        raw = decode(path, pix, n)
        outs, msg, code, nn = run_raw(exe, fmt, W, H, raw, tmp, "vid")
        bad = compare(fmt, W, H, raw, outs)
        good = code == 0 and not bad
        ok &= good
        print(f"  {pix:12s} {nn} frames: fields == numpy and join == frame: {'yes' if good else 'NO ' + str(bad[:4])} ({msg})")
        if pix == "yuv420p" and first_top is not None and spix != "yuv420p":
            # the conversion to yuv420p (swscale, not field-aware) mixes the fields' chroma rows of a frame, but not
            # of a field picture: the two decodes differ by the conversion, not by the split
            print(f"  separatefields: skipped (source {spix}: the yuv420p conversion is not field-aware)")
        elif pix == "yuv420p" and first_top is not None:
            # ffmpeg's separatefields: each frame -> its two fields as frames, the first in time first
            sep = decode(path, "yuv420p", 2 * n, "separatefields")
            sf = field_frames_of(sep, fmt, W, H)
            top, bot = field_frames_of(outs[0], fmt, W, H), field_frames_of(outs[1], fmt, W, H)
            ch = (H + 1) // 2
            pad = ch % 2 == 1          # the bottom field's last chroma row is the pad row
            m = skipped = 0
            for i in range(nn):
                a, b = (top[i], bot[i]) if first_top else (bot[i], top[i])
                eq = True
                for k in range(3):
                    for f, (x, y) in ((0, (sf[2 * i][k], a[k])), (1, (sf[2 * i + 1][k], b[k]))):
                        is_bot = (f == 0) != first_top
                        if pad and k and is_bot:
                            x, y = x[:-1], y[:-1]
                            skipped += 1
                        eq &= same(x, y)
                m += eq
            good = len(sf) == 2 * nn and m == nn
            ok &= good
            print(f"  separatefields ({'top' if first_top else 'bottom'} first): {m}/{nn} frames, all 3 planes "
                  f"byte-equal to split_frame" + (f" (bottom pad chroma row left out: {skipped} planes)" if pad else ""))
    return ok, (W, H, fo)


def make_clips(exe, out, n):
    """interlaced clips by tinterlace + lossless interlaced x264, and the check of their fields against the sources"""
    os.makedirs(out, exist_ok=True)
    # test_v02 (1080p24) is read as 60p (setpts) and interlaced to 1080i30; the x264 field flag comes from
    # -x264-params tff / bff (ffmpeg 8.0.1: -top and setfield alone leave x264 at tt)
    clips = [  # name, source, vf before tinterlace, tinterlace mode, field flag, expected field_order
        ("v02_1080i30_tt.mp4", "test_v02.mp4", "setpts=N/(60*TB),fps=60", "interleave_top", "tff", "tt"),
        ("v02_1080i30_bb.mp4", "test_v02.mp4", "setpts=N/(60*TB),fps=60", "interleave_bottom", "bff", "bb"),
        ("t_360i15_bb.mp4", "test.mp4", None, "interleave_bottom", "bff", "bb"),
    ]
    ok = True
    for name, src, pre, mode, top, want in clips:
        dst = os.path.join(out, name)
        vf = ",".join(([pre] if pre else []) + [f"tinterlace=mode={mode}", f"setfield={top}"])
        if not os.path.exists(dst):
            subprocess.run(["ffmpeg", "-y", "-v", "error", "-nostdin", "-i", os.path.join(ROOT, src), "-frames:v",
                            str(n), "-vf", vf, "-c:v", "libx264", "-qp", "0", "-preset", "ultrafast", "-pix_fmt", "yuv420p",
                            "-flags", "+ilme+ildct", "-x264-params", f"{top}=1", dst], check=True)
        g, (W, H, fo) = video(exe, dst, n, out)
        ok &= g and fo == want
        if fo != want:
            print(f"  field_order {fo} (expected {want})")
        # each field of the decoded clip is the field of the source frame it came from (lossless coding):
        # interleave_top: frame i = top of source 2i, bottom of source 2i + 1; interleave_bottom the other way
        srcraw = decode(os.path.join(ROOT, src), "yuv420p", 2 * n, pre)
        sfr = frames_of(srcraw, "u8_420", W, H)
        raw = decode(dst, "yuv420p", n)
        outs, msg, code, nn = run_raw(exe, "u8_420", W, H, raw, out, "src")
        tops, bots = field_frames_of(outs[0], "u8_420", W, H), field_frames_of(outs[1], "u8_420", W, H)
        m = 0
        for i in range(nn):
            ta, ba = (2 * i, 2 * i + 1) if mode == "interleave_top" else (2 * i + 1, 2 * i)
            m += all(same(tops[i][k], field(sfr[ta][k], 0)) and same(bots[i][k], field(sfr[ba][k], 1)) for k in range(3))
        ok &= m == nn and code == 0
        print(f"  fields == the source frames' fields ({mode}): {m}/{nn} frames, all 3 planes")
    return ok


def sensitivity(tmp):
    ok = True
    src = [os.path.join(HERE, "nv_fields_main.cpp"), os.path.join(HERE, "nv_fields.cpp")]
    for m in MUTANTS:
        exe = os.path.join(tmp, f"nv_fields_mut_{m}")
        subprocess.run(["clang++", "-std=c++17", "-O2", "-ffp-contract=off", "-fno-fast-math", f"-D{m}", "-o", exe] + src,
                       check=True)
        st = subprocess.run([exe, "selftest"], capture_output=True, text=True)
        with tempfile.TemporaryDirectory(dir=tmp) as t:
            with open(os.devnull, "w") as dn:
                old = sys.stdout
                sys.stdout = dn
                try:
                    syn = synthetic(exe, t)
                finally:
                    sys.stdout = old
        det = st.returncode != 0 or not syn          # NOALIAS changes only the refusals: the selftest sees it
        ok &= det
        print(f"  {m:18s} selftest: {st.stdout.strip()}; numpy comparison: {'mismatch' if not syn else 'all match'} -> "
              f"{'DETECTED' if det else 'NOT DETECTED'}")
    return ok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--video", action="append", default=[], help="CLIP[:N] (N frames, default 8)")
    ap.add_argument("--make-clips", default="")
    ap.add_argument("--frames", type=int, default=8)
    ap.add_argument("--sensitivity", action="store_true")
    ap.add_argument("--tmp", default="")
    A = ap.parse_args()
    if not os.path.exists(EXE):
        print(f"{EXE} missing: make -C native nv_fields_test")
        return 2
    with tempfile.TemporaryDirectory(dir=A.tmp or None) as tmp:
        st = subprocess.run([EXE, "selftest"], capture_output=True, text=True)
        print(f"nv_fields_test selftest: {st.stdout.strip()}{(' ' + st.stderr.strip()) if st.returncode else ''}")
        ok = st.returncode == 0
        ok &= synthetic(EXE, tmp)
        for v in A.video:
            path, _, n = v.partition(":")
            ok &= video(EXE, path, int(n) if n else A.frames, tmp)[0]
        if A.make_clips:
            ok &= make_clips(EXE, A.make_clips, A.frames)
        if A.sensitivity:
            print("negative controls (each mutant must fail):")
            ok &= sensitivity(A.tmp or tmp)
    print("ALL BYTE-EXACT" if ok else "FAILURES")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
