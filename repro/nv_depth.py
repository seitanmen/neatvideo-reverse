"""Pixel formats for high-bit-depth input / output (denoise_video_v5.py, nv_ui_qt, OFX bridge).

The v5 core (nvfilter.py) works on 8-bit-equivalent float32 planes 0..255 (noise-curve bins
CURVE_BINS, clip test 254.5, black level 16, thresholds). High-bit-depth frames are normalised to
that scale when read and returned to their own depth when written, so neither the core nor the
noise profiles depend on the depth (a profile made at one depth can be used at another):
  integer depth b : internal = v / 2^(b-8) (a power-of-two division, exact in float32 up to 16 bits);
                    back = clip(rint(x * 2^(b-8)), 0, 2^b - 1), so unprocessed pixels (Mix 100%)
                    return exactly
  float (RGB)     : internal = v * 255, back = x / 255 with neither clip nor rounding (HDR values
                    above 1.0 and negative values are kept)
  8-bit yuv420p   : the uint8 planes as read (the legacy path, unchanged)
Approximation: full-range high-bit white (2^b - 1, e.g. 1023 at 10 bits) becomes 255.75, not 255 as
the ITU full-range scale (2^b - 1) would give. The round trip stays exact; only the clip-test
boundary (254.5) moves slightly.

RGB formats (gbrp*; ffmpeg's plane order is G, B, R) are processed as full-range YCbCr 4:4:4 with
the coefficients of nvfilter._KRB: the exact inverse of nvfilter.yuv_to_rgb(planes, matrix,
full_range=True), which FindFlatArea and the display use. 4:2:2 chroma is widened to 4:4:4 for
processing (even columns = the co-sited samples, odd columns = mean of the left and right samples,
the last column repeated) and the even columns are taken back when writing.

Non-finite float input (NaN, +-inf; sanitize): the filters mix neighbouring pixels and the noise
statistics average whole frames, so a single NaN or inf would spread over the whole output. Such
pixels are given finite values for processing (NaN -> 0, +inf -> the largest finite value of that
plane in that frame, -inf -> the smallest; 0 if the plane has none) and their original values are
put back into the same pixels of the same planes when writing (from_internal(..., restore)).
"""
import re
from collections import namedtuple
from dataclasses import dataclass

import numpy as np

from nvfilter import _KRB

SrcFmt = namedtuple("SrcFmt", "family bits is_float chroma")
_CHROMA = {"420": "420", "422": "422", "444": "444", "440": "444", "411": "444", "410": "444"}


@dataclass(frozen=True)
class PixFmt:
    """A raw-video format the pipeline reads and writes (ffmpeg rawvideo pix_fmt `name`)."""
    name: str
    family: str          # "yuv" | "rgb"
    bits: int            # 8 / 10 / 12 / 16 / 32
    is_float: bool
    chroma: str          # "420" | "422" | "444"
    dtype: object        # np.uint8 / "<u2" / "<f4"

    @property
    def legacy(self):
        """The 8-bit yuv420p path of the original pipeline (uint8 planes, no conversion)."""
        return self.name == "yuv420p"

    def plane_shapes(self, W, H):
        """(rows, cols) of the three planes in ffmpeg's order (yuv: Y, U, V; rgb: G, B, R)."""
        c = ((H + 1) // 2, (W + 1) // 2) if self.chroma == "420" else \
            (H, (W + 1) // 2) if self.chroma == "422" else (H, W)
        return [(H, W), c, c]

    def frame_bytes(self, W, H):
        """Bytes of one raw frame."""
        return sum(h * w for h, w in self.plane_shapes(W, H)) * np.dtype(self.dtype).itemsize

    def internal_bytes(self, W, H):
        """Bytes of one frame as to_internal returns it (uint8 for the legacy path, else float32
        with 4:2:2 chroma widened to the full width)."""
        if self.legacy:
            return self.frame_bytes(W, H)
        c = ((H + 1) // 2) * ((W + 1) // 2) if self.chroma == "420" else H * W
        return 4 * (H * W + 2 * c)


def pixfmt(name):
    """PixFmt of a processing format: yuv420p, yuv{420,422,444}p{10,12,16}le, gbrp{10,12,16}le,
    gbrpf32le."""
    m = re.fullmatch(r"yuv(420|422|444)p(?:(10|12|16)le)?", name)
    if m:
        bits = int(m.group(2) or 8)
        if bits == 8 and m.group(1) != "420":
            raise ValueError(f"unsupported processing format {name} (8 bits is always yuv420p)")
        return PixFmt(name, "yuv", bits, False, m.group(1), np.uint8 if bits == 8 else "<u2")
    m = re.fullmatch(r"gbrp(10|12|16)le", name)
    if m:
        return PixFmt(name, "rgb", int(m.group(1)), False, "444", "<u2")
    if name == "gbrpf32le":
        return PixFmt(name, "rgb", 32, True, "444", "<f4")
    raise ValueError(f"unsupported processing format {name}")


# packed / semi-planar formats: (family, bits, chroma)
_FIXED = {"nv12": ("yuv", 8, "420"), "nv21": ("yuv", 8, "420"), "nv16": ("yuv", 8, "422"),
          "nv24": ("yuv", 8, "444"), "nv42": ("yuv", 8, "444"), "nv20": ("yuv", 10, "422"),
          "p010": ("yuv", 10, "420"), "p012": ("yuv", 12, "420"), "p016": ("yuv", 16, "420"),
          "p210": ("yuv", 10, "422"), "p212": ("yuv", 12, "422"), "p216": ("yuv", 16, "422"),
          "p410": ("yuv", 10, "444"), "p412": ("yuv", 12, "444"), "p416": ("yuv", 16, "444"),
          "yuyv422": ("yuv", 8, "422"), "uyvy422": ("yuv", 8, "422"), "yvyu422": ("yuv", 8, "422"),
          "uyyvyy411": ("yuv", 8, "444"), "y210": ("yuv", 10, "422"), "y212": ("yuv", 12, "422"),
          "y216": ("yuv", 16, "422"), "v210": ("yuv", 10, "422"), "y410": ("yuv", 10, "444"),
          "y412": ("yuv", 12, "444"), "xv30": ("yuv", 10, "444"), "v30x": ("yuv", 10, "444"),
          "xv36": ("yuv", 12, "444"), "xv48": ("yuv", 16, "444"), "ayuv64": ("yuv", 16, "444"),
          "ayuv": ("yuv", 8, "444"), "vuya": ("yuv", 8, "444"), "vuyx": ("yuv", 8, "444"),
          "uyva": ("yuv", 8, "444"), "v408": ("yuv", 8, "444"), "ya8": ("yuv", 8, "420"),
          "ya16": ("yuv", 16, "420"), "monow": ("yuv", 8, "420"), "monob": ("yuv", 8, "420"),
          "pal8": ("rgb", 8, "444"), "rgb4_byte": ("rgb", 8, "444"), "bgr4_byte": ("rgb", 8, "444"), "x2rgb10": ("rgb", 10, "444"), "x2bgr10": ("rgb", 10, "444"),
          "xyz12": ("rgb", 12, "444")}


def source_format(name):
    """(family, bits, is_float, chroma) of a source from its ffprobe pix_fmt name. Grey formats
    count as yuv 4:2:0 (no chroma to keep); formats below 8 bits as 8; unknown names (hardware
    surfaces) as 8-bit yuv 4:2:0."""
    # ffmpeg 8 "msb" formats (yuv444p10msble, gbrp12msbbe, ...) hold the samples in the high bits of
    # 16-bit words; the depth and chroma format are those of the name without the suffix
    n = re.sub(r"msb$", "", re.sub(r"(le|be)$", "", (name or "").lower()))
    if n in _FIXED:
        fam, bits, ch = _FIXED[n]
        return SrcFmt(fam, bits, False, ch)
    m = re.search(r"f(16|32)$", n)
    if m and re.match(r"(gbra?p|rgba?|bgra?|gray|ya|yuva?4\d\dp)", n):
        fam = "yuv" if n.startswith(("gray", "ya", "yuv")) else "rgb"
        ch = re.match(r"yuva?(4\d\d)", n)
        return SrcFmt(fam, int(m.group(1)), True, _CHROMA.get(ch.group(1), "444") if ch else
                      ("420" if fam == "yuv" else "444"))
    m = re.fullmatch(r"yuvj?a?(4\d\d)p(\d+)?", n)
    if m:
        return SrcFmt("yuv", int(m.group(2) or 8), False, _CHROMA.get(m.group(1), "444"))
    m = re.fullmatch(r"gray(\d+)?", n)
    if m:
        return SrcFmt("yuv", int(m.group(1) or 8), False, "420")
    m = re.fullmatch(r"gbra?p(\d+)?", n)
    if m:
        return SrcFmt("rgb", int(m.group(1) or 8), False, "444")
    m = re.fullmatch(r"(?:rgba?|bgra?|argb|abgr|0rgb|rgb0|0bgr|bgr0)(\d+)?", n)
    if m:
        bits = {"48": 16, "64": 16, "96": 32, "128": 32}.get(m.group(1), 8)     # rgb96 / rgba128: 32-bit integers
        return SrcFmt("rgb", bits, False, "444")
    m = re.fullmatch(r"bayer_\w+?(16)?", n)
    if m:
        return SrcFmt("rgb", 16 if m.group(1) else 8, False, "444")
    return SrcFmt("yuv", 8, False, "420")


def choose_format(src, depth=8):
    """Processing PixFmt for a source (source_format result) and a depth setting
    8 (default: always yuv420p, the legacy path) | 10 | 12 | 16 | 32 | "source".
    10/12/16: yuv sources keep their chroma format (4:4:0, 4:1:1, 4:1:0 are read as 4:4:4), RGB
    sources (and float RGB read at an integer depth) become gbrp. 32: always gbrpf32le.
    "source": the source's depth rounded up to 8 / 10 / 12 / 16, float or > 16 bits -> 32."""
    d = str(depth)
    if d == "source":
        b = src.bits
        d = "32" if src.is_float or b > 16 else "8" if b <= 8 else "10" if b <= 10 else "12" if b <= 12 else "16"
    if d == "8":
        return pixfmt("yuv420p")
    if d == "32":
        return pixfmt("gbrpf32le")
    if d in ("10", "12", "16"):
        return pixfmt(f"gbrp{d}le" if src.family == "rgb" else f"yuv{src.chroma}p{d}le")
    raise ValueError(f"unknown depth {depth!r} (8, 10, 12, 16, 32 or source)")


def full_range_internal(info, fmt):
    """Range of the internal planes: RGB formats become full-range YCbCr, yuv keeps the stream's."""
    return True if fmt.family == "rgb" else bool(info.get("full_range"))


def describe(fmt):
    """Short label, e.g. 'yuv420p10le · 10-bit 4:2:0' or 'gbrpf32le · 32-bit float RGB'."""
    kind = f"{fmt.bits}-bit" + (" float" if fmt.is_float else "")
    return f"{fmt.name} · {kind} " + ("RGB" if fmt.family == "rgb" else ":".join(fmt.chroma))


def describe_source(name):
    """e.g. 'yuv420p10le (10-bit 4:2:0)' from an ffprobe pix_fmt name."""
    s = source_format(name)
    kind = f"{s.bits}-bit" + (" float" if s.is_float else "")
    return f"{name or 'unknown'} ({kind} " + ("RGB" if s.family == "rgb" else ":".join(s.chroma)) + ")"


def read_filter(info, fmt, matrix):
    """Filter for reading a yuv source as RGB (depth 32 / gbrp): swscale converts with the
    reproduction's matrix and the stream's range (it would assume BT.601 for an untagged stream).
    The conversion is swscale's, not the exact BT.709 formula: for a 12/16-bit yuv 4:4:4 source read
    as gbrpf32le the independent verifier measured up to 0.0038 (0..1 scale) from the formula (a gain
    of about 0.99615 per plane), so to_internal gives back the stream's YCbCr only approximately.
    None when no conversion is involved."""
    if fmt.family != "rgb" or source_format(info.get("pix_fmt")).family == "rgb":
        return None
    return f"scale=in_color_matrix={matrix}:in_range={'pc' if info.get('full_range') else 'tv'}"


def split(buf, fmt, W, H):
    """Raw frame bytes -> the three planes (ffmpeg order) as read-only arrays of fmt.dtype."""
    a = np.frombuffer(buf, fmt.dtype)
    out, o = [], 0
    for h, w in fmt.plane_shapes(W, H):
        out.append(a[o:o + h * w].reshape(h, w))
        o += h * w
    return out


def sanitize(raw):
    """Float planes -> (planes with finite values, restore info or None). restore = {plane index:
    (flat indices, original values)} for the planes that had NaN / +-inf (module docstring)."""
    out, info = [], {}
    for k, p in enumerate(raw):
        p = np.asarray(p)
        bad = ~np.isfinite(p)
        if not bad.any():
            out.append(p)
            continue
        fin = p[~bad]
        top, low = (fin.max(), fin.min()) if fin.size else (0, 0)
        v = p[bad]
        q = p.copy()
        q[bad] = np.where(np.isnan(v), 0, np.where(v > 0, top, low))
        idx = np.flatnonzero(bad)
        info[k] = (idx, p.ravel()[idx].copy())
        out.append(q)
    return out, info or None


def _widen(c, W):
    """4:2:2 chroma (H, (W+1)//2) -> (H, W): even columns = the samples, odd = mean of neighbours."""
    out = np.empty((c.shape[0], W), np.float32)
    out[:, 0::2] = c[:, :(W + 1) // 2]
    nxt = np.concatenate([c[:, 1:], c[:, -1:]], 1)
    out[:, 1::2] = ((c + nxt) * np.float32(0.5))[:, :W // 2]
    return out


def to_internal(raw, fmt, matrix="bt709"):
    """Planes as read (ffmpeg order) -> (Y, Cb, Cr) on the internal 0..255 scale (float32; the legacy
    yuv420p path returns the uint8 planes unchanged)."""
    if fmt.legacy:
        return tuple(raw)
    if fmt.family == "yuv":
        k = np.float32(1.0 / 2 ** (fmt.bits - 8))
        Y, U, V = (np.asarray(p, np.float32) * k for p in raw)
        if fmt.chroma == "422":
            U, V = _widen(U, Y.shape[1]), _widen(V, Y.shape[1])
        return Y, U, V
    if fmt.is_float:
        raw, _ = sanitize(raw)
    G, B, R = raw
    s = 255.0 if fmt.is_float else 1.0 / 2 ** (fmt.bits - 8)
    r, g, b = (np.asarray(p, np.float64) * s for p in (R, G, B))
    kr, kb = _KRB.get(matrix, _KRB["bt709"])
    y = kr * r + (1 - kr - kb) * g + kb * b
    cb = (b - y) / (2 * (1 - kb)) + 128.0
    cr = (r - y) / (2 * (1 - kr)) + 128.0
    return tuple(np.ascontiguousarray(v, np.float32) for v in (y, cb, cr))


def from_internal(planes, fmt, matrix="bt709", restore=None):
    """(Y, Cb, Cr) internal planes -> raw frame bytes of `fmt` (the inverse of to_internal).
    Integers: rint and clip to 0..2^b-1. Float: neither clip nor rounding; `restore` (sanitize of
    the frame's raw planes) puts the original non-finite values back."""
    if fmt.legacy:
        return b"".join(np.clip(np.rint(o), 0, 255).astype(np.uint8).tobytes() for o in planes)
    if fmt.family == "yuv":
        Y, Cb, Cr = planes
        if fmt.chroma == "422":
            Cb, Cr = np.asarray(Cb)[:, ::2], np.asarray(Cr)[:, ::2]
        k, top = np.float32(2 ** (fmt.bits - 8)), 2 ** fmt.bits - 1
        return b"".join(np.clip(np.rint(np.asarray(p, np.float32) * k), 0, top).astype(fmt.dtype).tobytes()
                        for p in (Y, Cb, Cr))
    kr, kb = _KRB.get(matrix, _KRB["bt709"])
    y, cb, cr = (np.asarray(p, np.float64) for p in planes)
    r = y + 2 * (1 - kr) * (cr - 128.0)
    b = y + 2 * (1 - kb) * (cb - 128.0)
    g = (y - kr * r - kb * b) / (1 - kr - kb)
    if fmt.is_float:
        out = [(v / 255.0).astype(fmt.dtype) for v in (g, b, r)]
        for k, (idx, vals) in (restore or {}).items():
            out[k].ravel()[idx] = vals
        return b"".join(v.tobytes() for v in out)
    k, top = 2.0 ** (fmt.bits - 8), 2 ** fmt.bits - 1
    return b"".join(np.clip(np.rint(v * k), 0, top).astype(fmt.dtype).tobytes() for v in (g, b, r))
