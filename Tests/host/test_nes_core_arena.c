#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nes_core_arena.h"

static uint8_t g_storage[12288];

static int expect_true(const char *name, int condition)
{
    if (condition == 0) { printf("FAIL %s\n", name); return 1; }
    return 0;
}

int main(void)
{
    nes_core_arena_t arena;
    uint8_t *ram;
    uint8_t *sram;
    void *failed;
    int failures = 0;

    failures += expect_true("null_arena",
        nes_core_arena_init(NULL, g_storage, sizeof(g_storage)) ==
        NES_CORE_ARENA_INVALID_ARG);
    failures += expect_true("null_storage",
        nes_core_arena_init(&arena, NULL, sizeof(g_storage)) ==
        NES_CORE_ARENA_INVALID_ARG);
    failures += expect_true("zero_capacity",
        nes_core_arena_init(&arena, g_storage, 0U) ==
        NES_CORE_ARENA_INVALID_ARG);
    failures += expect_true("init",
        nes_core_arena_init(&arena, g_storage + 1U,
                            sizeof(g_storage) - 1U) == NES_CORE_ARENA_OK);

    memset(g_storage, 0xA5, sizeof(g_storage));
    ram = (uint8_t *)nes_core_arena_alloc(&arena, 2048U, 1024U);
    failures += expect_true("ram_alloc", ram != NULL);
    failures += expect_true("ram_aligned",
                            ((uintptr_t)ram & 1023U) == 0U);
    failures += expect_true("ram_zeroed",
                            (ram[0] == 0U) && (ram[2047] == 0U));
    sram = (uint8_t *)nes_core_arena_alloc(&arena, 8192U, 4U);
    failures += expect_true("sram_alloc", sram != NULL);
    failures += expect_true("sram_aligned",
                            ((uintptr_t)sram & 3U) == 0U);
    failures += expect_true("blocks_do_not_overlap",
                            (sram >= ram + 2048U) || (ram >= sram + 8192U));
    failures += expect_true("zero_size_rejected",
        nes_core_arena_alloc(&arena, 0U, 4U) == NULL);
    failures += expect_true("bad_alignment_rejected",
        nes_core_arena_alloc(&arena, 8U, 3U) == NULL);
    failed = nes_core_arena_alloc(&arena, sizeof(g_storage), 1U);
    failures += expect_true("capacity_enforced", failed == NULL);
    failures += expect_true("failure_does_not_advance",
                            nes_core_arena_used(&arena) <= arena.capacity);

    nes_core_arena_reset(&arena);
    failures += expect_true("reset_used", nes_core_arena_used(&arena) == 0U);
    failures += expect_true("reset_reuses_alignment",
        nes_core_arena_alloc(&arena, 2048U, 1024U) == ram);

    if (failures != 0) {
        printf("FAIL nes_core_arena failures=%d\n", failures);
        return 1;
    }
    printf("PASS nes_core_arena\n");
    return 0;
}
