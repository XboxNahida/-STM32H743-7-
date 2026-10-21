#include <stdint.h>
#include <stdio.h>

#include "nes_main.h"
#include "nes_ppu.h"
#include "nes_apu.h"
#include "nes_mapper.h"

static int expect_size(const char *name, size_t actual, size_t expected)
{
    if (actual != expected) {
        printf("FAIL %s actual=%lu expected=%lu\n", name,
               (unsigned long)actual, (unsigned long)expected);
        return 1;
    }
    return 0;
}

int main(void)
{
    int failures = 0;

    failures += expect_size("int8", sizeof(int8), 1U);
    failures += expect_size("int16", sizeof(int16), 2U);
    failures += expect_size("int32", sizeof(int32), 4U);
    failures += expect_size("uint8", sizeof(uint8), 1U);
    failures += expect_size("uint16", sizeof(uint16), 2U);
    failures += expect_size("uint32", sizeof(uint32), 4U);
    failures += expect_size("nes_header", sizeof(NES_header), 16U);
    if (failures != 0) {
        printf("FAIL nes_legacy_types failures=%d\n", failures);
        return 1;
    }
    printf("PASS nes_legacy_types\n");
    return 0;
}
