#!/usr/bin/env python3
"""Writes native/testdata/genuine_quality.json: the profile bytes the genuine Quality / Completeness read, cut from
13 decoded genuine .dnp files, with the values the genuine UI showed for them (wp_genuine_capture/REPORT.md
sections 2 and 4, read from shots/ by an independent verifier).

    python3 make_genuine_fixture.py [CAPTURE_DIR]      (default: the main checkout's work/omo/wp_genuine_capture)

The .dnp files are decoded with CAPTURE_DIR/tools/dnp.py --raw (data only; nothing of the genuine is run).
Kept ranges (everything quality.quality_from_blob reads): colour mode 0x48, model faces 3..5 words 0..54,
0xbf34..0xbf43 (valid flag, size class, clip flags, 0xbf40 word), the curve 0xbf44..0xc2a3, W/H 0xc2c0..0xc2c7.
"""
import json
import os
import subprocess
import sys
import tempfile

CAP = sys.argv[1] if len(sys.argv) > 1 else "/Users/seitan/Desktop/work/reverse/work/omo/wp_genuine_capture"
OUT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "..",
                                    "native", "testdata", "genuine_quality.json"))
MODEL = 0x1F54
RANGES = ([(0x48, 4)] + [(0x4C + (3 + i) * MODEL, 55 * 4) for i in range(3)]
          + [(0xBF34, 0x10), (0xBF44, 0x360), (0xC2C0, 8)])
# (file, screen Quality %, screen Completeness %, screen)
CASES = [
    ("dnp/pa1/pa1_s1_autoprofile.dnp", 72, 80, "pa1 s1 Auto Profile"),
    ("dnp/pa1/pa1_s3_resetft.dnp", 40, 0, "pa1 s3 Reset Fine-Tuning Results"),
    ("dnp/pa1/pa1_s5_aft_again.dnp", 72, 80, "pa1 s5 Auto Fine-Tune again"),
    ("dnp/pa1/pa1_s6_manual_R4_up.dnp", 71, 78, "pa1 s6 manual R bin 4 +50%"),
    ("dnp/pa1/pa1_s7_manual_G2_down.dnp", 70, 75, "pa1 s7 manual G bin 2 -50%"),
    ("dnp/e9/e9p_park_joy_p_y3.dnp", 59, 48, "E9 park_joy p_y3"),
    ("dnp/e9/e9p_park_joy_p_y6.dnp", 53, 51, "E9 park_joy p_y6"),
    ("dnp/e9/e9p_old_town_cross_p_y3.dnp", 65, 63, "E9 old_town_cross p_y3"),
    ("dnp/e9/e9p_sintel_p_y6.dnp", 73, 83, "E9 sintel p_y6"),
    ("dnp/e9/e9i_old_town_cross_tff_y6.dnp", 63, 58, "E9 old_town_cross tff_y6"),
    ("dnp/e9/e9i_park_joy_tff_y6.dnp", 68, 51, "E9 park_joy tff_y6"),
    ("dnp/e9/e9i_sintel_tff_y3.dnp", 54, 56, "E9 sintel tff_y3"),
    ("dnp/e9/e9i_sintel_tff_y6.dnp", 67, 68, "E9 sintel tff_y6"),
]


def main():
    cases = []
    with tempfile.TemporaryDirectory() as tmp:
        for rel, q, c, screen in CASES:
            raw = os.path.join(tmp, "img.bin")
            subprocess.run([sys.executable, "-I", os.path.join(CAP, "tools", "dnp.py"), os.path.join(CAP, rel),
                            "--raw", raw], check=True, capture_output=True)
            img = open(raw, "rb").read()
            cases.append({"name": os.path.basename(rel)[:-4], "screen": screen, "quality": q, "completeness": c,
                          "ranges": [[off, img[off:off + n].hex()] for off, n in RANGES]})
    doc = {"format": "genuine_quality_cases", "version": 1,
           "source": "work/omo/wp_genuine_capture/dnp (decoded with tools/dnp.py --raw); screen values from "
                     "wp_genuine_capture/REPORT.md sections 2 and 4; made by "
                     "work/omo/wp_quality_fix/tools/make_genuine_fixture.py",
           "blob_size": 0xC300, "cases": cases}
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w") as f:
        json.dump(doc, f, indent=1)
        f.write("\n")
    print(f"{OUT}: {len(cases)} cases")


if __name__ == "__main__":
    main()
