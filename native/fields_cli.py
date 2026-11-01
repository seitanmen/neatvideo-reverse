# fields_cli - INT-2 test CLI: an interlaced clip processed field by field by the C++ port (nv_v5_test kind 31 =
# nvfp::process_frame_fields) with a v5 profile of one field (nvfilter.build_profile_v5, Python).  Test tool only: the
# UI and the export are INT-3 (no audio, no colour tags, no high bit depth; yuv420p 8-bit in, FFV1 out).
#
#   python3 native/fields_cli.py IN OUT.mkv [--order auto|tff|bff] [--mode same|shift|plain|frame] [--radius R]
#                                [--start S] [--frames N] [--profile-frame K] [--jobs J]
#
# --order auto (default) decides the first field in time as qtui does (video.cpp scan_of / first_field, INT-1 L-1):
# the first decoded frame's interlaced_frame / top_field_first, else the stream's field_order tt / bb.  tb / bt alone
# never give the order (ffmpeg 8.0.1 writes tb for top-field-first ffv1 / prores).  A progressive clip, or an
# interlaced one without a known order, is refused: give --order tff / bff (or --mode frame for the frame path).
# A source that is not yuv420p is converted with swscale's interlaced mode (scale=interl=1: every field's chroma on
# its own).  The output is FFV1 with the same field order flags (setfield).
import argparse
import json
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.dont_write_bytecode = True
import v5_cases as vc  # noqa: E402
import fields_ref as fr  # noqa: E402
from v5_cases import F32, nv, np  # noqa: E402

vc.KIND.update({"tshift": 30, "fields": 31})


def probe(path):
    s = json.loads(subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries",
                                   "stream=width,height,field_order,pix_fmt", "-of", "json", path],
                                  capture_output=True, text=True, check=True).stdout)["streams"][0]
    fr_ = json.loads(subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-read_intervals", "%+#1",
                                     "-show_entries", "frame=interlaced_frame,top_field_first", "-of", "json", path],
                                    capture_output=True, text=True, check=True).stdout).get("frames", [])
    f0 = fr_[0] if fr_ else {}
    return s, f0.get("interlaced_frame", -1), f0.get("top_field_first", -1)


def first_field(fo, fi, tff):
    """qtui first_field(scan_of(...)): 0 top, 1 bottom, -1 progressive / unknown / interlaced without an order"""
    if fi == 1:
        return 0 if tff == 1 else 1 if tff == 0 else -1
    return {"tt": 0, "bb": 1}.get(fo, -1)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("inp")
    ap.add_argument("out")
    ap.add_argument("--order", choices=["auto", "tff", "bff"], default="auto")
    ap.add_argument("--mode", choices=["same", "shift", "plain", "frame"], default="same")
    ap.add_argument("--radius", type=int, default=2)
    ap.add_argument("--start", type=int, default=0)
    ap.add_argument("--frames", type=int, default=0, help="0 = to the end")
    ap.add_argument("--profile-frame", type=int, default=0, help="index inside the range")
    ap.add_argument("--jobs", type=int, default=2)
    a = ap.parse_args()
    s, fi, tff = probe(a.inp)
    W, H, fo = s["width"], s["height"], s.get("field_order", "")
    first = {"tff": 0, "bff": 1}.get(a.order, first_field(fo, fi, tff))
    print(f"{a.inp}: {W}x{H} {s.get('pix_fmt')}, stream field_order {fo or '(none)'}, first frame interlaced_frame {fi} "
          f"top_field_first {tff} -> first field {('top', 'bottom')[first] if first >= 0 else 'unknown'}"
          f"{' (--order)' if a.order != 'auto' else ''}")
    if a.mode != "frame" and first < 0:
        print("the field order is not known (progressive, or interlaced without an order): give --order tff / bff")
        return 2
    if H % 2 or W % 2:
        print("odd frame sizes are not handled by this test CLI (yuv420p)")
        return 2
    vf = [f"select=gte(n\\,{a.start})"] + (["scale=interl=1"] if s.get("pix_fmt") != "yuv420p" else [])
    cmd = ["ffmpeg", "-v", "error", "-nostdin", "-noautorotate", "-i", a.inp, "-vf", ",".join(vf)]
    if a.frames:
        cmd += ["-frames:v", str(a.frames)]
    raw = subprocess.run(cmd + ["-fps_mode", "passthrough", "-f", "rawvideo", "-pix_fmt", "yuv420p", "-"],
                         capture_output=True, check=True).stdout
    fs = W * H * 3 // 2
    n = len(raw) // fs
    buf = np.frombuffer(raw, np.uint8)[:n * fs].reshape(n, fs)
    frames = [(buf[i, :W * H].reshape(H, W).astype(F32),
               buf[i, W * H:W * H * 5 // 4].reshape(H // 2, W // 2).astype(F32),
               buf[i, W * H * 5 // 4:].reshape(H // 2, W // 2).astype(F32)) for i in range(n)]
    k = min(max(a.profile_frame, 0), max(n - 2, 0))
    matrix = "bt709" if H >= 720 else "bt601"
    P = nv.v5_params(radius=a.radius)
    if a.mode == "frame":
        prof = nv.build_profile_v5(frames[k], False, None, pair=frames[k + 1] if n > 1 else None, matrix=matrix)
    else:
        fields, _ = fr.field_sequence(frames, first)
        prof = nv.build_profile_v5(fields[2 * k], False, None, pair=fields[2 * k + 2] if n > 1 else None,
                                   matrix=matrix)
    mode = fr.MODES.get(a.mode)
    cases = []
    for i in range(n):
        if a.mode == "frame":
            lo, hi = max(0, i - a.radius), min(n, i + a.radius + 1)
            win = frames[lo:hi]
            cases.append(vc.encode_case("frame", prof, P, [i - lo, len(win), 0], (), [p for f in win for p in f]))
        else:
            m = a.radius // 2 + 1
            lo, hi = max(0, i - m), min(n, i + m + 1)
            win = frames[lo:hi]
            cases.append(vc.encode_case("fields", prof, P, [i - lo, len(win), first, mode], (),
                                        [p for f in win for p in f]))
    with tempfile.TemporaryDirectory() as tmp:
        inp, outp = os.path.join(tmp, "in.bin"), os.path.join(tmp, "out.bin")
        vc.write_cases(inp, cases)
        vc.run_exe(inp, outp, a.jobs)
        outs = vc.read_results(outp, n)
    data = b"".join(np.clip(np.rint(p), 0, 255).astype(np.uint8).tobytes() for o in outs for p in o[:3])
    sf = [] if a.mode == "frame" and first < 0 else [f"setfield={('tff', 'bff')[first]}"]
    enc = ["ffmpeg", "-y", "-v", "error", "-nostdin", "-f", "rawvideo", "-pix_fmt", "yuv420p", "-s", f"{W}x{H}", "-i", "-"]
    subprocess.run(enc + (["-vf", sf[0]] if sf else []) + ["-c:v", "ffv1", a.out], input=data, check=True)
    print(f"{n} frames, mode {a.mode}, radius {a.radius} ({'frames' if a.mode == 'frame' else 'fields'}) -> {a.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
