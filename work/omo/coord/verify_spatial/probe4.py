import os, sys, numpy as np
sys.dont_write_bytecode=True
exec(open("probe3.py").read().split("tot=0")[0])
orders={
 "((a+b)+c)+d":lambda a,b,c,d:((a+b)+c)+d,
 "(a+b)+(c+d)":lambda a,b,c,d:(a+b)+(c+d),
 "((a+c)+b)+d":lambda a,b,c,d:((a+c)+b)+d,
 "(a+c)+(b+d)":lambda a,b,c,d:(a+c)+(b+d),
}
for W in [8,16,24,64]:
    H=4
    a=f(rng.normal(0,50,W*H)); p=wr("pl.f32",a); nv("down2",W,H,p,os.path.join(T,"d.f32")); g=rd("d.f32",(W//2)*(H//2)); A=a.reshape(H,W)
    r0,r1=A[0::2],A[1::2]
    for nm,fn in orders.items():
        o=(fn(r0[:,0::2],r0[:,1::2],r1[:,0::2],r1[:,1::2])*f(0.25)).reshape(-1)
        print(W,nm,int((bits(o)!=bits(g)).sum()),"/",g.size)
