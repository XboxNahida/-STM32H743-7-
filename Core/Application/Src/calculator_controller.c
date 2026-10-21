#include "calculator_controller.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static uint8_t calculator_is_digit_button(const char *button)
{
    return (button[0] >= '0') && (button[0] <= '9') &&
           (button[1] == '\0');
}

static void calculator_format_value(double value, char *buffer,
                                    size_t capacity)
{
    size_t index;

    if (fabs(value) < 1.0e-12)
    {
        value = 0.0;
    }
    (void)snprintf(buffer, capacity, "%.12g", value);
    buffer[capacity - 1U] = '\0';
    for (index = 0U; buffer[index] != '\0'; index++)
    {
        if (buffer[index] == 'e')
        {
            buffer[index] = 'E';
        }
    }
}

static void calculator_reset_expression(calculator_controller_t *controller)
{
    (void)strcpy(controller->expression, "0");
    controller->result_text[0] = '\0';
    controller->just_evaluated = 0U;
    controller->last_result = CALCULATOR_OK;
}

static uint8_t calculator_append(calculator_controller_t *controller,
                                 const char *text)
{
    size_t current_length = strlen(controller->expression);
    size_t added_length = strlen(text);

    if ((current_length + added_length) >=
        CALCULATOR_EXPRESSION_CAPACITY)
    {
        controller->last_result = CALCULATOR_INVALID_ARG;
        return 0U;
    }
    (void)memcpy(controller->expression + current_length, text,
                 added_length + 1U);
    return 1U;
}

static uint8_t calculator_expression_is_zero(
    const calculator_controller_t *controller)
{
    return strcmp(controller->expression, "0") == 0;
}

static uint8_t calculator_last_is_operand(
    const calculator_controller_t *controller)
{
    size_t length = strlen(controller->expression);
    char last;

    if (length == 0U)
    {
        return 0U;
    }
    last = controller->expression[length - 1U];
    return ((last >= '0') && (last <= '9')) || (last == ')') ||
           (last == '!') || (last == '%') ||
           (last == 'e') || (last == 'i');
}

static size_t calculator_find_last_operand(const char *expression)
{
    size_t end = strlen(expression);
    size_t index = end;
    uint32_t depth = 0U;

    if (end == 0U)
    {
        return end;
    }
    while ((index > 0U) &&
           ((expression[index - 1U] == '!') ||
            (expression[index - 1U] == '%')))
    {
        index--;
    }
    if ((index > 0U) && (expression[index - 1U] == ')'))
    {
        size_t close = index;

        while (index > 0U)
        {
            index--;
            if (expression[index] == ')')
            {
                depth++;
            }
            else if (expression[index] == '(')
            {
                if (--depth == 0U)
                {
                    break;
                }
            }
        }
        if (depth != 0U)
        {
            return close;
        }
        while ((index > 0U) &&
               (((expression[index - 1U] >= 'a') &&
                 (expression[index - 1U] <= 'z')) ||
                ((expression[index - 1U] >= 'A') &&
                 (expression[index - 1U] <= 'Z'))))
        {
            index--;
        }
        return index;
    }
    index = end;
    while (index > 0U)
    {
        char character = expression[index - 1U];

        if (((character >= '0') && (character <= '9')) ||
            (character == '.') || (character == 'E') ||
            (character == 'e'))
        {
            index--;
            continue;
        }
        if (((character == '+') || (character == '-')) &&
            (index >= 2U) &&
            ((expression[index - 2U] == 'E') ||
             (expression[index - 2U] == 'e')))
        {
            index--;
            continue;
        }
        break;
    }
    if (index == end)
    {
        if ((end >= 2U) &&
            (strcmp(expression + end - 2U, "pi") == 0))
        {
            return end - 2U;
        }
        if (expression[end - 1U] == 'e')
        {
            return end - 1U;
        }
    }
    return index;
}

static uint8_t calculator_wrap_last_operand(
    calculator_controller_t *controller, const char *prefix,
    const char *suffix)
{
    char replacement[CALCULATOR_EXPRESSION_CAPACITY];
    size_t start;
    int written;

    if (calculator_expression_is_zero(controller) != 0U)
    {
        controller->expression[0] = '\0';
        return calculator_append(controller, prefix);
    }
    if (calculator_last_is_operand(controller) == 0U)
    {
        return calculator_append(controller, prefix);
    }
    start = calculator_find_last_operand(controller->expression);
    if (start >= strlen(controller->expression))
    {
        return 0U;
    }
    written = snprintf(replacement, sizeof(replacement), "%.*s%s%s%s",
                       (int)start, controller->expression, prefix,
                       controller->expression + start, suffix);
    if ((written < 0) || ((size_t)written >= sizeof(replacement)))
    {
        controller->last_result = CALCULATOR_INVALID_ARG;
        return 0U;
    }
    (void)strcpy(controller->expression, replacement);
    return 1U;
}

static uint8_t calculator_toggle_last_operand_sign(
    calculator_controller_t *controller)
{
    char replacement[CALCULATOR_EXPRESSION_CAPACITY];
    size_t length = strlen(controller->expression);
    size_t start;
    size_t prefix_length;
    size_t inner_length;

    if ((length >= 4U) && (controller->expression[length - 1U] == ')'))
    {
        start = calculator_find_last_operand(controller->expression);
        if ((start > 0U) && (controller->expression[start - 1U] == '-') &&
            (controller->expression[start] == '('))
        {
            prefix_length = start - 1U;
            inner_length = length - start - 2U;
            (void)memcpy(replacement, controller->expression,
                         prefix_length);
            (void)memcpy(replacement + prefix_length,
                         controller->expression + start + 1U,
                         inner_length);
            replacement[prefix_length + inner_length] = '\0';
            (void)strcpy(controller->expression, replacement);
            return 1U;
        }
    }
    return calculator_wrap_last_operand(controller, "-(", ")");
}

static int calculator_evaluate_current(calculator_controller_t *controller,
                                       double *value)
{
    int result = calculator_evaluate(controller->expression,
                                     controller->angle_mode, value);

    controller->last_result = result;
    if (result == CALCULATOR_OK)
    {
        char number[CALCULATOR_RESULT_CAPACITY - 2U];

        calculator_format_value(*value, number, sizeof(number));
        (void)snprintf(controller->result_text,
                       sizeof(controller->result_text), "= %s", number);
        controller->last_value = *value;
        controller->last_value_valid = 1U;
    }
    else if (result == CALCULATOR_DIVIDE_BY_ZERO)
    {
        (void)strcpy(controller->result_text, "= Divide by zero");
    }
    else if (result == CALCULATOR_DOMAIN_ERROR)
    {
        (void)strcpy(controller->result_text, "= Undefined");
    }
    else if (result == CALCULATOR_OVERFLOW)
    {
        (void)strcpy(controller->result_text, "= Overflow");
    }
    else
    {
        (void)strcpy(controller->result_text, "= Error");
    }
    return result;
}

static uint8_t calculator_append_value(calculator_controller_t *controller,
                                       const char *text)
{
    if (calculator_expression_is_zero(controller) != 0U)
    {
        controller->expression[0] = '\0';
    }
    else if (calculator_last_is_operand(controller) != 0U)
    {
        if (calculator_append(controller, "*") == 0U)
        {
            return 0U;
        }
    }
    return calculator_append(controller, text);
}

static void calculator_prepare_after_result(
    calculator_controller_t *controller, uint8_t continue_result)
{
    if (controller->just_evaluated == 0U)
    {
        return;
    }
    if ((continue_result != 0U) && (controller->last_value_valid != 0U))
    {
        calculator_format_value(controller->last_value,
                                controller->expression,
                                sizeof(controller->expression));
        controller->result_text[0] = '\0';
        controller->just_evaluated = 0U;
    }
    else
    {
        calculator_reset_expression(controller);
    }
}

static int calculator_press_digit(calculator_controller_t *controller,
                                  const char *button)
{
    calculator_prepare_after_result(controller, 0U);
    if (calculator_expression_is_zero(controller) != 0U)
    {
        controller->expression[0] = button[0];
        controller->expression[1] = '\0';
    }
    else
    {
        (void)calculator_append(controller, button);
    }
    return controller->last_result;
}

static int calculator_press_decimal(calculator_controller_t *controller)
{
    const char *cursor;
    const char *start;

    calculator_prepare_after_result(controller, 0U);
    start = controller->expression;
    cursor = controller->expression + strlen(controller->expression);
    while (cursor > start)
    {
        char character = cursor[-1];

        if ((character == '.') || (character == 'E') || (character == 'e'))
        {
            return CALCULATOR_OK;
        }
        if ((character == '+') || (character == '-') ||
            (character == '*') || (character == '/') ||
            (character == '^') || (character == '(') ||
            (character == ' '))
        {
            break;
        }
        cursor--;
    }
    if ((cursor == controller->expression) &&
        (calculator_expression_is_zero(controller) != 0U))
    {
        return calculator_append(controller, ".") != 0U ?
            CALCULATOR_OK : CALCULATOR_INVALID_ARG;
    }
    if ((cursor == controller->expression + strlen(controller->expression)) ||
        (calculator_last_is_operand(controller) == 0U))
    {
        if (calculator_append(controller, "0") == 0U)
        {
            return CALCULATOR_INVALID_ARG;
        }
    }
    return calculator_append(controller, ".") != 0U ?
        CALCULATOR_OK : CALCULATOR_INVALID_ARG;
}

static int calculator_press_operator(calculator_controller_t *controller,
                                     const char *operator_text)
{
    size_t length;

    calculator_prepare_after_result(controller, 1U);
    length = strlen(controller->expression);
    if ((strcmp(operator_text, "-") == 0) &&
        (calculator_expression_is_zero(controller) != 0U))
    {
        (void)strcpy(controller->expression, "-");
        return CALCULATOR_OK;
    }
    if (calculator_last_is_operand(controller) == 0U)
    {
        while ((length > 0U) && (controller->expression[length - 1U] == ' '))
        {
            controller->expression[--length] = '\0';
        }
        if ((length > 0U) &&
            (strchr("+-*/^", controller->expression[length - 1U]) != NULL))
        {
            controller->expression[length - 1U] = operator_text[0];
        }
        return CALCULATOR_OK;
    }
    return calculator_append(controller, operator_text) != 0U ?
        CALCULATOR_OK : CALCULATOR_INVALID_ARG;
}

static int calculator_press_function(calculator_controller_t *controller,
                                     const char *function_name)
{
    char prefix[10];
    int written;

    calculator_prepare_after_result(controller, 1U);
    written = snprintf(prefix, sizeof(prefix), "%s(", function_name);
    if ((written < 0) || ((size_t)written >= sizeof(prefix)))
    {
        return CALCULATOR_INVALID_ARG;
    }
    return calculator_wrap_last_operand(controller, prefix, ")") != 0U ?
        CALCULATOR_OK : CALCULATOR_INVALID_ARG;
}

static int calculator_press_parenthesis(calculator_controller_t *controller,
                                        char parenthesis)
{
    size_t index;
    uint32_t open_count = 0U;
    uint32_t close_count = 0U;

    calculator_prepare_after_result(controller, 0U);
    if (parenthesis == '(')
    {
        if (calculator_expression_is_zero(controller) != 0U)
        {
            (void)strcpy(controller->expression, "(");
        }
        else
        {
            if ((calculator_last_is_operand(controller) != 0U) &&
                (calculator_append(controller, "*") == 0U))
            {
                return CALCULATOR_INVALID_ARG;
            }
            if (calculator_append(controller, "(") == 0U)
            {
                return CALCULATOR_INVALID_ARG;
            }
        }
        return CALCULATOR_OK;
    }
    for (index = 0U; controller->expression[index] != '\0'; index++)
    {
        if (controller->expression[index] == '(') open_count++;
        if (controller->expression[index] == ')') close_count++;
    }
    if ((open_count > close_count) &&
        (calculator_last_is_operand(controller) != 0U))
    {
        (void)calculator_append(controller, ")");
    }
    return CALCULATOR_OK;
}

static int calculator_press_memory(calculator_controller_t *controller,
                                   const char *button)
{
    double value;
    int result;

    if (strcmp(button, "mc") == 0)
    {
        controller->memory = 0.0;
        controller->memory_valid = 0U;
        return CALCULATOR_OK;
    }
    if (strcmp(button, "mr") == 0)
    {
        char text[32];

        calculator_prepare_after_result(controller, 0U);
        calculator_format_value((controller->memory_valid != 0U) ?
            controller->memory : 0.0, text, sizeof(text));
        return calculator_append_value(controller, text) != 0U ?
            CALCULATOR_OK : CALCULATOR_INVALID_ARG;
    }
    if ((controller->just_evaluated != 0U) &&
        (controller->last_value_valid != 0U))
    {
        value = controller->last_value;
        result = CALCULATOR_OK;
    }
    else
    {
        result = calculator_evaluate(controller->expression,
                                     controller->angle_mode, &value);
    }
    if (result != CALCULATOR_OK)
    {
        controller->last_result = result;
        return result;
    }
    controller->memory += (strcmp(button, "m+") == 0) ? value : -value;
    controller->memory_valid = 1U;
    return CALCULATOR_OK;
}

static int calculator_press_special(calculator_controller_t *controller,
                                    const char *button)
{
    if (strcmp(button, "AC") == 0)
    {
        calculator_reset_expression(controller);
        return CALCULATOR_OK;
    }
    if (strcmp(button, "<-") == 0)
    {
        size_t length;

        calculator_prepare_after_result(controller, 0U);
        length = strlen(controller->expression);
        if (length > 0U)
        {
            controller->expression[--length] = '\0';
        }
        if (length == 0U)
        {
            (void)strcpy(controller->expression, "0");
        }
        return CALCULATOR_OK;
    }
    if (strcmp(button, "+/-") == 0)
    {
        calculator_prepare_after_result(controller, 1U);
        return calculator_toggle_last_operand_sign(controller) != 0U ?
            CALCULATOR_OK : CALCULATOR_INVALID_ARG;
    }
    if (strcmp(button, "2^nd") == 0)
    {
        controller->second_mode = (controller->second_mode == 0U) ? 1U : 0U;
        return CALCULATOR_OK;
    }
    if ((strcmp(button, "Rad") == 0) || (strcmp(button, "Deg") == 0))
    {
        controller->angle_mode =
            (controller->angle_mode == CALCULATOR_ANGLE_DEG) ?
            CALCULATOR_ANGLE_RAD : CALCULATOR_ANGLE_DEG;
        return CALCULATOR_OK;
    }
    return CALCULATOR_INVALID_ARG;
}

void calculator_controller_init(calculator_controller_t *controller)
{
    if (controller == NULL)
    {
        return;
    }
    (void)memset(controller, 0, sizeof(*controller));
    (void)strcpy(controller->expression, "0");
    controller->angle_mode = CALCULATOR_ANGLE_DEG;
    controller->last_result = CALCULATOR_OK;
    controller->random_state = 0x6D2B79F5U;
}

int calculator_controller_press(calculator_controller_t *controller,
                                const char *button)
{
    static const char *const function_buttons[] = {
        "sin", "cos", "tan", "asin", "acos", "atan",
        "sinh", "cosh", "tanh", "asinh", "acosh", "atanh"
    };
    static const char *const function_names[] = {
        "sin", "cos", "tan", "asin", "acos", "atan",
        "sinh", "cosh", "tanh", "asinh", "acosh", "atanh"
    };
    uint32_t index;

    if ((controller == NULL) || (button == NULL))
    {
        return CALCULATOR_INVALID_ARG;
    }
    controller->last_result = CALCULATOR_OK;
    if (calculator_is_digit_button(button) != 0U)
    {
        return calculator_press_digit(controller, button);
    }
    if (strcmp(button, ".") == 0)
    {
        return calculator_press_decimal(controller);
    }
    if ((strcmp(button, "+") == 0) || (strcmp(button, "-") == 0) ||
        (strcmp(button, "*") == 0) || (strcmp(button, "/") == 0) ||
        (strcmp(button, "X^y") == 0))
    {
        return calculator_press_operator(controller,
            (strcmp(button, "X^y") == 0) ? "^" : button);
    }
    if ((strcmp(button, "(") == 0) || (strcmp(button, ")") == 0))
    {
        return calculator_press_parenthesis(controller, button[0]);
    }
    if ((strcmp(button, "mc") == 0) || (strcmp(button, "m+") == 0) ||
        (strcmp(button, "m-") == 0) || (strcmp(button, "mr") == 0))
    {
        return calculator_press_memory(controller, button);
    }
    if ((strcmp(button, "AC") == 0) || (strcmp(button, "<-") == 0) ||
        (strcmp(button, "+/-") == 0) ||
        (strcmp(button, "2^nd") == 0) ||
        (strcmp(button, "Rad") == 0) || (strcmp(button, "Deg") == 0))
    {
        return calculator_press_special(controller, button);
    }
    if (strcmp(button, "=") == 0)
    {
        double value;
        int result = calculator_evaluate_current(controller, &value);

        controller->just_evaluated = (result == CALCULATOR_OK) ? 1U : 0U;
        return result;
    }
    if ((strcmp(button, "X!") == 0) || (strcmp(button, "%") == 0))
    {
        calculator_prepare_after_result(controller, 1U);
        return calculator_append(controller,
            (strcmp(button, "X!") == 0) ? "!" : "%") != 0U ?
            CALCULATOR_OK : CALCULATOR_INVALID_ARG;
    }
    if ((strcmp(button, "X^2") == 0) || (strcmp(button, "X^3") == 0))
    {
        calculator_prepare_after_result(controller, 1U);
        return calculator_append(controller,
            (strcmp(button, "X^2") == 0) ? "^2" : "^3") != 0U ?
            CALCULATOR_OK : CALCULATOR_INVALID_ARG;
    }
    if (strcmp(button, "sqr(X;Y)") == 0)
    {
        calculator_prepare_after_result(controller, 1U);
        return calculator_append(controller, " root ") != 0U ?
            CALCULATOR_OK : CALCULATOR_INVALID_ARG;
    }
    if (strcmp(button, "1/X") == 0)
    {
        calculator_prepare_after_result(controller, 1U);
        return calculator_wrap_last_operand(controller, "1/(", ")") != 0U ?
            CALCULATOR_OK : CALCULATOR_INVALID_ARG;
    }
    if (strcmp(button, "sqr(X;2)") == 0)
    {
        return calculator_press_function(controller, "sqrt");
    }
    if ((strcmp(button, "Lg") == 0) || (strcmp(button, "In") == 0))
    {
        return calculator_press_function(controller,
            (strcmp(button, "Lg") == 0) ? "lg" : "ln");
    }
    for (index = 0U; index < sizeof(function_buttons) /
         sizeof(function_buttons[0]); index++)
    {
        if (strcmp(button, function_buttons[index]) == 0)
        {
            return calculator_press_function(controller,
                                              function_names[index]);
        }
    }
    if ((strcmp(button, "10^x") == 0) || (strcmp(button, "2^x") == 0) ||
        (strcmp(button, "e^x") == 0))
    {
        const char *prefix = (strcmp(button, "2^x") == 0) ? "2^(" :
            ((strcmp(button, "e^x") == 0) ? "e^(" : "10^(");

        calculator_prepare_after_result(controller, 1U);
        return calculator_wrap_last_operand(controller, prefix, ")") != 0U ?
            CALCULATOR_OK : CALCULATOR_INVALID_ARG;
    }
    if (strcmp(button, "Pi") == 0)
    {
        calculator_prepare_after_result(controller, 0U);
        return calculator_append_value(controller, "pi") != 0U ?
            CALCULATOR_OK : CALCULATOR_INVALID_ARG;
    }
    if (strcmp(button, "EE") == 0)
    {
        calculator_prepare_after_result(controller, 0U);
        if ((calculator_last_is_operand(controller) != 0U) &&
            (strchr(calculator_find_last_operand(controller->expression) +
                    controller->expression, 'E') == NULL))
        {
            (void)calculator_append(controller, "E");
        }
        return CALCULATOR_OK;
    }
    if (strcmp(button, "Rand") == 0)
    {
        char text[32];
        double random_value;

        calculator_prepare_after_result(controller, 0U);
        controller->random_state = controller->random_state * 1664525U +
                                   1013904223U;
        random_value = (double)(controller->random_state >> 8) /
                       16777216.0;
        calculator_format_value(random_value, text, sizeof(text));
        return calculator_append_value(controller, text) != 0U ?
            CALCULATOR_OK : CALCULATOR_INVALID_ARG;
    }
    return CALCULATOR_INVALID_ARG;
}
