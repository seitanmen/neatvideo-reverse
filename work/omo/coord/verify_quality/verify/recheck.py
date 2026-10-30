"""独立検証: QUALITY.md の式を逆アセンブル (RS 0xa4acf8 / Demo 0x9bb3e0) から書き直し、quality_draft.py と比べる。
本家は実行しない。repro/ は import のみ。"""
import os, sys, struct, json, math, itertools
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..', '..', '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'repro'))
sys.path.insert(0, os.path.join(HERE, '..', 'static'))
sys.dont_write_bytecode = True
import nv_flat as nf
import quality_draft as qd

F = np.float32
H5 = F(0.5)

def pen(x):  # Demo 0x9d49b4 (s1=2, s2=-0.25)、逆アセンブルの演算順
    s, c = F(2.0), F(-0.25)
    a = F(F(H5 / s) / c); b = F(F(F(c + F(-1.0)) * H5) / c)
    v = F(F(a * F(x)) + b)
    if v < 0: return F(0.0)             # b.mi
    if not (v > F(1.0)): return v       # b.le (NaN も v のまま)
    return F(1.0)

def S_term(W, H, cls):  # 0xa4aebc..0xa4af98
    T = (16, 32, 64, 128, 256)
    if W == 0 or H == 0: return F(np.int32(cls))
    m = H if W > H else W
    if m < T[0]: return F(0.0)
    for k in range(1, 5):
        if m < T[k]:
            return F(F(F(m - T[k-1]) / F(T[k] - T[k-1])) + F(k))
    return F(5.0)

LUMA = bytes([1,1,1, 1,0,0, 1,0,0, 0,0,0])  # RS 0x1622348 / Demo 0x147dfc8 (ファイルから読んだ値)

def q_indep(blob, g):
    b = bytes(blob)
    if b[0xbf34] == 0: return F(0.0)
    s11 = F(F(F(g) * H5) + H5)
    mode = struct.unpack_from('<i', b, 0x48)[0]
    w = np.frombuffer(b, np.uint32)
    u = []
    for i, off in enumerate((0x5e48, 0x7d9c, 0x9cf0)):
        fl = LUMA[mode*3 + i]
        f = nf.flatness(w[off//4: off//4 + 55], bool(fl))
        v = pen(f)
        u.append(v if fl else F(F(v * H5) + H5))
    P = F(F(F(F(u[0] + H5) * F(u[1] + H5)) * F(u[2] + H5)) + F(0.25))
    q = F(s11 * F(P / F(3.625)))
    W, Hh = struct.unpack_from('<ii', b, 0xc2c0)
    q = F(q * F(S_term(W, Hh, struct.unpack_from('<i', b, 0xbf38)[0]) / F(5.0)))
    for c in range(3):
        if b[0xbf3c + c]: q = F(q * H5)
    return q

def main():
    res = {}
    # 1) 部品の照合 (境界値)
    xs = [F(v) for v in (-1, 0, 1.0, 1.4999999, 1.5, 1.5000001, 2.0, 2.4999998, 2.5, 3, 1e30, -0.0)] + [F(np.nan), F(np.inf), F(-np.inf)]
    bad = [float(x) for x in xs if not (np.array(qd.penalty(x)).view(np.uint32) == np.array(pen(x)).view(np.uint32))]
    res['penalty_mismatch'] = bad
    sz = []
    for W, Hh in itertools.product([0, 1, 15, 16, 17, 31, 32, 48, 63, 64, 96, 127, 128, 129, 191, 192, 255, 256, 257, 512], repeat=2):
        for cls in (0, 3, 5):
            a, bb = qd.size_term(W, Hh, cls), S_term(W, Hh, cls)
            if np.array(a).view(np.uint32) != np.array(bb).view(np.uint32): sz.append((W, Hh, cls, float(a), float(bb)))
    res['size_mismatch'] = sz
    # 2) 実データ: test_v02 n=312、領域 x832 y0 128x128 の完全なプロファイル
    import nv_profile as npf
    rgb = qd._frame(os.path.join(ROOT, 'test_v02.mp4'), 312)
    blob, _ = npf.build_profile(rgb, 1, (832, 0, 128, 128), with_floor=False, lzma=False)
    blob = blob.tobytes()
    d = qd.quality_from_profile(blob)
    res['draft_R'] = d['R']; res['draft_flatness'] = d['flatness']
    res['fields'] = dict(valid=blob[0xbf34], cls=struct.unpack_from('<i', blob, 0xbf38)[0],
                         clips=list(blob[0xbf3c:0xbf3f]), WH=struct.unpack_from('<ii', blob, 0xc2c0),
                         mode=struct.unpack_from('<i', blob, 0x48)[0])
    gs = [0.0, 0.25, 0.5, 1.0, -0.425, -1.0]
    res['q_indep'] = {str(g): float(q_indep(blob, g)) for g in gs}
    res['q_draft'] = {str(g): qd.quality_from_profile(blob, g)['q'] for g in gs}
    res['q_match'] = all(np.float32(res['q_indep'][k]) == np.float32(res['q_draft'][k]) for k in res['q_indep'])
    res['R_indep_g1'] = float(q_indep(blob, 1.0))    # g=1 → s11=1 → q=R
    # 3) 他の領域の大きさで (同じフレーム) R が S/5 になるか
    for sz_ in (48, 96, 192, 256):
        b2, _ = npf.build_profile(rgb, 1, (832, 0, sz_, sz_), with_floor=False, lzma=False)
        b2 = b2.tobytes()
        res[f'R_{sz_}'] = dict(indep=float(q_indep(b2, 1.0)), draft=qd.quality_from_profile(b2)['R'],
                              flat=qd.quality_from_profile(b2)['flatness'])
    out = os.path.join(HERE, 'recheck_results.json')
    json.dump(res, open(out, 'w'), indent=1, default=str)
    print(json.dumps(res, indent=1, default=str))

if __name__ == '__main__':
    main()
