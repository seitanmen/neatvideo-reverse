"""Round 5 verifier's own adversarial cases on cocoahost (ModalWait active)."""
import sys, os, ctypes, time, re
sys.path.insert(0, "/Users/seitan/Desktop/work/reverse/.claude/worktrees/n4-ofx/ofx/tools")
import verify_modal as V
S = "/private/tmp/claude-501/-Users-seitan-Desktop-work-reverse/040b381d-b723-4608-b0cb-2e82227f9c2d/scratchpad/verify_n4d"; R = S + "/r5"
work = f"{R}/adv"; os.makedirs(f"{work}/home", exist_ok=True)
import shutil; shutil.copy(f"{R}/vm/cp_S-params_0.json", work)
V.BUNDLE = f"{R}/build/MeatVideo.ofx.bundle"
stubs = V.write_stubs(work, f"{work}/cp_S-params_0.json")
V.AS.CGEventSetIntegerValueField.restype = None
V.AS.CGEventSetIntegerValueField.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.c_int64]
def rep_key(code):
    e = V.key(code, True); V.AS.CGEventSetIntegerValueField(e, 8, 1); return e   # kCGKeyboardEventAutorepeat
def mk(kind): return V.mouse(kind)
want = sys.argv[1] if len(sys.argv) > 1 else ""
# my own stubs: the helper stops reading for 6 s mid-request / dies mid-request
extra = {
 "stall": "#!/bin/sh\nif [ \"$1\" = \"--ofx-default\" ]; then printf 'DEFAULTS 0 none none\\n'; exit 0; fi\nhead -c 3000000 > /dev/null\nsleep 6\ncat > /dev/null\nprintf 'CANCEL\\n'\n",
 "middie": "#!/bin/sh\nif [ \"$1\" = \"--ofx-default\" ]; then printf 'DEFAULTS 0 none none\\n'; exit 0; fi\nhead -c 3000000 > /dev/null\nexit 0\n",
}
for k, b in extra.items():
    open(f"{stubs}/{k}", "w").write(b); os.chmod(f"{stubs}/{k}", 0o755)
CLIP = f"{S}/r4/tp/clip_1080x11.f32"
def steps(): return ["--frames", CLIP, "--size", "1920", "1080", "11", "--press", "prepare@5"]
if not want or want == "input":
    plan = [(1.0, lambda pid: V.post(pid, rep_key(1), rep_key(1))),
            (1.5, lambda pid: V.post(pid, mk(3), mk(4))),
            (2.0, lambda pid: V.post(pid, V.flags(0x20000))),
            (2.5, lambda pid: V.post(pid, V.flags(0))),
            (3.0, lambda pid: V.post(pid, mk(2), V.key(1, False))),
            (3.5, lambda pid: V.post(pid, mk(1))),
            (5.0, lambda pid: V.post(pid, V.key(7, True), rep_key(7), V.key(7, False))),
            (9.6, lambda pid: V.post(pid, mk(2)))]
    r = V.run_case(work, "input2", V.BUNDLE, f"{stubs}/apply", {"MV_STUB_SLEEP": "7"},
                   ["--frames", f"{S}/r3/vo_full/clip_A.f32", "--size", "640", "360", "8", "--press", "prepare@3"], ["--held"],
                   plan=plan, cpu_at=(1.2, 5.2), sample_at=(4.0,))
    V.show("input2", r)
    print("   wait", round(r["wait_s"], 2), "dispatched during the wait:", r["inputs_in_wait"], "after (1 s):", r["inputs_after"])
for name in ("stall", "middie"):
    if want and want != name: continue
    r = V.run_case(work, name, V.BUNDLE, f"{stubs}/{name}", {}, steps(), cpu_at=(1.0, 5.0), sample_at=(3.0,), timeout=120)
    V.show(name, r)
    print("   wait", round(r["wait_s"], 2), "cpu", r["cpu"], "press:", r["press"][:200])
