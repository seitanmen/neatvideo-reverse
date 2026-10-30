import os, sys, subprocess, numpy as np, itertools
sys.dont_write_bytecode=True
exec(open("leaf.py").read().split("rng = np.random")[0])
rng = np.random.default_rng(2); f=np.float32
def fma(a,b,c):
    return f(np.float64(a)*np.float64(b)+np.float64(c)) if np.ndim(a)==0 and np.ndim(b)==0 and np.ndim(c)==0 else None
def fmav(a,b,c):  # float32 fma emulated via float64 (double rounding rare issue ok; exact for products of f32 in f64, add rounds once to f64 then f32)
    return (np.float64(a)*np.float64(b)+np.float64(c)).astype(f)
res={}
for t in [1.1,1.65,0.0123,3.3,7.7,2.0]:
  for n in [4,8,64,1000,3,5,7,9]:
    t=f(t); x=f(rng.uniform(-1,3*t,n)); p=wr("in.f32",x)
    nv("ramp",repr(float(t)),p,n,os.path.join(T,"o.f32")); g=rd("o.f32",n)
    a=t;b=f(2)*t
    sl=f(f(0)-f(1))/f(b-a)
    for icf in (0,1):
      ic = fmav(-a,sl,f(1)) if icf else f(f(1)-f(a*sl))
      for mf in (0,1):
        for order in (0,1):
          v = fmav(x,sl,ic) if mf else f(ic+f(x*sl))
          v=np.minimum(np.maximum(v,0),1).astype(f)
          k=(icf,mf,"vec" if n%4==0 and n>=4 else "tail")
          res.setdefault(k,[0,0]); res[k][0]+=int((bits(v)!=bits(g)).sum()); res[k][1]+=n
for k,v in sorted(res.items()): print(k,v)
