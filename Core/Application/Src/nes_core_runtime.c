#include "nes_core_runtime.h"

#include <stddef.h>
#include <string.h>

#include "nes_apu.h"
#include "nes_main.h"
#include "nes_mapper.h"
#include "nes_ppu.h"
#include "nes_rom.h"

#define NES_CPU_RAM_SIZE       0x0800U
#define NES_CPU_SRAM_SIZE      0x2000U
#define NES_SPR_RAM_SIZE       0x0100U
#define NES_CHR_RAM_SIZE       0x2000U
#define NES_CPU_RAM_ALIGNMENT  1024U

int MapperNo;
int NES_scanline;
int VROM_1K_SIZE;
int VROM_8K_SIZE;
uint32_t NESrom_crc32;
uint8_t nes_frame_cnt;
uint8_t PADdata0;
uint8_t PADdata1;
uint8_t lianan_biao;
uint8_t *NES_RAM;
uint8_t *NES_SRAM;
NES_header *RomHeader;
MAPPER *NES_Mapper;
Mapper1Res *MAP1;
MapperCommRes *MAPx;
uint8_t *spr_ram;
ppu_data *ppu;
uint8_t *VROM_banks;
uint8_t *VROM_tiles;
apu_t *apu;
uint16_t *wave_buffers;
uint16_t *saibuf1;
uint16_t *saibuf2;
uint8_t *romfile;

static nes_core_runtime_t *g_active_runtime;

static void nes_core_runtime_clear_globals(void)
{
    MapperNo = 0;
    NES_scanline = 0;
    VROM_1K_SIZE = 0;
    VROM_8K_SIZE = 0;
    NESrom_crc32 = 0U;
    nes_frame_cnt = 0U;
    PADdata0 = 0U;
    PADdata1 = 0U;
    lianan_biao = 0U;
    NES_RAM = NULL;
    NES_SRAM = NULL;
    RomHeader = NULL;
    NES_Mapper = NULL;
    MAP1 = NULL;
    MAPx = NULL;
    spr_ram = NULL;
    ppu = NULL;
    VROM_banks = NULL;
    VROM_tiles = NULL;
    apu = NULL;
    wave_buffers = NULL;
    saibuf1 = NULL;
    saibuf2 = NULL;
    romfile = NULL;
}

static void *nes_core_runtime_alloc(nes_core_runtime_t *runtime,
                                    uint32_t size,
                                    uint32_t alignment)
{
    return nes_core_arena_alloc(&runtime->arena, size, alignment);
}

int nes_core_runtime_open(nes_core_runtime_t *runtime,
                          void *arena_storage,
                          uint32_t arena_capacity,
                          const uint8_t *rom,
                          uint32_t rom_size)
{
    nes_rom_info_t info;
    uint32_t chr_work_size;
    int parse_result;

    if (g_active_runtime != NULL)
    {
        return NES_CORE_RUNTIME_BUSY;
    }
    if ((runtime == NULL) || (arena_storage == NULL) ||
        (arena_capacity == 0U) || (rom == NULL))
    {
        return NES_CORE_RUNTIME_INVALID_ARG;
    }

    parse_result = nes_rom_parse_header(rom, rom_size, &info);
    if (parse_result == NES_ROM_UNSUPPORTED_MAPPER)
    {
        return NES_CORE_RUNTIME_UNSUPPORTED_MAPPER;
    }
    if (parse_result != NES_ROM_OK)
    {
        return NES_CORE_RUNTIME_INVALID_ROM;
    }
    if (nes_core_arena_init(&runtime->arena,
                            arena_storage,
                            arena_capacity) != NES_CORE_ARENA_OK)
    {
        return NES_CORE_RUNTIME_INVALID_ARG;
    }

    runtime->rom = rom;
    runtime->rom_size = rom_size;
    runtime->active = 0U;
    nes_core_runtime_clear_globals();

    NES_RAM = (uint8_t *)nes_core_runtime_alloc(runtime,
                                                NES_CPU_RAM_SIZE,
                                                NES_CPU_RAM_ALIGNMENT);
    NES_SRAM = (uint8_t *)nes_core_runtime_alloc(runtime,
                                                 NES_CPU_SRAM_SIZE,
                                                 4U);
    RomHeader = (NES_header *)nes_core_runtime_alloc(runtime,
                                                     sizeof(*RomHeader),
                                                     4U);
    NES_Mapper = (MAPPER *)nes_core_runtime_alloc(runtime,
                                                  sizeof(*NES_Mapper),
                                                  4U);
    spr_ram = (uint8_t *)nes_core_runtime_alloc(runtime,
                                                NES_SPR_RAM_SIZE,
                                                4U);
    ppu = (ppu_data *)nes_core_runtime_alloc(runtime, sizeof(*ppu), 4U);
    apu = (apu_t *)nes_core_runtime_alloc(runtime, sizeof(*apu), 4U);

    if ((NES_RAM == NULL) || (NES_SRAM == NULL) || (RomHeader == NULL) ||
        (NES_Mapper == NULL) || (spr_ram == NULL) || (ppu == NULL) ||
        (apu == NULL))
    {
        goto no_memory;
    }

    chr_work_size = (info.chr_rom_size != 0U) ?
                    info.chr_rom_size : NES_CHR_RAM_SIZE;
    if (info.chr_rom_size != 0U)
    {
        VROM_banks = (uint8_t *)(rom + info.chr_data_offset);
    }
    else
    {
        VROM_banks = (uint8_t *)nes_core_runtime_alloc(runtime,
                                                       NES_CHR_RAM_SIZE,
                                                       4U);
    }
    VROM_tiles = (uint8_t *)nes_core_runtime_alloc(runtime,
                                                   chr_work_size,
                                                   4U);
    if ((VROM_banks == NULL) || (VROM_tiles == NULL))
    {
        goto no_memory;
    }

    if ((info.mapper == 4U) || (info.mapper == 23U))
    {
        MAPx = (MapperCommRes *)nes_core_runtime_alloc(runtime,
                                                       sizeof(*MAPx),
                                                       4U);
        if (MAPx == NULL)
        {
            goto no_memory;
        }
    }

    memcpy(RomHeader, rom, sizeof(*RomHeader));
    MapperNo = (int)info.mapper;
    if (info.chr_rom_size != 0U)
    {
        VROM_1K_SIZE = (int)(info.chr_rom_size / 1024U);
        VROM_8K_SIZE = (int)(info.chr_rom_size / 8192U);
    }
    else
    {
        VROM_1K_SIZE = 8;
        VROM_8K_SIZE = 1;
    }
    romfile = (uint8_t *)rom;
    runtime->active = 1U;
    g_active_runtime = runtime;
    return NES_CORE_RUNTIME_OK;

no_memory:
    nes_core_runtime_clear_globals();
    nes_core_arena_reset(&runtime->arena);
    runtime->rom = NULL;
    runtime->rom_size = 0U;
    runtime->active = 0U;
    return NES_CORE_RUNTIME_NO_MEMORY;
}

void nes_core_runtime_close(nes_core_runtime_t *runtime)
{
    if ((runtime == NULL) || (g_active_runtime != runtime))
    {
        return;
    }
    nes_core_runtime_clear_globals();
    nes_core_arena_reset(&runtime->arena);
    runtime->rom = NULL;
    runtime->rom_size = 0U;
    runtime->active = 0U;
    g_active_runtime = NULL;
}

int nes_core_runtime_is_active(const nes_core_runtime_t *runtime)
{
    return ((runtime != NULL) && (g_active_runtime == runtime) &&
            (runtime->active != 0U));
}

uint32_t nes_core_runtime_arena_used(const nes_core_runtime_t *runtime)
{
    if (runtime == NULL)
    {
        return 0U;
    }
    return nes_core_arena_used(&runtime->arena);
}
