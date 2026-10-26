import os, subprocess, sys, numpy as np
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "probe.py")).read().split("res = {}")[0])
q = [tuple(np.clip(np.rint(p), 0, 255) for p in f) for f in frames]
d8 = b"".join(np.asarray(p).astype(np.uint8).tobytes() for f in q for p in f)
d16 = b"".join((np.asarray(p) * 256).astype("<u2").tobytes() for f in q for p in f)
fr = [list(f) for f in q]; fr[1][0] = fr[1][0].copy(); fr[1][0][:10, 100:140] = -20.0; fr[1][0][150:160, 250:260] = 300.0
d32 = b"".join((np.asarray(p, np.float32) / np.float32(255)).astype("<f4").tobytes() for f in fr for p in f)
o = {}
for depth, data in ((8, d8), (16, d16), (32, d32)):
    r = run(depth, data); dt = {8: np.uint8, 16: "<u2", 32: "<f4"}[depth]
    o[depth] = np.frombuffer(r.stdout, dt).astype(float); print(depth, r.returncode, len(r.stdout))
Y32 = o[32][:W * H].reshape(H, W) * 255
print("|out16/256-out8| max", np.abs(o[16] / 256 - o[8]).max())
m = np.ones((H, W), bool); m[:10, 100:140] = False; m[150:160, 250:260] = False
print("|out32*255-out8| max outside HDR patches", np.abs(Y32[m] - o[8][:W * H].reshape(H, W)[m]).max())
print("negative patch out (8-bit units) mean", Y32[2:8, 104:136].mean(), " min", Y32[:10, 100:140].min(), "; >255 patch mean", Y32[152:158, 252:258].mean(), "max", Y32[150:160, 250:260].max())
