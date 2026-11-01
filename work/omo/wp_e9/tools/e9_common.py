"""Common definitions of the E9 / INT-4 comparison material (work/omo/wp_e9).

Colour (the definition of the reproduction's reader: reference/python/nvfilter.yuv_to_rgb, the RGB that
native nvc_yuv_to_rgb gives the profile search):
  - YCbCr is 8-bit limited range ("tv"), BT.709 (kr 0.2126, kb 0.0722; the clips are untagged 1080 lines, and
    nvfilter.matrix_from_tags picks BT.709 for an untagged stream of 720 lines and more).
  - y = (Y - 16)·255/219, cb = (Cb - 128)·255/224, cr = (Cr - 128)·255/224;
    R = y + 2(1 - kr)·cr, B = y + 2(1 - kb)·cb, G = (y - kr·R - kb·B)/(1 - kr - kb)   (0..255 scale)
  - chroma up-sampling by repetition: progressive 2x2 blocks; interlaced (layout tff / bff) within the field
    (a luma row r of field p = r % 2 takes the chroma row 2·((r // 2) // 2) + p, i.e. the chroma rows of the
    woven 4:2:0 frame alternate between the fields, as tinterlace weaves the planes).
  - the inverse (rgb_to_yuv444) is the exact inverse of the formulas above; 4:4:4 -> 4:2:0 is the 2x2 mean
    (interlaced: the mean of the 2 columns x the 2 rows of the same field), so that
    down420(rgb_to_yuv444(yuv_to_rgb(x))) == x before any clipping.
  - all arithmetic in float64; RGB is clipped to [0, 255] where the text says so.
16-bit RGB files: code = rint(clip(RGB, 0, 255)·257) (65535 / 255 = 257, so 8-bit integers map exactly; a host
reading code/65535 gets RGB/255 within 1/(2·65535)).
"""
import hashlib
import json
import os
import subprocess

import numpy as np

KR, KB = 0.2126, 0.0722
KG = 1.0 - KR - KB
W, H = 1920, 1080
# the noise: seed of numpy.random.default_rng([SEED_BASE, source id, clip frame k])
SEED_BASE = 0xE9
# fps: the rate the clips are labelled with; first: the original frame number of the media file's frame 0;
# start_p / start_i: the media frames the progressive / interlaced clips start from
SOURCES = {
    "park_joy": {"id": 1, "file": "park_joy_1080p50_f150.y4m", "fps": "25:1", "first": 0, "start_p": 0,
                 "start_i": 0, "credit": "Xiph derf SVT park_joy_1080p50 (first 150 frames)"},
    "old_town_cross": {"id": 2, "file": "old_town_cross_1080p50_f150.y4m", "fps": "25:1", "first": 0,
                       "start_p": 0, "start_i": 0,
                       "credit": "Xiph derf SVT old_town_cross_1080p50 (first 150 frames)"},
    # Sintel trailer frames 530..679 (byte range of y4m/sintel_trailer_2k_1080p24.y4m); shots of the trailer:
    # ..570 close-up, 571..620 rooftops, 621..657 dragon flight, 658..671 face, 672.. black.  Progressive
    # 571..630 (one cut at 621), interlaced 537..656 (the cuts 571 / 621 fall between woven frames)
    "sintel": {"id": 3, "file": "sintel_trailer_2k_1080p24_f530-679.y4m", "fps": "24:1", "first": 530,
               "start_p": 41, "start_i": 7,
               "credit": "Sintel (c) Blender Foundation | durian.blender.org, CC-BY 3.0; "
                         "Xiph derf sintel_trailer_2k_1080p24.y4m frames 530..679"},
}
# Y noise of an RGB noise of sigma s (independent per channel, before rounding): s·sqrt(kr²+kg²+kb²)·219/255
Y_GAIN = float(np.sqrt(KR ** 2 + KG ** 2 + KB ** 2) * 219.0 / 255.0)
FPS = "25:1"


# ---------------------------------------------------------------- y4m
def y4m_header(path):
    with open(path, "rb") as f:
        line = f.readline()
    if not line.startswith(b"YUV4MPEG2"):
        raise ValueError(f"{path}: not a y4m file")
    tok = line.decode().split()
    d = {t[0]: t[1:] for t in tok[1:]}
    return int(d["W"]), int(d["H"]), d, len(line)


def read_y4m(path, lo=0, hi=None):
    """Yields (k, (Y, Cb, Cr)) uint8 planes of frames lo..hi-1 (4:2:0 only)."""
    w, h, d, hl = y4m_header(path)
    if not d.get("C", "420jpeg").startswith("420"):
        raise ValueError(f"{path}: only 4:2:0 y4m is read (C{d.get('C')})")
    fsz = w * h * 3 // 2
    with open(path, "rb") as f:
        f.seek(hl)
        k = 0
        while hi is None or k < hi:
            fl = f.readline()
            if not fl:
                break
            if not fl.startswith(b"FRAME"):
                raise ValueError(f"{path}: bad frame header at frame {k}")
            buf = f.read(fsz)
            if len(buf) < fsz:
                break
            if k >= lo:
                a = np.frombuffer(buf, np.uint8)
                Y = a[:w * h].reshape(h, w)
                Cb = a[w * h:w * h * 5 // 4].reshape(h // 2, w // 2)
                Cr = a[w * h * 5 // 4:].reshape(h // 2, w // 2)
                yield k, (Y, Cb, Cr)
            k += 1


class Y4MWriter:
    def __init__(self, path, w, h, fps=FPS, interlace="p"):
        self.f = open(path, "wb")
        self.f.write(f"YUV4MPEG2 W{w} H{h} F{fps} I{interlace} A1:1\n".encode())

    def write(self, planes):
        self.f.write(b"FRAME\n")
        for p in planes:
            self.f.write(np.ascontiguousarray(p, np.uint8).tobytes())

    def close(self):
        self.f.close()


# ---------------------------------------------------------------- colour
def _up(c, layout):
    """4:2:0 chroma plane -> full size by repetition (layout p: 2x2 blocks; tff / bff: within the field)."""
    c = np.repeat(np.asarray(c), 2, axis=1)
    if layout == "p":
        return np.repeat(c, 2, axis=0)
    h2 = c.shape[0]
    r = np.arange(2 * h2)
    return c[2 * ((r // 2) // 2) + (r % 2)]


def down420(c, layout):
    """Full-size chroma (float) -> 4:2:0 by the 2x2 mean (tff / bff: rows of the same field)."""
    c = np.asarray(c, np.float64)
    c = 0.5 * (c[:, 0::2] + c[:, 1::2])
    if layout == "p":
        return 0.5 * (c[0::2] + c[1::2])
    h = c.shape[0]
    cr = np.arange(h // 2)            # chroma row cr: field p = cr % 2, field chroma row q = cr // 2
    p, q = cr % 2, cr // 2
    return 0.5 * (c[4 * q + p] + c[4 * q + 2 + p])


def yuv_to_rgb(planes, layout="p", clip=True):
    """(Y, Cb, Cr) 8-bit limited BT.709 4:2:0 -> RGB float64 (3, H, W), 0..255 scale."""
    Y, Cb, Cr = (np.asarray(p, np.float64) for p in planes)
    if Cb.shape != Y.shape:
        Cb, Cr = _up(Cb, layout), _up(Cr, layout)
    y = (Y - 16.0) * 255.0 / 219.0
    cb = (Cb - 128.0) * 255.0 / 224.0
    cr = (Cr - 128.0) * 255.0 / 224.0
    r = y + 2.0 * (1.0 - KR) * cr
    b = y + 2.0 * (1.0 - KB) * cb
    g = (y - KR * r - KB * b) / KG
    rgb = np.stack([r, g, b])
    return np.clip(rgb, 0.0, 255.0) if clip else rgb


def rgb_to_yuv444(rgb):
    """RGB float (3, H, W) 0..255 -> (Y, Cb, Cr) float64 8-bit limited BT.709 code values, 4:4:4."""
    r, g, b = (np.asarray(x, np.float64) for x in rgb)
    y = KR * r + KG * g + KB * b
    cb = (b - y) / (2.0 * (1.0 - KB))
    cr = (r - y) / (2.0 * (1.0 - KR))
    return 16.0 + y * 219.0 / 255.0, 128.0 + cb * 224.0 / 255.0, 128.0 + cr * 224.0 / 255.0


def rgb_to_yuv420(rgb, layout="p"):
    """RGB float -> (Y, Cb, Cr) float64 4:2:0 (not rounded)."""
    Y, Cb, Cr = rgb_to_yuv444(rgb)
    return Y, down420(Cb, layout), down420(Cr, layout)


def quant8(planes):
    return tuple(np.clip(np.rint(p), 0, 255).astype(np.uint8) for p in planes)


def rgb16(rgb):
    """RGB float (3, H, W) -> interleaved uint16 (H, W, 3), code = rint(clip(v)·257)."""
    return np.rint(np.clip(rgb, 0.0, 255.0) * 257.0).astype("<u2").transpose(1, 2, 0)


# ---------------------------------------------------------------- noise
def noise(src_id, k, shape=(3, H, W)):
    """Unit Gaussian RGB noise of clip frame k (float64), the same for every layout and level."""
    return np.random.default_rng([SEED_BASE, src_id, k]).standard_normal(shape)


def add_noise(clean_planes, sigma_rgb, src_id, k, layout):
    """Clean 4:2:0 frame -> noisy 4:2:0 uint8: RGB (unclipped) + N(0, sigma_rgb²) per channel, clip to
    [0, 255] (a sensor clips), back to YCbCr 4:2:0 (mean), rint to 8 bits."""
    rgb = yuv_to_rgb(clean_planes, layout, clip=False)
    rgb = np.clip(rgb + sigma_rgb * noise(src_id, k, rgb.shape), 0.0, 255.0)
    return quant8(rgb_to_yuv420(rgb, layout))


# ---------------------------------------------------------------- interlace
def weave(top_src, bot_src):
    """Planes of one woven frame: the even rows (luma and chroma) from top_src, the odd rows from bot_src
    (tinterlace interleave_top / interleave_bottom on each plane)."""
    out = []
    for a, b in zip(top_src, bot_src):
        o = np.empty_like(a)
        o[0::2] = a[0::2]
        o[1::2] = b[1::2]
        out.append(o)
    return tuple(out)


def field(planes, parity):
    """The field picture (rows of parity 0 = top, 1 = bottom) of a woven 4:2:0 frame: luma rows r % 2 == parity,
    chroma rows of the same parity (a 4:2:0 picture of half the height)."""
    return tuple(np.ascontiguousarray(p[parity::2]) for p in planes)


# ---------------------------------------------------------------- misc
def sha256(path, bufsize=1 << 22):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while True:
            b = f.read(bufsize)
            if not b:
                break
            h.update(b)
    return h.hexdigest()


def write_json(path, obj):
    with open(path, "w") as f:
        json.dump(obj, f, indent=1, ensure_ascii=False)
        f.write("\n")


def ffmpeg_tiff_writer(pattern, w=W, h=H, fps="25"):
    """A subprocess taking interleaved rgb48le frames on stdin and writing an uncompressed 16-bit RGB TIFF
    sequence pattern (%05d, numbered from 0)."""
    os.makedirs(os.path.dirname(pattern), exist_ok=True)
    return subprocess.Popen(["ffmpeg", "-nostdin", "-v", "error", "-y", "-f", "rawvideo", "-pix_fmt", "rgb48le",
                             "-s", f"{w}x{h}", "-r", fps, "-i", "-", "-c:v", "tiff", "-compression_algo", "raw",
                             "-pix_fmt", "rgb48le", "-start_number", "0", pattern], stdin=subprocess.PIPE)


def read_rgb48(src, w=W, h=H, start_number=None):
    """Yields RGB float64 (3, H, W) 0..255 frames of any ffmpeg-readable file or image sequence pattern,
    decoded to rgb48le (code / 257)."""
    cmd = ["ffmpeg", "-nostdin", "-v", "error"]
    if "%" in src:
        cmd += ["-f", "image2"] + (["-start_number", str(start_number)] if start_number is not None else [])
    cmd += ["-i", src, "-f", "rawvideo", "-pix_fmt", "rgb48le", "-"]
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    fsz = w * h * 6
    try:
        while True:
            buf = p.stdout.read(fsz)
            if len(buf) < fsz:
                break
            a = np.frombuffer(buf, "<u2").reshape(h, w, 3).transpose(2, 0, 1)
            yield a.astype(np.float64) / 257.0
    finally:
        p.stdout.close()
        p.wait()


def read_yuv420(src, w=W, h=H):
    """Yields (Y, Cb, Cr) uint8 of any ffmpeg-readable 8-bit 4:2:0 file (no conversion when it is yuv420p)."""
    if src.endswith(".y4m"):
        yield from (pl for _, pl in read_y4m(src))
        return
    p = subprocess.Popen(["ffmpeg", "-nostdin", "-v", "error", "-i", src, "-map", "0:v:0", "-f", "rawvideo",
                          "-pix_fmt", "yuv420p", "-"], stdout=subprocess.PIPE)
    fsz = w * h * 3 // 2
    try:
        while True:
            buf = p.stdout.read(fsz)
            if len(buf) < fsz:
                break
            a = np.frombuffer(buf, np.uint8)
            yield (a[:w * h].reshape(h, w), a[w * h:w * h * 5 // 4].reshape(h // 2, w // 2),
                   a[w * h * 5 // 4:].reshape(h // 2, w // 2))
    finally:
        p.stdout.close()
        p.wait()
