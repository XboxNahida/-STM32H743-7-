#include <stdio.h>
#include <string.h>

#include "nes_core_engine.h"
#include "nes_audio_service.h"
#include "nes_platform.h"
#include "nes_runtime.h"
#include "nes_video.h"

#define WORKSPACE_SIZE 131072U

static uint8_t g_workspace[WORKSPACE_SIZE];
static uint16_t g_video_a[NES_VIDEO_PIXEL_COUNT];
static uint16_t g_video_b[NES_VIDEO_PIXEL_COUNT];
static int g_fail_step;
static int g_step;
static int g_workspace_owned;
static int g_rom_opened;
static int g_video_ready;
static int g_core_active;
static int g_engine_ready;
static int g_frame_result;
static uint32_t g_frames;
static uint32_t g_input_init_count;
static uint32_t g_audio_init_count;
static uint32_t g_audio_start_count;
static uint32_t g_audio_stop_count;
static uint32_t g_audio_submit_count;
static int g_audio_start_result;
static uint8_t g_audio_active;
static uint8_t g_audio_ready;
static uint8_t g_audio_paused;

void nes_input_init(void) { ++g_input_init_count; }
void nes_audio_init(void) { ++g_audio_init_count; }
int nes_audio_start(void)
{
    ++g_audio_start_count;
    if (g_audio_start_result == NES_AUDIO_OK) g_audio_active = 1U;
    return g_audio_start_result;
}
uint8_t nes_audio_ready_for_frame(void) { return g_audio_ready; }
int nes_audio_submit_frame(void)
{
    ++g_audio_submit_count;
    return NES_AUDIO_OK;
}
void nes_audio_set_paused(uint8_t paused) { g_audio_paused = paused; }
int nes_audio_stop(void)
{
    if (g_audio_active != 0U)
    {
        ++g_audio_stop_count;
        g_audio_active = 0U;
    }
    return NES_AUDIO_OK;
}

static int fail_now(void)
{
    ++g_step;
    return (g_fail_step == g_step) ? 1 : 0;
}

int nes_platform_workspace_acquire(uint8_t **address, uint32_t *size)
{
    if (fail_now() != 0)
    {
        return 1;
    }
    g_workspace_owned = 1;
    *address = g_workspace;
    *size = sizeof(g_workspace);
    return 0;
}

void nes_platform_workspace_release(void)
{
    g_workspace_owned = 0;
}

int nes_platform_rom_open(const char *path, uint32_t *size)
{
    (void)path;
    if (fail_now() != 0)
    {
        return 1;
    }
    g_rom_opened = 1;
    *size = 32784U;
    return 0;
}

int nes_platform_rom_read(uint8_t *data, uint32_t requested,
                          uint32_t *actual)
{
    if (fail_now() != 0)
    {
        return 1;
    }
    memset(data, 0, requested);
    data[0] = 'N'; data[1] = 'E'; data[2] = 'S'; data[3] = 0x1A;
    data[4] = 2U;
    *actual = requested;
    return 0;
}

int nes_platform_rom_close(void)
{
    int result = fail_now();
    g_rom_opened = 0;
    return result;
}

int nes_platform_get_video_buffers(uint16_t **buffer_a,
                                   uint16_t **buffer_b,
                                   uint32_t *pixels_per_buffer)
{
    if (fail_now() != 0)
    {
        return 1;
    }
    *buffer_a = g_video_a;
    *buffer_b = g_video_b;
    *pixels_per_buffer = NES_VIDEO_PIXEL_COUNT;
    return 0;
}

int nes_video_init(uint16_t *a, uint16_t *b, uint32_t pixels)
{
    if ((fail_now() != 0) || (a == NULL) || (b == NULL) ||
        (pixels < NES_VIDEO_PIXEL_COUNT))
    {
        return NES_VIDEO_INVALID_ARG;
    }
    g_video_ready = 1;
    return NES_VIDEO_OK;
}

void nes_video_deinit(void) { g_video_ready = 0; }

int nes_core_runtime_open(nes_core_runtime_t *runtime, void *arena,
                          uint32_t capacity, const uint8_t *rom,
                          uint32_t rom_size)
{
    (void)runtime; (void)arena; (void)capacity; (void)rom; (void)rom_size;
    if (fail_now() != 0)
    {
        return NES_CORE_RUNTIME_NO_MEMORY;
    }
    g_core_active = 1;
    return NES_CORE_RUNTIME_OK;
}

void nes_core_runtime_close(nes_core_runtime_t *runtime)
{
    (void)runtime;
    g_core_active = 0;
}

uint32_t nes_core_runtime_arena_used(const nes_core_runtime_t *runtime)
{
    (void)runtime;
    return g_core_active ? 4096U : 0U;
}

int nes_core_engine_prepare(nes_core_engine_t *engine,
                            nes_core_runtime_t *runtime)
{
    (void)engine; (void)runtime;
    if (fail_now() != 0)
    {
        return NES_CORE_ENGINE_CORE_ERROR;
    }
    g_engine_ready = 1;
    return NES_CORE_ENGINE_OK;
}

int nes_core_engine_run_frame(nes_core_engine_t *engine)
{
    (void)engine;
    if (g_frame_result == NES_CORE_ENGINE_OK)
    {
        ++g_frames;
    }
    return g_frame_result;
}

void nes_core_engine_stop(nes_core_engine_t *engine)
{
    (void)engine;
    g_engine_ready = 0;
}

uint32_t nes_core_engine_frame_count(const nes_core_engine_t *engine)
{
    (void)engine;
    return g_frames;
}

static void reset_fixture(void)
{
    g_fail_step = 0; g_step = 0; g_workspace_owned = 0;
    g_rom_opened = 0; g_video_ready = 0; g_core_active = 0;
    g_engine_ready = 0; g_frame_result = NES_CORE_ENGINE_OK; g_frames = 0U;
    g_input_init_count = 0U;
    g_audio_init_count = 0U; g_audio_start_count = 0U;
    g_audio_stop_count = 0U; g_audio_submit_count = 0U;
    g_audio_start_result = NES_AUDIO_OK; g_audio_active = 0U;
    g_audio_ready = 1U; g_audio_paused = 0U;
    nes_runtime_init();
}

static int clean(void)
{
    return !g_workspace_owned && !g_rom_opened && !g_video_ready &&
           !g_core_active && !g_engine_ready && !g_audio_active;
}

static int expect(const char *name, int condition)
{
    if (!condition)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    nes_runtime_status_t status;
    int failures = 0;
    int index;

    reset_fixture();
    failures += expect("input initialized", g_input_init_count == 1U);
    failures += expect("audio initialized", g_audio_init_count == 1U);
    failures += expect("invalid path",
                       nes_runtime_start(NULL) == NES_RUNTIME_INVALID_ARG);
    failures += expect("start", nes_runtime_start("0:/game.nes") == 0);
    nes_runtime_get_status(&status);
    failures += expect("running", status.state == NES_RUNTIME_RUNNING);
    failures += expect("audio started",
                       g_audio_start_count == 1U && g_audio_active != 0U);
    failures += expect("rom closed after load", !g_rom_opened);
    failures += expect("repeat busy",
                       nes_runtime_start("0:/game.nes") == NES_RUNTIME_BUSY);
    nes_runtime_poll(100U);
    failures += expect("one poll one frame", g_frames == 1U);
    failures += expect("one frame one audio block", g_audio_submit_count == 1U);
    g_audio_ready = 0U;
    nes_runtime_poll(100U);
    failures += expect("audio backpressure",
                       g_frames == 1U && g_audio_submit_count == 1U);
    g_audio_ready = 1U;
    failures += expect("pause", nes_runtime_set_paused(1U) == NES_RUNTIME_OK);
    nes_runtime_get_status(&status);
    failures += expect("paused state", status.state == NES_RUNTIME_PAUSED);
    failures += expect("audio paused", g_audio_paused == 1U);
    nes_runtime_poll(101U);
    failures += expect("paused no frame", g_frames == 1U);
    failures += expect("pause idempotent",
                       nes_runtime_set_paused(1U) == NES_RUNTIME_OK);
    failures += expect("resume", nes_runtime_set_paused(0U) == NES_RUNTIME_OK);
    failures += expect("audio resumed", g_audio_paused == 0U);
    nes_runtime_poll(102U);
    failures += expect("resumed frame", g_frames == 2U);
    g_frame_result = NES_CORE_ENGINE_VIDEO_BUSY;
    nes_runtime_poll(103U);
    nes_runtime_get_status(&status);
    failures += expect("video busy remains running",
                       status.state == NES_RUNTIME_RUNNING);
    failures += expect("stop", nes_runtime_stop() == NES_RUNTIME_OK && clean());
    failures += expect("audio stopped", g_audio_stop_count == 1U);

    reset_fixture();
    failures += expect("paused stop setup", nes_runtime_start("x") == 0);
    failures += expect("paused stop pause",
                       nes_runtime_set_paused(1U) == NES_RUNTIME_OK);
    failures += expect("paused stop",
                       nes_runtime_stop() == NES_RUNTIME_OK && clean());
    failures += expect("idle pause rejected",
                       nes_runtime_set_paused(1U) == NES_RUNTIME_BUSY);

    reset_fixture();
    g_audio_start_result = NES_AUDIO_BUSY;
    failures += expect("audio busy does not block game",
                       nes_runtime_start("x") == NES_RUNTIME_OK &&
                       g_audio_active == 0U);
    nes_runtime_poll(150U);
    failures += expect("silent mode drains frame",
                       g_frames == 1U && g_audio_submit_count == 1U);
    failures += expect("silent mode stop",
                       nes_runtime_stop() == NES_RUNTIME_OK && clean());

    reset_fixture();
    failures += expect("core error setup", nes_runtime_start("x") == 0);
    g_frame_result = NES_CORE_ENGINE_CORE_ERROR;
    nes_runtime_poll(200U);
    nes_runtime_get_status(&status);
    failures += expect("poll error state",
                       status.state == NES_RUNTIME_ERROR &&
                       status.result == NES_RUNTIME_CORE_ERROR && clean());
    failures += expect("poll error stop", nes_runtime_stop() == 0 && clean());

    for (index = 1; index <= 8; ++index)
    {
        reset_fixture();
        g_fail_step = index;
        failures += expect("failure result", nes_runtime_start("x") != 0);
        nes_runtime_get_status(&status);
        failures += expect("failure state", status.state == NES_RUNTIME_ERROR);
        failures += expect("failure rollback", clean());
        failures += expect("error stop", nes_runtime_stop() == 0 && clean());
    }

    reset_fixture();
    for (index = 0; index < 50; ++index)
    {
        failures += expect("cycle start", nes_runtime_start("x") == 0);
        failures += expect("cycle stop", nes_runtime_stop() == 0 && clean());
    }
    nes_runtime_get_status(&status);
    failures += expect("cycle counts",
                       status.start_count == 50U && status.stop_count == 50U);

    if (failures == 0)
    {
        printf("PASS nes_runtime_service\n");
    }
    return failures;
}
