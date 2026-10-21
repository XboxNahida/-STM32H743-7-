#ifndef OSCILLOSCOPE_IO_H
#define OSCILLOSCOPE_IO_H
#include <stdint.h>
/* UI序号与文件编码固定；不接受其它GPIO。 */
typedef struct { uint32_t enabled[2], input[2], output; } scope_io_config_t;
#define SCOPE_INPUT_PA5 0U
#define SCOPE_INPUT_PC4 1U
#define SCOPE_OUTPUT_PA4 0U
#define SCOPE_OUTPUT_PA5 1U /* 仅识别旧配置编码；当前输出只允许PA4。 */
static inline int scope_io_valid(const scope_io_config_t *c)
{
    return c&&c->enabled[0]<=1U&&c->enabled[1]<=1U&&
        (c->enabled[0]||c->enabled[1])&&c->input[0]<=1U&&c->input[1]<=1U&&c->output==SCOPE_OUTPUT_PA4&&
        !(c->enabled[0]&&c->enabled[1]&&c->input[0]==c->input[1]);
}
static inline uint32_t scope_io_max_rate(const scope_io_config_t *c)
{
    if(!scope_io_valid(c))return 0U;
    return ((c->enabled[0]&&c->input[0]==SCOPE_INPUT_PA5)||
            (c->enabled[1]&&c->input[1]==SCOPE_INPUT_PA5))?1000000U:2000000U;
}
static inline int scope_io_dac_conflict(const scope_io_config_t *c)
{
    return !scope_io_valid(c)||(c->output==SCOPE_OUTPUT_PA5&&
        ((c->enabled[0]&&c->input[0]==SCOPE_INPUT_PA5)||
         (c->enabled[1]&&c->input[1]==SCOPE_INPUT_PA5)));
}
#endif
