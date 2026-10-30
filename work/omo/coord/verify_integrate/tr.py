import sys,os,numpy as np
sys.path.insert(0,os.environ["VMODS"]); import nvfilter as nv
W,H=1920,1080
raw=np.fromfile("yuv/test_v02_296.yuv",np.uint8).reshape(32,-1)
def pl(k):
    f=raw[k];return [f[:W*H].reshape(H,W).astype(np.float32),f[W*H:W*H*5//4].reshape(H//2,W//2).astype(np.float32),f[W*H*5//4:].reshape(H//2,W//2).astype(np.float32)]
fr=[pl(k) for k in range(32)]
for kw in ({}, {"band_src":"model"}):
    p=nv.build_profile_v5(fr[16],pair=fr[17],mode="auto",more=(fr[17],fr[18]),**kw)
    print(kw,p["area"],p["flat_status"])
    for n in("Y","Cb","Cr"):
        print(n,"area_sigma",[round(v,3) for v in p["area_sigma"][n]],"band_ratio",[round(v,3) for v in p["band_ratio"][n]],"tot_ratio",round(p["tot_ratio"][n],3))
    if "model_q99" in p: print({n:[round(v,3) for v in p["model_q99"][n]] for n in p["model_q99"]})
