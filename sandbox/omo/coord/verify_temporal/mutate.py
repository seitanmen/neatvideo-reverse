import os, sys
sys.dont_write_bytecode=True
import numpy as np
import cmp as C
T=C.T
which=sys.argv[1]
F=np.float32
if which=="wmul_tail_order":   # swap tail operand order (NaN choice)
    orig=T.wmul_genuine
    def m(t,r,prev,neon=True):
        r_=np.ascontiguousarray(r,np.float32).reshape(-1); p=np.ascontiguousarray(prev,np.float32).reshape(-1)
        w=T._ramp_core(r_,F(t),F(2)*F(t),1.0,0.0,neon); return T._mul(w,p)
    T.wmul_genuine=m
elif which=="pairw_tail_ties":  # tail uses a>=b? a:b style -> use np.maximum (NaN propagate)
    def m(a,b,c,neon=True):
        a=np.asarray(a,np.float32);b=np.asarray(b,np.float32);c=np.asarray(c,np.float32)
        return T._mul(c,T._sub(np.maximum(a,b),b))
    T.pairw_genuine=m
elif which=="down2_w4":        # boundary W&~3 instead of W&~7
    def m(top,bot,neon=True):
        top=np.asarray(top,np.float32);bot=np.asarray(bot,np.float32);W=top.size;w8=W&~3
        out=[]
        if w8: out.append(T._mul(T._add(T._add(top[0:w8:2],top[1:w8:2]),T._add(bot[0:w8:2],bot[1:w8:2])),F(.25)))
        nr=(W-w8)>>1
        if nr:
            i=w8+2*np.arange(nr); out.append(T._mul(T._add(T._add(T._add(top[i],top[i+1]),bot[i]),bot[i+1]),F(.25)))
        return np.concatenate(out).astype(np.float32)
    T.down2_row=m
elif which=="ramp_nonfused":   # replace FMLA with mul+add on NEON part
    T._fma=lambda addend,op1,op2: T._add(addend,T._mul(op1,op2))
elif which=="ramp_noadjust":   # skip 1e-5 coefficient correction
    orig=T.ramp_coeffs
    def m(a,b,c=1.0,d=0.0):
        a,b,c,d=F(a),F(b),F(c),F(d); s=T._div(T._sub(d,c),T._sub(b,a)); return F(s),F(T._sub(c,T._mul(a,F(s))))
    T.ramp_coeffs=m
C.run(777003,quick=True)
print("MUT",which,"mismatch_batches=",C.bad)
