#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nes_core_engine.h"
#include "nes_input.h"
#include "nes_main.h"
#include "nes_mapper.h"
#include "nes_ppu.h"
#include "nes_video.h"

static uint16_t g_framebuffer[NES_VIDEO_PIXEL_COUNT];
static NES_header g_header;
static MAPPER g_mapper;
static int g_compile_calls;
static int g_cpu_init_calls;
static int g_mapper_init_calls;
static int g_ppu_reset_calls;
static int g_apu_init_calls;
static int g_run_calls;
static int g_hsync_calls;
static int g_scanline_calls;
static int g_start_frame_calls;
static int g_start_vblank_calls;
static int g_end_vblank_calls;
static int g_vsync_calls;
static int g_bind_calls;
static int g_unbind_calls;
static int g_publish_calls;
static int g_video_busy;

int NES_scanline;
uint8_t PADdata0;
uint8_t PADdata1;
uint8_t cpunmi;
NES_header *RomHeader = &g_header;
MAPPER *NES_Mapper = &g_mapper;
uint8_t *VROM_banks;
uint8_t *VROM_tiles;

static void hsync_stub(int line) { (void)line; ++g_hsync_calls; }
static void vsync_stub(void) { ++g_vsync_calls; }
int nes_core_runtime_is_active(const nes_core_runtime_t *runtime)
{ return runtime != NULL && runtime->active != 0U; }
void compile(int count, uint8 *src, uint8 *dest)
{ (void)count; (void)src; (void)dest; ++g_compile_calls; }
void cpu6502_init(void) { ++g_cpu_init_calls; }
void Mapper_Init(void)
{ ++g_mapper_init_calls; g_mapper.HSync = hsync_stub; g_mapper.VSync = vsync_stub; }
void PPU_reset(void) { ++g_ppu_reset_calls; }
void apu_init(void) { ++g_apu_init_calls; }
void run6502(uint32_t cycles) { (void)cycles; ++g_run_calls; }
void PPU_start_frame(void) { ++g_start_frame_calls; }
void scanline_draw(int line) { (void)line; ++g_scanline_calls; }
void start_vblank(void) { ++g_start_vblank_calls; }
uint32 NMI_enabled(void) { return 1U; }
void end_vblank(void) { ++g_end_vblank_calls; }
uint8_t nes_input_get_buttons(void) { return NES_BUTTON_A | NES_BUTTON_LEFT; }
int PPU_bind_framebuffer(uint16_t *pixels, uint16_t stride)
{ ++g_bind_calls; return (pixels == g_framebuffer && stride == NES_VIDEO_WIDTH) ? 0 : 1; }
void PPU_unbind_framebuffer(void) { ++g_unbind_calls; }
int nes_video_acquire_render_buffer(uint16_t **pixels)
{
    if (g_video_busy != 0) return NES_VIDEO_BUSY;
    *pixels = g_framebuffer;
    return NES_VIDEO_OK;
}
int nes_video_publish(void) { ++g_publish_calls; return NES_VIDEO_OK; }

static int expect_true(const char *name, int condition)
{
    if (condition == 0) { printf("FAIL %s\n", name); return 1; }
    return 0;
}

int main(void)
{
    nes_core_runtime_t runtime;
    nes_core_engine_t engine;
    nes_core_cpu_fault_t fault;
    uint8_t chr_raw[8192];
    uint8_t chr_tiles[8192];
    int failures = 0;

    memset(&runtime, 0, sizeof(runtime));
    memset(&engine, 0, sizeof(engine));
    memset(&g_header, 0, sizeof(g_header));
    memset(&g_mapper, 0, sizeof(g_mapper));
    runtime.active = 1U;
    g_header.num_8k_vrom_banks = 1U;
    VROM_banks = chr_raw;
    VROM_tiles = chr_tiles;

    failures += expect_true("prepare",
        nes_core_engine_prepare(&engine, &runtime) == NES_CORE_ENGINE_OK);
    failures += expect_true("prepare_ordered_components",
        g_compile_calls == 1 && g_cpu_init_calls == 1 &&
        g_mapper_init_calls == 1 && g_ppu_reset_calls == 1 &&
        g_apu_init_calls == 1);
    failures += expect_true("prepared", nes_core_engine_is_ready(&engine) != 0);

    debug_6502(0x1234U, 1U);
    debug_6502(0x5678U, 2U);
    nes_core_engine_get_cpu_fault(&fault);
    failures += expect_true("first_cpu_fault_only",
        fault.valid != 0U && fault.reg0 == 0x1234U && fault.reg1 == 1U);
    nes_core_engine_clear_cpu_fault();
    nes_core_engine_get_cpu_fault(&fault);
    failures += expect_true("cpu_fault_clear", fault.valid == 0U);
    debug_6502(0x9ABCU, 2U);
    nes_core_engine_get_cpu_fault(&fault);
    failures += expect_true("cpu_fault_recapture",
        fault.valid != 0U && fault.reg0 == 0x9ABCU && fault.reg1 == 2U);

    failures += expect_true("run_frame",
        nes_core_engine_run_frame(&engine) == NES_CORE_ENGINE_OK);
    failures += expect_true("input_latched",
        PADdata0 == (NES_BUTTON_A | NES_BUTTON_LEFT) && PADdata1 == 0U);
    failures += expect_true("scanline_timing",
        g_start_frame_calls == 1 && g_run_calls == 263 &&
        g_hsync_calls == 262 && g_scanline_calls == 240);
    failures += expect_true("vblank",
        g_start_vblank_calls == 1 && g_end_vblank_calls == 1 &&
        g_vsync_calls == 1 && cpunmi == 1U);
    failures += expect_true("video_publish",
        g_bind_calls == 1 && g_unbind_calls == 1 && g_publish_calls == 1);
    failures += expect_true("frame_count",
        nes_core_engine_frame_count(&engine) == 1U);

    g_video_busy = 1;
    failures += expect_true("video_busy",
        nes_core_engine_run_frame(&engine) == NES_CORE_ENGINE_VIDEO_BUSY);
    failures += expect_true("busy_does_not_advance",
        g_run_calls == 263 && nes_core_engine_frame_count(&engine) == 1U);

    nes_core_engine_stop(&engine);
    failures += expect_true("stopped", nes_core_engine_is_ready(&engine) == 0);
    failures += expect_true("run_stopped",
        nes_core_engine_run_frame(&engine) == NES_CORE_ENGINE_NOT_READY);

    if (failures != 0) {
        printf("FAIL nes_core_engine failures=%d\n", failures);
        return 1;
    }
    printf("PASS nes_core_engine\n");
    return 0;
}
