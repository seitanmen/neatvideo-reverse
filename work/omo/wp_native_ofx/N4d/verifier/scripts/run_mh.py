import os, subprocess, sys
S="/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/verify_n4d"
B="/Users/seitan/Desktop/work/reverse/.claude/worktrees/n4-ofx/ofx/build"
DRV="/Users/seitan/Desktop/work/reverse/.claude/worktrees/n4-ofx/qtui/build/qtui_ofx_session_drv"
def run(args, env_extra=None, timeout=300, bundle=None, guard=True):
    env = dict(os.environ, HOME=f"{S}/home", NV_UI_QT_SETTINGS=f"{S}/home/none.ini", MEATVIDEO_OFX_HELPER=DRV,
               QT_QPA_PLATFORM="offscreen")
    if guard: env["DYLD_INSERT_LIBRARIES"] = f"{B}/libfsguard.dylib"
    env.update(env_extra or {})
    for k, v in list(env.items()):
        if v is None: env.pop(k)
    try:
        p = subprocess.run([f"{B}/minihost", bundle or f"{B}/MeatVideo.ofx.bundle"] + args, env=env, capture_output=True, text=True, timeout=timeout)
        return p.returncode, p.stdout, p.stderr
    except subprocess.TimeoutExpired as e:
        return "TIMEOUT", (e.stdout or b"").decode() if isinstance(e.stdout, bytes) else (e.stdout or ""), ""
if __name__ == "__main__":
    import json
    sep = sys.argv.index("--")
    extra = json.loads(sys.argv[1]) if sep > 1 else {}
    rc, o, e = run(sys.argv[sep+1:], extra.get("env"), extra.get("timeout", 300))
    print("rc=", rc)
    for ln in o.splitlines():
        if ln.startswith(("create", "render", "press", "sync", "fsguard", "clip_params", "leaked")): print(ln[:400])
    print("stderr-tail:", e[-600:])
