import os, sys, subprocess, importlib
sys.dont_write_bytecode=True
import numpy as np
from numpy.lib.stride_tricks import sliding_window_view
HERE=os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0,HERE)
mods={m:importlib.import_module(m) for m in ["nv_motion_copy","mut_le","mut_cut1ulp"]}
NVH="/Users/seitan/Desktop/work/reverse/neatvideo_ofx_614/analysis/harness/nvh"
assert subprocess.run([NVH,"selftest"],capture_output=True).returncode==3
F=np.float32; rng=np.random.default_rng(7)
def run(a): return subprocess.run([NVH]+[str(x) for x in a],capture_output=True,text=True).stdout.strip().splitlines()[-1]
def port(nm,pa,pb,cx,cy,thr):
    wr=sliding_window_view(pa,(32,32)); wc=sliding_window_view(pb,(32,32))
    y0=np.array([[cy+8]],np.int32); x0=np.array([[cx+8]],np.int32); z=np.zeros((1,1),np.int32)
    fdy,fdx,best,base=nm.refine32(wc,wr,y0,x0,z,z,F(thr))
    return int(base[0,0]),int(best[0,0]),F(cx+fdx[0,0]).view(np.uint32).item(),F(cy+fdy[0,0]).view(np.uint32).item()
cut=F(1.5151515); cutn=np.nextafter(cut,F(2)); cutp=np.nextafter(cut,F(0))
stats={k:[0,0] for k in mods}; eq=0; hits=[0,0,0]
tmp=os.path.join(HERE,"tmp")
for it in range(10):
    W,H=100,90
    a=np.clip(rng.normal(128,40,(H,W)),0,255).round().astype(np.uint8); b=np.clip(np.roll(a,(1,1),(0,1))+rng.integers(-6,7,(H,W)),0,255).astype(np.uint8)
    fa=tmp+"/e_a.f32"; fb=tmp+"/e_b.f32"; a.astype(F).tofile(fa); b.astype(F).tofile(fb)
    pa=np.pad(a,8,mode="edge"); pb=np.pad(b,8,mode="edge")
    cx,cy=int(rng.integers(1,W-33)),int(rng.integers(1,H-33))
    wr=sliding_window_view(pa,(32,32)); c=pb[cy+8:cy+40,cx+8:cx+40].astype(np.int64)
    s=[int(np.abs(wr[cy+8+dy,cx+8+dx].astype(np.int64)-c).sum()) for dx,dy in [(0,0)]+[tuple(v) for v in mods["nv_motion_copy"].NEIGHBORS8]]
    best=min(s); ths=[]
    for dd in sorted({d-best for d in s if d>best})[:4]:
        x=F(F(dd)/cut-F(0.01)); x=F(x*F(0.9999))
        for _ in range(200000):
            inv=F(1)/(x+F(0.01)); d=inv*F(dd)
            if d==cut: ths.append((x,0)); 
            if d==cutn: ths.append((x,1))
            if d==cutp: ths.append((x,2))
            if d<cutp: break
            x=np.nextafter(x,F(np.inf))
    sel=[t for kk in (0,1,2) for t in [t for t in ths if t[1]==kk][:3]]
    for x,k in sel:
        hits[k]+=1
        o=run(["b32",W,H,fa,fb,cx,cy,"%.9g"%x]); bits=o.split("bits=")[1].split()
        ref=(int(o.split("ret=")[1].split()[0]),int(o.split("sad=")[1].split()[0]),int(bits[0],16),int(bits[1],16))
        for n,nm in mods.items():
            stats[n][0]+=1; stats[n][1]+= (port(nm,pa,pb,cx,cy,x)!=ref)
print("boundary thresholds (d==cut,d==cut+ulp,d==cut-ulp):",hits); print(stats)
