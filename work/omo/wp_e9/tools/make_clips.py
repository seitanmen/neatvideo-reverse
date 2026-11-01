"""Makes the E9 / INT-4 test material of one source (definitions in e9_common.py).

    python3 tools/make_clips.py --src park_joy [--frames 60] [--levels y3,y6] [--layouts p,tff,bff]
                                [--tiff p:y3,tff:y3,bff:y3,p:y6,tff:y6]

Writes clips/<src>/ (paths relative to work/omo/wp_e9):
  clean_p.y4m                  source frames start_p..start_p+N-1 as they are (labelled with the source's fps,
                               e9_common.SOURCES: the 50p sources as 25 fps, Sintel 24 fps)
  clean_tff.y4m / clean_bff.y4m  interlaced, woven from source frames s..s+2N-1 (s = start_i): frame k = the even rows
                               of s+2k and the odd rows of s+2k+1 (tff; tinterlace interleave_top) or the odd rows of
                               s+2k and the even rows of s+2k+1 (bff; interleave_bottom), on every plane
  clean_<tff|bff>_fields.y4m   the per-field truth: 2N field pictures (1920x540 4:2:0, 2 x fps) in time order
  noisy_<layout>_<level>.y4m   clean + Gaussian RGB noise (e9_common.add_noise; level yS = the RGB sigma that gives
                               a Y sigma of S code values before rounding), 8-bit 4:2:0: the reproduction's input
  tiff/<layout>_<level>/<src>_<layout>_<level>_%05d.tif   the same frames as uncompressed 16-bit RGB TIFF
                               (e9_common.yuv_to_rgb, clipped, code·257): the genuine's input in DaVinci Resolve
  manifest.json                definitions, field map, SHA256 of every file (a TIFF set: of the concatenated files)
"""
import argparse
import glob
import hashlib
import os
import sys
import time

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import e9_common as C  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))


def seq_sha(files):
    h = hashlib.sha256()
    for f in files:
        with open(f, "rb") as fh:
            h.update(fh.read())
    return h.hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", required=True, choices=sorted(C.SOURCES))
    ap.add_argument("--frames", type=int, default=60)
    ap.add_argument("--levels", default="y3,y6")
    ap.add_argument("--layouts", default="p,tff,bff")
    ap.add_argument("--tiff", default="p:y3,tff:y3,bff:y3,p:y6,tff:y6")
    a = ap.parse_args()
    N = a.frames
    S = C.SOURCES[a.src]
    src = os.path.join(ROOT, "media", S["file"])
    out = os.path.join(ROOT, "clips", a.src)
    os.makedirs(out, exist_ok=True)
    levels = [s for s in a.levels.split(",") if s]
    layouts = [s for s in a.layouts.split(",") if s]
    tiffs = {tuple(s.split(":")) for s in a.tiff.split(",") if s}
    sig = {lv: float(lv[1:]) / C.Y_GAIN for lv in levels}
    sp, si = S["start_p"], S["start_i"]
    need = max(sp + N if "p" in layouts else 0, si + 2 * N if any(l != "p" for l in layouts) else 0)
    fps = S["fps"]
    fps2 = f"{2 * int(fps.split(':')[0])}:1"
    t0 = time.time()
    allf = [tuple(np.array(p) for p in pl) for _, pl in C.read_y4m(src, 0, need)]
    if len(allf) < need:
        raise SystemExit(f"{src}: {len(allf)} frames, {need} needed")
    srcf = allf[si:]
    files = {}
    fieldmap = {}
    for lay in layouts:
        if lay == "p":
            clean = allf[sp:sp + N]
        elif lay == "tff":
            clean = [C.weave(srcf[2 * k], srcf[2 * k + 1]) for k in range(N)]
            fieldmap["tff"] = [{"frame": k, "top": si + 2 * k, "bottom": si + 2 * k + 1, "first": "top"}
                               for k in range(N)]
        elif lay == "bff":
            clean = [C.weave(srcf[2 * k + 1], srcf[2 * k]) for k in range(N)]
            fieldmap["bff"] = [{"frame": k, "top": si + 2 * k + 1, "bottom": si + 2 * k, "first": "bottom"}
                               for k in range(N)]
        else:
            raise SystemExit(f"unknown layout {lay}")
        il = {"p": "p", "tff": "t", "bff": "b"}[lay]
        path = os.path.join(out, f"clean_{lay}.y4m")
        w = C.Y4MWriter(path, C.W, C.H, fps=fps, interlace=il)
        for pl in clean:
            w.write(pl)
        w.close()
        files[os.path.basename(path)] = path
        if lay != "p":
            path = os.path.join(out, f"clean_{lay}_fields.y4m")
            w = C.Y4MWriter(path, C.W, C.H // 2, fps=fps2, interlace="p")
            order = (0, 1) if lay == "tff" else (1, 0)
            for pl in clean:
                for par in order:
                    w.write(C.field(pl, par))
            w.close()
            files[os.path.basename(path)] = path
        for lv in levels:
            path = os.path.join(out, f"noisy_{lay}_{lv}.y4m")
            w = C.Y4MWriter(path, C.W, C.H, fps=fps, interlace=il)
            tw = None
            if (lay, lv) in tiffs:
                tdir = os.path.join(out, "tiff", f"{lay}_{lv}")
                for f in glob.glob(os.path.join(tdir, "*.tif")):
                    os.remove(f)
                pattern = os.path.join(tdir, f"{a.src}_{lay}_{lv}_%05d.tif")
                tw = C.ffmpeg_tiff_writer(pattern, fps=fps.split(":")[0])
            first16 = None
            for k, pl in enumerate(clean):
                nz = C.add_noise(pl, sig[lv], S["id"], k, lay)
                w.write(nz)
                if tw:
                    t16 = C.rgb16(C.yuv_to_rgb(nz, lay))
                    if k == 0:
                        first16 = t16
                    tw.stdin.write(t16.tobytes())
            w.close()
            files[os.path.basename(path)] = path
            if tw:
                tw.stdin.close()
                if tw.wait() != 0:
                    raise SystemExit("ffmpeg (tiff) failed")
                tif = sorted(glob.glob(os.path.join(tdir, "*.tif")))
                if len(tif) != N:
                    raise SystemExit(f"{tdir}: {len(tif)} TIFF files, {N} expected")
                back = next(C.read_rgb48(tif[0]))
                code = np.rint(back * 257.0).astype("<u2").transpose(1, 2, 0)
                if not np.array_equal(code, first16):
                    raise SystemExit(f"{tif[0]}: the TIFF does not read back to the written codes")
                files[f"tiff/{lay}_{lv}"] = tif
            print(f"{a.src} {lay} {lv}: done ({time.time() - t0:.0f} s)", flush=True)
    man = {
        "source": {"file": "media/" + S["file"], "sha256": C.sha256(src), "credit": S["credit"],
                   "original_frames": {"progressive": [S["first"] + sp, S["first"] + sp + N - 1],
                                       "interlaced": [S["first"] + si, S["first"] + si + 2 * N - 1]},
                   "media_frames": {"progressive": [sp, sp + N - 1], "interlaced": [si, si + 2 * N - 1]},
                   "field_map_note": "fields[].top / bottom are media frame numbers"},
        "frames": N,
        "fps": {"progressive": f"{fps} (each source frame is one frame)",
                "interlaced": f"{fps} frames, {fps2} fields/s (two consecutive source frames per woven frame)",
                "fields_files": fps2},
        "colour": "8-bit limited range BT.709 4:2:0, untagged (as the source); RGB = e9_common.yuv_to_rgb "
                  "(nvfilter.yuv_to_rgb: chroma repetition, interlaced within the field), 0..255 float64",
        "noise": {"space": "RGB (independent per channel, before the 4:2:0 conversion)", "seed":
                  f"numpy.random.default_rng([{C.SEED_BASE}, {S['id']}, k]).standard_normal((3, 1080, 1920)), k = clip "
                  "frame; the same unit noise for every layout and level",
                  "levels": {lv: {"sigma_rgb": sig[lv], "sigma_y_nominal": float(lv[1:])} for lv in levels},
                  "pipeline": "clip(RGB_unclipped(clean) + sigma*n, 0, 255) -> YCbCr (exact inverse) -> 4:2:0 mean "
                              "(interlaced: within the field) -> rint, 8-bit"},
        "tiff": "uncompressed 16-bit RGB (rgb48le), code = rint(clip(yuv_to_rgb(noisy), 0, 255)*257), numbered from 0",
        "fields": fieldmap,
        "files": {},
    }
    for name, p in files.items():
        if isinstance(p, list):
            man["files"][name] = {"count": len(p), "bytes": sum(os.path.getsize(f) for f in p),
                                  "sha256_concat": seq_sha(p), "first": os.path.basename(p[0])}
        else:
            man["files"][name] = {"bytes": os.path.getsize(p), "sha256": C.sha256(p)}
    C.write_json(os.path.join(out, "manifest.json"), man)
    print(f"{a.src}: manifest written ({time.time() - t0:.0f} s)")


if __name__ == "__main__":
    main()
