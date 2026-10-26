import os, sys
sys.path.insert(0, os.getcwd())
import nv_depth as nd, denoise_video_v5 as dv
orig = nd.from_internal
D = os.path.dirname(os.path.abspath(__file__))
n = [0]
def spy(planes, fmt, matrix="bt709", restore=None):
    b = orig(planes, fmt, matrix, restore)
    open(os.path.join(D, f"dump_{n[0]}.raw"), "wb").write(b); n[0] += 1
    return b
if __name__ == "__main__":
    nd.from_internal = spy
    sys.argv = ["dv"] + sys.argv[1:]
    dv.main()
