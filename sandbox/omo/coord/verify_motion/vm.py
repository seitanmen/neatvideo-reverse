import os, sys, subprocess, itertools, importlib
os.environ["PYTHONDONTWRITEBYTECODE"]="1"; sys.dont_write_bytecode=True
import numpy as np
from numpy.lib.stride_tricks import sliding_window_view
from concurrent.futures import ThreadPoolExecutor
HERE=os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0,HERE)
MUT=os.environ.get("MUT","")
nm=importlib.import_module(os.environ.get("NMOD","nv_motion_copy"))
if MUT=="nofuse": pass
NVH="/Users/seitan/Desktop/work/reverse/neatvideo_ofx_614/analysis/harness/nvh"
F32=np.float32
rc=subprocess.run([NVH,"selftest"],capture_output=True).returncode
assert rc==3, rc
def run(a):
    r=subprocess.run([NVH]+[str(x) for x in a],capture_output=True,text=True,timeout=120)
    assert r.returncode==0,(a,r.stdout,r.stderr)
    return r.stdout.strip().splitlines()[-1]
rng=np.random.default_rng(20261018)
def q(p): return np.floor(p.astype(np.float64)+0.5).clip(0,255).astype(np.uint8)
def pad(p): return np.pad(q(p),8,mode="edge")
planes=[]  # (name,W,H,a,b)
def add(n,W,H,a,b): planes.append((n,W,H,np.clip(a,0,255).astype(F32),np.clip(b,0,255).astype(F32)))
for (W,H) in [(64,64),(100,80),(160,120),(256,200)]:
    a=rng.integers(0,256,(H,W)); add("rand",W,H,a,np.roll(a,(1,-1),(0,1)))
    add("rand_indep",W,H,a,rng.integers(0,256,(H,W)))
    add("const",W,H,np.full((H,W),77),np.full((H,W),77))
    add("const_diff",W,H,np.full((H,W),77),np.full((H,W),80))
    yy,xx=np.mgrid[0:H,0:W]; g=(xx*1.3+yy*0.7)%256
    add("grad",W,H,g,np.roll(g,(0,1),(0,1))+rng.normal(0,2,(H,W)))
    sm=np.zeros((H,W)); 
    for s in range(3): sm+=np.sin(xx/(7+3*s))*40+np.cos(yy/(9+2*s))*30
    sm+=128; add("smooth",W,H,sm+rng.normal(0,3,(H,W)),np.roll(sm,(1,1),(0,1))+rng.normal(0,3,(H,W)))
    add("sat",W,H,np.where(rng.random((H,W))>.5,255,0),np.where(rng.random((H,W))>.9,0,255))
    add("sub",W,H,sm+rng.normal(0,1,(H,W)),0.5*(np.roll(sm,(0,1),(0,1))+sm)+rng.normal(0,1,(H,W)))
raw=np.fromfile("crop_a.raw",np.uint8).reshape(31,360,640)
for k in (0,10,20): add("real_a%d"%k,640,360,raw[k],raw[k+1]) if False else None
for k in (0,10,20):
    add("real_a%d"%k,256,200,raw[k][80:280,100:356],raw[k+1][80:280,100:356])
rb=np.fromfile("crop_b.raw",np.uint8).reshape(7,240,320)
for k in (0,3,5): add("real_b%d"%k,320,240,rb[k],rb[k+1])
tmp=os.path.join(HERE,"tmp"); os.makedirs(tmp,exist_ok=True)
def thrs(pref,pcur,cx,cy):
    t=[1e-6,0.5,1.1,5.0,50.0,300.0,1e4,1e6,1e30,-0.005,-0.01,float(rng.uniform(0,2000)),float(rng.uniform(0,50))]
    wr=sliding_window_view(pref,(32,32)); c=pcur[cy+8:cy+40,cx+8:cx+40].astype(np.int64)
    s=[int(np.abs(wr[cy+8+dy,cx+8+dx].astype(np.int64)-c).sum()) for dx,dy in [(0,0)]+[tuple(v) for v in nm.NEIGHBORS8]]
    best=min(s)
    for dd in sorted({d-best for d in s if d>best})[:3]:
        t0=F32(F32(dd)/F32(1.5151515)-F32(0.01)); lo=hi=t0; t.append(float(t0))
        for _ in range(3):
            lo=np.nextafter(lo,F32(-np.inf)); hi=np.nextafter(hi,F32(np.inf)); t+= [float(lo),float(hi)]
    return t
jobs=[]; mism=[]; nb=0; ns=0
def sadport(pa,pb,ax,ay,bx,by):
    A=pa[ay+8:ay+8+32,ax+8:ax+8+32]; B=pb[by+8:by+8+32,bx+8:bx+8+32]
    return nm.sad16(A[:16,:16],B[:16,:16]), nm.sad16(A,B)
def hx(v): return np.float32(v).view(np.uint32).item()
def b32port(pref,pcur,cx,cy,thr):
    wr=sliding_window_view(pref,(32,32)); wc=sliding_window_view(pcur,(32,32))
    y0=np.array([[cy+8]],np.int32); x0=np.array([[cx+8]],np.int32); z=np.zeros((1,1),np.int32)
    fdy,fdx,best,base=nm.refine32(wc,wr,y0,x0,z,z,F32(thr))
    return int(base[0,0]),int(best[0,0]),F32(cx+fdx[0,0]),F32(cy+fdy[0,0])
def work(pl):
    global nb,ns
    n,W,H,a,b=pl; tag="%s_%dx%d_%d"%(n,W,H,abs(hash((n,W,H,a.tobytes())))%99999)
    fa=os.path.join(tmp,tag+"_a.f32"); fb=os.path.join(tmp,tag+"_b.f32")
    a.tofile(fa); b.tofile(fb); pa,pb=pad(a),pad(b)
    res=[0,0,0]
    # sad
    for _ in range(25):
        ax=int(rng.integers(-8,W+8-32+1)); ay=int(rng.integers(-8,H+8-32+1)); bx=int(rng.integers(-8,W+8-32+1)); by=int(rng.integers(-8,H+8-32+1))
        o=run(["sad",W,H,fa,fb,ax,ay,bx,by]); d=dict(t.split("=") for t in o.split())
        p=sadport(pa,pb,ax,ay,bx,by); res[0]+=1
        if (int(d["sad16"]),int(d["sad32"]))!=p: res[1]+=1; mism.append(("sad",tag,ax,ay,bx,by,d,p))
    # b32
    for _ in range(4):
        cx=int(rng.integers(1,W-33+1)); cy=int(rng.integers(1,H-33+1))
        for th in thrs(pa,pb,cx,cy):
            o=run(["b32",W,H,fa,fb,cx,cy,"%.9g"%F32(th)]); d=dict(t.split("=") for t in o.split()[1:] if "=" in t)
            p=b32port(pa,pb,cx,cy,F32(th)); res[2]+=1
            got=(int(d["ret"]),int(d["sad"]),int(d["bits"].split()[0] if False else 0))
            bits=o.split("bits=")[1].split()
            ok=(int(d["ret"])==p[0] and int(d["sad"])==p[1] and int(bits[0],16)==hx(p[2]) and int(bits[1],16)==hx(p[3]))
            if not ok: res[1]+=1; mism.append(("b32",tag,cx,cy,th,o,p))
    os.remove(fa); os.remove(fb)
    return res
with ThreadPoolExecutor(3) as ex: R=list(ex.map(work,planes))
print("planes",len(planes),"sad calls",sum(r[0] for r in R),"b32 calls",sum(r[2] for r in R),"mismatch",sum(r[1] for r in R),"MUT",MUT)
import collections;print(collections.Counter(m[1].rsplit("_",1)[0] for m in mism))
for m in mism[:6]: print(m)
