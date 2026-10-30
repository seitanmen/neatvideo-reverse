import os, sys, subprocess, numpy as np
sys.dont_write_bytecode=True
exec(open("leaf.py").read().split("rng = np.random")[0])
rng=np.random.default_rng(3); f=np.float32
def ulp(a,b):
    ia=bits(a).astype(np.int64); ib=bits(b).astype(np.int64)
    ia=np.where(ia>=2**31,2**31-ia,ia); ib=np.where(ib>=2**31,2**31-ib,ib); return np.abs(ia-ib)
def halve_alt(a):
    h2,w2=a.shape[0]>>1,a.shape[1]>>1
    r0,r1=a[0:2*h2:2],a[1:2*h2:2]
    return (((r0[:,0:2*w2:2]+r0[:,1:2*w2:2])+r1[:,0:2*w2:2])+r1[:,1:2*w2:2])*f(0.25)
tot=0;m1=0;m2=0;mx1=0;mx2=0
for (W,H) in [(2,2),(3,3),(4,4),(5,7),(8,6),(9,9),(16,16),(33,17),(100,57),(257,129),(64,3),(3,64),(2,9),(11,2),(1920,1080)]:
    a=f(rng.normal(0,50,W*H)); p=wr("pl.f32",a); nv("down2",W,H,p,os.path.join(T,"d.f32"))
    g=rd("d.f32",(W//2)*(H//2)); A=a.reshape(H,W)
    o1=S.halve(A).reshape(-1); o2=halve_alt(A).reshape(-1)
    tot+=g.size; m1+=int((bits(o1)!=bits(g)).sum()); m2+=int((bits(o2)!=bits(g)).sum()); mx1=max(mx1,ulp(o1,g).max()); mx2=max(mx2,ulp(o2,g).max())
print("down2 total",tot,"nv_spatial.halve mismatches",m1,"maxulp",mx1,"| order ((a+b)+c)+d mismatches",m2,"maxulp",mx2)
# ramp stats on realistic data
t=f(1.1);n=100000;x=f(np.abs(rng.normal(0,1.1,n)));p=wr("in.f32",x);nv("ramp",repr(float(t)),p,n,os.path.join(T,"o.f32"));g=rd("o.f32",n)
o=np.clip((f(2)*t-x)/t,0,1).astype(f);u=ulp(o,g);print("ramp realistic n",n,"mismatch",int((u>0).sum()),"max ulp",u.max(),"max abs",np.abs(o-g).max())
a=t;b=f(2)*t;sl=f(-1)/f(b-a);ic=f(f(1)-f(a*sl))
fm=np.minimum(np.maximum((np.float64(x)*np.float64(sl)+np.float64(ic)).astype(f),0),1).astype(f)
print("fma emu mismatch",int((bits(fm)!=bits(g)).sum()))
