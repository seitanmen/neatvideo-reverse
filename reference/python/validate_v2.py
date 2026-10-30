#!/usr/bin/env python3
"""Validate v2 pipeline on synthetic clip with known ground truth."""
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

spat_yc = [nv.spatial_filter_yc(f, prof) for f in yc]
spat = [np.clip(nv.ycbcr2rgb(f), 0, 255) for f in spat_yc]
grays = [f[..., 0].astype(np.float32) for f in spat_yc]
temp_yc = nv.temporal_filter(spat_yc, grays, prof)
temp = [np.clip(nv.ycbcr2rgb(f), 0, 255) for f in temp_yc]
final_yc = nv.deflicker(temp_yc)
final = [np.clip(nv.ycbcr2rgb(f), 0, 255) for f in final_yc]

for tag, clip in [("noisy", noisy), ("spatial", np.stack(spat)),
                  ("temporal", np.stack(temp)), ("final", np.stack(final))]:
    print(tag, round(float(np.mean([nr.psnr(a, b) for a, b in zip(clip, clean)])), 2))
