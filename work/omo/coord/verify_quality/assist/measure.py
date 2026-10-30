"""Task B: normal vs Assist outputs on test_v02 (n=312, auto profile, no AFT).
Writes measure.json and pngs next to this script. Does not modify repro/."""
import json, os, sys, time
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "../../../../.."))
sys.path.insert(0, os.path.join(ROOT, "repro"))


def main():
    import numpy as np
    from PIL import Image
    import nvfilter as nv
    from denoise_video_v5 import probe, read_frames
    from nv_ui_qt import backend
    from nv_ui_qt.state import default_params, backend_params
    from nv_ui_qt.main_window import assist_params as ui_assist
    from nv_aft_v5 import assist_params as aft_assist

    video = os.path.join(ROOT, "test_v02.mp4")
    info = probe(video)
    W, H = info["W"], info["H"]
    T, R = 312, 2
    frames = [None] * (T + R + 1)
    for i, f in read_frames(video, W, H, T - R, T + R + 1, info["fps"]):
        frames[i] = f
    matrix, full = "bt709", False
    eng = backend.PreviewEngine(frames, matrix, full)
    prof = backend.build_profile(frames, T, full, matrix, "auto")
    P_norm = backend_params(default_params())
    res = {"frame": T, "profile_mode": "auto", "aft": False}

    t0 = time.time(); norm, orig = eng.process(T, prof, 1, P_norm); res["t_normal"] = time.time() - t0
    t0 = time.time(); a_ui = eng.assist(T, prof, ui_assist()); res["t_assist_ui"] = time.time() - t0
    a_ui2 = eng.assist(T, prof, ui_assist())
    res["assist_ui_deterministic"] = bool(np.array_equal(a_ui, a_ui2))
    # reference: spatial_v5 directly on the frame with residual variance 1 (no backend wrapper)
    Pa = ui_assist()
    planes = [np.asarray(p, np.float32) for p in frames[T]]
    ref = nv.spatial_v5(planes, prof, [np.ones(p.shape, np.float32) for p in planes], Pa) \
        if backend._uniform(Pa) else None
    if ref is not None:
        ref = eng.to_rgb([np.clip(o, 0, 255) for o in ref])
        res["assist_ui_eq_spatial_v5"] = bool(np.array_equal(a_ui, ref))
    res["ui_assist_params"] = {k: Pa[k] for k in ("temporal", "luma", "chroma", "band_amounts", "mix",
                                                  "ui_spatial")}
    # nv_aft_v5.assist_params (other agent): needs backend params; with UI params it fails
    try:
        aft_assist(dict(default_params()), "profile_check"); res["aft_assist_with_ui_params"] = "ok"
    except Exception as e:
        res["aft_assist_with_ui_params"] = f"{type(e).__name__}: {e}"
    Pb = aft_assist(P_norm, "profile_check")
    res["aft_assist_params"] = {k: Pb[k] for k in ("temporal", "luma", "chroma", "band_amounts", "mix")}
    res["aft_assist_uniform"] = bool(backend._uniform(Pb))
    b_aft, _ = eng.process(T, prof, 2, Pb)
    Pc = aft_assist(P_norm, "noise_level_check")
    c_aft, _ = eng.process(T, prof, 3, Pc)

    def st(a, b):
        d = a.astype(np.float64) - b.astype(np.float64)
        mse = float((d ** 2).mean())
        return {"mae": float(np.abs(d).mean()), "max": float(np.abs(d).max()),
                "psnr": float(10 * np.log10(255 ** 2 / mse)) if mse > 0 else None,
                "identical": bool(mse == 0)}

    res["normal_vs_orig"] = st(norm, orig)
    res["assist_ui_vs_orig"] = st(a_ui, orig)
    res["assist_ui_vs_normal"] = st(a_ui, norm)
    res["aft_pc_vs_normal"] = st(b_aft, norm)
    res["aft_pc_vs_assist_ui"] = st(b_aft, a_ui)
    res["aft_nlc_vs_normal"] = st(c_aft, norm)
    # removed-component std (luma of RGB), flat sky area of the genuine profile (y0 x832 128x128)
    def lum(x): return 0.2989 * x[..., 0] + 0.5866 * x[..., 1] + 0.1145 * x[..., 2]
    sl = (slice(0, 128), slice(832, 960))
    for name, img in (("normal", norm), ("assist_ui", a_ui), ("aft_pc", b_aft), ("aft_nlc", c_aft)):
        res[f"removed_std_sky_{name}"] = float((lum(orig) - lum(img))[sl].std())
        res[f"residual_std_sky_{name}"] = float(lum(img)[sl].std())
    res["residual_std_sky_orig"] = float(lum(orig)[sl].std())
    for name, img in (("orig", orig), ("normal", norm), ("assist_ui", a_ui), ("aft_pc", b_aft)):
        Image.fromarray(np.clip(img + 0.5, 0, 255).astype(np.uint8)).save(os.path.join(HERE, f"f312_{name}.png"))
    with open(os.path.join(HERE, "measure.json"), "w") as f:
        json.dump(res, f, indent=1)
    print(json.dumps(res, indent=1))


if __name__ == "__main__":
    main()
