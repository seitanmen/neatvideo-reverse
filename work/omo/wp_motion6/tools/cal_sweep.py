"""E9 re-calibration sweep (2026-11-04): the remaining unswept knobs at each material's best
similarity config (pyramid for park_joy/old_town_cross, sim3c for sintel), v6 spatial v6set0,
spatial profile, nl=0.8 / cn=0.3 base. Each run = e9_nvm.py with NV_NATIVE=0 + NV6_E9_IMPL=py
(the env that reproduced the mtn_pyr15/mtn_sim3c baselines bit-exactly, see REPORT_v6temporal.md).
2 runs in parallel, jobs 4 each (M2 Max, 12 cores).

    python3 tools/cal_sweep.py [--only TAG,TAG..]

Runs go to wp_e9/runs/mtn_cal_<tag>/, evals to eval_v6temporal/cal_<tag>__<clip>.json.
"""
import json
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
E9 = os.path.normpath(os.path.join(HERE, "..", "..", "wp_e9"))
EVALDIR = os.path.join(HERE, "..", "wp_motion6", "eval_v6temporal")

PROFILE = {"park_joy": "runs/mtn_spatial_nl08/park_joy/p_y3/ui_profile_spatial.json",
           "sintel": "runs/mtn_spatial_nl08/sintel/p_y3/ui_profile_spatial.json",
           "old_town_cross": "runs/mtn_spatial_nl08/old_town_cross/p_y3/ui_profile_spatial.json"}
GEN_NUM = {"park_joy": 3, "sintel": 5, "old_town_cross": 1}

RUNS = [
    # sintel (variant sim3c): chroma is the big gap (Cb -1.25 / Cr -2.29 vs genuine)
    ("cal_sin_cn04", "sintel/p_y3", "sim3c", ["--chroma-noise", "0.4"]),
    ("cal_sin_cn05", "sintel/p_y3", "sim3c", ["--chroma-noise", "0.5"]),
    ("cal_sin_cn06", "sintel/p_y3", "sim3c", ["--chroma-noise", "0.6"]),
    ("cal_sin_cb03cr06", "sintel/p_y3", "sim3c", ["--set", "channel_noise={\"Y\": 1.0, \"Cb\": 0.3, \"Cr\": 0.6}"]),
    ("cal_sin_cb02cr06", "sintel/p_y3", "sim3c", ["--set", "channel_noise={\"Y\": 1.0, \"Cb\": 0.2, \"Cr\": 0.6}"]),
    ("cal_sin_thr2", "sintel/p_y3", "sim3c", ["--set", "threshold=2"]),
    ("cal_sin_thr6", "sintel/p_y3", "sim3c", ["--set", "threshold=6"]),
    ("cal_sin_nl07", "sintel/p_y3", "sim3c", ["--noise-level", "0.7"]),
    ("cal_sin_nl09", "sintel/p_y3", "sim3c", ["--noise-level", "0.9"]),
    # park_joy (v5 + pyramid): gap is Y only (-0.55); pyramid_scale finer + threshold + nl
    ("cal_pj_pyr12", "park_joy/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.2"]),
    ("cal_pj_pyr13", "park_joy/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.3"]),
    ("cal_pj_pyr14", "park_joy/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.4"]),
    ("cal_pj_pyr16", "park_joy/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.6"]),
    ("cal_pj_thr2", "park_joy/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "threshold=2"]),
    ("cal_pj_thr6", "park_joy/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "threshold=6"]),
    ("cal_pj_nl07", "park_joy/p_y3", "v5", ["--set", "similarity=pyramid", "--noise-level", "0.7"]),
    ("cal_pj_nl09", "park_joy/p_y3", "v5", ["--set", "similarity=pyramid", "--noise-level", "0.9"]),
    # old_town_cross (v5 + pyramid): already beats genuine; robustness only
    ("cal_otc_thr2", "old_town_cross/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "threshold=2"]),
    ("cal_otc_thr6", "old_town_cross/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "threshold=6"]),
    ("cal_otc_nl07", "old_town_cross/p_y3", "v5", ["--set", "similarity=pyramid", "--noise-level", "0.7"]),
    ("cal_otc_nl09", "old_town_cross/p_y3", "v5", ["--set", "similarity=pyramid", "--noise-level", "0.9"]),
    # wave 2: diag_removed shows sintel chroma over-smoothing at cn 0.3 (removed-picture dominates) ->
    # low side; pyramid_scale 1.2 improved park_joy Y, try 1.1; otc robustness at pyr12
    ("cal_sin_cn015", "sintel/p_y3", "sim3c", ["--chroma-noise", "0.15"]),
    ("cal_sin_cn02", "sintel/p_y3", "sim3c", ["--chroma-noise", "0.2"]),
    ("cal_sin_cb03cr02", "sintel/p_y3", "sim3c", ["--set", "channel_noise={\"Y\": 1.0, \"Cb\": 0.3, \"Cr\": 0.2}"]),
    ("cal_sin_cb02cr02", "sintel/p_y3", "sim3c", ["--set", "channel_noise={\"Y\": 1.0, \"Cb\": 0.2, \"Cr\": 0.2}"]),
    ("cal_pj_pyr11", "park_joy/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.1"]),
    ("cal_otc_pyr12", "old_town_cross/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.2"]),
    # spatial threshold scale (v6 spatial cfg sigma = genuine_t), default 1.0
    ("cal_pj_gt09", "park_joy/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "genuine_t=0.9"]),
    ("cal_sin_gt09", "sintel/p_y3", "sim3c", ["--set", "genuine_t=0.9"]),
    # wave 3: combinations of the wave-1/2 winners
    ("cal_sin_thr2cr02", "sintel/p_y3", "sim3c", ["--set", "threshold=2", "--set", "channel_noise={\"Y\": 1.0, \"Cb\": 0.3, \"Cr\": 0.2}"]),
    ("cal_sin_thr1", "sintel/p_y3", "sim3c", ["--set", "threshold=1"]),
    ("cal_pj_pyr11nl07", "park_joy/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.1", "--noise-level", "0.7"]),
    ("cal_pj_pyr10", "park_joy/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.0"]),
    ("cal_pj_pyr09", "park_joy/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=0.9"]),
    ("cal_otc_pyr12nl07", "old_town_cross/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.2", "--noise-level", "0.7"]),
    ("cal_otc_pyr11", "old_town_cross/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.1"]),
    # wave 4: final combinations
    ("cal_sin_thr15", "sintel/p_y3", "sim3c", ["--set", "threshold=1.5"]),
    ("cal_sin_thr1cr02", "sintel/p_y3", "sim3c", ["--set", "threshold=1", "--set", "channel_noise={\"Y\": 1.0, \"Cb\": 0.3, \"Cr\": 0.2}"]),
    ("cal_pj_pyr10nl07", "park_joy/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.0", "--noise-level", "0.7"]),
    ("cal_otc_pyr13nl07", "old_town_cross/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.3", "--noise-level", "0.7"]),
    ("cal_otc_pyr12nl065", "old_town_cross/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.2", "--noise-level", "0.65"]),
    # wave 5: otc nl trend + sigma-6 validation (global compromise + per-material best transfer)
    ("cal_otc_pyr12nl06", "old_town_cross/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.2", "--noise-level", "0.6"]),
    ("cal_otc_pyr12nl055", "old_town_cross/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.2", "--noise-level", "0.55"]),
    ("cal6_pj_glob", "park_joy/p_y6", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.1", "--noise-level", "0.75"]),
    ("cal6_sin_glob", "sintel/p_y6", "sim3c", ["--set", "threshold=1.5", "--noise-level", "0.75"]),
    ("cal6_otc_glob", "old_town_cross/p_y6", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.1", "--noise-level", "0.75"]),
    ("cal6_pj_best", "park_joy/p_y6", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.0", "--noise-level", "0.8"]),
    ("cal6_sin_best", "sintel/p_y6", "sim3c", ["--set", "threshold=1.5", "--noise-level", "0.8"]),
    ("cal6_otc_best", "old_town_cross/p_y6", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.2", "--noise-level", "0.65"]),
    # wave 6: map the sigma-6 tradeoff for the pyramid knobs
    ("cal6_pj_pyr12nl08", "park_joy/p_y6", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.2", "--noise-level", "0.8"]),
    ("cal6_pj_pyr11nl08", "park_joy/p_y6", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.1", "--noise-level", "0.8"]),
    ("cal6_otc_pyr12nl07", "old_town_cross/p_y6", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.2", "--noise-level", "0.7"]),
    ("cal6_otc_pyr11nl08", "old_town_cross/p_y6", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.1", "--noise-level", "0.8"]),
    # wave 7: the global compromise config at sigma-3 (one config for all materials)
    ("cal_glob_pj", "park_joy/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.2", "--noise-level", "0.75"]),
    ("cal_glob_sin", "sintel/p_y3", "sim3c", ["--set", "threshold=1.5", "--noise-level", "0.75"]),
    ("cal_glob_otc", "old_town_cross/p_y3", "v5", ["--set", "similarity=pyramid", "--set", "pyramid_scale=1.2", "--noise-level", "0.75"]),
]


RUNS6 = [
    ("cal6_pj_base", "park_joy/p_y6", "v5", ["--set", "similarity=pyramid"]),
    ("cal6_sin_base", "sintel/p_y6", "sim3c", []),
    ("cal6_otc_base", "old_town_cross/p_y6", "v5", ["--set", "similarity=pyramid"]),
]

GEN_NUM6 = {"park_joy": 4, "sintel": 6, "old_town_cross": 2}
PROFILE6 = {"park_joy": "runs/ours_frame_f5/park_joy/p_y6/ui_profile_spatial.json",
            "sintel": "runs/ours_frame_f5/sintel/p_y6/ui_profile_spatial.json",
            "old_town_cross": "runs/ours_frame_f5/old_town_cross/p_y6/ui_profile_spatial.json"}


def run_one(spec):
    tag, clip, variant, extra = spec
    src, lv = clip.split("/")
    prof_map = PROFILE6 if lv.endswith("y6") else PROFILE
    gen_map = GEN_NUM6 if lv.endswith("y6") else GEN_NUM
    dst = os.path.join(E9, "runs", "mtn_" + tag, src, lv, "out.mp4")
    if not os.path.exists(dst):
        base_nl = ["--noise-level", "0.8"]
        base_cn = ["--chroma-noise", "0.3"]
        if any("channel_noise=" in x for x in extra):
            base_cn = []  # the explicit per-channel set must not be overwritten by --chroma-noise
        cmd = [sys.executable, os.path.join(HERE, "e9_nvm.py"), clip, "--variant", variant,
               "--tag", tag, "--frames", "60", "--jobs", "4",
               "--profile-file", os.path.join(E9, prof_map[src]),
               "--spatial-variant", "v6set0"] + base_nl + base_cn + extra
        env = dict(os.environ, NV_NATIVE="0", NV6_E9_IMPL="py")
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=7200, env=env,
                           cwd=os.path.normpath(os.path.join(HERE, "..", "..", "..", "..")))
        if r.returncode != 0:
            return tag, clip, None, r.stderr[-300:]
    return tag, clip, dst, None


def main():
    only = None
    if len(sys.argv) > 2 and sys.argv[1] == "--only":
        only = set(sys.argv[2].split(","))
    jobs = [r for r in RUNS + RUNS6 if only is None or r[0] in only]
    with ThreadPoolExecutor(max_workers=2) as ex:
        results = list(ex.map(run_one, jobs))
    # evaluate everything that exists
    os.makedirs(EVALDIR, exist_ok=True)
    for tag, clip, dst, err in results:
        if err or not dst or not os.path.exists(dst):
            print("FAIL", tag, clip, err)
            continue
        src, lv = clip.split("/")
        gen_map = GEN_NUM6 if lv.endswith("y6") else GEN_NUM
        jf = os.path.join(EVALDIR, "%s__%s.json" % (tag, src))
        cmd = [sys.executable, os.path.join(E9, "tools", "e9_eval.py"), clip,
               "--test", "x=%s" % dst,
               "--test", "genuine=genuine/progressive/Event_バージョン 1_0001_000%d" % gen_map[src],
               "--no-pairs", "--out", jf]
        subprocess.run(cmd, capture_output=True, cwd=E9, check=True)
        d = json.load(open(jf))["tests"]
        i, g = d["x"]["interior"], d["genuine"]["interior"]
        print("%-16s %-16s Y %6.2f (g %6.2f)  RGB %6.2f (g %6.2f)  Cb %6.2f (g %6.2f)  Cr %6.2f (g %6.2f)"
              % (tag, src, i["yuv.Y"]["psnr"], g["yuv.Y"]["psnr"], i["rgb.RGB"]["psnr"], g["rgb.RGB"]["psnr"],
                 i["yuv.Cb"]["psnr"], g["yuv.Cb"]["psnr"], i["yuv.Cr"]["psnr"], g["yuv.Cr"]["psnr"]))


if __name__ == "__main__":
    main()
