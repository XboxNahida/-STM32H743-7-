#include "nes_rom.h"

#include <stddef.h>
#include <string.h>

int nes_rom_mapper_supported(uint16_t mapper)
{
    return ((mapper == 0U) || (mapper == 3U) ||
            (mapper == 4U) || (mapper == 23U)) ? 1 : 0;
}

int nes_rom_parse_header(const uint8_t header[NES_ROM_HEADER_SIZE],
                         uint32_t file_size,
                         nes_rom_info_t *info)
{
    nes_rom_info_t parsed;
    uint8_t flags6;
    uint8_t flags7;

    if ((header == NULL) || (info == NULL))
    {
        return NES_ROM_INVALID_ARG;
    }
    memset(info, 0, sizeof(*info));
    if (file_size < NES_ROM_HEADER_SIZE)
    {
        return NES_ROM_TRUNCATED;
    }
    if ((header[0] != 0x4EU) || (header[1] != 0x45U) ||
        (header[2] != 0x53U) || (header[3] != 0x1AU) ||
        (header[4] == 0U))
    {
        return NES_ROM_INVALID_HEADER;
    }

    flags6 = header[6];
    flags7 = header[7];
    if ((flags7 & 0x0CU) == 0x08U)
    {
        return NES_ROM_UNSUPPORTED_FORMAT;
    }

    memset(&parsed, 0, sizeof(parsed));
    parsed.prg_rom_banks = header[4];
    parsed.chr_rom_banks = header[5];
    parsed.has_trainer = ((flags6 & 0x04U) != 0U) ? 1U : 0U;
    parsed.has_battery = ((flags6 & 0x02U) != 0U) ? 1U : 0U;
    parsed.mapper = (uint16_t)(((uint16_t)(flags6 >> 4)) |
                               (uint16_t)(flags7 & 0xF0U));
    if ((flags6 & 0x08U) != 0U)
    {
        parsed.mirroring = NES_ROM_MIRROR_FOUR_SCREEN;
    }
    else
    {
        parsed.mirroring = ((flags6 & 0x01U) != 0U) ?
            NES_ROM_MIRROR_VERTICAL : NES_ROM_MIRROR_HORIZONTAL;
    }

    parsed.prg_rom_size =
        (uint32_t)parsed.prg_rom_banks * NES_ROM_PRG_BANK_SIZE;
    parsed.chr_rom_size =
        (uint32_t)parsed.chr_rom_banks * NES_ROM_CHR_BANK_SIZE;
    parsed.prg_data_offset = NES_ROM_HEADER_SIZE +
        ((parsed.has_trainer != 0U) ? NES_ROM_TRAINER_SIZE : 0U);
    parsed.chr_data_offset = parsed.prg_data_offset + parsed.prg_rom_size;
    parsed.required_file_size = parsed.chr_data_offset +
                                parsed.chr_rom_size;
    *info = parsed;

    if (file_size < parsed.required_file_size)
    {
        return NES_ROM_TRUNCATED;
    }
    if (nes_rom_mapper_supported(parsed.mapper) == 0)
    {
        return NES_ROM_UNSUPPORTED_MAPPER;
    }
    return NES_ROM_OK;
}
