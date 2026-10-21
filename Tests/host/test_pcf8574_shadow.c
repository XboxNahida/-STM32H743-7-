#include <stdint.h>
#include <stdio.h>

int pcf8574_init(void);
int pcf8574_write_bit_safe(uint8_t bit, uint8_t state);
uint8_t pcf8574_get_output_shadow(void);
int pcf8574_beep_set(uint8_t enabled);

#ifdef PCF8574_RED_ONLY
int pcf8574_init(void)
{
    return 0;
}
#endif

#define BEEP_IO       0U
#define DCMI_PWDN_IO  2U
#define EX_IO          4U

static uint8_t g_last_tx;
static uint32_t g_tx_count;
static int g_ready_result;
static int g_tx_result;

int pcf8574_test_is_ready(uint32_t timeout_ms)
{
    return ((timeout_ms != 0U) && (g_ready_result == 0)) ? 0 : 1;
}

int pcf8574_test_transmit(uint8_t data, uint32_t timeout_ms)
{
    if ((timeout_ms == 0U) || (g_tx_result != 0))
    {
        return 1;
    }
    g_last_tx = data;
    g_tx_count++;
    return 0;
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

int main(void)
{
    uint32_t tx_before;
    int failures = 0;

    g_ready_result = 0;
    g_tx_result = 0;
    failures += expect_int("init", pcf8574_init(), 0);
    failures += expect_u32("init_tx", g_last_tx, 0xFFU);
    failures += expect_u32("initial_shadow",
                           pcf8574_get_output_shadow(), 0xFFU);

    failures += expect_int("pwdn_low",
                           pcf8574_write_bit_safe(DCMI_PWDN_IO, 0U), 0);
    failures += expect_u32("pwdn_only", g_last_tx, 0xFBU);
    failures += expect_u32("beep_silent", g_last_tx & 0x01U, 0x01U);

    g_tx_result = 1;
    failures += expect_int("failed_write",
                           pcf8574_write_bit_safe(EX_IO, 0U), 1);
    failures += expect_u32("shadow_unchanged",
                           pcf8574_get_output_shadow(), 0xFBU);

    g_tx_result = 0;
    tx_before = g_tx_count;
    failures += expect_int("invalid_bit",
                           pcf8574_write_bit_safe(8U, 1U), 1);
    failures += expect_u32("invalid_no_tx", g_tx_count, tx_before);

    failures += expect_int("beep_on", pcf8574_beep_set(1U), 0);
    failures += expect_u32("beep_active_low", g_last_tx & 0x01U, 0U);
    failures += expect_int("beep_off", pcf8574_beep_set(0U), 0);
    failures += expect_u32("beep_returns_high", g_last_tx & 0x01U, 1U);

    if (failures == 0)
    {
        printf("PASS pcf8574_shadow\n");
        return 0;
    }

    printf("FAIL pcf8574_shadow failures=%d\n", failures);
    return 1;
}
