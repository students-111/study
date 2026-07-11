/**
 * @file test_motor_pwm.c
 * @brief 双轮电机 PWM 测试实现。
 */

/* ======== include ======== */

#include "test_motor_pwm.h"

#include <stdint.h>

#include "dal_motor.h"

/* ======== 内部变量 ======== */

static int16_t g_test_motor_pwm_output_permille;

/* ======== 公开 API ======== */

void test_motor_pwm_init(void)
{
    g_test_motor_pwm_output_permille =
        (int16_t)TEST_MOTOR_PWM_START_PERMILLE;
    dal_motor_init();
    (void)dal_motor_set_output(DAL_MOTOR_LEFT,
        g_test_motor_pwm_output_permille);
    (void)dal_motor_set_output(DAL_MOTOR_RIGHT,
        g_test_motor_pwm_output_permille);
}

void test_motor_pwm_refresh(void)
{
    if (g_test_motor_pwm_output_permille <
        (int16_t)TEST_MOTOR_PWM_MAX_PERMILLE) {
        g_test_motor_pwm_output_permille +=
            (int16_t)TEST_MOTOR_PWM_STEP_PERMILLE;

        if (g_test_motor_pwm_output_permille >
            (int16_t)TEST_MOTOR_PWM_MAX_PERMILLE) {
            g_test_motor_pwm_output_permille =
                (int16_t)TEST_MOTOR_PWM_MAX_PERMILLE;
        }
    }

    (void)dal_motor_set_output(DAL_MOTOR_LEFT,
        g_test_motor_pwm_output_permille);
    (void)dal_motor_set_output(DAL_MOTOR_RIGHT,
        g_test_motor_pwm_output_permille);
}
