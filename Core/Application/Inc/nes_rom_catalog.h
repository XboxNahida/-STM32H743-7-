#ifndef NES_ROM_CATALOG_H
#define NES_ROM_CATALOG_H

#include <stdint.h>

#define NES_ROM_CATALOG_PAGE_SIZE   12U
#define NES_ROM_CATALOG_NAME_SIZE  256U
#define NES_ROM_CATALOG_PATH_SIZE  512U

typedef enum
{
    NES_ROM_CATALOG_OK = 0,
    NES_ROM_CATALOG_INVALID_ARG,
    NES_ROM_CATALOG_NOT_READY,
    NES_ROM_CATALOG_IO_ERROR,
    NES_ROM_CATALOG_PATH_TOO_LONG
} nes_rom_catalog_result_t;

typedef struct
{
    char name[NES_ROM_CATALOG_NAME_SIZE];
    char path[NES_ROM_CATALOG_PATH_SIZE];
    uint32_t size;
} nes_rom_catalog_entry_t;

typedef struct
{
    uint8_t count;
    uint8_t has_more;
    uint32_t first_index;
    uint32_t total_count;
    int result;
} nes_rom_catalog_status_t;

void nes_rom_catalog_init(void);
int nes_rom_catalog_scan_page(const char *directory, uint32_t first_index);
uint8_t nes_rom_catalog_count(void);
const nes_rom_catalog_entry_t *nes_rom_catalog_get(uint8_t index);
void nes_rom_catalog_get_status(nes_rom_catalog_status_t *status);

#endif
