# Shared helpers of native/verify_spatial.py and native/verify_temporal.py (N1h / N1g):
# the case / result blob format of native/nv_v5_test (see nv_v5_main.cpp), real frames
# (yuv420p / yuv444p planes as float32, the way denoise_video_v5 hands them to the core),
# v5 profiles (nvfilter.build_profile_v5, cached as JSON) and the bit comparison.
import json
import os
import struct
import subprocess
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
REPRO = os.path.join(ROOT, "repro")
sys.path.insert(0, REPRO)
sys.dont_write_bytecode = True

import nvfilter as nv  # noqa: E402

F32 = np.float32
U32 = np.uint32
EXE = os.path.join(HERE, "nv_v5_test")
BANDS = nv.V5_BANDS
NANS = (0x7fc00000, 0xffc00000, 0x7fc12345, 0x7f800001, 0xff812345, 0x7fa00000)

KIND = {"spatial": 1, "spatial_mix": 4, "temporal": 2, "frame": 3, "gauss": 10, "uniform": 11, "mapc": 12,
        "median": 13, "curve": 14, "atrous": 15, "down2": 16, "flow": 17, "pyramid": 18,
        "exp": 19, "to_grid": 20, "dense": 21, "warp": 22, "blocksum": 23,
        "refine": 24}


def fbits(*u):
    return np.array(u, U32).view(F32)


# ------------------------------------------------------------------------------------- blobs
def _plane(a):
    a = np.ascontiguousarray(a, F32)
    assert a.ndim == 2
    return struct.pack("<ii", *a.shape) + a.tobytes()


def _profile(prof):
    n = len(prof["curve_centers"])
    b = struct.pack("<i", n) + struct.pack(f"<{n}d", *prof["curve_centers"])
    for nm in ("Y", "Cb", "Cr"):
        b += struct.pack(f"<{n}d", *prof["curve"][nm])
    b += struct.pack("<3i", *(prof["levels"][nm] for nm in ("Y", "Cb", "Cr")))
    for nm in ("Y", "Cb", "Cr"):
        r = list(prof["band_ratio"][nm])[:5]
        b += struct.pack("<5d", *(r + [0.0] * (5 - len(r))))
    b += struct.pack("<3d", *(prof["tot_ratio"][nm] for nm in ("Y", "Cb", "Cr")))
    return b


def _params(P):
    sim = {"v5": 0, "pyramid": 1}[P.get("similarity", "v5")]
    b = struct.pack("<4i", int(P["radius"]), int(P["bs"]), int(P["search"]), sim)
    b += struct.pack("<8d", P["temporal"], P["threshold"], P["spatial"], P["luma"], P["chroma"],
                     P["noise_level"], P.get("pyramid_scale", 1.5), P["mix"])
    b += struct.pack("<5d", *(P["band_amounts"][k] for k in BANDS))
    b += struct.pack("<5d", *(P["band_noise"][k] for k in BANDS))
    b += struct.pack("<3d", *(P["channel_noise"][k] for k in ("Y", "Cb", "Cr")))
    return b


DUMMY_PROF = {"curve_centers": [16.0 + 32 * i for i in range(8)], "curve": {n: [1.0] * 8 for n in "Y Cb Cr".split()},
              "levels": {"Y": 5, "Cb": 5, "Cr": 5}, "band_ratio": {n: [1.0] * 5 for n in "Y Cb Cr".split()},
              "tot_ratio": {"Y": 1.0, "Cb": 1.0, "Cr": 1.0}}


def encode_case(kind, prof=None, P=None, ints=(), dbls=(), planes=()):
    b = struct.pack("<I", KIND[kind]) + _profile(prof or DUMMY_PROF) + _params(P or nv.v5_params())
    b += struct.pack("<i", len(ints)) + struct.pack(f"<{len(ints)}i", *ints)
    b += struct.pack("<i", len(dbls)) + struct.pack(f"<{len(dbls)}d", *dbls)
    b += struct.pack("<i", len(planes)) + b"".join(_plane(p) for p in planes)
    return b


def write_cases(path, blobs):
    with open(path, "wb") as f:
        f.write(struct.pack("<I", len(blobs)))
        for b in blobs:
            f.write(b)


def read_results(path, n):
    data = open(path, "rb").read()
    off, out = 0, []
    for _ in range(n):
        (k,), off = struct.unpack_from("<I", data, off), off + 4
        planes = []
        for _ in range(k):
            H, W = struct.unpack_from("<ii", data, off)
            off += 8
            planes.append(np.frombuffer(data, F32, H * W, off).reshape(H, W))
            off += 4 * H * W
        out.append(planes)
    assert off == len(data), "trailing bytes in the results"
    return out


def run_exe(cases_path, results_path, jobs=1, exe=EXE):
    subprocess.run([exe, cases_path, results_path, "-j", str(jobs)], check=True)


def diff_count(exp, got):
    """Number of differing 32-bit words (shape mismatch counts every word)."""
    e = np.ascontiguousarray(exp, F32)
    g = np.ascontiguousarray(got, F32)
    if e.shape != g.shape:
        return max(e.size, g.size, 1)
    return int(np.count_nonzero(e.view(U32) != g.view(U32)))


# ------------------------------------------------------------------------------- real frames
VIDEOS = {"test_v02": os.path.join(ROOT, "test_v02.mp4"), "test": os.path.join(ROOT, "test.mp4")}
SIZES = {"test_v02": (1920, 1080), "test": (640, 360)}


def frame_planes(tmp, video, n, fmt="yuv420p"):
    """(Y, Cb, Cr) float32 of frame n (ffmpeg select on the decoded sequence), cached in `tmp`."""
    os.makedirs(tmp, exist_ok=True)
    path = os.path.join(tmp, f"{video}_{fmt}_{n}.npz")
    if os.path.exists(path):
        z = np.load(path)
        return tuple(z[k] for k in ("y", "cb", "cr"))
    W, H = SIZES[video]
    cw, ch = ((W + 1) // 2, (H + 1) // 2) if fmt == "yuv420p" else (W, H)
    fs = W * H + 2 * cw * ch
    buf = subprocess.run(["ffmpeg", "-v", "error", "-nostdin", "-noautorotate", "-i", VIDEOS[video],
                          "-vf", f"select=eq(n\\,{n})", "-frames:v", "1", "-fps_mode", "passthrough",
                          "-f", "rawvideo", "-pix_fmt", fmt, "-"], capture_output=True, check=True).stdout
    assert len(buf) == fs, (video, n, len(buf), fs)
    a = np.frombuffer(buf, np.uint8)
    y = a[:W * H].reshape(H, W).astype(F32)
    cb = a[W * H:W * H + cw * ch].reshape(ch, cw).astype(F32)
    cr = a[W * H + cw * ch:].reshape(ch, cw).astype(F32)
    np.savez(path, y=y, cb=cb, cr=cr)
    return y, cb, cr


def profile_for(tmp, video, n, fmt="yuv420p", mode="auto"):
    """nvfilter.build_profile_v5 of frame n (pair = frame n+1, FindFlatArea, BT.709 for 1080p),
    cached as JSON (Python floats, exactly as build_profile_v5 returns them)."""
    path = os.path.join(tmp, f"prof_{video}_{fmt}_{n}_{mode}.json")
    if os.path.exists(path):
        return json.load(open(path))
    p0 = frame_planes(tmp, video, n, fmt)
    p1 = frame_planes(tmp, video, n + 1, fmt)
    H = p0[0].shape[0]
    prof = nv.build_profile_v5(p0, False, None, pair=p1, mode=mode, matrix="bt709" if H >= 720 else "bt601")
    json.dump(prof, open(path, "w"))
    return json.load(open(path))
