#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "calculator_engine.h"

static int expect_result(const char *name, const char *expression,
                         calculator_angle_mode_t angle_mode,
                         int expected_result, double expected_value,
                         double tolerance)
{
    double value = 0.0;
    int result = calculator_evaluate(expression, angle_mode, &value);

    if (result != expected_result)
    {
        printf("FAIL %s result=%d expected=%d\n", name, result,
               expected_result);
        return 1;
    }
    if ((result == CALCULATOR_OK) &&
        (fabs(value - expected_value) > tolerance))
    {
        printf("FAIL %s value=%.12f expected=%.12f\n", name, value,
               expected_value);
        return 1;
    }
    return 0;
}

static int test_arithmetic_and_precedence(void)
{
    int failures = 0;

    failures += expect_result("precedence", "2+3*4",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, 14.0, 1e-12);
    failures += expect_result("parentheses", "(2+3)*4",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, 20.0, 1e-12);
    failures += expect_result("right_associative_power", "2^3^2",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, 512.0, 1e-12);
    failures += expect_result("negative_power", "2^-3",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, 0.125, 1e-12);
    failures += expect_result("unary_precedence", "-2^2",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, -4.0, 1e-12);
    return failures;
}

static int test_scientific_functions(void)
{
    int failures = 0;

    failures += expect_result("square_root", "sqrt(81)",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, 9.0, 1e-12);
    failures += expect_result("cube_root", "27 root 3",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, 3.0, 1e-12);
    failures += expect_result("factorial", "5!",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, 120.0, 1e-12);
    failures += expect_result("logs", "lg(1000)+ln(e)",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, 4.0, 1e-10);
    failures += expect_result("exponential", "exp(1)",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, 2.718281828459045,
        1e-10);
    failures += expect_result("hyperbolic", "sinh(0)+cosh(0)",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, 1.0, 1e-12);
    failures += expect_result("inverse_hyperbolic", "asinh(0)",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, 0.0, 1e-12);
    return failures;
}

static int test_angle_modes(void)
{
    int failures = 0;

    failures += expect_result("degree_sine", "sin(30)",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, 0.5, 1e-10);
    failures += expect_result("radian_sine", "sin(pi/2)",
        CALCULATOR_ANGLE_RAD, CALCULATOR_OK, 1.0, 1e-10);
    failures += expect_result("degree_inverse_sine", "asin(0.5)",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, 30.0, 1e-9);
    failures += expect_result("radian_inverse_sine", "asin(1)",
        CALCULATOR_ANGLE_RAD, CALCULATOR_OK, 1.5707963267948966,
        1e-10);
    return failures;
}

static int test_phone_percentage(void)
{
    int failures = 0;

    failures += expect_result("standalone_percent", "10%",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, 0.1, 1e-12);
    failures += expect_result("addition_percent", "200+10%",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, 220.0, 1e-12);
    failures += expect_result("subtraction_percent", "200-10%",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, 180.0, 1e-12);
    failures += expect_result("multiplication_percent", "200*10%",
        CALCULATOR_ANGLE_DEG, CALCULATOR_OK, 20.0, 1e-12);
    return failures;
}

static int test_errors(void)
{
    int failures = 0;

    failures += expect_result("divide_by_zero", "1/0",
        CALCULATOR_ANGLE_DEG, CALCULATOR_DIVIDE_BY_ZERO, 0.0, 0.0);
    failures += expect_result("sqrt_domain", "sqrt(-1)",
        CALCULATOR_ANGLE_DEG, CALCULATOR_DOMAIN_ERROR, 0.0, 0.0);
    failures += expect_result("factorial_domain", "1.5!",
        CALCULATOR_ANGLE_DEG, CALCULATOR_DOMAIN_ERROR, 0.0, 0.0);
    failures += expect_result("syntax", "2+",
        CALCULATOR_ANGLE_DEG, CALCULATOR_SYNTAX_ERROR, 0.0, 0.0);
    failures += expect_result("unknown_function", "foo(1)",
        CALCULATOR_ANGLE_DEG, CALCULATOR_SYNTAX_ERROR, 0.0, 0.0);
    failures += expect_result("empty", "",
        CALCULATOR_ANGLE_DEG, CALCULATOR_SYNTAX_ERROR, 0.0, 0.0);
    return failures;
}

int main(void)
{
    int failures = 0;

    failures += test_arithmetic_and_precedence();
    failures += test_scientific_functions();
    failures += test_angle_modes();
    failures += test_phone_percentage();
    failures += test_errors();
    if (failures != 0)
    {
        printf("FAIL calculator_engine failures=%d\n", failures);
        return 1;
    }
    printf("PASS calculator_engine\n");
    return 0;
}
