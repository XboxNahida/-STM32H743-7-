#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SCCB_OK 0
#define SCCB_INVALID_ARG 1
#define SCCB_NAK 2
#define SCCB_BUS_ERROR 3

void sccb_init(void);
int sccb_write_reg16(uint8_t device, uint16_t reg, uint8_t value);
int sccb_read_reg16(uint8_t device, uint16_t reg, uint8_t *value);

static uint8_t g_bus_bytes[16];
static uint32_t g_bus_byte_count;
static uint32_t g_start_count;
static uint32_t g_stop_count;
static uint32_t g_fail_byte_index;
static uint8_t g_read_value;
static uint8_t g_read_nack;

int sccb_test_init_bus(void)
{
    return 0;
}

void sccb_test_start(void)
{
    g_start_count++;
}

void sccb_test_stop(void)
{
    g_stop_count++;
}

int sccb_test_write_byte(uint8_t value)
{
    uint32_t index = g_bus_byte_count;
    if (index < (sizeof(g_bus_bytes) / sizeof(g_bus_bytes[0])))
    {
        g_bus_bytes[index] = value;
    }
    g_bus_byte_count++;
    return ((g_fail_byte_index != 0U) &&
            ((index + 1U) == g_fail_byte_index)) ? 1 : 0;
}

int sccb_test_read_byte(uint8_t nack, uint8_t *value)
{
    if (value == NULL) return 1;
    g_read_nack = nack;
    *value = g_read_value;
    return 0;
}

static void reset_bus(void)
{
    memset(g_bus_bytes, 0, sizeof(g_bus_bytes));
    g_bus_byte_count = 0U;
    g_start_count = 0U;
    g_stop_count = 0U;
    g_fail_byte_index = 0U;
    g_read_value = 0U;
    g_read_nack = 0U;
}

static int expect_int(const char *name, int actual, int expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%d expected=%d\n", name, actual, expected);
        return 1;
    }
    return 0;
}

static int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%lu expected=%lu\n",
               name, (unsigned long)actual, (unsigned long)expected);
        return 1;
    }
    return 0;
}

#if !defined(TEST_SCCB_ONLY)
typedef enum
{
    OV5640_OK = 0,
    OV5640_INVALID_ARG,
    OV5640_NOT_PRESENT,
    OV5640_IO_ERROR
} ov5640_result_t;

typedef struct
{
    uint8_t present;
    uint8_t ready;
    uint8_t mirror;
    uint8_t flip;
    uint8_t flash;
    uint8_t test_pattern;
    uint16_t chip_id;
    uint16_t width;
    uint16_t height;
    int last_result;
} ov5640_status_t;

void ov5640_service_init(void);
int ov5640_power_off(void);
int ov5640_probe(void);
int ov5640_configure_rgb565_600x400(void);
int ov5640_set_mirror(uint8_t enabled);
int ov5640_set_flip(uint8_t enabled);
int ov5640_set_flash(uint8_t enabled);
int ov5640_set_test_pattern(uint8_t enabled);
void ov5640_get_status(ov5640_status_t *status);

typedef struct
{
    uint16_t reg;
    uint8_t value;
} reg_write_t;

static reg_write_t g_writes[512];
static uint32_t g_write_count;
static uint8_t g_id_high = 0x56U;
static uint8_t g_id_low = 0x40U;
static uint16_t g_fail_reg = 0xFFFFU;

int ov5640_test_sccb_write(uint8_t device, uint16_t reg, uint8_t value)
{
    if ((device != 0x78U) || (reg == g_fail_reg)) return SCCB_NAK;
    if (g_write_count < (sizeof(g_writes) / sizeof(g_writes[0])))
    {
        g_writes[g_write_count].reg = reg;
        g_writes[g_write_count].value = value;
    }
    g_write_count++;
    return SCCB_OK;
}

int ov5640_test_sccb_read(uint8_t device, uint16_t reg, uint8_t *value)
{
    if ((device != 0x78U) || (value == NULL)) return SCCB_INVALID_ARG;
    if (reg == 0x300AU) *value = g_id_high;
    else if (reg == 0x300BU) *value = g_id_low;
    else *value = 0U;
    return SCCB_OK;
}

static int find_write(uint16_t reg, uint8_t value)
{
    uint32_t index;
    for (index = 0U; index < g_write_count; index++)
    {
        if ((g_writes[index].reg == reg) &&
            (g_writes[index].value == value))
        {
            return 1;
        }
    }
    return 0;
}

static int run_ov5640_tests(void)
{
    ov5640_status_t status;
    int failures = 0;

    memset(g_writes, 0, sizeof(g_writes));
    g_write_count = 0U;
    g_fail_reg = 0xFFFFU;
    ov5640_service_init();
    failures += expect_int("probe", ov5640_probe(), OV5640_OK);
    ov5640_get_status(&status);
    failures += expect_u32("chip_id", status.chip_id, 0x5640U);
    failures += expect_u32("present", status.present, 1U);

    failures += expect_int("configure",
                           ov5640_configure_rgb565_600x400(), OV5640_OK);
    failures += expect_int("rgb565_format", find_write(0x4300U, 0x6FU), 1);
    failures += expect_int("rgb565_isp", find_write(0x501FU, 0x01U), 1);
    failures += expect_int("width_high", find_write(0x3808U, 0x02U), 1);
    failures += expect_int("width_low", find_write(0x3809U, 0x58U), 1);
    failures += expect_int("height_high", find_write(0x380AU, 0x01U), 1);
    failures += expect_int("height_low", find_write(0x380BU, 0x90U), 1);

    failures += expect_int("mirror_on", ov5640_set_mirror(1U), OV5640_OK);
    failures += expect_int("mirror_reg_on", find_write(0x3821U, 0x06U), 1);
    failures += expect_int("mirror_off", ov5640_set_mirror(0U), OV5640_OK);
    failures += expect_int("mirror_reg_off", find_write(0x3821U, 0x00U), 1);
    failures += expect_int("flip_on", ov5640_set_flip(1U), OV5640_OK);
    failures += expect_int("flip_reg_on", find_write(0x3820U, 0x40U), 1);
    failures += expect_int("flip_off", ov5640_set_flip(0U), OV5640_OK);
    failures += expect_int("flip_reg_off", find_write(0x3820U, 0x46U), 1);

    failures += expect_int("flash_on", ov5640_set_flash(1U), OV5640_OK);
    failures += expect_int("flash_3016", find_write(0x3016U, 0x02U), 1);
    failures += expect_int("flash_301c", find_write(0x301CU, 0x02U), 1);
    failures += expect_int("flash_3019_on", find_write(0x3019U, 0x02U), 1);
    failures += expect_int("flash_off", ov5640_set_flash(0U), OV5640_OK);
    failures += expect_int("flash_3019_off", find_write(0x3019U, 0x00U), 1);
    failures += expect_int("pattern_on",
                           ov5640_set_test_pattern(1U), OV5640_OK);
    failures += expect_int("pattern_reg", find_write(0x503DU, 0x80U), 1);

    g_fail_reg = 0x3019U;
    failures += expect_int("write_error",
                           ov5640_set_flash(1U), OV5640_IO_ERROR);
    g_fail_reg = 0xFFFFU;
    g_id_low = 0x41U;
    failures += expect_int("wrong_id", ov5640_probe(), OV5640_NOT_PRESENT);
    failures += expect_int("power_off_after_wrong_id", ov5640_power_off(),
                           OV5640_OK);
    ov5640_get_status(&status);
    failures += expect_int("power_off_preserves_first_error",
                           status.last_result, OV5640_NOT_PRESENT);
    failures += expect_int("power_off_disable_data_output_3017",
                           find_write(0x3017U, 0x00U), 1);
    failures += expect_int("power_off_disable_data_output_3018",
                           find_write(0x3018U, 0x00U), 1);
    g_id_low = 0x40U;
    return failures;
}
#endif

static int run_sccb_tests(void)
{
    uint8_t value = 0U;
    int failures = 0;

    reset_bus();
    sccb_init();
    failures += expect_int("write_reg",
                           sccb_write_reg16(0x78U, 0x3019U, 0x02U), SCCB_OK);
    failures += expect_u32("write_starts", g_start_count, 1U);
    failures += expect_u32("write_stops", g_stop_count, 1U);
    failures += expect_u32("write_count", g_bus_byte_count, 4U);
    failures += expect_u32("write_device", g_bus_bytes[0], 0x78U);
    failures += expect_u32("write_reg_high", g_bus_bytes[1], 0x30U);
    failures += expect_u32("write_reg_low", g_bus_bytes[2], 0x19U);
    failures += expect_u32("write_value", g_bus_bytes[3], 0x02U);

    reset_bus();
    g_fail_byte_index = 2U;
    failures += expect_int("nak", sccb_write_reg16(0x78U, 0x3019U, 0x02U),
                           SCCB_NAK);
    failures += expect_u32("nak_stops", g_stop_count, 1U);
    failures += expect_u32("nak_stops_early", g_bus_byte_count, 2U);

    reset_bus();
    g_read_value = 0x56U;
    failures += expect_int("read_reg",
                           sccb_read_reg16(0x78U, 0x300AU, &value), SCCB_OK);
    failures += expect_u32("read_value", value, 0x56U);
    failures += expect_u32("read_starts", g_start_count, 2U);
    failures += expect_u32("read_stops", g_stop_count, 2U);
    failures += expect_u32("read_count", g_bus_byte_count, 4U);
    failures += expect_u32("read_device_write", g_bus_bytes[0], 0x78U);
    failures += expect_u32("read_device_read", g_bus_bytes[3], 0x79U);
    failures += expect_u32("read_nack", g_read_nack, 1U);
    failures += expect_int("read_null",
                           sccb_read_reg16(0x78U, 0x300AU, NULL),
                           SCCB_INVALID_ARG);
    return failures;
}

int main(void)
{
    int failures = run_sccb_tests();

#if !defined(TEST_SCCB_ONLY)
    failures += run_ov5640_tests();
#endif
    if (failures == 0)
    {
#if defined(TEST_SCCB_ONLY)
        printf("PASS ov5640_sccb mode=sccb_only\n");
#else
        printf("PASS ov5640_sccb mode=full\n");
#endif
        return 0;
    }
    printf("FAIL ov5640_sccb failures=%d\n", failures);
    return 1;
}
