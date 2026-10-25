#!/usr/bin/env python3
"""Neat Video style tuning UI for the reproduced pipeline (v5).

Layout mirrors the genuine tool: preview (Original/Processed) + right-side
panels: Device Noise Profile / Temporal Filter / Spatial Filter / Output.
Frames are kept as native YCbCr 4:2:0 planes and processed by nvfilter v5
(same code path as denoise_video_v5.py, which "Process full video" runs).
Core <-> UI interface: params() -> dict, process_frame(t, P) -> RGB float.
"""
import json
import os
import queue
import re
import subprocess
import sys
import threading
import tkinter as tk
from tkinter import filedialog, ttk

import numpy as np
from PIL import Image, ImageTk

import nvfilter as nv
from denoise_video_v5 import probe, read_frames

PREVIEW_W = 640
HERE = os.path.dirname(os.path.abspath(__file__))


class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Neat Repro — Denoise Tuner v5 (Neat Video style)")
        self.geometry("1220x760")
        self.src = tk.StringVar(value=os.path.join(os.path.dirname(HERE), "test.mp4"))
        self.frames = None          # list of (Y, Cb, Cr) uint8
        self.info = None
        self.W = self.H = self.N = 0
        self.cur = 0
        self.prof = None
        self.prof_req = 0                 # latest auto_profile request (older results are dropped)
        self.area = None
        self.proc_cache = {}        # frame_idx -> processed float rgb
        self.job_q = queue.Queue()
        self.gen = 0                # bumped on every settings change; stale previews are dropped
        self._build()
        self.after(200, self._poll)

    # ---------- layout ----------
    def _build(self):
        top = ttk.Frame(self)
        top.pack(fill="x", padx=6, pady=4)
        ttk.Button(top, text="Open…", command=self.open_file).pack(side="left")
        ttk.Entry(top, textvariable=self.src, width=60).pack(side="left", padx=6)
        ttk.Button(top, text="Load", command=self.load).pack(side="left")
        self.info_lb = ttk.Label(top, text="no video")
        self.info_lb.pack(side="left", padx=10)

        mid = ttk.Frame(self)
        mid.pack(fill="both", expand=True, padx=6)
        left = ttk.Frame(mid)
        left.pack(side="left", fill="both", expand=True)
        self.view = tk.StringVar(value="processed")
        vb = ttk.Frame(left)
        vb.pack(fill="x")
        for t, v in [("Original", "original"), ("Processed", "processed")]:
            ttk.Radiobutton(vb, text=t, variable=self.view, value=v,
                            command=self.show).pack(side="left")
        self.canvas = tk.Label(left, background="black")
        self.canvas.pack(fill="both", expand=True)
        nav = ttk.Frame(left)
        nav.pack(fill="x", pady=4)
        self.fslider = ttk.Scale(nav, from_=0, to=100, orient="horizontal",
                                 command=self.on_seek)
        self.fslider.pack(side="left", fill="x", expand=True)
        self.flabel = ttk.Label(nav, text="f 0")
        self.flabel.pack(side="left", padx=6)

        nb = ttk.Notebook(mid, width=400)
        nb.pack(side="right", fill="y")
        self._tab_profile(nb)
        self._tab_temporal(nb)
        self._tab_spatial(nb)
        self._tab_output(nb)

        self.status = ttk.Label(self, text="ready", relief="sunken", anchor="w")
        self.status.pack(fill="x", side="bottom")

    def _slider(self, parent, text, frm, to, default, fmt="%.2f"):
        f = ttk.Frame(parent)
        f.pack(fill="x", padx=6, pady=1)
        ttk.Label(f, text=text, width=18).pack(side="left")
        v = tk.DoubleVar(value=default)
        lab = ttk.Label(f, text=fmt % default, width=6)
        lab.pack(side="right")

        def cb(*a, v=v, lab=lab, fmt=fmt):
            lab.config(text=fmt % v.get())
            self.invalidate()
        ttk.Scale(f, from_=frm, to=to, variable=v, command=cb).pack(side="left", fill="x", expand=True)
        return v

    def _tab_profile(self, nb):
        D = nv.V5_DEFAULTS
        p = ttk.Frame(nb)
        nb.add(p, text="Profile")
        row = ttk.Frame(p)
        row.pack(pady=6)
        ttk.Button(row, text="Auto Profile", command=self.auto_profile).pack(side="left")
        self.p_mode = tk.StringVar(value="auto")
        for m in ("auto", "spatial", "temporal"):
            ttk.Radiobutton(row, text=m, variable=self.p_mode, value=m,
                            command=self.auto_profile).pack(side="left")
        self.prof_txt = tk.Text(p, height=11, width=50)
        self.prof_txt.pack(padx=6)
        self.p_noise = self._slider(p, "Noise Level", 0.25, 4.0, D["noise_level"])
        self.p_noise_y = self._slider(p, "Luma Noise Level", 0.25, 4.0, D["channel_noise"]["Y"])
        self.p_noise_c = self._slider(p, "Chroma Noise Level", 0.25, 8.0, D["channel_noise"]["Cb"])
        fr = ttk.Frame(p)
        fr.pack(pady=4)
        ttk.Label(fr, text="area y,x,size").pack(side="left")
        self.area_vars = [tk.IntVar(value=v) for v in (0, 0, 64)]
        for vv in self.area_vars:
            ttk.Entry(fr, textvariable=vv, width=6).pack(side="left")
        ttk.Button(fr, text="Set", command=self.set_area).pack(side="left", padx=4)

    def _tab_temporal(self, nb):
        D = nv.V5_DEFAULTS
        p = ttk.Frame(nb)
        nb.add(p, text="Temporal")
        self.p_temporal_on = tk.BooleanVar(value=True)
        ttk.Checkbutton(p, text="Enable Temporal Filter", variable=self.p_temporal_on,
                        command=self.invalidate).pack(anchor="w", padx=6)
        self.p_radius = self._slider(p, "Radius", 1, 5, D["radius"], fmt="%.0f")
        self.p_temporal = self._slider(p, "Amount", 0.0, 1.0, D["temporal"])
        self.p_threshold = self._slider(p, "Threshold", 0.25, 8.0, D["threshold"])

    def _tab_spatial(self, nb):
        D = nv.V5_DEFAULTS
        p = ttk.Frame(nb)
        nb.add(p, text="Spatial")
        self.p_spatial = self._slider(p, "Strength", 0.25, 4.0, D["spatial"])
        self.p_luma = self._slider(p, "Luminance amount", 0.0, 1.0, D["luma"])
        self.p_chroma = self._slider(p, "Chrominance amount", 0.0, 1.0, D["chroma"])
        ttk.Label(p, text="Noise Reduction Amount per band").pack(pady=(8, 0))
        self.p_bands = {b: self._slider(p, b, 0.0, 1.0, D["band_amounts"][b]) for b in nv.V5_BANDS}
        ttk.Label(p, text="Noise Level per band (× profile)").pack(pady=(8, 0))
        self.p_band_noise = {b: self._slider(p, b + " ", 0.25, 4.0, D["band_noise"][b]) for b in nv.V5_BANDS}

    def _tab_output(self, nb):
        p = ttk.Frame(nb)
        nb.add(p, text="Output")
        self.p_mix = self._slider(p, "Mix original", 0.0, 1.0, nv.V5_DEFAULTS["mix"])
        ttk.Button(p, text="Preview current frame", command=self.preview).pack(pady=6, fill="x", padx=6)
        ttk.Button(p, text="Save frame PNG", command=self.save_frame).pack(pady=2, fill="x", padx=6)
        ttk.Button(p, text="Process full video…", command=self.process_video).pack(pady=6, fill="x", padx=6)
        self.prog = ttk.Progressbar(p, mode="determinate", maximum=100)
        self.prog.pack(fill="x", padx=6)

    # ---------- video io ----------
    def open_file(self):
        f = filedialog.askopenfilename(filetypes=[("video", "*.mp4 *.mov *.mkv")])
        if f:
            self.src.set(f)
            self.load()

    def load(self):
        self.info = probe(self.src.get())
        self.W, self.H = self.info["W"], self.info["H"]
        self.frames = [f for _, f in read_frames(self.src.get(), self.W, self.H, fps=self.info["fps"])]
        self.N = len(self.frames)
        self.matrix = nv.matrix_from_tags(self.info["tags"], self.H)
        self.fslider.config(to=self.N - 1)
        self.info_lb.config(text=f"{self.W}x{self.H} {self.N}f {self.info['fps']}fps {self.matrix}")
        self.invalidate()
        self.area = None
        self.auto_profile()
        self.cur = 0
        self.show()

    def planes(self, i):
        return tuple(p.astype(np.float32) for p in self.frames[i])

    def invalidate(self):
        """Settings changed: drop cached previews and any preview still being computed."""
        self.gen += 1
        self.proc_cache.clear()

    def display(self, img):
        """Coded orientation -> display orientation (rotation tag, counter-clockwise)."""
        return np.rot90(img, (self.info["rotation"] // 90) % 4) if self.info else img

    def to_rgb(self, planes):
        return nv.yuv_to_rgb(planes, self.matrix, self.info["full_range"])

    # ---------- profile ----------
    def profile_frame(self):
        return min(self.N // 3, max(self.N - 3, 0))

    def auto_profile(self):
        """Profile in a worker thread (the genuine noise curve and flat-area search take a few
        seconds on HD frames); the result is applied in _poll unless a newer request superseded it."""
        if self.frames is None:
            return
        pf = self.profile_frame()
        planes = self.planes(pf)
        more = [self.planes(i) for i in range(pf + 1, min(pf + 3, self.N))]
        args = dict(area=self.area, pair=more[0] if more else None, mode=self.p_mode.get(), more=more,
                    matrix=self.matrix)
        full_range = self.info["full_range"]
        self.prof_req += 1
        req = self.prof_req
        self.prof = None                  # no preview with a stale profile (other video / mode / area)
        self.invalidate()
        self.status.config(text=f"profiling frame {pf}…")

        def job():
            try:
                cancel = lambda: req != self.prof_req       # a newer request cancels this one (status 5)
                self.job_q.put(("profile", req, nv.build_profile_v5(planes, full_range, cancel=cancel, **args)))
            except nv.ProfileCancelled:
                pass                                         # superseded: _poll drops it anyway
            except Exception as e:
                self.job_q.put(("error", str(e), req))
        threading.Thread(target=job, daemon=True).start()

    def set_area(self):
        self.area = tuple(int(v.get()) for v in self.area_vars)
        self.auto_profile()

    def show_profile(self):
        pr = self.prof
        st = pr.get("flat_status")
        lines = [f"frame {self.profile_frame()}  mode={pr['mode']}  area(y,x,size)={pr['area']}  "
                 f"[{'manual' if st is None else nv.FLAT_STATUS.get(st, st)}]",
                 "σ total @area: " + ", ".join(f"{k}={v:.2f}" for k, v in pr["sigma"].items())]
        for n in ("Y", "Cb", "Cr"):
            lines.append(f"{n} band ratio " + " ".join(f"{r:.2f}" for r in pr["band_ratio"][n]))
        lines.append("brightness bins " + " ".join(f"{c:.0f}" for c in pr["curve_centers"]))
        for n in ("Y", "Cb", "Cr"):
            lines.append(f"{n} σ1 curve " + " ".join(f"{v:.2f}" for v in pr["curve"][n]))
        self.prof_txt.delete("1.0", "end")
        self.prof_txt.insert("end", "\n".join(lines) + "\n")

    # ---------- processing ----------
    def params(self):
        P = nv.v5_params(
            radius=int(round(self.p_radius.get())),
            temporal=self.p_temporal.get() if self.p_temporal_on.get() else 0.0,
            threshold=self.p_threshold.get(),
            spatial=self.p_spatial.get(), luma=self.p_luma.get(), chroma=self.p_chroma.get(),
            band_amounts={b: v.get() for b, v in self.p_bands.items()},
            band_noise={b: v.get() for b, v in self.p_band_noise.items()},
            noise_level=self.p_noise.get(),
            channel_noise={"Y": self.p_noise_y.get(), "Cb": self.p_noise_c.get(), "Cr": self.p_noise_c.get()},
            mix=self.p_mix.get())
        P["prof"] = self.prof
        return P

    def process_frame(self, t, P):
        R = int(P["radius"])
        idx = [i for i in range(t - R, t + R + 1) if 0 <= i < self.N]
        out = nv.process_frame_v5([self.planes(i) for i in idx], idx.index(t), P["prof"], P)
        return np.clip(self.to_rgb([np.clip(o, 0, 255) for o in out]), 0, 255)

    def preview(self):
        if self.frames is None:
            return
        if self.prof is None:
            self.status.config(text="profile not ready yet")
            return
        self.status.config(text="processing…")
        P = self.params()
        t, gen = self.cur, self.gen

        def job():
            try:
                self.job_q.put(("preview", t, self.process_frame(t, P), gen))
            except Exception as e:
                self.job_q.put(("error", str(e)))
        threading.Thread(target=job, daemon=True).start()

    def process_video(self):
        if self.frames is None:
            return
        dst = filedialog.asksaveasfilename(defaultextension=".mp4", filetypes=[("mp4", "*.mp4")])
        if not dst:
            return
        P = self.params()
        P.pop("prof")
        pfile = os.path.splitext(dst)[0] + ".params.json"
        with open(pfile, "w") as fh:
            json.dump(P, fh, indent=1)
        cmd = [sys.executable, os.path.join(HERE, "denoise_video_v5.py"), self.src.get(), dst,
               "--params", pfile, "--profile", self.p_mode.get(), "--profile-frame", str(self.profile_frame())]
        if self.area is not None:
            cmd += ["--area", ",".join(str(v) for v in self.area)]
        self.status.config(text="encoding…")

        def job():
            try:
                pr = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
                for line in pr.stdout:
                    m = re.match(r"frame (\d+)/(\d+)", line)
                    if m:
                        self.job_q.put(("progress", 100.0 * int(m.group(1)) / max(int(m.group(2)), 1)))
                pr.wait()
                self.job_q.put(("done", dst) if pr.returncode == 0 else ("error", f"exit {pr.returncode}"))
            except Exception as e:
                self.job_q.put(("error", str(e)))
        threading.Thread(target=job, daemon=True).start()

    def save_frame(self):
        if self.cur in self.proc_cache:
            f = filedialog.asksaveasfilename(defaultextension=".png")
            if f:
                Image.fromarray(self.display(np.clip(self.proc_cache[self.cur], 0, 255).astype(np.uint8))).save(f)

    # ---------- view ----------
    def on_seek(self, *a):
        self.cur = int(float(self.fslider.get()))
        self.flabel.config(text=f"f {self.cur}")
        self.show()

    def show(self):
        if self.frames is None:
            return
        if self.view.get() == "processed" and self.cur in self.proc_cache:
            img = self.proc_cache[self.cur]
        else:
            img = self.to_rgb(self.planes(self.cur))
        img = self.display(np.clip(img, 0, 255).astype(np.uint8))
        h, w = img.shape[:2]
        s = PREVIEW_W / w
        im = Image.fromarray(img).resize((PREVIEW_W, int(h * s)))
        self.tkimg = ImageTk.PhotoImage(im)
        self.canvas.config(image=self.tkimg)

    def _poll(self):
        try:
            while True:
                msg = self.job_q.get_nowait()
                kind = msg[0]
                if kind == "preview":
                    _, t, out, gen = msg
                    if gen != self.gen:
                        self.status.config(text=f"frame {t}: settings changed meanwhile, preview again")
                        continue
                    self.proc_cache[t] = out
                    self.status.config(text=f"frame {t} done")
                    self.show()
                elif kind == "profile":
                    _, req, prof = msg
                    if req != self.prof_req:
                        continue          # superseded by a newer profile request
                    self.prof = prof
                    for vv, val in zip(self.area_vars, prof["area"]):
                        vv.set(val)
                    self.invalidate()
                    self.show_profile()
                    self.status.config(text="profile ready")
                elif kind == "progress":
                    self.prog["value"] = msg[1]
                    self.status.config(text=f"encoding {msg[1]:.0f}%")
                elif kind == "done":
                    self.status.config(text=f"wrote {msg[1]}")
                    self.prog["value"] = 0
                elif kind == "error":
                    if len(msg) > 2 and msg[2] != self.prof_req:
                        continue              # error of a superseded profiling request
                    self.status.config(text=f"error: {msg[1]}")
        except queue.Empty:
            pass
        self.after(200, self._poll)


if __name__ == "__main__":
    App().mainloop()
