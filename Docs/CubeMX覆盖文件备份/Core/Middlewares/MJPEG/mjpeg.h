/**
 ****************************************************************************************************
 * @file        mjpeg.h
 * @author      锟斤拷锟斤拷原锟斤拷锟脚讹拷(ALIENTEK)
 * @version     V1.0
 * @date        2023-01-05
 * @brief       MJPEG锟斤拷频锟斤拷锟斤拷 锟斤拷锟斤拷
 * @license     Copyright (c) 2020-2032, 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷涌萍锟斤拷锟斤拷薰锟剿?
 ****************************************************************************************************
 * @attention
 *
 * 实锟斤拷平台:锟斤拷锟斤拷原锟斤拷 锟斤拷锟斤拷锟斤拷 F429锟斤拷锟斤拷锟斤拷
 * 锟斤拷锟斤拷锟斤拷频:www.yuanzige.com
 * 锟斤拷锟斤拷锟斤拷坛:www.openedv.com
 * 锟斤拷司锟斤拷址:www.alientek.com
 * 锟斤拷锟斤拷锟街?openedv.taobao.com
 *
 * 锟睫革拷说锟斤拷
 * V1.0 20230105
 * 锟斤拷一锟轿凤拷锟斤拷
 *
 ****************************************************************************************************
 */

#ifndef __MJPEG_H
#define __MJPEG_H

#include "stdio.h" 
#include "stm32h7xx.h"
#include "jpegcodec.h" 


extern jpeg_codec_typedef mjpeg;


uint8_t mjpeg_jpeg_core_init(jpeg_codec_typedef *tjpeg);
void mjpeg_jpeg_core_destroy(jpeg_codec_typedef *tjpeg); 
void mjpeg_dma_in_callback(void);
void mjpeg_dma_out_callback(void);
void mjpeg_endofcovert_callback(void);
void mjpeg_hdrover_callback(void);
uint8_t mjpeg_init(uint16_t offx,uint16_t offy,uint32_t width,uint32_t height);
void mjpeg_free(void);
void mjpeg_ltdc_dma2d_yuv2rgb_fill(uint16_t sx,uint16_t sy,jpeg_codec_typedef *tjpeg);
void mjpeg_fill_color(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t *color);
uint8_t mjpeg_decode(uint8_t* buf,uint32_t bsize);
 
#endif
