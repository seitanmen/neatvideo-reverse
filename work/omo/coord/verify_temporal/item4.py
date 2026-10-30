import sys; sys.dont_write_bytecode=True
import numpy as np, importlib.util, os
sys.path.insert(0,"port")
import nv_temporal as T
import cmp as C
spec=importlib.util.spec_from_file_location("rpx","port/repro_nv_pixnoise.py"); 
rpx=importlib.util.module_from_spec(spec); spec.loader.exec_module(rpx)
rng=np.random.default_rng(424242); bad=0; n=0
for W in list(range(2,70))+[127,128,129,1919,1920,1921]:
  for H in (2,3,6,9):
    for k in "unr":
      x=C.gen(rng,W*H,k).reshape(H,W)
      a=rpx.down2(x,simd=True); b=T.down2_genuine(x)
      n+=1
      if a.shape!=b.shape or not np.array_equal(a.view(np.uint32),b.view(np.uint32)):
        # NaN payload differences?
        bad+=1
        if bad<4: print("diff",W,H,k,a.shape,b.shape, np.isnan(x).any())
print("item4 cases",n,"bits-differ",bad)
