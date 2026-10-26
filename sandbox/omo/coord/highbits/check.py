"""Checks for the high-bit-depth tests (SPEC.md §4); prints the measured numbers.

    cd repro && python3 ../sandbox/omo/coord/highbits/check.py t2|t3|t4 ...
"""
import json
import os
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
T = os.path.join(HERE, "test")


def pix_fmt(path):
    return subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries", "stream=pix_fmt",
                           "-of", "csv=p=0", path], capture_output=True, text=True).stdout.strip()


def raw(path, fmt, dtype, W, H, planes, extra=()):
    """Decoded frames as a list of plane lists (plane shapes `planes`)."""
    b = subprocess.run(["ffmpeg", "-v", "error", *extra, "-i", path, "-f", "rawvideo", "-pix_fmt", fmt, "-"],
                       capture_output=True, check=True).stdout
    a = np.frombuffer(b, dtype)
    n = sum(h * w for h, w in planes)
    out = []
    for k in range(len(a) // n):
        f, o = [], k * n
        for h, w in planes:
            f.append(a[o:o + h * w].reshape(h, w))
            o += h * w
        out.append(f)
    return out


def shapes(name, W, H):
    sys.path.insert(0, os.path.join(HERE, "..", "..", "..", "..", "repro"))
    import nv_depth as nd
    return nd.pixfmt(name).plane_shapes(W, H)


def t2():
    W, H = 640, 360
    j8 = json.load(open(os.path.join(T, "t2_out8.json")))
    j10 = json.load(open(os.path.join(T, "t2_out10.json")))
    same = {k: j8["profile"][k] == j10["profile"][k] for k in j8["profile"]}
    print("t2 profile keys equal:", all(same.values()), [k for k, v in same.items() if not v])
    print("t2 sigma 8:", j8["profile"]["sigma"], "10:", j10["profile"]["sigma"])
    print("t2 out pix_fmt:", pix_fmt(os.path.join(T, "t2_out8.mp4")), pix_fmt(os.path.join(T, "t2_out10.mkv")))
    s = shapes("yuv420p", W, H)
    o8 = raw(os.path.join(T, "t2_out8.mp4"), "yuv420p", np.uint8, W, H, s)
    o10 = raw(os.path.join(T, "t2_out10.mkv"), "yuv420p10le", "<u2", W, H, s)
    print("t2 frames:", len(o8), len(o10))
    for c, nm in enumerate("YUV"):
        d = np.concatenate([np.abs(b[c].astype(np.int64) - 4 * a[c].astype(np.int64)).ravel() for a, b in zip(o8, o10)])
        print(f"t2 |out10 - 4*out8| plane {nm}: max {d.max()} mean {d.mean():.4f} hist {np.bincount(d, minlength=3)[:3].tolist()}")


def t3(mix=False):
    W, H = 320, 256
    for name in ("yuv420p10le", "yuv422p10le", "yuv444p12le", "yuv444p16le"):
        tag = "mix1" if mix else "out"
        src = os.path.join(T, f"syn_{name}.mkv")
        dst = os.path.join(T, f"t3_{tag}_{name}.mkv")
        if not os.path.exists(dst):
            print("t3 missing", dst)
            continue
        bits = int(name[-4:-2])
        s = shapes(name, W, H)
        i_ = raw(src, name, "<u2", W, H, s)
        o_ = raw(dst, name, "<u2", W, H, s)
        meta = json.load(open(os.path.join(T, f"t3_{tag}_{name}.json")))
        lo, hi = meta["range"]
        i_ = i_[lo:hi]
        allv = np.concatenate([p.ravel() for f in o_ for p in f]).astype(np.int64)
        q = 2 ** (bits - 8)
        low = float(np.mean(allv % q != 0))
        line = f"t3 {tag} {name}: out pix_fmt {pix_fmt(dst)}, frames {len(o_)}, values not multiple of {q}: {low:.4f}"
        if mix:
            same = len(i_) == len(o_) and all(np.array_equal(a, b) for fa, fb in zip(i_, o_) for a, b in zip(fa, fb))
            line += f", Mix 1 output == input bit for bit: {same}"
        else:
            dmax = max(int(np.abs(a.astype(np.int64) - b).max()) for fa, fb in zip(i_, o_) for a, b in zip(fa, fb))
            ymad = np.mean([np.abs(fa[0].astype(np.float64) - fb[0]).mean() for fa, fb in zip(i_, o_)])
            line += f", max |out-in| {dmax}, mean |dY| {ymad:.2f}"
        print(line)


def t4():
    W, H = 320, 256
    s = shapes("gbrpf32le", W, H)
    src = np.load(os.path.join(T, "syn_gbrpf32le_frames.npy")).reshape(-1, 3, H, W)
    print(f"t4 input range: min {src.min():.4f} max {src.max():.4f}")
    for tag, path, extra in (("mkv mix1", "t4_mix1.mkv", ()), ("mkv out", "t4_out.mkv", ()),
                             ("exr mix1", "t4_mix1_exr/out_%05d.exr", ()), ("exr out", "t4_out_exr/out_%05d.exr", ())):
        p = os.path.join(T, path)
        side = os.path.join(T, path.split("/")[0].replace(".mkv", "") + ".json") if path.endswith(".mkv") else \
            os.path.join(T, os.path.dirname(path), "out.json")
        if not os.path.exists(side):
            print("t4 missing", side)
            continue
        meta = json.load(open(side))
        lo, hi = meta["range"]
        o = np.array(raw(p, "gbrpf32le", "<f4", W, H, s))
        i = src[lo:hi]
        err = np.abs(o.astype(np.float64) - i)
        print(f"t4 {tag}: pix_fmt {pix_fmt(p)} codec {meta['codec']} frames {len(o)}, max |out-in| {err.max():.3g}, "
              f"out min {o.min():.4f} max {o.max():.4f}, out>1: {np.mean(o > 1.0):.4f} (in {np.mean(i > 1.0):.4f}), "
              f"out<0: {np.mean(o < 0):.4f} (in {np.mean(i < 0):.4f})")


if __name__ == "__main__":
    for a in sys.argv[1:]:
        {"t2": t2, "t3": t3, "t3mix": lambda: t3(True), "t4": t4}[a]()
