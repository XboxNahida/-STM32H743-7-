#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nes_rom.h"

static int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%lu expected=%lu\n", name,
               (unsigned long)actual, (unsigned long)expected);
        return 1;
    }
    return 0;
}

static int test_representative_roms(void)
{
    static const uint8_t tank[16] = {
        0x4E, 0x45, 0x53, 0x1A, 0x02, 0x02, 0x30, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    static const uint8_t fighter[16] = {
        0x4E, 0x45, 0x53, 0x1A, 0x08, 0x10, 0x40, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    static const uint8_t mario[16] = {
        0x4E, 0x45, 0x53, 0x1A, 0x02, 0x01, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    static const uint8_t contra[16] = {
        0x4E, 0x45, 0x53, 0x1A, 0x08, 0x10, 0x70, 0x10,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    nes_rom_info_t info;
    int failures = 0;

    failures += expect_u32("tank_result",
        (uint32_t)nes_rom_parse_header(tank, 49168U, &info), NES_ROM_OK);
    failures += expect_u32("tank_mapper", info.mapper, 3U);
    failures += expect_u32("tank_prg", info.prg_rom_size, 32768U);
    failures += expect_u32("tank_chr", info.chr_rom_size, 16384U);

    failures += expect_u32("fighter_result",
        (uint32_t)nes_rom_parse_header(fighter, 262160U, &info), NES_ROM_OK);
    failures += expect_u32("fighter_mapper", info.mapper, 4U);
    failures += expect_u32("fighter_size", info.required_file_size, 262160U);

    failures += expect_u32("mario_result",
        (uint32_t)nes_rom_parse_header(mario, 40976U, &info), NES_ROM_OK);
    failures += expect_u32("mario_mapper", info.mapper, 0U);
    failures += expect_u32("mario_vertical", info.mirroring,
                           NES_ROM_MIRROR_VERTICAL);

    failures += expect_u32("contra_result",
        (uint32_t)nes_rom_parse_header(contra, 262160U, &info), NES_ROM_OK);
    failures += expect_u32("contra_mapper", info.mapper, 23U);
    failures += expect_u32("contra_prg_banks", info.prg_rom_banks, 8U);
    failures += expect_u32("contra_chr_banks", info.chr_rom_banks, 16U);
    return failures;
}

static int test_flags_and_trainer(void)
{
    static const uint8_t header[16] = {
        0x4E, 0x45, 0x53, 0x1A, 0x01, 0x00, 0x0FU, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    nes_rom_info_t info;
    int failures = 0;

    failures += expect_u32("trainer_result",
        (uint32_t)nes_rom_parse_header(header, 16912U, &info), NES_ROM_OK);
    failures += expect_u32("trainer_present", info.has_trainer, 1U);
    failures += expect_u32("battery_present", info.has_battery, 1U);
    failures += expect_u32("four_screen", info.mirroring,
                           NES_ROM_MIRROR_FOUR_SCREEN);
    failures += expect_u32("chr_ram", info.chr_rom_size, 0U);
    failures += expect_u32("trainer_offset", info.prg_data_offset, 528U);
    failures += expect_u32("trainer_required", info.required_file_size,
                           16912U);
    failures += expect_u32("extra_tail_allowed",
        (uint32_t)nes_rom_parse_header(header, 17000U, &info), NES_ROM_OK);
    return failures;
}

static int test_invalid_and_unsupported_headers(void)
{
    uint8_t header[16] = {
        0x4E, 0x45, 0x53, 0x1A, 0x01, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    nes_rom_info_t info;
    int failures = 0;

    failures += expect_u32("null_header",
        (uint32_t)nes_rom_parse_header(NULL, 24592U, &info),
        NES_ROM_INVALID_ARG);
    failures += expect_u32("null_info",
        (uint32_t)nes_rom_parse_header(header, 24592U, NULL),
        NES_ROM_INVALID_ARG);
    header[0] = 0U;
    failures += expect_u32("bad_magic",
        (uint32_t)nes_rom_parse_header(header, 24592U, &info),
        NES_ROM_INVALID_HEADER);
    header[0] = 0x4EU;
    header[4] = 0U;
    failures += expect_u32("zero_prg",
        (uint32_t)nes_rom_parse_header(header, 8208U, &info),
        NES_ROM_INVALID_HEADER);
    header[4] = 1U;
    header[7] = 0x08U;
    failures += expect_u32("nes2_rejected",
        (uint32_t)nes_rom_parse_header(header, 24592U, &info),
        NES_ROM_UNSUPPORTED_FORMAT);
    header[7] = 0U;
    failures += expect_u32("truncated",
        (uint32_t)nes_rom_parse_header(header, 24591U, &info),
        NES_ROM_TRUNCATED);
    return failures;
}

static int test_mapper_support_boundary(void)
{
    uint8_t header[16] = {
        0x4E, 0x45, 0x53, 0x1A, 0x01, 0x01, 0x10, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    nes_rom_info_t info;
    int failures = 0;

    failures += expect_u32("mapper0_supported", nes_rom_mapper_supported(0U),
                           1U);
    failures += expect_u32("mapper3_supported", nes_rom_mapper_supported(3U),
                           1U);
    failures += expect_u32("mapper4_supported", nes_rom_mapper_supported(4U),
                           1U);
    failures += expect_u32("mapper23_supported",
                           nes_rom_mapper_supported(23U), 1U);
    failures += expect_u32("mapper1_not_supported",
                           nes_rom_mapper_supported(1U), 0U);
    failures += expect_u32("mapper255_not_supported",
                           nes_rom_mapper_supported(255U), 0U);
    failures += expect_u32("unsupported_mapper_result",
        (uint32_t)nes_rom_parse_header(header, 24592U, &info),
        NES_ROM_UNSUPPORTED_MAPPER);
    failures += expect_u32("unsupported_mapper_reported", info.mapper, 1U);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_representative_roms();
    failures += test_flags_and_trainer();
    failures += test_invalid_and_unsupported_headers();
    failures += test_mapper_support_boundary();
    if (failures != 0)
    {
        printf("FAIL nes_rom failures=%d\n", failures);
        return 1;
    }
    printf("PASS nes_rom\n");
    return 0;
}
