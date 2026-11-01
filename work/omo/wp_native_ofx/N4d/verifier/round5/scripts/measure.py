import os, subprocess, re, time, sys
S = "/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/verify_n4d"; R = S + "/r5"
B = f"{R}/build"; DRV = f"{R}/qtui/qtui_ofx_session_drv"
ON, OFF = f"{B}/MeatVideo.ofx.bundle", f"{R}/ctl.bundle"
CLIP = (f"{S}/r4/tp/clip_1080x11.f32", "1920", "1080", "11", "prepare@5")
def env(script, helper=DRV, **kw):
    e = dict(os.environ, HOME=f"{S}/home", NV_UI_QT_SETTINGS=f"{S}/home/none.ini", MEATVIDEO_OFX_HELPER=helper, MEATVIDEO_OFX_TRACE="1",
             QT_QPA_PLATFORM="offscreen", NV_OFX_SESSION_SCRIPT=script); e.update(kw); return e
def args(bundle, log, clip=CLIP):
    return [f"{B}/cocoahost", bundle, "--frames", clip[0], "--size", clip[1], clip[2], clip[3], "--press", clip[4], "--log", log]
def wall(log):
    pl = [l for l in open(log).read().splitlines() if l.startswith("press")]
    w = re.search(r"wall=([\d.]+)\.\.([\d.]+)", pl[0]); return float(w.group(2)) - float(w.group(1)), re.search(r"status=\d", pl[0]).group(0)
print("== window-open time, 1080p x 11, script close (alternating)")
for k in range(3):
    for b, tag in ((OFF, "control"), (ON, "modal")):
        log = f"{R}/m/t.log"; p = subprocess.run(args(b, log), env=env("close"), capture_output=True, text=True, timeout=600)
        d, st = wall(log); print(f"   {tag:8s} {d:.3f}s {st}", [l for l in p.stderr.splitlines() if 'helper wait' in l][-1:])
def tsec(pid):
    s = subprocess.run(["ps", "-o", "time=", "-p", str(pid)], capture_output=True, text=True).stdout.strip()
    m, x = s.split(":"); return int(m) * 60 + float(x)
print("== CPU and main-thread sample while the window is open (sleep 9000;close, 640x360x8)")
small = (f"{S}/r3/vo_full/clip_A.f32", "640", "360", "8", "prepare@4")
for b, tag in ((ON, "modal"), (OFF, "control")):
    log = f"{R}/m/c.log"
    p = subprocess.Popen(args(b, log, small), env=env("sleep 9000;close"), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(4.0); c0 = tsec(p.pid); time.sleep(4.0); c1 = tsec(p.pid)
    s = subprocess.run(["sample", str(p.pid), "1", "-mayDie"], capture_output=True, text=True).stdout
    p.wait(timeout=60)
    open(f"{R}/m/sample_{tag}.txt", "w").write(s)
    main = s.split("com.apple.main-thread")[1].split("\n    Thread_")[0] if "com.apple.main-thread" in s else ""
    # leaf counts in the main thread: lines whose next line is less indented are leaves; approximate with kernel leaf names
    tot = re.search(r"^\s*(\d+)\s", main.splitlines()[1]).group(1) if main else "?"
    leaves = {n: len(re.findall(r"\b(\d+)\s+" + n + r"\s", main)) for n in ("poll", "__poll", "mach_msg2_trap", "read", "write", "__wait4")}
    cnt = {}
    for n in ("poll", "mach_msg2_trap", "read", "__wait4"):
        m = re.findall(r"(\d+)\s+" + re.escape(n) + r"\s+\(in libsystem_kernel", main)
        cnt[n] = sum(map(int, m))
    print(f"   {tag:8s} CPU over 4 s: {c1 - c0:.3f}s; main-thread samples {tot}, kernel leaves {cnt}; drain frames {main.count('ModalWait::drain')}")
