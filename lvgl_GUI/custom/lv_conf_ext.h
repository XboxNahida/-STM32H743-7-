/*
* Copyright 2024 NXP
* NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
* accordance with the applicable license terms. By expressly accepting such terms or by downloading, installing,
* activating and/or otherwise using the software, you are agreeing that you have read, and that you agree to
* comply with and are bound by, such license terms.  If you do not agree to be bound by the applicable license
* terms, then you may not retain, install, activate or otherwise use the software.
*/

/*
 * lv_conf_ext.h for custom lvconf file.
 * example :
 *	#undef LV_FONT_FMT_TXT_LARGE
 *  #define LV_FONT_FMT_TXT_LARGE 1
 */

#ifndef LV_CONF_EXT_H
#define LV_CONF_EXT_H


/* common code  begin  */

/*
 * LVGL display flush mode:
 *   1 = DMA2D hardware accelerated (recommended, faster)
 *   0 = CPU memcpy (original mode, no DMA2D dependency)
 */
#define LV_USE_DMA2D_FLUSH  0

/*
 * LTDC frame-buffer mode:
 *   1 = two full-screen RGB565 buffers with VBlank address switching
 *   0 = legacy 60-line partial buffer copied into the active LTDC frame
 */
#define LV_USE_LTDC_FULL_FRAME_BUFFER  1

/* common code end */


#if LV_USE_GUIDER_SIMULATOR
/* code for simulator begin  */


/* code for simulator end */
#else
/* code for board begin */


/* code for board end */	
#endif



#endif  /* LV_CONF_EXT_H */	
