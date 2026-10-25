import sys, os, importlib.util, json
sys.dont_write_bytecode=True
H=os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0,H+'/cp/base')
import numpy as np
def ld(p,d):
    sys.path.insert(0,d)
    import importlib
    spec=importlib.util.spec_from_file_location(p,d+'/nv_aft.py'); m=importlib.util.module_from_spec(spec); spec.loader.exec_module(m); sys.path.pop(0); return m
old=ld('na_old',H+'/pre'); new=ld('na_new',H+'/cp/src')
import nvfilter as nv, nv_profile as npr
W,Hh,N=1920,1080,32
def frames(path,ks):
    raw=np.memmap(path,np.uint8,'r').reshape(N,W*Hh*3//2)
    return {k:(raw[k,:W*Hh].reshape(Hh,W).astype(np.float32),raw[k,W*Hh:W*Hh*5//4].reshape(Hh//2,W//2).astype(np.float32),raw[k,W*Hh*5//4:].reshape(Hh//2,W//2).astype(np.float32)) for k in ks}
seg=sys.argv[1]; ks=[int(x) for x in sys.argv[2].split(',')]
fr=frames(H+f'/data/in_{seg}.yuv',[16,17,18]+ks)
for k in ks:
    prof=nv.build_profile_v5(fr[k],pair=fr.get(k+1,fr[17]),mode='auto',more=(fr[17],fr[18])) if False else nv.build_profile_v5(fr[16],pair=fr[17],mode='auto',more=(fr[17],fr[18]))
    rgb=np.clip(np.nan_to_num(nv.yuv_to_rgb(fr[k][0:3] and fr[k],'bt709',False),nan=0.0,posinf=255.0,neginf=0.0),0,255).astype(np.float32)
    base=npr.build_profile(rgb,1,(prof['area'][1],prof['area'][0],prof['area'][2],prof['area'][2]),with_floor=True,lzma=False)[0]
    a=bytes(old.auto_fine_tune_fast(rgb,base,1)); b=bytes(new.auto_fine_tune_fast(rgb,base,1))
    diff=[i for i in range(len(a)) if a[i]!=b[i]]
    import numpy as np
    A=np.frombuffer(a,np.uint8);B=np.frombuffer(b,np.uint8)
    d=np.nonzero(A!=B)[0]
    # ranges
    rs=[];
    for x in d:
        if rs and x-rs[-1][1]<=1: rs[-1][1]=x
        else: rs.append([x,x])
    print(' diff byte ranges:',[(hex(a_),hex(b_)) for a_,b_ in rs],flush=True)
    for ch in range(3):
        print(' ch',ch,'curve base byte',hex(old.CURVE[ch]),'bin8 level old/new',np.frombuffer(a,np.float32)[old.CURVE[ch]//4+64],np.frombuffer(b,np.float32)[old.CURVE[ch]//4+64])
