#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nes_apu.h"
#include "nes_core_runtime.h"
#include "nes_main.h"
#include "nes_mapper.h"
#include "nes_ppu.h"

static uint8_t g_arena_storage[98304];
static uint8_t g_mapper4_rom[16 + 32768 + 8192];
static uint8_t g_chr_ram_rom[16 + 16384];

static int expect_true(const char *name, int condition)
{
    if (condition == 0) { printf("FAIL %s\n", name); return 1; }
    return 0;
}

static void make_rom(uint8_t *rom,
                     uint8_t prg_banks,
                     uint8_t chr_banks,
                     uint8_t mapper)
{
    memset(rom, 0x5A, 16U + (uint32_t)prg_banks * 16384U +
                              (uint32_t)chr_banks * 8192U);
    rom[0] = 'N';
    rom[1] = 'E';
    rom[2] = 'S';
    rom[3] = 0x1AU;
    rom[4] = prg_banks;
    rom[5] = chr_banks;
    rom[6] = (uint8_t)((mapper & 0x0FU) << 4);
    rom[7] = (uint8_t)(mapper & 0xF0U);
    memset(rom + 8U, 0, 8U);
}

int main(void)
{
    nes_core_runtime_t runtime;
    uint32_t used_before_busy;
    int failures = 0;

    memset(&runtime, 0, sizeof(runtime));
    make_rom(g_mapper4_rom, 2U, 1U, 4U);
    failures += expect_true("open_mapper4",
        nes_core_runtime_open(&runtime,
                              g_arena_storage + 1U,
                              sizeof(g_arena_storage) - 1U,
                              g_mapper4_rom,
                              sizeof(g_mapper4_rom)) == NES_CORE_RUNTIME_OK);
    failures += expect_true("active", nes_core_runtime_is_active(&runtime) != 0);
    failures += expect_true("ram_alignment",
                            ((uintptr_t)NES_RAM & 1023U) == 0U);
    failures += expect_true("ram_and_sram", NES_RAM != NULL && NES_SRAM != NULL);
    failures += expect_true("fixed_state",
        RomHeader != NULL && NES_Mapper != NULL && spr_ram != NULL &&
        ppu != NULL && apu != NULL && MAPx != NULL);
    failures += expect_true("rom_zero_copy", romfile == g_mapper4_rom);
    failures += expect_true("rom_header",
        RomHeader->num_16k_rom_banks == 2U &&
        RomHeader->num_8k_vrom_banks == 1U && MapperNo == 4);
    failures += expect_true("chr_rom_binding",
        VROM_banks == g_mapper4_rom + 16U + 32768U &&
        VROM_tiles != NULL && VROM_tiles != VROM_banks);
    failures += expect_true("chr_sizes",
                            VROM_1K_SIZE == 8 && VROM_8K_SIZE == 1);
    failures += expect_true("allocated_state_zeroed",
        NES_RAM[0] == 0U && NES_SRAM[0] == 0U && spr_ram[0] == 0U &&
        ((uint8_t *)ppu)[0] == 0U && ((uint8_t *)apu)[0] == 0U &&
        VROM_tiles[0] == 0U);

    used_before_busy = nes_core_runtime_arena_used(&runtime);
    failures += expect_true("busy_rejected",
        nes_core_runtime_open(&runtime,
                              g_arena_storage,
                              sizeof(g_arena_storage),
                              g_mapper4_rom,
                              sizeof(g_mapper4_rom)) == NES_CORE_RUNTIME_BUSY);
    failures += expect_true("busy_preserves_state",
        nes_core_runtime_arena_used(&runtime) == used_before_busy &&
        romfile == g_mapper4_rom);

    nes_core_runtime_close(&runtime);
    failures += expect_true("close_inactive",
                            nes_core_runtime_is_active(&runtime) == 0);
    failures += expect_true("close_clears_globals",
        NES_RAM == NULL && NES_SRAM == NULL && RomHeader == NULL &&
        NES_Mapper == NULL && spr_ram == NULL && ppu == NULL && apu == NULL &&
        romfile == NULL && VROM_banks == NULL && VROM_tiles == NULL &&
        MAPx == NULL);
    failures += expect_true("close_resets_arena",
                            nes_core_runtime_arena_used(&runtime) == 0U);

    make_rom(g_chr_ram_rom, 1U, 0U, 0U);
    failures += expect_true("open_chr_ram",
        nes_core_runtime_open(&runtime,
                              g_arena_storage,
                              sizeof(g_arena_storage),
                              g_chr_ram_rom,
                              sizeof(g_chr_ram_rom)) == NES_CORE_RUNTIME_OK);
    failures += expect_true("chr_ram_buffers",
        VROM_banks != NULL && VROM_tiles != NULL &&
        VROM_banks != VROM_tiles && VROM_banks != g_chr_ram_rom);
    failures += expect_true("chr_ram_banks",
                            VROM_1K_SIZE == 8 && VROM_8K_SIZE == 1);
    failures += expect_true("mapper0_has_no_common_state", MAPx == NULL);
    nes_core_runtime_close(&runtime);

    failures += expect_true("small_arena_rejected",
        nes_core_runtime_open(&runtime,
                              g_arena_storage,
                              2048U,
                              g_mapper4_rom,
                              sizeof(g_mapper4_rom)) == NES_CORE_RUNTIME_NO_MEMORY);
    failures += expect_true("failure_rolls_back",
        nes_core_runtime_is_active(&runtime) == 0 &&
        nes_core_runtime_arena_used(&runtime) == 0U && NES_RAM == NULL &&
        romfile == NULL && MAPx == NULL);
    failures += expect_true("bad_rom_rejected",
        nes_core_runtime_open(&runtime,
                              g_arena_storage,
                              sizeof(g_arena_storage),
                              g_mapper4_rom,
                              8U) == NES_CORE_RUNTIME_INVALID_ROM);

    if (failures != 0) {
        printf("FAIL nes_core_runtime failures=%d\n", failures);
        return 1;
    }
    printf("PASS nes_core_runtime\n");
    return 0;
}
