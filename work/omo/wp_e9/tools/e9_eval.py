"""Evaluates processed E9 clips (reproduction or genuine) against the clean truth (definitions in e9_common.py).

    python3 tools/e9_eval.py CLIP [--test NAME=PATH ...] [--start-number K] [--out eval/X.json]
    python3 tools/e9_eval.py CLIP --floor --test resolve_noeffect=PATH_%05d.tif [--out ...]
    CLIP = <src>/<layout>_<level> (input clips/<src>/noisy_<layout>_<level>.y4m, truth clips/<src>/clean_<layout>.y4m)

A test is an 8-bit 4:2:0 file (y4m / mp4 / mkv: read as yuv420p, no conversion) or a 16-bit RGB image sequence
(a pattern with %05d, or a .tif/.tiff/.png file / pattern: read as rgb48le, code / 257). The noisy input itself is
always evaluated as "input" (the baseline).
Domains:
  - YCbCr 4:2:0 (the reproduction's native domain): a 4:2:0 test as it is; an RGB test through
    e9_common.rgb_to_yuv420 (exact inverse of the reader's RGB, 4:2:0 mean; interlaced within the field), in float
    ("yuv") and rounded to 8 bits ("yuv8": as if delivered in 8-bit 4:2:0 like the reproduction's output).
  - RGB 0..255 (the genuine's domain): an RGB test as it is; a 4:2:0 test and the truth through
    e9_common.yuv_to_rgb (clipped).
Metrics (PSNR with peak 255, from the MSE pooled over the frames; "interior" = frames 2..N-3, where both
temporal windows of radius 2 are complete):
  - PSNR / RMS of Y, Cb, Cr and of R, G, B, RGB;
  - Y and RGB of the active rows ("active", letterboxed sources only: the rows between the first and the last row
    that varies in a clean frame; the black bars get RGB noise clipped at 0);
  - Y and RGB by motion class of the truth: D = max over j in t±1, t±2 of |box5(Y_j) - box5(Y_t)| (clean Y,
    box 5x5 of the signed values so that the source's own grain averages out; interlaced: per field, on the field
    pictures, woven back); static D < 2, small 2 <= D < 8, large D >= 8 (8-bit code values);
  - temporal residual: RMS of (e_t - e_t-1) with e = Y_test - Y_clean (all, static), and the frame-difference RMS
    of Y_test on the static class next to the truth's;
  - interlaced: Y PSNR per field (top = even rows, bottom = odd rows), per field and class, and combing:
    K = Y[r] - (Y[r-1] + Y[r+1])/2; comb_ratio = sum K_test^2 / sum K_clean^2 on the large-motion class
    (1 = the truth's combing; < 1 = fields blended), comb_err_rms = RMS(K_test - K_clean) over the frame;
  - alignment: for the middle frame, the Y PSNR against the truth for frame offsets -2..2 and vertical shifts
    -1..1 (a host that drops / repeats frames or swaps fields shows up here);
  - pairs: PSNR between every two tests (Y in "yuv", RGB).
--floor: the test is compared with the input TIFF that was given to the host (codes: differing samples,
max |diff|, PSNR), and with the noisy 4:2:0 input in the YCbCr domain; frame alignment as above.
"""
import argparse
import glob
import os
import sys
import time

import numpy as np
from scipy.ndimage import uniform_filter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import e9_common as C  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
CLASSES = (("static", 0.0, 2.0), ("small", 2.0, 8.0), ("large", 8.0, np.inf))


def psnr(mse):
    return float("inf") if mse <= 0 else float(10.0 * np.log10(255.0 ** 2 / mse))


def is_rgb(path):
    return "%" in path or path.lower().endswith((".tif", ".tiff", ".png")) or os.path.isdir(path)


def rgb_source(path):
    """A directory of .tif files -> its %05d pattern (the files must be name_00000.tif .. consecutive)."""
    if os.path.isdir(path):
        tif = sorted(glob.glob(os.path.join(path, "*.tif")) + glob.glob(os.path.join(path, "*.tiff")))
        if not tif:
            raise SystemExit(f"{path}: no TIFF files")
        stem, ext = os.path.splitext(tif[0])
        digits = len(stem) - len(stem.rstrip("0123456789"))
        return stem[:-digits] + f"%0{digits}d" + ext, int(stem[-digits:])
    return path, None


class Acc:
    def __init__(self):
        self.d = {}

    def add(self, key, sse, n):
        s = self.d.setdefault(key, [0.0, 0])
        s[0] += float(sse)
        s[1] += int(n)

    def result(self):
        out = {}
        for k, (sse, n) in sorted(self.d.items()):
            if n:
                mse = sse / n
                out[k] = {"psnr": round(psnr(mse), 3), "rms": round(float(np.sqrt(mse)), 4), "n": n}
        return out


def motion_classes(Yc, layout):
    """Per-frame class maps (uint8 0/1/2) from the clean Y frames (list of uint8 arrays)."""
    N = len(Yc)
    if layout == "p":
        b = [uniform_filter(y.astype(np.float64), 5, mode="nearest") for y in Yc]
    else:
        b = []
        for y in Yc:
            o = np.empty(y.shape, np.float64)
            for par in (0, 1):
                o[par::2] = uniform_filter(y[par::2].astype(np.float64), 5, mode="nearest")
            b.append(o)
    out = []
    for t in range(N):
        D = np.zeros(Yc[t].shape)
        for j in (t - 2, t - 1, t + 1, t + 2):
            if 0 <= j < N:
                D = np.maximum(D, np.abs(b[j] - b[t]))
        cls = np.zeros(D.shape, np.uint8)
        cls[D >= CLASSES[1][1]] = 1
        cls[D >= CLASSES[2][1]] = 2
        out.append(cls)
    return out


def comb(Y):
    return Y[1:-1] - 0.5 * (Y[:-2] + Y[2:])


def frames_of(path, layout, start_number=None):
    """Yields dict(yuv=(Y, Cb, Cr) float64, yuv8=..., rgb=(3, H, W) float64) per frame."""
    if is_rgb(path):
        pat, sn = rgb_source(path)
        for rgb in C.read_rgb48(pat, start_number=start_number if start_number is not None else sn):
            yuv = C.rgb_to_yuv420(rgb, layout)
            yield {"yuv": yuv, "yuv8": tuple(p.astype(np.float64) for p in C.quant8(yuv)), "rgb": rgb,
                   "rgb16": np.rint(rgb * 257.0)}
    else:
        for pl in C.read_yuv420(path):
            yuv = tuple(p.astype(np.float64) for p in pl)
            yield {"yuv": yuv, "rgb": C.yuv_to_rgb(pl, layout)}


def evaluate(name, path, clean, cls, layout, a):
    N = len(clean)
    t0 = time.time()
    acc = {"all": Acc(), "interior": Acc()}
    per_frame = []
    prev_err = prev_y = None
    domains = None
    mid = N // 2
    mid_y = None
    k = -1
    for k, fr in enumerate(frames_of(path, layout, a.start_number)):
        if k >= N:
            break
        if domains is None:
            domains = [d for d in ("yuv", "yuv8") if d in fr]
        cy = tuple(x.astype(np.float64) for x in clean[k])
        crgb = C.yuv_to_rgb(clean[k], layout)
        c = cls[k]
        rec = {"frame": k}
        scopes = [acc["all"]] + ([acc["interior"]] if 2 <= k <= N - 3 else [])
        for dom in domains:
            for ci, nm in enumerate(("Y", "Cb", "Cr")):
                d2 = (fr[dom][ci] - cy[ci]) ** 2
                for s in scopes:
                    s.add(f"{dom}.{nm}", d2.sum(), d2.size)
                if ci == 0:
                    rec[f"{dom}.Y"] = round(psnr(d2.mean()), 3)
                    for v, (cn, _, _) in enumerate(CLASSES):
                        m = c == v
                        for s in scopes:
                            s.add(f"{dom}.Y.{cn}", d2[m].sum(), m.sum())
                    if a.active is not None:
                        dd = d2[a.active]
                        for s in scopes:
                            s.add(f"{dom}.Y.active", dd.sum(), dd.size)
                    if layout != "p":
                        for par, fn in ((0, "top"), (1, "bottom")):
                            dd = d2[par::2]
                            cc = c[par::2]
                            for s in scopes:
                                s.add(f"{dom}.Y.field_{fn}", dd.sum(), dd.size)
                                for v, (cn, _, _) in enumerate(CLASSES):
                                    m = cc == v
                                    s.add(f"{dom}.Y.field_{fn}.{cn}", dd[m].sum(), m.sum())
        d2 = (fr["rgb"] - crgb) ** 2
        for ci, nm in enumerate("RGB"):
            for s in scopes:
                s.add(f"rgb.{nm}", d2[ci].sum(), d2[ci].size)
        tot = d2.sum(0)
        for s in scopes:
            s.add("rgb.RGB", tot.sum(), d2.size)
            for v, (cn, _, _) in enumerate(CLASSES):
                m = c == v
                s.add(f"rgb.RGB.{cn}", tot[m].sum(), 3 * m.sum())
            if a.active is not None:
                s.add("rgb.RGB.active", tot[a.active].sum(), 3 * tot[a.active].size)
        rec["rgb.RGB"] = round(psnr(d2.mean()), 3)
        # temporal residual (yuv domain, Y)
        Yt = fr["yuv"][0]
        err = Yt - cy[0]
        st = c == 0
        if prev_err is not None:
            de = (err - prev_err) ** 2
            dy = (Yt - prev_y) ** 2
            dc = (cy[0] - clean[k - 1][0]) ** 2
            for s in scopes:
                s.add("temporal.err_diff.all", de.sum(), de.size)
                s.add("temporal.err_diff.static", de[st].sum(), st.sum())
                s.add("temporal.frame_diff.static", dy[st].sum(), st.sum())
                s.add("temporal.frame_diff_truth.static", dc[st].sum(), st.sum())
        prev_err, prev_y = err, Yt
        if layout != "p":
            Kt, Kc = comb(Yt), comb(cy[0])
            lg = c[1:-1] == 2
            for s in scopes:
                s.add("comb.test_energy.large", (Kt[lg] ** 2).sum(), lg.sum())
                s.add("comb.truth_energy.large", (Kc[lg] ** 2).sum(), lg.sum())
                s.add("comb.err", ((Kt - Kc) ** 2).sum(), Kt.size)
        if abs(k - mid) <= 0:
            mid_y = Yt.copy()
        per_frame.append(rec)
    nread = k + 1
    res = {"path": os.path.relpath(path, ROOT) if os.path.exists(path.split("%")[0]) else path,
           "frames_read": nread, "frames_evaluated": min(nread, N), "domains": domains}
    for scope, A in acc.items():
        r = A.result()
        # temporal and comb entries as RMS / ratios
        out = {k2: v for k2, v in r.items() if not k2.startswith(("temporal.", "comb."))}
        tmp = {k2: v["rms"] for k2, v in r.items() if k2.startswith("temporal.")}
        if tmp:
            out["temporal_rms"] = tmp
        if layout != "p" and "comb.err" in r:
            e_t = A.d["comb.test_energy.large"][0]
            e_c = A.d["comb.truth_energy.large"][0]
            out["comb"] = {"ratio_large": round(e_t / e_c, 4) if e_c else None, "err_rms": r["comb.err"]["rms"]}
        res[scope] = out
    if mid_y is not None:
        al = {}
        for off in (-2, -1, 0, 1, 2):
            j = mid + off
            if not 0 <= j < N:
                continue
            for dy in (-1, 0, 1):
                ref = np.roll(clean[j][0].astype(np.float64), dy, axis=0)[2:-2]
                al[f"{off:+d},{dy:+d}"] = round(psnr(((mid_y[2:-2] - ref) ** 2).mean()), 3)
        best = max(al, key=al.get)
        res["alignment"] = {"frame": mid, "psnr_by_offset_dy": al, "best": best, "ok": best == "+0,+0"}
    res["per_frame"] = per_frame
    res["seconds"] = round(time.time() - t0, 1)
    return res


def floor(clip, path, a, layout, noisy_y4m):
    """The host's read / write floor: an effect-free export of the input TIFF compared with that TIFF."""
    srcname, lv = clip.split("/")
    tdir = os.path.join(ROOT, "clips", srcname, "tiff", lv)
    pat_in, sn_in = rgb_source(tdir)
    pat, sn = rgb_source(path) if is_rgb(path) else (path, None)
    A = Acc()
    ndiff = 0
    nsamp = 0
    maxd = 0
    mean_d = np.zeros(3)
    yuvacc = Acc()
    ins = C.read_rgb48(pat_in, start_number=sn_in)
    noisy = C.read_y4m(noisy_y4m)
    k = -1
    mids = {}
    mid = None
    for k, (rin, rout, (_, npl)) in enumerate(zip(ins, C.read_rgb48(pat, start_number=sn), noisy)):
        ci, co = np.rint(rin * 257), np.rint(rout * 257)
        d = co - ci
        ndiff += int((d != 0).sum())
        nsamp += d.size
        maxd = max(maxd, int(np.abs(d).max()))
        mean_d += d.reshape(3, -1).mean(1)
        A.add("rgb.RGB", ((rout - rin) ** 2).sum(), d.size)
        yuv = C.rgb_to_yuv420(rout, layout)
        for c, nm in enumerate(("Y", "Cb", "Cr")):
            dd = (yuv[c] - npl[c].astype(np.float64)) ** 2
            yuvacc.add(f"yuv.{nm}", dd.sum(), dd.size)
        mids[k] = (rout[1], rin[1])
    n = k + 1
    if n:
        mid = n // 2
    al = {}
    if mid is not None:
        for off in (-2, -1, 0, 1, 2):
            if mid + off in mids:
                al[f"{off:+d}"] = round(psnr(((mids[mid][0] - mids[mid + off][1]) ** 2).mean()), 3)
    return {"test": os.path.relpath(path, ROOT) if os.path.exists(path.split("%")[0]) else path,
            "input_tiff": os.path.relpath(tdir, ROOT), "frames": n, "codes": {
                "samples": nsamp, "differing": ndiff, "max_abs_diff_16bit": maxd,
                "mean_diff_16bit_RGB": [round(float(x / max(n, 1)), 4) for x in mean_d]},
            "rgb_vs_input": A.result(), "yuv_vs_noisy_420": yuvacc.result(),
            "alignment_G_psnr_by_offset": al}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("clip")
    ap.add_argument("--test", action="append", default=[])
    ap.add_argument("--start-number", type=int, default=None)
    ap.add_argument("--floor", action="store_true")
    ap.add_argument("--no-pairs", dest="pairs", action="store_false")
    ap.add_argument("--out", default=None)
    a = ap.parse_args()
    srcname, lv = a.clip.split("/")
    layout, level = lv.split("_")
    cdir = os.path.join(ROOT, "clips", srcname)
    noisy = os.path.join(cdir, f"noisy_{layout}_{level}.y4m")
    tests = [t.split("=", 1) for t in a.test]
    out = a.out or os.path.join(ROOT, "eval", f"{srcname}_{lv}" + ("_floor" if a.floor else "") + ".json")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    if a.floor:
        res = {"clip": a.clip, "floor": {nm: floor(a.clip, p, a, layout, noisy) for nm, p in tests}}
        C.write_json(out, res)
        for nm, r in res["floor"].items():
            print(nm, r["codes"], r["rgb_vs_input"], r["yuv_vs_noisy_420"], r["alignment_G_psnr_by_offset"])
        return
    clean = []
    for _, pl in C.read_y4m(os.path.join(cdir, f"clean_{layout}.y4m")):
        clean.append(tuple(np.array(p) for p in pl))
    cls = motion_classes([c[0] for c in clean], layout)
    # letterbox: "active" = the rows between the first and the last row that varies (std > 0.5) in any clean frame
    live = np.zeros(clean[0][0].shape[0], bool)
    for c in clean:
        live |= c[0].std(1) > 0.5
    rows = np.where(live)[0]
    a.active = slice(int(rows.min()), int(rows.max()) + 1) if len(rows) and (rows.min() > 0 or rows.max() < len(live) - 1) else None
    frac = {cn: round(float(np.mean([(c == v).mean() for c in cls])), 4) for v, (cn, _, _) in enumerate(CLASSES)}
    res = {"clip": a.clip, "layout": layout, "level": level, "frames": len(clean), "class_fraction": frac,
           "active_rows": None if a.active is None else [a.active.start, a.active.stop - 1],
           "tests": {}}
    for nm, p in [("input", noisy)] + tests:
        r = evaluate(nm, p, clean, cls, layout, a)
        res["tests"][nm] = r
        al = r.get("alignment", {})
        print(f"{a.clip} {nm}: Y {r['all'][r['domains'][0] + '.Y']['psnr']} dB, RGB {r['all']['rgb.RGB']['psnr']} dB"
              f" (interior Y {r['interior'][r['domains'][0] + '.Y']['psnr']}), alignment best {al.get('best')}"
              f" ({r['seconds']} s)", flush=True)
    if a.pairs and len(tests) > 1:
        pairs = {}
        for i, (x, px) in enumerate(tests):
            for y, py in tests[i + 1:]:
                A = Acc()
                for k, (fx, fy) in enumerate(zip(frames_of(px, layout, a.start_number),
                                                 frames_of(py, layout, a.start_number))):
                    if k >= len(clean):
                        break
                    dy = (fx["yuv"][0] - fy["yuv"][0]) ** 2
                    dr = (fx["rgb"] - fy["rgb"]) ** 2
                    A.add("Y", dy.sum(), dy.size)
                    A.add("RGB", dr.sum(), dr.size)
                pairs[f"{x}~{y}"] = A.result()
        res["pairs"] = pairs
        for k2, v in pairs.items():
            print(f"{a.clip} pair {k2}: Y {v['Y']['psnr']} dB, RGB {v['RGB']['psnr']} dB")
    C.write_json(out, res)
    print("wrote", os.path.relpath(out, ROOT))


if __name__ == "__main__":
    main()
