import os, subprocess, re, time
S = "/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/verify_n4d"; R = S + "/r5"
B = f"{R}/build"; DRV = f"{R}/qtui/qtui_ofx_session_drv"
def tsec(pid):
    s = subprocess.run(["ps", "-o", "time=", "-p", str(pid)], capture_output=True, text=True).stdout.strip()
    m, x = s.split(":"); return int(m) * 60 + float(x)
for b, tag in ((f"{B}/MeatVideo.ofx.bundle", "modal"), (f"{R}/ctl.bundle", "control"), (f"{B}/MeatVideo.ofx.bundle", "modal")):
    e = dict(os.environ, HOME=f"{S}/home", NV_UI_QT_SETTINGS=f"{S}/home/none.ini", MEATVIDEO_OFX_HELPER=DRV, QT_QPA_PLATFORM="offscreen", NV_OFX_SESSION_SCRIPT="sleep 9000;close")
    p = subprocess.Popen([f"{B}/cocoahost", b, "--frames", f"{S}/r3/vo_full/clip_A.f32", "--size", "640", "360", "8", "--press", "prepare@4", "--log", f"{R}/m/c.log"], env=e, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(4.0); c0 = tsec(p.pid); time.sleep(4.0); c1 = tsec(p.pid)
    s = subprocess.run(["sample", str(p.pid), "1", "-mayDie"], capture_output=True, text=True).stdout
    p.wait(timeout=60)
    main = s.split("com.apple.main-thread")[1].split("Thread_")[0]
    pollc = sum(int(x) for x in re.findall(r"(\d+) poll  \(in libsystem_kernel", main))
    readc = sum(int(x) for x in re.findall(r"(\d+) read  \(in libsystem_kernel", main))
    drainc = sum(int(x) for x in re.findall(r"(\d+) mvofx::ModalWait::drain\(\)", main))
    tot = int(re.search(r"(\d+) ", main.strip().splitlines()[1]).group(1))
    print(f"{tag:8s} CPU/4s {c1 - c0:.3f}s  main samples {tot}: poll(kernel) {pollc}, read(kernel) {readc}, drain {drainc}")
