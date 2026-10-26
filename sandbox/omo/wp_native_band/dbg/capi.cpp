#include "../../../../native/nv_band.h"
extern "C" {
void c_box_rows(const float* X,int W,int H,int r,int m,float* o){nvb::box_rows(X,W,H,r,m,o);}
void c_box_cols(const float* X,int W,int H,int r,int m,float* o){nvb::box_cols(X,W,H,r,m,o);}
void c_down2(const float* X,int W,int H,float* o){nvb::down2(X,W,H,o);}
void c_band(const float* lo,int Wc,int Hc,const float* s,int W2,int H2,float* o){nvb::band_from(lo,Wc,Hc,s,W2,H2,o);}
float c_mean(const float* v,long n){return nvb::mean_exact(v,n);}
float c_std(const float* v,long n,float m){return nvb::std_exact(v,n,m);}
void c_cv(const float* E,int w,int h,float m,float* o){nvb::cv(E,w,h,m,o);}
}
extern "C" float c_quant(const float* v,long n,float q){float r=0; nvb::quantile(v,n,q,1,4,&r); return r;}
