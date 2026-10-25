import sys; sys.dont_write_bytecode=True
import numpy as np, os
import cmp as C
T=C.T; F=np.float32
r=np.array([0x7fc00001]*4+[0x7fc00001],np.uint32).view(F); p=np.array([0x7fc00002]*5,np.uint32).view(F)
C.put("r.f32",r);C.put("p.f32",p)
C.nvh(["wmul",1.0,os.path.join(C.TMP,"r.f32"),os.path.join(C.TMP,"p.f32"),5,os.path.join(C.TMP,"o.f32")])
print([hex(x) for x in C.get("o.f32",5)])
print([hex(x) for x in T.wmul_genuine(F(1),r,p).view(np.uint32)])
w=T._ramp_core(r,F(1),F(2),1.0,0.0,True); print([hex(x) for x in w.view(np.uint32)])
