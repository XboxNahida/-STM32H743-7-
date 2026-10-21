#ifndef NES_INPUT_H
#define NES_INPUT_H

#include <stdint.h>

#define NES_INPUT_MAX_TOUCH_POINTS   5U
#define NES_INPUT_MAX_TOUCH_REGIONS 16U
#define NES_INPUT_MAX_IR_BINDINGS   16U
#define NES_INPUT_IR_RELEASE_MS    180U

typedef uint8_t nes_button_mask_t;

enum
{
    NES_BUTTON_A      = 1U << 0,
    NES_BUTTON_B      = 1U << 1,
    NES_BUTTON_SELECT = 1U << 2,
    NES_BUTTON_START  = 1U << 3,
    NES_BUTTON_UP     = 1U << 4,
    NES_BUTTON_DOWN   = 1U << 5,
    NES_BUTTON_LEFT   = 1U << 6,
    NES_BUTTON_RIGHT  = 1U << 7
};

typedef enum
{
    NES_INPUT_OK = 0,
    NES_INPUT_INVALID_ARG
} nes_input_result_t;

typedef enum
{
    NES_INPUT_SOURCE_TOUCH = 0,
    NES_INPUT_SOURCE_EXTERNAL,
    NES_INPUT_SOURCE_IR,
    NES_INPUT_SOURCE_DEBUG,
    NES_INPUT_SOURCE_COUNT
} nes_input_source_t;

typedef struct
{
    int16_t x;
    int16_t y;
} nes_touch_point_t;

typedef struct
{
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    nes_button_mask_t buttons;
} nes_touch_region_t;

typedef struct
{
    uint8_t address;
    uint8_t command;
    nes_button_mask_t buttons;
} nes_ir_binding_t;

typedef struct
{
    uint8_t frame_valid;
    uint8_t address;
    uint8_t command;
    uint32_t frame_count;
    uint32_t repeat_count;
} nes_ir_snapshot_t;

void nes_input_init(void);
int nes_input_set_source(nes_input_source_t source,
                         nes_button_mask_t buttons);
nes_button_mask_t nes_input_get_source(nes_input_source_t source);
nes_button_mask_t nes_input_get_buttons(void);

int nes_input_map_touch(const nes_touch_point_t *points,
                        uint8_t point_count,
                        const nes_touch_region_t *regions,
                        uint8_t region_count,
                        nes_button_mask_t *buttons);

int nes_input_set_ir_map(const nes_ir_binding_t *bindings,
                         uint8_t count);
int nes_input_update_ir(const nes_ir_snapshot_t *snapshot,
                        uint32_t now_ms);

#endif
