"""Encoder failing in the middle of an export (verification round 2, finding 2).

    cd repro && python3 ../sandbox/omo/coord/highbits/test_encfail.py

denoise_video_v5.encoder is replaced (in a child process) by a stand-in that writes a partial
output (a file, or three frames of an EXR pattern), reads ~1 frame from the pipe and exits with
code 3. Expected: exit code 1, no Python traceback, the error message, and no output left behind.
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
T = os.path.join(HERE, "test", "r2")
CHILD = r'''
import subprocess, sys
sys.path.insert(0, ".")
import denoise_video_v5 as dv

def fake(dst, info, crf, plan=(), src=None, start=0.0, dur=None, fmt=None, codec="x264"):
    if dv.is_sequence(dst):
        for k in (1, 2, 3):
            open(dst % k, "w").write("partial")
    else:
        open(dst, "w").write("partial")
    return subprocess.Popen(["sh", "-c", "head -c 4000 > /dev/null; exit 3"], stdin=subprocess.PIPE)

if __name__ == "__main__":
    dv.encoder = fake
    sys.argv = ["denoise_video_v5.py"] + sys.argv[1:]
    dv.main()
'''


def main():
    os.makedirs(T, exist_ok=True)
    child = os.path.join(T, "encfail_child.py")
    open(child, "w").write(CHILD)
    src8 = os.path.join(HERE, "test", "t2_src8.mkv")
    srcf = os.path.join(HERE, "test", "syn_gbrpf32le.mkv")
    for name, src, dst, args in (("8-bit mp4", src8, os.path.join(T, "fail8.mp4"), []),
                                 ("10-bit mkv", os.path.join(HERE, "test", "t2_src10.mkv"),
                                  os.path.join(T, "fail10.mkv"), ["--depth", "10"]),
                                 ("EXR sequence", srcf, os.path.join(T, "failexr", "out_%05d.exr"), ["--depth", "32"])):
        r = subprocess.run([sys.executable, child, src, dst, "--frames", "6", "--jobs", "2", *args],
                           capture_output=True, text=True, cwd=os.path.join(HERE, "..", "..", "..", "..", "repro"))
        left = [n for n in os.listdir(os.path.dirname(dst))
                if n.startswith(os.path.basename(dst).split("%")[0].split(".")[0])] if os.path.isdir(os.path.dirname(dst)) else []
        last = r.stderr.strip().splitlines()[-1] if r.stderr.strip() else ""
        print(f"{name}: exit {r.returncode}, traceback {'Traceback' in r.stderr}, files left {left}, message: {last}")


if __name__ == "__main__":
    main()
