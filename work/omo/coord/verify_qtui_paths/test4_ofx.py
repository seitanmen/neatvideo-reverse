"""Test 4: OFX mode through a pipe (ffmpeg as the host stand-in) -> python3 ofx_driver.py, which runs
nv_ui_qt.main.main(["--ofx", W, H, FPS, ...]) with the UI driven off-screen.

    python3 test4_ofx.py OUTDIR [case ...]
Cases: name = scenario:W:H:depth:frames.
Checks per case: frames received, Open/Export/Generic disabled, Auto Profile -> AFT, number of frames on
stdout, stdout bytes == pack_yuv420p(emitted planes), value range, emitted == preview.
"""
import json
import os
import subprocess
import sys

REPRO = "/Users/seitan/Desktop/work/reverse/repro"
HERE = os.path.dirname(os.path.abspath(__file__))
VIDEO = "/Users/seitan/Desktop/work/reverse/test.mp4"
DEFAULT = ["apply_close:192:108:8:12", "apply_change:192:108:8:12", "close_only:192:108:8:12",
           "cancel:192:108:8:12", "apply_close:192:108:16:12", "apply_close:192:108:32:12",
           "apply_close:640:360:8:12"]
EXPECT = {"apply_close": 1, "apply_change": 2, "close_only": 1, "cancel": 1}


def run_case(outdir, case):
    sys.path.insert(0, REPRO)
    import numpy as np
    from nv_ui_qt.ofx_bridge import frame_size, pack_yuv420p, DEPTHS
    scen, W, H, depth, n = case.split(":")
    W, H, depth, n = int(W), int(H), int(depth), int(n)
    d = os.path.join(os.path.abspath(outdir), case.replace(":", "_"))
    os.makedirs(d, exist_ok=True)
    for f in os.listdir(d):
        if f.startswith("emit_"):
            os.remove(os.path.join(d, f))
    conv = "" if depth == 8 else f" | python3 {HERE}/conv_depth.py {depth}"
    dep = "" if depth == 8 else f" --depth {depth}"
    cmd = (f"ffmpeg -nostdin -v error -i {VIDEO} -vf scale={W}:{H} -pix_fmt yuv420p -frames:v {n} -f rawvideo -"
           f"{conv} | QT_QPA_PLATFORM=offscreen python3 {HERE}/ofx_driver.py {scen} {d} {W} {H} 24{dep}"
           f" > {d}/stdout.bin 2> {d}/stderr.log")
    rc = subprocess.run(cmd, shell=True).returncode
    ev = []
    for line in open(f"{d}/stderr.log"):
        if line.startswith("DRV "):
            ev.append(json.loads(line[4:]))
    E = {e["ev"]: e for e in ev}
    res = []

    def check(name, ok, detail=""):
        res.append(bool(ok))
        print(f"  [{'PASS' if ok else 'FAIL'}] {name} {detail}", flush=True)

    print(f"case {case} (rc {rc})")
    s = E.get("started", {})
    check("4.1 frames received from stdin", s.get("frames") == n,
          f"{s.get('frames')} frames, Y {s.get('shape')} {s.get('dtype')}, src {s.get('src')}, n {s.get('info', {}).get('n')}")
    check("4.2 Open / Export / Load / Generic disabled", s and not s["open_enabled"] and not s["export_enabled"]
          and not s["generic_enabled"] and not s["load_profile_enabled"],
          f"open {s.get('open_enabled')} export {s.get('export_enabled')} load {s.get('load_profile_enabled')} "
          f"generic {s.get('generic_enabled')}; triggering them: no export {E.get('disabled_triggered', {}).get('export')}")
    if scen != "close_only":
        p, a = E.get("profile", {}), E.get("aft", {})
        check("4.4 Auto Profile -> AFT", p.get("ok") and a.get("ok") and a.get("aft_applied"),
              f"flat_status {p.get('flat_status')} area {p.get('area')} frame {p.get('frame')} {p.get('secs')}s; "
              f"AFT {a.get('aft_applied')} {a.get('secs')}s")
    emits = [e for e in ev if e["ev"] == "emit"]
    fs = frame_size(W, H, depth)
    size = os.path.getsize(f"{d}/stdout.bin")
    if scen in ("apply_close", "apply_change"):
        check("4.3 Apply writes the processed frame to stdout", E.get("after_apply", {}).get("emitted") == 1,
              f"frames written by Apply: {E.get('after_apply', {}).get('emitted')}; status "
              f"'{E.get('after_apply', {}).get('status')}'")
    if "apply_preview" in E:
        check("4.3 frame written by Apply == the refreshed preview (same frame, same settings)",
              E["apply_preview"]["equals_last_emit"])
    if scen == "apply_change":
        check("4.3b second Apply without change writes nothing", E.get("after_apply2", {}).get("emitted") == 0,
              f"{E.get('after_apply2', {}).get('emitted')}; status '{E.get('after_apply2', {}).get('status')}'")
    if "after_close" in E:
        exp_close = 0 if scen == "apply_close" else 1
        check("close writes a frame only when it differs from the last one", E["after_close"]["emitted"] == exp_close,
              f"frames written on close: {E['after_close']['emitted']} (expected {exp_close})")
    check("stdout size == emitted frames x frame_size", size == len(emits) * fs and len(emits) == EXPECT[scen],
          f"{size} bytes = {size / fs:g} x {fs}; emits {len(emits)} (expected {EXPECT[scen]}); exit rc "
          f"{E.get('exit', {}).get('rc')}")
    raw = open(f"{d}/stdout.bin", "rb").read()
    for k, e in enumerate(emits):
        z = np.load(f"{d}/emit_{k}.npz")
        planes = [z[f"arr_{i}"] for i in range(3)]
        exp = pack_yuv420p(planes, depth)
        got = raw[k * fs:(k + 1) * fs]
        a = np.frombuffer(got, DEPTHS[depth][0])
        Y = a[:W * H]
        C = a[W * H:]
        rng = f"Y {Y.min():.6g}..{Y.max():.6g} mean {Y.astype(np.float64).mean():.6g}, CbCr {C.min():.6g}..{C.max():.6g}"
        finite = bool(np.isfinite(a.astype(np.float64)).all())
        check(f"emit {k} (frame {e['frame']}, amt_Y {e['amt_Y']}, AFT {e['aft']}): stdout bytes == pack(planes)",
              got == exp and finite, rng + f", finite {finite}")
        if e["preview_cached"]:
            check(f"emit {k}: emitted frame == preview shown", e["equals_preview"])
    if scen == "apply_change" and len(emits) == 2:
        a0, a1 = raw[:fs], raw[fs:2 * fs]
        check("apply_change: frame after the slider change differs", a0 != a1)
    return all(res)


if __name__ == "__main__":
    outdir = sys.argv[1]
    cases = sys.argv[2:] or DEFAULT
    ok = [run_case(outdir, c) for c in cases]
    print(f"{sum(ok)}/{len(ok)} cases PASS")
    sys.exit(0 if all(ok) else 1)
