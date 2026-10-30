import sys, numpy as np
sys.path.insert(0, "repro")
import nvfilter as nv
F32 = np.float32
def pw(a):
    n = len(a)
    if n < 8:
        r = F32(-0.0)
        for v in a: r = F32(r + v)
        return r
    if n <= 128:
        r = [F32(v) for v in a[:8]]
        i = 8
        while i < n - (n % 8):
            for j in range(8): r[j] = F32(r[j] + a[i + j])
            i += 8
        res = F32(F32(F32(r[0] + r[1]) + F32(r[2] + r[3])) + F32(F32(r[4] + r[5]) + F32(r[6] + r[7])))
        for k in range(i, n): res = F32(res + a[k])
        return res
    n2 = n // 2; n2 -= n2 % 8
    return F32(pw(a[:n2]) + pw(a[n2:]))
for L in (4, 5):
    n = 8 * 2 ** L + 1
    d = np.zeros((n, n), F32); d[n // 2, n // 2] = 1
    b = nv.atrous(d, L)[1]
    s = (b * b).sum()
    q = (b * b)
    whole = F32(pw(q.ravel()) + F32(0))
    rows = F32(0)
    for r in q: rows = F32(rows + pw(r))
    chunks = F32(0)
    flat = q.ravel()
    for k in range(0, flat.size, 8192): chunks = F32(chunks + pw(flat[k:k + 8192]))
    print(L, s, whole, rows, chunks, s == whole, s == rows, s == chunks)
rng = np.random.default_rng(0)
def variants(q):
    flat = np.ascontiguousarray(q).ravel()
    whole = F32(pw(flat) + F32(0))
    chunks = F32(0)
    for k in range(0, flat.size, 8192): chunks = F32(chunks + pw(flat[k:k + 8192]))
    rows = F32(0)
    if q.ndim == 2:
        for r in q: rows = F32(rows + pw(r))
    return whole, chunks, rows
for shape in [(20000,), (8192,), (9000,), (300, 300), (1080, 1920)]:
    a = rng.uniform(0, 255, shape).astype(F32)
    s = a.sum(); w, c, r = variants(a)
    print("contig", shape, s == w, s == c, s == r)
big = rng.uniform(0, 255, (1080, 1920)).astype(F32)
for (y0, x0, h, w) in [(10, 20, 128, 128), (0, 0, 512, 512), (100, 300, 300, 500), (5, 0, 200, 1920), (3, 7, 64, 100)]:
    v = big[y0:y0 + h, x0:x0 + w]
    s = v.sum(); W, C, R = variants(v)
    print("view", (h, w), s == W, s == C, s == R)
