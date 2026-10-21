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
/* Video back buffer in SDRAM (former FONT16 area, now unused — fonts are QSPI memory-mapped) */
#define VIDEO_BACKBUF_ADDR  0xC0580000UL  /* after LVGL 512KB pool */
#define VIDEO_OUTBUF0_ADDR  0xC07D0000UL  /* 64KB YUV */
#define VIDEO_OUTBUF1_ADDR  0xC07E0000UL  /* 64KB YUV */







jpeg_codec_typedef mjpeg;               /* JPEGӲ������ṹ�� */
uint16_t imgoffx,imgoffy;               /* ͼ����LCD��Ļ��x,y�����ƫ���� */
uint16_t* rgb565buf;                    /* ������RGBͼ���Դ��ַ */
volatile uint32_t mjpeg_remain_size;    /* MJPEGһ֡ͼ���ʣ���С(�ֽ���) */
volatile uint8_t mjpeg_fileover = 0;    /* MJPEGͼƬ�ļ���ȡ��ɱ�־ */
    
uint16_t *mjpeg_rgb_framebuf;           /* RGB��֡�����ַ */



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
    uint8_t i; 
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
    HAL_Delay(500);                              /* Wait AHB3 settle */
    
    for(i = 0;i < JPEG_DMA_OUTBUF_NB;i ++) 
    {        /* outbufs use fixed SDRAM, no free needed */
    }
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

/**
 * @brief       JPEG�����ļ�������ɻص�����
 * @param       ��
 * @retval      ��
 */
void mjpeg_endofcovert_callback(void)
{ 
    mjpeg.state = JPEG_STATE_FINISHED;                  /* ���JPEG������� */
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
    uint8_t i;
    uint8_t res;
    res = mjpeg_jpeg_core_init(&mjpeg);         /* ��ʼ��JPEG�ں�,������IN BUF */
    if (res) return 1;
    
    for (i = 0;i < JPEG_DMA_OUTBUF_NB;i++)
    {
        
        /* 使用 SDRAM 固定地址，避免 malloc 在 16KB 堆上失败 */
        if (i == 0)
            mjpeg.outbuf[i].buf = (uint8_t *)VIDEO_OUTBUF0_ADDR;
        else
            mjpeg.outbuf[i].buf = (uint8_t *)VIDEO_OUTBUF1_ADDR; 
        
        if(mjpeg.outbuf[i].buf == NULL) return 2; 
    }
    
    rgb565buf = 0;                                              /* RGB565 BUFָ������ */
    
    if (2  /* RGB565=2 bytes/pixel */ == 0 || 0  /* no rotation */ == 0)                 /* �������RGB����,����Ҫrgb565buf���� */
    {
        
        /* 使用 SDRAM 视频后备缓冲区，避免 malloc 失败 */
        rgb565buf = (uint16_t *)VIDEO_BACKBUF_ADDR;
        
        /* rgb565buf uses fixed SDRAM, always valid */
    }
    
    imgoffx = offx;
    imgoffy = offy;
    mjpeg_rgb_framebuf = (uint16_t *)0xC0800000UL;   /* fill_color output, DMA2D source → LCD */
    
    return 0; 
}

/**
 * @brief       �ͷ��ڴ�
 * @param       ��
 * @retval      ��
 */
void mjpeg_free(void)
{ 
    mjpeg_jpeg_core_destroy(&mjpeg); 
    /* rgb565buf uses fixed SDRAM, no free needed */
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
uint8_t mjpeg_decode(uint8_t* buf,uint32_t bsize) 
{ 
    volatile uint32_t timecnt = 0;
    
    if (bsize == 0) return 0;  
    
    jpeg_decode_init(&mjpeg);                       /* ��ʼ��Ӳ��JPEG������ */
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
    jpeg_in_callback = mjpeg_dma_in_callback;           /* JPEG DMA��ȡ���ݻص����� */
    jpeg_out_callback = mjpeg_dma_out_callback;         /* JPEG DMA������ݻص����� */
    jpeg_eoc_callback = mjpeg_endofcovert_callback;     /* JPEG ��������ص����� */
    jpeg_hdp_callback = mjpeg_hdrover_callback;         /* JPEG Header������ɻص����� */
    jpeg_in_dma_start();                                /* ���DMA IN����,��ʼ����JPEGͼƬ */
    
    while(1)
    { 
        if (mjpeg.outbuf[mjpeg.outbuf_read_ptr].sta == 1)   /* buf����������Ҫ���� */
        {
            timecnt = 0;          /* ��ʱ������ */
            /* SCB_CleanInvalidateDCache(); */     /* ���D catch */
            
            if (2  /* RGB565=2 bytes/pixel */ == 0 || 0  /* no rotation */ == 0)    /* ��RGB����,��Ҫ�Ƚ�YUV���ݽ��뵽rgb565buf,Ȼ���ٴ�rgb565buf������LCD��GRAM */
            {
                jpeg_dma2d_yuv2rgb_conversion(&mjpeg,(uint32_t*)rgb565buf); /* ����DMA2D,��YUVͼ��ת��RGB565ͼ��  */
            }
            else                                            /* RGB����,ֱ�ӽ�YUV���ݽ��뵽LCD��GRAM,ʡȥ�˿�������,���Դﵽ������� */
            {
                mjpeg_ltdc_dma2d_yuv2rgb_fill(imgoffx,imgoffy + mjpeg.yuvblk_curheight,&mjpeg); /* DMA2D��YUV���뵽LCD GRAM,�ٶ���� */
            }
            
            /* SCB_CleanInvalidateDCache(); */              /* ���D catch */
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
        
        if (mjpeg.state == JPEG_STATE_FINISHED)     /* ���������,����Ƿ��쳣���� */
        { 
            if (mjpeg.yuvblk_curheight < mjpeg.Conf.ImageHeight)
            {
                if (mjpeg.Conf.ImageHeight > (mjpeg.yuvblk_curheight + 16)) /* �����쳣,ֱ���˳� */
                {
                    mjpeg.state = JPEG_STATE_ERROR;   /* ��Ǵ��� */
/*                   printf("early finished!\r\n"); */
                    break;
                } 
                break;
            } 
        }
        
        if (mjpeg_fileover) /* �ļ�������,��ʱ�˳�,��ֹ��ѭ�� */
        {  
            if (mjpeg.state == JPEG_STATE_NOHEADER) break;  /* ����JPEGͷʧ���� */
        }
        
        timecnt++; 
        
        if (timecnt > 0X3FFFF)  /* ��ʱ�˳� */
        {
            printf("[JPEG] timeout: state=%d yuvblk_h=%d/%d timecnt=%lu\r\n", (int)mjpeg.state, (int)mjpeg.yuvblk_curheight, (int)mjpeg.Conf.ImageHeight, timecnt);
            break;
        }   
    } 
    
    if(mjpeg.state == JPEG_STATE_FINISHED)  /* ��������� */
    {
        mjpeg_fill_color(imgoffx,imgoffy,mjpeg.Conf.ImageWidth,mjpeg.Conf.ImageHeight,rgb565buf);  
    } 
    return 0;
}
