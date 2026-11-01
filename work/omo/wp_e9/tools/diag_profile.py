"""Diagnosis 1: the noise the profile assumes vs the noise that was added (observation only; nothing in the
processing core is changed or called with other settings than run_ours.py used).

    python3 tools/diag_profile.py CLIP [CLIP ...]        (CLIP = <src>/<layout>_<level>)

Scale (nvfilter.build_profile_v5 / curve_sigma): the profile's curve[plane][bin] is the sigma of the finest a-trous
band (nvfilter.atrous, B3 spline) of that plane in 8-bit code values, at the brightness bin centres
curve_centers (Y code value); the band j sigma is curve * band_ratio[j]; Cb / Cr curves include CHROMA_GAIN.
The same quantities are measured on the exactly known added noise n = noisy - clean (frame = the profile frame,
float): band sigmas = RMS of atrous(n) bands over the frame; the finest-band sigma per bin = RMS of band 1 over the
pixels whose noisy Y (Y grid; chroma: 2x2 mean of Y) falls in the bin (bins with >= W*H/1024 pixels).
Also: the profile estimators of nvfilter (noise_curve_spatial, area_band_sigma; the Python reference, bit-equal to
the native build per Agents.md 7.5 E2) run on the clean frame (what the profile would see in the source alone) and
on the noisy frame; and the static fraction of noise_curve_temporal (the auto/spatial switch) for the clip's frame
pair, for frames 0 and 2 of the progressive clip, and per field.
Writes eval/diag_profile_<src>_<clip>.json and prints a summary.
"""
import json
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import e9_common as C  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.path.insert(0, os.path.normpath(os.path.join(ROOT, "..", "..", "..", "reference", "python")))
import nvfilter as nv  # noqa: E402

NAMES = ("Y", "Cb", "Cr")


def r(x, n=3):
    if isinstance(x, (list, tuple, np.ndarray)):
        return [r(v, n) for v in x]
    return None if x is None else round(float(x), n)


def frames(path, n=3):
    return [tuple(np.asarray(p, np.float32) for p in pl) for _, pl in C.read_y4m(path, 0, n)]


def actual(noisy, clean, levels=5):
    n = [a - b for a, b in zip(noisy, clean)]
    Y = noisy[0]
    H, W = Y.shape
    out = {"band_sigma": {}, "curve": {}, "total_detail_sigma": {}, "plain_sigma": {}}
    for c, name in enumerate(NAMES):
        L = levels if n[c].shape == Y.shape else levels - 1
        b = nv.atrous(n[c], L)
        out["band_sigma"][name] = r([np.sqrt((b[j] ** 2).mean()) for j in range(L)])
        out["total_detail_sigma"][name] = r(np.sqrt(((n[c] - b[L]) ** 2).mean()))
        out["plain_sigma"][name] = r(n[c].std())
        lum = Y if n[c].shape == Y.shape else 0.25 * (Y[0::2, 0::2] + Y[1::2, 0::2] + Y[0::2, 1::2] + Y[1::2, 1::2])
        bins = np.clip((lum * nv.CURVE_BINS / 256.0).astype(int), 0, nv.CURVE_BINS - 1)
        cv = []
        for i in range(nv.CURVE_BINS):
            sel = bins == i
            ok = sel.sum() * (H * W / n[c].size) >= H * W / 1024.0
            cv.append(r(np.sqrt((b[0][sel] ** 2).mean())) if ok else None)
        out["curve"][name] = cv
    return out


def static_fraction(p0, p1):
    Y0, Y1 = np.asarray(p0[0], np.float32), np.asarray(p1[0], np.float32)
    from scipy.ndimage import uniform_filter
    still = (np.abs(uniform_filter(Y1, 15) - uniform_filter(Y0, 15)) < 0.5) & ~nv._dead(Y0)
    return round(float(still.mean()), 4)


def main():
    for clip in sys.argv[1:]:
        src, lv = clip.split("/")
        layout, level = lv.split("_")
        run = os.path.join(ROOT, "runs", "ours_frame", src, lv)
        eff = json.load(open(os.path.join(run, "ui_profile.json")))
        base = json.load(open(os.path.join(run, "nvprof.json")))["base"]
        pre = base.get("ui_pre_aft") or {}
        noisy = frames(os.path.join(ROOT, "clips", src, f"noisy_{layout}_{level}.y4m"))
        clean = frames(os.path.join(ROOT, "clips", src, f"clean_{layout}.y4m"))
        f = int(eff.get("ui_frame", 0))
        res = {"clip": clip, "profile_frame": f, "mode": eff["mode"], "area": eff["area"],
               "flat_status": eff.get("flat_status"), "static_fraction_profile": r(base.get("static_fraction"), 4),
               "curve_centers": r(eff["curve_centers"], 1), "chroma_gain": nv.CHROMA_GAIN,
               "wn_band_std": r(nv._wn_band_std(5), 4)}
        res["profile_used"] = {"curve": {k: r(v) for k, v in eff["curve"].items()}, "band_ratio":
                               {k: r(v) for k, v in eff["band_ratio"].items()}, "tot_ratio": r(list(eff["tot_ratio"].values())),
                               "sigma": {k: r(v) for k, v in eff["sigma"].items()}, "aft_applied": base.get("aft_applied")}
        if pre:
            res["profile_before_aft"] = {"curve": {k: r(v) for k, v in pre.get("curve", {}).items()},
                                         "sigma": {k: r(v) for k, v in pre.get("sigma", {}).items()}} \
                if isinstance(pre, dict) and "curve" in pre else {"keys": list(pre)[:20] if isinstance(pre, dict) else str(type(pre))}
        res["actual_added"] = actual(noisy[f], clean[f])
        # the profile's own estimators on clean / noisy (curve without CHROMA_GAIN; ratio2 = the profile's band 2 ratio)
        r2 = {n: eff["band_ratio"][n][1] for n in NAMES}
        for nm, fr in (("clean", clean), ("noisy", noisy)):
            cs = nv.noise_curve_spatial(fr[f], r2, False, matrix="bt709")
            res[f"estimator_on_{nm}"] = {
                "curve_single_frame_no_gain": {k: r(v) for k, v in cs.items()},
                "area_band_sigma_Y": r(nv.area_band_sigma(fr[f][0], eff["area"], 5)[0])}
        sf = {"profile_pair_0_1": static_fraction(noisy[0], noisy[1]),
              "clean_pair_0_1": static_fraction(clean[0], clean[1])}
        if layout == "p":
            sf["noisy_frames_0_2"] = static_fraction(noisy[0], noisy[2])
        else:
            for par, fn in ((0, "top"), (1, "bottom")):
                a = tuple(p[par::2] for p in noisy[0])
                b = tuple(p[par::2] for p in noisy[1])
                sf[f"noisy_field_{fn}_0_1"] = static_fraction(a, b)
            pc = os.path.join(ROOT, "clips", src, f"noisy_p_{level}.y4m")
            if os.path.exists(pc):
                pp = frames(pc, 3)
                sf["progressive_noisy_0_1"] = static_fraction(pp[0], pp[1])
                sf["progressive_noisy_0_2"] = static_fraction(pp[0], pp[2])
        res["static_fraction"] = sf
        # summary: the curve the processing uses / the actual finest-band sigma, per bin
        ratio = {}
        for n in NAMES:
            ratio[n] = [None if a is None else r(u / a, 2) for u, a in zip(eff["curve"][n], res["actual_added"]["curve"][n])]
        res["used_over_actual_finest_band"] = ratio
        C.write_json(os.path.join(ROOT, "eval", f"diag_profile_{src}_{lv}.json"), res)
        print(f"== {clip}: mode {res['mode']} area {res['area']} static {sf}")
        for n in NAMES:
            print(f"  {n}: used curve {res['profile_used']['curve'][n]}")
            print(f"     actual     {res['actual_added']['curve'][n]}")
            print(f"     used/actual {ratio[n]}")
            print(f"     band ratio used {res['profile_used']['band_ratio'][n]}  actual band sigma {res['actual_added']['band_sigma'][n]}")
            print(f"     single-frame estimator (no gain) clean {res['estimator_on_clean']['curve_single_frame_no_gain'][n]}")
            print(f"                                      noisy {res['estimator_on_noisy']['curve_single_frame_no_gain'][n]}")
        if "profile_before_aft" in res:
            print("  before AFT:", res["profile_before_aft"])


if __name__ == "__main__":
    main()
