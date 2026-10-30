import sys, random, subprocess
sys.dont_write_bytecode=True
sys.path.insert(0,'cp/src'); sys.path.insert(0,'cp/base')
import nv_aft as na
NVH='/Users/seitan/Desktop/work/reverse/neatvideo_ofx_614/analysis/harness/nvh'
r=subprocess.run([NVH,'selftest'],capture_output=True)
assert r.returncode==3, r.returncode
rnd=random.Random(20261017)
ns=[rnd.randint(1,4096) for _ in range(100)]+[0,1,2,3,4095,4096,2048,1023,1024,1025]
bad=0
for n in ns:
    p=subprocess.run([NVH,'aftshuffle',str(n)],capture_output=True,text=True)
    out=[tuple(map(int,l.split())) for l in p.stdout.splitlines() if l and l[0].isdigit()]
    got=[v for _,v in out]
    exp=na.shuffle_order(n)
    if p.returncode!=0 or got!=exp or [i for i,_ in out]!=list(range(n)):
        bad+=1; print('MISMATCH',n,p.returncode,len(got))
print('cases',len(ns),'mismatch',bad)
