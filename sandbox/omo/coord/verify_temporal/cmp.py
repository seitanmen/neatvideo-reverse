import os, sys, subprocess, numpy as np
os.environ["PYTHONDONTWRITEBYTECODE"]="1"
sys.dont_write_bytecode=True
HERE=os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE,"port"))
import nv_temporal as T
NVH=os.path.join(HERE,"../../../../neatvideo_ofx_614/analysis/harness/nvh")
TMP=os.path.join(HERE,"tmp")
def f32(a): return np.ascontiguousarray(a,np.float32)
def put(name,a):
    p=os.path.join(TMP,name); f32(a).tofile(p); return p
def nvh(args):
    r=subprocess.run([NVH]+[str(x) for x in args],capture_output=True,text=True,timeout=300)
    if r.returncode: raise RuntimeError((args[:2],r.returncode,r.stderr[-300:]))
def get(name,n): return np.fromfile(os.path.join(TMP,name),np.uint32,n)
def gen(rng,n,kind):
    if kind=="u": x=rng.uniform(-2,4,n)
    elif kind=="n": x=rng.normal(0,1,n)*10**rng.uniform(-6,6,n)
    elif kind=="r": x=rng.uniform(0,3,n)
    x=x.astype(np.float32)
    u=x.view(np.uint32).copy()
    sp=np.array([0x7fc00000,0xffc00000,0x7fa00001,0xffa55555,0x7fc12345,0x7f800000,0xff800000,0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x7f7fffff,0xff7fffff,0x3f800000,0x40000000,0x3f7fffff],np.uint32)
    m=rng.random(n)<0.12
    u[m]=rng.choice(sp,m.sum())
    return u.view(np.float32)
bad=0; cnt={}
def chk(op,name,exp,n,desc):
    global bad
    got=get(name,n); e=f32(exp).reshape(-1).view(np.uint32)
    cnt[op]=cnt.get(op,[0,0]); cnt[op][0]+=1; cnt[op][1]+=n
    if not np.array_equal(got,e):
        bad+=1; i=np.nonzero(got!=e)[0][:3]
        print("MISMATCH",op,desc,[(int(j),hex(int(got[j])),hex(int(e[j]))) for j in i])
def run(seed0=777001, quick=False):
    rng=np.random.default_rng(seed0)
    sizes=[1,2,3,4,5,6,7,8,9,15,16,17,31,33,63,64,65,127,129,1000,4099]
    ts=[0.001,0.01,0.2,0.5,1,1.3,3,4,10,100,1e-6,1e-20,1e-38,1e30,0,-1,-0.5,float('nan'),float('inf'),float('-inf'),1e-5,0.99999,2.5e-45]
    for n in sizes:
      for t in ts:
        for kind in "unr":
            if quick and rng.random()<0.6: continue
            x=gen(rng,n,kind); p=gen(rng,n,kind)
            put("in.f32",x); put("p.f32",p)
            nvh(["ramp",repr(float(np.float32(t))),os.path.join(TMP,"in.f32"),n,os.path.join(TMP,"o.f32")])
            chk("ramp","o.f32",T.ramp_genuine(np.float32(t),x),n,(n,t,kind))
            nvh(["wmul",repr(float(np.float32(t))),os.path.join(TMP,"in.f32"),os.path.join(TMP,"p.f32"),n,os.path.join(TMP,"o.f32")])
            chk("wmul","o.f32",T.wmul_genuine(np.float32(t),x,p),n,(n,t,kind))
    for n in sizes+[2048,4096,33333]:
      for kind in "unr":
        for rep in range(3):
            a,b,c=gen(rng,n,kind),gen(rng,n,kind),gen(rng,n,kind)
            if rep==1: b=a.copy(); b[::3]=gen(rng,len(b[::3]),kind)  # ties
            for nm,v in zip("abc",(a,b,c)): put(nm+".f32",v)
            nvh(["pairw"]+[os.path.join(TMP,k+".f32") for k in "abc"]+[n,os.path.join(TMP,"o.f32")])
            chk("pairw","o.f32",T.pairw_genuine(a,b,c),n,(n,kind,rep))
    for W,H in [(2,2),(3,2),(7,2),(8,2),(9,2),(10,4),(15,6),(16,2),(17,3),(23,5),(24,4),(25,4),(31,8),(33,6),(64,4),(65,5),(100,10),(127,9),(129,4),(640,6),(1920,8),(1921,6),(4096,4),(37,40)]:
      for kind in "unr":
        x=gen(rng,W*H,kind)
        put("pl.f32",x)
        nvh(["down2",W,H,os.path.join(TMP,"pl.f32"),os.path.join(TMP,"o.f32")])
        chk("down2","o.f32",T.down2_genuine(x.reshape(H,W)),(W//2)*(H//2),(W,H,kind))
if __name__=="__main__":
    run(int(sys.argv[1]) if len(sys.argv)>1 else 777001)
    print("RESULT bad=",bad,"cases/values=",cnt)
