#ifndef CALCULATOR_ENGINE_H
#define CALCULATOR_ENGINE_H

#include <stdint.h>

#define CALCULATOR_EXPRESSION_CAPACITY 128U

typedef enum
{
    CALCULATOR_OK = 0,
    CALCULATOR_INVALID_ARG,
    CALCULATOR_SYNTAX_ERROR,
    CALCULATOR_DOMAIN_ERROR,
    CALCULATOR_DIVIDE_BY_ZERO,
    CALCULATOR_OVERFLOW
} calculator_result_t;

typedef enum
{
    CALCULATOR_ANGLE_DEG = 0,
    CALCULATOR_ANGLE_RAD
} calculator_angle_mode_t;

int calculator_evaluate(const char *expression,
                        calculator_angle_mode_t angle_mode,
                        double *value);

#endif
