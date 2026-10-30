import subprocess, numpy as np
W, H = 640, 360
raw = subprocess.run(["ffmpeg", "-v", "error", "-i", "../../../../../../test.mp4", "-vf", "select=between(n\\,280\\,303)", "-fps_mode", "passthrough",
                      "-f", "rawvideo", "-pix_fmt", "yuv420p", "-"], capture_output=True).stdout
a = np.frombuffer(raw, np.uint8); print("frames", a.size / (W * H * 1.5))
def enc(pix, data, out):
    p = subprocess.Popen(["ffmpeg", "-y", "-v", "error", "-f", "rawvideo", "-pix_fmt", pix, "-s", f"{W}x{H}", "-r", "30", "-i", "-",
                          "-c:v", "ffv1", "-level", "3", "-pix_fmt", pix, out], stdin=subprocess.PIPE)
    p.stdin.write(data); p.stdin.close(); assert p.wait() == 0
enc("yuv420p", raw, "src8.mkv"); enc("yuv420p10le", (a.astype("<u2") * 4).tobytes(), "src10.mkv")
# does ffmpeg's own 8->10 conversion equal x4?
c = np.frombuffer(subprocess.run(["ffmpeg", "-v", "error", "-i", "src8.mkv", "-f", "rawvideo", "-pix_fmt", "yuv420p10le", "-"],
                                 capture_output=True).stdout, "<u2")
print("ffmpeg 8->10 == x4:", np.array_equal(c, a.astype(np.uint16) * 4), "max diff", int(np.abs(c.astype(int) - 4 * a.astype(int)).max()))
