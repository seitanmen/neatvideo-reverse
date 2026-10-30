"""Verifier: core backward compat (build_profile_v5 / apply_aft vs backup) + rectangle consistency.
   cd repro && python3 ../sandbox/omo/coord/genuine_ui/verify/v_core.py [--aft]"""
import importlib.machinery, importlib.util, math, os, sys, json
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, *[".."] * 5))
REPRO = os.path.join(ROOT, "repro"); BK = os.path.join(ROOT, "sandbox/omo/coord/backup")
sys.path.insert(0, REPRO)
import numpy as np

def load(name, path):
    l = importlib.machinery.SourceFileLoader(name, path)
    s = importlib.util.spec_from_loader(name, l); m = importlib.util.module_from_spec(s); l.exec_module(m); return m

def eq(a, b, p=""):
    if isinstance(a, dict):
        if not isinstance(b, dict) or set(a) != set(b): return [p + " keys"]
        return sum((eq(a[k], b[k], p + "." + str(k)) for k in a), [])
    if isinstance(a, (list, tuple)):
        if not isinstance(b, (list, tuple)) or len(a) != len(b): return [p + " len"]
        return sum((eq(x, y, p + f"[{i}]") for i, (x, y) in enumerate(zip(a, b))), [])
    if isinstance(a, np.ndarray):
        return [] if (a.dtype == b.dtype and np.array_equal(a, b, equal_nan=True)) else [p + " arr"]
    if isinstance(a, float) and isinstance(b, float):
        if math.isnan(a) and math.isnan(b): return []
        return [] if np.float64(a).tobytes() == np.float64(b).tobytes() else [p + f" {a!r}!={b!r}"]
    return [] if (type(a) is type(b) and a == b) else [p + f" {a!r}!={b!r}"]

def main():
    import nvfilter as new
    from denoise_video_v5 import probe, read_frames
    old = load("nvf_old", os.path.join(BK, "nvfilter.py.20261026_pre_genuineui"))
    n_ok = n = 0; fails = []
    cases = [("test.mp4", [123, 640]), ("test_v02.mp4", [37, 455, 700])]
    store = {}
    for name, idx in cases:
        path = os.path.join(ROOT, name); info = probe(path); H, W = info["H"], info["W"]
        mat = new.matrix_from_tags(info["tags"], H)
        for f in idx:
            win = [tuple(p.astype(np.float32) for p in fr) for _, fr in read_frames(path, W, H, f, f + 3, info["fps"])]
            planes, more = win[0], win[1:]
            store[(name, f)] = (planes, more, mat)
            for mode in ("auto", "spatial", "temporal"):
                for area in (None, (17, 29, 96), (H - 50, W - 50, 200), (3, 5, 48), (H // 2, 1, 257), (0, 0, 9999), (-3, W, 64)):
                    for fr_ in (False,) if area is not None else (False, True):
                        kw = dict(area=area, pair=more[0], mode=mode, more=more, matrix=mat)
                        a = old.build_profile_v5(planes, fr_, **kw); b = new.build_profile_v5(planes, fr_, **kw)
                        d = eq(a, b); n += 1; n_ok += not d
                        print(f"[{'PASS' if not d else 'FAIL'}] {name} f{f} {mode} fr={fr_} area={area} -> {b['area']} {d[:3]}", flush=True)
    # rectangle consistency
    print("--- rectangles")
    for (name, f), (planes, more, mat) in store.items():
        H, W = planes[0].shape
        sub = new._sub(planes)
        for rect in ((0, 0, 216, 512), (H - 10, W - 10, 216, 512), (11, 23, 33, 47), (-20, -30, 70, 40), (5, 7, 2000, 77), (5, 7, 3, 3000), (40, 60, 100, 101), (0, 0, 2000, 2000), (-5, 3, 64, 64)):
            p = new.build_profile_v5(planes, False, area=rect, pair=more[0], mode="spatial", more=more, matrix=mat)
            # independent clamp expectation
            h = min(max(rect[2], 8), H); w = min(max(rect[3], 8), W)
            y = min(max(rect[0], 0), H - h); x = min(max(rect[1], 0), W - w)
            exp = [y, x, h, w] if h != w else [y, x, h]
            ok_area = p["area"] == exp
            crop = planes[0][y:y + h, x:x + w]
            ok_lvl = np.float64(p["area_level"]).tobytes() == np.float64(float(np.mean(crop))).tobytes()
            # band sigma from crop alone
            okb = True
            for c, nm in enumerate(("Y", "Cb", "Cr")):
                s = 1 if c == 0 else sub; L = p["levels"][nm]
                pc = np.asarray(planes[c], np.float32)[y // s:(y + h) // s, x // s:(x + w) // s]
                sg, _ = old.area_band_sigma(pc, (0, 0, pc.shape[0] * s), L, s) if pc.shape[0] == pc.shape[1] else (None, None)
                sg2, _ = new.area_band_sigma(pc, (0, 0, pc.shape[0] * s, pc.shape[1] * s), L, s)
                okb &= sg2 == p["area_sigma"][nm]
            sig_ok = all(abs(p["sigma"][nm] - float(np.interp(p["area_level"], p["curve_centers"], p["curve"][nm]) * p["tot_ratio"][nm])) == 0 for nm in ("Y", "Cb", "Cr"))
            ok = ok_area and ok_lvl and okb and sig_ok
            n += 1; n_ok += ok
            print(f"[{'PASS' if ok else 'FAIL'}] {name} f{f} rect={rect} -> {p['area']} exp={exp} lvl={ok_lvl} band={okb} sigma={sig_ok} "
                  f"σ={ {k: round(v, 3) for k, v in p['sigma'].items()} }", flush=True)
    print("--- 4-tuple squares == old 3-tuple")
    for (name, f), (planes, more, mat) in store.items():
        for y, x, S in ((8, 8, 64), (17, 29, 96), (0, 0, 300)):
            a = old.build_profile_v5(planes, False, area=(y, x, S), pair=more[0], mode="auto", more=more, matrix=mat)
            b = new.build_profile_v5(planes, False, area=(y, x, S, S), pair=more[0], mode="auto", more=more, matrix=mat)
            d = eq(a, b); n += 1; n_ok += not d
            print(f"[{'PASS' if not d else 'FAIL'}] {name} f{f} new{(y, x, S, S)} == old{(y, x, S)} -> {b['area']} {d[:3]}", flush=True)
    if "--aft" in sys.argv:
        import nv_aft_v5 as anew
        aold = load("aft_old", os.path.join(BK, "nv_aft_v5.py.20261026_pre_genuineui"))
        import nv_ui_qt.backend as be
        for key in (("test.mp4", 123), ("test_v02.mp4", 455)):
            planes, more, mat = store[key]
            rgb = None
            # RGB as the UI uses it
            for nmf in ("rgb_from_planes", "to_rgb", "planes_to_rgb"):
                if hasattr(be, nmf): rgb = getattr(be, nmf); break
            print("rgb fn", rgb)
            import nvfilter
            Y, Cb, Cr = planes
            H, W = Y.shape
            def up(c): return np.repeat(np.repeat(c, 2, 0), 2, 1)[:H, :W]
            # simple BT.709 limited->RGB (any deterministic RGB works for old==new)
            yy = (Y - 16) * 255 / 219; cb = (up(Cb) - 128) * 255 / 224; cr = (up(Cr) - 128) * 255 / 224
            R = yy + 1.5748 * cr; G = yy - 0.1873 * cb - 0.4681 * cr; B = yy + 1.8556 * cb
            img = np.clip(np.stack([R, G, B], -1), 0, 255).astype(np.float32)
            for area in ((20, 30, 96), None):
                prof = new.build_profile_v5(planes, False, area=area, pair=more[0], mode="auto", more=more, matrix=mat)
                import copy
                a = aold.apply_aft(copy.deepcopy(prof), img); b = anew.apply_aft(copy.deepcopy(prof), img)
                d = eq(a, b); n += 1; n_ok += not d
                print(f"[{'PASS' if not d else 'FAIL'}] apply_aft {key} area={prof['area']} {d[:3]}", flush=True)
            prof = new.build_profile_v5(planes, False, area=(20, 30, 64, 160), pair=more[0], mode="auto", more=more, matrix=mat)
            b = anew.apply_aft(copy.deepcopy(prof), img)
            # rectangle aft == aft run on a crop-equivalent? check finite & changed
            ok = all(np.all(np.isfinite(b["curve"][k])) for k in b["curve"]) and b["curve"] != prof["curve"]
            n += 1; n_ok += ok
            print(f"[{'PASS' if ok else 'FAIL'}] apply_aft rect {key} area={prof['area']} Y={[round(v, 3) for v in b['curve']['Y']]}", flush=True)
    print(f"{n_ok}/{n} passed")

if __name__ == "__main__":
    main()
