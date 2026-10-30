"""§5.3: build_profile_v5 / apply_aft after the rectangle change == the pre-change backups bit for bit
for (y0, x0, size) areas and FindFlatArea, and rectangles (y0, x0, h, w) work.

    cd repro && python3 ../sandbox/omo/coord/genuine_ui/impl/test_rect.py [--aft]
"""
import importlib.machinery
import importlib.util
import json
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", "..", "..", ".."))
REPRO = os.path.join(ROOT, "repro")
BACKUP = os.path.join(ROOT, "sandbox", "omo", "coord", "backup")
sys.path.insert(0, REPRO)

import numpy as np  # noqa: E402


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path,
                                                  loader=importlib.machinery.SourceFileLoader(name, path))
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


def same(a, b):
    """Exact equality of nested profile dicts (floats compared by value, NaN == NaN)."""
    if isinstance(a, dict):
        return isinstance(b, dict) and a.keys() == b.keys() and all(same(a[k], b[k]) for k in a)
    if isinstance(a, (list, tuple)):
        return isinstance(b, (list, tuple)) and len(a) == len(b) and all(same(x, y) for x, y in zip(a, b))
    if isinstance(a, np.ndarray):
        return np.array_equal(a, b, equal_nan=True) and a.dtype == b.dtype
    if isinstance(a, float) and isinstance(b, float) and math.isnan(a) and math.isnan(b):
        return True
    return type(a) is type(b) and a == b


def frames(path, idx):
    from denoise_video_v5 import probe, read_frames
    info = probe(path)
    out = {}
    for f in idx:
        got = [tuple(p.astype(np.float32) for p in fr) for _, fr in
               read_frames(path, info["W"], info["H"], f, f + 3, info["fps"])]
        out[f] = got
    return info, out


def main():
    import nvfilter as new
    old = load("nvfilter_pre", os.path.join(BACKUP, "nvfilter.py.20261026_pre_genuineui"))
    res = []
    cases = [("test.mp4", [0, 300, 450]), ("test_v02.mp4", [100, 312, 576])]
    for name, idx in cases:
        info, fr = frames(os.path.join(ROOT, name), idx)
        H, W = info["H"], info["W"]
        mat = new.matrix_from_tags(info["tags"], H)
        for f, win in fr.items():
            planes, more = win[0], win[1:]
            for mode in ("auto", "spatial", "temporal"):
                for area in (None, (8, 8, 64), (0, W - 200, 128), (H - 100, 3, 333), (-5, -5, 5000)):
                    kw = dict(area=area, pair=more[0], mode=mode, more=more, matrix=mat)
                    a = old.build_profile_v5(planes, False, **kw)
                    b = new.build_profile_v5(planes, False, **kw)
                    ok = same(a, b)
                    res.append(ok)
                    print(f"[{'PASS' if ok else 'FAIL'}] {name} f{f} {mode} area={area} -> {b['area']}", flush=True)
                # the square given as a 4-tuple (h == w) gives the same profile as (y0, x0, size)
                a = new.build_profile_v5(planes, False, area=(8, 8, 64), pair=more[0], mode=mode, more=more,
                                         matrix=mat)
                b = new.build_profile_v5(planes, False, area=(8, 8, 64, 64), pair=more[0], mode=mode,
                                         more=more, matrix=mat)
                ok = same(a, b) and b["area"] == [8, 8, 64]
                res.append(ok)
                print(f"[{'PASS' if ok else 'FAIL'}] {name} f{f} {mode} (8,8,64,64) == (8,8,64)", flush=True)
            # rectangles: 4-element area, the band sigmas are those of the crop
            for area in ((0, 0, 216, 512), (H - 40, W - 300, 216, 512), (10, 20, 33, 47)):
                p = new.build_profile_v5(planes, False, area=area, pair=more[0], mode="spatial", more=more,
                                         matrix=mat)
                y0, x0, h, w = p["area"]
                sig, _ = new.area_band_sigma(planes[0], (y0, x0, h, w), p["levels"]["Y"])
                crop = planes[0][y0:y0 + h, x0:x0 + w]
                lvl = float(np.mean(np.asarray(crop, np.float32)))
                ok = (len(p["area"]) == 4 and 0 <= y0 <= H - h and 0 <= x0 <= W - w and sig == p["area_sigma"]["Y"]
                      and p["area_level"] == lvl and all(math.isfinite(v) for v in p["sigma"].values())
                      and (h, w) == (min(area[2], H), min(area[3], W)))
                res.append(ok)
                print(f"[{'PASS' if ok else 'FAIL'}] {name} f{f} rect {area} -> {p['area']} "
                      f"sigma {json.dumps({k: round(v, 3) for k, v in p['sigma'].items()})}", flush=True)
    if "--aft" in sys.argv:
        import nv_aft_v5 as aft_new
        aft_old = load("nv_aft_v5_pre", os.path.join(BACKUP, "nv_aft_v5.py.20261026_pre_genuineui"))
        info, fr = frames(os.path.join(ROOT, "test.mp4"), [300])
        planes, more = fr[300][0], fr[300][1:]
        mat = new.matrix_from_tags(info["tags"], info["H"])
        rgb = np.clip(new.yuv_to_rgb(planes, mat, False), 0, 255).astype(np.float32)
        import copy
        for area in (None, (40, 60, 96)):
            base = new.build_profile_v5(planes, False, area=area, pair=more[0], more=more, matrix=mat)
            a = aft_old.apply_aft(copy.deepcopy(base), rgb)
            b = aft_new.apply_aft(copy.deepcopy(base), rgb)
            ok = same(a, b)
            res.append(ok)
            print(f"[{'PASS' if ok else 'FAIL'}] apply_aft old == new, area {base['area']}", flush=True)
        base = new.build_profile_v5(planes, False, area=(40, 60, 64, 160), pair=more[0], more=more, matrix=mat)
        b = aft_new.apply_aft(copy.deepcopy(base), rgb)
        ok = b.get("aft_applied") and all(math.isfinite(v) for c in b["curve"].values() for v in c)
        res.append(ok)
        print(f"[{'PASS' if ok else 'FAIL'}] apply_aft rectangle {base['area']}: curve Y "
              f"{[round(v, 3) for v in b['curve']['Y']]}", flush=True)
    print(f"{sum(res)}/{len(res)} passed")
    return 0 if all(res) else 1


if __name__ == "__main__":
    sys.exit(main())
