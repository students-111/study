/**
 * @file drv_motor.c
 * @brief 简单有刷电机驱动库实现。
 */

/* ======== include ======== */

#include "drv_motor.h"

#include <stddef.h>

/* ======== 可调参数宏定义 ======== */

/* ======== 内部函数 ======== */

/**
 * @brief 限制速度命令范围。
 * @param speed 输入速度命令。
 * @return 限幅后的速度命令。
 */
static int8_t Motor_LimitSpeed(int8_t speed)
{
    if (speed > MOTOR_SPEED_MAX) {
        return MOTOR_SPEED_MAX;
    }
    if (speed < -MOTOR_SPEED_MAX) {
        return -MOTOR_SPEED_MAX;
    }
    return speed;
}

/**
 * @brief 获取速度命令绝对值。
 * @param speed 速度命令。
 * @return 速度绝对值。
 */
static uint8_t Motor_AbsSpeed(int8_t speed)
{
    return (speed < 0) ? (uint8_t)(-speed) : (uint8_t)speed;
}

/**
 * @brief 将速度百分比换算为 PWM 千分比。
 * @param speed_abs 速度绝对值，范围 0 到 100。
 * @return PWM 千分比。
 */
static uint16_t Motor_SpeedToDuty(uint8_t speed_abs)
{
    uint16_t duty = (uint16_t)speed_abs *
        (uint16_t)MOTOR_SPEED_TO_PERMILLE_SCALE;

    if (duty > MOTOR_PWM_PERMILLE_MAX) {
        duty = MOTOR_PWM_PERMILLE_MAX;
    }
    return duty;
}

/**
 * @brief 根据速度和安装方向判断电机状态。
 * @param speed 速度命令。
 * @param reverse 安装方向，0 表示正装，1 表示反装。
 * @return 电机状态。
 */
static MotorState_t Motor_GetState(int8_t speed, uint8_t reverse)
{
    if (speed == 0) {
        return MOTOR_STATE_STOP;
    }
    if (((speed > 0) && (reverse == 0U)) ||
        ((speed < 0) && (reverse != 0U))) {
        return MOTOR_STATE_FORWARD;
    }
    return MOTOR_STATE_BACKWARD;
}

/**
 * @brief 写入 TB6612 方向脚。
 * @param motor 电机实体。
 * @param state 电机目标状态。
 * @return 写入成功返回 1，否则返回 0。
 */
static uint8_t Motor_WriteDirection(Motor_t *motor, MotorState_t state)
{
    bool pin1_high = false;
    bool pin2_high = false;

    if (motor == NULL) {
        return 0U;
    }

    if (state == MOTOR_STATE_FORWARD) {
        pin1_high = true;
    } else if (state == MOTOR_STATE_BACKWARD) {
        pin2_high = true;
    }

    if (bsp_gpio_write(motor->hw.dir_pin1, pin1_high) != BSP_OK) {
        return 0U;
    }
    if (bsp_gpio_write(motor->hw.dir_pin2, pin2_high) != BSP_OK) {
        return 0U;
    }

    return 1U;
}

/* ======== 公开 API ======== */

uint8_t Motor_Init(Motor_t *motor, const MotorHW_t *hw, uint8_t reverse)
{
    if ((motor == NULL) || (hw == NULL)) {
        return 0U;
    }

    motor->hw = *hw;
    motor->speed = 0;
    motor->state = MOTOR_STATE_STOP;
    motor->enable = 1U;
    motor->reverse = (reverse != 0U) ? 1U : 0U;

    if (!Motor_WriteDirection(motor, MOTOR_STATE_STOP)) {
        motor->state = MOTOR_STATE_ERROR;
        return 0U;
    }
    if (bsp_pwm_set_duty(motor->hw.channel, 0U) != BSP_OK) {
        motor->state = MOTOR_STATE_ERROR;
        return 0U;
    }

    return 1U;
}

uint8_t Motor_Enable(Motor_t *motor, uint8_t enable)
{
    if (motor == NULL) {
        return 0U;
    }

    motor->enable = (enable != 0U) ? 1U : 0U;
    if (motor->enable == 0U) {
        return Motor_Stop(motor);
    }
    return 1U;
}

uint8_t Motor_SetSpeed(Motor_t *motor, int8_t speed)
{
    int8_t limited_speed;
    uint16_t duty_permille;
    MotorState_t next_state;

    if (motor == NULL) {
        return 0U;
    }
    if (motor->enable == 0U) {
        return Motor_Stop(motor);
    }

    limited_speed = Motor_LimitSpeed(speed);
    next_state = Motor_GetState(limited_speed, motor->reverse);
    duty_permille = Motor_SpeedToDuty(Motor_AbsSpeed(limited_speed));

    if (!Motor_WriteDirection(motor, next_state)) {
        motor->state = MOTOR_STATE_ERROR;
        return 0U;
    }
    if (bsp_pwm_set_duty(motor->hw.channel, duty_permille) != BSP_OK) {
        motor->state = MOTOR_STATE_ERROR;
        return 0U;
    }

    motor->speed = limited_speed;
    motor->state = next_state;
    return 1U;
}

uint8_t Motor_Stop(Motor_t *motor)
{
    if (motor == NULL) {
        return 0U;
    }

    if (!Motor_WriteDirection(motor, MOTOR_STATE_STOP)) {
        motor->state = MOTOR_STATE_ERROR;
        return 0U;
    }
    if (bsp_pwm_set_duty(motor->hw.channel, 0U) != BSP_OK) {
        motor->state = MOTOR_STATE_ERROR;
        return 0U;
    }

    motor->speed = 0;
    motor->state = MOTOR_STATE_STOP;
    return 1U;
}
