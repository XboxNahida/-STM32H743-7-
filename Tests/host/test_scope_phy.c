#include "oscilloscope_phy.h"
#include <assert.h>
#include <stdio.h>
static uint16_t regs[4]={0x1140U,0U,0U,0x128U};
static int fail_read,ignore_write;
static int rd(void *c,uint8_t a,uint8_t r,uint16_t *v)
{(void)c;if(fail_read||a!=1U||r>3U)return 0;*v=regs[r];return 1;}
static int wr(void *c,uint8_t a,uint8_t r,uint16_t v)
{(void)c;if(a!=1U||r!=0U)return 0;if(!ignore_write)regs[r]=v;return 1;}
static unsigned clocks,wire[80],response,absent;
static unsigned serial_bit(void *context,unsigned out)
{
    unsigned in=1U;(void)context;assert(clocks<80U);wire[clocks]=out;
    /* 地址最后一位的下降沿开始Z；下一拍为ACK，其后为MSB..LSB。 */
    if(!absent&&clocks==46U)in=0U;
    if(!absent&&clocks>=47U&&clocks<63U)in=(response>>(62U-clocks))&1U;
    clocks++;return in;
}
static void serial_test(void)
{
    scope_phy_serial_t serial={NULL,serial_bit};uint16_t value;unsigned i,header=0U;
    clocks=0U;response=0x0128U;assert(scope_phy_serial_read(&serial,1U,3U,&value));
    assert(value==0x0128U&&clocks==64U);
    for(i=0U;i<32U;i++)assert(wire[i]==1U);
    for(i=32U;i<46U;i++)header=(header<<1U)|wire[i];
    assert(header==0x1823U);
    clocks=0U;response=0U;assert(scope_phy_serial_read(&serial,1U,2U,&value)&&value==0U);
    clocks=0U;absent=1U;assert(!scope_phy_serial_read(&serial,1U,2U,&value));absent=0U;
    clocks=0U;assert(scope_phy_serial_write(&serial,1U,0U,0x1540U));
    assert(clocks==65U&&wire[46]==1U&&wire[47]==0U);
    value=0U;for(i=48U;i<64U;i++)value=(uint16_t)((value<<1U)|wire[i]);
    assert(value==0x1540U);
}
int main(void)
{
    serial_test();
    scope_phy_bus_t b={NULL,rd,wr};scope_phy_state_t s={0};
    assert(scope_phy_isolate(&b,&s));assert(s.active&&s.address==1U);
    assert(regs[0]==0x1540U&&s.control==0x1140U);
    assert(scope_phy_isolate(&b,&s)&&s.control==0x1140U);
    fail_read=1;assert(!scope_phy_restore(&b,&s)&&s.active);
    fail_read=0;assert(scope_phy_restore(&b,&s)&&!s.active&&regs[0]==0x1140U);
    ignore_write=1;assert(!scope_phy_isolate(&b,&s));assert(!s.active);
    assert(s.diagnostic.stage==SCOPE_PHY_ISOLATE_READBACK&&s.diagnostic.bmcr==0x1140U);
    ignore_write=0;regs[3]=0xffffU;assert(!scope_phy_isolate(&b,&s));
    assert(s.diagnostic.stage==SCOPE_PHY_ID_MISMATCH&&s.diagnostic.id2==0xffffU);
    regs[3]=0x128U;regs[0]=0x9140U;assert(!scope_phy_isolate(&b,&s));
    assert(s.diagnostic.stage==SCOPE_PHY_RESETTING);
    fail_read=1;assert(!scope_phy_isolate(&b,&s));
    assert(s.diagnostic.stage==SCOPE_PHY_NO_RESPONSE);
    puts("PHY隔离：识别/读回失败/复位中拒绝/恢复重试通过");return 0;
}
