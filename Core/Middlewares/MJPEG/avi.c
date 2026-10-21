/**
 ****************************************************************************************************
 * @file        audioplay.c
 * @author      姝ｇ偣鍘熷瓙鍥㈤槦(ALIENTEK)
 * @version     V1.0
 * @date        2023-01-05
 * @brief       AVI瑙嗛鏍煎紡瑙ｆ瀽 浠ｇ爜
 * @license     Copyright (c) 2020-2032, 骞垮窞甯傛槦缈肩數瀛愮鎶€鏈夐檺鍏徃
 ****************************************************************************************************
 * @attention
 *
 * 瀹為獙骞冲彴:姝ｇ偣鍘熷瓙 闃挎尝缃?F429寮€鍙戞澘
 * 鍦ㄧ嚎瑙嗛:www.yuanzige.com
 * 鎶€鏈鍧?www.openedv.com
 * 鍏徃缃戝潃:www.alientek.com
 * 璐拱鍦板潃:openedv.taobao.com
 *
 * 淇敼璇存槑
 * V1.0 20230105
 * 绗竴娆″彂甯?
 *
 ****************************************************************************************************
 */

#include "avi.h"
#include "mjpeg.h"
#include <stdio.h>


AVI_INFO g_avix;                                        /* avi鏂囦欢鐩稿叧淇℃伅 */
char *const AVI_VIDS_FLAG_TBL[2] = {"00dc", "01dc"};    /* 瑙嗛缂栫爜鏍囧織瀛楃涓?00dc/01dc */
char *const AVI_AUDS_FLAG_TBL[2] = {"00wb", "01wb"};    /* 闊抽缂栫爜鏍囧織瀛楃涓?00wb/01wb */

static uint16_t avi_read_u16_le(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] |
                      ((uint16_t)data[1] << 8));
}

static uint32_t avi_read_u32_le(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

AVISTATUS avi_parse_audio_format(const uint8_t *buf, uint32_t size,
                                 AVI_INFO *info)
{
    uint32_t payload_size;

    if ((buf == NULL) || (info == NULL) || (size < 24U))
    {
        return AVI_STRF_ERR;
    }
    if (avi_read_u32_le(buf) != AVI_STRF_ID)
    {
        return AVI_STRF_ERR;
    }
    payload_size = avi_read_u32_le(buf + 4U);
    if ((payload_size < 16U) || (payload_size > (size - 8U)))
    {
        return AVI_STRF_ERR;
    }
    info->AudioType = avi_read_u16_le(buf + 8U);
    info->Channels = avi_read_u16_le(buf + 10U);
    info->SampleRate = avi_read_u32_le(buf + 12U);
    info->AudioByteRate = avi_read_u32_le(buf + 16U);
    info->AudioBlockAlign = avi_read_u16_le(buf + 20U);
    info->BitsPerSample = avi_read_u16_le(buf + 22U);
    return AVI_OK;
}


/**
 * @brief       AVI瑙ｇ爜鍒濆鍖?
 * @param       buf  : 杈撳叆缂撳啿鍖?
 * @param       size : 缂撳啿鍖哄ぇ灏?
 * @retval      鎵ц缁撴灉
 *   @arg       AVI_OK, AVI鏂囦欢瑙ｆ瀽鎴愬姛
 *   @arg       鍏朵粬  , 閿欒浠ｇ爜
 */
AVISTATUS avi_init(uint8_t *buf, uint32_t size)
{
    uint16_t offset;
    uint8_t *tbuf;
    AVISTATUS res = AVI_OK;
    AVI_HEADER *aviheader;
    LIST_HEADER *listheader;
    AVIH_HEADER *avihheader;
    STRH_HEADER *strhheader;

    STRF_BMPHEADER *bmpheader;
    STRF_WAVHEADER *wavheader;

    tbuf = buf;
    aviheader = (AVI_HEADER *)buf;

    if (aviheader->RiffID != AVI_RIFF_ID)return AVI_RIFF_ERR;   /* RIFF ID閿欒 */

    if (aviheader->AviID != AVI_AVI_ID)return AVI_AVI_ERR;      /* AVI ID閿欒 */

    buf += sizeof(AVI_HEADER);  /* 鍋忕Щ */
    listheader = (LIST_HEADER *)(buf);

    if (listheader->ListID != AVI_LIST_ID)return AVI_LIST_ERR;  /* LIST ID閿欒 */

    if (listheader->ListType != AVI_HDRL_ID)return AVI_HDRL_ERR;/* HDRL ID閿欒 */

    buf += sizeof(LIST_HEADER); /* 鍋忕Щ */
    avihheader = (AVIH_HEADER *)(buf);

    if (avihheader->BlockID != AVI_AVIH_ID)return AVI_AVIH_ERR; /* AVIH ID閿欒 */

    g_avix.SecPerFrame = avihheader->SecPerFrame;   /* 寰楀埌甯ч棿闅旀椂闂?*/
    g_avix.TotalFrame = avihheader->TotalFrame;     /* 寰楀埌鎬诲抚鏁?*/
    buf += avihheader->BlockSize + 8;               /* 鍋忕Щ */
    listheader = (LIST_HEADER *)(buf);

    if (listheader->ListID != AVI_LIST_ID)return AVI_LIST_ERR;  /* LIST ID閿欒 */

    if (listheader->ListType != AVI_STRL_ID)return AVI_STRL_ERR;/* STRL ID閿欒 */

    strhheader = (STRH_HEADER *)(buf + 12);

    if (strhheader->BlockID != AVI_STRH_ID)return AVI_STRH_ERR; /* STRH ID閿欒 */

    if (strhheader->StreamType == AVI_VIDS_STREAM)              /* 瑙嗛甯у湪鍓?*/
    {
        if (strhheader->Handler != AVI_FORMAT_MJPG)return AVI_FORMAT_ERR;   /* 闈濵JPG瑙嗛娴?涓嶆敮鎸?*/

        g_avix.VideoFLAG = AVI_VIDS_FLAG_TBL[0];       /* 瑙嗛娴佹爣璁? "00dc" */
        g_avix.AudioFLAG = AVI_AUDS_FLAG_TBL[1];       /* 闊抽娴佹爣璁? "01wb" */
        bmpheader = (STRF_BMPHEADER *)(buf + 12 + strhheader->BlockSize + 8);   /* strf */

        if (bmpheader->BlockID != AVI_STRF_ID)return AVI_STRF_ERR;  /* STRF ID閿欒 */

        g_avix.Width = bmpheader->bmiHeader.Width;
        g_avix.Height = bmpheader->bmiHeader.Height;
        buf += listheader->BlockSize + 8;       /* 鍋忕Щ */
        listheader = (LIST_HEADER *)(buf);

        if (listheader->ListID != AVI_LIST_ID)  /* 鏄笉鍚湁闊抽甯х殑瑙嗛鏂囦欢 */
        {
            g_avix.SampleRate = 0;    /* 闊抽閲囨牱鐜?*/
            g_avix.AudioByteRate = 0;
            g_avix.Channels = 0;      /* 闊抽閫氶亾鏁?*/
            g_avix.AudioBlockAlign = 0;
            g_avix.BitsPerSample = 0;
            g_avix.AudioType = 0;     /* 闊抽鏍煎紡 */
        }
        else
        {
            if (listheader->ListType != AVI_STRL_ID)return AVI_STRL_ERR;    /* STRL ID閿欒 */

            strhheader = (STRH_HEADER *)(buf + 12);

            if (strhheader->BlockID != AVI_STRH_ID)return AVI_STRH_ERR;     /* STRH ID閿欒 */

            if (strhheader->StreamType != AVI_AUDS_STREAM)return AVI_FORMAT_ERR;    /* 鏍煎紡閿欒 */

            wavheader = (STRF_WAVHEADER *)(buf + 12 + strhheader->BlockSize + 8);   /* strf */

            if (wavheader->BlockID != AVI_STRF_ID)return AVI_STRF_ERR;  /* STRF ID閿欒 */

            if (avi_parse_audio_format((const uint8_t *)wavheader,
                                       wavheader->BlockSize + 8U,
                                       &g_avix) != AVI_OK)
            {
                return AVI_STRF_ERR;
            }
        }
    }
    else if (strhheader->StreamType == AVI_AUDS_STREAM) /* 闊抽甯у湪鍓?*/
    {
        g_avix.VideoFLAG = AVI_VIDS_FLAG_TBL[1];        /* 瑙嗛娴佹爣璁? "01dc" */
        g_avix.AudioFLAG = AVI_AUDS_FLAG_TBL[0];        /* 闊抽娴佹爣璁? "00wb" */
        wavheader = (STRF_WAVHEADER *)(buf + 12 + strhheader->BlockSize + 8);   /* strf */

        if (wavheader->BlockID != AVI_STRF_ID)return AVI_STRF_ERR;  /* STRF ID閿欒 */

        if (avi_parse_audio_format((const uint8_t *)wavheader,
                                   wavheader->BlockSize + 8U,
                                   &g_avix) != AVI_OK)
        {
            return AVI_STRF_ERR;
        }
        buf += listheader->BlockSize + 8;               /* 鍋忕Щ */
        listheader = (LIST_HEADER *)(buf);

        if (listheader->ListID != AVI_LIST_ID)return AVI_LIST_ERR;      /* LIST ID閿欒 */

        if (listheader->ListType != AVI_STRL_ID)return AVI_STRL_ERR;    /* STRL ID閿欒 */

        strhheader = (STRH_HEADER *)(buf + 12);

        if (strhheader->BlockID != AVI_STRH_ID)return AVI_STRH_ERR;     /* STRH ID閿欒 */

        if (strhheader->StreamType != AVI_VIDS_STREAM)return AVI_FORMAT_ERR;    /* 鏍煎紡閿欒 */

        bmpheader = (STRF_BMPHEADER *)(buf + 12 + strhheader->BlockSize + 8);   /* strf */

        if (bmpheader->BlockID != AVI_STRF_ID)return AVI_STRF_ERR;      /* STRF ID閿欒 */

        if (bmpheader->bmiHeader.Compression != AVI_FORMAT_MJPG)return AVI_FORMAT_ERR;  /* 鏍煎紡閿欒 */

        g_avix.Width = bmpheader->bmiHeader.Width;
        g_avix.Height = bmpheader->bmiHeader.Height;
    }

    offset = avi_srarch_id(tbuf, size, "movi");     /* 鏌ユ壘movi ID */

    if (offset == 0)return AVI_MOVI_ERR;            /* MOVI ID閿欒 */

    if (g_avix.SampleRate)    /* 鏈夐煶棰戞祦,鎵嶆煡鎵?*/
    {
        tbuf += offset;
        offset = avi_srarch_id(tbuf, size, g_avix.AudioFLAG); /* 鏌ユ壘闊抽娴佹爣璁?*/

        if (offset == 0)return AVI_STREAM_ERR;      /* 娴侀敊璇?*/

        tbuf += offset + 4;
        g_avix.AudioBufSize = *((uint16_t *)tbuf);    /* 寰楀埌闊抽娴乥uf澶у皬 */
    }

    printf("avi init ok\r\n");
    printf("g_avix.SecPerFrame:%d\r\n", g_avix.SecPerFrame);
    printf("g_avix.TotalFrame:%d\r\n", g_avix.TotalFrame);
    printf("g_avix.Width:%d\r\n", g_avix.Width);
    printf("g_avix.Height:%d\r\n", g_avix.Height);
    printf("g_avix.AudioType:%d\r\n", g_avix.AudioType);
    printf("g_avix.SampleRate:%d\r\n", g_avix.SampleRate);
    printf("g_avix.Channels:%d\r\n", g_avix.Channels);
    printf("g_avix.AudioBufSize:%d\r\n", g_avix.AudioBufSize);
    printf("g_avix.VideoFLAG:%s\r\n", g_avix.VideoFLAG);
    printf("g_avix.AudioFLAG:%s\r\n", g_avix.AudioFLAG);
    return res;
}

/**
 * @brief       鏌ユ壘 ID
 * @param       buf  : 杈撳叆缂撳啿鍖?
 * @param       size : 缂撳啿鍖哄ぇ灏?
 * @param       id   : 瑕佹煡鎵剧殑id, 蹇呴』鏄?瀛楄妭闀垮害
 * @retval      鎵ц缁撴灉
 *   @arg       0     , 娌℃壘鍒?
 *   @arg       鍏朵粬  , movi ID鍋忕Щ閲?
 */
uint32_t avi_srarch_id(uint8_t *buf, uint32_t size, char *id)
{
    uint32_t i;
    uint32_t idsize = 0;
    size -= 4;
    for (i = 0; i < size; i++)
    {
        if ((buf[i] == id[0]) &&
            (buf[i + 1] == id[1]) &&
            (buf[i + 2] == id[2]) &&
            (buf[i + 3] == id[3]))
        {
            idsize = MAKEDWORD(buf + i + 4);    /* 寰楀埌甯уぇ灏?蹇呴』澶т簬16瀛楄妭,鎵嶈繑鍥?鍚﹀垯涓嶆槸鏈夋晥鏁版嵁 */

            if (idsize > 0X10)return i;         /* 鎵惧埌"id"鎵€鍦ㄧ殑浣嶇疆 */
        }
    }

    return 0;
}

/**
 * @brief       寰楀埌stream娴佷俊鎭?
 * @param       buf  : 娴佸紑濮嬪湴鍧€(蹇呴』鏄?1wb/00wb/01dc/00dc寮€澶?
 * @retval      鎵ц缁撴灉
 *   @arg       AVI_OK, AVI鏂囦欢瑙ｆ瀽鎴愬姛
 *   @arg       鍏朵粬  , 閿欒浠ｇ爜
 */
AVISTATUS avi_get_streaminfo(uint8_t *buf)
{
    g_avix.StreamID = MAKEWORD(buf + 2);          /* 寰楀埌娴佺被鍨?*/
    g_avix.StreamSize = MAKEDWORD(buf + 4);       /* 寰楀埌娴佸ぇ灏?*/

    if (g_avix.StreamSize > AVI_MAX_FRAME_SIZE)   /* 甯уぇ灏忓お澶т簡,鐩存帴杩斿洖閿欒 */
    {
        printf("FRAME SIZE OVER:%d\r\n", g_avix.StreamSize);
        g_avix.StreamSize = 0;
        return AVI_STREAM_ERR;
    }

    if (g_avix.StreamSize % 2)
    {
        g_avix.StreamSize++;  /* 濂囨暟鍔?(g_avix.StreamSize,蹇呴』鏄伓鏁? */
    }

    if (g_avix.StreamID == AVI_VIDS_FLAG || g_avix.StreamID == AVI_AUDS_FLAG)
    {
        return AVI_OK;
    }

    return AVI_STREAM_ERR;
}




















