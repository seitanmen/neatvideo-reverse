import sys, os, subprocess, numpy as np, json
sys.path.insert(0, "/Users/seitan/Desktop/work/reverse/repro")
if __name__ == "__main__":
    from nv_ui_qt import backend
    from nv_ui_qt.ofx_bridge import unpack_yuv420p
    W, H = 192, 108
    raw = subprocess.run("ffmpeg -nostdin -v error -i /Users/seitan/Desktop/work/reverse/test.mp4 -vf scale=192:108 -pix_fmt yuv420p -frames:v 6 -f rawvideo -", shell=True, capture_output=True).stdout
    fs = W*H + 2*(W//2)*(H//2)
    frames = [unpack_yuv420p(raw[i*fs:(i+1)*fs], W, H) for i in range(6)]
    base = backend.build_profile(frames, 1, False, "bt709", "auto")
    def walk(o, path=""):
        if isinstance(o, dict):
            for k, v in o.items(): walk(v, f"{path}.{k}")
        elif isinstance(o, (list, tuple)):
            for i, v in enumerate(o[:2]): walk(v, f"{path}[{i}]")
        else:
            if not isinstance(o, (int, float, str, bool, type(None))): print("NONJSON", path, type(o), getattr(o, 'shape', None))
    walk(base)
    print("top-level keys/types:", {k: type(v).__name__ for k, v in base.items()})
    try:
        s = json.dumps(base, sort_keys=True, default=str); print("json ok len", len(s))
    except Exception as e: print("json err", e)
    # same params dict
    from nv_ui_qt import state
    P = state.backend_params(state.default_params() if hasattr(state, "default_params") else {})
