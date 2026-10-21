#ifndef FILE_MANAGER_TREE_H
#define FILE_MANAGER_TREE_H

#include <stdint.h>

#include "file_manager_service.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FILE_MANAGER_MAX_DEPTH 32U

typedef enum
{
    FILE_MANAGER_TREE_SCAN = 0,
    FILE_MANAGER_TREE_COPY,
    FILE_MANAGER_TREE_DELETE
} file_manager_tree_mode_t;

file_manager_result_t file_manager_tree_begin(file_manager_tree_mode_t mode,
                                              const char *source,
                                              const char *destination);
file_manager_result_t file_manager_tree_step(file_manager_status_t *status);
void file_manager_tree_abort(void);
uint8_t file_manager_tree_active(void);

#ifdef __cplusplus
}
#endif

#endif
