#!/usr/bin/env python3
"""Replay of the leaf calls native/nv_spatial6 made inside plane_filter (recorded by `nv_spatial6_test plane ... rec DIR`,
tools/compare_python.py --rec) on the genuine leaves through the guarded harness `nvh`: each recorded call is run with the
recorded inputs and the genuine output is compared with the recorded output (bit for bit; linefilt through its %.9g
text output, exact for finite values, NaN compared as NaN).

  python3 replay.py DIR [DIR ...] [--jobs 3] [--log FILE]

sel2 / gather with stride > 65536 (the `nvh sel2|gather` limit) are replayed in chunks of 65536 elements: element i
of FUN_0098eba8 / FUN_0098ed34 reads only a, b, i3, i4, best at i and src / wt at j * stride + i, the chunk starts are
multiples of 4 and the last chunk keeps the original n & 3 tail, so every element takes the same NEON / scalar form as in
the recorded call.  A call outside the remaining harness limits is counted as SKIPPED (reason), an unexpected harness
failure as ERROR; neither stops the run.  The exit status is 1 on a mismatch, an ERROR or (without --allow-skip) a SKIP.

Calls without a genuine counterpart that the harness may run (FUN_00993df0 up-sampling, FUN_00990da8 band split, the
byte copies of the B / I / A18 work planes) are not recorded.  Exit 1 on any mismatch."""
import argparse, collections, concurrent.futures as cf, os, subprocess, sys, tempfile, time
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
NVH = os.path.join(ROOT, "reference", "genuine", "analysis", "harness", "nvh")
BUNDLE = os.path.join(ROOT, "reference", "genuine", "NeatVideo6.ofx.bundle", "Contents", "MacOS", "NeatVideo6.ofx")
ENV = dict(os.environ, NV_BUNDLE=BUNDLE)


def nvh(args):
    r = subprocess.run([NVH, *args], capture_output=True, text=True, env=ENV)
    if r.returncode != 0:
        raise RuntimeError("nvh %s exit %d: %s" % (args[0], r.returncode, r.stderr[-300:]))
    return r.stdout


def same_bytes(a, b):
    return open(a, "rb").read() == open(b, "rb").read()


def pad(src, dst, nfloat):
    v = np.fromfile(src, np.float32)
    out = np.zeros(nfloat, np.float32)
    out[:v.size] = v
    out.tofile(dst)


CHUNK = 65536   # the stride limit of `nvh sel2` / `nvh gather`


class Skip(Exception):
    pass


def limits(kind, tok):
    """The harness limits (nvh.c) a recorded call must meet, or a SKIP reason."""
    if kind == "elem" or kind == "pairw":
        n = int(tok[3] if kind == "elem" else tok[2])
        if n > (1 << 22):
            return "n > 4M"
    if kind in ("strip3", "linefilt"):
        W, H = int(tok[2]), int(tok[3])
        if W > 512 or H > 512:
            return "side > 512"
        if kind == "strip3" and (W < 4 or H < 2):
            return "strip3 W < 4 or H < 2"
    if kind == "down2":
        W, H = int(tok[2]), int(tok[3])
        if W < 2 or H < 2 or W > 4096 or H > 4096:
            return "down2 side outside 2..4096"
    if kind == "argmax":
        n, cnt, stride = map(int, tok[2:5])
        if cnt > 64 or stride > (1 << 22):
            return "argmax count > 64 or stride > 4M"
    return None


def chunked(d, cid, kind, n, stride, flag, td):
    """sel2 / gather with a stride above the nvh limit, in chunks of CHUNK elements (see the module docstring)."""
    if n < 1 or stride < n:
        raise Skip("%s n = %d, stride = %d" % (kind, n, stride))
    f = lambda tag: os.path.join(d, "%s.%s.bin" % (cid, tag))
    rd = lambda tag, dt=np.float32: np.fromfile(f(tag), dt)
    ok = True
    for a0 in range(0, n, CHUNK):
        a1 = min(n, a0 + CHUNK)
        m = a1 - a0
        c = os.path.join(td, "c%s_%d" % (cid, a0))
        def rows(tag):   # 256 rows of m floats: row j = recorded[j * stride + a0 : j * stride + a1] (zero past the end)
            v = rd(tag)
            out = np.zeros((256, m), np.float32)
            for j in range(min(256, v.size // stride)):
                out[j] = v[j * stride + a0:j * stride + a1]
            fn = c + "." + tag + ".rows.f32"
            out.tofile(fn)
            return fn
        def sl(tag, dt=np.float32):
            fn = c + "." + tag + ".slice"
            rd(tag, dt)[a0:a1].tofile(fn)
            return fn
        g = c + ".g"
        if kind == "gather":
            nvh(["gather", str(m), str(m), sl("in0", np.uint8), rows("in1"), g])
            ok &= open(g, "rb").read() == rd("out")[a0:a1].tobytes()
        else:
            nvh(["sel2", str(m), str(flag), str(m), sl("in0"), sl("in1"), sl("in2", np.uint8), sl("in3", np.uint8), sl("in4"),
                 rows("in5"), rows("in6"), g, "full"])
            ok &= (open(g + ".out.f32", "rb").read() == rd("out")[a0:a1].tobytes() and
                   open(g + ".best.f32", "rb").read() == rd("out1")[a0:a1].tobytes() and
                   open(g + ".b.f32", "rb").read() == rd("out2")[a0:a1].tobytes() and
                   open(g + ".i4", "rb").read() == rd("out3", np.uint8)[a0:a1].tobytes())
    return ok


def replay_one(d, line, td):
    tok = line.split()
    why = limits(tok[1], tok)
    if why:
        raise Skip(why)
    cid, kind = tok[0], tok[1]
    f = lambda tag: os.path.join(d, "%s.%s.bin" % (cid, tag))
    g = os.path.join(td, "g%s" % cid)
    if kind == "elem":
        name, n = tok[2], tok[3]
        s = tok[4:8]
        ins = [f(t) for t in tok[8:] if t.startswith("in")]
        nvh(["elem", name, n, g, *s, *ins])
        return name, same_bytes(g, f("out"))
    if kind == "pairw":
        n = tok[2]
        nvh(["pairw", f("in0"), f("in1"), f("in2"), n, g])
        return "pairw", same_bytes(g, f("out"))
    if kind == "strip3":
        W, H, kd, mode, sb = tok[2:7]
        nvh(["strip3", W, H, f("in0"), kd, mode, sb, g])
        return "strip3-" + kd, same_bytes(g, f("out"))
    if kind == "down2":
        W, H = tok[2:4]
        nvh(["down2", W, H, f("in0"), g])
        return "down2", same_bytes(g, f("out"))
    if kind == "linefilt":
        W, H, ab, r, mode = tok[2:7]
        ang = np.uint32(int(ab, 16)).view(np.float32)
        txt = nvh(["linefilt", W, H, f("in0"), "%.9g" % ang, r, mode])
        got = np.array([float(x.split()[1]) for x in txt.splitlines() if x and x[0].isdigit()], np.float32)
        want = np.fromfile(f("out"), np.float32)
        ok = got.size == want.size and bool(np.all((got.view(np.uint32) == want.view(np.uint32)) | (np.isnan(got) & np.isnan(want))))
        return "linefilt", ok
    if kind == "argmax":
        n, cnt, stride = tok[2:5]
        nvh(["argmax", n, cnt, stride, f("in0"), g])
        return "argmax", same_bytes(g + ".idx", f("out")) and same_bytes(g + ".max.f32", f("out1"))
    if kind == "gather" and int(tok[3]) > CHUNK:
        return "gather-chunked", chunked(d, cid, "gather", int(tok[2]), int(tok[3]), 0, td)
    if kind == "sel2" and int(tok[4]) > CHUNK:
        return "sel2-chunked", chunked(d, cid, "sel2", int(tok[2]), int(tok[4]), int(tok[3]), td)
    if kind == "gather":
        n, stride = tok[2:4]
        mp = os.path.join(td, "maps%s.f32" % cid)
        pad(f("in1"), mp, 256 * int(stride))
        nvh(["gather", n, stride, f("in0"), mp, g])
        return "gather", same_bytes(g, f("out"))
    if kind == "sel2":
        n, flag, stride = tok[2:5]
        sp, wp = os.path.join(td, "src%s.f32" % cid), os.path.join(td, "wt%s.f32" % cid)
        pad(f("in5"), sp, 256 * int(stride))
        pad(f("in6"), wp, 256 * int(stride))
        nvh(["sel2", n, flag, stride, f("in0"), f("in1"), f("in2"), f("in3"), f("in4"), sp, wp, g, "full"])
        ok = (same_bytes(g + ".out.f32", f("out")) and same_bytes(g + ".best.f32", f("out1")) and
              same_bytes(g + ".b.f32", f("out2")) and same_bytes(g + ".i4", f("out3")))
        return "sel2", ok
    raise ValueError("unknown call kind " + kind)


def replay_dir(d):
    stats = collections.Counter()
    bad = []
    with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR")) as td:
        for line in open(os.path.join(d, "calls.txt")):
            line = line.strip()
            if not line:
                continue
            try:
                name, ok = replay_one(d, line, td)
            except Skip as e:
                name, ok = "%s SKIPPED (%s)" % (line.split()[1], e), "skip"
            except RuntimeError as e:
                name, ok = "%s ERROR" % line.split()[1], "error"
                bad.append(line + "  # " + str(e)[:200])
            stats[(name, ok)] += 1
            if ok is False:
                bad.append(line)
            for fn in os.listdir(td):
                os.remove(os.path.join(td, fn))
    return d, stats, bad


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dirs", nargs="+")
    ap.add_argument("--jobs", type=int, default=3)
    ap.add_argument("--log", default="")
    ap.add_argument("--allow-skip", action="store_true", help="exit 0 even when calls were skipped")
    a = ap.parse_args()
    log = open(a.log, "w") if a.log else None

    def say(m):
        print(m, flush=True)
        if log:
            log.write(m + "\n"); log.flush()

    r = subprocess.run([NVH, "selftest"], capture_output=True, text=True, env=ENV)
    if r.returncode != 3:
        sys.exit("nvh selftest exit %d (want 3)" % r.returncode)
    t0 = time.time()
    total = collections.Counter()
    nbad = 0
    with cf.ThreadPoolExecutor(a.jobs) as ex:
        for d, stats, bad in ex.map(replay_dir, a.dirs):
            calls = sum(stats.values())
            fails = sum(v for (nm, ok), v in stats.items() if ok is False or ok == "error")
            nbad += fails
            total.update(stats)
            skips = sum(v for (nm, ok), v in stats.items() if ok == "skip")
            say("%s: %d calls, %d mismatching, %d skipped" % (os.path.basename(d), calls, fails, skips))
            for b in bad[:5]:
                say("   MISMATCH " + b)
    names = sorted({nm for nm, ok in total if ok in (True, False)})
    for nm in names:
        say("  %-14s ok %6d  mismatch %d" % (nm, total[(nm, True)], total[(nm, False)]))
    other = sorted((nm, ok, v) for (nm, ok), v in total.items() if ok in ("skip", "error"))
    for nm, ok, v in other:
        say("  %s: %d" % (nm, v))
    nskip = sum(v for nm, ok, v in other if ok == "skip")
    nerr = sum(v for nm, ok, v in other if ok == "error")
    say("TOTAL calls %d, mismatching %d, errors %d, skipped %d (%.0f s)" % (sum(total.values()), nbad - nerr, nerr, nskip, time.time() - t0))
    sys.exit(1 if nbad or (nskip and not a.allow_skip) else 0)


if __name__ == "__main__":
    main()
