#!/usr/bin/env python3
"""Leaf check of genuine functions (arm64 slice): print every branch/call target outside the function and every
adrp/page reference, so a function can be shown to call nothing outside itself and read only the NEON byte
0x14a67a8 or __TEXT __const.  usage: leafcheck.py DISASM FUNCTIONS_CSV addr [addr ...]"""
import csv, re, sys

dis, fcsv = sys.argv[1], sys.argv[2]
sizes = {int(r["entry"], 16): int(r["size"]) for r in csv.DictReader(open(fcsv))}
want = [int(a, 16) for a in sys.argv[3:]]
rng = {a: (a, a + sizes[a]) for a in want}
lines = {a: [] for a in want}
for ln in open(dis):
    if not ln[:1].isdigit() and not ln[:1] in "abcdef":
        continue
    try:
        ad = int(ln.split("\t", 1)[0], 16)
    except ValueError:
        continue
    for a, (lo, hi) in rng.items():
        if lo <= ad < hi:
            lines[a].append(ln.rstrip("\n"))
for a in want:
    lo, hi = rng[a]
    out, glob = [], []
    for ln in lines[a]:
        f = ln.split("\t")
        op = f[1] if len(f) > 1 else ""
        args = f[2] if len(f) > 2 else ""
        if op in ("bl", "blr", "br", "b") or op.startswith("b.") or op in ("cbz", "cbnz", "tbz", "tbnz"):
            m = (re.findall(r"0x([0-9a-f]+)", args.split(";")[0]) or [None])[-1]
            if op in ("blr", "br") or not m or not (lo <= int(m, 16) < hi):
                out.append(ln)
        if op in ("adrp", "adr", "ldr") and ("literal" in args or op != "ldr"):
            glob.append(ln)
        if op in ("ret",):
            pass
    print(f"== 0x{a:x} size {hi - lo} ({len(lines[a])} instr): external branches {len(out)}, page refs {len(glob)}")
    for ln in out: print("  BR ", ln)
    for ln in glob: print("  REF", ln)
    print("  stack guard:", any("stack_chk" in ln for ln in lines[a]))
