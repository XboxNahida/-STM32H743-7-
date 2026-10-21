#include "oscilloscope_phy.h"
#include <stddef.h>
#define ISOLATE 0x0400U
static void serial_send(scope_phy_serial_t *s,uint32_t value,unsigned count)
{while(count){count--; (void)s->bit(s->context,(value>>count)&1U);}}
static void serial_header(scope_phy_serial_t *s,uint8_t a,uint8_t r,unsigned op)
{serial_send(s,0xffffffffU,32U);serial_send(s,1U,2U);serial_send(s,op,2U);serial_send(s,a,5U);serial_send(s,r,5U);}
int scope_phy_serial_read(void *context,uint8_t a,uint8_t r,uint16_t *value)
{
    scope_phy_serial_t *s=context;unsigned i,ack;uint16_t data=0U;
    if(!s||!s->bit||!value||a>31U||r>31U)return 0;
    /* 最后地址位的下降沿已进入Z；这里只读一次TA=0，不能吞掉数据MSB。 */
    serial_header(s,a,r,2U);ack=s->bit(s->context,1U);
    for(i=0U;i<16U;i++)data=(uint16_t)((data<<1U)|s->bit(s->context,1U));
    (void)s->bit(s->context,1U);*value=data;return ack==0U;
}
int scope_phy_serial_write(void *context,uint8_t a,uint8_t r,uint16_t value)
{
    scope_phy_serial_t *s=context;if(!s||!s->bit||a>31U||r>31U)return 0;
    serial_header(s,a,r,1U);serial_send(s,2U,2U);serial_send(s,value,16U);
    (void)s->bit(s->context,1U);return 1;
}
int scope_phy_restore(const scope_phy_bus_t *b,scope_phy_state_t *s)
{
    uint16_t now,check,target;
    if(!b||!s||!b->read||!b->write)return 0;
    if(!s->active)return 1;
    if(!b->read(b->context,s->address,0U,&now)||(now&0x8000U))return 0;
    target=(uint16_t)((now&~ISOLATE)|(s->control&ISOLATE));
    if(!b->write(b->context,s->address,0U,target)||
       !b->read(b->context,s->address,0U,&check)||((check^target)&ISOLATE))return 0;
    s->active=0U;return 1;
}
int scope_phy_isolate(const scope_phy_bus_t *b,scope_phy_state_t *s)
{
    uint16_t id1,id2,control,check=0U;uint8_t a;
    if(!b||!s||!b->read||!b->write)return 0;
    if(s->active){
        int ok=b->read(b->context,s->address,0U,&check)&&
            (check&(ISOLATE|0x8000U))==ISOLATE;
        s->diagnostic.bmcr=check;s->diagnostic.stage=ok?SCOPE_PHY_OK:SCOPE_PHY_ISOLATE_READBACK;
        return ok;
    }
    s->diagnostic=(scope_phy_diagnostic_t){0xffffU,0xffffU,0U,0xffU,SCOPE_PHY_NO_RESPONSE};
    for(a=0U;a<32U;a++){
        if(!b->read(b->context,a,2U,&id1)||!b->read(b->context,a,3U,&id2))continue;
        s->diagnostic.id1=id1;s->diagnostic.id2=id2;s->diagnostic.address=a;
        s->diagnostic.stage=SCOPE_PHY_ID_MISMATCH;
        if(id1!=0U||(id2&0xfff0U)!=0x0120U)continue;
        s->diagnostic.stage=SCOPE_PHY_CONTROL_READ;
        if(!b->read(b->context,a,0U,&control))return 0;
        s->diagnostic.bmcr=control;
        if(control&0x8000U){s->diagnostic.stage=SCOPE_PHY_RESETTING;return 0;}
        s->control=control;s->address=a;s->active=1U;
        s->diagnostic.stage=SCOPE_PHY_ISOLATE_READBACK;
        if(b->write(b->context,a,0U,(uint16_t)(control|ISOLATE))&&
           b->read(b->context,a,0U,&check)){
            s->diagnostic.bmcr=check;
            if((check&(ISOLATE|0x8000U))==ISOLATE){s->diagnostic.stage=SCOPE_PHY_OK;return 1;}
        }
        (void)scope_phy_restore(b,s);return 0;
    }
    return 0;
}
#ifndef SCOPE_HOST_TEST
#include "main.h"
typedef struct {uint32_t mode,pull,speed,type,af,output;} pin_state_t;
static pin_state_t saved_mdc,saved_mdio;
static scope_phy_state_t state;
static uint8_t pins_held;
void scope_phy_get_diagnostic(scope_phy_diagnostic_t *d){if(d)*d=state.diagnostic;}
static void save_pin(GPIO_TypeDef *g,unsigned p,pin_state_t *s)
{
    s->mode=(g->MODER>>(p*2U))&3U;s->pull=(g->PUPDR>>(p*2U))&3U;
    s->speed=(g->OSPEEDR>>(p*2U))&3U;s->type=(g->OTYPER>>p)&1U;
    s->af=(g->AFR[p/8U]>>((p%8U)*4U))&15U;s->output=(g->ODR>>p)&1U;
}
static void restore_pin(GPIO_TypeDef *g,unsigned p,const pin_state_t *s)
{
    g->BSRR=1UL<<(p+(s->output?0U:16U));
    g->OTYPER=(g->OTYPER&~(1UL<<p))|(s->type<<p);
    g->PUPDR=(g->PUPDR&~(3UL<<(p*2U)))|(s->pull<<(p*2U));
    g->OSPEEDR=(g->OSPEEDR&~(3UL<<(p*2U)))|(s->speed<<(p*2U));
    g->AFR[p/8U]=(g->AFR[p/8U]&~(15UL<<((p%8U)*4U)))|(s->af<<((p%8U)*4U));
    g->MODER=(g->MODER&~(3UL<<(p*2U)))|(s->mode<<(p*2U));
}
static void delay_edge(void)
{
    volatile uint32_t n=SystemCoreClock/1000000U;
    while(n--){__NOP();} /* 慢于1MHz，满足PHY MDC上限和建立时间。 */
}
static unsigned bit(void *context,unsigned value)
{
    unsigned sampled;(void)context;
    GPIOC->BSRR=GPIO_PIN_1<<16U;GPIOA->BSRR=GPIO_PIN_2<<(value?0U:16U);
    delay_edge();GPIOC->BSRR=GPIO_PIN_1;delay_edge();
    sampled=(GPIOA->IDR&GPIO_PIN_2)?1U:0U;
    GPIOC->BSRR=GPIO_PIN_1<<16U;return sampled;
}
static scope_phy_serial_t serial={NULL,bit};
static const scope_phy_bus_t bus={&serial,scope_phy_serial_read,scope_phy_serial_write};
int scope_phy_acquire(void)
{
    GPIO_InitTypeDef gpio={0};
    if(!pins_held){
        __HAL_RCC_GPIOA_CLK_ENABLE();__HAL_RCC_GPIOC_CLK_ENABLE();
        save_pin(GPIOC,1U,&saved_mdc);save_pin(GPIOA,2U,&saved_mdio);
        GPIOC->BSRR=GPIO_PIN_1<<16U;GPIOA->BSRR=GPIO_PIN_2;
        gpio.Pin=GPIO_PIN_1;gpio.Mode=GPIO_MODE_OUTPUT_PP;gpio.Pull=GPIO_NOPULL;
        gpio.Speed=GPIO_SPEED_FREQ_LOW;HAL_GPIO_Init(GPIOC,&gpio);
        gpio.Pin=GPIO_PIN_2;gpio.Mode=GPIO_MODE_OUTPUT_OD;gpio.Pull=GPIO_PULLUP;
        HAL_GPIO_Init(GPIOA,&gpio);pins_held=1U;
    }
    if(scope_phy_isolate(&bus,&state))return 1;
    (void)scope_phy_release();return 0;
}
int scope_phy_release(void)
{
    if(!pins_held)return 1;
    if(!scope_phy_restore(&bus,&state))return 0;
    restore_pin(GPIOA,2U,&saved_mdio);restore_pin(GPIOC,1U,&saved_mdc);
    pins_held=0U;return 1;
}
int scope_phy_check(void)
{uint16_t v;return pins_held&&state.active&&scope_phy_serial_read(&serial,state.address,0U,&v)&&
    (v&(ISOLATE|0x8000U))==ISOLATE;}
#endif
