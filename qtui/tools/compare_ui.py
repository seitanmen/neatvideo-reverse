"""Compare the outputs of qtui_dump (C++ UI) and py_dump.py (PySide6 UI) for the same scenario.

    python3 qtui/tools/compare_ui.py PY_DIR CPP_DIR [SIDE_BY_SIDE_DIR]

- state_*.json: every displayed string must be equal; the profile JSON is compared leaf by leaf (dict keys in
  order, Python types, floats by their 64-bit pattern) and as json.dumps(indent=1) text.
- panes/*.rgb: the viewer pane images (display.py vs libnvcore) byte for byte.
- *.png grabs: the number of differing pixels and the largest channel difference (fonts and widgets are the
  same Qt 6.12.0 offscreen build, so equal grabs are expected where the windows show the same thing);
  side-by-side images (Python left, C++ middle, |difference| x 8 right) are written to SIDE_BY_SIDE_DIR.
Exit code 0 when the states and the pane images are equal.
"""
import json
import os
import struct
import sys

import numpy as np


def leaves(a, b, path="$", out=None):
    out = [] if out is None else out
    if type(a) is not type(b):
        out.append(f"{path}: type {type(a).__name__} vs {type(b).__name__}")
    elif isinstance(a, dict):
        if list(a) != list(b):
            out.append(f"{path}: keys {list(a)} vs {list(b)}")
        for k in a:
            if k in b:
                leaves(a[k], b[k], f"{path}.{k}", out)
    elif isinstance(a, list):
        if len(a) != len(b):
            out.append(f"{path}: len {len(a)} vs {len(b)}")
        for i, (x, y) in enumerate(zip(a, b)):
            leaves(x, y, f"{path}[{i}]", out)
    elif isinstance(a, float):
        if struct.pack("<d", a) != struct.pack("<d", b):
            out.append(f"{path}: {a!r} vs {b!r}")
    elif a != b:
        out.append(f"{path}: {a!r} vs {b!r}")
    return out


def load_png(path):
    from PIL import Image
    return np.asarray(Image.open(path).convert("RGB"))


def main():
    py, cpp = sys.argv[1], sys.argv[2]
    sbs = sys.argv[3] if len(sys.argv) > 3 else None
    ok = True
    print("== states")
    for f in sorted(x for x in os.listdir(py) if x.startswith("state_")):
        a, b = json.load(open(os.path.join(py, f))), json.load(open(os.path.join(cpp, f)))
        pa, pb = a.pop("profile"), b.pop("profile")
        diffs = leaves(a, b)
        pd = leaves(pa, pb, "$.profile")
        text_same = json.dumps(pa, indent=1) == json.dumps(pb, indent=1)
        n_fields = len(a)
        print(f"  {f}: {n_fields - len([d for d in diffs if d.count('.') == 1])}/{n_fields} fields equal, "
              f"profile leaves {'equal' if not pd else f'{len(pd)} differ'}, profile JSON text "
              f"{'identical' if text_same else 'differs'}")
        for d in (diffs + pd)[:12]:
            print("     ", d)
        ok = ok and not diffs and not pd and text_same
    for f in sorted(x for x in os.listdir(py) if x.startswith("filter_")):
        if not os.path.exists(os.path.join(cpp, f)):
            print(f"  {f}: missing in the C++ dump")
            ok = False
            continue
        a, b = json.load(open(os.path.join(py, f))), json.load(open(os.path.join(cpp, f)))
        d = leaves(a, b)
        n = len(a["controls"]) + len(a["common"]) + len(a["headers"]) + len(a) - 3
        print(f"  {f}: {n} entries, {'identical' if not d else f'{len(d)} differences'}")
        for x in d[:20]:
            print("     ", x)
        ok = ok and not d
    for f in ("preset.nvpreset.json", "profile.nvprof.json", "export_a.ui_params.json", "export_a.ui_profile.json",
              "export_b.ui_params.json", "export_b.ui_profile.json"):
        pa, pb = os.path.join(py, f), os.path.join(cpp, f)
        if os.path.exists(pa) or os.path.exists(pb):
            same_t = os.path.exists(pa) and os.path.exists(pb) and open(pa, "rb").read() == open(pb, "rb").read()
            print(f"  {f}: {'identical text' if same_t else 'DIFFERENT'}")
            ok = ok and same_t
    for f in ("export_a", "export_b"):
        pa, pb = os.path.join(py, f + ".mp4"), os.path.join(cpp, f + ".mp4")
        if os.path.exists(pa) or os.path.exists(pb):
            ea = json.load(open(os.path.join(py, "exports.json"))).get(f)
            eb = json.load(open(os.path.join(cpp, "exports.json"))).get(f)
            same_b = os.path.exists(pa) and os.path.exists(pb) and open(pa, "rb").read() == open(pb, "rb").read()
            size = os.path.getsize(pa) if os.path.exists(pa) else -1
            ma = json.load(open(os.path.join(py, f + ".json"))) if os.path.exists(os.path.join(py, f + ".json")) else {}
            mb = json.load(open(os.path.join(cpp, f + ".json"))) if os.path.exists(os.path.join(cpp, f + ".json")) else {}
            ma.pop("seconds", None)
            mb.pop("seconds", None)
            md = leaves(ma, mb, "$.meta")
            print(f"  {f}.mp4: ok {ea}/{eb}, {'byte-identical' if same_b else 'DIFFERENT'} ({size} B), side file "
                  f"{'equal' if not md else f'{len(md)} differences'}")
            for x in md[:10]:
                print("     ", x)
            ok = ok and same_b and ea and eb and not md
    for f in ("menus.json", "toolbars.json"):
        if os.path.exists(os.path.join(py, f)) and os.path.exists(os.path.join(cpp, f)):
            a, b = json.load(open(os.path.join(py, f))), json.load(open(os.path.join(cpp, f)))
            d = leaves(a, b, "$")
            n = sum(len(m["items"]) for m in a) if f == "menus.json" else len(a)
            print(f"  {f}: {n} entries, {'identical' if not d else f'{len(d)} differences'}")
            for x in d[:40]:
                print("     ", x)
    print("== panes")
    ia = json.load(open(os.path.join(py, "panes", "index.json")))
    ib = {e["file"]: e for e in json.load(open(os.path.join(cpp, "panes", "index.json")))}
    same = 0
    bad = []
    for e in ia:
        fa = open(os.path.join(py, "panes", e["file"] + ".rgb"), "rb").read()
        fb = open(os.path.join(cpp, "panes", e["file"] + ".rgb"), "rb").read()
        eb = ib.get(e["file"])
        if eb and (eb["H"], eb["W"]) == (e["H"], e["W"]) and fa == fb:
            same += 1
        else:
            n = -1
            if eb and len(fa) == len(fb):
                n = int((np.frombuffer(fa, np.uint8) != np.frombuffer(fb, np.uint8)).sum())
            bad.append(f"{e['file']} ({e['H']}x{e['W']}): {n} bytes differ")
    print(f"  {same}/{len(ia)} pane images byte-identical "
          f"({sum(e['H'] * e['W'] for e in ia)} pixels in all)")
    for d in bad[:20]:
        print("     ", d)
    ok = ok and not bad
    print("== grabs")
    for f in sorted(x for x in os.listdir(py) if x.endswith(".png")):
        a, b = load_png(os.path.join(py, f)), load_png(os.path.join(cpp, f))
        if a.shape != b.shape:
            print(f"  {f}: size {a.shape} vs {b.shape}")
            continue
        d = np.abs(a.astype(int) - b.astype(int))
        nd = int((d.max(-1) > 0).sum())
        print(f"  {f}: {nd} of {a.shape[0] * a.shape[1]} pixels differ ({100.0 * nd / (a.shape[0] * a.shape[1]):.3f}%), "
              f"max |diff| {int(d.max())}")
        if sbs:
            from PIL import Image
            os.makedirs(sbs, exist_ok=True)
            gap = np.full((a.shape[0], 8, 3), 255, np.uint8)
            img = np.concatenate([a, gap, b, gap, np.clip(d * 8, 0, 255).astype(np.uint8)], 1)
            Image.fromarray(img).save(os.path.join(sbs, "sbs_" + f))
    print("RESULT:", "EQUAL" if ok else "DIFFERENT")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
