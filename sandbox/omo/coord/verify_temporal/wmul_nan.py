import sys; sys.dont_write_bytecode=True
import numpy as np, os
import cmp as C
T=C.T; F=np.float32
sp=np.array([0x7fc00001,0x7fc00002,0xffc00003,0x7fa00004,0xffa00005,0x7f800000,0x3f000000,0x3e800000],np.uint32)
rng=np.random.default_rng(99)
def trial(wm):
    bad=0
    for n in (5,6,7,9):
      for t in (1.0,0.5):
        r=rng.choice(sp,n).view(np.float32); p=rng.choice(sp,n).view(np.float32)
        C.put("r.f32",r); C.put("p.f32",p)
        C.nvh(["wmul",t,os.path.join(C.TMP,"r.f32"),os.path.join(C.TMP,"p.f32"),n,os.path.join(C.TMP,"o.f32")])
        g=C.get("o.f32",n); e=F(0)+wm(F(t),r,p); e=np.asarray(e,F).view(np.uint32)
        bad+= int((g!=e).any())
    return bad
print("port_bad",sum(trial(T.wmul_genuine) for _ in range(40)))
def mut(t,r,prev,neon=True):
    r_=np.ascontiguousarray(r,np.float32); p=np.ascontiguousarray(prev,np.float32)
    w=T._ramp_core(r_,F(t),F(2)*F(t),1.0,0.0,neon); return T._mul(w,p)
print("mutant_bad",sum(trial(mut) for _ in range(40)))
