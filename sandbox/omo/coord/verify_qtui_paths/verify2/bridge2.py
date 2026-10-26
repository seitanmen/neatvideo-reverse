import sys, numpy as np
sys.path.insert(0, "/Users/seitan/Desktop/work/reverse/repro")
from nv_ui_qt.ofx_bridge import unpack_yuv420p, pack_yuv420p, frame_size
if __name__ == "__main__":
    rng = np.random.default_rng(1)
    for (W, H) in [(640, 360), (192, 108), (64, 48), (1920, 1080)]:
        base = W * H + 2 * (W // 2) * (H // 2)
        assert frame_size(W, H) == base and frame_size(W, H, 16) == 2 * base and frame_size(W, H, 32) == 4 * base
        # 8
        d = rng.integers(0, 256, base, dtype=np.uint8).tobytes()
        r = []; p = unpack_yuv420p(d, W, H); assert pack_yuv420p(p) == d
        # 16
        d = rng.integers(0, 65536, base).astype("<u2").tobytes()
        p = unpack_yuv420p(d, W, H, 16); assert pack_yuv420p(p, 16) == d
        # 32 incl. NaN/inf
        a = (rng.random(base) * 1.2 - 0.1).astype("<f4"); a[5] = np.nan; a[7] = np.inf; a[9] = -np.inf
        d = a.tobytes(); rs = []; p = unpack_yuv420p(d, W, H, 32, rs)
        out = pack_yuv420p(p, 32, rs[0])
        b = np.frombuffer(out, "<f4"); fin = np.isfinite(a)
        nanbits = (a.view("<u4") == b.view("<u4"))[~fin].all()
        mx = float(np.abs(a[fin].astype(np.float64) - b[fin]).max())
        print(f"{W}x{H}: 8,16 exact; 32: nonfinite bit-exact={nanbits} finite maxdiff={mx:.3g} sizes ok")
