#!/usr/bin/env python3
# find adrp+add/ldr references to target address range in otool disassembly. usage: adrref.py disasm lo hi
import sys,re
lo=int(sys.argv[2],16); hi=int(sys.argv[3],16)
pg={}
rx=re.compile(r'^([0-9a-f]{16})\s+(\S+)\s+(.*)$')
for line in open(sys.argv[1]):
    m=rx.match(line)
    if not m: continue
    a=int(m.group(1),16); op=m.group(2); args=m.group(3)
    if op=='adrp':
        r=args.split(',')[0].strip(); t=re.search(r';\s*0x([0-9a-f]+)',args)
        if t: pg[r]=int(t.group(1),16)
        continue
    if op in('add','ldr','str','ldrb','ldrh','ldrsw','strb','strh','ldp','stp','ldur'):
        ps=[p.strip() for p in args.split(';')[0].split(',')]
        bi = 2 if op in ('ldp','stp') else 1
        if len(ps)<=bi: continue
        base=ps[bi].strip('[]! ')
        if base in pg:
            off=0
            if len(ps)>bi+1:
                im=re.match(r'#(-?0x[0-9a-f]+|-?\d+)', ps[bi+1].strip('[]! '))
                off=int(im.group(1),0) if im else 0
            t=pg[base]+off
            if lo<=t<hi: print(hex(a),op,args.split(';')[0].strip(),'->',hex(t))
