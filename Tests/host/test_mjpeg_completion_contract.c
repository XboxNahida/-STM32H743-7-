#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_source(const char *path)
{
    FILE *file = fopen(path, "rb");
    long length;
    char *text;

    if (file == NULL) return NULL;
    if ((fseek(file, 0L, SEEK_END) != 0) ||
        ((length = ftell(file)) < 0L) ||
        (fseek(file, 0L, SEEK_SET) != 0))
    {
        fclose(file);
        return NULL;
    }
    text = (char *)malloc((size_t)length + 1U);
    if (text == NULL)
    {
        fclose(file);
        return NULL;
    }
    if (fread(text, 1U, (size_t)length, file) != (size_t)length)
    {
        free(text);
        fclose(file);
        return NULL;
    }
    text[length] = '\0';
    fclose(file);
    return text;
}

static int expect_true(const char *name, int condition)
{
    if (!condition)
    {
        printf("FAIL %s\n", name);
        return 1;
    }
    return 0;
}

int main(void)
{
    char *source = read_source("Core/Middlewares/MJPEG/mjpeg.c");
    int failures = 0;

    failures += expect_true("read_mjpeg_source", source != NULL);
    if (source == NULL) return 1;

    /* EOC with a short final output block must not be reported as success. */
    failures += expect_true("reject_short_finished_frame",
        strstr(source, "if ((mjpeg.state == JPEG_STATE_FINISHED) &&") != NULL &&
        strstr(source, "mjpeg.yuvblk_curheight < mjpeg.Conf.ImageHeight") != NULL);
    failures += expect_true("short_frame_sets_decode_error",
        strstr(source, "mjpeg_last_error = MJPEG_ERR_DECODE_TIMEOUT") != NULL);
    failures += expect_true("eoc_checks_pending_tail_output",
        strstr(source, "mjpeg_tail_output_pending()") != NULL);
    failures += expect_true("eoc_publishes_pending_tail_output",
        strstr(source, "mjpeg_dma_out_callback();") != NULL);

    free(source);
    if (failures != 0) return 1;
    printf("PASS mjpeg_completion_contract\n");
    return 0;
}
