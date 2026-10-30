#!/usr/bin/env python3
"""Validate v4 pipeline (temporal-first, Neat defaults) on synthetic clip."""
import numpy as np
import neat_repro as nr
import nvfilter as nv

clean = nr.make_clean()
noisy = nr.add_noise(clean)
N = noisy.shape[0]

yc = [nv.rgb2ycbcr(f) for f in noisy]
flat = nv.find_flat(yc[N // 3][..., 0])
prof = nv.build_profile_yc(yc[N // 3], flat)
print("profile:", {k: round(v["sigma"], 2) for k, v in prof.items()})

# Neat order: temporal first (radius 2 = 5 frames), then spatial
grays = [f[..., 0].astype(np.float32) for f in yc]
temp_yc = nv.temporal_filter(yc, grays, prof, radius=2)
temp = [np.clip(nv.ycbcr2rgb(f), 0, 255) for f in temp_yc]
spat_yc = [nv.spatial_filter_yc(f, prof) for f in temp_yc]
spat = [np.clip(nv.ycbcr2rgb(f), 0, 255) for f in spat_yc]
final_yc = nv.deflicker(spat_yc)
final = [np.clip(nv.ycbcr2rgb(f), 0, 255) for f in final_yc]

for tag, clip in [("noisy", noisy), ("temporal", np.stack(temp)),
                  ("full", np.stack(spat)), ("final", np.stack(final))]:
    print(tag, round(float(np.mean([nr.psnr(a, b) for a, b in zip(clip, clean)])), 2))
