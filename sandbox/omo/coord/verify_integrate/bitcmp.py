import sys, os, pickle, numpy as np
sys.path.insert(0, sys.argv[1]); import nvfilter as nv
rng=np.random.default_rng(5)
H,W=192,256
def mk(seed):
    r=np.random.default_rng(seed)
    base=np.linspace(40,200,W)[None,:]*np.ones((H,1))
    y=base+r.normal(0,4,(H,W))+ (np.arange(H)[:,None]%7)
    cb=128+r.normal(0,2,(H//2,W//2)); cr=128+r.normal(0,2,(H//2,W//2))
    return (np.clip(np.rint(y),0,255).astype(np.float32),np.clip(np.rint(cb),0,255).astype(np.float32),np.clip(np.rint(cr),0,255).astype(np.float32))
fr=[mk(i) for i in range(5)]
res={}
for mode in ("auto","spatial","temporal"):
    for area in (None,(64,64,64)):
        p=nv.build_profile_v5(fr[2],area=area,pair=fr[3],mode=mode,more=(fr[3],fr[4]))
        p={k:v for k,v in p.items() if k not in("curve_src","band_src")}
        res[(mode,area)]=p
P=nv.v5_params()
prof=nv.build_profile_v5(fr[2],area=(64,64,64),pair=fr[3],mode="auto",more=(fr[3],fr[4]))
out=nv.process_frame_v5(fr,2,prof,P)
res["out"]=[np.asarray(o) for o in out]
pickle.dump(res,open(sys.argv[2],"wb"))
