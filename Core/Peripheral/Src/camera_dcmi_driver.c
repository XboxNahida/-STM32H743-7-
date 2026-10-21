#include "camera_dcmi_driver.h"

#include <string.h>

#include "sdram_layout.h"

#if defined(CAMERA_DCMI_HOST_TEST)
extern int camera_dcmi_test_platform_init(void);
extern int camera_dcmi_test_platform_start(uintptr_t address, uint32_t words,
                                           uint8_t snapshot);
extern int camera_dcmi_test_platform_start_chunks(uintptr_t address0,
                                                  uintptr_t address1,
                                                  uint32_t words,
                                                  uint8_t snapshot);
extern int camera_dcmi_test_platform_copy_chunk(uintptr_t source,
                                                uintptr_t destination,
                                                uint32_t bytes);
extern int camera_dcmi_test_platform_copy_busy(void);
extern int camera_dcmi_test_platform_stop(void);
extern void camera_dcmi_test_set_irqs(uint8_t enabled);
extern void camera_dcmi_test_invalidate_cache(uintptr_t address,
                                              uint32_t length);
#else
#include "stm32h7xx_hal.h"

static DCMI_HandleTypeDef g_camera_dcmi;
static DMA_HandleTypeDef g_camera_dma;
static void camera_dcmi_dma_chunk_complete(DMA_HandleTypeDef *hdma);
static void camera_dcmi_dma_error(DMA_HandleTypeDef *hdma);
#endif

static uint32_t g_camera_chunk_buffers[2][CAMERA_DCMI_CHUNK_WORDS]
    __attribute__((aligned(32)));

static volatile uint32_t g_camera_events;
static volatile uint32_t g_frame_count;
static volatile uint32_t g_error_count;
static volatile uint32_t g_dma_error_count;
static volatile uint32_t g_extra_tc_count;
static volatile uint32_t g_last_business_error_code;
static volatile uint32_t g_last_dcmi_error_code;
static volatile uint32_t g_last_dma_error_code;
static volatile uint32_t g_last_dcmi_risr;
static volatile uint32_t g_last_dma_lisr;
static camera_dcmi_status_t g_camera_status;
static uint8_t g_irqs_enabled;
static volatile uint32_t g_camera_chunk_count;
static volatile uint8_t g_camera_frame_seen;

static void camera_dcmi_invalidate_chunk(uint8_t buffer_index)
{
#if defined(CAMERA_DCMI_HOST_TEST)
    camera_dcmi_test_invalidate_cache(
        (uintptr_t)g_camera_chunk_buffers[buffer_index],
        CAMERA_DCMI_CHUNK_BYTES);
#else
    SCB_InvalidateDCache_by_Addr(
        g_camera_chunk_buffers[buffer_index],
        (int32_t)CAMERA_DCMI_CHUNK_BYTES);
#endif
}

static int camera_dcmi_copy_state(void)
{
#if defined(CAMERA_DCMI_HOST_TEST)
    return (camera_dcmi_test_platform_copy_busy() != 0) ? 1 : 0;
#else
    return 0;
#endif
}

static int camera_dcmi_start_chunk_copy(uint8_t buffer_index,
                                        uint32_t chunk_index)
{
    uintptr_t source = (uintptr_t)g_camera_chunk_buffers[buffer_index];
    uintptr_t destination = SDRAM_VIDEO_RGB_A_ADDR +
                            (chunk_index * CAMERA_DCMI_CHUNK_BYTES);

    camera_dcmi_invalidate_chunk(buffer_index);
#if defined(CAMERA_DCMI_HOST_TEST)
    return camera_dcmi_test_platform_copy_chunk(
        source, destination, CAMERA_DCMI_CHUNK_BYTES);
#else
    memcpy((void *)destination, (const void *)source,
           CAMERA_DCMI_CHUNK_BYTES);
    return 0;
#endif
}

static void camera_dcmi_invalidate_frame(void)
{
#if defined(CAMERA_DCMI_HOST_TEST)
    camera_dcmi_test_invalidate_cache(SDRAM_VIDEO_RGB_A_ADDR,
                                      CAMERA_DCMI_FRAME_BYTES);
#else
    SCB_InvalidateDCache_by_Addr((uint32_t *)SDRAM_VIDEO_RGB_A_ADDR,
                                 (int32_t)CAMERA_DCMI_FRAME_BYTES);
#endif
}

static void camera_dcmi_set_irqs(uint8_t enabled)
{
    enabled = (uint8_t)(enabled != 0U);
    if (enabled == g_irqs_enabled)
    {
        return;
    }
#if defined(CAMERA_DCMI_HOST_TEST)
    camera_dcmi_test_set_irqs(enabled);
#else
    if (enabled != 0U)
    {
        HAL_NVIC_ClearPendingIRQ(DCMI_IRQn);
        HAL_NVIC_ClearPendingIRQ(DMA1_Stream1_IRQn);
        HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 6U, 0U);
        HAL_NVIC_SetPriority(DCMI_IRQn, 7U, 0U);
        HAL_NVIC_EnableIRQ(DCMI_IRQn);
        HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
    }
    else
    {
        HAL_NVIC_DisableIRQ(DCMI_IRQn);
        HAL_NVIC_DisableIRQ(DMA1_Stream1_IRQn);
        HAL_NVIC_ClearPendingIRQ(DCMI_IRQn);
        HAL_NVIC_ClearPendingIRQ(DMA1_Stream1_IRQn);
    }
#endif
    g_irqs_enabled = enabled;
}

int camera_dcmi_init(void)
{
    memset(&g_camera_status, 0, sizeof(g_camera_status));
    g_camera_events = 0U;
    g_frame_count = 0U;
    g_error_count = 0U;
    g_dma_error_count = 0U;
    g_extra_tc_count = 0U;
    g_last_business_error_code = CAMERA_DCMI_BUSINESS_ERROR_NONE;
    g_last_dcmi_error_code = 0U;
    g_last_dma_error_code = 0U;
    g_last_dcmi_risr = 0U;
    g_last_dma_lisr = 0U;
    g_irqs_enabled = 0U;
    g_camera_chunk_count = 0U;
    g_camera_frame_seen = 0U;
    g_camera_status.frame_words = CAMERA_DCMI_FRAME_WORDS;
    g_camera_status.frame_bytes = CAMERA_DCMI_FRAME_BYTES;
    g_camera_status.cache_line = CAMERA_DCMI_CACHE_LINE;
    g_camera_status.last_result = CAMERA_DCMI_OK;
#if defined(CAMERA_DCMI_HOST_TEST)
    if (camera_dcmi_test_platform_init() != 0)
    {
        g_camera_status.last_result = CAMERA_DCMI_HAL_ERROR;
        return CAMERA_DCMI_HAL_ERROR;
    }
#else
    __HAL_RCC_DCMI_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();

    memset(&g_camera_dcmi, 0, sizeof(g_camera_dcmi));
    g_camera_dcmi.Instance = DCMI;
    g_camera_dcmi.Init.SynchroMode = DCMI_SYNCHRO_HARDWARE;
    g_camera_dcmi.Init.PCKPolarity = DCMI_PCKPOLARITY_RISING;
    g_camera_dcmi.Init.VSPolarity = DCMI_VSPOLARITY_LOW;
    g_camera_dcmi.Init.HSPolarity = DCMI_HSPOLARITY_LOW;
    g_camera_dcmi.Init.CaptureRate = DCMI_CR_ALL_FRAME;
    g_camera_dcmi.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
    g_camera_dcmi.Init.JPEGMode = DCMI_JPEG_DISABLE;
    if (HAL_DCMI_Init(&g_camera_dcmi) != HAL_OK)
    {
        g_camera_status.last_result = CAMERA_DCMI_HAL_ERROR;
        return CAMERA_DCMI_HAL_ERROR;
    }

    memset(&g_camera_dma, 0, sizeof(g_camera_dma));
    g_camera_dma.Instance = DMA1_Stream1;
    g_camera_dma.Init.Request = DMA_REQUEST_DCMI;
    g_camera_dma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    g_camera_dma.Init.PeriphInc = DMA_PINC_DISABLE;
    g_camera_dma.Init.MemInc = DMA_MINC_ENABLE;
    g_camera_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    g_camera_dma.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    g_camera_dma.Init.Mode = DMA_CIRCULAR;
    g_camera_dma.Init.Priority = DMA_PRIORITY_HIGH;
    g_camera_dma.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    g_camera_dma.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
    g_camera_dma.Init.MemBurst = DMA_MBURST_SINGLE;
    g_camera_dma.Init.PeriphBurst = DMA_PBURST_SINGLE;
    if (HAL_DMA_Init(&g_camera_dma) != HAL_OK)
    {
        g_camera_status.last_result = CAMERA_DCMI_HAL_ERROR;
        return CAMERA_DCMI_HAL_ERROR;
    }
    __HAL_LINKDMA(&g_camera_dcmi, DMA_Handle, g_camera_dma);
#endif
    g_camera_status.initialized = 1U;
    return CAMERA_DCMI_OK;
}

int camera_dcmi_start(void)
{
    int result = 0;

    if (g_camera_status.initialized == 0U)
    {
        g_camera_status.last_result = CAMERA_DCMI_NOT_READY;
        return CAMERA_DCMI_NOT_READY;
    }
    if (g_camera_status.running != 0U)
    {
        g_camera_status.last_result = CAMERA_DCMI_BUSY;
        return CAMERA_DCMI_BUSY;
    }

    camera_dcmi_set_irqs(1U);
    g_camera_chunk_count = 0U;
    g_camera_frame_seen = 0U;
    g_last_business_error_code = CAMERA_DCMI_BUSINESS_ERROR_NONE;
#if defined(CAMERA_DCMI_HOST_TEST)
    result = camera_dcmi_test_platform_start_chunks(
        (uintptr_t)g_camera_chunk_buffers[0],
        (uintptr_t)g_camera_chunk_buffers[1],
        CAMERA_DCMI_CHUNK_WORDS, 1U);
#else
    g_camera_dcmi.ErrorCode = HAL_DCMI_ERROR_NONE;
    g_camera_dma.ErrorCode = HAL_DMA_ERROR_NONE;
    g_camera_dma.XferCpltCallback = camera_dcmi_dma_chunk_complete;
    g_camera_dma.XferM1CpltCallback = camera_dcmi_dma_chunk_complete;
    g_camera_dma.XferErrorCallback = camera_dcmi_dma_error;
    __HAL_DCMI_CLEAR_FLAG(&g_camera_dcmi,
                          DCMI_FLAG_FRAMERI | DCMI_FLAG_VSYNCRI |
                          DCMI_FLAG_LINERI | DCMI_FLAG_ERRRI |
                          DCMI_FLAG_OVRRI);
    __HAL_DCMI_ENABLE_IT(&g_camera_dcmi,
                         DCMI_IT_FRAME | DCMI_IT_ERR | DCMI_IT_OVR);
    result = (HAL_DMAEx_MultiBufferStart_IT(
                  &g_camera_dma, (uint32_t)&DCMI->DR,
                  (uint32_t)g_camera_chunk_buffers[0],
                  (uint32_t)g_camera_chunk_buffers[1],
                  CAMERA_DCMI_CHUNK_WORDS) == HAL_OK) ? 0 : -1;
    if (result == 0)
    {
        __HAL_DMA_DISABLE_IT(&g_camera_dma, DMA_IT_FE);
        __HAL_DCMI_ENABLE(&g_camera_dcmi);
        MODIFY_REG(DCMI->CR, DCMI_CR_CM, DCMI_MODE_SNAPSHOT);
        SET_BIT(DCMI->CR, DCMI_CR_CAPTURE);
    }
#endif
    if (result != 0)
    {
        camera_dcmi_set_irqs(0U);
        g_camera_status.last_result = CAMERA_DCMI_HAL_ERROR;
        return CAMERA_DCMI_HAL_ERROR;
    }
    g_camera_status.running = 1U;
    g_camera_status.last_result = CAMERA_DCMI_OK;
    return CAMERA_DCMI_OK;
}

int camera_dcmi_stop(void)
{
    int result = 0;

    if (g_camera_status.initialized == 0U)
    {
        return CAMERA_DCMI_OK;
    }
    if (g_irqs_enabled != 0U)
    {
#if defined(CAMERA_DCMI_HOST_TEST)
        result = camera_dcmi_test_platform_stop();
#else
        result = (HAL_DCMI_Stop(&g_camera_dcmi) == HAL_OK) ? 0 : -1;
        CLEAR_BIT(DCMI->CR, DCMI_CR_CAPTURE);
        __HAL_DCMI_DISABLE_IT(&g_camera_dcmi,
                               DCMI_IT_FRAME | DCMI_IT_ERR | DCMI_IT_OVR);
        __HAL_DCMI_CLEAR_FLAG(&g_camera_dcmi,
                              DCMI_FLAG_FRAMERI | DCMI_FLAG_VSYNCRI |
                              DCMI_FLAG_LINERI | DCMI_FLAG_ERRRI |
                              DCMI_FLAG_OVRRI);
#endif
    }
    camera_dcmi_set_irqs(0U);
    g_camera_status.running = 0U;
    if (result != 0)
    {
        g_camera_status.last_result = CAMERA_DCMI_HAL_ERROR;
        return CAMERA_DCMI_HAL_ERROR;
    }
    g_camera_status.last_result = CAMERA_DCMI_OK;
    return CAMERA_DCMI_OK;
}

int camera_dcmi_poll_event(uint32_t *events)
{
    uint32_t pending;

    if (events == NULL)
    {
        return CAMERA_DCMI_INVALID_ARG;
    }
#if defined(CAMERA_DCMI_HOST_TEST)
    pending = g_camera_events;
    g_camera_events = 0U;
#else
    {
        uint32_t primask = __get_PRIMASK();
        __disable_irq();
        pending = g_camera_events;
        g_camera_events = 0U;
        if (primask == 0U)
        {
            __enable_irq();
        }
    }
#endif
    if (((pending & CAMERA_DCMI_EVENT_ERROR) == 0U) &&
        (g_camera_status.running != 0U) &&
        (g_camera_frame_seen != 0U) &&
        (g_camera_chunk_count == CAMERA_DCMI_CHUNKS_PER_FRAME))
    {
        int copy_state = camera_dcmi_copy_state();
        if (copy_state < 0)
        {
            g_error_count++;
            g_dma_error_count++;
            g_camera_status.running = 0U;
            g_camera_status.last_result = CAMERA_DCMI_HAL_ERROR;
            pending |= CAMERA_DCMI_EVENT_ERROR;
        }
        else if ((copy_state == 0) &&
                 (camera_dcmi_stop() == CAMERA_DCMI_OK))
        {
            camera_dcmi_invalidate_frame();
            g_frame_count++;
            pending |= CAMERA_DCMI_EVENT_FRAME;
        }
        else if (copy_state == 0)
        {
            pending |= CAMERA_DCMI_EVENT_ERROR;
            g_error_count++;
        }
    }
    *events = pending;
    return CAMERA_DCMI_OK;
}

void camera_dcmi_get_status(camera_dcmi_status_t *status)
{
    if (status != NULL)
    {
        *status = g_camera_status;
        status->frame_events = g_frame_count;
        status->error_events = g_error_count;
        status->dma_errors = g_dma_error_count;
        status->extra_tc_events = g_extra_tc_count;
        status->business_error_code = g_last_business_error_code;
        status->dcmi_error_code = g_last_dcmi_error_code;
        status->dma_error_code = g_last_dma_error_code;
        status->dcmi_risr = g_last_dcmi_risr;
        status->dma_lisr = g_last_dma_lisr;
        status->line_count = g_camera_chunk_count * CAMERA_DCMI_CHUNK_LINES;
        status->frame_seen = g_camera_frame_seen;
#if !defined(CAMERA_DCMI_HOST_TEST)
        status->dcmi_cr = DCMI->CR;
        status->dcmi_sr = DCMI->SR;
        status->dma_ndtr = DMA1_Stream1->NDTR;
        status->dma_cr = DMA1_Stream1->CR;
#endif
    }
}

void camera_dcmi_frame_callback(void)
{
    if (g_camera_status.running != 0U)
    {
        g_camera_frame_seen = 1U;
    }
}

void camera_dcmi_chunk_callback(uint8_t completed_buffer)
{
    int copy_state;

    if (g_camera_status.running == 0U)
    {
        return;
    }
    if (g_camera_chunk_count >= CAMERA_DCMI_CHUNKS_PER_FRAME)
    {
        if (g_extra_tc_count != UINT32_MAX)
        {
            g_extra_tc_count++;
        }
        return;
    }
    if (completed_buffer > 1U)
    {
        g_last_business_error_code =
            CAMERA_DCMI_BUSINESS_ERROR_INVALID_BUFFER;
        camera_dcmi_error_callback();
        return;
    }
    copy_state = camera_dcmi_copy_state();
    if (copy_state != 0)
    {
        g_last_business_error_code = CAMERA_DCMI_BUSINESS_ERROR_COPY_BUSY;
        camera_dcmi_error_callback();
        return;
    }
    if (camera_dcmi_start_chunk_copy(completed_buffer,
                                     g_camera_chunk_count) != 0)
    {
        g_last_business_error_code = CAMERA_DCMI_BUSINESS_ERROR_COPY_FAILED;
        camera_dcmi_error_callback();
        return;
    }
    g_camera_chunk_count++;
}

void camera_dcmi_error_callback(void)
{
    if (g_camera_status.running != 0U)
    {
        g_error_count++;
        g_dma_error_count++;
        g_camera_events |= CAMERA_DCMI_EVENT_ERROR;
        g_camera_status.running = 0U;
    }
}

#if !defined(CAMERA_DCMI_HOST_TEST)
static void camera_dcmi_dma_chunk_complete(DMA_HandleTypeDef *hdma)
{
    uint8_t completed_buffer;

    if (hdma != &g_camera_dma)
    {
        return;
    }
    completed_buffer = ((DMA1_Stream1->CR & DMA_SxCR_CT) != 0U) ? 0U : 1U;
    camera_dcmi_chunk_callback(completed_buffer);
}

static void camera_dcmi_dma_error(DMA_HandleTypeDef *hdma)
{
    if (hdma != &g_camera_dma)
    {
        return;
    }
    g_last_dcmi_error_code = HAL_DCMI_ERROR_DMA;
    g_last_dma_error_code = hdma->ErrorCode;
    g_last_dcmi_risr = DCMI->RISR;
    g_last_dma_lisr = DMA1->LISR;
    camera_dcmi_error_callback();
}

void DMA1_Stream1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_camera_dma);
}

void DCMI_IRQHandler(void)
{
    HAL_DCMI_IRQHandler(&g_camera_dcmi);
}

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)
{
    if (hdcmi == &g_camera_dcmi)
    {
        camera_dcmi_frame_callback();
    }
}

void HAL_DCMI_ErrorCallback(DCMI_HandleTypeDef *hdcmi)
{
    if (hdcmi == &g_camera_dcmi)
    {
        g_last_dcmi_error_code = hdcmi->ErrorCode;
        g_last_dma_error_code = g_camera_dma.ErrorCode;
        g_last_dcmi_risr = DCMI->RISR;
        g_last_dma_lisr = DMA1->LISR;
        camera_dcmi_error_callback();
    }
}
#endif
