#ifndef NES_ROM_H
#define NES_ROM_H

#include <stdint.h>

#define NES_ROM_HEADER_SIZE       16U
#define NES_ROM_TRAINER_SIZE     512U
#define NES_ROM_PRG_BANK_SIZE  16384U
#define NES_ROM_CHR_BANK_SIZE   8192U

typedef enum
{
    NES_ROM_OK = 0,
    NES_ROM_INVALID_ARG,
    NES_ROM_INVALID_HEADER,
    NES_ROM_UNSUPPORTED_FORMAT,
    NES_ROM_UNSUPPORTED_MAPPER,
    NES_ROM_TRUNCATED
} nes_rom_result_t;

typedef enum
{
    NES_ROM_MIRROR_HORIZONTAL = 0,
    NES_ROM_MIRROR_VERTICAL,
    NES_ROM_MIRROR_FOUR_SCREEN
} nes_rom_mirroring_t;

typedef struct
{
    uint8_t prg_rom_banks;
    uint8_t chr_rom_banks;
    uint8_t has_trainer;
    uint8_t has_battery;
    uint16_t mapper;
    nes_rom_mirroring_t mirroring;
    uint32_t prg_rom_size;
    uint32_t chr_rom_size;
    uint32_t prg_data_offset;
    uint32_t chr_data_offset;
    uint32_t required_file_size;
} nes_rom_info_t;

int nes_rom_mapper_supported(uint16_t mapper);
int nes_rom_parse_header(const uint8_t header[NES_ROM_HEADER_SIZE],
                         uint32_t file_size,
                         nes_rom_info_t *info);

#endif
