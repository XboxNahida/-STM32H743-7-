/**
 ****************************************************************************************************
 * @file        avi.h
 * @author      锟斤拷锟斤拷原锟斤拷锟脚讹拷(ALIENTEK)
 * @version     V1.0
 * @date        2023-01-05
 * @brief       AVI锟斤拷频锟斤拷式锟斤拷锟斤拷 锟斤拷锟斤拷
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


#ifndef __AVI_H
#define __AVI_H

#include "stm32h7xx.h"


/* 帧锟斤拷小, 锟斤拷锟斤拷锟皆硷拷锟斤拷锟节达拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?
 * 锟斤拷锟斤拷诖锟斤拷,锟斤拷锟斤拷锟斤拷锟矫达拷一锟斤拷. 锟斤拷锟斤拷诖锟斤拷锟?锟斤拷锟斤拷小一锟斤拷.
 * 一锟斤拷锟斤拷锟斤拷锟斤拷锟街碉拷锟斤拷锟斤拷锟斤拷堑锟街★拷锟斤拷锟斤拷小锟斤拷锟斤拷
 */
#define AVI_MAX_FRAME_SIZE  260 * 1024     /* 锟斤拷锟街★拷锟叫?锟斤拷锟杰筹拷锟斤拷260KB */

/* 锟斤拷锟斤拷锟斤拷锟斤拷 */
typedef enum
{
    AVI_OK = 0,             /* 0,锟缴癸拷 */
    AVI_RIFF_ERR,           /* 1,RIFF ID锟斤拷取失锟斤拷 */
    AVI_AVI_ERR,            /* 2,AVI ID锟斤拷取失锟斤拷 */
    AVI_LIST_ERR,           /* 3,LIST ID锟斤拷取失锟斤拷 */
    AVI_HDRL_ERR,           /* 4,HDRL ID锟斤拷取失锟斤拷 */
    AVI_AVIH_ERR,           /* 5,AVIH ID锟斤拷取失锟斤拷 */
    AVI_STRL_ERR,           /* 6,STRL ID锟斤拷取失锟斤拷 */
    AVI_STRH_ERR,           /* 7,STRH ID锟斤拷取失锟斤拷 */
    AVI_STRF_ERR,           /* 8,STRF ID锟斤拷取失锟斤拷 */
    AVI_MOVI_ERR,           /* 9,MOVI ID锟斤拷取失锟斤拷 */
    AVI_FORMAT_ERR,         /* 10,锟斤拷式锟斤拷锟斤拷 */
    AVI_STREAM_ERR,         /* 11,锟斤拷锟斤拷锟斤拷 */
} AVISTATUS;

#define AVI_RIFF_ID         0X46464952
#define AVI_AVI_ID          0X20495641
#define AVI_LIST_ID         0X5453494C
#define AVI_HDRL_ID         0X6C726468      /* 锟斤拷息锟斤拷锟街?*/
#define AVI_MOVI_ID         0X69766F6D      /* 锟斤拷锟捷匡拷锟街?*/
#define AVI_STRL_ID         0X6C727473      /* strl锟斤拷志 */

#define AVI_AVIH_ID         0X68697661      /* avih锟接匡拷锟紸VI_HDRL_ID */
#define AVI_STRH_ID         0X68727473      /* strh(锟斤拷头)锟接匡拷锟紸VI_STRL_ID */
#define AVI_STRF_ID         0X66727473      /* strf(锟斤拷锟斤拷式)锟接匡拷锟紸VI_STRL_ID */
#define AVI_STRD_ID         0X64727473      /* strd锟接匡拷锟紸VI_STRL_ID (锟斤拷选锟斤拷) */

#define AVI_VIDS_STREAM     0X73646976      /* 锟斤拷频锟斤拷 */
#define AVI_AUDS_STREAM     0X73647561      /* 锟斤拷频锟斤拷 */


#define AVI_VIDS_FLAG       0X6463          /* 锟斤拷频锟斤拷锟斤拷志 */
#define AVI_AUDS_FLAG       0X7762          /* 锟斤拷频锟斤拷锟斤拷志 */



#define AVI_FORMAT_MJPG     0X47504A4D
#define AVI_AUDIO_FORMAT_PCM 0X0001U
#define AVI_AUDIO_FORMAT_MP3 0X0055U


/* AVI 锟斤拷息锟结构锟斤拷 */
/* 锟斤拷一些锟斤拷要锟斤拷锟斤拷锟斤拷,锟斤拷锟斤拷锟斤拷锟斤拷锟?锟斤拷锟斤拷锟斤拷锟?*/
typedef __PACKED_STRUCT
{
    uint32_t SecPerFrame;       /* 锟斤拷频帧锟斤拷锟绞憋拷锟?锟斤拷位为us) */
    uint32_t TotalFrame;        /* 锟侥硷拷锟斤拷帧锟斤拷 */
    uint32_t Width;             /* 图锟斤拷锟?*/
    uint32_t Height;            /* 图锟斤拷锟?*/
    uint32_t SampleRate;        /* 锟斤拷频锟斤拷锟斤拷锟斤拷 */
    uint32_t AudioByteRate;
    uint16_t Channels;          /* 锟斤拷锟斤拷锟斤拷,一锟斤拷为2,锟斤拷示锟斤拷锟斤拷锟斤拷 */
    uint16_t AudioBlockAlign;
    uint16_t BitsPerSample;
    uint16_t AudioBufSize;      /* 锟斤拷频锟斤拷锟斤拷锟斤拷锟斤拷小 */
    uint16_t AudioType;         /* 锟斤拷频锟斤拷锟斤拷:0X0001=PCM;0X0050=MP2;0X0055=MP3;0X2000=AC3; */
    uint16_t StreamID;          /* 锟斤拷锟斤拷锟斤拷ID,StreamID=='dc'==0X6463 /StreamID=='wb'==0X7762 */
    uint32_t StreamSize;        /* 锟斤拷锟斤拷小,锟斤拷锟斤拷锟斤拷偶锟斤拷,锟斤拷锟斤拷锟饺★拷锟轿拷锟斤拷锟?锟斤拷锟?.锟斤拷为偶锟斤拷 */
    char *VideoFLAG;            /* 锟斤拷频帧锟斤拷锟?VideoFLAG="00dc"/"01dc" */
    char *AudioFLAG;            /* 锟斤拷频帧锟斤拷锟?AudioFLAG="00wb"/"01wb" */
} AVI_INFO;

extern AVI_INFO g_avix;         /* avi锟侥硷拷锟斤拷锟斤拷锟较?*/


/* AVI 锟斤拷锟斤拷息 */
typedef struct
{
    uint32_t RiffID;            /* RiffID=='RIFF'==0X61766968 */
    uint32_t FileSize;          /* AVI锟侥硷拷锟斤拷小(锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?锟街斤拷,也RIFFID锟斤拷FileSize锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷) */
    uint32_t AviID;             /* AviID=='AVI '==0X41564920 */
} AVI_HEADER;

/* AVI 锟斤拷锟斤拷息 */
typedef struct
{
    uint32_t FrameID;           /* 帧ID,FrameID=='RIFF'==0X61766968 */
    uint32_t FrameSize;         /* 帧锟斤拷小 */
} FRAME_HEADER;


/* LIST 锟斤拷锟斤拷息 */
typedef struct
{
    uint32_t ListID;            /* ListID=='LIST'==0X4c495354 */
    uint32_t BlockSize;         /* 锟斤拷锟叫?锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?锟街斤拷,也ListID锟斤拷BlockSize锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷) */
    uint32_t ListType;          /* LIST锟接匡拷锟斤拷锟斤拷:hdrl(锟斤拷息锟斤拷)/movi(锟斤拷锟捷匡拷)/idxl(锟斤拷锟斤拷锟斤拷,锟角憋拷锟斤拷,锟角匡拷选锟斤拷) */
} LIST_HEADER;

/* avih 锟接匡拷锟斤拷息 */
typedef struct
{
    uint32_t BlockID;           /* 锟斤拷锟街?avih==0X61766968 */
    uint32_t BlockSize;         /* 锟斤拷锟叫?锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?锟街斤拷,也锟斤拷锟斤拷BlockID锟斤拷BlockSize锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷) */
    uint32_t SecPerFrame;       /* 锟斤拷频帧锟斤拷锟绞憋拷锟?锟斤拷位为us) */
    uint32_t MaxByteSec;        /* 锟斤拷锟斤拷锟斤拷荽锟斤拷锟斤拷锟?锟街斤拷/锟斤拷 */
    uint32_t PaddingGranularity;/* 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷 */
    uint32_t Flags;             /* AVI锟侥硷拷锟斤拷全锟街憋拷牵锟斤拷锟斤拷锟斤拷欠锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷 */
    uint32_t TotalFrame;        /* 锟侥硷拷锟斤拷帧锟斤拷 */
    uint32_t InitFrames;        /* 为锟斤拷锟斤拷锟斤拷式指锟斤拷锟斤拷始帧锟斤拷锟斤拷锟角斤拷锟斤拷锟斤拷式应锟斤拷指锟斤拷为0锟斤拷 */
    uint32_t Streams;           /* 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?通锟斤拷为2 */
    uint32_t RefBufSize;        /* 锟斤拷锟斤拷锟饺★拷锟斤拷募锟斤拷幕锟斤拷锟斤拷小锟斤拷应锟斤拷锟斤拷锟斤拷锟斤拷锟侥块）默锟较匡拷锟斤拷锟斤拷1M锟街斤拷!!! */
    uint32_t Width;             /* 图锟斤拷锟?*/
    uint32_t Height;            /* 图锟斤拷锟?*/
    uint32_t Reserved[4];       /* 锟斤拷锟斤拷 */
} AVIH_HEADER;

/* strh 锟斤拷头锟接匡拷锟斤拷息(strh锟斤拷strl) */
typedef struct
{
    uint32_t BlockID;           /* 锟斤拷锟街?strh==0X73747268 */
    uint32_t BlockSize;         /* 锟斤拷锟叫?锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?锟街斤拷,也锟斤拷锟斤拷BlockID锟斤拷BlockSize锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷) */
    uint32_t StreamType;        /* 锟斤拷锟斤拷锟斤拷锟斤拷锟洁，vids(0X73646976):锟斤拷频;auds(0X73647561):锟斤拷频 */
    uint32_t Handler;           /* 指锟斤拷锟斤拷锟侥达拷锟斤拷锟竭ｏ拷锟斤拷锟斤拷锟斤拷锟斤拷频锟斤拷说锟斤拷锟角斤拷锟斤拷锟斤拷,锟斤拷锟斤拷MJPG/H264之锟斤拷锟?*/
    uint32_t Flags;             /* 锟斤拷牵锟斤拷欠锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟缴拷锟斤拷欠锟戒化锟斤拷 */
    uint16_t Priority;          /* 锟斤拷锟斤拷锟斤拷锟饺硷拷锟斤拷锟斤拷锟叫讹拷锟斤拷锟酵拷锟斤拷偷锟斤拷锟绞憋拷锟斤拷燃锟斤拷锟竭碉拷为默锟斤拷锟斤拷锟斤拷 */
    uint16_t Language;          /* 锟斤拷频锟斤拷锟斤拷锟皆达拷锟斤拷 */
    uint32_t InitFrames;        /* 为锟斤拷锟斤拷锟斤拷式指锟斤拷锟斤拷始帧锟斤拷 */
    uint32_t Scale;             /* 锟斤拷锟斤拷锟斤拷, 锟斤拷频每锟斤拷拇锟叫★拷锟斤拷锟斤拷锟狡碉拷牟锟斤拷锟斤拷锟叫?*/
    uint32_t Rate;              /* Scale/Rate=每锟斤拷锟斤拷锟斤拷锟?*/
    uint32_t Start;             /* 锟斤拷锟斤拷锟斤拷锟斤拷始锟斤拷锟脚碉拷位锟矫ｏ拷锟斤拷位为Scale */
    uint32_t Length;            /* 锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷位为Scale */
    uint32_t RefBufSize;        /* 锟斤拷锟斤拷使锟矫的伙拷锟斤拷锟斤拷锟斤拷小 */
    uint32_t Quality;           /* 锟斤拷压锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷值越锟斤拷锟斤拷锟斤拷越锟斤拷 */
    uint32_t SampleSize;        /* 锟斤拷频锟斤拷锟斤拷锟斤拷锟斤拷小 */
    struct                      /* 锟斤拷频帧锟斤拷占锟侥撅拷锟斤拷 */
    {
        short Left;
        short Top;
        short Right;
        short Bottom;
    } Frame;
} STRH_HEADER;

/* BMP锟结构锟斤拷 */
typedef struct
{
    uint32_t BmpSize;           /* bmp锟结构锟斤拷锟叫?锟斤拷锟斤拷(BmpSize锟斤拷锟斤拷) */
    long Width;                 /* 图锟斤拷锟?*/
    long Height;                /* 图锟斤拷锟?*/
    uint16_t  Planes;           /* 平锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷为1 */
    uint16_t  BitCount;         /* 锟斤拷锟斤拷位锟斤拷,0X0018锟斤拷示24位 */
    uint32_t  Compression;      /* 压锟斤拷锟斤拷锟酵ｏ拷锟斤拷锟斤拷:MJPG/H264锟斤拷 */
    uint32_t  SizeImage;        /* 图锟斤拷锟叫?*/
    long XpixPerMeter;          /* 水平锟街憋拷锟斤拷 */
    long YpixPerMeter;          /* 锟斤拷直锟街憋拷锟斤拷 */
    uint32_t  ClrUsed;          /* 实锟斤拷使锟斤拷锟剿碉拷色锟斤拷锟叫碉拷锟斤拷色锟斤拷,压锟斤拷锟斤拷式锟叫诧拷使锟斤拷 */
    uint32_t  ClrImportant;     /* 锟斤拷要锟斤拷锟斤拷色 */
} BMP_HEADER;

/* 锟斤拷色锟斤拷 */
typedef struct
{
    uint8_t  rgbBlue;           /* 锟斤拷色锟斤拷锟斤拷锟斤拷(值锟斤拷围为0-255) */
    uint8_t  rgbGreen;          /* 锟斤拷色锟斤拷锟斤拷锟斤拷(值锟斤拷围为0-255) */
    uint8_t  rgbRed;            /* 锟斤拷色锟斤拷锟斤拷锟斤拷(值锟斤拷围为0-255) */
    uint8_t  rgbReserved;       /* 锟斤拷锟斤拷锟斤拷锟斤拷锟轿? */
} AVIRGBQUAD;

/* 锟斤拷锟斤拷strh,锟斤拷锟斤拷锟斤拷锟狡碉拷锟?strf(锟斤拷锟斤拷式)使STRH_BMPHEADER锟斤拷 */
typedef struct
{
    uint32_t BlockID;           /* 锟斤拷锟街?strf==0X73747266 */
    uint32_t BlockSize;         /* 锟斤拷锟叫?锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?锟街斤拷,也锟斤拷锟斤拷BlockID锟酵憋拷BlockSize锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷) */
    BMP_HEADER bmiHeader;       /* 位图锟斤拷息头 */
    AVIRGBQUAD bmColors[1];     /* 锟斤拷色锟斤拷 */
} STRF_BMPHEADER;

/* 锟斤拷锟斤拷strh,锟斤拷锟斤拷锟斤拷锟狡碉拷锟?strf(锟斤拷锟斤拷式)使STRH_WAVHEADER锟斤拷 */
typedef struct
{
    uint32_t BlockID;           /* 锟斤拷锟街?strf==0X73747266 */
    uint32_t BlockSize;         /* 锟斤拷锟叫?锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟?锟街斤拷,也锟斤拷锟斤拷BlockID锟酵憋拷BlockSize锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷) */
    uint16_t FormatTag;         /* 锟斤拷式锟斤拷志:0X0001=PCM,0X0055=MP3 */
    uint16_t Channels;          /* 锟斤拷锟斤拷锟斤拷,一锟斤拷为2,锟斤拷示锟斤拷锟斤拷锟斤拷 */
    uint32_t SampleRate;        /* 锟斤拷频锟斤拷锟斤拷锟斤拷 */
    uint32_t BaudRate;          /* 锟斤拷锟斤拷锟斤拷 */
    uint16_t BlockAlign;        /* 锟斤拷锟捷匡拷锟斤拷锟斤拷志 */
    uint16_t Size;              /* 锟矫结构锟斤拷小 */
} STRF_WAVHEADER;

#define	 MAKEWORD(ptr)	(uint16_t)(((uint16_t)*((uint8_t*)(ptr))<<8)|(uint16_t)*(uint8_t*)((ptr)+1))
#define  MAKEDWORD(ptr)	(uint32_t)(((uint16_t)*(uint8_t*)(ptr)|(((uint16_t)*(uint8_t*)(ptr+1))<<8)|\
                               (((uint16_t)*(uint8_t*)(ptr+2))<<16)|(((uint16_t)*(uint8_t*)(ptr+3))<<24)))


AVISTATUS avi_init(uint8_t *buf, uint32_t size);                    /* 锟斤拷始锟斤拷avi锟斤拷锟斤拷锟斤拷 */
AVISTATUS avi_parse_audio_format(const uint8_t *buf, uint32_t size,
                                 AVI_INFO *info);
uint32_t avi_srarch_id(uint8_t *buf, uint32_t size, char *id);      /* 锟斤拷锟斤拷ID,ID锟斤拷锟斤拷锟斤拷4锟斤拷锟街节筹拷锟斤拷 */
AVISTATUS avi_get_streaminfo(uint8_t *buf);                         /* 锟斤拷取锟斤拷锟斤拷息 */


#endif






















