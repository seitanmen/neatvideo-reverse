# W2a: the subprocess hook of native/verify_libm_alt.py (loaded through PYTHONPATH by every Python process
# of a verify run, the multiprocessing workers included).  The verify scripts are not edited.
#
# It wraps subprocess.run.  A call whose program is one of the native test binaries (native/nv_*_test) is
#   NV_LIBM_HOOK_MODE=swap: run with the libm_alt build of the same name (native/libm_alt/bin) and
#                           NV_LIBM_ALT=$NV_LIBM_HOOK_ALT, so the verify script compares the alt output with
#                           Python (its own counts);
#   NV_LIBM_HOOK_MODE=capture: run as usual (the verify script sees the default output) and keep, in
#                           $NV_LIBM_CAPTURE/<call>/, the input files (in_<argv index>), the output files (the
#                           arguments the run created or changed: def_<argv index>), the NV_* environment and the
#                           stdout (meta.json); verify_libm_alt.py replays the call with the alt build later.
# Other calls (ffmpeg, make, ...) are untouched.
import os
import sys


def _chain_original():
    """Run the sitecustomize this one shadows (e.g. Anaconda's), as Python would have."""
    import importlib.util
    here = os.path.dirname(os.path.abspath(__file__))
    for d in sys.path:
        if not d or os.path.abspath(d) == here:
            continue
        p = os.path.join(d, "sitecustomize.py")
        if os.path.isfile(p):
            spec = importlib.util.spec_from_file_location("_nv_orig_sitecustomize", p)
            mod = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(mod)
            return


_chain_original()

_MODE = os.environ.get("NV_LIBM_HOOK_MODE", "")
if _MODE in ("swap", "capture"):
    import json
    import shutil
    import subprocess
    import threading

    _NATIVE = os.path.realpath(os.environ["NV_LIBM_NATIVE"])
    _ALTBIN = os.path.join(_NATIVE, "libm_alt", "bin")
    _BINS = {"nv_quality_test", "nv_band_test", "nv_curve_test", "nv_lzma_test", "nv_flat_test", "nv_model_test",
             "nv_v5_test", "nv_profile_test", "nv_aft_test"}
    _orig_run = subprocess.run
    _lock = threading.Lock()
    _seq = [0]

    def _target(args):
        if not isinstance(args, (list, tuple)) or not args or not isinstance(args[0], str):
            return None
        p = os.path.realpath(args[0])
        if os.path.dirname(p) == _NATIVE and os.path.basename(p) in _BINS:
            return os.path.basename(p)
        return None

    def _env_with(kw, extra):
        e = dict(kw.get("env") if kw.get("env") is not None else os.environ)
        for k in ("NV_LIBM_ALT", "NV_LIBM_ALT_FUNCS", "NV_LIBM_LOG"):
            e.pop(k, None)
        e.update(extra)
        return e

    def _stat(p):
        try:
            st = os.stat(p)
            return (st.st_mtime_ns, st.st_size, st.st_ino)
        except OSError:
            return None

    def _capture(name, args, kw):
        """run the default binary; keep its input files, output files, NV_* environment and stdout."""
        capdir = os.environ["NV_LIBM_CAPTURE"]
        paths = [(i, a) for i, a in enumerate(args) if i > 0 and isinstance(a, str) and os.path.isfile(a)]
        cand = [(i, a) for i, a in enumerate(args) if i > 0 and isinstance(a, str) and (os.sep in a or os.path.exists(a))]
        with _lock:
            _seq[0] += 1
            call = f"{os.getpid()}_{_seq[0]}"
        d = os.path.join(capdir, call)
        os.makedirs(d)
        before = {a: _stat(a) for _, a in cand}
        for i, a in paths:
            shutil.copyfile(a, os.path.join(d, f"in_{i}"))
        env = kw.get("env") if kw.get("env") is not None else os.environ
        res = _orig_run(args, **kw)
        outs = [i for i, a in cand if os.path.isfile(a) and _stat(a) != before[a]]
        for i in outs:
            shutil.copyfile(args[i], os.path.join(d, f"def_{i}"))
        so = res.stdout if kw.get("capture_output") or kw.get("stdout") == subprocess.PIPE else None
        if isinstance(so, bytes):
            so = so.decode("utf-8", "replace")
        meta = {"call": call, "bin": name, "argv": list(args[1:]), "inputs": [i for i, _ in paths if i not in outs],
                "outputs": outs, "env": {k: v for k, v in env.items() if k.startswith("NV_") and not k.startswith("NV_LIBM")},
                "rc": res.returncode, "stdout": so, "cwd": os.getcwd()}
        with open(os.path.join(d, "meta.json"), "w") as f:
            json.dump(meta, f)
        return res

    def _run(args, *a, **kw):
        name = _target(args)
        if name is None or a:
            return _orig_run(args, *a, **kw)
        if _MODE == "swap":
            extra = {"NV_LIBM_ALT": os.environ.get("NV_LIBM_HOOK_ALT", "0")}
            if os.environ.get("NV_LIBM_HOOK_FUNCS"):
                extra["NV_LIBM_ALT_FUNCS"] = os.environ["NV_LIBM_HOOK_FUNCS"]
            kw["env"] = _env_with(kw, extra)
            return _orig_run([os.path.join(_ALTBIN, name)] + list(args[1:]), **kw)
        return _capture(name, list(args), kw)

    subprocess.run = _run
