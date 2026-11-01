# Bit-exact verification of the field pipeline (INT-2: native/nv_fieldproc.h, nvt::temporal_v5's shift,
# nvc_temporal_shift) against the Python reference native/fields_ref.py (repro's nvfilter v5 functions on fields).
#
#   make -C native nv_v5_test lib && python3 native/verify_fieldproc.py [--quick] [--no-real] [--no-bridge]
#                                                                       [--sensitivity] [--jobs N]
#
# Groups (every output word compared as uint32):
#   copy   : fields_ref.temporal_shift without shifts and with all-zero shifts == nvfilter.temporal_v5 (Python only:
#            the reference is temporal_v5 itself where no offset is given)
#   tshift : nv_v5_test kind 30 (temporal_v5 with shifts) == fields_ref.temporal_shift on random windows (4:2:0 /
#            4:4:4, offsets 0 / +-0.5 / other values on luma and chroma, given flows, similarity v5 / pyramid);
#            out, rv and the block vectors; and kind 30 with zero shifts == kind 2 (temporal_v5) word for word
#   fields : kind 31 (nvfp::process_frame_fields) == fields_ref.process_frame_fields on random interlaced clips
#            (heights odd / 2 mod 4 / 0 mod 4, 4:2:0 / 4:4:4, top / bottom first, modes plain / shift / same,
#            radius 0..5, window ends, Amount 0, Mix)
#   real   : test.mp4 interlaced by ffmpeg tinterlace (TFF and BFF, 640x360 yuv420p), a v5 profile of a field,
#            kind 31 in the three modes
#   bridge : libnvcore nvc_temporal_shift (ctypes): NULL shift == nvc_temporal, shifts == fields_ref.temporal_shift
#   --sensitivity: mutants of the C++ sources (built in a temporary directory) must fail the comparison
import argparse
import ctypes
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from multiprocessing import Pool

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.dont_write_bytecode = True
import v5_cases as vc  # noqa: E402
from v5_cases import F32, nv, np  # noqa: E402
import fields_ref as fr  # noqa: E402
import verify_temporal as vt  # noqa: E402

vc.KIND.update({"tshift": 30, "fields": 31})
ROOT = vc.ROOT


# ------------------------------------------------------------------------------------------- inputs
def frames_420(rng, H, W, nf, grid, noise=None):
    """nf progressive frames (moving texture + noise), Y H x W, chroma 4:2:0 or 4:4:4."""
    base = [vt.texture(rng, H, W) for _ in range(3)]
    v = rng.uniform(-2.5, 2.5, 2)
    out = []
    for k in range(nf):
        dy, dx = v * (k - nf // 2) + rng.uniform(-0.3, 0.3, 2)
        dy, dx = float(np.clip(dy, -7, 7)), float(np.clip(dx, -7, 7))
        s = noise if noise is not None else rng.uniform(0.5, 6)
        Y = vt.shifted(base[0], H, W, dy, dx) + rng.normal(0, s, (H, W))
        if grid == "420":
            ch = ((H + 1) // 2, (W + 1) // 2)
            C = [vt.shifted(b, H, W, dy, dx)[::2, ::2][:ch[0], :ch[1]] * 0.3 + 90 + rng.normal(0, 1, ch) for b in base[1:]]
        else:
            C = [vt.shifted(b, H, W, dy, dx) * 0.3 + 90 for b in base[1:]]
        out.append(tuple(np.ascontiguousarray(np.clip(p, 0, 255), F32) for p in [Y] + C))
    return out


def interlace(prog, first):
    """Frame k: the field of parity `first` from prog[2k], the other from prog[2k + 1] (tinterlace interleave)."""
    out = []
    for k in range(len(prog) // 2):
        a, b = prog[2 * k], prog[2 * k + 1]
        f = []
        for pa, pb in zip(a, b):
            p = pa.copy()
            p[1 - first::2] = pb[1 - first::2]
            f.append(p)
        out.append(tuple(f))
    return out


def rand_shift(rng, nf, ci):
    sh = []
    for k in range(nf):
        c = rng.choice(["zero", "half", "mhalf", "luma", "chroma", "any"])
        if k == ci or c == "zero":
            sh.append((0.0, 0.0))
        elif c == "half":
            sh.append((0.5, 0.5))
        elif c == "mhalf":
            sh.append((-0.5, -0.5))
        elif c == "luma":
            sh.append((float(rng.choice([0.5, -0.5, 1.25])), 0.0))
        elif c == "chroma":
            sh.append((0.0, float(rng.choice([0.5, -0.5, 0.25]))))
        else:
            sh.append((float(rng.uniform(-1.5, 1.5)), float(rng.uniform(-1, 1))))
    return sh


def tshift_cases(rng, n):
    out = []
    shapes = [(32, 32), (48, 64), (64, 48), (72, 100), (45, 67), (40, 20), (100, 75), (16, 16)]
    for i in range(n):
        H, W = shapes[i % len(shapes)]
        grid = "420" if i % 3 else "444"
        nf = int(rng.integers(2, 6))
        frames = vt.make_window(rng, H, W, nf, grid, "normal" if i % 7 else "int")
        ci = int(rng.integers(0, nf))
        P = vt.rand_params(rng)
        P["bs"] = int(min(P["bs"], H, W))
        P["bs"] = max(P["bs"], 4)
        if rng.random() < 0.2:
            P["temporal"] = 1.0
        prof = vt.rand_profile(rng, 4 if grid == "420" else 5)
        shift = rand_shift(rng, nf, ci)
        flows = {}
        if rng.random() < 0.3:      # a given flow for one neighbour
            k = (ci + 1) % nf
            if k != ci:
                f0 = fr.estimate_flow_v5(frames[k][0], frames[ci][0], P["bs"], P["search"])
                flows[k - ci] = f0
        out.append(dict(group="tshift", label=f"{H}x{W} {grid} nf{nf} ci{ci} {P['similarity']}", kind="tshift",
                        frames=frames, ci=ci, prof=prof, P=P, shift=shift, flows=flows))
        if i % 4 == 0:              # zero shifts against kind 2 (C++ temporal_v5 itself)
            out.append(dict(group="tshift0", label=f"{H}x{W} {grid} nf{nf} zero shifts == kind 2", kind="tshift",
                            frames=frames, ci=ci, prof=prof, P=P, shift=[(0.0, 0.0)] * nf, flows=dict(flows),
                            vs_kind2=True))
    return out


def field_cases(rng, n):
    out = []
    shapes = [(40, 48), (66, 64), (67, 80), (64, 96), (34, 40), (81, 50), (96, 128), (50, 70)]
    for i in range(n):
        H, W = shapes[i % len(shapes)]
        grid = "420" if i % 4 else "444"
        nfr = int(rng.integers(1, 6))
        first = int(i % 2)
        prog = frames_420(rng, H, W, 2 * nfr, grid)
        frames = interlace(prog, first)
        ci = int(rng.integers(0, nfr))
        mode = (fr.PLAIN, fr.SHIFT, fr.SAME)[(i // 2) % 3]
        P = vt.rand_params(rng)
        P["radius"] = int(rng.integers(0, 6))
        hf = (H + 1) // 2
        P["bs"] = int(max(4, min(P["bs"], hf, W)))
        prof = vt.rand_profile(rng, 4 if grid == "420" else 5)
        out.append(dict(group="fields", label=f"{H}x{W} {grid} nfr{nfr} ci{ci} first{first} mode{mode} R{P['radius']} "
                                              f"{P['similarity']} a{P['temporal']}",
                        kind="fields", frames=frames, ci=ci, first=first, mode=mode, prof=prof, P=P))
    return out


def tinterlaced(tmp, first, n):
    """test.mp4 n = 300 .. 300 + 2n - 1 interlaced by tinterlace (interleave_top / _bottom), yuv420p 640x360."""
    path = os.path.join(tmp, f"real_{first}_{n}.npz")
    if not os.path.exists(path):
        W, H = 640, 360
        mode = "interleave_top" if first == 0 else "interleave_bottom"
        buf = subprocess.run(["ffmpeg", "-v", "error", "-nostdin", "-i", vc.VIDEOS["test"], "-vf",
                              f"select=between(n\\,300\\,{300 + 2 * n - 1}),setpts=N/(60*TB),tinterlace=mode={mode}",
                              "-frames:v", str(n), "-fps_mode", "passthrough", "-f", "rawvideo", "-pix_fmt", "yuv420p",
                              "-"], capture_output=True, check=True).stdout
        fs = W * H * 3 // 2
        assert len(buf) == n * fs, (len(buf), n * fs)
        a = np.frombuffer(buf, np.uint8).reshape(n, fs)
        y = a[:, :W * H].reshape(n, H, W)
        cb = a[:, W * H:W * H + W * H // 4].reshape(n, H // 2, W // 2)
        cr = a[:, W * H + W * H // 4:].reshape(n, H // 2, W // 2)
        np.savez(path, y=y, cb=cb, cr=cr)
    z = np.load(path)
    return [tuple(np.ascontiguousarray(z[k][i], F32) for k in ("y", "cb", "cr")) for i in range(z["y"].shape[0])]


def real_cases(tmp, quick):
    out = []
    n = 3 if quick else 5
    for first in (0, 1):
        frames = tinterlaced(tmp, first, n)
        fields, parity = fr.field_sequence(frames, first)
        # the profile of the field sequence's first field (pair: the next field of the same parity)
        prof = nv.build_profile_v5(fields[0], False, None, pair=fields[2], mode="auto", matrix="bt601")
        for mode in (fr.PLAIN, fr.SHIFT, fr.SAME):
            for ci in ([1] if quick else [0, 2, n - 1]):
                out.append(dict(group="real", label=f"test.mp4 i{'TFF' if first == 0 else 'BFF'} ci{ci} mode{mode}",
                                kind="fields", frames=frames, ci=ci, first=first, mode=mode, prof=prof,
                                P=nv.v5_params()))
    return out


# ----------------------------------------------------------------------------------- run / compare
def expected(c):
    if c["kind"] == "tshift":
        flows = dict(c["flows"])
        o, rv = fr.temporal_shift(c["frames"], c["ci"], c["prof"], c["P"], flows, c["shift"])
        return list(o) + list(rv) + [p for k in sorted(flows) for p in flows[k]]
    return fr.process_frame_fields(c["frames"], c["ci"], c["first"], c["mode"], c["prof"], c["P"])


def encode(c, as_kind2=False):
    if c["kind"] == "tshift":
        offs = sorted(c["flows"])
        planes = [p for f in c["frames"] for p in f] + [p for o in offs for p in c["flows"][o]]
        ints = [c["ci"], len(c["frames"]), len(offs)] + offs
        if as_kind2:
            return vc.encode_case("temporal", c["prof"], c["P"], ints, (), planes)
        dbls = [v for s in c["shift"] for v in s]
        return vc.encode_case("tshift", c["prof"], c["P"], ints, dbls, planes)
    planes = [p for f in c["frames"] for p in f]
    return vc.encode_case("fields", c["prof"], c["P"], [c["ci"], len(c["frames"]), c["first"], c["mode"]], (), planes)


def _exp_chunk(cs):
    import warnings
    warnings.simplefilter("ignore")
    return [expected(c) for c in cs]


def run_cpp(cases, tmp, exe, jobs, tag, as_kind2=False):
    inp, outp = os.path.join(tmp, f"{tag}_in.bin"), os.path.join(tmp, f"{tag}_out.bin")
    vc.write_cases(inp, [encode(c, as_kind2) for c in cases])
    vc.run_exe(inp, outp, jobs, exe)
    return vc.read_results(outp, len(cases))


def compare(exp, got):
    if len(exp) != len(got):
        return max(1, sum(np.size(e) for e in exp))
    return sum(vc.diff_count(e, g) for e, g in zip(exp, got))


def copy_check(rng, n):
    """fields_ref.temporal_shift without offsets == nvfilter.temporal_v5 (the reference's copy of the kernel)."""
    ok = tot = 0
    words = 0
    for c in tshift_cases(rng, n):
        if c.get("vs_kind2"):
            continue
        fl1, fl2, fl3 = dict(c["flows"]), dict(c["flows"]), dict(c["flows"])
        a = nv.temporal_v5(c["frames"], c["ci"], c["prof"], c["P"], fl1)
        b = fr.temporal_shift(c["frames"], c["ci"], c["prof"], c["P"], fl2, None)
        z = fr.temporal_shift(c["frames"], c["ci"], c["prof"], c["P"], fl3, [(0.0, 0.0)] * len(c["frames"]))
        ea = list(a[0]) + list(a[1]) + [p for k in sorted(fl1) for p in fl1[k]]
        bad = 0
        for r, fl in ((b, fl2), (z, fl3)):
            eb = list(r[0]) + list(r[1]) + [p for k in sorted(fl) for p in fl[k]]
            bad += compare(ea, eb)
        tot += 1
        ok += bad == 0
        words += 2 * sum(np.size(e) for e in ea)
        if bad:
            print(f"  copy MISMATCH {c['label']}: {bad} words")
    print(f"copy  : fields_ref.temporal_shift (no / zero shifts) == nvfilter.temporal_v5: {ok}/{tot} cases, {words} words")
    # mode plain: every field == nvfilter.process_frame_v5 on the field sequence (fields_ref adds only the sequence)
    ok2 = tot2 = words2 = 0
    for c in field_cases(rng, max(4, n // 2)):
        fields, parity = fr.field_sequence(c["frames"], c["first"])
        for j in (0, 1):
            s = 2 * c["ci"] + j
            e = nv.process_frame_v5(fields, s, c["prof"], c["P"], {})
            g = fr.process_field(fields, parity, s, fr.PLAIN, c["prof"], c["P"])
            bad = compare(e, g)
            tot2 += 1
            ok2 += bad == 0
            words2 += sum(np.size(x) for x in e)
            if bad:
                print(f"  plain MISMATCH {c['label']} field {j}: {bad} words")
    print(f"copy  : fields_ref.process_field (plain) == nvfilter.process_frame_v5 on the fields: {ok2}/{tot2} fields, "
          f"{words2} words")
    return ok == tot and ok2 == tot2


def check(cases, exps, gots, tag, quiet=False):
    groups = {}
    for c, e, g in zip(cases, exps, gots):
        bad = compare(e, g)
        n = sum(np.size(x) for x in e)
        s = groups.setdefault(c["group"], [0, 0, 0, 0])
        s[0] += 1
        s[1] += bad == 0
        s[2] += n
        s[3] += bad
        if bad and not quiet:
            print(f"  {tag} MISMATCH [{c['group']}] {c['label']}: {bad}/{n} words")
    return groups


def bridge_check(cases):
    lib_path = os.environ.get("NV_NATIVE_LIB", os.path.join(HERE, "libnvcore.dylib"))
    os.environ["NV_NATIVE_LIB"] = lib_path
    import nv_native as nn
    if not nn.AVAILABLE:
        print(f"bridge: {lib_path} not loadable")
        return False
    lib = nn._lib
    _P, _I = ctypes.c_void_p, ctypes.c_int
    lib.nvc_temporal_shift.argtypes = [_I] * 6 + [_P] * 5 + [_I] + [_P] * 7
    lib.nvc_temporal_shift.restype = _I
    ok = tot = words = 0
    for c in cases:
        fr_, ci, prof, P = c["frames"], c["ci"], c["prof"], c["P"]
        blocks = nn._v5_blocks(prof, P)
        H, W = fr_[0][0].shape
        Hc, Wc = fr_[0][1].shape
        bs = P["bs"]
        nby, nbx = H // bs, W // bs
        nf = len(fr_)
        planes = [nn._c32(p) for f in fr_ for p in f]
        pi, pd, qi, qd = blocks
        res = []
        for shift in (None, c["shift"]):
            fout = [np.empty((nby, nbx), F32) for _ in range(2 * nf)]
            new = np.zeros(nf, np.uint8)
            out = [np.empty((H, W), F32), np.empty((Hc, Wc), F32), np.empty((Hc, Wc), F32)]
            rv = [np.empty((H, W), F32), np.empty((Hc, Wc), F32), np.empty((Hc, Wc), F32)]
            offa = np.array([0], np.int32)
            sh = None if shift is None else np.ascontiguousarray([v for s in shift for v in s], np.float64)
            r = lib.nvc_temporal_shift(nf, ci, H, W, Hc, Wc, nn._ptrs(planes), nn._ptr(pi), nn._ptr(pd), nn._ptr(qi),
                                       nn._ptr(qd), 0, nn._ptr(offa), None, nn._ptrs(fout), nn._ptr(new),
                                       nn._ptrs(out), nn._ptrs(rv), None if sh is None else nn._ptr(sh))
            flows = [fout[2 * k + j] for k in range(nf) if new[k] for j in (0, 1)]
            res.append((r, out + rv + flows))
        # NULL shift == nvc_temporal (nv_native.temporal_v5 calls it), shifts == the reference
        fl = {}
        base = nn.temporal_v5(fr_, ci, prof, P, fl)
        eb = list(base[0]) + list(base[1]) + [p for k in sorted(fl) for p in fl[k]]
        fl2 = {}
        o, rv = fr.temporal_shift(fr_, ci, prof, P, fl2, c["shift"])
        es = list(o) + list(rv) + [p for k in sorted(fl2) for p in fl2[k]]
        bad = (res[0][0] != 0) + (res[1][0] != 0) + compare(eb, res[0][1]) + compare(es, res[1][1])
        tot += 1
        ok += bad == 0
        words += sum(np.size(x) for x in eb + es)
        if bad:
            print(f"  bridge MISMATCH {c['label']}: {bad}")
    print(f"bridge: nvc_temporal_shift (NULL == nvc_temporal, shifts == fields_ref) {ok}/{tot} cases, {words} words "
          f"({lib_path})")
    return ok == tot


MUTANTS = [   # name, file, old, new
    ("SIGN", "nv_fieldproc.cpp", "0.5 * (parity[ci] - parity[k])", "0.5 * (parity[k] - parity[ci])"),
    ("CHROMA_HALF", "nv_fieldproc.cpp", "shift.push_back(d);\n        shift.push_back(d);",
     "shift.push_back(d);\n        shift.push_back(0.5 * d);"),
    ("SAME_OFF", "nv_fieldproc.cpp", "if (mode == SAME && parity[k] != parity[ci]) continue;", ""),
    ("RADIUS", "nv_fieldproc.cpp", "std::max(0, ci - R)", "std::max(0, ci - R - 1)"),
    ("ORDER", "nv_fieldproc.cpp", "fields[2 * k] = first == nvfield::TOP ? t : b;", "fields[2 * k] = t;"),
    ("NO_PRESHIFT", "nv_temporal.cpp", "fl = estimate_flow_v5(ref, Y, P.bs, P.search);",
     "fl = estimate_flow_v5(nb.p[0], Y, P.bs, P.search);"),
    ("CHROMA420_BEFORE", "nv_temporal.cpp", "for (size_t i = 0; i < fyc.size(); ++i) fyc.d[i] = A(fyc.d[i], o);",
     "for (size_t i = 0; i < fyc.size(); ++i) fyc.d[i] = A(fyc.d[i], M(o, HALF));"),
    ("LUMA_SHIFT_OFF", "nv_temporal.cpp", "wy = warp_plane(nb.p[0], fyl, FX);", "wy = warp_plane(nb.p[0], FY, FX);"),
]


def sensitivity(cases, exps, tmp, jobs):
    ok = True
    src = ["nv_v5_main.cpp", "nv_ndimage.cpp", "nv_spatial.cpp", "nv_temporal.cpp", "nv_fieldproc.cpp", "nv_fields.cpp"]
    hdr = [f for f in os.listdir(HERE) if f.endswith(".h")]
    for name, fn, old, new in MUTANTS:
        d = os.path.join(tmp, f"mut_{name}")
        os.makedirs(d, exist_ok=True)
        for f in src + hdr:
            shutil.copy(os.path.join(HERE, f), d)
        s = open(os.path.join(d, fn)).read()
        assert s.count(old) == 1, (name, s.count(old))
        open(os.path.join(d, fn), "w").write(s.replace(old, new))
        exe = os.path.join(d, "nv_v5_test")
        subprocess.run(["clang++", "-std=c++17", "-O2", "-ffp-contract=off", "-fno-fast-math", "-o", exe] + src + ["-lpthread"],
                       cwd=d, check=True)
        gots = run_cpp(cases, d, exe, jobs, "m")
        g = check(cases, exps, gots, name, quiet=True)
        bad = sum(v[0] - v[1] for v in g.values())
        det = bad > 0
        ok &= det
        print(f"  {name:17s} {bad}/{len(cases)} cases differ -> {'DETECTED' if det else 'NOT DETECTED'}")
    return ok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--no-real", action="store_true")
    ap.add_argument("--no-bridge", action="store_true")
    ap.add_argument("--sensitivity", action="store_true")
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--seed", type=int, default=20261101)
    ap.add_argument("--exe", default=vc.EXE)
    ap.add_argument("--tmp", default=os.path.join(ROOT, "work", "omo", "wp_interlace", "INT-2", "tmp"))
    a = ap.parse_args()
    jobs = max(1, min(a.jobs, 3))
    os.makedirs(a.tmp, exist_ok=True)
    rng = np.random.default_rng(a.seed)
    nt, nfl = (16, 24) if a.quick else (48, 96)
    ok = copy_check(np.random.default_rng(a.seed + 1), 12 if a.quick else 24)
    cases = tshift_cases(rng, nt) + field_cases(rng, nfl)
    if not a.no_real:
        cases += real_cases(a.tmp, a.quick)
    t0 = time.time()
    chunks = [cases[i::jobs] for i in range(jobs)]
    with Pool(jobs) as pool:
        res = pool.map(_exp_chunk, chunks)
    exps = [None] * len(cases)
    for j, r in enumerate(res):
        for i, e in enumerate(r):
            exps[i * jobs + j] = e
    t_py = time.time() - t0
    t0 = time.time()
    with tempfile.TemporaryDirectory(dir=a.tmp) as tmp:
        gots = run_cpp(cases, tmp, a.exe, jobs, "c")
        t_cpp = time.time() - t0
        groups = check(cases, exps, gots, "C++")
        k2i = [i for i, c in enumerate(cases) if c.get("vs_kind2")]
        k2 = [cases[i] for i in k2i]
        if k2:
            g2 = run_cpp(k2, tmp, a.exe, jobs, "k2", as_kind2=True)
            g30 = [gots[i] for i in k2i]
            same = sum(compare(x, y) == 0 for x, y in zip(g2, g30))
            words = sum(sum(np.size(p) for p in x) for x in g2)
            print(f"kind 30 zero shifts == kind 2 (C++ temporal_v5): {same}/{len(k2)} cases, {words} words")
            ok &= same == len(k2)
        for gname, (n, good, words, bad) in groups.items():
            print(f"{gname:7s}: {good}/{n} cases bit-exact, {words - bad}/{words} words")
            ok &= good == n
        print(f"time: Python {t_py:.1f} s ({jobs} procs), C++ {t_cpp:.1f} s")
        if not a.no_bridge:
            bc = [c for c in cases if c["kind"] == "tshift" and not c["flows"]][:24]
            ok &= bridge_check(bc)
        if a.sensitivity:
            print("negative controls (each mutant must differ):")
            si = [i for i, c in enumerate(cases) if c["group"] in ("tshift", "fields")]
            ok &= sensitivity([cases[i] for i in si], [exps[i] for i in si], tmp, jobs)
    print("ALL BIT-EXACT" if ok else "FAILURES")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
