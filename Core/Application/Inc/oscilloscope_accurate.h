#ifndef OSCILLOSCOPE_ACCURATE_H
#define OSCILLOSCOPE_ACCURATE_H
#include "oscilloscope_core.h"
typedef enum {
    SCOPE_ACCURATE_OK, SCOPE_ACCURATE_DC, SCOPE_ACCURATE_TOO_SHORT,
    SCOPE_ACCURATE_AMBIGUOUS
} scope_accurate_reason_t;
typedef struct { scope_accurate_reason_t reason; unsigned steps; size_t analysis_samples; } scope_accurate_info_t;
/* 单实例、主循环调用。begin复制输入并做一次FFT；step最多8*8192投影。
 * begin和单音回退仍需板端测时，不可从ISR调用。取消后结果无效。 */
int scope_accurate_begin(const float *mv,size_t n,float fs);
int scope_accurate_step(void);
int scope_accurate_result(scope_measurements_t *out,scope_accurate_info_t *info);
void scope_accurate_cancel(void);
#endif
