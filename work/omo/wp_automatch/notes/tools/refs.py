#!/usr/bin/env python3
# usage: refs.py dis.txt functions.csv addr[:len] ...   find adrp+add/ldr refs into [addr, addr+len)
import sys, re, bisect
dis, fcsv = sys.argv[1], sys.argv[2]
T = []
for s in sys.argv[3:]:
    a, _, n = s.partition(':')
    T.append((int(a, 16), int(n, 0) if n else 1))
F = []
for l in open(fcsv).read().split('\n')[1:]:
    if l: F.append(int(l.split(',')[0], 16))
F.sort()
def fn(a):
    i = bisect.bisect_right(F, a) - 1
    return F[i] if i >= 0 else 0
rx = re.compile(r'^([0-9a-f]{16})\s+(\S+)\s+(.*)$')
pg = {}
for line in open(dis):
    m = rx.match(line)
    if not m: continue
    a = int(m.group(1), 16); op = m.group(2); args = m.group(3)
    if op == 'adrp':
        r = args.split(',')[0].strip(); t = re.search(r';\s*0x([0-9a-f]+)', args)
        if t: pg[r] = int(t.group(1), 16)
        continue
    if op == 'adr':
        r = args.split(',')[0].strip(); t = re.search(r'#(-?0x[0-9a-f]+)', args)
        if t:
            tt = a + int(t.group(1), 0)
            for lo, n in T:
                if lo <= tt < lo + n: print('%#x fn %#x adr -> %#x' % (a, fn(a), tt))
        continue
    if op in ('add', 'ldr', 'str', 'ldrb', 'ldrh', 'ldrsw', 'strb', 'strh', 'ldp', 'stp', 'ldur', 'ldrsb', 'ldrsh'):
        ps = [p.strip() for p in args.split(';')[0].split(',')]
        bi = 2 if op in ('ldp', 'stp') else 1
        if len(ps) <= bi: continue
        base = ps[bi].strip('[]! ')
        if base in pg:
            off = 0
            if len(ps) > bi + 1:
                im = re.match(r'#(-?0x[0-9a-f]+|-?\d+)', ps[bi + 1].strip('[]! '))
                off = int(im.group(1), 0) if im else 0
            t = pg[base] + off
            for lo, n in T:
                if lo <= t < lo + n:
                    print('%#x fn %#x %s %s -> %#x' % (a, fn(a), op, args.split(';')[0].strip(), t))
