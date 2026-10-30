#!/usr/bin/env python3
"""Bit-exact check of work/dirfilt/src/nv_model.py (FUN_00995250, all 4096 words of the struct that
`nvh model W H plane [set]` dumps) against the genuine, through the guarded harness only, on the case list of
cases.py (real crops, synthetic planes, size boundaries, fuzz).

usage: verify_model_cases.py [--groups real,synth,bound,fuzz] [--set S] [--sets] [--jobs 6] [--limit N]
                             [--every K] [--match REGEX] [--mutate NAME] [--ref] [--log FILE] [--list]
  --sets        also run the per-set entry (`nvh model ... s` for s = 0..4) on every 8th case
  --mutate      inject a deliberate error into the port (sensitivity test, see MUTATIONS); the genuine results
                come from the cache of a normal run (tmp/gcache)
  --ref         use the reference (slow) paths of the special part instead of the fast ones
Every compared word is the bit pattern (third column of `nvh model`), so -0, NaN payloads, the init pattern
and the unused words up to 0x4000 bytes are compared.  `nvh selftest` must exit 3 before the first genuine
call (common.require_guard).  Per case one JSON line is written (status, differing words, branch counters,
times); the summary goes to <log>_summary.json."""
import argparse
import hashlib
import json
import os
import re
import sys
import time
import traceback
from concurrent.futures import ProcessPoolExecutor

sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import numpy as np  # noqa: E402

import common  # noqa: E402

GCACHE = os.path.join(common.TMP, "gcache")
F32 = np.float32


# ------------------------------------------------------------------------------------------ mutations
def _mut_add_order():
    import nv_model as nm
    import nv_model_special as sp

    def box3_fused(X):
        X = np.ascontiguousarray(X, F32)
        H, W = X.shape
        z = np.zeros((H, 1), F32)
        L = np.concatenate([z, X[:, :-1]], 1)
        R = np.concatenate([X[:, 1:], z], 1)
        h = nm.fadd(X, nm.fadd(L, R))                         # x + (left + right) instead of (x + left) + right
        out = np.empty_like(h)
        zero = np.zeros(W, F32)
        out[0] = nm.fadd(nm.fadd(h[0], zero), h[1])
        out[1:H - 1] = nm.fadd(nm.fadd(h[1:H - 1], h[0:H - 2]), h[2:H])
        out[H - 1] = nm.fadd(zero, nm.fadd(h[H - 1], h[H - 2]))
        return nm.fmul(out, sp.C_NINTH)
    sp.box3_fused = box3_fused


def _mut_t_1ulp():
    import nv_model as nm
    orig = nm.robust_weight

    def rw(t, a, e):
        return orig(F32(np.nextafter(F32(t), F32(np.inf))), a, e)   # stands for a regrouped product (1 ulp)
    nm.robust_weight = rw


def _mut_fma_blend():
    import nv_model as nm

    def blend_add(c, d, A, B):
        A = np.ascontiguousarray(A, F32).ravel()
        B = np.ascontiguousarray(B, F32).ravel()
        return nm.fadd(nm.fadd(A, nm.fmul(B, F32(c))), F32(d))       # (A + B*c) + d everywhere
    nm.blend_add = blend_add


def _mut_fma_std():
    import nv_model as nm
    import nv_model_special as sp

    def std(B, m):
        B = np.asarray(B, F32)
        m = np.asarray(m, F32)[..., None]
        acc = np.zeros(B.shape[:-2] + (4,), F32)
        for r in range(32):
            for c in range(8):
                d = nm.fsub(B[..., r, 4 * c:4 * c + 4], m)
                acc = nm.fadd(nm.fmul(d, d), acc)                  # unfused
        s = nm.fadd(nm.fadd(acc[..., 0], acc[..., 1]), nm.fadd(acc[..., 2], acc[..., 3]))
        return nm.sqrt_abs(nm.fmul(s, sp.C_2M10))
    sp.block_std32 = std
    sp.block_std32_fast = std


def _mut_fma_aggr():
    import nv_model as nm
    import nv_model_bm as bm
    bm.FAST = False
    bm._fma = lambda a, b, c: nm.fadd(nm.fmul(a, b), c)


def _mut_drop_dir():
    import nv_model as nm
    orig = nm.argmax_dirs

    def am(E):
        return orig(E[:-1]) if E.shape[0] > 1 else orig(E)
    nm.argmax_dirs = am


def _mut_border_1px():
    import nv_model as nm
    orig = nm.crop

    def crop(E, bx, by):
        return orig(E, bx + 1, by) if E.shape[1] > 2 * bx + 2 else orig(E, bx, by)
    nm.crop = crop


def _mut_q_1ulp():
    import nv_model as nm
    nm.Q99 = np.nextafter(nm.Q99, F32(1.0)).astype(F32)


def _mut_argmax_gt():
    import nv_model as nm

    def am(E):
        cnt, n = E.shape
        best = np.zeros(n, F32)
        idx = np.zeros(n, np.uint8)
        for j in range(cnt):
            upd = E[j] > best                                         # > instead of >= (the earlier index wins ties)
            best = np.where(upd, E[j], best).astype(F32)
            idx = np.where(upd, np.uint8(j), idx)
        return idx, best
    nm.argmax_dirs = am


def _mut_pad_symmetric():
    import nv_model_bm as bm
    bm.pad_mirror = lambda X, p: np.pad(X, p, mode="symmetric")


def _mut_u8_floor():
    import nv_model as nm
    import nv_model_special as sp

    def to_u8_rows(A, scale, offset):
        y = nm.fadd(nm.fmul(np.asarray(A, F32), scale), offset)
        r = np.floor(y.astype(np.float64))
        return np.clip(np.where(np.isnan(r), 0, r), 0, 255).astype(np.uint8)
    sp.to_u8_rows = to_u8_rows


def _mut_sel_last():
    import nv_model_special as sp
    orig = sp.select_greedy

    def sel(costs, cand, nsel=12, sup=2, r=24):
        c = np.asarray(costs, np.int64)
        return orig(c[::-1], cand[::-1], nsel, sup, r)                 # the last minimum wins ties
    sp.select_greedy = sel


def _mut_level_c():
    import nv_model as nm
    nm.LEVEL_C = nm.LEVEL_C.copy()
    nm.LEVEL_C[0] = F32(1.0)                                         # DAT_012445a8[0] = 1.06 -> 1.0


def _mut_recip_div():
    import nv_model as nm

    def rw(t, a, e):
        a = np.ascontiguousarray(a, F32).ravel()
        e = np.ascontiguousarray(e, F32).ravel()
        inv = F32(F32(1.0) / F32(t))
        z = nm.fmul(inv, e)
        with np.errstate(all="ignore"):
            d = (a / nm.fmul(z, z)).astype(F32)                    # true division instead of FRECPE + FRECPS
        return np.where(np.abs(z) > F32(1.0), d, a).astype(F32)
    nm.robust_weight = rw


MUTATIONS = {
    "add_order": ("addition order: (x + left) + right -> x + (left + right) in FUN_009ed014", _mut_add_order),
    "t_1ulp": ("threshold of FUN_009ef0d0 one ulp higher (a regrouped product)", _mut_t_1ulp),
    "fma_blend": ("FUN_009ed978 NEON part unfused (FMLA -> fmul + fadd)", _mut_fma_blend),
    "fma_std": ("FUN_009ef664 unfused (FMLA -> fmul + fadd)", _mut_fma_std),
    "fma_aggr": ("FUN_009f97c8 aggregation unfused", _mut_fma_aggr),
    "drop_dir": ("the last direction left out of the argmax (FUN_009ee5fc)", _mut_drop_dir),
    "border_1px": ("crop border of the direction measures shifted by 1 px", _mut_border_1px),
    "q_1ulp": ("quantile q 0.99 one ulp higher", _mut_q_1ulp),
    "argmax_gt": ("argmax tie rule >= -> >", _mut_argmax_gt),
    "pad_symmetric": ("FUN_0097ef28 mirror padding with the edge repeated", _mut_pad_symmetric),
    "u8_floor": ("FUN_009f1110 rounding: floor instead of round-half-even", _mut_u8_floor),
    "sel_last": ("FUN_00ad0b88 greedy selection: last instead of first minimum", _mut_sel_last),
    "level_c": ("DAT_012445a8[0] = 1.06 replaced by 1.0", _mut_level_c),
    "recip_div": ("FUN_009ef0d0 NEON part: true division instead of FRECPE + FRECPS", _mut_recip_div),
}


# ------------------------------------------------------------------------------------------ worker
_CASES = None
_OPTS = {}


def _init(opts):
    global _CASES, _OPTS
    _OPTS = opts
    import warnings
    warnings.simplefilter("ignore")
    import cases
    _CASES = cases.all_cases()
    import nv_model_special as sp
    import nv_model_bm as bm
    if opts.get("ref"):
        sp.FAST = False
        bm.FAST = False
    if opts.get("mutate"):
        MUTATIONS[opts["mutate"]][1]()


def _gkey(plane, set_):
    h = hashlib.sha1(np.ascontiguousarray(plane, F32).tobytes()).hexdigest()
    return os.path.join(GCACHE, f"{h}_{plane.shape[1]}x{plane.shape[0]}_s{set_}.npy")


def genuine_cached(plane, set_, allow_run=True):
    os.makedirs(GCACHE, exist_ok=True)
    p = _gkey(plane, set_)
    if os.path.exists(p):
        return np.load(p)
    if not allow_run:
        raise RuntimeError("genuine result not cached (run without --mutate first)")
    g = common.genuine_model(plane, None if set_ < 0 else set_)
    np.save(p, g)
    return g


def work(job):
    i, set_ = job
    name, group, build = _CASES[i]
    import nv_cov
    import nv_model as nm
    rec = dict(i=i, name=name, group=group, set=set_)
    try:
        plane = build()
        rec["shape"] = list(plane.shape)
        rec["nan"] = int(np.isnan(plane).sum())
        rec["inf"] = int(np.isinf(plane).sum())
        t0 = time.time()
        try:
            g = genuine_cached(plane, set_, allow_run=not _OPTS.get("mutate"))
        except common.Rejected as e:
            rec["status"] = "rejected"
            rec["msg"] = str(e)
            return rec
        rec["t_gen"] = time.time() - t0
        nv_cov.reset()
        t0 = time.time()
        try:
            m = nm.model_full(plane, set_)
        except NotImplementedError as e:
            rec["status"] = "port_notimpl"
            rec["msg"] = str(e)
            rec["cov"] = nv_cov.snapshot()
            return rec
        rec["t_port"] = time.time() - t0
        rec["cov"] = nv_cov.snapshot()
        d = np.nonzero(g != m)[0]
        rec["ndiff"] = int(d.size)
        rec["diff_words"] = [int(x) for x in d[:20]]
        rec["status"] = "ok" if d.size == 0 else "DIFF"
        rec["written"] = int(np.count_nonzero(g != nm.nb.init_struct()))
    except Exception as e:                                       # noqa: BLE001
        rec["status"] = "error"
        rec["msg"] = f"{type(e).__name__}: {e}"
        rec["tb"] = traceback.format_exc()[-1500:]
    return rec


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--groups", default="real,synth,bound,fuzz")
    ap.add_argument("--set", type=int, default=-1)
    ap.add_argument("--sets", action="store_true")
    ap.add_argument("--jobs", type=int, default=6)
    ap.add_argument("--limit", type=int, default=0)
    ap.add_argument("--every", type=int, default=1)
    ap.add_argument("--match", default="")
    ap.add_argument("--mutate", default="")
    ap.add_argument("--ref", action="store_true")
    ap.add_argument("--log", default="")
    ap.add_argument("--list", action="store_true")
    a = ap.parse_args()
    if a.jobs > 6:
        raise SystemExit("at most 6 genuine processes in parallel")
    import cases
    allc = cases.all_cases()
    groups = set(a.groups.split(","))
    idx = [i for i, c in enumerate(allc) if c[1] in groups and (not a.match or re.search(a.match, c[0]))]
    idx = idx[::a.every]
    if a.limit:
        idx = idx[:a.limit]
    if a.list:
        for i in idx:
            print(i, allc[i][0])
        return
    common.require_guard()
    jobs = [(i, a.set) for i in idx]
    if a.sets:
        jobs += [(i, s) for i in idx[::8] for s in range(5)]
    log = a.log or os.path.join(common.ROOT, "logs", f"model_cases_{a.mutate or ('ref' if a.ref else 'fast')}_"
                                                     f"{time.strftime('%Y%m%d_%H%M%S')}.jsonl")
    os.makedirs(os.path.dirname(log), exist_ok=True)
    opts = dict(mutate=a.mutate, ref=a.ref)
    t0 = time.time()
    tot = dict()
    cov = {}
    words = 0
    with open(log, "w") as f, ProcessPoolExecutor(a.jobs, initializer=_init, initargs=(opts,)) as ex:
        for rec in ex.map(work, jobs, chunksize=1):
            f.write(json.dumps(rec) + "\n")
            f.flush()
            tot[rec["status"]] = tot.get(rec["status"], 0) + 1
            if rec["status"] in ("ok", "DIFF"):
                words += 4096
            for k, v in rec.get("cov", {}).items():
                cov[k] = cov.get(k, 0) + v
            if rec["status"] != "ok" and not a.mutate:
                print(f"[{rec['status']}] {rec['name']} set {rec['set']} {rec.get('shape')} "
                      f"ndiff={rec.get('ndiff')} {rec.get('msg', '')[:200]} {rec.get('diff_words', '')}", flush=True)
    summ = dict(log=log, jobs=len(jobs), status=tot, words_compared=words, seconds=round(time.time() - t0, 1),
                mutate=a.mutate, mutation=MUTATIONS.get(a.mutate, ("",))[0], cov=dict(sorted(cov.items())))
    with open(log.replace(".jsonl", "_summary.json"), "w") as f:
        json.dump(summ, f, indent=1)
    print(json.dumps({k: summ[k] for k in ("log", "jobs", "status", "words_compared", "seconds", "mutate")}))


if __name__ == "__main__":
    main()
