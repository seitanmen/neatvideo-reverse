"""Run the new CLI on the verifier's synthetic sources (3 at a time) and check the outputs."""
import json, os, subprocess, sys, glob
from concurrent.futures import ThreadPoolExecutor
import numpy as np
V = os.path.dirname(os.path.abspath(__file__)); S, O = os.path.join(V, "syn"), os.path.join(V, "out")
REPRO = os.path.abspath(os.path.join(V, "../../../../../repro"))
sys.path.insert(0, REPRO)
import nv_depth as nd

def cli(src, dst, *args):
    r = subprocess.run([sys.executable, "denoise_video_v5.py", src, dst, "--jobs", "2", *args], cwd=REPRO,
                       capture_output=True, text=True)
    with open(os.path.splitext(dst.replace("%04d", "").replace("%05d", ""))[0] + ".log", "w") as fh:
        fh.write(" ".join(args) + "\n" + r.stdout + r.stderr)
    return r.returncode, r.stdout + r.stderr

def pixfmt_of(path):
    return subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries", "stream=pix_fmt,codec_name,profile,width,height",
                           "-of", "csv=p=0", path], capture_output=True, text=True).stdout.strip()

def decode(path, pix):
    return subprocess.run(["ffmpeg", "-v", "error", "-i", path, "-f", "rawvideo", "-pix_fmt", pix, "-"], capture_output=True).stdout

cases = {"y420p10_odd": "yuv420p10le", "y422p10_odd": "yuv422p10le", "y422p12": "yuv422p12le", "y444p12": "yuv444p12le",
         "y444p16": "yuv444p16le", "y420p16": "yuv420p16le", "y422p10_even": "yuv422p10le", "gbrp12": "gbrp12le",
         "gbrpf32": "gbrpf32le"}
jobs = []
for n, pix in cases.items():
    src = os.path.join(S, n + ".mkv")
    jobs.append((n, pix, "mix1", src, os.path.join(O, f"{n}_mix1.mkv"), ["--depth", "source", "--mix", "1"]))
    jobs.append((n, pix, "proc", src, os.path.join(O, f"{n}_proc.mkv"), ["--depth", "source"]))
exr_src = os.path.join(S, "exr", "in_%04d.exr")
os.makedirs(os.path.join(O, "exr_mix1"), exist_ok=True); os.makedirs(os.path.join(O, "exr_proc"), exist_ok=True)
jobs.append(("exrseq", "gbrpf32le", "mix1", exr_src, os.path.join(O, "exr_mix1", "o_%05d.exr"), ["--depth", "32", "--mix", "1"]))
jobs.append(("exrseq", "gbrpf32le", "proc", exr_src, os.path.join(O, "exr_proc", "o_%05d.exr"), ["--depth", "source"]))

def run(j):
    n, pix, kind, src, dst, args = j
    rc, log = cli(src, dst, *args)
    return j, rc, log

res = {}
with ThreadPoolExecutor(3) as ex:
    for (n, pix, kind, src, dst, args), rc, log in ex.map(run, jobs):
        line = [n, kind, f"rc={rc}"]
        if rc:
            print(*line, log[-400:], flush=True); continue
        pf = pixfmt_of(dst.replace("%05d", "00001"))
        line.append(pf)
        raw_in = np.load(os.path.join(S, n + "_raw.npy")).tobytes()
        out = decode(dst, pix)
        f = nd.pixfmt(pix)
        dt = np.dtype(f.dtype)
        a, b = np.frombuffer(raw_in, dt), np.frombuffer(out, dt)
        line.append(f"len in/out {a.size}/{b.size}")
        if a.size == b.size:
            if f.is_float:
                d = np.abs(a.astype(np.float64) - b.astype(np.float64))
                line.append(f"maxabs={d.max():.3g} in[{a.min():.4f},{a.max():.4f}] out[{b.min():.4f},{b.max():.4f}] "
                            f"frac>1 in/out {np.mean(a > 1):.4f}/{np.mean(b > 1):.4f} frac<0 {np.mean(a < 0):.4f}/{np.mean(b < 0):.4f}")
            else:
                st = 2 ** (f.bits - 8)
                line.append(f"bitexact={np.array_equal(a, b)} maxdiff={int(np.abs(a.astype(int) - b.astype(int)).max())} "
                            f"frac_not_mult{st} in/out {np.mean(a % st != 0):.4f}/{np.mean(b % st != 0):.4f} "
                            f"lowbits_hist={np.bincount((b % 4).astype(int), minlength=4).tolist()}")
            res[(n, kind)] = (a, b)
        js = json.load(open(nd and (os.path.splitext(dst.replace("_%05d", ""))[0] + ".json")))
        line.append(f"json pix_fmt={js['pix_fmt']} depth={js['depth']} codec={js['codec']} src={js['source_pix_fmt']}")
        print(*line, flush=True)
