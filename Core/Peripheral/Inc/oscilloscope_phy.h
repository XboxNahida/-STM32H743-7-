#ifndef OSCILLOSCOPE_PHY_H
#define OSCILLOSCOPE_PHY_H
#include <stdint.h>
typedef struct {
    void *context;
    int (*read)(void *,uint8_t,uint8_t,uint16_t *);
    int (*write)(void *,uint8_t,uint8_t,uint16_t);
} scope_phy_bus_t;
typedef enum {
    SCOPE_PHY_OK=0,SCOPE_PHY_NO_RESPONSE,SCOPE_PHY_ID_MISMATCH,
    SCOPE_PHY_CONTROL_READ,SCOPE_PHY_RESETTING,SCOPE_PHY_ISOLATE_READBACK
} scope_phy_stage_t;
typedef struct {uint16_t id1,id2,bmcr;uint8_t address,stage;} scope_phy_diagnostic_t;
/* 每次回调先低电平设置/释放MDIO，再上升采样，最后下降沿推进PHY输出。 */
typedef struct {void *context;unsigned (*bit)(void *,unsigned);} scope_phy_serial_t;
int scope_phy_serial_read(void *serial,uint8_t address,uint8_t reg,uint16_t *value);
int scope_phy_serial_write(void *serial,uint8_t address,uint8_t reg,uint16_t value);
typedef struct {
    uint16_t control;uint8_t address,active;
    scope_phy_diagnostic_t diagnostic;
} scope_phy_state_t;
int scope_phy_isolate(const scope_phy_bus_t *bus,scope_phy_state_t *state);
int scope_phy_restore(const scope_phy_bus_t *bus,scope_phy_state_t *state);
/* 仅主循环：PC1=MDC、PA2=MDIO。成功后才允许将PC4设置模拟输入。 */
int scope_phy_acquire(void);
int scope_phy_release(void);
int scope_phy_check(void);
void scope_phy_get_diagnostic(scope_phy_diagnostic_t *diagnostic);
#endif
