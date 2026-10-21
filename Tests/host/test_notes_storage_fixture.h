#ifndef TEST_NOTES_STORAGE_FIXTURE_H
#define TEST_NOTES_STORAGE_FIXTURE_H

#include <stdint.h>

#define TEST_SHARED_BYTES 524288U
#define TEST_FILE_BYTES 400000U

extern uint8_t g_shared[TEST_SHARED_BYTES];
extern uint8_t g_file_data[TEST_FILE_BYTES];
extern uint8_t g_written[TEST_FILE_BYTES];
extern uint32_t g_file_size;
extern uint32_t g_shared_size;
extern uint32_t g_written_size;
extern uint32_t g_short_write_limit;
extern int g_acquire_result;
extern int g_open_read_result;
extern int g_open_write_result;
extern int g_read_result;
extern int g_write_result;
extern int g_close_result;
extern uint32_t g_acquire_calls;
extern uint32_t g_release_calls;
extern uint32_t g_open_write_calls;

void reset_fixture(void);
int expect_u32(const char *name, uint32_t actual, uint32_t expected);

#endif
