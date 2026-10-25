#!/usr/bin/env python3
"""Parse a genuine profile object dumped by `nvh profile` (layout from BuildNoiseModel FUN_009af718).

profile+0x48 : colour mode (uint)
profile+0x4c + i*0x1f54 : per-plane noise model (FUN_00995250 output), planes i = R,G,B, then the
                          3 transformed planes (mode 1: Y, (R-Y)*0.713, (B-Y)*0.565)
   +0x00 mean local RMS per level [5], +0x14 99% [5], +0x28 median [5], +0x3c CV[5][8]
profile+0xbf40 : curve mode (int); +0xbf44 + c*0x120 + bin*0x20 : entry
   {float level, float factor[5], int valid, byte locked, byte clip, byte level>3}
"""
import struct
import sys

import numpy as np


def parse(path):
    b = open(path, "rb").read()
    f = lambda off, n=1: np.frombuffer(b, "<f4", n, off)
    out = {"mode": struct.unpack_from("<I", b, 0x48)[0], "planes": []}
    for i in range(6):
        o = 0x4c + i * 0x1f54
        out["planes"].append({"mean": f(o, 5), "q99": f(o + 0x14, 5), "q50": f(o + 0x28, 5),
                              "cv": f(o + 0x3c, 40).reshape(5, 8)})
    out["curve_mode"] = struct.unpack_from("<i", b, 0xbf40)[0]
    cur = []
    for c in range(3):
        rows = []
        for k in range(9):
            o = 0xbf44 + c * 0x120 + k * 0x20
            lev, *fac = struct.unpack_from("<6f", b, o)
            valid = struct.unpack_from("<i", b, o + 0x18)[0]
            flags = struct.unpack_from("<4B", b, o + 0x1c)
            rows.append((lev, fac, valid, flags))
        cur.append(rows)
    out["curve"] = cur
    out["sizeclass"] = struct.unpack_from("<i", b, 0xbf38)[0]   # min(W, H) class 0..5 (BuildNoiseModel)
    return out


if __name__ == "__main__":
    p = parse(sys.argv[1])
    names = ["R", "G", "B", "T0", "T1", "T2"]
    print("colour mode", p["mode"], " curve mode", p["curve_mode"])
    for n, pl in zip(names, p["planes"]):
        print(f"{n:3s} mean {np.round(pl['mean'], 4)}  q99/q50 {np.round(pl['q99'] / pl['q50'], 3)}")
    for c in range(3):
        print("curve ch", c, [(round(r[0], 3), [round(v, 3) for v in r[1]], r[2], r[3]) for r in p["curve"][c] if r[2]])
