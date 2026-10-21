/**
 ****************************************************************************************************
 * @file        jpegcodec.h
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

#ifndef __JPEGCODEC_H
#define __JPEGCODEC_H

#include "stm32h7xx.h"


#define JPEG_DMA_INBUF_LEN          4096                /* 鍗曚釜MDMA IN  BUF鐨勫ぇ灏?*/
#define JPEG_DMA_INBUF_NB           10                  /* MDMA IN  BUF鐨勪釜鏁?*/
#define JPEG_DMA_OUTBUF_NB          2                   /* MDMA OUT BUF鐨勪釜鏁?*/


#define JPEG_QUANT_TABLE_SIZE       ((uint32_t)64U)     /* JPEG Quantization Table Size */

/* JPEG鏁版嵁缂撳啿缁撴瀯浣?*/
typedef struct
{
    uint8_t sta;        /* 鐘舵€?0,鏃犳暟鎹?1,鏈夋暟鎹?*/
    uint8_t *buf;       /* JPEG鏁版嵁缂撳啿鍖?*/
    uint16_t size;      /* JPEG鏁版嵁闀垮害 */
} jpeg_databuf_type;

#define JPEG_STATE_NOHEADER         0                   /* HEADER鏈鍙?鍒濆鐘舵€?*/
#define JPEG_STATE_HEADEROK         1                   /* HEADER璇诲彇鎴愬姛 */
#define JPEG_STATE_FINISHED         2                   /* 瑙ｇ爜瀹屾垚 */
#define JPEG_STATE_ERROR            3                   /* 瑙ｇ爜閿欒 */

#define JPEG_GRAYSCALE_COLORSPACE   ((uint32_t)0x00000000U)
#define JPEG_YCBCR_COLORSPACE       JPEG_CONFR1_COLORSPACE_0
#define JPEG_CMYK_COLORSPACE        JPEG_CONFR1_COLORSPACE

#define JPEG_444_SUBSAMPLING        ((uint32_t)0x00000000U)     /* Chroma Subsampling 4:4:4 */
#define JPEG_420_SUBSAMPLING        ((uint32_t)0x00000001U)     /* Chroma Subsampling 4:2:0 */
#define JPEG_422_SUBSAMPLING        ((uint32_t)0x00000002U)     /* Chroma Subsampling 4:2:2 */

#ifndef DMA2D_NO_CSS
#define DMA2D_NO_CSS                ((uint32_t)0x00000000U)     /* No chroma sub-sampling 4:4:4 */
#endif
#ifndef DMA2D_CSS_422
#define DMA2D_CSS_422               ((uint32_t)0x00000001U)     /* chroma sub-sampling 4:2:2 */
#endif
#ifndef DMA2D_CSS_420
#define DMA2D_CSS_420               ((uint32_t)0x00000002U)     /* chroma sub-sampling 4:2:0 */
#endif


/* JPEG鏂囦欢淇℃伅缁撴瀯浣?*/
typedef struct
{
    uint8_t  ColorSpace;            /* 鍥惧儚鐨勯鑹茬┖闂? gray-scale/YCBCR/RGB/CMYK */
    uint8_t  ChromaSubsampling;     /* YCBCR/CMYK棰滆壊绌洪棿鐨勮壊搴︽娊鏍锋儏鍐?0,4:4:4;1,4:2:2;2,4:1:1;3,4:2:0 */
    uint32_t ImageHeight;           /* 鍥惧儚楂樺害 */
    uint32_t ImageWidth;            /* 鍥惧儚瀹藉害 */
    uint8_t  ImageQuality;          /* 鍥惧儚缂栫爜璐ㄩ噺:1~100 */
} jpeg_codec_ConfTypeDef;


/* jpeg缂栬В鐮佹帶鍒剁粨鏋勪綋 */
typedef struct
{
    jpeg_codec_ConfTypeDef    Conf;                       /* 褰撳墠JPEG鏂囦欢鐩稿叧鍙傛暟 */
    jpeg_databuf_type inbuf[JPEG_DMA_INBUF_NB];     /* MDMA IN buf */
    jpeg_databuf_type outbuf[JPEG_DMA_OUTBUF_NB];   /* MDMA OUT buf */
    volatile uint8_t inbuf_read_ptr;                /* MDMA IN buf褰撳墠璇诲彇浣嶇疆 */
    volatile uint8_t inbuf_write_ptr;               /* MDMA IN buf褰撳墠鍐欏叆浣嶇疆 */
    volatile uint8_t indma_pause;                   /* 杈撳叆MDMA鏆傚仠鐘舵€佹爣璇?*/
    volatile uint8_t outbuf_read_ptr;               /* MDMA OUT buf褰撳墠璇诲彇浣嶇疆 */
    volatile uint8_t outbuf_write_ptr;              /* MDMA OUT buf褰撳墠鍐欏叆浣嶇疆 */
    volatile uint8_t outdma_pause;                  /* 杈撳叆MDMA鏆傚仠鐘舵€佹爣璇?*/
    volatile uint8_t state;                         /* 瑙ｇ爜鐘舵€?0,鏈瘑鍒埌Header;1,璇嗗埆鍒颁簡Header;2,瑙ｇ爜瀹屾垚; */
    uint32_t yuvblk_size;                           /* YUV杈撳嚭鐨勫瓧鑺傛暟,浣垮緱瀹屾垚涓€娆MA2D YUV2RGB杞崲,鍒氬ソ鏄浘鐗囧搴︾殑鏁存暟鍊?
                                                     * YUV420鍥剧墖,姣忎釜鍍忕礌鍗?.5涓猋UV瀛楄妭,姣忔杈撳嚭16琛?yuvblk_size=鍥剧墖瀹藉害*16*1.5
                                                     * YUV422鍥剧墖,姣忎釜鍍忕礌鍗?涓猋UV瀛楄妭鍜孯GB565涓€鏍?姣忔杈撳嚭8琛?yuvblk_size=鍥剧墖瀹藉害*8*2
                                                     * YUV444鍥剧墖,姣忎釜鍍忕礌鍗?涓猋UV瀛楄妭,姣忔杈撳嚭8琛?yuvblk_size=鍥剧墖瀹藉害*8*3
                                                     */
                                                    
    uint16_t yuvblk_height;                         /* 姣忎釜YUV鍧楄緭鍑哄儚绱犵殑楂樺害,瀵逛簬YUV420,涓?6,瀵逛簬YUV422/YUV444涓? */
    uint16_t yuvblk_curheight;                      /* 褰撳墠杈撳嚭楂樺害,0~鍒嗚鲸鐜囬珮搴?*/
} jpeg_codec_typedef;

/* MDMA鍥炶皟鍑芥暟(闇€瑕佸閮ㄥ疄鐜? */
extern void (*jpeg_in_callback)(void);              /* JPEG MDMA杈撳叆鍥炶皟鍑芥暟 */
extern void (*jpeg_out_callback)(void);             /* JPEG MDMA杈撳嚭 鍥炶皟鍑芥暟 */
extern void (*jpeg_eoc_callback)(void);             /* JPEG 瑙ｇ爜瀹屾垚 鍥炶皟鍑芥暟 */
extern void (*jpeg_hdp_callback)(void);             /* JPEG Header瑙ｇ爜瀹屾垚 鍥炶皟鍑芥暟 */


/* 鎺ュ彛鍑芥暟 */

void jpeg_dma_stop(void);
void jpeg_in_dma_start(void);
void jpeg_out_dma_start(void);
void jpeg_in_dma_resume(uint32_t memaddr, uint32_t memlen);
void jpeg_out_dma_resume(uint32_t memaddr, uint32_t memlen);
void jpeg_in_dma_init(uint32_t meminaddr, uint32_t meminsize);
void jpeg_out_dma_init(uint32_t memoutaddr, uint32_t memoutsize);

uint8_t jpeg_get_quality(void);
void jpeg_get_info(jpeg_codec_typedef *tjpeg);
void jpeg_decode_init(jpeg_codec_typedef *tjpeg);
void jpeg_core_destroy(jpeg_codec_typedef *tjpeg);
uint8_t jpeg_core_init(jpeg_codec_typedef *tjpeg);
uint8_t jpeg_dma2d_yuv2rgb_conversion(jpeg_codec_typedef *tjpeg, uint32_t *pdst);

#endif




















