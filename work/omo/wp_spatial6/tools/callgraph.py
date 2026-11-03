#!/usr/bin/env python3
"""Static pass for the integration check: the functions reachable from a root (arm64 slice, functions.csv bounds),
with every external call (stubs / imports), indirect branch (blr/br), and every page reference (adrp target +
the following add/ldr/str offset) of each.  usage: callgraph.py DISASM FUNCTIONS_CSV ROOT [--stop addr,...]"""
import bisect, csv, re, sys, collections

dis, fcsv, root = sys.argv[1], sys.argv[2], int(sys.argv[3], 16)
stop = set()
if "--stop" in sys.argv:
    stop = {int(x, 16) for x in sys.argv[sys.argv.index("--stop") + 1].split(",")}
fn = sorted((int(r["entry"], 16), int(r["size"])) for r in csv.DictReader(open(fcsv)))
starts = [a for a, _ in fn]
size = dict(fn)


def owner(a):
    i = bisect.bisect_right(starts, a) - 1
    return starts[i] if i >= 0 and a < starts[i] + size[starts[i]] else None


lines = collections.defaultdict(list)
for ln in open(dis):
    t = ln.split("\t")
    try:
        a = int(t[0], 16)
    except ValueError:
        continue
    o = owner(a)
    if o is not None:
        lines[o].append(t)
seen, order, todo = set(), [], [root]
info = {}
while todo:
    f = todo.pop()
    if f in seen:
        continue
    seen.add(f); order.append(f)
    calls, ext, ind, refs = set(), [], [], []
    L = lines[f]
    for i, t in enumerate(L):
        op = t[1].strip() if len(t) > 1 else ""
        args = t[2] if len(t) > 2 else ""
        if op in ("bl", "b") or op.startswith("b.") or op in ("cbz", "cbnz", "tbz", "tbnz"):
            m = re.findall(r"0x([0-9a-f]+)", args.split(";")[0])
            if m:
                tgt = int(m[-1], 16)
                if not (f <= tgt < f + size[f]):
                    if "symbol stub" in args or owner(tgt) is None:
                        ext.append((t[0], op, args.strip()))
                    else:
                        calls.add(owner(tgt))
        if op in ("blr", "br"):
            ind.append((t[0], op, args.strip()))
        if op in ("adrp", "adr"):
            m = re.search(r"; (0x[0-9a-f]+)", args)
            page = int(m.group(1), 16) if m else None
            nxt = L[i + 1] if i + 1 < len(L) else None
            off = ""
            if nxt is not None and len(nxt) > 2:
                mm = re.search(r"#(0x[0-9a-f]+|\d+)", nxt[2])
                off = "%s %s" % (nxt[1].strip(), nxt[2].split(";")[0].strip())
            refs.append((t[0], hex(page) if page else args.strip(), off, ("literal" in args)))
        if op == "ldr" and "literal pool" in args:
            refs.append((t[0], args.strip(), "", True))
        if op in ("svc", "hvc", "brk", "mrs", "msr"):
            ext.append((t[0], op, args.strip()))
    info[f] = (calls, ext, ind, refs)
    for c in calls:
        if c not in stop:
            todo.append(c)
for f in order:
    calls, ext, ind, refs = info[f]
    print("== FUN_%08x size %d: calls %s" % (f, size[f], " ".join("%x" % c for c in sorted(calls))))
    for e in ext:
        print("   EXT ", *e)
    for e in ind:
        print("   IND ", *e)
    for r in refs:
        print("   REF ", r[0], r[1], r[2])
print("TOTAL functions", len(order))
