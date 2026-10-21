#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ir_remote_service.h"
#include "nes_controller_port.h"
#include "nes_input.h"

static ir_remote_status_t g_ir_status;

void ir_remote_get_status(ir_remote_status_t *status)
{
    if (status != NULL)
    {
        *status = g_ir_status;
    }
}

static int expect_u32(const char *name, uint32_t actual, uint32_t expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=0x%08lX expected=0x%08lX\n", name,
               (unsigned long)actual, (unsigned long)expected);
        return 1;
    }
    return 0;
}

static void send_frame(uint8_t address, uint8_t command, uint32_t now_ms)
{
    g_ir_status.frame_valid = 1U;
    g_ir_status.address = address;
    g_ir_status.command = command;
    ++g_ir_status.frame_count;
    nes_controller_port_poll(now_ms);
}

static int test_official_remote_mapping(void)
{
    static const struct
    {
        uint8_t command;
        nes_button_mask_t button;
    } cases[] = {
        {0x62U, NES_BUTTON_UP},
        {0xA8U, NES_BUTTON_DOWN},
        {0x22U, NES_BUTTON_LEFT},
        {0xC2U, NES_BUTTON_RIGHT},
        {0x68U, NES_BUTTON_A},
        {0xB0U, NES_BUTTON_B},
        {0x10U, NES_BUTTON_SELECT},
        {0x5AU, NES_BUTTON_START}
    };
    uint8_t index;
    int failures = 0;

    memset(&g_ir_status, 0, sizeof(g_ir_status));
    nes_input_init();
    nes_controller_port_init();
    for (index = 0U; index < (uint8_t)(sizeof(cases) / sizeof(cases[0]));
         ++index)
    {
        send_frame(0x00U, cases[index].command, 10U + index);
        failures += expect_u32("official mapped button",
            nes_input_get_source(NES_INPUT_SOURCE_IR), cases[index].button);
    }
    return failures;
}

static int test_repeat_and_release(void)
{
    int failures = 0;

    memset(&g_ir_status, 0, sizeof(g_ir_status));
    nes_input_init();
    nes_controller_port_init();
    send_frame(0x00U, 0x62U, 100U);
    ++g_ir_status.repeat_count;
    nes_controller_port_poll(200U);
    failures += expect_u32("repeat keeps up",
        nes_input_get_source(NES_INPUT_SOURCE_IR), NES_BUTTON_UP);
    nes_controller_port_poll(200U + NES_INPUT_IR_RELEASE_MS + 1U);
    failures += expect_u32("timeout releases up",
        nes_input_get_source(NES_INPUT_SOURCE_IR), 0U);
    return failures;
}

static int test_pause_edge(void)
{
    int failures = 0;

    memset(&g_ir_status, 0, sizeof(g_ir_status));
    nes_input_init();
    nes_controller_port_init();
    send_frame(0x00U, 0x02U, 100U);
    failures += expect_u32("play toggles pause",
        nes_controller_port_take_pause_toggle(), 1U);
    failures += expect_u32("pause consumed once",
        nes_controller_port_take_pause_toggle(), 0U);
    ++g_ir_status.repeat_count;
    nes_controller_port_poll(120U);
    failures += expect_u32("repeat does not toggle pause",
        nes_controller_port_take_pause_toggle(), 0U);
    nes_controller_port_poll(130U);
    failures += expect_u32("same frame does not toggle pause",
        nes_controller_port_take_pause_toggle(), 0U);
    send_frame(0x01U, 0x02U, 140U);
    failures += expect_u32("other address ignored",
        nes_controller_port_take_pause_toggle(), 0U);
    send_frame(0x00U, 0x02U, 150U);
    failures += expect_u32("next play frame toggles",
        nes_controller_port_take_pause_toggle(), 1U);
    return failures;
}

static int test_init_ignores_old_frame(void)
{
    int failures = 0;

    memset(&g_ir_status, 0, sizeof(g_ir_status));
    g_ir_status.frame_valid = 1U;
    g_ir_status.address = 0x00U;
    g_ir_status.command = 0x02U;
    g_ir_status.frame_count = 9U;
    nes_input_init();
    nes_controller_port_init();
    nes_controller_port_poll(100U);
    failures += expect_u32("old play frame ignored",
        nes_controller_port_take_pause_toggle(), 0U);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_official_remote_mapping();
    failures += test_repeat_and_release();
    failures += test_pause_edge();
    failures += test_init_ignores_old_frame();
    if (failures == 0)
    {
        printf("PASS nes_controller_port\n");
    }
    return failures;
}
