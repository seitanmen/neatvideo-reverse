"""OFX bridge: stdin/stdout raw yuv420p frame exchange for DCC host integration.

Protocol (one frame at a time, little-endian, no headers):
  stdin : raw yuv420p frames (W*H + 2*(W/2)*(H/2) bytes each), concatenated
  stdout: processed raw yuv420p frames, same layout

The host (DCC tool) launches this process with:
  python3 -m nv_ui_qt.main --ofx W H FPS [total_frames]
and writes frames to stdin. The UI displays them, the user adjusts settings,
and presses Apply to process and emit the current frame to stdout.

A separate "--ofx-serve" mode reads all frames from stdin, shows the UI, and
writes the processed result of every frame to stdout when the user closes the
window (batch mode). This is simpler for hosts that can wait for the process.
"""
import os
import sys

import numpy as np


def read_exact(stream, n):
    """Read exactly n bytes from a binary stream; return None on EOF."""
    buf = bytearray()
    while len(buf) < n:
        chunk = stream.read(n - len(buf))
        if not chunk:
            return None
        buf.extend(chunk)
    return bytes(buf)


def frame_size(W, H):
    return W * H + 2 * (W // 2) * (H // 2)


def unpack_yuv420p(data, W, H):
    """bytes → (Y, Cb, Cr) uint8 arrays."""
    y = np.frombuffer(data[:W * H], np.uint8).reshape(H, W)
    cw, ch = W // 2, H // 2
    cb = np.frombuffer(data[W * H:W * H + cw * ch], np.uint8).reshape(ch, cw)
    cr = np.frombuffer(data[W * H + cw * ch:], np.uint8).reshape(ch, cw)
    return (y, cb, cr)


def pack_yuv420p(planes):
    """(Y, Cb, Cr) float arrays → yuv420p bytes (uint8, clipped)."""
    out = bytearray()
    for p in planes:
        out.extend(np.clip(np.rint(p), 0, 255).astype(np.uint8).tobytes())
    return bytes(out)


class OfxFrameSource:
    """Replaces the ffmpeg-based frame loader when running in OFX mode.
    Reads frames from stdin on demand (or all at once)."""

    def __init__(self, W, H, fps=24.0, max_frames=0):
        self.W, self.H, self.fps = W, H, fps
        self.max_frames = max_frames
        self._buf = []       # list of (Y, Cb, Cr) uint8 tuples
        self._eof = False

    def read_all(self, progress=None):
        """Read all available frames from stdin."""
        fs = frame_size(self.W, self.H)
        while True:
            data = read_exact(sys.stdin.buffer, fs)
            if data is None:
                break
            self._buf.append(unpack_yuv420p(data, self.W, self.H))
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
                "tags": {}, "full_range": False, "rotation": 0}

    def emit_frame(self, planes):
        """Write one processed frame to stdout."""
        sys.stdout.buffer.write(pack_yuv420p(planes))
        sys.stdout.buffer.flush()
