#include <stdio.h>
#include <string.h>

#include "image_viewer_app.h"
#include "sdram_layout.h"

static uint8_t g_fake_buffer[IMAGE_VIEWER_MAX_FILE_SIZE];
static uint8_t g_fake_file_data[IMAGE_VIEWER_MAX_FILE_SIZE];
static uint32_t g_fake_file_size;
static uint32_t g_fake_file_offset;
static uint32_t g_max_read_request;
static uint32_t g_acquire_calls;
static uint32_t g_release_calls;
static uint32_t g_open_calls;
static uint32_t g_close_calls;
static int g_acquire_result;
static int g_read_result;
static int g_decode_result;
static uint32_t g_decode_calls;
static uint16_t g_decode_width;
static uint16_t g_decode_height;
static uint32_t g_decode_target;
static const char *g_dir_names[8];
static uint8_t g_dir_is_directory[8];
static uint32_t g_dir_count;
static uint32_t g_dir_index;
static uint32_t g_dir_read_calls;
static uint32_t g_dir_close_calls;
static char g_last_open_path[IMAGE_VIEWER_PATH_SIZE];

int image_viewer_platform_buffer_acquire(uint32_t size, void **address)
{
    g_acquire_calls++;
    if (g_acquire_result != IMAGE_VIEWER_OK)
    {
        return g_acquire_result;
    }
    if ((size != IMAGE_VIEWER_MAX_FILE_SIZE) || (address == NULL))
    {
        return IMAGE_VIEWER_INVALID_ARG;
    }
    *address = g_fake_buffer;
    return IMAGE_VIEWER_OK;
}

void image_viewer_platform_buffer_release(void)
{
    g_release_calls++;
}

int image_viewer_platform_file_open(const char *path)
{
    g_open_calls++;
    g_fake_file_offset = 0U;
    if (path != NULL)
    {
        (void)snprintf(g_last_open_path, sizeof(g_last_open_path), "%s", path);
    }
    return ((path != NULL) && (path[0] != '\0')) ?
           IMAGE_VIEWER_OK : IMAGE_VIEWER_IO_ERROR;
}

int image_viewer_platform_file_size(uint32_t *size)
{
    if (size == NULL)
    {
        return IMAGE_VIEWER_INVALID_ARG;
    }
    *size = g_fake_file_size;
    return IMAGE_VIEWER_OK;
}

int image_viewer_platform_file_read(uint8_t *buffer,
                                    uint32_t request,
                                    uint32_t *bytes_read)
{
    uint32_t remaining;
    uint32_t count;

    if (g_read_result != IMAGE_VIEWER_OK)
    {
        return g_read_result;
    }
    if ((buffer == NULL) || (bytes_read == NULL))
    {
        return IMAGE_VIEWER_INVALID_ARG;
    }
    if (request > g_max_read_request)
    {
        g_max_read_request = request;
    }
    remaining = g_fake_file_size - g_fake_file_offset;
    count = (request < remaining) ? request : remaining;
    memcpy(buffer, g_fake_file_data + g_fake_file_offset, count);
    g_fake_file_offset += count;
    *bytes_read = count;
    return IMAGE_VIEWER_OK;
}

void image_viewer_platform_file_close(void)
{
    g_close_calls++;
}

int image_viewer_platform_dir_open(const char *path)
{
    g_dir_index = 0U;
    return ((path != NULL) && (path[0] != '\0')) ?
           IMAGE_VIEWER_OK : IMAGE_VIEWER_IO_ERROR;
}

int image_viewer_platform_dir_read(char *name,
                                   uint32_t name_size,
                                   uint8_t *is_directory,
                                   uint8_t *end)
{
    g_dir_read_calls++;
    if ((name == NULL) || (name_size == 0U) ||
        (is_directory == NULL) || (end == NULL))
    {
        return IMAGE_VIEWER_INVALID_ARG;
    }
    if (g_dir_index >= g_dir_count)
    {
        name[0] = '\0';
        *is_directory = 0U;
        *end = 1U;
        return IMAGE_VIEWER_OK;
    }
    (void)snprintf(name, name_size, "%s", g_dir_names[g_dir_index]);
    *is_directory = g_dir_is_directory[g_dir_index];
    *end = 0U;
    g_dir_index++;
    return IMAGE_VIEWER_OK;
}

void image_viewer_platform_dir_close(void)
{
    g_dir_close_calls++;
}

int image_viewer_platform_decode(uint8_t *buffer,
                                 uint32_t size,
                                 uint16_t width,
                                 uint16_t height,
                                 uint32_t target)
{
    (void)buffer;
    (void)size;
    g_decode_calls++;
    g_decode_width = width;
    g_decode_height = height;
    g_decode_target = target;
    return g_decode_result;
}

static void reset_fake_platform(uint32_t file_size)
{
    g_fake_file_size = file_size;
    g_fake_file_offset = 0U;
    g_max_read_request = 0U;
    g_acquire_calls = 0U;
    g_release_calls = 0U;
    g_open_calls = 0U;
    g_close_calls = 0U;
    g_acquire_result = IMAGE_VIEWER_OK;
    g_read_result = IMAGE_VIEWER_OK;
    g_decode_result = IMAGE_VIEWER_OK;
    g_decode_calls = 0U;
    g_decode_width = 0U;
    g_decode_height = 0U;
    g_decode_target = 0U;
    g_dir_count = 0U;
    g_dir_index = 0U;
    g_dir_read_calls = 0U;
    g_dir_close_calls = 0U;
    g_last_open_path[0] = '\0';
    memset(g_fake_file_data, 0xA5, file_size);
}

static void make_fake_jpeg(uint8_t marker,
                           uint16_t width,
                           uint16_t height)
{
    g_fake_file_data[0] = 0xFFU;
    g_fake_file_data[1] = 0xD8U;
    g_fake_file_data[2] = 0xFFU;
    g_fake_file_data[3] = marker;
    g_fake_file_data[4] = 0x00U;
    g_fake_file_data[5] = 0x11U;
    g_fake_file_data[6] = 0x08U;
    g_fake_file_data[7] = (uint8_t)(height >> 8);
    g_fake_file_data[8] = (uint8_t)height;
    g_fake_file_data[9] = (uint8_t)(width >> 8);
    g_fake_file_data[10] = (uint8_t)width;
}

static void poll_image_viewer(uint32_t count)
{
    while (count > 0U)
    {
        image_viewer_app_poll();
        count--;
    }
}

int main(void)
{
    image_viewer_status_t status;
    char long_path[IMAGE_VIEWER_PATH_SIZE + 4U];
    uint32_t cycle;
    int failures = 0;

    image_viewer_app_init();
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_IDLE) ||
        (status.result != IMAGE_VIEWER_OK))
    {
        printf("FAIL initial_state\n");
        failures++;
    }

    if (image_viewer_app_open_file("0:/DCIM/test.jpg") != IMAGE_VIEWER_OK)
    {
        printf("FAIL open_file_request\n");
        failures++;
    }
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_OPENING) ||
        (strcmp(status.display_name, "test.jpg") != 0))
    {
        printf("FAIL opening_state\n");
        failures++;
    }
    if (image_viewer_app_open_album("0:/DCIM/") != IMAGE_VIEWER_BUSY)
    {
        printf("FAIL busy_request\n");
        failures++;
    }

    if ((image_viewer_app_stop() != IMAGE_VIEWER_OK) ||
        (image_viewer_app_stop() != IMAGE_VIEWER_OK))
    {
        printf("FAIL stop_idempotent\n");
        failures++;
    }
    image_viewer_app_poll();
    image_viewer_app_get_status(&status);
    if (status.state != IMAGE_VIEWER_IDLE)
    {
        printf("FAIL stop_cleanup_state\n");
        failures++;
    }

    if (image_viewer_app_open_album("0:/DCIM/") != IMAGE_VIEWER_OK)
    {
        printf("FAIL album_request\n");
        failures++;
    }
    image_viewer_app_get_status(&status);
    if (status.state != IMAGE_VIEWER_SCANNING)
    {
        printf("FAIL scanning_state\n");
        failures++;
    }
    if ((image_viewer_app_next() != IMAGE_VIEWER_NOT_READY) ||
        (image_viewer_app_previous() != IMAGE_VIEWER_NOT_READY))
    {
        printf("FAIL navigation_before_ready\n");
        failures++;
    }
    (void)image_viewer_app_stop();
    image_viewer_app_poll();

    memset(long_path, 'x', sizeof(long_path));
    long_path[sizeof(long_path) - 1U] = '\0';
    if ((image_viewer_app_open_file(NULL) != IMAGE_VIEWER_INVALID_ARG) ||
        (image_viewer_app_open_file("") != IMAGE_VIEWER_INVALID_ARG) ||
        (image_viewer_app_open_file(long_path) != IMAGE_VIEWER_PATH_TOO_LONG))
    {
        printf("FAIL path_validation\n");
        failures++;
    }

    image_viewer_app_init();
    reset_fake_platform(64U);
    g_dir_names[0] = "subdir"; g_dir_is_directory[0] = 1U;
    g_dir_names[1] = "note.txt"; g_dir_is_directory[1] = 0U;
    g_dir_names[2] = "a.JPG"; g_dir_is_directory[2] = 0U;
    g_dir_names[3] = "b.jpeg"; g_dir_is_directory[3] = 0U;
    g_dir_count = 4U;
    (void)image_viewer_app_open_album("0:/DCIM/");
    image_viewer_app_poll();
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_SCANNING) ||
        (g_dir_read_calls != 1U) || (g_acquire_calls != 0U))
    {
        printf("FAIL album_one_entry_per_poll\n");
        failures++;
    }
    image_viewer_app_poll(); image_viewer_app_poll(); image_viewer_app_poll();
    image_viewer_app_poll();
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_OPENING) ||
        (g_dir_read_calls != 5U) || (g_dir_close_calls != 1U))
    {
        printf("FAIL album_scan_complete\n");
        failures++;
    }
    image_viewer_app_poll();
    if ((strcmp(g_last_open_path, "0:/DCIM/a.JPG") != 0) ||
        (g_open_calls != 1U))
    {
        printf("FAIL album_first_jpeg\n");
        failures++;
    }
    (void)image_viewer_app_stop(); image_viewer_app_poll();

    image_viewer_app_init();
    reset_fake_platform(64U);
    (void)image_viewer_app_open_album("0:/EMPTY/");
    image_viewer_app_poll();
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_ERROR) ||
        (status.result != IMAGE_VIEWER_EMPTY_ALBUM) ||
        (g_dir_close_calls != 1U))
    {
        printf("FAIL empty_album\n");
        failures++;
    }

    image_viewer_app_init();
    reset_fake_platform(64U);
    g_dir_names[0] = "a.jpg"; g_dir_count = 1U;
    (void)image_viewer_app_open_album("0:/DCIM/");
    image_viewer_app_poll();
    (void)image_viewer_app_stop(); image_viewer_app_poll();
    if (g_dir_close_calls != 1U)
    {
        printf("FAIL stop_closes_album\n");
        failures++;
    }

    image_viewer_app_init();
    reset_fake_platform(64U);
    make_fake_jpeg(0xC0U, 32U, 32U);
    g_dir_names[0] = "a.jpg";
    g_dir_names[1] = "b.JPEG";
    g_dir_count = 2U;
    (void)image_viewer_app_open_album("0:/DCIM/");
    poll_image_viewer(6U);
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_READY) ||
        (image_viewer_app_next() != IMAGE_VIEWER_OK))
    {
        printf("FAIL album_next_request\n");
        failures++;
    }
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_OPENING) ||
        (strcmp(status.display_name, "b.JPEG") != 0))
    {
        printf("FAIL album_next_path\n");
        failures++;
    }
    image_viewer_app_poll();
    if (strcmp(g_last_open_path, "0:/DCIM/b.JPEG") != 0)
    {
        printf("FAIL album_next_open\n");
        failures++;
    }
    (void)image_viewer_app_stop(); image_viewer_app_poll();

    image_viewer_app_init();
    reset_fake_platform(64U);
    make_fake_jpeg(0xC0U, 32U, 32U);
    g_dir_names[0] = "a.jpg";
    g_dir_names[1] = "b.JPEG";
    g_dir_count = 2U;
    (void)image_viewer_app_open_album("0:/DCIM/");
    poll_image_viewer(6U);
    if (image_viewer_app_previous() != IMAGE_VIEWER_OK)
    {
        printf("FAIL album_previous_request\n");
        failures++;
    }
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_OPENING) ||
        (strcmp(status.display_name, "b.JPEG") != 0))
    {
        printf("FAIL album_previous_wrap\n");
        failures++;
    }
    (void)image_viewer_app_stop(); image_viewer_app_poll();

    image_viewer_app_init();
    reset_fake_platform(70000U);
    (void)image_viewer_app_open_file("0:/DCIM/chunked.jpg");
    image_viewer_app_poll();
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_READING) ||
        (status.file_size != 70000U) ||
        (g_acquire_calls != 1U) || (g_open_calls != 1U))
    {
        printf("FAIL opening_to_reading\n");
        failures++;
    }

    image_viewer_app_poll();
    image_viewer_app_get_status(&status);
    if ((status.bytes_read != IMAGE_VIEWER_READ_CHUNK_SIZE) ||
        (g_max_read_request > IMAGE_VIEWER_READ_CHUNK_SIZE))
    {
        printf("FAIL first_read_chunk\n");
        failures++;
    }
    image_viewer_app_poll();
    image_viewer_app_get_status(&status);
    if (status.bytes_read != (IMAGE_VIEWER_READ_CHUNK_SIZE * 2U))
    {
        printf("FAIL second_read_chunk\n");
        failures++;
    }
    image_viewer_app_poll();
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_DECODING) ||
        (status.bytes_read != 70000U) ||
        (g_close_calls != 1U) || (g_release_calls != 0U))
    {
        printf("FAIL read_complete\n");
        failures++;
    }
    (void)image_viewer_app_stop();
    image_viewer_app_poll();
    if (g_release_calls != 1U)
    {
        printf("FAIL stop_release\n");
        failures++;
    }

    /* A second album image must decode into the alternate RGB buffer so a
     * failed decode cannot overwrite the currently displayed image. */
    image_viewer_app_init();
    reset_fake_platform(64U);
    make_fake_jpeg(0xC0U, 33U, 17U);
    g_dir_names[0] = "first.jpg";
    g_dir_names[1] = "second.jpg";
    g_dir_count = 2U;
    (void)image_viewer_app_open_album("0:/DCIM/");
    poll_image_viewer(6U);
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_READY) ||
        (status.rgb_address != SDRAM_VIDEO_RGB_A_ADDR) ||
        (g_decode_target != SDRAM_VIDEO_RGB_A_ADDR) ||
        (image_viewer_app_next() != IMAGE_VIEWER_OK))
    {
        printf("FAIL first_album_decode_target\n");
        failures++;
    }
    poll_image_viewer(3U);
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_READY) ||
        (status.rgb_address != SDRAM_VIDEO_RGB_B_ADDR) ||
        (g_decode_target != SDRAM_VIDEO_RGB_B_ADDR))
    {
        printf("FAIL alternate_album_decode_target\n");
        failures++;
    }
    (void)image_viewer_app_stop();
    image_viewer_app_poll();

    image_viewer_app_init();
    reset_fake_platform(IMAGE_VIEWER_MAX_FILE_SIZE + 1U);
    (void)image_viewer_app_open_file("0:/DCIM/too_large.jpg");
    image_viewer_app_poll();
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_ERROR) ||
        (status.result != IMAGE_VIEWER_FILE_TOO_LARGE) ||
        (g_close_calls != 1U) || (g_release_calls != 1U))
    {
        printf("FAIL oversized_cleanup\n");
        failures++;
    }

    image_viewer_app_init();
    reset_fake_platform(64U);
    make_fake_jpeg(0xC0U, 33U, 17U);
    (void)image_viewer_app_open_file("0:/DCIM/baseline.jpg");
    image_viewer_app_poll();
    image_viewer_app_poll();
    image_viewer_app_poll();
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_READY) ||
        (status.width != 33U) || (status.height != 17U) ||
        (status.rgb_stride != 96U) ||
        (status.rgb_address != SDRAM_VIDEO_RGB_A_ADDR) ||
        (g_decode_calls != 1U) ||
        (g_decode_width != 48U) || (g_decode_height != 32U) ||
        (g_decode_target != SDRAM_VIDEO_RGB_A_ADDR) ||
        (g_fake_buffer[7] != 0x00U) ||
        (g_fake_buffer[8] != 0x20U) ||
        (g_fake_buffer[9] != 0x00U) ||
        (g_fake_buffer[10] != 0x30U) ||
        (g_release_calls != 1U))
    {
        printf("FAIL baseline_decode\n");
        failures++;
    }

    image_viewer_app_init();
    reset_fake_platform(8U);
    g_fake_file_data[0] = 0xFFU;
    g_fake_file_data[1] = 0xD8U;
    g_fake_file_data[2] = 0xFFU;
    g_fake_file_data[3] = 0xC0U;
    g_fake_file_data[4] = 0x00U;
    g_fake_file_data[5] = 0x11U;
    (void)image_viewer_app_open_file("0:/DCIM/truncated.jpg");
    image_viewer_app_poll();
    image_viewer_app_poll();
    image_viewer_app_poll();
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_ERROR) ||
        (status.result != IMAGE_VIEWER_DECODE_ERROR) ||
        (g_decode_calls != 0U) || (g_release_calls != 1U))
    {
        printf("FAIL truncated_sof_rejected\n");
        failures++;
    }

    image_viewer_app_init();
    reset_fake_platform(64U);
    make_fake_jpeg(0xC0U, 1025U, 600U);
    (void)image_viewer_app_open_file("0:/DCIM/rgb_overflow.jpg");
    image_viewer_app_poll(); image_viewer_app_poll(); image_viewer_app_poll();
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_ERROR) ||
        (status.result != IMAGE_VIEWER_DECODE_ERROR) ||
        (g_decode_calls != 0U) || (g_release_calls != 1U))
    {
        printf("FAIL rgb_buffer_bounds\n");
        failures++;
    }

    image_viewer_app_init();
    reset_fake_platform(64U);
    make_fake_jpeg(0xC2U, 32U, 32U);
    (void)image_viewer_app_open_file("0:/DCIM/progressive.jpg");
    image_viewer_app_poll();
    image_viewer_app_poll();
    image_viewer_app_poll();
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_ERROR) ||
        (status.result != IMAGE_VIEWER_UNSUPPORTED) ||
        (g_decode_calls != 0U) || (g_release_calls != 1U))
    {
        printf("FAIL progressive_rejected\n");
        failures++;
    }

    image_viewer_app_init();
    reset_fake_platform(64U);
    make_fake_jpeg(0xC0U, 32U, 32U);
    g_decode_result = IMAGE_VIEWER_DECODE_ERROR;
    (void)image_viewer_app_open_file("0:/DCIM/decode_error.jpg");
    image_viewer_app_poll();
    image_viewer_app_poll();
    image_viewer_app_poll();
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_ERROR) ||
        (status.result != IMAGE_VIEWER_DECODE_ERROR) ||
        (g_decode_calls != 1U) || (g_release_calls != 1U))
    {
        printf("FAIL decode_error_cleanup\n");
        failures++;
    }

    image_viewer_app_init();
    reset_fake_platform(100U);
    g_acquire_result = IMAGE_VIEWER_BUSY;
    (void)image_viewer_app_open_file("0:/DCIM/busy.jpg");
    image_viewer_app_poll();
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_ERROR) ||
        (status.result != IMAGE_VIEWER_BUSY) ||
        (g_open_calls != 0U) || (g_release_calls != 0U))
    {
        printf("FAIL buffer_busy\n");
        failures++;
    }

    image_viewer_app_init();
    reset_fake_platform(100U);
    (void)image_viewer_app_open_file("0:/DCIM/read_error.jpg");
    image_viewer_app_poll();
    g_read_result = IMAGE_VIEWER_IO_ERROR;
    image_viewer_app_poll();
    image_viewer_app_get_status(&status);
    if ((status.state != IMAGE_VIEWER_ERROR) ||
        (status.result != IMAGE_VIEWER_IO_ERROR) ||
        (g_close_calls != 1U) || (g_release_calls != 1U))
    {
        printf("FAIL read_error_cleanup\n");
        failures++;
    }

    for (cycle = 0U; cycle < 50U; cycle++)
    {
        image_viewer_app_init();
        reset_fake_platform(64U);
        make_fake_jpeg(0xC0U, 32U, 32U);
        (void)image_viewer_app_open_file("0:/DCIM/cycle.jpg");
        image_viewer_app_poll();
        image_viewer_app_poll();
        (void)image_viewer_app_stop();
        image_viewer_app_poll();
        image_viewer_app_get_status(&status);
        if ((status.state != IMAGE_VIEWER_IDLE) ||
            (g_acquire_calls != 1U) || (g_release_calls != 1U) ||
            (g_close_calls != 1U))
        {
            printf("FAIL repeated_decode_stop cycle=%lu\n",
                   (unsigned long)cycle);
            failures++;
            break;
        }
    }

    if (failures != 0)
    {
        printf("FAIL image_viewer_app failures=%d\n", failures);
        return 1;
    }

    printf("PASS image_viewer_app\n");
    return 0;
}
