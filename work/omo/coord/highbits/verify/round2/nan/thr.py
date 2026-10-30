import os, subprocess, sys, numpy as np
from concurrent.futures import ThreadPoolExecutor
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "nan_test.py")).read().split("cases = {}")[0])
def one(v):
    a = base(); a[3, :, 10:20, 10:20] = v
    src, dst = os.path.join(N, f"thr_{v:g}_in.mkv"), os.path.join(N, f"thr_{v:g}_out.mkv")
    enc(a, src)
    r = subprocess.run([sys.executable, "denoise_video_v5.py", src, dst, "--depth", "32", "--jobs", "2"], cwd=REPRO, capture_output=True, text=True)
    o = dec(dst)
    return v, r.returncode, int((~np.isfinite(o)).sum()), float(o[3, 0, 12:18, 12:18].mean())
with ThreadPoolExecutor(3) as ex:
    for v, rc, nf, m in ex.map(one, (1e3, 1e6, 1e9, 1e12, 1e15, 1e17, 1e19)):
        print(f"patch value {v:g}: rc={rc} out_nonfinite={nf} patch mean out={m:.6g}", flush=True)
