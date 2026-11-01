#!/usr/bin/env python3
# usage: moc.py binary classname_string_addr   -> metaobject, methods with names, static_metacall
import struct, sys
B = open(sys.argv[1], 'rb').read()
s = int(sys.argv[2], 16)
a = s - 24
while struct.unpack_from('<i', B, a)[0] == -1: a -= 24
start = a + 24
def name(i):
    ref, size, alloc, off = struct.unpack_from('<iiIxxxxq', B, start + 24 * i)
    e = start + 24 * i + off
    return B[e:e + size].decode()
mo = None
for o in range(0, len(B) - 8, 8):
    v = struct.unpack_from('<Q', B, o)[0]
    if v >> 63 == 0 and (v & 0xfffffffff) == start and o > 0x1000000:
        mo = o - 8; break
P = lambda o: struct.unpack_from('<Q', B, o)[0] & 0xfffffffff
print('stringdata', hex(start), 'metaobject', hex(mo), 'super', hex(P(mo)), 'data', hex(P(mo + 16)), 'metacall', hex(P(mo + 24)))
d = P(mo + 16)
D = lambda i: struct.unpack_from('<I', B, d + 4 * i)[0]
mc, moff = D(4), D(5)
for k in range(mc):
    e = [D(moff + 5 * k + j) for j in range(5)]
    print(k, name(e[0]), 'argc', e[1])
