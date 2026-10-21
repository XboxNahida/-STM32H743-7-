/**
 ****************************************************************************************************
 * @file        jpegcodec.c
 * @author      姝ｇ偣鍘熷瓙鍥㈤槦(ALIENTEK)
 * @version     V1.0
 * @date        2023-03-24
 * @brief       JPEG纭欢缂栬В鐮佸櫒 椹卞姩浠ｇ爜
 * @license     Copyright (c) 2022-2032, 骞垮窞甯傛槦缈肩數瀛愮鎶€鏈夐檺鍏徃
 ****************************************************************************************************
 * @attention
 *
 * 瀹為獙骞冲彴:姝ｇ偣鍘熷瓙 STM32H743寮€鍙戞澘
 * 鍦ㄧ嚎瑙嗛:www.yuanzige.com
 * 鎶€鏈鍧?www.openedv.com
 * 鍏徃缃戝潃:www.alientek.com
 * 璐拱鍦板潃:openedv.taobao.com
 *
 * 淇敼璇存槑
 * V1.0 20220324
 * 绗竴娆″彂甯?
 *
 ****************************************************************************************************
 */

#include "./MALLOC/malloc.h"
#include "./SYSTEM/usart/usart.h"
#include "./BSP/JPEGCODEC/jpegcodec.h"
#include "os.h"


/* JPEG瑙勮寖(ISO/IEC 10918-1鏍囧噯)鐨勬牱鏈噺鍖栬〃
 * 鑾峰彇JPEG鍥剧墖璐ㄩ噺鏃堕渶瑕佺敤鍒?
 */
const uint8_t JPEG_LUM_QuantTable[JPEG_QUANT_TABLE_SIZE] =
{
    16, 11, 10, 16, 24, 40, 51, 61, 12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56, 14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77, 24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98, 112, 100, 103, 99
};

const uint8_t JPEG_ZIGZAG_ORDER[JPEG_QUANT_TABLE_SIZE] =
{
    0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

/**
 * @brief       JPEG纭欢瑙ｇ爜杈撳叆MDMA閰嶇疆
 * @param       meminaddr   : JPEG杈撳叆MDMA瀛樺偍鍣ㄥ湴鍧€
 * @param       meminsize   : 杈撳叆MDMA鏁版嵁闀垮害,0~262143,浠ュ瓧鑺備负鍗曚綅
 * @retval      鏃?
 */
void jpeg_in_dma_init(uint32_t meminaddr, uint32_t meminsize)
{
    uint32_t regval = 0;
    uint32_t addrmask = 0;
    RCC->AHB3ENR |= 1 << 0;             /* 浣胯兘MDMA鏃堕挓 */
    MDMA_Channel7->CCR = 0;             /* 杈撳叆MDMA娓呴浂 */

    while (MDMA_Channel7->CCR & 0X01);  /* 绛夊緟MDMA_Channel7鍏抽棴瀹屾垚 */

    MDMA_Channel7->CIFCR = 0X1F;        /* 涓柇鏍囧織娓呴浂 */
    MDMA_Channel7->CCR |= 1 << 2;       /* CTCIE=1,浣胯兘閫氶亾浼犺緭瀹屾垚涓柇 */
    MDMA_Channel7->CCR |= 2 << 6;       /* PL[1:0]=2,楂樹紭鍏堢骇 */
    MDMA_Channel7->CBNDTR = meminsize;  /* 浼犺緭闀垮害涓簃eminsize */
    MDMA_Channel7->CDAR = (uint32_t)&JPEG->DIR; /* 鐩爣鍦板潃涓?JPEG->DIR */
    MDMA_Channel7->CSAR = meminaddr;    /* meminaddr浣滀负婧愬湴鍧€ */
    regval = 0 << 28;                   /* TRGM[1:0]=0,姣忎釜MDMA璇锋眰瑙﹀彂涓€娆uffer浼犺緭 */
    regval |= 1 << 25;                  /* PKE=1,鎵撳寘浣胯兘 */
    regval |= (32 - 1) << 18;           /* TLEN[6:0]=31,buffer浼犺緭闀垮害涓?2瀛楄妭 */
    regval |= 4 << 15;                  /* DBURST[2:0]=4,鐩爣绐佸彂浼犺緭闀垮害涓?6 */
    regval |= 4 << 12;                  /* SBURST[2:0]=4,婧愮獊鍙戜紶杈撻暱搴︿负16 */
    regval |= 0 << 8;                   /* SINCOS[1:0]=0,婧愬湴鍧€鍙樺寲鍗曚綅涓?浣?瀛楄妭) */
    regval |= 2 << 6;                   /* DSIZE[1:0]=2,鐩爣浣嶅涓?2浣?*/
    regval |= 0 << 4;                   /* SSIZE[1:0]=0,婧愪綅瀹戒负8浣?*/
    regval |= 0 << 2;                   /* DINC[1:0]=0,鐩爣鍦板潃鍥哄畾 */
    regval |= 2 << 0;                   /* SINC[1:0]=2,婧愬湴鍧€鑷 */
    MDMA_Channel7->CTCR = regval;       /* 璁剧疆CTCR瀵勫瓨鍣?*/
    MDMA_Channel7->CTBR = 17 << 0;      /* MDMA鐨勭‖浠惰Е鍙戦€氶亾17瑙﹀彂inmdma,閫氶亾17=JPEG input FIFO threshold
                                         * 璇﹁<STM32H7xx鍙傝€冩墜鍐?577椤?table 95
                                         */
    addrmask = meminaddr & 0XFF000000;  /* 鑾峰彇鎺╃爜 */

    if (addrmask == 0X20000000 || addrmask == 0)MDMA_Channel7->CTBR |= 1 << 16; /* 浣跨敤AHBS鎬荤嚎璁块棶DTCM/ITCM */

    sys_nvic_init(2, 3, MDMA_IRQn, 2);  /* 鎶㈠崰2锛屽瓙浼樺厛绾?锛岀粍2 */
}

/**
 * @brief       JPEG纭欢瑙ｇ爜杈撳嚭MDMA閰嶇疆
 * @param       memoutaddr  : JPEG杈撳嚭MDMA瀛樺偍鍣ㄥ湴鍧€
 * @param       memoutsize  : 杈撳嚭MDMA鏁版嵁闀垮害,0~262143,浠ュ瓧鑺備负鍗曚綅
 * @retval      鏃?
 */
void jpeg_out_dma_init(uint32_t memoutaddr, uint32_t memoutsize)
{
    uint32_t regval = 0;
    uint32_t addrmask = 0;
    RCC->AHB3ENR |= 1 << 0;             /* 浣胯兘MDMA鏃堕挓 */
    MDMA_Channel6->CCR = 0;             /* 杈撳嚭MDMA娓呴浂 */

    while (MDMA_Channel6->CCR & 0X01);  /* 绛夊緟MDMA_Channel6鍏抽棴瀹屾垚 */

    MDMA_Channel6->CIFCR = 0X1F;        /* 涓柇鏍囧織娓呴浂 */
    MDMA_Channel6->CCR |= 3 << 6;       /* PL[1:0]=2,鏈€楂樹紭鍏堢骇 */
    MDMA_Channel6->CCR |= 1 << 2;       /* CTCIE=1,浣胯兘閫氶亾浼犺緭瀹屾垚涓柇 */
    MDMA_Channel6->CBNDTR = memoutsize; /* 浼犺緭闀垮害涓簃eminsize */
    MDMA_Channel6->CDAR = memoutaddr;   /* 鐩爣鍦板潃涓?memoutaddr */
    MDMA_Channel6->CSAR = (uint32_t)&JPEG->DOR; /* JPEG->DOR浣滀负婧愬湴鍧€ */
    regval = 0 << 28;                   /* TRGM[1:0]=0,姣忎釜MDMA璇锋眰瑙﹀彂涓€娆uffer浼犺緭 */
    regval |= 1 << 25;                  /* PKE=1,鎵撳寘浣胯兘 */
    regval |= (32 - 1) << 18;           /* TLEN[6:0]=31,buffer浼犺緭闀垮害涓?2瀛楄妭 */
    regval |= 4 << 15;                  /* DBURST[2:0]=4,鐩爣绐佸彂浼犺緭闀垮害涓?6 */
    regval |= 4 << 12;                  /* SBURST[2:0]=4,婧愮獊鍙戜紶杈撻暱搴︿负16 */
    regval |= 0 << 10;                  /* DINCOS[1:0]=0,鐩爣鍦板潃鍙樺寲鍗曚綅涓?浣?瀛楄妭) */
    regval |= 0 << 6;                   /* DSIZE[1:0]=0,鐩爣浣嶅涓?浣?*/
    regval |= 2 << 4;                   /* SSIZE[1:0]=2,婧愪綅瀹戒负32浣?*/
    regval |= 2 << 2;                   /* DINC[1:0]=2,鐩爣鍦板潃鑷 */
    regval |= 0 << 0;                   /* SINC[1:0]=0,婧愬湴鍧€鍥哄畾 */
    MDMA_Channel6->CTCR = regval;       /* 璁剧疆CTCR瀵勫瓨鍣?*/
    MDMA_Channel6->CTBR = 19 << 0;      /* MDMA鐨勭‖浠惰Е鍙戦€氶亾19瑙﹀彂outmdma,閫氶亾19=JPEG output FIFO threshold */
    /* 璇﹁<STM32H7xx鍙傝€冩墜鍐?577椤?table 95 */
    addrmask = memoutaddr & 0XFF000000; /* 鑾峰彇鎺╃爜 */

    if (addrmask == 0X20000000 || addrmask == 0)MDMA_Channel6->CTBR |= 1 << 17; /* 浣跨敤AHBS鎬荤嚎璁块棶DTCM/ITCM */

    /* 鍦╦peg_in_dma_init閲岄潰宸茬粡璁剧疆浜嗕腑鏂垵濮嬪寲, 鎵€浠ヨ繖閲屽彲浠ヤ笉璁剧疆 */
    //sys_nvic_init(2, 3, MDMA_IRQn, 2);    /* 鎶㈠崰2锛屽瓙浼樺厛绾?锛岀粍2 */
}

void (*jpeg_in_callback)(void);         /* JPEG MDMA杈撳叆鍥炶皟鍑芥暟 */
void (*jpeg_out_callback)(void);        /* JPEG MDMA杈撳嚭 鍥炶皟鍑芥暟 */
void (*jpeg_eoc_callback)(void);        /* JPEG 瑙ｇ爜瀹屾垚 鍥炶皟鍑芥暟 */
void (*jpeg_hdp_callback)(void);        /* JPEG Header瑙ｇ爜瀹屾垚 鍥炶皟鍑芥暟 */

/**
 * @brief       MDMA涓柇鏈嶅姟鍑芥暟
 *   @note      澶勭悊纭欢JPEG瑙ｇ爜鏃惰緭鍏?杈撳嚭鏁版嵁娴?
 * @param       鏃?
 * @retval      鏃?
 */
void MDMA_IRQHandler(void)
{
    OSIntEnter();
    if (MDMA_Channel7->CISR & (1 << 1))     /* CTCIF,閫氶亾7浼犺緭瀹屾垚(杈撳叆) */
        if (MDMA_Channel7->CISR & (1 << 1)) /* CTCIF,閫氶亾7浼犺緭瀹屾垚(杈撳叆) */
        {
            MDMA_Channel7->CIFCR |= 1 << 1; /* 娓呴櫎閫氶亾浼犺緭瀹屾垚涓柇 */
            JPEG->CR &= ~(0X7E);            /* 鍏抽棴JPEG涓柇,闃叉琚墦鏂?*/
            jpeg_in_callback();             /* 鎵ц杈撳叆鍥炶皟鍑芥暟,缁х画璇诲彇鏁版嵁 */
            JPEG->CR |= 3 << 5;             /* 浣胯兘EOC鍜孒PD涓柇 */
        }

    if (MDMA_Channel6->CISR & (1 << 1))     /* CTCIF,閫氶亾6浼犺緭瀹屾垚(杈撳嚭) */
    {
        MDMA_Channel6->CIFCR |= 1 << 1;     /* 娓呴櫎閫氶亾浼犺緭瀹屾垚涓柇 */
        JPEG->CR &= ~(0X7E);                /* 鍏抽棴JPEG涓柇,闃叉琚墦鏂?*/
        jpeg_out_callback();                /* 鎵ц杈撳嚭鍥炶皟鍑芥暟,灏嗘暟鎹浆鎹㈡垚RGB */
        JPEG->CR |= 3 << 5;                 /* 浣胯兘EOC鍜孒PD涓柇 */
    }
    OSIntExit();
}

/**
 * @brief       JPEG瑙ｇ爜涓柇鏈嶅姟鍑芥暟
 * @param       鏃?
 * @retval      鏃?
 */
void JPEG_IRQHandler(void)
{
    OSIntEnter();
    if (JPEG->SR & (1 << 6))    /* JPEG Header瑙ｇ爜瀹屾垚 */
    {
        jpeg_hdp_callback();
        JPEG->CR &= ~(1 << 6);  /* 绂佹Jpeg Header瑙ｇ爜瀹屾垚涓柇 */
        JPEG->CFR |= 1 << 6;    /* 娓呴櫎HPDF浣?header瑙ｇ爜瀹屾垚浣? */
    }

    if (JPEG->SR & (1 << 5))    /* JPEG瑙ｇ爜瀹屾垚 */
    {
        jpeg_dma_stop();
        jpeg_eoc_callback();
        JPEG->CFR |= 1 << 5;    /* 娓呴櫎EOC浣?瑙ｇ爜瀹屾垚浣? */
        MDMA_Channel6->CCR &= ~(1 << 0);    /* 鍏抽棴MDMA閫氶亾6 */
        MDMA_Channel7->CCR &= ~(1 << 0);    /* 鍏抽棴MDMA閫氶亾7 */
    }
    OSIntExit();
}

/**
 * @brief       鍒濆鍖栫‖浠禞PEG鍐呮牳
 * @param       tjpeg       : JPEG缂栬В鐮佹帶鍒剁粨鏋勪綋
 * @retval      0, 鎴愬姛; 1, 澶辫触;
 */
uint8_t jpeg_core_init(jpeg_codec_typedef *tjpeg)
{
    uint8_t i;
    RCC->AHB3ENR |= 1 << 5;             /* 浣胯兘纭欢jpeg鏃堕挓 */

    for (i = 0; i < JPEG_DMA_INBUF_NB; i++)
    {
        tjpeg->inbuf[i].buf = mymalloc(SRAMDTCM, JPEG_DMA_INBUF_LEN);

        if (tjpeg->inbuf[i].buf == NULL)
        {
            jpeg_core_destroy(tjpeg);
            return 1;
        }
    }

    JPEG->CR = 0;                       /* 鍏堟竻闆?*/
    JPEG->CR |= 1 << 0;                 /* 浣胯兘纭欢JPEG */
    JPEG->CONFR0 &= ~(1 << 0);          /* 鍋滄JPEG缂栬В鐮佽繘绋?*/
    JPEG->CR |= 1 << 13;                /* 娓呯┖杈撳叆fifo */
    JPEG->CR |= 1 << 14;                /* 娓呯┖杈撳嚭fifo */
    JPEG->CFR = 3 << 5;                 /* 娓呯┖鏍囧織 */
    sys_nvic_init(1, 3, JPEG_IRQn, 2);  /* 鎶㈠崰1锛屽瓙浼樺厛绾?锛岀粍2 */
    JPEG->CONFR1 |= 1 << 8;             /* 浣胯兘header澶勭悊 */
    return 0;
}

/**
 * @brief       鍏抽棴纭欢JPEG鍐呮牳,骞堕噴鏀惧唴瀛?
 * @param       tjpeg       : JPEG缂栬В鐮佹帶鍒剁粨鏋勪綋
 * @retval      鏃?
 */
void jpeg_core_destroy(jpeg_codec_typedef *tjpeg)
{
    uint8_t i;
    jpeg_dma_stop();    /* 鍋滄MDMA浼犺緭 */

    for (i = 0; i < JPEG_DMA_INBUF_NB; i++)
    {
        myfree(SRAMDTCM, tjpeg->inbuf[i].buf);  /* 閲婃斁鍐呭瓨 */
    }
    for (i = 0; i < JPEG_DMA_OUTBUF_NB; i++)
    {
        myfree(SRAMIN, tjpeg->outbuf[i].buf);   /* 閲婃斁鍐呭瓨 */
    }
}

/**
 * @brief       鍒濆鍖栫‖浠禞PEG瑙ｇ爜鍣?
 * @param       tjpeg       : JPEG缂栬В鐮佹帶鍒剁粨鏋勪綋
 * @retval      鏃?
 */
void jpeg_decode_init(jpeg_codec_typedef *tjpeg)
{
    uint8_t i;
    tjpeg->inbuf_read_ptr = 0;
    tjpeg->inbuf_write_ptr = 0;
    tjpeg->indma_pause = 0;
    tjpeg->outbuf_read_ptr = 0;
    tjpeg->outbuf_write_ptr = 0;
    tjpeg->outdma_pause = 0;
    tjpeg->state = JPEG_STATE_NOHEADER; /* 鍥剧墖瑙ｇ爜缁撴潫鏍囧織 */

    for (i = 0; i < JPEG_DMA_INBUF_NB; i++)
    {
        tjpeg->inbuf[i].sta = 0;
        tjpeg->inbuf[i].size = 0;
    }

    for (i = 0; i < JPEG_DMA_OUTBUF_NB; i++)
    {
        tjpeg->outbuf[i].sta = 0;
        tjpeg->outbuf[i].size = 0;
    }

    MDMA_Channel6->CCR = 0;         /* MDMA閫氶亾6绂佹 */
    MDMA_Channel7->CCR = 0;         /* MDMA閫氶亾7绂佹 */
    MDMA_Channel6->CIFCR = 0X1F;    /* 涓柇鏍囧織娓呴浂 */
    MDMA_Channel7->CIFCR = 0X1F;    /* 涓柇鏍囧織娓呴浂 */

    JPEG->CONFR1 |= 1 << 3;         /* 纭欢JPEG瑙ｇ爜妯″紡 */
    JPEG->CONFR0 &= ~(1 << 0);      /* 鍋滄JPEG缂栬В鐮佽繘绋?*/
    JPEG->CR &= ~(0X3F << 1);       /* 鍏抽棴鎵€鏈変腑鏂?*/
    JPEG->CR |= 1 << 13;            /* 娓呯┖杈撳叆fifo */
    JPEG->CR |= 1 << 14;            /* 娓呯┖杈撳嚭fifo */
    JPEG->CR |= 1 << 6;             /* 浣胯兘Jpeg Header瑙ｇ爜瀹屾垚涓柇 */
    JPEG->CR |= 1 << 5;             /* 浣胯兘瑙ｇ爜瀹屾垚涓柇 */
    JPEG->CFR = 3 << 5;             /* 娓呯┖鏍囧織 */
    JPEG->CONFR0 |= 1 << 0;         /* 浣胯兘JPEG缂栬В鐮佽繘绋?*/
}

/**
 * @brief       鍚姩 jpeg in mdma, 寮€濮嬭В鐮丣PEG
 * @param       鏃?
 * @retval      鏃?
 */
void jpeg_in_dma_start(void)
{
    MDMA_Channel7->CCR |= 1 << 0;   /* 浣胯兘MDMA閫氶亾7鐨勪紶杈?*/
}

/**
 * @brief       鍚姩 jpeg out mdma, 寮€濮嬭緭鍑篩UV鏁版嵁
 * @param       鏃?
 * @retval      鏃?
 */
void jpeg_out_dma_start(void)
{
    MDMA_Channel6->CCR |= 1 << 0;   /* 浣胯兘MDMA閫氶亾6鐨勪紶杈?*/
}

/**
 * @brief       鍋滄JPEG MDMA瑙ｇ爜杩囩▼
 * @param       鏃?
 * @retval      鏃?
 */
void jpeg_dma_stop(void)
{
    JPEG->CONFR0 &= ~(1 << 0);      /* 鍋滄JPEG缂栬В鐮佽繘绋?*/
    JPEG->CR &= ~(0X3F << 1);       /* 鍏抽棴鎵€鏈変腑鏂?*/
    JPEG->CFR = 3 << 5;             /* 娓呯┖鏍囧織 */
}

/**
 * @brief       鎭㈠MDMA IN杩囩▼
 * @param       memaddr     : 瀛樺偍鍖洪鍦板潃
 * @param       memlen      : 瑕佷紶杈撴暟鎹暱搴?浠ュ瓧鑺備负鍗曚綅)
 * @retval      鏃?
 */
void jpeg_in_dma_resume(uint32_t memaddr, uint32_t memlen)
{
    if (memlen % 4)memlen += 4 - memlen % 4;    /* 鎵╁睍鍒?鐨勫€嶆暟 */

    MDMA_Channel7->CIFCR = 0X1F;    /* 涓柇鏍囧織娓呴浂 */
    MDMA_Channel7->CBNDTR = memlen; /* 浼犺緭闀垮害涓簃emlen */
    MDMA_Channel7->CSAR = memaddr;  /* memaddr浣滀负婧愬湴鍧€ */
    MDMA_Channel7->CCR |= 1 << 0;   /* 浣胯兘MDMA閫氶亾7鐨勪紶杈?*/
}

/**
 * @brief       鎭㈠MDMA OUT杩囩▼
 * @param       memaddr     : 瀛樺偍鍖洪鍦板潃
 * @param       memlen      : 瑕佷紶杈撴暟鎹暱搴?浠ュ瓧鑺備负鍗曚綅)
 * @retval      鏃?
 */
void jpeg_out_dma_resume(uint32_t memaddr, uint32_t memlen)
{
    if (memlen % 4)memlen += 4 - memlen % 4;    /* 鎵╁睍鍒?鐨勫€嶆暟 */

    MDMA_Channel6->CIFCR = 0X1F;    /* 涓柇鏍囧織娓呴浂 */
    MDMA_Channel6->CBNDTR = memlen; /* 浼犺緭闀垮害涓簃emlen */
    MDMA_Channel6->CDAR = memaddr;  /* memaddr浣滀负婧愬湴鍧€ */
    MDMA_Channel6->CCR |= 1 << 0;   /* 浣胯兘MDMA閫氶亾6鐨勪紶杈?*/
}

/**
 * @brief       鑾峰彇鍥惧儚淇℃伅
 * @param       tjpeg       : JPEG缂栬В鐮佹帶鍒剁粨鏋勪綋
 * @retval      鏃?
 */
void jpeg_get_info(jpeg_codec_typedef *tjpeg)
{
    uint32_t yblockNb, cBblockNb, cRblockNb;

    switch (JPEG->CONFR1 & 0X03)
    {
        case 0:/* grayscale,1 color component */
            tjpeg->Conf.ColorSpace = JPEG_GRAYSCALE_COLORSPACE;
            break;

        case 2:/* YUV/RGB,3 color component */
            tjpeg->Conf.ColorSpace = JPEG_YCBCR_COLORSPACE;
            break;

        case 3:/* CMYK,4 color component */
            tjpeg->Conf.ColorSpace = JPEG_CMYK_COLORSPACE;
            break;
    }

    tjpeg->Conf.ImageHeight = (JPEG->CONFR1 & 0XFFFF0000) >> 16;    /* 鑾峰緱鍥惧儚楂樺害 */
    tjpeg->Conf.ImageWidth = (JPEG->CONFR3 & 0XFFFF0000) >> 16;     /* 鑾峰緱鍥惧儚瀹藉害 */

    if ((tjpeg->Conf.ColorSpace == JPEG_YCBCR_COLORSPACE) || (tjpeg->Conf.ColorSpace == JPEG_CMYK_COLORSPACE))
    {
        yblockNb  = (JPEG->CONFR4 & (0XF << 4)) >> 4;
        cBblockNb = (JPEG->CONFR5 & (0XF << 4)) >> 4;
        cRblockNb = (JPEG->CONFR6 & (0XF << 4)) >> 4;

        if ((yblockNb == 1) && (cBblockNb == 0) && (cRblockNb == 0))
        {
            tjpeg->Conf.ChromaSubsampling = JPEG_422_SUBSAMPLING;   /* 16x8 block */
        }
        else if ((yblockNb == 0) && (cBblockNb == 0) && (cRblockNb == 0))
        {
            tjpeg->Conf.ChromaSubsampling = JPEG_444_SUBSAMPLING;
        }
        else if ((yblockNb == 3) && (cBblockNb == 0) && (cRblockNb == 0))
        {
            tjpeg->Conf.ChromaSubsampling = JPEG_420_SUBSAMPLING;
        }
        else
        {
            tjpeg->Conf.ChromaSubsampling = JPEG_444_SUBSAMPLING;
        }
    }
    else
    {
        tjpeg->Conf.ChromaSubsampling = JPEG_444_SUBSAMPLING;   /* 榛樿鐢?:4:4 */
    }
    
    tjpeg->Conf.ImageQuality = 0;   /* 鍥惧儚璐ㄩ噺鍙傛暟鍦ㄦ暣涓浘鐗囩殑鏈€鏈熬,鍒氬紑濮嬬殑鏃跺€?鏄棤娉曡幏鍙栫殑,鎵€浠ョ洿鎺ヨ缃负0 */
}

/**
 * @brief       寰楀埌JPEG鍥惧儚璐ㄩ噺
 *   @note      鍦ㄨВ鐮佸畬鎴愬悗,鍙互璋冪敤骞惰幏寰楁纭殑缁撴灉.
 * @param       鏃?
 * @retval      鍥惧儚璐ㄩ噺, 0~100
 */
uint8_t jpeg_get_quality(void)
{
    uint32_t quality = 0;
    uint32_t quantRow, quantVal, scale, i, j;
    uint32_t *tableAddress = (uint32_t *)JPEG->QMEM0;
    i = 0;

    while (i < JPEG_QUANT_TABLE_SIZE)
    {
        quantRow = *tableAddress;

        for (j = 0; j < 4; j++)
        {
            quantVal = (quantRow >> (8 * j)) & 0xFF;

            if (quantVal == 1)quality += 100;   /* 100% */
            else
            {
                scale = (quantVal * 100) / ((uint32_t)JPEG_LUM_QuantTable[JPEG_ZIGZAG_ORDER[i + j]]);

                if (scale <= 100)
                {
                    quality += (200 - scale) / 2;
                }
                else
                {
                    quality += 5000 / scale;
                }
            }
        }

        i += 4;
        tableAddress++;
    }

    return (quality / ((uint32_t)64));
}

/**
 * @brief       灏哬UV鏁版嵁杞崲鎴怰GB鏁版嵁
 *   @note      鍒╃敤DMA2D, 灏咼PEG瑙ｇ爜鐨刌UV鏁版嵁杞崲鎴怰GB鏁版嵁, 鍏ㄧ‖浠跺畬鎴? 閫熷害闈炲父蹇?
 * @param       tjpeg       : JPEG缂栬В鐮佹帶鍒剁粨鏋勪綋
 * @param       pdst        : 杈撳嚭鏁扮粍棣栧湴鍧€
 * @retval      0, 鎴愬姛; 1, 瓒呮椂,澶辫触;
 */
uint8_t jpeg_dma2d_yuv2rgb_conversion(jpeg_codec_typedef *tjpeg, uint32_t *pdst)
{
    uint32_t regval = 0;
    uint32_t cm = 0;            /* 閲囨牱鏂瑰紡n */
    uint32_t timeout = 0;
    uint32_t destination = 0;

    if (tjpeg->Conf.ChromaSubsampling == JPEG_420_SUBSAMPLING)
    {
        cm = DMA2D_CSS_420;     /* YUV420杞琑GB */
    }
    else if (tjpeg->Conf.ChromaSubsampling == JPEG_422_SUBSAMPLING)
    {
        cm = DMA2D_CSS_422;     /* YUV422杞琑GB */
    }
    else if (tjpeg->Conf.ChromaSubsampling == JPEG_444_SUBSAMPLING)
    {
        cm = DMA2D_NO_CSS;      /* YUV444杞琑GB */
    }
    
    destination = (uint32_t)pdst + (tjpeg->yuvblk_curheight * tjpeg->Conf.ImageWidth) * 2;  /* 璁＄畻鐩爣鍦板潃鐨勯鍦板潃 */
    
    RCC->AHB3ENR |= 1 << 4;     /* 浣胯兘DMA2D鏃堕挓 */
    RCC->AHB3RSTR |= 1 << 4;    /* 澶嶄綅DMA2D */
    RCC->AHB3RSTR &= ~(1 << 4); /* 缁撴潫澶嶄綅 */
    DMA2D->CR &= ~(1 << 0);     /* 鍏堝仠姝MA2D */
    DMA2D->CR = 1 << 16;        /* MODE[2:0]=001,瀛樺偍鍣ㄥ埌瀛樺偍鍣?甯FC妯″紡 */
    DMA2D->OPFCCR = 2 << 0;     /* CM[2:0]=010,杈撳嚭涓篟GB565鏍煎紡 */
    DMA2D->OOR = 0;             /* 璁剧疆琛屽亸绉讳负0 */
    DMA2D->IFCR |= 1 << 1;      /* 娓呴櫎浼犺緭瀹屾垚鏍囧織 */
    regval = 11 << 0;           /* CM[3:0]=1011,杈撳叆鏁版嵁涓篩CbCr鏍煎紡 */
    regval |= cm << 18;         /* CSS[1:0]=cm,Chroma Sub-Sampling:0,4:4:4;1,4:2:2;2,4:2:0 */
    DMA2D->FGPFCCR = regval;    /* 璁剧疆FGPCCR瀵勫瓨鍣?*/
    DMA2D->FGOR = 0;            /* 鍓嶆櫙灞傝鍋忕Щ涓? */
    DMA2D->NLR = tjpeg->yuvblk_height | (tjpeg->Conf.ImageWidth << 16); /* 璁惧畾琛屾暟瀵勫瓨鍣?*/
    DMA2D->OMAR = destination;  /* 杈撳嚭瀛樺偍鍣ㄥ湴鍧€ */
    DMA2D->FGMAR = (uint32_t)tjpeg->outbuf[tjpeg->outbuf_read_ptr].buf; /* 婧愬湴鍧€ */
    DMA2D->CR |= 1 << 0;        /* 鍚姩DMA2D */

    while ((DMA2D->ISR & (1 << 1)) == 0)    /* 绛夊緟浼犺緭瀹屾垚 */
    {
        timeout++;

        if (timeout > 0X1FFFFFF)break;      /* 瓒呮椂閫€鍑?*/
    }

    tjpeg->yuvblk_curheight += tjpeg->yuvblk_height;/* 鍋忕Щ鍒颁笅涓€涓唴瀛樺湴鍧€ */
    /* YUV2RGB杞爜缁撴潫鍚?鍐嶅浣嶄竴娆MA2D */
    RCC->AHB3RSTR |= 1 << 4;    /* 澶嶄綅DMA2D */
    RCC->AHB3RSTR &= ~(1 << 4); /* 缁撴潫澶嶄綅 */

    if (timeout > 0X1FFFFFF)return 1;

    return 0;
}





























