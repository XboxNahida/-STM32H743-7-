/* 仅实现参考测频所需接口；数学库三角函数和有界radix-2 FFT，不依赖CMSIS包。 */
#include "scope_reference_dsp.h"
#include "oscilloscope_workspace.h"
#include <math.h>
#include <stddef.h>
float scope_ref_cos_f32(float x) { return cosf(x); }
float scope_ref_sin_f32(float x) { return sinf(x); }
arm_status scope_ref_atan2_f32(float y,float x,float *result) {
    if(result==NULL || (x==0 && y==0)) return -1;
    *result=atan2f(y,x); return ARM_MATH_SUCCESS;
}
arm_status scope_ref_rfft_fast_init_f32(arm_rfft_fast_instance_f32 *s,uint16_t n) {
    if(s==NULL || n<32 || n>SCOPE_MAX_RECORD_LENGTH || (n&(n-1u))) return -1;
    s->n=n; return ARM_MATH_SUCCESS;
}
void scope_ref_rfft_fast_f32(const arm_rfft_fast_instance_f32 *s,float *a,float *out,uint8_t inverse) {
    unsigned i,j=0,len,n=s->n;
    scope_workspace_t *w=scope_workspace_get();
    float *imag;
    if(w==NULL)return;
    imag=w->accurate_imag;
    (void)inverse;
    for(i=0;i<n;i++) imag[i]=0;
    for(i=1;i<n;i++) {
        unsigned bit=n>>1;
        float t;
        while(j&bit) { j^=bit; bit>>=1; }
        j^=bit;
        if(i<j) { t=a[i]; a[i]=a[j]; a[j]=t; }
    }
    for(len=2;len<=n;len<<=1) {
        float angle=-6.283185307179586f/(float)len;
        float wr=cosf(angle),wi=sinf(angle);
        for(i=0;i<n;i+=len) {
            float r=1,im=0;
            for(j=0;j<len/2;j++) {
                unsigned u=i+j,v=u+len/2;
                float tr=r*a[v]-im*imag[v], ti=r*imag[v]+im*a[v];
                float next=r*wr-im*wi;
                a[v]=a[u]-tr; imag[v]=imag[u]-ti;
                a[u]+=tr; imag[u]+=ti;
                im=r*wi+im*wr; r=next;
            }
        }
    }
    out[0]=a[0]; out[1]=a[n/2];
    for(i=1;i<n/2;i++) { out[2*i]=a[i]; out[2*i+1]=imag[i]; }
}
