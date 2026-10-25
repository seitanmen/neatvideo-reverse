import os, sys, numpy as np, warnings
warnings.filterwarnings("ignore")
sys.dont_write_bytecode=True
exec(open("leaf.py").read().split("tot = {")[0])
f=np.float32
def emu(t,x):
    a=t;b=f(2)*t
    if (b-a)<=b*f(1e-5): a=a*f(0.99999)+f(-1e-5); b=b*f(1.00001)+f(1e-5)
    sl=f(f(0)-f(1))/f(b-a); ic=f(f(1)-f(a*sl)); n=len(x)
    vec=(np.float64(x)*np.float64(sl)+np.float64(ic)).astype(f)
    nf=(ic+(x*sl).astype(f)).astype(f)
    v=np.where(np.arange(n)<(n&~3),vec,nf)
    return np.fmax(np.fmin(v,f(1)),f(0)).astype(f)  # NaN: fmin/fmax NEON propagate NaN? check
rep={"ramp":[0,0],"wmul":[0,0],"nan_diff":0}
for t in [1.1,1.65,0.0123,3.3,1e-6*1.1,7.7,2.0,0.0,1e-8]:
  for n in [1,2,3,4,5,6,7,8,9,12,17,64,1001]:
    t=f(t); x=inputs(t,n)
    for xx in x:
        xx=xx[:n]; prev=f(rng.normal(0,3,n)); p=wr("in.f32",xx); q=wr("prev.f32",prev)
        nv("ramp",repr(float(t)),p,n,os.path.join(T,"o.f32")); g=rd("o.f32",n)
        nv("wmul",repr(float(t)),p,q,n,os.path.join(T,"o2.f32")); g2=rd("o2.f32",n)
        e=emu(t,xx); e2=(prev*e).astype(f)
        # compare treating all NaN equal
        def ne(a,b): return int(((bits(a)!=bits(b))&~(np.isnan(a)&np.isnan(b))).sum())
        rep["ramp"][0]+=1; rep["ramp"][1]+=ne(g,e); rep["wmul"][0]+=1; rep["wmul"][1]+=ne(g2,e2)
print(rep)
print("--- breakdown")
from collections import Counter
C=Counter(); N=Counter()
for t in [1.1,1.65,0.0123,3.3,1e-6*1.1,7.7,2.0,0.0,1e-8]:
  for n in [1,2,3,4,5,6,7,8,9,12,17,64,1001]:
    t=f(t); xs=inputs(t,n)
    for kk,xx in enumerate(xs):
        xx=xx[:n]; p=wr("in.f32",xx)
        nv("ramp",repr(float(t)),p,n,os.path.join(T,"o.f32")); g=rd("o.f32",n); e=emu(t,xx)
        bad=(bits(g)!=bits(e))&~(np.isnan(g)&np.isnan(e))
        if bad.any():
            C[(float(t),kk)]+=int(bad.sum())
            if C[(float(t),kk)]==int(bad.sum()) and len(C)<12: i=np.argmax(bad); print("t",t,"n",n,"kind",kk,"x",xx[i],"genuine",g[i],"emu",e[i], "idx",i)
print(dict(C))
