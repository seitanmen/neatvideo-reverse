"""Encoders (x264 10-bit, x265 12-bit, prores) on odd sizes; colour tags; mix-1 bit-exactness with tags; yuv->float read."""
import json, os, subprocess, sys
from concurrent.futures import ThreadPoolExecutor
import numpy as np
V = os.path.dirname(os.path.abspath(__file__)); S, O = os.path.join(V, "syn"), os.path.join(V, "enc")
REPRO = os.path.abspath(os.path.join(V, "../../../../../repro"))
def cli(src, dst, *args):
    r = subprocess.run([sys.executable, "denoise_video_v5.py", src, dst, "--jobs", "2", "--frames", "4", *args], cwd=REPRO, capture_output=True, text=True)
    open(os.path.splitext(dst)[0] + ".log", "w").write(" ".join(args) + "\n" + r.stdout + r.stderr)
    return r.returncode, (r.stdout + r.stderr).strip().splitlines()[-1]
def pr(p):
    return subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
                           "stream=codec_name,profile,pix_fmt,width,height,color_space,color_primaries,color_transfer,color_range,nb_frames",
                           "-of", "compact=p=0", p], capture_output=True, text=True).stdout.strip()
jobs = [("x264_420_10odd", "y420p10_odd.mkv", "x264_420_10odd.mp4", ["--depth", "10"]),
        ("x264_422_10odd", "y422p10_odd.mkv", "x264_422_10odd.mp4", ["--depth", "10"]),
        ("x264_444_10odd", "y444p12_odd.mkv", "x264_444_10odd.mp4", ["--depth", "10"]),
        ("x265_420_12", "y420p10_odd.mkv", "x265_420_12.mp4", ["--depth", "12"]),
        ("x265_422_12", "y422p12.mkv", "x265_422_12.mov", ["--depth", "source"]),
        ("x265_444_12odd", "y444p12_odd.mkv", "x265_444_12odd.m4v", ["--depth", "source"]),
        ("prores_422odd", "y422p10_odd.mkv", "prores_422odd.mov", ["--depth", "10", "--codec", "prores"]),
        ("prores_444odd", "y444p12_odd.mkv", "prores_444odd.mov", ["--depth", "10", "--codec", "prores"]),
        ("tag_420pc_mix1", "y420p10_tag.mkv", "tag_420pc_mix1.mkv", ["--depth", "source", "--mix", "1"]),
        ("tag_444tv_mix1", "y444p12_tag.mkv", "tag_444tv_mix1.mkv", ["--depth", "source", "--mix", "1"]),
        ("tag_gbr_mix1", "gbrp12_tag.mkv", "tag_gbr_mix1.mkv", ["--depth", "source", "--mix", "1"]),
        ("tag_444tv_f32_mix1", "y444p12_tag.mkv", "tag_444tv_f32_mix1.mkv", ["--depth", "32", "--mix", "1"]),
        ("tag_420pc_x264", "y420p10_tag.mkv", "tag_420pc_x264.mp4", ["--depth", "10", "--crf", "0"])]
def run(j):
    n, s, d, a = j
    rc, last = cli(os.path.join(S, s), os.path.join(O, d), *a)
    return n, s, d, rc, last
with ThreadPoolExecutor(3) as ex:
    for n, s, d, rc, last in ex.map(run, jobs):
        print(n, f"rc={rc}", last[:160]); print("   src:", pr(os.path.join(S, s))); 
        if rc == 0: print("   out:", pr(os.path.join(O, d)))
        sys.stdout.flush()
# mix-1 bit-exactness on tagged sources (native pix_fmt decode, first 4 frames)
def dec(p, pix): return subprocess.run(["ffmpeg", "-v", "error", "-i", p, "-frames:v", "4", "-f", "rawvideo", "-pix_fmt", pix, "-"], capture_output=True).stdout
for s, d, pix in (("y420p10_tag.mkv", "tag_420pc_mix1.mkv", "yuv420p10le"), ("y444p12_tag.mkv", "tag_444tv_mix1.mkv", "yuv444p12le"),
                  ("gbrp12_tag.mkv", "tag_gbr_mix1.mkv", "gbrp12le")):
    a, b = dec(os.path.join(S, s), pix), dec(os.path.join(O, d), pix)
    print("mix1 bitexact", d, len(a), len(b), a == b)
# yuv(tv, bt709) -> float RGB mix 1: compare with an independent conversion of the 4:4:4 source
a = np.frombuffer(dec(os.path.join(S, "y444p12_tag.mkv"), "yuv444p12le"), "<u2").astype(np.float64).reshape(4, 3, 222, 300) / 16
b = np.frombuffer(dec(os.path.join(O, "tag_444tv_f32_mix1.mkv"), "gbrpf32le"), "<f4").astype(np.float64).reshape(4, 3, 222, 300)
kr, kb = 0.2126, 0.0722
y, cb, cr = (a[:, 0] - 16) * 255 / 219, (a[:, 1] - 128) * 255 / 224, (a[:, 2] - 128) * 255 / 224
r = y + 2 * (1 - kr) * cr; bb = y + 2 * (1 - kb) * cb; g = (y - kr * r - kb * bb) / (1 - kr - kb)
ref = np.stack([g, bb, r], 1) / 255
print("yuv444p12 tv bt709 -> gbrpf32 mix1 vs independent bt709 conversion: max abs", float(np.abs(ref - b).max()))
