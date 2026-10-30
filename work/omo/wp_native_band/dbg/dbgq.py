import sys, ctypes, numpy as np
sys.path.insert(0,'/Users/seitan/Desktop/work/reverse/native'); sys.path.insert(0,'/Users/seitan/Desktop/work/reverse/repro')
import verify_band as vb, nv_band as nb
F32=np.float32; U32=np.uint32
L=ctypes.CDLL('./libnvb.dylib'); P=ctypes.POINTER(ctypes.c_float)
L.c_mean.restype=ctypes.c_float; L.c_std.restype=ctypes.c_float
L.c_std.argtypes=[P,ctypes.c_long,ctypes.c_float]; L.c_mean.argtypes=[P,ctypes.c_long]
cs=[c for c in vb.quantile_cases(0x5EED+1, False) if c[1]==2 and 'densepayload' in c[-1]]
for c in cs:
  v=c[5]; p=v.ctypes.data_as(P)
  with np.errstate(all='ignore'):
    m=nb.mean_exact(v); s=nb.std_exact(v,m)
  cm=L.c_mean(p,v.size); csd=L.c_std(p,v.size,F32(m))
  h=lambda x: hex(np.array([x],F32).view(U32)[0])
  print(c[-1], 'mean', h(m), h(cm), 'std', h(s), h(csd))
