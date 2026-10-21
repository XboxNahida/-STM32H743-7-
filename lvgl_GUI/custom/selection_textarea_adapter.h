#ifndef SELECTION_TEXTAREA_ADAPTER_H
#define SELECTION_TEXTAREA_ADAPTER_H

#include "lvgl.h"
#include "text_selection_controller.h"

typedef void (*selection_textarea_delete_cb_t)(lv_obj_t *textarea, void *user_data);
typedef void (*selection_textarea_result_cb_t)(text_selection_result_t result,
                                               void *user_data);

const text_selection_target_ops_t *selection_textarea_adapter_ops(void);
void selection_textarea_adapter_set_callbacks(selection_textarea_delete_cb_t delete_cb,
                                              selection_textarea_result_cb_t result_cb,
                                              void *user_data);
bool selection_textarea_adapter_attach(lv_obj_t *textarea);
void selection_textarea_adapter_detach(lv_obj_t *textarea);
void selection_textarea_adapter_focus(lv_obj_t *textarea,
                                      text_selection_controller_t *controller);
void selection_textarea_adapter_blur(lv_obj_t *textarea);

#endif
