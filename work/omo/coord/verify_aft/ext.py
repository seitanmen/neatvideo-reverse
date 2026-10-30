import sys
s=int(sys.argv[1],16);e=int(sys.argv[2],16)
for l in open('/tmp/nv_expand/disasm_full.txt'):
    try: x=int(l.split('\t',1)[0],16)
    except: continue
    if s<=x<e: sys.stdout.write(l)
    elif x>=e: break
