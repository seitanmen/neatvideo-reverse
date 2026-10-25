#!/usr/bin/env python3
"""Claim J, part 2: the real-frame recomputation (test_v02 n=312, center half crop)
that lead_static_20261021.md §9.4 table row 1 reports as
    [1.039, 1.027, 1.011, 1.002, 1.001, 1.001, 1.001, 1.000]."""
import sys
import numpy as np

sys.dont_write_bytecode = True
sys.path.insert(0, __file__.rsplit('/', 1)[0] + '/port')

import nv_lines as nl  # noqa: E402
import nv_curve as ncv  # noqa: E402

raw = np.fromfile(__file__.rsplit('/', 1)[0] + '/frame312.rgb', np.uint8)
rgb = raw.reshape(540, 960, 3).astype(np.float32)
print('frame:', rgb.shape)

nl.COUNT_SHIFT = 0
nl.IDX_PAD = 0
xs, base = ncv.genuine_noise_curve(rgb, batch=False)

nl.COUNT_SHIFT = 1
nl.IDX_PAD = 512
_, shifted = ncv.genuine_noise_curve(rgb, batch=False)

print('xs           :', [float(x) for x in xs])
print('ys base      :', ['%.6g' % v for v in base])
print('ys count<<1  :', ['%.6g' % v for v in shifted])
print('shifted/base :', ['%.4f' % (s / b) for s, b in zip(shifted, base)])
print('lead claims  : [1.039, 1.027, 1.011, 1.002, 1.001, 1.001, 1.001, 1.000]')
