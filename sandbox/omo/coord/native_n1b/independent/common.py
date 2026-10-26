# Independent verifier helpers (N1b review). No caches from the workers are used.
import os, subprocess, sys
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", "..", ".."))
REPRO = os.path.join(ROOT, "repro")
TMP = os.path.join(HERE, "tmp")
BIN = os.path.join(HERE, "bin")
sys.path.insert(0, REPRO)
sys.dont_write_bytecode = True
F32, U32 = np.float32, np.uint32

def probe(video):
    wh = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
                         "stream=width,height", "-of", "csv=p=0", os.path.join(ROOT, video)],
                        check=True, capture_output=True, text=True).stdout.strip().split(",")
    return int(wh[0]), int(wh[1])

def rgb_frame(video, n):
    os.makedirs(TMP, exist_ok=True)
    p = os.path.join(TMP, f"rgb_{os.path.splitext(video)[0]}_{n}.npy")
    if os.path.exists(p):
        return np.load(p)
    W, H = probe(video)
    raw = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-i", os.path.join(ROOT, video), "-vf",
                          f"select=eq(n\\,{n})", "-frames:v", "1", "-fps_mode", "passthrough", "-f", "rawvideo",
                          "-pix_fmt", "rgb48le", "-"], check=True, capture_output=True).stdout
    assert len(raw) == W * H * 6
    a = (np.frombuffer(raw, np.uint16).reshape(H, W, 3).astype(F32) / F32(257.0)).astype(F32)
    np.save(p, a)
    return a

def yuv_frame(video, n, fmt):
    os.makedirs(TMP, exist_ok=True)
    p = os.path.join(TMP, f"{fmt}_{os.path.splitext(video)[0]}_{n}.npz")
    if os.path.exists(p):
        z = np.load(p); return (z["y"], z["cb"], z["cr"])
    W, H = probe(video)
    cw, ch = {"yuv420p": ((W + 1) // 2, (H + 1) // 2), "yuv444p": (W, H), "yuv422p": ((W + 1) // 2, H)}[fmt]
    raw = subprocess.run(["ffmpeg", "-nostdin", "-v", "error", "-noautorotate", "-i", os.path.join(ROOT, video), "-vf",
                          f"select=eq(n\\,{n})", "-frames:v", "1", "-fps_mode", "passthrough", "-f", "rawvideo",
                          "-pix_fmt", fmt, "-"], check=True, capture_output=True).stdout
    assert len(raw) == W * H + 2 * cw * ch, (len(raw),)
    a = np.frombuffer(raw, np.uint8)
    y = a[:W * H].reshape(H, W).astype(F32)
    cb = a[W * H:W * H + cw * ch].reshape(ch, cw).astype(F32)
    cr = a[W * H + cw * ch:].reshape(ch, cw).astype(F32)
    np.savez(p, y=y, cb=cb, cr=cr)
    return y, cb, cr

def bits_equal(a, b):
    a = np.asarray(a); b = np.asarray(b)
    if a.dtype != np.float32 or b.dtype != np.float32:
        return False, f"dtype {a.dtype}/{b.dtype}"
    if a.shape != b.shape:
        return False, f"shape {a.shape}/{b.shape}"
    nd = int(np.count_nonzero(np.ascontiguousarray(a).view(U32) != np.ascontiguousarray(b).view(U32)))
    return nd == 0, f"{nd} words"
