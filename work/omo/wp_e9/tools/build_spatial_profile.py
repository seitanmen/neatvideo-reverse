"""Builds the spatial-mode profile (mode="spatial", frame 5) for an E9 clip with the verified
Python oracle, the same conventions as runs/ours_frame_f5 (matrix bt709, full_range False).

    python3 tools/build_spatial_profile.py park_joy/p_y6 [more clips..]

Writes runs/ours_frame_f5/<src>/<level>/ui_profile_spatial.json (+ profile_info_spatial.json).
The processing core is only called, never modified.
"""
import json
import os
import subprocess
import sys

import numpy as np

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
REPRO = os.path.normpath(os.path.join(ROOT, "..", "..", "..", "reference", "python"))
sys.path.insert(0, REPRO)
import nvfilter as nv  # noqa: E402


def read_frame5(y4m):
    raw = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-i", y4m,
                          "-vf", "select=eq(n\\,5)", "-frames:v", "1",
                          "-f", "rawvideo", "-pix_fmt", "yuv420p", "-"],
                         capture_output=True, check=True).stdout
    H, W = 1080, 1920
    assert len(raw) == W * H * 3 // 2, (len(raw), W * H * 3 // 2)
    y = np.frombuffer(raw, np.uint8)[:W * H].reshape(H, W).astype(np.float32)
    u = np.frombuffer(raw, np.uint8)[W * H:W * H * 5 // 4].reshape(H // 2, W // 2).astype(np.float32)
    v = np.frombuffer(raw, np.uint8)[W * H * 5 // 4:].reshape(H // 2, W // 2).astype(np.float32)
    return y, u, v


def main():
    for clip in sys.argv[1:]:
        src, lv = clip.split("/")
        y4m = os.path.join(ROOT, "clips", src, "noisy_%s.y4m" % lv)
        out = os.path.join(ROOT, "runs", "ours_frame_f5", src, lv)
        os.makedirs(out, exist_ok=True)
        planes = read_frame5(y4m)
        prof = nv.build_profile_v5(planes, full_range=False, mode="spatial", matrix="bt709")
        pj = os.path.join(out, "ui_profile_spatial.json")
        json.dump(prof, open(pj, "w"), indent=1)
        info = {"video": y4m, "profile_select": "Current Frame", "profile_frame": 5,
                "matrix": "bt709", "full_range": False, "area": prof.get("area"),
                "flat_status": prof.get("flat_status"), "mode": "spatial"}
        json.dump(info, open(os.path.join(out, "profile_info_spatial.json"), "w"), indent=1)
        print("%s: area=%s flat_status=%s -> %s" % (clip, prof.get("area"),
                                                    prof.get("flat_status"), pj))


if __name__ == "__main__":
    main()
