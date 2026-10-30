import os, sys
sys.path.insert(0, os.getcwd())
import nv_depth as nd, denoise_video_v5 as dv
orig = nd.from_internal
def spy(planes, fmt, matrix="bt709", restore=None):
    import numpy as np
    print("from_internal restore:", None if restore is None else {k: len(v[0]) for k, v in restore.items()},
          "finite:", [bool(np.isfinite(p).all()) for p in planes], "C-contig:", [p.flags.c_contiguous for p in planes], "F-contig:", [p.flags.f_contiguous for p in planes], "dtype", [str(np.asarray(p).dtype) for p in planes], flush=True)
    return orig(planes, fmt, matrix, restore)
if __name__ == "__main__":
    nd.from_internal = spy
    sys.argv = ["dv"] + sys.argv[1:]
    dv.main()
