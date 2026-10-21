#ifndef OSCILLOSCOPE_WORKSPACE_H
#define OSCILLOSCOPE_WORKSPACE_H
#include "oscilloscope_core.h"
/* 主循环单实例；数组均为CPU访问，DMA仅访问片内专用缓冲。 */
typedef struct {
    uint16_t raw[SCOPE_MAX_RECORD_LENGTH];
    uint16_t stream[2U*SCOPE_MAX_RECORD_LENGTH];
    uint16_t incoming[SCOPE_MAX_RECORD_LENGTH];
    uint16_t raw2[SCOPE_MAX_RECORD_LENGTH],stream2[2U*SCOPE_MAX_RECORD_LENGTH];
    uint16_t incoming2[SCOPE_MAX_RECORD_LENGTH];
    float mv[SCOPE_MAX_RECORD_LENGTH],mv2[SCOPE_MAX_RECORD_LENGTH];
    /* 准确计算只在采集停止后运行；重新采集重置stream_count再覆盖此区。 */
    union {
        float stream_mv[2U*SCOPE_MAX_RECORD_LENGTH];
        struct {float accurate_samples[SCOPE_MAX_RECORD_LENGTH],accurate_in[SCOPE_MAX_RECORD_LENGTH];};
    };
    float display[SCOPE_MAX_RECORD_LENGTH],average[SCOPE_MAX_RECORD_LENGTH];
    float average2[SCOPE_MAX_RECORD_LENGTH]; /* CH2完整窗口平均，位于共享SDRAM。 */
    float bins[SCOPE_MAX_RECORD_LENGTH/2U+1U];
    float fft_real[SCOPE_MAX_RECORD_LENGTH],fft_imag[SCOPE_MAX_RECORD_LENGTH];
    float accurate_out[SCOPE_MAX_RECORD_LENGTH];
    float accurate_imag[SCOPE_MAX_RECORD_LENGTH];
} scope_workspace_t;
int scope_workspace_acquire(void);
void scope_workspace_release(void);
scope_workspace_t *scope_workspace_get(void);
#endif
