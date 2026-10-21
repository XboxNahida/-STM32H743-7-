#ifndef CALCULATOR_CONTROLLER_H
#define CALCULATOR_CONTROLLER_H

#include <stdint.h>

#include "calculator_engine.h"

#define CALCULATOR_RESULT_CAPACITY 64U

typedef struct
{
    char expression[CALCULATOR_EXPRESSION_CAPACITY];
    char result_text[CALCULATOR_RESULT_CAPACITY];
    double memory;
    double last_value;
    uint32_t random_state;
    calculator_angle_mode_t angle_mode;
    int32_t last_result;
    uint8_t memory_valid;
    uint8_t last_value_valid;
    uint8_t second_mode;
    uint8_t just_evaluated;
} calculator_controller_t;

void calculator_controller_init(calculator_controller_t *controller);
int calculator_controller_press(calculator_controller_t *controller,
                                const char *button);

#endif
