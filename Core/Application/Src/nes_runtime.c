#include "nes_runtime.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "nes_core_engine.h"
#include "nes_core_runtime.h"
#include "nes_audio_service.h"
#include "nes_input.h"
#include "nes_platform.h"
#include "nes_video.h"

#define NES_RUNTIME_ARENA_ALIGNMENT 1024U

typedef struct
{
    nes_runtime_status_t status;
    nes_core_runtime_t core;
    nes_core_engine_t engine;
    uint8_t *workspace;
    uint32_t workspace_size;
    uint8_t workspace_owned;
    uint8_t rom_opened;
    uint8_t video_ready;
    uint8_t core_active;
    uint8_t engine_ready;
} nes_runtime_context_t;

static nes_runtime_context_t g_nes_runtime;

static void nes_runtime_release_all(void)
{
    (void)nes_audio_stop();
    if (g_nes_runtime.engine_ready != 0U)
    {
        nes_core_engine_stop(&g_nes_runtime.engine);
        g_nes_runtime.engine_ready = 0U;
    }
    if (g_nes_runtime.core_active != 0U)
    {
        nes_core_runtime_close(&g_nes_runtime.core);
        g_nes_runtime.core_active = 0U;
    }
    if (g_nes_runtime.video_ready != 0U)
    {
        nes_video_deinit();
        g_nes_runtime.video_ready = 0U;
    }
    if (g_nes_runtime.rom_opened != 0U)
    {
        (void)nes_platform_rom_close();
        g_nes_runtime.rom_opened = 0U;
    }
    if (g_nes_runtime.workspace_owned != 0U)
    {
        nes_platform_workspace_release();
        g_nes_runtime.workspace_owned = 0U;
    }
    g_nes_runtime.workspace = NULL;
    g_nes_runtime.workspace_size = 0U;
}

static int nes_runtime_fail(int result)
{
    nes_runtime_release_all();
    g_nes_runtime.status.state = NES_RUNTIME_ERROR;
    g_nes_runtime.status.result = result;
    g_nes_runtime.status.rom_size = 0U;
    g_nes_runtime.status.arena_used = 0U;
    return result;
}

static int nes_runtime_map_core_result(int result)
{
    if (result == NES_CORE_RUNTIME_NO_MEMORY)
    {
        return NES_RUNTIME_NO_MEMORY;
    }
    if (result == NES_CORE_RUNTIME_UNSUPPORTED_MAPPER)
    {
        return NES_RUNTIME_UNSUPPORTED_ROM;
    }
    if ((result == NES_CORE_RUNTIME_INVALID_ROM) ||
        (result == NES_CORE_RUNTIME_INVALID_ARG))
    {
        return NES_RUNTIME_INVALID_ROM;
    }
    return NES_RUNTIME_CORE_ERROR;
}

void nes_runtime_init(void)
{
    nes_runtime_release_all();
    memset(&g_nes_runtime, 0, sizeof(g_nes_runtime));
    nes_input_init();
    nes_audio_init();
    g_nes_runtime.status.state = NES_RUNTIME_IDLE;
    g_nes_runtime.status.result = NES_RUNTIME_OK;
}

int nes_runtime_start(const char *path)
{
    uint16_t *video_a;
    uint16_t *video_b;
    uint32_t video_pixels;
    uint32_t actual;
    uintptr_t arena_start;
    uintptr_t workspace_end;
    uint32_t arena_capacity;
    int result;

    if ((path == NULL) || (path[0] == '\0'))
    {
        return NES_RUNTIME_INVALID_ARG;
    }
    if (g_nes_runtime.status.state != NES_RUNTIME_IDLE)
    {
        return NES_RUNTIME_BUSY;
    }

    g_nes_runtime.status.state = NES_RUNTIME_STARTING;
    g_nes_runtime.status.result = NES_RUNTIME_OK;
    if (nes_platform_workspace_acquire(&g_nes_runtime.workspace,
                                       &g_nes_runtime.workspace_size) != 0)
    {
        return nes_runtime_fail(NES_RUNTIME_NO_MEMORY);
    }
    g_nes_runtime.workspace_owned = 1U;

    if (nes_platform_rom_open(path, &g_nes_runtime.status.rom_size) != 0)
    {
        return nes_runtime_fail(NES_RUNTIME_STORAGE_ERROR);
    }
    g_nes_runtime.rom_opened = 1U;
    if ((g_nes_runtime.status.rom_size == 0U) ||
        (g_nes_runtime.status.rom_size >= g_nes_runtime.workspace_size))
    {
        return nes_runtime_fail(NES_RUNTIME_NO_MEMORY);
    }
    actual = 0U;
    if ((nes_platform_rom_read(g_nes_runtime.workspace,
                               g_nes_runtime.status.rom_size,
                               &actual) != 0) ||
        (actual != g_nes_runtime.status.rom_size))
    {
        return nes_runtime_fail(NES_RUNTIME_STORAGE_ERROR);
    }
    result = nes_platform_rom_close();
    g_nes_runtime.rom_opened = 0U;
    if (result != 0)
    {
        return nes_runtime_fail(NES_RUNTIME_STORAGE_ERROR);
    }

    if (nes_platform_get_video_buffers(&video_a, &video_b,
                                       &video_pixels) != 0)
    {
        return nes_runtime_fail(NES_RUNTIME_NO_MEMORY);
    }
    if (nes_video_init(video_a, video_b, video_pixels) != NES_VIDEO_OK)
    {
        return nes_runtime_fail(NES_RUNTIME_CORE_ERROR);
    }
    g_nes_runtime.video_ready = 1U;

    arena_start = (uintptr_t)g_nes_runtime.workspace +
                  g_nes_runtime.status.rom_size;
    arena_start = (arena_start + NES_RUNTIME_ARENA_ALIGNMENT - 1U) &
                  ~((uintptr_t)NES_RUNTIME_ARENA_ALIGNMENT - 1U);
    workspace_end = (uintptr_t)g_nes_runtime.workspace +
                    g_nes_runtime.workspace_size;
    if (arena_start >= workspace_end)
    {
        return nes_runtime_fail(NES_RUNTIME_NO_MEMORY);
    }
    arena_capacity = (uint32_t)(workspace_end - arena_start);
    result = nes_core_runtime_open(&g_nes_runtime.core,
                                   (void *)arena_start,
                                   arena_capacity,
                                   g_nes_runtime.workspace,
                                   g_nes_runtime.status.rom_size);
    if (result != NES_CORE_RUNTIME_OK)
    {
        return nes_runtime_fail(nes_runtime_map_core_result(result));
    }
    g_nes_runtime.core_active = 1U;

    if (nes_core_engine_prepare(&g_nes_runtime.engine,
                                &g_nes_runtime.core) !=
        NES_CORE_ENGINE_OK)
    {
        return nes_runtime_fail(NES_RUNTIME_CORE_ERROR);
    }
    g_nes_runtime.engine_ready = 1U;
    (void)nes_audio_start();
    g_nes_runtime.status.arena_used =
        nes_core_runtime_arena_used(&g_nes_runtime.core);
    g_nes_runtime.status.frame_count = 0U;
    ++g_nes_runtime.status.start_count;
    g_nes_runtime.status.state = NES_RUNTIME_RUNNING;
    return NES_RUNTIME_OK;
}

void nes_runtime_poll(uint32_t now_ms)
{
    int result;

    (void)now_ms;
    if (g_nes_runtime.status.state != NES_RUNTIME_RUNNING)
    {
        return;
    }
    if (nes_audio_ready_for_frame() == 0U)
    {
        return;
    }
    result = nes_core_engine_run_frame(&g_nes_runtime.engine);
    if (result == NES_CORE_ENGINE_OK)
    {
        if (nes_audio_submit_frame() != NES_AUDIO_OK)
        {
            (void)nes_runtime_fail(NES_RUNTIME_CORE_ERROR);
            return;
        }
        g_nes_runtime.status.frame_count =
            nes_core_engine_frame_count(&g_nes_runtime.engine);
    }
    else if (result != NES_CORE_ENGINE_VIDEO_BUSY)
    {
        (void)nes_runtime_fail(NES_RUNTIME_CORE_ERROR);
    }
}

int nes_runtime_set_paused(uint8_t paused)
{
    if (paused != 0U)
    {
        if (g_nes_runtime.status.state == NES_RUNTIME_PAUSED)
        {
            return NES_RUNTIME_OK;
        }
        if (g_nes_runtime.status.state != NES_RUNTIME_RUNNING)
        {
            return NES_RUNTIME_BUSY;
        }
        g_nes_runtime.status.state = NES_RUNTIME_PAUSED;
        nes_audio_set_paused(1U);
    }
    else
    {
        if (g_nes_runtime.status.state == NES_RUNTIME_RUNNING)
        {
            return NES_RUNTIME_OK;
        }
        if (g_nes_runtime.status.state != NES_RUNTIME_PAUSED)
        {
            return NES_RUNTIME_BUSY;
        }
        g_nes_runtime.status.state = NES_RUNTIME_RUNNING;
        nes_audio_set_paused(0U);
    }
    return NES_RUNTIME_OK;
}

int nes_runtime_stop(void)
{
    if (g_nes_runtime.status.state == NES_RUNTIME_IDLE)
    {
        return NES_RUNTIME_OK;
    }
    if ((g_nes_runtime.status.state == NES_RUNTIME_STARTING) ||
        (g_nes_runtime.status.state == NES_RUNTIME_STOPPING))
    {
        return NES_RUNTIME_BUSY;
    }

    g_nes_runtime.status.state = NES_RUNTIME_STOPPING;
    nes_runtime_release_all();
    ++g_nes_runtime.status.stop_count;
    g_nes_runtime.status.state = NES_RUNTIME_IDLE;
    g_nes_runtime.status.result = NES_RUNTIME_OK;
    g_nes_runtime.status.rom_size = 0U;
    g_nes_runtime.status.arena_used = 0U;
    g_nes_runtime.status.frame_count = 0U;
    return NES_RUNTIME_OK;
}

void nes_runtime_get_status(nes_runtime_status_t *status)
{
    if (status != NULL)
    {
        *status = g_nes_runtime.status;
    }
}
