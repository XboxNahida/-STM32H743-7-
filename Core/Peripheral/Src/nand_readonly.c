#include "nand_readonly.h"

#include <string.h>

#include "diagnostics.h"

#define NAND_DIAGNOSTICS_MODULE 0x4E44U

#if defined(NAND_READONLY_HOST_TEST)
extern void nand_readonly_platform_command(uint8_t value);
extern void nand_readonly_platform_address(uint8_t value);
extern uint8_t nand_readonly_platform_data_read(void);
extern uint8_t nand_readonly_platform_ready(void);
extern uint32_t nand_readonly_platform_tick(void);
extern void nand_readonly_platform_barrier(void);
extern void nand_readonly_platform_reset_settle(void);
#else
#include "main.h"

#define NAND_DATA_ADDRESS     0x80000000UL
#define NAND_COMMAND_ADDRESS (NAND_DATA_ADDRESS | (1UL << 16))
#define NAND_LATCH_ADDRESS   (NAND_DATA_ADDRESS | (1UL << 17))

static void nand_readonly_platform_command(uint8_t value)
{
    *(volatile uint8_t *)NAND_COMMAND_ADDRESS = value;
}

static void nand_readonly_platform_address(uint8_t value)
{
    *(volatile uint8_t *)NAND_LATCH_ADDRESS = value;
}

static uint8_t nand_readonly_platform_data_read(void)
{
    return *(volatile uint8_t *)NAND_DATA_ADDRESS;
}

static uint8_t nand_readonly_platform_ready(void)
{
    return (HAL_GPIO_ReadPin(NAND_RB_GPIO_Port, NAND_RB_Pin) == GPIO_PIN_SET) ?
           1U : 0U;
}

static uint32_t nand_readonly_platform_tick(void)
{
    return HAL_GetTick();
}

static void nand_readonly_platform_barrier(void)
{
    __DSB();
}

static void nand_readonly_platform_reset_settle(void)
{
    HAL_Delay(1U);
}

#endif

typedef struct
{
    uint8_t bytes[5];
    nand_geometry_t geometry;
} nand_model_t;

static const nand_model_t g_models[] =
{
    {{0x2CU, 0xDCU, 0x90U, 0x95U, 0x56U},
     {2048U, 64U, 64U, 4096U, 3U}},
    {{0xC8U, 0xDCU, 0x90U, 0x95U, 0x56U},
     {2048U, 64U, 64U, 4096U, 3U}},
    {{0x2CU, 0x48U, 0x00U, 0x26U, 0x89U},
     {4096U, 224U, 128U, 4096U, 3U}},
    {{0xADU, 0xDCU, 0x00U, 0xA2U, 0x62U},
     {4096U, 64U, 64U, 2048U, 3U}}
};

static nand_id_t g_id;
static nand_geometry_t g_geometry;
static nand_readonly_result_t g_probe_result;

static int nand_wait_ready(uint32_t timeout_ms)
{
    uint32_t start = nand_readonly_platform_tick();

    while (nand_readonly_platform_ready() == 0U)
    {
        if ((uint32_t)(nand_readonly_platform_tick() - start) >= timeout_ms)
        {
            return NAND_READONLY_TIMEOUT;
        }
    }
    return NAND_READONLY_OK;
}

static int nand_wait_operation_complete(uint32_t timeout_ms)
{
    uint32_t start = nand_readonly_platform_tick();

    while (nand_readonly_platform_ready() != 0U)
    {
        if ((uint32_t)(nand_readonly_platform_tick() - start) >= timeout_ms)
        {
            return NAND_READONLY_TIMEOUT;
        }
    }
    while (nand_readonly_platform_ready() == 0U)
    {
        if ((uint32_t)(nand_readonly_platform_tick() - start) >= timeout_ms)
        {
            return NAND_READONLY_TIMEOUT;
        }
    }
    return NAND_READONLY_OK;
}

static int nand_id_is_absent(const nand_id_t *id)
{
    uint32_t index;
    uint8_t all_zero = 1U;
    uint8_t all_ff = 1U;

    for (index = 0U; index < 5U; index++)
    {
        if (id->bytes[index] != 0U) all_zero = 0U;
        if (id->bytes[index] != 0xFFU) all_ff = 0U;
    }
    return ((all_zero != 0U) || (all_ff != 0U)) ? 1 : 0;
}

void nand_readonly_init(void)
{
    memset(&g_id, 0, sizeof(g_id));
    memset(&g_geometry, 0, sizeof(g_geometry));
    g_probe_result = NAND_READONLY_NOT_INITIALIZED;
}

int nand_readonly_probe(uint32_t timeout_ms)
{
    uint32_t index;
    int result;

    if (timeout_ms == 0U)
    {
        return NAND_READONLY_INVALID_ARG;
    }

    memset(&g_id, 0, sizeof(g_id));
    memset(&g_geometry, 0, sizeof(g_geometry));
    g_probe_result = NAND_READONLY_NOT_INITIALIZED;

    nand_readonly_platform_command(0xFFU);
    nand_readonly_platform_barrier();
    nand_readonly_platform_reset_settle();
    result = nand_wait_ready(timeout_ms);
    if (result != NAND_READONLY_OK)
    {
        g_probe_result = (nand_readonly_result_t)result;
        diagnostics_capture_first(NAND_DIAGNOSTICS_MODULE,
                                  (int32_t)result,
                                  0xFFU,
                                  timeout_ms);
        return result;
    }

    nand_readonly_platform_command(0x90U);
    nand_readonly_platform_address(0x00U);
    nand_readonly_platform_barrier();
    for (index = 0U; index < 5U; index++)
    {
        g_id.bytes[index] = nand_readonly_platform_data_read();
    }
    g_id.compact_id = ((uint32_t)g_id.bytes[1] << 24) |
                      ((uint32_t)g_id.bytes[2] << 16) |
                      ((uint32_t)g_id.bytes[3] << 8) |
                      (uint32_t)g_id.bytes[4];

    if (nand_id_is_absent(&g_id) != 0)
    {
        g_probe_result = NAND_READONLY_NOT_PRESENT;
        return g_probe_result;
    }

    memset(&g_geometry, 0, sizeof(g_geometry));
    for (index = 0U; index < (sizeof(g_models) / sizeof(g_models[0])); index++)
    {
        if (memcmp(g_models[index].bytes,
                   g_id.bytes,
                   sizeof(g_id.bytes)) == 0)
        {
            g_geometry = g_models[index].geometry;
            g_probe_result = NAND_READONLY_OK;
            return NAND_READONLY_OK;
        }
    }

    g_probe_result = NAND_READONLY_UNKNOWN_ID;
    return g_probe_result;
}

int nand_readonly_get_id(nand_id_t *id)
{
    if (id == NULL)
    {
        return NAND_READONLY_INVALID_ARG;
    }
    if ((g_probe_result != NAND_READONLY_OK) &&
        (g_probe_result != NAND_READONLY_UNKNOWN_ID))
    {
        return g_probe_result;
    }
    *id = g_id;
    return NAND_READONLY_OK;
}

int nand_readonly_get_geometry(nand_geometry_t *geometry)
{
    if (geometry == NULL)
    {
        return NAND_READONLY_INVALID_ARG;
    }
    if (g_probe_result != NAND_READONLY_OK)
    {
        return g_probe_result;
    }
    *geometry = g_geometry;
    return NAND_READONLY_OK;
}

int nand_readonly_read_status(uint8_t *status)
{
    if (status == NULL)
    {
        return NAND_READONLY_INVALID_ARG;
    }
    if ((g_probe_result != NAND_READONLY_OK) &&
        (g_probe_result != NAND_READONLY_UNKNOWN_ID))
    {
        return g_probe_result;
    }
    nand_readonly_platform_command(0x70U);
    nand_readonly_platform_barrier();
    *status = nand_readonly_platform_data_read();
    return NAND_READONLY_OK;
}

int nand_readonly_read_page(uint32_t page,
                            uint32_t column,
                            uint8_t *buffer,
                            uint32_t length,
                            uint32_t timeout_ms)
{
    uint32_t page_count;
    uint32_t page_total_size;
    uint32_t index;
    int result;

    if ((buffer == NULL) || (length == 0U) || (timeout_ms == 0U))
    {
        return NAND_READONLY_INVALID_ARG;
    }
    if (g_probe_result != NAND_READONLY_OK)
    {
        return g_probe_result;
    }

    page_count = g_geometry.pages_per_block * g_geometry.block_count;
    page_total_size = g_geometry.page_main_size + g_geometry.page_spare_size;
    if ((page >= page_count) || (column >= page_total_size) ||
        (length > (page_total_size - column)))
    {
        return NAND_READONLY_INVALID_ARG;
    }

    nand_readonly_platform_command(0x00U);
    nand_readonly_platform_address((uint8_t)column);
    nand_readonly_platform_address((uint8_t)(column >> 8));
    nand_readonly_platform_address((uint8_t)page);
    nand_readonly_platform_address((uint8_t)(page >> 8));
    nand_readonly_platform_address((uint8_t)(page >> 16));
    nand_readonly_platform_command(0x30U);
    nand_readonly_platform_barrier();

    result = nand_wait_operation_complete(timeout_ms);
    if (result != NAND_READONLY_OK)
    {
        diagnostics_capture_first(NAND_DIAGNOSTICS_MODULE,
                                  (int32_t)result,
                                  page,
                                  column);
        return result;
    }
    for (index = 0U; index < length; index++)
    {
        buffer[index] = nand_readonly_platform_data_read();
    }
    return NAND_READONLY_OK;
}
