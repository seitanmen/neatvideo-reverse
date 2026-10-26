"""Kill the CLI's ffmpeg encoder mid-run (SIGKILL) and check: rc, no traceback, dst / EXR frames removed,
unrelated files kept. Run from repro/."""
import glob, os, signal, subprocess, sys, time
K = os.path.dirname(os.path.abspath(__file__))
def encoder_pids(tag):
    out = subprocess.run(["pgrep", "-f", f"^ffmpeg -y .*{tag}"], capture_output=True, text=True).stdout.split()
    return [int(p) for p in out]
def case(name, dst, args, ready, unrelated=()):
    for u in unrelated:
        open(u, "w").write("keep")
    p = subprocess.Popen([sys.executable, "denoise_video_v5.py", "../test.mp4", dst, *args, "--jobs", "2"],
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    t0 = time.time(); killed = None
    while p.poll() is None and time.time() - t0 < 300:
        if ready():
            pids = encoder_pids(os.path.basename(dst).replace("%05d", "%05d"))
            if pids:
                n_before = len(glob.glob(os.path.join(os.path.dirname(dst), "cx_*.exr"))) if "%" in dst else os.path.getsize(dst)
                for q in pids: os.kill(q, signal.SIGKILL)
                killed = (pids, n_before); break
        time.sleep(0.2)
    out, _ = p.communicate(timeout=300)
    open(os.path.join(K, name + ".log"), "w").write(out)
    d = os.path.dirname(dst)
    print(f"{name}: killed={killed} rc={p.returncode} traceback={'Traceback' in out} "
          f"dst_exists={os.path.exists(dst) if '%' not in dst else None} dir_now={sorted(os.listdir(d))[:12]} "
          f"unrelated_kept={all(os.path.exists(u) for u in unrelated)}\n   last: {out.strip().splitlines()[-1][:200]}", flush=True)
# 1. legacy 8-bit mp4
os.makedirs(os.path.join(K, "mp4"), exist_ok=True)
d1 = os.path.join(K, "mp4", "k8.mp4")
case("k8_mp4", d1, [], lambda: os.path.exists(d1) and os.path.getsize(d1) > 0 or bool(encoder_pids("k8.mp4")) and time.sleep(3) is None,
     [os.path.join(K, "mp4", "k8_other.mp4")])
# 2. 10-bit mkv
os.makedirs(os.path.join(K, "mkv"), exist_ok=True)
d2 = os.path.join(K, "mkv", "k10.mkv")
case("k10_mkv", d2, ["--depth", "10"], lambda: os.path.exists(d2) and os.path.getsize(d2) > 200000, [os.path.join(K, "mkv", "k10.mkv.keep")])
# 3. EXR sequence
ed = os.path.join(K, "exr"); os.makedirs(ed, exist_ok=True)
d3 = os.path.join(ed, "cx_%05d.exr")
unrel = [os.path.join(ed, n) for n in ("other_00001.exr", "cx_00001.exr.keep", "cx_1.exr", "cxx_00001.exr", "cx_123456.exr", "cx.json")]
case("k32_exr", d3, ["--depth", "32"], lambda: len(glob.glob(os.path.join(ed, "cx_0*.exr"))) >= 4, unrel)
for u in unrel: print("   ", os.path.basename(u), "kept" if os.path.exists(u) else "REMOVED")
