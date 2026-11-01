import os, subprocess, re, sys, itertools
S = "/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/verify_n4d"; R = S + "/r3"
B = "/Users/seitan/Desktop/work/reverse/.claude/worktrees/n4-ofx/ofx/build"
DRV = "/Users/seitan/Desktop/work/reverse/.claude/worktrees/n4-ofx/qtui/build/qtui_ofx_session_drv"
helper = sys.argv[1] if len(sys.argv) > 1 else DRV
tagp = sys.argv[2] if len(sys.argv) > 2 else "drv"
clip = f"{S}/r2/ext/clip_E.f32"
res = {}
for closed in [(), (0,), (1,), (2,), (0, 1), (0, 2), (1, 2), (0, 1, 2)]:
    tag = f"{tagp}_c{''.join(map(str, closed)) or 'none'}"
    out = f"{R}/fd/o_{tag}.f32"; logf = out + ".log"
    for f in [out, logf, out + ".cp_0.json", out + ".cp_1.json"]:
        if os.path.exists(f): os.remove(f)
    env = dict(os.environ, DYLD_INSERT_LIBRARIES=f"{B}/libfsguard.dylib", HOME=f"{S}/home", QT_QPA_PLATFORM="offscreen",
               NV_UI_QT_SETTINGS=f"{S}/r2/ext/home/defaults_set.ini", MEATVIDEO_OFX_HELPER=helper)
    args = [f"{B}/minihost", f"{R}/build/MeatVideo.ofx.bundle", "--frames", clip, "--size", "400", "256", "12", "--first", "20",
            "--render", "23", "--env", "NV_OFX_SESSION_SCRIPT=frame 3;auto;aft;frame 5;set t_radius 4;apply", "--press", "prepare@25",
            "--render", "25,20,31", "--env", "NV_OFX_SESSION_SCRIPT=set amt_Cb 40;tune Y 1.3;apply", "--press", "adjust@28",
            "--render", "28", "--out", out, "--clip-out", out + ".cp", "--log", logf]
    p = subprocess.Popen(args, env=env, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                         preexec_fn=lambda c=closed: [os.close(fd) for fd in c])
    rc = p.wait(timeout=600)
    log = open(logf).read() if os.path.exists(logf) else ""
    pl = [l for l in log.splitlines() if l.startswith("press")]
    rl = [l for l in log.splitlines() if l.startswith("render")]
    ci = [l for l in log.splitlines() if l.startswith("create")]
    calls = re.findall(r"fsguard:   (.*)", log)
    st = lambda ls: [re.search(r"status=(\d+)", l).group(1) for l in ls]
    data = (open(out, "rb").read() if os.path.exists(out) else b"",
            [open(f"{out}.cp_{k}.json").read() if os.path.exists(f"{out}.cp_{k}.json") else None for k in range(2)])
    res[closed] = data
    same = data == res[()]
    msgs = [re.search(r'message="([^"]{0,100})', l).group(1) for l in pl if 'message="' in l]
    print(f"closed {str(closed):10s} rc={rc} create_adjust={'adjust_enabled=1' in (ci[0] if ci else '')} presses={st(pl)} "
          f"changed={[('clip_params=changed' in l) for l in pl]} renders={st(rl)} calls={len(calls)} identical_to_all_open={same} "
          f"marks={re.findall(r'host fd (\d): closed', log)} {msgs}")
