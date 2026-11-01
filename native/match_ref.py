"""Python reference of the repro Auto Match score (N4e; native/nvcore_c_match.h states the same formula).

Not a genuine function: the genuine Auto Match is an empty slot (work/omo/wp_automatch/DECODE.md).  The score is
built only from ports that are bit-exact with the genuine or with their Python sources:
  nv_model.model_full (FUN_00995250), nv_curve.is_clip_genuine (FUN_009c6f88),
  aft_port nv_aft.similarity (FUN_009b6338) and nv_aft._penalty (FUN_009d49b4).

  match_model(prof)                      -> (faces 3 x 492 float32, stats 3 x 51 float32) of a 0xc300-byte profile
  tile_models(rgb)                       -> (ntiles, [(mR, mG, mB) or None (clipped)]) of the AFT grid
  scores(tiles, ref, faces_list, stats_list) -> [(score, accept)] per model, used tiles, reference accept
  match_score(rgb, ref, faces_list, stats_list) -> (status, scores, accept, info) as nvc_match_score
  (ref = (faces, stats) of the frame's own model; score = fuzzy Jaccard index sum min(p_k, r) / sum max(p_k, r))

Run with numpy 2.1.3 / scipy 1.15.3 / Python 3.13 (the other ports' verification environment).
"""
import os
import sys

import numpy as np

_HERE = os.path.dirname(os.path.abspath(__file__))
_ROOT = os.path.abspath(os.path.join(_HERE, ".."))
for _p in (os.path.join(_ROOT, "reference", "python"),
           os.path.join(_ROOT, "reference", "genuine", "analysis", "noise", "aft_port", "src")):
    if _p not in sys.path:
        sys.path.insert(0, _p)
import nv_aft as aft            # noqa: E402  (aft_port: similarity, _penalty, FACE, STATS)
import nv_curve as ncv          # noqa: E402
import nv_model as nm           # noqa: E402

F32 = np.float32
NFACE = 492
NSTATS = 51
CLIP = F32(0.33)                # nv_aft CLIP (0x3ea8f5c3)
HALF = F32(0.5)
S_LVL = aft.S_LVL[0]            # 3.0f (the AFT's mode 0, as qtui runs it)
S_MULT = aft.S_MULT[0]          # 4.0f


def match_model(prof):
    b = np.frombuffer(bytes(prof), np.uint8)
    faces = [b[o:o + 4 * NFACE].view(F32).copy() for o in aft.FACE]
    stats = [b[o:o + 4 * NSTATS].view(F32).copy() for o in aft.STATS]
    return faces, stats


def _face_term(m, F, S):
    with np.errstate(all="ignore"):
        sim = aft.similarity(m, F, S, S_LVL, S_MULT)
        l5 = aft._penalty(F32(abs(F32(F32(F[5]) / F32(m[5])))), S_LVL)
        l6 = aft._penalty(F32(abs(F32(F32(F[6]) / F32(m[6])))), S_LVL)
        return F32(sim * F32(l5 * l6))


def tile_models(rgb):
    """The used tiles of the AFT grid (row-major), None for a clipped tile."""
    rgb = np.ascontiguousarray(rgb, F32)
    H, W = rgb.shape[:2]
    nx, ny = W // 32, H // 32
    out = []
    for ty in range(ny):
        for tx in range(nx):
            t = rgb[32 * ty:32 * ty + 32, 32 * tx:32 * tx + 32]
            R = np.ascontiguousarray(t[..., 0], F32)
            G = np.ascontiguousarray(t[..., 1], F32)
            B = np.ascontiguousarray(t[..., 2], F32)
            with np.errstate(all="ignore"):
                if ncv.is_clip_genuine(R) > CLIP or ncv.is_clip_genuine(G) > CLIP or ncv.is_clip_genuine(B) > CLIP:
                    out.append(None)
                    continue
            out.append(tuple(nm.model_full(p, 2, None, 2).view(F32) for p in (R, G, B)))
    return nx * ny, out


def _product(mR, mG, mB, F, S):
    qR = _face_term(mR, F[0], S[0])
    qG = _face_term(mG, F[1], S[1])
    qB = _face_term(mB, F[2], S[2])
    with np.errstate(all="ignore"):
        p = F32(F32(qG * qR) * qB)
    return F32(0.0) if np.isnan(p) else p


def scores(tiles, ref, faces_list, stats_list):
    used = [t for t in tiles if t is not None]
    r = [_product(mR, mG, mB, ref[0], ref[1]) for mR, mG, mB in used]
    racc = sum(1 for x in r if x > HALF)
    res = []
    for F, S in zip(faces_list, stats_list):
        num = 0.0
        den = 0.0
        acc = 0
        for (mR, mG, mB), rt in zip(used, r):
            p = _product(mR, mG, mB, F, S)
            num += float(min(p, rt))
            den += float(max(p, rt))
            if p > HALF:
                acc += 1
        res.append((num / den if den > 0.0 else 0.0, acc))
    return res, len(used), racc


def match_score(rgb, ref, faces_list, stats_list):
    H, W = rgb.shape[:2]
    n = len(faces_list)
    if W < 48 or H < 48:
        return 1, [0.0] * n, [0] * n, (0, 0, 0)
    nt, tiles = tile_models(rgb)
    res, used, racc = scores(tiles, ref, faces_list, stats_list)
    if used == 0:
        return 4, [0.0] * n, [0] * n, (nt, 0, 0)
    return 0, [r[0] for r in res], [r[1] for r in res], (nt, used, racc)
