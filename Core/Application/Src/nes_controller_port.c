#include "nes_controller_port.h"

#include "ir_remote_service.h"
#include "nes_input.h"

#define NES_IR_ADDRESS       0x00U
#define NES_IR_PAUSE_COMMAND 0x02U

static const nes_ir_binding_t g_nes_ir_map[] =
{
    {NES_IR_ADDRESS, 0x62U, NES_BUTTON_UP},
    {NES_IR_ADDRESS, 0xA8U, NES_BUTTON_DOWN},
    {NES_IR_ADDRESS, 0x22U, NES_BUTTON_LEFT},
    {NES_IR_ADDRESS, 0xC2U, NES_BUTTON_RIGHT},
    {NES_IR_ADDRESS, 0x68U, NES_BUTTON_A},
    {NES_IR_ADDRESS, 0xB0U, NES_BUTTON_B},
    {NES_IR_ADDRESS, 0x10U, NES_BUTTON_SELECT},
    {NES_IR_ADDRESS, 0x5AU, NES_BUTTON_START}
};

static uint32_t g_last_frame_count;
static uint8_t g_pause_toggle;

static void nes_controller_copy_snapshot(const ir_remote_status_t *status,
                                         nes_ir_snapshot_t *snapshot)
{
    snapshot->frame_valid = status->frame_valid;
    snapshot->address = status->address;
    snapshot->command = status->command;
    snapshot->frame_count = status->frame_count;
    snapshot->repeat_count = status->repeat_count;
}

void nes_controller_port_init(void)
{
    ir_remote_status_t status;
    nes_ir_snapshot_t baseline;

    ir_remote_get_status(&status);
    (void)nes_input_set_ir_map(g_nes_ir_map,
        (uint8_t)(sizeof(g_nes_ir_map) / sizeof(g_nes_ir_map[0])));
    nes_controller_copy_snapshot(&status, &baseline);
    baseline.frame_valid = 0U;
    (void)nes_input_update_ir(&baseline, 0U);
    g_last_frame_count = status.frame_count;
    g_pause_toggle = 0U;
}

void nes_controller_port_poll(uint32_t now_ms)
{
    ir_remote_status_t status;
    nes_ir_snapshot_t snapshot;

    ir_remote_get_status(&status);
    nes_controller_copy_snapshot(&status, &snapshot);
    if (status.frame_count != g_last_frame_count)
    {
        if ((status.frame_valid != 0U) &&
            (status.address == NES_IR_ADDRESS) &&
            (status.command == NES_IR_PAUSE_COMMAND))
        {
            g_pause_toggle = 1U;
        }
        g_last_frame_count = status.frame_count;
    }
    (void)nes_input_update_ir(&snapshot, now_ms);
}

uint8_t nes_controller_port_take_pause_toggle(void)
{
    uint8_t pause_toggle = g_pause_toggle;

    g_pause_toggle = 0U;
    return pause_toggle;
}
