import os, sys, subprocess, numpy as np
sys.dont_write_bytecode=True
exec(open("leaf.py").read().split("rng = np.random")[0])
rng = np.random.default_rng(1)
f=np.float32
t=f(1.65); n=11
x=f(rng.uniform(0,3*t,n))
p=wr("in.f32",x); nv("ramp",repr(float(t)),p,n,os.path.join(T,"o.f32")); g=rd("o.f32",n)
ours=np.clip((f(2)*t-x)/t,0,1).astype(f)
print(x);print(g);print(ours); print(bits(g)-bits(ours).astype(np.int64))
slope=f(-1)/f(t); 
for name,v in [("a",f(1)+(x-t)*slope)]:
    pass
a=t;b=f(2)*t
sl=f(f(0)-f(1))/f(b-a); ic=f(1)-f(a*sl)
print("emu",ic+x*sl, bits(g).astype(np.int64)-bits(np.clip(ic+x*sl,0,1).astype(f)))
ic2=f(np.float64(1)-np.float64(a)*np.float64(sl)); 
print("emu fma ic", bits(g).astype(np.int64)-bits(np.clip(f(ic2)+x*sl,0,1).astype(f)))
# fma form
xs=np.float64(x); r=np.float64(f(ic2))+xs*np.float64(sl); print("fma-run", bits(g).astype(np.int64)-bits(np.clip(r.astype(f),0,1).astype(f)))
