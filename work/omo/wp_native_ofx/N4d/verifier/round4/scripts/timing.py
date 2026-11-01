import os, subprocess, re, time, sys
S = "/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/verify_n4d"; R = S + "/r4"
B = f"{R}/build"; DRV = f"{R}/qtui/qtui_ofx_session_drv"
env = dict(os.environ, HOME=f"{S}/home", NV_UI_QT_SETTINGS=f"{S}/home/none.ini", MEATVIDEO_OFX_HELPER=DRV, MEATVIDEO_OFX_TRACE="1",
           QT_QPA_PLATFORM="offscreen", NV_OFX_SESSION_SCRIPT="close")
clip = f"{R}/tp/clip_1080x11.f32"
for host, bundle, tag in (("cocoahost", f"{R}/ctl.bundle", "control(OFF)"), ("cocoahost", f"{B}/MeatVideo.ofx.bundle", "modal(ON)"), ("cocoahost", f"{R}/ctl.bundle", "control(OFF)"), ("cocoahost", f"{B}/MeatVideo.ofx.bundle", "modal(ON)")):
    p = subprocess.run([f"{B}/{host}", bundle, "--frames", clip, "--size", "1920", "1080", "11", "--press", "prepare@5", "--log", f"{R}/tp/log.txt"],
                       env=env, capture_output=True, text=True, timeout=900)
    log = open(f"{R}/tp/log.txt").read()
    pl = [l for l in log.splitlines() if l.startswith("press")]
    w = re.search(r"wall=([\d.]+)\.\.([\d.]+)", pl[0])
    t0 = float(w.group(1))
    shown = [l for l in p.stderr.splitlines() if "window shown" in l]
    print(f"{tag:13s} press wait {float(w.group(2))-t0:6.2f}s", [l for l in p.stderr.splitlines() if "helper wait" in l][-1:])
