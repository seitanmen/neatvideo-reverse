"""source_format over every pix_fmt of `ffmpeg -pix_fmts` (round 2 of the verification).

    cd repro && python3 ../sandbox/omo/coord/highbits/unit_pixfmts.py

Expected bits = ffmpeg's largest component depth, raised to 8 (choose_format treats <= 8 as 8);
hardware surfaces (no components) fall back to 8-bit yuv 4:2:0. Float = name ending f16 / f32.
Chroma is checked for the yuv planar names (yuv[j][a]4XXp...) against their digits.
"""
import re
import subprocess
import sys

sys.path.insert(0, ".")
import nv_depth as nd                                   # noqa: E402

lines = subprocess.run(["ffmpeg", "-hide_banner", "-pix_fmts"], capture_output=True, text=True).stdout.splitlines()
rows = []
for ln in lines:
    m = re.match(r"^[I.][O.][H.][P.][B.]\s+(\S+)\s+(\d+)\s+(\d+)\s+([\d-]+)$", ln.strip())
    if m:
        rows.append((m.group(1), int(m.group(2)), max(int(x) for x in m.group(4).split("-"))))
CH = {"420": "420", "422": "422", "444": "444", "440": "444", "411": "444", "410": "444"}
bad, hw = [], 0
for name, nc, depth in rows:
    s = nd.source_format(name)
    if nc == 0:
        hw += 1
        ok = s == nd.SrcFmt("yuv", 8, False, "420")
    else:
        ok = s.bits == max(depth, 8) and s.is_float == bool(re.search(r"f(16|32)(le|be)?$", name))
        m = re.match(r"yuvj?a?(4\d\d)p", name)
        if m:
            ok &= s.family == "yuv" and s.chroma == CH[m.group(1)]
        if name.startswith(("gbr", "rgb", "bgr", "argb", "abgr", "0rgb", "0bgr", "x2rgb", "x2bgr", "bayer")):
            ok &= s.family == "rgb"
    if not ok:
        bad.append((name, nc, depth, s))
print(f"{len(rows)} pix_fmts ({hw} hardware surfaces): mismatches {len(bad)}")
for b in bad:
    print("  ", b)
for n in ("yuv444p10msble", "yuv444p10msbbe", "yuv444p12msble", "yuv444p12msbbe", "gbrp10msble", "gbrp10msbbe",
          "gbrp12msble", "gbrp12msbbe"):
    s = nd.source_format(n)
    print(n, tuple(s), "-> depth 10 / source / 32:", [nd.choose_format(s, d).name for d in (10, "source", 32)])
