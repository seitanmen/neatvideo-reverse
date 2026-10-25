import sys, os, subprocess, numpy as np
sys.path.insert(0, os.environ["VMODS"]); import nvfilter as nv, nv_curve, nv_profile
NVH="/Users/seitan/Desktop/work/reverse/neatvideo_ofx_614/analysis/harness/nvh"
r=subprocess.run([NVH,"selftest"],capture_output=True,text=True)
assert r.returncode==3, r.returncode
print("selftest exit",r.returncode)
H,W=1080,1920
raw=np.fromfile("yuv/test_v02_296.yuv",np.uint8).reshape(32,-1)
for tag,k,(x,y,s) in (("n312",16,(832,0,128)),("n296",0,(792,0,128))):
    f=raw[k]
    pl=[f[:W*H].reshape(H,W).astype(np.float32),f[W*H:W*H*5//4].reshape(H//2,W//2).astype(np.float32),f[W*H*5//4:].reshape(H//2,W//2).astype(np.float32)]
    rgb=nv._profile_rgb(pl,False,"bt709")
    p=f"nvh/{tag}_rgb.f32"
    np.concatenate([np.ascontiguousarray(rgb[...,c]).ravel() for c in range(3)]).astype(np.float32).tofile(p)
    xs,ys=nv_curve.genuine_noise_curve(rgb)
    o=subprocess.run([NVH,"curve",str(W),str(H),p],capture_output=True,text=True)
    assert o.returncode==0,o.stderr[-300:]
    g=[float(l.split()[1]) for l in o.stdout.splitlines()[1:] if l.strip()]
    print(tag,"curve n=",len(g),"mismatch",sum(float(a)!=b for a,b in zip(ys,g)), [round(v,4) for v in g])
    buf,_=nv_profile.build_profile(rgb,1,(x,y,s,s),lzma=False)
    ob=f"nvh/{tag}_prof.bin"
    o=subprocess.run([NVH,"profile",str(W),str(H),p,"1",ob,str(x),str(y),str(s),str(s)],capture_output=True,text=True)
    assert o.returncode==0,o.stderr[-300:]
    gb=np.fromfile(ob,np.uint8)
    nb=0
    for pi in range(6):
        for kk in range(5):
            off=(0x4c+pi*0x1f54+(5+kk)*4)//4
            a=buf.view(np.float32)[off];b=gb.view(np.float32)[off]
            nb+= not (a==b or (a!=a and b!=b))
    print(tag,"profile q99 words 30 mismatch",nb, "area",(x,y,s))
