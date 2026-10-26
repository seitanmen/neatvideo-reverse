"""OFX bridge: stdin/stdout raw yuv420p frame exchange for DCC host integration.

Protocol (one frame at a time, little-endian, no headers):
  stdin : raw frames in the planar 4:2:0 layout Y (W x H), Cb, Cr ((W//2) x (H//2) each), concatenated
  stdout: processed raw frames, same layout and sample type
Sample type by --depth (default 8 = the original protocol):
  8 : uint8 code values; frame_size = W*H + 2*(W//2)*(H//2) bytes
  16: uint16 little-endian code values (8-bit value x 256); frame_size = 2 x the 8-bit size;
      internal scale v/256, written back as clip(rint(x*256), 0, 65535)
  32: float32 little-endian, value = 8-bit code value / 255 (Y 16/255..235/255, Cb/Cr 128/255 at
      neutral); frame_size = 4 x the 8-bit size; written back as x/255 with neither clip nor rounding.
      NaN / +-inf samples are processed as finite values and written back unchanged
      (nv_depth.sanitize)
Limited range, BT.709 (the host converts).

The host (DCC tool) launches this process with:
  python3 -m nv_ui_qt.main --ofx W H [FPS] [MAX_FRAMES] [--depth 8|16|32]
and writes frames to stdin, then closes it (the UI starts after EOF, or after MAX_FRAMES frames).
The UI displays them and the user adjusts settings. Frames written to stdout (main.py):
  - Apply: the current frame, processed with the current settings;
  - window closed (also Cancel): the same, unless an identical frame (same frame index, settings and
    profile) was already written; an unchanged Apply writes nothing either.
stdout therefore holds k >= 1 whole frames (k x frame_size bytes; k = 0 only when processing failed,
reported on stderr). The host waits for the process to exit and keeps the last complete frame.
"""
import os
import sys

import numpy as np

import nv_depth as nd


def read_exact(stream, n):
    """Read exactly n bytes from a binary stream; return None on EOF."""
    buf = bytearray()
    while len(buf) < n:
        chunk = stream.read(n - len(buf))
        if not chunk:
            return None
        buf.extend(chunk)
    return bytes(buf)


DEPTHS = {8: (np.uint8, 1), 16: ("<u2", 2), 32: ("<f4", 4)}     # depth -> (sample dtype, bytes)


def frame_size(W, H, depth=8):
    return (W * H + 2 * (W // 2) * (H // 2)) * DEPTHS[depth][1]


def unpack_yuv420p(data, W, H, depth=8, restore=None):
    """bytes → (Y, Cb, Cr): uint8 arrays (depth 8) or float32 on the 0..255 scale (16 / 32).
    Depth 32: non-finite samples are replaced (nv_depth.sanitize); a list `restore` receives the
    restore info (None when the frame is finite)."""
    a = np.frombuffer(data, DEPTHS[depth][0])
    cw, ch = W // 2, H // 2
    y = a[:W * H].reshape(H, W)
    cb = a[W * H:W * H + cw * ch].reshape(ch, cw)
    cr = a[W * H + cw * ch:].reshape(ch, cw)
    if depth == 8:
        return (y, cb, cr)
    planes = (y, cb, cr)
    if depth == 32:
        planes, info = nd.sanitize(planes)
        if restore is not None:
            restore.append(info)
    k = np.float32(1 / 256 if depth == 16 else 255)
    return tuple(p.astype(np.float32) * k for p in planes)


def pack_yuv420p(planes, depth=8, restore=None):
    """(Y, Cb, Cr) float arrays → bytes (uint8 / uint16 clipped and rounded; float32 as is / 255, with
    the original non-finite samples of `restore` put back)."""
    out = bytearray()
    for c, p in enumerate(planes):
        if depth == 8:
            out.extend(np.clip(np.rint(p), 0, 255).astype(np.uint8).tobytes())
        elif depth == 16:
            out.extend(np.clip(np.rint(np.asarray(p, np.float32) * 256), 0, 65535).astype("<u2").tobytes())
        else:
            v = (np.asarray(p, np.float32) / np.float32(255)).astype("<f4")
            if restore and c in restore:
                idx, vals = restore[c]
                v.ravel()[idx] = vals
            out.extend(v.tobytes())
    return bytes(out)


class OfxFrameSource:
    """Replaces the ffmpeg-based frame loader when running in OFX mode.
    Reads frames from stdin on demand (or all at once)."""

    def __init__(self, W, H, fps=24.0, max_frames=0, depth=8):
        if depth not in DEPTHS:
            raise ValueError(f"--depth {depth}: 8, 16 or 32")
        self.W, self.H, self.fps = W, H, fps
        self.max_frames = max_frames
        self.depth = depth
        self._buf = []       # list of (Y, Cb, Cr) tuples (uint8, or float32 0..255 for 16 / 32)
        self._restore = []   # per frame: non-finite samples to write back (depth 32) or None
        self._eof = False

    def read_all(self, progress=None):
        """Read all available frames from stdin."""
        fs = frame_size(self.W, self.H, self.depth)
        while True:
            data = read_exact(sys.stdin.buffer, fs)
            if data is None:
                break
            self._buf.append(unpack_yuv420p(data, self.W, self.H, self.depth, self._restore))
            if progress:
                progress(len(self._buf))
            if self.max_frames and len(self._buf) >= self.max_frames:
                break
        self._eof = True
        return self._buf

    def frames(self):
        return self._buf

    def info(self):
        return {"W": self.W, "H": self.H, "fps": f"{self.fps}", "n": len(self._buf),
                "tags": {}, "full_range": False, "rotation": 0, "pix_fmt": None, "ofx_depth": self.depth}

    def emit_frame(self, planes, index=None):
        """Write one processed frame (of input frame `index`) to stdout."""
        restore = self._restore[index] if index is not None and index < len(self._restore) else None
        sys.stdout.buffer.write(pack_yuv420p(planes, self.depth, restore))
        sys.stdout.buffer.flush()
