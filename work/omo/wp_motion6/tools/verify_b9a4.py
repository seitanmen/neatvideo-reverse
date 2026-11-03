#!/usr/bin/env python3
"""Bit-exact check of the FUN_0098b9a4 transcription (the 16x16 pattern search) against the genuine via
`nvh b9a4` (reference/genuine/analysis/harness).  The transcription below is written straight from the
disassembly (2026-11-03, decode session); every mismatch is a decode error to fix, not a tolerance issue.

  python3 verify_b9a4.py [--seed S] [--cases N] [--log FILE]

Inputs: random planes built from real frames (../..../../../test.mp4 crops) plus noise, random block positions
(including the borders) and random start displacements (including out-of-range starts that exercise the slow
path and the failure exit).  Compared: ret (base SAD or 0x7fffffff), out_x, out_y, out_sad - exactly.
"""
import argparse, os, subprocess, sys, tempfile
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
NVH = os.path.join(ROOT, "reference", "genuine", "analysis", "harness", "nvh")
ENV = dict(os.environ, NV_BUNDLE=os.path.join(ROOT, "reference", "genuine", "NeatVideo6.ofx.bundle",
                                              "Contents", "MacOS", "NeatVideo6.ofx"))
NB = [(-1, -1), (0, -1), (1, -1), (-1, 0), (1, 0), (-1, 1), (0, 1), (1, 1)]  # DAT_014877f8


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
    # the coordinates are plane coordinates (the harness's origin = plane (0,0)); the padded arrays carry an
    # 8px margin, so the block at plane (ax, ay) lives at [ay+8, ax+8]
    a = ref[ay + 8:ay + 24, ax + 8:ax + 24].astype(np.int32)
    b = cur[cy + 8:cy + 24, cx + 8:cx + 24].astype(np.int32)
    return int(np.abs(a - b).sum()) & 0xFFFFFFFFFFFFFFFF


def genuine_b9a4(ref, cur, W, H, cx, cy, vx, vy, rounds=2):
    """The transcription of FUN_0098b9a4.  ref/cur are the padded (H+16, W+16) uint8 planes with the origin at
    [8, 8]; coordinates below are in the padded plane with (0, 0) = the plane origin."""
    bs = 16
    x = cx + int(np.rint(np.float32(vx)))
    y = cy + int(np.rint(np.float32(vy)))
    r = 2 * rounds - 1

    def blk(ax, ay):
        return sad16(ref, cur, ay, ax, cy, cy and cy or cy) if False else sad16(ref, cur, ay, ax, cy, cx)

    fast = (y + r <= H + 8 - bs) and (y - r >= -8) and (x - r >= -8) and (x + r <= W + 8 - bs)
    if not fast:
        # the base check is unsigned: (x + 8) <= W + 16 - bs and (y + 8) <= H + 16 - bs
        if not ((x + 8 >= 0 and x + 8 <= W + 16 - bs) and (y + 8 >= 0 and y + 8 <= H + 16 - bs)):
            return 0x7FFFFFFF, x, y, 0x7FFFFFFF
    best = sad16(ref, cur, y, x, cy, cx)  # ret / running best
    ret = best
    pages = []
    for scale in (2, 1):
        page = []
        for dx, dy in NB:
            page.append((dx * scale, dy * scale))
        pages.append(page)
    for rnd in range(rounds if rounds < 3 else 2):  # the callee halves x24: rounds 3+ still give 2 rounds
        page = pages[0] if rnd == 0 else pages[1]
        improved = None
        for dx, dy in page:  # scan order, strict unsigned <, first wins ties (0x9f8ddc returns 0 on tie)
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


def run_case(nvh, tmp, ref, cur, W, H, cx, cy, vx, vy, rounds):
    rf, cf = os.path.join(tmp, "ref.f32"), os.path.join(tmp, "cur.f32")
    ref.tofile(rf)
    cur.tofile(cf)
    r = subprocess.run([nvh, "b9a4", str(W), str(H), rf, cf, str(cx), str(cy), repr(float(vx)), repr(float(vy)),
                        str(rounds)], capture_output=True, text=True, env=ENV)
    if r.returncode != 0:
        raise RuntimeError(r.stderr[-300:])
    toks = r.stdout.split()
    vals = {}
    for t in toks[1:]:
        k, v = t.split("=", 1)
        vals[k] = int(v)
    return vals["ret"], vals["out_x"], vals["out_y"], vals["sad"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--cases", type=int, default=200)
    ap.add_argument("--log")
    a = ap.parse_args()
    rng = np.random.default_rng(a.seed)
    tmp = tempfile.mkdtemp(prefix="b9a4_")
    bad = 0
    log = open(a.log, "w") if a.log else None
    for i in range(a.cases):
        W = int(rng.integers(48, 513))
        H = int(rng.integers(48, 361))
        base = (rng.random((H, W)) * 255).astype(np.float32)
        # structure: smooth blobs + edges so the search has signal
        xx, yy = np.meshgrid(np.arange(W), np.arange(H))
        base += 40 * np.sin(xx / (8 + rng.integers(4, 30))) * np.sin(yy / (8 + rng.integers(4, 30)))
        shift = (int(rng.integers(-6, 7)), int(rng.integers(-6, 7)))
        moved = np.roll(np.roll(base, shift[0], 0), shift[1], 1)
        ref = np.clip(base + rng.normal(0, 4, (H, W)), 0, 255).astype(np.float32)
        cur = np.clip(moved + rng.normal(0, 4, (H, W)), 0, 255).astype(np.float32)
        ref8 = pad8(np.floor(ref + 0.5).astype(np.uint8))   # the harness converts with (unsigned char)(v + 0.5f)
        cur8 = pad8(np.floor(cur + 0.5).astype(np.uint8))
        cx = int(rng.integers(0, W - 15))
        cy = int(rng.integers(0, H - 15))
        # half the cases: wild vectors (out-of-range starts, the failure exit), half: plausible
        if rng.random() < 0.5:
            vx, vy = float(rng.integers(-80, 81)), float(rng.integers(-80, 81))
        else:
            vx, vy = float(rng.normal(0, 8)), float(rng.normal(0, 8))
        vx += 0.5 * (rng.random() < 0.3)  # exercise frintn ties
        rounds = 2 if rng.random() < 0.8 else int(rng.integers(1, 4))
        got = run_case(NVH, tmp, ref.reshape(-1), cur.reshape(-1), W, H, cx, cy, vx, vy, rounds)
        want = genuine_b9a4(ref8, cur8, W, H, cx, cy, vx, vy, rounds)
        if got != want:
            bad += 1
            msg = f"case {i}: W={W} H={H} c=({cx},{cy}) v=({vx},{vy}) r={rounds}: genuine {got} != transcription {want}"
            print(msg)
            if log:
                log.write(msg + "\n")
            if bad > 8:
                break
    print(f"cases {a.cases} mismatches {bad}")
    if log:
        log.close()


if __name__ == "__main__":
    main()
