#ifndef OSCILLOSCOPE_STORAGE_H
#define OSCILLOSCOPE_STORAGE_H

#include "oscilloscope_core.h"
#include "oscilloscope_io.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SCOPE_STORAGE_PATH_MAX 128u
#define SCOPE_BMP_MAX_WIDTH 1024u

typedef enum { SCOPE_FILE_READ, SCOPE_FILE_CREATE_NEW } scope_file_mode_t;
typedef enum { SCOPE_IO_ERROR = -1, SCOPE_IO_EXISTS = 0, SCOPE_IO_OK = 1 } scope_io_result_t;
/* 同步、仅主循环；每个适配器同一时刻一个打开文件。CREATE_NEW 必须原子拒绝覆盖。
 * read/write 返回实际字节数（出错 SIZE_MAX；read 的 0 只表示 EOF），close 返回 1 成功。
 * next_file 返回 1 条目/0 结束/-1 错误，配置保存/扫描恢复必须提供该回调，
 * 输出目录内的文件名（无路径）；index 从 0 开始。所有回调不保留传入缓冲区指针。 */
typedef struct {
    void *context;
    int (*open)(void *context, const char *path, scope_file_mode_t mode);
    size_t (*read)(void *context, void *buffer, size_t bytes);
    size_t (*write)(void *context, const void *buffer, size_t bytes);
    int (*close)(void *context);
    int (*remove)(void *context, const char *path);
    int (*next_file)(void *context, const char *directory, size_t index,
                     char *name, size_t capacity);
} scope_storage_io_t;

typedef struct {
    float sample_rate_hz;
    uint32_t trigger_mode; /* 0 Auto，1 Normal，2 Single。 */
    scope_trigger_t trigger;
    uint32_t holdoff_ms;
    scope_display_t display;
    scope_calibration_t calibration;
    scope_dac_config_t dac;
    float time_per_div_s, mv_per_div, vertical_offset_mv;
    uint32_t acquisition_mode; /* 0 采样，1 峰值，2 平均。 */
    uint32_t average_count, grid_enabled, persistence, cursor_enabled;
    float cursor_x[2]; /* 捕获记录内的采样点位置。 */
    float cursor_mv[2];
    uint32_t measurement_mode; /* 0 流畅，1 准确；独立于触发模式。 */
    uint32_t dac_sample_rate_hz;
    uint32_t render_mode; /* 0 原始采样点，1 局部保形插值；仅用于显示。 */
    scope_io_config_t io;
    scope_display_t display2;
    scope_calibration_t calibration2;
    float mv_per_div2, vertical_offset2_mv;
    uint32_t measurement_source, fft_source; /* 0 CH1，1 CH2；FFT另有2=两路。 */
    uint32_t reading_item[4],reading_source[4]; /* 常驻卡：10项测量、独立通道。 */
} scope_settings_t;

/* 所有函数 1 成功/0 失败；默认配置不会启动 DAC。
 * 设置文件使用固定小端字段编码、版本和 CRC32；读取失败不修改 *settings。
 * 配置采用目录内最大序号加一；扫描有界，目录最多 9999 项，编号 00000000..99999999。
 * CSV 导出完整冻结记录（64～8192点）及真实 mV，不应用显示耦合/探头/拟合。
 * 保存成功后才写入 path；失败时尝试移除本次新建的未完成文件。目录须适配器提前创建。 */
void scope_settings_defaults(scope_settings_t *settings);
int scope_settings_valid(const scope_settings_t *settings);
int scope_storage_save_settings(const scope_storage_io_t *io, const char *directory,
                                const scope_settings_t *settings, char *path, size_t capacity);
int scope_storage_load_settings(const scope_storage_io_t *io, const char *path,
                                scope_settings_t *settings);
int scope_storage_load_latest_settings(const scope_storage_io_t *io, const char *directory,
                                       scope_settings_t *settings);
int scope_storage_save_csv(const scope_storage_io_t *io, const char *directory,
                           const uint16_t *raw, size_t count, float sample_rate_hz,
                           uint32_t capture_sequence, const scope_calibration_t *calibration,
                           char *path, size_t capacity);
int scope_storage_save_csv_mode(const scope_storage_io_t *io, const char *directory,
                           const uint16_t *raw, size_t count, float sample_rate_hz,
                           uint32_t capture_sequence, const scope_calibration_t *calibration,
                           uint32_t measurement_mode,uint32_t measurement_sequence,
                           char *path,size_t capacity);
int scope_storage_save_csv_pair(const scope_storage_io_t *io,const char *directory,
                           const uint16_t *raw1,const uint16_t *raw2,size_t count,
                           float sample_rate_hz,uint32_t capture_sequence,
                           const scope_io_config_t *channels,
                           const scope_calibration_t *calibration1,
                           const scope_calibration_t *calibration2,
                           uint32_t measurement_mode,uint32_t measurement_sequence,
                           char *path,size_t capacity);
/* read_row(y) 按屏幕从顶到下读 RGB565。内部依 BMP 自下向上调用，缓冲区静态且有界，
 * 不可重入；最大宽 1024，24 位 BGR 输出含 4 字节行对齐。 */
typedef int (*scope_rgb565_row_fn)(void *context, uint32_t y, uint16_t *row, size_t width);
int scope_storage_save_bmp(const scope_storage_io_t *io, const char *directory,
                           uint32_t width, uint32_t height, scope_rgb565_row_fn read_row,
                           void *row_context, char *path, size_t capacity);

#ifdef __cplusplus
}
#endif
#endif
