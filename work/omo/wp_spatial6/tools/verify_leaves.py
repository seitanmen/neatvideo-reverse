#!/usr/bin/env python3
"""Leaf check of native/nv_spatial6 against the genuine leaves through the guarded harness `nvh` (bit for bit, NaN
payloads included).  Every call is run twice with the same files - `nvh ...` (the genuine function inside the bundle)
and `native/nv_spatial6_test ...` (the port, the same argument form) - and the output files are compared byte for byte.

  python3 verify_leaves.py [--seed S] [--big N] [--small N] [--only a,b] [--mutants] [--log FILE]

Each element-wise leaf gets `--big` calls with n in [2000, 120000] and `--small` calls with n in 1..40 (the scalar
tails n & 3 only show up there), inputs mixing normal values of several scales with +-0, +-inf, quiet / signalling NaNs
of both signs and various payloads, subnormals, huge values and values near the thresholds; scalars likewise.
--mutants: builds native/nv_spatial6_mut1..9 (one mutation per leaf, NV6_MUT) and checks that each is detected.
"""
import argparse, os, subprocess, sys, tempfile, time
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
NVH = os.path.join(ROOT, "reference", "genuine", "analysis", "harness", "nvh")
PORT = os.path.join(ROOT, "native", "nv_spatial6_test")
BUNDLE = os.path.join(ROOT, "reference", "genuine", "NeatVideo6.ofx.bundle", "Contents", "MacOS", "NeatVideo6.ofx")
ENV = dict(os.environ, NV_BUNDLE=BUNDLE)

# name: (number of scalars, number of input arrays); "pairw" is its own nvh subcommand
ELEM = {"recip": (1, 1), "mul": (0, 2), "sqr": (0, 1), "sub": (0, 2), "sqrtk": (1, 1), "rstep": (1, 2),
        "robust": (1, 2), "scale": (1, 1), "clampmul": (4, 2),
        # the other leaves FUN_0098edb0 / FUN_00990f74 call
        "add": (0, 2), "sqrtabs": (0, 1), "rinv": (1, 2), "affine": (2, 1), "acc": (1, 3), "blend": (2, 2),
        "bladd": (2, 2), "ratio": (1, 2), "ramp4": (4, 1), "wmul4": (4, 2), "fabs": (0, 1)}
NINE = ["recip", "mul", "sqr", "sub", "sqrtk", "rstep", "robust", "scale", "clampmul"]
MUTANT_OF = {1: "recip", 2: "mul", 3: "sqr", 4: "sub", 5: "sqrtk", 6: "rstep", 7: "robust", 8: "scale", 9: "clampmul"}

SPECIAL = np.array([0x00000000, 0x80000000, 0x7f800000, 0xff800000, 0x7fc00000, 0xffc00000, 0x7fc12345, 0xffd54321,
                    0x7f800001, 0xff812345, 0x7fa00000, 0x00000001, 0x80000001, 0x007fffff, 0x807fffff, 0x00800000,
                    0x7f7fffff, 0xff7fffff, 0x3f800000, 0xbf800000, 0x3f800001, 0x3f7fffff, 0x5f000000, 0x1f000000],
                   np.uint32).view(np.float32)


def values(rng, n, p_special=0.15):
    scale = rng.choice([1e-3, 0.05, 1.0, 3.0, 30.0, 255.0, 1e6])
    v = (rng.standard_normal(n) * scale).astype(np.float32)
    m = rng.random(n) < p_special
    v[m] = rng.choice(SPECIAL, m.sum())
    return v


def scalar(rng, kind):
    if rng.random() < 0.12:
        return np.float32(rng.choice(SPECIAL))
    if kind == "pos":
        return np.float32(10.0 ** rng.uniform(-3, 2))
    return np.float32(rng.standard_normal() * rng.choice([0.1, 1.0, 10.0]))


def scalars(rng, name):
    if name in ("clampmul",):
        a = scalar(rng, "any"); b = scalar(rng, "any")
        lo, hi = sorted([scalar(rng, "any"), scalar(rng, "any")]) if rng.random() < 0.8 else (scalar(rng, "any"), scalar(rng, "any"))
        return [a, b, np.float32(lo), np.float32(hi)]
    if name in ("ramp4", "wmul4"):
        a = scalar(rng, "pos")
        r = rng.random()
        b = np.float32(a * 2) if r < 0.4 else (a if r < 0.5 else scalar(rng, "any"))
        cd = [(1.0, 0.0), (0.0, 1.0), (0.0, -1.0), (1.0, 1.0)][rng.integers(4)] if rng.random() < 0.8 else (scalar(rng, "any"), scalar(rng, "any"))
        return [a, np.float32(b), np.float32(cd[0]), np.float32(cd[1])]
    if name in ("recip", "ratio"):
        return [np.float32(1e-4) if rng.random() < 0.5 else scalar(rng, "pos")]
    if name in ("rstep", "robust", "rinv", "sqrtk", "scale"):
        return [scalar(rng, "pos")]
    return [scalar(rng, "any") for _ in range(2)]


def hexs(s):
    s = list(s) + [np.float32(0)] * (4 - len(s))
    return ["%08x" % np.float32(x).view(np.uint32) for x in s]


def clear(td):
    """Every check starts from an empty work directory, so no output of an earlier call can be compared by mistake."""
    for fn in os.listdir(td):
        os.remove(os.path.join(td, fn))


def run(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True, env=ENV)
    if r.returncode != 0:
        raise RuntimeError("%s exit %d: %s" % (cmd[0], r.returncode, r.stderr[-300:]))
    if "alert" in r.stderr.lower() and cmd[0] == NVH:
        raise RuntimeError("genuine assertion: " + r.stderr[-300:])


def compare(files_g, files_p):
    bad = 0
    for g, p in zip(files_g, files_p):
        a, b = open(g, "rb").read(), open(p, "rb").read()
        if a != b:
            ua = np.frombuffer(a, np.uint8 if g.endswith((".idx", ".i4")) else np.uint32)
            ub = np.frombuffer(b, ua.dtype)
            bad += int((ua != ub).sum()) if ua.size == ub.size else max(ua.size, ub.size)
    return bad


def check_elem(rng, td, name, n, port=PORT):
    clear(td)
    ns, ni = ELEM[name]
    ins = []
    for i in range(ni):
        x = values(rng, n)
        if name in ("rstep", "robust", "rinv") and i == 1 and rng.random() < 0.5:   # |v| around 1
            x = (np.sign(rng.standard_normal(n)) * 10.0 ** rng.uniform(-1, 1, n)).astype(np.float32)
        f = os.path.join(td, "in%d.f32" % i)
        x.tofile(f)
        ins.append(f)
    s = hexs(scalars(rng, name)[:max(ns, 0)] if ns else [])
    og, op = os.path.join(td, "g.f32"), os.path.join(td, "p.f32")
    run([NVH, "elem", name, str(n), og, *s, *ins])
    run([port, "elem", name, str(n), op, *s, *ins])
    bad = compare([og], [op])
    if bad and os.environ.get("NV6_KEEP"):   # keep the failing case for inspection
        import shutil
        dst = os.environ["NV6_KEEP"]
        os.makedirs(dst, exist_ok=True)
        for f in ins + [og, op]:
            shutil.copy(f, dst)
        open(os.path.join(dst, "cmd.txt"), "w").write(" ".join(["elem", name, str(n), *s]) + "\n")
    return bad


def check_pairw(rng, td, n, port=PORT):
    clear(td)
    ins = []
    for i in range(3):
        f = os.path.join(td, "in%d.f32" % i)
        values(rng, n).tofile(f)
        ins.append(f)
    og, op = os.path.join(td, "g.f32"), os.path.join(td, "p.f32")
    run([NVH, "pairw", *ins, str(n), og])
    run([port, "elem", "pairw", str(n), op, "0", "0", "0", "0", *ins])
    return compare([og], [op])


def check_argmax(rng, td, n):
    clear(td)
    count = int(rng.integers(0, 17))
    stride = n + int(rng.integers(0, 5))
    maps = values(rng, max(count * stride, 1), 0.1)
    if rng.random() < 0.5:
        maps = np.abs(maps)
    f = os.path.join(td, "maps.f32")
    maps.tofile(f)
    g, p = os.path.join(td, "g"), os.path.join(td, "p")
    run([NVH, "argmax", str(n), str(count), str(stride), f, g])
    run([PORT, "argmax", str(n), str(count), str(stride), f, p])
    return compare([g + ".idx", g + ".max.f32"], [p + ".idx", p + ".max.f32"])


def check_gather(rng, td, n):
    clear(td)
    stride = n + int(rng.integers(0, 5))
    idx = rng.integers(0, 256, n).astype(np.uint8)
    maps = values(rng, 256 * stride, 0.2)
    fi, fm = os.path.join(td, "idx.u8"), os.path.join(td, "maps.f32")
    idx.tofile(fi); maps.tofile(fm)
    g, p = os.path.join(td, "g.f32"), os.path.join(td, "p.f32")
    run([NVH, "gather", str(n), str(stride), fi, fm, g])
    run([PORT, "gather", str(n), str(stride), fi, fm, p])
    return compare([g], [p])


def check_sel2(rng, td, n):
    clear(td)
    stride = n + int(rng.integers(0, 5))
    flag = int(rng.integers(0, 2))
    arrs = {"a": values(rng, n), "b": values(rng, n), "best": values(rng, n),
            "src": values(rng, 256 * stride, 0.1), "wt": values(rng, 256 * stride, 0.1)}
    nd = int(rng.integers(1, 17))
    i3 = rng.integers(0, nd, n).astype(np.uint8)
    i4 = rng.integers(0, nd, n).astype(np.uint8)
    fs = {}
    for k, v in arrs.items():
        fs[k] = os.path.join(td, k + ".f32"); v.tofile(fs[k])
    fs["i3"], fs["i4"] = os.path.join(td, "i3.u8"), os.path.join(td, "i4.u8")
    i3.tofile(fs["i3"]); i4.tofile(fs["i4"])
    g, p = os.path.join(td, "g"), os.path.join(td, "p")
    args = [str(n), str(flag), str(stride), fs["a"], fs["b"], fs["i3"], fs["i4"], fs["best"], fs["src"], fs["wt"]]
    run([NVH, "sel2", *args, g, "full"])
    run([PORT, "sel2", *args, p, "full"])
    sfx = [".out.f32", ".best.f32", ".b.f32", ".i4"]
    return compare([g + s for s in sfx], [p + s for s in sfx])


def check_strip3(rng, td):
    clear(td)
    W, H = int(rng.integers(4, 200)), int(rng.integers(2, 200))
    if rng.random() < 0.1:
        W, H = int(rng.integers(4, 513)), int(rng.integers(2, 513))
    kind = "sq" if rng.random() < 0.5 else "box"
    mode = int(rng.integers(0, 2))
    s = hexs([scalar(rng, "pos") if kind == "sq" else np.float32(0)])[0]
    f = os.path.join(td, "x.f32")
    values(rng, W * H, 0.05).tofile(f)
    g, p = os.path.join(td, "g.f32"), os.path.join(td, "p.f32")
    run([NVH, "strip3", str(W), str(H), f, kind, str(mode), s, g])
    run([PORT, "strip3", str(W), str(H), f, kind, str(mode), s, p])
    return compare([g], [p]), W * H


def check_down2(rng, td):
    clear(td)
    W, H = int(rng.integers(2, 300)), int(rng.integers(2, 300))
    f = os.path.join(td, "x.f32")
    values(rng, W * H, 0.05).tofile(f)
    g, p = os.path.join(td, "g.f32"), os.path.join(td, "p.f32")
    run([NVH, "down2", str(W), str(H), f, g])
    run([PORT, "down2", str(W), str(H), f, p])
    return compare([g], [p]), (W >> 1) * (H >> 1)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seed", type=int, default=20261103)
    ap.add_argument("--big", type=int, default=6)
    ap.add_argument("--small", type=int, default=300)
    ap.add_argument("--only", default="")
    ap.add_argument("--mutants", action="store_true")
    ap.add_argument("--log", default="")
    a = ap.parse_args()
    log = open(a.log, "w") if a.log else None

    def say(msg):
        print(msg, flush=True)
        if log:
            log.write(msg + "\n"); log.flush()

    r = subprocess.run([NVH, "selftest"], capture_output=True, text=True, env=ENV)
    if r.returncode != 3:
        sys.exit("nvh selftest exit %d (want 3)" % r.returncode)
    rng = np.random.default_rng(a.seed)
    only = set(a.only.split(",")) if a.only else None
    total_bad = 0
    t0 = time.time()
    with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR")) as td:
        if a.mutants:
            for k, leaf in MUTANT_OF.items():
                subprocess.run(["make", "-s", "-C", os.path.join(ROOT, "native"), "nv_spatial6_mut", "MUT=%d" % k],
                               check=True, capture_output=True)
                port = os.path.join(ROOT, "native", "nv_spatial6_mut%d" % k)
                bad = calls = 0
                while calls < 400 and bad == 0:
                    n = int(rng.integers(1, 41)) if calls % 2 else int(rng.integers(100, 5000))
                    bad += check_elem(rng, td, leaf, n, port)
                    calls += 1
                say("mutant %d (%s): %s after %d calls (%d differing words)" % (k, leaf, "DETECTED" if bad else "NOT detected", calls, bad))
                os.remove(port)
            return
        names = [x for x in ELEM if not only or x in only] + [x for x in ("pairw", "argmax", "gather", "sel2", "strip3", "down2") if not only or x in only]
        for name in names:
            bad = elems = calls = 0
            if name in ("strip3", "down2"):
                for _ in range(a.small // 2):
                    b, m = (check_strip3 if name == "strip3" else check_down2)(rng, td)
                    bad += b; elems += m; calls += 1
            else:
                sizes = [int(rng.integers(2000, 120001)) for _ in range(a.big)] + [int(rng.integers(1, 41)) for _ in range(a.small)]
                if name in ("gather", "sel2"):
                    sizes = [min(s, 20000) for s in sizes]
                for n in sizes:
                    if name in ELEM:
                        b = check_elem(rng, td, name, n)
                    elif name == "pairw":
                        b = check_pairw(rng, td, n)
                    elif name == "argmax":
                        b = check_argmax(rng, td, n)
                    elif name == "gather":
                        b = check_gather(rng, td, n)
                    else:
                        b = check_sel2(rng, td, n)
                    bad += b; elems += n; calls += 1
            total_bad += bad
            say("%-9s calls %4d  elements %8d  tail calls %4d  mismatching words %d" %
                (name, calls, elems, sum(1 for _ in range(a.small)) if name not in ("strip3", "down2") else 0, bad))
    say("TOTAL mismatching words %d (%.0f s)" % (total_bad, time.time() - t0))
    sys.exit(1 if total_bad else 0)


if __name__ == "__main__":
    main()
