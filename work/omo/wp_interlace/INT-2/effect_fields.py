# INT-2 effect check (.claude/tasks/interlace.md INT-2 4): a synthetic progressive clip (static texture + a moving
# textured circle, 60p, noise) interlaced by ffmpeg tinterlace (TFF: interleave_top, BFF: interleave_bottom), processed
#   (a) frame by frame as now (nvt::process_frame_v5, nv_v5_test kind 3; profile of an interlaced frame), and
#   (b) field by field (nvfp::process_frame_fields, kind 31, modes plain / shift / same; profile of a field),
# all with the v5 defaults (Temporal Radius 2: frames in (a), fields in (b); also radius 4 fields in (b)).
# The residuals against the clean interlaced clip (the same tinterlace of the clean frames) are reported per plane in
# the static part (pixels the circle never reaches) and the moving part (the circle at either field's instant,
# dilated by 2 px), with a comb index (RMS of the vertical high-pass e[y] - (e[y-1] + e[y+1]) / 2 of the luma
# error inside the moving part).  The C++ outputs are bit-exact with native/fields_ref.py (verify_fieldproc.py); a
# few frames are cross-checked here too (--check).
#
#   python3 work/omo/wp_interlace/INT-2/effect_fields.py [--frames 30] [--jobs 2] [--check 2] [--out DIR]
import argparse
import json
import os
import subprocess
import sys
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "native"))
sys.dont_write_bytecode = True
import v5_cases as vc  # noqa: E402
import fields_ref as fr  # noqa: E402
from v5_cases import nv  # noqa: E402

vc.KIND.update({"tshift": 30, "fields": 31})
F32 = np.float32
W, H = 640, 360


def clean_progressive(n):
    """n progressive frames (Y, U, V full resolution, float): static texture + a moving textured circle."""
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float64)
    bgY = 70 + 50 * np.sin(xx / 37.0) * np.cos(yy / 23.0) + 0.12 * xx
    bgY[60:140, 380:600] += 30 * (np.floor(yy[60:140, 380:600] / 3) % 2)      # fine horizontal stripes (period 6)
    bgY[220:330, 400:560] += 25 * np.sin(xx[220:330, 400:560] / 2.5)          # fine vertical detail
    bgU = 128 + 18 * np.sin(xx / 61.0)
    bgV = 128 + 18 * np.cos(yy / 47.0)
    out = []
    for t in range(n):
        cx, cy = 90 + 4.0 * t, 180 + 40 * np.sin(2 * np.pi * t / 40.0)
        d = np.hypot(xx - cx, yy - cy)
        m = np.clip(52.5 - d, 0, 1)                         # anti-aliased disc of radius 52
        tex = 200 + 25 * np.sin((xx - cx) / 5.0) * np.sin((yy - cy) / 4.0)
        Y = bgY * (1 - m) + tex * m
        U = bgU * (1 - m) + 95 * m
        V = bgV * (1 - m) + 175 * m
        out.append((Y, U, V, (cx, cy)))
    return out


def to420(Y, U, V):
    def d2(a):
        return a.reshape(H // 2, 2, W // 2, 2).mean((1, 3))
    return [np.clip(np.rint(p), 0, 255).astype(np.uint8) for p in (Y, d2(U), d2(V))]


def interlace_ffmpeg(frames420, first):
    """ffmpeg tinterlace of yuv420p frames (60p -> 30i), returns the interlaced frames (Y, U, V uint8)."""
    mode = "interleave_top" if first == 0 else "interleave_bottom"
    raw = b"".join(p.tobytes() for f in frames420 for p in f)
    r = subprocess.run(["ffmpeg", "-v", "error", "-nostdin", "-f", "rawvideo", "-pix_fmt", "yuv420p", "-s", f"{W}x{H}",
                        "-r", "60", "-i", "-", "-vf", f"tinterlace=mode={mode}", "-f", "rawvideo", "-pix_fmt", "yuv420p",
                        "-"], input=raw, capture_output=True, check=True)
    fs = W * H * 3 // 2
    a = np.frombuffer(r.stdout, np.uint8).reshape(-1, fs)
    return [(a[i, :W * H].reshape(H, W), a[i, W * H:W * H * 5 // 4].reshape(H // 2, W // 2),
             a[i, W * H * 5 // 4:].reshape(H // 2, W // 2)) for i in range(a.shape[0])]


def masks(prog, n, first):
    """static (never reached by the circle, dilated 4 px) and moving (the circle at the frame's two field instants,
    dilated 2 px) luma masks of interlaced frame n; chroma masks by 2x2 any / all"""
    yy, xx = np.mgrid[0:H, 0:W]
    ever = np.zeros((H, W), bool)
    for (_, _, _, (cx, cy)) in prog:
        ever |= np.hypot(xx - cx, yy - cy) <= 52 + 4
    static = ~ever
    mov = np.zeros((H, W), bool)
    for t in (2 * n, 2 * n + 1):
        cx, cy = prog[t][3]
        mov |= np.hypot(xx - cx, yy - cy) <= 52 + 2
    c = lambda m, f: f(m.reshape(H // 2, 2, W // 2, 2), axis=(1, 3))
    return (static, mov), (c(static, np.all), c(mov, np.any))


def run_cpp(cases, tmp, jobs, tag):
    inp, outp = os.path.join(tmp, f"{tag}_in.bin"), os.path.join(tmp, f"{tag}_out.bin")
    vc.write_cases(inp, cases)
    vc.run_exe(inp, outp, jobs)
    r = vc.read_results(outp, len(cases))
    os.remove(inp)
    os.remove(outp)
    return r


def rms(a, m):
    return float(np.sqrt(np.mean(a[m] ** 2))) if m.any() else float("nan")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--frames", type=int, default=30, help="interlaced frames (2x progressive)")
    ap.add_argument("--sigma", type=float, default=6.0)
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--check", type=int, default=2, help="frames cross-checked against fields_ref per run")
    ap.add_argument("--out", default=os.path.join(HERE, "logs"))
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    tmp = os.path.join(HERE, "tmp")
    os.makedirs(tmp, exist_ok=True)
    N = a.frames
    rng = np.random.default_rng(1101)
    prog = clean_progressive(2 * N)
    clean420 = [to420(*f[:3]) for f in prog]
    noisy420 = []
    for f in prog:
        Y = f[0] + rng.normal(0, a.sigma, (H, W))
        U = f[1] + rng.normal(0, a.sigma * 0.5, (H, W))
        V = f[2] + rng.normal(0, a.sigma * 0.5, (H, W))
        noisy420.append(to420(Y, U, V))
    results = {}
    for first, name in ((0, "TFF"), (1, "BFF")):
        clean = [tuple(p.astype(F32) for p in f) for f in interlace_ffmpeg(clean420, first)]
        noisy = [tuple(np.ascontiguousarray(p, F32) for p in f) for f in interlace_ffmpeg(noisy420, first)]
        # tinterlace itself: frame n = the field `first` of progressive 2n and the other of 2n + 1
        for n in (0, N // 2):
            for k in range(3):
                exp = clean420[2 * n][k].copy()
                exp[1 - first::2] = clean420[2 * n + 1][k][1 - first::2]
                assert np.array_equal(exp.astype(F32), clean[n][k]), (name, n, k)
        prof_frame = nv.build_profile_v5(noisy[N // 2], False, None, pair=noisy[N // 2 + 1], mode="auto", matrix="bt601")
        fields, _ = fr.field_sequence(noisy, first)
        s0 = 2 * (N // 2)
        prof_field = nv.build_profile_v5(fields[s0], False, None, pair=fields[s0 + 2], mode="auto", matrix="bt601")
        runs = [("a frame R2", "frame", None, 2, prof_frame), ("b plain R2", "fields", fr.PLAIN, 2, prof_field),
                ("b shift R2", "fields", fr.SHIFT, 2, prof_field), ("b same R2", "fields", fr.SAME, 2, prof_field),
                ("b shift R4", "fields", fr.SHIFT, 4, prof_field), ("b same R4", "fields", fr.SAME, 4, prof_field),
                ("b shift R2 frame-profile", "fields", fr.SHIFT, 2, prof_frame)]
        evalf = list(range(2, N - 2))
        res = {"input": None}
        outs_all = {}
        for label, kind, mode, R, prof in runs:
            P = nv.v5_params(radius=R)
            cases, wins = [], []
            for n in evalf:
                if kind == "frame":
                    lo, hi = max(0, n - R), min(N, n + R + 1)
                    win = noisy[lo:hi]
                    cases.append(vc.encode_case("frame", prof, P, [n - lo, len(win), 0], (), [p for f in win for p in f]))
                else:
                    m = R // 2 + 1
                    lo, hi = max(0, n - m), min(N, n + m + 1)
                    win = noisy[lo:hi]
                    cases.append(vc.encode_case("fields", prof, P, [n - lo, len(win), first, mode], (),
                                                [p for f in win for p in f]))
                wins.append((win, n - lo))
            t0 = time.time()
            outs = run_cpp(cases, tmp, a.jobs, "eff")
            dt = time.time() - t0
            outs_all[label] = outs
            # cross-check a few frames with the Python reference
            for i in list(range(len(evalf)))[::max(1, len(evalf) // max(a.check, 1))][:a.check]:
                win, ci = wins[i]
                e = (nv.process_frame_v5(win, ci, prof, P, {}) if kind == "frame"
                     else fr.process_frame_fields(win, ci, first, mode, prof, P))
                bad = sum(vc.diff_count(x, y) for x, y in zip(e, outs[i][:3]))
                assert bad == 0, (label, evalf[i], bad)
            res[label] = {"time_s": round(dt, 2)}
        # metrics
        def stats(get):
            acc = {k: [] for k in ("Y_static", "Y_moving", "U_static", "U_moving", "V_static", "V_moving", "Y_all",
                                   "comb_moving", "comb_static")}
            for i, n in enumerate(evalf):
                (ls, lm), (cs, cm) = masks(prog, n, first)
                o = get(i, n)
                for k, nm in enumerate("YUV"):
                    e = np.clip(np.rint(o[k]), 0, 255) - clean[n][k]
                    ms, mm = (ls, lm) if k == 0 else (cs, cm)
                    acc[f"{nm}_static"].append(np.mean(e[ms] ** 2))
                    acc[f"{nm}_moving"].append(np.mean(e[mm] ** 2))
                    if k == 0:
                        acc["Y_all"].append(np.mean(e ** 2))
                        h = e[1:-1] - 0.5 * (e[:-2] + e[2:])
                        acc["comb_moving"].append(np.mean(h[lm[1:-1]] ** 2))
                        acc["comb_static"].append(np.mean(h[ls[1:-1]] ** 2))
            r = {k: float(np.sqrt(np.mean(v))) for k, v in acc.items()}
            r.update({f"psnr_{k}": float(10 * np.log10(255 ** 2 / np.mean(v))) for k, v in acc.items()
                      if not k.startswith("comb")})
            return r
        res["input"] = stats(lambda i, n: noisy[n])
        for label in outs_all:
            res[label].update(stats(lambda i, n, L=label: outs_all[L][i][:3]))
        results[name] = res
        print(f"== {name} ({N} interlaced frames, evaluated {evalf[0]}..{evalf[-1]}; RMS error vs the clean "
              f"interlaced frames, comb = RMS of the vertical high-pass of the luma error)")
        print(f"{'run':26s} {'Y stat':>7s} {'Y mov':>7s} {'U stat':>7s} {'U mov':>7s} {'V stat':>7s} {'V mov':>7s} "
              f"{'comb mv':>8s} {'comb st':>8s} {'PSNR Y':>7s} {'time':>6s}")
        for label, r in res.items():
            print(f"{label:26s} {r['Y_static']:7.3f} {r['Y_moving']:7.3f} {r['U_static']:7.3f} {r['U_moving']:7.3f} "
                  f"{r['V_static']:7.3f} {r['V_moving']:7.3f} {r['comb_moving']:8.3f} {r['comb_static']:8.3f} "
                  f"{r['psnr_Y_all']:7.2f} {r.get('time_s', 0):6.1f}")
    with open(os.path.join(a.out, "effect_fields.json"), "w") as f:
        json.dump(results, f, indent=1)
    return 0


if __name__ == "__main__":
    sys.exit(main())
