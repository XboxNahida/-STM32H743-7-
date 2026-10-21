#include <math.h>
#include <stdio.h>
#include <string.h>

#include "calculator_controller.h"

static int expect_i32(const char *name, int actual, int expected)
{
    if (actual != expected)
    {
        printf("FAIL %s actual=%d expected=%d\n", name, actual, expected);
        return 1;
    }
    return 0;
}

static int expect_text(const char *name, const char *actual,
                       const char *expected)
{
    if ((actual == NULL) || (strcmp(actual, expected) != 0))
    {
        printf("FAIL %s actual=%s expected=%s\n", name,
               (actual != NULL) ? actual : "<null>", expected);
        return 1;
    }
    return 0;
}

static int press(calculator_controller_t *controller, const char *button)
{
    return calculator_controller_press(controller, button);
}

static int test_expression_and_result_display(void)
{
    calculator_controller_t controller;
    int failures = 0;

    calculator_controller_init(&controller);
    failures += expect_text("initial_expression", controller.expression,
                            "0");
    failures += expect_text("initial_result", controller.result_text, "");
    failures += expect_i32("press_2", press(&controller, "2"),
                           CALCULATOR_OK);
    failures += expect_i32("press_plus", press(&controller, "+"),
                           CALCULATOR_OK);
    failures += expect_i32("press_3", press(&controller, "3"),
                           CALCULATOR_OK);
    failures += expect_i32("press_multiply", press(&controller, "*"),
                           CALCULATOR_OK);
    failures += expect_i32("press_4", press(&controller, "4"),
                           CALCULATOR_OK);
    failures += expect_text("readable_expression", controller.expression,
                            "2+3*4");
    failures += expect_i32("equals", press(&controller, "="),
                           CALCULATOR_OK);
    failures += expect_text("result", controller.result_text, "= 14");
    failures += expect_i32("operator_continues_result",
                           press(&controller, "+"), CALCULATOR_OK);
    failures += expect_text("continued_expression", controller.expression,
                            "14+");
    failures += expect_i32("continued_digit", press(&controller, "7"),
                           CALCULATOR_OK);
    failures += expect_text("continued_with_digit", controller.expression,
                            "14+7");
    (void)press(&controller, "=");
    failures += expect_i32("digit_after_equals_resets",
                           press(&controller, "8"), CALCULATOR_OK);
    failures += expect_text("reset_expression", controller.expression, "8");
    return failures;
}

static int test_unary_and_power_display(void)
{
    calculator_controller_t controller;
    int failures = 0;

    calculator_controller_init(&controller);
    (void)press(&controller, "9");
    failures += expect_i32("sqrt_button", press(&controller, "sqr(X;2)"),
                           CALCULATOR_OK);
    failures += expect_text("sqrt_display", controller.expression,
                            "sqrt(9)");
    (void)press(&controller, "=");
    failures += expect_text("sqrt_result", controller.result_text, "= 3");

    (void)press(&controller, "AC");
    (void)press(&controller, "8");
    (void)press(&controller, "sqr(X;Y)");
    (void)press(&controller, "3");
    failures += expect_text("root_display", controller.expression,
                            "8 root 3");
    (void)press(&controller, "=");
    failures += expect_text("root_result", controller.result_text, "= 2");

    (void)press(&controller, "AC");
    (void)press(&controller, "5");
    (void)press(&controller, "X!");
    failures += expect_text("factorial_display", controller.expression,
                            "5!");
    (void)press(&controller, "=");
    failures += expect_text("factorial_result", controller.result_text,
                            "= 120");

    (void)press(&controller, "AC");
    (void)press(&controller, "4");
    (void)press(&controller, "1/X");
    failures += expect_text("reciprocal_display", controller.expression,
                            "1/(4)");
    (void)press(&controller, "X^2");
    failures += expect_text("square_display", controller.expression,
                            "1/(4)^2");
    return failures;
}

static int test_functions_constants_and_modes(void)
{
    calculator_controller_t controller;
    int failures = 0;

    calculator_controller_init(&controller);
    (void)press(&controller, "sin");
    (void)press(&controller, "3");
    (void)press(&controller, "0");
    (void)press(&controller, ")");
    failures += expect_text("sin_display", controller.expression, "sin(30)");
    (void)press(&controller, "=");
    failures += expect_text("sin_result", controller.result_text, "= 0.5");

    failures += expect_i32("second_enable", press(&controller, "2^nd"),
                           CALCULATOR_OK);
    failures += expect_i32("second_state", controller.second_mode, 1);
    (void)press(&controller, "AC");
    (void)press(&controller, "asin");
    (void)press(&controller, "0");
    (void)press(&controller, ".");
    (void)press(&controller, "5");
    (void)press(&controller, ")");
    (void)press(&controller, "=");
    failures += expect_text("inverse_result", controller.result_text,
                            "= 30");

    failures += expect_i32("rad_enable", press(&controller, "Rad"),
                           CALCULATOR_OK);
    failures += expect_i32("rad_state", controller.angle_mode,
                           CALCULATOR_ANGLE_RAD);
    (void)press(&controller, "AC");
    (void)press(&controller, "sin");
    (void)press(&controller, "Pi");
    (void)press(&controller, "/");
    (void)press(&controller, "2");
    (void)press(&controller, ")");
    (void)press(&controller, "=");
    failures += expect_text("rad_result", controller.result_text, "= 1");

    (void)press(&controller, "AC");
    (void)press(&controller, "10^x");
    (void)press(&controller, "2");
    (void)press(&controller, ")");
    failures += expect_text("ten_power_display", controller.expression,
                            "10^(2)");
    (void)press(&controller, "=");
    failures += expect_text("ten_power_result", controller.result_text,
                            "= 100");
    (void)press(&controller, "AC");
    (void)press(&controller, "3");
    (void)press(&controller, "10^x");
    failures += expect_text("ten_power_wraps_operand",
                            controller.expression, "10^(3)");
    (void)press(&controller, "AC");
    (void)press(&controller, "4");
    (void)press(&controller, "e^x");
    failures += expect_text("e_power_wraps_operand",
                            controller.expression, "e^(4)");
    return failures;
}

static int test_editing_percentage_and_memory(void)
{
    calculator_controller_t controller;
    int failures = 0;

    calculator_controller_init(&controller);
    (void)press(&controller, "2");
    (void)press(&controller, "0");
    (void)press(&controller, "0");
    (void)press(&controller, "+");
    (void)press(&controller, "1");
    (void)press(&controller, "0");
    (void)press(&controller, "%");
    failures += expect_text("percent_display", controller.expression,
                            "200+10%");
    (void)press(&controller, "=");
    failures += expect_text("percent_result", controller.result_text,
                            "= 220");
    failures += expect_i32("memory_add", press(&controller, "m+"),
                           CALCULATOR_OK);
    failures += expect_i32("memory_present", controller.memory_valid, 1);
    (void)press(&controller, "AC");
    failures += expect_i32("memory_recall", press(&controller, "mr"),
                           CALCULATOR_OK);
    failures += expect_text("memory_expression", controller.expression,
                            "220");
    failures += expect_i32("memory_subtract", press(&controller, "m-"),
                           CALCULATOR_OK);
    failures += expect_i32("memory_clear", press(&controller, "mc"),
                           CALCULATOR_OK);
    failures += expect_i32("memory_absent", controller.memory_valid, 0);

    (void)press(&controller, "AC");
    (void)press(&controller, "1");
    (void)press(&controller, "2");
    (void)press(&controller, "3");
    (void)press(&controller, "<-");
    failures += expect_text("backspace", controller.expression, "12");
    (void)press(&controller, "+/-");
    failures += expect_text("sign", controller.expression, "-(12)");
    (void)press(&controller, "+/-");
    failures += expect_text("sign_toggles_back", controller.expression,
                            "12");
    return failures;
}

static int test_remaining_named_buttons(void)
{
    static const char *const functions[] = {
        "cos", "tan", "In", "Lg", "sinh", "cosh", "tanh",
        "acos", "atan", "asinh", "acosh", "atanh"
    };
    calculator_controller_t controller;
    uint32_t index;
    int failures = 0;

    for (index = 0U; index < sizeof(functions) / sizeof(functions[0]); index++)
    {
        calculator_controller_init(&controller);
        failures += expect_i32(functions[index],
            press(&controller, functions[index]), CALCULATOR_OK);
        failures += expect_i32("function_opens_parenthesis",
            controller.expression[strlen(controller.expression) - 1U], '(');
    }

    calculator_controller_init(&controller);
    (void)press(&controller, "1");
    (void)press(&controller, "EE");
    (void)press(&controller, "3");
    (void)press(&controller, "=");
    failures += expect_text("scientific_notation", controller.result_text,
                            "= 1000");
    (void)press(&controller, "AC");
    failures += expect_i32("random", press(&controller, "Rand"),
                           CALCULATOR_OK);
    failures += expect_i32("random_range",
        (controller.expression[0] == '0') &&
        (strchr(controller.expression, '.') != NULL), 1);
    (void)press(&controller, "AC");
    (void)press(&controller, "e^x");
    failures += expect_text("e_power", controller.expression, "e^(");
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_expression_and_result_display();
    failures += test_unary_and_power_display();
    failures += test_functions_constants_and_modes();
    failures += test_editing_percentage_and_memory();
    failures += test_remaining_named_buttons();
    if (failures != 0)
    {
        printf("FAIL calculator_controller failures=%d\n", failures);
        return 1;
    }
    printf("PASS calculator_controller\n");
    return 0;
}
