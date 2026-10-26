
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
