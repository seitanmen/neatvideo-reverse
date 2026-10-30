import ctypes, sys, numpy as np
sys.path.insert(0, "repro")
import nvfilter as nv
lib = ctypes.CDLL("native/libnvcore.dylib")
P = ctypes.c_void_p
rng = np.random.default_rng(0)
bad = 0
for t in range(60):
    H, W = rng.integers(2, 40, 2)
    sub = t % 3
    Hc, Wc = ((H + 1) // 2, (W + 1) // 2) if sub == 0 else (H, W) if sub == 1 else (H // 2 + rng.integers(0, 2), W // 2 + rng.integers(0, 2))
    Y = rng.uniform(-20, 280, (H, W)).astype(np.float32)
    Cb = rng.uniform(-20, 280, (Hc, Wc)).astype(np.float32)
    Cr = rng.uniform(-20, 280, (Hc, Wc)).astype(np.float32)
    if t % 5 == 0:
        Y.flat[rng.integers(0, Y.size)] = np.nan; Cb.flat[0] = np.inf
    for mat, mi in (("bt709", 0), ("bt601", 1), ("bt2020", 2)):
        for fr in (False, True):
            ref = nv.yuv_to_rgb((Y, Cb, Cr), mat, fr)
            refs = [ref, np.clip(ref, 0, 255), np.clip(np.nan_to_num(ref, nan=0.0, posinf=255.0, neginf=0.0), 0, 255)]
            for clip in range(3):
                out = np.empty((H, W, 3), np.float32)
                lib.nvc_yuv_to_rgb(Y.ctypes.data_as(P), Cb.ctypes.data_as(P), Cr.ctypes.data_as(P), int(H), int(W), int(Hc), int(Wc), mi, int(fr), clip, out.ctypes.data_as(P))
                if out.view(np.uint32).tobytes() != refs[clip].astype(np.float32).view(np.uint32).tobytes():
                    bad += 1
print("bad", bad)
