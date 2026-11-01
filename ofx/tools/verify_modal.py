#!/usr/bin/env python3
"""verify_modal - the host stays responsive while a button's window is open, without spending CPU and without
slowing the window down (N4d, ofx/src/modal_wait.h).

ofx/build/cocoahost (a running NSApplication; the minihost session on its main thread) loads the bundle and presses
Prepare Noise Profile; the plug-in's helper is a stub (MEATVIDEO_OFX_HELPER) that replies after a delay, or the session
driver of the C++ UI (offscreen).  Every case runs on the bundle under test and, with --control, on a bundle built with
MV_MODAL_WAIT=OFF (the blocking wait).  From this process (another process) the test:
  - reads the host's CPU time with ps(1) twice, 4 s apart, inside the wait (pass: < 0.2 s);
  - samples the host's main thread with sample(1) (1 s, twice in the wait): samples asleep in the kernel (poll /
    mach_msg / kevent / wait4 / read / write: the thread is not running) and samples that run in the plug-in's event
    drain; pass: >= 80 % asleep and the drain present (the events are taken);
  - posts synthetic input to the host (CGEventPostToPid: no extra privileges were needed) and reads which of it the host
    dispatched before, during and after the wait (events left unread in the queue are what makes the system call an
    application not responding: the blocking wait leaves them queued and they are dispatched after it);
  - asks SkyLight's CGSEventIsAppUnresponsive (reported only: it stays false even for a SIGSTOPped host that has events
    posted to it - the flag needs events the WindowServer routes itself, from the real cursor / keyboard);
and reads the host's own record (stderr: default-mode timer ticks, the input it dispatched with the event types, the
main-queue block's time) and the minihost log (the press's wall-clock interval, the outcome).  The open time (the
press's interval for a session that closes at once) of 1080p x 11 frames is compared with the control (pass: <= 1.5x).

  python3 ofx/tools/verify_modal.py --control BUNDLE [--work DIR] [--only NAME]
Needs ofx/build (cocoahost, minihost, the bundle), qtui/build/qtui_ofx_session_drv and ofx/build/verify (clips and a
ClipParams file to reply with, from verify_ofx.py).  A small "cocoahost" window appears at the bottom left of the
screen for each case; it does not take the focus.
"""
import argparse
import ctypes
import glob
import os
import re
import subprocess
import sys
import threading
import time

import numpy as np

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
BUILD = os.path.join(ROOT, "ofx", "build")
COCOAHOST = os.path.join(BUILD, "cocoahost")
BUNDLE = os.path.join(BUILD, "MeatVideo.ofx.bundle")
DRV = os.path.join(ROOT, "qtui", "build", "qtui_ofx_session_drv")
FSGUARD = os.path.join(BUILD, "libfsguard.dylib")


# ---------------------------------------------------------------- the WindowServer's view and synthetic input
class PSN(ctypes.Structure):
    _fields_ = [("high", ctypes.c_uint32), ("low", ctypes.c_uint32)]


class CGPoint(ctypes.Structure):
    _fields_ = [("x", ctypes.c_double), ("y", ctypes.c_double)]


SKY = ctypes.CDLL("/System/Library/PrivateFrameworks/SkyLight.framework/SkyLight")
AS = ctypes.CDLL("/System/Library/Frameworks/ApplicationServices.framework/ApplicationServices")
SKY.CGSMainConnectionID.restype = ctypes.c_int
SKY.CGSEventIsAppUnresponsive.restype = ctypes.c_bool
SKY.CGSEventIsAppUnresponsive.argtypes = [ctypes.c_int, ctypes.POINTER(PSN)]
AS.GetProcessForPID.restype = ctypes.c_int32
AS.GetProcessForPID.argtypes = [ctypes.c_int, ctypes.POINTER(PSN)]
for f, r, a in (("CGEventCreateKeyboardEvent", ctypes.c_void_p, [ctypes.c_void_p, ctypes.c_uint16, ctypes.c_bool]),
                ("CGEventCreateMouseEvent", ctypes.c_void_p, [ctypes.c_void_p, ctypes.c_uint32, CGPoint, ctypes.c_uint32]),
                ("CGEventCreate", ctypes.c_void_p, [ctypes.c_void_p]),
                ("CGEventSetType", None, [ctypes.c_void_p, ctypes.c_uint32]),
                ("CGEventSetFlags", None, [ctypes.c_void_p, ctypes.c_uint64]),
                ("CGEventPostToPid", None, [ctypes.c_int, ctypes.c_void_p]),
                ("CFRelease", None, [ctypes.c_void_p])):
    getattr(AS, f).restype = r
    getattr(AS, f).argtypes = a


def unresponsive(pid):
    psn = PSN()
    if AS.GetProcessForPID(pid, ctypes.byref(psn)) != 0:
        return None
    return bool(SKY.CGSEventIsAppUnresponsive(SKY.CGSMainConnectionID(), ctypes.byref(psn)))


def post(pid, *events):
    for e in events:
        AS.CGEventPostToPid(pid, e)
        AS.CFRelease(e)
    return len(events)


def key(code, down):
    return AS.CGEventCreateKeyboardEvent(None, code, down)


def mouse(kind):            # kCGEventLeftMouseDown 1, LeftMouseUp 2, MouseMoved 5
    return AS.CGEventCreateMouseEvent(None, kind, CGPoint(120, 0), 0)


def flags(mask):            # kCGEventFlagsChanged 12 with the modifier flags `mask` (shift 0x20000)
    e = AS.CGEventCreate(None)
    AS.CGEventSetType(e, 12)
    AS.CGEventSetFlags(e, mask)
    return e


def cpu_seconds(pid):
    t = subprocess.run(["ps", "-o", "time=", "-p", str(pid)], capture_output=True, text=True).stdout.strip()
    if not t:
        return None
    parts = [float(x) for x in t.replace("-", ":").split(":")]
    return sum(v * 60 ** k for k, v in enumerate(reversed(parts)))


def main_thread_profile(text):
    """sample(1)'s call tree of the main thread: samples in all, asleep in the kernel (the leaves poll / mach_msg2_trap /
    kevent / wait4 / read / write / __semwait), and running in the plug-in's event drain"""
    i = text.find("com.apple.main-thread")
    if i < 0:
        return None
    line = text[text.rfind("\n", 0, i) + 1:text.find("\n", i)]
    m = re.search(r"(\d+) Thread_", line)
    total = int(m.group(1)) if m else 0
    block = text[i:]
    m = re.search(r"\n    \d+ Thread_", block)
    block = block[:m.start()] if m else block
    count = lambda pat: sum(int(x) for x in re.findall(r"(\d+) " + pat, block))
    sleep = count(r"(?:poll|mach_msg2_trap|kevent|kevent64|wait4|read|write|__read_nocancel|__write_nocancel|"
                  r"__semwait_signal|__psynch_cvwait)  \(in libsystem_kernel")
    return {"total": total, "asleep": sleep, "drain": count(r"mvofx::ModalWait::drain"),
            "blocked_io": count(r"(?:read|write|__read_nocancel|__write_nocancel|wait4)  \(in libsystem_kernel")}


# ---------------------------------------------------------------- one case
def run_case(work, name, bundle, helper, env_extra, steps, flags_=(), timeout=600, cpu_at=(2.0, 6.0),
             sample_at=(3.0, 6.5), plan=None, clip=None, size=(640, 360, 8)):
    """plan: (seconds after the session start, callable(pid) -> number of events posted) instead of the default posts
    (a key down / up, a mouse move every 0.5 s, a left click every 2 s)"""
    clip = clip or os.path.join(BUILD, "verify", "clip_A.f32")
    log = os.path.join(work, f"modal_{name}.log")
    if os.path.exists(log):
        os.remove(log)
    args = [COCOAHOST, *flags_, bundle, "--frames", clip, "--size", *map(str, size), "--log", log] + steps
    env = dict(os.environ, MEATVIDEO_OFX_HELPER=helper, HOME=os.path.join(work, "home"), QT_QPA_PLATFORM="offscreen",
               NV_UI_QT_SETTINGS=os.path.join(work, "home", "defaults_none.ini"), MEATVIDEO_OFX_TRACE="1",
               DYLD_INSERT_LIBRARIES=FSGUARD, **env_extra)
    p = subprocess.Popen(args, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    lines, pid, done, started = [], [None], threading.Event(), [None]

    def reader():
        for ln in p.stderr:
            lines.append((time.time(), ln.rstrip("\n")))
            m = re.match(r"cocoahost: pid (\d+) ready", ln)
            if m:
                pid[0] = int(m.group(1))
            if ln.startswith("cocoahost: session start"):
                started[0] = time.time()
            if ln.startswith("cocoahost: session done"):
                done.set()
    th = threading.Thread(target=reader, daemon=True)
    th.start()
    samples, posted, profiles, cpus = [], [], [], []
    todo_s, todo_c, todo_p = list(sample_at), list(cpu_at), list(plan or [])
    t0 = time.time()
    k = 0
    while p.poll() is None and time.time() - t0 < timeout:
        el = time.time() - started[0] if started[0] else None
        if pid[0] and not done.is_set():
            if el is not None and todo_c and el >= todo_c[0]:
                cpus.append((time.time(), cpu_seconds(pid[0])))
                todo_c.pop(0)
            if el is not None and todo_s and el >= todo_s[0]:
                f = os.path.join(work, f"modal_{name}_{todo_s[0]:.1f}.sample.txt")
                subprocess.run(["sample", str(pid[0]), "1", "-mayDie", "-file", f], capture_output=True)
                profiles.append((todo_s[0], main_thread_profile(open(f).read()) if os.path.exists(f) else None))
                todo_s.pop(0)
            if plan is not None:
                while el is not None and todo_p and el >= todo_p[0][0]:
                    posted.append((time.time(), todo_p.pop(0)[1](pid[0])))
            elif k % 2 == 0:
                n = k // 2
                evs = [key(0, True), key(0, False), mouse(5)] + ([mouse(1), mouse(2)] if n % 4 == 0 else [])
                posted.append((time.time(), post(pid[0], *evs)))
            samples.append((time.time(), unresponsive(pid[0])))
            k += 1
        time.sleep(0.25)
    if p.poll() is None:
        p.kill()
    p.wait()
    th.join(5)
    text = open(log).read() if os.path.exists(log) else ""
    press = [ln for ln in text.splitlines() if ln.startswith("press ")]
    m = re.search(r"wall=([\d.]+)\.\.([\d.]+)", press[0]) if press else None
    w0, w1 = (float(m.group(1)), float(m.group(2))) if m else (0.0, 0.0)
    err = [ln for _, ln in lines]
    ticks = [float(ln.split()[1]) for ln in err if ln.startswith("tick ")]
    inputs = [(int(ln.split()[1]), float(ln.split()[2])) for ln in err if ln.startswith("input ")]
    block = [float(ln.split()[1]) for ln in err if ln.startswith("block ")]
    inside = lambda t: w0 + 0.05 < t < w1 - 0.05
    cpu_in = [(t, c) for t, c in cpus if w0 < t < w1 and c is not None]
    return {
        "rc": p.returncode, "press": press[0] if press else "", "wait_s": w1 - w0,
        "unresponsive": len([t for t, u in samples if u and w0 < t < w1]),
        "samples": len([t for t, u in samples if u is not None and w0 < t < w1]),
        "cpu": (cpu_in[-1][1] - cpu_in[0][1], cpu_in[-1][0] - cpu_in[0][0]) if len(cpu_in) >= 2 else None,
        "ticks_in_wait": len([t for t in ticks if inside(t)]), "ticks_total": len(ticks),
        "inputs_before": len([t for _, t in inputs if t < w0]),
        "inputs_in_wait": [ty for ty, t in inputs if inside(t)],
        "inputs_after": len([t for _, t in inputs if w1 <= t < w1 + 1.0]),
        "posted_in_wait": sum(n for t, n in posted if w0 < t < w1),
        "block_after_wait": (block[0] - w1) if block else None,
        "pokes": [ln for ln in text.splitlines() if ln.startswith("poke ")],
        "trace": [ln for ln in err if ln.startswith("meatvideo-ofx: helper wait")],
        "profiles": profiles, "log": text, "stderr": err,
    }


def show(name, r):
    pr = r["press"]
    status = re.search(r"status=(\d+)", pr)
    msg = re.search(r'message="([^"]*)"', pr)
    cpm = re.search(r"clip_params=(\w+)", pr)
    print(f"== {name}: rc {r['rc']}; press status {status.group(1) if status else '?'}, "
          f"clip_params {cpm.group(1) if cpm else '?'}, wait {r['wait_s']:.2f} s, "
          f"message \"{(msg.group(1) if msg else '')[:110]}\"")
    if r["cpu"]:
        print(f"   host CPU in the wait (ps, another process): {r['cpu'][0]:.3f} s in {r['cpu'][1]:.2f} s")
    for at, pf in r["profiles"]:
        print(f"   main thread {at:.1f} s into the session (sample): " + (
            f"{pf['asleep']}/{pf['total']} asleep in the kernel ({pf['blocked_io']} of them in read / write / wait4), "
            f"{pf['drain']} in the event drain" if pf else "not sampled"))
    print(f"   input: {r['inputs_before']} dispatched before the wait, {r['posted_in_wait']} posted during it, "
          f"dispatched during it {r['inputs_in_wait']} (NSEvent types), {r['inputs_after']} in the 1 s after it; "
          f"CGSEventIsAppUnresponsive {r['unresponsive']}/{r['samples']}")
    print(f"   default-mode timer: {r['ticks_in_wait']} ticks during the wait ({r['ticks_total']} in all); main-queue "
          + (f"block ran {r['block_after_wait']:+.2f} s after the wait" if r["block_after_wait"] is not None else "block: none"))
    for p in r["pokes"]:
        print(f"   {p[:200]}")
    print(f"   plug-in trace: {r['trace']}")


def modal_ok(r):
    """served and asleep: little CPU, the main thread mostly in the kernel, events taken, input not delivered"""
    prof = r["profiles"] and all(pf and pf["asleep"] >= 0.8 * pf["total"] and pf["drain"] > 0 for _, pf in r["profiles"])
    return (r["rc"] == 0 and r["cpu"] is not None and r["cpu"][0] < 0.2 and prof and r["ticks_in_wait"] == 0 and
            r["inputs_before"] > 0 and r["inputs_after"] == 0 and r["block_after_wait"] is not None and
            r["block_after_wait"] >= -0.01 and
            r["trace"][-1:] == ["meatvideo-ofx: helper wait: host event loop (MeatVideoOFXModal)"])


def write_stubs(work, reply):
    d = os.path.join(work, "modal_stubs")
    os.makedirs(d, exist_ok=True)
    head = "#!/bin/sh\nif [ \"$1\" = \"--ofx-default\" ]; then printf 'DEFAULTS 0 none none\\n'; exit 0; fi\ncat > /dev/null\n"
    stubs = {
        "apply": head + f"sleep ${{MV_STUB_SLEEP:-10}}\nn=$(wc -c < '{reply}')\nprintf 'APPLY %d\\n' $n\ncat '{reply}'\n",
        "die": head + "sleep 7\nkill -9 $$\n",
        "eof": head + "sleep 9\nexit 0\n",
        "drain": head + "printf 'CANCEL\\n'\n",
    }
    for k, body in stubs.items():
        path = os.path.join(d, k)
        open(path, "w").write(body)
        os.chmod(path, 0o755)
    return d


def clip_1080(work):
    """test_v02 n = 312..322 as RGBA float32 frames (1920 x 1080 x 11, as the verifier's timing.py)"""
    path = os.path.join(work, "clip_1080x11.f32")
    if not os.path.exists(path):
        sys.path.insert(0, os.path.dirname(__file__))
        import verify_ofx
        verify_ofx.rgba(verify_ofx.decode("test_v02.mp4", 312, 11)).tofile(path)
    return path


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--work", default=os.path.join(BUILD, "verify"))
    ap.add_argument("--control", required=True)
    ap.add_argument("--only", default="")
    a = ap.parse_args()
    os.makedirs(os.path.join(a.work, "home"), exist_ok=True)
    reply = sorted(glob.glob(os.path.join(a.work, "cp_S-params_*.json")))[0]
    stubs = write_stubs(a.work, reply)
    fails = []
    want = lambda name: not a.only or re.search(a.only, name)

    def check(name, ok):
        print(f"   -> {'ok' if ok else 'FAIL'}")
        if not ok:
            fails.append(name)

    # 1. Apply after 10 s, a second press from a thread: served / asleep, against the control (blocked)
    if want("apply"):
        for tag, bundle in (("", BUNDLE), ("-control", a.control)):
            r = run_case(a.work, "apply-10s" + tag, bundle, os.path.join(stubs, "apply"), {"MV_STUB_SLEEP": "10"},
                         ["--press", "prepare@3"], ["--poke"])
            show("apply-10s" + (" (control: MV_MODAL_WAIT=OFF, the blocking wait)" if tag else ""), r)
            poked = any("from background thread" in p and "already open" in p and "status=0" in p for p in r["pokes"])
            changed = " status=0 " in r["press"] and "clip_params=changed" in r["press"]
            if not tag:
                check("apply-10s", modal_ok(r) and changed and poked and r["inputs_in_wait"] == [])
            else:
                blocked = r["profiles"] and all(pf and pf["blocked_io"] >= 0.8 * pf["total"] and pf["drain"] == 0
                                                for _, pf in r["profiles"])
                ok = changed and poked and blocked and r["inputs_after"] > 0 and \
                    r["trace"][-1:] == ["meatvideo-ofx: helper wait: blocking"]
                print(f"   -> control {'as expected (the main thread blocked in read, input left queued)' if ok else 'NOT as expected'}")
                if not ok:
                    fails.append("apply-10s-control")
    # 2. the edge cases
    for name, helper, env, want_msg, at in (
            ("helper-dies", os.path.join(stubs, "die"), {}, "signal 9", ((1.0, 5.0), (2.0,))),
            ("no-reply-eof", os.path.join(stubs, "eof"), {}, "unexpected reply (0 bytes)", ((2.0, 6.0), (3.0, 6.5))),
            ("window-closed", DRV, {"NV_OFX_SESSION_SCRIPT": "sleep 9000;close"}, "clip_params=unchanged",
             ((2.0, 6.0), (3.0, 6.5)))):
        if not want(name):
            continue
        r = run_case(a.work, name, BUNDLE, helper, env, ["--press", "prepare@3"], cpu_at=at[0], sample_at=at[1])
        show(name, r)
        check(name, modal_ok(r) and want_msg in r["press"] and r["inputs_in_wait"] == [])
    # 3. R4-3: releases of a button / key pressed before the wait reach the host, presses during it are discarded
    #    with their releases, modifier changes reach the host
    if want("held"):
        plan = [(1.0, lambda pid: post(pid, mouse(2), key(1, False), flags(0x20000), flags(0))),
                (3.0, lambda pid: post(pid, mouse(1), mouse(2), key(0, True), key(0, False), mouse(5)))]
        r = run_case(a.work, "held", BUNDLE, os.path.join(stubs, "apply"), {"MV_STUB_SLEEP": "8"},
                     ["--press", "prepare@3"], ["--held"], plan=plan)
        show("held (left button and 's' pressed before the wait, released during it)", r)
        got = sorted(r["inputs_in_wait"])
        ok = modal_ok(r) and got.count(2) == 1 and got.count(11) == 1 and got.count(12) == 2 and \
            not any(t in got for t in (1, 5, 10))
        print(f"   dispatched during the wait: {got} (want one LeftMouseUp 2, one KeyUp 11, two FlagsChanged 12; no "
              f"LeftMouseDown 1 / MouseMoved 5 / KeyDown 10)")
        check("held", ok)
    # 4. R4-4: DestroyInstance (from another thread) while the window is open: freed after the wait (MallocScribble)
    if want("destroy"):
        r = run_case(a.work, "destroy", BUNDLE, os.path.join(stubs, "apply"),
                     {"MV_STUB_SLEEP": "8", "MallocScribble": "1", "MallocGuardEdges": "1"},
                     ["--press", "prepare@3"], ["--destroy"])
        show("destroy (DestroyInstance from a thread 3 s into the wait)", r)
        ok = r["rc"] == 0 and " status=0 " in r["press"] and "clip_params=unchanged" in r["press"] and \
            any("poke destroy from background thread" in p and "status=0" in p for p in r["pokes"])
        check("destroy", ok)
    # 5. the open time (R4-2): a session that closes at once, 1080p x 11 frames, against the control
    if want("open"):
        clip = clip_1080(a.work)
        for helper, label in ((DRV, "C++ UI driver, script close"), (os.path.join(stubs, "drain"), "drain stub")):
            t = {}
            for tag, bundle in (("control", a.control), ("modal", BUNDLE), ("control", a.control), ("modal", BUNDLE)):
                r = run_case(a.work, f"open-{tag}", bundle, helper, {"NV_OFX_SESSION_SCRIPT": "close"},
                             ["--press", "prepare@5"], cpu_at=(), sample_at=(), plan=[], clip=clip,
                             size=(1920, 1080, 11))
                t.setdefault(tag, []).append(r["wait_s"])
            ratio = max(t["modal"]) / min(t["control"])
            print(f"== open time, 1080p x 11 frames ({label}): control {t['control']} s, modal {t['modal']} s "
                  f"(worst / best ratio {ratio:.2f})")
            check(f"open {label}", ratio <= 1.5)
    # 6. a headless host (ofx/build/minihost: no NSApp) keeps the blocking wait
    if want("minihost"):
        env = dict(os.environ, MEATVIDEO_OFX_HELPER=os.path.join(stubs, "apply"), MV_STUB_SLEEP="1",
                   MEATVIDEO_OFX_TRACE="1", HOME=os.path.join(a.work, "home"),
                   NV_UI_QT_SETTINGS=os.path.join(a.work, "home", "defaults_none.ini"))
        p = subprocess.run([os.path.join(BUILD, "minihost"), BUNDLE, "--frames", os.path.join(BUILD, "verify", "clip_A.f32"),
                            "--size", "640", "360", "8", "--press", "prepare@3"], env=env, capture_output=True, text=True,
                           timeout=120)
        trace = [ln for ln in p.stderr.splitlines() if ln.startswith("meatvideo-ofx: helper wait")]
        press = [ln for ln in p.stdout.splitlines() if ln.startswith("press ")]
        print(f"== minihost (headless, no NSApp): plug-in trace {trace}; {press[0][:90] if press else '?'}")
        check("minihost", p.returncode == 0 and trace == ["meatvideo-ofx: helper wait: blocking"] * 2 and press and
              " status=0 " in press[0] and "clip_params=changed" in press[0])
    print("ALL OK" if not fails else "FAILURES: " + ", ".join(fails))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
