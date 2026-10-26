"""stdin 8-bit yuv420p frames -> stdout depth 16 (v*256, uint16 LE) or 32 (v/255, float32 LE)."""
import sys
import numpy as np
if __name__ == "__main__":
    d = int(sys.argv[1])
    data = np.frombuffer(sys.stdin.buffer.read(), np.uint8)
    out = (data.astype("<u2") * 256) if d == 16 else (data.astype(np.float32) / np.float32(255)).astype("<f4")
    sys.stdout.buffer.write(out.tobytes())
