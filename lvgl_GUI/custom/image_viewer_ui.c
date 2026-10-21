#include "image_viewer_ui.h"

#include <stdio.h>

#include "custom.h"
#include "image_viewer_app.h"

extern lv_image_dsc_t g_img_dsc;
extern uint32_t g_img_zoom;
extern uint32_t g_img_fit_zoom;
extern uint32_t g_img_saved_zoom;
extern int32_t g_pan_x;
extern int32_t g_pan_y;
extern void image_viewer_reposition(void);

static image_viewer_state_t g_last_state;
static int32_t g_last_result;
static uint32_t g_last_rgb_address;

static int image_viewer_ui_screen_valid(void)
{
    return ((guider_ui.screen_image_viewer != NULL) &&
            lv_obj_is_valid(guider_ui.screen_image_viewer)) ? 1 : 0;
}

static int image_viewer_ui_can_open(void)
{
    image_viewer_status_t status;

    image_viewer_app_get_status(&status);
    if (status.state != IMAGE_VIEWER_IDLE)
    {
        return IMAGE_VIEWER_BUSY;
    }
    if ((guider_ui.DeskTop == NULL) ||
        (lv_obj_is_valid(guider_ui.DeskTop) == false) ||
        (lv_screen_active() != guider_ui.DeskTop))
    {
        return IMAGE_VIEWER_NOT_READY;
    }
    return IMAGE_VIEWER_OK;
}

static void image_viewer_ui_load_screen(void)
{
    ui_load_scr_animation(&guider_ui, &guider_ui.screen_image_viewer,
        guider_ui.screen_image_viewer_del, &guider_ui.DeskTop_del,
        setup_scr_screen_image_viewer, LV_SCR_LOAD_ANIM_NONE,
        200, 200, true, true);
}

static void image_viewer_ui_show_ready(const image_viewer_status_t *status)
{
    uint32_t aligned_height;
    uint32_t scale_x;
    uint32_t scale_y;
    char label[300];

    if ((status == NULL) || (status->width == 0U) ||
        (status->height == 0U) || (status->rgb_stride == 0U) ||
        (image_viewer_ui_screen_valid() == 0))
    {
        return;
    }

    aligned_height = ((uint32_t)status->height + 15U) & ~15UL;
    g_img_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    g_img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    g_img_dsc.header.w = status->width;
    g_img_dsc.header.h = status->height;
    g_img_dsc.header.stride = status->rgb_stride;
    g_img_dsc.data_size = status->rgb_stride * aligned_height;
    g_img_dsc.data = (const uint8_t *)status->rgb_address;

    lv_image_set_src(guider_ui.screen_image_viewer_img, &g_img_dsc);
    scale_x = (1024UL * 256UL) / status->width;
    scale_y = (600UL * 256UL) / status->height;
    g_img_fit_zoom = (scale_x < scale_y) ? scale_x : scale_y;
    g_img_zoom = (g_img_fit_zoom > 256UL) ? 256UL : g_img_fit_zoom;
    g_img_saved_zoom = g_img_zoom;
    g_pan_x = 0;
    g_pan_y = 0;
    image_viewer_reposition();

    (void)snprintf(label, sizeof(label), "%s (%luB)",
                   status->display_name,
                   (unsigned long)status->file_size);
    lv_label_set_text(guider_ui.screen_image_viewer_label_2, label);
    lv_obj_remove_flag(guider_ui.screen_image_viewer_cont_2,
                       LV_OBJ_FLAG_HIDDEN);
}

static void image_viewer_ui_show_error(int32_t result)
{
    const char *message = "Image error";

    if (image_viewer_ui_screen_valid() == 0)
    {
        return;
    }
    if (result == IMAGE_VIEWER_EMPTY_ALBUM) message = "Album is empty";
    else if (result == IMAGE_VIEWER_FILE_TOO_LARGE) message = "Image exceeds 512KB";
    else if (result == IMAGE_VIEWER_UNSUPPORTED) message = "Progressive JPEG unsupported";
    else if (result == IMAGE_VIEWER_BUSY) message = "Image buffer busy";
    else if (result == IMAGE_VIEWER_IO_ERROR) message = "Image I/O error";
    else if (result == IMAGE_VIEWER_DECODE_ERROR) message = "JPEG decode error";
    lv_label_set_text(guider_ui.screen_image_viewer_label_2, message);
}

void image_viewer_ui_init(void)
{
    image_viewer_app_init();
    g_last_state = IMAGE_VIEWER_IDLE;
    g_last_result = IMAGE_VIEWER_OK;
    g_last_rgb_address = 0U;
}

void image_viewer_ui_poll(void)
{
    image_viewer_status_t status;

    image_viewer_app_poll();
    image_viewer_app_get_status(&status);
    if ((status.state == IMAGE_VIEWER_READY) &&
        ((g_last_state != IMAGE_VIEWER_READY) ||
         (g_last_rgb_address != status.rgb_address)))
    {
        image_viewer_ui_show_ready(&status);
    }
    else if ((status.state == IMAGE_VIEWER_ERROR) &&
             ((g_last_state != IMAGE_VIEWER_ERROR) ||
              (g_last_result != status.result)))
    {
        image_viewer_ui_show_error(status.result);
    }
    g_last_state = status.state;
    g_last_result = status.result;
    g_last_rgb_address = status.rgb_address;
}

int image_viewer_ui_open_album(const char *directory)
{
    int result = image_viewer_ui_can_open();
    if (result != IMAGE_VIEWER_OK) return result;
    result = image_viewer_app_open_album(directory);
    if (result == IMAGE_VIEWER_OK) image_viewer_ui_load_screen();
    return result;
}

int image_viewer_ui_open_file(const char *path)
{
    int result = image_viewer_ui_can_open();
    if (result != IMAGE_VIEWER_OK) return result;
    result = image_viewer_app_open_file(path);
    if (result == IMAGE_VIEWER_OK) image_viewer_ui_load_screen();
    return result;
}

int image_viewer_ui_open_unsupported(const char *path)
{
    int result = image_viewer_ui_can_open();
    if ((result != IMAGE_VIEWER_OK) || (path == NULL)) return result;
    image_viewer_ui_load_screen();
    if (image_viewer_ui_screen_valid() != 0)
        lv_label_set_text(guider_ui.screen_image_viewer_label_2,
                          "当前暂不支持此图片格式预览");
    return IMAGE_VIEWER_OK;
}

int image_viewer_ui_next(void) { return image_viewer_app_next(); }
int image_viewer_ui_previous(void) { return image_viewer_app_previous(); }

void image_viewer_ui_stop(void)
{
    (void)image_viewer_app_stop();
}
