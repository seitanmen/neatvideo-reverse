#!/usr/bin/env python3
"""Statistics of the c90c variant logs (c90c_<pid>.jsonl: one record per frame, neighbour offset and channel).

    python3 c90c_stats.py LOGDIR [--out stats.json]
Each output frame is processed once, so a (frame, off, ch) record appears once (duplicates are dropped)."""
import argparse
import glob
import json
import os

import numpy as np

ap = argparse.ArgumentParser()
ap.add_argument("logdir")
ap.add_argument("--out", default=None)
a = ap.parse_args()
recs = {}
for f in glob.glob(os.path.join(a.logdir, "c90c_*.jsonl")):
    for line in open(f):
        r = json.loads(line)
        recs[(r["frame"], r["off"], r["ch"])] = r
res = {}
for ch, name in enumerate(("Y", "Cb", "Cr")):
    rs = [r for k, r in recs.items() if k[2] == ch]
    if not rs:
        continue
    A = np.array([r["a"] for r in rs]); B = np.array([r["b"] for r in rs])
    V = np.array([r["valid"] for r in rs]); app = np.array([r["applied"] for r in rs])
    d2 = np.array([r["d2"] for r in rs]); d3 = np.array([r["d3"] for r in rs])
    fb = np.array([r["valid"] <= 30.0 for r in rs])
    pct = lambda x: [float(np.percentile(x, q)) for q in (0, 5, 50, 95, 100)]
    res[name] = {"pairs": len(rs), "applied": int(app.sum()), "fallback_validity<=30": int(fb.sum()),
                 "a_clamped": int(((A <= float(np.float32(0.66))) | (A >= 1.5)).sum()), "b_clamped": int((np.abs(B) >= 64).sum()),
                 "a_pct_0_5_50_95_100": pct(A), "b_pct": pct(B), "valid_pct": pct(V),
                 "abs_a-1_mean": float(np.abs(A - 1).mean()), "abs_b_mean": float(np.abs(B).mean()),
                 "d2_mean": float(d2.mean()), "d3_mean": float(d3.mean()),
                 "by_offset": {str(o): {"a_med": float(np.median([r["a"] for r in rs if r["off"] == o])),
                                        "b_med": float(np.median([r["b"] for r in rs if r["off"] == o])),
                                        "applied": int(sum(r["applied"] for r in rs if r["off"] == o)),
                                        "n": sum(1 for r in rs if r["off"] == o)}
                               for o in sorted({r["off"] for r in rs})}}
print(json.dumps(res, indent=1))
if a.out:
    json.dump(res, open(a.out, "w"), indent=1)
