#!/usr/bin/env python3
# find vtables from typeinfo name string addresses; print selected slots. usage: vt.py nameaddr... 
import struct,sys
B=open('/Users/seitan/Desktop/work/reverse/neatvideo_ofx_614/NeatVideo6.arm64.ofx','rb').read()
def ptrs():
    for off in range(0x13b4000, 0x1586000, 8):
        v=struct.unpack_from('<Q',B,off)[0]
        if v>>63==0: yield off, v & 0xfffffffff
P=list(ptrs())
slots=[int(s,16) for s in sys.argv[1].split(',')]
for na in sys.argv[2:]:
    na=int(na,16)
    tis=[o-8 for o,t in P if t==na]
    for ti in tis:
        for o,t in P:
            if t==ti:
                ap=o+8  # address point
                print('name',hex(na),'typeinfo',hex(ti),'vtable addr point',hex(ap), ' '.join('%#x:%#x'%(s, struct.unpack_from('<Q',B,ap+s)[0]&0xfffffffff) for s in slots))
