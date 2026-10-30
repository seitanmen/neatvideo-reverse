"""Test 2: ofx_bridge unit test (no stdin). Task item 2 plus depth 16 / 32 and non-trivial data."""
import sys
sys.path.insert(0, "/Users/seitan/Desktop/work/reverse/repro")
import numpy as np
from nv_ui_qt.ofx_bridge import unpack_yuv420p, pack_yuv420p, frame_size

ok = True
def check(name, c, detail=""):
    global ok
    ok &= bool(c)
    print(f"[{'PASS' if c else 'FAIL'}] {name} {detail}")

W, H = 640, 360
y = np.full((H, W), 128, np.uint8)
cb = np.full((H // 2, W // 2), 128, np.uint8)
cr = np.full((H // 2, W // 2), 128, np.uint8)
data = y.tobytes() + cb.tobytes() + cr.tobytes()
planes = unpack_yuv420p(data, W, H)
packed = pack_yuv420p(planes)
check("task snippet: 8-bit roundtrip (constant 128)", packed == data)
check("task snippet: frame_size", frame_size(W, H) == len(data), f"{frame_size(W, H)} == {len(data)}")

rng = np.random.default_rng(0)
for W, H in ((640, 360), (192, 108), (191, 107)):
    n8 = W * H + 2 * (W // 2) * (H // 2)
    d8 = rng.integers(0, 256, n8, dtype=np.uint8).tobytes()
    check(f"8-bit random roundtrip {W}x{H}", pack_yuv420p(unpack_yuv420p(d8, W, H)) == d8)
    d16 = (rng.integers(0, 65536, n8).astype("<u2")).tobytes()
    check(f"16-bit random roundtrip {W}x{H}", pack_yuv420p(unpack_yuv420p(d16, W, H, 16), 16) == d16,
          f"size {frame_size(W, H, 16)}=={len(d16)}")
    f = rng.random(n8, dtype=np.float32) * 1.2 - 0.1
    f[:5] = [np.nan, np.inf, -np.inf, 0.5, -0.0]
    d32 = f.astype("<f4").tobytes()
    rest = []
    pl = unpack_yuv420p(d32, W, H, 32, rest)
    back = pack_yuv420p(pl, 32, rest[0])
    a, b = np.frombuffer(d32, "<f4"), np.frombuffer(back, "<f4")
    same_bits = np.array_equal(a.view("<u4"), b.view("<u4"))
    maxrel = float(np.nanmax(np.abs(a[np.isfinite(a)] - b[np.isfinite(a)])))
    fin = np.isfinite(a)
    nonfin_same = np.array_equal(a[~fin].view("<u4"), b[~fin].view("<u4"))
    check(f"32-bit roundtrip {W}x{H}", len(back) == len(d32) and nonfin_same and maxrel <= 2.4e-7,
          f"(non-finite samples bit-identical {nonfin_same}; finite: whole frame bit-identical {same_bits}, "
          f"max |diff| {maxrel:.3g} = x*255/255 float32 rounding)")
print("ALL PASS" if ok else "SOME FAIL")
sys.exit(0 if ok else 1)
