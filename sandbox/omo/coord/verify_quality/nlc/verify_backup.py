"""backend.py after the change vs the copy before it (backup/backend.py.20261026_pre_nlc) at Temporal Noise
Level +0%: process_frame and PreviewEngine.process bit for bit on test.mp4 (several parameter sets).

    cd repro && python3 ../sandbox/omo/coord/verify_quality/nlc/verify_backup.py ../test.mp4
"""
import importlib.machinery
import importlib.util
import os
import sys

sys.path.insert(0, os.getcwd())
import numpy as np

from denoise_video_v5 import probe, read_frames
from nv_ui_qt import backend as new
from nv_ui_qt.state import backend_params, default_params

HERE = os.path.dirname(os.path.abspath(__file__))
BAK = os.path.join(HERE, "..", "..", "backup", "backend.py.20261026_pre_nlc")
loader = importlib.machinery.SourceFileLoader("backend_pre_nlc", BAK)
spec = importlib.util.spec_from_loader("backend_pre_nlc", loader)
old = importlib.util.module_from_spec(spec)
loader.exec_module(old)


def main(video):
    info = probe(video)
    frames = [tuple(np.array(p) for p in f) for _, f in read_frames(video, info["W"], info["H"], 295, 306, info["fps"])]
    prof = new.build_profile(frames, 5, False, "bt709", "auto")
    D = default_params()
    sets = {"default": D, "radius1_crband": dict(D, t_radius=1, bamt_Cr_medium=60.0),
            "mix_noise150": dict(D, mix_enable=True, mix=25.0, adv_noise_level=150.0),
            "temporal_off": dict(D, t_enable=False), "pyramid": dict(D, adv_similarity="pyramid")}
    n_ok = n = 0
    for name, ui in sets.items():
        P = backend_params(ui)
        assert P["t_noise"] == 1.0
        for t in (0, 5, 10):
            idx = [i for i in range(t - P["radius"], t + P["radius"] + 1) if 0 <= i < len(frames)]
            win = [tuple(p.astype(np.float32) for p in frames[i]) for i in idx]
            a = new.process_frame(win, idx.index(t), prof, P)
            b = old.process_frame(win, idx.index(t), prof, P)
            same = all(np.array_equal(x, y) for x, y in zip(a, b))
            ea = new.PreviewEngine(frames, "bt709", False).process(t, prof, 1, P)
            eb = old.PreviewEngine(frames, "bt709", False).process(t, prof, 1, P)
            same_e = all(np.array_equal(x, y) for x, y in zip(ea, eb))
            ca = new.PreviewEngine(frames, "bt709", False).process(t, prof, 1, P, (40, 64, 96, 128))
            cb = old.PreviewEngine(frames, "bt709", False).process(t, prof, 1, P, (40, 64, 96, 128))
            same_c = all(np.array_equal(x, y) for x, y in zip(ca, cb))
            n += 1
            n_ok += same and same_e and same_c
            print(f"{name:16s} t={t:2d} process_frame {same} engine {same_e} crop {same_c}")
    # t_noise != 1 differs from the old code (the setting has an effect)
    P = backend_params(dict(D, t_noise=60.0))
    idx = list(range(3, 8))
    win = [tuple(p.astype(np.float32) for p in frames[i]) for i in idx]
    diff = not all(np.array_equal(x, y) for x, y in zip(new.process_frame(win, 2, prof, P),
                                                         old.process_frame(win, 2, prof, P)))
    print(f"{n_ok}/{n} identical at +0%; -40% differs from the old code: {diff}")
    return 0 if n_ok == n and diff else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
