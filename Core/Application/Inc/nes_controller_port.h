#ifndef NES_CONTROLLER_PORT_H
#define NES_CONTROLLER_PORT_H

#include <stdint.h>

void nes_controller_port_init(void);
void nes_controller_port_poll(uint32_t now_ms);
uint8_t nes_controller_port_take_pause_toggle(void);

#endif
