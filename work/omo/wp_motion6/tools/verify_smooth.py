#!/usr/bin/env python3
"""Bit-exact check of the FUN_0098a94c transcription (the level-0 smoothing pass, with FUN_0098a79c the
5x5 neighbour average) against the genuine via `nvh smooth` / `nvh smoothfull`.

  python3 verify_smooth.py [--seed S] [--cases N] [--log FILE]

The transcription is written straight from the disassembly (2026-11-03).  Every mismatch is a decode error.
"""
import argparse, os, struct, subprocess, sys, tempfile
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", "..", ".."))
NVH = os.path.join(ROOT, "reference", "genuine", "analysis", "harness", "nvh")
ENV = dict(os.environ, NV_BUNDLE=os.path.join(ROOT, "reference", "genuine", "NeatVideo6.ofx.bundle",
                                              "Contents", "MacOS", "NeatVideo6.ofx"))
REC = None
import importlib.util
spec = importlib.util.spec_from_file_location("va", os.path.join(HERE, "verify_adbc.py"))
va = importlib.util.module_from_spec(spec)
spec.loader.exec_module(va)
REC = va.REC
F32 = np.float32
MAX_SAD = 0x7FFFFFFF


def f32(v):
    return F32(v)


def neighbor_avg(records, nx, ny, col, row):
    """FUN_0098a79c: the 5x5 neighbour average.  Returns (ok, smx, smy)."""
    own = records[row * nx + col]
    c0, c1 = max(col - 2, 0), min(col + 2, nx - 1)
    r0, r1 = max(row - 2, 0), min(row + 2, ny - 1)
    sum_x = f32(0.0)
    sum_y = f32(0.0)
    count = 0
    up = down = left = right = 0
    for r in range(r0, r1 + 1):
        dwn = 1 if r > row else 0
        upn = 1 if r < row else 0
        for c in range(c0, c1 + 1):
            rec = records[r * nx + c]
            if int(rec["sad"]) == MAX_SAD:      # the SAD field, read as i32, == 0x7fffffff -> skip
                continue
            if abs(f32(f32(rec["vx"]) - f32(own["vx"]))) > f32(2.0):
                continue
            if abs(f32(f32(rec["vy"]) - f32(own["vy"]))) > f32(2.0):
                continue
            sum_x = f32(sum_x + f32(rec["vx"]))
            sum_y = f32(sum_y + f32(rec["vy"]))
            count += 1
            left += 1 if c < col else 0
            right += 1 if c > col else 0
            up += upn
            down += dwn
    if not (up >= 4 and down >= 4 and left >= 4 and right > 3):
        return False, f32(0.0), f32(0.0)
    n = f32(count)
    return True, f32(sum_x / n), f32(sum_y / n)


def sad32(ref, cur, ay, ax, cy, cx):
    return va.sad_block(ref, cur, ay, ax, cy, cx, 32)


def smooth_pass(ref, cur, W, H, records, scale):
    """FUN_0098a94c: one smoothing pass over all records (the harness runs one pass over all rows)."""
    nx, ny = W // 16, H // 16
    for row in range(ny):
        for col in range(nx):
            rec = records[row * nx + col]
            rec["sx"] = f32(rec["vx"])
            rec["sy"] = f32(rec["vy"])
            if rec["flag32"] == 0:
                continue
            ok, smx, smy = neighbor_avg(records, nx, ny, col, row)
            if not ok:
                continue
            if (int(np.rint(f32(rec["vx"]))) == int(np.rint(smx))
                    and int(np.rint(f32(rec["vy"]))) == int(np.rint(smy))):
                rec["sx"] = smx
                rec["sy"] = smy
                continue
            anchor_x = min(max(int(rec["x0"]) - 8, 0), W - 32)
            anchor_y = min(max(int(rec["y0"]) - 8, 0), H - 32)
            cx = anchor_x + int(np.rint(smx))
            cy = anchor_y + int(np.rint(smy))
            if not (-8 <= cx <= W - 24 and -8 <= cy <= H - 24):
                continue
            s = sad32(ref, cur, cy, cx, anchor_y, anchor_x)
            thr = f32(f32(f32(rec["slack"]) * f32(scale)) + f32(F32(int(rec["sad"]) << 2)))
            if thr > f32(s):
                rec["sx"] = smx
                rec["sy"] = smy


def smooth_full(ref, cur, W, H, records):
    """FUN_0098ab5c: smooth(1.0) -> copy sx/sy into vx/vy -> smooth(2.0)."""
    smooth_pass(ref, cur, W, H, records, 1.0)
    for rec in records:
        rec["vx"] = f32(rec["sx"])
        rec["vy"] = f32(rec["sy"])
    smooth_pass(ref, cur, W, H, records, 2.0)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--cases", type=int, default=6)
    ap.add_argument("--log")
    a = ap.parse_args()
    rng = np.random.default_rng(a.seed)
    tmp = tempfile.mkdtemp(prefix="smooth_")
    bad = 0
    log = open(a.log, "w") if a.log else None
    for i in range(a.cases):
        W = int(rng.integers(64, 513))
        H = int(rng.integers(64, 361))
        base = (rng.random((H, W)) * 255).astype(np.float32)
        xx, yy = np.meshgrid(np.arange(W), np.arange(H))
        base += 40 * np.sin(xx / (8 + rng.integers(4, 30))) * np.sin(yy / (8 + rng.integers(4, 30)))
        shift = (int(rng.integers(-6, 7)), int(rng.integers(-6, 7)))
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
                # plausible vectors around the true shift so the smoothing has signal
                rec[b]["vx"] = np.float32(shift[1] + rng.normal(0, 1.5))
                rec[b]["vy"] = np.float32(shift[0] + rng.normal(0, 1.5))
                rec[b]["sad"] = int(rng.integers(0, 30000))
                rec[b]["slack"] = np.float32(rng.integers(0, 3))
                rec[b]["flag32"] = 1 if rng.random() < 0.7 else 0
        rec_path = os.path.join(tmp, f"rec{i}.bin")
        rec.tofile(rec_path)
        mode = "smoothfull" if i % 2 == 0 else "smooth"
        scale = 1.0 if i % 4 == 1 else 2.0
        ref.reshape(-1).tofile(os.path.join(tmp, "ref.f32"))
        cur.reshape(-1).tofile(os.path.join(tmp, "cur.f32"))
        args = [NVH, mode, str(W), str(H),
                os.path.join(tmp, "ref.f32"), os.path.join(tmp, "cur.f32"),
                rec_path, str(nblocks)]
        if mode == "smooth":
            args.append(repr(scale))
        r = subprocess.run(args, capture_output=True, text=True, env=ENV)
        if r.returncode != 0:
            raise RuntimeError(r.stderr[-400:])
        got = np.fromfile(rec_path + ".out.bin", dtype=REC)
        want = rec.copy()
        ref8 = va.pad8(np.floor(ref + 0.5).astype(np.uint8))
        cur8 = va.pad8(np.floor(cur + 0.5).astype(np.uint8))
        if mode == "smoothfull":
            smooth_full(ref8, cur8, W, H, want)
        else:
            smooth_pass(ref8, cur8, W, H, want, scale)
        if not np.array_equal(got, want):
            bad += 1
            diffs = np.where(got != want)[0]
            d0 = int(diffs[0])
            msg = f"case {i} ({mode}): mismatch at record {d0}: genuine {got[d0]} != transcription {want[d0]}"
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
