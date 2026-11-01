#!/usr/bin/env python3
"""verify_modal - the host stays responsive while a button's window is open (N4d, ofx/src/modal_wait.h).

ofx/build/cocoahost (a running NSApplication; the minihost session on its main thread) loads the bundle and presses
Prepare Noise Profile; the plug-in's helper is a stub (MEATVIDEO_OFX_HELPER) that replies after a delay, or the session
driver of the C++ UI (offscreen) with a delayed close.  From this process (another process) the test:
  - samples the host's main thread with sample(1) (1 s, twice in the wait): how many samples are in the plug-in's
    event pump (ModalWait::pump -> -[NSApplication nextEventMatchingMask:...], i.e. the host takes its events from the
    WindowServer) and how many are blocked in read / write / wait4;
  - posts synthetic input to the host every 0.5 s (CGEventPostToPid: a key down / up, a mouse move, a left click; no
    extra privileges were needed here) and reads which of them the host dispatched before, during and after the wait
    (events left unread in the queue are what makes the system call an application not responding; the blocking wait
    leaves them queued and they are dispatched after it, the pump takes them and discards them);
  - asks SkyLight's CGSEventIsAppUnresponsive(CGSMainConnectionID(), psn of the host) every 0.25 s.  Observed: it stays
    false even for a SIGSTOPped host that has events posted to it (the flag needs events the WindowServer routes itself,
    from the real cursor / keyboard), so it is reported but not used to decide;
and reads the host's own record (stderr: default-mode timer ticks, the input events it dispatched, the main-queue
block's time) and the minihost log (the press's wall-clock interval, the outcome).

  python3 ofx/tools/verify_modal.py [--work DIR] [--control BUNDLE] [--only NAME]
    --control BUNDLE: also run the cases on a bundle built with MV_MODAL_WAIT=OFF (the blocking wait): the control
Needs ofx/build (cocoahost, minihost, the bundle), qtui/build/qtui_ofx_session_drv and a ClipParams file to reply with
(made by verify_ofx.py: ofx/build/verify/cp_*.json).  A small "cocoahost" window appears at the bottom left of the
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
AS.CGEventCreateKeyboardEvent.restype = ctypes.c_void_p
AS.CGEventCreateKeyboardEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint16, ctypes.c_bool]
AS.CGEventCreateMouseEvent.restype = ctypes.c_void_p
AS.CGEventCreateMouseEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint32, CGPoint, ctypes.c_uint32]
AS.CGEventPostToPid.argtypes = [ctypes.c_int, ctypes.c_void_p]
AS.CFRelease.argtypes = [ctypes.c_void_p]


def unresponsive(pid):
    psn = PSN()
    if AS.GetProcessForPID(pid, ctypes.byref(psn)) != 0:
        return None
    return bool(SKY.CGSEventIsAppUnresponsive(SKY.CGSMainConnectionID(), ctypes.byref(psn)))


def post_input(pid, k):
    """a key down / up, a mouse move and (every 4th time) a left click on the host's window, to the pid only"""
    evs = [AS.CGEventCreateKeyboardEvent(None, 0, True), AS.CGEventCreateKeyboardEvent(None, 0, False),
           AS.CGEventCreateMouseEvent(None, 5, CGPoint(120, 0), 0)]                     # kCGEventMouseMoved
    if k % 4 == 0:
        evs += [AS.CGEventCreateMouseEvent(None, 1, CGPoint(120, 0), 0),               # left down / up
                AS.CGEventCreateMouseEvent(None, 2, CGPoint(120, 0), 0)]
    for e in evs:
        AS.CGEventPostToPid(pid, e)
        AS.CFRelease(e)
    return len(evs)


def main_thread_profile(text):
    """sample(1)'s call tree of the main thread: samples in all, fetching events (ModalWait::pump ->
    -[NSApplication nextEventMatchingMask:...]), polling the pipe, blocked in read / write / waitpid"""
    i = text.find("com.apple.main-thread")
    if i < 0:
        return None
    block = text[i:]
    m = re.search(r"\n    \d+ Thread_", block)
    block = block[:m.start()] if m else block
    count = lambda pat: sum(int(x) for x in re.findall(r"(\d+) " + pat, block))
    line = text[text.rfind("\n", 0, i) + 1:text.find("\n", i)]
    m = re.search(r"(\d+) Thread_", line)
    total = int(m.group(1)) if m else 0
    return {"total": total, "pump": count(r"mvofx::ModalWait::pump"),
            "blocked": count(r"(?:read|__read_nocancel|write|__write_nocancel|wait4)  \(in libsystem_kernel"),
            "poll": count(r"poll  \(in libsystem_kernel")}


# ---------------------------------------------------------------- one case
def run_case(work, name, bundle, helper, env_extra, steps, poke=False, timeout=180, sample_at=(3.0, 6.0)):
    clip = os.path.join(BUILD, "verify", "clip_A.f32")
    log = os.path.join(work, f"modal_{name}.log")
    if os.path.exists(log):
        os.remove(log)
    args = [COCOAHOST] + (["--poke"] if poke else []) + [bundle, "--frames", clip, "--size", "640", "360", "8",
                                                         "--log", log] + steps
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
    samples, posted, profiles = [], [], []
    todo = list(sample_at)
    t0 = time.time()
    k = 0
    while p.poll() is None and time.time() - t0 < timeout:
        if todo and started[0] and time.time() - started[0] >= todo[0] and not done.is_set():
            # the host's main thread from another process (sample(1), 1 s)
            f = os.path.join(work, f"modal_{name}_{todo[0]:.0f}.sample.txt")
            subprocess.run(["sample", str(pid[0]), "1", "-mayDie", "-file", f], capture_output=True)
            prof = main_thread_profile(open(f).read()) if os.path.exists(f) else None
            profiles.append((todo[0], prof))
            todo.pop(0)
        if pid[0] and not done.is_set():
            samples.append((time.time(), unresponsive(pid[0])))
            if k % 2 == 0:
                posted.append((time.time(), post_input(pid[0], k // 2)))
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
    inputs = [float(ln.split()[2]) for ln in err if ln.startswith("input ")]
    block = [float(ln.split()[1]) for ln in err if ln.startswith("block ")]
    inside = lambda ts: [t for t in ts if w0 + 0.05 < t < w1 - 0.05]
    flagged = [t for t, u in samples if u and w0 < t < w1]
    sampled = [t for t, u in samples if u is not None and w0 + 1 < t < w1]
    pokes = [ln for ln in text.splitlines() if ln.startswith("poke ")]
    trace = [ln for ln in err if ln.startswith("meatvideo-ofx: helper wait")]
    nfs = re.search(r"fsguard: (\d+) file/process", text)
    r = {
        "rc": p.returncode, "press": press[0] if press else "", "wait_s": w1 - w0,
        "unresponsive": len(flagged), "samples": len(sampled),
        "first_unresponsive": (flagged[0] - w0) if flagged else None,
        "ticks_in_wait": len(inside(ticks)), "ticks_total": len(ticks),
        "inputs_before": len([t for t in inputs if t < w0]), "inputs_in_wait": len(inside(inputs)),
        "inputs_after": len([t for t in inputs if w1 <= t < w1 + 1.0]),
        "posted_in_wait": sum(n for t, n in posted if w0 < t < w1),
        "block_after_wait": (block[0] - w1) if block else None, "pokes": pokes, "trace": trace,
        "fsguard": int(nfs.group(1)) if nfs else None, "log": text, "stderr": err, "profiles": profiles,
    }
    return r


def show(name, r):
    pr = r["press"]
    status = re.search(r"status=(\d+)", pr)
    msg = re.search(r'message="([^"]*)"', pr)
    cpm = re.search(r"clip_params=(\w+)", pr)
    print(f"== {name}: rc {r['rc']}; press status {status.group(1) if status else '?'}, "
          f"clip_params {cpm.group(1) if cpm else '?'}, wait {r['wait_s']:.2f} s, "
          f"message \"{(msg.group(1) if msg else '')[:110]}\"")
    for at, pf in r["profiles"]:
        print(f"   main thread {at:.0f} s into the session (sample, another process): " + (
            f"{pf['pump']}/{pf['total']} samples fetching events (ModalWait::pump -> nextEventMatchingMask), "
            f"{pf['poll']} polling the pipe, {pf['blocked']} blocked in read / write / wait4" if pf else "not sampled"))
    print(f"   CGSEventIsAppUnresponsive: {r['unresponsive']}/{r['samples']} samples flagged in the wait"
          + (f" (first {r['first_unresponsive']:.2f} s in)" if r["first_unresponsive"] is not None else ""))
    print(f"   input: {r['inputs_before']} dispatched before the wait (posting works), {r['posted_in_wait']} posted "
          f"during it, {r['inputs_in_wait']} dispatched during it, {r['inputs_after']} in the 1 s after it")
    print(f"   default-mode timer: {r['ticks_in_wait']} ticks during the wait ({r['ticks_total']} in all); "
          f"main-queue block ran {r['block_after_wait']:+.2f} s after the wait" if r["block_after_wait"] is not None
          else f"   default-mode timer: {r['ticks_in_wait']} ticks during the wait; main-queue block: not run")
    for p in r["pokes"]:
        print(f"   {p[:200]}")
    print(f"   plug-in trace: {r['trace']}; fsguard records {r['fsguard']}")


def write_stubs(work, reply):
    d = os.path.join(work, "modal_stubs")
    os.makedirs(d, exist_ok=True)
    head = "#!/bin/sh\nif [ \"$1\" = \"--ofx-default\" ]; then printf 'DEFAULTS 0 none none\\n'; exit 0; fi\ncat > /dev/null\n"
    stubs = {
        "apply": head + f"sleep ${{MV_STUB_SLEEP:-10}}\nn=$(wc -c < '{reply}')\nprintf 'APPLY %d\\n' $n\ncat '{reply}'\n",
        "die": head + "sleep 3\nkill -9 $$\n",
        "eof": head + "sleep 9\nexit 0\n",
    }
    for k, body in stubs.items():
        path = os.path.join(d, k)
        open(path, "w").write(body)
        os.chmod(path, 0o755)
    return d


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--work", default=os.path.join(BUILD, "verify"))
    ap.add_argument("--control")
    ap.add_argument("--only", default="")
    a = ap.parse_args()
    os.makedirs(os.path.join(a.work, "home"), exist_ok=True)
    reply = sorted(glob.glob(os.path.join(a.work, "cp_S-params_*.json")))[0]
    stubs = write_stubs(a.work, reply)
    fails = []
    cases = [
        # name, helper, env, steps, poke, expectation
        ("apply-10s", os.path.join(stubs, "apply"), {"MV_STUB_SLEEP": "10"}, ["--press", "prepare@3"], True, "changed"),
        ("helper-dies", os.path.join(stubs, "die"), {}, ["--press", "prepare@3"], False, "signal 9"),
        ("no-reply-eof", os.path.join(stubs, "eof"), {}, ["--press", "prepare@3"], False, "unexpected reply (0 bytes)"),
        ("window-closed", DRV, {"NV_OFX_SESSION_SCRIPT": "sleep 9000;close"}, ["--press", "prepare@3"], False, "unchanged"),
    ]
    for name, helper, env, steps, poke, want in cases:
        if a.only and not re.search(a.only, name):
            continue
        at = (1.5,) if name == "helper-dies" else (3.0, 6.0)
        r = run_case(a.work, name, BUNDLE, helper, env, steps, poke, sample_at=at)
        show(name, r)
        pumping = r["profiles"] and all(pf and pf["pump"] > 0.8 * pf["total"] and pf["blocked"] == 0
                                         for _, pf in r["profiles"])
        ok = (r["rc"] == 0 and want in r["press"] and r["unresponsive"] == 0 and pumping and
              r["ticks_in_wait"] == 0 and r["inputs_in_wait"] == 0 and r["inputs_before"] > 0 and
              r["inputs_after"] == 0 and r["block_after_wait"] is not None and r["block_after_wait"] >= -0.01 and
              r["trace"][-1:] == ["meatvideo-ofx: helper wait: host event loop (MeatVideoOFXModal)"])
        if poke:
            ok = ok and any("from background thread" in p and "already open" in p and "status=0" in p for p in r["pokes"])
        print(f"   -> {'ok' if ok else 'FAIL'}")
        if not ok:
            fails.append(name)
        if a.control and name == "apply-10s":
            rc = run_case(a.work, name + "-control", a.control, helper, env, steps, poke, sample_at=at)
            show(name + " (control: MV_MODAL_WAIT=OFF, the blocking wait)", rc)
            blocked = rc["profiles"] and all(pf and pf["blocked"] > 0.8 * pf["total"] and pf["pump"] == 0
                                             for _, pf in rc["profiles"])
            ok = rc["rc"] == 0 and want in rc["press"] and blocked and \
                rc["trace"][-1:] == ["meatvideo-ofx: helper wait: blocking"] and rc["inputs_after"] > 0
            print(f"   -> control {'as expected (the main thread blocked, input left queued)' if ok else 'NOT as expected'}")
            if not ok:
                fails.append(name + "-control")
    # a headless host (ofx/build/minihost: no NSApp) keeps the blocking wait
    if not a.only or re.search(a.only, "minihost"):
        env = dict(os.environ, MEATVIDEO_OFX_HELPER=os.path.join(stubs, "apply"), MV_STUB_SLEEP="1",
                   MEATVIDEO_OFX_TRACE="1", HOME=os.path.join(a.work, "home"),
                   NV_UI_QT_SETTINGS=os.path.join(a.work, "home", "defaults_none.ini"))
        p = subprocess.run([os.path.join(BUILD, "minihost"), BUNDLE, "--frames", os.path.join(BUILD, "verify", "clip_A.f32"),
                            "--size", "640", "360", "8", "--press", "prepare@3"], env=env, capture_output=True, text=True,
                           timeout=120)
        trace = [ln for ln in p.stderr.splitlines() if ln.startswith("meatvideo-ofx: helper wait")]
        press = [ln for ln in p.stdout.splitlines() if ln.startswith("press ")]
        ok = p.returncode == 0 and trace == ["meatvideo-ofx: helper wait: blocking"] * 2 and press and \
            " status=0 " in press[0] and "clip_params=changed" in press[0]
        print(f"== minihost (headless, no NSApp): plug-in trace {trace}; {press[0][:90] if press else '?'} -> "
              f"{'ok' if ok else 'FAIL'}")
        if not ok:
            fails.append("minihost headless")
    print("ALL OK" if not fails else "FAILURES: " + ", ".join(fails))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
