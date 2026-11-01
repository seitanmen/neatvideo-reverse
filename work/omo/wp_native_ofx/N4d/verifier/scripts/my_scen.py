"""Verifier's own N4d scenarios (new clips / times / settings), using verify_ofx's run_scenario machinery."""
import os, sys, re, json, types
ROOT = "/Users/seitan/Desktop/work/reverse/.claude/worktrees/n4-ofx"
sys.path.insert(0, os.path.join(ROOT, "ofx", "tools"))
import verify_ofx as V

work = sys.argv[1]
pure = "--pure" in sys.argv
only = next((a.split("=", 1)[1] for a in sys.argv if a.startswith("--only=")), "")
os.makedirs(work, exist_ok=True)
ini0 = V.sha(V.USER_INI)
clips = {}
def put(name, arr):
    path = os.path.join(work, f"clip_{name}.f32"); arr.tofile(path)
    n, H, W, _ = arr.shape
    clips[name] = {"path": path, "N": n, "H": H, "W": W}
put("V1", V.rgba(V.decode("test_v02.mp4", 560, 10, crop=(720, 720, 600, 200))))
put("T", V.rgba(V.decode("test.mp4", 100, 6, crop=(600, 64, 20, 150))))
V.make_defaults(work, clips["V1"])
P = os.path.join(work, "proc_")
E = lambda **kw: dict({"changed": True, "adjust": 1, "spawn": True, "build": False, "process": None, "status": 0}, **kw)
S = [
 {"name": "X-v02-aft", "clip": "V1", "first": 50, "origin": [-7, 13], "layout": "neg", "ini": "none",
  "steps": [("render", [52]),
            ("press", "prepare", 55, f"frame 2;auto;aft;frame 5;set t_radius 3;set amt_Cr 70;set bnoise_Y_high 120;tune Cb 1.3;process {P}x1.f32;apply", E(process=f"{P}x1.f32")),
            ("render", [55, 50, 59, 57]),
            ("press", "adjust", 58, f"set t_radius 5;set noise_Y 140;setb mix_enable 1;set mix 50;process {P}x2.f32;apply", E(process=f"{P}x2.f32")),
            ("render", [58, 51]),
            ("press", "adjust", 51, "set amt_Y 5;cancel", E(changed=False)),
            ("press", "prepare", 52, "resetprofile;apply", E(adjust=0)),
            ("render", [52])]},
 {"name": "X-dset", "clip": "V1", "first": 0, "origin": [3, 3], "layout": "pad", "ini": "set",
  "defaults_status": ["ok", "ok"],
  "steps": [("render", [3, 9]), ("sync",), ("press", "adjust", 6, f"set amt_Y 80;process {P}x3.f32;apply", E(process=f"{P}x3.f32")),
            ("render", [6, 0]), ("ini", "none"), ("sync",), ("render", [1])]},
 {"name": "X-thin", "clip": "T", "first": 0, "origin": [0, 0], "layout": "pad", "ini": "none",
  "steps": [("press", "prepare", 2, f"auto;process {P}x4.f32;apply", E(build=True, process=f"{P}x4.f32")),
            ("render", [2, 0, 5])]},
]
S = [s for s in S if not only or re.search(only, s["name"])]
a = types.SimpleNamespace(pure=pure)
fails, totals, texts = [], [0, 0, 0], {}
for sc in S:
    try:
        V.run_scenario(a, work, sc, clips, texts, fails, totals)
    except SystemExit as e:
        fails.append(f"{sc['name']}: {str(e)[:2000]}")
ini1 = V.sha(V.USER_INI)
print("user ini", "unchanged" if ini0 == ini1 else "CHANGED")
print(f"{totals[2]} comparisons, {totals[0]}/{totals[1]} words differ")
print("ALL MATCH" if not fails else "FAILURES:\n  " + "\n  ".join(fails))
