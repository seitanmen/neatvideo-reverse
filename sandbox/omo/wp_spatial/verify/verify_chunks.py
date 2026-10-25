"""空間フィルタのチャンク配置 (スケジューラ FUN_00abf6a8 の格子) を `nvh tiles W H tile margin` (FUN_00ac1194、読み取りのみ) と
repro/nv_curve.tile_layout (base/ のコピー) で比べる。空間段の核と余白は ctx[0x7f0] = max_c 表 0x1243740[組][L−1]、
ctx[0x7f1] = max_c 表 0x1243808[組][L−1] (FUN_0099ead0)。Filter は FUN_00ac14d4(…, 0, 0, W, H, 核, 核, 余白, 0x30, …) を呼ぶ
(0x9ac880〜0x9ac8c0)。FUN_00ac14d4 には Demo の領域制限 (0x14de194.. の大域) があるが、nvh tiles の FUN_00ac1194 にはない。"""
import os, subprocess, sys
sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "base"))
import nv_curve as ncv  # noqa: E402
NVH = os.path.abspath(os.path.join(HERE, "..", "..", "..", "..", "neatvideo_ofx_614", "analysis", "harness", "nvh"))
assert subprocess.run([NVH, "selftest"], capture_output=True).returncode == 3
print("selftest exit 3 OK")
pairs = sorted({(96, m) for m in (1, 2, 4, 8, 16, 32, 64, 96)} | {(128, m) for m in (16, 32, 48, 64, 96, 128)} |
               {(192, m) for m in (64, 96, 128)})
sizes = [(48, 48), (49, 77), (100, 70), (300, 200), (513, 300), (640, 360), (1280, 720), (1920, 1080), (1919, 1081),
         (3840, 2160), (97, 400), (2000, 61)]
bad = tot = 0
for tile, m in pairs:
    for (W, H) in sizes:
        out = subprocess.run([NVH, "tiles", str(W), str(H), str(tile), str(m)],
                             capture_output=True, text=True).stdout.split("\n")
        g = []
        for l in out:
            if l.startswith("tile "):
                v = dict(kv.split("=") for kv in l.split()[2:])
                g.append(tuple(int(v[k]) for k in ("x", "y", "w", "h", "f4", "f5", "core_w", "core_h", "core_x", "core_y", "f10")))
        p = [tuple(t) for t in ncv.tile_layout(W, H, tile, m, 48)]
        tot += 1
        if g != p:
            bad += 1
            print("mismatch", W, H, tile, m, len(g), len(p))
print("layouts %d mismatched %d" % (tot, bad))
