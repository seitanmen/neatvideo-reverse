"""L5: the file descriptors of the ffmpeg children during a qtui_export run (lsof).  cwd = repro/."""
import os, subprocess, sys, time
O = "../sandbox/omo/wp_qtui_c/depth/export"
side = sys.argv[1] if len(sys.argv) > 1 else "cpp"
exe = ["../qtui/build/qtui_export"] if side == "cpp" else [sys.executable, "-m", "nv_ui_qt.export"]
p = subprocess.Popen([*exe, "../test.mp4", f"{O}/l5_{side}.mp4", "--ui-profile", f"{O}/profile_t.ui_profile.json", "--params",
                      f"{O}/a_t_default.ui_params.json", "--jobs", "2", "--crf", "16", "--start", "0", "--frames", "300"],
                     stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=dict(os.environ, QT_QPA_PLATFORM="offscreen"))
seen = {}
t0 = time.time()
while p.poll() is None and time.time() - t0 < 60:
    ps = subprocess.run(["ps", "-A", "-o", "pid=,ppid=,command="], capture_output=True, text=True).stdout.splitlines()
    rows = [l.split(None, 2) for l in ps]
    desc, grow = {str(p.pid)}, True
    while grow:
        n = len(desc)
        desc |= {r[0] for r in rows if len(r) > 2 and r[1] in desc}
        grow = len(desc) > n
    kids = [r for r in rows if len(r) > 2 and r[0] in desc and "ffmpeg" in r[2].split()[0]]
    for pid, _, cmd in kids:
        if pid in seen:
            continue
        lo = subprocess.run(["lsof", "-p", pid], capture_output=True, text=True).stdout.splitlines()
        fds = [l.split()[3] + " " + l.split()[4] for l in lo[1:] if l.split()[3][:1].isdigit()]
        if fds:
            seen[pid] = (cmd[:70], fds)
    if len(seen) >= 3:
        break
out = p.communicate()[0].decode()
for pid, (cmd, fds) in seen.items():
    print(pid, cmd)
    print("   fds:", fds)
print("export:", out.strip().splitlines()[-1])
