# G9: classifies the differences of an x86_64 replay (native/x86_bridge_replay.py --dump) against an arm64 replay
# of the same calls (--select ... --dump-all) into the two known kinds; native/verify_x86_bridge.sh --allow-known.
#
#   (a) nvc_temporal (the v5 temporal stage, also inside the export): only the float32 output planes out / rv
#       (C arguments 16 / 17, role out, 4-byte words); every differing word finite (not NaN, not +-inf) on both
#       sides and MIN_ULP..MAX_ULP apart (libm expf: x86_64 and arm64 differ by 1 ULP on 2,018,697 float inputs;
#       the accepted platform difference of W2c).  MAX_ULP = 8 is the largest difference observed (no margin).
#   (b) nvc_nlc_image: only its float32 output (argument 4); every differing word NaN on both sides (sign /
#       payload of the default NaN)
# Anything else (another function or argument, a uint8 / int buffer, a return value, a cancel-query count, a buffer
# of another size, a NaN or an inf on one side, a finite difference outside the bound, a replay with unspecified
# calls or without the argument indices of its differences) fails.
#
#   python3 native/x86_bridge_known.py --x86 replay_x86.json --x86-dump DIR --arm-dump DIR
import argparse
import json
import os
import sys

import numpy as np

MIN_ULP, MAX_ULP = 1, 8


def ordered(v):
    i = v.view("<i4").astype(np.int64)
    return np.where(i < 0, -(i & 0x7fffffff), i)


# function -> the C arguments whose float32 output buffers may differ
KNOWN_ARGS = {"nvc_temporal": (16, 17), "nvc_nlc_image": (4,)}


def classify(fn, d, a, x):
    """None if the buffer difference d (a diff entry of the x86 replay; a, x: arm64 / x86_64 bytes) is of the
    known kind of fn, else the reason."""
    if fn not in KNOWN_ARGS:
        return "not a known function"
    if d.get("arg") not in KNOWN_ARGS[fn] or d.get("esize") != 4 or d.get("role") != "out":
        return f"buffer arg {d.get('arg')} esize {d.get('esize')} role {d.get('role')} is not a known float32 output"
    if a.size != x.size or a.size % 4 or a.size != d.get("n"):
        return f"size {x.size} vs {a.size}"
    au, xu = a.view("<u4"), x.view("<u4")
    m = au != xu
    if not m.any():
        return "no differing word in a buffer reported as different"
    fa, fx = au.view(np.float32)[m], xu.view(np.float32)[m]
    na, nx = np.isnan(fa), np.isnan(fx)
    if fn == "nvc_temporal":
        if (na | nx).any():
            return f"{int((na | nx).sum())} NaN words differ"
        if not (np.isfinite(fa).all() and np.isfinite(fx).all()):
            return f"{int((~np.isfinite(fa) | ~np.isfinite(fx)).sum())} inf words differ"
        u = np.abs(ordered(au[m]) - ordered(xu[m]))
        if u.min() < MIN_ULP or u.max() > MAX_ULP:
            return f"finite difference of {int(u.min())}..{int(u.max())} ULP (bound {MIN_ULP}..{MAX_ULP})"
        return None
    if fn == "nvc_nlc_image":
        if not (na & nx).all():
            return f"{int((~(na & nx)).sum())} differing words not NaN on both sides"
        return None
    return "not a known function"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--x86", required=True)
    ap.add_argument("--x86-dump", required=True)
    ap.add_argument("--arm-dump", required=True)
    A = ap.parse_args()
    r = json.load(open(A.x86))
    if r.get("unspec"):
        print(f"{r['unspec']} unspecified calls: not classifiable")
        return 1
    if not r["bad"]:
        print("no differences to classify (the replay reported differences): fail")
        return 1
    known = {"nvc_temporal": [0, 0, 0], "nvc_nlc_image": [0, 0, 0]}     # calls, buffers, words
    bad = []
    for b in r["bad"]:
        fn = b["fn"]
        pid, seq = b["call"].split(":")
        reasons = []
        nbuf = nword = 0
        for d in b["diffs"]:
            if not isinstance(d, dict) or "region" not in d:
                reasons.append(f"{d}")
                continue
            name = f"{pid}_{seq}_r{d['region']}.bin"
            px, pa = os.path.join(A.x86_dump, name), os.path.join(A.arm_dump, name)
            if not (os.path.exists(px) and os.path.exists(pa)):
                reasons.append(f"{name}: dump missing")
                continue
            a, x = np.fromfile(pa, np.uint8), np.fromfile(px, np.uint8)
            why = classify(fn, d, a, x)
            if why:
                reasons.append(f"region {d['region']}: {why}")
            else:
                nbuf += 1
                nword += int((a.view("<u4") != x.view("<u4")).sum())
        if reasons or fn not in known:
            bad.append((b["call"], fn, b["label"], reasons or ["not a known function"]))
        else:
            k = known[fn]
            k[0] += 1
            k[1] += nbuf
            k[2] += nword
    print(f"known differences (temporal bound {MIN_ULP}..{MAX_ULP} ULP, finite only; nlc_image NaN bits only):")
    for fn, (c, nb, nw) in known.items():
        print(f"  {fn}: {c} calls, {nb} buffers, {nw} words")
    for call, fn, label, why in bad[:20]:
        print(f"  UNKNOWN {call} {fn} {label}: {why[:3]}")
    print(f"{len(bad)} differences outside the known kinds")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
