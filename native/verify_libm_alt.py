# W2a: the libm sensitivity harness.  How far do the outputs of the C++ ports move when the system libm
# (libSystem: pow, tanf, atan2, __sincos_stret, expf) is replaced by openlibm (alt 1), musl (alt 2) or naive
# formulas (alt 3)?  The default build is the reference: it is bit-exact with Python (make verify), so a word
# the alt build changes is a word that differs from Python.
#
#   make -C native libm-alt && python3 native/verify_libm_alt.py [--out DIR] [--jobs 2] [--stages ...] [--suites ...]
#
# Stages (default: all, in this order; each can be run alone, results accumulate in --out):
#   capture each of the 11 verify scripts runs as usual (it must still pass: the default binaries are
#           unchanged) while a subprocess hook (libm_alt/pyhook/sitecustomize.py) keeps the input files,
#           output files, NV_* environment and stdout of every native call.
#   replay  every captured call is run again with the alt build at NV_LIBM_ALT = 0 (the plumbing check:
#           the system libm through libm_alt, must equal the default output), 1, 2 and 3; the differing
#           4-byte words of every output are recorded (replay.jsonl, diff_*.npz).  The NV_LIBM_ALT = 0 run
#           also records the distinct libm arguments (NV_LIBM_LOG).
#   swap    the verify scripts run against the alt build for the (suite, alt) pairs whose replay differs
#           (NV_LIBM_ALT = 1, 2, 3): the scripts' own counts of the cases that still match Python.  The other
#           pairs need no run: their outputs are byte-identical to the default ones.
#   bridge  native/verify_bridge.py with NV_NATIVE_LIB = libm_alt/bin/libnvcore.dylib, NV_LIBM_ALT = 0..3.
#   frames  processed frames end to end (libm_alt/frames_e2e.py: backend.process_frame with the default and the
#           alt library, test_v02 n = 308..318 and test.mp4 n = 300..306): float words and 8 / 10 / 16-bit codes.
#   ulp     libm_ulp: every alt against the system libm on the recorded arguments, and on every float in
#           the recorded ranges of tanf / expf.
#   impact  the differing words mapped to fields (fields.py): FindFlatArea area / status, profile fields and
#           Quality / Completeness %, AFT bins, curve tables, frames.
#   report  summary.json + summary.md in --out.
# Heavy jobs: one verify script at a time (each with --jobs JOBS processes, default 2).
import argparse
import glob
import json
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
ALTBIN = os.path.join(HERE, "libm_alt", "bin")
HOOK = os.path.join(HERE, "libm_alt", "pyhook")
ULP = os.path.join(HERE, "libm_alt", "libm_ulp")
DEFAULT_OUT = os.path.join(ROOT, "work", "omo", "wp_native_libm", "runs")

SUITES = ["quality", "band", "curve", "lzma", "flat", "flat_cancel", "model", "temporal", "spatial", "profile", "aft"]
NOJOBS = {"quality"}                     # verify_quality.py has no --jobs
ALTS = (1, 2, 3)
ALT_NAME = {0: "system (alt build)", 1: "openlibm 0.8.8", 2: "musl 1.2.6", 3: "naive (double)"}
# the success line each script prints when everything matches
PASS = re.compile(r"ALL BIT-EXACT|ALL IDENTICAL|ALL MATCH|\b0 mismatches|\b(\d+)/\1 (bit-exact|byte equal|identical)")


def hook_env(extra):
    e = dict(os.environ)
    e["PYTHONPATH"] = HOOK + (os.pathsep + e["PYTHONPATH"] if e.get("PYTHONPATH") else "")
    e["NV_LIBM_NATIVE"] = HERE
    e.update(extra)
    return e


def script_cmd(suite, jobs):
    cmd = [sys.executable, os.path.join(HERE, f"verify_{suite}.py")]
    if suite not in NOJOBS:
        cmd += ["--jobs", str(jobs)]
    return cmd


def run_logged(cmd, env, log, timeout=None):
    t0 = time.time()
    with open(log, "w") as f:
        r = subprocess.run(cmd, env=env, stdout=f, stderr=subprocess.STDOUT, cwd=HERE, timeout=timeout)
    return r.returncode, time.time() - t0


def tail(path, n=6):
    with open(path, errors="replace") as f:
        return f.read().splitlines()[-n:]


# ------------------------------------------------------------------------------------ capture / replay
def stage_capture(out, suites, jobs):
    for s in suites:
        d = os.path.join(out, "capture", s)
        if os.path.isdir(d):
            shutil.rmtree(d)
        os.makedirs(os.path.join(d, "calls"))
        env = hook_env({"NV_LIBM_HOOK_MODE": "capture", "NV_LIBM_CAPTURE": os.path.join(d, "calls")})
        rc, dt = run_logged(script_cmd(s, jobs), env, os.path.join(d, "verify.log"))
        ok = rc == 0 and any(PASS.search(x) for x in tail(os.path.join(d, "verify.log"), 12))
        ncalls = len(os.listdir(os.path.join(d, "calls")))
        with open(os.path.join(d, "status.json"), "w") as f:
            json.dump({"suite": s, "rc": rc, "seconds": round(dt, 1), "default_pass": ok, "calls": ncalls,
                       "tail": tail(os.path.join(d, "verify.log"))}, f, indent=1)
        print(f"[capture] {s}: rc {rc}, {dt:.0f} s, {ncalls} native calls, default build "
              f"{'passes' if ok else 'FAILS'}: {tail(os.path.join(d, 'verify.log'), 1)}", flush=True)


def _strip_timing(t):
    return None if t is None else re.sub(r"[0-9.]+ ?(s|ms|µs)\b", "<t>", t)


def compare_words(a, b):
    import numpy as np
    n = max(len(a), len(b))
    n += (-n) % 4
    A = np.frombuffer(a + b"\0" * (n - len(a)), dtype="<u4")
    B = np.frombuffer(b + b"\0" * (n - len(b)), dtype="<u4")
    return A, B, np.nonzero(A != B)[0]


def stage_replay(out, suites, alts, budget=None):
    """resumable: calls already in replay.jsonl are skipped; stops after `budget` seconds (rerun to continue)."""
    import numpy as np
    t_start = time.time()
    for s in suites:
        d = os.path.join(out, "capture", s)
        calls = sorted(os.listdir(os.path.join(d, "calls")), key=lambda c: [int(x) for x in c.split("_")])
        rec = os.path.join(d, "replay.jsonl")
        done = set()
        if os.path.exists(rec):
            done = {(r["call"], r["alt"]) for r in load_calls(rec)}
        t0 = time.time()
        for c in calls:
            cd = os.path.join(d, "calls", c)
            with open(os.path.join(cd, "meta.json")) as f:
                meta = json.load(f)
            for a in alts:
                if (c, a) in done:
                    continue
                if budget is not None and time.time() - t_start > budget:
                    print(f"[replay] {s}: time budget reached, rerun to continue; " + summarize_calls(rec), flush=True)
                    return False
                argv = list(meta["argv"])
                for i in meta["inputs"]:
                    argv[i - 1] = os.path.join(cd, f"in_{i}")
                for i in meta["outputs"]:
                    argv[i - 1] = os.path.join(cd, f"alt{a}_{i}")
                env = dict(os.environ)
                for k in [k for k in env if k.startswith("NV_")]:
                    env.pop(k)
                env.update(meta["env"])
                env["NV_LIBM_ALT"] = str(a)
                if a == 0:
                    env["NV_LIBM_LOG"] = os.path.join(d, "args")
                    os.makedirs(env["NV_LIBM_LOG"], exist_ok=True)
                r = subprocess.run([os.path.join(ALTBIN, meta["bin"])] + argv, env=env, capture_output=True, text=True,
                                   cwd=meta["cwd"])
                entry = {"call": c, "bin": meta["bin"], "argv0": meta["argv"][0] if meta["argv"] else "", "alt": a,
                         "rc_def": meta["rc"], "rc_alt": r.returncode, "outputs": []}
                if meta["stdout"] is not None:
                    entry["stdout_equal"] = _strip_timing(meta["stdout"]) == _strip_timing(r.stdout)
                    if not entry["stdout_equal"]:
                        entry["stdout_def"] = meta["stdout"][-3000:]
                        entry["stdout_alt"] = r.stdout[-3000:]
                if r.returncode != meta["rc"]:
                    entry["stderr_alt"] = r.stderr[-2000:]
                for i in meta["outputs"]:
                    pd, pa = os.path.join(cd, f"def_{i}"), os.path.join(cd, f"alt{a}_{i}")
                    with open(pd, "rb") as f:
                        bd = f.read()
                    ba = b""
                    if os.path.exists(pa):
                        with open(pa, "rb") as f:
                            ba = f.read()
                    A, B, idx = compare_words(bd, ba)
                    o = {"arg": i, "bytes_def": len(bd), "bytes_alt": len(ba), "words": int(A.size),
                         "diff_words": int(idx.size)}
                    if idx.size:
                        k = min(idx.size, 4_000_000)
                        np.savez_compressed(os.path.join(cd, f"diff_alt{a}_{i}.npz"), idx=idx[:k].astype(np.uint64),
                                            a=A[idx[:k]], b=B[idx[:k]], truncated=idx.size > k)
                        o["first"] = [int(x) for x in idx[:8]]
                        if len(ba) > 64 << 20:
                            os.remove(pa)
                            o["kept"] = False
                        else:
                            o["kept"] = True
                    elif os.path.exists(pa):
                        os.remove(pa)
                    entry["outputs"].append(o)
                with open(rec, "a") as f:
                    f.write(json.dumps(entry) + "\n")
        print(f"[replay] {s}: {time.time() - t0:.0f} s; " + summarize_calls(rec), flush=True)
    return True


def load_calls(path):
    if not os.path.exists(path):
        return []
    with open(path) as f:
        return [json.loads(x) for x in f if x.strip()]


def summarize_calls(path):
    rows = load_calls(path)
    by = {}
    for r in rows:
        b = by.setdefault(r["alt"], [0, 0, 0, 0])
        b[0] += 1
        w = sum(o["words"] for o in r["outputs"])
        dw = sum(o["diff_words"] for o in r["outputs"])
        b[1] += dw > 0 or r["rc_alt"] != r["rc_def"] or r.get("stdout_equal") is False
        b[2] += w
        b[3] += dw
    return ", ".join(f"alt{a}: {v[1]}/{v[0]} calls differ, {v[3]}/{v[2]} words" for a, v in sorted(by.items())) or "no calls"


def sensitive_suites(out):
    """{suite: [alts]} of the (suite, alt) pairs whose replay differs from the default somewhere."""
    res = {}
    for s in SUITES:
        for r in load_calls(os.path.join(out, "capture", s, "replay.jsonl")):
            if r["alt"] > 0 and (sum(o["diff_words"] for o in r["outputs"]) or r["rc_alt"] != r["rc_def"]
                                 or r.get("stdout_equal") is False):
                res.setdefault(s, set()).add(r["alt"])
    return {s: sorted(a) for s, a in res.items()}


# -------------------------------------------------------------------------------------------- swap
def stage_swap(out, pairs, jobs):
    """pairs: {suite: [alts]}; the pairs whose outputs are identical to the default need no run (the script
    would see the default outputs)."""
    for s, alts in pairs.items():
        for a in alts:
            d = os.path.join(out, "swap", s)
            os.makedirs(d, exist_ok=True)
            env = hook_env({"NV_LIBM_HOOK_MODE": "swap", "NV_LIBM_HOOK_ALT": str(a)})
            log = os.path.join(d, f"alt{a}.log")
            rc, dt = run_logged(script_cmd(s, jobs), env, log)
            with open(os.path.join(d, f"alt{a}.json"), "w") as f:
                json.dump({"suite": s, "alt": a, "rc": rc, "seconds": round(dt, 1), "tail": tail(log, 12)}, f, indent=1)
            print(f"[swap] {s} alt{a}: rc {rc}, {dt:.0f} s: {' | '.join(tail(log, 2))}", flush=True)


# ------------------------------------------------------------------------------------------ bridge
def stage_bridge(out, jobs, alts):
    d = os.path.join(out, "bridge")
    os.makedirs(d, exist_ok=True)
    seed = os.path.join(d, "frames_seed")
    for a in alts:
        tmp = os.path.join(d, f"tmp_alt{a}")       # own cache per alt (the AFT profiles are cached there)
        if os.path.isdir(tmp):
            shutil.rmtree(tmp)
        os.makedirs(tmp)
        for p in glob.glob(os.path.join(seed, "frames_*.npz")):
            shutil.copy(p, tmp)
        extra = {"NV_NATIVE_LIB": os.path.join(ALTBIN, "libnvcore.dylib"), "NV_LIBM_ALT": str(a)}
        if a == 0:
            extra["NV_LIBM_LOG"] = os.path.join(d, "args")
            os.makedirs(extra["NV_LIBM_LOG"], exist_ok=True)
        env = dict(os.environ)
        env.update(extra)
        log = os.path.join(d, f"alt{a}.log")
        rc, dt = run_logged([sys.executable, os.path.join(HERE, "verify_bridge.py"), "--jobs", str(jobs), "--tmp", tmp],
                            env, log)
        if not os.path.isdir(seed):
            os.makedirs(seed)
            for p in glob.glob(os.path.join(tmp, "frames_*.npz")):
                shutil.copy(p, seed)
        with open(os.path.join(d, f"alt{a}.json"), "w") as f:
            json.dump({"alt": a, "rc": rc, "seconds": round(dt, 1), "tail": tail(log, 12)}, f, indent=1)
        print(f"[bridge] alt{a}: rc {rc}, {dt:.0f} s: {tail(log, 1)}", flush=True)


# --------------------------------------------------------------------------------------------- ulp
def stage_ulp(out):
    logs = sorted(glob.glob(os.path.join(out, "capture", "*", "args", "libm_args_*.bin")) +
                  glob.glob(os.path.join(out, "bridge", "args", "libm_args_*.bin")))
    d = os.path.join(out, "ulp")
    os.makedirs(d, exist_ok=True)
    rows = []
    if logs:
        r = subprocess.run([ULP, "args"] + logs, capture_output=True, text=True, check=True)
        rows += [json.loads(x) for x in r.stdout.splitlines() if x.strip()]
    # per suite (which arguments each suite sees)
    per = {}
    for s in SUITES + ["bridge"]:
        ls = sorted(glob.glob(os.path.join(out, "capture" if s != "bridge" else "", s, "args", "libm_args_*.bin")))
        if ls:
            r = subprocess.run([ULP, "args"] + ls, capture_output=True, text=True, check=True)
            per[s] = [json.loads(x) for x in r.stdout.splitlines() if x.strip()]
    # every float in the recorded ranges of the float functions
    for f in ("tanf", "expf"):
        rng = [x for x in rows if x["func"] == f and x["alt"] == 1 and x["n"]]
        if rng:
            lo, hi = float.fromhex(rng[0]["arg_min"]), float.fromhex(rng[0]["arg_max"])
            r = subprocess.run([ULP, "sweep", f, repr(lo), repr(hi), "-j", "2"], capture_output=True, text=True, check=True)
            rows += [json.loads(x) for x in r.stdout.splitlines() if x.strip()]
    with open(os.path.join(d, "ulp.json"), "w") as fo:
        json.dump({"all": rows, "per_suite": per}, fo, indent=1)
    for x in rows:
        if "hist" in x:
            print(f"[ulp] {x['func']:12s} alt{x['alt']}: {x['bitdiff']}/{x['n']} differ, max {x['max_ulp']} ulp "
                  f"(rel {x['max_rel']:.3g}){'  calls ' + str(x['calls']) if 'calls' in x else ''}")
        else:
            print(f"[ulp] {x}")


# ------------------------------------------------------------------------------------------ frames
def stage_frames(out):
    tmp = os.path.join(out, "bridge", "tmp_alt0")
    if not os.path.exists(os.path.join(tmp, "aft_v02_312.json")):
        sys.exit("frames: run the bridge stage with alt 0 first (it makes the frame / AFT profile caches)")
    subprocess.run([sys.executable, os.path.join(HERE, "libm_alt", "frames_e2e.py"), out, tmp], check=True)


# ------------------------------------------------------------------------------------------ impact
def stage_impact(out):
    sys.path.insert(0, os.path.join(HERE, "libm_alt"))
    import fields   # noqa: E402  (native/libm_alt/fields.py)
    res = fields.analyze(out)
    with open(os.path.join(out, "impact.json"), "w") as f:
        json.dump(res, f, indent=1)
    fields.print_summary(res)


# ------------------------------------------------------------------------------------------ report
def stage_report(out):
    sys.path.insert(0, os.path.join(HERE, "libm_alt"))
    import fields   # noqa: E402
    fields.write_report(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=DEFAULT_OUT)
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--stages", default="capture,replay,swap,bridge,frames,ulp,impact,report")
    ap.add_argument("--suites", default=",".join(SUITES))
    ap.add_argument("--budget", type=float, default=None, help="replay: stop after this many seconds (resumable)")
    ap.add_argument("--alts", default="0,1,2,3", help="alts of the replay / swap / bridge stages (swap skips 0)")
    A = ap.parse_args()
    out = os.path.abspath(A.out)
    os.makedirs(out, exist_ok=True)
    if not os.path.exists(os.path.join(ALTBIN, "libnvcore.dylib")) or not os.path.exists(ULP):
        sys.exit("libm_alt build missing: make -C native libm-alt")
    stages = A.stages.split(",")
    suites = [s for s in A.suites.split(",") if s]
    alts = [int(x) for x in A.alts.split(",")]
    t0 = time.time()
    if "capture" in stages:
        stage_capture(out, suites, A.jobs)
    if "replay" in stages:
        if not stage_replay(out, suites, alts, A.budget):
            return
    if "swap" in stages:
        sens = {s: [a for a in v if a in alts] for s, v in sensitive_suites(out).items() if s in suites}
        sens = {s: v for s, v in sens.items() if v}
        print(f"[swap] (suite, alts) with differing outputs (replay): {sens}", flush=True)
        stage_swap(out, sens, A.jobs)
    if "bridge" in stages:
        stage_bridge(out, A.jobs, alts)
    if "frames" in stages:
        stage_frames(out)
    if "ulp" in stages:
        stage_ulp(out)
    if "impact" in stages:
        stage_impact(out)
    if "report" in stages:
        stage_report(out)
    print(f"done in {time.time() - t0:.0f} s ({out})")


if __name__ == "__main__":
    main()
