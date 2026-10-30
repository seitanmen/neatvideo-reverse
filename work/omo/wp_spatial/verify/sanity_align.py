"""spatial-align の移植の自己試験 (本家は呼ばない)。葉どうしの整合と、1 面の駆動部が動くことを確かめる。
本家との照合は verify_align_leaves.py (ハーネス追加後)。"""
import os
import sys
import time

import numpy as np

sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "base"))
sys.path.insert(0, os.path.join(HERE, "..", "src"))
import nv_spatial as sp  # noqa: E402

rng = np.random.default_rng(1)
F = np.float32
fails = 0


def check(name, ok):
    global fails
    print(("ok  " if ok else "FAIL"), name)
    fails += 0 if ok else 1


# 1. strip_box3 (mode 0/1) を素朴な 4 列帯ループ (逆アセンブルの手順どおり) と比べる
def strip_ref(x, mode, sq=False, s=1.0):
    H, W = x.shape
    out = np.zeros_like(x)
    z = F(0)
    x0 = 0
    while True:
        c = min(x0, W - 4)
        first, last = c == 0, W - 4 <= x0
        h = np.zeros((H, 4), F)
        for r in range(H):
            row = x[r]
            for j in range(4):
                col = c + j
                L = row[col - 1] if col > 0 and not (first and j == 0) else (row[col] if mode == 0 else z)
                if j == 0 and not first:
                    L = row[col - 1]
                R = row[col + 1] if col < W - 1 and not (last and j == 3) else (row[col] if mode == 0 else z)
                if j == 3 and not last:
                    R = row[col + 1]
                if sq:
                    h[r, j] = F(F(F(row[col] * row[col]) + F(L * L)) + F(R * R))
                else:
                    h[r, j] = F(F(row[col] + L) + R)
        k = F(F(F(s) * sp.C_NINTH) * F(s)) if sq else sp.C_NINTH
        for r in range(H):
            if r == 0:
                v = F(F(h[0] + (h[0] if mode == 0 else z)) + h[1])
            elif r == H - 1:
                v = F((h[r] if mode == 0 else z) + F(h[r] + h[r - 1]))
            else:
                v = F(F(h[r] + h[r - 1]) + h[r + 1])
            v = F(v * k) if not sq else F(k * v)
            out[r, c:c + 4] = np.sqrt(v).astype(F) if sq else v
        x0 = c + 4
        if x0 >= W:
            break
    return out


for (H, W) in ((2, 4), (3, 5), (7, 9), (16, 13)):
    x = (rng.standard_normal((H, W)) * 10).astype(F)
    for mode in (0, 1):
        a = sp.strip_box3(x, mode)
        b = strip_ref(x, mode)
        check(f"strip_box3 {H}x{W} mode {mode}", np.array_equal(a.view(np.uint32), b.view(np.uint32)))
    a = sp.strip_rms3(x, 1, 1.0)
    b = strip_ref(x, 1, sq=True)
    check(f"strip_rms3 {H}x{W} mode 1 (first row uses sum*k, others k*sum)", np.allclose(a, b, rtol=0, atol=0))

# 2. 要素ごとの葉: NaN を含まない入力で、NEON 部と端数部が数学的に近いこと (FRECPE 系は 1 ulp 程度ずれ得る)
x = (rng.random(4099) * 50 + 0.01).astype(F)
y = (rng.random(4099) * 3).astype(F)
r = sp.leaf_recip(sp.EPS_WHITEN_F, x)
check("recip ≈ 1/(x+eps)", np.allclose(r, 1 / (x.astype(np.float64) + 1e-4), rtol=1e-5))
check("acc ≈ c + a·b", np.allclose(sp.leaf_acc(F(1), x, y, y), y + x * y, rtol=1e-6))
check("u8 saturates", sp.leaf_u8(np.array([-3, 0.5, 1.5, 254.5, 255.5, 300, np.nan], F)).tolist() == [0, 0, 2, 254, 255, 255, 0])

# 3. 段の切り出し: 全面の ROI は切り出しなし
sizes = sp.level_sizes(256, 256, 5)
check("level_rects full ROI → untiled", all(not r[4] for r in sp.level_rects((0, 0, 256, 256), sizes, 4, 5)))
rr = sp.level_rects((64, 64, 128, 128), sizes, 4, 5)
check("level_rects inner ROI → tiled at level 0", rr[0][:4] == (60, 60, 136, 136) and rr[0][4])

# 4. プロファイル (nv_profile、本家とビット一致) から cfg・σ・1 面
import nv_profile as npf  # noqa: E402
H = W = 96
base = np.full((H, W, 3), 110.0, F) + np.linspace(0, 40, W, dtype=F)[None, :, None]
img = (base + rng.standard_normal((H, W, 3)).astype(F) * 3.0).astype(F)
t0 = time.time()
prof, _ = npf.build_profile(img, 1, (16, 16, 64, 64), with_floor=False, lzma=False)
print("build_profile %.1f s" % (time.time() - t0))
pf = prof.view(F)
models = [pf[(0x4c + i * 0x1f54) // 4:(0x4c + (i + 1) * 0x1f54) // 4] for i in range(6)]
st = sp.default_settings()
cfgs = sp.build_cfg(st, colour_mode=1)
check("cfg sets Y/Cr/Cb = 2/1/1 (Normal, YCrCb)", [c["set"] for c in cfgs] == [2, 1, 1])
L = cfgs[0]["levels"]
tabs = sp.lum_tables(pf)
check("lum tables = 1 without AutoFineTune", np.all(tabs == 1.0))
R, G, B = (np.ascontiguousarray(img[..., c]) for c in range(3))
sig = sp.sigma_maps(R, G, B, models[:3], 1, L, tabs)
check("sigma maps finite", all(np.isfinite(s).all() for c in sig for s in c))
print("sigma level0 Y mean", float(sig[0][0].mean()))
import nv_flat as nf  # noqa: E402
P = [np.ascontiguousarray(p, F) for p in nf.transform(R, G, B, 1)]
import time as _t
t0 = _t.time()
outd = sp.plane_filter(P[0], sig[0], cfgs[0], models[3])
d = outd - P[0]
print("directional Y (set 2): %.1f s, removed rms %.3f" % (_t.time() - t0, float(np.sqrt((d * d).mean()))))
check("directional Y finite", bool(np.isfinite(outd).all()))
outc = sp.plane_filter(P[1], sig[1], cfgs[1], models[4])
d = outc - P[1]
print("directional Cr (set 1): removed rms %.3f" % float(np.sqrt((d * d).mean())))
check("directional Cr finite", bool(np.isfinite(outc).all()))
outt = sp.plane_filter(P[0], sig[0], cfgs[0], models[3], roi=(0, 0, H, W))
check("directional: roi = whole plane → same as untiled", np.array_equal(outt.view(np.uint32), outd.view(np.uint32)))
outr = sp.plane_filter(P[0], sig[0], cfgs[0], models[3], roi=(24, 24, 48, 48))
dd = (outr - outd)[24:72, 24:72]
print("inner ROI (tiled) vs untiled inside ROI: max |diff| %.3g" % float(np.abs(dd).max()))
out = outd
c0 = dict(cfgs[0], amount=(0.0,) * 5)
check("amount 0 → copy", np.array_equal(sp.plane_filter(P[0], sig[0], c0, models[3]), P[0]))
# 5. 品質 High (組 3・2・2)、任意機能 (cfg+0x28/0x3c/0x78/0x90) を有効にした設定でも動くこと
st2 = dict(st); st2.update({0x104: 1, 0x48: 1, 0x4c: 0.5, 0x50: 1.0, 0x54: (1.0, 0.8, 0.6), 0x60: 1, 0x64: (0.3, 0.3, 0.3),
                  0x90: 1, 0x94: (1.0, 1.0, 1.0), 0xa0: (0.5, 0.3, 0.2), 0x154: (1, 2, 0)})
cf2 = sp.build_cfg(st2, colour_mode=1)
check("High: sets 3/2/2", [c["set"] for c in cf2] == [3, 2, 2])
for c, (pl, mdl) in enumerate(((P[0], models[3]), (P[1], models[4]))):
    o = sp.plane_filter(pl, sig[c], cf2[c], mdl)
    d = o - pl
    print("High plane %d (set %d): removed rms %.3f" % (c, cf2[c]["set"], float(np.sqrt((d * d).mean()))))
    check("High plane %d finite" % c, bool(np.isfinite(o).all()))
o3 = sp.plane_filter(P[0], sig[0], cf2[0], models[3], roi=(0, 0, H, W))
o3b = sp.plane_filter(P[0], sig[0], cf2[0], models[3])
check("High set 3: roi = whole plane → same as untiled", np.array_equal(o3.view(np.uint32), o3b.view(np.uint32)))
o3r = sp.plane_filter(P[0], sig[0], cf2[0], models[3], roi=(24, 24, 48, 48))
check("High set 3 tiled finite", bool(np.isfinite(o3r).all()))
# 6. チャンク・フレーム駆動部
img2 = (np.full((150, 220, 3), 100.0, F) + np.linspace(0, 60, 220, dtype=F)[None, :, None]
        + rng.standard_normal((150, 220, 3)).astype(F) * 3.0).astype(F)
rgb2 = np.ascontiguousarray(img2.transpose(2, 0, 1))
t0 = time.time()
fr = sp.filter_frame(rgb2, prof, st)
print("filter_frame 220x150: %.1f s" % (time.time() - t0))
check("frame finite", bool(np.isfinite(fr).all()))
d = fr - rgb2
print("frame removed rms R/G/B", [round(float(np.sqrt((d[c] ** 2).mean())), 3) for c in range(3)])
st0 = dict(st); st0[0x00] = 0
check("spatial off → identity (amount 0)", np.allclose(sp.filter_frame(rgb2, prof, st0), rgb2, atol=1e-3))
for md in (0, 2):
    pr_m = prof.copy(); pr_m[0x48:0x4c] = np.frombuffer(np.uint32(md).tobytes(), np.uint8)
    o = sp.filter_chunk(rgb2[:, :64, :64], pr_m, st0)
    check("colour mode %d: transform + inverse ≈ identity" % md, np.allclose(o, rgb2[:, :64, :64], atol=1e-3))
stb = dict(st); stb.update({0x10c: 1, 0x110: 0.5})
fb = sp.filter_chunk(rgb2[:, :64, :64], prof, stb)
fa = sp.filter_chunk(rgb2[:, :64, :64], prof, st)
check("blend 0.5 → between input and filtered", np.allclose(fb, 0.5 * (fa + rgb2[:, :64, :64]), atol=0.05))
small = np.ascontiguousarray(rgb2[:, :60, :50])
try:
    o = sp.filter_frame(small, prof, st)
    check("50x60 frame (small levels) finite", bool(np.isfinite(o).all()))
except NotImplementedError as e:
    print("50x60 frame:", e)
print("FAILS", fails)
sys.exit(1 if fails else 0)
