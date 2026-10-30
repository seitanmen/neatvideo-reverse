#!/usr/bin/env python3
"""Claim J (REQUEST): lower-bound recomputation of the count<<1 kernel shift.

Runs nv_curve.genuine_noise_curve(…, batch=False) twice on the same input:
  base    : unmodified port (shift 0, i.e. DAT_01561d58 = 0 as in every harness run)
  shifted : nl.COUNT_SHIFT = 1, nl.IDX_PAD = 512 (count<<1, stale idx tail = 0,
            out-of-plane gathered pixels ignored) — the post-spatial-chunk state of
            lead_static_20261021.md §9.4.
Input: seed 7 random tile(s), 96x96, normal noise sigma 40 around 128 (REQUEST's spec).
Expected trend (claim): dark bins move, bright bins stay near 1.
"""
import sys
import numpy as np

sys.dont_write_bytecode = True
sys.path.insert(0, __file__.rsplit('/', 1)[0] + '/port')

import nv_lines as nl          # noqa: E402  (my patched copy in this folder)
import nv_curve as ncv         # noqa: E402

rng = np.random.default_rng(7)
rgb = np.empty((96, 96, 3), np.float32)
rgb[:] = (128.0 + 40.0 * rng.standard_normal((96, 96, 1))).astype(np.float32)

xs, base = ncv.genuine_noise_curve(rgb, batch=False)
assert base is not None

nl.COUNT_SHIFT = 1
nl.IDX_PAD = 512
xs2, shifted = ncv.genuine_noise_curve(rgb, batch=False)

print('tile: 96x96, normal(128, 40), seed 7, all three channels equal')
print('xs            :', list(xs))
print('ys base       :', ['%.6g' % v for v in base])
print('ys count<<1   :', ['%.6g' % v for v in shifted])
print('shifted/base  :', ['%.4f' % (s / b) for s, b in zip(shifted, base)])

# second variant: independent noise per channel (still seed 7 stream)
rng2 = np.random.default_rng(7)
rgb2 = (128.0 + 40.0 * rng2.standard_normal((96, 96, 3))).astype(np.float32)
nl.COUNT_SHIFT = 0
nl.IDX_PAD = 0
_, base2 = ncv.genuine_noise_curve(rgb2, batch=False)
nl.COUNT_SHIFT = 1
nl.IDX_PAD = 512
_, shifted2 = ncv.genuine_noise_curve(rgb2, batch=False)
print()
print('per-channel independent noise variant:')
print('ys base       :', ['%.6g' % v for v in base2])
print('ys count<<1   :', ['%.6g' % v for v in shifted2])
print('shifted/base  :', ['%.4f' % (s / b) for s, b in zip(shifted2, base2)])
