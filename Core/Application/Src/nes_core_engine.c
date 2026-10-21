#include "nes_core_engine.h"

#include <stddef.h>

#include "nes_apu.h"
#include "nes_input.h"
#include "nes_main.h"
#include "nes_mapper.h"
#include "nes_ppu.h"
#include "nes_ppu_output.h"
#include "nes_video.h"

#define NES_CPU_CYCLES_PER_SCANLINE (113U * 256U)
#define NES_NMI_CYCLES              (7U * 256U)

static volatile nes_core_cpu_fault_t g_cpu_fault;

void debug_6502(uint16_t reg0, uint8_t reg1)
{
    if (g_cpu_fault.valid != 0U)
    {
        return;
    }
    g_cpu_fault.reg0 = reg0;
    g_cpu_fault.reg1 = reg1;
    g_cpu_fault.valid = 1U;
}

int nes_core_engine_prepare(nes_core_engine_t *engine,
                            nes_core_runtime_t *runtime)
{
    if ((engine == NULL) || (runtime == NULL))
    {
        return NES_CORE_ENGINE_INVALID_ARG;
    }
    if ((nes_core_runtime_is_active(runtime) == 0) ||
        (RomHeader == NULL) || (NES_Mapper == NULL) ||
        (VROM_banks == NULL) || (VROM_tiles == NULL))
    {
        return NES_CORE_ENGINE_NOT_READY;
    }

    if (RomHeader->num_8k_vrom_banks != 0U)
    {
        compile((int)((uint32_t)RomHeader->num_8k_vrom_banks *
                      8192U / 16U),
                VROM_banks,
                VROM_tiles);
    }
    cpu6502_init();
    Mapper_Init();
    PPU_reset();
    apu_init();
    nes_core_engine_clear_cpu_fault();

    engine->runtime = runtime;
    engine->frame_count = 0U;
    engine->ready = 1U;
    return NES_CORE_ENGINE_OK;
}

int nes_core_engine_run_frame(nes_core_engine_t *engine)
{
    uint16_t *framebuffer;
    int video_result;

    if ((engine == NULL) || (engine->ready == 0U) ||
        (nes_core_runtime_is_active(engine->runtime) == 0))
    {
        return NES_CORE_ENGINE_NOT_READY;
    }

    video_result = nes_video_acquire_render_buffer(&framebuffer);
    if (video_result == NES_VIDEO_BUSY)
    {
        return NES_CORE_ENGINE_VIDEO_BUSY;
    }
    if (video_result != NES_VIDEO_OK)
    {
        return NES_CORE_ENGINE_CORE_ERROR;
    }
    if (PPU_bind_framebuffer(framebuffer, NES_VIDEO_WIDTH) !=
        NES_PPU_OUTPUT_OK)
    {
        return NES_CORE_ENGINE_CORE_ERROR;
    }

    PADdata0 = nes_input_get_buttons();
    PADdata1 = 0U;
    PPU_start_frame();
    for (NES_scanline = 0; NES_scanline < 240; ++NES_scanline)
    {
        run6502(NES_CPU_CYCLES_PER_SCANLINE);
        NES_Mapper->HSync(NES_scanline);
        scanline_draw(NES_scanline);
    }

    NES_scanline = 240;
    run6502(NES_CPU_CYCLES_PER_SCANLINE);
    NES_Mapper->HSync(NES_scanline);
    start_vblank();
    if (NMI_enabled() != 0U)
    {
        cpunmi = 1U;
        run6502(NES_NMI_CYCLES);
    }
    NES_Mapper->VSync();

    for (NES_scanline = 241; NES_scanline < 262; ++NES_scanline)
    {
        run6502(NES_CPU_CYCLES_PER_SCANLINE);
        NES_Mapper->HSync(NES_scanline);
    }
    end_vblank();
    PPU_unbind_framebuffer();

    if (nes_video_publish() != NES_VIDEO_OK)
    {
        return NES_CORE_ENGINE_CORE_ERROR;
    }
    ++engine->frame_count;
    return NES_CORE_ENGINE_OK;
}

void nes_core_engine_stop(nes_core_engine_t *engine)
{
    if (engine != NULL)
    {
        PPU_unbind_framebuffer();
        engine->runtime = NULL;
        engine->frame_count = 0U;
        engine->ready = 0U;
    }
}

int nes_core_engine_is_ready(const nes_core_engine_t *engine)
{
    return ((engine != NULL) && (engine->ready != 0U));
}

uint32_t nes_core_engine_frame_count(const nes_core_engine_t *engine)
{
    return (engine != NULL) ? engine->frame_count : 0U;
}

void nes_core_engine_get_cpu_fault(nes_core_cpu_fault_t *fault)
{
    if (fault != NULL)
    {
        fault->reg0 = g_cpu_fault.reg0;
        fault->reg1 = g_cpu_fault.reg1;
        fault->valid = g_cpu_fault.valid;
    }
}

void nes_core_engine_clear_cpu_fault(void)
{
    g_cpu_fault.valid = 0U;
    g_cpu_fault.reg0 = 0U;
    g_cpu_fault.reg1 = 0U;
}
