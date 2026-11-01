# INT-2 L-1 check: for interlaced clips made by tinterlace from a known progressive sequence (test.mp4 n = 300..309 as
# 60p; interleave_top = the top field from the earlier frame), which field of each decoded frame comes from the
# earlier progressive frame (the actual first field in time), against qtui's first_field (scanprobe output on stdin).
#   scanprobe CLIPS... | python3 l1_actual.py
import subprocess
import sys

import numpy as np

SRC = sys.argv[1] if len(sys.argv) > 1 else "test.mp4"
W, H = 640, 360


def dec(path, n, vf=None):
    a = ["ffmpeg", "-v", "error", "-nostdin", "-i", path] + (["-vf", vf] if vf else []) + [
        "-frames:v", str(n), "-fps_mode", "passthrough", "-f", "rawvideo", "-pix_fmt", "gray", "-"]
    b = subprocess.run(a, capture_output=True, check=True).stdout
    return np.frombuffer(b, np.uint8).reshape(-1, H, W).astype(np.float64)


src = dec(SRC, 10, "select=between(n\\,300\\,309),setpts=N/(60*TB)")
ok = True
for line in sys.stdin:
    path, *kv = line.rstrip("\n").split("\t")
    ff = int([x for x in kv if x.startswith("first_field=")][0].split("=")[1])
    if "prog" in path:
        continue
    d = dec(path, 4)
    votes = []
    for k in range(d.shape[0]):
        e_top_early = np.abs(d[k][0::2] - src[2 * k][0::2]).mean()
        e_top_late = np.abs(d[k][0::2] - src[2 * k + 1][0::2]).mean()
        votes.append(0 if e_top_early < e_top_late else 1)    # 0: the top field is the earlier one
    actual = votes[0] if len(set(votes)) == 1 else -2
    good = ff == actual or ff == -1
    ok &= good
    print(f"{path.split('/')[-1]:22s} actual first field {actual} (frames {votes}), qtui first_field {ff}: "
          f"{'agrees' if ff == actual else ('order unknown (not used)' if ff == -1 else 'WRONG')}")
print("L-1: qtui's field order agrees with the content" if ok else "L-1: MISMATCH")
