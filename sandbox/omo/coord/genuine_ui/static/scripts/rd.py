#!/usr/bin/env python3
# read-only reader of the arm64 slice (vmaddr == file offset). usage: rd.py kind addr count
import sys, struct
B=open('/Users/seitan/Desktop/work/reverse/neatvideo_ofx_614/NeatVideo6.arm64.ofx','rb').read()
k=sys.argv[1]; a=int(sys.argv[2],16); n=int(sys.argv[3]) if len(sys.argv)>3 else 1
for i in range(n):
    if k=='p':  # chained-fixup pointer -> low 36 bits target
        v=struct.unpack_from('<Q',B,a+8*i)[0]; print(hex(a+8*i), hex(v), '->', hex(v & 0xfffffffff) if v>>63==0 else 'bind')
    elif k=='f':
        v=struct.unpack_from('<f',B,a+4*i)[0]; print(hex(a+4*i), repr(v), hex(struct.unpack_from('<I',B,a+4*i)[0]))
    elif k=='d':
        v=struct.unpack_from('<d',B,a+8*i)[0]; print(hex(a+8*i), repr(v))
    elif k=='i':
        print(hex(a+4*i), struct.unpack_from('<i',B,a+4*i)[0], hex(struct.unpack_from('<I',B,a+4*i)[0]))
    elif k=='q':
        print(hex(a+8*i), struct.unpack_from('<q',B,a+8*i)[0])
    elif k=='s':
        e=B.index(b'\0',a); print(B[a:e]); break
