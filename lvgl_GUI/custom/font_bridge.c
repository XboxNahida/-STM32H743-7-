#include "font_bridge.h"
#include "norflash.h"
#include "hander.h"
#include <string.h>

lv_font_t lv_font_myyh_12 = { 0 };
lv_font_t lv_font_myyh_14 = { 0 };
lv_font_t lv_font_myyh_16 = { 0 };
lv_font_t lv_font_myyh_18 = { 0 };
lv_font_t lv_font_myyh_20 = { 0 };
lv_font_t lv_font_myyh_30 = { 0 };

static int font_bridge_validate_qspi(const uint8_t *qspi_ptr)
{
    uint32_t label_len;
    memcpy(&label_len, qspi_ptr, 4);

    /* "head" section must be 32..512 bytes */
    if (label_len < 32 || label_len > 512) return 0;

    /* label must be "head" */
    if (memcmp(qspi_ptr + 4, "head", 4) != 0) return 0;

    return 1;
}

bool font_bridge_load_one(lv_font_t *dest, uint32_t qspi_offset,
                          uint32_t max_size, int size_px)
{
    const uint8_t *qspi_ptr = (const uint8_t *)(QSPI_BASE_ADDR + qspi_offset);
    (void)size_px;

    if (!font_bridge_validate_qspi(qspi_ptr)) {
        return false;
    }

    led_poc(1, 2);
    lv_font_t *loaded = lv_binfont_create_from_buffer((void *)qspi_ptr, max_size);
    led_poc(1, 2);
    if (loaded == NULL) {
        return false;
    }

    *dest = *loaded;
    return true;
}
