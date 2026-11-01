"""Round 2: the UI's extreme values through the window -> ClipParams -> render (must be accepted, bit-exact)."""
import os, sys, re, types
sys.path.insert(0, "/Users/seitan/Desktop/work/reverse/.claude/worktrees/n4-ofx/ofx/tools")
import verify_ofx as V
work = sys.argv[1]; pure = "--pure" in sys.argv
os.makedirs(work, exist_ok=True)
ini0 = V.sha(V.USER_INI)
clips = {}
def put(name, arr):
    path = os.path.join(work, f"clip_{name}.f32"); arr.tofile(path)
    n, H, W, _ = arr.shape; clips[name] = {"path": path, "N": n, "H": H, "W": W}
put("E", V.rgba(V.decode("test_v02.mp4", 400, 12, crop=(400, 256, 900, 600))))
V.make_defaults(work, clips["E"])
C = ["Y", "Cb", "Cr"]; BANDS = ["ultra-low", "very-low", "low", "medium", "high"]
def sets(lo):
    s = [f"set t_radius {0 if lo else 5}", f"set t_amount {0 if lo else 100}", f"set t_noise {0 if lo else 250}",
         f"set adv_threshold {0.25 if lo else 8}", f"set adv_spatial {0.25 if lo else 4}",
         f"set adv_noise_level {25 if lo else 400}", "setb mix_enable 1", f"set mix {0 if lo else 100}"]
    for c in C:
        s += [f"set noise_{c} {0 if lo else 250}", f"set amt_{c} {0 if lo else 100}"]
        for b in BANDS: s += [f"set bamt_{c}_{b} {0 if lo else 100}", f"set bnoise_{c}_{b} {0 if lo else 250}"]
    s += [f"tune {k} {0.2 if lo else 3.0}" for k in ("all", "Y", "Cb", "Cr")]
    return ";".join(s)
P = os.path.join(work, "proc_")
E = lambda **kw: dict({"changed": True, "adjust": 1, "spawn": True, "build": False, "process": None, "status": 0}, **kw)
S = [
 {"name": "E-min", "clip": "E", "first": 0, "origin": [0, 0], "layout": "pad", "ini": "none",
  "steps": [("press", "prepare", 6, f"auto;{sets(True)};process {P}emin.f32;apply", E(process=f"{P}emin.f32")),
            ("render", [6, 0, 11])]},
 {"name": "E-max", "clip": "E", "first": 0, "origin": [0, 0], "layout": "neg", "ini": "none",
  "steps": [("press", "prepare", 6, f"auto;aft;{sets(False)};process {P}emax.f32;apply", E(process=f"{P}emax.f32")),
            ("render", [6, 0, 11, 1]),
            # temporal off, spatial off, spatial-only profile
            ("press", "adjust", 3, f"setb t_enable 0;setb s_enable 0;set mix 0;process {P}eoff.f32;apply", E(process=f"{P}eoff.f32")),
            ("render", [3])]},
]
a = types.SimpleNamespace(pure=pure)
fails, totals, texts = [], [0, 0, 0], {}
for sc in S:
    try: V.run_scenario(a, work, sc, clips, texts, fails, totals)
    except SystemExit as e: fails.append(f"{sc['name']}: {str(e)[:2000]}")
ini1 = V.sha(V.USER_INI)
print("user ini", "unchanged" if ini0 == ini1 else "CHANGED")
print(f"{totals[2]} comparisons, {totals[0]}/{totals[1]} words differ")
print("ALL MATCH" if not fails else "FAILURES:\n  " + "\n  ".join(fails))
