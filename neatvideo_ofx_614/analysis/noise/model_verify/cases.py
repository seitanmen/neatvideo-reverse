"""Deterministic input planes for the FUN_00995250 verification (sandbox/dirfilt).

Each case is (name, group, builder) with builder() -> float32 (H, W) plane; the groups are
  real     crops of test_v02.mp4 / test.mp4 frames (several positions, sizes 48..512, square / rectangular /
           odd, planes R, G, B, Y, Cb', Cr')
  synth    white noise (several sigma), coloured noise, straight edges (16 directions), checkerboards,
           constants, quantised planes, black bands, blown-out planes, NaN / inf / huge / subnormal values
  bound    sizes at the measurability limits (levels, families A / B / C / dc, FUN_00ad319c, tiling) and 512
  fuzz     random sizes 2..512 (log-uniform) and random contents (seeded)"""
import os
import sys

import numpy as np

sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
from common import frame_cached  # noqa: E402

F32 = np.float32
V02_FRAMES = [0, 36, 72, 108, 144, 180, 216, 252, 288, 312, 348, 384, 420, 456, 492, 528, 576, 612, 648, 700]
T_FRAMES = [0, 45, 90, 135, 180, 225, 270, 300, 360, 405, 450, 495, 540, 585, 630, 675, 720, 750, 810, 880]
REAL_SIZES = [(48, 48), (64, 64), (96, 96), (128, 128), (160, 120), (200, 150), (256, 256), (320, 320),
              (384, 216), (512, 512), (512, 288), (57, 301), (101, 77), (333, 129), (89, 89), (88, 88),
              (129, 512), (75, 75), (49, 500), (300, 55), (147, 93), (61, 61), (210, 333), (99, 451)]


def plane_of(rgb, kind):
    R, G, B = (rgb[..., i].astype(F32) for i in range(3))
    Y = (F32(0.299) * R + F32(0.587) * G + F32(0.114) * B).astype(F32)
    return {"R": R, "G": G, "B": B, "Y": Y, "Cb": ((B - Y) * F32(0.565)).astype(F32),
            "Cr": ((R - Y) * F32(0.713)).astype(F32)}[kind]


def _real(clip, n, x, y, w, h, kind):
    def build():
        fr = frame_cached(clip, n)
        return np.ascontiguousarray(plane_of(fr[y:y + h, x:x + w], kind))
    return build


def real_cases():
    out = []
    rng = np.random.default_rng(20261016)
    kinds = ["R", "G", "B", "Y", "Cb", "Cr"]
    si = 0
    for clip, frames, (FW, FH) in (("test_v02.mp4", V02_FRAMES, (1920, 1080)), ("test.mp4", T_FRAMES, (640, 360))):
        for n in frames:
            for _ in range(3):
                w, h = REAL_SIZES[si % len(REAL_SIZES)]
                si += 1
                w, h = min(w, FW), min(h, FH)
                x = int(rng.integers(0, FW - w + 1))
                y = int(rng.integers(0, FH - h + 1))
                kind = kinds[int(rng.integers(0, len(kinds)))]
                out.append((f"real_{clip.split('.')[0]}_n{n}_{x}_{y}_{w}x{h}_{kind}", "real",
                            _real(clip, n, x, y, w, h, kind)))
    return out


def _blur(x, s):
    if s <= 0:
        return x
    r = int(3 * s + 1)
    k = np.exp(-0.5 * (np.arange(-r, r + 1) / s) ** 2)
    k /= k.sum()
    x = np.apply_along_axis(lambda v: np.convolve(np.pad(v, r, mode="wrap"), k, "valid"), 1, x)
    x = np.apply_along_axis(lambda v: np.convolve(np.pad(v, r, mode="wrap"), k, "valid"), 0, x)
    return x


def _noise(h, w, sigma, seed, mean=100.0, blur=0.0):
    rng = np.random.default_rng(seed)
    n = rng.normal(0, 1, (h, w))
    if blur:
        n = _blur(n, blur)
        n /= n.std() + 1e-12
    return (mean + sigma * n).astype(F32)


def _edge(h, w, ang, seed, sigma=2.0):
    yy, xx = np.mgrid[0:h, 0:w]
    d = (xx - w / 2) * np.cos(ang) + (yy - h / 2) * np.sin(ang)
    return (np.where(d > 0, 180.0, 60.0) + _noise(h, w, sigma, seed, 0.0)).astype(F32)


def _checker(h, w, p, seed):
    yy, xx = np.mgrid[0:h, 0:w]
    return (np.where(((xx // p) + (yy // p)) % 2 == 0, 150.0, 90.0) + _noise(h, w, 1.5, seed, 0.0)).astype(F32)


def synth_cases():
    out = []
    add = lambda name, f: out.append((name, "synth", f))
    for i, s in enumerate([0.3, 1.0, 3.0, 10.0, 40.0]):
        for (w, h) in [(64, 64), (128, 96), (200, 200)]:
            add(f"white_s{s}_{w}x{h}", lambda h=h, w=w, s=s, i=i: _noise(h, w, s, 100 + i))
    for b in [0.7, 1.5, 3.0]:
        for (w, h) in [(96, 96), (160, 128)]:
            add(f"coloured_b{b}_{w}x{h}", lambda h=h, w=w, b=b: _noise(h, w, 5.0, 200, 120.0, b))
    for i in range(16):
        a = np.pi * i / 16
        add(f"edge_a{i}_96", lambda a=a, i=i: _edge(96, 96, a, 300 + i))
    for i in (0, 3, 5, 9):
        add(f"edge_a{i}_160x128", lambda i=i: _edge(128, 160, np.pi * i / 16 + 0.05, 400 + i))
    for p in [1, 2, 3, 8, 16]:
        add(f"checker_p{p}_96", lambda p=p: _checker(96, 96, p, 500 + p))
    for c in [0.0, 128.0, 255.0, -5.0, 1e6]:
        add(f"const_{c}_64", lambda c=c: np.full((64, 64), c, F32))
    add("const_128_120x100", lambda: np.full((100, 120), 128.0, F32))
    for q in [1.0, 8.0, 32.0]:
        add(f"quant_q{q}_128", lambda q=q: (np.round(_noise(128, 128, 6.0, 600, 128.0) / q) * q).astype(F32))
    add("blackband_128x96", lambda: np.where((np.arange(96) < 20)[:, None] | (np.arange(96) > 75)[:, None],
                                               F32(0), _noise(96, 128, 4.0, 700, 90.0)).astype(F32))
    add("blackband_cols_200x150", lambda: np.where((np.arange(200) < 30)[None, :], F32(16.0),
                                                     _noise(150, 200, 3.0, 701, 80.0)).astype(F32))
    add("blown_128", lambda: np.clip(_noise(128, 128, 30.0, 800, 230.0), 0, 255).astype(F32))
    add("blown_half_160x96", lambda: np.where((np.arange(160) > 80)[None, :], F32(255.0),
                                               _noise(96, 160, 5.0, 801, 140.0)).astype(F32))

    def special(kind, h=96, w=96):
        def f():
            x = _noise(h, w, 5.0, 900, 100.0)
            if kind == "nan1":
                x[40, 33] = np.nan
            elif kind == "nanblock":
                x[10:30, 50:70] = np.nan
            elif kind == "pinf":
                x[50, 50] = np.inf
            elif kind == "ninf":
                x[3, 77] = -np.inf
            elif kind == "huge":
                x[20:24, 20:24] = 1e30
            elif kind == "huge_all":
                x = (x * F32(1e17)).astype(F32)
            elif kind == "subnormal":
                x = (x * F32(1e-41)).astype(F32)
            elif kind == "allnan":
                x[:] = np.nan
            elif kind == "nanpayload":
                x[60, 61] = np.array([0x7fa12345], np.uint32).view(F32)[0]     # signalling NaN
                x[61, 62] = np.array([0xffc00007], np.uint32).view(F32)[0]
            elif kind == "negzero":
                x = np.where(np.abs(x - 100) < 3, F32(-0.0), x).astype(F32)
            return x
        return f
    for kind in ["nan1", "nanblock", "pinf", "ninf", "huge", "huge_all", "subnormal", "allnan", "nanpayload",
                 "negzero"]:
        add(f"special_{kind}_96", special(kind))
    add("special_nan1_200x150", special("nan1", 150, 200))
    add("gradient_200x120", lambda: (np.mgrid[0:120, 0:200][1] * 0.7 + np.mgrid[0:120, 0:200][0] * 0.3
                                       + _noise(120, 200, 1.0, 901, 0.0)).astype(F32))
    add("stripes_vert_128", lambda: (np.where((np.arange(128) // 4) % 2 == 0, 140.0, 100.0)[None, :]
                                       + _noise(128, 128, 2.0, 902, 0.0)).astype(F32))
    return out


BOUND_SIZES = [(2, 2), (3, 5), (8, 8), (9, 9), (9, 40), (9, 39), (40, 9), (16, 16), (17, 17), (12, 40), (13, 13),
               (20, 20), (21, 28), (25, 25), (26, 26), (27, 27), (28, 28), (29, 29), (36, 36), (37, 37), (43, 43),
               (44, 44), (45, 45), (48, 48), (49, 49), (88, 88), (89, 89), (88, 89), (89, 88), (90, 90), (97, 97),
               (128, 129), (511, 7), (512, 2), (2, 512), (512, 512), (512, 300), (300, 512), (511, 511), (257, 129),
               (18, 512), (24, 24), (33, 33), (50, 26), (26, 50), (56, 56), (57, 57), (176, 176), (177, 177),
               (352, 352), (353, 100)]


def bound_cases():
    return [(f"bound_{w}x{h}", "bound", (lambda w=w, h=h, i=i: _noise(h, w, 5.0, 1000 + i)))
            for i, (w, h) in enumerate(BOUND_SIZES)]


def fuzz_cases(n=220, seed=4242):
    rng = np.random.default_rng(seed)
    out = []
    for i in range(n):
        w = int(np.clip(np.round(np.exp(rng.uniform(np.log(2), np.log(512)))), 2, 512))
        h = int(np.clip(np.round(np.exp(rng.uniform(np.log(2), np.log(512)))), 2, 512))
        kind = ["white", "coloured", "edge", "checker", "gradient", "real", "real"][int(rng.integers(0, 7))]
        s = float(np.exp(rng.uniform(np.log(0.2), np.log(50))))
        scale = float([1, 1, 1, 1, 1e-3, 1e3, 255][int(rng.integers(0, 7))])
        nan = bool(rng.random() < 0.05)
        sd = int(rng.integers(0, 2 ** 31))
        rx, ry = float(rng.random()), float(rng.random())
        fi = int(rng.integers(0, len(V02_FRAMES)))

        def f(w=w, h=h, kind=kind, s=s, scale=scale, nan=nan, sd=sd, rx=rx, ry=ry, fi=fi):
            if kind == "white":
                x = _noise(h, w, s, sd)
            elif kind == "coloured":
                x = _noise(h, w, s, sd, 100.0, 1.0 + (sd % 3))
            elif kind == "edge":
                x = _edge(h, w, (sd % 1000) / 1000 * np.pi, sd, s)
            elif kind == "checker":
                x = _checker(h, w, 1 + sd % 12, sd)
            elif kind == "gradient":
                yy, xx = np.mgrid[0:h, 0:w]
                x = (xx * 0.3 + yy * 0.6 + _noise(h, w, s, sd, 0.0)).astype(F32)
            else:
                fr = frame_cached("test_v02.mp4", V02_FRAMES[fi])
                x0 = int(rx * (1920 - w))
                y0 = int(ry * (1080 - h))
                x = plane_of(fr[y0:y0 + h, x0:x0 + w], "RGBY"[sd % 4] if sd % 4 < 3 else "Y")
            x = (np.asarray(x, np.float64) * scale).astype(F32)
            if nan:
                x[sd % h, (sd // 7) % w] = np.nan
            return np.ascontiguousarray(x)
        out.append((f"fuzz{i:03d}_{kind}_{w}x{h}", "fuzz", f))
    return out


def all_cases():
    return real_cases() + synth_cases() + bound_cases() + fuzz_cases()
