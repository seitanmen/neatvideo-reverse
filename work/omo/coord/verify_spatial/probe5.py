import os, sys, numpy as np
sys.dont_write_bytecode=True
exec(open("probe3.py").read().split("tot=0")[0])
def fmav(a,b,c): return (np.float64(a)*np.float64(b)+np.float64(c)).astype(f)
res={}
for t in [1.1,1.65,0.0123,3.3,7.7,2.0,0.5,9.1]:
  for n in [1,2,3,5,6,7,9,10,11,13]:
    for rep in range(6):
        t=f(t); x=f(rng.uniform(-1,3*t,n)); p=wr("in.f32",x)
        nv("ramp",repr(float(t)),p,n,os.path.join(T,"o.f32")); g=rd("o.f32",n)
        a=t;b=f(2)*t; sl=f(f(0)-f(1))/f(b-a)
        sel=np.arange(n)>=(n&~3)
        for icf in (0,1):
          ic=fmav(-a,sl,f(1)) if icf else f(f(1)-f(a*sl))
          for mode in ("nf","f","xs_ic_div"):
            if mode=="nf": v=(ic+(x*sl).astype(f)).astype(f)
            elif mode=="f": v=fmav(x,sl,ic)
            else: v=((x-a)/(b-a)*f(-1)+f(1)).astype(f)
            v=np.minimum(np.maximum(v,0),1).astype(f)
            k=(icf,mode); res.setdefault(k,[0,0]); res[k][0]+=int((bits(v)[sel]!=bits(g)[sel]).sum()); res[k][1]+=int(sel.sum())
for k,v in sorted(res.items()): print(k,v)
