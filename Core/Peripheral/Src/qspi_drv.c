/**
 ********************************************************************
 * @file    qspi_drv.c
 * @brief   QSPI底层驱动 (纯HAL, 完全对齐正点原子官方示例)
 ********************************************************************
 */
#include "qspi_drv.h"

extern QSPI_HandleTypeDef hqspi;

uint8_t qspi_wait_flag(uint32_t flag, uint8_t sta, uint32_t wtime)
{
    uint8_t flagsta;
    while (wtime) {
        flagsta = (QUADSPI->SR & flag) ? 1 : 0;
        if (flagsta == sta) break;
        wtime--;
    }
    return (wtime ? 0 : 1);
}

uint8_t qspi_send_cmd_timeout(uint8_t cmd, uint32_t addr, uint8_t mode,
                              uint8_t dmcycle, uint32_t timeout_ms)
{
    QSPI_CommandTypeDef sCmd = {0};

    if (timeout_ms == 0U)
    {
        return 1U;
    }

    sCmd.Instruction = cmd;
    sCmd.Address = addr;
    sCmd.DummyCycles = dmcycle;

    if      (((mode >> 0) & 0x03) == 0) sCmd.InstructionMode = QSPI_INSTRUCTION_NONE;
    else if (((mode >> 0) & 0x03) == 1) sCmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    else if (((mode >> 0) & 0x03) == 2) sCmd.InstructionMode = QSPI_INSTRUCTION_2_LINES;
    else                                 sCmd.InstructionMode = QSPI_INSTRUCTION_4_LINES;

    if      (((mode >> 2) & 0x03) == 0) sCmd.AddressMode = QSPI_ADDRESS_NONE;
    else if (((mode >> 2) & 0x03) == 1) sCmd.AddressMode = QSPI_ADDRESS_1_LINE;
    else if (((mode >> 2) & 0x03) == 2) sCmd.AddressMode = QSPI_ADDRESS_2_LINES;
    else                                 sCmd.AddressMode = QSPI_ADDRESS_4_LINES;

    if      (((mode >> 4) & 0x03) == 0) sCmd.AddressSize = QSPI_ADDRESS_8_BITS;
    else if (((mode >> 4) & 0x03) == 1) sCmd.AddressSize = QSPI_ADDRESS_16_BITS;
    else if (((mode >> 4) & 0x03) == 2) sCmd.AddressSize = QSPI_ADDRESS_24_BITS;
    else                                 sCmd.AddressSize = QSPI_ADDRESS_32_BITS;

    if      (((mode >> 6) & 0x03) == 0) sCmd.DataMode = QSPI_DATA_NONE;
    else if (((mode >> 6) & 0x03) == 1) sCmd.DataMode = QSPI_DATA_1_LINE;
    else if (((mode >> 6) & 0x03) == 2) sCmd.DataMode = QSPI_DATA_2_LINES;
    else                                 sCmd.DataMode = QSPI_DATA_4_LINES;

    sCmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
    sCmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    sCmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    sCmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;

    return (HAL_QSPI_Command(&hqspi, &sCmd, timeout_ms) == HAL_OK) ? 0U : 1U;
}

void qspi_send_cmd(uint8_t cmd, uint32_t addr, uint8_t mode, uint8_t dmcycle)
{
    (void)qspi_send_cmd_timeout(cmd, addr, mode, dmcycle, 5000U);
}

uint8_t qspi_receive_timeout(uint8_t *buf, uint32_t datalen,
                             uint32_t timeout_ms)
{
    if ((buf == NULL) || (datalen == 0U) || (timeout_ms == 0U))
    {
        return 1U;
    }
    hqspi.Instance->DLR = datalen - 1U;
    return (HAL_QSPI_Receive(&hqspi, buf, timeout_ms) == HAL_OK) ? 0U : 1U;
}

uint8_t qspi_receive(uint8_t *buf, uint32_t datalen)
{
    return qspi_receive_timeout(buf, datalen, 5000U);
}

uint8_t qspi_transmit(uint8_t *buf, uint32_t datalen)
{
    hqspi.Instance->DLR = datalen - 1;
    if (HAL_QSPI_Transmit(&hqspi, buf, 5000) == HAL_OK)
        return 0;
    else
        return 1;
}
