#!/usr/bin/env python3
"""verify_ofx - MeatVideo.ofx (N4d: the genuine's two buttons, the window in a separate process, the state in
ClipParams) rendered and driven by the test host (ofx/build/minihost), bit for bit against the bridge path of the
Python UI and against the C++ UI's own processing.

Host side (this script and minihost): test clips are decoded with ffmpeg into RGBA float32 frames (alpha a fixed
pattern), written to a raw file and read by minihost, which hands them to the plug-in as OFX images.  The plug-in
itself reads no file: fsguard (the injected file-access monitor) records every file / process call of the plug-in
session; expected are the host's dlopen of the bundle and one posix_spawn of the window helper per button press that
opens the window - nothing from a render.

The window: minihost presses Prepare Noise Profile / Adjust Filter Settings (InstanceChanged, kOfxChangeUserEdited);
the plug-in fetches the frames around the current time from the host and starts the helper.  Here the helper is
qtui/build/qtui_ofx_session_drv (MEATVIDEO_OFX_HELPER), which runs the real session code (nvqt::ofx_session_main)
offscreen with a script (NV_OFX_SESSION_SCRIPT, set per press with minihost --env): e.g. "auto;process F;apply".
HOME and the UI's settings file (NV_UI_QT_SETTINGS) point into the work directory; ~/.config/NeatRepro/nv_ui_qt.ini is
hashed before and after.

ClipParams states compared:
  - from the window (a press): the reply the plug-in stored (minihost --clip-out);
  - synthetic: made here with the Python UI's functions (backend.build_profile (+ _run_aft), state.backend_params,
    state.effective_profile) and handed to the plug-in as a saved project's / a pasted value (minihost
    --init-clip-params / --clip-params), for the breadth of the N4b checks (parameters, per-channel spatial settings,
    Temporal Noise Level, Mix, matrices, profile modes, HDR / NaN, missing frames, 1080p, layouts).

Reference of a render (a child process, NV_NATIVE=1 = the bridge through nv_native / libnvcore, or NV_NATIVE=0 = the
Python functions): ClipParams' "render" read with json; no profile -> the source frame unchanged (RGBA); else the
frames mapped as the --depth 32 path maps float RGB (nv_depth.to_internal(G, B, R planes, pixfmt("gbrpf32le"),
render.matrix); the restore info of nv_depth.sanitize), backend.process_frame(window, ci, render.profile, render.P) on
the frames t-R .. t+R inside the clip (each side ending at the first frame the host does not give), and
nv_depth.from_internal back to R, G, B with the source alpha.  Compared as uint32 words (NaN payloads included).

Checks of a window's ClipParams (Python, independent of the C++ UI): "render.P" == state.backend_params(
state.params_from_json("preset")); the effective profile keeps the base profile's keys other than the curve / sigma;
for a press scripted "auto" on the current frame, the base profile (its pre-AFT curve / sigma) == Python's
backend.build_profile on the frames the window received (the helper's frame conversion and Auto Profile).  And the
C++ UI's processing of the current frame (driver step "process") == the plug-in's render of that frame.

  python3 ofx/tools/verify_ofx.py [--quick] [--pure] [--mutate] [--work DIR]
    --quick   no 1080p / B clips
    --pure    also the reference with NV_NATIVE=0 (the Python functions; slow)
    --mutate  the comparisons and checks must fail on changed inputs (sensitivity)
INT-4 (interlaced clips, scenarios I-*): the window's Video Scan Type is stored in ClipParams ("scan", render.P
["interlace"]) and the render filters field by field; the reference then is the C++ UI's field pipeline (qtui fields.cpp
process_fields) in Python: native/fields_ref.py's field sequence and temporal_shift (the half-line offsets) with the
Python UI's backend.finish, on the frames t - m .. t + m (m = radius // 2 + 1); a render of one field (FieldToRender
lower / upper) is compared on that field's rows, the others staying the host's zeros.  The host's field order
(kOfxImageClipPropFieldOrder) is the window's default scan only.
Needs ofx/build (ofx/build_ofx.sh), qtui/build/qtui_ofx_session_drv, ffmpeg, numpy, PySide6 and the test videos.
"""
import argparse
import copy
import hashlib
import json
import os
import re
import subprocess
import sys
import time

import numpy as np

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
REF = os.path.join(ROOT, "reference", "python")
BUILD = os.path.join(ROOT, "ofx", "build")
MINIHOST = os.path.join(BUILD, "minihost")
BUNDLE = os.path.join(BUILD, "MeatVideo.ofx.bundle")
FSGUARD = os.path.join(BUILD, "libfsguard.dylib")
DRV = os.path.join(ROOT, "qtui", "build", "qtui_ofx_session_drv")
USER_INI = os.path.expanduser("~/.config/NeatRepro/nv_ui_qt.ini")
MODES = ["auto", "spatial", "temporal"]


def sha(path):
    return hashlib.sha256(open(path, "rb").read()).hexdigest() if os.path.exists(path) else None


# ---------------------------------------------------------------- host side: test clips
def decode(video, start, n, crop=None):
    """n frames from frame `start` as RGB float (N, H, W, 3), BT.709 limited range -> RGB 0..1 (swscale)."""
    vf = f"select='between(n\\,{start}\\,{start + n - 1})'"
    if crop:
        w, h, x, y = crop
        vf += f",crop={w}:{h}:{x}:{y}"
    vf += ",scale=in_color_matrix=bt709:in_range=tv,format=gbrpf32le"
    out = subprocess.run(["ffmpeg", "-v", "error", "-nostdin", "-i", os.path.join(ROOT, video), "-vf", vf,
                          "-fps_mode", "passthrough", "-frames:v", str(n), "-f", "rawvideo", "-pix_fmt", "gbrpf32le",
                          "-"], check=True, capture_output=True).stdout
    a = np.frombuffer(out, "<f4")
    W, H = (crop[0], crop[1]) if crop else probe(video)
    a = a.reshape(n, 3, H, W)
    return np.stack([a[:, 2], a[:, 0], a[:, 1]], -1)          # G, B, R planes -> R, G, B


def probe(video):
    s = subprocess.run(["ffprobe", "-v", "error", "-select_streams", "v:0", "-show_entries", "stream=width,height",
                        "-of", "csv=p=0", os.path.join(ROOT, video)], check=True, capture_output=True, text=True).stdout
    w, h = s.strip().split(",")
    return int(w), int(h)


def rgba(rgb):
    n, H, W, _ = rgb.shape
    k, y, x = np.meshgrid(np.arange(n), np.arange(H), np.arange(W), indexing="ij")
    alpha = (((x * 7 + y * 3 + k) % 11) / 10.0).astype(np.float32)
    return np.ascontiguousarray(np.concatenate([rgb.astype(np.float32), alpha[..., None]], -1))


def make_clips(work, quick):
    clips = {}

    def put(name, arr):
        path = os.path.join(work, f"clip_{name}.f32")
        arr.tofile(path)
        n, H, W, _ = arr.shape
        clips[name] = {"path": path, "N": n, "H": H, "W": W}

    a = decode("test.mp4", 300, 8)
    put("A", rgba(a))
    # C: HDR / negative values and NaN / +-inf samples (every frame)
    c = a[:5, 88:272, 160:480].copy() * np.float32(1.3) - np.float32(0.05)
    c = rgba(c)
    rng = np.random.default_rng(7)
    for k in range(c.shape[0]):
        for v in (np.nan, np.inf, -np.inf, np.float32(-0.0)):
            for _ in range(3):
                c[k, rng.integers(0, c.shape[1]), rng.integers(0, c.shape[2]), rng.integers(0, 4)] = v
    put("C", c)
    rng = np.random.default_rng(11)
    for name, (n, h, w) in {"R96": (6, 80, 96), "R63": (2, 63, 80)}.items():
        put(name, rng.random((n, h, w, 4), dtype=np.float32))
    # INT-4: an odd height (the bottom field's pad row; OFX's lower field = top-down rows 0, 2, .. when H is odd)
    put("O", np.ascontiguousarray(rgba(a)[:, :359, :352]))
    if not quick:
        put("B", rgba(decode("test_v02.mp4", 304, 7, crop=(512, 288, 704, 0))))
        put("D", rgba(decode("test_v02.mp4", 312, 3)))
    return clips


def load_clip(clip):
    return np.fromfile(clip["path"], "<f4").reshape(clip["N"], clip["H"], clip["W"], 4)


# ---------------------------------------------------------------- Python UI side (imported lazily, NV_NATIVE=1)
_PY = {}


def py():
    if not _PY:
        os.environ.setdefault("NV_NATIVE", "1")
        sys.path.insert(0, REF)
        import nv_depth
        import nvfilter as nv
        from nv_ui_qt import backend, state
        sys.path.insert(0, os.path.join(ROOT, "native"))
        import fields_ref
        _PY.update(nv_depth=nv_depth, nv=nv, backend=backend, state=state, fr=fields_ref)
    return _PY


def jsonable(o):
    """the value as json.dumps writes it (numpy arrays / scalars as lists / floats)"""
    return json.loads(json.dumps(o, default=lambda x: x.tolist() if hasattr(x, "tolist") else float(x)))


def internal_frames(frames, mat):
    """the clip's RGBA frames as the window / the render map them: (planes, restore) per frame"""
    m = py()
    fmt = m["nv_depth"].pixfmt("gbrpf32le")
    out = []
    for k in range(frames.shape[0]):
        raw = [np.ascontiguousarray(frames[k, :, :, c]) for c in (1, 2, 0)]   # G, B, R (ffmpeg order)
        out.append((tuple(m["nv_depth"].to_internal(raw, fmt, mat)), m["nv_depth"].sanitize(raw)[1]))
    return out


def synth_cp(clip, spec):
    """a ClipParams made with the Python UI's functions: the profile of clip frames spec["at"].. (build_profile +
    _run_aft), the UI values spec["ui"], the profile Noise Level spec["tune"]"""
    m = py()
    backend, state, nv = m["backend"], m["state"], m["nv"]
    frames = load_clip(clip)
    H = clip["H"]
    mat = spec.get("matrix") or nv.matrix_from_tags(None, H)
    at = spec.get("at", 0)
    fl = [p for p, _ in internal_frames(frames[at:at + 3], mat)]
    il = spec.get("interlace")
    if il:                      # INT-4: a One field profile of the field `parity` (the window's field profiles)
        q = spec.get("parity", il["first"])
        fl = [tuple(m["fr"].split_plane(pl, q) for pl in f) for f in fl]
    prof = backend.build_profile(fl, 0, True, mat, spec.get("mode", "auto"))
    if spec.get("aft", True):
        prof = backend._run_aft(prof, fl[0], True, mat)
    if il:
        prof["fields"], prof["field"] = "one", ("top", "bottom")[q]
    ui = state.default_params()
    for k, v in spec.get("ui", {}).items():
        ui[k] = state.coerce(k, v)
    tune = spec.get("tune", {})
    ps = state.ProfileState(prof, state.unit_factors(), [set(), set(), set()], "Unspecified device", dict(tune))
    eff = state.effective_profile(prof, ps.factors, ps.tune)
    cp = {"format": "meatvideo_ofx_clip_params", "version": 1, "frame_size": [clip["W"], H],
          "preset": state.preset_json(ui), "profile": ps.to_json(),
          "render": {"matrix": mat, "full_range": True, "P": state.backend_params(ui), "profile": eff}}
    if il:                      # INT-4: version 2 (an older plug-in refuses it)
        cp["version"] = 2
        cp["render"]["P"]["interlace"] = dict(il)
        cp["scan"] = ("tff", "bff")[il["first"]]
    return json.dumps(jsonable(cp))


def process_fields_ui(window, ci, il, prof, P):
    """qtui fields.cpp process_fields in Python: the field sequence of the window, every field's neighbours within
    il["radius"] fields (same parity / half-line shift / plain), temporal (fields_ref.temporal_shift with backend.temporal's
    t_noise) and backend.finish on the field, the two fields joined"""
    m = py()
    fr, backend = m["fr"], m["backend"]
    first, mode, R = il["first"], fr.MODES[il["mode"]], int(il["radius"])
    fields, parity = fr.field_sequence(window, first)
    tn = float(P.get("t_noise", 1.0))
    Qt = P if tn == 1.0 else dict(P, noise_level=P["noise_level"] * tn)
    out = {}
    for j in (0, 1):
        s = 2 * ci + j
        q = parity[s]
        lo, hi = max(0, s - R), min(len(fields), s + R + 1)
        idx = [k for k in range(lo, hi) if mode != fr.SAME or parity[k] == q]
        shift = [(0.5 * (q - parity[k]),) * 2 for k in idx] if mode == fr.SHIFT else None
        tmp, rv = fr.temporal_shift([fields[k] for k in idx], idx.index(s), prof, Qt, {}, shift)
        out[q] = backend.finish(fields, s, tmp, rv, prof, P)
    H = np.asarray(window[ci][0]).shape[0]
    return [fr.join_plane(t, b, H) for t, b in zip(out[0], out[1])]


# ---------------------------------------------------------------- the plug-in run
def minihost_args(sc, clip, out, cp_prefix, texts):
    a = [MINIHOST, BUNDLE, "--frames", clip["path"], "--size", str(clip["W"]), str(clip["H"]), str(clip["N"]),
         "--first", str(sc["first"]), "--origin", *map(str, sc["origin"]), "--layout", sc["layout"]]
    if sc.get("field_order"):
        a += ["--field-order", sc["field_order"]]
    if sc.get("field_to_render"):
        a += ["--field-to-render", sc["field_to_render"]]
    if sc.get("missing"):
        a += ["--missing", ",".join(str(t) for t in sc["missing"])]
    if sc.get("depth"):
        a += ["--depth", sc["depth"]]
    if sc.get("reenter"):
        a += ["--reenter"]
    if sc.get("stack"):
        a += ["--thread-stack", str(sc["stack"])]                # a host worker thread's stack (KB)
    if sc.get("init") is not None:
        a += ["--init-clip-params", texts[id(sc["init"])]]
    for st in sc["steps"]:
        kind = st[0]
        if kind == "clip":
            a += ["--clip-params", texts[id(st[1])]]
        elif kind == "range":
            a += ["--range", f"{st[1][0]},{st[1][1]}"]
        elif kind == "env":
            a += ["--env", st[1]]
        elif kind == "press":
            a += ["--env", "NV_OFX_SESSION_SCRIPT=" + st[3], "--press", f"{st[1]}@{st[2]}"]
        elif kind == "sync":
            a += ["--sync"]
        elif kind == "failset":
            a += ["--fail-set", str(st[1])]
        elif kind == "ini":
            a += ["--env", "NV_UI_QT_SETTINGS=" + ini_path(os.path.dirname(out), st[1])]
        else:
            a += ["--render", ",".join(str(t) for t in st[1])]
    return a + ["--out", out, "--clip-out", cp_prefix]


def run_plugin(work, sc, clip, out, texts):
    home = os.path.join(work, "home")
    os.makedirs(home, exist_ok=True)
    env = dict(os.environ, DYLD_INSERT_LIBRARIES=FSGUARD, HOME=home, QT_QPA_PLATFORM="offscreen",
               NV_UI_QT_SETTINGS=ini_path(work, sc.get("ini", "none")), MEATVIDEO_OFX_HELPER=sc.get("helper", DRV))
    env.pop("NV_OFX_SESSION_SCRIPT", None)
    cp_prefix = out + ".cp"
    t0 = time.time()
    p = subprocess.run(minihost_args(sc, clip, out, cp_prefix, texts), env=env, capture_output=True, text=True,
                       timeout=1800)
    if p.returncode != 0:
        raise SystemExit(f"minihost failed ({p.returncode}):\n{p.stdout}\n{p.stderr}")
    open(out + ".log", "w").write(p.stdout)
    open(out + ".err", "w").write(p.stderr)
    presses = [ln for ln in p.stdout.splitlines() if ln.startswith(("press ", "sync "))]
    cps = [open(f"{cp_prefix}_{k}.json").read() for k in range(len(presses))]
    return p.stdout, p.stderr, cps, time.time() - t0


# ---------------------------------------------------------------- reference (child process)
def reference(spec_path, out_path):
    m = py()
    backend, nv_depth = m["backend"], m["nv_depth"]
    spec = json.load(open(spec_path))
    clip = spec["clip"]
    N, H, W = clip["N"], clip["H"], clip["W"]
    frames = load_clip(clip)
    if spec.get("mutate_input"):
        u = frames.view("<u4")
        u[0, H // 2, W // 2, 1] ^= 1
    fmt = nv_depth.pixfmt("gbrpf32le")
    internal = {}
    first = spec["first"]
    missing = set(spec.get("missing", []))
    lo, hi = first, first + N - 1
    cp, outs, info = "", [], []

    def available(tt):
        return lo <= tt <= hi and tt not in missing

    for st in spec["steps"]:
        if st[0] == "clip":
            cp = st[1]
            continue
        if st[0] == "range":
            lo, hi = st[1]
            continue
        for t in st[1]:
            k = int(t - first)
            r = json.loads(cp)["render"] if cp else None
            if r is None:
                outs.append(frames[k].astype("<f4"))
                info.append({"t": t, "passthrough": True})
                continue
            mat, P, prof = r["matrix"], r["P"], r["profile"]
            if mat == "auto":                       # a new instance's defaults: the window's rule by the height
                mat = m["nv"].matrix_from_tags(None, H)
            if mat not in internal:
                internal[mat] = internal_frames(frames, mat)
            fl = internal[mat]
            il = P.get("interlace")          # INT-4: the frames the field window reaches
            R = int(il["radius"]) // 2 + 1 if il else int(P["radius"])
            left = []
            for d in range(1, R + 1):
                if not available(t - d):
                    break
                left.append(t - d)
            right = []
            for d in range(1, R + 1):
                if not available(t + d):
                    break
                right.append(t + d)
            idx = [int(tt - first) for tt in left[::-1] + [t] + right]
            if il:
                out = process_fields_ui([fl[i][0] for i in idx], len(left), il, prof, P)
            else:
                out = backend.process_frame([fl[i][0] for i in idx], len(left), prof, P)
            g, b, rr = np.frombuffer(nv_depth.from_internal(out, fmt, mat, fl[k][1]), "<f4").reshape(3, H, W)
            o = np.stack([rr, g, b, frames[k, :, :, 3]], -1).astype("<f4")
            ftr = spec.get("field_to_render")
            if ftr in ("lower", "upper"):   # only that field's rows (OFX rows bottom-up: lower = 0, 2, .. from the bottom)
                par = (H - 1 - (ftr == "upper")) & 1
                o[np.arange(H) % 2 != par] = 0
            outs.append(o)
            info.append({"t": t, "passthrough": False, "area": prof.get("area"), "native": backend.native_enabled(),
                         "matrix": mat, "window": [i + first for i in idx], "uniform": backend._uniform(P),
                         "fields": il})
    np.concatenate([o[None] for o in outs]).tofile(out_path)
    json.dump(info, open(out_path + ".json", "w"))


def run_reference(work, name, sc, clip, steps, native, tag, mutate_input=False):
    spec = {"clip": clip, "first": sc["first"], "steps": steps, "mutate_input": mutate_input,
            "missing": sc.get("missing", []), "field_to_render": sc.get("field_to_render")}
    sp = os.path.join(work, f"spec_{name}_{tag}.json")
    json.dump(spec, open(sp, "w"))
    out = os.path.join(work, f"ref_{name}_{tag}.f32")
    env = dict(os.environ, NV_NATIVE="1" if native else "0")
    env.pop("DYLD_INSERT_LIBRARIES", None)
    t0 = time.time()
    subprocess.run([sys.executable, __file__, "--reference", sp, out], env=env, check=True, cwd=REF)
    return out, time.time() - t0


# ---------------------------------------------------------------- checks of a window's ClipParams
def check_cp(cp_text, clip, first, press_t, fetched, check_build):
    """the Python checks of a ClipParams the window returned: a list of failures (empty = ok)"""
    m = py()
    state, backend = m["state"], m["backend"]
    bad = []
    cp = json.loads(cp_text)
    r = cp.get("render")
    if r is None:
        return bad if cp.get("profile") is None else ["render null with a profile"]
    P = state.backend_params(state.params_from_json(cp["preset"]))
    rP = dict(r["P"])
    il = rP.pop("interlace", None)       # INT-4: the field processing of an interlaced scan, with "scan"
    if jsonable(P) != rP:
        bad.append("render.P != state.backend_params(params_from_json(preset))")
    if (il is None) != ("scan" not in cp) or (il is not None and (("tff", "bff")[il["first"]] != cp["scan"] or
                                                                 il["mode"] not in ("same", "shift", "plain"))):
        bad.append(f"scan {cp.get('scan')} / render.P interlace {il} disagree")
    base, eff = cp["profile"]["base"], r["profile"]
    for k in base:
        if k not in ("curve", "sigma") and eff.get(k) != base[k]:
            bad.append(f"effective profile key {k} != base")
    if check_build:
        frames = load_clip(clip)
        times = sorted(fetched)
        fl = [p for p, _ in internal_frames(frames[[int(t - first) for t in times]], r["matrix"])]
        f = times.index(press_t)
        kind = base.get("fields", "frame")
        if kind == "frame":
            prof = jsonable(backend.build_profile(fl, f, True, r["matrix"], "auto"))
        else:                           # INT-4: qtui fields.h build_profile_fields
            fr = m["fr"]
            q = 1 if base.get("field") == "bottom" else 0
            sp = lambda k, qq: tuple(fr.split_plane(pl, qq) for pl in fl[k])
            if kind == "one":
                more = [sp(k, q) for k in range(f + 1, min(f + 3, len(fl)))]
                pair = more[0] if more else None
            else:
                more = [sp(f, 1 - q)] + ([sp(f + 1, q)] if f + 1 < len(fl) else [])
                pair = more[1] if len(more) > 1 else None
            prof = m["nv"].build_profile_v5(sp(f, q), True, None, pair=pair, mode="auto", more=tuple(more),
                                            matrix=r["matrix"])
            prof = jsonable(dict(prof, ui_frame=f, fields=kind, field=("top", "bottom")[q]))
        pre = base.get("ui_pre_aft", {})
        for k, v in prof.items():
            got = pre[k] if k in pre else base.get(k)
            if got != v:
                bad.append(f"profile {k}: window {str(got)[:60]} != Python build_profile {str(v)[:60]}")
    return bad


# ---------------------------------------------------------------- the Default Profile / Preset (isolated settings)
INIS = ("none", "set", "broken", "badprof", "badpre")
DEFAULTS = {}        # ini key -> (profile text or None, preset text or None) as stored
BROKEN = set()       # the stored values that are not loadable


def ini_path(work, key):
    return os.path.join(work, "home", f"defaults_{key}.ini")


def make_defaults(work, clip):
    """the settings files of the default cases, written with Qt's QSettings as the UI writes them: the Default Profile
    = an Auto Profile the window made on clip frames 0..7 (current 4), the Default Preset = changed filter settings
    (per-channel bands, Mix, radius 3); "broken" values are not loadable (not JSON / not a preset)"""
    from PySide6.QtCore import QSettings
    home = os.path.join(work, "home")
    os.makedirs(home, exist_ok=True)
    frames = load_clip(clip)
    h = {"protocol": "meatvideo-ofx-session", "version": 1, "page": "prepare", "width": clip["W"],
         "height": clip["H"], "fps": 24.0, "matrix": "bt601", "format": "gbrpf32le",
         "times": [float(k) for k in range(clip["N"])], "current": 4, "clip_params": ""}
    data = (json.dumps(h) + "\n").encode() + b"".join(
        np.ascontiguousarray(np.stack([frames[k, :, :, 1], frames[k, :, :, 2], frames[k, :, :, 0]])).tobytes()
        for k in range(clip["N"]))
    open(os.path.join(work, "default_request.bin"), "wb").write(data)
    st = os.path.join(work, "default_source.json")
    env = dict(os.environ, HOME=home, QT_QPA_PLATFORM="offscreen", NV_UI_QT_SETTINGS=ini_path(work, "none"),
               NV_OFX_SESSION_SCRIPT=f"auto;state {st};cancel")
    p = subprocess.run([DRV, "--ofx-session"], input=data, capture_output=True, env=env, timeout=600)
    if p.returncode != 0 or p.stdout != b"CANCEL\n":
        raise SystemExit(f"default profile: the window failed ({p.returncode}) {p.stderr[-400:]}")
    cp = json.load(open(st))
    prof = json.dumps(cp["profile"])
    pre = json.loads(json.dumps(cp["preset"]))
    pre["params"].update({"amt_Y": 55.0, "t_radius": 3, "bamt_Cb_high": 40.0, "mix_enable": True, "mix": 20.0})
    pre = json.dumps(pre)
    bad_prof, bad_pre = "{not json", json.dumps({"format": "not_a_preset"})
    BROKEN.update((bad_prof, bad_pre))
    vals = {"none": (None, None), "set": (prof, pre), "broken": (bad_prof, bad_pre), "badprof": (bad_prof, pre),
            "badpre": (prof, bad_pre)}
    for k, (a, b) in vals.items():
        path = ini_path(work, k)
        if os.path.exists(path):
            os.remove(path)
        if a is None and b is None:
            continue
        q = QSettings(path, QSettings.IniFormat)
        q.setValue("default_profile", a)
        q.setValue("default_preset", b)
        q.sync()
        del q
    DEFAULTS.update(vals)
    print(f"== defaults: profile from the window ({len(prof)} bytes, area {cp['profile']['base']['area']}), "
          f"preset {len(pre)} bytes; settings files {', '.join(k for k in vals if k != 'none')}")


def defaults_text(work, key):
    """what --ofx-default answers for the settings `key` (the driver, run directly): (status line, ClipParams)"""
    env = dict(os.environ, NV_UI_QT_SETTINGS=ini_path(work, key))
    p = subprocess.run([DRV, "--ofx-default"], stdin=subprocess.DEVNULL, capture_output=True, env=env, timeout=120)
    head, _, body = p.stdout.partition(b"\n")
    return head.decode(), body.decode()


def check_defaults(cp_text, key, profile=True, preset=True):
    """a ClipParams holds the defaults of the settings `key`: the stored profile (base, factors, tune) and the preset
    (Python's params_from_json of it; the factory settings when it is not loadable)"""
    m = py()
    state = m["state"]
    bad = []
    cp = json.loads(cp_text)
    ptext, qtext = DEFAULTS[key]
    loadable = lambda t: t is not None and t not in BROKEN
    if profile:
        if loadable(ptext):
            want = json.loads(ptext)
            got = cp.get("profile") or {}
            for k in ("base", "factors", "tune", "device"):
                if got.get(k) != want.get(k):
                    bad.append(f"profile {k} != the Default Profile")
        elif cp.get("profile") is not None:
            bad.append("a profile although the Default Profile is not loadable / not set")
    if preset:
        want = jsonable(state.params_from_json(json.loads(qtext))) if loadable(qtext) else jsonable(state.default_params())
        if cp["preset"]["params"] != want:
            diff = [k for k in want if cp["preset"]["params"].get(k) != want[k]]
            bad.append(f"preset != {'the Default Preset' if loadable(qtext) else 'the factory settings'} ({diff[:5]})")
    return bad


# ---------------------------------------------------------------- scenarios
def scenarios(quick, work):
    """name, clip, layout and steps: ("render", [t..]) / ("clip", text-spec) / ("range", [a, b]) /
    ("press", "prepare" | "adjust", t, script, expect) with expect = {"changed": bool, "adjust": 0 / 1, "spawn": bool,
    "build": check the profile build, "process": path of the driver's processing of frame t, "status": 0}"""
    P = os.path.join(work, "proc_")
    E = lambda **kw: dict({"changed": True, "adjust": 1, "spawn": True, "build": False, "process": None, "status": 0}, **kw)
    S = [
        # the user's path: no profile -> passthrough; Prepare (Auto Profile on the current frame, Apply); renders;
        # Adjust (settings changed, Apply); renders; Prepare + Cancel / close: nothing changes
        {"name": "W-prepare-adjust", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad",
         "steps": [("render", [3]),
                   ("press", "prepare", 4, f"auto;process {P}w1_4.f32;apply",
                    E(build=True, process=f"{P}w1_4.f32")),
                   ("render", [0, 4, 7]),
                   ("press", "adjust", 6, f"set amt_Y 45;set adv_threshold 3;set noise_Cb 80;set t_noise 70;"
                                         f"set bamt_Cb_high 50;set bnoise_Cr_low 150;set amt_Cr 60;"
                                         f"setb mix_enable 1;set mix 25;tune all 1.2;tune Y 0.9;"
                                         f"process {P}w1_6.f32;apply", E(process=f"{P}w1_6.f32")),
                   ("render", [6, 1, 5]),
                   ("press", "prepare", 2, "auto;set amt_Y 10;cancel", E(changed=False)),
                   ("press", "adjust", 2, "set amt_Y 10;close", E(changed=False)),
                   ("render", [6, 2])]},
        # Prepare on another frame with a time offset and odd layout; radius 4 window from the 11 session frames
        {"name": "W-offset", "clip": "A", "first": 100, "origin": [10, 20], "layout": "neg",
         "steps": [("press", "prepare", 103, f"auto;set t_radius 4;setb s_enable 0;process {P}w2.f32;apply",
                    E(build=True, process=f"{P}w2.f32")),
                   ("render", [103, 100, 107])]},
        # Apply without a profile: ClipParams set (filters only), Adjust stays disabled, passthrough; an Adjust press
        # while disabled opens nothing
        {"name": "W-noprofile", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad",
         "steps": [("press", "prepare", 1, "set amt_Y 30;apply", E(adjust=0)),
                   ("render", [1, 2]),
                   ("press", "adjust", 1, "apply", E(changed=False, adjust=0, spawn=False)),
                   ("press", "prepare", 1, "auto;apply", E()),
                   ("clip", ""), ("render", [3])]},
        # missing frames: the window gets the contiguous frames around t; the render's window ends at the gap
        {"name": "W-missing", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad", "missing": [1, 6],
         "steps": [("press", "prepare", 3, f"auto;process {P}w4.f32;apply", E(build=True, process=f"{P}w4.f32")),
                   ("render", [3, 4, 0, 7])]},
        # HDR / NaN / +-inf frames through the window
        {"name": "W-hdr-nan", "clip": "C", "first": 0, "origin": [0, 0], "layout": "pad",
         "steps": [("press", "prepare", 2, f"auto;process {P}w5.f32;apply", E(build=True, process=f"{P}w5.f32")),
                   ("render", [2, 0, 4])]},
        # synthetic states (the Python UI's functions): a saved project's value, host pastes, the N4b breadth
        {"name": "S-params", "clip": "A", "first": 100, "origin": [10, 20], "layout": "neg",
         "init": {"at": 0, "ui": {"t_radius": 1, "adv_threshold": 3, "noise_Y": 120, "noise_Cb": 80, "noise_Cr": 90,
                                  "amt_Y": 50, "amt_Cb": 60, "amt_Cr": 60, "adv_spatial": 1.5, "adv_noise_level": 110}},
         "steps": [("render", [100, 103]),
                   ("clip", {"at": 4, "aft": False, "ui": {"t_radius": 3, "t_noise": 60}}), ("render", [107, 102]),
                   ("clip", {"at": 1, "mode": "spatial", "matrix": "bt2020",
                             "ui": {"bamt_Cb_high": 40, "bnoise_Y_low": 130, "amt_Cr": 30}}), ("render", [101, 105]),
                   ("clip", {"at": 2, "mode": "temporal", "matrix": "bt709",
                             "ui": {"mix_enable": True, "mix": 30, "s_enable": False}}), ("render", [104]),
                   ("clip", {"at": 5, "ui": {"t_enable": False, "adv_similarity": "pyramid", "t_amount": 60},
                             "tune": {"all": 1.3, "Cb": 0.7}}), ("render", [106, 107]),
                   ("clip", {"at": 0, "matrix": "bt601", "ui": {"t_radius": 5}}), ("render", [100])]},
        {"name": "S-hdr-nan", "clip": "C", "first": 0, "origin": [0, 0], "layout": "pad",
         "init": {"at": 0},
         "steps": [("render", [0, 2, 4]), ("clip", {"at": 1, "mode": "temporal", "ui": {"t_radius": 4}}),
                   ("render", [2])]},
        {"name": "S-missing", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad", "missing": [2, 5],
         "init": {"at": 0},
         "steps": [("render", [3, 4, 6]), ("clip", {"at": 0, "ui": {"t_radius": 3, "mix_enable": True, "mix": 70}}),
                   ("render", [1, 7]), ("range", [1, 6]), ("render", [1, 6])]},
        # GetFramesNeeded + InstanceChanged (ClipParams) from inside every clipGetImage of a render (no lock held
        # across host calls)
        {"name": "S-reenter", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad", "reenter": True,
         "init": {"at": 0}, "steps": [("render", [0, 4, 7])]},
    ]
    # INT-4: interlaced clips.  The window's Video Scan Type (ClipParams "scan" / render.P["interlace"]) decides; the
    # host's field order is the window's default only.  A (640x360): OfxFieldUpper = top field first, Lower = bottom.
    il = lambda first, mode, R: {"first": first, "mode": mode, "radius": R}
    S += [
        # synthetic interlaced states: same R4 / shift R3 / plain R2 + Mix, then a progressive state on the fielded
        # clip (processed as full frames, with the warning)
        {"name": "I-synth", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad", "field_order": "OfxFieldUpper",
         "init": {"at": 0, "interlace": il(0, "same", 4)},
         "steps": [("render", [0, 3, 7]),
                   ("clip", {"at": 2, "aft": False, "interlace": il(1, "shift", 3), "parity": 1}), ("render", [4]),
                   ("clip", {"at": 1, "interlace": il(0, "plain", 2), "ui": {"mix_enable": True, "mix": 30,
                                                                            "amt_Cb": 60}}), ("render", [2, 6]),
                   ("clip", {"at": 0}), ("render", [5])]},
        # the host gives no field order: the state still says interlaced (the user's choice)
        {"name": "I-synth-none", "clip": "A", "first": 100, "origin": [10, 20], "layout": "neg",
         "init": {"at": 3, "interlace": il(1, "same", 2), "ui": {"t_noise": 60}}, "steps": [("render", [101, 106])]},
        # the host renders one field: that field's rows only
        {"name": "I-field-lower", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad",
         "field_order": "OfxFieldUpper", "field_to_render": "lower", "init": {"at": 0, "interlace": il(0, "same", 4)},
         "steps": [("render", [3, 0])]},
        {"name": "I-field-upper", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad",
         "field_order": "OfxFieldUpper", "field_to_render": "upper", "init": {"at": 0, "interlace": il(0, "same", 4)},
         "steps": [("render", [3])]},
        # odd height (359 rows): the field rows and the bottom field's pad row; FieldToRender upper / lower / both
        {"name": "I-odd-upper", "clip": "O", "first": 0, "origin": [0, 0], "layout": "pad", "field_order": "OfxFieldUpper",
         "field_to_render": "upper", "init": {"at": 0, "interlace": il(1, "same", 2)}, "steps": [("render", [3, 7])]},
        {"name": "I-odd-lower", "clip": "O", "first": 0, "origin": [0, 0], "layout": "neg", "field_order": "OfxFieldLower",
         "field_to_render": "lower", "init": {"at": 1, "interlace": il(0, "shift", 3)}, "steps": [("render", [2])]},
        {"name": "I-odd-both", "clip": "O", "first": 0, "origin": [0, 0], "layout": "pad", "field_order": "OfxFieldLower",
         "init": {"at": 0, "interlace": il(0, "plain", 2)}, "steps": [("render", [4, 0])]},
        # Round 2 (M-1): a new instance with the Default Profile / Preset on a fielded clip: the window keeps the host's
        # order (the defaults' ClipParams, frame_size null, say nothing about the scan)
        {"name": "I-default-host", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad", "ini": "set",
         "field_order": "OfxFieldLower",
         "steps": [("render", [3]),
                   ("press", "prepare", 4, f"process {P}i4.f32;apply",
                    E(process=f"{P}i4.f32", scan="bff", fields=None)),
                   ("render", [4])]},
        # the window on a fielded clip: its scan starts from the host's order (Lower: bottom first), Two fields profile
        {"name": "I-press-host", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad", "field_order": "OfxFieldLower",
         "steps": [("press", "prepare", 4, f"auto;process {P}i1.f32;apply",
                    E(build=True, process=f"{P}i1.f32", scan="bff", fields="two")),
                   ("render", [4, 1])]},
        # the user sets the scan in the window (no field order from the host): top first, One field (bottom); then a
        # change back to Progressive in the window resets the profile (Cancel: the instance keeps its state)
        {"name": "I-press-user", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad",
         "steps": [("press", "prepare", 3, f"scan 0;fprof one1;auto;process {P}i2.f32;apply",
                    E(build=True, process=f"{P}i2.f32", scan="tff", fields="one")),
                   ("render", [3]),
                   ("press", "adjust", 3, f"process {P}i3.f32;scan -1;cancel", E(changed=False, process=f"{P}i3.f32")),
                   ("render", [3, 5])]},
    ]
    # the Default Profile / Preset (settings files make_defaults wrote)
    S += [
        # set: a new instance renders with the defaults at once (not stored yet), they are stored at SyncPrivateData;
        # the window then restores them (the C++ UI's processing of the same state)
        {"name": "D-set", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad", "ini": "set",
         "defaults_status": ["ok", "ok"], "create_message": "message=\"\"",
         "steps": [("render", [3, 0]), ("sync",), ("render", [7]), ("sync",),
                   ("press", "adjust", 5, f"process {P}d1.f32;cancel", E(changed=False, process=f"{P}d1.f32")),
                   ("render", [5])]},
        # not set: nothing stored, passthrough
        {"name": "D-none", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad", "ini": "none",
         "defaults_status": ["none", "none"], "create_message": "message=\"\"",
         "steps": [("render", [3]), ("sync",), ("render", [4])]},
        # broken files: the genuine's messages, nothing stored, passthrough
        {"name": "D-broken", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad", "ini": "broken",
         "defaults_status": ["error", "error"],
         "create_message": "Selected default profile cannot be loaded. Selected default preset cannot be loaded.",
         "steps": [("render", [3]), ("sync",), ("render", [4])]},
        {"name": "D-badprof", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad", "ini": "badprof",
         "defaults_status": ["error", "ok"], "create_message": "Selected default profile cannot be loaded.",
         "steps": [("render", [3]), ("sync",)]},
        {"name": "D-badpre", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad", "ini": "badpre",
         "defaults_status": ["ok", "error"], "create_message": "Selected default preset cannot be loaded.",
         "steps": [("render", [3]), ("sync",), ("render", [2])]},
        # the window opened on an empty ClipParams loads the defaults assigned since; Reset Profile / Reset Filter
        # Settings in the window return to them; a later change of the defaults leaves the instance as it is
        {"name": "D-window", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad", "ini": "none",
         "steps": [("render", [4]), ("ini", "set"),
                   ("press", "prepare", 4, f"process {P}d2.f32;apply", E(defaults="set", process=f"{P}d2.f32")),
                   ("render", [4]),
                   # (the reset state is the one stored before: Apply stores the same text)
                   ("press", "adjust", 4, f"auto;set amt_Y 20;tune all 1.4;resetprofile;resetfilter;process {P}d3.f32;apply",
                    E(defaults="set", process=f"{P}d3.f32", changed=False, applied=True)),
                   ("render", [4, 6]),
                   ("ini", "broken"), ("sync",),
                   ("press", "adjust", 6, f"process {P}d4.f32;cancel", E(changed=False, process=f"{P}d4.f32")),
                   ("render", [6]),
                   ("press", "prepare", 2, "resetprofile;resetfilter;apply", E(adjust=0, defaults="broken")),
                   ("render", [2])]},
        # the host refuses the first store of the defaults (L4): they stay in use and are stored at the next sync
        {"name": "D-failset", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad", "ini": "set",
         "steps": [("render", [3]), ("failset", 1), ("sync",), ("render", [4]), ("sync",), ("render", [5])]},
        # the whole session on a 512 KB thread (a host worker's stack): window, defaults, renders
        {"name": "D-stack512", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad", "ini": "set", "stack": 512,
         "steps": [("render", [3]), ("sync",),
                   ("press", "adjust", 4, f"set amt_Y 45;process {P}st.f32;apply", E(process=f"{P}st.f32")),
                   ("render", [4, 0])]},
        # an existing instance (a saved project's ClipParams) with the defaults set: no defaults asked for or stored
        {"name": "D-existing", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad", "ini": "set",
         "init": {"at": 2, "ui": {"amt_Y": 35}}, "steps": [("render", [3]), ("sync",), ("render", [5])]},
    ]
    if not quick:
        S += [
            {"name": "S-B-r5", "clip": "B", "first": 0, "origin": [0, 0], "layout": "pad",
             "init": {"at": 0, "matrix": "bt709", "ui": {"t_radius": 5}}, "steps": [("render", [0, 3, 6])]},
            {"name": "W-D-1080p", "clip": "D", "first": 0, "origin": [0, 0], "layout": "neg",
             "steps": [("press", "prepare", 0, f"auto;process {P}wD.f32;apply", E(build=True, process=f"{P}wD.f32")),
                       ("render", [0, 1])]},
        ]
    return S


# ---------------------------------------------------------------- comparison
def renders(log):
    return [ln for ln in log.splitlines() if ln.startswith("render ")]


def compare(got, exp, clip, times):
    shape = (len(times), clip["H"], clip["W"], 4)
    g = np.fromfile(got, "<u4").reshape(shape)
    e = np.fromfile(exp, "<u4").reshape(shape)
    return [(t, int(np.count_nonzero(g[i] != e[i])), g[i].size) for i, t in enumerate(times)]


def fsguard_calls(log):
    m = re.search(r"fsguard: (\d+) file/process call", log)
    return (int(m.group(1)) if m else -1), re.findall(r"fsguard:   (.*)", log)


def field(line, key):
    m = re.search(key + r"=(\S+)", line)
    return m.group(1) if m else None


def run_scenario(a, work, sc, clips, texts, fails, totals, keep=None):
    clip = clips[sc["clip"]]
    times = [t for st in sc["steps"] if st[0] == "render" for t in st[1]]
    out = os.path.join(work, f"plugin_{sc['name']}.f32")
    log, err, cps, dt = run_plugin(work, sc, clip, out, texts)
    print(f"== {sc['name']}: clip {sc['clip']} {clip['W']}x{clip['H']}x{clip['N']}, first {sc['first']}, "
          f"origin {sc['origin']}, layout {sc['layout']}  (plug-in {dt:.1f} s)")
    for ln in log.splitlines():
        if ln.startswith(("render ", "press ", "clip_params ", "create instance")):
            print("   ", ln[:400])
    rl = renders(log)
    if not (all(" status=0 " in ln and " declared=1 " in ln for ln in rl) and len(rl) == len(times)):
        fails.append(f"{sc['name']}: render status / undeclared frames")
    if sc["name"] == "I-synth" and not (rl and "the host reports interlaced material" in rl[-1] and
                                        all("host reports" not in ln for ln in rl[:-1])):
        fails.append(f"{sc['name']}: the progressive state on the fielded clip lacks the warning (or others have it)")
    # Adjust Filter Settings enabled iff ClipParams holds a profile: at CreateInstance (a saved project's value) and
    # after the host sets ClipParams (presses are checked below)
    has = lambda text: bool(text) and json.loads(text).get("render") is not None
    init_text = texts.get(("value", id(sc.get("init")))) if sc.get("init") is not None else ""
    pending = ""
    if sc.get("init") is None:
        # a new instance: the plug-in asks for the defaults (the same answer the driver gives here directly)
        dhead, pending = defaults_text(work, sc.get("ini", "none"))
        print(f"    defaults of {sc.get('ini', 'none')}: {dhead}")
        if sc.get("defaults_status") and dhead.split()[2:] != sc["defaults_status"]:
            fails.append(f"{sc['name']}: --ofx-default answered {dhead}, expected {sc['defaults_status']}")
        if pending and check_defaults(pending, sc.get("ini", "none")):
            fails.append(f"{sc['name']}: defaults {check_defaults(pending, sc.get('ini', 'none'))}")
    ci = [ln for ln in log.splitlines() if ln.startswith("create instance")]
    host_sets = [ln for ln in log.splitlines() if ln.startswith("clip_params set by the host")]
    want_sets = [texts[("value", id(st[1]))] for st in sc["steps"] if st[0] == "clip"]
    if sc.get("create_message") is not None and sc["create_message"] not in (ci[0] if ci else ""):
        fails.append(f"{sc['name']}: CreateInstance message {ci}")
    if not (ci and field(ci[0], "adjust_enabled") == str(int(has(init_text or pending))) and
            len(host_sets) == len(want_sets) and
            all(field(ln, "adjust_enabled") == str(int(has(t))) for ln, t in zip(host_sets, want_sets))):
        fails.append(f"{sc['name']}: Adjust Filter Settings enabled state {ci} {host_sets}")
    # presses: outcome, Adjust state, ClipParams checks, the driver's processing
    presses = [ln for ln in log.splitlines() if ln.startswith(("press ", "sync "))]
    pst = [st for st in sc["steps"] if st[0] in ("press", "sync")]
    k_press, spawns = 0, []
    prev = init_text or pending
    ref_steps = [["clip", prev]] if prev else []
    helper = sc.get("helper", DRV)
    if sc.get("init") is None:
        spawns.append(f"posix_spawn({helper} --ofx-default)")
    pend = bool(pending)
    fail_next = 0
    for st in sc["steps"]:
        if st[0] == "failset":
            fail_next = st[1]
            continue
        if st[0] == "sync" and fail_next:
            # the host refuses the store: nothing stored, the undo block closed, the defaults kept pending
            ln, cp = presses[k_press], cps[k_press]
            k_press += 1
            fail_next -= 1
            ok = ln.startswith("sync status=0 ") and cp == "" and " edit_depth=0 " in ln and \
                "clip_params=unchanged" in ln and "could not be stored in the instance yet" in ln and pend
            print(f"    sync with the store refused: {'ok (kept pending)' if ok else 'FAIL'}")
            if not ok:
                fails.append(f"{sc['name']} refused sync: {ln}")
            continue
        if st[0] == "sync":
            # SyncPrivateData: the defaults stored in ClipParams once (only while it is empty)
            ln, cp = presses[k_press], cps[k_press]
            k_press += 1
            want_cp = pending if pend else prev
            ok = ln.startswith("sync status=0 ") and cp == want_cp and " edit_depth=0 " in ln and \
                "could not be stored" not in ln and \
                (field(ln, "clip_params") == "changed") == (pend and cp != "") and field(ln, "adjust_enabled") == str(int(has(cp)))
            print(f"    sync: ClipParams {'= the defaults asked for at CreateInstance' if pend else 'unchanged'} "
                  f"({len(cp)} bytes): {'ok' if ok else 'FAIL'}")
            if not ok:
                fails.append(f"{sc['name']} sync: {ln}")
            pend = False
            prev = cp
            continue
        if st[0] == "ini":
            continue
        if st[0] == "clip":
            prev = texts[("value", id(st[1]))]
            ref_steps.append(["clip", prev])
        elif st[0] == "range":
            ref_steps.append(["range", st[1]])
        elif st[0] == "render":
            ref_steps.append(["render", st[1]])
        elif st[0] == "press":
            ln, exp, cp = presses[k_press], st[4], cps[k_press]
            k_press += 1
            was_pend = pend and bool(pending)   # the defaults are stored at the press's InstanceChanged first
            pend = False
            if exp["spawn"]:
                spawns.append(f"posix_spawn({helper} --ofx-session)")
            changed = field(ln, "clip_params") == "changed"
            applied = exp.get("applied", exp["changed"])     # Apply stored a ClipParams (maybe the same text)
            blocks = int(applied) + int(was_pend)
            ok = (field(ln, "status") == str(exp["status"]) and changed == (exp["changed"] or was_pend) and
                  field(ln, "edit_blocks") == str(blocks) and
                  field(ln, "adjust_enabled") == str(exp["adjust"]) and (cp != prev) == exp["changed"] and
                  field(ln, "released") == field(ln, "images"))
            if applied:
                hv = [int(x) for x in field(ln, "hash").split(",")]
                ok = ok and "outside paramEditBegin" not in ln and hv[2] == len(cp.encode()) and hv[0] != 0
            if not ok:
                fails.append(f"{sc['name']} press {st[1]}@{st[2]}: outcome {ln}")
            if applied:
                fetched = [float(x) for x in re.search(r"fetched=\[([^\]]*)\]", ln).group(1).split()]
                bad = check_cp(cp, clip, sc["first"], float(st[2]), fetched, exp["build"])
                print(f"    ClipParams of press {st[1]}@{st[2]} ({len(cp)} bytes): "
                      f"{'checks ok' if not bad else bad}{' (incl. profile build)' if exp['build'] else ''}")
                if exp.get("defaults"):
                    bad2 = check_defaults(cp, exp["defaults"], **exp.get("defaults_parts", {}))
                    print(f"    ... holds the defaults of {exp['defaults']}: {'yes' if not bad2 else bad2}")
                    if bad2:
                        fails.append(f"{sc['name']} press {st[1]}@{st[2]}: {bad2}")
                if exp.get("scan") is not None:   # INT-4: the window's Video Scan Type and field profile
                    c = json.loads(cp)
                    got = (c.get("scan"), c["profile"]["base"].get("fields") if c.get("profile") else None)
                    print(f"    ... scan / profile fields: {got} (expected {(exp['scan'], exp.get('fields'))})")
                    if got != (exp["scan"], exp.get("fields")):
                        bad = bad + [f"scan / fields {got}"]
                if bad:
                    fails.append(f"{sc['name']} press {st[1]}@{st[2]}: {bad}")
            prev = cp
            ref_steps.append(["clip", cp])
            if exp["process"]:
                ref_steps.append(["process", exp["process"], st[2]])
    if k_press != len(pst):
        fails.append(f"{sc['name']}: {k_press} press / sync lines for {len(pst)} steps")
    nfs, calls = fsguard_calls(log)
    want = [f"dlopen({BUNDLE}/Contents/MacOS/MeatVideo.ofx)"] + spawns
    print(f"    fsguard: {nfs} call(s) while armed: {calls}")
    if sorted(calls) != sorted(want) or nfs != len(want):
        fails.append(f"{sc['name']}: file / process calls {calls} (expected {want})")
    if "leaked source images: 0" not in log:
        fails.append(f"{sc['name']}: source images not released")
    # the C++ UI's processing (driver "process") against the plug-in's render of the same frame and state: the first
    # render of that time after the press
    rsteps = [s for s in ref_steps if s[0] != "process"]
    pending, pairs, cnt = {}, [], 0
    for s in ref_steps:
        if s[0] == "process":
            pending[s[2]] = s[1]
        elif s[0] == "render":
            for tt in s[1]:
                if tt in pending:
                    pairs.append((tt, cnt, pending.pop(tt)))
                cnt += 1
    for t in pending:
        fails.append(f"{sc['name']}: no render after the processing check at t={t}")
    for t, pos, path in pairs:
        if not os.path.exists(path):
            fails.append(f"{sc['name']}: no driver output {path}")
            continue
        g = np.fromfile(out, "<u4").reshape(len(times), clip["H"], clip["W"], 4)[pos]
        q = np.fromfile(path, "<u4").reshape(3, clip["H"], clip["W"])       # G, B, R
        bad = int(np.count_nonzero(np.stack([q[2], q[0], q[1]], -1) != g[..., :3]))
        print(f"    t={t:<4} qtui   {'MATCH' if bad == 0 else 'DIFF '} {bad}/{q.size} words differ "
              f"(the C++ UI's processing in the window vs the plug-in's render)")
        totals[0] += bad
        totals[1] += q.size
        totals[2] += 1
        if bad:
            fails.append(f"{sc['name']} t={t} qtui: {bad} words")
    for native in ([True, False] if a.pure else [True]):
        tag = "native" if native else "pure"
        exp, rdt = run_reference(work, sc["name"], sc, clip, rsteps, native, tag)
        info = json.load(open(exp + ".json"))
        for k, ((t, bad, words), inf) in enumerate(zip(compare(out, exp, clip, times), info)):
            if inf["passthrough"] and "no noise profile yet; the source is passed through" not in rl[k]:
                fails.append(f"{sc['name']} t={t}: passthrough without the message")
            desc = "passthrough" if inf["passthrough"] else \
                f"window {inf['window']}, {inf['matrix']}, area {inf['area']}, uniform {inf['uniform']}"
            print(f"    t={t:<4} {tag:<6} {'MATCH' if bad == 0 else 'DIFF '} {bad}/{words} words differ  ({desc})")
            totals[0] += bad
            totals[1] += words
            totals[2] += 1
            if bad:
                fails.append(f"{sc['name']} t={t} {tag}: {bad} words")
        print(f"    reference {tag}: {rdt:.1f} s")
    if keep is not None:
        keep[sc["name"]] = {"out": out, "times": times, "rsteps": rsteps, "log": log}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--pure", action="store_true")
    ap.add_argument("--mutate", action="store_true")
    ap.add_argument("--only", default="")
    ap.add_argument("--work", default=os.path.join(BUILD, "verify"))
    ap.add_argument("--reference", nargs=2)
    a = ap.parse_args()
    if a.reference:
        return reference(*a.reference)
    os.makedirs(a.work, exist_ok=True)
    ini0 = sha(USER_INI)
    fails = []
    # the plug-in must load the libnvcore inside its bundle (minihost does not link one)
    p = subprocess.run([MINIHOST, BUNDLE, "--check"], env=dict(os.environ, DYLD_PRINT_LIBRARIES="1"),
                       capture_output=True, text=True)
    libs = [ln.split()[-1] for ln in p.stderr.splitlines() if "libnvcore" in ln]
    print(f"== structure check: {'CHECK OK' if 'CHECK OK' in p.stdout else 'CHECK FAILED'}; libnvcore loaded: {libs}")
    own = os.path.realpath(os.path.join(BUNDLE, "Contents", "Frameworks", "libnvcore.dylib"))
    if not (p.returncode == 0 and len(libs) == 1 and os.path.realpath(libs[0]) == own):
        fails.append("structure check / libnvcore of the bundle")
    # positive control of the monitor: accesses from the host image must be recorded
    p = subprocess.run([MINIHOST, "--fsguard-selftest"], env=dict(os.environ, DYLD_INSERT_LIBRARIES=FSGUARD),
                       capture_output=True, text=True)
    nfs, calls = fsguard_calls(p.stdout)
    print(f"== fsguard positive control: exit {p.returncode}, {nfs} calls recorded {calls}")
    if p.returncode != 0 or nfs < 5:
        fails.append("fsguard positive control")
    clips = make_clips(a.work, a.quick)
    make_defaults(a.work, clips["A"])
    S = [s for s in scenarios(a.quick, a.work) if not a.only or re.search(a.only, s["name"])]
    # the synthetic ClipParams (texts[id(spec)] = file, texts[("value", id(spec))] = text)
    texts = {}
    for sc in S:
        specs = ([sc["init"]] if sc.get("init") is not None else []) + [st[1] for st in sc["steps"] if st[0] == "clip"]
        for spec in specs:
            val = spec if isinstance(spec, str) else synth_cp(clips[sc["clip"]], spec)
            path = os.path.join(a.work, f"cp_{sc['name']}_{len(texts)}.json")
            open(path, "w").write(val)
            texts[id(spec)] = path
            texts[("value", id(spec))] = val
    totals = [0, 0, 0]
    keep = {}
    for sc in S:
        run_scenario(a, a.work, sc, clips, texts, fails, totals, keep)
    refusal_checks(a, clips, texts, fails)
    helper_failures(a, clips, fails)
    bundled_helper(a, clips, fails)
    bundled_data(a, fails)
    closed_fds(a, clips, fails)
    if a.mutate:
        mutations(a, clips, keep, fails)
    ini1 = sha(USER_INI)
    print(f"== {USER_INI}: sha256 before {ini0}, after {ini1}: {'unchanged' if ini0 == ini1 else 'CHANGED'}")
    if ini0 != ini1:
        fails.append("the user's settings file changed")
    print(f"\n{totals[2]} comparisons, {totals[0]}/{totals[1]} words differ")
    print("ALL MATCH" if not fails else "FAILURES:\n  " + "\n  ".join(fails))
    return 0 if not fails else 1


def refusal_checks(a, clips, texts, fails):
    """renders that must fail with kOfxStatFailed (1) and a message (a profile in ClipParams)"""
    sc0 = {"name": "refuse", "first": 0, "origin": [0, 0], "layout": "pad"}
    spec = {"at": 0}
    val = synth_cp(clips["R96"], spec)
    path = os.path.join(a.work, "cp_refuse.json")
    open(path, "w").write(val)
    texts[id(spec)] = path
    cases = [
        ("below 64", {"clip": "R63"}, "below 64"),
        ("8-bit image", {"clip": "R96", "depth": "byte"}, "unsupported image format"),
        ("16-bit image", {"clip": "R96", "depth": "short"}, "unsupported image format"),
        ("current frame missing", {"clip": "R96", "missing": [1]}, "no image at frame 1"),
    ]
    for label, extra, needle in cases:
        sc = dict(sc0, init=spec, steps=[("render", [1])], **extra)
        log, _, _, _ = run_plugin(a.work, sc, clips[sc["clip"]], os.path.join(a.work, "plugin_refuse.f32"), texts)
        rl = renders(log)
        nfs, _ = fsguard_calls(log)
        ok = len(rl) == 1 and " status=1 " in rl[0] and needle in rl[0] and "OfxMessageError" in rl[0] and nfs == 1
        print(f"== refusal: {label}: {'ok' if ok else 'FAIL'}  {rl}")
        if not ok:
            fails.append(f"refusal {label}")
    # a ClipParams the render cannot read: refused with a message, the source not written
    bad = {"raw": "{\"format\": \"meatvideo_ofx_clip_params\", \"version\": 3}"}
    path = os.path.join(a.work, "cp_badversion.json")
    open(path, "w").write(bad["raw"])
    texts[id(bad)] = path
    sc = dict(sc0, clip="R96", init=bad, steps=[("render", [1])])
    log, _, _, _ = run_plugin(a.work, sc, clips["R96"], os.path.join(a.work, "plugin_refuse.f32"), texts)
    rl = renders(log)
    ok = len(rl) == 1 and " status=1 " in rl[0] and "version 3 is not supported (1, 2)" in rl[0]
    print(f"== refusal: unreadable ClipParams: {'ok' if ok else 'FAIL'}  {rl}")
    if not ok:
        fails.append("refusal unreadable ClipParams")
    # INT-4: version 1 / 2 and the interlaced keys go together (a version-1 text with "scan" or render.P["interlace"],
    # a version-2 text without them, "first" against "scan")
    base1 = json.loads(val)
    v2 = json.loads(synth_cp(clips["R96"], {"at": 0, "aft": False, "interlace": {"first": 0, "mode": "same", "radius": 2}}))
    cases = []
    d = copy.deepcopy(base1); d["scan"] = "tff"; cases.append(("version 1 with scan", d, "version 1 has no \"scan\""))
    d = copy.deepcopy(base1); d["render"]["P"]["interlace"] = {"first": 0, "mode": "same", "radius": 2}
    cases.append(("version 1 with interlace", d, "goes with version 2 only"))
    d = copy.deepcopy(v2); del d["scan"]; cases.append(("version 2 without scan", d, "version 2 needs \"scan\""))
    d = copy.deepcopy(v2); del d["render"]["P"]["interlace"]; cases.append(("version 2 without interlace", d, "goes with version 2 only"))
    d = copy.deepcopy(v2); d["scan"] = "bff"; cases.append(("version 2 first != scan", d, "disagrees with \"scan\""))
    for label, d, needle in cases:
        spec = {"raw": json.dumps(d)}
        path = os.path.join(a.work, "cp_v2case.json")
        open(path, "w").write(spec["raw"])
        texts[id(spec)] = path
        sc = dict(sc0, clip="R96", init=spec, steps=[("render", [1])])
        log, _, _, _ = run_plugin(a.work, sc, clips["R96"], os.path.join(a.work, "plugin_refuse_v2.f32"), texts)
        rl = renders(log)
        ok = len(rl) == 1 and " status=1 " in rl[0] and needle in rl[0]
        print(f"== refusal: {label}: {'ok' if ok else 'FAIL'}  {rl}")
        if not ok:
            fails.append(f"refusal {label}")
    # ClipParams the render must refuse with a message (on a 512 KB thread, the host may call from a worker): not
    # JSON (L5: no libstdc++ look-up recorded), nested too deep (M1: no stack overflow), values outside what the UI
    # writes (L6).  At CreateInstance (a saved project) and set by the host before a render.
    pre = '{"format":"meatvideo_ofx_clip_params","version":1,"x":'
    base = json.loads(val)

    def edited(f):
        d = copy.deepcopy(base)
        f(d)
        return json.dumps(d)
    bad_cases = [
        ("not JSON", "{not json", "is not valid JSON"),
        ("nested 256 + 1 levels", pre + "[" * 256 + "]" * 256 + "}", "nesting deeper than 256 levels"),
        ("nested 100000 levels", pre + "[" * 100000 + "]" * 100000 + "}", "nesting deeper than 256 levels"),
        ("300000 open brackets", "[" * 300000, "nesting deeper than 256 levels"),
        ("objects nested 100000 levels", '{"a":' * 100000 + "1" + "}" * 100000, "nesting deeper than 256 levels"),
        ("bs as a string", edited(lambda d: d["render"]["P"].__setitem__("bs", "16")), 'P["bs"] must be an integer'),
        ("search -50", edited(lambda d: d["render"]["P"].__setitem__("search", -50)), 'P["search"] must be an integer'),
        ("radius 2.5", edited(lambda d: d["render"]["P"].__setitem__("radius", 2.5)), 'P["radius"] must be an integer'),
        ("threshold NaN", edited(lambda d: d["render"]["P"].__setitem__("threshold", float("nan"))),
         'P["threshold"] is not a finite number'),
        ("mix 1.5", edited(lambda d: d["render"]["P"].__setitem__("mix", 1.5)), 'P["mix"] = 1.5 is outside [0.0, 1.0]'),
        ("mix 1.0000001", edited(lambda d: d["render"]["P"].__setitem__("mix", 1.0000001)),
         'P["mix"] = 1.0000001 is outside [0.0, 1.0]'),
        ("frame_size 1e300 (informative; accepted)", edited(lambda d: d.__setitem__("frame_size", [1e300, -1e300])),
         None),
    ]
    for label, text, needle in bad_cases:
        spec = {"raw": label}
        path = os.path.join(a.work, "cp_bad.json")
        open(path, "w").write(text)
        texts[id(spec)] = path
        sc = dict(sc0, clip="R96", init=spec, stack=512, steps=[("render", [1]), ("clip", spec), ("render", [2])])
        log, _, _, _ = run_plugin(a.work, sc, clips["R96"], os.path.join(a.work, "plugin_refuse.f32"), texts)
        rl = renders(log)
        ci = [ln for ln in log.splitlines() if ln.startswith("create instance")]
        nfs, calls = fsguard_calls(log)
        if needle is None:                  # accepted: rendered as the unchanged state
            ok = (len(rl) == 2 and all(" status=0 " in ln for ln in rl) and ci and "adjust_enabled=1" in ci[0] and
                  nfs == 1 and "session thread stack: 512 KB" in log)
        else:
            ok = (len(rl) == 2 and all(" status=1 " in ln and needle in ln for ln in rl) and ci and
                  "adjust_enabled=0" in ci[0] and needle in ci[0] and nfs == 1 and "session thread stack: 512 KB" in log)
        print(f"== refusal (512 KB thread): {label} ({len(text)} bytes): {'ok' if ok else 'FAIL'}  "
              f"{rl[0][rl[0].find('message='):][:120] if rl else log[-300:]}  fsguard {calls}")
        if not ok:
            fails.append(f"refusal {label}")
    # the deepest accepted nesting (256 levels) on a 128 KB thread: read (no render section: passed through)
    spec = {"raw": "256"}
    path = os.path.join(a.work, "cp_deep256.json")
    open(path, "w").write(pre + "[" * 255 + "]" * 255 + "}")
    texts[id(spec)] = path
    sc = dict(sc0, clip="R96", init=spec, stack=128, steps=[("render", [1])])
    log, _, _, _ = run_plugin(a.work, sc, clips["R96"], os.path.join(a.work, "plugin_refuse.f32"), texts)
    rl = renders(log)
    ok = len(rl) == 1 and " status=0 " in rl[0] and "no noise profile yet" in rl[0] and "session thread stack: 128 KB" in log
    print(f"== nesting of exactly 256 levels on a 128 KB thread: {'ok (read, passed through)' if ok else 'FAIL'}")
    if not ok:
        fails.append("nesting of 256 levels")
    # the window on an instance whose ClipParams cannot be read: it starts from the defaults, its Apply replaces it
    spec = {"raw": "deep"}
    path = os.path.join(a.work, "cp_deepwin.json")
    open(path, "w").write(pre + "[" * 100000 + "]" * 100000 + "}")
    texts[id(spec)] = path
    sc = dict(sc0, clip="A", init=spec, stack=512,
              steps=[("press", "prepare", 3, "auto;apply", None), ("render", [3])])
    log, err, cps, _ = run_plugin(a.work, sc, clips["A"], os.path.join(a.work, "plugin_refuse.f32"), texts)
    pl = [ln for ln in log.splitlines() if ln.startswith("press ")]
    rl = renders(log)
    ok = (len(pl) == 1 and " status=0 " in pl[0] and "clip_params=changed" in pl[0] and "adjust_enabled=1" in pl[0] and
          len(rl) == 1 and " status=0 " in rl[0] and cps and json.loads(cps[0]).get("render") is not None)
    print(f"== the window on an unreadable (deep) ClipParams: {'ok (started from the defaults, Apply stored)' if ok else 'FAIL'}")
    if not ok:
        fails.append("window on deep ClipParams")


def helper_failures(a, clips, fails):
    """the window helper missing / failing / replying nonsense / returning unusable settings: ClipParams unchanged,
    an error message, the host not killed (no SIGPIPE)"""
    fake = os.path.join(a.work, "fake_helpers")
    os.makedirs(fake, exist_ok=True)
    scripts = {
        "garbage": "#!/bin/sh\ncat > /dev/null\nprintf 'HELLO\\n'\n",
        "badparams": "#!/bin/sh\ncat > /dev/null\nprintf 'APPLY 3\\nabc'\n",
        "exit3": "#!/bin/sh\ncat > /dev/null\nexit 3\n",
        "early": "#!/bin/sh\nexit 0\n",
        "short": "#!/bin/sh\ncat > /dev/null\nprintf 'APPLY 10\\nabc'\n",
        # a complete reply, then a process that keeps the reply pipe open for 30 s (as a process the host forked
        # meanwhile would): the plug-in must return at once (L3: the reply is read up to its own length)
        "holder": "#!/bin/sh\ncat > /dev/null\nprintf 'CANCEL\\n'\n(sleep 30 2> /dev/null &)\nexit 0\n",
    }
    for k, body in scripts.items():
        p = os.path.join(fake, k)
        open(p, "w").write(body)
        os.chmod(p, 0o755)
    cases = [("missing helper", os.path.join(fake, "does-not-exist"), "cannot start the window helper"),
             ("unexpected reply", os.path.join(fake, "garbage"), "unexpected reply"),
             ("unusable settings", os.path.join(fake, "badparams"), "settings the render cannot use"),
             ("exit status 3", os.path.join(fake, "exit3"), "exited with status 3"),
             ("exits before reading", os.path.join(fake, "early"), "unexpected reply"),
             ("truncated reply", os.path.join(fake, "short"), "unexpected reply")]
    sc = {"name": "helperfail", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad",
          "helper": os.path.join(fake, "holder"), "init": None, "steps": [("press", "prepare", 2, "x", None)]}
    t0 = time.time()
    log, _, cps, _ = run_plugin(a.work, sc, clips["A"], os.path.join(a.work, "plugin_helperfail.f32"), {})
    dt = time.time() - t0
    pl = [ln for ln in log.splitlines() if ln.startswith("press ")]
    ok = len(pl) == 1 and " status=0 " in pl[0] and "clip_params=unchanged" in pl[0] and dt < 15
    print(f"== helper keeps a child holding the reply pipe: {'ok' if ok else 'FAIL'}: the press returned after {dt:.1f} s")
    if not ok:
        fails.append("helper with a stray holder of the reply pipe")
    for label, helper, needle in cases:
        sc = {"name": "helperfail", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad", "helper": helper,
              "steps": [("press", "prepare", 2, "apply", None), ("render", [2])]}
        log, _, cps, _ = run_plugin(a.work, sc, clips["A"], os.path.join(a.work, "plugin_helperfail.f32"), {})
        pl = [ln for ln in log.splitlines() if ln.startswith("press ")]
        rl = renders(log)
        nfs, calls = fsguard_calls(log)
        # (a reply that is not JSON once made libc++abi look libstdc++ up, dlopen RTLD_NOLOAD, when PyObj::parse's
        # std::runtime_error was destroyed; PyObj::ParseError has no refstring: nothing extra may be recorded)
        extra = [c for c in calls if c not in (f"posix_spawn({helper} --ofx-session)",
                                               f"posix_spawn({helper} --ofx-default)",
                                               f"dlopen({BUNDLE}/Contents/MacOS/MeatVideo.ofx)")]
        ok = (len(pl) == 1 and " status=1 " in pl[0] and needle in pl[0] and "clip_params=unchanged" in pl[0] and
              "adjust_enabled=0" in pl[0] and cps == [""] and len(rl) == 1 and " status=0 " in rl[0] and
              "no noise profile" in rl[0] and "Default Profile and Default Preset could not be read" in rl[0] and
              calls.count(f"posix_spawn({helper} --ofx-session)") == 1 and
              calls.count(f"posix_spawn({helper} --ofx-default)") == 1 and
              nfs == 3 and extra == [])
        if extra:
            print(f"   (fsguard also recorded {extra})")
        print(f"== helper failure: {label}: {'ok' if ok else 'FAIL'}  {pl}")
        if not ok:
            fails.append(f"helper failure {label}")


def bundled_helper(a, clips, fails):
    """the helper inside the bundle (MEATVIDEO_OFX_HELPER unset): located from the plug-in's image, started with the
    Qt inside its app (offscreen here); the window is left open, so the test ends it with SIGTERM - the plug-in must
    then leave ClipParams unchanged and report the signal"""
    import signal
    exe = os.path.join(BUNDLE, "Contents", "Helpers", "MeatVideoQt.app", "Contents", "MacOS", "MeatVideoQt")
    home = os.path.join(a.work, "home")
    os.makedirs(home, exist_ok=True)
    env = dict(os.environ, DYLD_INSERT_LIBRARIES=FSGUARD, HOME=home, QT_QPA_PLATFORM="offscreen",
               NV_UI_QT_SETTINGS=ini_path(a.work, "set"))
    env.pop("MEATVIDEO_OFX_HELPER", None)
    clip = clips["A"]
    args = [MINIHOST, BUNDLE, "--frames", clip["path"], "--size", str(clip["W"]), str(clip["H"]), str(clip["N"]),
            "--press", "prepare@3", "--render", "3", "--out", os.path.join(a.work, "plugin_bundled.f32"),
            "--clip-out", os.path.join(a.work, "plugin_bundled.cp")]
    p = subprocess.Popen(args, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    pid, err, t0 = None, "", time.time()
    os.set_blocking(p.stderr.fileno(), False)
    while pid is None and time.time() - t0 < 120 and p.poll() is None:
        try:
            err += p.stderr.read() or ""
        except (BlockingIOError, TypeError):
            pass
        m = re.search(r"ofx-session: pid (\d+) window shown", err)
        if m:
            pid = int(m.group(1))
        time.sleep(0.1)
    if pid:
        os.kill(pid, signal.SIGTERM)
    out, err2 = p.communicate(timeout=120)
    pl = [ln for ln in out.splitlines() if ln.startswith("press ")]
    nfs, calls = fsguard_calls(out)
    # with the Default Profile / Preset set: the bundled helper's --ofx-default (no window) gives the new instance
    # its defaults (stored at the press), then the window is killed: ClipParams keeps the defaults
    want_cp = defaults_text(a.work, "set")[1]
    cps = [open(os.path.join(a.work, "plugin_bundled.cp_0.json")).read()] if pl else []
    ok = (pid is not None and len(pl) == 1 and " status=1 " in pl[0] and "terminated by signal 15" in pl[0] and
          "clip_params=changed" in pl[0] and cps == [want_cp] and "adjust_enabled=1" in pl[0] and
          calls == [f"dlopen({BUNDLE}/Contents/MacOS/MeatVideo.ofx)", f"posix_spawn({exe} --ofx-default)",
                    f"posix_spawn({exe} --ofx-session)"])
    print(f"== bundled helper: {'ok' if ok else 'FAIL'}: window shown by pid {pid}, ended with SIGTERM; {pl}; "
          f"fsguard {calls}")
    if not ok:
        fails.append("bundled helper")


def closed_fds(a, clips, fails):
    """R2-1: the host started with fd 0, fd 1, or fds 0..2 closed (the pipe ends the plug-in makes could land there):
    the defaults (--ofx-default) of a new instance, Prepare (Auto Profile, Apply) and Adjust (a change, Apply) work,
    and the renders and ClipParams equal those of the same run with all fds open.  The log goes to a file (--log)."""
    clip = clips["A"]
    home = os.path.join(a.work, "home")
    env = dict(os.environ, DYLD_INSERT_LIBRARIES=FSGUARD, HOME=home, QT_QPA_PLATFORM="offscreen",
               NV_UI_QT_SETTINGS=ini_path(a.work, "set"), MEATVIDEO_OFX_HELPER=DRV)
    env.pop("NV_OFX_SESSION_SCRIPT", None)
    results = {}
    combos = [(), (0,), (1,), (2,), (0, 1), (0, 2), (1, 2), (0, 1, 2)]
    for label, closed in [("all open" if not c else "fd " + ",".join(map(str, c)) + " closed", c) for c in combos]:
        tag = "fds" + "".join(map(str, closed))
        out = os.path.join(a.work, f"plugin_{tag}.f32")
        logf = out + ".log"
        for f in [out, logf] + [f"{out}.cp_{k}.json" for k in range(2)]:
            if os.path.exists(f):
                os.remove(f)
        args = [MINIHOST, BUNDLE, "--frames", clip["path"], "--size", str(clip["W"]), str(clip["H"]), str(clip["N"]),
                "--render", "3",
                "--env", "NV_OFX_SESSION_SCRIPT=auto;apply", "--press", "prepare@4", "--render", "4",
                "--env", "NV_OFX_SESSION_SCRIPT=set amt_Y 30;setb mix_enable 1;set mix 20;apply", "--press", "adjust@5",
                "--render", "5,2", "--out", out, "--clip-out", out + ".cp", "--log", logf]
        p = subprocess.Popen(args, env=env, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                             stderr=subprocess.DEVNULL, preexec_fn=lambda c=closed: [os.close(fd) for fd in c])
        rc = p.wait(timeout=600)
        log = open(logf).read() if os.path.exists(logf) else ""
        ci = [ln for ln in log.splitlines() if ln.startswith("create instance")]
        pl = [ln for ln in log.splitlines() if ln.startswith("press ")]
        rl = renders(log)
        nfs, calls = fsguard_calls(log)
        marks = [f"host fd {fd}: closed at start" in log for fd in range(3)]
        ok = (rc == 0 and ci and "adjust_enabled=1" in ci[0] and len(pl) == 2 and
              all(" status=0 " in ln and "clip_params=changed" in ln for ln in pl) and len(rl) == 4 and
              all(" status=0 " in ln for ln in rl) and marks == [fd in closed for fd in range(3)] and
              calls == [f"dlopen({BUNDLE}/Contents/MacOS/MeatVideo.ofx)", f"posix_spawn({DRV} --ofx-default)",
                        f"posix_spawn({DRV} --ofx-session)", f"posix_spawn({DRV} --ofx-session)"])
        res = (open(out, "rb").read() if os.path.exists(out) else b"",
               [open(f"{out}.cp_{k}.json").read() if os.path.exists(f"{out}.cp_{k}.json") else None for k in range(2)])
        results[label] = res
        same = res == results["all open"]
        print(f"== host started with {label}: {'ok' if ok and same else 'FAIL'} (rc {rc}; defaults at CreateInstance: "
              f"{'yes' if ci and 'adjust_enabled=1' in ci[0] else 'no'}; presses "
              f"{[field(ln, 'status') + '/' + str(field(ln, 'clip_params')) for ln in pl]}; renders "
              f"{[field(ln, 'status') for ln in rl]}; output and ClipParams "
              f"{'identical to' if same else 'DIFFERENT from'} the all-open run)")
        if not (ok and same):
            fails.append(f"host started with {label}")
            print(log[-1500:])


def bundled_data(a, fails):
    """L1: the helper of the bundle names no path of the build machine and reads the reference floor from its own
    Resources/data.  The session driver built in the bundle's qtui tree (NV_EMBED_SOURCE_ROOT=OFF, as the bundled
    MeatVideoQt) is put into a copy of the helper app and runs the Auto Profile of the default-profile request under
    fsguard (FSGUARD_ALWAYS): the files it opens, and its profile against the one the development driver made (it
    reads <checkout>/data/ref_floor.npy)"""
    import shutil
    app = os.path.join(BUNDLE, "Contents", "Helpers", "MeatVideoQt.app")
    hits = subprocess.run(["grep", "-rlF", "-e", ROOT, app], capture_output=True, text=True).stdout.split()
    exe_strings = subprocess.run(["strings", os.path.join(app, "Contents", "MacOS", "MeatVideoQt")], capture_output=True,
                                 text=True).stdout
    users = sorted({ln for ln in exe_strings.splitlines() if "/Users/" in ln})
    print(f"== bundled helper: files naming {ROOT}: {hits}; '/Users/' strings in MeatVideoQt: {users}")
    if hits or users:
        fails.append("the bundled helper names build-machine paths")
    tree = os.path.join(BUILD, "qtui")
    b = subprocess.run(["cmake", "--build", tree, "--target", "qtui_ofx_session_drv", "-j", "2"], capture_output=True,
                       text=True)
    cache = open(os.path.join(tree, "CMakeCache.txt")).read()
    if b.returncode != 0 or "NV_EMBED_SOURCE_ROOT:BOOL=OFF" not in cache:
        fails.append("bundled data: the driver of the bundle's qtui tree")
        print(b.stdout[-500:], b.stderr[-500:])
        return
    copy_app = os.path.join(a.work, "helper_copy.app")
    shutil.rmtree(copy_app, ignore_errors=True)
    shutil.copytree(app, copy_app, symlinks=True)
    drv = os.path.join(copy_app, "Contents", "MacOS", "qtui_ofx_session_drv")
    shutil.copy2(os.path.join(tree, "qtui_ofx_session_drv"), drv)
    st = os.path.join(a.work, "bundled_state.json")
    env = dict(os.environ, HOME=os.path.join(a.work, "home"), QT_QPA_PLATFORM="offscreen",
               NV_UI_QT_SETTINGS=ini_path(a.work, "none"), NV_OFX_SESSION_SCRIPT=f"auto;state {st};cancel",
               DYLD_INSERT_LIBRARIES=FSGUARD, FSGUARD_ALWAYS="1")
    env.pop("NV_REF_FLOOR", None)
    p = subprocess.run([drv, "--ofx-session"], input=open(os.path.join(a.work, "default_request.bin"), "rb").read(),
                       capture_output=True, env=env, timeout=600)
    err = p.stderr.decode(errors="replace")
    floor = sorted({ln.strip() for ln in err.splitlines() if "ref_floor" in ln})
    outside = sorted({ln.strip() for ln in err.splitlines() if "fsguard:" in ln and "ref_floor" in ln and
                      copy_app not in ln})
    want = json.load(open(os.path.join(a.work, "default_source.json")))
    got = json.load(open(st)) if os.path.exists(st) else None
    same = got is not None and got["profile"] == want["profile"]
    inside = os.path.join(copy_app, "Contents", "MacOS", "../Resources/data/ref_floor.npy")
    ok = p.returncode == 0 and p.stdout == b"CANCEL\n" and same and not outside and \
        any(inside in ln for ln in floor) and all(inside in ln for ln in floor)
    print(f"== bundled helper data: {'ok' if ok else 'FAIL'}: ref_floor opened {floor}; checkout data opened {outside}; "
          f"profile {'identical to' if same else 'DIFFERENT from'} the development driver's (it reads the checkout's file)")
    if not ok:
        fails.append("bundled helper data (ref_floor)")


def mutations(a, clips, keep, fails):
    """sensitivity: each changed input must make its comparison / check fail"""
    texts = {}
    clip = clips["A"]
    sc = {"name": "mut", "clip": "A", "first": 0, "origin": [0, 0], "layout": "pad"}
    base_spec = {"at": 0}
    base = synth_cp(clip, base_spec)
    times = [0, 1, 4, 6, 7]

    def plugin_with(text, tag):
        spec = {"raw": tag}
        path = os.path.join(a.work, f"cp_mut_{tag}.json")
        open(path, "w").write(text)
        texts[id(spec)] = path
        s = dict(sc, init=spec, steps=[("render", times)])
        out = os.path.join(a.work, f"plugin_mut_{tag}.f32")
        run_plugin(a.work, s, clip, out, texts)
        return out

    ref, _ = run_reference(a.work, "mut", sc, clip, [["clip", base], ["render", times]], True, "native")
    rows = compare(plugin_with(base, "none"), ref, clip, times)
    print(f"== mutation 0 (unchanged ClipParams): {sum(1 for r in rows if r[1])}/{len(rows)} renders differ")
    if any(r[1] for r in rows):
        fails.append("mutation 0: unchanged ClipParams differs")

    def report(n, label, rows):
        nd = sum(1 for r in rows if r[1])
        print(f"== mutation {n} ({label}): {nd}/{len(rows)} renders differ, {sum(r[1] for r in rows)} words")
        if not nd:
            fails.append(f"mutation {n} not detected")

    d = json.loads(base)
    d["render"]["P"]["threshold"] = 4.0001
    report(1, "plug-in ClipParams render.P threshold 4.0001", compare(plugin_with(json.dumps(d), "thr"), ref, clip, times))
    d = json.loads(base)
    y = np.float32(d["render"]["profile"]["curve"]["Y"][3])          # the C ABI takes the curve as float32
    d["render"]["profile"]["curve"]["Y"][3] = float(np.nextafter(y, np.float32(np.inf)))
    report(2, "plug-in ClipParams profile curve Y[3] + 1 float32 ulp", compare(plugin_with(json.dumps(d), "curve"), ref, clip, times))
    d = json.loads(base)
    d["render"]["P"]["ui_channels"]["Cr"]["bands"]["high"] = 0.99
    report(3, "plug-in ClipParams Cr high band 0.99 (per-channel path)",
           compare(plugin_with(json.dumps(d), "chan"), ref, clip, times))
    ref2, _ = run_reference(a.work, "mut2", sc, clip, [["clip", base], ["render", times]], True, "mutin",
                            mutate_input=True)
    report(4, "reference input: one word of frame 0 changed by 1 ulp", compare(plugin_with(base, "none2"), ref2, clip, times))
    # the checks of a window's ClipParams
    # the defaults checks
    dtext = defaults_text(a.work, "set")[1]
    bad = check_defaults(dtext, "badpre")
    print(f"== mutation 9 (the defaults of 'set' checked as those of 'badpre', i.e. the factory settings): check "
          f"{'fails' if bad else 'PASSES'} {bad[:1]}")
    if not bad:
        fails.append("mutation 9 not detected")
    d = json.loads(dtext)
    d["profile"]["tune"] = {"all": 1.1}
    bad = check_defaults(json.dumps(d), "set")
    print(f"== mutation 10 (the defaults with the profile tune changed): check {'fails' if bad else 'PASSES'} {bad[:1]}")
    if not bad:
        fails.append("mutation 10 not detected")
    d = json.loads(dtext)
    d["render"]["matrix"] = "bt709"           # 360-line frames: "auto" is BT.601
    report(11, "the defaults' render matrix auto -> bt709 (the plug-in) vs auto (the reference)",
           compare(plugin_with(json.dumps(d), "mat"), run_reference(a.work, "mut3", sc, clip, [["clip", dtext], ["render", times]],
                                                                   True, "native")[0], clip, times))
    # the data check must see a read of the checkout's file: the development driver (NV_SOURCE_ROOT compiled in)
    env = dict(os.environ, HOME=os.path.join(a.work, "home"), QT_QPA_PLATFORM="offscreen",
               NV_UI_QT_SETTINGS=ini_path(a.work, "none"), NV_OFX_SESSION_SCRIPT="auto;cancel",
               DYLD_INSERT_LIBRARIES=FSGUARD, FSGUARD_ALWAYS="1")
    p = subprocess.run([DRV, "--ofx-session"], input=open(os.path.join(a.work, "default_request.bin"), "rb").read(),
                       capture_output=True, env=env, timeout=600)
    outside = sorted({ln.strip() for ln in p.stderr.decode(errors="replace").splitlines()
                      if "fsguard:" in ln and "/data/ref_floor.npy" in ln})
    print(f"== mutation 12 (the data check on the development driver): checkout data opened {outside}")
    if not outside:
        fails.append("mutation 12 not detected")
    w = keep.get("W-prepare-adjust")
    if not w:
        print("== mutations 5-8 skipped (they use the W-prepare-adjust scenario, not run)")
    if w:
        cps = [s[1] for s in w["rsteps"] if s[0] == "clip"]
        cp = json.loads(cps[0])
        fetched = [float(x) for x in re.search(r"fetched=\[([^\]]*)\]",
                                               [ln for ln in w["log"].splitlines() if ln.startswith("press ")][0]).group(1).split()]
        m1 = copy.deepcopy(cp)
        m1["preset"]["params"]["amt_Y"] = 69.0
        bad = check_cp(json.dumps(m1), clip, 0, 4.0, fetched, False)
        print(f"== mutation 5 (window ClipParams: preset amt_Y 69 with render.P unchanged): check {'fails' if bad else 'PASSES'} {bad[:1]}")
        if not bad:
            fails.append("mutation 5 not detected")
        bad = check_cp(cps[0], clip, 0, 5.0, fetched, True)
        print(f"== mutation 6 (profile build check against Python's profile of frame 5 instead of 4): check {'fails' if bad else 'PASSES'} {bad[:1]}")
        if not bad:
            fails.append("mutation 6 not detected")
        m2 = copy.deepcopy(cp)
        m2["render"]["profile"]["area"] = [0, 0, 1, 1]
        bad = check_cp(json.dumps(m2), clip, 0, 4.0, fetched, False)
        print(f"== mutation 7 (window ClipParams: effective profile area changed): check {'fails' if bad else 'PASSES'} {bad[:1]}")
        if not bad:
            fails.append("mutation 7 not detected")
        # the qtui processing comparison: the driver output against the render of another frame
        g = np.fromfile(w["out"], "<u4").reshape(len(w["times"]), clip["H"], clip["W"], 4)
        q = np.fromfile(os.path.join(a.work, "proc_w1_4.f32"), "<u4").reshape(3, clip["H"], clip["W"])
        k = w["times"].index(0)
        bad = int(np.count_nonzero(np.stack([q[2], q[0], q[1]], -1) != g[k, ..., :3]))
        print(f"== mutation 8 (the C++ UI's frame 4 against the render of frame 0): {bad} words differ")
        if not bad:
            fails.append("mutation 8 not detected")


if __name__ == "__main__":
    sys.exit(main())
