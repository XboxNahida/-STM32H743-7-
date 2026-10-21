/**
 ********************************************************************
 * @file    qspi_drv.h
 * @brief   QSPI底层驱动(发送命令/收发数据)
 * @note    依赖 CubeMX 生成的 MX_QUADSPI_Init() 完成初始化
 *          移植自正点原子 qspi.c, 适配 HAL 框架
 ********************************************************************
 */
#ifndef __QSPI_DRV_H__
#define __QSPI_DRV_H__

#include "main.h"
#include "quadspi.h"

extern QSPI_HandleTypeDef hqspi;

/* ===== QSPI 模式编码 (mode参数) ===== */
/*
 * mode[1:0]: 指令模式  00=无  01=单线  10=双线  11=四线
 * mode[3:2]: 地址模式  00=无  01=单线  10=双线  11=四线
 * mode[5:4]: 地址长度  00=8位  01=16位  10=24位  11=32位
 * mode[7:6]: 数据模式  00=无  01=单线  10=双线  11=四线
 */

uint8_t qspi_wait_flag(uint32_t flag, uint8_t sta, uint32_t wtime);
uint8_t qspi_send_cmd_timeout(uint8_t cmd, uint32_t addr, uint8_t mode,
                              uint8_t dmcycle, uint32_t timeout_ms);
uint8_t qspi_receive_timeout(uint8_t *buf, uint32_t datalen,
                             uint32_t timeout_ms);
void    qspi_send_cmd(uint8_t cmd, uint32_t addr, uint8_t mode, uint8_t dmcycle);
uint8_t qspi_receive(uint8_t *buf, uint32_t datalen);
uint8_t qspi_transmit(uint8_t *buf, uint32_t datalen);

#endif
