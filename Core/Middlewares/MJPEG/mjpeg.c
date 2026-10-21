/**
 ****************************************************************************************************
 * @file        mjpeg.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2023-01-05
 * @brief       MJPEG��Ƶ���� ����
 * @license     Copyright (c) 2020-2032, ������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� ������ F429������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 *
 * �޸�˵��
 * V1.0 20230105
 * ��һ�η���
 *
 ****************************************************************************************************
 */
 
#include "mjpeg.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sdram_layout.h"
/* Video back buffer in SDRAM (former FONT16 area, now unused — fonts are QSPI memory-mapped) */
#define VIDEO_BACKBUF_ADDR  SDRAM_VIDEO_RGB_A_ADDR
#define VIDEO_OUTBUF0_ADDR  SDRAM_VIDEO_YUV0_ADDR
#define VIDEO_OUTBUF1_ADDR  SDRAM_VIDEO_YUV1_ADDR

extern volatile uint8_t g_avi_checkpoint;
extern volatile uint8_t g_avi_diag_active;
extern volatile uint8_t g_avi_diag_first_frame;
extern void avi_diag_checkpoint(uint8_t checkpoint);

#define AVI_CP_JPEG_START  6U
#define AVI_CP_YUV_READY   7U
#define AVI_CP_DMA2D_DONE  8U






jpeg_codec_typedef mjpeg;               /* JPEGӲ������ṹ�� */
uint16_t imgoffx,imgoffy;               /* ͼ����LCD��Ļ��x,y�����ƫ���� */
uint16_t* rgb565buf;                    /* ������RGBͼ���Դ��ַ */
volatile uint32_t mjpeg_remain_size;    /* MJPEGһ֡ͼ���ʣ���С(�ֽ���) */
volatile uint8_t mjpeg_fileover = 0;    /* MJPEGͼƬ�ļ���ȡ��ɱ�־ */
    
uint16_t *mjpeg_rgb_framebuf;           /* RGB��֡�����ַ */
static uint8_t mjpeg_session_active = 0U;
static uint16_t mjpeg_session_max_width = 0U;
static uint16_t mjpeg_session_max_height = 0U;
static mjpeg_result_t mjpeg_last_error = MJPEG_OK;
static uint8_t mjpeg_legacy_copy = 0U;
static uint32_t mjpeg_decode_call_count = 0U;
static uint8_t mjpeg_trace_second_frame = 0U;

static mjpeg_result_t mjpeg_decode_frame(uint8_t *buf, uint32_t bsize,
                                         uint32_t timeout_ms);

mjpeg_result_t mjpeg_session_open(uint16_t max_width, uint16_t max_height)
{
    uint8_t i;

    if (max_width == 0U || max_height == 0U ||
        max_width > 2048U || max_height > 2048U)
    {
        mjpeg_last_error = MJPEG_ERR_ARGUMENT;
        return mjpeg_last_error;
    }
    if (mjpeg_session_active != 0U)
    {
        mjpeg_last_error = MJPEG_ERR_BUSY;
        return mjpeg_last_error;
    }

    memset(&mjpeg, 0, sizeof(mjpeg));
    if (mjpeg_jpeg_core_init(&mjpeg) != 0U)
    {
        mjpeg_last_error = MJPEG_ERR_HARDWARE;
        return mjpeg_last_error;
    }

    for (i = 0U; i < JPEG_DMA_OUTBUF_NB; i++)
    {
        mjpeg.outbuf[i].buf = (i == 0U) ?
            (uint8_t *)VIDEO_OUTBUF0_ADDR : (uint8_t *)VIDEO_OUTBUF1_ADDR;
    }

    rgb565buf = (uint16_t *)VIDEO_BACKBUF_ADDR;
    imgoffx = 0U;
    imgoffy = 0U;
    mjpeg_session_max_width = max_width;
    mjpeg_session_max_height = max_height;
    mjpeg_legacy_copy = 0U;
    mjpeg_decode_call_count = 0U;
    mjpeg_trace_second_frame = 0U;
    mjpeg_last_error = MJPEG_OK;
    mjpeg_session_active = 1U;
    return MJPEG_OK;
}

mjpeg_result_t mjpeg_session_decode(const uint8_t *data, uint32_t size,
                                    uint32_t rgb_target, uint32_t timeout_ms)
{
    if (mjpeg_session_active == 0U)
    {
        mjpeg_last_error = MJPEG_ERR_NOT_OPEN;
        return mjpeg_last_error;
    }
    if (data == NULL || size == 0U || rgb_target == 0U || timeout_ms == 0U)
    {
        mjpeg_last_error = MJPEG_ERR_ARGUMENT;
        return mjpeg_last_error;
    }

    rgb565buf = (uint16_t *)rgb_target;
    mjpeg_legacy_copy = 0U;
    mjpeg_decode_call_count++;
    mjpeg_trace_second_frame = (mjpeg_decode_call_count == 2U) ? 1U : 0U;
    if (mjpeg_trace_second_frame != 0U)
    {
        printf("[MJPEG-D2-01] tick=%lu size=%lu jpeg_sr=%08lX jpeg_cr=%08lX\r\n",
               (unsigned long)HAL_GetTick(), (unsigned long)size,
               (unsigned long)JPEG->SR, (unsigned long)JPEG->CR);
    }
    return mjpeg_decode_frame((uint8_t *)data, size, timeout_ms);
}

void mjpeg_session_close(void)
{
    if (mjpeg_session_active == 0U) return;

    mjpeg_jpeg_core_destroy(&mjpeg);
    mjpeg_session_active = 0U;
    mjpeg_session_max_width = 0U;
    mjpeg_session_max_height = 0U;
    mjpeg_legacy_copy = 0U;
}

mjpeg_result_t mjpeg_session_get_last_error(void)
{
    return mjpeg_last_error;
}



/**
 * @brief       ��Ƶ����汾��JPEG_Core_Init����
 * @param       tjpeg:jpeg�������ƽṹ��
 * @retval      ����ֵ:0,�ɹ�;����,ʧ��
 */
uint8_t mjpeg_jpeg_core_init(jpeg_codec_typedef *tjpeg)
{  
    RCC->AHB3ENR |= 1 << 5;             /* ʹ��Ӳ��jpegʱ�� */
    RCC->AHB3RSTR |= 1 << 5;            /* ��λӲ��jpeg������ */
    RCC->AHB3RSTR &= ~(1 << 5);         /* ������λ */
    JPEG->CR = 0;                       /* ������ */
    JPEG->CR |= 1 << 0;                 /* ʹ��Ӳ��JPEG */
    JPEG->CONFR0 &= ~(1 << 0);          /* ֹͣJPEG�������� */
    JPEG->CR |= 1 << 13;                /* �������fifo */
    JPEG->CR |= 1 << 14;                /* ������fifo */
    JPEG->CFR = 3 << 5;                 /* ��ձ�־  */
    HAL_NVIC_SetPriority(JPEG_IRQn, 1, 3); HAL_NVIC_EnableIRQ(JPEG_IRQn);      /* ��ռ1�������ȼ�3����2 */
    JPEG->CONFR1 |= 1 << 8;             /* ʹ��header���� */
    return 0;
}
 
/**
 * @brief       ��Ƶ����汾��JPEG_Core_Destroy����
 * @param       tjpeg:jpeg�������ƽṹ��
 * @retval      ��
 */
void mjpeg_jpeg_core_destroy(jpeg_codec_typedef *tjpeg)
{
    (void)tjpeg;
    jpeg_dma_stop();                            /* Stop MDMA transfers */
    HAL_NVIC_DisableIRQ(MDMA_IRQn);             /* Disable MDMA IRQ */
    HAL_NVIC_DisableIRQ(JPEG_IRQn);             /* Disable JPEG IRQ */
    JPEG->CR &= ~(1 << 0);                      /* Disable HW JPEG */
    JPEG->CONFR0 &= ~(1 << 0);                  /* Stop codec */
    JPEG->CFR = 3 << 5;                         /* Clear flags */
    RCC->AHB3RSTR |= 1 << 5;                    /* Reset JPEG peripheral */
    RCC->AHB3RSTR &= ~(1 << 5);                 /* Release reset */
    jpeg_in_callback = NULL;                    /* Clear callbacks */
    jpeg_out_callback = NULL;
    jpeg_eoc_callback = NULL;
    jpeg_hdp_callback = NULL;
}

/**
 * @brief       JPEG����������,�ص�����,���ڻ�ȡJPEG�ļ�ԭʼ����
 * @param       ��
 * @retval      ��
 */
void mjpeg_dma_in_callback(void)
{ 
    if (mjpeg_remain_size)                              /* ����ʣ��������Ҫ���� */
    {
        mjpeg.inbuf[0].buf += JPEG_DMA_INBUF_LEN;       /* ƫ�Ƶ���һ��λ�� */
        
        if (mjpeg_remain_size<JPEG_DMA_INBUF_LEN)       /* ʣ�����ݱȽ���,һ�ξͿ��Դ������ */
        {
            mjpeg.inbuf[0].size = mjpeg_remain_size;    /* �����С����ʣ���ܴ�С */
            mjpeg_remain_size = 0;                      /* һ�δ���Ϳ��Ը��� */
        }
        else                                            /* ͼƬ�Ƚϴ�,��Ҫ�ֶ�δ��� */
        {
            mjpeg.inbuf[0].size = JPEG_DMA_INBUF_LEN;   /* ������䳤��,�����δ��� */
            mjpeg_remain_size -= JPEG_DMA_INBUF_LEN;    /* ʣ�೤�ȵݼ� */
        }
        
        jpeg_in_dma_resume((uint32_t)mjpeg.inbuf[0].buf,mjpeg.inbuf[0].size);   /* ������һ��DMA���� */
        
    }
    else mjpeg_fileover = 1;                            /* �ļ���ȡ���  */
}

/**
 * @brief       JPEG���������(YCBCR)�ص�����,�������YCbCr������
 * @param       ��
 * @retval      ��
 */
void mjpeg_dma_out_callback(void)
{
    uint32_t *pdata = 0; 
    if (g_avi_diag_active != 0U && g_avi_diag_first_frame != 0U &&
        g_avi_checkpoint < AVI_CP_YUV_READY)
    {
        g_avi_checkpoint = AVI_CP_YUV_READY;
    }
    mjpeg.outbuf[mjpeg.outbuf_write_ptr].sta = 1;       /* ��buf���� */
    mjpeg.outbuf[mjpeg.outbuf_write_ptr].size = mjpeg.yuvblk_size - (MDMA_Channel6->CBNDTR & 0X1FFFF);  /* ��buf�������ݵĳ��� */
    
    if (mjpeg.state == JPEG_STATE_FINISHED)                /* ����ļ��Ѿ��������,��Ҫ��ȡDOR��������(<=32�ֽ�) */
    {
        pdata = (uint32_t*)(mjpeg.outbuf[mjpeg.outbuf_write_ptr].buf + mjpeg.outbuf[mjpeg.outbuf_write_ptr].size);
        
        while (JPEG->SR & (1 << 4))
        {
            *pdata = JPEG->DOR;
            pdata++;
            mjpeg.outbuf[mjpeg.outbuf_write_ptr].size += 4; 
        }
    }
    
    mjpeg.outbuf_write_ptr ++;                  /* ָ����һ��buf */
    
    if (mjpeg.outbuf_write_ptr >= JPEG_DMA_OUTBUF_NB) mjpeg.outbuf_write_ptr = 0;        /* ���� */
    
    if (mjpeg.outbuf[mjpeg.outbuf_write_ptr].sta == 1)  /* ����Чbuf */
    { 
        mjpeg.outdma_pause = 1;                         /* �����ͣ */
    }
    else                                                /* ��Ч��buf */
    {
        jpeg_out_dma_resume((uint32_t)mjpeg.outbuf[mjpeg.outbuf_write_ptr].buf,mjpeg.yuvblk_size);  /* ������һ��DMA���� */
    }
}

static uint8_t mjpeg_tail_output_pending(void)
{
    uint32_t remaining;

    if ((mjpeg.yuvblk_size == 0U) ||
        (mjpeg.outbuf[mjpeg.outbuf_write_ptr].sta != 0U))
    {
        return 0U;
    }

    remaining = MDMA_Channel6->CBNDTR & 0X1FFFFU;
    return ((remaining < mjpeg.yuvblk_size) ||
            ((JPEG->SR & (1U << 4)) != 0U)) ? 1U : 0U;
}

/**
 * @brief       JPEG�����ļ�������ɻص�����
 * @param       ��
 * @retval      ��
 */
void mjpeg_endofcovert_callback(void)
{ 
    mjpeg.state = JPEG_STATE_FINISHED;                  /* ���JPEG������� */
    if (mjpeg_tail_output_pending() != 0U)
    {
        mjpeg_dma_out_callback();
    }
}

/**
 * @brief       JPEG header�����ɹ��ص�����
 * @param       ��
 * @retval      ��
 */
void mjpeg_hdrover_callback(void)
{ 
    mjpeg.state = JPEG_STATE_HEADEROK;                  /* HEADER��ȡ�ɹ� */
    jpeg_get_info(&mjpeg);                              /* ��ȡJPEG�����Ϣ,������С,ɫ�ʿռ�,������ */
    if (mjpeg.Conf.ImageWidth == 0U || mjpeg.Conf.ImageHeight == 0U ||
        mjpeg.Conf.ImageWidth > mjpeg_session_max_width ||
        mjpeg.Conf.ImageHeight > mjpeg_session_max_height)
    {
        mjpeg_last_error = MJPEG_ERR_DIMENSION;
        mjpeg.state = JPEG_STATE_ERROR;
        return;
    }
    /* ��Ҫ��ȡJPEG������Ϣ�Ժ�,���ܸ���jpeg�����С�Ͳ�����ʽ,��������������С,��������MDMA */
    switch(mjpeg.Conf.ChromaSubsampling)
    {
        case JPEG_420_SUBSAMPLING: 
            mjpeg.yuvblk_size = 24 * mjpeg.Conf.ImageWidth; /* YUV420,ÿ��YUV����ռ1.5���ֽ�.ÿ�����16��.16*1.5=24 */
            mjpeg.yuvblk_height = 16;                       /* ÿ�����16�� */
            break;
        case JPEG_422_SUBSAMPLING:
            mjpeg.yuvblk_size = 16 * mjpeg.Conf.ImageWidth; /* YUV420,ÿ��YUV����ռ2���ֽ�.ÿ�����8��.8*2=16 */
            mjpeg.yuvblk_height = 8;                        /* ÿ�����8�� */
            break;
        case JPEG_444_SUBSAMPLING:
            mjpeg.yuvblk_size = 24 * mjpeg.Conf.ImageWidth; /* YUV420,ÿ��YUV����ռ3���ֽ�.ÿ�����8��.8*3=24 */
            mjpeg.yuvblk_height = 8;                        /* ÿ�����8�� */
            break;
    }
    
    mjpeg.yuvblk_curheight = 0;                             /* ��ǰ�м��������� */
    
    if (mjpeg.outbuf[1].buf != NULL)                        /* ����buf������OK */
    {
        jpeg_out_dma_init((uint32_t)mjpeg.outbuf[0].buf,mjpeg.yuvblk_size);	/* �������DMA */
        jpeg_out_dma_start();                               /* ���DMA OUT����,��ʼ����JPEG����������  */
    } 
} 

/**
 * @brief       ��ʼ��MJPEG
 * @param       offx,offy:��Ƶ��LCD����,X,Y�����ƫ����
 * @param       width,height:ͼ��Ŀ�Ⱥ͸߶�
 * @retval      ����ֵ:0,�ɹ�;����,ʧ��
 */
uint8_t mjpeg_init(uint16_t offx,uint16_t offy,uint32_t width,uint32_t height)
{
    mjpeg_result_t result;
    if (width > 0XFFFFU || height > 0XFFFFU) return 1U;
    if (mjpeg_session_active == 0U)
    {
        result = mjpeg_session_open((uint16_t)width, (uint16_t)height);
        if (result != MJPEG_OK) return 1U;
    }
    else if (width > mjpeg_session_max_width || height > mjpeg_session_max_height)
    {
        return 1U;
    }
    imgoffx = offx;
    imgoffy = offy;
    mjpeg_rgb_framebuf = (uint16_t *)0xC0800000UL;   /* fill_color output, DMA2D source → LCD */
    mjpeg_legacy_copy = 1U;
    return 0U;
}

/**
 * @brief       �ͷ��ڴ�
 * @param       ��
 * @retval      ��
 */
void mjpeg_free(void)
{ 
    mjpeg_session_close();
} 

/**
 * @brief       ����RGB��,���Ǻ���ģʽ,���������DMA2D,ֱ�ӽ�JPEG�����YUV����ת����RGB����,����䵽��Ӧ��GRAM����,���в���ȫӲ�����,�ٶȷǳ���.
 * @param       sx,sy:��ʼ����
 * @param       tjpeg:jpeg����ṹ�� 
 * @retval      ��
 */
void mjpeg_ltdc_dma2d_yuv2rgb_fill(uint16_t sx,uint16_t sy,jpeg_codec_typedef *tjpeg)
{
    uint32_t timeout = 0; 
    uint16_t offline;
    uint32_t addr; 
    uint32_t regval = 0;
    uint32_t cm = 0;                                                                    /* ������ʽ */
    
    if (tjpeg->Conf.ChromaSubsampling == JPEG_420_SUBSAMPLING) cm = DMA2D_CSS_420;      /* YUV420תRGB */
    
    if (tjpeg->Conf.ChromaSubsampling == JPEG_422_SUBSAMPLING) cm = DMA2D_CSS_422;      /* YUV422תRGB */
    else if (tjpeg->Conf.ChromaSubsampling == JPEG_444_SUBSAMPLING) cm = DMA2D_NO_CSS;  /* YUV444תRGB */
    
    offline = 2  /* RGB565=2 bytes/pixel */ - tjpeg->Conf.ImageWidth;
    addr = ((uint32_t)((uint32_t *)VIDEO_BACKBUF_ADDR)+2  /* pixsize */*(2  /* RGB565=2 bytes/pixel */*sy+sx)); 
    RCC->AHB3ENR |= 1 << 4;             /* ʹ��DMA2Dʱ�� */
    RCC->AHB3RSTR |= 1 << 4;            /* ��λDMA2D */
    RCC->AHB3RSTR &= ~(1 << 4);         /* ������λ */
    DMA2D->CR &= ~(1 << 0);             /* ��ֹͣDMA2D */
    DMA2D->CR = 1 << 16;                /* MODE[1:0]=01,�洢�����洢��,��PFCģʽ */
    DMA2D->OPFCCR = 2 << 0;             /* CM[2:0]=010,���ΪRGB565��ʽ */
    DMA2D->OOR = offline;               /* ������ƫ��  */
    DMA2D->IFCR |= 1 << 1;              /* ���������ɱ�־ */
    regval = 11 << 0;                   /* CM[3:0]=1011,��������ΪYCbCr��ʽ */
    regval |= cm << 18;                 /* CSS[1:0]=cm,Chroma Sub-Sampling:0,4:4:4;1,4:2:2;2,4:2:0  */
    DMA2D->FGPFCCR = regval;            /* ����FGPCCR�Ĵ��� */
    DMA2D->FGOR = 0;                    /* ǰ������ƫ��Ϊ0 */
    DMA2D->NLR = tjpeg->yuvblk_height | (tjpeg->Conf.ImageWidth << 16); /* �趨�����Ĵ���  */
    DMA2D->OMAR = addr;                 /* ����洢����ַ  */
    DMA2D->FGMAR = (uint32_t)tjpeg->outbuf[tjpeg->outbuf_read_ptr].buf; /* Դ��ַ */
    DMA2D->CR |= 1 << 0;                /* ���DMA2D */
    
    while ((DMA2D->ISR & (1 << 1)) == 0)    /* �ȴ�������� */
    {
        timeout++;
        
        if (timeout > 0X1FFFFFF) break;     /* ��ʱ�˳� */
    }
    
    tjpeg->yuvblk_curheight += tjpeg->yuvblk_height;    /* ƫ�Ƶ���һ���ڴ��ַ  */
    /* YUV2RGBת�������,�ٸ�λһ��DMA2D */
    RCC->AHB3RSTR |= 1 << 4;                /* ��λDMA2D */
    RCC->AHB3RSTR &= ~(1 << 4);             /* ������λ */
} 

/**
 * @brief       �����ɫ
 * @param       x,y:��ʼ����
 * @param       width��height����Ⱥ͸߶�
 * @param       *color����ɫ����
 * @retval      ��
 */
void mjpeg_fill_color(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t *color)
{  
    uint16_t i,j;
    uint32_t param1;
    uint32_t param2;
    uint32_t param3;
    uint16_t* pdata;
    
    if (2  /* RGB565=2 bytes/pixel */ != 0 && 0  /* no rotation */ == 0) /* �����RGB��,������,����亯������ֱ���� */
    { 
        param1 = 2  /* pixsize */ * 2  /* RGB565=2 bytes/pixel */ * (600  /* panel height */ - x - 1) + 2  /* pixsize */ * y;    /* ������������,����ٶ� */
        param2 = 2  /* pixsize */ * 2  /* RGB565=2 bytes/pixel */;
        
        for (i = 0;i < height;i ++)
        {
            param3 = i * 2  /* pixsize */ + param1;
            pdata = color + i * width;
            
            for (j = 0;j < width;j++)
            { 
                *(uint16_t*)((uint32_t)mjpeg_rgb_framebuf + param3 - param2 * j) = pdata[j];  
            }
        }
    }
    else if (2  /* RGB565=2 bytes/pixel */ == 0) /* lcd_color_fill not available */;   /* ��MCU��(RGB�����������!!,��YUVת����ʱ��,ֱ�Ӿ������) */
}

/**
 * @brief       ����һ��JPEGͼƬ
                1,������ͼƬ�ķֱ���,����С�ڵ�����Ļ�ķֱ���!
                2,�뱣֤ͼƬ�Ŀ����16�ı���,���������ֻ���.
 * @param       buf:jpeg����������
 * @param       bsize:�����С
 * @retval      ����ֵ:0,�ɹ�������,����
 */
static mjpeg_result_t mjpeg_decode_frame(uint8_t *buf, uint32_t bsize,
                                         uint32_t timeout_ms)
{ 
    uint32_t last_progress_tick;
    
    if (buf == NULL || bsize == 0U || timeout_ms == 0U)
    {
        mjpeg_last_error = MJPEG_ERR_ARGUMENT;
        return mjpeg_last_error;
    }
    
    mjpeg_last_error = MJPEG_OK;
    jpeg_decode_init(&mjpeg);                       /* ��ʼ��Ӳ��JPEG������ */
    if (mjpeg_trace_second_frame != 0U)
    {
        printf("[MJPEG-D2-02] tick=%lu jpeg_sr=%08lX jpeg_cr=%08lX ch6=%08lX ch7=%08lX\r\n",
               (unsigned long)HAL_GetTick(),
               (unsigned long)JPEG->SR, (unsigned long)JPEG->CR,
               (unsigned long)MDMA_Channel6->CCR,
               (unsigned long)MDMA_Channel7->CCR);
    }
    mjpeg_remain_size = bsize;                      /* ��¼��ǰͼƬ�Ĵ�С(�ֽ���) */
    mjpeg.inbuf[0].buf = buf;                       /* ָ��jpeg���������׵�ַ   */
    mjpeg_fileover = 0;                             /* ���δ���� */
    
    if(mjpeg_remain_size < JPEG_DMA_INBUF_LEN)      /* ͼƬ�Ƚ�С,һ�ξͿ��Դ������ */
    {
        /* mjpeg.inbuf[0].size = mjpeg_remain_size;  */     /* �����С�����ܴ�С */
        /* mjpeg_remain_size = 0;  */                       /* һ�δ���Ϳ��Ը��� */
        mjpeg.inbuf[0].size = mjpeg_remain_size;       /* 缓冲大小等于总大小 */
        mjpeg_remain_size = 0;                         /* 一次处理就可以覆盖 */
    }
    else                                                /* ͼƬ�Ƚϴ�,��Ҫ�ֶ�δ��� */
    {
        mjpeg.inbuf[0].size = JPEG_DMA_INBUF_LEN;       /* ������䳤��,�����δ��� */
        mjpeg_remain_size -= JPEG_DMA_INBUF_LEN;        /* ʣ�೤�� */
    }
    
    jpeg_in_dma_init((uint32_t)mjpeg.inbuf[0].buf,mjpeg.inbuf[0].size); /* ��������DMA  */
    if (mjpeg_trace_second_frame != 0U)
    {
        printf("[MJPEG-D2-03] tick=%lu in=%lu remain=%lu ch7_cbndtr=%08lX\r\n",
               (unsigned long)HAL_GetTick(),
               (unsigned long)mjpeg.inbuf[0].size,
               (unsigned long)mjpeg_remain_size,
               (unsigned long)MDMA_Channel7->CBNDTR);
    }
    jpeg_in_callback = mjpeg_dma_in_callback;           /* JPEG DMA��ȡ���ݻص����� */
    jpeg_out_callback = mjpeg_dma_out_callback;         /* JPEG DMA������ݻص����� */
    jpeg_eoc_callback = mjpeg_endofcovert_callback;     /* JPEG ��������ص����� */
    jpeg_hdp_callback = mjpeg_hdrover_callback;         /* JPEG Header������ɻص����� */
    avi_diag_checkpoint(AVI_CP_JPEG_START);
    jpeg_in_dma_start();                                /* ���DMA IN����,��ʼ����JPEGͼƬ */
    if (mjpeg_trace_second_frame != 0U)
    {
        printf("[MJPEG-D2-04] tick=%lu jpeg_sr=%08lX ch7=%08lX\r\n",
               (unsigned long)HAL_GetTick(),
               (unsigned long)JPEG->SR,
               (unsigned long)MDMA_Channel7->CCR);
    }
    last_progress_tick = HAL_GetTick();
    
    while(1)
    { 
        if (mjpeg.outbuf[mjpeg.outbuf_read_ptr].sta == 1)   /* buf����������Ҫ���� */
        {
            last_progress_tick = HAL_GetTick();
            if (mjpeg_trace_second_frame == 1U)
            {
                printf("[MJPEG-D2-05] tick=%lu state=%u y=%u/%u out=%lu\r\n",
                       (unsigned long)HAL_GetTick(),
                       (unsigned int)mjpeg.state,
                       (unsigned int)mjpeg.yuvblk_curheight,
                       (unsigned int)mjpeg.Conf.ImageHeight,
                       (unsigned long)mjpeg.outbuf[mjpeg.outbuf_read_ptr].size);
                mjpeg_trace_second_frame = 2U;
            }
            /* SCB_CleanInvalidateDCache(); */     /* ���D catch */
            /* Invalidate outbuf D-Cache: ensure MDMA writes visible to DMA2D */
            SCB_InvalidateDCache_by_Addr(
                (uint32_t *)((uint32_t)mjpeg.outbuf[mjpeg.outbuf_read_ptr].buf & ~0x1FUL),
                ((mjpeg.yuvblk_size + 31) & ~0x1FUL));
            __DSB();
            if (2  /* RGB565=2 bytes/pixel */ == 0 || 0  /* no rotation */ == 0)    /* ��RGB����,��Ҫ�Ƚ�YUV���ݽ��뵽rgb565buf,Ȼ���ٴ�rgb565buf������LCD��GRAM */
            {
                uint8_t dma2d_res;
                if (mjpeg_trace_second_frame == 2U)
                {
                    printf("[MJPEG-D2-06] tick=%lu dma2d_isr=%08lX\r\n",
                           (unsigned long)HAL_GetTick(),
                           (unsigned long)DMA2D->ISR);
                }
                dma2d_res = jpeg_dma2d_yuv2rgb_conversion(&mjpeg,(uint32_t*)rgb565buf); /* ����DMA2D,��YUVͼ��ת��RGB565ͼ��  */
                if (mjpeg_trace_second_frame == 2U)
                {
                    printf("[MJPEG-D2-07] tick=%lu res=%u dma2d_isr=%08lX\r\n",
                           (unsigned long)HAL_GetTick(),
                           (unsigned int)dma2d_res,
                           (unsigned long)DMA2D->ISR);
                    mjpeg_trace_second_frame = 0U;
                }
                if (dma2d_res != 0U)
                {
                    uint32_t failed_y = (mjpeg.yuvblk_curheight >= mjpeg.yuvblk_height) ?
                        (mjpeg.yuvblk_curheight - mjpeg.yuvblk_height) : 0U;
                    mjpeg_last_error = MJPEG_ERR_DMA2D_TIMEOUT;
                    mjpeg.state = JPEG_STATE_ERROR;
                    printf("[MJPEG-DMA2D] timeout tick=%lu call=%lu y=%lu/%u checkpoint=%u "
                           "jpeg_sr=%08lX mdma_gisr=%08lX isr=%08lX cr=%08lX nlr=%08lX "
                           "omar=%08lX fgmar=%08lX oor=%08lX fgpfc=%08lX\r\n",
                           (unsigned long)HAL_GetTick(),
                           (unsigned long)mjpeg_decode_call_count,
                           (unsigned long)failed_y,
                           (unsigned int)mjpeg.Conf.ImageHeight,
                           (unsigned int)g_avi_checkpoint,
                           (unsigned long)JPEG->SR,
                           (unsigned long)MDMA->GISR0,
                           (unsigned long)DMA2D->ISR,
                           (unsigned long)DMA2D->CR,
                           (unsigned long)DMA2D->NLR,
                           (unsigned long)DMA2D->OMAR,
                           (unsigned long)DMA2D->FGMAR,
                           (unsigned long)DMA2D->OOR,
                           (unsigned long)DMA2D->FGPFCCR);
                    break;
                }
                if (g_avi_diag_active != 0U && g_avi_diag_first_frame != 0U &&
                    g_avi_checkpoint < AVI_CP_DMA2D_DONE)
                {
                    g_avi_checkpoint = AVI_CP_DMA2D_DONE;
                }
            }
            else                                            /* RGB����,ֱ�ӽ�YUV���ݽ��뵽LCD��GRAM,ʡȥ�˿�������,���Դﵽ������� */
            {
                mjpeg_ltdc_dma2d_yuv2rgb_fill(imgoffx,imgoffy + mjpeg.yuvblk_curheight,&mjpeg); /* DMA2D��YUV���뵽LCD GRAM,�ٶ���� */
            }
            
            /* SCB_CleanInvalidateDCache(); */              /* ���D catch */
            /* Ensure DMA2D reads complete before freeing outbuf (prevent read-write conflict) */
            __DSB(); { volatile uint32_t *__ob = (volatile uint32_t *)((uint32_t)mjpeg.outbuf[mjpeg.outbuf_read_ptr].buf & ~0x3UL); (void)__ob[0]; }
            mjpeg.outbuf[mjpeg.outbuf_read_ptr].sta = 0;    /* ���bufΪ�� */
            mjpeg.outbuf[mjpeg.outbuf_read_ptr].size = 0;   /* ��������� */
            mjpeg.outbuf_read_ptr++;
            
            if (mjpeg.outbuf_read_ptr >= JPEG_DMA_OUTBUF_NB) mjpeg.outbuf_read_ptr = 0; /* ���Ʒ�Χ */
            if (mjpeg.yuvblk_curheight >= mjpeg.Conf.ImageHeight) break;                /* ��ǰ�߶ȵ��ڻ��߳���ͼƬ�ֱ��ʵĸ߶�,��˵�����������,ֱ���˳�. */
        }
        else if (mjpeg.outdma_pause == 1 && mjpeg.outbuf[mjpeg.outbuf_write_ptr].sta == 0)  /* out��ͣ,�ҵ�ǰwritebuf�Ѿ�Ϊ����,��ָ�out��� */
        {
            jpeg_out_dma_resume((uint32_t)mjpeg.outbuf[mjpeg.outbuf_write_ptr].buf,mjpeg.yuvblk_size);  /* ������һ��DMA���� */
            mjpeg.outdma_pause = 0;
        }
        
        if (mjpeg.state == JPEG_STATE_ERROR)        /* �������,ֱ���˳� */
        { 
            break;
        }
        
        if (mjpeg.state == JPEG_STATE_FINISHED)     /* 解码完成,检查是否异常结束 */
        { 
            if (mjpeg.yuvblk_curheight < mjpeg.Conf.ImageHeight)
            {
                if (mjpeg.Conf.ImageHeight > (mjpeg.yuvblk_curheight + 16)) /* 异常提前结束,直接退出 */
                {
                    mjpeg.state = JPEG_STATE_ERROR;   /* 标记错误 */
/*                   printf("early finished!\r\n"); */
                    break;
                } 
            } 
        }
        
        if (mjpeg_fileover) /* �ļ�������,��ʱ�˳�,��ֹ��ѭ�� */
        {  
            if (mjpeg.state == JPEG_STATE_NOHEADER)
            {
                mjpeg_last_error = MJPEG_ERR_HEADER_TIMEOUT;
                break;
            }
        }

        if ((HAL_GetTick() - last_progress_tick) > timeout_ms)
        {
            mjpeg_last_error = (mjpeg.state == JPEG_STATE_NOHEADER) ?
                MJPEG_ERR_HEADER_TIMEOUT : MJPEG_ERR_DECODE_TIMEOUT;
            printf("[JPEG-FIRST] decode timeout checkpoint=%u state=%d yuvblk_h=%d/%d elapsed=%lu jpeg_sr=%08lX mdma_gisr=%08lX ch6_ccr=%08lX ch7_ccr=%08lX dma2d_isr=%08lX\r\n",
                   (unsigned int)g_avi_checkpoint,
                   (int)mjpeg.state,
                   (int)mjpeg.yuvblk_curheight,
                   (int)mjpeg.Conf.ImageHeight,
                   (unsigned long)(HAL_GetTick() - last_progress_tick),
                   (unsigned long)JPEG->SR,
                   (unsigned long)MDMA->GISR0,
                   (unsigned long)MDMA_Channel6->CCR,
                   (unsigned long)MDMA_Channel7->CCR,
                   (unsigned long)DMA2D->ISR);
            break;
        }   
    } 
    
    if ((mjpeg.state == JPEG_STATE_FINISHED) &&
        (mjpeg.yuvblk_curheight < mjpeg.Conf.ImageHeight))
    {
        /* EOC may arrive before the final YUV block has been drained. */
        mjpeg_last_error = MJPEG_ERR_DECODE_TIMEOUT;
        mjpeg.state = JPEG_STATE_ERROR;
    }

    if (mjpeg.Conf.ImageWidth != 0U && mjpeg.Conf.ImageHeight != 0U)
    {
        SCB_InvalidateDCache_by_Addr(
            (uint32_t *)((uint32_t)rgb565buf & ~0x1FUL),
            (mjpeg.Conf.ImageWidth * mjpeg.Conf.ImageHeight * 2U + 31U) & ~0x1FUL);
    }
    if (mjpeg.state == JPEG_STATE_FINISHED)
    {
        if (mjpeg_legacy_copy != 0U)
        {
            mjpeg_fill_color(imgoffx, imgoffy, mjpeg.Conf.ImageWidth,
                             mjpeg.Conf.ImageHeight, rgb565buf);
        }
        if (mjpeg_decode_call_count == 2U)
        {
            printf("[MJPEG-D2-08] tick=%lu y=%u/%u\r\n",
                   (unsigned long)HAL_GetTick(),
                   (unsigned int)mjpeg.yuvblk_curheight,
                   (unsigned int)mjpeg.Conf.ImageHeight);
        }
        mjpeg_last_error = MJPEG_OK;
        return MJPEG_OK;
    }

    if (mjpeg_last_error == MJPEG_OK) mjpeg_last_error = MJPEG_ERR_HARDWARE;
    return mjpeg_last_error;
}

uint8_t mjpeg_decode(uint8_t *buf, uint32_t bsize)
{
    mjpeg_legacy_copy = 1U;
    return (mjpeg_decode_frame(buf, bsize, 1000U) == MJPEG_OK) ? 0U : 1U;
}
