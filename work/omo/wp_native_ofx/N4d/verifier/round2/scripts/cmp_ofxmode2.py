import os, sys, subprocess
sys.path.insert(0, "/Users/seitan/Desktop/work/reverse/.claude/worktrees/n4-ofx/qtui/tools")
import compare_ofx as C
S = "/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/verify_n4d"
EXE = {"head": f"{S}/head/qtui/build/qtui_ofx_drv", "new": f"{S}/r2/qtui/qtui_ofx_drv"}
out = f"{S}/r2/ofxmode"; os.makedirs(out, exist_ok=True)
ins = C.inputs(out)
def run(side, args, data, script, tag):
    ini = os.path.join(out, f"{tag}_{side}.ini")
    if os.path.exists(ini): os.remove(ini)
    env = dict(os.environ, QT_QPA_PLATFORM="offscreen", NV_NATIVE="1", NV_UI_QT_SETTINGS=ini, NV_OFX_SCRIPT=script, HOME=f"{S}/home")
    r = subprocess.run([EXE[side], "--ofx", *args], input=data, capture_output=True, env=env, cwd=S, timeout=600)
    err = r.stderr.decode(errors="replace").splitlines()
    return r.returncode, r.stdout, [l for l in err if l.startswith("MSG: ")], [l for l in err if l.startswith(("OFX emit error", "usage:", "no frames"))]
bad = 0
cases = dict(C.CASES)
for name, (inp, d, sc, extra) in cases.items():
    W, H, data = ins[inp]
    args = [str(W), str(H), *(extra or ["30", "0"]), "--depth", str(d)]
    res = {s: run(s, args, data[d], C.SCRIPTS.get(sc, sc), name) for s in EXE}
    same = res["head"] == res["new"]
    bad += not same
    print(f"{name:24s} {'SAME' if same else 'DIFF'} stdout {len(res['new'][1])} B exit {res['new'][0]} msgs {len(res['new'][2])}", flush=True)
for args, data, tag in [([], b"", "noargs"), (["10"], b"", "badargs"), (["640", "360", "--depth", "32"], b"", "noframes")]:
    res = {s: run(s, args, data, "close", tag) for s in EXE}
    same = res["head"] == res["new"]; bad += not same
    print(f"{tag:24s} {'SAME' if same else 'DIFF'} exit {res['new'][0]} {res['new'][3][:1]}")
print("ALL SAME" if not bad else f"{bad} DIFF")
