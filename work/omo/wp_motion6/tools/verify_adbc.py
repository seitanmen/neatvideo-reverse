#!/usr/bin/env python3
"""Bit-exact check of the FUN_0098adbc transcription (per-block candidate verification and acceptance of the
block matching) against the genuine via `nvh adbc`.

  python3 verify_adbc.py [--seed S] [--cases N] [--blocks N] [--log FILE]

The transcription below is written straight from the disassembly (2026-11-03).  Every mismatch is a decode
error to fix, not a tolerance issue.  All float arithmetic is float32, in the order of the instructions.

Record layout (0x44 bytes, as written by FUN_0098a368 / FUN_009896fc):
  +0x00 i32 x0   +0x04 i32 y0   +0x08 f32 vx  +0x0c f32 vy
  +0x10/+0x20 f32 candidate 0   +0x14/+0x24 candidate 1   +0x18/+0x28 candidate 2   +0x1c/+0x2c candidate 3
  +0x30/+0x34 f32 smoothed      +0x38 f32 slack           +0x3c i32 sad   +0x40 u8 flag32   +0x41 u8 flag41
"""
import argparse, os, struct, subprocess, sys, tempfile
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
NVH = os.path.join(ROOT, "reference", "genuine", "analysis", "harness", "nvh")
ENV = dict(os.environ, NV_BUNDLE=os.path.join(ROOT, "reference", "genuine", "NeatVideo6.ofx.bundle",
                                              "Contents", "MacOS", "NeatVideo6.ofx"))
NB = [(-1, -1), (0, -1), (1, -1), (-1, 0), (1, 0), (-1, 1), (0, 1), (1, 1)]  # DAT_014877f8
REC = np.dtype([
    ("x0", "<i4"), ("y0", "<i4"), ("vx", "<f4"), ("vy", "<f4"),
    # the candidates are interleaved: the four x at +0x10..+0x1c, the four y at +0x20..+0x2c
    ("c0x", "<f4"), ("c1x", "<f4"), ("c2x", "<f4"), ("c3x", "<f4"),
    ("c0y", "<f4"), ("c1y", "<f4"), ("c2y", "<f4"), ("c3y", "<f4"),
    ("sx", "<f4"), ("sy", "<f4"), ("slack", "<f4"), ("sad", "<i4"),
    ("flag32", "u1"), ("flag41", "u1"), ("pad", "V2"),
])
MAX_SAD = 0x7FFFFFFF
F32 = np.float32
BS = 16


def f32(v):
    return F32(v)


def pad8(plane):
    h, w = plane.shape
    out = np.empty((h + 16, w + 16), np.uint8)
    out[8:8 + h, 8:8 + w] = plane
    out[:8, 8:8 + w] = plane[0]
    out[8 + h:, 8:8 + w] = plane[h - 1]
    out[:, :8] = out[:, 8:9]
    out[:, 8 + w:] = out[:, 7 + w:8 + w]
    return out


def sad16(ref, cur, ay, ax, cy, cx):
    """SAD of the 16x16 ref block at plane (ax, ay) and the cur block at plane (cx, cy).
    The coordinates are plane coordinates; the padded arrays carry an 8px margin."""
    a = ref[ay + 8:ay + 24, ax + 8:ax + 24].astype(np.int32)
    b = cur[cy + 8:cy + 24, cx + 8:cx + 24].astype(np.int32)
    return int(np.abs(a - b).sum())


def pattern_search(ref, cur, W, H, cx, cy, x, y, rounds=2):
    """FUN_0098b9a4: the 16x16 pattern search.  Returns (ret, out_x, out_y, out_sad)."""
    bs = 16
    r = 2 * rounds - 1
    fast = (y + r <= H + 8 - bs) and (y - r >= -8) and (x - r >= -8) and (x + r <= W + 8 - bs)
    if not fast:
        if not ((x + 8 >= 0 and x + 8 <= W + 16 - bs) and (y + 8 >= 0 and y + 8 <= H + 16 - bs)):
            return 0x7FFFFFFF, x, y, 0x7FFFFFFF
    best = sad16(ref, cur, y, x, cy, cx)
    ret = best
    pages = [[(dx * 2, dy * 2) for dx, dy in NB], [(dx, dy) for dx, dy in NB]]
    # the callee halves the round counter, so rounds of 3 and more still give 2 rounds
    for rnd in range(min(rounds, 2)):
        improved = None
        for dx, dy in pages[rnd]:
            tx, ty = x + dx, y + dy
            if not fast:
                if not (tx + 8 >= 0 and tx + 8 <= W + 16 - bs and ty + 8 >= 0 and ty + 8 <= H + 16 - bs):
                    continue
            s = sad16(ref, cur, ty, tx, cy, cx)
            if s < best:
                best, improved = s, (dx, dy)
        if improved is not None:
            x += improved[0]
            y += improved[1]
    return ret, x, y, best


def refine32(ref, cur, W, H, cx, cy, x, y, thr):
    """FUN_0098bf18: the 32x32 search with the SAD weighted centroid subpixel correction.
    Returns (ret, out_x, out_y, out_sad)."""
    offs = NB
    base = sad16(ref, cur, y, x, cy, cx)
    fast = -7 <= x < W - 24 and -7 <= y < H - 24
    sads = []
    for dx, dy in offs:
        tx, ty = x + dx, y + dy
        if not fast and not (0 <= tx + 8 <= W - 16 and 0 <= ty + 8 <= H - 16):
            sads.append(0x7FFFFFFF)
        else:
            sads.append(sad16(ref, cur, ty, tx, cy, cx))
    best = min([base] + sads)
    if best == 0x7FFFFFFF or not (0 <= x + 8 <= W - 16 and 0 <= y + 8 <= H - 16):
        return base, x, y, 0x7FFFFFFF
    inv = f32(1.0) / f32(f32(thr) + f32(0.01))
    num_x = f32(0.0)
    num_y = f32(0.0)
    den = f32(0.0)
    d = f32(inv * f32(F32(base - best)))
    if d < f32(1.5151515):
        den = f32(f32(d * f32(-0.66)) + f32(1.0)) + f32(0.0)
    for (dx, dy), s in zip(offs, sads):
        d = f32(inv * f32(F32(s - best)))
        if d < f32(1.5151515):
            w = f32(f32(d * f32(-0.66)) + f32(1.0))
            num_x = f32(num_x + f32(f32(dx) * w))
            num_y = f32(num_y + f32(f32(dy) * w))
            den = f32(den + w)
    out_x = f32(f32(num_x / den) + f32(x))
    out_y = f32(f32(num_y / den) + f32(y))
    return base, out_x, out_y, best


def adbc(ref, cur, W, H, records, weights, s140, s168, sd4, slvl, simg, s11, chunkdiv=1):
    """Transcription of FUN_0098adbc.  records is a numpy structured array (REC), mutated in place.
    The chunking does not influence the result: the 1-in-16 test only gates the (inert) obfuscation trap."""
    bs = 16
    # the weight products, in the order of the instructions (0x98b2f0..0x98b2fc)
    s8 = f32(s140)
    if s168 != 1:
        s8 = f32(s8 * f32(sd4))
    s0 = f32(s8 * f32(slvl))
    s8 = f32(s11 * s0)
    s9 = f32(s0 * f32(simg))
    for block in range(len(records)):
        r = records[block]
        r["sad"] = MAX_SAD
        r["flag32"] = 0
        x0, y0 = int(r["x0"]), int(r["y0"])
        # 1) the pattern search from the current vector
        ret1, out_x1, out_y1, out_sad1 = pattern_search(
            ref, cur, W, H, cx=x0, cy=y0,
            x=x0 + int(np.rint(F32(r["vx"]))), y=y0 + int(np.rint(F32(r["vy"]))), rounds=2)
        sad_first = ret1
        # 2) the candidates of the record, verified with the SAD
        best, best_x, best_y = MAX_SAD, 0, 0
        vx_i = int(np.rint(F32(r["vx"])))
        vy_i = int(np.rint(F32(r["vy"])))
        for cx_f, cy_f in ((r["c0x"], r["c0y"]), (r["c1x"], r["c1y"]), (r["c2x"], r["c2y"]), (r["c3x"], r["c3y"])):
            dx = abs(int(np.rint(F32(cx_f))) - vx_i)
            dy = abs(int(np.rint(F32(cy_f))) - vy_i)
            if max(dx, dy) < 4:
                continue
            ax = int(np.rint(F32(cx_f))) + x0
            ay = int(np.rint(F32(cy_f))) + y0
            if not (-8 <= ax <= W - 8 and -8 <= ay <= H - 8):
                continue
            s = sad16(ref, cur, ay, ax, y0, x0)
            if s < best:
                best, best_x, best_y = s, ax, ay
        out_x, out_y, best_sad = out_x1, out_y1, out_sad1
        if best != MAX_SAD:
            # 3) another pattern search from the best candidate, adopted when it beats the first search
            _, out_x2, out_y2, out_sad2 = pattern_search(ref, cur, W, H, cx=x0, cy=y0, x=best_x, y=best_y, rounds=2)
            if out_sad2 != MAX_SAD and out_sad2 < out_sad1:
                out_x, out_y, best_sad = out_x2, out_y2, out_sad2
        elif out_sad1 == MAX_SAD:
            continue
        # 4) the acceptance: 2 * (s8 * weight) + best < SAD_first
        w = f32(s8 * F32(weights[block]))
        s1 = f32(f32(w + w) + f32(F32(best_sad)))
        if s1 < f32(F32(sad_first)):
            r["vx"] = F32(out_x - x0)
            r["vy"] = F32(out_y - y0)
            r["sad"] = best_sad & 0xFFFFFFFF
        else:
            r["sad"] = sad_first & 0xFFFFFFFF
            # 5) the 32x32 refinement, on every block as long as W > 31 and H > 31
            if W > 31 and H > 31:
                anchor_x = min(max(x0 - 8, 0), W - 32)
                anchor_y = min(max(y0 - 8, 0), H - 32)
                slack = f32(s9 * F32(weights[block]))
                r["slack"] = slack
                thr = f32(slack * f32(1.1))
                _, ox, oy, osad = refine32(ref, cur, W, H, cx=x0, cy=y0,
                                           x=anchor_x + int(np.rint(F32(r["vx"]))),
                                           y=anchor_y + int(np.rint(F32(r["vy"]))), thr=thr)
                if osad != MAX_SAD:
                    r["vx"] = F32(F32(ox) - F32(anchor_x))
                    r["vy"] = F32(F32(oy) - F32(anchor_y))
                    r["sad"] = (osad >> 2) & 0xFFFFFFFF
                    r["flag32"] = 1


def run_case(nvh, tmp, ref, cur, rec_path, nblocks, settings):
    rf, cf = os.path.join(tmp, "ref.f32"), os.path.join(tmp, "cur.f32")
    ref.tofile(rf)
    cur.tofile(cf)
    args = [nvh, "adbc", str(settings["W"]), str(settings["H"]), rf, cf, rec_path, str(nblocks)]
    args += [repr(float(v)) for v in (settings["s140"], settings["s168"], settings["sd4"], settings["slvl"],
                                      settings["simg"], settings["s11"], settings["chunkdiv"])]
    r = subprocess.run(args, capture_output=True, text=True, env=ENV)
    if r.returncode != 0:
        raise RuntimeError(r.stderr[-400:])
    out = np.fromfile(rec_path + ".out.bin", dtype=REC)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--cases", type=int, default=20)
    ap.add_argument("--log")
    a = ap.parse_args()
    rng = np.random.default_rng(a.seed)
    tmp = tempfile.mkdtemp(prefix="adbc_")
    bad = 0
    log = open(a.log, "w") if a.log else None
    for i in range(a.cases):
        W = int(rng.integers(48, 513))
        H = int(rng.integers(48, 361))
        base = (rng.random((H, W)) * 255).astype(np.float32)
        xx, yy = np.meshgrid(np.arange(W), np.arange(H))
        base += 40 * np.sin(xx / (8 + rng.integers(4, 30))) * np.sin(yy / (8 + rng.integers(4, 30)))
        shift = (int(rng.integers(-8, 9)), int(rng.integers(-8, 9)))
        moved = np.roll(np.roll(base, shift[0], 0), shift[1], 1)
        ref = np.clip(base + rng.normal(0, 5, (H, W)), 0, 255).astype(np.float32)
        cur = np.clip(moved + rng.normal(0, 5, (H, W)), 0, 255).astype(np.float32)
        nx, ny = W // 16, H // 16
        nblocks = nx * ny
        rec = np.zeros(nblocks, dtype=REC)
        for r_ in range(ny):
            for c_ in range(nx):
                b = r_ * nx + c_
                rec[b]["x0"] = min(16 * c_, W - 16)
                rec[b]["y0"] = min(16 * r_, H - 16)
                rec[b]["sad"] = MAX_SAD
                # candidates: the propagation of the coarser level (2x the parent vector), here random
                for k in range(4):
                    rec[b][f"c{k}x"] = np.float32(rng.normal(0, 6))
                    rec[b][f"c{k}y"] = np.float32(rng.normal(0, 6))
        vx0 = np.float32(rng.normal(0, 4))
        vy0 = np.float32(rng.normal(0, 4))
        rec["vx"] = vx0
        rec["vy"] = vy0
        rec_path = os.path.join(tmp, f"rec{i}.bin")
        rec.tofile(rec_path)
        settings = {"W": W, "H": H, "s140": 1.0, "s168": 1, "sd4": 1.0, "slvl": 1.0, "simg": 1.0,
                    "s11": 1.0, "chunkdiv": 1}
        got = run_case(NVH, tmp, ref.reshape(-1), cur.reshape(-1), rec_path, nblocks, settings)
        want = rec.copy()
        adbc(pad8(np.floor(ref + 0.5).astype(np.uint8)), pad8(np.floor(cur + 0.5).astype(np.uint8)),
             W, H, want, np.ones(nblocks, np.float32),
             settings["s140"], settings["s168"], settings["sd4"], settings["slvl"],
             settings["simg"], settings["s11"], settings["chunkdiv"])
        if not np.array_equal(got, want):
            bad += 1
            diffs = np.argmax(got != want)
            msg = (f"case {i}: W={W} H={H} n={nblocks}: mismatch at record {diffs} "
                   f"genuine {got[diffs]} != transcription {want[diffs]}")
            print(msg)
            if log:
                log.write(msg + "\n")
            if bad > 4:
                break
    print(f"cases {a.cases} mismatches {bad}")
    if log:
        log.close()


if __name__ == "__main__":
    main()
