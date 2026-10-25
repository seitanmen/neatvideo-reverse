#!/usr/bin/env python3
"""Validate the v5 pipeline on the synthetic clip (known ground truth, sigma 15 RGB noise).
The clip is 176x144: FindFlatArea needs >= 64 px (the genuine one gives up below that), so
the known 32x32 flat patch is passed as a manual profile area, like a user selection."""
import numpy as np
import neat_repro as nr
import nvfilter as nv

clean = nr.make_clean()
noisy = nr.add_noise(clean)
N = noisy.shape[0]
planes = [nv.rgb_to_planes(f) for f in noisy]
area = (nr.FLAT[0].start, nr.FLAT[1].start, nr.FLAT[0].stop - nr.FLAT[0].start)
prof = nv.build_profile_v5(planes[N // 3], full_range=True, area=area, mode="spatial")
print("profile sigma:", {k: round(v, 2) for k, v in prof["sigma"].items()},
      "Y band ratio", [round(r, 3) for r in prof["band_ratio"]["Y"]])


def run(P, flicker=0):
    outs = []
    for t in range(N):
        lo, hi = max(0, t - P["radius"]), min(N, t + P["radius"] + 1)
        outs.append(nv.process_frame_v5(planes[lo:hi], t - lo, prof, P))
    if flicker:
        g = nv.flicker_gains([o[0].mean() for o in outs], flicker)
        outs = [(o[0] * gi, o[1], o[2]) for o, gi in zip(outs, g)]
    return np.stack([np.clip(nv.ycbcr2rgb(np.stack(o, -1)), 0, 255) for o in outs])


def score(clip):
    return round(float(np.mean([nr.psnr(a, b) for a, b in zip(clip, clean)])), 2)


print("noisy", score(noisy))
print("v5 temporal-only", score(run(nv.v5_params(luma=0.0, chroma=0.0))))
print("v5 spatial-only", score(run(nv.v5_params(temporal=0.0))))
print("v5 full", score(run(nv.v5_params())))
print("v5 full + flicker(r=12)", score(run(nv.v5_params(), 12)))
