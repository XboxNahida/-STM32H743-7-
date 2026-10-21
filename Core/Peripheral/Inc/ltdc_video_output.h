#ifndef LTDC_VIDEO_OUTPUT_H
#define LTDC_VIDEO_OUTPUT_H

#include <stdbool.h>
#include <stdint.h>

bool ltdc_video_output_prepare(uint16_t width, uint16_t height);
uint32_t ltdc_video_output_acquire_decode_target(void);
bool ltdc_video_output_activate_first_frame(uint32_t timeout_ms);
bool ltdc_video_output_present(uint32_t timeout_ms);
void ltdc_video_output_abort(void);

#endif
