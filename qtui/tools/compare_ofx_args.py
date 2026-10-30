"""Compare the OFX argument handling of the C++ UI (qtui_ofx_drv = nvqt::ofx_main) with the Python UI's (py_ofx_drv.py ->
nv_ui_qt.main._ofx_main): the numbers of `--ofx W H [FPS] [MAX_FRAMES] [--depth D]` as Python's int() / float() read
them (N3c verification L-2 and difference 3: Unicode digits and spaces, underscores, 0x10, hex floats, 1e400 /
1e-400 / 1e-310, inf / nan, the 4300-digit limit) and the frame sizes Python cannot read or process (I-6: the
MemoryError / OverflowError tracebacks of the first stdin read, frames smaller than the motion block).

    NV_NATIVE=1 python3 qtui/tools/compare_ofx_args.py OUTDIR [--exe qtui/build_main/qtui_ofx_drv] [--only a,b]

Per case both processes get the same stdin and the script "info;close" (INFO = the window's video / depth labels,
which print FPS as Python's repr does); compared: the exit code, stdout byte for byte (the frame written on close),
the MSG / INFO lines, the usage / "no frames" / "OFX emit error" lines, the last line of stderr (the exception
line of a traceback) and the traceback's nv_ui_qt frames (file, line, function, source); one MemoryError case is
also run with the real `python3 -m nv_ui_qt.main` for main.py's <module> frame.  Prints one row per case and RESULT: ALL SAME / DIFFERENCES.
"""
import argparse
import os
import re
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from compare_ofx import REPRO, ROOT, inputs  # noqa: E402

BIG = "1" * 4300
# group, name, args (str, or bytes for an argument that is not UTF-8)
CASES = [
    # the 16 edge values (L-2 of the N3c verification and difference 3 of the N3c report first)
    ("edge", "fps_1e400", ["640", "360", "1e400"]),
    ("edge", "fps_1e-400", ["640", "360", "1e-400"]),
    ("edge", "fps_1e-310", ["640", "360", "1e-310"]),
    ("edge", "W_0x10", ["0x10", "360"]),
    ("edge", "WH_arabic_indic", ["٦٤٠", "٣٦٠"]),
    ("edge", "fps_1_000", ["640", "360", "1_000"]),
    ("edge", "fps_hexfloat", ["640", "360", "0x1p4"]),
    ("edge", "fps_neg_underflow", ["640", "360", "-1e-400"]),
    ("edge", "fps_round_to_inf", ["640", "360", "1.7976931348623159e308"]),
    ("edge", "fps_min_subnormal", ["640", "360", "4.9406564584124654e-324"]),
    ("edge", "W_ideographic_space", ["　640　", "360"]),
    ("edge", "W_fullwidth", ["６４０", "360"]),
    ("edge", "fps_nan_payload", ["640", "360", "nan(1)"]),
    ("edge", "fps_Infinity", ["640", "360", "Infinity"]),
    ("edge", "W_4301_digits", ["1" * 4301, "360"]),
    ("edge", "depth_fullwidth", ["640", "360", "30", "0", "--depth", "１６"]),
    # 20 more
    ("extra", "WH_arabic_10", ["١٠", "١٠"]),          # int("١٠") == 10: a 10 x 10 frame
    ("extra", "W_math_bold", ["\U0001d7d4\U0001d7d2\U0001d7ce", "360"]),
    ("extra", "W_devanagari_H_thai", ["६४०", "๓๖๐"]),
    ("extra", "W_mixed_scripts", ["٦٤0", "360"]),
    ("extra", "W_minus_sign", ["−640", "360"]),
    ("extra", "W_fullwidth_plus", ["＋640", "360"]),
    ("extra", "W_double_underscore", ["6__40", "360"]),
    ("extra", "W_leading_underscore", ["_640", "360"]),
    ("extra", "W_trailing_underscore", ["640_", "360"]),
    ("extra", "W_underscore_plus", ["+6_40", "36_0"]),
    ("extra", "W_leading_zeros", ["0640", "00360"]),
    ("extra", "W_ascii_fs", ["\x1c640", "360"]),
    ("extra", "W_nbsp_tab", [" 640\t", "360"]),
    ("extra", "H_float_text", ["640", "3.6e2"]),
    ("extra", "fps_empty", ["640", "360", ""]),
    ("extra", "fps_underscores", ["640", "360", "2_9.9_7"]),
    ("extra", "fps_underscore_e", ["640", "360", "1_e5"]),
    ("extra", "fps_dot5_max_neg", ["640", "360", ".5", "-1"]),
    ("extra", "fps_bare_e", ["640", "360", "1e"]),
    ("extra", "W_invalid_utf8", [b"64\xff0", b"360"]),
    # frame sizes (I-6): Python's first stdin read raises, or the frame is smaller than the motion block
    ("size", "WH_2^31-1_d8", ["2147483647", "2147483647"]),
    ("size", "WH_2^31-1_d16", ["2147483647", "2147483647", "24", "0", "--depth", "16"]),
    ("size", "WH_3037000500_d32", ["3037000500", "3037000500", "24", "0", "--depth", "32"]),
    ("size", "W_1e20", ["99999999999999999999", "120"]),
    ("size", "W_2^63-1_H1", ["9223372036854775807", "1"]),
    ("size", "W_2^62_H1", ["4611686018427387904", "1"]),
    ("size", "W_4300_digits", [BIG, "1"]),
    ("size", "W_2^32_H120", ["4294967296", "120"]),
    ("size", "WH_1x1", ["1", "1"]),
    ("size", "WH_40x2", ["40", "2"]),
    ("size", "WH_2x40", ["2", "40"]),
    ("size", "WH_15x300", ["15", "300"]),
    ("size", "WH_8x8_1frame", ["8", "8", "30", "1"]),
    ("size", "WH_3x5_1frame_d32", ["3", "5", "30", "1", "--depth", "32"]),
]


def py_parse(args):
    """(W, H, depth, max_frames) as nv_ui_qt.main._ofx_main reads them, or None (usage)."""
    rest = [a.decode("utf-8", "surrogateescape") if isinstance(a, bytes) else a for a in args]
    try:
        depth = 8
        if "--depth" in rest:
            i = rest.index("--depth")
            depth = int(rest[i + 1])
            rest = rest[:i] + rest[i + 2:]
        W, H = int(rest[0]), int(rest[1])
        float(rest[2]) if len(rest) > 2 else 24.0
        mf = int(rest[3]) if len(rest) > 3 else 0
    except (IndexError, ValueError):
        return None
    if depth not in (8, 16, 32) or W <= 0 or H <= 0:
        return None
    return W, H, depth, mf


def stdin_for(args, ins):
    """2 frames of the parsed size (test.mp4 at 640 x 360, seeded random samples otherwise); the 640 x 360 8-bit
    frames where the arguments are invalid or the frame is too large to write."""
    p = py_parse(args)
    W0, H0, d640 = ins["t640"]
    if p is None:
        return d640[8][:2 * (W0 * H0 * 3 // 2)]
    W, H, d, _ = p
    n = (W * H + 2 * (W // 2) * (H // 2))
    if (W, H) == (W0, H0):
        return d640[d][:2 * n * d // 8]
    if n * d // 8 > 8 << 20:
        return d640[8][:2 * (W0 * H0 * 3 // 2)]
    rng = np.random.default_rng(W * 1000003 + H)
    a8 = rng.integers(16, 236, 2 * n).astype(np.uint8)
    if d == 8:
        return a8.tobytes()
    if d == 16:
        return (a8.astype("<u2") * 256 + rng.integers(0, 256, a8.size).astype("<u2")).astype("<u2").tobytes()
    return (a8.astype(np.float32) / np.float32(255)).astype("<f4").tobytes()


def tb_frames(lines):
    """The nv_ui_qt frames of a traceback: (path from nv_ui_qt/, line, function, source line); caret lines skipped."""
    res = []
    for i, l in enumerate(lines):
        m = re.match(r'  File "(.*)", line (\d+), in (.*)$', l)
        if m and "nv_ui_qt/" in m.group(1):
            src = lines[i + 1].strip() if i + 1 < len(lines) and lines[i + 1].startswith("    ") else ""
            res.append((m.group(1)[m.group(1).index("nv_ui_qt/"):], int(m.group(2)), m.group(3), src))
    return res


def run(side, exe, args, data, out, tag):
    ini = os.path.join(out, f"{tag}_{side}.ini")
    if os.path.exists(ini):
        os.remove(ini)
    env = dict(os.environ, QT_QPA_PLATFORM="offscreen", NV_NATIVE="1", NV_UI_QT_SETTINGS=ini, NV_OFX_SCRIPT="info;close")
    cmd = ([exe] if side == "cpp" else [sys.executable, os.path.join(HERE, "py_ofx_drv.py")]) + ["--ofx", *args]
    try:
        r = subprocess.run(cmd, input=data, capture_output=True, env=env, cwd=REPRO, timeout=120)
        rc = r.returncode
    except subprocess.TimeoutExpired as e:
        r, rc = e, "TIMEOUT"
    so, se = r.stdout or b"", (r.stderr or b"").decode(errors="replace")
    with open(os.path.join(out, f"{tag}_{side}.out"), "wb") as fh:
        fh.write(so)
    with open(os.path.join(out, f"{tag}_{side}.err"), "w") as fh:
        fh.write(se)
    lines = [l for l in se.splitlines() if l.strip()]
    keep = [l for l in lines if l.startswith(("MSG: ", "INFO: ", "OFX emit error", "usage:", "no frames"))]
    frames = tb_frames(se.splitlines())
    if side == "cpp":          # (py_ofx_drv.py calls main.main: no frame of main.py's <module>, checked by real_python)
        frames = [f for f in frames if f[2] != "<module>"]
    return {"rc": rc, "stdout": so, "lines": keep, "last": lines[-1] if lines else "",
            "traceback": any(l.startswith("Traceback (most recent call last):") for l in lines), "frames": frames}


def real_python(exe, out):
    """`python3 -m nv_ui_qt.main --ofx` itself (not the driver) for a MemoryError: its nv_ui_qt frames, <module>
    included, against the C++ traceback (the line numbers the C++ UI copies; N3 fixes verification L2)."""
    args = ["2147483647", "2147483647"]
    env = dict(os.environ, QT_QPA_PLATFORM="offscreen", NV_NATIVE="1", NV_UI_QT_SETTINGS=os.path.join(out, "real.ini"))
    py = subprocess.run([sys.executable, "-m", "nv_ui_qt.main", "--ofx", *args], capture_output=True, text=True,
                        env=env, cwd=REPRO, timeout=120, stdin=subprocess.DEVNULL)
    cp = subprocess.run([exe, "--ofx", *args], capture_output=True, text=True, env=dict(env, NV_OFX_SCRIPT="close"),
                        cwd=REPRO, timeout=120, stdin=subprocess.DEVNULL)
    a, b = tb_frames(py.stderr.splitlines()), tb_frames(cp.stderr.splitlines())
    last = [x.strip().splitlines()[-1] if x.strip() else "" for x in (py.stderr, cp.stderr)]
    same = a == b and len(a) == 5 and last[0] == last[1] and py.returncode == cp.returncode == 1
    print(f"real python3 -m nv_ui_qt.main --ofx {' '.join(args)}: {len(a)} / {len(b)} nv_ui_qt frames "
          f"{'SAME' if same else 'DIFF'}, last: {last[0]!r} / {last[1]!r}", flush=True)
    if not same:
        print(f"      py  {a}\n      cpp {b}")
    return same


def show(a):
    s = " ".join(repr(x) if not isinstance(x, str) or not x.isascii() or not x.isprintable() or not x else x
                 for x in a)
    return s if len(s) <= 46 else s[:43] + "..."


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--exe", default=os.path.join(ROOT, "qtui", "build_main", "qtui_ofx_drv"))
    ap.add_argument("--only", default="")
    A = ap.parse_args()
    os.makedirs(A.out, exist_ok=True)
    ins = inputs(A.out)
    only = set(filter(None, A.only.split(",")))
    ok_all, n = True, {}
    for group, name, args in CASES:
        if only and name not in only and group not in only:
            continue
        data = stdin_for(args, ins)
        res = {s: run(s, A.exe, args, data, A.out, name) for s in ("py", "cpp")}
        py, cp = res["py"], res["cpp"]
        same = py == cp
        ok_all &= same
        n[group] = n.get(group, [0, 0])
        n[group][0] += same
        n[group][1] += 1
        print(f"{group:5s} {name:22s} {show(args):46s} exit {py['rc']}/{cp['rc']} stdout {len(py['stdout'])}/"
              f"{len(cp['stdout'])} B {'=' if py['stdout'] == cp['stdout'] else '!='} "
              f"{'SAME' if same else 'DIFF'}  last: {py['last'][:90]}", flush=True)
        if not same:
            for k in ("rc", "lines", "last", "traceback", "frames"):
                if py[k] != cp[k]:
                    print(f"      {k}: py {py[k]!r:.300}\n      {' ' * len(k)}  cpp {cp[k]!r:.300}")
    if not only or "size" in only:
        ok_all &= real_python(A.exe, A.out)
    print("groups:", ", ".join(f"{g} {a}/{b}" for g, (a, b) in n.items()))
    print("RESULT:", "ALL SAME" if ok_all else "DIFFERENCES")
    return 0 if ok_all else 1


if __name__ == "__main__":
    sys.exit(main())
