#include "calculator_engine.h"

#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define CALCULATOR_PI 3.14159265358979323846
#define CALCULATOR_ZERO_EPSILON 1.0e-15
#define CALCULATOR_TAN_EPSILON  1.0e-12

typedef struct
{
    double number;
    uint8_t percent;
} calculator_value_t;

typedef struct
{
    const char *cursor;
    calculator_angle_mode_t angle_mode;
    int error;
} calculator_parser_t;

static calculator_value_t calculator_parse_expression(
    calculator_parser_t *parser);
static calculator_value_t calculator_parse_unary(calculator_parser_t *parser);

static calculator_value_t calculator_value(double number)
{
    calculator_value_t value;

    value.number = number;
    value.percent = 0U;
    return value;
}

static void calculator_set_error(calculator_parser_t *parser, int error)
{
    if ((parser != NULL) && (parser->error == CALCULATOR_OK))
    {
        parser->error = error;
    }
}

static void calculator_skip_spaces(calculator_parser_t *parser)
{
    while (isspace((unsigned char)*parser->cursor) != 0)
    {
        parser->cursor++;
    }
}

static uint8_t calculator_match_char(calculator_parser_t *parser,
                                     char expected)
{
    calculator_skip_spaces(parser);
    if (*parser->cursor != expected)
    {
        return 0U;
    }
    parser->cursor++;
    return 1U;
}

static uint8_t calculator_match_word(calculator_parser_t *parser,
                                     const char *word)
{
    const char *start;
    size_t length;

    calculator_skip_spaces(parser);
    start = parser->cursor;
    length = strlen(word);
    if ((strncmp(start, word, length) != 0) ||
        (isalpha((unsigned char)start[length]) != 0))
    {
        return 0U;
    }
    parser->cursor += length;
    return 1U;
}

static double calculator_to_radians(double value,
                                    calculator_angle_mode_t angle_mode)
{
    return (angle_mode == CALCULATOR_ANGLE_DEG) ?
        value * CALCULATOR_PI / 180.0 : value;
}

static double calculator_from_radians(double value,
                                      calculator_angle_mode_t angle_mode)
{
    return (angle_mode == CALCULATOR_ANGLE_DEG) ?
        value * 180.0 / CALCULATOR_PI : value;
}

static calculator_value_t calculator_checked_value(
    calculator_parser_t *parser, double number)
{
    if (isnan(number) != 0)
    {
        calculator_set_error(parser, CALCULATOR_DOMAIN_ERROR);
        return calculator_value(0.0);
    }
    if (isfinite(number) == 0)
    {
        calculator_set_error(parser, CALCULATOR_OVERFLOW);
        return calculator_value(0.0);
    }
    return calculator_value(number);
}

static calculator_value_t calculator_apply_function(
    calculator_parser_t *parser, const char *name, double argument)
{
    double radians;
    double result;

    if (strcmp(name, "sin") == 0)
    {
        result = sin(calculator_to_radians(argument, parser->angle_mode));
    }
    else if (strcmp(name, "cos") == 0)
    {
        result = cos(calculator_to_radians(argument, parser->angle_mode));
    }
    else if (strcmp(name, "tan") == 0)
    {
        radians = calculator_to_radians(argument, parser->angle_mode);
        if (fabs(cos(radians)) < CALCULATOR_TAN_EPSILON)
        {
            calculator_set_error(parser, CALCULATOR_DOMAIN_ERROR);
            return calculator_value(0.0);
        }
        result = tan(radians);
    }
    else if (strcmp(name, "asin") == 0)
    {
        if ((argument < -1.0) || (argument > 1.0))
        {
            calculator_set_error(parser, CALCULATOR_DOMAIN_ERROR);
            return calculator_value(0.0);
        }
        result = calculator_from_radians(asin(argument), parser->angle_mode);
    }
    else if (strcmp(name, "acos") == 0)
    {
        if ((argument < -1.0) || (argument > 1.0))
        {
            calculator_set_error(parser, CALCULATOR_DOMAIN_ERROR);
            return calculator_value(0.0);
        }
        result = calculator_from_radians(acos(argument), parser->angle_mode);
    }
    else if (strcmp(name, "atan") == 0)
    {
        result = calculator_from_radians(atan(argument), parser->angle_mode);
    }
    else if (strcmp(name, "sinh") == 0)
    {
        result = sinh(argument);
    }
    else if (strcmp(name, "cosh") == 0)
    {
        result = cosh(argument);
    }
    else if (strcmp(name, "tanh") == 0)
    {
        result = tanh(argument);
    }
    else if (strcmp(name, "asinh") == 0)
    {
        result = (argument < 0.0) ?
            -log(-argument + sqrt(argument * argument + 1.0)) :
            log(argument + sqrt(argument * argument + 1.0));
    }
    else if (strcmp(name, "acosh") == 0)
    {
        if (argument < 1.0)
        {
            calculator_set_error(parser, CALCULATOR_DOMAIN_ERROR);
            return calculator_value(0.0);
        }
        result = log(argument + sqrt(argument * argument - 1.0));
    }
    else if (strcmp(name, "atanh") == 0)
    {
        if ((argument <= -1.0) || (argument >= 1.0))
        {
            calculator_set_error(parser, CALCULATOR_DOMAIN_ERROR);
            return calculator_value(0.0);
        }
        result = 0.5 * log((1.0 + argument) / (1.0 - argument));
    }
    else if (strcmp(name, "sqrt") == 0)
    {
        if (argument < 0.0)
        {
            calculator_set_error(parser, CALCULATOR_DOMAIN_ERROR);
            return calculator_value(0.0);
        }
        result = sqrt(argument);
    }
    else if (strcmp(name, "ln") == 0)
    {
        if (argument <= 0.0)
        {
            calculator_set_error(parser, CALCULATOR_DOMAIN_ERROR);
            return calculator_value(0.0);
        }
        result = log(argument);
    }
    else if (strcmp(name, "lg") == 0)
    {
        if (argument <= 0.0)
        {
            calculator_set_error(parser, CALCULATOR_DOMAIN_ERROR);
            return calculator_value(0.0);
        }
        result = log10(argument);
    }
    else if (strcmp(name, "exp") == 0)
    {
        result = exp(argument);
    }
    else
    {
        calculator_set_error(parser, CALCULATOR_SYNTAX_ERROR);
        return calculator_value(0.0);
    }
    return calculator_checked_value(parser, result);
}

static calculator_value_t calculator_parse_primary(
    calculator_parser_t *parser)
{
    calculator_value_t value;
    char identifier[8];
    char *number_end;
    size_t length = 0U;

    calculator_skip_spaces(parser);
    if (calculator_match_char(parser, '(') != 0U)
    {
        value = calculator_parse_expression(parser);
        if (calculator_match_char(parser, ')') == 0U)
        {
            calculator_set_error(parser, CALCULATOR_SYNTAX_ERROR);
        }
        return value;
    }
    if ((isdigit((unsigned char)*parser->cursor) != 0) ||
        (*parser->cursor == '.'))
    {
        value.number = strtod(parser->cursor, &number_end);
        value.percent = 0U;
        if (number_end == parser->cursor)
        {
            calculator_set_error(parser, CALCULATOR_SYNTAX_ERROR);
            return calculator_value(0.0);
        }
        parser->cursor = number_end;
        return calculator_checked_value(parser, value.number);
    }
    while ((isalpha((unsigned char)*parser->cursor) != 0) &&
           (length + 1U < sizeof(identifier)))
    {
        identifier[length++] = (char)tolower((unsigned char)*parser->cursor);
        parser->cursor++;
    }
    identifier[length] = '\0';
    if (length == 0U)
    {
        calculator_set_error(parser, CALCULATOR_SYNTAX_ERROR);
        return calculator_value(0.0);
    }
    if (strcmp(identifier, "pi") == 0)
    {
        return calculator_value(CALCULATOR_PI);
    }
    if (strcmp(identifier, "e") == 0)
    {
        return calculator_value(exp(1.0));
    }
    if (calculator_match_char(parser, '(') == 0U)
    {
        calculator_set_error(parser, CALCULATOR_SYNTAX_ERROR);
        return calculator_value(0.0);
    }
    value = calculator_parse_expression(parser);
    if (calculator_match_char(parser, ')') == 0U)
    {
        calculator_set_error(parser, CALCULATOR_SYNTAX_ERROR);
        return calculator_value(0.0);
    }
    return calculator_apply_function(parser, identifier, value.number);
}

static calculator_value_t calculator_parse_postfix(
    calculator_parser_t *parser)
{
    calculator_value_t value = calculator_parse_primary(parser);

    while (parser->error == CALCULATOR_OK)
    {
        if (calculator_match_char(parser, '!') != 0U)
        {
            double factor;
            uint32_t index;

            if ((value.number < 0.0) || (value.number > 170.0) ||
                (floor(value.number) != value.number))
            {
                calculator_set_error(parser, CALCULATOR_DOMAIN_ERROR);
                return calculator_value(0.0);
            }
            factor = 1.0;
            for (index = 2U; index <= (uint32_t)value.number; index++)
            {
                factor *= (double)index;
            }
            value = calculator_checked_value(parser, factor);
        }
        else if (calculator_match_char(parser, '%') != 0U)
        {
            value.number /= 100.0;
            value.percent = 1U;
        }
        else
        {
            break;
        }
    }
    return value;
}

static calculator_value_t calculator_nth_root(calculator_parser_t *parser,
                                               double radicand,
                                               double degree)
{
    double integer_degree;
    double result;

    if (fabs(degree) < CALCULATOR_ZERO_EPSILON)
    {
        calculator_set_error(parser, CALCULATOR_DIVIDE_BY_ZERO);
        return calculator_value(0.0);
    }
    if (radicand < 0.0)
    {
        integer_degree = floor(fabs(degree) + 0.5);
        if ((fabs(fabs(degree) - integer_degree) > 1.0e-12) ||
            (fmod(integer_degree, 2.0) == 0.0))
        {
            calculator_set_error(parser, CALCULATOR_DOMAIN_ERROR);
            return calculator_value(0.0);
        }
        result = -pow(-radicand, 1.0 / fabs(degree));
        if (degree < 0.0)
        {
            result = 1.0 / result;
        }
    }
    else
    {
        result = pow(radicand, 1.0 / degree);
    }
    return calculator_checked_value(parser, result);
}

static calculator_value_t calculator_parse_power(calculator_parser_t *parser)
{
    calculator_value_t left = calculator_parse_postfix(parser);
    calculator_value_t right;

    if (calculator_match_char(parser, '^') != 0U)
    {
        right = calculator_parse_unary(parser);
        return calculator_checked_value(parser,
                                        pow(left.number, right.number));
    }
    if (calculator_match_word(parser, "root") != 0U)
    {
        right = calculator_parse_unary(parser);
        return calculator_nth_root(parser, left.number, right.number);
    }
    return left;
}

static calculator_value_t calculator_parse_unary(calculator_parser_t *parser)
{
    calculator_value_t value;

    if (calculator_match_char(parser, '+') != 0U)
    {
        return calculator_parse_unary(parser);
    }
    if (calculator_match_char(parser, '-') != 0U)
    {
        value = calculator_parse_unary(parser);
        value.number = -value.number;
        return value;
    }
    return calculator_parse_power(parser);
}

static calculator_value_t calculator_parse_term(calculator_parser_t *parser)
{
    calculator_value_t left = calculator_parse_unary(parser);

    while (parser->error == CALCULATOR_OK)
    {
        calculator_value_t right;

        if (calculator_match_char(parser, '*') != 0U)
        {
            right = calculator_parse_unary(parser);
            left = calculator_checked_value(parser,
                                             left.number * right.number);
        }
        else if (calculator_match_char(parser, '/') != 0U)
        {
            right = calculator_parse_unary(parser);
            if (fabs(right.number) < CALCULATOR_ZERO_EPSILON)
            {
                calculator_set_error(parser, CALCULATOR_DIVIDE_BY_ZERO);
                return calculator_value(0.0);
            }
            left = calculator_checked_value(parser,
                                             left.number / right.number);
        }
        else
        {
            break;
        }
        left.percent = 0U;
    }
    return left;
}

static calculator_value_t calculator_parse_expression(
    calculator_parser_t *parser)
{
    calculator_value_t left = calculator_parse_term(parser);

    while (parser->error == CALCULATOR_OK)
    {
        calculator_value_t right;
        double operand;

        if (calculator_match_char(parser, '+') != 0U)
        {
            right = calculator_parse_term(parser);
            operand = (right.percent != 0U) ?
                left.number * right.number : right.number;
            left = calculator_checked_value(parser, left.number + operand);
        }
        else if (calculator_match_char(parser, '-') != 0U)
        {
            right = calculator_parse_term(parser);
            operand = (right.percent != 0U) ?
                left.number * right.number : right.number;
            left = calculator_checked_value(parser, left.number - operand);
        }
        else
        {
            break;
        }
        left.percent = 0U;
    }
    return left;
}

int calculator_evaluate(const char *expression,
                        calculator_angle_mode_t angle_mode,
                        double *value)
{
    calculator_parser_t parser;
    calculator_value_t result;

    if ((expression == NULL) || (value == NULL) ||
        ((angle_mode != CALCULATOR_ANGLE_DEG) &&
         (angle_mode != CALCULATOR_ANGLE_RAD)))
    {
        return CALCULATOR_INVALID_ARG;
    }
    if (strlen(expression) >= CALCULATOR_EXPRESSION_CAPACITY)
    {
        return CALCULATOR_INVALID_ARG;
    }
    parser.cursor = expression;
    parser.angle_mode = angle_mode;
    parser.error = CALCULATOR_OK;
    calculator_skip_spaces(&parser);
    if (*parser.cursor == '\0')
    {
        return CALCULATOR_SYNTAX_ERROR;
    }
    result = calculator_parse_expression(&parser);
    calculator_skip_spaces(&parser);
    if ((parser.error == CALCULATOR_OK) && (*parser.cursor != '\0'))
    {
        parser.error = CALCULATOR_SYNTAX_ERROR;
    }
    if (parser.error != CALCULATOR_OK)
    {
        return parser.error;
    }
    *value = result.number;
    return CALCULATOR_OK;
}
