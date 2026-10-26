import sys, ctypes, numpy as np
sys.path.insert(0,'/Users/seitan/Desktop/work/reverse/native'); sys.path.insert(0,'/Users/seitan/Desktop/work/reverse/repro')
import verify_band as vb, nv_band as nb, nv_lines as nl
F32=np.float32; U32=np.uint32
L=ctypes.CDLL('./libnvb.dylib')
P=ctypes.POINTER(ctypes.c_float)
def ptr(a): return a.ctypes.data_as(P)
L.c_mean.restype=ctypes.c_float; L.c_std.restype=ctypes.c_float
L.c_std.argtypes=[P,ctypes.c_long,ctypes.c_float]; L.c_mean.argtypes=[P,ctypes.c_long]
L.c_cv.argtypes=[P,ctypes.c_int,ctypes.c_int,ctypes.c_float,P]
def cfun(name, X, *args, shape=None):
    X=np.ascontiguousarray(X,F32); o=np.zeros(shape or X.shape,F32)
    getattr(L,name)(ptr(X),*args,ptr(o)); return o
def diff(a,b,name):
    a=np.ascontiguousarray(a,F32).view(U32); b=np.ascontiguousarray(b,F32).view(U32)
    d=np.nonzero(a.ravel()!=b.ravel())[0]
    print(name, 'diff', d.size, [ (hex(a.ravel()[i]),hex(b.ravel()[i])) for i in d[:4]])
label=sys.argv[1]
cases=vb.random_cases(0x5EED, True)
c=[c for c in cases if c[-1]==label and c[1]==int(sys.argv[2])][0]
_,kind,W,H,_,data,_=c
X=data.reshape(H,W); bord=2 if kind==1 else 4
src=X
with np.errstate(all='ignore'):
  for k in range(5):
    Hk,Wk=src.shape
    bx,by,small=nb._borders(Wk,Hk,bord)
    print(k,Wk,Hk,bx,by,small)
    if Hk<3 or Wk<3 or small: break
    a=nl.box_rows(src,1,0); ca=cfun('c_box_rows',src,Wk,Hk,1,0); diff(ca,a,f'L{k} rows0')
    b=nl.box_cols(a,1,0); cb=cfun('c_box_cols',a,Wk,Hk,1,0); diff(cb,b,f'L{k} cols0')
    lo=nb.npx.down2(b); clo=cfun('c_down2',b,Wk,Hk,shape=lo.shape); diff(clo,lo,f'L{k} down2')
    band=nb.npx.band_from(lo,src,Wk&1,Hk&1); lo_=np.ascontiguousarray(lo)
    o=np.zeros(src.shape,F32); L.c_band(ptr(lo_),lo.shape[1],lo.shape[0],ptr(np.ascontiguousarray(src)),Wk,Hk,ptr(o)); diff(o,band,f'L{k} band')
    sq=band*band
    a2=nl.box_rows(sq,1,1); diff(cfun('c_box_rows',sq,Wk,Hk,1,1),a2,f'L{k} rows1')
    b2=nl.box_cols(a2,1,1); diff(cfun('c_box_cols',a2,Wk,Hk,1,1),b2,f'L{k} cols1')
    e=np.sqrt(np.abs(b2)); crop=np.ascontiguousarray(e[by:Hk-by,bx:Wk-bx])
    m=nb.mean_exact(crop); cm=L.c_mean(ptr(crop),crop.size); diff(np.array([cm]),np.array([m]),f'L{k} mean')
    s=nb.std_exact(crop,m); cs=L.c_std(ptr(crop),crop.size,F32(m)); diff(np.array([cs]),np.array([s]),f'L{k} std')
    cvp=nb.cv_bm(crop[:,:,None],np.array([m]))[:,0]; o8=np.zeros(8,F32); L.c_cv(ptr(crop),crop.shape[1],crop.shape[0],F32(m),ptr(o8)); diff(o8,cvp,f'L{k} cv')
    src=lo
# find first differing row at level 2 rows0
src=X
with np.errstate(all='ignore'):
  for k in range(2):
    a=nl.box_rows(src,1,0); b=nl.box_cols(a,1,0); src=nb.npx.down2(b)
  a=nl.box_rows(src,1,0); ca=cfun('c_box_rows',src,src.shape[1],src.shape[0],1,0)
  rows=np.nonzero((a.view(U32)!=ca.view(U32)).any(1))[0]
  r=rows[0]; print('row',r,[hex(x) for x in src[r].view(U32)]); print('py',[hex(x) for x in a[r].view(U32)]); print('c ',[hex(x) for x in ca[r].view(U32)])
  print('bm', [hex(x) for x in nl.box_rows_bm(src[:,:,None],1,0)[r,:,0].view(U32)])
