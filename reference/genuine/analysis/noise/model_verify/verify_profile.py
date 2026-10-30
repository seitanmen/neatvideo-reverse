#!/usr/bin/env python3
"""BuildNoiseModel (FUN_009af718) part of `nvh profile W H rgb mode out x y w h` (area given: FindFlatArea is not
called) against src/nv_profile.build_profile, word by word over the 0xc300-byte profile object.

usage: verify_profile.py [--quick] [--jobs 6] [--log FILE]
Regions (byte ranges) are reported separately; the part nv_profile cannot reproduce (heap pointers 0x00..0x17) is
listed as such and excluded from the match count (the LZMA sizes 0x18..0x23 are ported since 2026-10-16, nv_lzma;
the blobs themselves are compared by ../lzma_oracle/verify_lzma.py --genuine).
`nvh selftest` must exit 3 before the first genuine call."""
import argparse
import json
import os
import sys
import time
from concurrent.futures import ProcessPoolExecutor

sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import numpy as np  # noqa: E402

import common  # noqa: E402

F32 = np.float32
REGIONS = [("pointers", 0x00, 0x18), ("lzma_sizes", 0x18, 0x24), ("hdr", 0x24, 0x4c),
           ("models", 0x4c, 0xbc44), ("tile_dev", 0xbc44, 0xbea8), ("mc_stats", 0xbea8, 0xbf28),
           ("means_flags", 0xbf28, 0xbf40), ("curve", 0xbf40, 0xc2a4), ("tail", 0xc2a4, 0xc300)]
NOT_PORTED = {"pointers"}


def case_list(quick):
    out = []
    rng = np.random.default_rng(77)
    clips = [("test_v02.mp4", n) for n in (0, 100, 312, 576, 700)] + [("test.mp4", n) for n in (0, 300, 450, 750)]
    sizes = [(128, 128), (64, 64), (96, 96), (200, 150), (48, 48), (160, 90), (97, 71), (256, 256)]
    for ci, (clip, n) in enumerate(clips):
        for si, (w, h) in enumerate(sizes[:3] if quick else sizes):
            mode = (ci + si) % 3
            out.append((clip, n, w, h, mode, int(rng.integers(0, 1 << 30))))
    for i, kind in enumerate(("blown", "black", "bluesat", "noise")):
        out.append((kind, 0, (128, 96, 200, 64)[i], (128, 96, 150, 64)[i], i % 3, 900 + i))
    return out[:6] if quick else out


def synth_only():
    return [c for c in case_list(False) if c[0] in ("blown", "black", "bluesat", "noise")]


def synth_frame(kind, seed):
    """Synthetic 240 x 320 RGB contexts for the clip / curve branches: blown-out sky, black band, flat grey, noise."""
    rng = np.random.default_rng(seed)
    base = rng.normal(0, 3, (240, 320, 3))
    if kind == "blown":
        x = np.clip(250 + base * 4, 0, 255)                      # > 1/3 of every channel at 255
    elif kind == "black":
        x = np.where((np.arange(240) < 120)[:, None, None], 0.0, 60 + base)
    elif kind == "bluesat":
        x = np.stack([40 + base[..., 0], 90 + base[..., 1], np.full((240, 320), 255.0)], -1)
    else:
        x = 128 + base * 10
    return np.clip(x, 0, 255).astype(F32)


def work(c):
    clip, n, w, h, mode, seed = c
    import warnings
    warnings.simplefilter("ignore")
    import nv_profile as npf
    fr = synth_frame(clip, seed) if clip in ("blown", "black", "bluesat", "noise") else \
        common.frame_cached(clip, n).astype(F32)
    H, W = fr.shape[:2]
    # the profile works on a W x H context crop around the area (smaller files, same area semantics)
    rng = np.random.default_rng(seed)
    cw, ch = min(W, w + 160), min(H, h + 120)
    cx, cy = int(rng.integers(0, W - cw + 1)), int(rng.integers(0, H - ch + 1))
    ctx = np.ascontiguousarray(fr[cy:cy + ch, cx:cx + cw])
    ax, ay = int(rng.integers(0, cw - w + 1)), int(rng.integers(0, ch - h + 1))
    rec = dict(clip=clip, n=n, area=[ax, ay, w, h], ctx=[cw, ch], mode=mode)
    t0 = time.time()
    g, line = common.genuine_profile(ctx, mode, (ax, ay, w, h))
    rec["t_gen"] = time.time() - t0
    rec["line"] = line
    t0 = time.time()
    m, missing = npf.build_profile(ctx, mode, (ax, ay, w, h))
    rec["t_port"] = time.time() - t0
    gw, mw = np.frombuffer(g, np.uint32), m.view(np.uint32)
    reg = {}
    for name, a, b in REGIONS:
        d = np.nonzero(gw[a // 4:b // 4] != mw[a // 4:b // 4])[0]
        reg[name] = dict(words=(b - a) // 4, ndiff=int(d.size), first=[hex(a + 4 * int(i)) for i in d[:6]])
    rec["regions"] = reg
    rec["ok"] = all(reg[k]["ndiff"] == 0 for k in reg if k not in NOT_PORTED)
    return rec


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--jobs", type=int, default=6)
    ap.add_argument("--log", default="")
    ap.add_argument("--synth-only", action="store_true")
    a = ap.parse_args()
    common.require_guard()
    cases = synth_only() if a.synth_only else case_list(a.quick)
    log = a.log or os.path.join(common.ROOT, "logs", f"profile_{time.strftime('%Y%m%d_%H%M%S')}.jsonl")
    tot = {}
    with open(log, "w") as f, ProcessPoolExecutor(min(a.jobs, 6)) as ex:
        for rec in ex.map(work, cases):
            f.write(json.dumps(rec) + "\n")
            f.flush()
            for k, v in rec["regions"].items():
                t = tot.setdefault(k, [0, 0, 0])
                t[0] += v["words"]
                t[1] += v["ndiff"]
                t[2] += v["ndiff"] > 0
            print(rec["clip"], rec["n"], rec["area"], "mode", rec["mode"], "ok" if rec["ok"] else "DIFF",
                  {k: v["ndiff"] for k, v in rec["regions"].items() if v["ndiff"]}, flush=True)
    print("totals (words compared, differing words, cases with a difference):")
    for k, (wds, nd, nc) in tot.items():
        print(f"  {k:12s} {wds:8d} {nd:8d} {nc:4d}" + ("   (not ported)" if k in NOT_PORTED else ""))
    print("log", log)


if __name__ == "__main__":
    main()
