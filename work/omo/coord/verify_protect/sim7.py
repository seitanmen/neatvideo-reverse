#!/usr/bin/env python3
"""Fixed simulator (immediates parsed) for the chain 0x99aabc..0x99ac44 (FUN_0099a76c).
Determines the exact 64-bit value fed to scvtf s0 (claim H) as a function of
w22 (obfuscation state) and [sp+0x64] (= raw state word w10)."""
import random

M32 = 0xFFFFFFFF
M64 = 0xFFFFFFFFFFFFFFFF

def load():
    prog = []
    for ln in open('chain_listing.txt'):
        parts = ln.rstrip().split('\t')
        if len(parts) < 3:
            continue
        prog.append((int(parts[0], 16), ' '.join(parts[1:]).split()))
    return prog

def imm(t):
    if t.startswith('#'):
        return ('imm', int(t[1:], 0))
    return ('reg', t)

def run(prog, w22, sp64):
    R = {'w22': w22 & M32}

    def val(t):
        k, v = imm(t)
        if k == 'imm':
            return v
        return get(v)

    def get(r):
        if r in R:
            return R[r]
        if r.startswith('x'):
            w = 'w' + r[1:]
            if w in R:
                return R[w]
        return 0

    def setw(r, v):
        if r.startswith('x'):
            R[r] = v & M64
            R['w' + r[1:]] = v & M32
        else:
            R[r] = v & M32

    for addr, p in prog:
        if len(p) < 3:
            continue
        op = p[0]
        p = [t.rstrip(',') for t in p]
        d = p[1]
        if op == 'ror':
            n = val(p[3]) & 31
            setw(d, (get(p[2]) >> n) | (get(p[2]) << ((32 - n) & 31)))
        elif op == 'lsr':
            setw(d, get(p[2]) >> val(p[3]))
        elif op == 'bfi':
            lsb = val(p[3])
            width = val(p[4])
            mask = ((1 << width) - 1) << lsb
            setw(d, (get(d) & ~mask) | ((get(p[2]) << lsb) & mask))
        elif op == 'mvn':
            setw(d, ~get(p[2]))
        elif op in ('sub', 'add'):
            a, b = val(p[2]), val(p[3])
            setw(d, (a - b) if op == 'sub' else a + b)
        elif op == 'ldr':
            setw(d, sp64)
        elif op == 'mov':
            setw(d, val(p[2]))
        elif op == 'scvtf':
            pass
        else:
            raise ValueError('unhandled: ' + ' '.join(p))
    x8 = R.get('x8', 0)
    s = x8 if x8 < (1 << 63) else x8 - (1 << 64)
    w8fin = R.get('w8', 0)
    w8s = w8fin if w8fin < (1 << 31) else w8fin - (1 << 32)
    return s, w8s

prog = load()
print('== sp64 = 0 ==')
for w22 in (0x00000000, 0x00000001, 0x00000003, 0x0000000F, 0x7FFFFFFF,
            0xFFFFFFFF, 0x12345678, 0xDEADBEEF, 0x80000000):
    s, w8s = run(prog, w22, 0)
    pc = bin(w22).count('1')
    print('w22=%08x popc=%2d -> w8_fin(s32)=%d  scvtf_arg(s64)=%d' % (w22, pc, w8s, s))

random.seed(5)
same = 0
n = 0
diffs = {}
for _ in range(20000):
    w22 = random.getrandbits(32)
    s, w8s = run(prog, w22, 0)
    pc = bin(w22).count('1')
    n += 1
    d = w8s - (pc - 2)
    diffs[d] = diffs.get(d, 0) + 1
print('w8_fin - (popc-2) distribution:', diffs)
