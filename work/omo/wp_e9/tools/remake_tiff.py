"""Re-creates one TIFF set of make_clips.py from the existing noisy y4m (exactly the make_clips.py conversion:
rgb16(yuv_to_rgb(noisy, layout)), same folder and file names), touching nothing else, and checks the result against
the SHA256 recorded in clips/<src>/manifest.json (the manifest itself is not rewritten).

    python3 tools/remake_tiff.py <src>/<layout>_<level> [...]
"""
import glob
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import e9_common as C  # noqa: E402
from make_clips import seq_sha  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

for clip in sys.argv[1:]:
    src, lv = clip.split("/")
    layout, level = lv.split("_")
    man = json.load(open(os.path.join(ROOT, "clips", src, "manifest.json")))
    rec = man["files"][f"tiff/{lv}"]
    tdir = os.path.join(ROOT, "clips", src, "tiff", lv)
    if glob.glob(os.path.join(tdir, "*.tif")):
        raise SystemExit(f"{tdir} already holds TIFF files; not overwriting")
    fps = C.SOURCES[src]["fps"].split(":")[0]
    tw = C.ffmpeg_tiff_writer(os.path.join(tdir, f"{src}_{layout}_{level}_%05d.tif"), fps=fps)
    for _, pl in C.read_y4m(os.path.join(ROOT, "clips", src, f"noisy_{layout}_{level}.y4m")):
        tw.stdin.write(C.rgb16(C.yuv_to_rgb(pl, layout)).tobytes())
    tw.stdin.close()
    if tw.wait() != 0:
        raise SystemExit("ffmpeg failed")
    tif = sorted(glob.glob(os.path.join(tdir, "*.tif")))
    h = seq_sha(tif)
    ok = len(tif) == rec["count"] and h == rec["sha256_concat"] and os.path.basename(tif[0]) == rec["first"]
    print(f"{clip}: {len(tif)} files, sha256_concat {h} {'MATCHES manifest' if ok else 'DOES NOT MATCH manifest ' + rec['sha256_concat']}")
    if not ok:
        sys.exit(1)
