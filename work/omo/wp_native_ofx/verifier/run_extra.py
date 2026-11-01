# extra scenarios (independent verifier): same harness, new clips / params / edges
import importlib.util, os, sys
import numpy as np
WT = "/Users/seitan/Desktop/work/reverse/.claude/worktrees/n4-ofx"
B = os.environ["VN4_BUILD"]
spec = importlib.util.spec_from_file_location("verify_ofx", WT + "/ofx/tools/verify_ofx.py")
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
m.BUILD, m.MINIHOST, m.BUNDLE, m.FSGUARD = B, B + "/minihost", B + "/MeatVideo.ofx.bundle", B + "/libfsguard.dylib"

def make_clips(work, quick):
    clips = {}
    def put(name, arr):
        path = os.path.join(work, f"clip_{name}.f32"); arr = np.ascontiguousarray(arr, np.float32); arr.tofile(path)
        n, H, W, _ = arr.shape; clips[name] = {"path": path, "N": n, "H": H, "W": W}
    a = m.decode("test.mp4", 600, 4)                      # frames not used by the worker (300..307)
    put("E", m.rgba(a[:3, 11:198, 101:434]))               # odd size 333x187, 3 frames
    put("F", m.rgba(a[:1, 100:196, 200:328]))              # single frame 128x96
    put("G", m.rgba(a[:, 150:214, 300:364]))               # exactly 64x64, 4 frames
    v = m.decode("test_v02.mp4", 560, 4, crop=(720, 404, 600, 500))
    put("H", m.rgba(v))                                    # 720x404 crop of interval B, 4 frames
    rng = np.random.default_rng(123)
    k = np.zeros((3, 96, 160, 3), np.float32) + np.float32(0.5)
    put("K", m.rgba(k))                                    # constant grey (no noise)
    g = (np.linspace(0, 1, 200, dtype=np.float32)[None, None, :, None] * np.ones((4, 120, 1, 3), np.float32)
         + rng.normal(0, 0.03, (4, 120, 200, 3)).astype(np.float32)) * np.float32(40.0) - np.float32(3.0)
    g = m.rgba(g)
    g[1, 5, 7, 0] = np.nan; g[2, 60, 100, 2] = np.inf; g[0, 0, 0, 1] = -np.inf; g[3, 119, 199, 3] = np.nan
    put("L", g)                                            # HDR gradient x40 with noise, extreme values
    return clips

def scenarios(quick):
    return [
        {"name": "E-odd", "clip": "E", "first": 7, "origin": [-3, 5], "layout": "neg",
         "steps": [["set", {"t_radius": 5, "profile_frame": 2, "matrix": 1}], ["render", [7, 8, 9]],
                   ["set", {"profile_frame": 1, "profile_mode": 1}], ["render", [9]]]},
        {"name": "F-single", "clip": "F", "first": 0, "origin": [0, 0], "layout": "pad",
         "steps": [["set", {"adv_noise_level": 400, "noise_Y": 250, "noise_Cb": 0, "amt_Y": 0, "adv_spatial": 4,
                            "adv_threshold": 0.25}], ["render", [0]],
                   ["set", {"profile_mode": 2}], ["render", [0]]]},
        {"name": "G-64", "clip": "G", "first": 0, "origin": [0, 0], "layout": "pad",
         "steps": [["render", [0, 3]], ["set", {"t_radius": 0, "mix_enable": 1, "mix": 100}], ["render", [1]],
                   ["set", {"mix": 45.5, "t_radius": 3, "t_amount": 33.3, "noise_Cr": 175.25}], ["render", [2]]]},
        {"name": "H-v02B", "clip": "H", "first": 560, "origin": [600, 500], "layout": "neg",
         "steps": [["set", {"profile_frame": 1}], ["render", [560, 563]],
                   ["set", {"adv_threshold": 100, "t_radius": 9, "noise_Y": -5, "amt_C": 250}], ["render", [561]]]},
        {"name": "K-const", "clip": "K", "first": 0, "origin": [0, 0], "layout": "pad", "steps": [["render", [0, 2]]]},
        {"name": "L-hdr", "clip": "L", "first": 0, "origin": [0, 0], "layout": "pad",
         "steps": [["render", [0, 1, 2, 3]], ["set", {"matrix": 2, "aft": 0}], ["render", [3]]]},
    ]
m.make_clips, m.scenarios = make_clips, scenarios
sys.argv = [WT + "/ofx/tools/verify_ofx.py"] + sys.argv[1:]
sys.exit(m.main())
