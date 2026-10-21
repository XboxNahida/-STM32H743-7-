#include "touch_i2c.h"

/* 简单可靠的延时函数 */
void touch_iic_delay(void)
{
    volatile uint32_t count = 500;
    while(count--)
    {
        __NOP();
    }
}

/* 产生IIC起始信号 */
void touch_iic_start(void)
{
    TOUCH_SDA_HIGH();
    TOUCH_SCL_HIGH();
    touch_iic_delay();
    TOUCH_SDA_LOW();     /* START: SCL高时，SDA从高变低 */
    touch_iic_delay();
    TOUCH_SCL_LOW();     /* 钳住总线，准备发送数据 */
    touch_iic_delay();
}

/* 产生IIC停止信号 */
void touch_iic_stop(void)
{
    TOUCH_SDA_LOW();
    touch_iic_delay();
    TOUCH_SCL_HIGH();
    touch_iic_delay();
    TOUCH_SDA_HIGH();    /* STOP: SCL高时，SDA从低变高 */
    touch_iic_delay();
}

/* 等待应答信号 */
uint8_t touch_iic_wait_ack(void)
{
    uint16_t waittime = 0;
    
    TOUCH_SDA_HIGH();    /* 释放SDA */
    touch_iic_delay();
    TOUCH_SCL_HIGH();    /* SCL拉高，等待从机拉低SDA */
    touch_iic_delay();
    
    while(TOUCH_SDA_READ())
    {
        waittime++;
        if(waittime > 5000)  /* 超时 */
        {
            touch_iic_stop();
            return 1;        /* 应答失败 */
        }
    }
    
    TOUCH_SCL_LOW();
    touch_iic_delay();
    return 0;                /* 应答成功 */
}

/* 产生ACK应答 */
void touch_iic_ack(void)
{
    TOUCH_SDA_LOW();
    touch_iic_delay();
    TOUCH_SCL_HIGH();
    touch_iic_delay();
    TOUCH_SCL_LOW();
    touch_iic_delay();
    TOUCH_SDA_HIGH();    /* 释放SDA */
    touch_iic_delay();
}

/* 产生NACK不应答 */
void touch_iic_nack(void)
{
    TOUCH_SDA_HIGH();
    touch_iic_delay();
    TOUCH_SCL_HIGH();
    touch_iic_delay();
    TOUCH_SCL_LOW();
    touch_iic_delay();
}

/* 发送一个字节 */
void touch_iic_send_byte(uint8_t data)
{
    uint8_t t;
    
    for(t = 0; t < 8; t++)
    {
        if(data & 0x80)
            TOUCH_SDA_HIGH();
        else
            TOUCH_SDA_LOW();
        
        data <<= 1;
        touch_iic_delay();
        TOUCH_SCL_HIGH();
        touch_iic_delay();
        TOUCH_SCL_LOW();
        touch_iic_delay();
    }
    
    TOUCH_SDA_HIGH();    /* 释放SDA */
}

/* 读取一个字节 */
uint8_t touch_iic_read_byte(uint8_t ack)
{
    uint8_t i, receive = 0;
    
    TOUCH_SDA_HIGH();    /* 释放SDA */
    
    for(i = 0; i < 8; i++)
    {
        receive <<= 1;
        TOUCH_SCL_HIGH();
        touch_iic_delay();
        
        if(TOUCH_SDA_READ())
            receive |= 0x01;
        
        TOUCH_SCL_LOW();
        touch_iic_delay();
    }
    
    if(ack)
        touch_iic_ack();
    else
        touch_iic_nack();
    
    return receive;
}

