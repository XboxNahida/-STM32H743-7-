#ifndef SCOPE_REFERENCE_DSP_H
#define SCOPE_REFERENCE_DSP_H
#include <stdint.h>
typedef float float32_t;
typedef int arm_status;
#define ARM_MATH_SUCCESS 0
typedef struct { uint16_t n; } arm_rfft_fast_instance_f32;
arm_status scope_ref_rfft_fast_init_f32(arm_rfft_fast_instance_f32 *s,uint16_t n);
void scope_ref_rfft_fast_f32(const arm_rfft_fast_instance_f32 *s,float *input,float *output,uint8_t inverse);
float scope_ref_cos_f32(float x);
float scope_ref_sin_f32(float x);
arm_status scope_ref_atan2_f32(float y,float x,float *result);
#endif
