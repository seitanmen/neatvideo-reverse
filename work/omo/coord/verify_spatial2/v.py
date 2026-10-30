import os, sys, subprocess, numpy as np
from concurrent.futures import ThreadPoolExecutor
HERE=os.path.dirname(os.path.abspath(__file__))
NVH="/Users/seitan/Desktop/work/reverse/neatvideo_ofx_614/analysis/harness/nvh"
sys.path.insert(0,HERE+"/src"); sys.dont_write_bytecode=True
import nv_spatial as S
F=np.float32; D=HERE+"/data"
st=subprocess.run([NVH,"selftest"],capture_output=True)
assert st.returncode==3, st.returncode
rng=np.random.default_rng(777123)
SP=np.array([0x7FC00000,0xFFC00000,0x7F800001,0xFFA00000,0x7F800000,0xFF800000,0,0x80000000,1,0x00800000,0x7F7FFFFF,0x3F800000],np.uint32).view(F)
def hexf(t):
    t=F(t)
    if np.isnan(t): return "nan"
    if np.isinf(t): return "inf" if t>0 else "-inf"
    return float(t).hex()
cases=[]
def add(kind,args,out,fn):
    cases.append((kind,args,out,fn))
def mk(i,a):
    p=f"{D}/{i}.f32"; np.ascontiguousarray(a,"<f4").tofile(p); return p
cnt=0
def plane(kind,H,W):
    with np.errstate(all="ignore"):
        if kind=="wide": return (rng.choice([-1,1],(H,W))*np.exp(rng.uniform(-20,20,(H,W)))).astype(F)
        if kind=="band": return rng.normal(0,3,(H,W)).astype(F)
        if kind=="sp":
            a=rng.normal(0,5,(H,W)).astype(F); idx=rng.integers(0,H*W,max(1,H*W//6)); a.reshape(-1)[idx]=rng.choice(SP,idx.size); return a
        if kind=="int": return rng.integers(-2**25,2**25,(H,W)).astype(F)
        if kind=="bits": return rng.integers(0,2**32,(H,W),dtype=np.uint64).astype(np.uint32).view(F)
# down2: W in all residues around 8 boundaries, H incl odd
for W in list(range(2,41))+[47,48,49,63,64,65,100,255,256,257,500,511,512]:
    for H in (2,3,4,7,10,33):
        for k in ("wide","band","sp","int","bits"):
            cnt+=1; pl=plane(k,H,W); pi=mk(cnt,pl); po=f"{D}/{cnt}_o"
            add("down2",["down2",str(W),str(H),pi,po],po,lambda pl=pl:S.halve(pl))
# wmul: n all residues mod 4 (1..40 via shapes), realistic values
shapes=[(1,n) for n in range(1,14)]+[(n,1) for n in (2,3,5)]+[(3,3),(5,5),(7,9),(9,7),(13,11),(31,33),(64,63),(127,129),(255,257),(100,101),(512,512),(333,509)]
ts=[F(1.1),F(1.1*0.7),F(1.1*2.3),F(0.0),F(-0.5),F(np.inf),F(np.nan),F(1e-30),F(3.3)]
for (H,W) in shapes:
    for ti,t in enumerate(ts):
        kinds=("band","sp","wide","bits") if ti<3 else ("band","sp")
        for k in kinds:
            cnt+=1
            Wp=plane(k,H,W)
            if k=="band":  # realistic: whitened band, R=local_rms
                Wp=(Wp/F(1.7)).astype(F)
            R=S.local_rms(Wp) if k!="bits" else plane("bits",H,W)
            pr=mk(f"{cnt}r",R); pp=mk(f"{cnt}p",Wp); po=f"{D}/{cnt}_o"
            add("wmul",["wmul",hexf(t),pr,pp,str(H*W),po],po,lambda t=t,R=R,Wp=Wp:S.wmul_genuine(t,R,Wp))
            if k=="band":
                # R near t/2t edges
                R2=(np.abs(R)*F(0.0)+F(float(t)*rng.uniform(0.8,2.2,R.shape)) if np.isfinite(t) else R).astype(F)
                cnt+=1; pr2=mk(f"{cnt}r",R2); pp2=mk(f"{cnt}p",Wp); po2=f"{D}/{cnt}_o"
                add("wmul",["wmul",hexf(t),pr2,pp2,str(H*W),po2],po2,lambda t=t,R=R2,Wp=Wp:S.wmul_genuine(t,R,Wp))
print("cases",len(cases),flush=True)
def run(a):
    r=subprocess.run([NVH]+a,capture_output=True)
    if r.returncode: raise RuntimeError((a[:2],r.returncode,r.stderr[-200:]))
with ThreadPoolExecutor(3) as ex: list(ex.map(run,[c[1] for c in cases]))
stat={}; bad=[]
for kind,args,po,fn in cases:
    g=np.fromfile(po,"<u4"); m=np.asarray(fn(),F).reshape(-1).view(np.uint32)
    s=stat.setdefault(kind,[0,0,0]); s[0]+=1; s[1]+=g.size
    if m.size!=g.size or (m!=g).any():
        s[2]+=1; bad.append((args[:3],m.size,g.size, int(np.flatnonzero(m!=g)[0]) if m.size==g.size else -1))
for k,v in stat.items(): print(k,"cases",v[0],"values",v[1],"mismatch cases",v[2])
print(bad[:10])
def oldh(a):
    h2,w2=a.shape[0]>>1,a.shape[1]>>1
    r0,r1=a[0:2*h2:2],a[1:2*h2:2]
    with np.errstate(all="ignore"): return (r0[:,0:2*w2:2]+r1[:,0:2*w2:2]+r0[:,1:2*w2:2]+r1[:,1:2*w2:2])*F(0.25)
d=0;n=0
for kind,args,po,fn in cases:
    if kind!="down2": continue
    pl=np.fromfile(args[3],"<f4").reshape(int(args[2]),int(args[1])); g=np.fromfile(po,"<u4")
    n+=1; d+= int((oldh(pl).reshape(-1).view(np.uint32)!=g).any())
print("sensitivity old halve differs",d,"/",n)
